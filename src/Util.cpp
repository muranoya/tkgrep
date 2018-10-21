#include "Util.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>

unsigned int Util::get_filesize(const std::string& filename)
{
    std::ifstream file(filename.c_str(), std::ifstream::ate);
    if (!file.is_open()) {
        throw TkGrepException("Cannot open the file");
    }
    return file.tellg();
}

std::string Util::strtolower(std::string str) noexcept
{
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    return str;
}

std::vector<std::string> Util::read_file_content(const std::string& filename) noexcept
{
    std::ifstream ifs(filename.c_str(), std::ifstream::in);
    std::vector<std::string> file_content;
    std::string line;

    while (std::getline(ifs, line)) {
        file_content.emplace_back(std::move(line));
    }

    return file_content;
}
