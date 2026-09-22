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
    : model_(std::move(model)), length_(unit_length), scale_(size_t(kModelTotalBits) * unit_length)
{
    if (!model_) throw std::invalid_argument("guided line needs a model");
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
}

GuidedLine::GuidedLine(std::shared_ptr<const CharModel> model, uint32_t unit_length, const Ranker* sieve)
    : GuidedLine(std::move(model), unit_length)
{
    sieve_ = sieve;
    if (sieve_ && (sieve_->length() != unit_length || sieve_->base() != model_->base()))
        throw std::invalid_argument("the sieve's unit shape does not match the guided line");
    if (sieve_ && sieve_->count().is_zero()) throw std::invalid_argument("no unit survives the sieve");
}

uint64_t GuidedLine::advance(uint64_t state, uint32_t symbol) const
{
    return sieve_ ? sieve_->next(state, symbol) : 0;
}

const uint32_t* GuidedLine::table(std::span<const uint32_t> history, uint64_t state, std::vector<uint32_t>& buf) const
{
    const uint32_t* cum = model_->cumulative(history);
    if (!sieve_) return cum;
    const uint32_t N = model_->base();
    const uint32_t remaining = length_ - uint32_t(history.size()) - 1;
    std::vector<uint32_t> live;
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
    std::vector<uint32_t> f(N, 0);
    std::vector<uint64_t> rem(N, 0);
    uint64_t used = 0;
    for (uint32_t s : live)
    {
        const uint64_t x = spread * (cum[s + 1] - cum[s]);
        f[s] = uint32_t(1 + x / D);
        rem[s] = x % D;
        used += f[s];
    }
    std::vector<uint32_t> order = live;
    std::stable_sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return rem[a] > rem[b]; });
    for (uint64_t k = 0; used < kModelTotal; ++k, ++used) ++f[order[size_t(k)]];
    buf.assign(N + 1, 0);
    for (uint32_t s = 0; s < N; ++s) buf[s + 1] = buf[s] + f[s];
    return buf.data();
}

GuidedLine::Interval GuidedLine::interval(const Digits& unit) const
{
    if (unit.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t N = model_->base();
    Interval iv{BigUint(), BigUint(1)};
    uint64_t state = sieve_ ? sieve_->start() : 0;
    std::vector<uint32_t> buf;
    for (size_t i = 0; i < unit.size(); ++i)
    {
        const uint32_t s = unit[i];
        if (s >= N) throw std::invalid_argument("digit out of range for this model");
        const uint32_t* cum = table(std::span<const uint32_t>(unit.data(), i), state, buf);
        if (cum[s + 1] == cum[s]) throw std::invalid_argument("the unit does not pass the sieve");
        state = advance(state, s);
        iv.low <<= kModelTotalBits;
        if (cum[s])
        {
            BigUint t = iv.width;
            t.mul_small(cum[s]);
            iv.low += t;
        }
        iv.width.mul_small(cum[s + 1] - cum[s]);
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
    Digits unit(length_);
    BigUint rest = point, width(1);
    uint64_t state = sieve_ ? sieve_->start() : 0;
    std::vector<uint32_t> buf;
    for (size_t i = 0; i < length_; ++i)
    {
        const size_t shift = kModelTotalBits * (length_ - 1 - i);
        BigUint top = rest;
        top >>= shift;
        const uint32_t* cum = table(std::span<const uint32_t>(unit.data(), i), state, buf);
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
        state = advance(state, lo);
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
