#include "parser.hpp"

#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include <sstream>
#include <ctime> 

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

std::array<uint8_t, TUPLE_SIZE> tuple37(std::vector<std::string> fields) {
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

    return key;
}

std::array<uint8_t, TUPLE_SIZE> tuple32(std::vector<std::string> fields) {
    std::array<uint8_t, TUPLE_SIZE> key = {};  // inicializovane cele na 0
    int offset = 0;

    uint8_t src_IP[16];
    ipToBytes16(fields[0], src_IP);
    std::memcpy(key.data(), src_IP, 16);
    offset += 16;

    uint8_t dst_IP[16];
    ipToBytes16(fields[1], dst_IP);
    std::memcpy(key.data() + offset, dst_IP, 16);
    offset += 16;

    // uint16_t vlan_id = htons(static_cast<uint16_t>(std::stoi(fields[2])));
    // std::memcpy(key.data() + offset, &vlan_id, 2);
    // offset += 2;

    return key; 
}


std::array<uint8_t, TUPLE_SIZE> parseLineToTuple(const std::string& line) {
    std::vector<std::string> fields = splitCsvLine(line);

    if (fields.size() == 5) {
        return tuple37(fields);
    } else if( fields.size() == 3) {
        return tuple32(fields);
    }  else throw std::runtime_error("The row has not correct number of fields");

    return {};
}


// "YYYY-MM-DD HH:MM:SS.fffffffff" -> seconds of day (as double, keeps the 9-digit
// fraction). Downstream only needs differences and ordering, so the date is
// dropped - assumes the whole dataset is within one day.
static double parseFlowTimestamp(const std::string& ts) {
    size_t sp = ts.find(' ');
    std::string t = ts.substr(sp + 1);          // "HH:MM:SS.fffffffff"
    int hh = std::stoi(t.substr(0, 2));
    int mm = std::stoi(t.substr(3, 2));
    double ss = std::stod(t.substr(6));         // "SS.fffffffff"
    return hh * 3600.0 + mm * 60.0 + ss;
}

// Column order:
//   0 flowstart  1 flowend  2 srcip  3 srcport  4 dstip  5 dstport
//   6 protocol   7 bytes    8 packets  9 bytes_rev  10 packets_rev
FlowRow parseFlowLine(const std::string& line) {
    std::vector<std::string> f = splitCsvLine(line);
    if (f.size() != 11) {
        throw std::runtime_error("flow row does not have 11 fields");
    }
    FlowRow row;
    row.t_start     = parseFlowTimestamp(f[0]);
    row.t_end       = parseFlowTimestamp(f[1]);
    if (row.t_end < row.t_start) row.t_end = row.t_start; // midnight wrap / bad data -> instant flow

#if IP_ONLY_ON
    row.fwd         = tuple32({ f[2],f[4]});   // src -> dst
    row.rev         = tuple32({ f[4],f[2]});   // dst -> src (swapped for the reverse packets)
#else 
    row.fwd         = tuple37({ f[2], f[3], f[4], f[5], f[6] });   // src -> dst
    row.rev         = tuple37({ f[4], f[5], f[2], f[3], f[6] });   // dst -> src (swapped for the reverse packets)
#endif

    row.bytes_fwd   = std::stoll(f[7]);
    row.packets_fwd = std::stoll(f[8]);
    row.bytes_rev   = std::stoll(f[9]);
    row.packets_rev = std::stoll(f[10]);
    return row;
}




