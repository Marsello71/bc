#include "halfsiphash_rss.h" 

#include "../../external/siphash/siphash.h"

#include <cstring>

uint32_t halfSipHashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint64_t key1; 
    memcpy(&key1,key, sizeof(key1));
    return  HalfSipHash::Hash_2_4(data, length, key1);
}