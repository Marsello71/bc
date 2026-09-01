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
#include "../endian.hpp"

constexpr uint32_t TOEPLITZ_WORDS = 9;


uint32_t toeplitzRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //TOEPLITZ_RSS_H