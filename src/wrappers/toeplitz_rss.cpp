#include "toeplitz_rss.h"

#include "../../external/toeplitz/toeplitz_dpdk_reference.h"

#include <cstring>

uint32_t toeplitzRssWrapper(const uint8_t *data, size_t /*length*/ , const uint8_t *key) {
    uint32_t input_tuple[9];
    for( int i = 0; i < 9; i++){
        input_tuple[i] = load32(&data[i*4]);
    }
    
    
    return rte_softrss(input_tuple, TOEPLITZ_WORDS, key);
}
