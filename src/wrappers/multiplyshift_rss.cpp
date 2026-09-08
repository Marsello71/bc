#include "multiplyshift_rss.h"

#include "../../external/nh/nh.h"

namespace {
    // 36-byte tuple -> 9 words, padded to 10 so NH has whole (m,k) pairs.
    constexpr std::size_t NH_WORDS = 10;
}

uint32_t multiplyShiftRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint32_t m[NH_WORDS] = {};
    uint32_t k[NH_WORDS] = {};

    // RSS key is 44 bytes = 11 words, so all NH_WORDS key words are real.
    for (std::size_t i = 0; i < NH_WORDS; i++) {
        k[i] = load32(&key[i * 4]);
    }

    std::size_t full = length / 4;
    for (std::size_t i = 0; i < full && i < NH_WORDS; i++) {
        m[i] = load32(&data[i * 4]);
    }

    // Fold any trailing 1-3 bytes into the next word (none for a 36-byte
    // tuple; kept so the wrapper is correct for other tuple lengths too).
    std::size_t rem = length % 4;
    if (rem != 0 && full < NH_WORDS) {
        uint32_t last = 0;
        for (std::size_t i = 0; i < rem; i++) {
            last |= static_cast<uint32_t>(data[full * 4 + i]) << (24 - 8 * i);
        }
        m[full] = last;
    }

    uint64_t y = nh_hash(m, k, NH_WORDS);

    // multiply-shift reduction: the high half carries the well-mixed bits.
    return static_cast<uint32_t>(y >> 32);
}
