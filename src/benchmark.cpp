/**
 * @file benchmark.cpp
 * @brief RSS hash benchmark harness. Loads tuples, runs every registered hash
 * over them, and writes the per-window DMA channel distribution metrics.
 *
 * Two input formats, auto-detected from the CSV header field count:
 *   - simple tuple: 3- or 5-field tuple CSV, one row = one packet
 *   - flow:         11-field CESNET flow CSV, expanded to real per-packet load
 *                   (see flow_expand.hpp), weighted by flow / packet / byte
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
#include <string>

#include "parser.hpp"
#include "wrappers/hash_register.hpp"
#include "results_writer.hpp"
#include "config.hpp"
#include "transform.hpp"
#include "metrics.hpp"
#include "flow_expand.hpp"
#include "reta.hpp"

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

using KeySet = std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>>;

// simple-tuple path: one CSV row == one packet, always "flow" weighting.
// all tuples fit in RAM, so we can loop over them once per (algo, key) pair.
static void runSimpleTupleMode(std::istream &reader, ResultsWriter &writer,
                               Symmetry sym, size_t offset, const KeySet &keys) {
    std::vector<std::array<uint8_t, TUPLE_SIZE>> tuples;
    tuples.reserve(5000000);

    std::string line;
    while (std::getline(reader, line)) {
        if (line.empty()) continue;
        try {
            tuples.push_back(parseLineToTuple(line));
        } catch (const std::exception &e) {
            std::cerr << "Skipping malformed line: " << e.what() << "\n";
        }
    }
    std::cout << "Loaded " << tuples.size() << " tuples.\n";

    for (const auto &algo : hash_functions_arr) {
        for (std::size_t i = 0; i < config::NUM_KEYS; i++) {
            std::vector<std::vector<int64_t>> histograms(config::CHANNEL_COUNTS.size());
            for (std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++) {
                histograms[j] = std::vector<int64_t>(config::CHANNEL_COUNTS[j], 0);
            }

            size_t count = 0;
            for (const auto &tuple : tuples) {
                count++;
                auto keyed_tuple = applySymmetry(sym, tuple, offset);
                uint32_t hash = algo.fn(keyed_tuple.data(), TUPLE_SIZE, keys[i].data());

                for (std::size_t j = 0; j < config::CHANNEL_COUNTS.size(); j++) {
                    histograms[j][hash % config::CHANNEL_COUNTS[j]]++;

                    if (count % config::WINDOW_SIZE == 0) {
                        int n = config::CHANNEL_COUNTS[j];
                        ResultRow row{
                            algo.name, sym, "flow",
                            static_cast<long>(count / config::WINDOW_SIZE),
                            static_cast<int>(i), n,
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
} 

// flow path: the packet stream is too big to store, so flow_expand feeds us one
// packet at a time and we count it into EVERY (algo, key, channel-count) bucket
// in a single pass. that's why the histogram has so many dimensions - all of them
// have to be live at once.
static void runFlowMode(std::istream &reader, ResultsWriter &writer,
                        Symmetry sym, size_t offset, Weighting weighting,
                        const KeySet &keys) {
    const auto &CC = config::CHANNEL_COUNTS;
    const std::size_t NA = hash_functions_arr.size();
    const std::size_t NK = config::NUM_KEYS;
    const std::size_t NC = config::CHANNEL_COUNTS.size();

#if RETA_ON

    std::vector<int64_t> buckets(NA * NK * RETA_SIZE, 0);
    std::vector<int64_t> chan;
    chan.reserve(static_cast<std::size_t>(CC.back()));
#else
    using Hist = std::vector<int64_t>;
    std::vector<std::vector<std::vector<Hist>>> histograms(NA, std::vector<std::vector<Hist>>(NK, std::vector<Hist>(NC)));

    for (std::size_t a = 0; a < NA; a++)
        for (std::size_t k = 0; k < NK; k++)
            for (std::size_t c = 0; c < NC; c++)
                histograms[a][k][c].assign(CC[c], 0);
#endif

    int64_t packet_count = 0;
    int64_t window_sum_total = 0;   // sum of weights in the current window (metric denominator)
    long    window_index = 0;

    // runs for every packet flow_expand produces, in time order. this lambda is
    // effectively the inner loop body - flow_expand is the "for each packet" around it.
    PacketSink sink = [&](const std::array<uint8_t, TUPLE_SIZE> &tuple, int64_t weight) {
        packet_count++;
        window_sum_total += weight;

        // hash this one packet with every algo+key, drop it in the right channel
        for (std::size_t a = 0; a < NA; ++a) {
            for (std::size_t k = 0; k < NK; ++k) {
                uint32_t h = hash_functions_arr[a].fn(tuple.data(), TUPLE_SIZE, keys[k].data());
#if RETA_ON
                // one increment, no division - the channel counts come later
                buckets[(a * NK + k) * RETA_SIZE + (h & (RETA_SIZE - 1))] += weight;
#else
                for (std::size_t c = 0; c < NC; ++c) {
                    histograms[a][k][c][h % CC[c]] += weight;
                }
#endif
            }
        }

        // window full -> one CSV row per (algo, key, channel-count), then reset
        if (packet_count % config::WINDOW_SIZE == 0) {
            window_index++;
#if RETA_ON
            for (std::size_t a = 0; a < NA; ++a) {
                for (std::size_t k = 0; k < NK; ++k) {
                    const std::size_t base = (a * NK + k) * RETA_SIZE;

                    // fold the RETA entries of this (algo, key) into channel loads:
                    // every entry adds its weight to the channel the table points at
                    for (std::size_t c = 0; c < NC; ++c) {
                        int n = CC[c];
                        chan.assign(static_cast<std::size_t>(n), 0);
                        for (std::size_t i = 0; i < RETA_SIZE; ++i)
                            chan[RETAS[c][i]] += buckets[base + i];

                        ResultRow row{
                            hash_functions_arr[a].name, sym, weightingName(weighting),
                            window_index,
                            (int)k,
                            n,
                            computeOverThreshold(chan, window_sum_total, n),
                            computeFairness    (chan, window_sum_total, n),
                            computeChi         (chan, window_sum_total, n)
                        };
                        writer.write(row);
                    }

                    // only now, once every channel count has been folded out of them
                    std::fill(buckets.begin() + base, buckets.begin() + base + RETA_SIZE, 0);
                }
            }
#else
            for (std::size_t a = 0; a < NA; ++a) {
                for (std::size_t k = 0; k < NK; ++k) {
                    for (std::size_t c = 0; c < NC; ++c) {
                        int n = CC[c];
                        ResultRow row{
                            hash_functions_arr[a].name, sym, weightingName(weighting),
                            window_index,
                            (int)k,
                            n,
                            computeOverThreshold(histograms[a][k][c], window_sum_total, n),
                            computeFairness    (histograms[a][k][c], window_sum_total, n),
                            computeChi         (histograms[a][k][c], window_sum_total, n)
                        };
                        writer.write(row);
                        std::fill(histograms[a][k][c].begin(), histograms[a][k][c].end(), 0);
                    }
                }
            }
#endif
            window_sum_total = 0;
        }
    };

    expandInterleaved(reader, sym, offset, weighting, sink);
    std::cout << "Streamed " << packet_count << " packets, " << window_index << " full windows.\n";
}

// number of comma-separated fields in a CSV line (commas inside quotes don't count)
static int countFields(const std::string &line) {
    if (line.empty()) return 0;
    int n = 1;
    bool q = false;
    for (char c : line) {
        if (c == '"') q = !q;
        else if (c == ',' && !q) n++;
    }
    return n;
}

int main(int argc, char *argv[]) {
    if (argc < 5 || argc > 6) {
        std::cerr <<
            "usage: benchmark <dataset.csv> <output.csv> <sym 0|1|2> <short_tuple 0|1> [weighting flow|packet|byte]\n"
            "  simple-tuple CSV (3 or 5 fields): 4th arg picks the short-tuple offset, weighting is ignored\n"
            "  flow CSV (11 fields): 5th arg required, picks how channel load is counted\n";
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

    std::string header;
    std::getline(reader, header);          // consume header, use it to pick the format
    int fields = countFields(header);

    auto keys = getKeys();
    
    if (fields == 11) {
        if (argc != 6) {
            std::cerr << "flow CSV needs the weighting argument (flow|packet|byte)\n";
            return 1;
        }
        Weighting weighting = parseWeighting(argv[5]);
        size_t offset = 18;                // flow tuples are always the 5-tuple layout
        runFlowMode(reader, writer, sym, offset, weighting, keys);
    } else if (fields == 3 || fields == 5) {
        bool short_tuple = std::atoi(argv[4]);
        size_t offset = short_tuple ? 16 : 18;
        runSimpleTupleMode(reader, writer, sym, offset, keys);
    } else {
        std::cerr << "unrecognised CSV: header has " << fields << " fields\n";
        return 1;
    }

    return 0;
}
