#include "hash_register.hpp" 


const std::array<hash_algoritm, 8> hash_functions_arr = {{
        {"toeplitz", toeplitzRssWrapper},
        {"jhash", jhashlittleRssWrapper},
        {"chaskey", chaskeyRssWrapper},
        {"halfsiphash", halfSipHashRssWrapper},
        {"simpletab",simpletabRssWrapper},
        {"multiplyshift", multiplyShiftRssWrapper},
        {"siphash", sipHashRssWrapper},
        {"xorhash", xorHashRssWrapper}
}};