/**
 * @file spookyhash_rss.h
 * @brief RSS wrapper around the SpookyHash implementation in external/siphash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef SPOOKYHASH_RSS_H
 #define SPOOKYHASH_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t spookyhashRssWrapper(const uint8_t *data, size_t length,const uint8_t *key);

 #endif //SPOOKYHASH_RSS_H