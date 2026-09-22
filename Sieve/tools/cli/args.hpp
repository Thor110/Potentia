// Sieve CLI — command-line argument parsing.
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

// A whole number written with decimal digits only (no sign, no spaces), at most `max`.
inline uint64_t parse_whole(const std::string& v, const std::string& what, uint64_t max = 0xFFFFFFFFull)
{
    if (v.empty() || v.size() > 20 || v.find_first_not_of("0123456789") != std::string::npos)
        throw std::invalid_argument(what + " expects a whole number, got '" + v + "'");
    uint64_t n = 0;
    for (char c : v)
    {
        if (n > (max - uint64_t(c - '0')) / 10) throw std::invalid_argument(what + " is too large (at most " + std::to_string(max) + "): '" + v + "'");
        n = n * 10 + uint64_t(c - '0');
    }
    return n;
}

// Options that are switches (take no value).
inline bool is_flag(const std::string& key)
{
    return key == "short" || key == "help" || key == "take" || key == "menu" || key == "main-menu" || key == "edge-glow" || key == "real-graphics" || key == "no-menu" || key == "compact";
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
        return static_cast<uint32_t>(parse_whole(get(k), "--" + k));
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
