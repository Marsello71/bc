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

constexpr uint32_t TOEPLITZ_WORDS = 9;

inline uint32_t load32(const uint8_t *key) {
    return  (static_cast<uint32_t>(key[0]) << 24) |
            (static_cast<uint32_t>(key[1]) << 16) |
            (static_cast<uint32_t>(key[2]) << 8 ) |
            (static_cast<uint32_t>(key[3])      ) ;
}

uint32_t toeplitzRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //TOEPLITZ_RSS_H