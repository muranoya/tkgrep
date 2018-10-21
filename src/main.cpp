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

static void print_match_tokens(const CXTranslationUnit& tu, const CXToken* tokens,
    unsigned int num_tokens, std::string pattern, bool ignore_case)
{
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
        if (ignore_case) {
            pattern = Util::strtolower(pattern);
            spell_s = Util::strtolower(spell_s);
        }

        if (pattern == spell_s) {
            std::printf("%s:%d:%d\t%s\t%s\n", clang_getCString(filename), line, column,
                get_token_kind(kind), clang_getCString(spell));
        }

        clang_disposeString(filename);
        clang_disposeString(spell);
    }
}

static CXSourceRange get_filerange(const CXTranslationUnit& tu, const std::string& filename)
{
    CXFile file = clang_getFile(tu, filename.c_str());
    const unsigned int filesize = Util::get_filesize(filename);
    if (filesize == 0U) {
        throw TkGrepException("Cannot open file.");
    }

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

static void print_usage(int argc, char* argv[])
{
    CXString version = clang_getClangVersion();

    std::printf("Usage: %s [OPTION]... PATTERN FILE...\n", argv[0]);
    std::printf("%s\n", clang_getCString(version));
    std::printf("\n");
    std::printf("\t-i:\tignore case distinctions");
    std::printf("\t-h:\tdisplay this help text");

    clang_disposeString(version);
}

static Config parse_opt(int argc, char* argv[])
{
    Config c;

    int opt;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        switch (opt) {
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

    if (optind < argc) {
        c.pattern = argv[optind];
    } else {
        std::cerr << "PATTERN is not specified." << std::endl;
        std::exit(EXIT_FAILURE);
    }

    for (int i = optind + 1; i < argc; i++) {
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
        CXIndex index = clang_createIndex(1, 1);
        CXTranslationUnit tu
            = clang_parseTranslationUnit(index, file.c_str(), nullptr, 0, nullptr, 0, 0);
        if (tu == nullptr) {
            std::cerr << file << ": Cannot parse translation unit." << std::endl;
            continue;
        }

        try {
            CXSourceRange range = get_filerange(tu, file.c_str());
            CXToken* tokens;
            unsigned int numTokens;
            clang_tokenize(tu, range, &tokens, &numTokens);

            print_match_tokens(tu, tokens, numTokens, c.pattern, c.ignore_case);

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
