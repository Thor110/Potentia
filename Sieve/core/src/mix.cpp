// Sieve — mixing, mix-v1 (see mix.hpp). Integers only, so every machine makes the same code.
#include "sieve/mix.hpp"

#include <cstdio>
#include <memory>
#include <stdexcept>

namespace sieve {

namespace {

[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument("mix-v1: " + why); }

// squash(x) = 4096 / (1 + e^(-x / 256)) for x in -2047 .. 2047, from 33 points joined by straight
// lines (as lpaq1 has it), and stretch, its inverse, as the least x whose squash reaches p.
struct Tables
{
    int squash[4096];
    int stretch[4096];
    Tables()
    {
        static const int ts[33] = {1,    2,    3,    6,    10,   16,   27,   45,   73,   120,  194,  310,  488,  747,  1101, 1546, 2047,
                                   2549, 2994, 3348, 3607, 3785, 3901, 3975, 4024, 4050, 4068, 4079, 4085, 4089, 4092, 4093, 4094};
        for (int x = -2048; x < 2048; ++x)
        {
            const int w = x & 127, i = (x >> 7) + 16;
            squash[x + 2048] = (ts[i] * (128 - w) + ts[i + 1] * w + 64) >> 7;
        }
        int pi = 0;
        for (int x = -2047; x <= 2047; ++x)
        {
            const int v = squash[x + 2048];
            for (int j = pi; j <= v; ++j) stretch[j] = x;
            pi = v + 1;
        }
        for (int j = pi; j < 4096; ++j) stretch[j] = 2047;
    }
    int sq(int x) const { return squash[(x > 2047 ? 2047 : x < -2047 ? -2047 : x) + 2048]; }
};

const Tables& tables()
{
    static const Tables t;
    return t;
}

constexpr int kOrders[6] = {1, 2, 3, 4, 5, 6};
constexpr size_t kInputs = 8; // order 0, orders 1 to 6, the word
constexpr int kRate = 2;      // the mixer's learning rate, in 2^-12
constexpr int kWeightMax = 1 << 22; // a weight's bound, 64 in 16.16

struct Counter
{
    uint16_t p = 32768;
    uint8_t n = 0;
};

// The models, the mixer and the bytes seen: the same steps coding and decoding.
class Model
{
public:
    explicit Model(uint32_t bits) : mask_((uint32_t(1) << bits) - 1), t_(new Counter[kInputs << bits]), w_(kInputs * 256, 1 << 14)
    {
        new_byte();
    }
    int p() // the probability (12 bits) that the next bit is 1
    {
        const Tables& T = tables();
        int64_t dot = 0;
        const int* ws = &w_[size_t(c0_) * kInputs];
        for (size_t i = 0; i < kInputs; ++i)
        {
            at_[i] = (size_t(i) << shift()) + ((base_[i] + uint32_t(c0_) * 0x9E3779B1u) & mask_);
            st_[i] = T.stretch[t_[at_[i]].p >> 4];
            dot += int64_t(st_[i]) * ws[i];
        }
        pm_ = T.sq(int(dot >> 16));
        return pm_ < 1 ? 1 : pm_ > 4095 ? 4095 : pm_;
    }
    void update(int bit)
    {
        const int err = ((bit << 12) - pm_) * kRate;
        int* ws = &w_[size_t(c0_) * kInputs];
        for (size_t i = 0; i < kInputs; ++i)
        {
            // (Held within +-64, so no weight can ever overflow, however long the file.)
            const int nw = ws[i] + ((st_[i] * err) >> 12);
            ws[i] = nw > kWeightMax ? kWeightMax : nw < -kWeightMax ? -kWeightMax : nw;
            Counter& c = t_[at_[i]];
            const int target = bit ? 65535 : 0;
            c.p = uint16_t(int(c.p) + (target - int(c.p)) / (int(c.n) + 2));
            if (c.n < 255) ++c.n;
        }
        c0_ = c0_ * 2 + bit;
        if (c0_ >= 256)
        {
            const uint8_t b = uint8_t(c0_ - 256);
            for (int k = 5; k > 0; --k) last_[k] = last_[k - 1];
            last_[0] = b;
            ++seen_;
            const bool letter = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
            word_ = letter ? word_ * 263 + uint32_t(b | 32) : 0;
            c0_ = 1;
            new_byte();
        }
    }

private:
    uint32_t shift() const
    {
        uint32_t s = 0;
        while ((uint32_t(1) << s) <= mask_) ++s;
        return s;
    }
    void new_byte()
    {
        base_[0] = 0;
        for (size_t k = 0; k < 6; ++k)
        {
            const int o = kOrders[k];
            uint32_t h = uint32_t(o) * 0x2545F491u;
            for (int j = 0; j < o && uint64_t(j) < seen_; ++j) h = (h ^ last_[j]) * 0x01000193u;
            base_[k + 1] = h * 0x9E3779B1u + uint32_t(o) * 0x632BE5ABu;
        }
        base_[7] = word_ * 0x85EBCA6Bu + 0x7FEB352Du;
    }
    uint32_t mask_;
    std::unique_ptr<Counter[]> t_;
    std::vector<int> w_;
    uint32_t base_[kInputs] = {};
    size_t at_[kInputs] = {};
    int st_[kInputs] = {};
    int c0_ = 1, pm_ = 2048;
    uint8_t last_[6] = {};
    uint64_t seen_ = 0;
    uint32_t word_ = 0;
};

} // namespace

void MixParams::check() const
{
    if (table_bits < 16 || table_bits > 22) bad("tables of 2^16 to 2^22 counters (B16 to B22)");
}

std::string MixParams::text() const { return "B" + std::to_string(table_bits); }

MixParams MixParams::parse(std::string_view t)
{
    MixParams p;
    unsigned b = 0;
    char extra = 0;
    const std::string s(t);
    if (std::sscanf(s.c_str(), "B%u%c", &b, &extra) != 1) bad("its parameters are written \"B22\": " + s);
    p.table_bits = b;
    p.check();
    if (p.text() != s) bad("parameters not written as they would be: " + s);
    return p;
}

MixParams MixParams::for_size(uint64_t bytes)
{
    MixParams p;
    p.table_bits = 16;
    while (p.table_bits < 22 && (uint64_t(1) << p.table_bits) < 2 * bytes) ++p.table_bits;
    return p;
}

std::vector<uint8_t> mix_code(std::span<const uint8_t> file, const MixParams& p)
{
    p.check();
    Model m(p.table_bits);
    std::vector<uint8_t> out;
    out.reserve(file.size() / 3 + 16);
    uint32_t x1 = 0, x2 = 0xFFFFFFFFu;
    for (const uint8_t b : file)
        for (int j = 7; j >= 0; --j)
        {
            const int bit = (b >> j) & 1;
            const uint32_t xmid = x1 + uint32_t((uint64_t(x2 - x1) * uint32_t(m.p())) >> 12);
            bit ? (x2 = xmid) : (x1 = xmid + 1);
            m.update(bit);
            while (((x1 ^ x2) & 0xFF000000u) == 0)
            {
                out.push_back(uint8_t(x2 >> 24));
                x1 <<= 8;
                x2 = (x2 << 8) | 255;
            }
        }
    // The end: one byte, the least whose value (followed by zeros, as a reader reads past the
    // code) lies within the range left.
    out.push_back(uint8_t((x1 >> 24) + 1));
    return out;
}

std::vector<uint8_t> mix_file(std::span<const uint8_t> code, const MixParams& p, uint64_t size)
{
    p.check();
    if (size >= (uint64_t(1) << 32)) bad("a file of 4 GB or more");
    Model m(p.table_bits);
    size_t at = 0;
    auto next = [&]() -> uint32_t { return at < code.size() ? code[at++] : (++at, 0u); };
    uint32_t x1 = 0, x2 = 0xFFFFFFFFu, x = 0;
    for (int i = 0; i < 4; ++i) x = (x << 8) | next();
    std::vector<uint8_t> out(static_cast<size_t>(size));
    for (size_t i = 0; i < out.size(); ++i)
    {
        int c = 0;
        for (int j = 0; j < 8; ++j)
        {
            const uint32_t xmid = x1 + uint32_t((uint64_t(x2 - x1) * uint32_t(m.p())) >> 12);
            const int bit = x <= xmid;
            bit ? (x2 = xmid) : (x1 = xmid + 1);
            m.update(bit);
            c = c * 2 + bit;
            while (((x1 ^ x2) & 0xFF000000u) == 0)
            {
                x1 <<= 8;
                x2 = (x2 << 8) | 255;
                x = (x << 8) | next();
            }
        }
        out[i] = uint8_t(c);
        if (at > code.size() + 4) bad("a code that ends before the file does");
    }
    // Every byte of the code read: the reader takes four ahead, so it has gone at most four past it.
    if (at < code.size()) bad("a code with bytes past the file's end");
    return out;
}

} // namespace sieve
