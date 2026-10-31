// Sieve CLI -- a PNG re-saved at its smallest (png_resave.hpp).

#include "cli/png_resave.hpp"

#include <zlib.h>

#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

namespace {

uint32_t be32(const uint8_t* p) { return uint32_t(p[0]) << 24 | uint32_t(p[1]) << 16 | uint32_t(p[2]) << 8 | uint32_t(p[3]); }

void put_be32(std::vector<uint8_t>& o, uint32_t v)
{
    for (int s = 24; s >= 0; s -= 8) o.push_back(uint8_t(v >> s));
}

const uint8_t kSignature[8] = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};

// What the IHDR says of the rows: how many, each's bytes, and a pixel's (at least 1, the distance
// PNG's filters look back).
struct Rows
{
    uint64_t height = 0, row_bytes = 0, pixel_bytes = 0;
    bool interlaced = false;
};

// The IHDR, the first chunk after the signature; nothing where it is not one PNG allows.
std::optional<Rows> read_ihdr(const uint8_t* b, size_t n)
{
    if (n < 8 + 25 || std::memcmp(b, kSignature, 8) != 0 || be32(b + 8) != 13 || std::memcmp(b + 12, "IHDR", 4) != 0) return std::nullopt;
    const uint32_t w = be32(b + 16), h = be32(b + 20);
    const int depth = b[24], type = b[25];
    if (w == 0 || h == 0 || w > 0x7FFFFFFFu || h > 0x7FFFFFFFu || b[26] != 0 || b[27] != 0 || b[28] > 1) return std::nullopt;
    int channels = 0;
    switch (type)
    {
    case 0: channels = (depth == 1 || depth == 2 || depth == 4 || depth == 8 || depth == 16) ? 1 : 0; break;
    case 2: channels = (depth == 8 || depth == 16) ? 3 : 0; break;
    case 3: channels = (depth == 1 || depth == 2 || depth == 4 || depth == 8) ? 1 : 0; break;
    case 4: channels = (depth == 8 || depth == 16) ? 2 : 0; break;
    case 6: channels = (depth == 8 || depth == 16) ? 4 : 0; break;
    default: break;
    }
    if (channels == 0) return std::nullopt;
    Rows r;
    const uint64_t bits = uint64_t(channels) * uint64_t(depth);
    r.height = h;
    r.row_bytes = (uint64_t(w) * bits + 7) / 8;
    r.pixel_bytes = bits >= 8 ? bits / 8 : 1;
    r.interlaced = b[28] == 1;
    // The rows with their filter bytes must fit one zlib call (its sizes are 32 bits).
    if (r.row_bytes * r.height + r.height > 0xFFFFFFFFu) return std::nullopt;
    return r;
}

int paeth(int a, int b, int c)
{
    const int p = a + b - c, pa = std::abs(p - a), pb = std::abs(p - b), pc = std::abs(p - c);
    return pa <= pb && pa <= pc ? a : pb <= pc ? b : c;
}

// One row filtered by filter f (0 to 4) into out, `prior` the row above unfiltered (zeros for the first).
void filter_row(int f, const uint8_t* row, const uint8_t* prior, size_t n, size_t bpp, uint8_t* out)
{
    for (size_t i = 0; i < n; ++i)
    {
        const int a = i >= bpp ? row[i - bpp] : 0, b = prior[i], c = i >= bpp ? prior[i - bpp] : 0;
        const int pred = f == 0 ? 0 : f == 1 ? a : f == 2 ? b : f == 3 ? (a + b) / 2 : paeth(a, b, c);
        out[i] = uint8_t(row[i] - pred);
    }
}

// The rows filtered by `filter` (0 to 4, or 5 each row its own), each with its filter byte before it.
std::vector<uint8_t> filtered_rows(const std::vector<uint8_t>& inner, uint64_t head, const Rows& rows, int filter)
{
    const size_t n = size_t(rows.row_bytes), bpp = size_t(rows.pixel_bytes);
    std::vector<uint8_t> filtered(size_t(rows.height) * (n + 1)), zero(n, 0), best(n), trial(n);
    const uint8_t* px = inner.data() + head;
    for (size_t y = 0; y < size_t(rows.height); ++y)
    {
        const uint8_t* row = px + y * n;
        const uint8_t* prior = y ? row - n : zero.data();
        uint8_t* out = filtered.data() + y * (n + 1);
        int f = filter;
        if (filter == 5)
        {
            // Each row the filter whose bytes, as signed, sum to the least (libpng's measure).
            uint64_t least = ~uint64_t(0);
            for (int t = 0; t <= 4; ++t)
            {
                filter_row(t, row, prior, n, bpp, trial.data());
                uint64_t sum = 0;
                for (const uint8_t v : trial) sum += uint64_t(v < 128 ? v : 256 - v);
                if (sum < least) least = sum, f = t, best.swap(trial);
            }
            out[0] = uint8_t(f);
            std::memcpy(out + 1, best.data(), n);
            continue;
        }
        out[0] = uint8_t(f);
        filter_row(f, row, prior, n, bpp, out + 1);
    }
    return filtered;
}

// The PNG: the filtered rows deflated by `strategy` into one IDAT, put into the head at `split`.
std::vector<uint8_t> assembled(const std::vector<uint8_t>& inner, const std::vector<uint8_t>& filtered, int strategy, uint64_t split, uint64_t head)
{
    z_stream z{};
    if (deflateInit2(&z, 9, Z_DEFLATED, 15, 9, strategy ? Z_FILTERED : Z_DEFAULT_STRATEGY) != Z_OK) throw std::runtime_error("zlib could not begin");
    std::vector<uint8_t> data(deflateBound(&z, uLong(filtered.size())));
    z.next_in = const_cast<uint8_t*>(filtered.data());
    z.avail_in = uInt(filtered.size());
    z.next_out = data.data();
    z.avail_out = uInt(data.size());
    const int r = deflate(&z, Z_FINISH);
    data.resize(z.total_out);
    deflateEnd(&z);
    if (r != Z_STREAM_END || data.size() > 0x7FFFFFFFu) throw std::runtime_error("zlib could not pack a PNG's rows");
    std::vector<uint8_t> png(inner.begin(), inner.begin() + std::ptrdiff_t(split));
    const size_t at = png.size();
    put_be32(png, uint32_t(data.size()));
    png.insert(png.end(), {'I', 'D', 'A', 'T'});
    png.insert(png.end(), data.begin(), data.end());
    put_be32(png, uint32_t(crc32(0, png.data() + at + 4, uInt(data.size() + 4))));
    png.insert(png.end(), inner.begin() + std::ptrdiff_t(split), inner.begin() + std::ptrdiff_t(head));
    return png;
}

} // namespace

std::vector<uint8_t> png_resave_make(const std::vector<uint8_t>& inner, int filter, int strategy, uint64_t split, uint64_t head)
{
    if (filter < 0 || filter > 5 || strategy < 0 || strategy > 1 || head > inner.size() || split > head || split < 8 + 25)
        throw std::runtime_error("a re-saved PNG's recipe this Sieve does not know");
    const auto rows = read_ihdr(inner.data(), size_t(head));
    if (!rows || rows->interlaced || inner.size() - head != rows->row_bytes * rows->height)
        throw std::runtime_error("a re-saved PNG whose head and rows do not agree");
    return assembled(inner, filtered_rows(inner, head, *rows, filter), strategy, split, head);
}

static std::optional<PngResaved> png_resave_unguarded(const std::vector<uint8_t>& b);

std::optional<PngResaved> png_resave(const std::vector<uint8_t>& b)
{
    // Nothing a PNG holds stops the packing: one that cannot be re-saved (zlib refusing it, or the
    // memory short) is left as it is.
    try
    {
        return png_resave_unguarded(b);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

static std::optional<PngResaved> png_resave_unguarded(const std::vector<uint8_t>& b)
{
    const auto rows = read_ihdr(b.data(), b.size());
    if (!rows || rows->interlaced) return std::nullopt;
    // Every chunk's CRC right, the IDATs one run.
    size_t at = 8, first = 0, end = 0;
    std::vector<uint8_t> joined;
    bool after = false;
    while (at < b.size())
    {
        if (b.size() - at < 12) return std::nullopt;
        const uint32_t len = be32(b.data() + at);
        if (len > 0x7FFFFFFFu || len > b.size() - at - 12) return std::nullopt;
        if (uint32_t(crc32(0, b.data() + at + 4, uInt(len + 4))) != be32(b.data() + at + 8 + len)) return std::nullopt;
        const bool idat = std::memcmp(b.data() + at + 4, "IDAT", 4) == 0;
        if (idat)
        {
            if (after) return std::nullopt;
            if (!first) first = at;
            joined.insert(joined.end(), b.begin() + std::ptrdiff_t(at + 8), b.begin() + std::ptrdiff_t(at + 8 + len));
            end = at + 12 + len;
        }
        else if (first) after = true;
        const bool iend = std::memcmp(b.data() + at + 4, "IEND", 4) == 0;
        at += 12 + len;
        if (iend) break; // (whatever follows IEND is kept as it is, in the head)
    }
    if (!first) return std::nullopt;
    // The rows: exactly one zlib stream of exactly the rows, each with its filter byte. (Deflate makes
    // at most about 1,032 bytes of each it is given, so an IHDR claiming more rows than its IDATs can
    // hold is refused before anything that size is made.)
    const size_t n = size_t(rows->row_bytes), bpp = size_t(rows->pixel_bytes), h = size_t(rows->height);
    if (uint64_t(h) * (n + 1) > uint64_t(joined.size()) * 1032 + 1032) return std::nullopt;
    std::vector<uint8_t> filtered(h * (n + 1) + 1);
    z_stream z{};
    if (inflateInit(&z) != Z_OK) return std::nullopt;
    z.next_in = joined.data();
    z.avail_in = uInt(joined.size());
    z.next_out = filtered.data();
    z.avail_out = uInt(filtered.size());
    const int r = inflate(&z, Z_FINISH);
    const size_t got = z.total_out;
    inflateEnd(&z);
    if (r != Z_STREAM_END || got != h * (n + 1)) return std::nullopt;
    PngResaved out;
    out.inner.assign(b.begin(), b.begin() + std::ptrdiff_t(first));
    out.inner.insert(out.inner.end(), b.begin() + std::ptrdiff_t(end), b.end());
    const size_t head = out.inner.size();
    out.inner.resize(head + h * n);
    std::vector<uint8_t> zero(n, 0);
    for (size_t y = 0; y < h; ++y)
    {
        const uint8_t* in = filtered.data() + y * (n + 1);
        uint8_t* row = out.inner.data() + head + y * n;
        const uint8_t* prior = y ? row - n : zero.data();
        const int f = in[0];
        if (f > 4) return std::nullopt;
        for (size_t i = 0; i < n; ++i)
        {
            const int a = i >= bpp ? row[i - bpp] : 0, bb = prior[i], c = i >= bpp ? prior[i - bpp] : 0;
            const int pred = f == 0 ? 0 : f == 1 ? a : f == 2 ? bb : f == 3 ? (a + bb) / 2 : paeth(a, bb, c);
            row[i] = uint8_t(in[1 + i] + pred);
        }
    }
    // Every filter (each alone, or each row its own) at both of zlib's strategies; the smallest kept,
    // the first of equals in the order strategy 0's six, then strategy 1's. Each filter's rows made
    // once, for both strategies.
    const int filters[6] = {5, 0, 1, 2, 3, 4};
    size_t best_order = 0;
    for (size_t fi = 0; fi < 6; ++fi)
    {
        const std::vector<uint8_t> f = filtered_rows(out.inner, head, *rows, filters[fi]);
        for (const int strategy : {0, 1})
        {
            std::vector<uint8_t> png = assembled(out.inner, f, strategy, first, head);
            const size_t order = size_t(strategy) * 6 + fi;
            if (out.png.empty() || png.size() < out.png.size() || (png.size() == out.png.size() && order < best_order))
            {
                best_order = order;
                out.png = std::move(png);
                out.recipe = "pngr " + std::to_string(filters[fi]) + " " + std::to_string(strategy) + " " + std::to_string(first) + " " + std::to_string(head) +
                             " " + std::to_string(out.inner.size());
            }
        }
    }
    return out;
}

} // namespace sieve::cli
