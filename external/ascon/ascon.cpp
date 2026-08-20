#include "ascon.h"

#include <cstring>

namespace
{
    struct AsconState
    {
        uint64_t x[5];
    };

    constexpr size_t RATE = 8; // bytes absorbed per permutation round

    // IV for Ascon-Hash256 (variant=2, a=12, b=12, hash size=256, rate=8),
    // as defined by NIST SP 800-232. Precomputed here since it only ever
    // takes this one value for this fixed variant.
    constexpr uint64_t HASH_IV = 0x0000080100CC0002ULL;

    constexpr uint8_t ROUND_CONSTANTS[12] = {
        0xf0, 0xe1, 0xd2, 0xc3, 0xb4, 0xa5,
        0x96, 0x87, 0x78, 0x69, 0x5a, 0x4b,
    };

    inline uint64_t rotr(uint64_t x, int n)
    {
        return (x >> n) | (x << (64 - n));
    }

    void round(AsconState &s, uint8_t roundConstant)
    {
        AsconState t;

        // addition of round constant
        s.x[2] ^= roundConstant;

        // substitution layer (Keccak-style S-box)
        s.x[0] ^= s.x[4];
        s.x[4] ^= s.x[3];
        s.x[2] ^= s.x[1];

        t.x[0] = s.x[0] ^ (~s.x[1] & s.x[2]);
        t.x[1] = s.x[1] ^ (~s.x[2] & s.x[3]);
        t.x[2] = s.x[2] ^ (~s.x[3] & s.x[4]);
        t.x[3] = s.x[3] ^ (~s.x[4] & s.x[0]);
        t.x[4] = s.x[4] ^ (~s.x[0] & s.x[1]);

        t.x[1] ^= t.x[0];
        t.x[0] ^= t.x[4];
        t.x[3] ^= t.x[2];
        t.x[2] = ~t.x[2];

        // linear diffusion layer
        s.x[0] = t.x[0] ^ rotr(t.x[0], 19) ^ rotr(t.x[0], 28);
        s.x[1] = t.x[1] ^ rotr(t.x[1], 61) ^ rotr(t.x[1], 39);
        s.x[2] = t.x[2] ^ rotr(t.x[2], 1) ^ rotr(t.x[2], 6);
        s.x[3] = t.x[3] ^ rotr(t.x[3], 10) ^ rotr(t.x[3], 17);
        s.x[4] = t.x[4] ^ rotr(t.x[4], 7) ^ rotr(t.x[4], 41);
    }

    void p12(AsconState &s)
    {
        for (uint8_t roundConstant : ROUND_CONSTANTS)
        {
            round(s, roundConstant);
        }
    }

    // load up to 8 bytes little-endian into a 64-bit word (0-padded)
    uint64_t loadBytes(const uint8_t *bytes, size_t n)
    {
        uint64_t x = 0;

        for (size_t i = 0; i < n; i++)
        {
            x |= static_cast<uint64_t>(bytes[i]) << (8 * i);
        }

        return x;
    }
}

uint32_t asconHashFunction(const uint8_t *data, size_t length)
{
    AsconState s{{HASH_IV, 0, 0, 0, 0}};
    p12(s);

    // absorb full 8-byte blocks
    while (length >= RATE)
    {
        s.x[0] ^= loadBytes(data, RATE);
        p12(s);
        data += RATE;
        length -= RATE;
    }

    // absorb final (possibly empty) partial block, with padding
    s.x[0] ^= loadBytes(data, length);
    s.x[0] ^= static_cast<uint64_t>(0x01) << (8 * length); // padding byte
    p12(s);

    // squeeze: the first 4 bytes of the standard 32-byte digest are exactly
    // the low 32 bits of x[0] at this point (little-endian), so we can
    // return them directly without running the rest of the squeeze phase.
    return static_cast<uint32_t>(s.x[0]);
}
