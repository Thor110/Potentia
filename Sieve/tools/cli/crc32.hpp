// Sieve — the CRC-32 of any length of bytes (zlib's, the one of zip, gzip, PNG and 7z): the one copy
// the packer's ways share.
#pragma once

#include <zlib.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

namespace sieve::cli {

// zlib takes a length of 32 bits (uInt) at a time, so the bytes go to it a gibibyte at a time: the
// same CRC as one call, for any length.
inline uint32_t crc32_of(const uint8_t* p, size_t n, uint32_t crc = 0)
{
    uLong c = crc;
    for (size_t at = 0; at < n;)
    {
        const size_t k = std::min<size_t>(n - at, size_t(1) << 30);
        c = ::crc32(c, p + at, uInt(k));
        at += k;
    }
    return uint32_t(c);
}

inline uint32_t crc32_of(std::span<const uint8_t> d) { return crc32_of(d.data(), d.size()); }

} // namespace sieve::cli
