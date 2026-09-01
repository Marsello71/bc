/**
 * @file xorhash_rss.h
 * @brief RSS wrapper: cheapest-possible hardware baseline - XOR-fold the
 * tuple and the key together, word by word. Single combinational XOR
 * layer, no rounds/rotation/S-box (same hardware tier as Toeplitz/CRC32C),
 * used as a lower-bound reference candidate.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */
#ifndef XORHASH_RSS_H
#define XORHASH_RSS_H

#include <cstddef>
#include <cstdint>
#include "../endian.hpp"

uint32_t xorHashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

#endif // XORHASH_RSS_H
