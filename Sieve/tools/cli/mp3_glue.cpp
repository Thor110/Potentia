#include "cli/mp3_glue.hpp"

#include <cstdlib>
#include <cstring>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>

#include "packmp3lib.h"

namespace sieve::cli {

namespace {

std::mutex g_packmp3; // (packMP3's globals: one conversion at a time)

// One conversion by packMP3, whichever way its input's first bytes say (PMP's magic: to an MP3; else to
// PMP); nothing where it fails or throws (a damaged stream, a failed allocation).
std::optional<std::vector<uint8_t>> convert(const std::vector<uint8_t>& in, std::string& why)
{
    if (in.size() < 2 || in.size() > size_t(0x7FFFFFFF)) return std::nullopt;
    std::lock_guard<std::mutex> lock(g_packmp3);
    std::vector<unsigned char> copy(in.begin(), in.end()); // (its streams take a non-const pointer)
    char msg[256] = {0};
    unsigned char* out = nullptr;
    unsigned int n = 0;
    bool ok = false;
    try
    {
        pmplib_init_streams(copy.data(), 1, int(copy.size()), nullptr, 1);
        ok = pmplib_convert_stream2mem(&out, &n, msg);
    }
    catch (const std::exception& e)
    {
        why = e.what();
        ok = false;
    }
    std::optional<std::vector<uint8_t>> r;
    if (ok && out) r.emplace(out, out + n);
    else if (why.empty()) why = msg;
    if (out) std::free(out);
    return r;
}

} // namespace

bool is_mp3(const std::vector<uint8_t>& b)
{
    return b.size() >= 4 && ((std::memcmp(b.data(), "ID3", 3) == 0) || (b[0] == 0xFF && (b[1] & 0xFE) == 0xFA));
}

std::optional<std::vector<uint8_t>> mp3_to_pmp(const std::vector<uint8_t>& b)
{
    if (!is_mp3(b)) return std::nullopt;
    std::string why;
    auto pmp = convert(b, why);
    // (its own form must start with its magic, so the way back is taken; and give b back exactly)
    if (!pmp || pmp->size() < 2 || is_mp3(*pmp)) return std::nullopt;
    auto back = convert(*pmp, why);
    if (!back || *back != b) return std::nullopt;
    return pmp;
}

std::vector<uint8_t> pmp_to_mp3(const std::vector<uint8_t>& pmp)
{
    // (an MP3's own bytes given as "PMP" would be coded, not decoded: refused)
    if (pmp.size() < 2 || is_mp3(pmp)) throw std::runtime_error("an MP3's packMP3 form this Sieve does not read");
    std::string why;
    auto mp3 = convert(pmp, why);
    if (!mp3 || !is_mp3(*mp3)) throw std::runtime_error("an MP3's packMP3 form that does not give back an MP3" + (why.empty() ? std::string() : " (" + why + ")"));
    return std::move(*mp3);
}

} // namespace sieve::cli
