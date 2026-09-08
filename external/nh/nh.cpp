/**
 * @file nh.cpp
 * @brief NH universal hash - see nh.h and RFC 4418 section 2.
 */
#include "nh.h"

uint64_t nh_hash(const uint32_t *M, const uint32_t *K, size_t n_words) {
    uint64_t y = 0;

    for (size_t i = 0; i + 1 < n_words; i += 2) {
        uint32_t a = M[i]     + K[i];       // + is mod 2^32 for uint32_t
        uint32_t b = M[i + 1] + K[i + 1];
        y += static_cast<uint64_t>(a) * static_cast<uint64_t>(b);
    }

    return y;                               // += is mod 2^64 for uint64_t
}
