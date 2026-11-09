// Sieve — names that are one name where case is ignored (case_names.hpp).
#include "case_names.hpp"
#include "case_fold_table.hpp"
#include "normalise_table.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>

#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace sieve::cli
{

namespace
{

// A character folded as a folder ignoring case folds it (case_fold_table.hpp, one character to one):
// to its uppercase, as Windows compares names.
uint32_t fold_cp(uint32_t c)
{
    if (c < 0x80) return c >= 'a' && c <= 'z' ? c - 'a' + 'A' : c;
    const auto* end = std::end(kCaseFold);
    const auto* it = std::lower_bound(std::begin(kCaseFold), end, c, [](const CaseFoldPair& p, uint32_t v) { return p.from < v; });
    return it != end && it->from == c ? it->to : c;
}

// A character's canonical decomposition, in full (normalise_table.hpp; Hangul syllables by rule).
void decompose(uint32_t c, std::vector<uint32_t>& out)
{
    if (c >= 0xAC00 && c <= 0xD7A3)
    {
        const uint32_t s = c - 0xAC00, t = s % 28;
        out.push_back(0x1100 + s / 588);
        out.push_back(0x1161 + (s % 588) / 28);
        if (t) out.push_back(0x11A7 + t);
        return;
    }
    const auto* end = std::end(kDecompositions);
    const auto* it = std::lower_bound(std::begin(kDecompositions), end, c, [](const Decomposition& d, uint32_t v) { return d.from < v; });
    if (it == end || it->from != c)
    {
        out.push_back(c);
        return;
    }
    for (const uint32_t t : it->to)
        if (t) out.push_back(t);
}

uint8_t combining_class(uint32_t c)
{
    if (c < 0x300) return 0;
    const auto* end = std::end(kCombiningClasses);
    const auto* it = std::lower_bound(std::begin(kCombiningClasses), end, c, [](const CombiningClass& k, uint32_t v) { return k.cp < v; });
    return it != end && it->cp == c ? it->ccc : 0;
}

// A name as the folders compare it: every letter Unicode gives a case folded (A to Z, and, 6 November
// 2026, IDEAS 19.120, every other: É.txt and é.txt one name, as on Windows and macOS); and with
// `normalise`, every character decomposed and its marks put in their canonical order first (é one
// character, and e with a combining accent, one name, as macOS has them; Windows keeps them two).
// Read as UTF-8; a byte that does not begin a well-formed character is kept as it is.
std::string folded(const std::string& p, bool fold_case = true, bool normalise = false)
{
    constexpr uint32_t kRaw = 0x110000; // (a byte kept as it is: kRaw + the byte)
    std::vector<uint32_t> cps;
    cps.reserve(p.size());
    for (size_t i = 0; i < p.size();)
    {
        const auto b = static_cast<unsigned char>(p[i]);
        size_t n = b < 0x80 ? 1 : (b >> 5) == 6 ? 2 : (b >> 4) == 14 ? 3 : (b >> 3) == 30 ? 4 : 0;
        uint32_t c = n == 1 ? b : n == 2 ? (b & 0x1F) : n == 3 ? (b & 0x0F) : (b & 0x07);
        bool ok = n != 0 && i + n <= p.size();
        for (size_t k = 1; ok && k < n; ++k)
        {
            const auto t = static_cast<unsigned char>(p[i + k]);
            if ((t >> 6) != 2) ok = false;
            else c = c << 6 | (t & 0x3F);
        }
        // (the shortest form only, and no surrogate or code point past Unicode's)
        if (ok && ((n == 2 && c < 0x80) || (n == 3 && c < 0x800) || (n == 4 && (c < 0x10000 || c > 0x10FFFF)) || (c >= 0xD800 && c <= 0xDFFF))) ok = false;
        if (!ok)
        {
            cps.push_back(kRaw + b);
            ++i;
            continue;
        }
        cps.push_back(c);
        i += n;
    }
    if (normalise)
    {
        std::vector<uint32_t> d;
        d.reserve(cps.size());
        for (const uint32_t c : cps)
            if (c >= kRaw) d.push_back(c);
            else decompose(c, d);
        cps.swap(d);
    }
    if (fold_case)
        for (uint32_t& c : cps)
            if (c < kRaw) c = fold_cp(c);
    if (normalise)
    {
        // (a folded letter decomposed again where it has a decomposition; then each run of marks in
        // the canonical order, by combining class, equal classes as they came)
        std::vector<uint32_t> d;
        d.reserve(cps.size());
        for (const uint32_t c : cps)
            if (c >= kRaw) d.push_back(c);
            else decompose(c, d);
        cps.swap(d);
        for (size_t i = 0; i < cps.size();)
        {
            if (cps[i] >= kRaw || !combining_class(cps[i]))
            {
                ++i;
                continue;
            }
            size_t j = i;
            while (j < cps.size() && cps[j] < kRaw && combining_class(cps[j])) ++j;
            std::stable_sort(cps.begin() + std::ptrdiff_t(i), cps.begin() + std::ptrdiff_t(j),
                             [](uint32_t x, uint32_t y) { return combining_class(x) < combining_class(y); });
            i = j;
        }
    }
    std::string f;
    f.reserve(p.size());
    for (const uint32_t g : cps)
    {
        if (g >= kRaw) f.push_back(char(g - kRaw));
        else if (g < 0x80) f.push_back(char(g));
        else if (g < 0x800) f.push_back(char(0xC0 | g >> 6)), f.push_back(char(0x80 | (g & 0x3F)));
        else if (g < 0x10000) f.push_back(char(0xE0 | g >> 12)), f.push_back(char(0x80 | (g >> 6 & 0x3F))), f.push_back(char(0x80 | (g & 0x3F)));
        else f.push_back(char(0xF0 | g >> 18)), f.push_back(char(0x80 | (g >> 12 & 0x3F))), f.push_back(char(0x80 | (g >> 6 & 0x3F))), f.push_back(char(0x80 | (g & 0x3F)));
    }
    return f;
}

// The first two of `files` that are one name compared so, or a file one name with a folder another
// file is in.
std::optional<std::pair<std::string, std::string>> names_one(const std::vector<std::string>& files, bool fold_case, bool normalise)
{
    std::unordered_map<std::string, const std::string*> seen;
    seen.reserve(files.size());
    for (const std::string& p : files)
    {
        auto [at, fresh] = seen.emplace(folded(p, fold_case, normalise), &p);
        if (!fresh && *at->second != p) return std::make_pair(*at->second, p);
    }
    // A file and a folder that another file is in: `a` beside `A/b`.
    for (const std::string& p : files)
        for (size_t s = p.find('/'); s != std::string::npos; s = p.find('/', s + 1))
            if (auto it = seen.find(folded(p.substr(0, s), fold_case, normalise)); it != seen.end())
                return std::make_pair(*it->second, p);
    return std::nullopt;
}

} // namespace

std::optional<std::pair<std::string, std::string>> names_one_ignoring_case(const std::vector<std::string>& files)
{
    return names_one(files, true, false);
}

std::optional<std::pair<std::string, std::string>> names_one_on_macos(const std::vector<std::string>& files)
{
    return names_one(files, true, true);
}

bool folders_ignore_case(const std::filesystem::path& dest)
{
    namespace fs = std::filesystem;
    std::error_code ec;
    for (fs::path p = fs::absolute(dest, ec); !ec && !p.empty() && p.has_relative_path(); p = p.parent_path())
    {
        if (!fs::is_directory(p, ec)) continue;
        std::string name = p.filename().string();
        bool letter = false;
        for (char& c : name)
            if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A'), letter = true;
            else if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a'), letter = true;
        if (!letter) continue;
        const fs::path turned = p.parent_path() / name;
        if (!fs::exists(turned, ec)) return false;
        return fs::equivalent(p, turned, ec) && !ec;
    }
#if defined(_WIN32) || defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

std::optional<std::pair<std::string, std::string>> name_windows_cannot_hold(const std::vector<std::string>& files)
{
    auto why_not = [](const std::string& part) -> std::string {
        for (const char c : part)
            if (static_cast<unsigned char>(c) < 32) return "a control character";
            else if (std::string_view("<>:\"\\|?*").find(c) != std::string_view::npos) return std::string("the character ") + c;
        if (!part.empty() && (part.back() == '.' || part.back() == ' ')) return part.back() == '.' ? "a dot at its end" : "a space at its end";
        std::string stem = part.substr(0, part.find('.'));
        while (!stem.empty() && stem.back() == ' ') stem.pop_back();
        for (char& c : stem)
            if (c >= 'a' && c <= 'z') c = char(c - 'a' + 'A');
        if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL" || stem == "CONIN$" || stem == "CONOUT$") return "a device's name (" + stem + ")";
        if (stem.size() == 4 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0) && stem[3] >= '0' && stem[3] <= '9')
            return "a device's name (" + stem + ")";
        // (COM and LPT with a superscript one, two or three, which Windows also takes for devices)
        if (stem.size() == 5 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0) && static_cast<unsigned char>(stem[3]) == 0xC2 &&
            (static_cast<unsigned char>(stem[4]) == 0xB9 || static_cast<unsigned char>(stem[4]) == 0xB2 || static_cast<unsigned char>(stem[4]) == 0xB3))
            return "a device's name (" + stem + ")";
        return {};
    };
    for (const std::string& p : files)
        for (size_t at = 0; at <= p.size();)
        {
            size_t s = p.find('/', at);
            if (s == std::string::npos) s = p.size();
            if (const std::string why = why_not(p.substr(at, s - at)); !why.empty()) return std::make_pair(p, why);
            at = s + 1;
        }
    return std::nullopt;
}

void refuse_names_windows_cannot_hold(const std::vector<std::string>& files)
{
#ifdef _WIN32
    if (auto bad = name_windows_cannot_hold(files))
        throw std::runtime_error(bad->first + " is a name Windows cannot hold (" + bad->second +
                                 "): written here it would be another file, so nothing is written");
#else
    (void)files;
#endif
}

void refuse_names_one_ignoring_case(const std::vector<std::string>& files, const std::filesystem::path& dest)
{
    const bool ignores = folders_ignore_case(dest);
    if (auto pair = names_one_ignoring_case(files); pair && ignores)
        throw std::runtime_error(pair->first + " and " + pair->second +
                                 " are one name here, where case is ignored: installing both would write one over the other, so nothing is written");
#if defined(__APPLE__)
    // (macOS also takes a letter written as one character and as a letter and its accent as one
    // name, whether its folders heed case or not: 6 November 2026)
    if (auto pair = names_one(files, ignores, true))
        throw std::runtime_error(pair->first + " and " + pair->second +
                                 " are one name here, the same letters written in two ways: installing both would write one over the other, so nothing is written");
#endif
}

} // namespace sieve::cli
