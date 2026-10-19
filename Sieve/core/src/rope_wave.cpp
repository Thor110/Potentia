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

namespace sieve {

namespace {

[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument("rope-wave-v1: " + why); }

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

} // namespace

void RopeParams::check() const
{
    if (bits != 8 && bits != 16 && bits != 32) bad("values of 8, 16 or 32 bits");
    if (width < 1) bad("a line of at least one value");
    if (step < 1 || step > 8) bad("a step of 1 to 8 values");
    if (phases > 3) bad("phases 0 to 3");
}

std::string RopeParams::text() const
{
    return "L" + std::to_string(bits) + " W" + std::to_string(width) + " S" + std::to_string(step) + " P" + std::to_string(phases) + " K" +
           (keep ? "1" : "0");
}

RopeParams RopeParams::parse(std::string_view t)
{
    RopeParams p;
    unsigned long long L = 0, W = 0, S = 0, P = 0, K = 0;
    char extra = 0;
    const std::string s(t);
    if (std::sscanf(s.c_str(), "L%llu W%llu S%llu P%llu K%llu%c", &L, &W, &S, &P, &K, &extra) != 5 || K > 1 || W > 0xFFFFFFFFull ||
        L > 64 || S > 8 || P > 3)
        bad("parameters are written \"L8 W949 S1 P3 K0\": " + s);
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
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<uint64_t> e = residuals(v.x, p, pts);
    std::vector<uint8_t> out;
    for (uint32_t plane = 0; plane < v.bytes; ++plane)
    {
        std::vector<uint64_t> counts(65536, 0);
        std::vector<uint16_t> sym(e.size());
        for (size_t i = 0; i < e.size(); ++i)
        {
            sym[i] = uint16_t((e[i] >> (8 * plane)) & 0xFF);
            ++counts[sym[i]];
        }
        if (e.empty())
        {
            put_varint(out, 0);
            continue;
        }
        const std::vector<uint32_t> freq = weights::quantise(counts);
        uint64_t k = 0;
        for (const uint32_t f : freq) k += f != 0;
        put_varint(out, k);
        uint32_t last = 0;
        bool first = true;
        for (uint32_t s = 0; s < freq.size(); ++s)
            if (freq[s])
            {
                put_varint(out, first ? s : s - last);
                put_varint(out, freq[s]);
                last = s;
                first = false;
            }
        const std::vector<uint32_t> words = weights::encode(sym, freq);
        put_varint(out, words.size());
        for (const uint32_t w : words)
            for (int b = 0; b < 4; ++b) out.push_back(uint8_t(w >> (8 * b)));
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
    const uint32_t vb = p.bits / 8;
    const uint64_t n = size / vb;
    const std::vector<uint64_t> pts = p.keep ? kept_points(n) : std::vector<uint64_t>{};
    const uint64_t m = n - pts.size();
    size_t at = 0;
    std::vector<uint64_t> e(m, 0);
    for (uint32_t plane = 0; plane < vb; ++plane)
    {
        const uint64_t k = get_varint(code, at);
        if (k == 0)
        {
            if (m) bad("an empty plane for values that are there");
            continue;
        }
        if (k > 256 || !m) bad("a plane's table out of range");
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
        const uint64_t w = get_varint(code, at);
        if (w < 2 || w > (code.size() - at) / 4) bad("a plane's stream longer than the code");
        std::vector<uint32_t> words(w);
        for (uint64_t j = 0; j < w; ++j, at += 4)
            words[j] = uint32_t(code[at]) | uint32_t(code[at + 1]) << 8 | uint32_t(code[at + 2]) << 16 | uint32_t(code[at + 3]) << 24;
        const std::vector<uint16_t> sym = weights::decode(words, freq, m);
        for (uint64_t i = 0; i < m; ++i) e[i] |= uint64_t(sym[i]) << (8 * plane);
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
    const Values v = values_of(file, p.bits);
    std::vector<uint64_t> pts;
    const std::vector<uint64_t> e = residuals(v.x, p, pts);
    double bytes = double(pts.size() * v.bytes + (file.size() - v.x.size() * v.bytes));
    for (uint32_t plane = 0; plane < v.bytes; ++plane)
    {
        std::vector<uint64_t> counts(256, 0);
        for (const uint64_t r : e) ++counts[(r >> (8 * plane)) & 0xFF];
        bytes += entropy_bytes(counts, e.size());
    }
    return bytes;
}

namespace {

RopeSearch rope_search_uncached(std::span<const uint8_t> file, double seconds);

// The searches done in this run, by the file's SHA-256 and the time given: the File Locator weighs
// a file and then makes its instructions, and both pull it tight; the second is the first's answer.
std::mutex g_cache_mx;
std::map<std::string, RopeSearch> g_cache;

} // namespace

RopeSearch rope_search(std::span<const uint8_t> file, double seconds)
{
    char t[40];
    std::snprintf(t, sizeof t, "|%.6f", seconds);
    Sha256 h;
    h.update(file);
    const std::string key = Sha256::hex(h.finish()) + t;
    {
        std::lock_guard<std::mutex> lock(g_cache_mx);
        if (auto it = g_cache.find(key); it != g_cache.end()) return it->second;
    }
    RopeSearch r = rope_search_uncached(file, seconds);
    std::lock_guard<std::mutex> lock(g_cache_mx);
    if (g_cache.size() >= 4096) g_cache.clear();
    g_cache.emplace(key, r);
    return r;
}

namespace {

RopeSearch rope_search_uncached(std::span<const uint8_t> file, double seconds)
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
        std::sort(lags.begin(), lags.end());
        std::vector<uint32_t> widths{1};
        for (size_t i = 0; i < lags.size() && widths.size() < 4; ++i) widths.push_back(lags[i].second);
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
    }
keep:
    if (!out_of_time())
    {
        RopeParams k = r.best;
        k.keep = true;
        try_one(k);
    }
    else r.timed_out = true;
    return r;
}

} // namespace

} // namespace sieve
