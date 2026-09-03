#include "crc32c_rss.h"

#include "../../external/crc32c/crc32c.h"

uint32_t crc32cRssWrapper(const uint8_t *data, size_t length, const uint8_t *key) {
    uint32_t init   = load32(&key[0]);
    return crc32c(init, data, length);
}
