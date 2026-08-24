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
#include <random>
#include <vector>

namespace {
    constexpr uint32_t KEY_SEED =  65536;
    constexpr std::size_t NUM_KEYS = 32;
    constexpr std::size_t RSS_KEY_SIZE = 16;
}

std::vector<std::array<uint8_t, RSS_KEY_SIZE>> getKeys() { 
    std::vector<std::array<uint8_t, RSS_KEY_SIZE>>  keys;
    std::mt19937 generator(KEY_SEED);
    std::uniform_int_distribution<int> distribution(0, 255);

    for(std::size_t i = 0; i < NUM_KEYS; i++) {
        std::array<uint8_t, RSS_KEY_SIZE> key;
        for(std::size_t j = 0; j < RSS_KEY_SIZE; j++) { 
            key[j] = static_cast<uint8_t>(distribution(generator));
        }
        keys.push_back(key);
    }
    return keys;
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

    results << "algorithm,tuple_count,total_time_ns,avg_time_ns,key_id";
    for (int c = 0; c < DMA; c++) {
        results << ",channel_" << c;
    }
    results << "\n";

    std::vector<std::array<uint8_t, RSS_KEY_SIZE>> keys = getKeys();

    for (const auto &algo : hash_functions_arr) {

        std::size_t key_count = algo.keyed ? NUM_KEYS : 1;
        for(std::size_t i = 0; i < key_count; i++) {
            std::vector<int> histogram(DMA, 0);

            auto start = std::chrono::steady_clock::now();
            for (const auto &tuple : tuples) {
                uint32_t hash = algo.fn(tuple.data(), tuple.size(), keys[i].data());
                int channel = static_cast<int>(hash % DMA);
                histogram[channel]++;
            }
            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            double avg_ns = static_cast<double>(elapsed.count()) / static_cast<double>(tuples.size());
            results << algo.name << "," << tuples.size() << "," << elapsed.count() << "," << avg_ns << ",";
            if (algo.keyed) results << i; else results << "N/A";
            for (int c = 0; c < DMA; c++) {
                results << "," << histogram[c];
            }
            results << "\n";
        }
    }

    return 0;
}
