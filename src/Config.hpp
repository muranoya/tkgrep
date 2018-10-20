#ifndef TKGREP_CONFIG
#define TKGREP_CONFIG

#include <string>
#include <vector>

struct Config {
    bool ignore_case = false;
    std::string pattern;
    std::vector<std::string> files;
};

#endif // TKGREP_CONFIG
