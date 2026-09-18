/**
 * @file flow_expand.cpp
 * @brief flow CSV -> time-ordered per-packet stream. See flow_expand.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "flow_expand.hpp"

#include <cstdlib>
#include <functional>
#include <queue>
#include <stdexcept>
#include <string>
#include <vector>
#include <iostream>

#include "parser.hpp"

Weighting parseWeighting(const char *arg) {
    std::string s = arg ? arg : "";
    if (s == "packet" || s == "1") return Weighting::Packet;
    if (s == "byte"   || s == "2") return Weighting::Byte;
    return Weighting::Flow;
}

const char *weightingName(Weighting w) {
    switch (w) {
        case Weighting::Packet: return "packet";
        case Weighting::Byte:   return "byte";
        default:                return "flow";
    }
}

namespace {

// one direction of one flow that is currently "live" in the merge heap.
// it carries only the caller's token, not the tuple - the heap moves entries
// around on every push/pop, so the less each one weighs the better.
struct ActiveFlow {
    FlowToken token;
    double  t_start;
    double  t_end;
    int64_t packets_total;                  // how many packets this direction has in total
    int64_t bytes_total;
    int64_t emitted;                        // how many already went out
    double  next_time;                      // time of this flow's next unsent packet

    // we want the earliest packet on top. priority_queue is a max-heap by default,
    // so we flip the comparison: operator> + std::greater => min-heap on next_time.
    bool operator>(const ActiveFlow &o) const { return next_time > o.next_time; }
};

// time of packet #k (0-based) if the flow's packets are spread evenly over [t_start, t_end]
double packetTime(const ActiveFlow &f, int64_t k) {
    if (f.packets_total <= 1) {
        return f.t_start;                   // one packet -> nothing to spread (also avoids /0)
    }
    return f.t_start + (f.t_end - f.t_start) * k / (f.packets_total - 1);
}

// how much this packet adds to its channel's load
int64_t packetWeight(const ActiveFlow &f, Weighting w) {
    if (w == Weighting::Byte) {
        return f.packets_total > 0 ? f.bytes_total / f.packets_total : 0;  // mean packet size
    }
    return 1;                               // Packet and Flow: every packet counts as 1
}

} // namespace

void expandInterleaved(std::istream &in, Symmetry sym, size_t offset,
                       Weighting weighting, const FlowPrepare &prepare,
                       const PacketSink &sink, const FlowRelease &release) {
    std::priority_queue<ActiveFlow, std::vector<ActiveFlow>, std::greater<ActiveFlow>> heap;

    auto emitTop = [&]() {
        ActiveFlow af = heap.top(); heap.pop();            // copy + pop: top() can't be edited in place
        sink(af.token, packetWeight(af, weighting));
        af.emitted++;
        if (af.emitted < af.packets_total) {              // still has packets -> put it back
            af.next_time = packetTime(af, af.emitted);
            heap.push(af);
        } else {
            release(af.token);                            // done: the slot can be reused
        }
    };

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        FlowRow r;
        try {
            r = parseFlowLine(line);
        } catch (const std::exception &e) {
            std::cerr << "Problem with the line: " << e.what() << "\n";
            continue;
        }

        // before we add the new flow, emit everything queued that happens before it starts
        while (!heap.empty() && heap.top().next_time <= r.t_start) emitTop();

        // symmetry and hashing happen once here, not once per packet

        // forward direction (Flow weighting collapses it to a single packet)
        int64_t pf = (weighting == Weighting::Flow) ? 1 : r.packets_fwd;
        if (pf > 0) {
            FlowToken tok = prepare(applySymmetry(sym, r.fwd, offset));
            ActiveFlow a{ tok, r.t_start, r.t_end, pf, r.bytes_fwd, 0, r.t_start };
            heap.push(a);
        }

        // reverse direction, only if it actually carried traffic
        int64_t pr = (weighting == Weighting::Flow)
                        ? (r.packets_rev > 0 ? 1 : 0) : r.packets_rev;
        if (pr > 0) {
            FlowToken tok = prepare(applySymmetry(sym, r.rev, offset));
            ActiveFlow b{ tok, r.t_start, r.t_end, pr, r.bytes_rev, 0, r.t_start };
            heap.push(b);
        }
    }

    // end of file: nothing new will arrive, drain whatever is left
    while (!heap.empty()) emitTop();
}
