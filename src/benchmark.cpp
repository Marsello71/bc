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
#include <algorithm>

namespace {
    constexpr uint32_t KEY_SEED =  65536;
    constexpr std::size_t NUM_KEYS = 32;
    constexpr std::size_t RSS_KEY_SIZE = 16;
    constexpr std::array<int, 6> CHANNEL_COUNTS = {4,8,16,32,64,128};
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

double computeFairness(std::vector<int> histogram,double tuple_count) {
    if (histogram.empty() || tuple_count == 0) {
        return 0.0; 
    }
    double sum = 0.0, sum_sq = 0.0;
    for (double x : histogram) {
        sum += x;
        sum_sq += x * x;
    }
    return( sum * sum ) / ( sum_sq * histogram.size());
}


double computeChi(const std::vector<int>& histogram, double tuple_count) {
    if (histogram.empty() || tuple_count == 0) {
        return 0.0;
    }

    double n = histogram.size();
    double expected = tuple_count / n;

    double chi2 = 0.0;
    for (int x : histogram) {
        double diff = x - expected;
        chi2 += (diff * diff) / expected;
    }

    return chi2 / tuple_count;
}


double computeMinMaxDiff(const std::vector<int>& histogram, std::size_t tuple_count) {
    if (histogram.empty() || tuple_count == 0) {
        return 0.0; 
    }
    int min_val = *std::min_element(histogram.begin(), histogram.end());
    int max_val = *std::max_element(histogram.begin(), histogram.end());

    int diff = max_val - min_val;

    double percentage = (diff / static_cast<double>(tuple_count)) * 100.0;

    return percentage;
}

int main(int argc, char* argv[]) {
    if(argc != 3) {
    std::cerr << "tu run the analysis main needs 3 argumenst: [dataset] [output_file]\n";
        return 1;
    }

    std::ifstream reader(argv[1]);
    if (!reader.is_open()) {
        std::cerr << "Failed to open file for reading\n";
        return 1;
    }

    std::ofstream results(argv[2]);
    if (!results.is_open()) {
        std::cerr << "Failed to open file for writing\n";
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

    results << "algorithm,tuple_count,avg_time_ns,key_id,num_channels,fairness,chi,min_max_diff";
    results << "\n";

    std::vector<std::array<uint8_t, RSS_KEY_SIZE>> keys = getKeys();

    for (const auto &algo : hash_functions_arr) {
        std::size_t key_count = algo.keyed ? NUM_KEYS : 1;

        for(std::size_t i = 0; i < key_count; i++) {
            std::vector<std::vector<int>> histograms(CHANNEL_COUNTS.size());
            
            for(std::size_t j = 0; j < CHANNEL_COUNTS.size(); j++){ 
                histograms[j] = std::vector<int>(CHANNEL_COUNTS[j], 0);
            }

            auto start = std::chrono::steady_clock::now();
            for (const auto &tuple : tuples) {
                uint32_t hash = algo.fn(tuple.data(), tuple.size(), keys[i].data());

                for(std::size_t j = 0; j < CHANNEL_COUNTS.size(); j++) {
                    int channel = static_cast<int>(hash % CHANNEL_COUNTS[j]);
                    histograms[j][channel]++;
                }
            }

            auto end = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            double avg_ns = static_cast<double>(elapsed.count()) / static_cast<double>(tuples.size());

            for(std::size_t j = 0; j < CHANNEL_COUNTS.size(); j++) {
                double fair    = computeFairness(histograms[j],tuples.size());
                double chi     = computeChi(histograms[j],tuples.size());
                double max_min = computeMinMaxDiff(histograms[j],tuples.size());

                results << algo.name << "," << tuples.size() << "," << avg_ns << ",";
                if (algo.keyed) results << i; else results << "N/A";
                results << "," << CHANNEL_COUNTS[j] << "," << fair << "," << chi << "," << max_min  << "\n";
            }
        }
    }

    return 0;
}

