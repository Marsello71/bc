// Minimal Ascon-Hash256 (NIST SP 800-232) implementation, trimmed down to
// what RSS actually needs: absorb a short arbitrary-length input (the
// 5-tuple, ~37 bytes or less) and return a uint32_t.
//
// Derived from the official public-domain (CC0 1.0) reference
// implementation: https://github.com/ascon/ascon-c
// (crypto_hash/asconhash256/ref), trimmed from the full 256-bit/multi-file
// SUPERCOP layout down to a single absorb-only pass over the sponge, since
// only the first output block of the digest is needed.
//
// Note: Ascon-Hash256 is an UNKEYED hash (no secret key input), unlike
// Chaskey/SipHash/Toeplitz here, which are keyed PRFs. Keep that in mind if
// this is meant to resist adversarial flow placement in RSS.

#ifndef ASCON_H_
#define ASCON_H_

#include <cstddef>
#include <cstdint>

uint32_t asconHashFunction(const uint8_t *data, size_t length);

#endif // ASCON_H_
