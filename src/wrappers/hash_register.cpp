#include "hash_register.hpp" 


const std::array<hash_algoritm, 6> hash_functions_arr = {{
        {"toeplitz", toeplitzRssWrapper},
        {"jhash", jhashlittleRssWrapper},
        {"chaskey", chaskeyRssWrapper},
        {"halfsiphash", halfSipHashRssWrapper},
        {"crc32c", crc32cRssWrapper},
        {"xorhash", xorHashRssWrapper}
}};