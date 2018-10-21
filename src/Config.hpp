#ifndef TKGREP_CONFIG
#define TKGREP_CONFIG

#include <string>
#include <vector>

struct Config {
    bool only_count = false;
    bool print_tokens_exit = false;
    bool ignore_case = false;
    std::string pattern;
    std::vector<std::string> files;
};

#endif // TKGREP_CONFIG
