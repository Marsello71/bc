#include "ascon_rss.h"

#include "../../external/ascon/ascon.h"


uint32_t asconRssWrapper(const uint8_t *data, size_t length, const uint8_t */*key*/ ) {
    return asconHashFunction(data,length);
}