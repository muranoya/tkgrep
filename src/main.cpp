#include "Config.hpp"
#include "TkGrepException.hpp"
#include "Util.hpp"
#include <algorithm>
#include <clang-c/Index.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <iostream>
#include <regex>
#include <set>
#include <string>
#include <unistd.h>

const std::string CL_RED("\x1B[31m");
const std::string CL_NO("\033[0m");

struct MatchLoc {
    explicit MatchLoc(int line, CXTokenKind k, int start, int len)
        : line(line)
        , kind(k)
        , start(start)
        , len(len)
    {
    }
    explicit MatchLoc(int l)
        : line(l)
        , kind()
        , start(0)
        , len(0)
    {
    }

    bool operator==(const MatchLoc& other) { return other.line == line; }

    int line;
    CXTokenKind kind;
    int start;
    int len;
};

bool operator<(const MatchLoc& lhs, const MatchLoc& rhs) { return lhs.line < rhs.line; }

static const char* get_token_kind(CXTokenKind kind) noexcept
{
    switch (kind) {
    case CXToken_Punctuation:
        return "P";
    case CXToken_Keyword:
        return "K";
    case CXToken_Identifier:
        return "I";
    case CXToken_Literal:
        return "L";
    case CXToken_Comment:
        return "C";
    default:
        return "U";
    }
}

static bool is_match_token_kind(CXTokenKind kind, unsigned char target) noexcept
{
    if (target == 0U)
        return true;

    switch (kind) {
    case CXToken_Punctuation:
        return (target & Config::TokenKind::Punctuation) == Config::TokenKind::Punctuation;
    case CXToken_Keyword:
        return (target & Config::TokenKind::Keyword) == Config::TokenKind::Keyword;
    case CXToken_Identifier:
        return (target & Config::TokenKind::Identifier) == Config::TokenKind::Identifier;
    case CXToken_Literal:
        return (target & Config::TokenKind::Literal) == Config::TokenKind::Literal;
    case CXToken_Comment:
        return (target & Config::TokenKind::Comment) == Config::TokenKind::Comment;
    default:
        return (target & Config::TokenKind::Unknown) == Config::TokenKind::Unknown;
    }
}

static void print_result(const std::string& filename, const std::vector<std::string>& file_content,
    const std::set<MatchLoc>& match_lines, const Config& c) noexcept
{
    if (c.only_count) {
        std::printf("%zu\n", match_lines.size());
        return;
    }

    if (file_content.empty()) {
        return;
    }

    int printed_line = 1;
    for (const MatchLoc& m : match_lines) {
        const int start_line = std::max(printed_line, m.line - c.before_context);
        const int end_line
            = std::min(static_cast<int>(file_content.size()), m.line + c.after_context + 1);
        for (int i = start_line; i < end_line; ++i) {
            const char* tk = " ";
            const auto& iter = match_lines.find(MatchLoc(i));
            std::string match_line = file_content[i - 1];
            if (iter != match_lines.cend()) {
                tk = get_token_kind(iter->kind);
                if (c.color_print && !c.stdout_is_pipe) {
                    match_line.insert(iter->start, CL_RED);
                    match_line.insert(iter->start + iter->len + CL_RED.length(), CL_NO);
                }
            }

            std::printf("%s:%d:%s\t%s\n", filename.c_str(), i, tk, match_line.c_str());
            printed_line = i + 1;
        }
    }
}

static std::set<MatchLoc> match_tokens(const CXTranslationUnit& tu, const CXToken* tokens,
    unsigned int num_tokens, const Config& c) noexcept
{
    std::set<MatchLoc> match_lines;

    std::regex re(c.pattern);
    for (unsigned int i = 0U; i < num_tokens; i++) {
        const CXToken& token = tokens[i];
        const CXTokenKind kind = clang_getTokenKind(token);
        CXString spell = clang_getTokenSpelling(tu, token);
        const CXSourceLocation loc = clang_getTokenLocation(tu, token);

        CXFile file;
        unsigned int line, column, offset;
        clang_getFileLocation(loc, &file, &line, &column, &offset);

        std::string spell_s(clang_getCString(spell));
        if (c.ignore_case) {
            spell_s = Util::strtolower(spell_s);
        }

        std::smatch match;
        if (std::regex_search(spell_s, match, re) && is_match_token_kind(kind, c.target)) {
            match_lines.emplace(line, kind, column + match.position() - 1, match.length());
        }

        clang_disposeString(spell);
    }

    return match_lines;
}

static CXSourceRange get_filerange(
    const CXTranslationUnit& tu, const std::string& filename, unsigned int filesize)
{
    CXFile file = clang_getFile(tu, filename.c_str());

    CXSourceLocation topLoc = clang_getLocationForOffset(tu, file, 0);
    CXSourceLocation lastLoc = clang_getLocationForOffset(tu, file, filesize);
    if (clang_equalLocations(topLoc, clang_getNullLocation())
        || clang_equalLocations(lastLoc, clang_getNullLocation())) {
        throw TkGrepException("Cannot retrieve location.");
    }

    CXSourceRange range = clang_getRange(topLoc, lastLoc);
    if (clang_Range_isNull(range)) {
        throw TkGrepException("Cannot retrieve range.");
    }

    return range;
}

static void print_usage(int argc, char* argv[]) noexcept
{
    CXString version = clang_getClangVersion();

    std::printf("Usage: %s [OPTION]... PATTERN FILE...\n", argv[0]);
    std::printf("%s\n", clang_getCString(version));
    std::printf("\n");
    std::printf("  -A NUM: Print NUM lines of trailing context after matching lines.\n");
    std::printf("  -B NUM: Print NUM lines of trailing context before matching lines.\n");
    std::printf("  -C NUM: Print NUM lines of output context.\n");
    std::printf("  -t KIND: Token kind to be searched. KIND is p(punctuation), k(keyword), "
                "i(identifier), l(literal), c(comment), u(unknown).\n");
    std::printf("  -G: Surround the matched strings with color on the terminal.\n");
    std::printf("  -R: Read all files under each directory.\n");
    std::printf("  -i: Ignore case distinctions.\n");
    std::printf("  -h: Display this help text.\n");

    clang_disposeString(version);
}

static Config parse_opt(int argc, char* argv[]) noexcept
{
    Config c;

    int opt;
    while ((opt = getopt(argc, argv, "A:B:C:t:GRcih")) != -1) {
        switch (opt) {
        case 'A': {
            const int c_len = std::atoi(optarg);
            if (c_len <= 0) {
                std::cerr << c_len << ": invalid context length argument" << std::endl;
                std::exit(EXIT_FAILURE);
            } else {
                c.after_context = std::max(c_len, c.after_context);
            }
        } break;
        case 'B': {
            const int c_len = std::atoi(optarg);
            if (c_len <= 0) {
                std::cerr << c_len << ": invalid context length argument" << std::endl;
                std::exit(EXIT_FAILURE);
            } else {
                c.before_context = std::max(c_len, c.before_context);
            }
        } break;
        case 'C': {
            const int c_len = std::atoi(optarg);
            if (c_len <= 0) {
                std::cerr << c_len << ": invalid context length argument" << std::endl;
                std::exit(EXIT_FAILURE);
            } else {
                c.after_context = std::max(c_len, c.after_context);
                c.before_context = std::max(c_len, c.before_context);
            }
        } break;
        case 't': {
            const int len = std::strlen(optarg);
            for (int i = 0; i < len; ++i) {
                switch (optarg[i]) {
                case 'p':
                    c.target |= Config::TokenKind::Punctuation;
                    break;
                case 'k':
                    c.target |= Config::TokenKind::Keyword;
                    break;
                case 'i':
                    c.target |= Config::TokenKind::Identifier;
                    break;
                case 'l':
                    c.target |= Config::TokenKind::Literal;
                    break;
                case 'c':
                    c.target |= Config::TokenKind::Comment;
                    break;
                case 'u':
                    c.target |= Config::TokenKind::Unknown;
                    break;
                default:
                    std::cerr << "Unknown option value: " << optarg[i] << std::endl;
                    break;
                }
            }
        } break;
        case 'G':
            c.color_print = true;
            break;
        case 'R':
            c.recursive = true;
            break;
        case 'c':
            c.only_count = true;
            break;
        case 'i':
            c.ignore_case = true;
            break;
        case 'h':
        default:
            print_usage(argc, argv);
            std::exit(EXIT_FAILURE);
            break;
        }
    }

    c.stdout_is_pipe = (isatty(fileno(stdout)) != 1);

    int oi = optind;
    if (optind < argc) {
        c.pattern = argv[oi];
    } else {
        std::cerr << "PATTERN is not specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (int i = oi + 1; i < argc; i++) {
        c.files.emplace_back(argv[i]);
    }

    if (c.files.empty()) {
        std::cerr << "FILE is not specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return c;
}

static void tkgrep_r(const std::string& filename, const Config& c) noexcept
{
    bool is_dir = false;
    try {
        is_dir = Util::is_directory(filename);
    } catch (const TkGrepException& e) {
        std::cerr << filename << ": " << e.msg << std::endl;
        return;
    }

    if (is_dir) {
        DIR* dirp = opendir(filename.c_str());
        if (dirp == nullptr) {
            return;
        }

        for (;;) {
            struct dirent* dp = readdir(dirp);
            if (dp == nullptr) {
                break;
            }
            if (std::string(dp->d_name) == "." || std::string(dp->d_name) == "..") {
                continue;
            }

            std::string next_path = filename;
            if (next_path.back() != '/') {
                next_path += '/';
            }
            next_path += dp->d_name;
            tkgrep_r(next_path, c);
        }
    } else {
        unsigned int filesize = 0U;
        try {
            filesize = Util::get_filesize(filename);
        } catch (const TkGrepException& e) {
            std::cerr << filename << ": " << e.msg << std::endl;
            return;
        }

        const std::vector<std::string> file_content = Util::read_file_content(filename);

        CXIndex index = clang_createIndex(1, 0);
        CXTranslationUnit tu = clang_parseTranslationUnit(index, filename.c_str(), nullptr, 0,
            nullptr, 0, CXTranslationUnit_KeepGoing | CXTranslationUnit_SingleFileParse);
        if (tu == nullptr) {
            std::cerr << filename << ": Cannot parse translation unit." << std::endl;
            clang_disposeIndex(index);
            return;
        }

        CXSourceRange range;
        try {
            range = get_filerange(tu, filename.c_str(), filesize);
        } catch (TkGrepException& e) {
            std::cerr << filename << ": " << e.msg << std::endl;
            clang_disposeTranslationUnit(tu);
            clang_disposeIndex(index);
            return;
        }

        CXToken* tokens;
        unsigned int numTokens;
        clang_tokenize(tu, range, &tokens, &numTokens);

        const std::set<MatchLoc> match_lines = match_tokens(tu, tokens, numTokens, c);
        print_result(filename, file_content, match_lines, c);

        clang_disposeTokens(tu, tokens, numTokens);
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
    }
}

int main(int argc, char* argv[])
{
    const Config c = parse_opt(argc, argv);

    for (const std::string& file : c.files) {
        tkgrep_r(file, c);
    }

    return EXIT_SUCCESS;
}
