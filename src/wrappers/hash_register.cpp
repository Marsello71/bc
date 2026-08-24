#include "hash_register.hpp" 


const std::array<hash_algoritm, 7> hash_functions_arr = {{
        {"jhash", jhashlittleRssWrapper},
        {"crc32c", crc32cRssWrapper},
        {"chaskey", chaskeyRssWrapper},
        {"halfsiphash", halfSipHashRssWrapper},
        {"siphash", sipHashRssWrapper},
        {"spookyhash", spookyhashRssWrapper},
        {"ascon", asconRssWrapper}
}};