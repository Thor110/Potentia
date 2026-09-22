// Filters for pictures: neighbour-agreement (image and video).

#include "sieve/filter.hpp"

#include <algorithm>
#include <memory>

namespace sieve {

namespace {

// Pure noise has no spatial or temporal structure: neighbouring pixels agree only as often as
// chance allows (half the time in black and white). Counts pairs of neighbours (left-right and
// up-down within a frame, and the same pixel in consecutive frames) that share a colour.
//   passes iff  1000 * equal >= min_permille * pairs   (a unit with no pairs passes)
// The survivors of neighbour-agreement, counted and ranked exactly by a transfer matrix.
//
// Cells are visited in address order: pixel x, row y, frame f is cell i = f*W*H + y*W + x. When a
// cell is placed it is compared with the neighbours already placed: left (x > 0: cell i-1), up
// (y > 0: cell i-W) and the previous frame (f > 0: cell i-W*H). A unit passes when its number of
// disagreeing pairs is at most D = pairs - ceil(min * pairs / 1000). So the walk state is
//     (next cell i, disagreements so far d, profile p = the last P cells),
// with P = W for pictures and P = W*H for video (the previous frame must be remembered), and
//     T[i][p][b] = number of ways to fill cells i.. with at most b more disagreements
// is built backwards from T[n][p][b] = 1. The table has n * B^P * (D+1) entries: exponential in
// the width (for video, the frame), which is inherent to counting pictures by their neighbours
// exactly. Budgets above what the remaining cells could still spend are clamped, and entries are
// stored as fixed-width limbs per cell. Above kMaxLimbs (1 GiB) no ranker is built: a hardware
// limit, raised as machines grow.
class AgreementRanker : public Ranker
{
public:
    static constexpr uint64_t kMaxLimbs = uint64_t(1) << 28;
    static constexpr uint64_t kMaxProfiles = uint64_t(1) << 24;

    // Returns null when the table would not fit.
    static std::unique_ptr<AgreementRanker> make(uint32_t base, uint32_t w, uint32_t h, uint32_t frames, uint64_t pairs, uint64_t max_dis)
    {
        const uint64_t n = uint64_t(w) * h * frames;
        const uint32_t P = frames > 1 ? w * h : w;
        uint64_t BP = 1;
        for (uint32_t k = 0; k < P; ++k)
        {
            BP *= base;
            if (BP > kMaxProfiles) return nullptr;
        }
        if (n > 0xFFFFFFu) return nullptr;
        auto r = std::unique_ptr<AgreementRanker>(new AgreementRanker(base, w, h, frames, P, BP, pairs, max_dis));
        if (!r->plan()) return nullptr;
        r->build();
        r->set_count();
        return r;
    }

    uint32_t length() const override { return uint32_t(n_); }
    uint32_t base() const override { return B_; }
    State start() const override { return pack(0, 0, 0); }
    State next(State s, uint32_t c) const override
    {
        if (c >= B_) return kDead;
        const auto [i, d, p] = unpack(s);
        if (i >= n_) return kDead;
        const uint64_t nd = d + cost(i, p, c);
        if (nd > D_) return kDead;
        return pack(i + 1, nd, (p * B_ + c) % BP_);
    }
    BigUint completions(State s, uint32_t) const override
    {
        const auto [i, d, p] = unpack(s);
        const uint32_t* e = entry(i, p, D_ - d);
        BigUint v;
        for (uint32_t k = width_[size_t(i)]; k-- > 0;)
        {
            v <<= 32;
            v.add_small(e[k]);
        }
        return v;
    }
    bool alive(State s, uint32_t) const override
    {
        const auto [i, d, p] = unpack(s);
        const uint32_t* e = entry(i, p, D_ - d);
        for (uint32_t k = 0; k < width_[size_t(i)]; ++k)
            if (e[k]) return true;
        return false;
    }

private:
    AgreementRanker(uint32_t base, uint32_t w, uint32_t h, uint32_t frames, uint32_t P, uint64_t BP, uint64_t pairs, uint64_t max_dis)
        : B_(base), w_(w), h_(h), f_(frames), P_(P), BP_(BP), n_(uint64_t(w) * h * frames), pairs_(pairs), D_(max_dis)
    {
    }

    struct Unpacked
    {
        uint64_t i, d, p;
    };
    // State = (i * (D+1) + d) * B^P + p.
    State pack(uint64_t i, uint64_t d, uint64_t p) const { return (i * (D_ + 1) + d) * BP_ + p; }
    Unpacked unpack(State s) const
    {
        const uint64_t p = s % BP_, id = s / BP_;
        return {id / (D_ + 1), id % (D_ + 1), p};
    }

    // Disagreements between symbol c at cell i and its placed neighbours in profile p (the most
    // recent cell is the lowest digit).
    uint32_t cost(uint64_t i, uint64_t p, uint32_t c) const
    {
        const uint64_t frame = uint64_t(w_) * h_;
        const uint64_t x = i % w_, y = (i / w_) % h_, f = i / frame;
        auto digit = [&](uint64_t back) { return uint32_t(p / pow_[size_t(back - 1)] % B_); }; // the cell `back` places before i
        uint32_t k = 0;
        if (x > 0 && digit(1) != c) ++k;
        if (y > 0 && digit(w_) != c) ++k;
        if (f > 0 && digit(frame) != c) ++k;
        return k;
    }

    bool plan()
    {
        pow_.assign(P_ + 1, 1);
        for (uint32_t k = 1; k <= P_; ++k) pow_[k] = pow_[k - 1] * B_;
        if (BP_ && (n_ + 1) * (D_ + 1) > ~uint64_t(0) / BP_) return false; // states must fit in 64 bits
        width_.resize(n_ + 1);
        bmax_.resize(n_ + 1);
        offset_.resize(n_ + 2);
        // Widths: B^(n - i) needs this many limbs; the rest of the plan follows.
        BigUint all(1);
        std::vector<uint32_t> w(n_ + 1);
        for (uint64_t r = 0; r <= n_; ++r)
        {
            w[size_t(n_ - r)] = uint32_t(std::max<size_t>(1, (all.bit_length() + 31) / 32));
            all.mul_small(B_);
        }
        // Pairs remaining, from the end.
        std::vector<uint64_t> rem(n_ + 1, 0);
        const uint64_t frame = uint64_t(w_) * h_;
        for (uint64_t j = n_; j-- > 0;) rem[size_t(j)] = rem[size_t(j + 1)] + (j % w_ > 0) + ((j / w_) % h_ > 0) + (j / frame > 0);
        uint64_t total = 0;
        for (uint64_t i = 0; i <= n_; ++i)
        {
            width_[size_t(i)] = w[size_t(i)];
            bmax_[size_t(i)] = std::min<uint64_t>(D_, rem[size_t(i)]);
            offset_[size_t(i)] = total;
            const uint64_t add = uint64_t(width_[size_t(i)]) * BP_ * (bmax_[size_t(i)] + 1);
            total += add;
            if (total > kMaxLimbs) return false;
        }
        offset_[size_t(n_ + 1)] = total;
        pool_.assign(size_t(total), 0);
        return true;
    }

    const uint32_t* entry(uint64_t i, uint64_t p, uint64_t b) const
    {
        const uint64_t bb = std::min<uint64_t>(b, bmax_[size_t(i)]);
        return pool_.data() + offset_[size_t(i)] + (p * (bmax_[size_t(i)] + 1) + bb) * width_[size_t(i)];
    }
    uint32_t* entry(uint64_t i, uint64_t p, uint64_t b)
    {
        return const_cast<uint32_t*>(static_cast<const AgreementRanker*>(this)->entry(i, p, b));
    }

    void build()
    {
        for (uint64_t p = 0; p < BP_; ++p)
            for (uint64_t b = 0; b <= bmax_[size_t(n_)]; ++b) entry(n_, p, b)[0] = 1;
        for (uint64_t i = n_; i-- > 0;)
        {
            const uint32_t W = width_[size_t(i)], Wn = width_[size_t(i + 1)];
            std::vector<uint32_t> costs(B_);
            for (uint64_t p = 0; p < BP_; ++p)
            {
                for (uint32_t c = 0; c < B_; ++c) costs[c] = cost(i, p, c);
                for (uint64_t b = 0; b <= bmax_[size_t(i)]; ++b)
                {
                    uint32_t* dst = entry(i, p, b);
                    for (uint32_t c = 0; c < B_; ++c)
                    {
                        const uint32_t k = costs[c];
                        if (k > b) continue;
                        const uint32_t* src = entry(i + 1, (p * B_ + c) % BP_, b - k);
                        uint64_t carry = 0;
                        for (uint32_t t = 0; t < W; ++t)
                        {
                            const uint64_t v = uint64_t(dst[t]) + (t < Wn ? src[t] : 0) + carry;
                            dst[t] = uint32_t(v);
                            carry = v >> 32;
                        }
                    }
                }
            }
        }
    }

    uint32_t B_, w_, h_, f_, P_;
    uint64_t BP_, n_, pairs_, D_;
    std::vector<uint32_t> width_;
    std::vector<uint64_t> bmax_, offset_, pow_;
    std::vector<uint32_t> pool_;
};

// Pure noise has no spatial or temporal structure: neighbouring pixels agree only as often as
// chance allows (half the time in black and white). Counts pairs of neighbours (left-right and
// up-down within a frame, and the same pixel in consecutive frames) that share a colour.
//   passes iff  1000 * equal >= min_permille * pairs   (a unit with no pairs passes)
class NeighbourAgreement : public Filter
{
public:
    NeighbourAgreement(uint32_t w, uint32_t h, uint32_t frames, int64_t min_permille, uint32_t base)
        : w_(w), h_(h), f_(frames), min_(min_permille)
    {
        provenance_ = "min_permille=" + std::to_string(min_permille);
        const uint64_t pairs = uint64_t(h) * (w - 1) * frames + uint64_t(w) * (h - 1) * frames + uint64_t(w) * h * (frames - 1);
        const uint64_t need = (uint64_t(min_permille) * pairs + 999) / 1000; // agreeing pairs required
        ranker_ = AgreementRanker::make(base, w, h, frames, pairs, need > pairs ? 0 : pairs - need);
        if (need > pairs) ranker_.reset(); // cannot happen for min <= 1000
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        uint64_t equal = 0, pairs = 0;
        const size_t frame = size_t(w_) * h_;
        for (uint32_t f = 0; f < f_; ++f)
            for (uint32_t y = 0; y < h_; ++y)
                for (uint32_t x = 0; x < w_; ++x)
                {
                    const size_t i = f * frame + size_t(y) * w_ + x;
                    if (x + 1 < w_) { ++pairs; equal += u[i] == u[i + 1]; }
                    if (y + 1 < h_) { ++pairs; equal += u[i] == u[i + w_]; }
                    if (f + 1 < f_) { ++pairs; equal += u[i] == u[i + frame]; }
                }
        return pairs == 0 || equal * 1000 >= uint64_t(min_) * pairs;
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    uint32_t w_, h_, f_;
    int64_t min_;
    std::unique_ptr<AgreementRanker> ranker_;
};

} // namespace

void add_media_filters(std::vector<FilterSpec>& out)
{
    FilterSpec n;
    n.id = "neighbour-agreement";
    n.title = "neighbour-agreement";
    n.description = "At least min_permille of neighbouring pixels (and frames) share a colour. "
                    "Pure noise sits at chance (500 in black and white). "
                    "Can rank (compact) while colours^width (video: colours^(width*height)) stays small.";
    n.params = {{"min_permille", "agreeing neighbours required, per thousand", FilterParam::Kind::Integer, "600", 0, 1000, 10, {}}};
    n.applies = [](const FilterLine& l) { return (l.kind == "image" || l.kind == "video") && l.width && l.height; };
    n.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        return std::make_unique<NeighbourAgreement>(l.width, l.height, l.frames ? l.frames : 1,
                                                    param_int(*find_filter("neighbour-agreement-v1"), v, "min_permille"), l.base);
    };
    out.push_back(n);
}

} // namespace sieve
