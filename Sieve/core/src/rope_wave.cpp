// Sieve — the tightened rope, rope-wave-v1 (see rope_wave.hpp).
#include "sieve/rope_wave.hpp"

#include "sieve/sha256.hpp"
#include "sieve/weights.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace sieve {

namespace {

[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument("rope-wave: " + why); }

int64_t floor_div(int64_t a, int64_t b) // b > 0
{
    int64_t q = a / b;
    if ((a % b) != 0 && (a < 0)) --q;
    return q;
}

// The kept points of n values: start, a third, the centre (odd n only), two thirds, end.
std::vector<uint64_t> kept_points(uint64_t n)
{
    std::vector<uint64_t> p;
    if (n == 0) return p;
    p = {0, n / 3, 2 * n / 3, n - 1};
    if (n % 2) p.push_back((n - 1) / 2);
    std::sort(p.begin(), p.end());
    p.erase(std::unique(p.begin(), p.end()), p.end());
    return p;
}

struct Values
{
    std::vector<uint64_t> x;
    uint32_t bytes = 1; // L / 8
};

Values values_of(std::span<const uint8_t> file, uint32_t bits)
{
    Values v;
    v.bytes = bits / 8;
    const size_t n = file.size() / v.bytes;
    v.x.resize(n);
    for (size_t i = 0; i < n; ++i)
    {
        uint64_t a = 0;
        for (uint32_t b = 0; b < v.bytes; ++b) a |= uint64_t(file[i * v.bytes + b]) << (8 * b);
        v.x[i] = a;
    }
    return v;
}

// The rope at every value, from the kept points (0 everywhere without them).
std::vector<int64_t> rope_of(const std::vector<uint64_t>& x, const std::vector<uint64_t>& pts)
{
    std::vector<int64_t> r(x.size(), 0);
    if (pts.size() == 1) r[pts[0]] = int64_t(x[pts[0]]);
    for (size_t k = 0; k + 1 < pts.size(); ++k)
    {
        const uint64_t a = pts[k], b = pts[k + 1];
        const int64_t xa = int64_t(x[a]), dx = int64_t(x[b]) - xa, d = int64_t(b - a);
        for (uint64_t i = a; i <= b; ++i) r[i] = xa + floor_div(2 * dx * int64_t(i - a) + d, 2 * d);
    }
    return r;
}

// The prediction of value i from the values before it (and the rope).
uint64_t predict(const std::vector<uint64_t>& x, const std::vector<int64_t>& rope, uint64_t i, const RopeParams& p, uint64_t mask)
{
    const uint64_t col = i % p.width;
    const bool left = p.phases >= 1 && col >= p.step;
    const bool up = p.phases >= 2 && i >= p.width;
    const uint64_t r = uint64_t(rope[i]) & mask;
    if (!left && !up) return r;
    if (left && !up) return x[i - p.step];
    if (up && !left) return x[i - p.width];
    if (p.phases == 2) return (x[i - p.step] + x[i - p.width]) / 2;
    // phases 3: value before + line before - diagonal, held within the values
    const int64_t v = int64_t(x[i - p.step]) + int64_t(x[i - p.width]) - int64_t(x[i - p.width - p.step]);
    return v < 0 ? 0 : uint64_t(v) > mask ? mask : uint64_t(v);
}

uint64_t mask_of(uint32_t bits) { return bits == 64 ? ~uint64_t(0) : (uint64_t(1) << bits) - 1; }

// What the wave leaves, for every value that is not kept, in order; and the kept points.
std::vector<uint64_t> residuals(const std::vector<uint64_t>& x, const RopeParams& p, std::vector<uint64_t>& pts)
{
    if (x.size() > 0 && p.columns && p.width < 2) bad("columns need lines of two values or more");
    const uint64_t mask = mask_of(p.bits);
    pts = p.keep ? kept_points(x.size()) : std::vector<uint64_t>{};
    const std::vector<int64_t> rope = rope_of(x, pts);
    std::vector<uint64_t> e;
    e.reserve(x.size());
    size_t k = 0;
    for (uint64_t i = 0; i < x.size(); ++i)
    {
        if (k < pts.size() && pts[k] == i)
        {
            ++k;
            continue;
        }
        e.push_back((x[i] - predict(x, rope, i, p, mask)) & mask);
    }
    return e;
}

void put_varint(std::vector<uint8_t>& out, uint64_t v)
{
    while (v >= 0x80)
    {
        out.push_back(uint8_t(v | 0x80));
        v >>= 7;
    }
    out.push_back(uint8_t(v));
}

uint64_t get_varint(std::span<const uint8_t> in, size_t& at)
{
    uint64_t v = 0;
    for (int shift = 0;; shift += 7)
    {
        if (at >= in.size() || shift > 63) bad("a code that ends inside a number");
        const uint8_t b = in[at++];
        v |= uint64_t(b & 0x7F) << shift;
        if (!(b & 0x80)) return v;
    }
}

double entropy_bytes(const std::vector<uint64_t>& counts, uint64_t n)
{
    if (!n) return 1;
    double bits = 0;
    size_t k = 0;
    for (const uint64_t c : counts)
        if (c)
        {
            bits += double(c) * std::log2(double(n) / double(c));
            ++k;
        }
    // the table (a gap and a frequency for each byte present) and the stream's length and two words
    return bits / 8 + 1 + double(k) * 4 + 3 + 8;
}

// rope-wave-v2's segments: a segment with one byte present has no stream (every value is that byte).
double entropy_bytes_v2(const std::vector<uint64_t>& counts, uint64_t n)
{
    size_t k = 0;
    for (const uint64_t c : counts) k += c != 0;
    if (k <= 1) return 1 + double(k) * 4;
    return entropy_bytes(counts, n);
}

// One segment of what is left (a plane's bytes, or one column's of them): its table, then its rANS
// stream; v2 writes no stream for a segment of one byte value.
void put_segment(std::vector<uint8_t>& out, const std::vector<uint16_t>& sym, bool v2)
{
    if (sym.empty())
    {
        put_varint(out, 0);
        return;
    }
    std::vector<uint64_t> counts(65536, 0);
    for (const uint16_t b : sym) ++counts[b];
    const std::vector<uint32_t> freq = weights::quantise(counts);
    uint64_t k = 0;
    for (const uint32_t f : freq) k += f != 0;
    put_varint(out, k);
    uint32_t last = 0;
    bool first = true;
    for (uint32_t b = 0; b < freq.size(); ++b)
        if (freq[b])
        {
            put_varint(out, first ? b : b - last);
            put_varint(out, freq[b]);
            last = b;
            first = false;
        }
    if (v2 && k == 1) return;
    const std::vector<uint32_t> words = weights::encode(sym, freq);
    put_varint(out, words.size());
    for (const uint32_t w : words)
        for (int b = 0; b < 4; ++b) out.push_back(uint8_t(w >> (8 * b)));
}

std::vector<uint16_t> get_segment(std::span<const uint8_t> code, size_t& at, uint64_t count, bool v2)
{
    const uint64_t k = get_varint(code, at);
    if (k == 0)
    {
        if (count) bad("an empty segment for values that are there");
        return {};
    }
    if (k > 256 || !count) bad("a segment's table out of range");
    std::vector<uint32_t> freq(65536, 0);
    uint64_t s = 0, sum = 0;
    for (uint64_t j = 0; j < k; ++j)
    {
        const uint64_t gap = get_varint(code, at);
        if (j && gap == 0) bad("a table's symbols not increasing");
        s = j ? s + gap : gap;
        const uint64_t f = get_varint(code, at);
        if (s > 255 || f == 0 || f > (uint64_t(1) << 24)) bad("a table's entry out of range");
        freq[s] = uint32_t(f);
        sum += f;
    }
    if (sum != (uint64_t(1) << 24)) bad("a table that does not sum to 2^24");
    if (v2 && k == 1) return std::vector<uint16_t>(size_t(count), uint16_t(s));
    const uint64_t w = get_varint(code, at);
    if (w < 2 || w > (code.size() - at) / 4) bad("a segment's stream longer than the code");
    std::vector<uint32_t> words(w);
    for (uint64_t j = 0; j < w; ++j, at += 4)
        words[j] = uint32_t(code[at]) | uint32_t(code[at + 1]) << 8 | uint32_t(code[at + 2]) << 16 | uint32_t(code[at + 3]) << 24;
    return weights::decode(words, freq, count);
}

// rope-wave-v3 (C2): each byte's column, the bytes since the last line feed (10) before it, held
// below W (the last column takes every byte past it).
std::vector<uint32_t> line_columns(std::span<const uint8_t> b, uint32_t width)
{
    std::vector<uint32_t> col(b.size());
    uint64_t c = 0;
    for (size_t i = 0; i < b.size(); ++i)
    {
        col[i] = uint32_t(std::min<uint64_t>(c, width - 1));
        c = b[i] == 10 ? 0 : c + 1;
    }
    return col;
}

// The positions (value indices) of the values that are not kept, in order.
std::vector<uint64_t> free_positions(uint64_t n, const std::vector<uint64_t>& pts)
{
    std::vector<uint64_t> out;
    out.reserve(size_t(n - pts.size()));
    size_t k = 0;
    for (uint64_t i = 0; i < n; ++i)
    {
        if (k < pts.size() && pts[k] == i)
        {
            ++k;
            continue;
        }
        out.push_back(i);
    }
    return out;
}

// The segments, in the order the code holds them: for each plane, every value left (v1, or no
// columns), or each column's in turn (columns: position mod the line's width); each a list of
// indices into what is left.
std::vector<std::vector<size_t>> segments_of(const std::vector<uint64_t>& positions, const RopeParams& p)
{
    std::vector<std::vector<size_t>> seg(p.columns ? p.width : 1);
    for (size_t j = 0; j < positions.size(); ++j) seg[p.columns ? size_t(positions[j] % p.width) : 0].push_back(j);
    return seg;
}

// rope-wave-v4 (G1): columns that share a table. Each column's bytes are counted (counts: 256 to a
// column); the columns are taken in order, each joining the group whose cost grows least by it, or
// starting a group of its own where that costs less (a tie joins the first such group). A group's
// cost is its bytes' entropy, its table and its stream, as entropy_bytes_v2, and `per_group` more
// (the count C2 writes before each group). At most 256 groups: past them a column joins the best.
// Groups are numbered in the order they start, so each column's number is at most one past the
// largest before it (what a reader checks). Gives each column's group; `cost` their total.
std::vector<uint32_t> group_columns(const std::vector<uint64_t>& counts, const std::vector<uint64_t>& n, double per_group, double& cost)
{
    const size_t W = n.size();
    auto cost_of = [&](const uint64_t* c, uint64_t m) {
        if (!m) return 0.0;
        const std::vector<uint64_t> one(c, c + 256);
        return entropy_bytes_v2(one, m) + per_group;
    };
    std::vector<std::vector<uint64_t>> gc;
    std::vector<uint64_t> gn;
    std::vector<double> gcost;
    std::vector<uint32_t> group(W, 0);
    std::vector<uint64_t> merged(256);
    for (size_t c = 0; c < W; ++c)
    {
        const uint64_t* col = counts.data() + c * 256;
        const double alone = cost_of(col, n[c]);
        double best = gc.size() < 256 ? alone : HUGE_VAL;
        size_t pick = gc.size(); // a group of its own
        double best_cost = alone;
        for (size_t g = 0; g < gc.size(); ++g)
        {
            for (size_t b = 0; b < 256; ++b) merged[b] = gc[g][b] + col[b];
            const double m = cost_of(merged.data(), gn[g] + n[c]);
            const double d = m - gcost[g];
            if (d < best || (d == best && pick == gc.size()))
            {
                best = d;
                pick = g;
                best_cost = m;
            }
        }
        if (pick == gc.size())
        {
            gc.emplace_back(col, col + 256);
            gn.push_back(n[c]);
            gcost.push_back(alone);
        }
        else
        {
            for (size_t b = 0; b < 256; ++b) gc[pick][b] += col[b];
            gn[pick] += n[c];
            gcost[pick] = best_cost;
        }
        group[c] = uint32_t(pick);
    }
    cost = 0;
    for (const double g : gcost) cost += g;
    for (size_t c = 0; c < W; ++c) cost += group[c] < 128 ? 1 : 2; // each column's group number
    return group;
}

// The groups' numbers, written and read: one varint for each column.
void put_groups(std::vector<uint8_t>& out, const std::vector<uint32_t>& group)
{
    for (const uint32_t g : group) put_varint(out, g);
}

std::vector<uint32_t> get_groups(std::span<const uint8_t> code, size_t& at, uint32_t width, uint32_t& count)
{
    std::vector<uint32_t> group(width);
    count = 0;
    for (uint32_t c = 0; c < width; ++c)
    {
        const uint64_t g = get_varint(code, at);
        if (g > count || g >= 256) bad("a column's group past the groups started before it");
        group[c] = uint32_t(g);
        if (g == count) ++count;
    }
    return group;
}

} // namespace

void RopeParams::check() const
{
    if (bits != 8 && bits != 16 && bits != 32) bad("values of 8, 16 or 32 bits");
    if (width < 1) bad("a line of at least one value");
    if (step < 1 || step > 8) bad("a step of 1 to 8 values");
    if (phases > 3) bad("phases 0 to 3");
    if (columns > 2) bad("columns 0, 1 or 2");
    if (columns && width < 2) bad("columns need lines of two values or more");
    if (columns == 2 && (bits != 8 || phases != 0 || keep || step != 1))
        bad("columns by line (C2) are for bytes with no wave: L8, S1, P0, K0");
    if (grouped && !columns) bad("groups (G1) are of columns: C1 or C2 first");
}

std::string RopeParams::text() const
{
    return "L" + std::to_string(bits) + " W" + std::to_string(width) + " S" + std::to_string(step) + " P" + std::to_string(phases) + " K" +
           (keep ? "1" : "0") + (columns ? " C" + std::to_string(columns) : "") + (grouped ? " G1" : "");
}

RopeParams RopeParams::parse(std::string_view t)
{
    RopeParams p;
    unsigned long long L = 0, W = 0, S = 0, P = 0, K = 0;
    char extra = 0;
    const std::string s(t);
    unsigned long long C = 0, G = 0;
    const int got = std::sscanf(s.c_str(), "L%llu W%llu S%llu P%llu K%llu C%llu G%llu%c", &L, &W, &S, &P, &K, &C, &G, &extra);
    if ((got != 5 && got != 6 && got != 7) || K > 1 || C > 2 || G != (got == 7 ? 1u : 0u) || W > 0xFFFFFFFFull || L > 64 || S > 8 || P > 3)
        bad("parameters are written \"L8 W949 S1 P3 K0\" (rope-wave-v2: and \" C1\"; v3: \" C2\"; v4: and \" G1\"): " + s);
    p.columns = uint32_t(C);
    p.grouped = G == 1;
    p.bits = uint32_t(L);
    p.width = uint32_t(W);
    p.step = uint32_t(S);
    p.phases = uint32_t(P);
    p.keep = K == 1;
    p.check();
    if (p.text() != s) bad("parameters not written as they would be: " + s);
    return p;
}

std::vector<uint8_t> rope_code(std::span<const uint8_t> file, const RopeParams& p)
{
    p.check();
    if (file.size() >= (uint64_t(1) << 30)) bad("a file of a gigabyte or more");
    if (p.columns == 2)
    {
        // v3: each line column's bytes, its count first (a reader learns the columns only as it
        // makes the file again), then its segment.
        const std::vector<uint32_t> col = line_columns(file, p.width);
        std::vector<uint8_t> out;
        std::vector<uint32_t> group(p.width);
        for (uint32_t c = 0; c < p.width; ++c) group[c] = c;
        size_t groups = p.width;
        if (p.grouped)
        {
            // v4: the columns' groups first, then each group's count and segment.
            std::vector<uint64_t> counts(size_t(p.width) * 256, 0), n(p.width, 0);
            for (size_t i = 0; i < file.size(); ++i)
            {
                ++counts[size_t(col[i]) * 256 + file[i]];
                ++n[col[i]];
            }
            double cost = 0;
            group = group_columns(counts, n, 3, cost);
            put_groups(out, group);
            groups = size_t(*std::max_element(group.begin(), group.end())) + 1;
        }
        std::vector<std::vector<uint16_t>> seg(groups);
        for (size_t i = 0; i < file.size(); ++i) seg[group[col[i]]].push_back(file[i]);
        for (const auto& one : seg)
        {
            put_varint(out, one.size());
            put_segment(out, one, true);
        }
        return out;
    }
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<uint64_t> e = residuals(v.x, p, pts);
    std::vector<uint8_t> out;
    const std::vector<uint64_t> pos = free_positions(v.x.size(), pts);
    const std::vector<std::vector<size_t>> cols = segments_of(pos, p);
    for (uint32_t plane = 0; plane < v.bytes; ++plane)
    {
        auto byte_of = [&](size_t j) { return uint16_t((e[j] >> (8 * plane)) & 0xFF); };
        const std::vector<std::vector<size_t>>* seg = &cols;
        std::vector<std::vector<size_t>> grouped;
        if (p.grouped)
        {
            // v4: this plane's columns' groups, then each group's segment (its values in order).
            std::vector<uint64_t> counts(size_t(p.width) * 256, 0), n(p.width, 0);
            for (uint32_t c = 0; c < p.width; ++c)
            {
                for (const size_t j : cols[c]) ++counts[size_t(c) * 256 + byte_of(j)];
                n[c] = cols[c].size();
            }
            double cost = 0;
            const std::vector<uint32_t> group = group_columns(counts, n, 0, cost);
            put_groups(out, group);
            grouped.resize(size_t(*std::max_element(group.begin(), group.end())) + 1);
            for (size_t j = 0; j < pos.size(); ++j) grouped[group[size_t(pos[j] % p.width)]].push_back(j);
            seg = &grouped;
        }
        for (const std::vector<size_t>& one : *seg)
        {
            std::vector<uint16_t> sym(one.size());
            for (size_t i = 0; i < one.size(); ++i) sym[i] = byte_of(one[i]);
            put_segment(out, sym, p.columns != 0);
        }
    }
    for (const uint64_t i : pts)
        for (uint32_t b = 0; b < v.bytes; ++b) out.push_back(file[i * v.bytes + b]);
    for (size_t i = v.x.size() * v.bytes; i < file.size(); ++i) out.push_back(file[i]);
    return out;
}

std::vector<uint8_t> rope_file(std::span<const uint8_t> code, const RopeParams& p, uint64_t size)
{
    p.check();
    if (size >= (uint64_t(1) << 30)) bad("a file of a gigabyte or more");
    if (p.columns == 2)
    {
        size_t at = 0;
        std::vector<uint32_t> group(p.width);
        for (uint32_t c = 0; c < p.width; ++c) group[c] = c;
        uint32_t groups = p.width;
        if (p.grouped) group = get_groups(code, at, p.width, groups);
        std::vector<std::vector<uint16_t>> seg(groups);
        uint64_t total = 0;
        for (auto& one : seg)
        {
            const uint64_t n = get_varint(code, at);
            if (n > size - total) bad("columns holding more bytes than the file");
            total += n;
            one = get_segment(code, at, n, true);
        }
        if (total != size || at != code.size()) bad("the code does not end where the file's columns do");
        std::vector<uint8_t> out(static_cast<size_t>(size));
        std::vector<size_t> next(groups, 0);
        uint64_t c = 0;
        for (size_t i = 0; i < out.size(); ++i)
        {
            const size_t k = group[size_t(std::min<uint64_t>(c, p.width - 1))];
            if (next[k] >= seg[k].size()) bad("a column with fewer bytes than the file needs");
            out[i] = uint8_t(seg[k][next[k]++]);
            c = out[i] == 10 ? 0 : c + 1;
        }
        return out;
    }
    const uint32_t vb = p.bits / 8;
    const uint64_t n = size / vb;
    const std::vector<uint64_t> pts = p.keep ? kept_points(n) : std::vector<uint64_t>{};
    const uint64_t m = n - pts.size();
    size_t at = 0;
    std::vector<uint64_t> e(m, 0);
    const std::vector<uint64_t> pos = free_positions(n, pts);
    const std::vector<std::vector<size_t>> cols = segments_of(pos, p);
    for (uint32_t plane = 0; plane < vb; ++plane)
    {
        const std::vector<std::vector<size_t>>* seg = &cols;
        std::vector<std::vector<size_t>> grouped;
        if (p.grouped)
        {
            uint32_t groups = 0;
            const std::vector<uint32_t> group = get_groups(code, at, p.width, groups);
            grouped.resize(groups);
            for (size_t j = 0; j < pos.size(); ++j) grouped[group[size_t(pos[j] % p.width)]].push_back(j);
            seg = &grouped;
        }
        for (const std::vector<size_t>& one : *seg)
        {
            const std::vector<uint16_t> sym = get_segment(code, at, one.size(), p.columns != 0);
            for (size_t i = 0; i < one.size(); ++i) e[one[i]] |= uint64_t(sym[i]) << (8 * plane);
        }
    }
    const uint64_t tail = size - n * vb;
    if (code.size() - at != pts.size() * vb + tail) bad("the code does not end where the file's kept bytes do");
    std::vector<uint64_t> x(n, 0);
    for (size_t k = 0; k < pts.size(); ++k)
        for (uint32_t b = 0; b < vb; ++b) x[pts[k]] |= uint64_t(code[at + k * vb + b]) << (8 * b);
    const std::vector<int64_t> rope = rope_of(x, pts);
    const uint64_t mask = mask_of(p.bits);
    size_t k = 0, j = 0;
    for (uint64_t i = 0; i < n; ++i)
    {
        if (k < pts.size() && pts[k] == i)
        {
            ++k;
            continue;
        }
        x[i] = (e[j++] + predict(x, rope, i, p, mask)) & mask;
    }
    std::vector<uint8_t> out(static_cast<size_t>(size));
    for (uint64_t i = 0; i < n; ++i)
        for (uint32_t b = 0; b < vb; ++b) out[i * vb + b] = uint8_t(x[i] >> (8 * b));
    for (uint64_t t = 0; t < tail; ++t) out[n * vb + t] = code[at + pts.size() * vb + t];
    return out;
}

double rope_estimate(std::span<const uint8_t> file, const RopeParams& p)
{
    p.check();
    if (p.columns == 2)
    {
        const std::vector<uint32_t> col = line_columns(file, p.width);
        std::vector<uint64_t> counts(size_t(p.width) * 256, 0), n(p.width, 0);
        for (size_t i = 0; i < file.size(); ++i)
        {
            ++counts[size_t(col[i]) * 256 + file[i]];
            ++n[col[i]];
        }
        double bytes = 0;
        if (p.grouped)
        {
            group_columns(counts, n, 3, bytes);
            return bytes;
        }
        for (uint32_t c = 0; c < p.width; ++c)
        {
            const std::vector<uint64_t> one(counts.begin() + std::ptrdiff_t(c) * 256, counts.begin() + std::ptrdiff_t(c + 1) * 256);
            bytes += 3 + (n[c] ? entropy_bytes_v2(one, n[c]) : 1);
        }
        return bytes;
    }
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<uint64_t> e = residuals(v.x, p, pts);
    double bytes = double(pts.size() * v.bytes + (file.size() - v.x.size() * v.bytes));
    if (!p.columns)
    {
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
        {
            std::vector<uint64_t> counts(256, 0);
            for (const uint64_t r : e) ++counts[(r >> (8 * plane)) & 0xFF];
            bytes += entropy_bytes(counts, e.size());
        }
        return bytes;
    }
    // Columns: each column's bytes counted on their own, a pass over what is left.
    const std::vector<uint64_t> pos = free_positions(v.x.size(), pts);
    for (uint32_t plane = 0; plane < v.bytes; ++plane)
    {
        std::vector<uint64_t> counts(size_t(p.width) * 256, 0), n(p.width, 0);
        for (size_t j = 0; j < e.size(); ++j)
        {
            const size_t c = size_t(pos[j] % p.width);
            ++counts[c * 256 + ((e[j] >> (8 * plane)) & 0xFF)];
            ++n[c];
        }
        if (p.grouped)
        {
            double cost = 0;
            group_columns(counts, n, 0, cost);
            bytes += cost;
            continue;
        }
        for (uint32_t c = 0; c < p.width; ++c)
        {
            const std::vector<uint64_t> one(counts.begin() + std::ptrdiff_t(c) * 256, counts.begin() + std::ptrdiff_t(c + 1) * 256);
            bytes += n[c] ? entropy_bytes_v2(one, n[c]) : 1;
        }
    }
    return bytes;
}

namespace {

RopeSearch rope_search_uncached(std::span<const uint8_t> file, double seconds, uint32_t newest);

// The searches done in this run, by the file's SHA-256 and the time given: the File Locator weighs
// a file and then makes its instructions, and both pull it tight; the second is the first's answer.
std::mutex g_cache_mx;
std::map<std::string, RopeSearch> g_cache;

} // namespace

RopeSearch rope_search(std::span<const uint8_t> file, double seconds, uint32_t newest)
{
    if (newest < 1 || newest > 4) bad("the newest version searched is 1 to 4");
    char t[48];
    std::snprintf(t, sizeof t, "|%.6f|%u", seconds, newest);
    Sha256 h;
    h.update(file);
    const std::string key = Sha256::hex(h.finish()) + t;
    {
        std::lock_guard<std::mutex> lock(g_cache_mx);
        if (auto it = g_cache.find(key); it != g_cache.end()) return it->second;
    }
    RopeSearch r = rope_search_uncached(file, seconds, newest);
    std::lock_guard<std::mutex> lock(g_cache_mx);
    if (g_cache.size() >= 4096) g_cache.clear();
    g_cache.emplace(key, r);
    return r;
}

namespace {

RopeSearch rope_search_uncached(std::span<const uint8_t> file, double seconds, uint32_t newest)
{
    using steady = std::chrono::steady_clock;
    const auto start = steady::now();
    auto out_of_time = [&] { return seconds > 0 && std::chrono::duration<double>(steady::now() - start).count() >= seconds; };
    RopeSearch r;
    r.estimate = -1;
    auto try_one = [&](const RopeParams& p) {
        const double e = rope_estimate(file, p);
        ++r.tried;
        if (r.estimate < 0 || e < r.estimate)
        {
            r.estimate = e;
            r.best = p;
        }
    };
    try_one(RopeParams{}); // no wave at all: the bytes' own histogram
    // First the wave of the value before, at each value length: quick, and often most of the gain,
    // so that a short search still finds it.
    for (const uint32_t bits : {8u, 16u, 32u})
        if (file.size() / (bits / 8) >= 2 && !out_of_time()) try_one(RopeParams{bits, 1, 1, 2, false});
    for (const uint32_t bits : {8u, 16u, 32u})
    {
        const uint32_t vb = bits / 8;
        const uint64_t n = file.size() / vb;
        if (n < 2) continue;
        // The widths at which the values best match themselves a line before: the lags with the
        // smallest mean difference over evenly spread values.
        std::vector<std::pair<double, uint32_t>> lags;
        const uint64_t max_lag = std::min<uint64_t>(n - 1, 16384);
        const Values v = values_of(file, bits);
        for (uint64_t lag = 2; lag <= max_lag; ++lag)
        {
            if (out_of_time()) break;
            const uint64_t span = n - lag, samples = std::min<uint64_t>(span, 2048);
            double sum = 0;
            for (uint64_t s = 0; s < samples; ++s)
            {
                const uint64_t i = lag + s * span / samples;
                const double d = double(v.x[i]) - double(v.x[i - lag]);
                sum += d < 0 ? -d : d;
            }
            lags.emplace_back(sum / double(samples), uint32_t(lag));
        }
        std::vector<double> score(size_t(max_lag) + 1, -1);
        for (const auto& [sc, lag] : lags) score[lag] = sc;
        std::sort(lags.begin(), lags.end());
        // The four best, each taken down to its smallest divisor that matches almost as well (a
        // file's lines repeat at every multiple of their length, so the true length can sit below a
        // crowd of its multiples), and a multiple of one already taken left out (a picture of three
        // colours would otherwise give 3, 6, 9 and miss its rows).
        std::vector<uint32_t> widths{1};
        // And first, the length the best lags have in common: their greatest common divisors, pair
        // by pair among the best sixteen, the one found most often (at least three times) taken as
        // a width where it matches within 10% of the best lag. Where the values are noisy (digits, say) a line's own length scores no better than
        // its multiples, and the multiples' scores are what show it.
        {
            std::map<uint32_t, int> common;
            const size_t top = std::min<size_t>(lags.size(), 16);
            for (size_t a = 0; a < top; ++a)
                for (size_t b = a + 1; b < top; ++b)
                {
                    uint32_t x = lags[a].second, y = lags[b].second;
                    while (y) x = std::exchange(y, x % y);
                    if (x >= 2) ++common[x];
                }
            uint32_t best = 0;
            int seen = 2;
            for (const auto& [g, c] : common)
                if (c > seen)
                {
                    best = g;
                    seen = c;
                }
            // Only where it matches nearly as well as the best lag (within 10%): a divisor the best
            // lags share by chance (2, say, among lags near a picture's width) is no line.
            if (best && best <= max_lag && score[best] >= 0 && score[best] <= lags.front().first * 1.10 + 1e-9) widths.push_back(best);
        }
        for (size_t i = 0; i < lags.size() && widths.size() < 5; ++i)
        {
            uint32_t lag = lags[i].second;
            for (uint32_t d = 2; d < lag; ++d)
                if (lag % d == 0 && score[d] >= 0 && score[d] <= lags[i].first * 1.02 + 1e-9)
                {
                    lag = d;
                    break;
                }
            bool multiple = false;
            for (size_t w = 1; w < widths.size(); ++w) multiple = multiple || lag % widths[w] == 0;
            if (!multiple) widths.push_back(lag);
        }
        for (const uint32_t w : widths)
            for (uint32_t phases = 0; phases <= 3; ++phases)
                for (uint32_t step = 1; step <= 4; ++step)
                {
                    if (out_of_time())
                    {
                        r.timed_out = true;
                        goto keep;
                    }
                    if (phases == 0 && (w != 1 || step != 1)) continue;   // the rope alone: no lines, no step
                    if (w == 1 && (phases == 1 || step != 1)) continue;   // no line to step within
                    if (w > 1 && step >= w) continue;
                    try_one(RopeParams{bits, w, step, phases, false});
                }
        // rope-wave-v2: each width's columns counted apart, for every phase (a column the same
        // byte all the way down, a separator in a table, then costs only its table entry).
        for (const uint32_t w : widths)
            for (uint32_t phases = 0; phases <= 3 && w > 1 && w <= n / 4 && newest >= 2; ++phases)
            {
                if (out_of_time())
                {
                    r.timed_out = true;
                    goto keep;
                }
                try_one(RopeParams{bits, w, 1, phases, false, 1});
                // rope-wave-v4: and the columns in groups that share tables.
                if (newest >= 4 && !out_of_time()) try_one(RopeParams{bits, w, 1, phases, false, 1, true});
            }
    }
    // rope-wave-v3: columns counted from each line's start, for a file of lines (text, tables);
    // as many columns as its longest line, up to 4,096.
    if (newest >= 3 && !out_of_time())
    {
        uint64_t longest = 0, c = 0;
        bool lines = false;
        for (const uint8_t b : file)
        {
            if (b == 10)
            {
                lines = true;
                longest = std::max(longest, c + 1);
                c = 0;
            }
            else ++c;
        }
        longest = std::max(longest, c + 1);
        if (lines && longest >= 2)
        {
            const uint32_t w = uint32_t(std::min<uint64_t>(longest, 4096));
            try_one(RopeParams{8, w, 1, 0, false, 2});
            if (newest >= 4 && !out_of_time()) try_one(RopeParams{8, w, 1, 0, false, 2, true}); // rope-wave-v4
        }
    }
keep:
    if (!out_of_time())
    {
        RopeParams k = r.best;
        k.keep = true;
        if (k.columns != 2) try_one(k); // columns by line keep nothing
    }
    else r.timed_out = true;
    return r;
}

} // namespace

} // namespace sieve
