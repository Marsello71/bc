#include "siphash_rss.h" 

#include "../../external/siphash/siphash.h"

#include <cstring>

uint32_t sipHashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint64_t key1; 
    uint64_t key2; 
    memcpy(&key1,key, sizeof(key1));
    memcpy(&key2,key + sizeof(key1), sizeof(key2));
    return  static_cast<uint32_t>(SipHash::Hash_2_4(data, length, key1, key2)); 
}