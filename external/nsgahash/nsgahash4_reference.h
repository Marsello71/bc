/**
 * @file nsgahash4_reference.h
 * @brief Reference implementation of NSGAHash4 — a non-cryptographic hash
 * function evolved via multi-objective linear genetic programming for
 * network flow hashing in FPGAs.
 *
 * Source (algorithm reproduced verbatim from Fig. 2 of the paper, no
 * modification to the mixing logic):
 *   D. Grochol and L. Sekanina, "Fast Reconfigurable Hash Functions for
 *   Network Flow Hashing in FPGAs," IEEE International Symposium on
 *   Adaptive Hardware and Systems (AHS), 2019.
 *   https://www.fit.vut.cz/research/publication-file/11706/ahs19_hash.pdf
 *
 * NSGAHash4 takes 3x 32-bit input words (96 bits total), matching the
 * paper's reduced IPv4 5-tuple representation:
 *   r0 = source IP (32b)
 *   r1 = destination IP (32b)
 *   r2 = ((src_port << 16) | dst_port) ^ protocol
 * Any adaptation needed to map a wider tuple (e.g. IPv6-style 16-byte
 * addresses) down to these 3 words is intentionally NOT done here — it is
 * a methodology decision left to the caller, so this file stays a faithful,
 * citable reproduction of the published, hardware-synthesized circuit.
 *
 * Note: the published algorithm reduces its output to 16 meaningful bits
 * (the paper's "output reduction" XORs the two 16-bit halves of the final
 * 32-bit register together). This function returns the raw 32-bit
 * expression as given in the paper (r0 ^ (r0 >> 16)) without truncating it
 * to 16 bits — the caller should be aware that only the lower 16 bits carry
 * mixed entropy from both halves; the upper 16 bits equal the unmodified
 * upper half of the internal r0.
 */
#ifndef NSGAHASH4_REFERENCE_H
#define NSGAHASH4_REFERENCE_H

#include <cstdint>

uint32_t nsgaHash4Core(uint32_t r0, uint32_t r1, uint32_t r2);

#endif // NSGAHASH4_REFERENCE_H
