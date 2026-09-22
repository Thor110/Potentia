// Sieve CLI — command-line argument parsing.
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

// Options that are switches (take no value).
inline bool is_flag(const std::string& key)
{
    return key == "short" || key == "help" || key == "take" || key == "menu" || key == "no-menu" || key == "compact";
}

struct Args
{
    std::string command;
    std::map<std::string, std::string> opts;
    std::vector<std::string> positional;
    bool help = false;

    bool has(const std::string& k) const { return opts.count(k) != 0; }
    std::string get(const std::string& k, const std::string& def = "") const
    {
        auto it = opts.find(k);
        return it == opts.end() ? def : it->second;
    }
    uint32_t get_u32(const std::string& k, uint32_t def) const
    {
        if (!has(k)) return def;
        const std::string v = get(k);
        size_t used = 0;
        unsigned long n = 0;
        try { n = std::stoul(v, &used); } catch (const std::exception&) { used = 0; }
        if (used != v.size() || v.empty() || n > 0xFFFFFFFFul)
            throw std::invalid_argument("--" + k + " expects a whole number, got '" + v + "'");
        return static_cast<uint32_t>(n);
    }
    uint32_t get_positive(const std::string& k, uint32_t def) const
    {
        const uint32_t v = get_u32(k, def);
        if (v == 0) throw std::invalid_argument("--" + k + " must be at least 1");
        return v;
    }
    uint32_t require_positive(const std::string& k) const
    {
        if (!has(k)) throw std::invalid_argument("missing --" + k);
        return get_positive(k, 0);
    }
};

inline Args parse_args(int argc, char** argv)
{
    Args a;
    if (argc < 2) throw std::invalid_argument("no command given");
    a.command = argv[1];
    for (int i = 2; i < argc; ++i)
    {
        const std::string s = argv[i];
        if (s == "--help" || s == "-h") { a.help = true; continue; }
        if (s.rfind("--", 0) == 0 && s.size() > 2)
        {
            const std::string key = s.substr(2);
            if (is_flag(key)) { a.opts[key] = "1"; continue; }
            if (i + 1 >= argc) throw std::invalid_argument("option --" + key + " needs a value");
            a.opts[key] = argv[++i];
        }
        else a.positional.push_back(s);
    }
    return a;
}

} // namespace sieve::cli
