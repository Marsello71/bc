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

// How a packet counts toward its channel's load.
//   Flow   - one unit per flow direction (reproduces the simple-tuple view)
//   Packet - one unit per packet
//   Byte   - the packet's size in bytes (bandwidth / DMA load)
enum class Weighting { Flow = 0, Packet = 1, Byte = 2 };

Weighting parseWeighting(const char *arg);   // "flow"/"packet"/"byte" or "0"/"1"/"2"
const char *weightingName(Weighting w);      // lowercase name for the CSV column

// The callback expandInterleaved calls for every packet it produces. Think of it
// as the body of the per-packet loop: `tuple` is already symmetrised, `weight` is
// what to add to the channel bucket. We use a callback (not a returned vector)
// because the packet count runs into the billions - nothing may be kept around.
using PacketSink =
    std::function<void(const std::array<uint8_t, TUPLE_SIZE> &tuple, int64_t weight)>;

// Read 11-field flow rows from `in` (header already consumed), blow each flow up
// into its real packets, interleave packets of concurrent flows by time, apply
// `sym`/`offset`, and call `sink` once per packet in time order.
// Precondition: rows are sorted by flow start time.
void expandInterleaved(std::istream &in, Symmetry sym, size_t offset,
                       Weighting weighting, const PacketSink &sink);

#endif // FLOW_EXPAND_HPP
