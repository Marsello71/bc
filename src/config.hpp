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

/// Number of packets per measurement window. Each window produces one CSV row
/// per (algorithm, key_id, num_channels). The last partial window is dropped.
/// 150k packets ~= 10 ms of a saturated 100 GbE link 
constexpr uint32_t WINDOW_SIZE = 150000;

/// How many independent random keys each keyed hash is evaluated with.
constexpr std::size_t NUM_KEYS = 16;

/// Length of an RSS key in bytes.
constexpr std::size_t RSS_KEY_SIZE = 44;

/// DMA channel counts under test. Powers of two and two non-powers (20, 40) —
/// keep this ascending, the RETA_SIZE static_assert below reads the last element.
constexpr std::array<int, 7> CHANNEL_COUNTS = {8, 16, 20, 32, 40, 64, 128};

} // namespace config

constexpr std::size_t TUPLE_SIZE = 36;

#define RETA_ON 1

constexpr std::size_t RETA_SIZE = 128; //128/512

static_assert((RETA_SIZE & (RETA_SIZE - 1)) == 0,
              "RETA_SIZE must be a power of two - the index is a bit mask");
static_assert(RETA_SIZE >= static_cast<std::size_t>(config::CHANNEL_COUNTS.back()),
              "RETA_SIZE must be at least the largest channel count");

#endif // CONFIG_HPP
