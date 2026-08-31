#include "nsgahash4_rss.h"

#include "../../external/nsgahash/nsgahash4_reference.h"

namespace {
    // Big-endian 16-bit read, analogous to load32() in toeplitz_rss.h.
    // Used instead of memcpy() so multi-byte fields are interpreted the
    // same way (network byte order) regardless of host endianness.
    inline uint16_t load16(const uint8_t *p) {
        return static_cast<uint16_t>((static_cast<uint16_t>(p[0]) << 8) | p[1]);
    }
    inline uint32_t load32(const uint8_t *key) {
    return  (static_cast<uint32_t>(key[0]) << 24) |
            (static_cast<uint32_t>(key[1]) << 16) |
            (static_cast<uint32_t>(key[2]) << 8 ) |
            (static_cast<uint32_t>(key[3])      ) ;
}

}

static uint32_t xor_fold(const uint8_t *data) {
    uint32_t s0 = load32(&data[0]);
    uint32_t s1 = load32(&data[4]);
    uint32_t s2 = load32(&data[8]);
    uint32_t s3 = load32(&data[12]);

    return s0 ^ s1 ^ s2 ^ s3;
}

uint32_t nsgahashRssWrapper(const uint8_t *data, size_t /*length*/, const uint8_t * /*key*/) {
    uint32_t r0 = xor_fold(&data[0]);   // src_ip, folded 16 -> 4 bytes
    uint32_t r1 = xor_fold(&data[18]);  // dst_ip, folded 16 -> 4 bytes

    uint16_t src_port = load16(&data[16]);
    uint16_t dst_port = load16(&data[34]);
    uint32_t r2 = (static_cast<uint32_t>(src_port) << 16 | dst_port) ^ data[36];

    return nsgaHash4Core(r0, r1, r2);
}
