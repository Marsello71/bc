#include "simpletab_rss.h"

#include "../../external/simpletab/hashing.h"

#include <cstring>
#include <random>
#include <cassert>

namespace {
    /// One key's tables: [byte position][byte value] -> 32-bit random word.
    using SimpleTabSet = std::array<std::array<uint32_t, 256>, TUPLE_SIZE>;

    /// One set per key, filled once by simpletabInit(); indexed by key order.
    std::vector<SimpleTabSet> g_tables;

    /// Address of keys[0]; the wrapper derives the key index from its key pointer.
    const uint8_t *g_base = nullptr;
}

uint32_t simpletabRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    size_t index = (key - g_base) / config::RSS_KEY_SIZE;
    assert(index < g_tables.size());
    const SimpleTabSet &tab = g_tables[index];
    uint32_t hash = 0;
    assert( length <= TUPLE_SIZE);

    for(size_t i = 0; i < length; i++) { 
        hash = hash ^ tab[i][data[i]];
    }
    return hash;
}

void simpletabInit(const std::vector<std::array<uint8_t, config::RSS_KEY_SIZE>> &keys){
    g_tables.clear();
    g_tables.reserve(keys.size());
    g_base = keys[0].data();

    for(std::size_t i = 0; i < keys.size(); i++) {
        uint32_t seed_array[config::RSS_KEY_SIZE / 4];
        memcpy(seed_array, keys[i].data(), config::RSS_KEY_SIZE);
        std::seed_seq seq(seed_array, seed_array + config::RSS_KEY_SIZE / 4);
        std::mt19937 generator(seq);

        SimpleTabSet set;
        for(std::size_t j = 0; j < TUPLE_SIZE; j++) {
            for(std::size_t k = 0; k < 256; k++) {
                set[j][k] = generator();
            }
        }
        g_tables.push_back(set);
    }
}