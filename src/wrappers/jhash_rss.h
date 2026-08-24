/**
 * @file jhash_rss.h
 * @brief RSS wrapper around the jhash implementation in external/jhash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef JHASH_RSS_H
 #define JHASH_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t jhashlittleRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //JHASH_RSS_H