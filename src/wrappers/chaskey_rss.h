/**
 * @file chaskey_rss.h
 * @brief RSS wrapper around the Chaskey Hash implementation in external/chaskey.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef CHEASKEY_RSS_H
 #define CHEASKEY_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t chaskeyRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //CHEASKEY_RSS_H