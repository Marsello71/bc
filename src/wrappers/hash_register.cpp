#include "hash_register.hpp" 


const std::array<hash_algoritm, 6> hash_functions_arr = {{
        /*
        {"ascon", asconRssWrapper, false},
        {"crc32c", crc32cRssWrapper, false},
        {"nsgahash4", nsgahashRssWrapper, false},
        {"spookyhash", spookyhashRssWrapper, true},
        */
        {"toeplitz", toeplitzRssWrapper, true},
        {"jhash", jhashlittleRssWrapper, true},
        {"chaskey", chaskeyRssWrapper, true},
        {"halfsiphash", halfSipHashRssWrapper, true},
        {"siphash", sipHashRssWrapper, true},
        {"xorhash", xorHashRssWrapper, true}
}};