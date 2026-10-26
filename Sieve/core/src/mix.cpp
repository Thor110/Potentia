// Sieve — mixing, mix-v1 (see mix.hpp). Integers only, so every machine makes the same code.
#include "sieve/mix.hpp"

#include <algorithm>
#include <cstdio>
#include <memory>
#include <stdexcept>

namespace sieve {

namespace {

[[noreturn]] void bad(const std::string& why) { throw std::invalid_argument("mix-v1: " + why); }
[[noreturn]] void bad2(const std::string& why) { throw std::invalid_argument("mix-v2: " + why); }

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

// ---------------------------------------------------------------------------------------------
// mix-v2 (see mix.hpp, SPECIFICATIONS §12.2).

namespace {

constexpr int kLimit2 = 60;       // a counter's count stops here: it never settles slower than 1/62
constexpr int kMinMatch = 6;      // the match model's hash: the last six bytes
constexpr uint32_t kMatchBits = 22; // its table: 2^22 places
constexpr size_t kMaxContexts = 16;

uint32_t hash2(uint32_t a, uint32_t b) { return ((a * 0x9E3779B1u) ^ (b * 0x85EBCA6Bu) ^ (a >> 15)) * 0xC2B2AE35u + b; }

// An SSE stage: in each context, 33 probabilities (16 bits) at the stretched probability's 33
// points, read between the two nearest and both moved 1/128 of the way to each bit.
class Apm
{
public:
    explicit Apm(size_t contexts) : t_(contexts * 33)
    {
        const Tables& T = tables();
        for (size_t i = 0; i < contexts; ++i)
            for (int j = 0; j < 33; ++j) t_[i * 33 + size_t(j)] = uint16_t(T.sq((j - 16) * 128) * 16);
    }
    int p(int pr, size_t cx)
    {
        const int s = tables().stretch[pr] + 2048, w = s & 127;
        at_ = cx * 33 + size_t(s >> 7);
        return (t_[at_] * (128 - w) + t_[at_ + 1] * w) >> 11;
    }
    void update(int bit)
    {
        const int target = bit ? 65535 : 0;
        t_[at_] = uint16_t(t_[at_] + ((target - t_[at_]) >> 7));
        t_[at_ + 1] = uint16_t(t_[at_ + 1] + ((target - t_[at_ + 1]) >> 7));
    }

private:
    std::vector<uint16_t> t_;
    size_t at_ = 0;
};

struct Counter2
{
    uint16_t p = 32768;
    uint8_t n = 0, unused = 0;
};

class Model2
{
public:
    Model2(const Mix2Params& p, uint64_t size)
        : picture_(p.width > 0), bits_(p.table_bits), mask_((uint32_t(1) << p.table_bits) - 1), bpp_(p.channels),
          row_(size_t(p.width) * p.channels), buf_(static_cast<size_t>(size)), ht_(size_t(1) << kMatchBits, 0), a1_(65536), a2_(65536)
    {
        // The contexts: order 0, orders 1 to 6 (a picture: 1 to 3), the word, then the column and
        // sparse contexts (a picture: the triangle's six), then the byte the match expects.
        orders_ = picture_ ? 3 : 6;
        nctx_ = 1 + size_t(orders_) + 1 + 6 + 1;
        ninputs_ = nctx_ + 3;
        t_.reset(new Counter2[nctx_ << bits_]);
        w1_.assign(ninputs_ * 256, 1 << 14);
        w2_.assign(ninputs_ * 2048, 1 << 14);
        new_byte();
    }
    int p()
    {
        const Tables& T = tables();
        if (bitpos_ == 0 || bitpos_ == 4)
            for (size_t i = 0; i < nctx_; ++i) slot_[i] = (i << bits_) + (hash2(base_[i], uint32_t(c0_)) & mask_ & ~uint32_t(15));
        const int nib = bitpos_ < 4 ? c0_ : (1 << (bitpos_ - 4)) | (c0_ & ((1 << (bitpos_ - 4)) - 1));
        size_t k = 0;
        for (size_t i = 0; i < nctx_; ++i)
        {
            at_[i] = slot_[i] + size_t(nib);
            st_[k++] = T.stretch[t_[at_[i]].p >> 4];
        }
        // The match: the bit the byte it expects has here, while the bits so far agree with it.
        mi_ = -1;
        if (mlen_ > 0)
        {
            const int eb = buf_[mptr_];
            if (((eb + 256) >> (8 - bitpos_)) == c0_) mi_ = int(std::min<uint32_t>(mlen_, 31)) * 2 + ((eb >> (7 - bitpos_)) & 1);
            else mlen_ = 0;
        }
        if (mi_ >= 0)
        {
            const int strength = int(std::min<uint32_t>(mlen_, 16)) * 64;
            st_[k++] = T.stretch[mm_[mi_].p >> 4];
            st_[k++] = (mi_ & 1) ? strength : -strength;
        }
        else st_[k++] = 0, st_[k++] = 0;
        st_[k++] = 256;
        const int ml = mlen_ == 0 ? 0 : mlen_ < 16 ? 1 : mlen_ < 32 ? 2 : 3;
        set2_ = size_t((ml * 8 + bitpos_) * 64 + (c1() >> 2));
        s1_ = dot(&w1_[size_t(c0_) * ninputs_]);
        s2_ = dot(&w2_[set2_ * ninputs_]);
        int pr = T.sq((s1_ + s2_) >> 1);
        const int q1 = a1_.p(pr, size_t(c0_) | size_t(c1()) << 8);
        const int q2 = a2_.p(pr, (hash2(uint32_t(c1() | c2() << 8), uint32_t(c0_)) >> 16) & 0xFFFF);
        pr = (2 * pr + q1 + q2 + 2) >> 2;
        return pr < 1 ? 1 : pr > 4095 ? 4095 : pr;
    }
    void update(int bit)
    {
        const Tables& T = tables();
        train(&w1_[size_t(c0_) * ninputs_], ((bit << 12) - T.sq(s1_)) * kRate);
        train(&w2_[set2_ * ninputs_], ((bit << 12) - T.sq(s2_)) * kRate);
        const int target = bit ? 65535 : 0;
        for (size_t i = 0; i < nctx_; ++i)
        {
            Counter2& c = t_[at_[i]];
            c.p = uint16_t(int(c.p) + (target - int(c.p)) / (int(c.n) + 2));
            if (c.n < kLimit2) ++c.n;
        }
        if (mi_ >= 0)
        {
            Counter& c = mm_[mi_];
            c.p = uint16_t(int(c.p) + (target - int(c.p)) / (int(c.n) + 2));
            if (c.n < 255) ++c.n;
        }
        a1_.update(bit);
        a2_.update(bit);
        c0_ = c0_ * 2 + bit;
        if (++bitpos_ == 8)
        {
            buf_[pos_++] = uint8_t(c0_ - 256);
            c0_ = 1;
            bitpos_ = 0;
            new_byte();
        }
    }

private:
    int dot(const int* w) const
    {
        int64_t d = 0;
        for (size_t i = 0; i < ninputs_; ++i) d += int64_t(st_[i]) * w[i];
        const int64_t s = d >> 16;
        return int(s > 2047 ? 2047 : s < -2047 ? -2047 : s);
    }
    void train(int* w, int err)
    {
        for (size_t i = 0; i < ninputs_; ++i)
        {
            const int nw = w[i] + ((st_[i] * err) >> 12);
            w[i] = nw > kWeightMax ? kWeightMax : nw < -kWeightMax ? -kWeightMax : nw;
        }
    }
    int at(size_t back) const { return pos_ >= back ? buf_[pos_ - back] : 0; }
    int c1() const { return at(1); }
    int c2() const { return at(2); }
    void new_byte()
    {
        if (pos_ > 0)
        {
            const uint8_t b = buf_[pos_ - 1];
            const bool letter = (b >= 'a' && b <= 'z') || (b >= 'A' && b <= 'Z');
            word_ = letter ? word_ * 263 + uint32_t(b | 32) : 0;
            if (b == '\n') prevline_ = linestart_, linestart_ = pos_, field_ = 0;
            if (b == '\t' || b == ',') ++field_;
            // The match goes on where the byte was the one it expected.
            if (mlen_ > 0 && buf_[mptr_] == b) ++mlen_, ++mptr_;
            else mlen_ = 0;
        }
        size_t k = 0;
        base_[k++] = 0;
        for (int o = 1; o <= orders_; ++o)
        {
            uint32_t h = uint32_t(o) * 0x2545F491u;
            for (int j = 1; j <= o; ++j) h = (h ^ uint32_t(at(size_t(j)))) * 0x01000193u;
            base_[k++] = h * 0x9E3779B1u + uint32_t(o) * 0x632BE5ABu;
        }
        base_[k++] = word_ * 0x85EBCA6Bu + 0x7FEB352Du;
        if (picture_)
        {
            const size_t bp = bpp_, row = row_;
            const int W = at(bp), N = at(row), NW = at(row + bp), NE = at(row - bp), WW = at(2 * bp), NN = at(2 * row);
            const int mx = std::max(W, N), mn = std::min(W, N);
            const int med = NW >= mx ? mn : NW <= mn ? mx : W + N - NW;
            const int grad = std::clamp(W + N - NW, 0, 255);
            const uint32_t ch = uint32_t(pos_ % bp);
            base_[k++] = hash2(0x3010 + ch, uint32_t(med));
            base_[k++] = hash2(0x3020 + ch, uint32_t(W) | uint32_t(N) << 8);
            base_[k++] = hash2(0x3030 + ch, uint32_t(grad) | uint32_t(((W - NW) >> 3) + 64) << 8);
            base_[k++] = hash2(0x3040 + ch, uint32_t(N) | uint32_t(NE) << 8);
            base_[k++] = hash2(0x3050 + ch, uint32_t((W + N + 1) >> 1) | uint32_t(((W - WW) >> 2) + 128) << 8 | uint32_t(((N - NN) >> 2) + 128) << 16);
            base_[k++] = hash2(0x3060 + ch, uint32_t(med) | uint32_t(bp > 1 ? at(1) : 0) << 8);
        }
        else
        {
            const size_t col = pos_ - linestart_;
            const int above = prevline_ + col < linestart_ ? buf_[prevline_ + col] : 0;
            const int above1 = prevline_ + col + 1 < linestart_ ? buf_[prevline_ + col + 1] : 0;
            base_[k++] = hash2(0x1001, uint32_t(above) | uint32_t(c1()) << 8);
            base_[k++] = hash2(0x1002, uint32_t(above) | uint32_t(above1) << 8 | uint32_t(std::min<size_t>(col, 255)) << 16);
            base_[k++] = hash2(0x1003, std::min<uint32_t>(field_, 65535) | uint32_t(c1()) << 16 | uint32_t(c2()) << 24);
            base_[k++] = hash2(0x2001, uint32_t(at(2)) | uint32_t(at(3)) << 8);
            base_[k++] = hash2(0x2002, uint32_t(at(4)) | uint32_t(at(3)) << 8);
            base_[k++] = hash2(0x2003, uint32_t(at(1)) | uint32_t(at(3)) << 8);
        }
        // A new match where there is none: the place the last six bytes were last seen, if the
        // bytes before it agree for six or more (up to 32 counted).
        if (pos_ >= size_t(kMinMatch))
        {
            uint32_t h = 0;
            for (int j = 1; j <= kMinMatch; ++j) h = (h * 0x2F0F3C35u) ^ uint32_t(at(size_t(j))) * 0x9E3779B1u;
            h = (h >> 10) & ((uint32_t(1) << kMatchBits) - 1);
            if (mlen_ == 0 && ht_[h] > 0)
            {
                const size_t cand = ht_[h];
                uint32_t l = 0;
                while (l < 32 && l < cand && buf_[cand - 1 - l] == buf_[pos_ - 1 - l]) ++l;
                if (l >= uint32_t(kMinMatch)) mlen_ = l, mptr_ = cand;
            }
            ht_[h] = uint32_t(pos_);
        }
        base_[k++] = hash2(0x4001, mlen_ > 0 ? uint32_t(buf_[mptr_]) | 256u | std::min<uint32_t>(mlen_, 15) << 9 : 0u);
    }

    bool picture_;
    uint32_t bits_, mask_;
    size_t bpp_, row_;
    int orders_ = 6;
    size_t nctx_ = 0, ninputs_ = 0;
    std::vector<uint8_t> buf_;
    size_t pos_ = 0, linestart_ = 0, prevline_ = 0;
    uint32_t field_ = 0, word_ = 0;
    std::unique_ptr<Counter2[]> t_;
    uint32_t base_[kMaxContexts] = {};
    size_t slot_[kMaxContexts] = {}, at_[kMaxContexts] = {};
    int st_[kMaxContexts + 3] = {};
    std::vector<int> w1_, w2_;
    Counter mm_[64];
    std::vector<uint32_t> ht_;
    uint32_t mlen_ = 0;
    size_t mptr_ = 0;
    int mi_ = -1;
    Apm a1_, a2_;
    int c0_ = 1, bitpos_ = 0, s1_ = 0, s2_ = 0;
    size_t set2_ = 0;
};

} // namespace

void Mix2Params::check() const
{
    if (table_bits < 16 || table_bits > 22) bad2("tables of 2^16 to 2^22 counters (B16 to B22)");
    if (width == 0 ? channels != 1 : (width < 2 || width > (uint32_t(1) << 24) || channels < 1 || channels > 4))
        bad2("a picture 2 to 16,777,216 pixels wide, of 1 to 4 bytes a pixel (I949x1)");
}

std::string Mix2Params::text() const
{
    std::string t = "B" + std::to_string(table_bits);
    if (width) t += " I" + std::to_string(width) + "x" + std::to_string(channels);
    return t;
}

Mix2Params Mix2Params::parse(std::string_view t)
{
    Mix2Params p;
    unsigned b = 0, w = 0, c = 0;
    char extra = 0;
    const std::string s(t);
    const int n = std::sscanf(s.c_str(), "B%u I%ux%u%c", &b, &w, &c, &extra);
    if (n != 1 && n != 3) bad2("its parameters are written \"B22\", or a picture's \"B22 I949x1\": " + s);
    p.table_bits = b;
    if (n == 3) p.width = w, p.channels = c;
    p.check();
    if (p.text() != s) bad2("parameters not written as they would be: " + s);
    return p;
}

Mix2Params Mix2Params::for_size(uint64_t bytes, uint32_t width, uint32_t channels)
{
    Mix2Params p;
    p.table_bits = MixParams::for_size(bytes).table_bits;
    p.width = width;
    p.channels = width ? channels : 1;
    p.check();
    return p;
}

std::vector<uint8_t> mix2_code(std::span<const uint8_t> file, const Mix2Params& p)
{
    p.check();
    if (file.size() >= (uint64_t(1) << 32)) bad2("a file of 4 GB or more");
    Model2 m(p, file.size());
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
    out.push_back(uint8_t((x1 >> 24) + 1));
    return out;
}

std::vector<uint8_t> mix2_file(std::span<const uint8_t> code, const Mix2Params& p, uint64_t size)
{
    p.check();
    if (size >= (uint64_t(1) << 32)) bad2("a file of 4 GB or more");
    Model2 m(p, size);
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
        if (at > code.size() + 4) bad2("a code that ends before the file does");
    }
    if (at < code.size()) bad2("a code with bytes past the file's end");
    return out;
}

} // namespace sieve
