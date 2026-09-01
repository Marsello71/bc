/**
 * @file benchmark.cpp
 * @brief RSS hash benchmark harness: loads tuples from data/output.csv, runs
 * every registered hash algorithm over them, and writes timing + DMA channel
 * distribution to results/perf_results.csv.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "parser.hpp"
#include "wrappers/hash_register.hpp"
#include "results_writer.hpp"
#include "config.hpp"
#include "transform.hpp"
#include "metrics.hpp"

std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>> getKeys() { 
    std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>>  keys;
    std::mt19937 generator(config::KEY_SEED);
    std::uniform_int_distribution<int> distribution(0, 255);

    for(std::size_t i = 0; i < config::NUM_KEYS; i++) {
        std::array<uint8_t, config::RSS_KEY_SIZE> key;
        for(std::size_t j = 0; j < config::RSS_KEY_SIZE; j++) { 
            key[j] = static_cast<uint8_t>(distribution(generator));
        }
        keys.push_back(key);
    }
    return keys;
}


int main(int argc, char* argv[]) {
    if(argc != 4) {
    std::cerr << "tu run the analysis main needs 4 argumenst: [dataset] [output_file] [symetri 2/1/0]\n";
        return 1;
    }

    std::ifstream reader(argv[1]);
    if (!reader.is_open()) {
        std::cerr << "Failed to open file for reading\n";
        return 1;
    }

    std::ofstream out(argv[2]);
    if (!out.is_open()) {
        std::cerr << "Failed to open file for writing\n";
        return 1;
    }
    ResultsWriter writer(out);
    Symmetry sym = parseSymmetry(argv[3]);

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
    auto keys = getKeys();

    for (const auto &algo : hash_functions_arr) {
        for(std::size_t i = 0; i < config::NUM_KEYS; i++) {
            std::vector<std::vector<int>> histograms(config::CHANNEL_COUNTS.size());
            
            for(std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++){ 
                histograms[j] = std::vector<int>(config::CHANNEL_COUNTS[j], 0);
            }

            size_t count = 0;
            for (const auto &tuple : tuples) {
                count++;
                auto keyed_tuple = applySymmetry(sym, tuple);
                uint32_t hash = algo.fn(keyed_tuple.data(), TUPLE_SIZE, keys[i].data());

                for(std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++) {
                    histograms[j][hash % config::CHANNEL_COUNTS[j]]++;

                    if(count % config::WINDOW_SIZE == 0) {
                        int n = config::CHANNEL_COUNTS[j];
                        ResultRow row{
                            algo.name, sym,
                            static_cast<long>(count / config::WINDOW_SIZE),
                            static_cast<int>(i),
                            n,
                            computeOverThreshold(histograms[j], config::WINDOW_SIZE, n),
                            computeFairness(histograms[j], config::WINDOW_SIZE, n),
                            computeChi(histograms[j], config::WINDOW_SIZE, n)
                        };
                        writer.write(row);
                        std::fill(histograms[j].begin(), histograms[j].end(), 0);
                    }
                }
            }
        }
    }

    return 0;
}
