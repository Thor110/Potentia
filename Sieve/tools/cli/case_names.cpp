// Sieve — names that are one name where case is ignored (case_names.hpp).
#include "case_names.hpp"

#include <filesystem>
#include <stdexcept>
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

void refuse_names_one_ignoring_case(const std::vector<std::string>& files, const std::filesystem::path& dest)
{
    if (auto pair = names_one_ignoring_case(files); pair && folders_ignore_case(dest))
        throw std::runtime_error(pair->first + " and " + pair->second +
                                 " are one name here, where case is ignored: installing both would write one over the other, so nothing is written");
}

} // namespace sieve::cli
