#include "nsgahash4_reference.h"

namespace {
    // Portable 32-bit rotate-right (std::rotr is C++20, this project targets C++17).
    inline uint32_t rotr32(uint32_t x, int n) {
        return (x >> n) | (x << (32 - n));
    }
}

uint32_t nsgaHash4Core(uint32_t r0, uint32_t r1, uint32_t r2) {
    r1 = rotr32(r1, 22);
    uint32_t r3 = r2 + r0;
    r0 = r1 + r3;
    return r0 ^ (r0 >> 16);
}
