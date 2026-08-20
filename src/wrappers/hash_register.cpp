#include "hash_register.hpp" 


const std::array<hash_algoritm, 5> hash_functions_arr = {{
        {"chaskey", chaskeyRssWrapper},
        {"halfsiphash", halfSipHashRssWrapper},
        {"siphash", sipHashRssWrapper},
        {"spookyhash", spookyhashRssWrapper},
        {"ascon", asconRssWrapper}
}};