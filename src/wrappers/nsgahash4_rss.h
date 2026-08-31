/**
 * @file nsgahash4_rss.h
 * @brief RSS wrapper around the jhash implementation in external/nsgahash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef NSGAHASH_RSS_H
 #define NSGAHASH_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t nsgahashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //NSGAHASH_RSS_H