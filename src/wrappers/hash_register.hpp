/**
 * @file hash_register.hpp
 * @brief Central list of all RSS hash wrappers the harness runs.
 * Marcel Koptak xkoptam00@vutbr.cz
 */

 #ifndef HASH_REGISTER_HPP
 #define HASH_REGISTER_HPP

#include <array>

#include "ascon_rss.h"
#include "chaskey_rss.h"
#include "siphash_rss.h"
#include "halfsiphash_rss.h"
#include "spookyhash_rss.h"
#include "jhash_rss.h"
#include "crc32c_rss.h"

using RssHashFunction  = uint32_t (*)(const uint8_t *, size_t, const uint8_t *);

typedef struct  
{
   const char *name;
   RssHashFunction fn;
   bool keyed;
}hash_algoritm;

extern const std::array<hash_algoritm, 7> hash_functions_arr;

 #endif //HASH_REGISTER_HPP
