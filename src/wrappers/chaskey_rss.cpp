#include "chaskey_rss.h" 

#include "../../external/chaskey/chaskey.h"

#include <cstring>

uint32_t chaskeyRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint64_t key1; 
    uint64_t key2; 
    memcpy(&key1,key, sizeof(key1));
    memcpy(&key2,key + sizeof(key1), sizeof(key2));
    return  static_cast<uint32_t>(Chaskey::Hash_8_64(data, length, key1, key2));  
    
    // mozno nahradit za hash64 kde si viem sam dastavovat 
    // kolko rounds pojdem a porovnavat chaskey medzi sebou podla toho
}