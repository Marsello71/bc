#include "crc32c_rss.h"

#include "../../external/crc32c/crc32c.h"


uint32_t crc32cRssWrapper(const uint8_t *data, size_t length, const uint8_t */*key*/ ) {
    return crc32c_sw(0, data, length);
}