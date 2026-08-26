/**
 * @file toeplitz_dpdk_reference.h
 * @brief Reference Toeplitz RSS hash, vendored from DPDK
 *
 * Vendored (algorithm unchanged) from DPDK lib/hash/rte_thash.h, function
 * rte_softrss(). Original copyright:
 *   Copyright(c) 2015-2019 Vladimir Medvedkin <medvedkinv@gmail.com>
 *   Copyright(c) 2021 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * rte_bsf32()/rte_cpu_to_be_32() below are NOT part of the original
 * algorithm - they are minimal standalone stand-ins for the DPDK EAL
 * helpers of the same name (bit-scan-forward / host-to-big-endian byte
 * swap), added so this compiles without linking the full DPDK library.
 * input_tuple was changed from uint32_t* to const uint32_t* since this
 * copy never writes through it.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef TOEPLITZ_DPDK_REFERENCE_H
#define TOEPLITZ_DPDK_REFERENCE_H

#include <cstdint>

static inline uint32_t rte_bsf32(uint32_t v) {
    return static_cast<uint32_t>(__builtin_ctz(v));
}

static inline uint32_t rte_cpu_to_be_32(uint32_t x) {
    return __builtin_bswap32(x);
}

uint32_t rte_softrss(const uint32_t *input_tuple, uint32_t input_len, const uint8_t *rss_key);

#endif // TOEPLITZ_DPDK_REFERENCE_H
