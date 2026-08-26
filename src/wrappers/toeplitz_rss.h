/**
 * @file toeplitz_rss.h
 * @brief RSS wrapper around the jhash implementation in external/jhash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef TOEPLITZ_RSS_H
 #define TOEPLITZ_RSS_H

#include <cstddef>
#include <cstdint>

static const uint8_t TOEPLITZ_KEY[40] = {
    0x6d, 0x5a, 0x56, 0xda, 0x25, 0x5b, 0x0e, 0xc2,
    0x41, 0x67, 0x25, 0x3d, 0x43, 0xa3, 0x8f, 0xb0,
    0xd0, 0xca, 0x2b, 0xcb, 0xae, 0x7b, 0x30, 0xb4,
    0x77, 0xcb, 0x2d, 0xa3, 0x80, 0x30, 0xf2, 0x0c,
    0x6a, 0x42, 0xb7, 0x3b, 0xbe, 0xac, 0x01, 0xfa
};

constexpr uint32_t TOEPLITZ_WORDS = 9;

inline uint32_t load32(const uint8_t *key) {
    return  (static_cast<uint32_t>(key[0]) << 24) |
            (static_cast<uint32_t>(key[1]) << 16) |
            (static_cast<uint32_t>(key[2]) << 8 ) |
            (static_cast<uint32_t>(key[3])      ) ;
}

uint32_t toeplitzRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //TOEPLITZ_RSS_H