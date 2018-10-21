#ifndef TKGREP_UTIL
#define TKGREP_UTIL

#include "TkGrepException.hpp"
#include <string>
#include <vector>

namespace Util {
unsigned int get_filesize(const std::string& filename);
std::string strtolower(std::string str) noexcept;
std::vector<std::string> read_file_content(const std::string& filename) noexcept;
bool is_directory(const std::string& path);
} // namespace Util

#endif // TKGREP_UTIL
