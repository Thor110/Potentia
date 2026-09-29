// Sieve — not-a-pattern-v1: fails a unit that is a pattern rather than content (FILTER-PLUGINS
// §17). Read as a run of values of `width` digits each (1, 2, 4 or 8; big- or little-endian, the
// last value possibly cut short), a unit fails if
//   repeats  it repeats a block of j values, j <= `period`, at least twice: digit i equals digit
//            i - j*width all the way along, with 2*j*width <= L (00 FF 00 FF ..., 1 2 3 1 2 3 ...)
//   ramps    its values count up by a fixed step, wrapping: value k = (a + k*d) mod B^width, the
//            whole unit being the first L digits of that run (00 01 02 ... FF 00 ..., a 16-bit
//            counter, a gradient); it needs at least three whole values to be one
// Every constant run is both. The rules count exactly, so the filter ranks:
//   repeats  two block lengths that both fit twice give their greatest common divisor as a block
//            too (Fine and Wilf), so a unit has one smallest block, and the units whose smallest
//            block is j values number sum over i | j of mu(j/i) B^(i*width) (Mobius). The union
//            over j <= Q is sum over i <= Q of M(Q/i) B^(i*width), M the Mertens function, and
//            the same holds with some digits already fixed: a fixed prefix either repeats with a
//            block of i values (then B^(the block's digits still free)) or it does not (0).
//   ramps    B^(2*width) of them (a and d); a prefix fixes a, then d, then all.
//   both     a ramp repeats with a block of j values exactly when j*d = 0 mod B^width, so the ramps
//            that also repeat are those whose step d returns to 0 within the period limit: a
//            short list D of steps, with any start.
// The units set aside, under any prefix, are repeats + ramps - both. Floating-point ramps (whose
// steps round) and the binary line (files of every length) are not in v1.

#include "sieve/filter.hpp"

#include <algorithm>
#include <map>
#include <mutex>
#include <numeric>
#include <set>
#include <stdexcept>

namespace sieve {

namespace {

int mobius(uint32_t n)
{
    int m = 1;
    for (uint32_t p = 2; p * p <= n; ++p)
        if (n % p == 0)
        {
            n /= p;
            if (n % p == 0) return 0;
            m = -m;
        }
    return n > 1 ? -m : m;
}

class PatternRules
{
public:
    PatternRules(uint32_t base, uint32_t length, uint32_t width, bool little, uint32_t max_period, bool ramps)
        : b_(base), l_(length), w_(width), little_(little)
    {
        for (uint32_t j = 1; j <= max_period; ++j)
            if (uint64_t(2) * j * w_ <= l_) q_ = j;
        ramps_ = ramps && l_ / w_ >= 3;
        pow_.push_back(BigUint(1));
        for (uint32_t e = 0; e < l_ + 2 * w_; ++e)
        {
            BigUint n = pow_.back();
            n.mul_small(b_);
            pow_.push_back(n);
        }
        bw_ = pow_[w_];
        // c_j = M(Q / j): the union of the repeats, by smallest block (see above).
        c_.assign(q_ + 1, 0);
        for (uint32_t j = 1; j <= q_; ++j)
            for (uint32_t i = 1; i <= q_ / j; ++i) c_[j] += mobius(i);
        // D: the steps whose ramps repeat within the limit (s * d = 0 mod B^w for some s <= Q).
        if (ramps_)
        {
            std::set<std::string> seen;
            for (uint32_t s = 1; s <= q_; ++s)
            {
                // g = gcd(s, B^w) = gcd(s, B^w mod s): the steps are the multiples of B^w / g.
                BigUint rest = bw_;
                const uint32_t g = std::gcd(s, rest.divmod_small(s));
                BigUint step = bw_;
                step.divmod_small(g);
                BigUint d;
                for (uint32_t i = 0; i < g; ++i, d += step)
                    if (seen.insert(d.to_hex()).second) d_.push_back(d);
            }
        }
    }

    uint32_t periods() const { return q_; }
    bool ramps() const { return ramps_; }

    bool passes(std::span<const uint32_t> u) const
    {
        for (uint32_t j = 1; j <= q_; ++j)
        {
            bool rep = true;
            for (size_t i = size_t(j) * w_; i < u.size() && rep; ++i) rep = u[i] == u[i - size_t(j) * w_];
            if (rep) return false;
        }
        if (ramps_)
        {
            const BigUint a = value(u.subspan(0, w_)), v1 = value(u.subspan(w_, w_));
            const auto want = run(a, minus(v1, a));
            if (std::equal(u.begin(), u.end(), want.begin())) return false;
        }
        return true;
    }

    // The walk along a unit, digit by digit.
    struct Walk
    {
        std::vector<uint32_t> digits;
        std::vector<char> rep;                 // rep[j]: the digits so far repeat with a block of j values
        std::vector<std::vector<uint32_t>> v1; // with a known: the digits of a + d, for each d in D
        std::vector<uint32_t> expected;        // with a and d known: the whole ramp
        bool ramp_ok = false, d_in_D = false;
    };
    Walk start() const
    {
        Walk w;
        w.rep.assign(q_ + 1, 1);
        return w;
    }

    // The units set aside among those that begin with the walk's digits and then s.
    BigUint excluded_after(const Walk& wk, uint32_t s) const
    {
        const size_t t = wk.digits.size(), t1 = t + 1;
        BigUint plus, minus_;
        for (uint32_t j = 1; j <= q_; ++j)
        {
            if (c_[j] == 0 || !wk.rep[j]) continue;
            const size_t jw = size_t(j) * w_;
            if (t >= jw && s != wk.digits[t - jw]) continue;
            BigUint term = pow_[jw > t1 ? jw - t1 : 0];
            term.mul_small(uint32_t(std::abs(c_[j])));
            (c_[j] > 0 ? plus : minus_) += term;
        }
        if (ramps_)
        {
            const size_t k1 = t1 / w_, r1 = t1 % w_;
            if (k1 == 0)
            {
                plus += BigUint::mul(pow_[w_ - r1], bw_);
                BigUint both = pow_[w_ - r1];
                both.mul_small(uint32_t(d_.size()));
                minus_ += both;
            }
            else if (k1 == 1)
            {
                plus += pow_[w_ - r1];
                if (r1 == 0) minus_ += BigUint(d_.size());
                else
                {
                    uint32_t n = 0;
                    for (const auto& v : wk.v1)
                    {
                        bool ok = v[r1 - 1] == s;
                        for (size_t i = 0; ok && i + 1 < r1; ++i) ok = v[i] == wk.digits[w_ + i];
                        n += ok ? 1 : 0;
                    }
                    minus_ += BigUint(n);
                }
            }
            else if (t < 2 * size_t(w_)) // s completes the second value: a and d are now fixed
            {
                std::vector<uint32_t> second(wk.digits.begin() + w_, wk.digits.end());
                second.push_back(s);
                const BigUint d = minus(value(second), value(std::span<const uint32_t>(wk.digits).subspan(0, w_)));
                plus += BigUint(1);
                if (in_D(d)) minus_ += BigUint(1);
            }
            else if (wk.ramp_ok && s == wk.expected[t])
            {
                plus += BigUint(1);
                if (wk.d_in_D) minus_ += BigUint(1);
            }
        }
        return plus -= minus_;
    }

    void advance(Walk& wk, uint32_t s) const
    {
        const size_t t = wk.digits.size();
        for (uint32_t j = 1; j <= q_; ++j)
            if (wk.rep[j] && t >= size_t(j) * w_ && s != wk.digits[t - size_t(j) * w_]) wk.rep[j] = 0;
        wk.digits.push_back(s);
        const size_t t1 = t + 1;
        if (!ramps_) return;
        if (t1 == w_)
        {
            const BigUint a = value(wk.digits);
            wk.v1.clear();
            for (const BigUint& d : d_) wk.v1.push_back(digits_of(plus_mod(a, d)));
        }
        else if (t1 == 2 * size_t(w_))
        {
            const std::span<const uint32_t> all(wk.digits);
            const BigUint a = value(all.subspan(0, w_)), d = minus(value(all.subspan(w_, w_)), a);
            wk.d_in_D = in_D(d);
            wk.expected = run(a, d);
            wk.ramp_ok = true;
        }
        else if (t1 > 2 * size_t(w_)) wk.ramp_ok = wk.ramp_ok && s == wk.expected[t];
    }

    // Every unit set aside.
    BigUint excluded_total() const
    {
        BigUint plus, minus_;
        for (uint32_t j = 1; j <= q_; ++j)
        {
            BigUint term = pow_[size_t(j) * w_];
            term.mul_small(uint32_t(std::abs(c_[j])));
            (c_[j] > 0 ? plus : minus_) += term;
        }
        if (ramps_)
        {
            plus += pow_[2 * size_t(w_)];
            BigUint both = bw_;
            both.mul_small(uint32_t(d_.size()));
            minus_ += both;
        }
        return plus -= minus_;
    }

    const BigUint& power(size_t e) const { return pow_[e]; }

private:
    uint32_t b_, l_, w_, q_ = 0;
    bool little_, ramps_ = false;
    std::vector<BigUint> pow_;
    BigUint bw_;
    std::vector<int> c_;
    std::vector<BigUint> d_;

    BigUint value(std::span<const uint32_t> digits) const
    {
        std::vector<uint32_t> v(digits.begin(), digits.end());
        if (little_) std::reverse(v.begin(), v.end());
        return BigUint::from_digits(v, b_);
    }
    std::vector<uint32_t> digits_of(const BigUint& v) const
    {
        auto d = v.to_digits(b_, w_);
        if (little_) std::reverse(d.begin(), d.end());
        return d;
    }
    BigUint plus_mod(const BigUint& a, const BigUint& d) const
    {
        BigUint r = a;
        r += d;
        if (r >= bw_) r -= bw_;
        return r;
    }
    BigUint minus(const BigUint& x, const BigUint& y) const // (x - y) mod B^w
    {
        BigUint r = x;
        r += bw_;
        r -= y;
        if (r >= bw_) r -= bw_;
        return r;
    }
    bool in_D(const BigUint& d) const { return std::find(d_.begin(), d_.end(), d) != d_.end(); }
    std::vector<uint32_t> run(const BigUint& a, const BigUint& d) const // the ramp's first L digits
    {
        std::vector<uint32_t> out;
        BigUint v = a;
        while (out.size() < l_)
        {
            for (uint32_t x : digits_of(v)) out.push_back(x);
            v = plus_mod(v, d);
        }
        out.resize(l_);
        return out;
    }
};

class PatternRanker : public Ranker
{
public:
    PatternRanker(uint32_t base, uint32_t length, std::shared_ptr<const PatternRules> rules)
        : base_(base), length_(length), rules_(std::move(rules))
    {
        prefixes_.push_back({});
        set_count();
    }
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= base_) return kDead;
        std::lock_guard<std::mutex> lock(mx_);
        std::vector<uint32_t> p = prefixes_[size_t(s)];
        if (p.size() >= length_) return kDead;
        p.push_back(symbol);
        std::string key;
        for (uint32_t x : p) key += std::to_string(x) + ",";
        const auto [it, added] = ids_.emplace(key, State(prefixes_.size()));
        if (added) prefixes_.push_back(std::move(p));
        return it->second;
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        std::vector<uint32_t> p;
        {
            std::lock_guard<std::mutex> lock(mx_);
            p = prefixes_[size_t(s)];
        }
        if (p.size() + remaining != length_) return {};
        BigUint all = rules_->power(remaining);
        if (p.empty()) return all -= rules_->excluded_total();
        auto wk = rules_->start();
        for (size_t i = 0; i + 1 < p.size(); ++i) rules_->advance(wk, p[i]);
        return all -= rules_->excluded_after(wk, p.back());
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (!rules_->passes(unit)) throw std::invalid_argument("the unit is not a survivor");
        BigUint r;
        auto wk = rules_->start();
        for (size_t t = 0; t < unit.size(); ++t)
        {
            for (uint32_t s = 0; s < unit[t]; ++s)
            {
                BigUint block = rules_->power(length_ - t - 1);
                block -= rules_->excluded_after(wk, s);
                r += block;
            }
            rules_->advance(wk, unit[t]);
        }
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        BigUint rest = k;
        auto wk = rules_->start();
        for (size_t t = 0; t < length_; ++t)
        {
            uint32_t s = 0;
            for (;; ++s)
            {
                if (s >= base_) throw std::logic_error("pattern unrank ran past the symbols");
                BigUint block = rules_->power(length_ - t - 1);
                block -= rules_->excluded_after(wk, s);
                if (rest < block) break;
                rest -= block;
            }
            rules_->advance(wk, s);
        }
        return wk.digits;
    }

private:
    uint32_t base_, length_;
    std::shared_ptr<const PatternRules> rules_;
    mutable std::mutex mx_;
    mutable std::vector<std::vector<uint32_t>> prefixes_;
    mutable std::map<std::string, State> ids_;
};

class NotAPattern : public Filter
{
public:
    NotAPattern(const FilterLine& l, uint32_t width, bool little, uint32_t period, bool ramps, std::string prov)
    {
        provenance_ = std::move(prov);
        rules_ = std::make_shared<PatternRules>(l.base, l.length, width, little, period, ramps);
        ranker_ = std::make_unique<PatternRanker>(l.base, l.length, rules_);
    }
    bool passes(std::span<const uint32_t> unit) const override { return rules_->passes(unit); }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    std::shared_ptr<const PatternRules> rules_;
    std::unique_ptr<PatternRanker> ranker_;
};

} // namespace

void add_pattern_filters(std::vector<FilterSpec>& out)
{
    FilterSpec p;
    p.id = "not-a-pattern";
    p.title = "not-a-pattern";
    p.description = "Not a pattern: fails a unit that repeats a short block all the way along (00 FF 00 FF ..., 1 2 3 1 2 3 "
                    "...) or counts up by a fixed step (00 01 02 ... FF, a gradient), read as values of 1, 2, 4 or 8 digits. "
                    "Structure, not content, and it slips past the noise filters: a ramp through every byte has the highest "
                    "entropy there is. Exact.";
    p.params = {{"period", "the longest repeated block, in values (0: no repeats)", FilterParam::Kind::Integer, "16", 0, 64, 1, {}},
                {"ramps", "fail values counting up by a fixed step", FilterParam::Kind::Text, "on", 0, 0, 1, {"on", "off"}},
                {"width", "digits a value: 1, 2, 4 or 8 (bytes: 16-, 32- or 64-bit values)", FilterParam::Kind::Text, "1", 0, 0, 1,
                 {"1", "2", "4", "8"}},
                {"order", "byte order of a value wider than one digit", FilterParam::Kind::Text, "big", 0, 0, 1, {"big", "little"}}};
    p.applies = [](const FilterLine& l) { return l.kind != "binary" && l.kind != "models" && l.base >= 2 && l.length >= 2; };
    p.counts_as = "arithmetic";
    p.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter("not-a-pattern-v1");
        const std::string period = param_value(spec, v, "period"), ramps = param_value(spec, v, "ramps"),
                          width = param_value(spec, v, "width"), order = param_value(spec, v, "order");
        const std::string prov = "period=" + period + " ramps=" + ramps + " width=" + width + " order=" + order;
        return std::make_unique<NotAPattern>(l, uint32_t(std::stoul(width)), order == "little", uint32_t(std::stoul(period)),
                                             ramps == "on", prov);
    };
    out.push_back(p);
}

} // namespace sieve
