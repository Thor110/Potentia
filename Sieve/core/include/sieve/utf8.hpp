// Sieve — strict UTF-8 encoding and decoding.
#pragma once

#include <string>
#include <string_view>

namespace sieve {

// Decodes strict UTF-8. Throws std::invalid_argument on malformed input,
// overlong encodings, surrogates or codepoints above U+10FFFF.
std::u32string utf8_decode(std::string_view bytes);

// Encodes codepoints as UTF-8. Throws std::invalid_argument for invalid codepoints.
std::string utf8_encode(std::u32string_view text);
std::string utf8_encode(char32_t cp);

} // namespace sieve
