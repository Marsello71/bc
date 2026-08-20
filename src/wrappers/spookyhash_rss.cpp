#include "spookyhash_rss.h" 

#include "../../external/spookyhash/spookyV2.h"

#include <cstring>

uint32_t spookyhashRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint32_t seed; 
    memcpy(&seed,key, sizeof(seed));
    return SpookyHash::Hash32(data,length,seed);
}