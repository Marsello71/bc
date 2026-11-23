/**
 * @file settings.hpp
 * @brief Everything the simulation can be told, read from one config file.
 *
 *   ./rss_sim config.conf
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#ifndef SETTINGS_HPP
#define SETTINGS_HPP

#include <string>
#include <vector>

struct Settings {
    std::string      data;        // input dataset (flow CSV)
    std::string      out;         // output file
    std::string      hash;        // "toeplitz", "jhash", ...
    std::string      tuple;       // "full" | "ip"
    std::string      symmetry;    // "none" | "xor" | "sort"
    bool             reta;        // "on" = true, "off" = false
    bool             scaling;     // "on" = dynamic size of window
    std::string      weighting;   // "flow" | "packet" | "byte"
    int              keys;        // number of random keys
    std::vector<int> channels;    // DMA channel counts
    long             window;      // packets per window
    long              can_size;   // number of packets per cannal
};

/// Read the config file. On any problem prints the reason and exits.
Settings loadSettings(const std::string &path);

/// Print all settings (to check what was loaded).
void printSettings(const Settings &s);

#endif // SETTINGS_HPP
