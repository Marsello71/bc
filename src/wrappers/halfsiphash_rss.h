/**
 * @file halfsiphash_rss.h
 * @brief RSS wrapper around the Half SipHash implementation in external/siphash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef HALFSIPHASH_RSS_H
 #define HALFSIPHASH_RSS_H

#include <cstddef>
#include <cstdint>


uint32_t halfSipHashRssWrapper(const uint8_t *data, size_t length,const uint8_t *key);


 #endif //HALFSIPHASH_RSS_H