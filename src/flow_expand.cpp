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

// Returns a pointer to a string literal, not a function pointer: the `*` belongs
// to the return type (const char*). Literals live in read-only memory for the
// whole program, so returning their address is safe and costs no allocation.
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
// the heap holds one entry per live flow, never one per packet - that's what
// keeps memory flat even though the full dataset expands to billions of packets.
struct ActiveFlow {
    FlowToken token;                        // the caller's ticket for this flow, see flow_expand.hpp
    double  t_start;
    double  t_end;
    int64_t packets_total;                  // how many packets this direction has in total
    int64_t bytes_total;
    int64_t emitted;                        // how many already went out
    double  next_time;                      // time of this flow's next unsent packet (the heap key)

    // we want the earliest packet on top. priority_queue is a max-heap by default,
    // so we flip the comparison: operator> + std::greater => min-heap on next_time.
    bool operator>(const ActiveFlow &o) const { return next_time > o.next_time; }
};

// time of packet #k (0-based) if the flow's packets are spread evenly over [t_start, t_end].
// the first packet lands exactly on t_start and the last exactly on t_end, so N packets
// leave N-1 gaps between them - hence the "- 1". e.g. 5 packets over [0.0, 0.5]:
//   gap = 0.5 / 4 = 0.125  ->  0.000, 0.125, 0.250, 0.375, 0.500
// dividing by N instead would make the flow end early (last packet at 0.4).
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
    // min-heap of live flows, ordered by the time of each flow's NEXT packet.
    // top() is always the flow whose next packet is the earliest of all.
    std::priority_queue<ActiveFlow, std::vector<ActiveFlow>, std::greater<ActiveFlow>> heap;

    // send out the globally earliest packet:
    //   1. take the flow on top of the heap
    //   2. hand its next packet to sink()
    //   3. if it has more packets, compute when the next one comes and put it back -
    //      the heap re-sorts it among the other flows, which is what interleaves them
    //   4. otherwise the flow is finished: release() its token for reuse
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

        // before we add the new flow, emit everything queued that happens before it starts.
        // this is only correct because rows are sorted by start time: no later row can
        // start before r, so every packet up to r.t_start is final and can go out now.
        while (!heap.empty() && heap.top().next_time <= r.t_start) emitTop();

        // symmetry and hashing happen once here, not once per packet:
        // prepare() gets the symmetrised tuple, the heap entry only keeps the token

        // forward direction (Flow weighting collapses it to a single packet).
        // the entry is built first (token filled in below) so prepare() can be told
        // exactly what will be emitted: packet count and per-packet weight
        int64_t pf = (weighting == Weighting::Flow) ? 1 : r.packets_fwd;
        if (pf > 0) {
            ActiveFlow a{ 0, r.t_start, r.t_end, pf, r.bytes_fwd, 0, r.t_start };  // first packet at t_start
            a.token = prepare(applySymmetry(sym, r.fwd, offset),
                              FlowInfo{ a.t_start, a.t_end, a.packets_total, packetWeight(a, weighting) });
            heap.push(a);
        }

        // reverse direction, only if it actually carried traffic
        int64_t pr = (weighting == Weighting::Flow)
                        ? (r.packets_rev > 0 ? 1 : 0) : r.packets_rev;
        if (pr > 0) {
            ActiveFlow b{ 0, r.t_start, r.t_end, pr, r.bytes_rev, 0, r.t_start };
            b.token = prepare(applySymmetry(sym, r.rev, offset),
                              FlowInfo{ b.t_start, b.t_end, b.packets_total, packetWeight(b, weighting) });
            heap.push(b);
        }
    }

    // end of file: nothing new will arrive, drain whatever is left
    while (!heap.empty()) emitTop();
}
