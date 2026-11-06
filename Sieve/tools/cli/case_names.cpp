// Sieve — names that are one name where case is ignored (case_names.hpp).
#include "case_names.hpp"

#include <filesystem>
#include <stdexcept>
#include <string_view>
#include <system_error>
#include <unordered_map>

namespace sieve::cli
{

namespace
{

std::string folded(const std::string& p)
{
    std::string f = p;
    for (char& c : f)
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
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
