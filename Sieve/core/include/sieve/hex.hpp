// Sieve — bytes as lower-case hex, two digits a byte, and hex digits read back: the one copy the
// programs share.
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>

namespace sieve {

// The bytes in hex, high digit first; nothing for no bytes.
inline std::string hex_of(std::span<const uint8_t> bytes)
{
    static const char k[] = "0123456789abcdef";
    std::string s;
    for (const uint8_t c : bytes) s += {k[c >> 4], k[c & 15]};
    return s;
}

// Whether the text is exactly `digits` lower-case hex digits. Upper case is refused, as the formats
// that write hex write it in lower case only, so one value has one spelling.
inline bool is_lower_hex(std::string_view s, size_t digits)
{
    return s.size() == digits && s.find_first_not_of("0123456789abcdef") == std::string_view::npos;
}

// Whether the text is a SHA-256 as Sieve writes one: 64 lower-case hex digits.
inline bool is_sha256_hex(std::string_view s) { return is_lower_hex(s, 64); }

// One lower-case hex digit's value, or -1: for the formats that refuse upper case.
inline int lower_hex_digit_value(char c) { return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1; }

// One hex digit's value in either case, or -1: for text a person may have typed.
inline int hex_digit_value(char c) { return c >= 'A' && c <= 'F' ? c - 'A' + 10 : lower_hex_digit_value(c); }

} // namespace sieve
