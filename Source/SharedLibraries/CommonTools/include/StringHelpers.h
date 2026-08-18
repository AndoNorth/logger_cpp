#pragma once

#include <string>
#include <algorithm> 
#include <functional> 
#include <cctype>
#include <locale>
#include <string_view>
#include <vector>
#include <ranges>

// Split by delimiter into a vector of strings.
static inline std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> retVal;
    for (auto range : std::views::split(s, delimiter))
    {
        auto str = std::string(std::ranges::begin(range), std::ranges::end(range));
        retVal.emplace_back(str);
    }
    return retVal;
}

static inline std::vector<std::string> split(const std::string& s, const std::string& delimiter) {
    std::vector<std::string> retVal;
    for (auto range : std::views::split(s, delimiter))
    {
        auto str = std::string(std::ranges::begin(range), std::ranges::end(range));
        retVal.emplace_back(str);
    }
    return retVal;
}

// trim from start (in place)
static inline void ltrim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
    ltrim(s);
    rtrim(s);
}

// trim from start (in place)
static inline void ltrim_char(std::string& s, unsigned char ch) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [ch](unsigned char c) {
        return c != ch;
    }));
}

// trim from end (in place)
static inline void rtrim_char(std::string& s, unsigned char ch) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [ch](unsigned char c) {
        return c != ch;
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim_char(std::string& s, unsigned char ch) {
    ltrim_char(s, ch);
    rtrim_char(s, ch);
}

// trim from start (in place)
static inline void ltrim_chars(std::string& s, unsigned char* ch) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [ch](unsigned char c) {
        return std::string((const char*)ch).find(c) == std::string::npos;
    }));
}

// trim from end (in place)
static inline void rtrim_chars(std::string& s, unsigned char* ch) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [ch](unsigned char c) {
        return std::string((const char*)ch).find(c) == std::string::npos;
    }).base(), s.end());
}

// trim from both ends (in place)
static inline void trim_chars(std::string& s, unsigned char* ch) {
    ltrim_chars(s, ch);
    rtrim_chars(s, ch);
}

// replace all instances of "what", with "with" in "inout"
static inline std::size_t replace_all(std::string& inout, std::string what, std::string with) {
    std::size_t count{};
    for (std::string::size_type pos{};
        inout.npos != (pos = inout.find(what.data(), pos, what.length()));
        pos += with.length(), ++count) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
    return count;
}

// replace all instances of "what", with "with" in "in"
static inline std::string replace_all_copy(const std::string& in, std::string what, std::string with) {
    std::size_t count{};
    std::string result{ in };

    for (std::string::size_type pos{};
        result.npos != (pos = result.find(what.data(), pos, what.length()));
        pos += with.length(), ++count) {
        result.replace(pos, what.length(), with.data(), with.length());
    }
    return result;
}

static inline std::string str_toupper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return std::toupper(c); }
    );
    return s;
}

static inline std::string str_tolower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [] (unsigned char c) { return std::tolower(c); }
    );
    return s;
}

// Match a string against a wildcard pattern supporting '*' (any sequence) and '?' (any single char)
static inline bool matches_wildcard(std::string_view pattern, std::string_view str) {
    std::size_t pattern_pos = 0, str_pos = 0;
    std::size_t last_star_pos = std::string_view::npos, backtrack_str_pos = 0;
    while (str_pos < str.size()) {
        if (pattern_pos < pattern.size() && (pattern[pattern_pos] == str[str_pos] || pattern[pattern_pos] == '?')) {
            ++pattern_pos; ++str_pos;
        }
        else if (pattern_pos < pattern.size() && pattern[pattern_pos] == '*') {
            last_star_pos = pattern_pos++;
            backtrack_str_pos = str_pos;
        }
        else if (last_star_pos != std::string_view::npos) {
            pattern_pos = last_star_pos + 1;
            str_pos = ++backtrack_str_pos;
        }
        else {
            return false;
        }
    }
    while (pattern_pos < pattern.size() && pattern[pattern_pos] == '*') ++pattern_pos;
    return pattern_pos == pattern.size();
}