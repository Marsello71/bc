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

/// Number of DMA channels (default)
constexpr int DMA = 8;

/// Size of the generated key in bytes
constexpr std::size_t TUPLE_SIZE = 37;

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

#endif // PARSER_HPP

