#include "sieve/canon.hpp"

#include "sieve/utf8.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace sieve {

namespace {

bool is_whitespace(char32_t c)
{
    return c == 0x09 || c == 0x0A || c == 0x0B || c == 0x0C || c == 0x0D || c == 0x20 || c == 0xA0;
}

struct Mapping
{
    char32_t from;
    const char* to;
};

// Typographic punctuation -> ASCII (canon-text-v2, step 2).
constexpr Mapping kTransliterate[] = {
    {0x00AB, "\""}, {0x00BB, "\""}, {0x02BC, "'"},  {0x2010, "-"},   {0x2011, "-"}, {0x2012, "-"},
    {0x2013, "-"},  {0x2014, "-"},  {0x2015, "-"},  {0x2018, "'"},   {0x2019, "'"}, {0x201C, "\""},
    {0x201D, "\""}, {0x201E, "\""}, {0x2026, "..."}, {0x2032, "'"},  {0x2033, "\""}, {0x2212, "-"},
};

// Latin accented letters -> ASCII (canon-text-v2, step 3). Sorted by codepoint.
constexpr Mapping kFold[] = {
    // 190 letters U+00C0-U+017F: Unicode NFD base letter, or a fixed spelling for letters without one.
    {0x00C0, "A"},
    {0x00C1, "A"},
    {0x00C2, "A"},
    {0x00C3, "A"},
    {0x00C4, "A"},
    {0x00C5, "A"},
    {0x00C6, "AE"},
    {0x00C7, "C"},
    {0x00C8, "E"},
    {0x00C9, "E"},
    {0x00CA, "E"},
    {0x00CB, "E"},
    {0x00CC, "I"},
    {0x00CD, "I"},
    {0x00CE, "I"},
    {0x00CF, "I"},
    {0x00D0, "D"},
    {0x00D1, "N"},
    {0x00D2, "O"},
    {0x00D3, "O"},
    {0x00D4, "O"},
    {0x00D5, "O"},
    {0x00D6, "O"},
    {0x00D8, "O"},
    {0x00D9, "U"},
    {0x00DA, "U"},
    {0x00DB, "U"},
    {0x00DC, "U"},
    {0x00DD, "Y"},
    {0x00DE, "TH"},
    {0x00DF, "ss"},
    {0x00E0, "a"},
    {0x00E1, "a"},
    {0x00E2, "a"},
    {0x00E3, "a"},
    {0x00E4, "a"},
    {0x00E5, "a"},
    {0x00E6, "ae"},
    {0x00E7, "c"},
    {0x00E8, "e"},
    {0x00E9, "e"},
    {0x00EA, "e"},
    {0x00EB, "e"},
    {0x00EC, "i"},
    {0x00ED, "i"},
    {0x00EE, "i"},
    {0x00EF, "i"},
    {0x00F0, "d"},
    {0x00F1, "n"},
    {0x00F2, "o"},
    {0x00F3, "o"},
    {0x00F4, "o"},
    {0x00F5, "o"},
    {0x00F6, "o"},
    {0x00F8, "o"},
    {0x00F9, "u"},
    {0x00FA, "u"},
    {0x00FB, "u"},
    {0x00FC, "u"},
    {0x00FD, "y"},
    {0x00FE, "th"},
    {0x00FF, "y"},
    {0x0100, "A"},
    {0x0101, "a"},
    {0x0102, "A"},
    {0x0103, "a"},
    {0x0104, "A"},
    {0x0105, "a"},
    {0x0106, "C"},
    {0x0107, "c"},
    {0x0108, "C"},
    {0x0109, "c"},
    {0x010A, "C"},
    {0x010B, "c"},
    {0x010C, "C"},
    {0x010D, "c"},
    {0x010E, "D"},
    {0x010F, "d"},
    {0x0110, "D"},
    {0x0111, "d"},
    {0x0112, "E"},
    {0x0113, "e"},
    {0x0114, "E"},
    {0x0115, "e"},
    {0x0116, "E"},
    {0x0117, "e"},
    {0x0118, "E"},
    {0x0119, "e"},
    {0x011A, "E"},
    {0x011B, "e"},
    {0x011C, "G"},
    {0x011D, "g"},
    {0x011E, "G"},
    {0x011F, "g"},
    {0x0120, "G"},
    {0x0121, "g"},
    {0x0122, "G"},
    {0x0123, "g"},
    {0x0124, "H"},
    {0x0125, "h"},
    {0x0126, "H"},
    {0x0127, "h"},
    {0x0128, "I"},
    {0x0129, "i"},
    {0x012A, "I"},
    {0x012B, "i"},
    {0x012C, "I"},
    {0x012D, "i"},
    {0x012E, "I"},
    {0x012F, "i"},
    {0x0130, "I"},
    {0x0131, "i"},
    {0x0132, "IJ"},
    {0x0133, "ij"},
    {0x0134, "J"},
    {0x0135, "j"},
    {0x0136, "K"},
    {0x0137, "k"},
    {0x0138, "k"},
    {0x0139, "L"},
    {0x013A, "l"},
    {0x013B, "L"},
    {0x013C, "l"},
    {0x013D, "L"},
    {0x013E, "l"},
    {0x013F, "L"},
    {0x0140, "l"},
    {0x0141, "L"},
    {0x0142, "l"},
    {0x0143, "N"},
    {0x0144, "n"},
    {0x0145, "N"},
    {0x0146, "n"},
    {0x0147, "N"},
    {0x0148, "n"},
    {0x0149, "n"},
    {0x014A, "N"},
    {0x014B, "n"},
    {0x014C, "O"},
    {0x014D, "o"},
    {0x014E, "O"},
    {0x014F, "o"},
    {0x0150, "O"},
    {0x0151, "o"},
    {0x0152, "OE"},
    {0x0153, "oe"},
    {0x0154, "R"},
    {0x0155, "r"},
    {0x0156, "R"},
    {0x0157, "r"},
    {0x0158, "R"},
    {0x0159, "r"},
    {0x015A, "S"},
    {0x015B, "s"},
    {0x015C, "S"},
    {0x015D, "s"},
    {0x015E, "S"},
    {0x015F, "s"},
    {0x0160, "S"},
    {0x0161, "s"},
    {0x0162, "T"},
    {0x0163, "t"},
    {0x0164, "T"},
    {0x0165, "t"},
    {0x0166, "T"},
    {0x0167, "t"},
    {0x0168, "U"},
    {0x0169, "u"},
    {0x016A, "U"},
    {0x016B, "u"},
    {0x016C, "U"},
    {0x016D, "u"},
    {0x016E, "U"},
    {0x016F, "u"},
    {0x0170, "U"},
    {0x0171, "u"},
    {0x0172, "U"},
    {0x0173, "u"},
    {0x0174, "W"},
    {0x0175, "w"},
    {0x0176, "Y"},
    {0x0177, "y"},
    {0x0178, "Y"},
    {0x0179, "Z"},
    {0x017A, "z"},
    {0x017B, "Z"},
    {0x017C, "z"},
    {0x017D, "Z"},
    {0x017E, "z"},
    {0x017F, "s"},
};

const char* lookup(const Mapping* begin, const Mapping* end, char32_t c)
{
    const Mapping* it = std::lower_bound(begin, end, c, [](const Mapping& m, char32_t v) { return m.from < v; });
    return (it != end && it->from == c) ? it->to : nullptr;
}

void note_example(std::u32string& examples, char32_t c)
{
    if (examples.size() < 16 && examples.find(c) == std::u32string::npos) examples.push_back(c);
}

} // namespace

const char* to_string(CanonVersion v)
{
    return v == CanonVersion::V1 ? "canon-text-v1" : v == CanonVersion::Bytes ? "canon-bytes-v1" : "canon-text-v2";
}

CanonVersion canon_version_from_string(std::string_view s)
{
    if (s == "v1" || s == "canon-text-v1") return CanonVersion::V1;
    if (s == "v2" || s == "canon-text-v2") return CanonVersion::V2;
    if (s == "bytes" || s == "canon-bytes-v1") return CanonVersion::Bytes;
    throw std::invalid_argument("unknown canonicalisation version '" + std::string(s) + "' (v1|v2|bytes)");
}

// canon-bytes-v1. There is nothing to canonicalise: on a line whose alphabet holds every byte
// value, each byte already means itself, and folding or collapsing anything would stop a file
// coming back the way it went in. So this only cuts the bytes into units and pads the last one.
CanonResult canonicalise_bytes(std::string_view raw, const Alphabet& alphabet, uint32_t unit_length)
{
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
    if (!holds_all_bytes(alphabet))
        throw std::invalid_argument("canon-bytes-v1 needs an alphabet holding every byte (bytes256)");
    CanonResult r;
    r.version = CanonVersion::Bytes;
    r.input_codepoints = raw.size();
    r.canonical_length = raw.size();
    std::u32string unit;
    unit.reserve(unit_length);
    for (unsigned char c : raw)
    {
        unit.push_back(char32_t(c));
        if (unit.size() == unit_length)
        {
            r.units.push_back(unit);
            unit.clear();
        }
    }
    if (!unit.empty() || r.units.empty())
    {
        r.padding = unit_length - unit.size();
        unit.append(r.padding, char32_t(0)); // NUL, which is digit 0 of the alphabet
        r.units.push_back(unit);
    }
    return r;
}

CanonResult canonicalise_text(std::string_view utf8, const Alphabet& alphabet, uint32_t unit_length, CanonVersion version)
{
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
    static_assert(std::is_sorted(std::begin(kTransliterate), std::end(kTransliterate),
                                 [](const Mapping& a, const Mapping& b) { return a.from < b.from; }));
    static_assert(std::is_sorted(std::begin(kFold), std::end(kFold),
                                 [](const Mapping& a, const Mapping& b) { return a.from < b.from; }));

    CanonResult r;
    r.version = version;
    const std::u32string input = utf8_decode(utf8);
    r.input_codepoints = input.size();

    std::u32string kept;
    kept.reserve(input.size());
    std::u32string expanded;
    for (const char32_t original : input)
    {
        // Steps 1-3: produce one or more candidate characters.
        expanded.clear();
        // Whitespace the alphabet can hold itself is kept as it is: an alphabet with a line feed
        // (ascii96) is meant to carry text whose line breaks are part of the unit, so folding
        // them to spaces would lose the structure. No alphabet pinned before ascii96 holds any
        // whitespace but the space, so this leaves every one of them exactly as it was.
        if (is_whitespace(original))
        {
            // On an alphabet that holds the line feed, a carriage return is dropped, so text
            // arriving with either line ending canonicalises the same way.
            if (original == U'\r' && alphabet.contains(U'\n')) ++r.whitespace_mapped;
            else
            {
                const char32_t as = alphabet.contains(original) ? original : U' ';
                if (as != original) ++r.whitespace_mapped;
                expanded.push_back(as);
            }
        }
        else if (version == CanonVersion::V2)
        {
            if (const char* t = lookup(std::begin(kTransliterate), std::end(kTransliterate), original))
            {
                ++r.transliterated;
                for (const char* p = t; *p; ++p) expanded.push_back(static_cast<char32_t>(*p));
            }
            else if (const char* f = lookup(std::begin(kFold), std::end(kFold), original))
            {
                ++r.accents_folded;
                for (const char* p = f; *p; ++p) expanded.push_back(static_cast<char32_t>(*p));
            }
            else expanded.push_back(original);
        }
        else expanded.push_back(original);

        for (char32_t c : expanded)
        {
            // Step 4.
            if (!alphabet.has_uppercase() && c >= U'A' && c <= U'Z')
            {
                c = c - U'A' + U'a';
                ++r.case_folded;
            }
            if (alphabet.contains(c))
            {
                kept.push_back(c);
                continue;
            }
            if (version == CanonVersion::V1 || c == U'\'')
            {
                ++r.dropped;
                note_example(r.dropped_examples, original);
                continue;
            }
            ++r.separated;
            note_example(r.separated_examples, original);
            kept.push_back(U' ');
        }
    }

    // Step 5.
    std::u32string canon;
    canon.reserve(kept.size());
    for (char32_t c : kept)
    {
        if (c == U' ' && (canon.empty() || canon.back() == U' '))
        {
            ++r.spaces_collapsed;
            continue;
        }
        canon.push_back(c);
    }
    while (!canon.empty() && canon.back() == U' ')
    {
        canon.pop_back();
        ++r.spaces_collapsed;
    }
    r.canonical_length = canon.size();

    // Step 6.
    for (size_t i = 0; i < canon.size(); i += unit_length)
    {
        std::u32string unit = canon.substr(i, unit_length);
        if (unit.size() < unit_length)
        {
            // Padding is digit 0, which for every alphabet pinned before ascii96 is the space.
            // On ascii96 it is the line feed, so a short .obj is padded with blank lines.
            r.padding = unit_length - unit.size();
            unit.append(r.padding, alphabet.symbol(0));
        }
        r.units.push_back(std::move(unit));
    }
    return r;
}

} // namespace sieve
