/**
 * @file hash_register.hpp
 * @brief Central list of all RSS hash wrappers the harness runs.
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef HASH_REGISTER_HPP
 #define HASH_REGISTER_HPP

#include <array>

#include "chaskey_rss.h"
#include "siphash_rss.h"
#include "halfsiphash_rss.h"
#include "jhash_rss.h"
#include "toeplitz_rss.h"
#include "xorhash_rss.h"

using RssHashFunction  = uint32_t (*)(const uint8_t *, size_t, const uint8_t *);

typedef struct  
{
   const char *name;
   RssHashFunction fn;
}hash_algoritm;

extern const std::array<hash_algoritm, 6> hash_functions_arr;

 #endif //HASH_REGISTER_HPP
