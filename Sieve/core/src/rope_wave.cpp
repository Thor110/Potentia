// Sieve — the tightened rope, rope-wave-v1 (see rope_wave.hpp).
#include "sieve/rope_wave.hpp"
#include "sieve/stop.hpp"

#include "sieve/sha256.hpp"
#include "sieve/weights.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <map>
#include <mutex>
#include <stdexcept>
#include <unordered_map>
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

// A file's values read where they lie: value i is the L / 8 bytes at i * L / 8, little-endian. (A
// copy of 64 bits a value took eight times a file of bytes, and the search makes many: 28 October
// 2026, a 1.4 GB folder's packing went past 11 GB.)
struct FileValues
{
    const uint8_t* b = nullptr;
    uint32_t vb = 1;
    size_t n = 0;
    size_t size() const { return n; }
    uint64_t operator[](size_t i) const
    {
        const uint8_t* q = b + i * vb;
        if (vb == 1) return q[0];
        uint64_t a = 0;
        for (uint32_t k = 0; k < vb; ++k) a |= uint64_t(q[k]) << (8 * k);
        return a;
    }
};

struct Values
{
    FileValues x;
    uint32_t bytes = 1; // L / 8
};

Values values_of(std::span<const uint8_t> file, uint32_t bits)
{
    Values v;
    v.bytes = bits / 8;
    v.x = FileValues{file.data(), v.bytes, file.size() / v.bytes};
    return v;
}

// The rope at every value, from the kept points (0 everywhere without them: then nothing is made,
// and a reader takes an empty rope as 0).
template <class X>
std::vector<int64_t> rope_of(const X& x, const std::vector<uint64_t>& pts)
{
    if (pts.empty()) return {};
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

// The prediction of value i from the values before it (and the rope). X: the values, read in place
// or held at their own width; each is taken as 64 bits before anything is done with it.
template <class X>
uint64_t predict(const X& x, const std::vector<int64_t>& rope, uint64_t i, const RopeParams& p, uint64_t mask)
{
    const uint64_t col = i % p.width;
    const bool left = p.phases >= 1 && col >= p.step;
    const bool up = p.phases >= 2 && i >= p.width;
    const uint64_t r = rope.empty() ? 0 : uint64_t(rope[i]) & mask;
    if (!left && !up) return r;
    if (left && !up) return uint64_t(x[i - p.step]);
    if (up && !left) return uint64_t(x[i - p.width]);
    if (p.phases == 2) return (uint64_t(x[i - p.step]) + uint64_t(x[i - p.width])) / 2;
    if (p.phases == 4)
    {
        // rope-wave-v6: the triangle (the median edge detector).
        const uint64_t a = x[i - p.step], b = x[i - p.width], c = x[i - p.width - p.step];
        const uint64_t mx = std::max(a, b), mn = std::min(a, b);
        if (c >= mx) return mn;
        if (c <= mn) return mx;
        return a + b - c; // between mn and mx
    }
    // phases 3: value before + line before - diagonal, held within the values
    const int64_t v = int64_t(x[i - p.step]) + int64_t(x[i - p.width]) - int64_t(x[i - p.width - p.step]);
    return v < 0 ? 0 : uint64_t(v) > mask ? mask : uint64_t(v);
}

uint64_t mask_of(uint32_t bits) { return bits == 64 ? ~uint64_t(0) : (uint64_t(1) << bits) - 1; }

// rope-wave-v6 (T1): how busy value i's triangle is, as a bucket 0 to 7, or 8 where it has none.
constexpr uint32_t kTriangleBuckets = 9;
template <class X>
uint32_t triangle_bucket(const X& x, uint64_t i, const RopeParams& p)
{
    if (i % p.width < p.step || i < p.width + p.step) return 8;
    const uint64_t a = x[i - p.step], b = x[i - p.width], c = x[i - p.width - p.step];
    const uint64_t act = ((a > c ? a - c : c - a) + (b > c ? b - c : c - b)) >> (p.bits - 8);
    uint32_t k = 0;
    for (const uint64_t bound : {2, 4, 8, 16, 32, 64, 128}) k += act >= bound;
    return k;
}

// What the wave leaves, for every value that is not kept, in order; and the kept points. E: each
// held at the values' own width (what is left is below 2^L).
template <class E, class X>
std::vector<E> residuals(const X& x, const RopeParams& p, std::vector<uint64_t>& pts)
{
    if (x.size() > 0 && p.columns && p.width < 2) bad("columns need lines of two values or more");
    const uint64_t mask = mask_of(p.bits);
    pts = p.keep ? kept_points(x.size()) : std::vector<uint64_t>{};
    const std::vector<int64_t> rope = rope_of(x, pts);
    std::vector<E> e;
    e.reserve(x.size() - pts.size());
    size_t k = 0;
    for (uint64_t i = 0; i < x.size(); ++i)
    {
        if (k < pts.size() && pts[k] == i)
        {
            ++k;
            continue;
        }
        e.push_back(E((uint64_t(x[i]) - predict(x, rope, i, p, mask)) & mask));
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
std::vector<uint32_t> group_columns(const std::vector<uint64_t>& counts, const std::vector<uint64_t>& n, double per_group, double& cost,
                                    bool numbers_counted = true)
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
    if (numbers_counted)
        for (size_t c = 0; c < W; ++c) cost += group[c] < 128 ? 1 : 2; // each column's group number
    return group;
}

// rope-wave-v5: each byte's pair (its column in its line, held below W; the byte above it, the same
// column of the line before, 256 where that line is shorter or there is none), numbered in the
// order the pairs are first met. Gives each byte's pair's number; `pairs` their count.
std::vector<uint32_t> above_pairs(std::span<const uint8_t> b, uint32_t width, uint32_t& pairs)
{
    std::vector<uint32_t> out(b.size());
    std::unordered_map<uint64_t, uint32_t> seen;
    size_t line = 0, prev = 0, prev_len = 0; // where this line and the one before start; its length
    bool have_prev = false;
    for (size_t i = 0; i < b.size(); ++i)
    {
        const size_t c = i - line;
        const uint32_t col = uint32_t(std::min<size_t>(c, width - 1));
        const uint32_t up = have_prev && c < prev_len ? b[prev + c] : 256u;
        const uint64_t key = uint64_t(col) * 257 + up;
        auto it = seen.find(key);
        if (it == seen.end()) it = seen.emplace(key, uint32_t(seen.size())).first;
        out[i] = it->second;
        if (b[i] == 10)
        {
            prev = line;
            prev_len = i + 1 - line;
            have_prev = true;
            line = i + 1;
        }
    }
    pairs = uint32_t(seen.size());
    return out;
}

// v5's estimate: the groups' cost, and their numbers (one a pair) coded as a segment of their own.
double above_cost(const std::vector<uint32_t>& group)
{
    std::vector<uint64_t> h(256, 0);
    for (const uint32_t g : group) ++h[g];
    return entropy_bytes_v2(h, group.size()) + 3;
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
    if (phases > 4) bad("phases 0 to 4");
    if (triangle && (phases < 3 || columns)) bad("the triangle's contexts (T1) are for P3 or P4, with no columns");
    if (columns > 2) bad("columns 0, 1 or 2");
    if (above && (columns != 2 || !grouped)) bad("the byte above (A1) is for columns by line in groups: C2 G1 first");
    if (columns && width < 2) bad("columns need lines of two values or more");
    if (columns == 2 && (bits != 8 || phases != 0 || keep || step != 1))
        bad("columns by line (C2) are for bytes with no wave: L8, S1, P0, K0");
    if (grouped && !columns) bad("groups (G1) are of columns: C1 or C2 first");
}

std::string RopeParams::text() const
{
    return "L" + std::to_string(bits) + " W" + std::to_string(width) + " S" + std::to_string(step) + " P" + std::to_string(phases) + " K" +
           (keep ? "1" : "0") + (columns ? " C" + std::to_string(columns) : "") + (grouped ? " G1" : "") + (above ? " A1" : "") + (triangle ? " T1" : "");
}

RopeParams RopeParams::parse(std::string_view t)
{
    RopeParams p;
    unsigned long long L = 0, W = 0, S = 0, P = 0, K = 0;
    char extra = 0;
    std::string s(t);
    // rope-wave-v6's " T1", last.
    const bool tri = s.size() > 3 && s.compare(s.size() - 3, 3, " T1") == 0;
    if (tri) s.resize(s.size() - 3);
    unsigned long long C = 0, G = 0, A = 0;
    const int got = std::sscanf(s.c_str(), "L%llu W%llu S%llu P%llu K%llu C%llu G%llu A%llu%c", &L, &W, &S, &P, &K, &C, &G, &A, &extra);
    if (got < 5 || got > 8 || K > 1 || C > 2 || G != (got >= 7 ? 1u : 0u) || A != (got == 8 ? 1u : 0u) || W > 0xFFFFFFFFull || L > 64 || S > 8 || P > 4)
        bad("parameters are written \"L8 W949 S1 P3 K0\" (rope-wave-v2: and \" C1\"; v3: \" C2\"; v4: and \" G1\"; v5: and \" A1\"; v6: P4, and \" T1\"): " + std::string(t));
    p.columns = uint32_t(C);
    p.grouped = G == 1;
    p.above = A == 1;
    p.bits = uint32_t(L);
    p.width = uint32_t(W);
    p.step = uint32_t(S);
    p.phases = uint32_t(P);
    p.keep = K == 1;
    p.triangle = tri;
    p.check();
    if (p.text() != std::string(t)) bad("parameters not written as they would be: " + std::string(t));
    return p;
}

// rope_code's values (no columns by line): what the wave leaves, E wide, then each plane's segments.
// Where no value is kept, a value's place among those left is its own (no list of them is made).
template <class E>
std::vector<uint8_t> code_values(std::span<const uint8_t> file, const RopeParams& p)
{
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<E> e = residuals<E>(v.x, p, pts);
    std::vector<uint8_t> out;
    std::vector<uint64_t> listed;
    if (!pts.empty() || p.columns) listed = free_positions(v.x.size(), pts);
    auto pos = [&](size_t j) { return pts.empty() && !p.columns ? uint64_t(j) : listed[j]; };
    if (p.triangle)
    {
        // v6 (T1): for each plane, each bucket's count and segment, the values left in order.
        std::vector<uint8_t> bucket(e.size());
        for (size_t j = 0; j < e.size(); ++j) bucket[j] = uint8_t(triangle_bucket(v.x, pos(j), p));
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
            for (uint32_t k = 0; k < kTriangleBuckets; ++k)
            {
                std::vector<uint16_t> sym;
                for (size_t j = 0; j < e.size(); ++j)
                    if (bucket[j] == k) sym.push_back(uint16_t((uint64_t(e[j]) >> (8 * plane)) & 0xFF));
                put_varint(out, sym.size());
                put_segment(out, sym, true);
            }
        for (const uint64_t i : pts)
            for (uint32_t b = 0; b < v.bytes; ++b) out.push_back(file[i * v.bytes + b]);
        for (size_t i = v.x.size() * v.bytes; i < file.size(); ++i) out.push_back(file[i]);
        return out;
    }
    auto byte_of = [&](size_t j, uint32_t plane) { return uint16_t((uint64_t(e[j]) >> (8 * plane)) & 0xFF); };
    if (!p.columns)
    {
        // v1's one segment a plane: every value left, in order.
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
        {
            std::vector<uint16_t> sym(e.size());
            for (size_t j = 0; j < e.size(); ++j) sym[j] = byte_of(j, plane);
            put_segment(out, sym, false);
        }
    }
    else
    {
        const std::vector<std::vector<size_t>> cols = segments_of(listed, p);
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
        {
            const std::vector<std::vector<size_t>>* seg = &cols;
            std::vector<std::vector<size_t>> grouped;
            if (p.grouped)
            {
                // v4: this plane's columns' groups, then each group's segment (its values in order).
                std::vector<uint64_t> counts(size_t(p.width) * 256, 0), n(p.width, 0);
                for (uint32_t c = 0; c < p.width; ++c)
                {
                    for (const size_t j : cols[c]) ++counts[size_t(c) * 256 + byte_of(j, plane)];
                    n[c] = cols[c].size();
                }
                double cost = 0;
                const std::vector<uint32_t> group = group_columns(counts, n, 0, cost);
                put_groups(out, group);
                grouped.resize(size_t(*std::max_element(group.begin(), group.end())) + 1);
                for (size_t j = 0; j < listed.size(); ++j) grouped[group[size_t(listed[j] % p.width)]].push_back(j);
                seg = &grouped;
            }
            for (const std::vector<size_t>& one : *seg)
            {
                std::vector<uint16_t> sym(one.size());
                for (size_t i = 0; i < one.size(); ++i) sym[i] = byte_of(one[i], plane);
                put_segment(out, sym, true);
            }
        }
    }
    for (const uint64_t i : pts)
        for (uint32_t b = 0; b < v.bytes; ++b) out.push_back(file[i * v.bytes + b]);
    for (size_t i = v.x.size() * v.bytes; i < file.size(); ++i) out.push_back(file[i]);
    return out;
}

std::vector<uint8_t> rope_code(std::span<const uint8_t> file, const RopeParams& p)
{
    p.check();
    if (file.size() >= (uint64_t(1) << 30)) bad("a file of a gigabyte or more");
    if (p.columns == 2 && p.above)
    {
        // v5: the pairs' count and their groups (a segment of group numbers, one a pair, in the
        // order the pairs are first met), then each group's count and segment.
        uint32_t pairs = 0;
        const std::vector<uint32_t> pair = above_pairs(file, p.width, pairs);
        std::vector<uint64_t> counts(size_t(pairs) * 256, 0), n(pairs, 0);
        for (size_t i = 0; i < file.size(); ++i)
        {
            ++counts[size_t(pair[i]) * 256 + file[i]];
            ++n[pair[i]];
        }
        double cost = 0;
        const std::vector<uint32_t> group = pairs ? group_columns(counts, n, 3, cost, false) : std::vector<uint32_t>{};
        std::vector<uint8_t> out;
        put_varint(out, pairs);
        put_segment(out, std::vector<uint16_t>(group.begin(), group.end()), true);
        const size_t groups = group.empty() ? 0 : size_t(*std::max_element(group.begin(), group.end())) + 1;
        std::vector<std::vector<uint16_t>> seg(groups);
        for (size_t i = 0; i < file.size(); ++i) seg[group[pair[i]]].push_back(file[i]);
        for (const auto& one : seg)
        {
            put_varint(out, one.size());
            put_segment(out, one, true);
        }
        return out;
    }
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
    if (p.bits == 8) return code_values<uint8_t>(file, p);
    if (p.bits == 16) return code_values<uint16_t>(file, p);
    return code_values<uint32_t>(file, p);
}

// rope_file's values (no columns by line), each held V wide (L bits). What the wave leaves is read
// straight into the values' places, and each made the value by adding its prediction, which reads
// only values before it; for bytes, the values are the file itself.
template <class V>
std::vector<uint8_t> file_values(std::span<const uint8_t> code, const RopeParams& p, uint64_t size)
{
    const uint32_t vb = p.bits / 8;
    const uint64_t n = size / vb;
    const std::vector<uint64_t> pts = p.keep ? kept_points(n) : std::vector<uint64_t>{};
    const uint64_t m = n - pts.size();
    size_t at = 0;
    std::vector<V> x(static_cast<size_t>(n), 0);
    // The kept values, after the segments; and how many bytes past the last whole value.
    auto finish = [&](size_t from) {
        const uint64_t tail = size - n * vb;
        if (code.size() - from != pts.size() * vb + tail) bad("the code does not end where the file's kept bytes do");
        for (size_t k = 0; k < pts.size(); ++k)
            for (uint32_t b = 0; b < vb; ++b) x[pts[k]] = V(uint64_t(x[pts[k]]) | uint64_t(code[from + k * vb + b]) << (8 * b));
        return tail;
    };
    auto output = [&](size_t from, uint64_t tail) {
        if constexpr (sizeof(V) == 1)
            if (tail == 0) return std::vector<uint8_t>(std::move(x)); // (the values are the bytes)
        std::vector<uint8_t> out(static_cast<size_t>(size));
        for (uint64_t i = 0; i < n; ++i)
            for (uint32_t b = 0; b < vb; ++b) out[i * vb + b] = uint8_t(uint64_t(x[i]) >> (8 * b));
        for (uint64_t t = 0; t < tail; ++t) out[n * vb + t] = code[from + pts.size() * vb + t];
        return out;
    };
    const uint64_t mask = mask_of(p.bits);
    if (p.triangle)
    {
        // v6 (T1): every plane's buckets read first; then the file made again value by value, each
        // taking the next of its triangle's bucket in each plane.
        std::vector<std::vector<uint16_t>> seg(size_t(vb) * kTriangleBuckets);
        for (uint32_t plane = 0; plane < vb; ++plane)
        {
            uint64_t total = 0;
            for (uint32_t k = 0; k < kTriangleBuckets; ++k)
            {
                const uint64_t c = get_varint(code, at);
                if (c > m - total) bad("buckets holding more values than the file");
                total += c;
                seg[size_t(plane) * kTriangleBuckets + k] = get_segment(code, at, c, true);
            }
            if (total != m) bad("buckets holding fewer values than the file");
        }
        const uint64_t tail = finish(at);
        const std::vector<int64_t> rope = rope_of(x, pts);
        std::vector<size_t> next(seg.size(), 0);
        size_t k = 0;
        for (uint64_t i = 0; i < n; ++i)
        {
            if (k < pts.size() && pts[k] == i)
            {
                ++k;
                continue;
            }
            const uint32_t bk = triangle_bucket(x, i, p);
            uint64_t ei = 0;
            for (uint32_t plane = 0; plane < vb; ++plane)
            {
                const size_t s = size_t(plane) * kTriangleBuckets + bk;
                if (next[s] >= seg[s].size()) bad("a bucket with fewer values than the file needs");
                ei |= uint64_t(seg[s][next[s]++]) << (8 * plane);
            }
            x[i] = V((ei + predict(x, rope, i, p, mask)) & mask);
        }
        return output(at, tail);
    }
    std::vector<uint64_t> listed;
    if (!pts.empty() || p.columns) listed = free_positions(n, pts);
    auto pos = [&](size_t j) { return pts.empty() && !p.columns ? uint64_t(j) : listed[j]; };
    if (!p.columns)
    {
        for (uint32_t plane = 0; plane < vb; ++plane)
        {
            const std::vector<uint16_t> sym = get_segment(code, at, m, false);
            for (size_t j = 0; j < sym.size(); ++j) x[pos(j)] = V(uint64_t(x[pos(j)]) | uint64_t(sym[j]) << (8 * plane));
        }
    }
    else
    {
        const std::vector<std::vector<size_t>> cols = segments_of(listed, p);
        for (uint32_t plane = 0; plane < vb; ++plane)
        {
            const std::vector<std::vector<size_t>>* seg = &cols;
            std::vector<std::vector<size_t>> grouped;
            if (p.grouped)
            {
                uint32_t groups = 0;
                const std::vector<uint32_t> group = get_groups(code, at, p.width, groups);
                grouped.resize(groups);
                for (size_t j = 0; j < listed.size(); ++j) grouped[group[size_t(listed[j] % p.width)]].push_back(j);
                seg = &grouped;
            }
            for (const std::vector<size_t>& one : *seg)
            {
                const std::vector<uint16_t> sym = get_segment(code, at, one.size(), true);
                for (size_t i = 0; i < one.size(); ++i) x[pos(one[i])] = V(uint64_t(x[pos(one[i])]) | uint64_t(sym[i]) << (8 * plane));
            }
        }
    }
    const uint64_t tail = finish(at);
    const std::vector<int64_t> rope = rope_of(x, pts);
    size_t k = 0;
    for (uint64_t i = 0; i < n; ++i)
    {
        if (k < pts.size() && pts[k] == i)
        {
            ++k;
            continue;
        }
        x[i] = V((uint64_t(x[i]) + predict(x, rope, i, p, mask)) & mask);
    }
    return output(at, tail);
}

std::vector<uint8_t> rope_file(std::span<const uint8_t> code, const RopeParams& p, uint64_t size)
{
    p.check();
    if (size >= (uint64_t(1) << 30)) bad("a file of a gigabyte or more");
    if (p.columns == 2 && p.above)
    {
        // v5: the pairs' groups, each group's bytes; then the file made again byte by byte, each
        // pair given the next group number the first time it is met.
        size_t at = 0;
        const uint64_t pairs = get_varint(code, at);
        if (pairs > size) bad("more pairs than the file has bytes");
        const std::vector<uint16_t> listed = get_segment(code, at, pairs, true);
        uint32_t groups = 0;
        for (const uint16_t g : listed)
        {
            if (g > groups || g >= 256) bad("a pair's group past the groups started before it");
            if (g == groups) ++groups;
        }
        std::vector<std::vector<uint16_t>> seg(groups);
        uint64_t total = 0;
        for (auto& one : seg)
        {
            const uint64_t n = get_varint(code, at);
            if (n > size - total) bad("groups holding more bytes than the file");
            total += n;
            one = get_segment(code, at, n, true);
        }
        if (total != size || at != code.size()) bad("the code does not end where the file's groups do");
        std::vector<uint8_t> out(static_cast<size_t>(size));
        std::vector<size_t> next(groups, 0);
        std::unordered_map<uint64_t, uint32_t> seen;
        size_t line = 0, prev = 0, prev_len = 0, met = 0;
        bool have_prev = false;
        for (size_t i = 0; i < out.size(); ++i)
        {
            const size_t c = i - line;
            const uint64_t col = std::min<uint64_t>(c, p.width - 1);
            const uint32_t up = have_prev && c < prev_len ? out[prev + c] : 256u;
            const uint64_t key = col * 257 + up;
            auto it = seen.find(key);
            if (it == seen.end())
            {
                if (met >= listed.size()) bad("a pair met that has no group");
                it = seen.emplace(key, listed[met++]).first;
            }
            const uint32_t k = it->second;
            if (next[k] >= seg[k].size()) bad("a group with fewer bytes than the file needs");
            out[i] = uint8_t(seg[k][next[k]++]);
            if (out[i] == 10)
            {
                prev = line;
                prev_len = i + 1 - line;
                have_prev = true;
                line = i + 1;
            }
        }
        if (met != listed.size()) bad("groups listed for pairs the file never meets");
        return out;
    }
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
    if (p.bits == 8) return file_values<uint8_t>(code, p, size);
    if (p.bits == 16) return file_values<uint16_t>(code, p, size);
    return file_values<uint32_t>(code, p, size);
}

// rope_estimate's values (no columns by line), what the wave leaves held E wide.
template <class E>
double estimate_values(std::span<const uint8_t> file, const RopeParams& p)
{
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<E> e = residuals<E>(v.x, p, pts);
    double bytes = double(pts.size() * v.bytes + (file.size() - v.x.size() * v.bytes));
    if (p.triangle)
    {
        std::vector<uint64_t> listed;
        if (!pts.empty()) listed = free_positions(v.x.size(), pts);
        std::vector<uint8_t> bucket(e.size());
        for (size_t j = 0; j < e.size(); ++j) bucket[j] = uint8_t(triangle_bucket(v.x, pts.empty() ? uint64_t(j) : listed[j], p));
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
        {
            std::vector<uint64_t> counts(size_t(kTriangleBuckets) * 256, 0), nb(kTriangleBuckets, 0);
            for (size_t j = 0; j < e.size(); ++j)
            {
                ++counts[size_t(bucket[j]) * 256 + ((uint64_t(e[j]) >> (8 * plane)) & 0xFF)];
                ++nb[bucket[j]];
            }
            for (uint32_t k = 0; k < kTriangleBuckets; ++k)
            {
                const std::vector<uint64_t> one(counts.begin() + std::ptrdiff_t(k) * 256, counts.begin() + std::ptrdiff_t(k + 1) * 256);
                bytes += 1 + (nb[k] ? entropy_bytes_v2(one, nb[k]) : 1);
            }
        }
        return bytes;
    }
    if (!p.columns)
    {
        for (uint32_t plane = 0; plane < v.bytes; ++plane)
        {
            std::vector<uint64_t> counts(256, 0);
            for (const E r : e) ++counts[(uint64_t(r) >> (8 * plane)) & 0xFF];
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
            ++counts[c * 256 + ((uint64_t(e[j]) >> (8 * plane)) & 0xFF)];
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

double rope_estimate(std::span<const uint8_t> file, const RopeParams& p)
{
    p.check();
    if (p.columns == 2 && p.above)
    {
        uint32_t pairs = 0;
        const std::vector<uint32_t> pair = above_pairs(file, p.width, pairs);
        if (!pairs) return 2;
        std::vector<uint64_t> counts(size_t(pairs) * 256, 0), n(pairs, 0);
        for (size_t i = 0; i < file.size(); ++i)
        {
            ++counts[size_t(pair[i]) * 256 + file[i]];
            ++n[pair[i]];
        }
        double bytes = 0;
        const std::vector<uint32_t> group = group_columns(counts, n, 3, bytes, false);
        return bytes + above_cost(group) + 3;
    }
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
    if (p.bits == 8) return estimate_values<uint8_t>(file, p);
    if (p.bits == 16) return estimate_values<uint16_t>(file, p);
    return estimate_values<uint32_t>(file, p);
}

namespace {

RopeSearch rope_search_uncached(std::span<const uint8_t> file, double seconds, uint32_t newest);

// Whether a search goes on through every wave where nothing so far helps (off: it stops there).
std::atomic<bool> g_every_wave{false};

// The searches done in this run, by the file's SHA-256 and the time given: the File Locator weighs
// a file and then makes its instructions, and both pull it tight; the second is the first's answer.
std::mutex g_cache_mx;
std::map<std::string, RopeSearch> g_cache;

// The searches' time since the last reset (under g_cache_mx).
RopeSearchSpent g_spent;

void count_search(double seconds, double took)
{
    std::lock_guard<std::mutex> lock(g_cache_mx);
    ++g_spent.searches;
    g_spent.took += took;
    if (seconds > 0) g_spent.given += seconds;
}

} // namespace

RopeSearchSpent rope_search_spent()
{
    std::lock_guard<std::mutex> lock(g_cache_mx);
    return g_spent;
}

void reset_rope_search_spent()
{
    std::lock_guard<std::mutex> lock(g_cache_mx);
    g_spent = {};
}

void set_rope_search_every_wave(bool every) { g_every_wave = every; }
bool rope_search_every_wave() { return g_every_wave.load(std::memory_order_relaxed); }

RopeSearch rope_search(std::span<const uint8_t> file, double seconds, uint32_t newest)
{
    if (newest < 1 || newest > 6) bad("the newest version searched is 1 to 6");
    char t[48];
    std::snprintf(t, sizeof t, "|%.6f|%u|%d", seconds, newest, int(rope_search_every_wave()));
    Sha256 h;
    h.update(file);
    const std::string key = Sha256::hex(h.finish()) + t;
    {
        std::lock_guard<std::mutex> lock(g_cache_mx);
        if (auto it = g_cache.find(key); it != g_cache.end())
        {
            const RopeSearch r = it->second;
            ++g_spent.searches;
            if (seconds > 0) g_spent.given += seconds;
            return r;
        }
    }
    const auto start = std::chrono::steady_clock::now();
    RopeSearch r = rope_search_uncached(file, seconds, newest);
    count_search(seconds, std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count());
    if (stop_requested()) return r; // a search cut short is not the answer to keep for next time
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
    // Out of time, or asked to stop (sieve/stop.hpp): either way the best found so far is kept.
    auto out_of_time = [&] { return stop_requested() || (seconds > 0 && std::chrono::duration<double>(steady::now() - start).count() >= seconds); };
    RopeSearch r;
    r.estimate = -1;
    auto try_one = [&](const RopeParams& p) {
        const double e = rope_estimate(file, p);
        ++r.tried;
        if (r.estimate < 0 || e < r.estimate)
        {
            r.estimate = e;
            r.best = p;
            r.best_at = std::chrono::duration<double>(steady::now() - start).count();
            r.best_trial = r.tried;
            char t[160];
            std::snprintf(t, sizeof t, "the rope's search: %llu ways tried, best so far %.0f bytes of %llu (%s)", (unsigned long long)r.tried, e,
                          (unsigned long long)file.size(), p.text().c_str());
            progress(t, e, double(file.size()));
        }
    };
    try_one(RopeParams{}); // no wave at all: the bytes' own histogram
    // First the wave of the value before, at each value length: quick, and often most of the gain,
    // so that a short search still finds it.
    for (const uint32_t bits : {8u, 16u, 32u})
        if (file.size() / (bits / 8) >= 2 && !out_of_time()) try_one(RopeParams{bits, 1, 1, 2, false});
    // A text's own way: columns counted from each line's start (rope-wave-v3 to v5). For a file that
    // is text (at least 95% of its bytes printable, white space or UTF-8, in lines of at most 1,024)
    // it is tried early (24 October
    // 2026: on every text measured it was the best, and as the last tried a short search never reached
    // it; IDEAS §19.99); for any other, last, as before (on a mostly binary file each of these waves is
    // long, and one under way is not cut short by the time).
    uint64_t longest = 0, text_bytes = 0, shortest = UINT64_MAX;
    bool lines = false;
    {
        uint64_t c = 0;
        for (const uint8_t b : file)
        {
            text_bytes += (b >= 32 && b != 127) || b == 9 || b == 10 || b == 13;
            if (b == 10)
            {
                lines = true;
                longest = std::max(longest, c + 1);
                shortest = std::min(shortest, c + 1);
                c = 0;
            }
            else ++c;
        }
        longest = std::max(longest, c + 1);
    }
    auto try_lines = [&] {
        // rope-wave-v3: columns counted from each line's start, for a file of lines (text, tables);
        // as many columns as its longest line, up to 4,096.
        if (newest >= 3 && !out_of_time())
        {
            if (lines && longest >= 2)
            {
                const uint32_t w = uint32_t(std::min<uint64_t>(longest, 4096));
                try_one(RopeParams{8, w, 1, 0, false, 2});
                if (newest >= 4 && !out_of_time()) try_one(RopeParams{8, w, 1, 0, false, 2, true}); // rope-wave-v4
                if (newest >= 5 && !out_of_time()) try_one(RopeParams{8, w, 1, 0, false, 2, true, true}); // rope-wave-v5
            }
        }
    };
    // (And lines of at most 1,024 bytes: the texts measured had 59 to 452; a tar of texts and
    // pictures, at 4,096, took 17 s for one of these waves.)
    const bool textlike = !file.empty() && double(text_bytes) >= 0.95 * double(file.size()) && longest <= 1024;
    if (textlike)
    {
        try_lines();
        // Lines all of one length (a table of fixed width): its columns counted apart too, at that
        // width (rope-wave-v2 and v4), where they can be a little better than by line.
        if (lines && shortest == longest && longest >= 2 && longest <= file.size() / 4)
            for (const bool grouped : {false, true})
                if (newest >= (grouped ? 4u : 2u) && !out_of_time()) try_one(RopeParams{8, uint32_t(longest), 1, 0, false, 1, grouped});
        // Then a text's search ends: on 118 texts measured (24 October 2026: this repository's docs,
        // code, tables, word lists, meshes and pi's digits) the best was always one tried by now, or
        // within 5 bytes of it; the waves after found nothing better. --search-every-wave goes on.
        if (!g_every_wave.load(std::memory_order_relaxed)) goto keep;
    }
    // Bytes that nothing so far makes even 2% smaller (a JPEG, a PNG, a film: already packed) are
    // not searched further: no wave has ever helped them, and the search would take its whole time
    // for nothing. rope_search_every_wave(true) searches on regardless.
    if (!g_every_wave.load(std::memory_order_relaxed) && r.estimate >= 0.98 * double(file.size())) goto keep;
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
        // rope-wave-v6: the triangle (P4) at each width and step, and the triangle's contexts (T1)
        // with it and with the plane (P3).
        for (const uint32_t w : widths)
            for (uint32_t step = 1; step <= 4 && w > 1 && newest >= 6; ++step)
            {
                if (step >= w) continue;
                for (const auto& [phases, tri] : {std::pair<uint32_t, bool>{4, false}, {4, true}, {3, true}})
                {
                    if (out_of_time())
                    {
                        r.timed_out = true;
                        goto keep;
                    }
                    RopeParams t{bits, w, step, phases, false};
                    t.triangle = tri;
                    try_one(t);
                }
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
    if (!textlike) try_lines();
keep:
    if (!out_of_time())
    {
        RopeParams k = r.best;
        k.keep = true;
        if (k.columns != 2) try_one(k); // columns by line keep nothing
    }
    else r.timed_out = true;
    r.took = std::chrono::duration<double>(steady::now() - start).count();
    return r;
}

} // namespace

} // namespace sieve
