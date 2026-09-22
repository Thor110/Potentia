// Statistical filters: max-run (text), symbol-entropy (every line), model-information (text).
// All decisions are exact integer comparisons (sieve/intlog.hpp).

#include "sieve/filter.hpp"
#include "sieve/intlog.hpp"

#include <algorithm>
#include <map>
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
    SymbolEntropy(int64_t min_mb, int64_t max_mb) : min_(min_mb), max_(max_mb)
    {
        provenance_ = "min_millibits=" + std::to_string(min_mb) + " max_millibits=" + std::to_string(max_mb);
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        if (u.empty()) return true;
        std::map<uint32_t, uint64_t> counts;
        for (uint32_t s : u) ++counts[s];
        const uint64_t L = u.size();
        // HL = L*lg(L) - sum c*lg(c), in BigUint (never negative in exact arithmetic; floors
        // can make it dip below zero by less than L, so it is clamped).
        BigUint plus = mul(BigUint(L), uint64_t(log2_q16(L))), minus;
        for (const auto& [s, c] : counts) minus += mul(BigUint(c), uint64_t(log2_q16(c)));
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
    run.params = {{"max_run", "longest run of one letter allowed", FilterParam::Kind::Integer, "3", 1, 1000000, 1}};
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
                      "Separates text from noise on long units (English ~4.1, random letters ~4.7 at length 1000).";
    ent.params = {{"min_millibits", "lowest entropy allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "0", 0, 32000, 50},
                  {"max_millibits", "highest entropy allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "4400", 0, 32000, 50}};
    ent.applies = [](const FilterLine&) { return true; };
    ent.make = [](const FilterLine&, const FilterValues& v, const FilterResources&) {
        const FilterSpec& s = *find_filter("symbol-entropy-v1");
        return std::make_unique<SymbolEntropy>(param_int(s, v, "min_millibits"), param_int(s, v, "max_millibits"));
    };
    out.push_back(ent);

    FilterSpec info;
    info.id = "model-information";
    info.title = "model-information";
    info.description = "Information content under the pinned frequency model, at most max bits per symbol. "
                       "English ~1.9-4.1, random letters 9 or more.";
    info.params = {{"model", "registered model id (empty: the alphabet's default)", FilterParam::Kind::Text, "", 0, 0, 1},
                   {"max_millibits", "most information allowed, in 1/1000 bits per symbol", FilterParam::Kind::Integer, "5000", 1, 16000, 100}};
    info.applies = is_text;
    info.make = [](const FilterLine& l, const FilterValues& v, const FilterResources& r) {
        const FilterSpec& s = *find_filter("model-information-v1");
        const std::string id = param_value(s, v, "model");
        auto m = r.model(id, l.symbols_id);
        return std::make_unique<ModelInformation>(m, param_int(s, v, "max_millibits"), id.empty() ? "default" : id);
    };
    out.push_back(info);
}

} // namespace sieve
