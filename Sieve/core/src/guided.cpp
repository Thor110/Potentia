#include "sieve/guided.hpp"

#include "sieve/filter.hpp"

#include <algorithm>

#include <cmath>
#include <span>
#include <stdexcept>

namespace sieve {

namespace {

BigUint power_of_two(size_t bits)
{
    BigUint v(1);
    v <<= bits;
    return v;
}

// lo rounded up to a multiple of 2^t.
BigUint round_up(const BigUint& lo, size_t t)
{
    if (t == 0) return lo;
    BigUint x = power_of_two(t);
    x -= BigUint(1);
    x += lo;
    x >>= t;
    x <<= t;
    return x;
}

} // namespace

GuidedLine::GuidedLine(std::shared_ptr<const CharModel> model, uint32_t unit_length)
    : model_(std::move(model)), length_(unit_length), min_(unit_length), scale_(size_t(kModelTotalBits) * unit_length)
{
    if (!model_) throw std::invalid_argument("guided line needs a model");
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
}

GuidedLine::GuidedLine(std::shared_ptr<const CharModel> model, uint32_t min_length, uint32_t max_length, std::vector<const Ranker*> sieves)
    : model_(std::move(model)), length_(max_length), min_(min_length), scale_(size_t(kModelTotalBits) * (size_t(max_length) + 1)),
      sieves_(std::move(sieves))
{
    if (!model_) throw std::invalid_argument("guided line needs a model");
    if (min_length == 0 || min_length >= max_length) throw std::invalid_argument("a guided range needs 1 <= min < max");
    const uint32_t n = max_length - min_length + 1;
    if (n > kModelTotal) throw std::invalid_argument("too many lengths for one step of the guided code");
    if (!sieves_.empty() && sieves_.size() != n) throw std::invalid_argument("a sieved guided range needs a ranker for each length");
    // The length step: an equal share to each length that has a unit (a survivor), the rest one each to the shortest.
    std::vector<char> live(n, 1);
    uint32_t alive = n;
    for (uint32_t i = 0; i < sieves_.size(); ++i)
        if (const Ranker* r = sieves_[i])
        {
            if (r->length() != min_length + i || r->base() != model_->base())
                throw std::invalid_argument("a length's sieve does not match the guided line");
            if (r->count().is_zero()) live[i] = 0, --alive;
        }
    if (alive == 0) throw std::invalid_argument("no unit survives the sieve");
    const uint32_t share = uint32_t(kModelTotal / alive);
    uint32_t extra = uint32_t(kModelTotal - uint64_t(share) * alive);
    lengths_.assign(n + 1, 0);
    for (uint32_t i = 0; i < n; ++i)
    {
        uint32_t f = 0;
        if (live[i])
        {
            f = share;
            if (extra > 0) ++f, --extra;
        }
        lengths_[i + 1] = lengths_[i] + f;
    }
}

namespace {
GuidedLine sieved_line(std::shared_ptr<const CharModel> model, const Ranker& sieve)
{
    if (sieve.min_length() >= sieve.length()) return GuidedLine(std::move(model), sieve.length(), &sieve);
    std::vector<const Ranker*> v;
    for (uint32_t l = sieve.min_length(); l <= sieve.length(); ++l) v.push_back(sieve.at_length(l));
    return GuidedLine(std::move(model), sieve.min_length(), sieve.length(), std::move(v));
}
} // namespace

GuidedLine::GuidedLine(std::shared_ptr<const CharModel> model, const Ranker& sieve) : GuidedLine(sieved_line(std::move(model), sieve)) {}

GuidedLine::GuidedLine(std::shared_ptr<const CharModel> model, uint32_t unit_length, const Ranker* sieve)
    : GuidedLine(std::move(model), unit_length)
{
    sieve_ = sieve;
    if (sieve_ && (sieve_->length() != unit_length || sieve_->base() != model_->base()))
        throw std::invalid_argument("the sieve's unit shape does not match the guided line");
    if (sieve_ && sieve_->count().is_zero()) throw std::invalid_argument("no unit survives the sieve");
}

const uint32_t* GuidedLine::table(const Ranker* sieve_, uint32_t length, std::span<const uint32_t> history, uint64_t state,
                                  std::vector<uint32_t>& buf) const
{
    const uint32_t* cum = model_->cumulative(history);
    if (!sieve_) return cum;
    const uint32_t N = model_->base();
    const uint32_t remaining = length - uint32_t(history.size()) - 1;
    // (Its working lists made once a thread and cleared each time: they were four allocations a
    // symbol, each position of a unit read. Per thread, since the hallway paints on several.)
    thread_local std::vector<uint32_t> live, f, order;
    thread_local std::vector<uint64_t> rem;
    live.clear();
    for (uint32_t s = 0; s < N; ++s)
    {
        const Ranker::State t = sieve_->next(state, s);
        if (t != Ranker::kDead && sieve_->alive(t, remaining)) live.push_back(s);
    }
    if (live.size() == N) return cum;
    if (live.empty()) throw std::logic_error("sieved guided line reached a dead end");
    uint64_t D = 0;
    for (uint32_t s : live) D += cum[s + 1] - cum[s];
    const uint64_t spread = kModelTotal - live.size();
    f.assign(N, 0);
    rem.assign(N, 0);
    uint64_t used = 0;
    for (uint32_t s : live)
    {
        const uint64_t x = spread * (cum[s + 1] - cum[s]);
        f[s] = uint32_t(1 + x / D);
        rem[s] = x % D;
        used += f[s];
    }
    order.assign(live.begin(), live.end());
    std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return rem[a] > rem[b]; });
    for (uint64_t k = 0; used < kModelTotal; ++k, ++used) ++f[order[size_t(k)]];
    buf.assign(N + 1, 0);
    for (uint32_t s = 0; s < N; ++s) buf[s + 1] = buf[s] + f[s];
    return buf.data();
}

GuidedLine::Interval GuidedLine::interval(const Digits& unit) const
{
    const uint32_t L = uint32_t(unit.size());
    if (L < min_ || L > length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t N = model_->base();
    Interval iv{BigUint(), BigUint(1)};
    if (is_range())
    {
        // The length step (see the header): the arc of length L's units.
        const uint32_t lo = lengths_[L - min_], hi = lengths_[L - min_ + 1];
        if (lo == hi) throw std::invalid_argument("the unit does not pass the sieve");
        iv.low = BigUint(lo);
        iv.width = BigUint(hi - lo);
    }
    const Ranker* sieve = sieve_of(L);
    uint64_t state = sieve ? sieve->start() : 0;
    std::vector<uint32_t> buf;
    for (size_t i = 0; i < unit.size(); ++i)
    {
        const uint32_t s = unit[i];
        if (s >= N) throw std::invalid_argument("digit out of range for this model");
        const uint32_t* cum = table(sieve, L, std::span<const uint32_t>(unit.data(), i), state, buf);
        if (cum[s + 1] == cum[s]) throw std::invalid_argument("the unit does not pass the sieve");
        state = sieve ? sieve->next(state, s) : 0;
        iv.low <<= kModelTotalBits;
        if (cum[s]) iv.low.add_mul_small(iv.width, cum[s]); // (in place: no copy of the width)
        iv.width.mul_small(cum[s + 1] - cum[s]);
    }
    if (L < length_) // (a shorter unit of a range: its arc scaled out to the line's S bits)
    {
        iv.low <<= size_t(kModelTotalBits) * (length_ - L);
        iv.width <<= size_t(kModelTotalBits) * (length_ - L);
    }
    return iv;
}

GuidedLine::Code GuidedLine::code_of(const Interval& iv) const
{
    BigUint hi = iv.low;
    hi += iv.width;
    // Largest t such that an aligned block [m 2^t, (m+1) 2^t) fits inside [low, high); the
    // lowest such block is the address, with S - t bits. t = 0 always fits (width >= 1).
    auto fits = [&](size_t t) {
        BigUint end = round_up(iv.low, t);
        end += power_of_two(t);
        return end <= hi;
    };
    size_t a = 0, b = scale_;
    while (a < b)
    {
        const size_t mid = (a + b + 1) / 2;
        if (fits(mid)) a = mid;
        else b = mid - 1;
    }
    Code c;
    c.point = round_up(iv.low, a);
    c.bits = scale_ - a;
    c.hex = hex_of(c.point, c.bits);
    return c;
}

GuidedLine::Code GuidedLine::code(const Digits& unit) const { return code_of(interval(unit)); }

GuidedLine::Digits GuidedLine::unit_at(const BigUint& point) const
{
    if (point.bit_length() > scale_) throw std::out_of_range("point is outside the line");
    const uint32_t N = model_->base();
    BigUint rest = point, width(1);
    uint32_t L = length_;
    if (is_range())
    {
        // The length step: the length whose share holds the point's first 16 bits.
        BigUint top = point;
        top >>= scale_ - kModelTotalBits;
        const uint64_t t = top.limbs().empty() ? 0 : top.limbs()[0];
        uint32_t i = 0;
        while (lengths_[i + 1] <= t) ++i;
        L = min_ + i;
        BigUint below(lengths_[i]);
        below <<= scale_ - kModelTotalBits;
        rest -= below;
        width = BigUint(lengths_[i + 1] - lengths_[i]);
    }
    Digits unit(L);
    const Ranker* sieve = sieve_of(L);
    uint64_t state = sieve ? sieve->start() : 0;
    std::vector<uint32_t> buf;
    for (size_t i = 0; i < L; ++i)
    {
        // (On a range the shift counts from the line's longest length: the symbols' steps follow the length's.)
        const size_t shift = kModelTotalBits * (length_ - 1 - i);
        BigUint top = rest;
        top >>= shift;
        const uint32_t* cum = table(sieve, L, std::span<const uint32_t>(unit.data(), i), state, buf);
        // Largest s with cum[s] * width <= top.
        uint32_t lo = 0, hi = N - 1;
        while (lo < hi)
        {
            const uint32_t mid = (lo + hi + 1) / 2;
            BigUint t = width;
            t.mul_small(cum[mid]);
            if (t <= top) lo = mid;
            else hi = mid - 1;
        }
        unit[i] = lo;
        state = sieve ? sieve->next(state, lo) : 0;
        if (cum[lo])
        {
            BigUint t = width;
            t.mul_small(cum[lo]);
            t <<= shift;
            rest -= t;
        }
        width.mul_small(cum[lo + 1] - cum[lo]);
    }
    return unit;
}

BigUint GuidedLine::point_of(std::string_view hex) const
{
    if (hex.empty()) throw std::invalid_argument("empty guided address");
    BigUint v = BigUint::from_hex(hex);
    const size_t bits = 4 * hex.size();
    if (bits <= scale_) v <<= scale_ - bits;
    else v >>= bits - scale_;
    return v;
}

std::string GuidedLine::hex_of(const BigUint& point, size_t bits) const
{
    if (bits == 0) return "0";
    if (bits > scale_) throw std::invalid_argument("more bits than the line has");
    const size_t nh = (bits + 3) / 4;
    BigUint v = point;
    v >>= scale_ - 4 * nh;
    return v.to_hex(nh);
}

BigUint GuidedLine::step(const BigUint& point, int64_t offset, uint32_t depth) const
{
    if (depth > scale_) throw std::invalid_argument("zoom depth is finer than the line");
    const bool forward = offset >= 0;
    uint64_t k = forward ? uint64_t(offset) : 0 - uint64_t(offset);
    if (depth < 64) k &= (uint64_t(1) << depth) - 1; // whole turns of the loop do nothing
    BigUint delta(k);
    delta <<= scale_ - depth;
    const BigUint full = power_of_two(scale_);
    BigUint p = point;
    if (forward)
    {
        p += delta;
        if (p >= full) p -= full;
    }
    else if (p >= delta) p -= delta;
    else
    {
        p += full;
        p -= delta;
    }
    return p;
}

double GuidedLine::fraction(const BigUint& point) const { return point.ratio_to_power_of_two(scale_); }

double GuidedLine::information_bits(const Interval& iv) const
{
    return double(scale_) - iv.width.log10_approx() / std::log10(2.0);
}

} // namespace sieve
