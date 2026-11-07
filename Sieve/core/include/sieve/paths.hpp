// Sieve — a path's UTF-8 bytes, and back: the one copy every program shares.
#pragma once

#include <filesystem>
#include <string>

namespace sieve {

// The path as UTF-8 bytes in a plain string.
inline std::string u8(const std::filesystem::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

// The path the UTF-8 bytes name.
inline std::filesystem::path from_u8(const std::string& s)
{
    return std::filesystem::path(std::u8string(s.begin(), s.end()));
}

} // namespace sieve
