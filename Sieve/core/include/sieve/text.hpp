// Sieve — plain text cut into lines and fields, trimmed, lowered and checked: the one copy the
// programs share, so that every reader of a Sieve text format cuts it the same way.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

// The lines of a text whose every line ends in a line feed (the strict formats: saves, action maps,
// catalogue entries, RAID manifests and indexes), added to `lines`. False when the last line has
// none, so that each reader can say so in its own words.
inline bool lf_lines(std::string_view text, std::vector<std::string>& lines)
{
    for (size_t i = 0; i < text.size();)
    {
        const size_t e = text.find('\n', i);
        if (e == std::string_view::npos) return false;
        lines.emplace_back(text.substr(i, e - i));
        i = e + 1;
    }
    return true;
}

// The lines of a text read leniently (characters and animations): the last line may lack its line
// feed, a carriage return at a line's end is dropped, and empty lines are left out.
inline std::vector<std::string> text_lines(std::string_view text)
{
    std::vector<std::string> lines;
    for (size_t i = 0; i < text.size();)
    {
        size_t e = text.find('\n', i);
        if (e == std::string_view::npos) e = text.size();
        std::string_view l = text.substr(i, e - i);
        if (!l.empty() && l.back() == '\r') l.remove_suffix(1);
        if (!l.empty()) lines.emplace_back(l);
        i = e + 1;
    }
    return lines;
}

// The fields between separators, empty ones kept: "a,,b" gives three and "" gives one, as a record
// with an empty field must still be read in its place.
inline std::vector<std::string> split_fields(std::string_view s, char sep)
{
    std::vector<std::string> out;
    size_t at = 0;
    for (;;)
    {
        const size_t p = s.find(sep, at);
        out.emplace_back(s.substr(at, p == std::string_view::npos ? std::string_view::npos : p - at));
        if (p == std::string_view::npos) return out;
        at = p + 1;
    }
}

// The fields between separators, empty ones left out: a list such as the settings' comma lists,
// where "a,,b," is just a and b. The fields are kept as they are, spaces included.
inline std::vector<std::string> split_nonempty(std::string_view s, char sep)
{
    std::vector<std::string> out;
    for (size_t i = 0; i <= s.size();)
    {
        const size_t c = std::min(s.find(sep, i), s.size());
        if (c > i) out.emplace_back(s.substr(i, c - i));
        i = c + 1;
    }
    return out;
}

// The fields with a separator between each two, as split_nonempty reads them back: the settings'
// comma lists written. (No separator goes before a field while nothing has been written, as the lists
// have always been joined.)
inline std::string join_fields(const std::vector<std::string>& fields, char sep)
{
    std::string out;
    for (const std::string& f : fields)
    {
        if (!out.empty()) out += sep;
        out += f;
    }
    return out;
}

// The text without the blanks at either end (spaces, tabs and the carriage return a file saved with
// CRLF leaves, unless the caller names its own set).
inline std::string trim(std::string_view s, std::string_view blanks = " \t\r")
{
    const size_t a = s.find_first_not_of(blanks);
    if (a == std::string_view::npos) return std::string();
    return std::string(s.substr(a, s.find_last_not_of(blanks) - a + 1));
}

// The text with A-Z made a-z and every other byte left as it is. Written out rather than through
// std::tolower, whose answer for bytes from 0x80 up depends on the locale.
inline std::string ascii_lower(std::string_view s)
{
    std::string t(s);
    for (char& c : t)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    return t;
}

// Whether the text is an id as the catalogue and the saves write one: 1 to `most` characters, each
// a-z, 0-9, '-', '.' or '_'.
inline bool is_plain_id(std::string_view s, size_t most = 128)
{
    if (s.empty() || s.size() > most) return false;
    for (const char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '.' || c == '_')) return false;
    return true;
}

// A whole decimal number of 1 to 19 digits, leading zeros allowed; nothing if the text is not one.
// Nineteen digits always fit in 64 bits, so no number read this way can overflow.
inline std::optional<uint64_t> decimal_u64(std::string_view t)
{
    if (t.empty() || t.size() > 19 || t.find_first_not_of("0123456789") != std::string_view::npos) return std::nullopt;
    uint64_t v = 0;
    for (const char c : t) v = v * 10 + uint64_t(c - '0');
    return v;
}

// A number as "%.9g" writes it: nine significant digits, enough that every float reads back as
// itself, so the text formats that hold floats (recordings, world files) have one spelling for each.
inline std::string nine_digits(double v)
{
    char b[40];
    std::snprintf(b, sizeof b, "%.9g", v);
    return b;
}

} // namespace sieve
