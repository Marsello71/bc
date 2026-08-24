/**
 * @file crc32c_rss.h
 * @brief RSS wrapper around the Crc32c implementation in external/crc32c.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef CRC32C_RSS_H
 #define CRC32C_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t crc32cRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //CRC32C_RSS_H