#ifndef TKGREP_CONFIG
#define TKGREP_CONFIG

#include <string>
#include <vector>

struct Config {
    enum TokenKind {
        Punctuation = 0x01,
        Keyword = 0x02,
        Identifier = 0x04,
        Literal = 0x08,
        Comment = 0x10,
        Unknown = 0x20,
    };
    // TokenKind bitwise.
    // target = 0 then all token kind are searched.
    unsigned char target = 0U;

    int after_context = 0U;
    int before_context = 0U;

    bool only_count = false;
    bool print_tokens_exit = false;
    bool ignore_case = false;
    std::string pattern;
    std::vector<std::string> files;
};

#endif // TKGREP_CONFIG
