/**
 * @file multiplyshift_rss.h
 * @brief RSS wrapper around the NH universal hash in external/nh.
 *
 * "Multiply-shift" family: one NH layer (RFC 4418 section 2) followed by a
 * multiply-shift style reduction - keep the high 32 bits of the 64-bit NH
 * value. The RSS key is added to the tuple words *inside* the NH products
 * ((m_i + k_i) * (m_{i+1} + k_{i+1})), so the keying is genuinely
 * non-linear: a different key changes the bucket sizes, not just their
 * labels. This is the opposite of crc32c / xorhash, where the key entered
 * only through a GF(2)-linear step and for power-of-2 channel counts was a
 * fixed permutation with no effect on load balance.
 *
 * It is still a deliberately weak hash: a single NH layer, no polynomial
 * finalisation, no diffusion pass, so avalanche is poor. It is the "cheap
 * keyed hash" reference point.
 *
 * Hardware note: NH is n/2 independent 32x32 multiplies feeding an adder
 * tree plus one final shift - the parallel structure UMAC was designed
 * around, unlike a sequential CRC or a per-byte ARX round.
 *
 * Tuple layout: TUPLE_SIZE is 36 bytes = 9 words, padded to 10 (NH needs an
 * even word count). Word 8 (bytes 32..35, the VLAN id region) is paired
 * with the zero pad word, so it enters only as (m8 + k8) * k9 - keyed but
 * linear in m8. Acceptable for a low-entropy trailing field in a weak hash.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */
#ifndef MULTIPLYSHIFT_RSS_H
#define MULTIPLYSHIFT_RSS_H

#include <cstddef>
#include <cstdint>
#include "../endian.hpp"

uint32_t multiplyShiftRssWrapper(const uint8_t *data, size_t length, const uint8_t *key);

#endif // MULTIPLYSHIFT_RSS_H
