#include "parser.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sstream>

std::vector<std::string> splitCsvLine(const std::string& line) {
    std::vector<std::string> out;
    std::string part;
    bool in_quotes = false;

    for (size_t i = 0; i < line.size(); i++) {
        char c = line[i];
        if (c == '"') {
            in_quotes = !in_quotes;
        } else if (c == ',' && !in_quotes) {
            out.push_back(part);
            part.clear();
        } else {
            part.push_back(c);
        }
    }
    out.push_back(part);
    return out;
}

void ipToBytes16(const std::string& ip_str, uint8_t out[16]) {
    if (inet_pton(AF_INET6, ip_str.c_str(), out) == 1) {
        return;
    }
    throw std::runtime_error("invalid IP address " + ip_str);
}

std::array<uint8_t, TUPLE_SIZE> parseLineToTuple(const std::string& line) {
    std::vector<std::string> fields = splitCsvLine(line);
    if (fields.size() != 5) {
        throw std::runtime_error("The row has not 5 fields");
    }

    std::array<uint8_t, TUPLE_SIZE> key = {};
    int offset = 0;

    uint8_t src_IP[16];
    ipToBytes16(fields[0], src_IP);
    std::memcpy(key.data(), src_IP, 16);
    offset += 16;

    uint16_t src_port = htons(static_cast<uint16_t>(std::stoi(fields[1])));
    std::memcpy(key.data() + offset, &src_port, 2);
    offset += 2;

    uint8_t dst_IP[16];
    ipToBytes16(fields[2], dst_IP);
    std::memcpy(key.data() + offset, dst_IP, 16);
    offset += 16;

    uint16_t dst_port = htons(static_cast<uint16_t>(std::stoi(fields[3])));
    std::memcpy(key.data() + offset, &dst_port, 2);
    offset += 2;

    uint8_t protocol = static_cast<uint8_t>(std::stoi(fields[4]));
    std::memcpy(key.data() + offset, &protocol, 1);

    return key;
}

