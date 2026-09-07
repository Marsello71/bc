/**
 * @file transform.cpp
 * @brief Implementation of the pre-hash symmetrisation. See transform.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "transform.hpp"

#include <cstring>
#include <cstdlib>


static std::array<uint8_t, TUPLE_SIZE> symmetric_xor(const std::array<uint8_t, TUPLE_SIZE> &tuple, size_t offset) {
    std::array<uint8_t, TUPLE_SIZE> xor_key;
    int remaining = TUPLE_SIZE - 2 * offset;
    for(size_t i = 0; i < offset; i++) {
        xor_key[i] = tuple[i] ^ tuple[i + offset];
        xor_key[i + offset] = tuple[i] ^ tuple[i + offset];
    }

    if(remaining) {
        int from = TUPLE_SIZE - remaining;
        memcpy(&xor_key[from], &tuple[from], remaining);
    }
    
    return xor_key;
} 

static std::array<uint8_t, TUPLE_SIZE> symmetric_sort(const std::array<uint8_t, TUPLE_SIZE> &tuple, size_t offset) {
    std::array<uint8_t, TUPLE_SIZE> sorted_key;

    int ip_cmp = memcmp(&tuple[0], &tuple[offset], offset);
    bool need_swap = ip_cmp < 0;
    int remaining = TUPLE_SIZE - 2 * offset;

    if (need_swap) {
        memcpy(&sorted_key[0], &tuple[offset], offset);
        memcpy(&sorted_key[offset], &tuple[0], offset);
    } else {
        memcpy(&sorted_key[0], &tuple[0], 2 * offset);
    }
    if(remaining) {
        int from = TUPLE_SIZE - remaining;
        memcpy(&sorted_key[from], &tuple[from], remaining);
    }

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

std::array<uint8_t, TUPLE_SIZE> applySymmetry(Symmetry s, const std::array<uint8_t, TUPLE_SIZE> &in, size_t offset) {
    switch (s) {
        case Symmetry::XorFold:  return symmetric_xor(in,offset);
        case Symmetry::SortFold: return symmetric_sort(in,offset);
        default: return in;
    }
}