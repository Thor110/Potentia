// The comparison (compare.hpp): the compressors called once each, at their strongest settings.

#include "cli/compare.hpp"

#include "sieve/stop.hpp"

// bzlib.h includes <windows.h> on Windows: without these first, its min and max macros break
// std::min and std::max (as in pack.cpp).
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <bzlib.h>
#include <lzma.h>
#include <zlib.h>
#include <zstd.h>
#include <brotli/encode.h>
#include <lz4frame.h>
#include <lz4hc.h>

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

namespace {

constexpr size_t kPiece = size_t(1) << 20;

// bzip2 at block size 9 (900 KB), as bzip2 -9.
uint64_t bzip2_size(std::span<const uint8_t> data)
{
    bz_stream z{};
    if (BZ2_bzCompressInit(&z, 9, 0, 0) != BZ_OK) throw std::runtime_error("bzip2: cannot start");
    std::vector<char> out(kPiece);
    size_t at = 0;
    uint64_t total = 0;
    for (;;)
    {
        if (stop_requested())
        {
            BZ2_bzCompressEnd(&z);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = const_cast<char*>(reinterpret_cast<const char*>(data.data() + at));
        z.avail_in = unsigned(n);
        at += n;
        const int action = at == data.size() ? BZ_FINISH : BZ_RUN;
        int r = BZ_RUN_OK;
        do
        {
            z.next_out = out.data();
            z.avail_out = unsigned(out.size());
            r = BZ2_bzCompress(&z, action);
            if (r < 0)
            {
                BZ2_bzCompressEnd(&z);
                throw std::runtime_error("bzip2: could not compress");
            }
            total += out.size() - z.avail_out;
        } while (z.avail_in > 0 || (action == BZ_FINISH && r != BZ_STREAM_END));
        if (action == BZ_FINISH) break;
    }
    BZ2_bzCompressEnd(&z);
    return total;
}

// zstd at level 19 with long-distance matching over a 128 MiB window, as zstd -19 --long=27.
uint64_t zstd_size(std::span<const uint8_t> data)
{
    ZSTD_CCtx* c = ZSTD_createCCtx();
    if (!c) throw std::runtime_error("zstd: cannot start");
    ZSTD_CCtx_setParameter(c, ZSTD_c_compressionLevel, 19);
    ZSTD_CCtx_setParameter(c, ZSTD_c_enableLongDistanceMatching, 1);
    ZSTD_CCtx_setParameter(c, ZSTD_c_windowLog, 27);
    std::vector<uint8_t> out(ZSTD_CStreamOutSize());
    size_t at = 0;
    uint64_t total = 0;
    for (;;)
    {
        if (stop_requested())
        {
            ZSTD_freeCCtx(c);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        ZSTD_inBuffer in{data.data() + at, n, 0};
        at += n;
        const ZSTD_EndDirective mode = at == data.size() ? ZSTD_e_end : ZSTD_e_continue;
        size_t left = 0;
        do
        {
            ZSTD_outBuffer o{out.data(), out.size(), 0};
            left = ZSTD_compressStream2(c, &o, &in, mode);
            if (ZSTD_isError(left))
            {
                ZSTD_freeCCtx(c);
                throw std::runtime_error(std::string("zstd: ") + ZSTD_getErrorName(left));
            }
            total += o.pos;
        } while (mode == ZSTD_e_end ? left != 0 : in.pos < in.size);
        if (mode == ZSTD_e_end) break;
    }
    ZSTD_freeCCtx(c);
    return total;
}

// brotli at quality 11 with its largest standard window (16 MiB), as brotli -q 11 -w 24.
uint64_t brotli_size(std::span<const uint8_t> data)
{
    BrotliEncoderState* e = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
    if (!e) throw std::runtime_error("brotli: cannot start");
    BrotliEncoderSetParameter(e, BROTLI_PARAM_QUALITY, 11);
    BrotliEncoderSetParameter(e, BROTLI_PARAM_LGWIN, 24);
    if (data.size() <= 0x7FFFFFFF) BrotliEncoderSetParameter(e, BROTLI_PARAM_SIZE_HINT, uint32_t(data.size()));
    size_t at = 0;
    uint64_t total = 0;
    for (;;)
    {
        if (stop_requested())
        {
            BrotliEncoderDestroyInstance(e);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        size_t avail_in = n;
        const uint8_t* next_in = data.data() + at;
        at += n;
        const BrotliEncoderOperation op = at == data.size() ? BROTLI_OPERATION_FINISH : BROTLI_OPERATION_PROCESS;
        for (;;)
        {
            size_t avail_out = 0;
            if (!BrotliEncoderCompressStream(e, op, &avail_in, &next_in, &avail_out, nullptr, nullptr))
            {
                BrotliEncoderDestroyInstance(e);
                throw std::runtime_error("brotli: could not compress");
            }
            size_t got = 0;
            while (BrotliEncoderHasMoreOutput(e)) (void)BrotliEncoderTakeOutput(e, &got), total += got;
            if (op == BROTLI_OPERATION_FINISH ? bool(BrotliEncoderIsFinished(e)) : avail_in == 0) break;
        }
        if (op == BROTLI_OPERATION_FINISH) break;
    }
    BrotliEncoderDestroyInstance(e);
    return total;
}

// LZ4 at its strongest (LZ4HC level 12), blocks of 4 MB linked, as lz4 -12.
uint64_t lz4_size(std::span<const uint8_t> data)
{
    LZ4F_cctx* c = nullptr;
    if (LZ4F_isError(LZ4F_createCompressionContext(&c, LZ4F_VERSION))) throw std::runtime_error("lz4: cannot start");
    LZ4F_preferences_t p{};
    p.compressionLevel = LZ4HC_CLEVEL_MAX;
    p.frameInfo.blockSizeID = LZ4F_max4MB;
    p.frameInfo.blockMode = LZ4F_blockLinked;
    std::vector<uint8_t> out(LZ4F_compressBound(kPiece, &p) + 64);
    auto fail = [&](size_t r) {
        if (LZ4F_isError(r))
        {
            LZ4F_freeCompressionContext(c);
            throw std::runtime_error(std::string("lz4: ") + LZ4F_getErrorName(r));
        }
        return uint64_t(r);
    };
    uint64_t total = fail(LZ4F_compressBegin(c, out.data(), out.size(), &p));
    for (size_t at = 0; at < data.size();)
    {
        if (stop_requested())
        {
            LZ4F_freeCompressionContext(c);
            return kNotMeasured;
        }
        const size_t n = std::min(kPiece, data.size() - at);
        total += fail(LZ4F_compressUpdate(c, out.data(), out.size(), data.data() + at, n, nullptr));
        at += n;
    }
    total += fail(LZ4F_compressEnd(c, out.data(), out.size(), nullptr));
    LZ4F_freeCompressionContext(c);
    return total;
}

} // namespace

std::vector<FormatSize> more_format_sizes(std::span<const uint8_t> data)
{
    // (each checked for a stop before it starts, so a stop leaves the rest not measured at once)
    auto measured = [&](uint64_t (*f)(std::span<const uint8_t>)) { return stop_requested() ? kNotMeasured : f(data); };
    std::vector<FormatSize> out;
    out.push_back({"gzip -9 (as .tar.gz)", "deflate as zip's, but everything as one stream", measured(deflate_size)});
    out.push_back({"bzip2 -9 (as .tar.bz2)", "900 KB blocks", measured(bzip2_size)});
    out.push_back({"zstd -19 --long=27", "long-distance matching, a 128 MiB window", measured(zstd_size)});
    out.push_back({"brotli -q 11 -w 24", "its strongest, a 16 MiB window (slow)", measured(brotli_size)});
    out.push_back({"lz4 -12", "the fastest to unpack", measured(lz4_size)});
    return out;
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
    if (c.compared)
    {
        t += row("zip (deflate, level 9)", c.deflate, "each file on its own, without the zip container");
        t += row("7z (LZMA2, preset 9e)", c.lzma2, "everything as one stream, without the 7z container");
        for (const FormatSize& f : c.more) t += row(f.name.c_str(), f.bytes, f.note.c_str()); // (where more formats were asked for)
    }
    t += row("address, as raw bytes", c.address_bytes, "its place on the binary line as a number: the file, plus 0101...01");
    t += row("address, written in hex", c.address_hex, "two characters a byte");
    if (c.manifest && !c.installer_hex)
        t += row("manifest", c.manifest, "the tree's structure, sizes and SHA-256s, which the addresses do not carry");
    if (c.installer_hex)
    {
        t += c.packed ? row("listing and files", c.manifest, ("the folder's listing, then every file packed (" + c.version + ")").c_str())
                      : row("listing and files", c.manifest, "the folder's listing, then every file's bytes (sieve-manifest-v3)");
        if (c.compared)
        {
            t += row("  zip of it (deflate)", c.manifest_deflate, "that compressed");
            t += row("  7z of it (LZMA2)", c.manifest_lzma2, "");
        }
        t += row("Sieve instructions", c.installer_raw, "the .sieve: all of that as one number, its address, in raw bytes");
        t += row("  the same, in hex", c.installer_hex, "that number written in hex");
    }
    return t;
}

} // namespace sieve::cli
