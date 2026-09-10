/**
 * @file parser.hpp
 * @brief Parser utilities for the RSS analyzer.
 *
 * Marcel Koptak xkoptam00@vutbr.cz 
 */

#ifndef PARSER_HPP
#define PARSER_HPP

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "config.hpp"


/**
 * Split a CSV line into fields.
 * @param line Input CSV line
 * @return Vector of fields
 */
std::vector<std::string> splitCsvLine(const std::string& line);

/**
 * Convert textual IPv4/IPv6 representation into 16-byte array.
 * @throws std::runtime_error on parse error
 */
void ipToBytes16(const std::string& ip_str, uint8_t out[16]);

/**
 * Parse a CSV row (expected 5 fields) into a TUPLE_SIZE byte key.
 * The key layout is: src_ip(16) | src_port(2) | dst_ip(16) | dst_port(2) | protocol(1)
 */
std::array<uint8_t, TUPLE_SIZE> parseLineToTuple(const std::string& line);


/**
 * One parsed row of the 11-field flow CSV:
 *   flowstart,flowend,srcip,srcport,dstip,dstport,protocol,bytes,packets,bytes_rev,packets_rev
 *
 * `t_start` / `t_end` are seconds (any fixed origin is fine - only differences
 * and ordering are used downstream). `fwd` is the src->dst tuple in the same
 * layout as parseLineToTuple; `rev` is the dst->src tuple. `*_rev` counts are
 * zero when the flow is unidirectional.
 */
struct FlowRow {
    double  t_start;
    double  t_end;
    std::array<uint8_t, TUPLE_SIZE> fwd;
    std::array<uint8_t, TUPLE_SIZE> rev;
    int64_t packets_fwd;
    int64_t bytes_fwd;
    int64_t packets_rev;
    int64_t bytes_rev;
};

/**
 * Parse one 11-field flow CSV line into a FlowRow.
 * @throws std::runtime_error on a wrong field count or unparseable field
 */
FlowRow parseFlowLine(const std::string& line);

#endif // PARSER_HPP

