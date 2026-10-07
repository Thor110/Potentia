// Sieve CLI -- a line's filters tailored to the items that must keep their places (tailor.hpp).
#include "cli/tailor.hpp"

#include "cli/dictionaries.hpp"
#include "sieve/corridor.hpp"

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

// The places a number's shortest route is worked out exactly; past them, its digits are taken.
constexpr uint64_t kRouteBits = 1u << 16;

// A stack judged against the anchors: whether every one passes it, and if so its score (see
// tailor.hpp; -1 where it cannot be counted, with why) and the parts of it.
struct Verdict
{
    bool passes = false;
    double score = -1, route = 0, count = 0, description = 0;
    std::string blocker;
};

Verdict judge(const FilterLine& line, const LineFilters& lf, const std::vector<std::vector<uint32_t>>& anchors, const TailorOptions& options)
{
    Verdict v;
    try
    {
        const FilterStack st = build_stack(line, lf);
        for (const auto& a : anchors)
            if (!st.passes(a)) return v;
        v.passes = true;
        const Ranker* r = st.ranker();
        if (!r)
        {
            v.blocker = st.compact_blocker();
            return v;
        }
        v.count = r->count().log10_approx() * kBitsPerDigit;
        for (const auto& a : anchors) v.route += route_bits(r->rank(a), r->count());
        v.description = description_bits(line, lf);
        v.score = v.route + (options.count_description ? v.description : 0.0);
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

// Better: every anchor passes, it can be counted, and it scores less.
bool better(const Verdict& v, const std::optional<Verdict>& best)
{
    return v.passes && v.score >= 0 && (!best || v.score < best->score);
}

// Whether a filter can count anywhere on the line (the ones the search tries, and the mask prices).
bool can_count_on(const FilterSpec& f, const FilterLine& line)
{
    if (f.retired) return false;
    const std::string how = f.counts_as_on ? f.counts_as_on(line, nullptr) : f.counts_as;
    return !(how.empty() && f.counts_as.empty());
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

double route_bits(const BigUint& number, const BigUint& count)
{
    if (count.bit_length() > kRouteBits) return number.is_zero() ? 0.0 : 4.0 * double(number.to_hex().size());
    return shortest_path(number, count, 20).bits;
}

double description_bits(const FilterLine& line, const LineFilters& stack)
{
    if (stack.enabled.empty()) return 0;
    double bits = 0;
    for (const FilterSpec* f : filters_for(line))
        if (can_count_on(*f, line)) bits += 1; // the mask
    for (const std::string& name : stack.enabled)
    {
        const FilterSpec* f = find_filter(name);
        if (!f) continue;
        for (const FilterParam& p : f->params)
        {
            if (p.kind == FilterParam::Kind::Integer) bits += std::log2(double((p.max - p.min) / std::max<int64_t>(1, p.step) + 1));
            else if (!p.choices.empty()) bits += std::log2(double(p.choices.size()));
            else if (p.registry == "dictionary" || p.key == "dictionary") bits += std::log2(double(std::max<size_t>(1, dictionary_ids().size())));
            else bits += 8.0 * double(param_value(*f, stack.values_of(name), p.key).size());
        }
    }
    return bits;
}

TailorResult tailor_filters(const FilterLine& line, std::span<const uint32_t> unit, const LineFilters& current, TailorProgress* progress,
                            const TailorOptions& options)
{
    return tailor_filters(line, std::vector<std::vector<uint32_t>>{std::vector<uint32_t>(unit.begin(), unit.end())}, current, progress, options);
}

TailorResult tailor_filters(const FilterLine& line, const std::vector<std::vector<uint32_t>>& anchors, const LineFilters& current,
                            TailorProgress* progress, const TailorOptions& options)
{
    TailorResult result;
    // The base: each anchor's shortest route on the line as it is, its positional index among them all.
    const BigUint size = BigUint::pow(line.base, line.length);
    for (const auto& a : anchors) result.line_bits += route_bits(BigUint::from_digits(a, line.base), size);
    result.bits = result.route_bits = result.line_bits;
    result.count_bits = size.log10_approx() * kBitsPerDigit;
    result.filters.mode = FilterMode::Compact;
    auto cancelled = [&] { return progress && progress->cancel.load(); };

    // The filters that could count: not retired, and not one that judges only wherever it is used.
    std::vector<const FilterSpec*> specs;
    for (const FilterSpec* f : filters_for(line))
        if (can_count_on(*f, line)) specs.push_back(f);
    if (progress) progress->total = int(specs.size());

    // 1. Each filter on its own, its settings searched one at a time.
    struct Variant
    {
        size_t choice;
        FilterValues values;
        Verdict alone;
    };
    std::vector<Variant> variants;
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
        std::optional<Verdict> best, fewest; // the shortest routes, and the fewest survivors
        FilterValues fewest_values;
        std::map<FilterValues, Verdict> seen;
        auto trial = [&](const FilterValues& values) -> const Verdict& {
            auto it = seen.find(values);
            if (it == seen.end()) it = seen.emplace(values, judge(line, alone(c.name, values, current), anchors, options)).first;
            return it->second;
        };
        auto consider = [&](const FilterValues& values) {
            const Verdict& v = trial(values);
            if (better(v, best))
            {
                best = v;
                c.values = values;
            }
            if (v.passes && v.score >= 0 && (!fewest || v.count < fewest->count))
            {
                fewest = v;
                fewest_values = values;
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
        if (best)
        {
            c.bits = best->score;
            // Both settings go on to the set: the one with the shortest routes, and the one that
            // leaves the fewest survivors (which bounds every route, a number being below the count).
            variants.push_back({result.choices.size(), c.values, *best});
            if (fewest_values != c.values) variants.push_back({result.choices.size(), fewest_values, *fewest});
        }
        else
        {
            const Verdict& at = trial(c.values);
            c.why_not = !at.passes ? (anchors.size() == 1 ? "the item fails it at every setting tried" : "an item fails it at every setting tried")
                                   : "cannot be counted: " + at.blocker;
        }
        result.choices.push_back(std::move(c));
        if (progress) ++progress->done;
    }
    if (cancelled())
    {
        result.finished = false;
        return result;
    }

    // 2. The set. Stacks are grown from each setting kept, two ways: adding every other where it
    // leaves fewer survivors, and adding every other where it shortens the routes. Ranks move as the
    // stack changes, so a search by routes alone falls into corners that shrinking the count avoids;
    // shrinking the count alone misses an anchor that lands near 0 under a wider stack. Each way
    // takes the settings in its own order (fewest survivors, or shortest routes, alone), each filter
    // at most once in a stack, only where it can be counted with those there (filter_conflict). The
    // stack with the best score of all those grown is kept; a setting already in a stack grown the
    // same way seeds none of its own (it would grow that stack again).
    struct Grown
    {
        LineFilters stack;
        Verdict verdict;
        std::vector<size_t> used; // variants
    };
    Verdict none;
    none.passes = true;
    none.score = none.route = result.line_bits;
    none.count = result.count_bits;
    auto grow = [&](size_t seed, bool by_count, const std::vector<size_t>& order) {
        Grown g{LineFilters{}, none, {}};
        g.stack.mode = FilterMode::Compact;
        g.stack.values = current.values;
        std::vector<size_t> sequence{seed};
        for (const size_t i : order)
            if (i != seed) sequence.push_back(i);
        for (const size_t i : sequence)
        {
            if (cancelled()) break;
            const Variant& v = variants[i];
            const std::string& name = result.choices[v.choice].name;
            if (g.stack.is_enabled(name)) continue;
            const FilterSpec* spec = find_filter(name);
            bool clash = false;
            for (const std::string& e : g.stack.enabled)
            {
                const FilterSpec* other = find_filter(e);
                const FilterValues& ov = g.stack.values_of(e);
                if (spec && other && !filter_conflict(*spec, *other, &line, &v.values, &ov).empty())
                {
                    clash = true;
                    break;
                }
            }
            if (clash) continue;
            LineFilters with = g.stack;
            with.values[name] = v.values;
            (void)tick_filter(with, name, true);
            const Verdict j = judge(line, with, anchors, options);
            if (!j.passes || j.score < 0) continue;
            if (by_count ? j.count >= g.verdict.count - 1e-9 : j.score >= g.verdict.score - 1e-9) continue;
            g.stack = std::move(with);
            g.verdict = j;
            g.used.push_back(i);
        }
        return g;
    };
    std::optional<Grown> best;
    for (const bool by_count : {true, false})
    {
        std::vector<size_t> order(variants.size());
        for (size_t i = 0; i < order.size(); ++i) order[i] = i;
        std::stable_sort(order.begin(), order.end(), [&](size_t a, size_t b) {
            return by_count ? variants[a].alone.count < variants[b].alone.count : variants[a].alone.score < variants[b].alone.score;
        });
        std::vector<bool> grown(variants.size(), false);
        for (const size_t seed : order)
        {
            if (cancelled()) break;
            if (grown[seed]) continue;
            Grown g = grow(seed, by_count, order);
            for (const size_t i : g.used) grown[i] = true;
            if (!g.used.empty() && (!best || g.verdict.score < best->verdict.score)) best = std::move(g);
        }
    }
    if (best && best->verdict.score < none.score - 1e-9)
    {
        result.filters = std::move(best->stack);
        result.bits = best->verdict.score;
        result.route_bits = best->verdict.route;
        result.description_bits = best->verdict.description;
        result.count_bits = best->verdict.count;
        for (const size_t i : best->used)
        {
            TailorChoice& c = result.choices[variants[i].choice];
            c.used = true;
            c.values = variants[i].values;
            c.bits = variants[i].alone.score;
        }
    }
    for (TailorChoice& c : result.choices)
        if (!c.used && c.why_not.empty()) c.why_not = "not in the stack with the shortest routes";
    LineFilters& stack = result.filters;
    // Only the settings of what is ticked are kept.
    for (auto it = stack.values.begin(); it != stack.values.end();)
        it = stack.is_enabled(it->first) ? std::next(it) : stack.values.erase(it);
    result.finished = !cancelled();
    if (progress) progress->done = progress->total + 1;
    return result;
}

} // namespace sieve::cli
