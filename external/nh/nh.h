/**
 * @file nh.h
 * @brief NH universal hash, the inner compression function of UMAC.
 *
 * Reference: T. Krovetz (ed.), "UMAC: Message Authentication Code using
 * Universal Hashing", RFC 4418, section 2 ("NH").
 *
 *   NH_K(M) = ( SUM_{i=1..n/2}
 *                 ((M[2i-1] + K[2i-1]) mod 2^32)
 *               * ((M[2i]   + K[2i]  ) mod 2^32) ) mod 2^64
 *
 * M and K are arrays of n 32-bit words; n must be a positive even number
 * (the caller pads M with zero words if needed). K must hold at least n
 * words. Word endianness is the caller's choice - it does not affect the
 * Delta-universality of NH, only which byte maps to which word lane.
 *
 * This file is the plain arithmetic definition only, with no UMAC framing
 * (no polynomial layer, no pad, no MAC). It is kept unmodified as the
 * external reference; project-specific glue lives in src/wrappers/.
 */
#ifndef NH_REFERENCE_H
#define NH_REFERENCE_H

#include <cstddef>
#include <cstdint>

/// NH_K(M) over @p n_words 32-bit words. @p n_words must be even; @p K must
/// point to at least @p n_words words. Returns the 64-bit NH value.
uint64_t nh_hash(const uint32_t *M, const uint32_t *K, size_t n_words);

#endif // NH_REFERENCE_H
