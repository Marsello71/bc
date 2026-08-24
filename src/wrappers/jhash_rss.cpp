#include "jhash_rss.h"

#include "../../external/jhash/jhash.h"

#include <cstring>

uint32_t jhashlittleRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint32_t initial = ((uint32_t)key[0])       | 
                       ((uint32_t)key[1] << 8)  | 
                       ((uint32_t)key[2] << 16) | 
                       ((uint32_t)key[3] << 24);
    return hashlittle(data, length, initial);
}