// Sieve — bytes as lower-case hex, two digits a byte: the one copy the programs share.
#pragma once

#include <cstdint>
#include <span>
#include <string>

namespace sieve {

// The bytes in hex, high digit first; nothing for no bytes.
inline std::string hex_of(std::span<const uint8_t> bytes)
{
    static const char k[] = "0123456789abcdef";
    std::string s;
    for (const uint8_t c : bytes) s += {k[c >> 4], k[c & 15]};
    return s;
}

} // namespace sieve
