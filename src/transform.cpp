/**
 * @file transform.cpp
 * @brief Implementation of the pre-hash symmetrisation. See transform.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "transform.hpp"

#include <cstring>

Symmetry parseSymmetry(const char *arg) {
    // TODO: map "0"/"1"/"2" (and reject anything else) to the enum.
    (void)arg;
    return Symmetry::None;
}

const char *symmetryName(Symmetry s) {
    // TODO: switch over the three values.
    (void)s;
    return "none";
}

void applySymmetry(Symmetry s,
                   const std::array<uint8_t, TUPLE_SIZE> &in,
                   std::array<uint8_t, TUPLE_SIZE> &out) {
    using namespace tuple_layout;

    // TODO:
    //   None      -> out = in  (std::memcpy)
    //   XorFold   -> port from benchmark.cpp symmetric_control_bit_XOR (lines ~118-126):
    //                for each byte i in the first half, x = in[i] ^ in[i + halfspan];
    //                write x into both halves. out[PROTOCOL_OFFSET] = in[PROTOCOL_OFFSET].
    //   SortFold  -> port from benchmark.cpp symmetric_control_bit (lines ~128-143):
    //                compare (srcip|srcport) vs (dstip|dstport); if src > dst swap the
    //                two halves, else copy straight. Keep the protocol byte last.
    //
    // Watch the exact byte ranges — the old code folded (TUPLE_SIZE-1)/2 = 18 bytes.
    (void)s;
    std::memcpy(out.data(), in.data(), TUPLE_SIZE);
}
