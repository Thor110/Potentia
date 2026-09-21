#include "sieve/utf8.hpp"

#include <stdexcept>

namespace sieve {

namespace {
bool valid_scalar(char32_t cp) { return cp <= 0x10FFFF && !(cp >= 0xD800 && cp <= 0xDFFF); }
} // namespace

std::u32string utf8_decode(std::string_view bytes)
{
    std::u32string out;
    out.reserve(bytes.size());
    size_t i = 0;
    while (i < bytes.size())
    {
        const auto b0 = static_cast<unsigned char>(bytes[i]);
        char32_t cp = 0;
        size_t extra = 0;
        char32_t min = 0;
        if (b0 < 0x80) { cp = b0; extra = 0; min = 0; }
        else if ((b0 & 0xE0) == 0xC0) { cp = b0 & 0x1F; extra = 1; min = 0x80; }
        else if ((b0 & 0xF0) == 0xE0) { cp = b0 & 0x0F; extra = 2; min = 0x800; }
        else if ((b0 & 0xF8) == 0xF0) { cp = b0 & 0x07; extra = 3; min = 0x10000; }
        else throw std::invalid_argument("invalid UTF-8 lead byte at offset " + std::to_string(i));

        if (i + extra >= bytes.size() && extra > 0)
            throw std::invalid_argument("truncated UTF-8 sequence at offset " + std::to_string(i));
        for (size_t k = 1; k <= extra; ++k)
        {
            const auto b = static_cast<unsigned char>(bytes[i + k]);
            if ((b & 0xC0) != 0x80)
                throw std::invalid_argument("invalid UTF-8 continuation byte at offset " + std::to_string(i + k));
            cp = (cp << 6) | (b & 0x3F);
        }
        if (cp < min) throw std::invalid_argument("overlong UTF-8 encoding at offset " + std::to_string(i));
        if (!valid_scalar(cp)) throw std::invalid_argument("invalid codepoint in UTF-8 at offset " + std::to_string(i));
        out.push_back(cp);
        i += extra + 1;
    }
    return out;
}

std::string utf8_encode(char32_t cp)
{
    if (!valid_scalar(cp)) throw std::invalid_argument("cannot encode invalid codepoint");
    std::string s;
    if (cp < 0x80) s.push_back(static_cast<char>(cp));
    else if (cp < 0x800)
    {
        s.push_back(static_cast<char>(0xC0 | (cp >> 6)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else if (cp < 0x10000)
    {
        s.push_back(static_cast<char>(0xE0 | (cp >> 12)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    else
    {
        s.push_back(static_cast<char>(0xF0 | (cp >> 18)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
        s.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
    return s;
}

std::string utf8_encode(std::u32string_view text)
{
    std::string s;
    s.reserve(text.size());
    for (char32_t cp : text) s += utf8_encode(cp);
    return s;
}

} // namespace sieve
