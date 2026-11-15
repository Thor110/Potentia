// Sieve — the one way a footer of keys is laid out (IDEAS 19.224; Edward, 14 November 2026: "make sure
// that all windows and menus that have button text and descriptions at the bottom of their bounding box,
// use the wrap around feature for new keys that are added so that they wrap around above them").
//
// A footer is a list of entries ("Tab: next view", "Esc: close"), written apart by two spaces or more (an
// entry's own words are one space apart: "Shift + wheel: across").
// It is laid out in lines that fit the room it has, broken only between entries, so a key and what it
// does are never parted; an entry too long for a line of its own is broken between words, and a word too
// long for a line between characters. The screen draws the lines with the last at the foot, so a footer
// that gains keys grows upwards rather than off the side. Here in the core, with the width measured by a
// function the caller gives (the hallway's measures its font), so the unit tests can check the layout
// without a window.
#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace sieve {

// One entry of a footer and the gap written before it ("" for the first), kept so a line that holds
// entries as they were written reads exactly as written.
struct FooterEntry
{
    std::string gap;
    std::string text;
};

// A footer's entries: apart wherever two spaces or more stand between them. Spaces before the first and
// after the last are dropped.
inline std::vector<FooterEntry> footer_entries(const std::string& s)
{
    std::vector<FooterEntry> out;
    size_t i = s.find_first_not_of(' ');
    std::string gap;
    while (i != std::string::npos && i < s.size())
    {
        size_t end = s.find("  ", i);
        if (end == std::string::npos) end = s.size();
        std::string text = s.substr(i, end - i);
        while (!text.empty() && text.back() == ' ') text.pop_back();
        if (!text.empty()) out.push_back({out.empty() ? std::string() : gap, text});
        if (end >= s.size()) break;
        const size_t next = s.find_first_not_of(' ', end);
        if (next == std::string::npos) break;
        gap = s.substr(end, next - end);
        i = next;
    }
    return out;
}

// The length in bytes of the UTF-8 character starting at s[i] (1 for a stray continuation byte), so a
// word cut between characters is never cut inside one.
inline size_t footer_char_len(const std::string& s, size_t i)
{
    const unsigned char c = static_cast<unsigned char>(s[i]);
    size_t n = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
    if (i + n > s.size()) n = s.size() - i;
    return n;
}

// One entry too long for a line alone, broken into lines of at most `room` (as `measure` counts): between
// words where it can be, between characters where a word alone is too long. Every line holds at least one
// character, so a room too small for anything still ends.
template <class Measure>
std::vector<std::string> footer_break_entry(const std::string& e, double room, Measure measure)
{
    std::vector<std::string> out;
    std::string line;
    size_t i = 0;
    while (i < e.size())
    {
        size_t j = e.find(' ', i);
        if (j == std::string::npos) j = e.size();
        const std::string word = e.substr(i, j - i);
        const std::string joined = line.empty() ? word : line + " " + word;
        if (double(measure(joined)) <= room) line = joined;
        else
        {
            if (!line.empty()) out.push_back(line);
            line.clear();
            // A word longer than the room: as many characters as fit, then the rest on the next line.
            size_t k = 0;
            while (k < word.size())
            {
                const size_t n = footer_char_len(word, k);
                if (!line.empty() && double(measure(line + word.substr(k, n))) > room)
                {
                    out.push_back(line);
                    line.clear();
                }
                line += word.substr(k, n);
                k += n;
            }
        }
        i = j;
        while (i < e.size() && e[i] == ' ') ++i;
    }
    if (!line.empty()) out.push_back(line);
    return out;
}

// A footer laid out in lines of at most `room`, as `measure` counts a string (pixels, characters: any unit
// the room is in), top line first. Entries on one line keep the gap they were written with, so a footer
// that fits comes out exactly as written. At least one line, empty for an empty footer.
template <class Measure>
std::vector<std::string> wrap_footer(const std::string& s, double room, Measure measure)
{
    std::vector<std::string> out;
    std::string line;
    for (const FooterEntry& e : footer_entries(s))
    {
        if (!line.empty() && double(measure(line + e.gap + e.text)) <= room)
        {
            line += e.gap + e.text;
            continue;
        }
        if (!line.empty()) out.push_back(line);
        line.clear();
        if (double(measure(e.text)) <= room) line = e.text;
        else
        {
            std::vector<std::string> parts = footer_break_entry(e.text, room, measure);
            if (parts.empty()) continue;
            line = parts.back();
            parts.pop_back();
            out.insert(out.end(), parts.begin(), parts.end());
        }
    }
    if (!line.empty()) out.push_back(line);
    if (out.empty()) out.push_back("");
    return out;
}

// The same, counted in bytes (one per character of plain ASCII): `width` characters to a line.
inline std::vector<std::string> wrap_footer(const std::string& s, size_t width)
{
    return wrap_footer(s, double(width), [](const std::string& t) { return t.size(); });
}

} // namespace sieve
