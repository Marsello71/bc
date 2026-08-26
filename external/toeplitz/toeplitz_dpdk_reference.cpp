/**
 * @file toeplitz_dpdk_reference.cpp
 * @brief See toeplitz_dpdk_reference.h - vendored from DPDK rte_softrss()
 * (lib/hash/rte_thash.h), algorithm unchanged.
 */

#include "toeplitz_dpdk_reference.h"

uint32_t rte_softrss(const uint32_t *input_tuple, uint32_t input_len, const uint8_t *rss_key) {
    uint32_t i, j, map, ret = 0;

    for (j = 0; j < input_len; j++) {
        for (map = input_tuple[j]; map; map &= (map - 1)) {
            i = rte_bsf32(map);
            ret ^= rte_cpu_to_be_32(((const uint32_t *)rss_key)[j]) << (31 - i) |
                    (uint32_t)((uint64_t)(rte_cpu_to_be_32(((const uint32_t *)rss_key)[j + 1])) >> (i + 1));
        }
    }
    return ret;
}
