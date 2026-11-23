/**
 * @file config.hpp
 * @brief Central configuration for the RSS benchmark.
 *
 *
 * Anything here is part of the results contract — see README.md.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace config {

/// Seed for the std::mt19937 that generates the random RSS keys.
/// Fixed => key generation is deterministic and runs are reproducible.
constexpr uint32_t KEY_SEED = 65536;

/// Length of an RSS key in bytes.
constexpr std::size_t RSS_KEY_SIZE = 40;

} // namespace config

/// Bytes hashed per packet: src IP (16) + src port (2) + dst IP (16) + dst port (2).
/// With IP only the ports are left out and the last 4 bytes stay zero.
constexpr std::size_t TUPLE_SIZE = 36;

/// Entries of the RSS indirection table (RETA). Must be a power of two (the
/// index is the low bits of the hash) and caps the channel count at 128.
constexpr std::size_t RETA_SIZE = 128;

static_assert((RETA_SIZE & (RETA_SIZE - 1)) == 0,
              "RETA_SIZE must be a power of two - the index is a bit mask");

#endif // CONFIG_HPP
