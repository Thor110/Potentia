#include "sieve/filter.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"

#include "sieve/audio.hpp"
#include "sieve/dfa.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sha256.hpp"
#include "sieve/written.hpp"

#include <algorithm>
#include <iterator>
#include <map>
#include <memory>
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
    // At most `states` different states asked for: room for them all at once, so the references
    // handed out stay valid.
    explicit StepCounts(size_t states) { seen_.reserve(states); }
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

// The symbols below `below` at one place, as runs of neighbours that lead to the same state (dead
// runs left out), into `out`: a large alphabet usually has few, so the big counts are touched once
// a run, not once a symbol.
struct Run
{
    uint32_t from = 0, n = 0;
    Ranker::State to = Ranker::kDead;
};
void runs_below(const Ranker& r, Ranker::State s, uint32_t below, std::vector<Run>& out)
{
    out.clear();
    for (uint32_t c = 0; c < below; ++c)
    {
        const Ranker::State t = r.next(s, c);
        if (t == Ranker::kDead) continue;
        if (!out.empty() && out.back().to == t && out.back().from + out.back().n == c) ++out.back().n;
        else out.push_back({c, 1, t});
    }
}

} // namespace

std::vector<uint32_t> Ranker::unrank(const BigUint& k0) const
{
    if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
    BigUint k = k0;
    const uint32_t L = length(), B = base();
    std::vector<uint32_t> u(L);
    std::vector<Run> runs;
    State s = start();
    for (uint32_t i = 0; i < L; ++i)
    {
        // Run by run: k falls in the run whose symbols' completions, n each, take it past k.
        runs_below(*this, s, B, runs);
        StepCounts counts(runs.size());
        bool placed = false;
        for (const Run& run : runs)
        {
            const BigUint& n = counts.get(*this, run.to, L - i - 1);
            if (run.n == 1)
            {
                if (k < n)
                {
                    u[i] = run.from;
                    s = run.to;
                    placed = true;
                    break;
                }
                k -= n;
                continue;
            }
            const BigUint all = BigUint::mul(n, BigUint(run.n));
            if (k < all)
            {
                BigUint q, r;
                BigUint::divmod(k, n, q, r);
                u[i] = run.from + uint32_t(q.limbs().empty() ? 0 : q.limbs()[0]); // q < run.n
                s = run.to;
                k = std::move(r);
                placed = true;
                break;
            }
            k -= all;
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
    std::vector<Run> runs;
    std::vector<std::pair<State, uint64_t>> below;
    State s = start();
    for (uint32_t i = 0; i < L; ++i)
    {
        if (unit[i] >= base()) throw std::invalid_argument("unit is not a survivor");
        // How many smaller symbols lead to each state, counted as plain numbers, then one product
        // each, rather than a big addition for every smaller symbol.
        runs_below(*this, s, unit[i], runs);
        below.clear();
        for (const Run& run : runs)
        {
            auto it = std::find_if(below.begin(), below.end(), [&](const auto& b) { return b.first == run.to; });
            if (it == below.end()) below.emplace_back(run.to, run.n);
            else it->second += run.n;
        }
        StepCounts counts(below.size());
        for (const auto& [t, n] : below)
        {
            const BigUint& c = counts.get(*this, t, L - i - 1);
            if (n == 1) k += c;
            else k += BigUint::mul(c, BigUint(n));
        }
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

// Several voices (or a sound's channels), one ranker: V copies of one voice's walk, one after
// another. A state is the voice, the place in it and the one voice's own state, packed into 64
// bits: as many as the voices and the places need, and the rest for the one voice's state.
class VoicesRanker : public Ranker
{
public:
    VoicesRanker(const Ranker& one, uint32_t voices) : one_(one), v_(voices), L_(one.length())
    {
        const auto bits = [](uint64_t n) { // bits to write 0..n
            uint32_t b = 0;
            while (b < 64 && (n >> b) != 0) ++b;
            return b;
        };
        vbits_ = bits(v_); // the voice runs to v_ (all of them done)
        pbits_ = bits(L_ > 0 ? L_ - 1 : 0);
        if (vbits_ + pbits_ >= 64) throw std::invalid_argument("too many voices or places to rank them together");
        ibits_ = 64 - vbits_ - pbits_;
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
        // per_ to the power of the voices after this one.
        return BigUint::mul(one_.completions(inner(s), left), BigUint::pow(per_, after));
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
    State pack(uint32_t v, uint32_t p, State inner) const
    {
        if (ibits_ < 64 && inner >= (State(1) << ibits_)) throw std::invalid_argument("a voice's ranker state is too large to rank several voices");
        return (State(v) << (pbits_ + ibits_)) | (State(p) << ibits_) | inner;
    }
    uint32_t voice(State s) const { return uint32_t(s >> (pbits_ + ibits_)); }
    uint32_t place(State s) const { return pbits_ ? uint32_t((s >> ibits_) & ((State(1) << pbits_) - 1)) : 0; }
    State inner(State s) const { return s & ((State(1) << ibits_) - 1); }

    const Ranker& one_;
    uint32_t v_, L_, vbits_ = 0, pbits_ = 0, ibits_ = 0;
    BigUint per_;
};

} // namespace

std::unique_ptr<Ranker> voices_ranker(const Ranker& one, uint32_t voices) { return std::make_unique<VoicesRanker>(one, voices); }

uint32_t line_voices(const FilterLine& l)
{
    if (l.kind != "audio") return 1;
    if (is_note_symbols(l.symbols_id)) return note_set_of(l.symbols_id).voices;
    if (is_pcm_symbols(l.symbols_id)) return pcm_format_of(l.symbols_id).channels;
    if (is_notes3_symbols(l.symbols_id)) return notes3_set_of(l.symbols_id).voices;
    return 1;
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

// The plugins' automata of a stack intersected into one (the units every one accepts), minimal.
// The smallest go in first: each step costs about the size of what has been merged so far, so
// merging the dictionaries' automata (nearly 300,000 states) last rather than first took 5.4 s
// instead of 13.3 s for every text plugin at 32 characters. Minimising makes the result the same
// automaton in any order, so ranks and counts do not depend on it.
//
// Results are kept for the process, by the line and the automata's provenance (with their
// counting tables: shared_table, below): the menu counts
// the same plugins for the text line and for the books' title, and again on every visit, and a
// large merge takes seconds. They are kept in up to merge_cache_share() of the filter memory (a
// half by default; the rest is for the counting tables built from them), and a stack that is
// counted on several threads at once is merged once, the others waiting for it.
std::mutex g_merged_mx;
std::map<std::string, std::shared_ptr<const Dfa>> g_merged;          // (g_merged_mx)
std::map<std::string, std::shared_ptr<std::mutex>> g_merging;        // one merge at a time per key
double g_merged_bytes = 0;                                           // (g_merged_mx)
// The merges' counting tables (shared_table, below), by merge key and then length: those still
// used by a stack, and those kept.
std::map<std::string, std::map<uint32_t, std::weak_ptr<const DfaRanker>>> g_tables_used;   // (g_merged_mx)
std::map<std::string, std::map<uint32_t, std::shared_ptr<const DfaRanker>>> g_tables_kept; // (g_merged_mx), in g_merged_bytes
std::map<std::string, std::shared_ptr<std::mutex>> g_tabling; // one build at a time per merge key

double dfa_bytes(const Dfa& d) { return double(d.next.size()) * sizeof(int32_t) + double(d.accept.size()); }

// Every unit passes: one accepting state that every symbol leads back to (nothing to intersect).
bool accepts_all(const Dfa& d)
{
    return d.states() == 1 && d.start == 0 && d.accept[0] && std::all_of(d.next.begin(), d.next.end(), [](int32_t t) { return t == 0; });
}

// `key`, if given, is set to what the result is kept under ("" when it is not kept: an automaton
// that does not say what it is), for its counting table to be kept with it (shared_table).
std::shared_ptr<const Dfa> merge_plugins(const std::vector<const Filter*>& filters, const std::string& line, std::string* key_out = nullptr)
{
    std::vector<std::pair<const Dfa*, const std::string*>> parts;
    for (const Filter* f : filters)
        if (const Dfa* d = plugin_dfa(*f)) parts.emplace_back(d, &f->provenance());
    if (parts.empty()) return nullptr;
    std::sort(parts.begin(), parts.end(), [](const auto& a, const auto& b) {
        return a.first->states() != b.first->states() ? a.first->states() < b.first->states() : *a.second < *b.second;
    });
    bool named = true;
    std::string key = line;
    for (const auto& [d, prov] : parts)
    {
        if (prov->empty()) named = false; // an automaton that does not say what it is is never shared
        key += "\n" + *prov;
    }
    auto merge = [&] {
        Dfa d = *parts[0].first;
        bool merged = false; // intersect minimises; a lone automaton is minimised here
        for (size_t i = 1; i < parts.size(); ++i)
            if (!accepts_all(*parts[i].first))
            {
                d = intersect(d, *parts[i].first);
                merged = true;
            }
        return std::make_shared<const Dfa>(merged ? std::move(d) : minimise(d));
    };
    if (!named) return merge();
    if (key_out) *key_out = key;
    std::shared_ptr<std::mutex> own;
    {
        std::lock_guard<std::mutex> lock(g_merged_mx);
        if (const auto it = g_merged.find(key); it != g_merged.end()) return it->second;
        auto& m = g_merging[key];
        if (!m) m = std::make_shared<std::mutex>();
        own = m;
    }
    std::lock_guard<std::mutex> merging(*own);
    {
        std::lock_guard<std::mutex> lock(g_merged_mx);
        if (const auto it = g_merged.find(key); it != g_merged.end()) return it->second; // merged meanwhile
    }
    auto d = merge();
    std::lock_guard<std::mutex> lock(g_merged_mx);
    const double bytes = dfa_bytes(*d);
    const double room = filter_memory() * merge_cache_share();
    if (g_merged_bytes + bytes > room) // (also when the share or the filter memory was lowered)
    {
        g_merged.clear(); // older stacks' merges and tables: made again if they are asked for
        g_tables_kept.clear();
        g_merged_bytes = 0;
    }
    if (bytes <= room)
    {
        g_merged[key] = d;
        g_merged_bytes += bytes;
    }
    g_merging.erase(key);
    return d;
}

// The counting table of a merged automaton at one length, shared. The pages line and the books'
// title count the same plugins at the same length, and building that table (every text plugin at
// 32 characters: 236,034 states, 99 MB packed) took a second each. A table is built once while
// any stack still uses it: a second stack asking meanwhile waits for it and uses the same one. A
// table answers every length up to its own (DfaRanker), so one at a longer length serves a
// shorter one too, with nothing built. It is also kept between counts, with the merges and in the
// same share of the filter memory (merge_cache_share), when it fits there. Read-only once built,
// so stacks on several threads share it safely. `key` is merge_plugins'; empty, the table is the
// asker's alone.
std::shared_ptr<const DfaRanker> shared_table(const std::string& merge_key, const std::shared_ptr<const Dfa>& d, uint32_t length)
{
    if (merge_key.empty()) return std::make_shared<const DfaRanker>(d, length);
    // The shortest table at this length or longer, as a ranker at this length (g_merged_mx held).
    auto find = [&]() -> std::shared_ptr<const DfaRanker> {
        std::shared_ptr<const DfaRanker> best;
        auto consider = [&](uint32_t len, std::shared_ptr<const DfaRanker> t) {
            if (t && len >= length && (!best || len < best->length())) best = std::move(t);
        };
        if (const auto it = g_tables_kept.find(merge_key); it != g_tables_kept.end())
            for (const auto& [len, t] : it->second) consider(len, t);
        if (const auto it = g_tables_used.find(merge_key); it != g_tables_used.end())
            for (const auto& [len, t] : it->second) consider(len, t.lock());
        if (!best || best->length() == length) return best;
        auto view = std::make_shared<const DfaRanker>(*best, length); // the longer table's first rows
        g_tables_used[merge_key][length] = view;
        return view;
    };
    std::shared_ptr<std::mutex> own;
    {
        std::lock_guard<std::mutex> lock(g_merged_mx);
        if (auto t = find()) return t;
        auto& m = g_tabling[merge_key];
        if (!m) m = std::make_shared<std::mutex>();
        own = m;
    }
    std::lock_guard<std::mutex> building(*own);
    {
        std::lock_guard<std::mutex> lock(g_merged_mx);
        if (auto t = find()) return t; // built meanwhile
    }
    auto t = std::make_shared<const DfaRanker>(d, length);
    std::lock_guard<std::mutex> lock(g_merged_mx);
    for (auto& [k, by_length] : g_tables_used) // (those no stack uses any more)
        for (auto it = by_length.begin(); it != by_length.end();)
            it = it->second.expired() ? by_length.erase(it) : std::next(it);
    g_tables_used[merge_key][length] = t;
    const double bytes = DfaRanker::table_bytes(d->states(), d->base, length);
    const double room = filter_memory() * merge_cache_share();
    if (g_merged_bytes + bytes > room)
    {
        g_merged.clear(); // what was kept before: made again if it is asked for
        g_tables_kept.clear();
        g_merged_bytes = 0;
    }
    if (bytes <= room)
    {
        g_tables_kept[merge_key][length] = t;
        g_merged_bytes += bytes;
    }
    return t;
}

} // namespace

FilterStack::FilterStack(const FilterLine& whole, const std::vector<Entry>& entries, const FilterResources& resources)
    : length_(whole.length)
{
    provenance_ = whole.kind + "/" + whole.symbols_id + "/L" + std::to_string(whole.length);
    symbols_id_ = whole.kind + "/" + whole.symbols_id;
    // Several voices: the filters are made for one voice and judge each (see the header).
    FilterLine line = whole;
    if (const uint32_t v = line_voices(whole); v > 1 || (whole.kind == "audio" && is_note_symbols(whole.symbols_id)))
    {
        voices_ = v;
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
        std::vector<const Filter*> raw;
        for (const auto& f : filters_) raw.push_back(f.get());
        // The line's symbols alone ("text/lower27"): not the filters in the order they were ticked
        // (the merge is the same in any order, and is kept by the automata themselves), and not
        // the length. An automaton filter's automaton never depends on the length (a plugin's is
        // compiled and kept without it, plugin.cpp), so the pages line at 32 characters and the
        // books' pages at 128 merge the same plugins once, and a table built at one length serves
        // the shorter ones (shared_table).
        const std::string& line_id = symbols_id_;
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
            std::string key;
            const std::shared_ptr<const Dfa> keep = merge_plugins(raw, line_id, &key);
            std::string why;
            own_ranker_ = written_ranker(written, keep.get(), line.length, why, &table_bytes_,
                                         [&] { return shared_table(key, keep, line.length); });
            if (own_ranker_) compact_ = own_ranker_.get();
            else blocker_ = why;
        }
        else if (all_plugins && written_count == 0)
        {
            std::string key;
            const std::shared_ptr<const Dfa> d = merge_plugins(raw, line_id, &key);
            const double bytes = DfaRanker::table_bytes(d->states(), d->base, line.length);
            table_bytes_ = bytes;
            if (bytes <= filter_memory())
            {
                shared_ranker_ = shared_table(key, d, line.length);
                compact_ = shared_ranker_.get();
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

std::string filter_conflict(const FilterSpec& a, const FilterSpec& b, const FilterLine* line, const FilterValues* va, const FilterValues* vb)
{
    if (a.name() == b.name() || a.counts_as.empty() || b.counts_as.empty()) return {};
    auto implies = [](const FilterSpec& x, const FilterSpec& y) {
        return std::find(x.implies.begin(), x.implies.end(), y.name()) != x.implies.end();
    };
    if (implies(a, b) || implies(b, a)) return {};
    auto how = [line](const FilterSpec& f, const FilterValues* v) { return line && f.counts_as_on ? f.counts_as_on(*line, v) : f.counts_as; };
    const std::string x = how(a, va);
    const std::string y = how(b, vb);
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
