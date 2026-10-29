// Sieve — the AI line's filters (SPECIFICATIONS §12.0f): on a model's weights, its digits.
//
//   weight-rows-v1       a row of a tensor (a run of digits the line reads together) fails if one
//                        digit repeats more than max_run times in a row, if it repeats a block of
//                        `period` digits or fewer all the way along (2 x the block at most the
//                        row), or if it steps by a fixed amount from end to end without wrapping
//                        (a ramp: 0 1 2 3 ..., 15 13 11 ...). Structure, which trained weights do
//                        not have: Edward's "seek the noise". Rows are judged one at a time, so a
//                        model passes when every row does, and its survivors are those of each row
//                        multiplied together: counted, ranked and unranked exactly (compact).
//   canonical-model-v1   one model of each class of the line's symmetries (sieve/aispace.hpp): the
//                        model is its own canonical form, chosen by SHA-256 so it looks like noise
//                        too. Counted exactly; judged on the shelves (it does not rank yet: its
//                        groups' members are read across rows, interleaved in address order).
//
// Counting one row of c digits of K (R = max_run, Q = period, at most c / 2):
//   runs     the rows with no run past R, from a prefix: g(rest, r), the ways to finish with `rest`
//            digits after a run of r: g(0, r) = 1, g(rest, r) = g(rest - 1, r + 1) [r < R] +
//            (K - 1) g(rest - 1, 1).
//   periods  a row whose smallest period is p <= Q is its first p digits (a primitive block)
//            repeated; any two periods of at most c / 2 have their greatest common divisor as one
//            too (Fine and Wilf), so the rows with some period up to Q are those with smallest
//            period p, for each p, disjoint, and by Mobius over the divisors e of p, sum over e of
//            M(Q / e) P(e), M the Mertens function and P(e) the rows of period e (with no run past
//            R) that agree with the prefix. A row of period e repeats its block, so its runs are the
//            block's read round in a circle (the circle fits in the row twice): P(e) counts blocks
//            whose runs, round the circle, are at most R (a block of one digit throughout has a run
//            the row's length).
//   ramps    a + i d, d not 0, every value a digit: never periodic, every run 1.
// A row passes the runs, has no small period and is not a ramp: runs - periods - ramps.

#include "sieve/aispace.hpp"
#include "sieve/filter.hpp"
#include "sieve/sha256.hpp"
#include "sieve/training.hpp"

#include <algorithm>
#include <cstdlib>
#include <map>
#include <mutex>
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

// A signed sum of counts.
struct Tally
{
    BigUint plus, minus;
    void add(const BigUint& x, int times)
    {
        if (times == 0 || x.is_zero()) return;
        BigUint t = x;
        t.mul_small(uint32_t(std::abs(times)));
        (times > 0 ? plus : minus) += t;
    }
};

class RowRule
{
public:
    RowRule(uint32_t base, uint32_t length, uint32_t max_run, uint32_t period, bool ramps)
        : k_(base), c_(length), r_(max_run == 0 || max_run >= length ? length : max_run), q_(std::min(period, length / 2)),
          ramps_(ramps && length >= 3 && base >= 2)
    {
        runs_on_ = r_ < c_;
        // g[rest][r], r = 1 .. R (with no limit, K^rest).
        if (runs_on_) g_.assign(c_ + 1, std::vector<BigUint>(r_ + 2));
        for (uint32_t rest = 0; runs_on_ && rest <= c_; ++rest)
            for (uint32_t r = 1; r <= r_; ++r)
            {
                if (rest == 0)
                {
                    g_[0][r] = BigUint(1);
                    continue;
                }
                BigUint x = g_[rest - 1][1];
                x.mul_small(k_ - 1);
                if (r < r_) x += g_[rest - 1][r + 1];
                g_[rest][r] = std::move(x);
            }
        pow_.push_back(BigUint(1));
        for (uint32_t e = 0; e < c_; ++e)
        {
            BigUint x = pow_.back();
            x.mul_small(k_);
            pow_.push_back(std::move(x));
        }
        // M(Q / e), e = 1 .. Q.
        mert_.assign(q_ + 1, 0);
        for (uint32_t e = 1; e <= q_; ++e)
            for (uint32_t i = 1; i <= q_ / e; ++i) mert_[e] += mobius(i);
        count_ = completions({});
    }

    uint32_t length() const { return c_; }
    uint32_t base() const { return k_; }
    const BigUint& count() const { return count_; }

    bool passes(std::span<const uint32_t> row) const
    {
        uint32_t run = 0;
        for (size_t i = 0; i < row.size(); ++i)
        {
            run = i > 0 && row[i] == row[i - 1] ? run + 1 : 1;
            if (run > r_) return false;
        }
        for (uint32_t p = 1; p <= q_; ++p)
        {
            bool rep = true;
            for (size_t i = p; i < row.size() && rep; ++i) rep = row[i] == row[i - p];
            if (rep) return false;
        }
        return !ramp(row);
    }

    // The rows that pass and begin with `prefix`.
    BigUint completions(std::span<const uint32_t> prefix) const
    {
        BigUint all = runs(prefix);
        if (all.is_zero()) return all;
        Tally t = excluded(prefix);
        all += t.minus;
        all -= t.plus;
        return all;
    }

    BigUint rank(std::span<const uint32_t> row) const
    {
        std::vector<uint32_t> p;
        BigUint r;
        for (size_t t = 0; t < row.size(); ++t)
        {
            for (uint32_t s = 0; s < row[t]; ++s)
            {
                p.push_back(s);
                r += completions(p);
                p.pop_back();
            }
            p.push_back(row[t]);
        }
        return r;
    }

    std::vector<uint32_t> unrank(BigUint k) const
    {
        std::vector<uint32_t> p;
        for (uint32_t t = 0; t < c_; ++t)
        {
            uint32_t s = 0;
            for (;; ++s)
            {
                if (s >= k_) throw std::logic_error("weight-rows: unrank ran past the digits");
                p.push_back(s);
                const BigUint n = completions(p);
                if (k < n) break;
                k -= n;
                p.pop_back();
            }
        }
        return p;
    }

private:
    uint32_t k_, c_, r_, q_;
    bool ramps_, runs_on_ = false;
    std::vector<std::vector<BigUint>> g_;
    std::vector<BigUint> pow_;
    std::vector<int> mert_;
    BigUint count_;

    bool ramp(std::span<const uint32_t> row) const
    {
        if (!ramps_ || row.size() < 3) return false;
        const int64_t d = int64_t(row[1]) - int64_t(row[0]);
        if (d == 0) return false;
        for (size_t i = 2; i < row.size(); ++i)
            if (int64_t(row[i]) - int64_t(row[i - 1]) != d) return false;
        return true;
    }

    // Rows with no run past R that begin with the prefix.
    BigUint runs(std::span<const uint32_t> p) const
    {
        if (p.size() > c_) return {};
        if (!runs_on_) return pow_[c_ - p.size()];
        if (p.empty())
        {
            BigUint x = g_[c_ - 1][1];
            x.mul_small(k_);
            return x;
        }
        uint32_t run = 0;
        for (size_t i = 0; i < p.size(); ++i)
        {
            run = i > 0 && p[i] == p[i - 1] ? run + 1 : 1;
            if (run > r_) return {};
        }
        return g_[c_ - p.size()][run];
    }

    // Rows (with no run past R) of the prefix that the periods and the ramps set aside.
    Tally excluded(std::span<const uint32_t> p) const
    {
        Tally t;
        for (uint32_t e = 1; e <= q_; ++e)
            if (mert_[e] != 0) t.add(periodic(p, e), mert_[e]);
        t.add(ramps(p), 1);
        return t;
    }

    // Rows of period e with no run past R that begin with p.
    BigUint periodic(std::span<const uint32_t> p, uint32_t e) const
    {
        const size_t t = p.size();
        if (t >= e)
        {
            for (size_t i = e; i < t; ++i)
                if (p[i] != p[i - e]) return {};
            return circle_ok(p.subspan(0, e)) ? BigUint(1) : BigUint();
        }
        if (!runs_on_) return pow_[e - t];
        if (t == 0)
        {
            // Every first digit alike: K times the blocks that begin with any one of them.
            const uint32_t zero = 0;
            BigUint x = blocks(std::span<const uint32_t>(&zero, 1), e);
            x.mul_small(k_);
            return x;
        }
        return blocks(p, e);
    }

    // Whether the row that repeats this block has no run past R: the block's runs round a circle.
    bool circle_ok(std::span<const uint32_t> b) const
    {
        if (!runs_on_) return true;
        const size_t e = b.size();
        size_t first = 0;
        while (first < e && b[first] == b[0]) ++first;
        if (first == e) return false; // one digit throughout: a run the row's length
        size_t run = 0, longest = 0;
        for (size_t i = 0; i < e; ++i)
        {
            run = i > 0 && b[i] == b[i - 1] ? run + 1 : 1;
            longest = std::max(longest, run);
        }
        if (b[e - 1] == b[0]) longest = std::max(longest, run + first); // round the end
        return longest <= r_;
    }

    // Blocks of e digits that begin with p (1 <= |p| < e) whose runs round the circle are at most R.
    BigUint blocks(std::span<const uint32_t> p, uint32_t e) const
    {
        const size_t t = p.size();
        const uint32_t f = p[0];
        uint32_t run = 0, first = 0;
        bool open = true; // the prefix is the first digit throughout
        for (size_t i = 0; i < t; ++i)
        {
            run = i > 0 && p[i] == p[i - 1] ? run + 1 : 1;
            if (run > r_) return {};
            if (open && p[i] != f) open = false, first = uint32_t(i);
        }
        if (!open) return walk(t, p[t - 1] == f, run, first, e);
        // The first run goes on to some place k (t <= k < e), where a digit other than f comes.
        BigUint total;
        for (uint32_t k = uint32_t(t); k < e; ++k)
        {
            if (k > r_) break;
            BigUint x = walk(k + 1, false, 1, k, e);
            x.mul_small(k_ - 1);
            total += x;
        }
        return total;
    }

    // Ways to finish a block of e digits from place `at`, the last digit f (the first) or not, after a
    // run of r, the first run `first` long: every run at most R, and a last run of f, with the first,
    // at most R round the end.
    BigUint walk(size_t at, bool is_f, uint32_t r, uint32_t first, uint32_t e) const
    {
        // ways[f?][r]
        std::vector<BigUint> wf(r_ + 2), wo(r_ + 2);
        (is_f ? wf : wo)[r] = BigUint(1);
        for (size_t i = at; i < e; ++i)
        {
            std::vector<BigUint> nf(r_ + 2), no(r_ + 2);
            BigUint sum_f, sum_o;
            for (uint32_t x = 1; x <= r_; ++x)
            {
                if (x < r_)
                {
                    nf[x + 1] += wf[x];
                    no[x + 1] += wo[x];
                }
                sum_f += wf[x];
                sum_o += wo[x];
            }
            // To f from another digit; to another digit (not f) from f, or from another (not itself).
            nf[1] += sum_o;
            BigUint a = sum_f;
            a.mul_small(k_ - 1);
            BigUint b = sum_o;
            b.mul_small(k_ - 2);
            no[1] += a;
            no[1] += b;
            wf.swap(nf);
            wo.swap(no);
        }
        BigUint total;
        for (uint32_t x = 1; x <= r_; ++x)
        {
            total += wo[x];
            if (x + first <= r_) total += wf[x];
        }
        return total;
    }

    // Ramps that begin with p.
    BigUint ramps(std::span<const uint32_t> p) const
    {
        if (!ramps_) return {};
        const int64_t K = k_, last = int64_t(c_) - 1;
        auto fits = [&](int64_t a, int64_t d) { return a + last * d >= 0 && a + last * d < K; };
        if (p.empty())
        {
            uint64_t n = 0;
            for (int64_t d = 1; d * last < K; ++d) n += 2 * uint64_t(K - d * last);
            return BigUint(n);
        }
        if (p.size() == 1)
        {
            uint64_t n = 0;
            for (int64_t d = 1; d * last < K; ++d) n += uint64_t(fits(p[0], d)) + uint64_t(fits(p[0], -d));
            return BigUint(n);
        }
        const int64_t d = int64_t(p[1]) - int64_t(p[0]);
        if (d == 0 || !fits(p[0], d)) return {};
        for (size_t i = 2; i < p.size(); ++i)
            if (int64_t(p[i]) - int64_t(p[i - 1]) != d) return {};
        return BigUint(1);
    }
};

// A unit made of rows, each judged by the rule for its length: survivors the product of the rows'.
// A survivor's rank reads its rows' ranks as one number, each row a digit in base its count.
class RowsRanker : public Ranker
{
public:
    // `rows`: each row's length, in address order; `rules`: the rule for each length.
    RowsRanker(std::vector<uint32_t> rows, std::map<uint32_t, std::shared_ptr<const RowRule>> rules, uint32_t base)
        : rows_(std::move(rows)), rules_(std::move(rules)), base_(base)
    {
        starts_.push_back(0);
        for (uint32_t c : rows_) starts_.push_back(starts_.back() + c);
        length_ = uint32_t(starts_.back());
        prefixes_.push_back({0, {}});
        set_count();
    }
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= base_) return kDead;
        std::lock_guard<std::mutex> lock(mx_);
        auto [row, p] = prefixes_[size_t(s)];
        if (row >= rows_.size()) return kDead;
        p.push_back(symbol);
        const RowRule& rule = rule_of(row);
        if (rule.completions(p).is_zero()) return kDead;
        if (p.size() == rule.length()) ++row, p.clear();
        std::string key = std::to_string(row) + ":";
        for (uint32_t x : p) key += std::to_string(x) + ",";
        const auto [it, added] = ids_.emplace(key, State(prefixes_.size()));
        if (added) prefixes_.push_back({row, std::move(p)});
        return it->second;
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        size_t row;
        std::vector<uint32_t> p;
        {
            std::lock_guard<std::mutex> lock(mx_);
            row = prefixes_[size_t(s)].first;
            p = prefixes_[size_t(s)].second;
        }
        if (starts_[row] + p.size() + remaining != length_) return {};
        if (row >= rows_.size()) return BigUint(1);
        return BigUint::mul(rule_of(row).completions(p), after(row + 1));
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != length_) throw std::invalid_argument("weight-rows: a unit of another length");
        BigUint r;
        for (size_t i = 0; i < rows_.size(); ++i)
        {
            const RowRule& rule = rule_of(i);
            const std::span<const uint32_t> row = unit.subspan(starts_[i], rows_[i]);
            if (!rule.passes(row)) throw std::invalid_argument("the unit is not a survivor");
            r = BigUint::mul(r, rule.count());
            r += rule.rank(row);
        }
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        std::vector<uint32_t> out(length_);
        BigUint rest = k;
        for (size_t i = rows_.size(); i-- > 0;)
        {
            const RowRule& rule = rule_of(i);
            BigUint q, r;
            BigUint::divmod(rest, rule.count(), q, r);
            const std::vector<uint32_t> row = rule.unrank(r);
            std::copy(row.begin(), row.end(), out.begin() + std::ptrdiff_t(starts_[i]));
            rest = std::move(q);
        }
        return out;
    }

private:
    std::vector<uint32_t> rows_;
    std::vector<uint64_t> starts_;
    std::map<uint32_t, std::shared_ptr<const RowRule>> rules_;
    uint32_t base_, length_ = 0;
    mutable std::mutex mx_;
    mutable std::vector<std::pair<size_t, std::vector<uint32_t>>> prefixes_; // a state: its row, and that row's digits so far
    mutable std::map<std::string, State> ids_;
    mutable std::map<size_t, BigUint> after_; // the survivors of rows i onwards

    const RowRule& rule_of(size_t row) const { return *rules_.at(rows_[row]); }
    BigUint after(size_t row) const
    {
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (auto it = after_.find(row); it != after_.end()) return it->second;
        }
        // Rows of one length come together (a tensor's), so the rest is a few powers.
        BigUint x(1);
        for (size_t i = row; i < rows_.size();)
        {
            size_t j = i;
            while (j < rows_.size() && rows_[j] == rows_[i]) ++j;
            x = BigUint::mul(x, BigUint::pow(rule_of(i).count(), j - i));
            i = j;
        }
        std::lock_guard<std::mutex> lock(mx_);
        return after_.emplace(row, std::move(x)).first->second;
    }
};

class WeightRows : public Filter
{
public:
    WeightRows(const AiSpace& space, uint32_t max_run, uint32_t period, bool ramps, std::string prov)
    {
        provenance_ = std::move(prov);
        const uint32_t K = 1u << space.shape().bits;
        std::map<uint32_t, std::shared_ptr<const RowRule>> rules;
        std::vector<uint32_t> rows;
        for (const AiSpace::Tensor& t : space.tensors())
        {
            if (!rules.count(t.cols)) rules[t.cols] = std::make_shared<RowRule>(K, t.cols, max_run, period, ramps);
            rows.insert(rows.end(), t.rows, t.cols);
        }
        rows_ = rows;
        rules_ = rules;
        ranker_ = std::make_unique<RowsRanker>(std::move(rows), std::move(rules), K);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        size_t at = 0;
        for (uint32_t c : rows_)
        {
            if (at + c > unit.size() || !rules_.at(c)->passes(unit.subspan(at, c))) return false;
            at += c;
        }
        return at == unit.size();
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    std::vector<uint32_t> rows_;
    std::map<uint32_t, std::shared_ptr<const RowRule>> rules_;
    std::unique_ptr<RowsRanker> ranker_;
};

class CanonicalModel : public Filter
{
public:
    explicit CanonicalModel(const AiSpace& space) : space_(space) { provenance_ = "symmetries=" + std::string(kAiSpaceVersion); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != space_.weights()) return false;
        AiSpace::Digits d(unit.size());
        for (size_t i = 0; i < unit.size(); ++i) d[i] = uint8_t(unit[i]);
        return space_.is_canonical(d);
    }
    const BigUint* survivors() const override { return &space_.canonical_count(); }

private:
    AiSpace space_;
};

// language-v1: a model's mean probability of each next byte of a run's text, exact (training-v1's
// integer forward pass), at least min_permille thousandths.
class Language : public Filter
{
public:
    Language(const AiSpace& space, std::shared_ptr<const ModelRun> run, uint32_t bytes, int64_t min_permille)
        : space_(space), run_(std::move(run)), min_(min_permille)
    {
        const size_t n = std::min<size_t>(bytes, run_->text.size());
        if (n < 2) throw std::invalid_argument("language-v1: run '" + run_->id + "' has under two bytes of text");
        text_.assign(run_->text.begin(), run_->text.begin() + std::ptrdiff_t(n));
        provenance_ = "run=" + run_->id + " text=" + Sha256::hex(Sha256::hash(std::string(text_.begin(), text_.end()))) + " bytes=" +
                      std::to_string(n) + " min_permille=" + std::to_string(min_permille);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const training::Score s = score(unit);
        // mean = prob_sum / (2^16 predictions) >= min / 1000, in integers.
        return s.prob_sum * 1000 >= uint64_t(min_) * s.predictions * 65536;
    }
    training::Score score(std::span<const uint32_t> unit) const
    {
        AiSpace::Digits d(unit.size());
        for (size_t i = 0; i < unit.size(); ++i) d[i] = uint8_t(unit[i]);
        return training::evaluate_model(space_, d, {std::span<const uint8_t>(text_)});
    }

private:
    AiSpace space_;
    std::shared_ptr<const ModelRun> run_;
    std::vector<uint8_t> text_;
    int64_t min_;
};

// near-model-v1: at most `changes` digits other than a run's model's (its layers: the models k digits
// from it). Counted and ranked in address order: from a place with b changes still allowed and r
// digits to go, sum over i <= b of C(r, i) (K - 1)^i ways to finish.
class NearRanker : public Ranker
{
public:
    NearRanker(std::vector<uint32_t> anchor, uint32_t base, uint32_t changes)
        : anchor_(std::move(anchor)), base_(base), k_(changes)
    {
        set_count();
    }
    uint32_t length() const override { return uint32_t(anchor_.size()); }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= base_) return kDead;
        const uint64_t at = s / (k_ + 1), used = s % (k_ + 1);
        if (at >= anchor_.size()) return kDead;
        const uint64_t u = used + (symbol != anchor_[size_t(at)] ? 1 : 0);
        if (u > k_) return kDead;
        return (at + 1) * (k_ + 1) + u;
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        const uint64_t at = s / (k_ + 1), used = s % (k_ + 1);
        if (at + remaining != anchor_.size()) return {};
        return ways(remaining, uint32_t(k_ - used));
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != anchor_.size()) throw std::invalid_argument("near-model: a unit of another length");
        BigUint r;
        uint32_t left = k_;
        for (size_t t = 0; t < unit.size(); ++t)
        {
            const uint32_t x = unit[t], a = anchor_[t], rest = uint32_t(unit.size() - t - 1);
            if (x > 0)
            {
                // The symbols below x: the anchor's (if below) keeps the changes left, the others use one.
                const uint32_t others = x - (a < x ? 1 : 0);
                if (a < x) r += ways(rest, left);
                if (others && left > 0)
                {
                    BigUint w = ways(rest, left - 1);
                    w.mul_small(others);
                    r += w;
                }
            }
            if (x != a)
            {
                if (left == 0) throw std::invalid_argument("the unit is not a survivor");
                --left;
            }
        }
        return r;
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        std::vector<uint32_t> out(anchor_.size());
        BigUint rest = k;
        uint32_t left = k_;
        for (size_t t = 0; t < anchor_.size(); ++t)
        {
            const uint32_t a = anchor_[t], r = uint32_t(anchor_.size() - t - 1);
            if (left == 0)
            {
                std::copy(anchor_.begin() + std::ptrdiff_t(t), anchor_.end(), out.begin() + std::ptrdiff_t(t));
                break;
            }
            const BigUint same = ways(r, left), other = ways(r, left - 1);
            // Symbols below the anchor's each use a change; then the anchor's; then those above.
            uint32_t s = 0;
            for (;; ++s)
            {
                if (s >= base_) throw std::logic_error("near-model unrank ran past the digits");
                const BigUint& n = s == a ? same : other;
                if (rest < n) break;
                rest -= n;
            }
            out[t] = s;
            if (s != a) --left;
        }
        return out;
    }

private:
    std::vector<uint32_t> anchor_;
    uint32_t base_, k_;
    mutable std::mutex mx_;
    mutable std::map<std::pair<uint32_t, uint32_t>, BigUint> ways_;
    BigUint ways(uint32_t r, uint32_t b) const // sum over i <= min(b, r) of C(r, i) (K - 1)^i
    {
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (auto it = ways_.find({r, b}); it != ways_.end()) return it->second;
        }
        BigUint total, term(1); // term = C(r, i) (K - 1)^i
        for (uint32_t i = 0; i <= std::min(b, r); ++i)
        {
            if (i > 0)
            {
                term.mul_small(r - i + 1);
                term.mul_small(base_ - 1);
                BigUint q, rem;
                BigUint::divmod(term, BigUint(i), q, rem);
                term = std::move(q);
            }
            total += term;
        }
        std::lock_guard<std::mutex> lock(mx_);
        if (ways_.size() > 4096) ways_.clear();
        return ways_.emplace(std::make_pair(r, b), std::move(total)).first->second;
    }
};

class NearModel : public Filter
{
public:
    NearModel(std::vector<uint32_t> anchor, uint32_t base, uint32_t changes, std::string prov) : anchor_(anchor), k_(changes)
    {
        provenance_ = std::move(prov);
        ranker_ = std::make_unique<NearRanker>(std::move(anchor), base, changes);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != anchor_.size()) return false;
        uint32_t d = 0;
        for (size_t i = 0; i < unit.size(); ++i)
            if (unit[i] != anchor_[i] && ++d > k_) return false;
        return true;
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    std::vector<uint32_t> anchor_;
    uint32_t k_;
    std::unique_ptr<NearRanker> ranker_;
};

// A run's model on this line: its shape must be the line's.
std::vector<uint32_t> anchor_of(const AiSpace& space, const ModelRun& run, const char* filter)
{
    const AiShape& s = space.shape();
    if (run.layers != s.layers || run.width != s.width || run.heads != s.heads || run.bits != s.bits)
        throw std::invalid_argument(std::string(filter) + ": run '" + run.id + "' trains a model of another shape (L" + std::to_string(run.layers) +
                                    "/H" + std::to_string(run.width) + "/A" + std::to_string(run.heads) + "/B" + std::to_string(run.bits) + ")");
    if (run.final_address.empty()) throw std::invalid_argument(std::string(filter) + ": run '" + run.id + "' has not been trained");
    const AiSpace::Digits d = space.digits_at(space.parse(run.final_address), AddressMode::Positional);
    return std::vector<uint32_t>(d.begin(), d.end());
}

bool is_ai(const FilterLine& l) { return l.kind == "ai" && l.ai != nullptr; }

} // namespace

void add_weight_filters(std::vector<FilterSpec>& out)
{
    FilterSpec rows;
    rows.id = "weight-rows";
    rows.title = "weight-rows";
    rows.description = "Every row of every tensor free of structure: no digit more than max_run times in a row, no block of "
                       "`period` digits or fewer repeated all the way along, no ramp (a fixed step from end to end). Trained "
                       "models pass at the defaults (measured at 1 to 8 bits a weight: runs of at most 12, at 1 bit). Exact, "
                       "row by row: counts and ranks (compact).";
    rows.params = {{"max_run", "longest run of one digit in a row (0: no limit)", FilterParam::Kind::Integer, "16", 0, 4096, 1, {}},
                   {"period", "the longest repeated block, in digits (0: no repeats)", FilterParam::Kind::Integer, "4", 0, 16, 1, {}},
                   {"ramps", "fail rows that step by a fixed amount end to end", FilterParam::Kind::Text, "on", 0, 0, 1, {"on", "off"}}};
    rows.applies = is_ai;
    rows.counts_as = "own";
    rows.make = [own = rows.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& s = *find_filter(own);
        const int64_t run = param_int(s, v, "max_run"), period = param_int(s, v, "period");
        const std::string ramps = param_value(s, v, "ramps");
        return std::make_unique<WeightRows>(*l.ai, uint32_t(run), uint32_t(period), ramps == "on",
                                            "max_run=" + std::to_string(run) + " period=" + std::to_string(period) + " ramps=" + ramps);
    };
    out.push_back(rows);

    FilterSpec canon;
    canon.id = "canonical-model";
    canon.title = "canonical-model";
    canon.description = "One model of each class of the line's symmetries: the residual stream's dimensions, the feed-forward's "
                        "units, the heads and each head's value dimensions permuted, negated or turned without changing what the "
                        "model computes (up to the order of a sum). Of each class, the model whose every choice has the smallest "
                        "SHA-256 key, so it looks like any other. At Micro it keeps 1 in 2^484 of the line. Counted exactly; "
                        "judges (does not yet rank).";
    canon.applies = is_ai;
    canon.judge_only = true; // counted, but not ranked: its SHA-256 keys cannot be counted under a prefix
    canon.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<CanonicalModel>(*l.ai);
    };
    out.push_back(canon);

    FilterSpec lang;
    lang.id = "language";
    lang.title = "language";
    lang.description = "Speaks a run's language: the model's mean probability of each next byte of the run's text (the first "
                       "`bytes` of the first file it reads) is at least min_permille thousandths. Exact, on training-v1's "
                       "integer forward pass, the same on every machine; it judges (counting the models that speak is as "
                       "hard as the line is large).";
    lang.params = {{"run", "the training run whose text is the language (in the training folder)", FilterParam::Kind::Text,
                    "universalbibliothek", 0, 0, 1, {}},
                   {"bytes", "how many bytes of its text are read", FilterParam::Kind::Integer, "257", 2, 513, 1, {}},
                   {"min_permille", "the least mean probability of the right next byte, in thousandths", FilterParam::Kind::Integer,
                    "20", 0, 1000, 1, {}}};
    lang.applies = is_ai;
    lang.judge_only = true;
    lang.make = [own = lang.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& s = *find_filter(own);
        return std::make_unique<Language>(*l.ai, model_run(param_value(s, v, "run")), uint32_t(param_int(s, v, "bytes")),
                                          param_int(s, v, "min_permille"));
    };
    out.push_back(lang);

    FilterSpec near;
    near.id = "near-model";
    near.title = "near-model";
    near.description = "A layer round a trained model: at most `changes` of its digits differ from the model a run trained (the "
                       "anchor). Layer k holds C(W, k) (2^B - 1)^k models; the line kept is the anchor and its first layers, "
                       "outwards. Exact: counts and ranks (compact).";
    near.params = {{"run", "the training run whose model is the anchor (in the training folder)", FilterParam::Kind::Text,
                    "universalbibliothek", 0, 0, 1, {}},
                   {"changes", "how many digits may differ from the anchor's", FilterParam::Kind::Integer, "2", 0, 1000000, 1, {}}};
    near.applies = is_ai;
    near.counts_as = "own";
    near.make = [own = near.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& s = *find_filter(own);
        const auto run = model_run(param_value(s, v, "run"));
        const int64_t k = param_int(s, v, "changes");
        std::vector<uint32_t> anchor = anchor_of(*l.ai, *run, "near-model-v1");
        const std::string prov = "run=" + run->id + " anchor=" + run->sha256 + " changes=" + std::to_string(k);
        return std::make_unique<NearModel>(std::move(anchor), l.base, uint32_t(k), prov);
    };
    out.push_back(near);
}

} // namespace sieve
