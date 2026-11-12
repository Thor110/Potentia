// sieve compress (compress.hpp): the formats people already use, made at their strongest usual
// settings and read back before they are kept.

#include "cli/compress.hpp"

#include "cli/gather.hpp"
#include "unpack_7z.hpp"

// bzlib.h includes <windows.h> on Windows: without these first, its min and max macros break
// std::min and std::max (as in compare.cpp).
#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <archive.h>
#include <archive_entry.h>
#include <brotli/decode.h>
#include <brotli/encode.h>
#include <bzlib.h>
#include <lz4frame.h>
#include <lz4hc.h>
#include <lzma.h>
#include <zlib.h>
#include <zstd.h>

#include <algorithm>
#include <chrono>
#include <clocale>
#include <ctime>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

namespace sieve::cli {

namespace {

namespace fs = std::filesystem;
using Bytes = std::vector<uint8_t>;
constexpr size_t kPiece = size_t(1) << 20;

// gzip at level 9, as gzip -9 (no name or time in its header, so the same input makes the same file).
Bytes gzip_of(std::span<const uint8_t> data)
{
    z_stream z{};
    if (deflateInit2(&z, 9, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY) != Z_OK) throw std::runtime_error("zlib: cannot start");
    Bytes out;
    Bytes piece(kPiece);
    size_t at = 0;
    int r = Z_OK;
    while (r != Z_STREAM_END)
    {
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = const_cast<Bytef*>(data.data() + at);
        z.avail_in = uInt(n);
        at += n;
        const int flush = at == data.size() ? Z_FINISH : Z_NO_FLUSH;
        do
        {
            z.next_out = piece.data();
            z.avail_out = uInt(piece.size());
            r = deflate(&z, flush);
            if (r == Z_STREAM_ERROR) break;
            out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
        } while (z.avail_out == 0 || (flush == Z_FINISH && r != Z_STREAM_END));
        if (r == Z_STREAM_ERROR) break;
    }
    deflateEnd(&z);
    if (r != Z_STREAM_END) throw std::runtime_error("zlib: the data did not compress");
    return out;
}

Bytes gunzip(std::span<const uint8_t> data)
{
    z_stream z{};
    if (inflateInit2(&z, 15 + 16) != Z_OK) throw std::runtime_error("zlib: cannot start");
    Bytes out;
    Bytes piece(kPiece);
    size_t fed = 0; // (fed a GiB at a time: zlib counts in 32 bits)
    int r = Z_OK;
    while (r != Z_STREAM_END)
    {
        if (z.avail_in == 0 && fed < data.size())
        {
            const size_t n = std::min(data.size() - fed, size_t(1) << 30);
            z.next_in = const_cast<Bytef*>(data.data() + fed);
            z.avail_in = uInt(n);
            fed += n;
        }
        z.next_out = piece.data();
        z.avail_out = uInt(piece.size());
        r = inflate(&z, Z_NO_FLUSH);
        if (r != Z_OK && r != Z_STREAM_END) break;
        out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
    }
    inflateEnd(&z);
    if (r != Z_STREAM_END) throw std::runtime_error("gzip: does not read back");
    return out;
}

// xz (.xz, preset 9e, CRC-64, as xz -9e) or lzma (.lzma, the older "alone" format, as xz
// --format=lzma -9e), both through liblzma. The dictionary (64 MiB at 9e) is cut to the input's size
// where that is smaller, as xz itself does not but 7-Zip does: the same bytes made, less memory to
// undo them.
Bytes xz_of(std::span<const uint8_t> data, bool alone)
{
    lzma_options_lzma opt;
    if (lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME)) throw std::runtime_error("liblzma: no preset 9");
    opt.dict_size = uint32_t(std::clamp<uint64_t>(data.size(), LZMA_DICT_SIZE_MIN, opt.dict_size));
    lzma_stream z = LZMA_STREAM_INIT;
    lzma_ret r;
    if (alone)
        r = lzma_alone_encoder(&z, &opt);
    else
    {
        const lzma_filter filters[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
        r = lzma_stream_encoder(&z, filters, LZMA_CHECK_CRC64);
    }
    if (r != LZMA_OK) throw std::runtime_error("liblzma: cannot start");
    Bytes out;
    Bytes piece(kPiece);
    z.next_in = data.data();
    z.avail_in = data.size();
    do
    {
        z.next_out = piece.data();
        z.avail_out = piece.size();
        r = lzma_code(&z, LZMA_FINISH);
        out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
    } while (r == LZMA_OK);
    lzma_end(&z);
    if (r != LZMA_STREAM_END) throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
    return out;
}

Bytes unxz(std::span<const uint8_t> data, bool alone)
{
    lzma_stream z = LZMA_STREAM_INIT;
    lzma_ret r = alone ? lzma_alone_decoder(&z, UINT64_MAX) : lzma_stream_decoder(&z, UINT64_MAX, 0);
    if (r != LZMA_OK) throw std::runtime_error("liblzma: cannot start");
    Bytes out;
    Bytes piece(kPiece);
    z.next_in = data.data();
    z.avail_in = data.size();
    do
    {
        z.next_out = piece.data();
        z.avail_out = piece.size();
        r = lzma_code(&z, LZMA_FINISH);
        out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
    } while (r == LZMA_OK);
    lzma_end(&z);
    if (r != LZMA_STREAM_END) throw std::runtime_error(std::string(alone ? "lzma" : "xz") + ": does not read back");
    return out;
}

// bzip2 at block size 9 (900 KB), as bzip2 -9.
Bytes bzip2_of(std::span<const uint8_t> data)
{
    bz_stream z{};
    if (BZ2_bzCompressInit(&z, 9, 0, 0) != BZ_OK) throw std::runtime_error("bzip2: cannot start");
    Bytes out;
    std::vector<char> piece(kPiece);
    size_t at = 0;
    for (;;)
    {
        const size_t n = std::min(kPiece, data.size() - at);
        z.next_in = const_cast<char*>(reinterpret_cast<const char*>(data.data() + at));
        z.avail_in = unsigned(n);
        at += n;
        const int action = at == data.size() ? BZ_FINISH : BZ_RUN;
        int r = BZ_RUN_OK;
        do
        {
            z.next_out = piece.data();
            z.avail_out = unsigned(piece.size());
            r = BZ2_bzCompress(&z, action);
            if (r < 0)
            {
                BZ2_bzCompressEnd(&z);
                throw std::runtime_error("bzip2: could not compress");
            }
            out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
        } while (z.avail_in > 0 || (action == BZ_FINISH && r != BZ_STREAM_END));
        if (action == BZ_FINISH) break;
    }
    BZ2_bzCompressEnd(&z);
    return out;
}

Bytes bunzip2(std::span<const uint8_t> data)
{
    bz_stream z{};
    if (BZ2_bzDecompressInit(&z, 0, 0) != BZ_OK) throw std::runtime_error("bzip2: cannot start");
    Bytes out;
    std::vector<char> piece(kPiece);
    size_t fed = 0; // (fed a GiB at a time: bzip2 counts in 32 bits)
    int r = BZ_OK;
    while (r == BZ_OK)
    {
        if (z.avail_in == 0 && fed < data.size())
        {
            const size_t n = std::min(data.size() - fed, size_t(1) << 30);
            z.next_in = const_cast<char*>(reinterpret_cast<const char*>(data.data() + fed));
            z.avail_in = unsigned(n);
            fed += n;
        }
        z.next_out = piece.data();
        z.avail_out = unsigned(piece.size());
        r = BZ2_bzDecompress(&z);
        out.insert(out.end(), piece.begin(), piece.end() - z.avail_out);
        if (r == BZ_OK && z.avail_in == 0 && fed == data.size() && z.avail_out != 0) break; // (ended early)
    }
    BZ2_bzDecompressEnd(&z);
    if (r != BZ_STREAM_END) throw std::runtime_error("bzip2: does not read back");
    return out;
}

// zstd at level 19 with long-distance matching over a 128 MiB window, as zstd -19 --long=27 (a
// window every zstd reads without being asked for more).
Bytes zstd_of(std::span<const uint8_t> data)
{
    ZSTD_CCtx* c = ZSTD_createCCtx();
    if (!c) throw std::runtime_error("zstd: cannot start");
    ZSTD_CCtx_setParameter(c, ZSTD_c_compressionLevel, 19);
    ZSTD_CCtx_setParameter(c, ZSTD_c_enableLongDistanceMatching, 1);
    ZSTD_CCtx_setParameter(c, ZSTD_c_windowLog, 27);
    ZSTD_CCtx_setParameter(c, ZSTD_c_checksumFlag, 1);
    ZSTD_CCtx_setPledgedSrcSize(c, data.size());
    Bytes out;
    Bytes piece(ZSTD_CStreamOutSize());
    ZSTD_inBuffer in{data.data(), data.size(), 0};
    size_t left = 0;
    do
    {
        ZSTD_outBuffer o{piece.data(), piece.size(), 0};
        left = ZSTD_compressStream2(c, &o, &in, ZSTD_e_end);
        if (ZSTD_isError(left))
        {
            ZSTD_freeCCtx(c);
            throw std::runtime_error(std::string("zstd: ") + ZSTD_getErrorName(left));
        }
        out.insert(out.end(), piece.begin(), piece.begin() + o.pos);
    } while (left != 0);
    ZSTD_freeCCtx(c);
    return out;
}

Bytes unzstd(std::span<const uint8_t> data)
{
    ZSTD_DCtx* d = ZSTD_createDCtx();
    if (!d) throw std::runtime_error("zstd: cannot start");
    ZSTD_DCtx_setParameter(d, ZSTD_d_windowLogMax, 27);
    Bytes out;
    Bytes piece(ZSTD_DStreamOutSize());
    ZSTD_inBuffer in{data.data(), data.size(), 0};
    size_t r = 1;
    for (;;)
    {
        ZSTD_outBuffer o{piece.data(), piece.size(), 0};
        r = ZSTD_decompressStream(d, &o, &in);
        if (ZSTD_isError(r))
        {
            ZSTD_freeDCtx(d);
            throw std::runtime_error(std::string("zstd: does not read back: ") + ZSTD_getErrorName(r));
        }
        out.insert(out.end(), piece.begin(), piece.begin() + o.pos);
        if (in.pos == in.size && o.pos < o.size) break; // (the input used up, and nothing more held)
    }
    ZSTD_freeDCtx(d);
    if (r != 0) throw std::runtime_error("zstd: does not read back (ended early)");
    return out;
}

// brotli at quality 11 with its largest standard window (16 MiB), as brotli -q 11 -w 24.
Bytes brotli_of(std::span<const uint8_t> data)
{
    BrotliEncoderState* e = BrotliEncoderCreateInstance(nullptr, nullptr, nullptr);
    if (!e) throw std::runtime_error("brotli: cannot start");
    BrotliEncoderSetParameter(e, BROTLI_PARAM_QUALITY, 11);
    BrotliEncoderSetParameter(e, BROTLI_PARAM_LGWIN, 24);
    if (data.size() <= 0x7FFFFFFF) BrotliEncoderSetParameter(e, BROTLI_PARAM_SIZE_HINT, uint32_t(data.size()));
    Bytes out;
    size_t avail_in = data.size();
    const uint8_t* next_in = data.data();
    for (;;)
    {
        size_t avail_out = 0;
        if (!BrotliEncoderCompressStream(e, BROTLI_OPERATION_FINISH, &avail_in, &next_in, &avail_out, nullptr, nullptr))
        {
            BrotliEncoderDestroyInstance(e);
            throw std::runtime_error("brotli: could not compress");
        }
        size_t got = 0;
        while (BrotliEncoderHasMoreOutput(e))
        {
            const uint8_t* p = BrotliEncoderTakeOutput(e, &got);
            out.insert(out.end(), p, p + got);
        }
        if (BrotliEncoderIsFinished(e)) break;
    }
    BrotliEncoderDestroyInstance(e);
    return out;
}

Bytes unbrotli(std::span<const uint8_t> data)
{
    BrotliDecoderState* d = BrotliDecoderCreateInstance(nullptr, nullptr, nullptr);
    if (!d) throw std::runtime_error("brotli: cannot start");
    Bytes out;
    size_t avail_in = data.size();
    const uint8_t* next_in = data.data();
    BrotliDecoderResult r = BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT;
    while (r == BROTLI_DECODER_RESULT_NEEDS_MORE_OUTPUT)
    {
        size_t avail_out = 0;
        r = BrotliDecoderDecompressStream(d, &avail_in, &next_in, &avail_out, nullptr, nullptr);
        size_t got = 0;
        while (BrotliDecoderHasMoreOutput(d))
        {
            const uint8_t* p = BrotliDecoderTakeOutput(d, &got);
            out.insert(out.end(), p, p + got);
        }
    }
    BrotliDecoderDestroyInstance(d);
    if (r != BROTLI_DECODER_RESULT_SUCCESS || avail_in != 0) throw std::runtime_error("brotli: does not read back");
    return out;
}

// LZ4 at its strongest (LZ4HC level 12), blocks of 4 MB linked, with the frame's checksum, as lz4 -12.
Bytes lz4_of(std::span<const uint8_t> data)
{
    LZ4F_preferences_t p{};
    p.compressionLevel = LZ4HC_CLEVEL_MAX;
    p.frameInfo.blockSizeID = LZ4F_max4MB;
    p.frameInfo.blockMode = LZ4F_blockLinked;
    p.frameInfo.contentChecksumFlag = LZ4F_contentChecksumEnabled;
    p.frameInfo.contentSize = data.size();
    Bytes out(LZ4F_compressFrameBound(data.size(), &p));
    const size_t n = LZ4F_compressFrame(out.data(), out.size(), data.data(), data.size(), &p);
    if (LZ4F_isError(n)) throw std::runtime_error(std::string("lz4: ") + LZ4F_getErrorName(n));
    out.resize(n);
    return out;
}

Bytes unlz4(std::span<const uint8_t> data)
{
    LZ4F_dctx* d = nullptr;
    if (LZ4F_isError(LZ4F_createDecompressionContext(&d, LZ4F_VERSION))) throw std::runtime_error("lz4: cannot start");
    Bytes out;
    Bytes piece(kPiece);
    size_t at = 0, r = 1;
    while (at < data.size() && r != 0)
    {
        size_t in_n = data.size() - at, out_n = piece.size();
        r = LZ4F_decompress(d, piece.data(), &out_n, data.data() + at, &in_n, nullptr);
        if (LZ4F_isError(r))
        {
            LZ4F_freeDecompressionContext(d);
            throw std::runtime_error(std::string("lz4: does not read back: ") + LZ4F_getErrorName(r));
        }
        at += in_n;
        out.insert(out.end(), piece.begin(), piece.begin() + out_n);
        if (in_n == 0 && out_n == 0) break;
    }
    LZ4F_freeDecompressionContext(d);
    if (r != 0 || at != data.size()) throw std::runtime_error("lz4: does not read back (ended early)");
    return out;
}

// --- tar and zip, through libarchive's writers (built in; neither needs a library Sieve lacks) ---

la_ssize_t to_bytes(struct archive*, void* out, const void* buf, size_t n)
{
    auto* o = static_cast<Bytes*>(out);
    o->insert(o->end(), static_cast<const uint8_t*>(buf), static_cast<const uint8_t*>(buf) + n);
    return la_ssize_t(n);
}

Bytes libarchive_of(const std::vector<CompressEntry>& entries, bool zip)
{
    struct archive* a = archive_write_new();
    if (!a) throw std::runtime_error("libarchive: cannot start");
    Bytes out;
    auto fail = [&](const char* what) {
        const std::string why = archive_error_string(a) ? archive_error_string(a) : "";
        archive_write_free(a);
        throw std::runtime_error(std::string(zip ? "zip" : "tar") + ": " + what + (why.empty() ? "" : " (" + why + ")"));
    };
    // tar as POSIX.1-2001 (pax), its extended headers only where a name or size needs them; zip
    // with deflate at 9 and UTF-8 names.
    if ((zip ? archive_write_set_format_zip(a) : archive_write_set_format_pax_restricted(a)) != ARCHIVE_OK) fail("no such format");
    if (zip && archive_write_set_options(a, "zip:compression=deflate,zip:compression-level=9") != ARCHIVE_OK) fail("options");
    // (Names as UTF-8, marked so, where libarchive can say so: on Windows. Elsewhere it writes them in
    // the locale's character set, which cmd_compress makes UTF-8; read back below either way.)
    if (zip) (void)archive_write_set_options(a, "zip:hdrcharset=UTF-8");
    archive_write_set_bytes_in_last_block(a, 1); // (no padding past the end: none is needed)
    if (archive_write_open(a, &out, nullptr, to_bytes, nullptr) != ARCHIVE_OK) fail("cannot open");
    for (const CompressEntry& e : entries)
    {
        struct archive_entry* en = archive_entry_new();
        archive_entry_set_pathname_utf8(en, (e.dir ? e.name + "/" : e.name).c_str());
        archive_entry_set_filetype(en, e.dir ? AE_IFDIR : AE_IFREG);
        archive_entry_set_perm(en, e.dir ? 0755 : 0644);
        archive_entry_set_size(en, e.dir ? 0 : la_int64_t(e.bytes.size()));
        archive_entry_set_mtime(en, time_t(e.mtime), 0);
        if (archive_write_header(a, en) != ARCHIVE_OK)
        {
            archive_entry_free(en);
            fail(("cannot write " + e.name).c_str());
        }
        archive_entry_free(en);
        for (size_t at = 0; !e.dir && at < e.bytes.size();)
        {
            const size_t n = std::min(kPiece, e.bytes.size() - at);
            const la_ssize_t w = archive_write_data(a, e.bytes.data() + at, n);
            if (w <= 0) fail(("cannot write " + e.name).c_str());
            at += size_t(w);
        }
    }
    if (archive_write_close(a) != ARCHIVE_OK) fail("cannot finish");
    archive_write_free(a);
    return out;
}

} // namespace

// What a tar or zip holds, read back by libarchive: name (folders ending '/') to bytes.
std::map<std::string, std::vector<uint8_t>> libarchive_read(std::span<const uint8_t> data, bool zip)
{
    struct archive* a = archive_read_new();
    if (!a) throw std::runtime_error("libarchive: cannot start");
    zip ? archive_read_support_format_zip(a) : archive_read_support_format_tar(a);
    std::map<std::string, Bytes> got;
    if (archive_read_open_memory(a, data.data(), data.size()) != ARCHIVE_OK)
    {
        archive_read_free(a);
        throw std::runtime_error(std::string(zip ? "zip" : "tar") + ": does not read back");
    }
    struct archive_entry* en = nullptr;
    int r;
    while ((r = archive_read_next_header(a, &en)) == ARCHIVE_OK)
    {
        const char* u = archive_entry_pathname_utf8(en);
        std::string name = u ? u : "";
        Bytes& b = got[name];
        Bytes piece(kPiece);
        la_ssize_t n;
        while ((n = archive_read_data(a, piece.data(), piece.size())) > 0) b.insert(b.end(), piece.begin(), piece.begin() + n);
        if (n < 0) break;
    }
    const bool ok = r == ARCHIVE_EOF;
    archive_read_free(a);
    if (!ok) throw std::runtime_error(std::string(zip ? "zip" : "tar") + ": does not read back");
    return got;
}

namespace {

// --- 7z: written here (libarchive's 7z writer has no LZMA2 without liblzma inside it) ---
//
// One solid folder: every file's bytes one after another, as one raw LZMA2 stream (preset 9e,
// the dictionary cut to the input's size, as 7-Zip cuts it), then the header, not compressed, as
// 7z's format describes it (7-Zip's DOC/7zFormat.txt): the streams' sizes and CRCs, then the names
// (UTF-16), which are empty (folders and empty files), the times and the attributes.

// CRC-32 of any length (zlib's takes 32 bits of length at a time).
uint32_t crc_of(std::span<const uint8_t> d)
{
    uLong c = crc32(0, nullptr, 0);
    for (size_t at = 0; at < d.size();)
    {
        const size_t n = std::min(d.size() - at, size_t(1) << 30);
        c = crc32(c, d.data() + at, uInt(n));
        at += n;
    }
    return uint32_t(c);
}

void put_number(Bytes& b, uint64_t v)
{
    uint8_t first = 0, mask = 0x80;
    int i = 0;
    for (; i < 8; ++i)
    {
        if (v < (uint64_t(1) << (7 * (i + 1))))
        {
            first |= uint8_t(v >> (8 * i));
            break;
        }
        first |= mask;
        mask >>= 1;
    }
    b.push_back(first);
    for (; i > 0; --i, v >>= 8) b.push_back(uint8_t(v));
}

void put_le(Bytes& b, uint64_t v, int n)
{
    for (int i = 0; i < n; ++i) b.push_back(uint8_t(v >> (8 * i)));
}

void put_bits(Bytes& b, const std::vector<bool>& bits)
{
    uint8_t c = 0;
    for (size_t i = 0; i < bits.size(); ++i)
    {
        if (bits[i]) c |= uint8_t(0x80 >> (i % 8));
        if (i % 8 == 7) b.push_back(c), c = 0;
    }
    if (bits.size() % 8) b.push_back(c);
}

// UTF-8 to UTF-16LE, ending with a zero.
void put_utf16(Bytes& b, const std::string& s)
{
    for (size_t i = 0; i < s.size();)
    {
        const uint8_t c = uint8_t(s[i]);
        uint32_t cp = c;
        int extra = c < 0x80 ? 0 : c < 0xE0 ? 1 : c < 0xF0 ? 2 : 3;
        if (extra) cp = c & (0x3F >> extra);
        for (int k = 1; k <= extra && i + k < s.size(); ++k) cp = (cp << 6) | (uint8_t(s[i + k]) & 0x3F);
        i += size_t(extra) + 1;
        if (cp >= 0x10000)
        {
            cp -= 0x10000;
            put_le(b, 0xD800 + (cp >> 10), 2);
            put_le(b, 0xDC00 + (cp & 0x3FF), 2);
        }
        else
            put_le(b, cp, 2);
    }
    put_le(b, 0, 2);
}

Bytes seven_zip_of(const std::vector<CompressEntry>& entries)
{
    // The files with bytes, one stream.
    Bytes all;
    std::vector<uint64_t> sizes;
    std::vector<uint32_t> crcs;
    for (const CompressEntry& e : entries)
        if (!e.dir && !e.bytes.empty())
        {
            all.insert(all.end(), e.bytes.begin(), e.bytes.end());
            sizes.push_back(e.bytes.size());
            crcs.push_back(crc_of(e.bytes));
        }
    Bytes packed;
    uint8_t dict_prop = 0;
    if (!all.empty())
    {
        lzma_options_lzma opt;
        if (lzma_lzma_preset(&opt, 9 | LZMA_PRESET_EXTREME)) throw std::runtime_error("liblzma: no preset 9");
        // The dictionary: the smallest size LZMA2's property byte can name that holds the input
        // (2 or 3 times a power of two), no larger than the preset's.
        for (dict_prop = 0; dict_prop < 40; ++dict_prop)
        {
            const uint64_t d = uint64_t(2 | (dict_prop & 1)) << (dict_prop / 2 + 11);
            if (d >= all.size() || d >= opt.dict_size) break;
        }
        opt.dict_size = uint32_t(uint64_t(2 | (dict_prop & 1)) << (dict_prop / 2 + 11));
        const lzma_filter filters[] = {{LZMA_FILTER_LZMA2, &opt}, {LZMA_VLI_UNKNOWN, nullptr}};
        lzma_stream z = LZMA_STREAM_INIT;
        if (lzma_raw_encoder(&z, filters) != LZMA_OK) throw std::runtime_error("liblzma: cannot start");
        Bytes piece(kPiece);
        z.next_in = all.data();
        z.avail_in = all.size();
        lzma_ret r;
        do
        {
            z.next_out = piece.data();
            z.avail_out = piece.size();
            r = lzma_code(&z, LZMA_FINISH);
            packed.insert(packed.end(), piece.begin(), piece.end() - z.avail_out);
        } while (r == LZMA_OK);
        lzma_end(&z);
        if (r != LZMA_STREAM_END) throw std::runtime_error("liblzma: could not compress (" + std::to_string(int(r)) + ")");
    }

    Bytes h;
    h.push_back(0x01); // Header
    if (!all.empty())
    {
        h.push_back(0x04); // MainStreamsInfo
        h.push_back(0x06); // PackInfo
        put_number(h, 0);  // (the packed stream starts straight after the signature header)
        put_number(h, 1);
        h.push_back(0x09);
        put_number(h, packed.size());
        h.push_back(0x00);
        h.push_back(0x07); // UnPackInfo
        h.push_back(0x0B); // Folder
        put_number(h, 1);
        h.push_back(0x00);            // (not external)
        put_number(h, 1);             // one coder
        h.push_back(0x21);            // its id one byte long, with properties
        h.push_back(0x21);            // LZMA2
        put_number(h, 1);
        h.push_back(dict_prop);
        h.push_back(0x0C); // CodersUnPackSize
        put_number(h, all.size());
        h.push_back(0x00);
        h.push_back(0x08); // SubStreamsInfo
        if (sizes.size() != 1)
        {
            h.push_back(0x0D);
            put_number(h, sizes.size());
            h.push_back(0x09); // every size but the last (which is what is left)
            for (size_t i = 0; i + 1 < sizes.size(); ++i) put_number(h, sizes[i]);
        }
        h.push_back(0x0A); // CRCs, all given
        h.push_back(0x01);
        for (const uint32_t c : crcs) put_le(h, c, 4);
        h.push_back(0x00);
        h.push_back(0x00);
    }
    h.push_back(0x05); // FilesInfo
    put_number(h, entries.size());
    std::vector<bool> empty_stream, empty_file;
    for (const CompressEntry& e : entries)
    {
        const bool none = e.dir || e.bytes.empty();
        empty_stream.push_back(none);
        if (none) empty_file.push_back(!e.dir);
    }
    auto property = [&](uint8_t id, const Bytes& body) {
        h.push_back(id);
        put_number(h, body.size());
        h.insert(h.end(), body.begin(), body.end());
    };
    if (std::find(empty_stream.begin(), empty_stream.end(), true) != empty_stream.end())
    {
        Bytes b;
        put_bits(b, empty_stream);
        property(0x0E, b);
        if (std::find(empty_file.begin(), empty_file.end(), true) != empty_file.end())
        {
            Bytes f;
            put_bits(f, empty_file);
            property(0x0F, f);
        }
    }
    {
        Bytes b{0x00}; // (not external)
        for (const CompressEntry& e : entries) put_utf16(b, e.name);
        property(0x11, b);
    }
    {
        Bytes b{0x01, 0x00}; // all given, not external; FILETIME: 100 ns steps since 1601
        for (const CompressEntry& e : entries) put_le(b, uint64_t(e.mtime + 11644473600LL) * 10000000ULL, 8);
        property(0x14, b);
    }
    {
        Bytes b{0x01, 0x00}; // all given, not external: a folder's attribute, or an archived file's
        for (const CompressEntry& e : entries) put_le(b, e.dir ? 0x10 : 0x20, 4);
        property(0x15, b);
    }
    h.push_back(0x00);
    h.push_back(0x00);

    Bytes out{'7', 'z', 0xBC, 0xAF, 0x27, 0x1C, 0x00, 0x04};
    Bytes start;
    put_le(start, packed.size(), 8); // where the header is, after the packed stream
    put_le(start, h.size(), 8);
    put_le(start, uint32_t(crc32(0, h.data(), uInt(h.size()))), 4);
    put_le(out, uint32_t(crc32(0, start.data(), uInt(start.size()))), 4);
    out.insert(out.end(), start.begin(), start.end());
    out.insert(out.end(), packed.begin(), packed.end());
    out.insert(out.end(), h.begin(), h.end());
    return out;
}

// What went in, by name (folders ending '/'), to compare with what reads back.
std::map<std::string, Bytes> expected_of(const std::vector<CompressEntry>& entries)
{
    std::map<std::string, Bytes> m;
    for (const CompressEntry& e : entries) m[e.dir ? e.name + "/" : e.name] = e.dir ? Bytes{} : e.bytes;
    return m;
}

void check_same(const std::map<std::string, Bytes>& want, const std::map<std::string, Bytes>& got, const std::string& format)
{
    if (want == got) return;
    for (const auto& [name, bytes] : want)
    {
        auto it = got.find(name);
        if (it == got.end()) throw std::runtime_error(format + ": made, but " + name + " is not in it when read back");
        if (it->second != bytes) throw std::runtime_error(format + ": made, but " + name + " reads back differently");
    }
    throw std::runtime_error(format + ": made, but it holds more than went in when read back");
}

int64_t mtime_of(const fs::path& p)
{
    std::error_code ec;
    const auto t = fs::last_write_time(p, ec);
    if (ec) return 0;
    // (Through now on both clocks: clock_cast is missing from some standard libraries, Apple's among them.)
    const auto sys = std::chrono::system_clock::now() + std::chrono::duration_cast<std::chrono::system_clock::duration>(t - fs::file_time_type::clock::now());
    return std::chrono::duration_cast<std::chrono::seconds>(sys.time_since_epoch()).count();
}

std::string utf8_of(const fs::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

fs::path path_of_utf8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

Bytes read_file(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + utf8_of(p));
    return Bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

// A file, or a folder and everything under it, as entries named from `top` (its own name).
void gather_entries(const fs::path& p, const std::string& top, std::vector<CompressEntry>& out, std::vector<std::string>& skipped)
{
    std::error_code ec;
    const fs::file_status st = fs::symlink_status(p, ec);
    if (fs::is_regular_file(st))
    {
        out.push_back({top, false, read_file(p), mtime_of(p)});
        return;
    }
    if (!fs::is_directory(st))
    {
        skipped.push_back(top);
        return;
    }
    out.push_back({top, true, {}, mtime_of(p)});
    std::vector<fs::path> under;
    // (A folder that cannot be read is refused, not carried empty: the review of 6 November 2026.)
    fs::directory_iterator it(p, ec);
    if (ec) throw std::runtime_error("cannot read the folder " + utf8_of(p) + ": " + ec.message());
    for (; it != fs::directory_iterator(); it.increment(ec))
    {
        if (ec) throw std::runtime_error("cannot read the folder " + utf8_of(p) + ": " + ec.message());
        under.push_back(it->path());
    }
    std::sort(under.begin(), under.end()); // (the same order on every machine)
    for (const fs::path& u : under) gather_entries(u, top + "/" + utf8_of(u.filename()), out, skipped);
}

} // namespace

const std::vector<std::string>& compress_formats()
{
    static const std::vector<std::string> f{"gz", "xz", "lzma", "bz2", "zst", "lz4", "br", "zip", "7z", "tar"};
    return f;
}

bool is_stream_format(const std::string& format) { return format != "zip" && format != "7z" && format != "tar"; }

std::vector<uint8_t> compress_stream(std::span<const uint8_t> data, const std::string& format)
{
    if (format == "gz") return gzip_of(data);
    if (format == "xz") return xz_of(data, false);
    if (format == "lzma") return xz_of(data, true);
    if (format == "bz2") return bzip2_of(data);
    if (format == "zst") return zstd_of(data);
    if (format == "lz4") return lz4_of(data);
    if (format == "br") return brotli_of(data);
    throw std::invalid_argument("not a stream format: " + format);
}

std::vector<uint8_t> decompress_stream(std::span<const uint8_t> data, const std::string& format)
{
    if (format == "gz") return gunzip(data);
    if (format == "xz") return unxz(data, false);
    if (format == "lzma") return unxz(data, true);
    if (format == "bz2") return bunzip2(data);
    if (format == "zst") return unzstd(data);
    if (format == "lz4") return unlz4(data);
    if (format == "br") return unbrotli(data);
    throw std::invalid_argument("not a stream format: " + format);
}

std::vector<uint8_t> compress_entries(const std::vector<CompressEntry>& entries, const std::string& format)
{
    if (format == "zip" || format == "tar")
    {
        Bytes out = libarchive_of(entries, format == "zip");
        check_same(expected_of(entries), libarchive_read(out, format == "zip"), format);
        return out;
    }
    if (format == "7z")
    {
        Bytes out = seven_zip_of(entries);
        // Read back by the installer's own 7z reader.
        std::map<std::string, Bytes> got;
        for (auto& m : install::read_7z(out)) got[m.entry.dir ? m.entry.path + "/" : m.entry.path] = std::move(m.bytes);
        // (read_7z takes away a top folder every entry is in, as the installer unpacking it does:
        // so is it taken from what went in)
        std::map<std::string, Bytes> want = expected_of(entries);
        const std::string top = entries.empty() ? "" : entries[0].name.substr(0, entries[0].name.find('/'));
        bool shared = !top.empty(), inside = false;
        for (const CompressEntry& e : entries)
        {
            if (e.name == top) shared = shared && e.dir;
            else if (e.name.rfind(top + "/", 0) == 0) inside = true;
            else shared = false;
        }
        if (shared && inside)
        {
            std::map<std::string, Bytes> cut;
            for (auto& [name, bytes] : want)
                if (name.size() > top.size() + 1) cut[name.substr(top.size() + 1)] = std::move(bytes);
            want = std::move(cut);
        }
        check_same(want, got, format);
        return out;
    }
    if (!is_stream_format(format)) throw std::invalid_argument("no such format: " + format);
    const bool one = entries.size() == 1 && !entries[0].dir;
    Bytes tar;
    if (!one) tar = libarchive_of(entries, false);
    const std::span<const uint8_t> inner = one ? std::span<const uint8_t>(entries[0].bytes) : std::span<const uint8_t>(tar);
    Bytes out = compress_stream(inner, format);
    const Bytes back = decompress_stream(out, format);
    if (!std::equal(back.begin(), back.end(), inner.begin(), inner.end())) throw std::runtime_error(format + ": made, but it reads back differently");
    return out;
}

namespace {

// The name a result is given beside what went in, when --out is not: one file or folder by its own
// name, several by the folder they are in. NAME.EXT, then NAME (2).EXT ... where that is taken.
fs::path free_name(const fs::path& folder, const std::string& stem, const std::string& ext)
{
    std::error_code ec;
    for (int n = 1;; ++n)
    {
        const fs::path p = folder / path_of_utf8(stem + (n == 1 ? "" : " (" + std::to_string(n) + ")") + ext);
        if (!fs::exists(p, ec)) return p;
    }
}

int compress_run(const Args& a)
{
#ifndef _WIN32
    // Names in UTF-8 whatever the locale: libarchive (built without iconv) writes a name in the
    // locale's character set, and in the "C" locale cannot write one with an accent at all. On
    // Windows it keeps names in UTF-16 and needs nothing.
    struct Locale
    {
        std::string was;
        Locale()
        {
            const char* now = std::setlocale(LC_CTYPE, nullptr);
            was = now ? now : "C";
            for (const char* l : {"C.UTF-8", "C.utf8", "en_US.UTF-8"})
                if (std::setlocale(LC_CTYPE, l)) break;
        }
        ~Locale() { std::setlocale(LC_CTYPE, was.c_str()); }
    } utf8_names;
#endif
    const std::string format = a.get("format");
    const auto& all = compress_formats();
    if (format.empty() || std::find(all.begin(), all.end(), format) == all.end())
        throw std::invalid_argument("--format takes one of: gz xz lzma bz2 zst lz4 br zip 7z tar");
    if (a.positional.empty()) throw std::invalid_argument("name a file or folder to compress");
    std::vector<std::string> paths = a.positional;
    if (a.has("gather"))
    {
        // Several chosen in Explorer: one program each, the first gathering the others (gather.hpp).
        const auto gathered = gather_chosen(paths[0], "compress " + format);
        if (!gathered) return 0; // (handed to the first, which makes the one result)
        paths = *gathered;
    }
    std::vector<CompressEntry> entries;
    std::vector<std::string> skipped;
    std::vector<fs::path> where;
    std::map<std::string, std::string> names; // (two chosen with one name, from different folders, refused)
    for (const std::string& s : paths)
    {
        std::error_code ec;
        fs::path p = fs::absolute(path_of_utf8(s), ec).lexically_normal();
        if (p.filename().empty()) p = p.parent_path(); // ("Documents/")
        if (!fs::exists(p, ec)) throw std::runtime_error("no such file or folder: " + s);
        const std::string name = utf8_of(p.filename());
        if (name.empty()) throw std::runtime_error("cannot compress a whole drive: " + s);
        if (!names.emplace(name, s).second) throw std::runtime_error("two chosen with one name, " + name + ": " + names[name] + " and " + s);
        where.push_back(p);
        gather_entries(p, name, entries, skipped);
    }
    for (const std::string& k : skipped) std::cout << "(left out, neither a file nor a folder: " << k << ")\n";
    if (entries.empty()) throw std::runtime_error("nothing to compress");

    const bool one_file = entries.size() == 1 && !entries[0].dir;
    uint64_t in_bytes = 0;
    for (const CompressEntry& e : entries) in_bytes += e.bytes.size();
    fs::path out;
    if (a.has("out"))
    {
        out = path_of_utf8(a.get("out"));
        std::error_code ec;
        if (fs::exists(out, ec) && !a.has("force")) throw std::runtime_error(a.get("out") + " is there already (--force to replace it)");
    }
    else
    {
        const fs::path folder = where[0].parent_path();
        std::string stem = where.size() == 1 ? utf8_of(where[0].filename()) : utf8_of(folder.filename());
        if (stem.empty()) stem = "compressed";
        std::string ext = "." + format;
        if (is_stream_format(format) && !one_file) ext = ".tar" + ext;
        // zip, 7z and tar named as 7-Zip names them, a file by its name without its extension
        // (notes.txt -> notes.zip); a stream by the whole name, as gzip does (notes.txt.gz).
        if (!is_stream_format(format) && one_file && where.size() == 1) stem = utf8_of(where[0].stem());
        out = free_name(folder, stem, ext);
    }

    std::cout << "Compressing " << (one_file ? std::string("1 file") : std::to_string(entries.size()) + " files and folders") << " ("
              << in_bytes << " bytes) to " << utf8_of(out.filename()) << " ...\n" << std::flush;
    const auto t0 = std::chrono::steady_clock::now();
    const Bytes made = compress_entries(entries, format);
    const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    {
        fs::path part = out;
        part += ".part"; // (not through string(), which on Windows cannot hold every name)
        std::ofstream o(part, std::ios::binary);
        o.write(reinterpret_cast<const char*>(made.data()), std::streamsize(made.size()));
        o.close();
        if (!o) throw std::runtime_error("cannot write " + utf8_of(part));
        std::error_code ec;
        fs::rename(part, out, ec);
        if (ec)
        {
            fs::remove(part, ec);
            throw std::runtime_error("cannot write " + utf8_of(out));
        }
    }
    char line[160];
    std::snprintf(line, sizeof line, "%llu bytes, %.2f%% of %llu, in %.1f s; read back and the same.",
                  (unsigned long long)made.size(), in_bytes ? 100.0 * double(made.size()) / double(in_bytes) : 0.0,
                  (unsigned long long)in_bytes, took);
    std::cout << utf8_of(out) << ": " << line << "\n";
    return 0;
}

} // namespace

int cmd_compress(const Args& a)
{
    // From the right-click menu (--from-menu), in a console of its own: a failure waits for Enter, so
    // that it can be read before the window closes; a success closes it after a moment.
    if (!a.has("from-menu")) return compress_run(a);
    try
    {
        const int r = compress_run(a);
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return r;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\nPress Enter to close.\n";
        std::string ignored;
        std::getline(std::cin, ignored);
        return 1;
    }
}

} // namespace sieve::cli
