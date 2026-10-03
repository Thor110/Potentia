#include "sieve/filter.hpp"

#include "sieve/audio.hpp"
#include "sieve/dfa.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"
#include "sieve/written.hpp"

#include <algorithm>
#include <mutex>
#include <optional>
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

// The built-in filters, then the plugins (sieve/plugin.hpp).
const FilterSpec* find_filter(const std::string& name)
{
    const FilterSpec* newest = nullptr;
    auto look = [&](const auto& list) -> const FilterSpec* {
        for (const auto& f : list)
        {
            if (f.name() == name) return &f;
            if (f.id == name && (!newest || f.version > newest->version)) newest = &f;
        }
        return nullptr;
    };
    if (const FilterSpec* f = look(filter_registry())) return f;
    if (const FilterSpec* f = look(plugin_registry())) return f;
    return newest;
}

std::vector<const FilterSpec*> filters_for(const FilterLine& line)
{
    std::vector<const FilterSpec*> out;
    for (const auto& f : filter_registry())
        if (f.applies(line)) out.push_back(&f);
    for (const auto& f : plugin_registry())
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

// Several voices, one ranker: V copies of one voice's walk, one after another. A state is the
// voice, the place in it and the one voice's own state, packed into 64 bits (3, 16 and 45 bits),
// which every ranker a note line has (their states are automata's states) fits.
class VoicesRanker : public Ranker
{
public:
    VoicesRanker(const Ranker& one, uint32_t voices) : one_(one), v_(voices), L_(one.length())
    {
        if (L_ >= (1u << 16)) throw std::invalid_argument("a voice of 65536 or more events is too long to rank");
        per_ = one_.count();
        set_count();
    }
    uint32_t length() const override { return v_ * L_; }
    uint32_t base() const override { return one_.base(); }
    State start() const override { return pack(0, 0, one_.start()); }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead) return kDead;
        const uint32_t v = voice(s), p = place(s);
        if (v >= v_) return kDead;
        const State n = one_.next(inner(s), symbol);
        if (n == kDead) return kDead;
        if (p + 1 < L_) return pack(v, p + 1, n);
        // The voice is complete: it must be a survivor on its own, and the next voice starts.
        if (one_.completions(n, 0).is_zero()) return kDead;
        return pack(v + 1, 0, v + 1 < v_ ? one_.start() : 0);
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        const uint32_t v = voice(s), p = place(s);
        if (v >= v_) return remaining == 0 ? BigUint(1) : BigUint();
        const uint32_t left = L_ - p, after = v_ - 1 - v;
        if (uint64_t(remaining) != uint64_t(left) + uint64_t(after) * L_) return BigUint();
        BigUint c = one_.completions(inner(s), left);
        for (uint32_t i = 0; i < after; ++i) c = BigUint::mul(c, per_);
        return c;
    }
    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
        std::vector<BigUint> parts(v_);
        BigUint k = k0;
        for (uint32_t i = v_; i-- > 0;)
        {
            BigUint q, r;
            BigUint::divmod(k, per_, q, r);
            parts[i] = r;
            k = q;
        }
        std::vector<uint32_t> out;
        out.reserve(size_t(v_) * L_);
        for (const BigUint& part : parts)
        {
            const auto u = one_.unrank(part);
            out.insert(out.end(), u.begin(), u.end());
        }
        return out;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != size_t(v_) * L_) throw std::invalid_argument("unit has the wrong length");
        BigUint k;
        for (uint32_t i = 0; i < v_; ++i)
        {
            k = BigUint::mul(k, per_);
            k += one_.rank(unit.subspan(size_t(i) * L_, L_));
        }
        return k;
    }

private:
    static constexpr uint64_t kInnerBits = 45;
    static State pack(uint32_t v, uint32_t p, State inner)
    {
        if (inner >= (State(1) << kInnerBits)) throw std::invalid_argument("a voice's ranker state is too large to rank several voices");
        return (State(v) << 61) | (State(p) << kInnerBits) | inner;
    }
    static uint32_t voice(State s) { return uint32_t(s >> 61); }
    static uint32_t place(State s) { return uint32_t((s >> kInnerBits) & 0xFFFF); }
    static State inner(State s) { return s & ((State(1) << kInnerBits) - 1); }

    const Ranker& one_;
    uint32_t v_, L_;
    BigUint per_;
};

} // namespace

std::unique_ptr<Ranker> voices_ranker(const Ranker& one, uint32_t voices) { return std::make_unique<VoicesRanker>(one, voices); }

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

FilterStack::FilterStack(const FilterLine& whole, const std::vector<Entry>& entries, const FilterResources& resources)
    : length_(whole.length)
{
    provenance_ = whole.kind + "/" + whole.symbols_id + "/L" + std::to_string(whole.length);
    // Several voices: the filters are made for one voice and judge each (see the header).
    FilterLine line = whole;
    if (whole.kind == "audio" && is_note_symbols(whole.symbols_id))
    {
        voices_ = note_set_of(whole.symbols_id).voices;
        if (whole.length % voices_ != 0) throw std::invalid_argument("a unit of " + std::to_string(voices_) + " voices has a length they divide");
        line.length = whole.length / voices_;
        if (voices_ > 1) provenance_ += " (each of " + std::to_string(voices_) + " voices)";
    }
    for (const auto& e : entries)
    {
        if (!e.spec->applies(line)) throw std::invalid_argument(e.spec->name() + " does not apply to the " + line.kind + " line");
        filters_.push_back(e.spec->make(line, e.values, resources));
        names_.push_back(e.spec->name());
        provenance_ += "; " + e.spec->name() + "{" + filters_.back()->provenance() + "}";
    }
    id_ = Sha256::hex(Sha256::hash(provenance_));

    entries_ = entries;
    unit_length_ = line.length;
    lazy_ = std::make_unique<std::once_flag>();
}

// Compact is worked out the first time it is asked for (ranker(), compact_blocker()): combining
// large automata, and building their counting tables, takes seconds and hundreds of megabytes,
// and a stack that only marks or hides never needs it.
void FilterStack::settle() const
{
    if (lazy_) std::call_once(*lazy_, [this] { settle_now(); }); // (a default stack has nothing to settle)
}

void FilterStack::settle_now() const
{
    const std::vector<Entry>& entries = entries_;
    struct
    {
        uint32_t length;
    } line{unit_length_};
    // Compact: one filter with a ranker whose survivors are exactly the stack's.
    for (size_t i = 0; i < filters_.size() && !compact_; ++i)
    {
        if (!filters_[i]->can_rank()) continue;
        bool covers = true;
        for (size_t j = 0; j < entries.size(); ++j)
            if (j != i && !implied_by(entries[i], entries[j])) covers = false;
        if (covers)
        {
            compact_ = filters_[i]->ranker(); // built here, only for the filter that is used
            table_bytes_ = filters_[i]->table_bytes();
        }
    }
    // A stack of plugins only: their automata combined into one (the units every one accepts),
    // which ranks the stack exactly when its table fits. With not-written-v1 among them, its rule
    // counts the units the plugins keep that it keeps too (sieve/written.hpp).
    if (!compact_ && filters_.size() > 1)
    {
        bool all_plugins = true;
        std::shared_ptr<const WrittenRule> written;
        size_t written_count = 0;
        for (const auto& f : filters_)
            if (auto w = written_rule_of(*f))
            {
                written = w;
                ++written_count;
            }
            else if (!plugin_dfa(*f)) all_plugins = false;
        if (all_plugins && written_count == 1)
        {
            std::optional<Dfa> keep;
            for (const auto& f : filters_)
                if (const Dfa* d = plugin_dfa(*f)) keep = keep ? intersect(*keep, *d) : *d;
            std::string why;
            own_ranker_ = written_ranker(written, keep ? &*keep : nullptr, line.length, why, &table_bytes_);
            if (own_ranker_) compact_ = own_ranker_.get();
            else blocker_ = why;
        }
        else if (all_plugins && written_count == 0)
        {
            Dfa d = *plugin_dfa(*filters_[0]);
            for (size_t i = 1; i < filters_.size(); ++i) d = intersect(d, *plugin_dfa(*filters_[i]));
            const double bytes = DfaRanker::table_bytes(d.states(), d.base, line.length);
            table_bytes_ = bytes;
            if (bytes <= filter_memory())
            {
                own_ranker_ = std::make_unique<DfaRanker>(d, line.length);
                compact_ = own_ranker_.get();
            }
            else blocker_ = over_table_limit("the plugins' combined table", bytes) + ": they judge only";
        }
    }
    if (compact_ && voices_ > 1)
    {
        voices_ranker_ = std::make_unique<VoicesRanker>(*compact_, voices_);
        compact_ = voices_ranker_.get();
    }
    if (filters_.empty()) blocker_ = "no filters ticked";
    else if (!compact_ && blocker_.empty()) // (a stack of plugins over the budget has said why already)
    {
        // Name the first ranking filter and the ticked filters it does not imply.
        size_t r = filters_.size();
        for (size_t i = 0; i < filters_.size() && r == filters_.size(); ++i)
            if (filters_[i]->can_rank()) r = i;
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
    const size_t per = length_ / voices_;
    for (size_t i = 0; i < filters_.size(); ++i)
        for (uint32_t v = 0; v < voices_; ++v)
            if (!filters_[i]->passes(unit.subspan(v * per, per))) return int(i);
    return -1;
}

std::string filter_conflict(const FilterSpec& a, const FilterSpec& b)
{
    if (a.name() == b.name() || a.counts_as.empty() || b.counts_as.empty()) return {};
    auto implies = [](const FilterSpec& x, const FilterSpec& y) {
        return std::find(x.implies.begin(), x.implies.end(), y.name()) != x.implies.end();
    };
    if (implies(a, b) || implies(b, a)) return {};
    const std::string& x = a.counts_as;
    const std::string& y = b.counts_as;
    auto pair = [&](const char* p, const char* q) { return (x == p && y == q) || (x == q && y == p); };
    if (pair("automaton", "automaton") || pair("automaton", "written") || pair("model-rule", "model-rule")) return {};
    if (x == "arithmetic" || y == "arithmetic") return "conflict";
    return "merge";
}

std::string FilterStack::compact_blocker() const
{
    settle();
    return blocker_;
}

double FilterStack::table_bytes() const
{
    settle();
    // The one filter that ranks the stack, or the plugins merged (with not-written or without), as
    // worked out in settle; where nothing ranks it, the largest any filter's table would need.
    if (table_bytes_ > 0 || compact_) return table_bytes_;
    double most = 0;
    for (const auto& f : filters_) most = std::max(most, f->table_bytes());
    return most;
}

const Ranker* FilterStack::ranker() const
{
    settle();
    return compact_;
}

} // namespace sieve
