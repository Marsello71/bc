/**
 * @file transform.hpp
 * @brief Optional pre-hash symmetrisation of the tuple.
 *
 * Symmetric RSS: make the hash of (A->B) equal the hash of (B->A) so both
 * directions of a flow land on the same DMA channel. Two ways to achieve it:
 *
 *   None      no change, hash the tuple as-is
 *   XorFold   replace each src/dst field pair with (src XOR dst) in both halves
 *   SortFold  put the lexicographically smaller endpoint first (swap halves if needed)
 *
 * The protocol byte (offset 36) is always passed through unchanged.
 *
 * This replaces symmetric_control_bit_XOR() / symmetric_control_bit() from
 * benchmark.cpp. Those returned a fresh std::array per call — i.e. one heap-free
 * but still copy-heavy allocation per tuple * per key * per algorithm. The new
 * API writes into a caller-owned buffer so the hot loop allocates nothing.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <array>
#include <cstdint>

#include "tuple.hpp"

enum class Symmetry {
    None     = 0,
    XorFold  = 1,   // was: symmetry == 1  (symmetric_control_bit_XOR)
    SortFold = 2,   // was: symmetry == 2  (symmetric_control_bit)
};

/// Parse the CLI argument (argv[3], "0"/"1"/"2") into a Symmetry value.
Symmetry parseSymmetry(const char *arg);

/// Short lowercase name for CSV / filenames ("none" / "xorfold" / "sortfold").
const char *symmetryName(Symmetry s);

/// Write the (possibly transformed) tuple into @p out.
/// For Symmetry::None this is just a copy of @p in.
/// @p in and @p out may NOT alias.
void applySymmetry(Symmetry s,
                   const std::array<uint8_t, TUPLE_SIZE> &in,
                   std::array<uint8_t, TUPLE_SIZE> &out);

#endif // TRANSFORM_HPP
