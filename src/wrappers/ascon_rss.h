/**
 * @file ascon_rss.h
 * @brief RSS wrapper around the Ascon-Hash256 implementation in external/ascon.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef ASCON_RSS_H
 #define ASCON_RSS_H

#include <cstddef>
#include <cstdint>

uint32_t asconRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

 #endif //ASCON_RSS_H