// The comparison (compare.hpp): the compressors called once each, at their strongest settings.

#include "cli/compare.hpp"

#include "sieve/stop.hpp"

#include <lzma.h>
#include <zlib.h>

#include <algorithm>
#include <cstdio>
#include <stdexcept>

namespace sieve::cli {

uint64_t deflate_size(std::span<const uint8_t> data)
{
    // Raw deflate (windowBits -15: no zlib header or checksum), as a zip archive holds it; fed a MiB
    // at a time (the same stream as at once), the output counted and let go.
    z_stream z{};
    if (deflateInit2(&z, 9, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY) != Z_OK) throw std::runtime_error("zlib: cannot start");
    std::vector<uint8_t> out(size_t(1) << 20);
    constexpr size_t kPiece = size_t(1) << 20;
    size_t at = 0;
    int r = Z_OK;
    while (r != Z_STREAM_END)
    {
        if (stop_requested())
        {
            deflateEnd(&z);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = const_cast<Bytef*>(data.data() + at);
        z.avail_in = uInt(n);
        at += n;
        const int flush = at == data.size() ? Z_FINISH : Z_NO_FLUSH;
        do
        {
            z.next_out = out.data();
            z.avail_out = uInt(out.size());
            r = deflate(&z, flush);
            if (r == Z_STREAM_ERROR) break;
        } while (z.avail_out == 0 || (flush == Z_FINISH && r != Z_STREAM_END));
        if (r == Z_STREAM_ERROR) break;
    }
    const uint64_t total = z.total_out;
    deflateEnd(&z);
    if (r != Z_STREAM_END) throw std::runtime_error("zlib: the data did not compress");
    return total;
}

uint64_t lzma2_size(std::span<const uint8_t> data)
{
    lzma_options_lzma opt;
    if (lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME)) throw std::runtime_error("liblzma: no preset 9");
    const lzma_filter filters[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
    // The same raw LZMA2 as one call makes, fed a MiB at a time, the output counted and let go.
    lzma_stream z = LZMA_STREAM_INIT;
    if (lzma_raw_encoder(&z, filters) != LZMA_OK) throw std::runtime_error("liblzma: cannot start");
    std::vector<uint8_t> out(size_t(1) << 20);
    constexpr size_t kPiece = size_t(1) << 20;
    size_t at = 0;
    lzma_ret r = LZMA_OK;
    while (r != LZMA_STREAM_END)
    {
        if (stop_requested())
        {
            lzma_end(&z);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = data.data() + at;
        z.avail_in = n;
        at += n;
        const lzma_action action = at == data.size() ? LZMA_FINISH : LZMA_RUN;
        do
        {
            z.next_out = out.data();
            z.avail_out = out.size();
            r = lzma_code(&z, action);
            if (r != LZMA_OK && r != LZMA_STREAM_END)
            {
                lzma_end(&z);
                throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
            }
        } while (z.avail_in > 0 || (action == LZMA_FINISH && r != LZMA_STREAM_END));
    }
    const uint64_t total = z.total_out;
    lzma_end(&z);
    return total;
}

std::string comparison_table(const Comparison& c)
{
    auto row = [&](const char* name, uint64_t v, const char* note) {
        if (v == kNotMeasured)
        {
            char line[256];
            std::snprintf(line, sizeof line, "  %-24s %22s %10s   %s\n", name, "not measured", "", "stopped before it was");
            return std::string(line);
        }
        char pct[32];
        if (c.original == 0) std::snprintf(pct, sizeof pct, "%10s", "-");
        else std::snprintf(pct, sizeof pct, "%9.2f%%", 100.0 * double(v) / double(c.original));
        char line[256];
        std::snprintf(line, sizeof line, "  %-24s %16llu bytes %s   %s\n", name, (unsigned long long)v, pct, note);
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
        t += c.packed ? row("listing and files", c.manifest, ("the folder's listing, then every file packed (" + c.version + ")").c_str())
                      : row("listing and files", c.manifest, "the folder's listing, then every file's bytes (sieve-manifest-v3)");
        t += row("  zip of it (deflate)", c.manifest_deflate, "that compressed");
        t += row("  7z of it (LZMA2)", c.manifest_lzma2, "");
        t += row("Sieve instructions", c.installer_raw, "the .sieve: all of that as one number, its address, in raw bytes");
        t += row("  the same, in hex", c.installer_hex, "that number written in hex");
    }
    return t;
}

} // namespace sieve::cli
