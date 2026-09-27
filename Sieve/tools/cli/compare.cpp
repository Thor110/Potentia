// The comparison (compare.hpp): the compressors called once each, at their strongest settings.

#include "cli/compare.hpp"

#include <lzma.h>
#include <zlib.h>

#include <cstdio>
#include <stdexcept>

namespace sieve::cli {

uint64_t deflate_size(std::span<const uint8_t> data)
{
    // Raw deflate (windowBits -15: no zlib header or checksum), as a zip archive holds it.
    z_stream z{};
    if (deflateInit2(&z, 9, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY) != Z_OK) throw std::runtime_error("zlib: cannot start");
    std::vector<uint8_t> out(deflateBound(&z, uLong(data.size())) + 64);
    z.next_in = const_cast<Bytef*>(data.data());
    z.avail_in = uInt(data.size());
    z.next_out = out.data();
    z.avail_out = uInt(out.size());
    const int r = deflate(&z, Z_FINISH);
    const uint64_t n = z.total_out;
    deflateEnd(&z);
    if (r != Z_STREAM_END) throw std::runtime_error("zlib: the data did not compress in one pass");
    return n;
}

uint64_t lzma2_size(std::span<const uint8_t> data)
{
    lzma_options_lzma opt;
    if (lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME)) throw std::runtime_error("liblzma: no preset 9");
    const lzma_filter filters[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    std::vector<uint8_t> out(data.size() + data.size() / 16 + 4096);
    size_t pos = 0;
    const lzma_ret r = lzma_raw_buffer_encode(filters, nullptr, data.data(), data.size(), out.data(), &pos, out.size());
    if (r != LZMA_OK) throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
    return pos;
}

std::string comparison_table(const Comparison& c)
{
    auto row = [&](const char* name, uint64_t v, const char* note) {
        char pct[32];
        if (c.original == 0) std::snprintf(pct, sizeof pct, "%10s", "-");
        else std::snprintf(pct, sizeof pct, "%9.2f%%", 100.0 * double(v) / double(c.original));
        char line[256];
        std::snprintf(line, sizeof line, "  %-22s %16llu bytes %s   %s\n", name, (unsigned long long)v, pct, note);
        return std::string(line);
    };
    std::string t = "comparison (the original is 100%; under 100% is smaller)\n";
    t += row("original", c.original, "");
    t += row("zip (deflate, level 9)", c.deflate, "each file on its own, without the zip container");
    t += row("7z (LZMA2, preset 9e)", c.lzma2, "everything as one stream, without the 7z container");
    t += row("address, as raw bytes", c.address_bytes, "its place on the binary line as a number: the file, plus 0101...01");
    t += row("address, written in hex", c.address_hex, "two characters a byte");
    if (c.manifest && !c.installer_hex)
        t += row("manifest", c.manifest, "the tree's structure, sizes and SHA-256s, which the addresses do not carry");
    if (c.installer_hex)
    {
        t += row("listing and files", c.manifest, "the folder's listing, then every file's bytes (sieve-manifest-v3)");
        t += row("  zip of it (deflate)", c.manifest_deflate, "that compressed");
        t += row("  7z of it (LZMA2)", c.manifest_lzma2, "");
        t += row("Sieve instructions", c.installer_raw, "the .sieve: all of that as one number, its address, in raw bytes");
        t += row("  the same, in hex", c.installer_hex, "that number written in hex");
    }
    return t;
}

} // namespace sieve::cli
