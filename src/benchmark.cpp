/**
 * @file benchmark.cpp
 * @brief RSS hash benchmark harness: loads tuples from data/output.csv, runs
 * every registered hash algorithm over them, and writes timing + DMA channel
 * distribution to results/perf_results.csv.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "parser.hpp"
#include "wrappers/hash_register.hpp"

#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <vector>

namespace {
    constexpr std::size_t RSS_KEY_SIZE = 16;
    // Fixed key so every algorithm is tested under identical conditions and
    // the run is reproducible.
    constexpr std::array<uint8_t, RSS_KEY_SIZE> RSS_KEY = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
}

int main() {
    std::ifstream reader("data/output.csv");
    if (!reader.is_open()) {
        std::cerr << "Failed to open data/output.csv\n";
        return 1;
    }

    std::ofstream results("results/perf_results.csv");
    if (!results.is_open()) {
        std::cerr << "Failed to open results/perf_results.csv for writing\n";
        return 1;
    }

    std::vector<std::array<uint8_t, TUPLE_SIZE>> tuples;
    tuples.reserve(5000000);

    std::string line;
    std::getline(reader, line); // skip CSV header

    while (std::getline(reader, line)) {
        if (line.empty()) continue;

        try {
            tuples.push_back(parseLineToTuple(line));
        } catch (const std::exception &e) {
            std::cerr << "Skipping malformed line: " << e.what() << "\n";
        }
    }

    std::cout << "Loaded " << tuples.size() << " tuples.\n";

    results << "algorithm,tuple_count,total_time_ns,avg_time_ns";
    for (int c = 0; c < DMA; c++) {
        results << ",channel_" << c;
    }
    results << "\n";

    for (const auto &algo : hash_functions_arr) {
        std::vector<int> histogram(DMA, 0);

        auto start = std::chrono::steady_clock::now();

        for (const auto &tuple : tuples) {
            uint32_t hash = algo.fn(tuple.data(), tuple.size(), RSS_KEY.data());
            int channel = static_cast<int>(hash % DMA);
            histogram[channel]++;
        }

        auto end = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);

        double avg_ns = static_cast<double>(elapsed.count()) / static_cast<double>(tuples.size());

        results << algo.name << "," << tuples.size() << "," << elapsed.count() << "," << avg_ns;
        for (int c = 0; c < DMA; c++) {
            results << "," << histogram[c];
        }
        results << "\n";

        std::cout << algo.name << ": " << elapsed.count() << " ns total, " << avg_ns << " ns/hash\n";
    }

    return 0;
}
