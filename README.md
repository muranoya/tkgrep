# tkgrep
tkgrep is a grep specialized in C/C++ source code. tkgrep is using libclang to parse a source code and find out specify words.

* Regular expression search
* Search for specific types of tokens
* Colorize output
* Recursive grep
* print with context

# Installation

First, install all required dependecies.

```bash
[sudo] apt-get install build-essential libclang-dev llvm-dev
```

You will need to manually compile.

```bash
make
```

# Usage

basic grep of C/C++ source code.

```bash
tkgrep main src/main.cpp
```

```
src/main.cpp:370:I	int main(int argc, char* argv[])
```

with colorize output.

```bash
tkgrep -G main src/main.cpp
```

directory specify.

```bash
tkgrep -RG Config src
```

```
src/main.cpp:1:L	#include "Config.hpp"
src/main.cpp:70:I	        return (target & Config::TokenKind::Punctuation) == Config::TokenKind::Punctuation;
src/main.cpp:72:I	        return (target & Config::TokenKind::Keyword) == Config::TokenKind::Keyword;
src/main.cpp:74:I	        return (target & Config::TokenKind::Identifier) == Config::TokenKind::Identifier;
src/main.cpp:76:I	        return (target & Config::TokenKind::Literal) == Config::TokenKind::Literal;
src/main.cpp:78:I	        return (target & Config::TokenKind::Comment) == Config::TokenKind::Comment;
src/main.cpp:80:I	        return (target & Config::TokenKind::Unknown) == Config::TokenKind::Unknown;
src/main.cpp:85:I	    const std::set<MatchLoc>& match_lines, const Config& c) noexcept
src/main.cpp:120:I	    const CXTranslationUnit& tu, const CXToken* tokens, unsigned int num_tokens, const Config& c)
src/main.cpp:193:I	static Config parse_opt(int argc, char* argv[]) noexcept
src/main.cpp:195:I	    Config c;
src/main.cpp:233:I	                    c.target |= Config::TokenKind::Punctuation;
src/main.cpp:236:I	                    c.target |= Config::TokenKind::Keyword;
src/main.cpp:239:I	                    c.target |= Config::TokenKind::Identifier;
src/main.cpp:242:I	                    c.target |= Config::TokenKind::Literal;
src/main.cpp:245:I	                    c.target |= Config::TokenKind::Comment;
src/main.cpp:248:I	                    c.target |= Config::TokenKind::Unknown;
src/main.cpp:305:I	static void tkgrep_r(const std::string& filename, const Config& c) noexcept
src/main.cpp:372:I	    const Config c = parse_opt(argc, argv);
src/Config.hpp:7:I	struct Config {
```

regex search.

```
tkgrep -RG '.*Config.*' src
```

```
src/main.cpp:1:L	#include "Config.hpp"
src/main.cpp:70:I	        return (target & Config::TokenKind::Punctuation) == Config::TokenKind::Punctuation;
src/main.cpp:72:I	        return (target & Config::TokenKind::Keyword) == Config::TokenKind::Keyword;
src/main.cpp:74:I	        return (target & Config::TokenKind::Identifier) == Config::TokenKind::Identifier;
src/main.cpp:76:I	        return (target & Config::TokenKind::Literal) == Config::TokenKind::Literal;
src/main.cpp:78:I	        return (target & Config::TokenKind::Comment) == Config::TokenKind::Comment;
src/main.cpp:80:I	        return (target & Config::TokenKind::Unknown) == Config::TokenKind::Unknown;
src/main.cpp:85:I	    const std::set<MatchLoc>& match_lines, const Config& c) noexcept
src/main.cpp:120:I	    const CXTranslationUnit& tu, const CXToken* tokens, unsigned int num_tokens, const Config& c)
src/main.cpp:193:I	static Config parse_opt(int argc, char* argv[]) noexcept
src/main.cpp:195:I	    Config c;
src/main.cpp:233:I	                    c.target |= Config::TokenKind::Punctuation;
src/main.cpp:236:I	                    c.target |= Config::TokenKind::Keyword;
src/main.cpp:239:I	                    c.target |= Config::TokenKind::Identifier;
src/main.cpp:242:I	                    c.target |= Config::TokenKind::Literal;
src/main.cpp:245:I	                    c.target |= Config::TokenKind::Comment;
src/main.cpp:248:I	                    c.target |= Config::TokenKind::Unknown;
src/main.cpp:305:I	static void tkgrep_r(const std::string& filename, const Config& c) noexcept
src/main.cpp:372:I	    const Config c = parse_opt(argc, argv);
src/Config.hpp:7:I	struct Config {
```

print with context.

```
tkgrep -C 2 main src/main.cpp
```

```
src/main.cpp:368: 	}
src/main.cpp:369: 	
src/main.cpp:370:I	int main(int argc, char* argv[])
src/main.cpp:371: 	{
src/main.cpp:372: 	    const Config c = parse_opt(argc, argv);
```

search only comment and literal.

```
tkgrep -t cl -RG '.*token.*' src
```

```
src/main.cpp:187:L	    std::printf("  -p: Print all tokens and exit, ignore PATTERN.\n");
src/Config.hpp:17:C	    // target = 0 then all token kind are searched.
```

