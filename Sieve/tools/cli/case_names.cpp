// Sieve — names that are one name where case is ignored (case_names.hpp).
#include "case_names.hpp"
#include "case_fold_table.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>

#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_map>

namespace sieve::cli
{

namespace
{

// A character folded as a folder ignoring case folds it (case_fold_table.hpp, one character to one).
uint32_t fold_cp(uint32_t c)
{
    if (c < 0x80) return c >= 'A' && c <= 'Z' ? c - 'A' + 'a' : c;
    const auto* end = std::end(kCaseFold);
    const auto* it = std::lower_bound(std::begin(kCaseFold), end, c, [](const CaseFoldPair& p, uint32_t v) { return p.from < v; });
    return it != end && it->from == c ? it->to : c;
}

// A name with every letter folded: A to Z, and (6 November 2026, IDEAS 19.120) every other letter
// Unicode gives a case, so É.txt and é.txt are one name, as on Windows and macOS. Read as UTF-8; a
// byte that does not begin a well-formed character is kept as it is.
std::string folded(const std::string& p)
{
    std::string f;
    f.reserve(p.size());
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
            f.push_back(p[i]);
            ++i;
            continue;
        }
        const uint32_t g = fold_cp(c);
        if (g < 0x80) f.push_back(char(g));
        else if (g < 0x800) f.push_back(char(0xC0 | g >> 6)), f.push_back(char(0x80 | (g & 0x3F)));
        else if (g < 0x10000) f.push_back(char(0xE0 | g >> 12)), f.push_back(char(0x80 | (g >> 6 & 0x3F))), f.push_back(char(0x80 | (g & 0x3F)));
        else f.push_back(char(0xF0 | g >> 18)), f.push_back(char(0x80 | (g >> 12 & 0x3F))), f.push_back(char(0x80 | (g >> 6 & 0x3F))), f.push_back(char(0x80 | (g & 0x3F)));
        i += n;
    }
    return f;
}

} // namespace

std::optional<std::pair<std::string, std::string>> names_one_ignoring_case(const std::vector<std::string>& files)
{
    std::unordered_map<std::string, const std::string*> seen;
    seen.reserve(files.size());
    for (const std::string& p : files)
    {
        auto [at, fresh] = seen.emplace(folded(p), &p);
        if (!fresh && *at->second != p) return std::make_pair(*at->second, p);
    }
    // A file and a folder that another file is in: `a` beside `A/b`.
    for (const std::string& p : files)
        for (size_t s = p.find('/'); s != std::string::npos; s = p.find('/', s + 1))
            if (auto it = seen.find(folded(p.substr(0, s))); it != seen.end()) return std::make_pair(*it->second, p);
    return std::nullopt;
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
        if (stem == "CON" || stem == "PRN" || stem == "AUX" || stem == "NUL") return "a device's name (" + stem + ")";
        if (stem.size() == 4 && (stem.compare(0, 3, "COM") == 0 || stem.compare(0, 3, "LPT") == 0) && stem[3] >= '0' && stem[3] <= '9')
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
    if (auto pair = names_one_ignoring_case(files); pair && folders_ignore_case(dest))
        throw std::runtime_error(pair->first + " and " + pair->second +
                                 " are one name here, where case is ignored: installing both would write one over the other, so nothing is written");
}

} // namespace sieve::cli
