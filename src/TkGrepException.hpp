#ifndef TKGREP_EXCEPTION
#define TKGREP_EXCEPTION

#include <exception>
#include <string>

struct TkGrepException : public std::exception {
    std::string msg;
    explicit TkGrepException(const std::string& msg)
        : msg(msg)
    {
    }
};

#endif // TKGREP_EXCEPTION
