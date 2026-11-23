/**
 * @file rss.cpp
 * @brief The RSS pipeline. See rss.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "rss.hpp"

#include <algorithm>   // std::max, std::fill
#include <cstring>
#include <random>

#include "wrappers/hash_register.hpp"

// ---- once per program -------------------------------------------------------

std::vector<Key> generateKeys(int count) {
    std::vector<Key> keys;
    std::mt19937 generator(config::KEY_SEED);
    std::uniform_int_distribution<int> distribution(0, 255);

    for (int i = 0; i < count; i++) {
        Key key;
        for (std::size_t j = 0; j < config::RSS_KEY_SIZE; j++) {
            key[j] = static_cast<uint8_t>(distribution(generator));
        }
        keys.push_back(key);
    }
    return keys;
}

int findHash(const std::string &name) {
    for (std::size_t i = 0; i < hash_functions_arr.size(); i++) {
        if (name == hash_functions_arr[i].name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// ---- step 1: symmetry --------------------------------------------------------
// (moved unchanged from the old transform.cpp)

namespace {

// both halves become (src XOR dst), so swapping src and dst gives the same bytes
Tuple symmetricXor(const Tuple &tuple, std::size_t offset) {
    Tuple xor_key;
    int remaining = TUPLE_SIZE - 2 * offset;
    for (std::size_t i = 0; i < offset; i++) {
        xor_key[i] = tuple[i] ^ tuple[i + offset];
        xor_key[i + offset] = tuple[i] ^ tuple[i + offset];
    }
    if (remaining) {
        int from = TUPLE_SIZE - remaining;
        std::memcpy(&xor_key[from], &tuple[from], remaining);
    }
    return xor_key;
}

// the bigger half (compared byte by byte) always goes first
Tuple symmetricSort(const Tuple &tuple, std::size_t offset) {
    Tuple sorted_key;
    int ip_cmp = std::memcmp(&tuple[0], &tuple[offset], offset);
    bool need_swap = ip_cmp < 0;
    int remaining = TUPLE_SIZE - 2 * offset;

    if (need_swap) {
        std::memcpy(&sorted_key[0], &tuple[offset], offset);
        std::memcpy(&sorted_key[offset], &tuple[0], offset);
    } else {
        std::memcpy(&sorted_key[0], &tuple[0], 2 * offset);
    }
    if (remaining) {
        int from = TUPLE_SIZE - remaining;
        std::memcpy(&sorted_key[from], &tuple[from], remaining);
    }
    return sorted_key;
}

} // namespace

Tuple applySymmetry(const Tuple &t, Symmetry s, bool ip_only) {
    // where the dst half starts: IP (16) + port (2), or just IP (16)
    std::size_t offset = ip_only ? 16 : 18;
    switch (s) {
        case Symmetry::Xor:  return symmetricXor(t, offset);
        case Symmetry::Sort: return symmetricSort(t, offset);
        default:             return t;
    }
}

// ---- steps 2-4 ---------------------------------------------------------------

uint32_t computeHash(int hash_id, const Tuple &t, const Key &key) {
    const auto &hash_func = hash_functions_arr[hash_id];
    return hash_func.fn(t.data(), TUPLE_SIZE, key.data());
}

int retaIndex(uint32_t hash) {
    return hash & (RETA_SIZE - 1);
}

int retaChannel(int index, int channels) {
    return index % channels;
}

int moduloChannel(uint32_t hash, int channels) {
    return hash % (uint32_t)channels;
}

// ---- the whole pipeline ------------------------------------------------------

void rssChannels(const Tuple &tuple, const RssConfig &cfg, const std::vector<Key> &keys,
                 const std::vector<int> &channels, uint8_t *out) {
    // 1. symmetry does not depend on the key -> once per flow
    Tuple input = applySymmetry(tuple, cfg.symmetry, cfg.ip_only);

    for (std::size_t k = 0; k < keys.size(); k++) {
        // 2. the hash does not depend on the channel count -> once per key
        uint32_t h = computeHash(cfg.hash_id, input, keys[k]);

        for (std::size_t c = 0; c < channels.size(); c++) {
            // 3 + 4. hash -> channel
            int n = channels[c];
            int ch;
            if (cfg.reta) {
                ch = retaChannel(retaIndex(h), n);
            } else {
                ch = moduloChannel(h, n);
            }
            out[k * channels.size() + c] = static_cast<uint8_t>(ch);
        }
    }
}

// ---- JSQ oracle --------------------------------------------------------------

// flows shorter than this count as instant: they are seen through the delivered
// load, and a 2-packet flow lasting 1 ns does not get an absurd rate
static const double MIN_DURATION = 1e-3;     // seconds

void jsqInit(JsqState &j, const std::vector<int> &channels) {
    j.rate_sum.resize(channels.size());               // one list per channel count
    for (std::size_t c = 0; c < channels.size(); c++) {
        j.rate_sum[c].assign(channels[c], 0.0);       // no live flows yet -> rate 0
    }
    j.packet_rate_total = 0.0;                        // packets per second of all live flows
    j.rated_flows = 0;                                // live flows with a non-zero rate
}

void jsqFlowStart(JsqState &j, FlowToken flow, const FlowInfo &info,
                  const std::vector<std::vector<int64_t>> &window_hist,
                  int64_t packets_in_window, long window, uint8_t *out) {
    if (flow >= j.flow_wrate.size()) {                // first time we see this flow number
        j.flow_wrate.resize(flow + 1);                // room for its load rate
        j.flow_prate.resize(flow + 1);                // room for its packet rate
    }

    double prate = 0.0;                               // packets per second of this flow
    double wrate = 0.0;                               // load per second of this flow
    const double duration = info.t_end - info.t_start;
    if (info.packets > 1 && duration > 0.0) {         // instant flows keep rate 0
        prate = static_cast<double>(info.packets) / std::max(duration, MIN_DURATION);
        wrate = prate * static_cast<double>(info.weight);
    }

    // how many seconds the rest of this window should take
    const int64_t left = window - packets_in_window;  // packets missing in the window
    const double t_left = j.packet_rate_total > 0.0 ? left / j.packet_rate_total : 0.0;

    for (std::size_t c = 0; c < j.rate_sum.size(); c++) {   // decide for every channel count
        std::size_t best = 0;                         // start with channel 0
        double best_load = window_hist[c][0] + t_left * j.rate_sum[c][0];

        for (std::size_t i = 1; i < window_hist[c].size(); i++) {   // check the other channels
            const double load = window_hist[c][i] + t_left * j.rate_sum[c][i];
            if (load < best_load) {                   // smaller expected load -> new best
                best = i;
                best_load = load;
            }
        }
        out[c] = static_cast<uint8_t>(best);          // the whole flow goes to this channel
        j.rate_sum[c][best] += wrate;                 // the channel now carries its rate
    }

    j.packet_rate_total += prate;                     // one more live flow
    if (prate > 0.0) j.rated_flows++;
    j.flow_wrate[flow] = wrate;                       // remember, flowEnd takes it back out
    j.flow_prate[flow] = prate;
}

void jsqFlowEnd(JsqState &j, FlowToken flow, const uint8_t *chosen) {
    for (std::size_t c = 0; c < j.rate_sum.size(); c++) {
        j.rate_sum[c][chosen[c]] -= j.flow_wrate[flow];   // its channel no longer carries it
    }
    j.packet_rate_total -= j.flow_prate[flow];

    if (j.flow_prate[flow] > 0.0) {                   // this flow was counted as rated
        j.rated_flows--;
        if (j.rated_flows == 0) {                     // nothing live: clear rounding leftovers
            j.packet_rate_total = 0.0;
            for (std::size_t c = 0; c < j.rate_sum.size(); c++) {
                std::fill(j.rate_sum[c].begin(), j.rate_sum[c].end(), 0.0);
            }
        }
    }
}
