#include "xorhash_rss.h"

namespace {
    // Matches RSS_KEY_SIZE in benchmark.cpp (real NIC Toeplitz-style key
    // length); XOR-hash just consumes the whole thing.
    constexpr std::size_t XORHASH_KEY_BYTES = 40;
}

uint32_t xorHashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint32_t hash = 0;

    // XOR every full 4-byte word of the tuple together.
    std::size_t full_words = length / 4;
    for (std::size_t i = 0; i < full_words; i++) {
        hash ^= load32(&data[i * 4]);
    }

    // Tuple length isn't a multiple of 4 (37 bytes) - fold the trailing
    // 1-3 bytes in too instead of dropping them, big-endian packed.
    std::size_t remainder = length % 4;
    if (remainder != 0) {
        uint32_t last = 0;
        for (std::size_t i = 0; i < remainder; i++) {
            last |= static_cast<uint32_t>(data[full_words * 4 + i]) << (24 - 8 * i);
        }
        hash ^= last;
    }

    // XOR the key in, word by word.
    for (std::size_t i = 0; i < XORHASH_KEY_BYTES / 4; i++) {
        hash ^= load32(&key[i * 4]);
    }

    return hash;
}
