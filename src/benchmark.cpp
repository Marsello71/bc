/**
 * @file benchmark.cpp
 * @brief RSS hash benchmark harness. Loads tuples, runs every registered hash
 * over them, and writes the per-window DMA channel distribution metrics.
 *
 * Two input formats, auto-detected from the CSV header field count:
 *   - simple tuple: 3- or 5-field tuple CSV, one row = one packet
 *   - flow:         11-field CESNET flow CSV, expanded to real per-packet load
 *                   (see flow_expand.hpp), weighted by flow / packet / byte
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

#include "parser.hpp"
#include "wrappers/hash_register.hpp"
#include "results_writer.hpp"
#include "config.hpp"
#include "transform.hpp"
#include "metrics.hpp"
#include "flow_expand.hpp"
#include "reta.hpp"
#include "wrappers/simpletab_rss.h"

// What one flow stores per (algo, key) in flow_slots - the CONTENT of a slot.
// Don't mix it up with FlowToken, which is the NUMBER of the slot (which flow):
//   FlowToken - up to ~800k live flows at once  -> needs uint32_t
//   FlowSlot  - with RETA a 0..127 table index  -> fits in uint8_t
//               without RETA the full 32-bit hash -> uint32_t
// flow_slots is the biggest array in the program (~800k flows x 80 values),
// so 1 byte instead of 4 is 64 MB instead of 256 MB.
#if RETA_ON
    using FlowSlot = uint8_t;
#else
    using FlowSlot = uint32_t;
#endif

// NUM_KEYS random RSS keys. Fixed seed => the same keys on every run,
// so results are reproducible and comparable across runs.
std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>> getKeys() {
    std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>>  keys;
    std::mt19937 generator(config::KEY_SEED);
    std::uniform_int_distribution<int> distribution(0, 255);

    for(std::size_t i = 0; i < config::NUM_KEYS; i++) {
        std::array<uint8_t, config::RSS_KEY_SIZE> key;
        for(std::size_t j = 0; j < config::RSS_KEY_SIZE; j++) {
            key[j] = static_cast<uint8_t>(distribution(generator));
        }
        keys.push_back(key);
    }
    return keys;
}

using KeySet = std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>>;

// simple-tuple path: one CSV row == one packet, always "flow" weighting.
// all tuples fit in RAM, so we can loop over them once per (algo, key) pair.
static void runSimpleTupleMode(std::istream &reader, ResultsWriter &writer,
                               Symmetry sym, size_t offset, const KeySet &keys) {
    std::vector<std::array<uint8_t, TUPLE_SIZE>> tuples;
    tuples.reserve(5000000);

    std::string line;
    while (std::getline(reader, line)) {
        if (line.empty()) continue;
        try {
            tuples.push_back(parseLineToTuple(line));
        } catch (const std::exception &e) {
            std::cerr << "Skipping malformed line: " << e.what() << "\n";
        }
    }
    std::cout << "Loaded " << tuples.size() << " tuples.\n";

    for (const auto &algo : hash_functions_arr) {
        for (std::size_t i = 0; i < config::NUM_KEYS; i++) {
            std::vector<std::vector<int64_t>> histograms(config::CHANNEL_COUNTS.size());
            for (std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++) {
                histograms[j] = std::vector<int64_t>(config::CHANNEL_COUNTS[j], 0);
            }

            size_t count = 0;
            for (const auto &tuple : tuples) {
                count++;
                auto keyed_tuple = applySymmetry(sym, tuple, offset);
                uint32_t hash = algo.fn(keyed_tuple.data(), TUPLE_SIZE, keys[i].data());

                for (std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++) {
                    histograms[j][hash % config::CHANNEL_COUNTS[j]]++;

                    if (count % config::WINDOW_SIZE == 0) {
                        int n = config::CHANNEL_COUNTS[j];
                        ResultRow row{
                            algo.name, sym, "flow",
                            static_cast<long>(count / config::WINDOW_SIZE),
                            static_cast<int>(i), n,
                            computeOverThreshold(histograms[j], config::WINDOW_SIZE, n),
                            computeFairness(histograms[j], config::WINDOW_SIZE, n),
                            computeChi(histograms[j], config::WINDOW_SIZE, n)
                        };
                        writer.write(row);
                        std::fill(histograms[j].begin(), histograms[j].end(), 0);
                    }
                }
            }
        }
    }
} 

// flow path: the packet stream is too big to store, so flow_expand feeds us one
// packet at a time and we count it into EVERY (algo, key, channel-count) bucket
// in a single pass. that's why the histogram has so many dimensions - all of them
// have to be live at once.
//
// Think of every (algo, key) pair as its own parallel simulation ("world"):
// 5 algorithms x 16 keys = 80 worlds. Each packet is counted in all 80 at once,
// and in each world it goes where THAT world's hash sends it. The worlds are
// told apart only in the output, by the algorithm and key_id CSV columns.
//
// RETA version: the hash picks one of 128 RETA entries; which channel an entry
// means is decided only at the end of the window (see the fold below).
static void runFlowModeRetaOn(std::istream &reader, ResultsWriter &writer,
                        Symmetry sym, size_t offset, Weighting weighting,
                        const KeySet &keys) {
    const auto &CC = config::CHANNEL_COUNTS;            // {8, 16, 20, 32, 40, 64, 128}
    const std::size_t NA = hash_functions_arr.size();   // 5 algorithms
    const std::size_t NK = config::NUM_KEYS;            // 16 keys
    const std::size_t NC = config::CHANNEL_COUNTS.size(); // 7 channel counts

    // Per-window load of every RETA entry, for every world, in one flat array.
    // Each world (a, k) owns a block of RETA_SIZE (128) counters:
    //   [ toeplitz,k0: 128 ][ toeplitz,k1: 128 ] ... [ simpletab,k15: 128 ]
    // counter of entry v in world (a, k) = buckets[(a * NK + k) * RETA_SIZE + v]
    // These are table entries, NOT channels yet.
    std::vector<int64_t> buckets(NA * NK * RETA_SIZE, 0);

    // Scratch histogram for one channel count, filled by the fold at window end.
    // Reused for every (world, channel count); reserve the largest size once so
    // resizing it never reallocates.
    std::vector<int64_t> chan;
    chan.reserve(static_cast<std::size_t>(CC.back()));

    // ---- per flow direction state -------------------------------------------
    // Every packet of a direction has the same tuple, so its NA*NK hashes are
    // computed once here and only looked up per packet. Slots are recycled when
    // a direction ends, so this stays at the high-water mark of concurrent
    // directions (~800k on the full dataset), not one slot per flow in the file.
    //
    // Cloakroom picture: flow_slots is a row of lockers, PER_FLOW values each.
    // A token is the locker number, free_slots is the pile of returned tickets,
    // next_slot is the number of the next never-used locker.
    //   locker of token t = flow_slots[t * PER_FLOW ... t * PER_FLOW + PER_FLOW - 1]
    //   value for world (a, k) inside it = flow_slots[t * PER_FLOW + a * NK + k]
    const std::size_t PER_FLOW = NA * NK;        // one value per (algo, key) = 80

    std::vector<FlowSlot>  flow_slots;            // RETA entry index, fits in a byte
    std::vector<FlowToken> free_slots;            // tickets of finished flows, ready for reuse
    FlowToken next_slot = 0;                      // plain counter, must start at 0 (not a vector)

    // called once when a flow direction starts
    FlowPrepare prepare = [&](const std::array<uint8_t, TUPLE_SIZE> &tuple,
                              const FlowInfo & /*info*/) -> FlowToken {   // hashing only needs the tuple
        // 1. get a locker: reuse a returned one if there is any, otherwise open a new one
        FlowToken tok;
        if (!free_slots.empty()) {
            tok = free_slots.back();
            free_slots.pop_back();
        } else {
            tok = next_slot++;                    // take the current value, then increment
            flow_slots.resize(static_cast<std::size_t>(next_slot) * PER_FLOW);  // grow by one locker
        }
        const std::size_t base = static_cast<std::size_t>(tok) * PER_FLOW;  // where this locker starts

        // 2. fill it: the only place a hash is computed now - once per flow direction.
        // h & (RETA_SIZE - 1) keeps the low 7 bits of the hash (= h % 128, cheaper
        // because 128 is a power of two) - the RETA entry, exactly like a real NIC.
        // The entry does not depend on the channel count: the same entry is used
        // for all 7 configurations, only the table it is looked up in differs.
        for (std::size_t a = 0; a < NA; ++a) {
            for (std::size_t k = 0; k < NK; ++k) {
                uint32_t h = hash_functions_arr[a].fn(tuple.data(), TUPLE_SIZE, keys[k].data());
                flow_slots[base + a * NK + k] = static_cast<uint8_t>(h & (RETA_SIZE - 1));
            }
        }

        // 3. flow_expand keeps the ticket in the flow's heap entry and hands it
        // back with every packet of this direction
        return tok;
    };

    // called after the direction's last packet. The locker is not cleared:
    // no packet of this flow will come anymore, and prepare() overwrites all
    // PER_FLOW values when the ticket is handed out again.
    FlowRelease release = [&](FlowToken tok) { free_slots.push_back(tok); };
    // -------------------------------------------------------------------------

    // number of sink() calls so far; decides where windows end. Never reset.
    // int64_t because the full dataset has ~2.6 billion packets (> int32 max).
    int64_t packet_count = 0;
    // sum of WEIGHTS in the current window, the metric denominator (fair share =
    // window_sum_total / n). A window is always WINDOW_SIZE packets, but with byte
    // weighting their total weight is bytes, so WINDOW_SIZE would be wrong here.
    int64_t window_sum_total = 0;   // sum of weights in the current window (metric denominator)
    long    window_index = 0;       // window number written to the CSV, first window = 1

    // runs for every packet flow_expand produces, in time order. this lambda is
    // effectively the inner loop body - flow_expand is the "for each packet" around it.
    PacketSink sink = [&](FlowToken token, int64_t weight) {
        packet_count++;
        window_sum_total += weight;

        const std::size_t base = static_cast<std::size_t>(token) * PER_FLOW;  // this flow's locker

        // no hashing here any more - just count the packet into the bucket its
        // direction was mapped to when it went live.
        // all 80 worlds get the packet: world (a, k) reads its own RETA entry v
        // from the locker and adds the weight to entry v of its own block.
        // e.g. a=1 (jhash), k=3, v=57: block 19 starts at 19*128 = 2432 -> buckets[2489]
        // only 80 additions per packet (no loop over channel counts - that's the fold's job)
        for (std::size_t a = 0; a < NA; ++a) {
            for (std::size_t k = 0; k < NK; ++k) {
                const auto v = flow_slots[base + a * NK + k];
                buckets[(a * NK + k) * RETA_SIZE + v] += weight;
            }
        }

        // window full -> one CSV row per (algo, key, channel-count), then reset
        // (5 x 16 x 7 = 560 rows per window)
        if (packet_count % config::WINDOW_SIZE == 0) {
            window_index++;
            for (std::size_t a = 0; a < NA; ++a) {
                for (std::size_t k = 0; k < NK; ++k) {
                    const std::size_t base = (a * NK + k) * RETA_SIZE;  // this world's block

                    // fold the RETA entries of this (algo, key) into channel loads:
                    // every entry adds its weight to the channel the table points at.
                    // RETAS[c][i] = i % n (round robin), so for n = 8:
                    //   chan[0] = b0 + b8 + b16 + ... + b120, chan[1] = b1 + b9 + ...
                    // when n does not divide 128 (20, 40) some channels get one entry
                    // more than others - that structural imbalance shows up even with
                    // a perfect hash (37.5 per-mille at n=20, 50 at n=40)
                    for (std::size_t c = 0; c < NC; ++c) {
                        int n = CC[c];
                        chan.assign(static_cast<std::size_t>(n), 0);
                        for (std::size_t i = 0; i < RETA_SIZE; ++i)
                            chan[RETAS[c][i]] += buckets[base + i];

                        ResultRow row{
                            hash_functions_arr[a].name, sym, weightingName(weighting),
                            window_index,
                            (int)k,
                            n,
                            computeOverThreshold(chan, window_sum_total, n),
                            computeFairness    (chan, window_sum_total, n),
                            computeChi         (chan, window_sum_total, n)
                        };
                        writer.write(row);
                    }

                    // only now, once every channel count has been folded out of them
                    std::fill(buckets.begin() + base, buckets.begin() + base + RETA_SIZE, 0);
                }
            }
            window_sum_total = 0;
        }
    };

    expandInterleaved(reader, sym, offset, weighting, prepare, sink, release);
    std::cout << "Streamed " << packet_count << " packets, " << window_index << " full windows, "
              << next_slot << " flow slots (" << (next_slot * PER_FLOW * sizeof(flow_slots[0])) / (1024 * 1024)
              << " MB).\n";
}

// Same 80 parallel worlds as runFlowModeRetaOn, but without a RETA: the channel
// is the plain hash % n, computed for all 7 channel counts on every packet.
// Slower (560 additions per packet instead of 80), and 20/40 channels have no
// structural imbalance because a 32-bit hash % 20 is almost uniform.
static void runFlowModeRetaOff(std::istream &reader, ResultsWriter &writer,
                        Symmetry sym, size_t offset, Weighting weighting,
                        const KeySet &keys) {
    const auto &CC = config::CHANNEL_COUNTS;            // {8, 16, 20, 32, 40, 64, 128}
    const std::size_t NA = hash_functions_arr.size();   // 5 algorithms
    const std::size_t NK = config::NUM_KEYS;            // 16 keys
    const std::size_t NC = config::CHANNEL_COUNTS.size(); // 7 channel counts

    // Hist is only a type alias: one histogram = load per channel. It has no size
    // of its own - every Hist starts empty.
    using Hist = std::vector<int64_t>;
    // histograms[a][k][c][ch] = load of channel ch, with CC[c] channels, in world (algo a, key k).
    // Read the constructor inside out: vector<Hist>(NC) = 7 empty histograms,
    // (NK, that) = 16 copies of it, (NA, that) = 5 copies of that -> 5 x 16 x 7.
    std::vector<std::vector<std::vector<Hist>>> histograms(NA, std::vector<std::vector<Hist>>(NK, std::vector<Hist>(NC)));

    // only here do the histograms get their length: c is just the index 0..6,
    // CC[c] turns it into the real channel count (8, 16, ..., 128)
    for (std::size_t a = 0; a < NA; a++)
        for (std::size_t k = 0; k < NK; k++)
            for (std::size_t c = 0; c < NC; c++)
                histograms[a][k][c].assign(CC[c], 0);

    // every flow remembers NA * NK hashes, one per world (5 * 16 = 80)
    const std::size_t PER_FLOW = NA * NK;

    // full 32-bit hash per (algo, key) - NOT a RETA index here. This relies on
    // FlowSlot being uint32_t, which holds because main() calls this function
    // only when RETA_ON is 0. Called with RETA_ON 1, the hash would be silently
    // truncated to 8 bits.
    std::vector<FlowSlot>  flow_slots;
    std::vector<FlowToken> free_slots;            // tickets of finished flows, ready for reuse
    FlowToken next_slot = 0;                      // next never-used locker number

    // called once when a flow direction starts - same locker handling as in
    // runFlowModeRetaOn, only the stored value differs (whole hash)
    FlowPrepare prepare = [&](const std::array<uint8_t, TUPLE_SIZE> &tuple,
                              const FlowInfo & /*info*/) -> FlowToken {   // hashing only needs the tuple
        FlowToken tok;
        if (!free_slots.empty()) {
            tok = free_slots.back();
            free_slots.pop_back();
        } else {
            tok = next_slot++;
            flow_slots.resize(static_cast<std::size_t>(next_slot) * PER_FLOW);
        }
        const std::size_t base = static_cast<std::size_t>(tok) * PER_FLOW;

        // the only place a hash is computed now - once per flow direction
        for (std::size_t a = 0; a < NA; ++a) {
            for (std::size_t k = 0; k < NK; ++k) {
                uint32_t h = hash_functions_arr[a].fn(tuple.data(), TUPLE_SIZE, keys[k].data());
                flow_slots[base + a * NK + k] = h;
            }
        }

        return tok;
    };

    // locker is not cleared - prepare() overwrites it on reuse
    FlowRelease release = [&](FlowToken tok) { free_slots.push_back(tok); };

    int64_t packet_count = 0;       // sink() calls so far, never reset; int64_t: billions of packets
    int64_t window_sum_total = 0;   // sum of weights in the current window (metric denominator)
    long    window_index = 0;       // first window = 1

    // runs for every packet flow_expand produces, in time order. this lambda is
    // effectively the inner loop body - flow_expand is the "for each packet" around it.
    PacketSink sink = [&](FlowToken token, int64_t weight) {
        packet_count++;
        window_sum_total += weight;

        const std::size_t base = static_cast<std::size_t>(token) * PER_FLOW;

        // no hashing here any more - just count the packet into the bucket its
        // direction was mapped to when it went live.
        // for each of the 80 worlds and each of the 7 channel counts the channel is
        // hash % n, so one packet lands in a different channel in every configuration
        // (e.g. hash 2654435769: n=8 -> 1, n=16 -> 9, n=128 -> 57) - 560 additions
        for (std::size_t a = 0; a < NA; ++a) {
            for (std::size_t k = 0; k < NK; ++k) {
                const auto v = flow_slots[base + a * NK + k];
                for (std::size_t c = 0; c < NC; ++c) {
                    histograms[a][k][c][v % CC[c]] += weight;
                }
            }
        }

        // window full -> one CSV row per (algo, key, channel-count), then reset
        // the histograms (flow_slots stay: a flow can span several windows)
        if (packet_count % config::WINDOW_SIZE == 0) {
            window_index++;
            for (std::size_t a = 0; a < NA; ++a) {
                for (std::size_t k = 0; k < NK; ++k) {
                    for (std::size_t c = 0; c < NC; ++c) {
                        int n = CC[c];
                        ResultRow row{
                            hash_functions_arr[a].name, sym, weightingName(weighting),
                            window_index,
                            (int)k,
                            n,
                            computeOverThreshold(histograms[a][k][c], window_sum_total, n),
                            computeFairness    (histograms[a][k][c], window_sum_total, n),
                            computeChi         (histograms[a][k][c], window_sum_total, n)
                        };
                        writer.write(row);
                        std::fill(histograms[a][k][c].begin(), histograms[a][k][c].end(), 0);
                    }
                }
            }
            window_sum_total = 0;
        }
    };

    expandInterleaved(reader, sym, offset, weighting, prepare, sink, release);
    std::cout << "Streamed " << packet_count << " packets, " << window_index << " full windows, "
              << next_slot << " flow slots (" << (next_slot * PER_FLOW * sizeof(flow_slots[0])) / (1024 * 1024)
              << " MB).\n";
}

// JSQ oracle ("smart RSS"): the best we can do while every flow stays on ONE
// channel. A real NIC can't do this - the oracle knows each flow's packet count
// and duration the moment it starts - and that is the point: it is the lower
// bound the hash functions are compared against.
//
// A new flow goes to the channel whose load at the END of the current window is
// expected to be the smallest:
//
//   expected[ch] = histograms[c][ch]            weight already delivered this window
//                + t_left * rate_sum[c][ch]     what the channel's live flows will
//                                               still add before the window is full
//
//   rate_sum = sum of (weight per second) of the live flows on the channel
//   t_left   = packets still missing in the window / packets per second of ALL
//              live flows = how many seconds the rest of the window should last
//
// The whole flow then stays on that channel (flow affinity, like hashing).
// Symmetry has no effect here - the tuple is never looked at.
static void runJsqMode(std::istream &reader, ResultsWriter &writer,
                        Symmetry sym, size_t offset, Weighting weighting) {
    const auto &CC = config::CHANNEL_COUNTS;
    const std::size_t NC = CC.size();

    // flows shorter than this count as "instant": all their packets go out right
    // away, so the oracle sees them through histograms (delivered weight), not
    // through rates. Also keeps rates bounded - a 2-packet flow lasting 1 ns
    // would otherwise be 2e9 packets per second.
    constexpr double MIN_DURATION = 1e-3;   // seconds

    // weight delivered to each channel in the current window. Used twice: as the
    // metric input and as the "already there" part of the expected load.
    // Reset at every window end.
    std::vector<std::vector<int64_t>> histograms(NC) ;
    // weight per second of the live flows on each channel. NOT reset per window:
    // a flow keeps adding its rate for as long as it lives (+ in prepare, - in release)
    std::vector<std::vector<double>> rate_sum(NC);
    for (std::size_t c = 0; c < NC; c++) {
        histograms[c].assign(CC[c], 0);
        rate_sum[c].assign(CC[c], 0.0);
    }
    double  packet_rate_total = 0.0;   // packets per second of all live flows, any channel
    int64_t rated_flows = 0;           // live flows with a non-zero rate (see release)

    // locker per token: the chosen channel for each of the NC channel counts,
    // plus the flow's own rates so release() can take them back out again
    const std::size_t PER_FLOW = NC;
    std::vector<FlowSlot>  flow_slots;
    std::vector<double>    flow_wrate;     // weight per second, one per token
    std::vector<double>    flow_prate;     // packets per second, one per token
    std::vector<FlowToken> free_slots;
    FlowToken next_slot = 0;

    int64_t packet_count = 0;
    int64_t window_sum_total = 0;
    long    window_index = 0;

    FlowPrepare prepare = [&](const std::array<uint8_t, TUPLE_SIZE> & /*tuple*/,
                              const FlowInfo &info) -> FlowToken {
        FlowToken tok;
        if (!free_slots.empty()) {
            tok = free_slots.back();
            free_slots.pop_back();
        } else {
            tok = next_slot++;
            flow_slots.resize(static_cast<std::size_t>(next_slot) * PER_FLOW);
            flow_wrate.resize(next_slot);
            flow_prate.resize(next_slot);
        }
        const std::size_t base = static_cast<std::size_t>(tok) * PER_FLOW;

        // how fast this flow is going to send (flow_expand spreads its packets
        // evenly over [t_start, t_end]); instant flows get rate 0, see MIN_DURATION
        double prate = 0.0, wrate = 0.0;
        const double duration = info.t_end - info.t_start;
        if (info.packets > 1 && duration > 0.0) {
            prate = static_cast<double>(info.packets) / std::max(duration, MIN_DURATION);
            wrate = prate * static_cast<double>(info.weight);
        }

        // expected time until the current window is full
        const int64_t left = config::WINDOW_SIZE - packet_count % config::WINDOW_SIZE;
        const double t_left = packet_rate_total > 0.0 ? left / packet_rate_total : 0.0;

        // pick the channel with the smallest expected end-of-window load, per channel count
        for (std::size_t c = 0; c < NC; c++) {
            std::size_t best = 0;
            double best_load = histograms[c][0] + t_left * rate_sum[c][0];
            for (std::size_t i = 1; i < histograms[c].size(); i++) {
                const double load = histograms[c][i] + t_left * rate_sum[c][i];
                if (load < best_load) { best = i; best_load = load; }
            }
            flow_slots[base + c] = static_cast<FlowSlot>(best);
            rate_sum[c][best] += wrate;        // from now on this channel carries the flow's rate
        }

        packet_rate_total += prate;
        if (prate > 0.0) rated_flows++;
        flow_wrate[tok] = wrate;
        flow_prate[tok] = prate;
        return tok;
    };

    FlowRelease release = [&](FlowToken tok) {
        // the flow is over: take its rate back out of the channels it was on
        const std::size_t base = static_cast<std::size_t>(tok) * PER_FLOW;
        for (std::size_t c = 0; c < NC; c++)
            rate_sum[c][flow_slots[base + c]] -= flow_wrate[tok];
        packet_rate_total -= flow_prate[tok];

        // adding and subtracting doubles leaves tiny rounding leftovers; whenever no
        // rated flow is live the sums must be exactly 0, so wipe the leftovers then
        if (flow_prate[tok] > 0.0 && --rated_flows == 0) {
            packet_rate_total = 0.0;
            for (std::size_t c = 0; c < NC; c++)
                std::fill(rate_sum[c].begin(), rate_sum[c].end(), 0.0);
        }
        free_slots.push_back(tok);
    };

    PacketSink sink = [&](FlowToken token, int64_t weight) {
        packet_count++;
        window_sum_total += weight;

        const std::size_t base = static_cast<std::size_t>(token) * PER_FLOW;

        // the channel was chosen once in prepare(); just count the packet there
        for (std::size_t c = 0; c < NC; ++c) {
            const std::size_t ch = flow_slots[base + c];
            histograms[c][ch] += weight;
        }

        if (packet_count % config::WINDOW_SIZE == 0) {
            window_index++;
            for (std::size_t c = 0; c < NC; ++c) {
                int n = CC[c];
                ResultRow row{
                    "jsq", sym, weightingName(weighting),
                    window_index,
                    0,                  // key_id: JSQ nema kluce
                    n,
                    computeOverThreshold(histograms[c], window_sum_total, n),
                    computeFairness    (histograms[c], window_sum_total, n),
                    computeChi         (histograms[c], window_sum_total, n)
                };
                writer.write(row);
                std::fill(histograms[c].begin(), histograms[c].end(), 0);
            }
            window_sum_total = 0;
        }
    };

    expandInterleaved(reader, sym, offset, weighting, prepare, sink, release);
    std::cout << "Streamed " << packet_count << " packets, " << window_index << " full windows, "
              << next_slot << " flow slots (" << (next_slot * PER_FLOW * sizeof(flow_slots[0])) / (1024 * 1024)
              << " MB).\n";
}


// number of comma-separated fields in a CSV line (commas inside quotes don't count)
static int countFields(const std::string &line) {
    if (line.empty()) return 0;
    int n = 1;
    bool q = false;
    for (char c : line) {
        if (c == '"') q = !q;
        else if (c == ',' && !q) n++;
    }
    return n;
}

int main(int argc, char *argv[]) {
    if (argc < 4 || argc > 6) {
        std::cerr <<
            "usage: benchmark <dataset.csv> <output.csv> <sym 0|1|2> [weighting flow|packet|byte]\n"
            "       benchmark <dataset.csv> <output.csv> <sym 0|1|2> jsq <weighting>\n"
            "  simple-tuple CSV (3 or 5 fields): tuple layout is auto-detected from field count, weighting is ignored\n"
            "  flow CSV (11 fields): 5th arg required, picks how channel load is counted\n"
            "  jsq: join-the-shortest-queue reference instead of the hash functions\n";
        return 1;
    }

    std::ifstream reader(argv[1]);
    if (!reader.is_open()) {
        std::cerr << "Failed to open file for reading\n";
        return 1;
    }
    std::ofstream out(argv[2]);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing\n";
        return 1;
    }

    ResultsWriter writer(out);
    Symmetry sym = parseSymmetry(argv[3]);

    std::string header;
    std::getline(reader, header);          // consume header, use it to pick the format
    int fields = countFields(header);

    auto keys = getKeys();
    simpletabInit(keys);
    
    if (fields == 11) {
        size_t offset = OFFSET;              // offset is based on the config file full tuple/IP_only
        std::string mode = (argc >= 5) ? argv[4] : "";

        if (mode == "jsq") {
            if (argc != 6) {
                std::cerr << "jsq needs the weighting argument: ... jsq flow|packet|byte\n";
                return 1;
            }
            runJsqMode(reader, writer, sym, offset, parseWeighting(argv[5]));
        } else {
            if (argc != 5) {
                std::cerr << "flow CSV needs the weighting argument (flow|packet|byte)\n";
                return 1;
            }
            Weighting weighting = parseWeighting(argv[4]);
#if RETA_ON
            runFlowModeRetaOn(reader, writer, sym, offset, weighting, keys);
#else
            runFlowModeRetaOff(reader, writer, sym, offset, weighting, keys);
#endif
        }
    }
 else if (fields == 3 || fields == 5) {
        size_t offset = (fields == 3) ? 16 : 18;
        runSimpleTupleMode(reader, writer, sym, offset, keys);
    } else {
        std::cerr << "unrecognised CSV: header has " << fields << " fields\n";
        return 1;
    }
    return 0;
}