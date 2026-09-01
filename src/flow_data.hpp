/**
 * @file tuple.hpp
 * @brief Byte layout of the 37-byte flow tuple that gets hashed.
 *
 * The tuple is the flat byte buffer the parser fills and the hash wrappers
 * consume. Layout (network byte order for the ports):
 *
 *   offset  0 .. 15   srcip     (16 B, IPv6 or IPv4-mapped ::ffff:a.b.c.d)
 *   offset 16 .. 17   srcport   ( 2 B, htons)
 *   offset 18 .. 33   dstip     (16 B)
 *   offset 34 .. 35   dstport   ( 2 B, htons)
 *   offset 36         protocol  ( 1 B)
 *
 * Keep this in sync with results/README.md and parser.cpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef TUPLE_HPP
#define TUPLE_HPP

#include <cstddef>
#include <cstdint>

namespace tuple_layout {

constexpr std::size_t SRCIP_OFFSET   = 0;
constexpr std::size_t SRCIP_LEN      = 16;

constexpr std::size_t SRCPORT_OFFSET = 16;
constexpr std::size_t SRCPORT_LEN    = 2;

constexpr std::size_t DSTIP_OFFSET   = 18;
constexpr std::size_t DSTIP_LEN      = 16;

constexpr std::size_t DSTPORT_OFFSET = 34;
constexpr std::size_t DSTPORT_LEN    = 2;

constexpr std::size_t PROTOCOL_OFFSET = 36;
constexpr std::size_t PROTOCOL_LEN    = 1;

} // namespace tuple_layout

/// Total tuple size in bytes (5-tuple, IPv6-wide).
constexpr std::size_t TUPLE_SIZE = 37;

#endif // TUPLE_HPP
