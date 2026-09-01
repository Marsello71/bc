/**
 * @file endian.hpp
 * @brief Definition of the big-endian (network byte order) word load.
 *
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef ENDIAN_HPP
#define ENDIAN_HPP

#include <cstdint>

/// Assemble 4 bytes at @p p into a uint32_t, most significant byte first
/// (network byte order). @p p must point to at least 4 readable bytes.
inline uint32_t load32(const uint8_t *p) {
    return  (static_cast<uint32_t>(p[0]) << 24) |
            (static_cast<uint32_t>(p[1]) << 16) |
            (static_cast<uint32_t>(p[2]) << 8 ) |
            (static_cast<uint32_t>(p[3])      ) ;
}

#endif // ENDIAN_HPP
