/**
 * @file simulation.cpp
 * @brief Counting packets into channels, window by window. See simulation.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #include <algorithm>
#include "simulation.hpp"
#include "metrics.hpp"

Simulation::Simulation(const Settings &s, const RssConfig &cfg,
                       const std::vector<Key> &keys, std::ostream &out)
    : cfg_(cfg), keys_(keys), channels_(s.channels), out_(out) {
    const std::size_t NK = keys_.size();
    const std::size_t NC = channels_.size();

    hash_hist_.resize(NK);
    for(std::size_t k = 0; k < NK; k++) {
        hash_hist_[k].resize(NC);
        for(std::size_t c = 0; c < NC; c++) {
            hash_hist_[k][c].assign(channels_[c], 0);
        }
    }

    window_sizes_.resize(NC);
    jsq_hist_.resize(NC);
    for(std::size_t c = 0; c < NC; c++) {
        jsq_hist_[c].assign(channels_[c], 0);
        window_sizes_[c] = s.scaling ? s.can_size * channels_[c] : s.window;
    }
    jsqInit(jsq_, channels_);                  // JSQ starts with no live flows

    packets_in_window_.assign(NC, 0);
    window_weight_.assign(NC, 0);
    window_index_.assign(NC, 0);

    out_ << "method,window,num_channels,overload,fairness,chi\n";
}

FlowToken Simulation::flowStart(const Tuple &tuple, const FlowInfo &info) {
    const std::size_t PER_FLOW = keys_.size() * channels_.size();
    FlowToken flow;                         // number of this flow's slot

    if (!free_flows_.empty()) {             // a finished flow left its slot free
        flow = free_flows_.back();          // take the last freed number
        free_flows_.pop_back();             // and remove it from the free list
    } else {                                // no free slot
        flow = next_flow_;                  // use the next new number
        next_flow_++;                       // the following flow gets the one after it
        hash_slots_.resize(next_flow_ * PER_FLOW);   // make room for the new slot
        jsq_slots_.resize(next_flow_ * channels_.size());   // room for JSQ's channels too
    }

    // run the RSS pipeline, it writes all channels into this flow's slot
    rssChannels(tuple, cfg_, keys_, channels_, &hash_slots_[flow * PER_FLOW]);

    // JSQ picks its own channel from the current loads, one per channel count
    jsqFlowStart(jsq_, flow, info, jsq_hist_, packets_in_window_, window_sizes_,
                 &jsq_slots_[flow * channels_.size()]);

    return flow;                            // flow_expand passes this number back with every packet
}

void Simulation::packet(FlowToken flow, int64_t weight) {
    const std::size_t NK = keys_.size();       // number of keys, e.g. 16
    const std::size_t NC = channels_.size();   // number of channel counts, e.g. 7

    const uint8_t *slot = &hash_slots_[flow * NK * NC];

    for (std::size_t k = 0; k < NK; k++) {         // every key is its own simulation
        for (std::size_t c = 0; c < NC; c++) {     // every channel count too
            uint8_t ch = slot[k * NC + c];         // channel chosen for this key and count
            hash_hist_[k][c][ch] += weight;        // add the packet to that channel's load
        }
    }

    const uint8_t *jsq_slot = &jsq_slots_[flow * NC];   // channels JSQ chose for this flow
    for (std::size_t c = 0; c < NC; c++) {
        jsq_hist_[c][jsq_slot[c]] += weight;            // add the packet to that channel
    }

    for (std::size_t c = 0; c < NC; c++) {
        packets_in_window_[c]++;                       // one more packet in this window
        window_weight_[c] += weight;                   // and its load
        if (packets_in_window_[c] == window_sizes_[c]) {   // this window is full
            writeWindow(c);                            // write and reset only this c
        }
    }
}

void Simulation::writeWindow(std::size_t c) {
    const std::size_t NK = keys_.size();       // number of keys

    window_index_[c]++;

    int n = channels_[c];                  // real channel count, e.g. 8

    double overload = 0.0;                 // sums over all keys
    double fairness = 0.0;
    double chi = 0.0;

    for (std::size_t k = 0; k < NK; k++) {
        const std::vector<int64_t> &hist = hash_hist_[k][c];   // channel loads, no copy
        overload += computeOverThreshold(hist, window_weight_[c], n);
        fairness += computeFairness(hist, window_weight_[c], n);
        chi      += computeChi(hist, window_weight_[c], n);
    }
    out_ << "hash,"                        // method
        << window_index_[c] << ','         // window number
        << n << ','                        // channel count
        << overload / NK << ','            // mean over keys
        << fairness / NK << ','
        << chi / NK << '\n';

    out_ << "jsq,"                         // JSQ has no keys, so no mean
            << window_index_[c] << ','
            << n << ','
            << computeOverThreshold(jsq_hist_[c], window_weight_[c], n) << ','
            << computeFairness(jsq_hist_[c], window_weight_[c], n) << ','
            << computeChi(jsq_hist_[c], window_weight_[c], n) << '\n';

    for (std::size_t k = 0; k < NK; k++) {
        std::fill(hash_hist_[k][c].begin(), hash_hist_[k][c].end(), 0);   // only this channel count
    }
    std::fill(jsq_hist_[c].begin(), jsq_hist_[c].end(), 0);               // JSQ loads to 0

    packets_in_window_[c] = 0;
    window_weight_[c] = 0;
}

void Simulation::flowEnd(FlowToken flow) {
    jsqFlowEnd(jsq_, flow, &jsq_slots_[flow * channels_.size()]);   // take its rate back out
    free_flows_.push_back(flow);               // this slot can be reused by a new flow
}

//   Simulation::flowEnd      - jsqFlowEnd(...), give the flow number back
//   Simulation::writeWindow  - metrics per key -> mean over keys -> one "hash" row per
//                              channel count; one "jsq" row per channel count; zero everything
