/**
 * @file crc32c_rss.h
 * @brief RSS wrapper around the CRC-32C implementation in external/crc32c.
 *
 * Keyed CRC: the first 8 bytes of the RSS key supply a per-key initial
 * value (bytes 0..4) and a final XOR constant (bytes 4..8).
 *
 * NOTE: CRC is GF(2)-linear, so this keying is weak by construction. For
 * power-of-2 channel counts it is only a fixed channel permutation (bucket
 * sizes unchanged); for non-power-of-2 counts it perturbs the distribution
 * slightly. This is intentional - crc32c is the "linear hash + linear key"
 * reference point sitting between xorhash and Toeplitz.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef CRC32C_RSS_H
#define CRC32C_RSS_H

#include <cstddef>
#include <cstdint>
#include "../endian.hpp"

uint32_t crc32cRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

#endif // CRC32C_RSS_H
