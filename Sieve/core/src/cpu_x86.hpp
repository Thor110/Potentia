// Sieve — what an x86-64 processor says it has (CPUID) and what the system saves for it (XCR0), read
// one way for MSVC and one for GCC and Clang. Training's AVX2 and AVX-512 loops and SHA-256's SHA-NI
// path each choose themselves with these when the program starts (sha256.cpp and training.cpp share
// this one copy, so the bits each tests are all that differ between them). Include only where
// __x86_64__ or _M_X64 is defined.
#pragma once

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

namespace sieve::cpu_x86 {

// One CPUID leaf's four registers.
struct Cpuid
{
    uint32_t a = 0, b = 0, c = 0, d = 0;
};

// The highest basic leaf the processor answers (leaf 0's EAX).
inline uint32_t cpuid_max()
{
#if defined(_MSC_VER)
    int r[4];
    __cpuid(r, 0);
    return uint32_t(r[0]);
#else
    return __get_cpuid_max(0, nullptr);
#endif
}

// A leaf (and sub-leaf) at or below cpuid_max(): a leaf above it reads as zeros under GCC and Clang,
// so the caller checks cpuid_max() first.
inline Cpuid cpuid(uint32_t leaf, uint32_t sub = 0)
{
    Cpuid r;
#if defined(_MSC_VER)
    int x[4];
    __cpuidex(x, int(leaf), int(sub));
    r = {uint32_t(x[0]), uint32_t(x[1]), uint32_t(x[2]), uint32_t(x[3])};
#else
    __get_cpuid_count(leaf, sub, &r.a, &r.b, &r.c, &r.d);
#endif
    return r;
}

// XCR0: which registers the system saves. Only to be read when leaf 1 says OSXSAVE (ECX bit 27):
// without it the instruction faults.
inline uint64_t xcr0()
{
#if defined(_MSC_VER)
    return _xgetbv(0);
#else
    unsigned lo = 0, hi = 0;
    __asm__ volatile("xgetbv" : "=a"(lo), "=d"(hi) : "c"(0));
    return (uint64_t(hi) << 32) | lo;
#endif
}

} // namespace sieve::cpu_x86
