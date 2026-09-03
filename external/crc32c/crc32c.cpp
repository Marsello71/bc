/**
 * @file crc32c.cpp
 * @brief CRC-32C (Castagnoli) - portable table-driven implementation.
 *
 * See crc32c.h for provenance (RFC 3720 B.4 / Mark Adler reference).
 */

#include "crc32c.h"

#include <array>

namespace {

/// Reflected Castagnoli polynomial.
constexpr uint32_t kPoly = 0x82F63B78u;

/// Byte-indexed lookup table, built once at static-init time.
const std::array<uint32_t, 256> kTable = [] {
    std::array<uint32_t, 256> table{};
    for (uint32_t n = 0; n < 256; ++n) {
        uint32_t c = n;
        for (int k = 0; k < 8; ++k) {
            c = (c & 1u) ? (c >> 1) ^ kPoly : (c >> 1);
        }
        table[n] = c;
    }
    return table;
}();

} // namespace

uint32_t crc32c(uint32_t crc, const void *buf, size_t len) {
    const uint8_t *p = static_cast<const uint8_t *>(buf);
    crc = ~crc;
    while (len--) {
        crc = kTable[(crc ^ *p++) & 0xffu] ^ (crc >> 8);
    }
    return ~crc;
}
