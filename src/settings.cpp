/**
 * @file settings.cpp
 * @brief Reading the config file. See settings.hpp.
 *
 * Marcel Koptak xkoptam00@vutbr.cz
 */

#include "settings.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>

namespace {

// remove spaces and tabs from both ends
std::string trim(const std::string &s) {
    std::size_t first = s.find_first_not_of(" \t\r");
    if (first == std::string::npos) {
        return "";
    }
    std::size_t last = s.find_last_not_of(" \t\r");
    return s.substr(first, last - first + 1);
}

void fail(const std::string &path, int line, const std::string &reason) {
    std::cerr << path << ":" << line << ": " << reason << "\n";
    std::exit(1);
}

} // namespace

Settings loadSettings(const std::string &path) {
    std::ifstream in(path);
    if (!in.is_open()) {
        std::cerr << path << ": cannot open the config file\n";
        std::exit(1);
    }

    // defaults, used when a key is missing from the file
    Settings s;
    s.hash      = "toeplitz";
    s.tuple     = "full";
    s.symmetry  = "none";
    s.reta      = true;
    s.weighting = "packet";
    s.keys      = 16;
    s.channels  = {8, 16, 20, 32, 40, 64, 128};
    s.window    = 150000;

    std::string text;
    int line = 0;
    while (std::getline(in, text)) {
        line++;

        // 1. cut off the comment
        std::size_t hash_sign = text.find('#');
        if (hash_sign != std::string::npos) {
            text = text.substr(0, hash_sign);
        }

        // 2. lines without '=' are empty or comment-only
        std::size_t eq = text.find('=');
        if (eq == std::string::npos) {
            continue;
        }

        // 3. key = value
        std::string key   = trim(text.substr(0, eq));
        std::string value = trim(text.substr(eq + 1));
        if (value.empty()) {
            fail(path, line, "'" + key + "' has no value");
        }

        // 4. store it
        if (key == "data") {
            s.data = value;
        } else if (key == "out") {
            s.out = value;
        } else if (key == "hash") {
            s.hash = value;
        } else if (key == "tuple") {
            if (value != "full" && value != "ip") fail(path, line, "tuple must be full or ip");
            s.tuple = value;
        } else if (key == "symmetry") {
            if (value != "none" && value != "xor" && value != "sort") fail(path, line, "symmetry must be none, xor or sort");
            s.symmetry = value;
        } else if (key == "reta") {
            if (value != "on" && value != "off") fail(path, line, "reta must be on or off");
            s.reta = (value == "on");
        } else if (key == "weighting") {
            if (value != "flow" && value != "packet" && value != "byte") fail(path, line, "weighting must be flow, packet or byte");
            s.weighting = value;
        } else if (key == "keys") {
            s.keys = std::stoi(value);
        } else if (key == "channels") {
            s.channels.clear();
            std::istringstream numbers(value);
            int n;
            while (numbers >> n) {
                if (n < 1 || n > 128) fail(path, line, "channel counts must be 1..128");
                s.channels.push_back(n);
            }
        } else if (key == "window") {
            s.window = std::stol(value);
        } else {
            fail(path, line, "unknown key '" + key + "'");
        }
    }

    if (s.data.empty() || s.out.empty()) {
        std::cerr << path << ": 'data' and 'out' are required\n";
        std::exit(1);
    }
    return s;
}

void printSettings(const Settings &s) {
    std::cout << "data      = " << s.data << "\n"
              << "out       = " << s.out << "\n"
              << "hash      = " << s.hash << "\n"
              << "tuple     = " << s.tuple << "\n"
              << "symmetry  = " << s.symmetry << "\n"
              << "reta      = " << (s.reta ? "on" : "off") << "\n"
              << "weighting = " << s.weighting << "\n"
              << "keys      = " << s.keys << "\n"
              << "channels  =";
    for (int n : s.channels) {
        std::cout << " " << n;
    }
    std::cout << "\n"
              << "window    = " << s.window << "\n";
}
