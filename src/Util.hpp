#ifndef TKGREP_UTIL
#define TKGREP_UTIL

#include "TkGrepException.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <string>

namespace Util {
unsigned int get_filesize(const std::string& filename)
{
    std::ifstream file(filename.c_str(), std::ifstream::ate);
    if (!file.is_open()) {
        throw TkGrepException("Cannot open the file");
    }
    return file.tellg();
}

std::string strtolower(std::string str) noexcept
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}
} // namespace Util

#endif // TKGREP_UTIL
