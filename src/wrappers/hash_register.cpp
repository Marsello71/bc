#include "hash_register.hpp" 


const std::array<hash_algoritm, 8> hash_functions_arr = {{
        {"ascon", asconRssWrapper, false},
        {"crc32c", crc32cRssWrapper, false},
        {"toeplitz", toeplitzRssWrapper, false},
        {"jhash", jhashlittleRssWrapper, true},
        {"chaskey", chaskeyRssWrapper, true},
        {"halfsiphash", halfSipHashRssWrapper, true},
        {"siphash", sipHashRssWrapper, true},
        {"spookyhash", spookyhashRssWrapper, true}
}};