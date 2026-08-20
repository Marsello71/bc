/**
 * @file siphash_rss.h
 * @brief RSS wrapper around the SipHash implementation in external/siphash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef SIPHASH_RSS_H
 #define SIPHASH_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t sipHashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //SIPHASH_RSS_H