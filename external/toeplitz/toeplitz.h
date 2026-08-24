/**
 * @file toeplitz.h
 * @brief toeplitz function
 * 
 * Marcel Koptak xkoptam00@vutbr.cz 
 */

#ifndef TOEPLITZ_HASH_H
#define TOEPLITZ_HASH_H

#include <cstdint>

constexpr int toepletz_key_bit_size = 328;

static const uint8_t TOEPLITZ_KEY[41] = {
    0x60, 0xfa, 0xcb, 0xf5, 0xcf, 0xaa, 0x6b, 0xb5, 0x78, 0x5e,
    0xcd, 0x6a, 0x17, 0xf0, 0x34, 0x10, 0x36, 0x91, 0xc0, 0x66,
    0xc9, 0x78, 0x20, 0x87, 0x38, 0xad, 0x9d, 0x3d, 0x11, 0x48,
    0x98, 0x4e, 0xff, 0xc9, 0x0a, 0xc9, 0xb6, 0x6e, 0xa9, 0xfd,
    0x97
};

uint32_t toeplitzHashFunction(const uint8_t *key);

inline uint32_t load32(const uint8_t *key) {
    return  (static_cast<uint64_t>(key[0]) << 24) |
            (static_cast<uint64_t>(key[1]) << 16) |
            (static_cast<uint64_t>(key[2]) << 8 ) |
            (static_cast<uint64_t>(key[3])      ) ;
}

#endif // TOEPLITZ_HASH_H
