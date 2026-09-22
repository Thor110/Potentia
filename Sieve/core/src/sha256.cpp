#include "sieve/sha256.hpp"

#include <atomic>

#include <algorithm>
#include <cstring>

#if defined(__x86_64__) || defined(_M_X64)
#define SIEVE_SHA_X86 1
#include <immintrin.h>
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

namespace sieve {

namespace {

alignas(16) constexpr uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

constexpr uint32_t rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

// Portable compression function: `blocks` consecutive 64-byte blocks.
void blocks_portable(uint32_t* h, const uint8_t* p, size_t blocks)
{
    for (; blocks--; p += 64)
    {
        uint32_t w[64];
        for (int i = 0; i < 16; ++i)
            w[i] = (uint32_t(p[4 * i]) << 24) | (uint32_t(p[4 * i + 1]) << 16) | (uint32_t(p[4 * i + 2]) << 8) |
                   uint32_t(p[4 * i + 3]);
        for (int i = 16; i < 64; ++i)
        {
            const uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
        for (int i = 0; i < 64; ++i)
        {
            const uint32_t S1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            const uint32_t ch = (e & f) ^ (~e & g);
            const uint32_t t1 = hh + S1 + ch + K[i] + w[i];
            const uint32_t S0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2 = S0 + maj;
            hh = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
        }
        h[0] += a; h[1] += b; h[2] += c; h[3] += d; h[4] += e; h[5] += f; h[6] += g; h[7] += hh;
    }
}

#if SIEVE_SHA_X86

bool cpu_has_sha()
{
#if defined(_MSC_VER)
    int r[4];
    __cpuid(r, 0);
    if (r[0] < 7) return false;
    __cpuid(r, 1);
    const bool sse41 = ((r[2] >> 19) & 1) != 0, ssse3 = ((r[2] >> 9) & 1) != 0;
    __cpuidex(r, 7, 0);
    return sse41 && ssse3 && ((r[1] >> 29) & 1) != 0;
#else
    unsigned a = 0, b = 0, c = 0, d = 0;
    if (__get_cpuid_max(0, nullptr) < 7) return false;
    __get_cpuid(1, &a, &b, &c, &d);
    const bool sse41 = ((c >> 19) & 1) != 0, ssse3 = ((c >> 9) & 1) != 0;
    __get_cpuid_count(7, 0, &a, &b, &c, &d);
    return sse41 && ssse3 && ((b >> 29) & 1) != 0;
#endif
}

// SHA-NI compression. Each loop step does four rounds; the message schedule for steps 4..15 is
// W[i] = msg2(msg1(W[i-4], W[i-3]) + alignr(W[i-1], W[i-2]), W[i-1]).
#if defined(__GNUC__) || defined(__clang__)
__attribute__((target("sha,sse4.1,ssse3")))
#endif
void blocks_shani(uint32_t* h, const uint8_t* p, size_t blocks)
{
    const __m128i swap = _mm_set_epi64x(0x0c0d0e0f08090a0bULL, 0x0405060700010203ULL);
    __m128i tmp = _mm_loadu_si128(reinterpret_cast<const __m128i*>(h));
    __m128i state1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(h + 4));
    tmp = _mm_shuffle_epi32(tmp, 0xB1);              // CDAB
    state1 = _mm_shuffle_epi32(state1, 0x1B);        // EFGH
    __m128i state0 = _mm_alignr_epi8(tmp, state1, 8); // ABEF
    state1 = _mm_blend_epi16(state1, tmp, 0xF0);     // CDGH

    for (; blocks--; p += 64)
    {
        const __m128i save0 = state0, save1 = state1;
        __m128i w[4];
        for (int i = 0; i < 16; ++i)
        {
            __m128i m;
            if (i < 4)
                m = _mm_shuffle_epi8(_mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16 * i)), swap);
            else
            {
                __m128i t = _mm_sha256msg1_epu32(w[i % 4], w[(i + 1) % 4]);
                t = _mm_add_epi32(t, _mm_alignr_epi8(w[(i + 3) % 4], w[(i + 2) % 4], 4));
                m = _mm_sha256msg2_epu32(t, w[(i + 3) % 4]);
            }
            w[i % 4] = m;
            __m128i wk = _mm_add_epi32(m, _mm_load_si128(reinterpret_cast<const __m128i*>(K + 4 * i)));
            state1 = _mm_sha256rnds2_epu32(state1, state0, wk);
            wk = _mm_shuffle_epi32(wk, 0x0E);
            state0 = _mm_sha256rnds2_epu32(state0, state1, wk);
        }
        state0 = _mm_add_epi32(state0, save0);
        state1 = _mm_add_epi32(state1, save1);
    }

    tmp = _mm_shuffle_epi32(state0, 0x1B);        // FEBA
    state1 = _mm_shuffle_epi32(state1, 0xB1);     // DCHG
    state0 = _mm_blend_epi16(tmp, state1, 0xF0);  // DCBA
    state1 = _mm_alignr_epi8(state1, tmp, 8);     // HGFE
    _mm_storeu_si128(reinterpret_cast<__m128i*>(h), state0);
    _mm_storeu_si128(reinterpret_cast<__m128i*>(h + 4), state1);
}

#endif

using BlocksFn = void (*)(uint32_t*, const uint8_t*, size_t);

// Atomic: switching paths (tests do) while other threads hash is well defined. Both paths give
// the same digests, so a thread that reads either one is correct.
std::atomic<BlocksFn> g_blocks{
#if SIEVE_SHA_X86
    cpu_has_sha() ? blocks_shani : blocks_portable
#else
    blocks_portable
#endif
};

} // namespace

bool Sha256::hardware_available()
{
#if SIEVE_SHA_X86
    return cpu_has_sha();
#else
    return false;
#endif
}

bool Sha256::use_hardware(bool enable)
{
#if SIEVE_SHA_X86
    g_blocks = enable && cpu_has_sha() ? blocks_shani : blocks_portable;
#else
    (void)enable;
#endif
    return using_hardware();
}

bool Sha256::using_hardware() { return g_blocks.load(std::memory_order_relaxed) != blocks_portable; }

Sha256::Sha256()
    : h_{0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19}
{
}

void Sha256::update(std::span<const uint8_t> data)
{
    total_len_ += data.size();
    if (data.empty()) return; // (memcpy from a null pointer is undefined even for 0 bytes)
    const uint8_t* p = data.data();
    size_t n = data.size();
    if (buf_len_)
    {
        const size_t take = std::min(n, 64 - buf_len_);
        std::memcpy(buf_.data() + buf_len_, p, take);
        buf_len_ += take;
        p += take;
        n -= take;
        if (buf_len_ < 64) return;
        g_blocks.load(std::memory_order_relaxed)(h_.data(), buf_.data(), 1);
        buf_len_ = 0;
    }
    if (n >= 64)
    {
        g_blocks.load(std::memory_order_relaxed)(h_.data(), p, n / 64);
        p += n / 64 * 64;
        n %= 64;
    }
    std::memcpy(buf_.data(), p, n);
    buf_len_ = n;
}

void Sha256::update(std::string_view data)
{
    update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(data.data()), data.size()));
}

void Sha256::update_u32le(uint32_t v)
{
    const uint8_t b[4] = {uint8_t(v), uint8_t(v >> 8), uint8_t(v >> 16), uint8_t(v >> 24)};
    update(std::span<const uint8_t>(b, 4));
}

Sha256::Digest Sha256::finish()
{
    const uint64_t bits = total_len_ * 8;
    // 0x80, zeros up to 56 mod 64, then the 64-bit big-endian length: in one update.
    uint8_t pad[72] = {0x80};
    const size_t zeros = buf_len_ < 56 ? 55 - buf_len_ : 119 - buf_len_;
    for (int i = 0; i < 8; ++i) pad[1 + zeros + i] = uint8_t(bits >> (56 - 8 * i));
    update(std::span<const uint8_t>(pad, 1 + zeros + 8));
    Digest d;
    for (int i = 0; i < 8; ++i)
    {
        d[4 * i] = uint8_t(h_[i] >> 24);
        d[4 * i + 1] = uint8_t(h_[i] >> 16);
        d[4 * i + 2] = uint8_t(h_[i] >> 8);
        d[4 * i + 3] = uint8_t(h_[i]);
    }
    return d;
}

Sha256::Digest Sha256::hash(std::string_view data)
{
    Sha256 s;
    s.update(data);
    return s.finish();
}

std::string Sha256::hex(const Digest& d)
{
    static const char* digits = "0123456789abcdef";
    std::string s;
    for (uint8_t b : d) { s.push_back(digits[b >> 4]); s.push_back(digits[b & 0xF]); }
    return s;
}

} // namespace sieve
