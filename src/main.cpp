#include "Config.hpp"
#include "TkGrepException.hpp"
#include "Util.hpp"
#include <clang-c/Index.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

static const char* get_token_kind(const CXTokenKind& kind)
{
    switch (kind) {
    case CXToken_Punctuation:
        return "Punctuation";
        break;
    case CXToken_Keyword:
        return "Keyword";
        break;
    case CXToken_Identifier:
        return "Identifier";
        break;
    case CXToken_Literal:
        return "Literal";
        break;
    case CXToken_Comment:
        return "Comment";
        break;
    default:
        return "Unknown";
        break;
    }
}

static void print_match_tokens(
    const CXTranslationUnit& tu, const CXToken* tokens, unsigned int num_tokens, const Config& c)
{
    std::string pattern = c.pattern;
    if (c.ignore_case) {
        pattern = Util::strtolower(pattern);
    }

    unsigned int count = 0U;
    for (unsigned int i = 0U; i < num_tokens; i++) {
        const CXToken& token = tokens[i];
        CXTokenKind kind = clang_getTokenKind(token);
        CXString spell = clang_getTokenSpelling(tu, token);
        CXSourceLocation loc = clang_getTokenLocation(tu, token);

        CXFile file;
        unsigned int line, column, offset;
        clang_getFileLocation(loc, &file, &line, &column, &offset);
        CXString filename = clang_getFileName(file);

        std::string spell_s(clang_getCString(spell));
        if (!c.print_tokens_exit && c.ignore_case) {
            spell_s = Util::strtolower(spell_s);
        }

        if (c.print_tokens_exit || pattern == spell_s) {
            if (c.only_count) {
                count++;
            } else {
                std::printf("%s:%d:%d\t%s\t%s\n", clang_getCString(filename), line, column,
                    get_token_kind(kind), clang_getCString(spell));
            }
        }

        clang_disposeString(filename);
        clang_disposeString(spell);
    }

    if (c.only_count) {
        std::printf("%d\n", count);
    }
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
    std::printf("\t-i:\tignore case distinctions\n");
    std::printf("\t-p:\tprint all tokens and exit, ignore PATTERN\n");
    std::printf("\t-h:\tdisplay this help text\n");

    clang_disposeString(version);
}

static Config parse_opt(int argc, char* argv[]) noexcept
{
    Config c;

    int opt;
    while ((opt = getopt(argc, argv, "cpih")) != -1) {
        switch (opt) {
        case 'c':
            c.only_count = true;
            break;
        case 'p':
            c.print_tokens_exit = true;
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

    int oi = optind;
    if (optind < argc) {
        if (!c.print_tokens_exit) {
            c.pattern = argv[oi];
        } else {
            oi--;
        }
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

int main(int argc, char* argv[])
{
    const Config c = parse_opt(argc, argv);

    for (const std::string& file : c.files) {
        unsigned int filesize = 0U;
        try {
            filesize = Util::get_filesize(file);
        } catch (const TkGrepException& e) {
            std::cerr << file << ": " << e.msg << std::endl;
            continue;
        }

        CXIndex index = clang_createIndex(1, 0);
        CXTranslationUnit tu = clang_parseTranslationUnit(index, file.c_str(), nullptr, 0, nullptr,
            0, CXTranslationUnit_KeepGoing | CXTranslationUnit_SingleFileParse);
        if (tu == nullptr) {
            std::cerr << file << ": Cannot parse translation unit." << std::endl;
            continue;
        }

        try {
            CXSourceRange range = get_filerange(tu, file.c_str(), filesize);
            CXToken* tokens;
            unsigned int numTokens;
            clang_tokenize(tu, range, &tokens, &numTokens);

            print_match_tokens(tu, tokens, numTokens, c);

            clang_disposeTokens(tu, tokens, numTokens);
        } catch (TkGrepException& e) {
            std::cerr << file << ": " << e.msg << std::endl;
            continue;
        }
        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);
    }

    return EXIT_SUCCESS;
}
