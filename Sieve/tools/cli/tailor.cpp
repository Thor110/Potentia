// Sieve CLI -- a line's filters tailored to one item (tailor.hpp).
#include "cli/tailor.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <optional>

namespace sieve::cli {

namespace {

constexpr double kBitsPerDigit = 3.321928094887362; // log2(10)

// How many values of an integer setting are tried at once: evenly spread over the range, then
// over the stretch between the best one's neighbours, until the values are next to each other.
constexpr int64_t kGrid = 7;

// A stack judged against the item: whether the item passes it, and if so log2 of its survivors
// (-1 where it cannot be counted, with why).
struct Verdict
{
    bool passes = false;
    double bits = -1;
    std::string blocker;
};

Verdict judge(const FilterLine& line, const LineFilters& lf, std::span<const uint32_t> unit)
{
    Verdict v;
    try
    {
        const FilterStack st = build_stack(line, lf);
        if (!st.passes(unit)) return v;
        v.passes = true;
        if (const Ranker* r = st.ranker()) v.bits = r->count().log10_approx() * kBitsPerDigit;
        else v.blocker = st.compact_blocker();
    }
    catch (const std::exception& e)
    {
        v.passes = false;
        v.blocker = e.what();
    }
    return v;
}

// `name` alone at `values`, with what ticking it ticks too (its prerequisites, at the settings
// they pin, or those in `base`).
LineFilters alone(const std::string& name, const FilterValues& values, const LineFilters& base)
{
    LineFilters one;
    one.mode = FilterMode::Compact;
    one.values = base.values;
    one.values[name] = values;
    (void)tick_filter(one, name, true);
    return one;
}

// Better: the item passes, it can be counted, and it leaves fewer survivors.
bool better(const Verdict& v, const std::optional<Verdict>& best)
{
    return v.passes && v.bits >= 0 && (!best || v.bits < best->bits);
}

// Values of an integer setting from `lo` to `hi` that `step` reaches from `origin`: all of them
// when there are few, else kGrid spread evenly, both ends included.
std::vector<int64_t> grid(int64_t lo, int64_t hi, int64_t step, int64_t origin)
{
    std::vector<int64_t> out;
    if (hi < lo || step <= 0) return out;
    const int64_t first = origin + (lo - origin + step - 1) / step * step; // the first reachable value >= lo
    if (first > hi) return out;
    const int64_t n = (hi - first) / step + 1;
    if (n <= 2 * kGrid)
    {
        for (int64_t i = 0; i < n; ++i) out.push_back(first + i * step);
        return out;
    }
    for (int64_t i = 0; i < kGrid; ++i) out.push_back(first + (n - 1) * i / (kGrid - 1) * step);
    out.erase(std::unique(out.begin(), out.end()), out.end());
    return out;
}

// The ids of the registered dictionaries (none if the registry cannot be found).
const std::vector<std::string>& dictionary_ids()
{
    static const std::vector<std::string> ids = [] {
        std::vector<std::string> out;
        try
        {
            for (const DictionaryEntry& e : load_registry().entries) out.push_back(e.id);
        }
        catch (const std::exception&)
        {
        }
        return out;
    }();
    return ids;
}

} // namespace

TailorResult tailor_filters(const FilterLine& line, std::span<const uint32_t> unit, const LineFilters& current, TailorProgress* progress)
{
    TailorResult result;
    result.line_bits = double(line.length) * std::log2(double(line.base));
    result.bits = result.line_bits;
    result.filters.mode = FilterMode::Compact;
    auto cancelled = [&] { return progress && progress->cancel.load(); };

    // The filters that could count: not retired, and not one that judges only wherever it is used.
    std::vector<const FilterSpec*> specs;
    for (const FilterSpec* f : filters_for(line))
    {
        if (f->retired) continue;
        const std::string how = f->counts_as_on ? f->counts_as_on(line, nullptr) : f->counts_as;
        if (how.empty() && f->counts_as.empty()) continue;
        specs.push_back(f);
    }
    if (progress) progress->total = int(specs.size());

    // 1. Each filter on its own, its settings searched one at a time.
    for (const FilterSpec* f : specs)
    {
        if (cancelled()) break;
        if (progress)
        {
            std::lock_guard<std::mutex> lock(progress->mx);
            progress->current = f->name();
        }
        TailorChoice c;
        c.name = f->name();
        c.values = current.values_of(c.name);
        std::optional<Verdict> best;
        std::map<FilterValues, Verdict> seen;
        auto trial = [&](const FilterValues& values) -> const Verdict& {
            auto it = seen.find(values);
            if (it == seen.end()) it = seen.emplace(values, judge(line, alone(c.name, values, current), unit)).first;
            return it->second;
        };
        auto consider = [&](const FilterValues& values) {
            const Verdict& v = trial(values);
            if (better(v, best))
            {
                best = v;
                c.values = values;
            }
        };
        consider(c.values);
        for (const FilterParam& p : f->params)
        {
            if (cancelled()) break;
            if (p.kind == FilterParam::Kind::Integer)
            {
                // Over the whole range first, then between the best value's neighbours, closer each time.
                int64_t lo = p.min, hi = p.max;
                const int64_t step = std::max<int64_t>(1, p.step), origin = p.min;
                for (;;)
                {
                    const std::vector<int64_t> values = grid(lo, hi, step, origin);
                    if (values.empty() || cancelled()) break;
                    for (const int64_t x : values)
                    {
                        FilterValues v = c.values;
                        v[p.key] = std::to_string(x);
                        consider(v);
                    }
                    if (values.size() < size_t(kGrid)) break; // every value in the stretch was tried
                    // The stretch around the best value, if it is in this one; else none closer.
                    int64_t at = 0;
                    try
                    {
                        at = param_int(*f, c.values, p.key);
                    }
                    catch (const std::exception&)
                    {
                        break;
                    }
                    const auto pos = std::find(values.begin(), values.end(), at);
                    if (pos == values.end()) break;
                    const int64_t nlo = pos == values.begin() ? *pos : *(pos - 1), nhi = pos + 1 == values.end() ? *pos : *(pos + 1);
                    if (nlo == lo && nhi == hi) break;
                    lo = nlo;
                    hi = nhi;
                }
            }
            else
            {
                const std::vector<std::string>& choices = !p.choices.empty() ? p.choices
                                                          : p.registry == "dictionary" || p.key == "dictionary" ? dictionary_ids()
                                                                                                                : p.choices;
                for (const std::string& x : choices)
                {
                    if (cancelled()) break;
                    FilterValues v = c.values;
                    v[p.key] = x;
                    consider(v);
                }
            }
        }
        if (best) c.bits = best->bits;
        else
        {
            const Verdict& at = trial(c.values);
            c.why_not = !at.passes ? "the item fails it at every setting tried" : "cannot be counted: " + at.blocker;
        }
        result.choices.push_back(std::move(c));
        if (progress) ++progress->done;
    }
    if (cancelled())
    {
        result.finished = false;
        return result;
    }

    // 2. The set. From each filter kept, strongest first, a stack is grown greedily: every other
    // filter, strongest first, is added where it can be counted with those there and removes more.
    // A filter that counts its own way clashes with the rest, so it makes a stack of one, and the
    // automata, which merge, make another: the best of the stacks grown is kept. A filter already in
    // a stack grown earlier starts none of its own (it would grow the same stack again).
    std::vector<size_t> order;
    for (size_t i = 0; i < result.choices.size(); ++i)
        if (result.choices[i].why_not.empty()) order.push_back(i);
    std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) { return result.choices[a].bits < result.choices[b].bits; });
    struct Grown
    {
        LineFilters stack;
        double bits;
        std::vector<size_t> used;
        std::map<size_t, std::string> why_not; // why each other filter was left out of it
    };
    auto grow = [&](size_t seed) {
        Grown g{LineFilters{}, result.line_bits, {}, {}};
        g.stack.mode = FilterMode::Compact;
        g.stack.values = current.values;
        std::vector<size_t> sequence{seed};
        for (const size_t i : order)
            if (i != seed) sequence.push_back(i);
        for (const size_t i : sequence)
        {
            if (cancelled()) break;
            const TailorChoice& c = result.choices[i];
            const FilterSpec* spec = find_filter(c.name);
            std::string why;
            for (const std::string& e : g.stack.enabled)
            {
                const FilterSpec* other = find_filter(e);
                const FilterValues& ov = g.stack.values_of(e);
                if (spec && other && !filter_conflict(*spec, *other, &line, &c.values, &ov).empty())
                {
                    why = "cannot be counted with " + e;
                    break;
                }
            }
            if (why.empty())
            {
                LineFilters with = g.stack;
                with.values[c.name] = c.values;
                (void)tick_filter(with, c.name, true);
                const Verdict v = judge(line, with, unit);
                if (!v.passes) why = "the item fails it with the others";
                else if (v.bits < 0) why = "cannot be counted with the others: " + v.blocker;
                else if (v.bits >= g.bits - 1e-9) why = "removes nothing the others do not";
                else
                {
                    g.stack = std::move(with);
                    g.bits = v.bits;
                    g.used.push_back(i);
                }
            }
            if (!why.empty()) g.why_not[i] = why;
        }
        return g;
    };
    std::optional<Grown> best;
    std::vector<bool> grown(result.choices.size(), false);
    for (const size_t seed : order)
    {
        if (cancelled()) break;
        if (grown[seed]) continue;
        Grown g = grow(seed);
        for (const size_t i : g.used) grown[i] = true;
        if (!best || g.bits < best->bits) best = std::move(g);
    }
    if (best)
    {
        result.filters = std::move(best->stack);
        result.bits = best->bits;
        for (const size_t i : best->used) result.choices[i].used = true;
        for (const auto& [i, why] : best->why_not) result.choices[i].why_not = why;
    }
    LineFilters& stack = result.filters;
    // Only the settings of what is ticked are kept.
    for (auto it = stack.values.begin(); it != stack.values.end();)
        it = stack.is_enabled(it->first) ? std::next(it) : stack.values.erase(it);
    result.finished = !cancelled();
    if (progress) progress->done = progress->total + 1;
    return result;
}

} // namespace sieve::cli
