/**
 * @file transform.cpp
 * @brief Implementation of the pre-hash symmetrisation. See transform.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "transform.hpp"

#include <cstring>
#include <cstdlib>


static std::array<uint8_t, TUPLE_SIZE> symmetric_xor(const std::array<uint8_t, TUPLE_SIZE> &tuple) {
    std::array<uint8_t, TUPLE_SIZE> xor_key;
    for(size_t i = 0; i < (TUPLE_SIZE-1)/2; i++) {
        xor_key[i] = tuple[i] ^ tuple[i+18];
        xor_key[i+18] = tuple[i] ^ tuple[i+18];
    }
    xor_key[36] = tuple[36];
    return xor_key;
} 

static std::array<uint8_t, TUPLE_SIZE> symmetric_sort(const std::array<uint8_t, TUPLE_SIZE> &tuple) {
    std::array<uint8_t, TUPLE_SIZE> sorted_key;

    int ip_cmp = memcmp(&tuple[0], &tuple[18], 16);
    bool need_swap = (ip_cmp < 0) || (ip_cmp == 0 && memcmp(&tuple[16], &tuple[34], 2) < 0);

    if (need_swap) {
        memcpy(&sorted_key[0], &tuple[18], 18);
        memcpy(&sorted_key[18], &tuple[0], 18);
    } else {
        memcpy(&sorted_key[0], &tuple[0], 36);
    }
    sorted_key[36] = tuple[36];

    return sorted_key;
}

Symmetry parseSymmetry(const char *arg) {
    switch (std::atoi(arg)) {
        case 1:  return Symmetry::XorFold;
        case 2:  return Symmetry::SortFold;
        default: return Symmetry::None;
    }
}

const char *symmetryName(Symmetry s) {
    switch (s) {
        case Symmetry::XorFold:  return "xorfold";
        case Symmetry::SortFold: return "sortfold";
        default:                 return "none";
    }
}

std::array<uint8_t, TUPLE_SIZE> applySymmetry(Symmetry s, const std::array<uint8_t, TUPLE_SIZE> &in) {
    switch (s) {
        case Symmetry::XorFold:  return symmetric_xor(in);
        case Symmetry::SortFold: return symmetric_sort(in);
        default: return in;
    }
}