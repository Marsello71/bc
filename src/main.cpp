/**
 * @file main.cpp
 * @brief rss_sim - simulates how RSS distributes packets into DMA channels.
 *
 *   ./rss_sim config.conf
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "flow_expand.hpp"            // expandInterleaved - the packet generator
#include "rss.hpp"                    // RssConfig, generateKeys, findHash
#include "settings.hpp"               // the config file
#include "simulation.hpp"             // counting and results
#include "wrappers/simpletab_rss.h"   // simpletabInit

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "usage: rss_sim <config.conf>\n";
        return 1;
    }

    Settings s = loadSettings(argv[1]);

    RssConfig cfg;
    cfg.hash_id = findHash(s.hash);                  // hash name -> index in the hash list
    if (cfg.hash_id < 0) {                           // no hash of that name
        std::cerr << "unknown hash '" << s.hash << "'\n";
        return 1;
    }
    cfg.symmetry = Symmetry::None;                   // "none"
    if (s.symmetry == "xor")  cfg.symmetry = Symmetry::Xor;
    if (s.symmetry == "sort") cfg.symmetry = Symmetry::Sort;

    cfg.ip_only = (s.tuple == "ip");                 // true = ports are not hashed
    cfg.reta = s.reta;                               // true = RETA, false = hash % n

    const std::vector<Key> keys = generateKeys(s.keys);   // same keys on every run
    simpletabInit(keys);                                  // simpletab builds its tables from them

    std::ifstream in(s.data);                        // input dataset
    if (!in.is_open()) {
        std::cerr << s.data << ": cannot open\n";
        return 1;
    }
    std::string header;
    std::getline(in, header);                        // the first line is not a flow we need to skip it

    std::filesystem::path out_dir = std::filesystem::path(s.out).parent_path();
    if (!out_dir.empty()) {
        std::filesystem::create_directories(out_dir);   // create the output folder if missing
    }
    std::ofstream out(s.out);                        // output CSV
    if (!out.is_open()) {
        std::cerr << s.out << ": cannot open\n";
        return 1;
    }

    Simulation sim(s, cfg, keys, out);               // empty counters, writes the CSV header

    FlowPrepare prepare = [&](const Tuple &tuple, const FlowInfo &info) {
        return sim.flowStart(tuple, info);           // a flow started
    };
    PacketSink sink = [&](FlowToken flow, int64_t weight) {
        sim.packet(flow, weight);                    // a packet arrived
    };
    FlowRelease release = [&](FlowToken flow) {
        sim.flowEnd(flow);                           // a flow ended
    };

    Weighting weighting = parseWeighting(s.weighting.c_str());   // "packet" -> Weighting::Packet
    expandInterleaved(in, cfg.ip_only, weighting, prepare, sink, release);   // run everything
    return 0;
}
