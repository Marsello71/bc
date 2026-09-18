/**
 * @file reta.hpp
 * @brief RSS indirection tables (RETA), one per channel count under test.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef RETA_HPP
#define RETA_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "config.hpp"

/// One table per channel count: RETAS[c][entry] -> channel index.
using RetaTable = std::array<std::array<uint8_t, RETA_SIZE>, config::CHANNEL_COUNTS.size()>;

constexpr RetaTable buildRetas() {
    RetaTable t{};
    for (std::size_t c = 0; c < config::CHANNEL_COUNTS.size(); c++)
        for (std::size_t i = 0; i < RETA_SIZE; i++)
            t[c][i] = static_cast<uint8_t>(i % static_cast<std::size_t>(config::CHANNEL_COUNTS[c]));
    return t;
}

constexpr RetaTable RETAS = buildRetas();

#endif // RETA_HPP
