#include "sieve/filter.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

std::vector<FilterSpec> builtin_filters(); // core/src/filters/builtin.cpp

const std::vector<FilterSpec>& filter_registry()
{
    static const std::vector<FilterSpec> list = [] {
        auto v = builtin_filters();
        for (size_t i = 0; i < v.size(); ++i)
            for (size_t j = 0; j < i; ++j)
                if (v[i].name() == v[j].name()) throw std::logic_error("filter registered twice: " + v[i].name());
        return v;
    }();
    return list;
}

const FilterSpec* find_filter(const std::string& name)
{
    const FilterSpec* newest = nullptr;
    for (const auto& f : filter_registry())
    {
        if (f.name() == name) return &f;
        if (f.id == name && (!newest || f.version > newest->version)) newest = &f;
    }
    return newest;
}

std::vector<const FilterSpec*> filters_for(const FilterLine& line)
{
    std::vector<const FilterSpec*> out;
    for (const auto& f : filter_registry())
        if (f.applies(line)) out.push_back(&f);
    return out;
}

std::string param_value(const FilterSpec& spec, const FilterValues& values, const std::string& key)
{
    for (const auto& p : spec.params)
        if (p.key == key)
        {
            const auto it = values.find(key);
            const std::string& v = it == values.end() ? p.default_value : it->second;
            if (!p.choices.empty() && std::find(p.choices.begin(), p.choices.end(), v) == p.choices.end())
            {
                std::string list;
                for (const auto& c : p.choices) list += (list.empty() ? "" : ", ") + c;
                throw std::invalid_argument(spec.name() + ": " + key + " must be one of " + list + ", got '" + v + "'");
            }
            return v;
        }
    throw std::logic_error(spec.name() + " has no parameter '" + key + "'");
}

int64_t param_int(const FilterSpec& spec, const FilterValues& values, const std::string& key)
{
    const std::string v = param_value(spec, values, key);
    size_t used = 0;
    int64_t n = 0;
    try { n = std::stoll(v, &used); } catch (const std::exception&) { used = 0; }
    if (v.empty() || used != v.size()) throw std::invalid_argument(spec.name() + ": " + key + " must be a whole number, got '" + v + "'");
    for (const auto& p : spec.params)
        if (p.key == key && (n < p.min || n > p.max))
            throw std::invalid_argument(spec.name() + ": " + key + " must be " + std::to_string(p.min) + ".." + std::to_string(p.max));
    return n;
}

// ---------------------------------------------------------------- Ranker

namespace {

// Within one step of a walk many symbols lead to the same state (every letter after a letter, in
// clean): their completion counts are computed once.
class StepCounts
{
public:
    explicit StepCounts(uint32_t base) { seen_.reserve(base); } // references stay valid: no regrowth
    const BigUint& get(const Ranker& r, Ranker::State t, uint32_t remaining)
    {
        for (const auto& [st, n] : seen_)
            if (st == t) return n;
        seen_.emplace_back(t, r.completions(t, remaining));
        return seen_.back().second;
    }

private:
    std::vector<std::pair<Ranker::State, BigUint>> seen_;
};

} // namespace

std::vector<uint32_t> Ranker::unrank(const BigUint& k0) const
{
    if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
    BigUint k = k0;
    const uint32_t L = length(), B = base();
    std::vector<uint32_t> u(L);
    State s = start();
    for (uint32_t i = 0; i < L; ++i)
    {
        StepCounts counts(B);
        bool placed = false;
        for (uint32_t c = 0; c < B && !placed; ++c)
        {
            const State t = next(s, c);
            if (t == kDead) continue;
            const BigUint& n = counts.get(*this, t, L - i - 1);
            if (k < n)
            {
                u[i] = c;
                s = t;
                placed = true;
            }
            else k -= n;
        }
        if (!placed) throw std::logic_error("ranker counts are inconsistent");
    }
    return u;
}

BigUint Ranker::rank(std::span<const uint32_t> unit) const
{
    const uint32_t L = length();
    if (unit.size() != L) throw std::invalid_argument("unit has the wrong length");
    BigUint k;
    State s = start();
    for (uint32_t i = 0; i < L; ++i)
    {
        if (unit[i] >= base()) throw std::invalid_argument("unit is not a survivor");
        StepCounts counts(base());
        for (uint32_t c = 0; c < unit[i]; ++c)
            if (const State t = next(s, c); t != kDead) k += counts.get(*this, t, L - i - 1);
        s = next(s, unit[i]);
        if (s == kDead) throw std::invalid_argument("unit is not a survivor");
    }
    if (!alive(s, 0)) throw std::invalid_argument("unit is not a survivor");
    return k;
}

bool Ranker::accepts(std::span<const uint32_t> unit) const
{
    if (unit.size() != length()) return false;
    State s = start();
    for (uint32_t c : unit)
    {
        if (c >= base()) return false;
        s = next(s, c);
        if (s == kDead) return false;
    }
    return alive(s, 0);
}

namespace {

// Entry j is implied by entry i when i's spec lists j's name among its implications and they agree
// on every parameter they share (words with one dictionary does not imply window with another).
bool implied_by(const FilterStack::Entry& i, const FilterStack::Entry& j)
{
    const auto& imp = i.spec->implies;
    if (std::find(imp.begin(), imp.end(), j.spec->name()) == imp.end()) return false;
    for (const auto& pj : j.spec->params)
        for (const auto& pi : i.spec->params)
            if (pi.key == pj.key && param_value(*i.spec, i.values, pi.key) != param_value(*j.spec, j.values, pj.key)) return false;
    return true;
}

} // namespace

FilterStack::FilterStack(const FilterLine& line, const std::vector<Entry>& entries, const FilterResources& resources)
    : length_(line.length)
{
    provenance_ = line.kind + "/" + line.symbols_id + "/L" + std::to_string(line.length);
    for (const auto& e : entries)
    {
        if (!e.spec->applies(line)) throw std::invalid_argument(e.spec->name() + " does not apply to the " + line.kind + " line");
        filters_.push_back(e.spec->make(line, e.values, resources));
        names_.push_back(e.spec->name());
        provenance_ += "; " + e.spec->name() + "{" + filters_.back()->provenance() + "}";
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));

    // Compact: one filter with a ranker whose survivors are exactly the stack's.
    for (size_t i = 0; i < filters_.size() && !compact_; ++i)
    {
        if (!filters_[i]->ranker()) continue;
        bool covers = true;
        for (size_t j = 0; j < entries.size(); ++j)
            if (j != i && !implied_by(entries[i], entries[j])) covers = false;
        if (covers) compact_ = filters_[i]->ranker();
    }
    if (filters_.empty()) blocker_ = "no filters ticked";
    else if (!compact_)
    {
        // Name the first ranking filter and the ticked filters it does not imply.
        size_t r = filters_.size();
        for (size_t i = 0; i < filters_.size() && r == filters_.size(); ++i)
            if (filters_[i]->ranker()) r = i;
        if (r == filters_.size()) blocker_ = "none of the ticked filters can rank its survivors at this size";
        else
        {
            std::string extra;
            for (size_t j = 0; j < names_.size(); ++j)
                if (j != r && !implied_by(entries[r], entries[j])) extra += (extra.empty() ? "" : ", ") + names_[j];
            blocker_ = names_[r] + " can rank, but not together with " + extra;
        }
    }
}

int FilterStack::first_failure(std::span<const uint32_t> unit) const
{
    if (filters_.empty()) return -1;
    if (unit.size() != length_) throw std::invalid_argument("unit has the wrong length for this filter stack");
    for (size_t i = 0; i < filters_.size(); ++i)
        if (!filters_[i]->passes(unit)) return int(i);
    return -1;
}

std::string FilterStack::compact_blocker() const { return blocker_; }

} // namespace sieve
