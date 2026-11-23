/**
 * @file simulation.hpp
 * @brief Counting packets into DMA channels, window by window.
 *
 * flow_expand.cpp calls the three functions below (through three one-line
 * lambdas in main.cpp):
 *
 *   flowStart(tuple, info)   a flow direction starts: run the RSS pipeline
 *                            and the JSQ oracle once, remember the channels
 *   packet(flow, weight)     a packet: add its weight to the remembered channels;
 *                            window full -> write CSV rows, start the next window
 *   flowEnd(flow)            the direction is over: free its slot
 *
 * Output (one CSV):
 *   method,window,num_channels,overload,fairness,chi
 *   hash  - the metrics are the MEAN over all keys
 *   jsq   - no keys, one value
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef SIMULATION_HPP
#define SIMULATION_HPP

#include <cstdint>
#include <ostream>
#include <vector>

#include "flow_expand.hpp"   // FlowToken, FlowInfo
#include "rss.hpp"
#include "settings.hpp"

class Simulation {
public:
    /// `keys` is kept by reference (never copied, see generateKeys).
    Simulation(const Settings &s, const RssConfig &cfg, const std::vector<Key> &keys, std::ostream &out);

    FlowToken flowStart(const Tuple &tuple, const FlowInfo &info);
    void      packet(FlowToken flow, int64_t weight);
    void      flowEnd(FlowToken flow);

private:
    void writeWindow();                  // one CSV row per (method, channel count), then zero the counters

    // ---- settings ----
    RssConfig               cfg_;
    const std::vector<Key> &keys_;
    std::vector<int>        channels_;   // e.g. 8 16 20 32 40 64 128
    long                    window_;     // packets per window
    std::ostream           &out_;

    // ---- per flow direction: the decisions made at flowStart ----
    // hash_slots_[flow * keys * NC + k * NC + c] = channel (key k, channel count c)
    // jsq_slots_ [flow * NC + c]                 = channel chosen by JSQ
    std::vector<uint8_t>   hash_slots_;
    std::vector<uint8_t>   jsq_slots_;
    std::vector<FlowToken> free_flows_;  // numbers of finished flows, ready for reuse
    FlowToken              next_flow_ = 0;

    // ---- the current window ----
    std::vector<std::vector<std::vector<int64_t>>> hash_hist_;   // [key][c][channel]
    std::vector<std::vector<int64_t>>              jsq_hist_;    // [c][channel]
    JsqState jsq_;
    int64_t  packets_in_window_ = 0;
    int64_t  window_weight_ = 0;         // sum of the weights, the metrics divide by it
    long     window_index_ = 0;          // number of the last written window
};

#endif // SIMULATION_HPP
