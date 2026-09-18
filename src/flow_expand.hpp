/**
 * @file flow_expand.hpp
 * @brief Turn a flow CSV into a time-ordered stream of per-packet tuples.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef FLOW_EXPAND_HPP
#define FLOW_EXPAND_HPP

#include <array>
#include <cstdint>
#include <functional>
#include <istream>

#include "config.hpp"
#include "transform.hpp"

enum class Weighting { Flow = 0, Packet = 1, Byte = 2 };

Weighting parseWeighting(const char *arg);   // "flow"/"packet"/"byte" or "0"/"1"/"2"
const char *weightingName(Weighting w);      // lowercase name for the CSV column


using FlowToken = uint32_t;


using FlowPrepare =
    std::function<FlowToken(const std::array<uint8_t, TUPLE_SIZE> &tuple)>;


using PacketSink = std::function<void(FlowToken token, int64_t weight)>;


using FlowRelease = std::function<void(FlowToken token)>;

void expandInterleaved(std::istream &in, Symmetry sym, size_t offset,
                       Weighting weighting, const FlowPrepare &prepare,
                       const PacketSink &sink, const FlowRelease &release);

#endif // FLOW_EXPAND_HPP
