// Statistical filters: max-run (text), symbol-entropy (every line), model-information (text).
// All decisions are exact integer comparisons (sieve/intlog.hpp).

#include "sieve/filter.hpp"
#include "sieve/intlog.hpp"

#include <algorithm>
#include <map>
#include <memory>
#include <stdexcept>

namespace sieve {

namespace {

// ---------------------------------------------------------------- max-run

// Fails if any symbol other than SPACE repeats more than max_run times in a row.
class MaxRun : public Filter
{
public:
    MaxRun(int64_t max_run, int64_t space) : max_(max_run), space_(space) { provenance_ = "max_run=" + std::to_string(max_run); }
    bool passes(std::span<const uint32_t> u) const override
    {
        int64_t run = 0;
        for (size_t i = 0; i < u.size(); ++i)
        {
            run = (i > 0 && u[i] == u[i - 1] && int64_t(u[i]) != space_) ? run + 1 : 1;
            if (run > max_) return false;
        }
        return true;
    }

private:
    int64_t max_, space_;
};

// ---------------------------------------------------------------- symbol-entropy

// Shannon entropy of the unit's own symbol frequencies, H = log2 L - (1/L) sum c log2 c, in
// bits per symbol. Exact form: with lg = log2_q16,
//   HL = L lg(L) - sum_c c lg(c)            (H * L in 1/65536 bits; at least 0)
//   passes iff  min * L * 65536 <= 1000 * HL <= max * L * 65536   (min, max in millibits)
class SymbolEntropy : public Filter
{
public:
    SymbolEntropy(int64_t min_mb, int64_t max_mb, uint32_t base, uint32_t length) : min_(min_mb), max_(max_mb)
    {
        provenance_ = "min_millibits=" + std::to_string(min_mb) + " max_millibits=" + std::to_string(max_mb);
        if (base == 2 && length <= kMaxBinaryRankLength) ranker_ = std::make_unique<BinaryRanker>(*this, length);
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        if (u.empty()) return true;
        std::map<uint32_t, uint64_t> counts;
        for (uint32_t s : u) ++counts[s];
        std::vector<uint64_t> c;
        for (const auto& [s, n] : counts) c.push_back(n);
        return passes_counts(c, u.size());
    }
    const Ranker* ranker() const override { return ranker_.get(); }

    // The test itself depends only on how often each symbol occurs.
    bool passes_counts(const std::vector<uint64_t>& counts, uint64_t L) const
    {
        if (L == 0) return true;
        // HL = L*lg(L) - sum c*lg(c), in BigUint (never negative in exact arithmetic; floors
        // can make it dip below zero by less than L, so it is clamped).
        BigUint plus = mul(BigUint(L), uint64_t(log2_q16(L))), minus;
        for (uint64_t c : counts)
            if (c) minus += mul(BigUint(c), uint64_t(log2_q16(c)));
        BigUint hl;
        if (plus >= minus)
        {
            hl = plus;
            hl -= minus;
        }
        BigUint lhs = hl;
        lhs.mul_small(1000);
        BigUint lo = mul(mul(BigUint(L), uint64_t(kLogOne)), uint64_t(min_));
        BigUint hi = mul(mul(BigUint(L), uint64_t(kLogOne)), uint64_t(max_));
        return lo <= lhs && lhs <= hi;
    }

private:
    // Two symbols (black-and-white pictures): the entropy depends only on how many 1s a unit
    // has, so the survivors are the units whose count of 1s is in an allowed set K, and
    //   completions(j ones so far, r left) = sum over k in K of C(r, k - j).
    // rank and unrank walk the unit once, keeping C(r, k - j) for every k in K and stepping each
    // to the next position with one small multiply and divide, instead of rebuilding binomial
    // rows at every position. Above this length compact is not offered (a hardware limit).
    static constexpr uint32_t kMaxBinaryRankLength = 2048;
    class BinaryRanker : public Ranker
    {
    public:
        BinaryRanker(const SymbolEntropy& f, uint32_t L) : L_(L), allowed_(L + 1)
        {
            for (uint32_t k = 0; k <= L; ++k) allowed_[k] = f.passes_counts({uint64_t(L - k), uint64_t(k)}, L);
            set_count();
        }
        uint32_t length() const override { return L_; }
        uint32_t base() const override { return 2; }
        State start() const override { return 0; }
        State next(State j, uint32_t c) const override { return c == 0 ? j : c == 1 ? j + 1 : kDead; }
        BigUint completions(State j, uint32_t r) const override
        {
            BigUint total, binom(1); // C(r, m), m = 0, 1, ...
            for (uint32_t m = 0; m <= r; ++m)
            {
                if (j + m <= L_ && allowed_[size_t(j + m)]) total += binom;
                if (m < r)
                {
                    binom.mul_small(r - m);
                    binom.divmod_small(m + 1);
                }
            }
            return total;
        }
        bool alive(State j, uint32_t r) const override
        {
            for (uint64_t k = j; k <= j + r && k <= L_; ++k)
                if (allowed_[size_t(k)]) return true;
            return false;
        }

        BigUint rank(std::span<const uint32_t> unit) const override
        {
            if (unit.size() != L_) throw std::invalid_argument("unit has the wrong length");
            Walk w(*this);
            BigUint k;
            for (uint32_t i = 0; i < L_; ++i)
            {
                if (unit[i] > 1) throw std::invalid_argument("unit is not a survivor");
                if (unit[i] == 1) k += w.sum(); // every survivor with a 0 here comes first
                w.step(unit[i]);
            }
            if (!allowed_[w.ones]) throw std::invalid_argument("unit is not a survivor");
            return k;
        }
        std::vector<uint32_t> unrank(const BigUint& k) const override
        {
            if (k >= count()) throw std::out_of_range("survivor number beyond the count");
            Walk w(*this);
            BigUint rest = k;
            std::vector<uint32_t> out(L_);
            for (uint32_t i = 0; i < L_; ++i)
            {
                const BigUint zeros = w.sum();
                if (rest >= zeros)
                {
                    rest -= zeros;
                    out[i] = 1;
                }
                w.step(out[i]);
            }
            return out;
        }

    private:
        // Before position i, with j ones so far: val[t] = C(r, K[t] - j), r = L - 1 - i (the
        // positions after i), so sum() counts the survivors that put a 0 at position i.
        struct Walk
        {
            explicit Walk(const BinaryRanker& b) : L(b.L_)
            {
                for (uint32_t k = 0; k <= L; ++k)
                    if (b.allowed_[k]) K.push_back(k);
                val.resize(K.size());
                if (L == 0) return;
                BigUint c(1); // C(L - 1, m)
                size_t t = 0;
                for (uint32_t m = 0; m <= L - 1 && t < K.size(); ++m)
                {
                    while (t < K.size() && K[t] < m) ++t;
                    if (t < K.size() && K[t] == m) val[t++] = c;
                    c.mul_small(L - 1 - m);
                    c.divmod_small(m + 1);
                }
            }
            BigUint sum() const
            {
                BigUint s;
                for (const auto& v : val) s += v;
                return s;
            }
            void step(uint32_t d)
            {
                const uint32_t r = L - 1 - i++;
                ones += d;
                if (r == 0) return;
                for (size_t t = 0; t < K.size(); ++t)
                {
                    if (val[t].is_zero()) continue; // zeros stay zero
                    const int64_t m = int64_t(K[t]) - int64_t(ones - d);
                    // d = 0: C(r - 1, m) = C(r, m) (r - m) / r;  d = 1: C(r - 1, m - 1) = C(r, m) m / r
                    val[t].mul_small(uint32_t(d == 0 ? r - m : m));
                    val[t].divmod_small(r);
                }
            }
            uint32_t L, i = 0, ones = 0;
            std::vector<uint32_t> K;
            std::vector<BigUint> val;
        };
        uint32_t L_;
        std::vector<bool> allowed_;
    };

    static BigUint mul(BigUint a, uint64_t b)
    {
        BigUint lo = a, hi = a;
        lo.mul_small(uint32_t(b & 0xFFFFFFFFu));
        hi.mul_small(uint32_t(b >> 32));
        hi <<= 32;
        lo += hi;
        return lo;
    }
    int64_t min_, max_;
    std::unique_ptr<Ranker> ranker_;
};

// ---------------------------------------------------------------- model-information

// The unit's information under the pinned model: the sum over its symbols of
// 16 - log2(f / 2^16) bits, each term taken as 16*65536 - lg(f) in 1/65536 bits.
//   passes iff  1000 * cost <= max * L * 65536   (max in millibits per symbol)
class ModelInformation : public Filter
{
public:
    ModelInformation(std::shared_ptr<const CharModel> m, int64_t max_mb, const std::string& id)
        : m_(std::move(m)), max_(max_mb)
    {
        provenance_ = "model=" + id + " sha256=" + m_->sha256() + " max_millibits=" + std::to_string(max_mb);
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        static const std::vector<int64_t> lg = [] {
            std::vector<int64_t> t(kModelTotal + 1, 0);
            for (uint32_t x = 1; x <= kModelTotal; ++x) t[x] = log2_q16(x);
            return t;
        }();
        const uint64_t full = uint64_t(kModelTotalBits) << kLogFractionBits;
        uint64_t cost = 0;
        for (size_t i = 0; i < u.size(); ++i)
        {
            const uint32_t* cum = m_->cumulative(u.first(i));
            cost += full - uint64_t(lg[cum[u[i] + 1] - cum[u[i]]]);
        }
        // Both sides fit in 64 bits for any length below 2^32 (max <= 16000 millibits).
        return cost * 1000 <= uint64_t(max_) * u.size() * uint64_t(kLogOne);
    }

private:
    std::shared_ptr<const CharModel> m_;
    int64_t max_;
};

bool is_text(const FilterLine& l) { return l.kind == "text" && l.alphabet != nullptr; }

} // namespace

void add_statistics_filters(std::vector<FilterSpec>& out)
{
    FilterSpec run;
    run.id = "max-run";
    run.title = "max-run";
    run.description = "No letter repeated more than max_run times in a row (English never exceeded 3 in the held-out books).";
    run.params = {{"max_run", "longest run of one letter allowed", FilterParam::Kind::Integer, "3", 1, 1000000, 1, {}}};
    run.applies = is_text;
    run.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        const auto space = l.alphabet->digit_of(U' ');
        return std::make_unique<MaxRun>(param_int(*find_filter("max-run-v1"), v, "max_run"), space ? int64_t(*space) : -1);
    };
    out.push_back(run);

    FilterSpec ent;
    ent.id = "symbol-entropy";
    ent.title = "symbol-entropy";
    ent.description = "Shannon entropy of the unit's own symbol frequencies, in bits per symbol, within [min, max]. "
                      "Separates text from noise on long units (English ~4.1, random letters ~4.7 at length 1000). "
                      "In black and white (at most 1 bit), low values keep mostly-one-colour pictures; can rank (compact) there.";
    ent.params = {{"min_millibits", "lowest entropy allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "0", 0, 32000, 50, {}},
                  {"max_millibits", "highest entropy allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "4400", 0, 32000, 50, {}}};
    ent.applies = [](const FilterLine&) { return true; };
    ent.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        const FilterSpec& s = *find_filter("symbol-entropy-v1");
        return std::make_unique<SymbolEntropy>(param_int(s, v, "min_millibits"), param_int(s, v, "max_millibits"), l.base, l.length);
    };
    out.push_back(ent);

    FilterSpec info;
    info.id = "model-information";
    info.title = "model-information";
    info.description = "Information content under the pinned frequency model, at most max bits per symbol. "
                       "English ~1.9-4.1, random letters 9 or more.";
    info.params = {{"model", "registered model id (empty: the alphabet's default)", FilterParam::Kind::Text, "", 0, 0, 1, {}},
                   {"max_millibits", "most information allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "5000", 1, 16000, 100, {}}};
    info.applies = is_text;
    info.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        const FilterSpec& s = *find_filter("model-information-v1");
        const std::string id = param_value(s, v, "model");
        auto m = r.model(id, l.symbols_id);
        if (m->base() != l.base) throw std::invalid_argument("model-information: the model has " + std::to_string(m->base()) +
                                                            " symbols, the line has " + std::to_string(l.base));
        return std::make_unique<ModelInformation>(m, param_int(s, v, "max_millibits"), id.empty() ? "default" : id);
    };
    out.push_back(info);
}

} // namespace sieve
