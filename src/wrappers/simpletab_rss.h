/**
 * @file simpletab_rss.h
 * @brief RSS wrapper around the simpletab implementation in external/simpletab.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef SIMPLETAB_RSS_H
 #define SIMPLETAB_RSS_H

#include <cstddef>
#include <cstdint>
#include <array>
#include <vector>
#include "../endian.hpp"
#include "../config.hpp"

uint32_t simpletabRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

void simpletabInit(const std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>> &keys);

 #endif //SIMPLETAB_RSS_H