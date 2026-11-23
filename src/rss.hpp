/**
 * @file rss.hpp
 * @brief The RSS pipeline: from a 36-byte tuple to a DMA channel.
 *
 *   tuple (36 B)
 *    -> 1. applySymmetry   none / xor / sort
 *    -> 2. computeHash     toeplitz, jhash, ... with one key -> 32 bits
 *    -> 3. retaIndex       hash & 127             (reta = on)
 *    -> 4. retaChannel     RETA table: index % n  (reta = on)
 *       or moduloChannel   hash % n               (reta = off)
 *
 * rssChannels() runs the whole pipeline for one flow direction, for every key
 * and every channel count. A new variant = a new step function and one line
 * in rssChannels().
 *
 * The JSQ oracle ("smart RSS") is a separate path: it does not hash at all,
 * it picks the channel with the smallest expected load.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef RSS_HPP
#define RSS_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "config.hpp"
#include "flow_expand.hpp"   // FlowToken, FlowInfo

using Tuple = std::array<uint8_t, TUPLE_SIZE>;
using Key   = std::array<uint8_t, config::RSS_KEY_SIZE>;

enum class Symmetry { None, Xor, Sort };

/// How the pipeline is set up (made once from Settings in main.cpp).
struct RssConfig {
    int      hash_id;    // index into hash_functions_arr (see findHash)
    Symmetry symmetry;
    bool     ip_only;    // the tuple has no ports -> the symmetry offset is 16, not 18
    bool     reta;       // true: RETA, false: hash % n
};

// ---- once per program -------------------------------------------------------

/// `count` random keys from a fixed seed: the same keys on every run.
/// Keep the returned vector in one place and never copy it (simpletab finds
/// its tables by the key's address).
std::vector<Key> generateKeys(int count);

/// Index of a hash in hash_functions_arr, -1 if there is none of that name.
int findHash(const std::string &name);

// ---- the pipeline, one function per step ------------------------------------

Tuple    applySymmetry(const Tuple &t, Symmetry s, bool ip_only);      // step 1
uint32_t computeHash(int hash_id, const Tuple &t, const Key &key);     // step 2
int      retaIndex(uint32_t hash);                                      // step 3: hash & (RETA_SIZE - 1)
int      retaChannel(int index, int channels);                          // step 4: round robin RETA, index % channels
int      moduloChannel(uint32_t hash, int channels);                    // step 3+4 without RETA: hash % channels

/// The whole pipeline for one flow direction.
/// out[k * channels.size() + c] = channel for key k at channel count channels[c]
void rssChannels(const Tuple &tuple, const RssConfig &cfg, const std::vector<Key> &keys,
                 const std::vector<int> &channels, uint8_t *out);

// ---- JSQ oracle --------------------------------------------------------------

/// State of the oracle: what the live flows will still send.
struct JsqState {
    std::vector<std::vector<double>> rate_sum;   // [c][channel] weight per second of the live flows on it
    double  packet_rate_total;                   // packets per second of all live flows
    int64_t rated_flows;                         // live flows with a non-zero rate
    std::vector<double> flow_wrate;              // [flow] weight per second of that flow
    std::vector<double> flow_prate;              // [flow] packets per second of that flow
};

void jsqInit(JsqState &j, const std::vector<int> &channels);

/// Pick a channel for a new flow at every channel count.
///   window_hist        weight already delivered in this window, [c][channel]
///   packets_in_window  packets counted since the window started, [c]
///   window_sizes       packets per window, [c]
///   out                one channel per channel count
void jsqFlowStart(JsqState &j, FlowToken flow, const FlowInfo &info,
                  const std::vector<std::vector<int64_t>> &window_hist,
                  const std::vector<int64_t> &packets_in_window,
                  const std::vector<long> &window_sizes, uint8_t *out);

/// The flow ended: take its rate back out of the channels in `chosen`.
void jsqFlowEnd(JsqState &j, FlowToken flow, const uint8_t *chosen);

#endif // RSS_HPP
