// Sieve — sealing bytes for others to keep (see seal.hpp).
//
// The infinite RAID save and data preservation system was designed by Edward James Gordon (Potentia)
// (additional term under section 7(b) of the GNU AGPL v3, docs/RAID-NOTICE.md).
#include "sieve/seal.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <cstring>
#include <span>
#include <random>
#include <stdexcept>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <bcrypt.h>
#endif

namespace sieve {

namespace {

std::string_view as_view(const std::array<uint8_t, 32>& a) { return {reinterpret_cast<const char*>(a.data()), a.size()}; }

uint32_t rotl(uint32_t v, int n) { return (v << n) | (v >> (32 - n)); }

void quarter(uint32_t& a, uint32_t& b, uint32_t& c, uint32_t& d)
{
    a += b; d ^= a; d = rotl(d, 16);
    c += d; b ^= c; b = rotl(b, 12);
    a += b; d ^= a; d = rotl(d, 8);
    c += d; b ^= c; b = rotl(b, 7);
}

uint32_t le32(const uint8_t* p) { return uint32_t(p[0]) | uint32_t(p[1]) << 8 | uint32_t(p[2]) << 16 | uint32_t(p[3]) << 24; }

// Equal in time that does not depend on where they differ.
bool same(std::string_view a, std::string_view b)
{
    if (a.size() != b.size()) return false;
    uint8_t d = 0;
    for (size_t i = 0; i < a.size(); ++i) d |= uint8_t(a[i] ^ b[i]);
    return d == 0;
}

struct Keys
{
    std::array<uint8_t, 32> enc, mac;
};

Keys keys_of(const SealKey& key)
{
    return {hmac_sha256(as_view(key), "sieve-seal-v1 encrypt"), hmac_sha256(as_view(key), "sieve-seal-v1 mac")};
}

std::array<uint8_t, 12> nonce_of(const Keys& k, std::string_view plain)
{
    std::string m = "sieve-seal-v1 nonce";
    m.append(plain);
    const auto h = hmac_sha256(as_view(k.mac), m);
    std::array<uint8_t, 12> n{};
    std::memcpy(n.data(), h.data(), 12);
    return n;
}

} // namespace

std::array<uint8_t, 32> hmac_sha256(std::string_view key, std::string_view message)
{
    std::array<uint8_t, 64> k{};
    if (key.size() > 64)
    {
        const auto d = Sha256::hash(key);
        std::memcpy(k.data(), d.data(), d.size());
    }
    else
        std::memcpy(k.data(), key.data(), key.size());
    std::array<uint8_t, 64> ipad{}, opad{};
    for (size_t i = 0; i < 64; ++i)
    {
        ipad[i] = uint8_t(k[i] ^ 0x36);
        opad[i] = uint8_t(k[i] ^ 0x5c);
    }
    Sha256 in;
    in.update(std::span<const uint8_t>(ipad));
    in.update(message);
    const auto inner = in.finish();
    Sha256 out;
    out.update(std::span<const uint8_t>(opad));
    out.update(std::span<const uint8_t>(inner));
    return out.finish();
}

std::string chacha20(const std::array<uint8_t, 32>& key, const std::array<uint8_t, 12>& nonce, uint32_t counter, std::string_view bytes)
{
    uint32_t state[16] = {0x61707865, 0x3320646e, 0x79622d32, 0x6b206574};
    for (int i = 0; i < 8; ++i) state[4 + i] = le32(key.data() + 4 * i);
    state[12] = counter;
    for (int i = 0; i < 3; ++i) state[13 + i] = le32(nonce.data() + 4 * i);
    std::string out(bytes);
    for (size_t at = 0; at < out.size(); at += 64)
    {
        uint32_t x[16];
        std::memcpy(x, state, sizeof x);
        for (int r = 0; r < 10; ++r)
        {
            quarter(x[0], x[4], x[8], x[12]);
            quarter(x[1], x[5], x[9], x[13]);
            quarter(x[2], x[6], x[10], x[14]);
            quarter(x[3], x[7], x[11], x[15]);
            quarter(x[0], x[5], x[10], x[15]);
            quarter(x[1], x[6], x[11], x[12]);
            quarter(x[2], x[7], x[8], x[13]);
            quarter(x[3], x[4], x[9], x[14]);
        }
        uint8_t block[64];
        for (int i = 0; i < 16; ++i)
        {
            const uint32_t v = x[i] + state[i];
            block[4 * i] = uint8_t(v);
            block[4 * i + 1] = uint8_t(v >> 8);
            block[4 * i + 2] = uint8_t(v >> 16);
            block[4 * i + 3] = uint8_t(v >> 24);
        }
        const size_t n = std::min<size_t>(64, out.size() - at);
        for (size_t i = 0; i < n; ++i) out[at + i] = char(uint8_t(out[at + i]) ^ block[i]);
        ++state[12];
    }
    return out;
}

std::string seal(const SealKey& key, std::string_view plain)
{
    const Keys k = keys_of(key);
    const auto nonce = nonce_of(k, plain);
    std::string s(reinterpret_cast<const char*>(nonce.data()), nonce.size());
    s += chacha20(k.enc, nonce, 0, plain);
    const auto tag = hmac_sha256(as_view(k.mac), s);
    s.append(as_view(tag));
    return s;
}

std::optional<std::string> unseal(const SealKey& key, std::string_view sealed)
{
    if (sealed.size() < kSealOverhead) return std::nullopt;
    const Keys k = keys_of(key);
    const std::string_view body = sealed.substr(0, sealed.size() - 32);
    const auto tag = hmac_sha256(as_view(k.mac), body);
    if (!same(as_view(tag), sealed.substr(sealed.size() - 32))) return std::nullopt;
    std::array<uint8_t, 12> nonce{};
    std::memcpy(nonce.data(), body.data(), 12);
    std::string plain = chacha20(k.enc, nonce, 0, body.substr(12));
    if (nonce_of(k, plain) != nonce) return std::nullopt;
    return plain;
}

SealKey new_seal_key()
{
    SealKey k{};
#ifdef _WIN32
    // The system's generator, asked directly: some MinGW builds' std::random_device gave a fixed
    // sequence, which would make every key the same.
    if (BCryptGenRandom(nullptr, k.data(), ULONG(k.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) throw std::runtime_error("the system's random source did not answer");
#else
    std::random_device rd; // /dev/urandom or the CPU's generator, with libstdc++ and libc++
    for (size_t i = 0; i < k.size(); i += 4)
    {
        const uint32_t v = rd();
        for (size_t j = 0; j < 4; ++j) k[i + j] = uint8_t(v >> (8 * j));
    }
#endif
    return k;
}

std::string seal_key_text(const SealKey& key)
{
    static const char* kHex = "0123456789abcdef";
    std::string t = "sieve-raid-key-v1 ";
    for (const uint8_t b : key)
    {
        t += kHex[b >> 4];
        t += kHex[b & 15];
    }
    return t + "\n";
}

SealKey read_seal_key(std::string_view text)
{
    const std::string_view want = "sieve-raid-key-v1 ";
    while (!text.empty() && (text.back() == '\n' || text.back() == '\r')) text.remove_suffix(1);
    if (text.substr(0, want.size()) != want || text.size() != want.size() + 64) throw std::invalid_argument("not a RAID key: it is \"sieve-raid-key-v1\" and 64 hex digits");
    SealKey k{};
    for (size_t i = 0; i < 32; ++i)
    {
        int v = 0;
        for (size_t j = 0; j < 2; ++j)
        {
            const char c = text[want.size() + 2 * i + j];
            const int d = c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
            if (d < 0) throw std::invalid_argument("not a RAID key: its digits are not lower-case hex");
            v = v * 16 + d;
        }
        k[i] = uint8_t(v);
    }
    return k;
}

} // namespace sieve
