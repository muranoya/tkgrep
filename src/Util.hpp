#ifndef TKGREP_UTIL
#define TKGREP_UTIL

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>

namespace Util {
unsigned int get_filesize(const std::string& filename)
{
    std::ifstream file(filename.c_str(), std::ifstream::ate);
    if (!file.is_open()) {
        return 0UL;
    }
    return file.tellg();
}

std::string strtolower(const std::string& str)
{
    std::string ret;
    std::transform(str.begin(), str.end(), ret.begin(), ::tolower);
    return ret;
}
} // namespace Util

#endif // TKGREP_UTIL
