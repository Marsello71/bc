/**
 * @file transform.hpp
 * @brief Optional pre-hash symmetrisation of the flow_data.
 *
 * Symmetric RSS: make the hash of (A->B) equal the hash of (B->A) so both
 * directions of a flow land on the same DMA channel.
 *
 *   None      no change, hash the tuple as-is
 *   XorFold   replace each src/dst field pair with (src XOR dst) in both halves
 *   SortFold  put the lexicographically smaller endpoint first (swap halves if needed)
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <array>
#include <cstdint>
#include "config.hpp"

enum class Symmetry {
    None     = 0,
    XorFold  = 1,   // was: symmetry == 1  (symmetric_control_bit_XOR)
    SortFold = 2,   // was: symmetry == 2  (symmetric_control_bit)
};

/// Parse the CLI argument (argv[3], "0"/"1"/"2") into a Symmetry value.
Symmetry parseSymmetry(const char *arg);

/// Short lowercase name for CSV / filenames ("none" / "xorfold" / "sortfold").
const char *symmetryName(Symmetry s);

std::array<uint8_t, TUPLE_SIZE> applySymmetry(Symmetry s, const std::array<uint8_t, TUPLE_SIZE> &in);

#endif // TRANSFORM_HPP
