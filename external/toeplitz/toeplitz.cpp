#include "toeplitz.h"

static int isBitOne(const uint8_t *number, int position) {
    int byte_pos = position / 8;
    int bit_pos  = position % 8;

    if ((number[byte_pos] & (1u << bit_pos)) != 0u) {
        return 1;
    }
    return 0;
}

uint32_t toeplitzHashFunction(const uint8_t *key) {
    uint32_t hash = 0;
    uint32_t window = load32(TOEPLITZ_KEY);
    int key_size_bits = 37 * 8;

    for(int i = 0; i < key_size_bits; i++) {
        if(isBitOne(key,i)) {
            hash ^= window;
        }
        window <<= 1;

        if(isBitOne(TOEPLITZ_KEY, i + 32)) {
            window |= 1;
        }
    }
    return hash;
}
