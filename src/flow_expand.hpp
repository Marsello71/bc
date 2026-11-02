/**
 * @file flow_expand.hpp
 * @brief Turn a flow CSV into a time-ordered stream of per-packet tuples.
 *
 * The input has one row per flow (start, end, 5-tuple, packet and byte counts),
 * but RSS sees individual packets. expandInterleaved() rebuilds the packet
 * stream: every flow's packets are spread evenly over its lifetime and all
 * flows are merged so that packets come out in global time order, the way
 * they would reach the NIC.
 *
 * The caller never sees the packets themselves. It plugs in three callbacks:
 *
 *   prepare(tuple)        once, when a flow direction starts -> returns a token
 *   sink(token, weight)   once per packet of that direction, in time order
 *   release(token)        once, after the direction's last packet
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

/// What one packet adds to its channel's load.
///   Flow   - every flow direction is collapsed to ONE packet of weight 1
///            (counts flows, so elephant flows weigh the same as mice)
///   Packet - every real packet has weight 1
///   Byte   - every packet weighs the flow's mean packet size in bytes
enum class Weighting { Flow = 0, Packet = 1, Byte = 2 };

Weighting parseWeighting(const char *arg);   // "flow"/"packet"/"byte" or "0"/"1"/"2"
const char *weightingName(Weighting w);      // lowercase name for the CSV column


/// Ticket number of a live flow direction - a plain integer, not a pointer.
/// prepare() hands it out, flow_expand carries it inside the flow's heap entry
/// and passes it back with every packet, so the caller can find whatever it
/// stored for that flow. Tokens are recycled after release(), so at any moment
/// one token belongs to at most one live direction. uint32_t because up to
/// ~800k directions are live at once (more than uint16_t can count).
using FlowToken = uint32_t;


/// What prepare() knows about a flow direction besides its tuple - exactly the
/// numbers flow_expand will use to emit it. The hash modes ignore it (a real NIC
/// can't know any of this when the first packet arrives); the JSQ oracle uses it
/// to know in advance how fast the flow is going to send.
struct FlowInfo {
    double  t_start;
    double  t_end;
    int64_t packets;   // packets this direction will emit (always 1 with Flow weighting)
    int64_t weight;    // weight of each of those packets (see Weighting)
};

/// Called once per flow direction, with its (already symmetrised) tuple.
/// The caller does the expensive per-flow work here (hashing) and returns the
/// token under which it stored the result.
using FlowPrepare =
    std::function<FlowToken(const std::array<uint8_t, TUPLE_SIZE> &tuple, const FlowInfo &info)>;


/// Called once per packet. Gets only the flow's token and the packet's weight -
/// no tuple, no timestamp. Time is used inside flow_expand for ordering only.
using PacketSink = std::function<void(FlowToken token, int64_t weight)>;


/// Called after the direction's last packet: the token may be handed out again.
using FlowRelease = std::function<void(FlowToken token)>;

/// Read flow rows from `in` and drive the three callbacks.
/// Precondition: rows are sorted by flow start time.
void expandInterleaved(std::istream &in, Symmetry sym, size_t offset,
                       Weighting weighting, const FlowPrepare &prepare,
                       const PacketSink &sink, const FlowRelease &release);

#endif // FLOW_EXPAND_HPP
