// Sieve — deterministic automata (dfa.hpp): minimising to the canonical form, the product of two,
// and the ranker's completion table.
//
// Minimising is Hopcroft's algorithm, O(n x symbols x log n), on flat arrays (a dictionary's
// automaton has hundreds of thousands of states, and a debug build is slow with anything else).
// The oracle minimises with its own copy of Hopcroft in Python, written separately; both end in the
// same canonical numbering, which CI compares state for state.

#include "sieve/dfa.hpp"

#include <algorithm>
#include <cmath>
#include <deque>
#include <map>
#include <stdexcept>
#include <unordered_map>

namespace sieve {

bool Dfa::accepts(std::span<const uint32_t> unit) const
{
    int32_t s = start;
    for (uint32_t c : unit)
    {
        s = step(s, c);
        if (s < 0) return false;
    }
    return s >= 0 && accept[size_t(s)] != 0;
}

namespace {

Dfa empty_dfa(uint32_t base)
{
    Dfa e;
    e.base = base;
    return e;
}

} // namespace

Dfa minimise(const Dfa& d)
{
    const size_t n = d.states(), B = d.base;
    if (B == 0) throw std::invalid_argument("an automaton needs at least one symbol");
    if (d.next.size() != n * B) throw std::invalid_argument("an automaton's table must have states x symbols entries");
    for (int32_t t : d.next)
        if (t < Dfa::kDead || (t >= 0 && size_t(t) >= n)) throw std::invalid_argument("a transition leads to a state that does not exist");
    if (d.start == Dfa::kDead || n == 0) return empty_dfa(d.base);
    if (d.start < 0 || size_t(d.start) >= n) throw std::invalid_argument("the start state does not exist");

    // Reachable from the start, and able to reach an accepting state: the live states.
    std::vector<uint8_t> reach(n, 0), coreach(n, 0);
    std::vector<int32_t> stack{d.start};
    reach[size_t(d.start)] = 1;
    while (!stack.empty())
    {
        const int32_t s = stack.back();
        stack.pop_back();
        for (size_t c = 0; c < B; ++c)
            if (const int32_t t = d.next[size_t(s) * B + c]; t >= 0 && !reach[size_t(t)])
            {
                reach[size_t(t)] = 1;
                stack.push_back(t);
            }
    }
    std::vector<std::vector<int32_t>> back(n);
    for (size_t s = 0; s < n; ++s)
        if (reach[s])
            for (size_t c = 0; c < B; ++c)
                if (const int32_t t = d.next[s * B + c]; t >= 0) back[size_t(t)].push_back(int32_t(s));
    for (size_t s = 0; s < n; ++s)
        if (reach[s] && d.accept[s])
        {
            coreach[s] = 1;
            stack.push_back(int32_t(s));
        }
    while (!stack.empty())
    {
        const int32_t s = stack.back();
        stack.pop_back();
        for (int32_t p : back[size_t(s)])
            if (!coreach[size_t(p)])
            {
                coreach[size_t(p)] = 1;
                stack.push_back(p);
            }
    }
    if (!coreach[size_t(d.start)]) return empty_dfa(d.base);
    auto live = [&](int32_t s) { return s >= 0 && reach[size_t(s)] && coreach[size_t(s)]; };

    // Hopcroft's refinement over the live states, completed with a sink for every dead transition
    // (without it, states that differ only in where a symbol is missing would merge). Flat arrays
    // throughout: a refinable partition of the states (each block a range of `elems`, its marked
    // members at the front), and each state's predecessors on each symbol.
    std::vector<int32_t> id(n, -1); // live state -> dense index; the sink is index live_n
    int32_t live_n = 0;
    for (size_t s = 0; s < n; ++s)
        if (live(int32_t(s))) id[s] = live_n++;
    const int32_t sink = live_n, N = live_n + 1;
    std::vector<int32_t> orig(static_cast<size_t>(live_n), 0);
    for (size_t s = 0; s < n; ++s)
        if (id[s] >= 0) orig[size_t(id[s])] = int32_t(s);
    auto next_dense = [&](int32_t x, size_t c) -> int32_t {
        if (x == sink) return sink;
        const int32_t t = d.next[size_t(orig[size_t(x)]) * B + c];
        return live(t) ? id[size_t(t)] : sink;
    };
    // Predecessors, by (target, symbol), in one array.
    std::vector<uint32_t> pstart(size_t(N) * B + 1, 0);
    for (int32_t x = 0; x < N; ++x)
        for (size_t c = 0; c < B; ++c) ++pstart[size_t(next_dense(x, c)) * B + c + 1];
    for (size_t k = 1; k < pstart.size(); ++k) pstart[k] += pstart[k - 1];
    std::vector<int32_t> preds(pstart.back());
    {
        std::vector<uint32_t> fill(pstart.begin(), pstart.end() - 1);
        for (int32_t x = 0; x < N; ++x)
            for (size_t c = 0; c < B; ++c) preds[fill[size_t(next_dense(x, c)) * B + c]++] = x;
    }
    // The partition: accepting states, and the rest with the sink.
    std::vector<int32_t> elems(static_cast<size_t>(N), 0), loc(static_cast<size_t>(N), 0), blk(static_cast<size_t>(N), 0);
    std::vector<int32_t> first, end, mid;
    {
        int32_t k = 0;
        for (int32_t x = 0; x < live_n; ++x)
            if (d.accept[size_t(orig[size_t(x)])]) elems[size_t(k++)] = x;
        const int32_t split = k;
        for (int32_t x = 0; x < live_n; ++x)
            if (!d.accept[size_t(orig[size_t(x)])]) elems[size_t(k++)] = x;
        elems[size_t(k++)] = sink;
        if (split > 0)
        {
            first.push_back(0);
            end.push_back(split);
        }
        first.push_back(split);
        end.push_back(N);
        for (size_t b = 0; b < first.size(); ++b)
            for (int32_t i = first[b]; i < end[b]; ++i)
            {
                blk[size_t(elems[size_t(i)])] = int32_t(b);
                loc[size_t(elems[size_t(i)])] = i;
            }
        mid = first;
    }
    std::vector<uint8_t> waiting(first.size(), 0);
    std::vector<int32_t> work;
    if (first.size() == 2)
    {
        const int32_t smaller = end[0] - first[0] <= end[1] - first[1] ? 0 : 1;
        work.push_back(smaller);
        waiting[size_t(smaller)] = 1;
    }
    else
    {
        work.push_back(0);
        waiting[0] = 1;
    }
    std::vector<int32_t> splitter, touched;
    while (!work.empty())
    {
        const int32_t a = work.back();
        work.pop_back();
        waiting[size_t(a)] = 0;
        splitter.assign(elems.begin() + first[size_t(a)], elems.begin() + end[size_t(a)]);
        for (size_t c = 0; c < B; ++c)
        {
            touched.clear();
            for (int32_t t : splitter)
                for (uint32_t k = pstart[size_t(t) * B + c]; k < pstart[size_t(t) * B + c + 1]; ++k)
                {
                    const int32_t x = preds[k];
                    const int32_t b = blk[size_t(x)];
                    const int32_t i = loc[size_t(x)];
                    if (i < mid[size_t(b)]) continue; // marked already
                    if (mid[size_t(b)] == first[size_t(b)]) touched.push_back(b);
                    const int32_t j = mid[size_t(b)]++;
                    std::swap(elems[size_t(i)], elems[size_t(j)]);
                    loc[size_t(elems[size_t(i)])] = i;
                    loc[size_t(elems[size_t(j)])] = j;
                }
            for (int32_t b : touched)
            {
                if (mid[size_t(b)] == end[size_t(b)])
                {
                    mid[size_t(b)] = first[size_t(b)]; // all of it: no split
                    continue;
                }
                // The marked front becomes a new block.
                const int32_t nb = int32_t(first.size());
                first.push_back(first[size_t(b)]);
                end.push_back(mid[size_t(b)]);
                mid.push_back(first[size_t(b)]);
                first[size_t(b)] = mid[size_t(b)];
                mid[size_t(b)] = first[size_t(b)];
                waiting.push_back(0);
                for (int32_t i = first[size_t(nb)]; i < end[size_t(nb)]; ++i) blk[size_t(elems[size_t(i)])] = nb;
                if (waiting[size_t(b)])
                {
                    work.push_back(nb);
                    waiting[size_t(nb)] = 1;
                }
                else
                {
                    const int32_t pick = end[size_t(nb)] - first[size_t(nb)] <= end[size_t(b)] - first[size_t(b)] ? nb : b;
                    work.push_back(pick);
                    waiting[size_t(pick)] = 1;
                }
            }
        }
    }
    // Each live state's class: its block.
    std::vector<int32_t> cls(n, -1);
    for (size_t s = 0; s < n; ++s)
        if (id[s] >= 0) cls[s] = blk[size_t(id[s])];
    const size_t classes = first.size();

    // Canonical numbering: breadth first from the start's class, symbols in order.
    std::vector<int32_t> rep(classes, -1);
    for (size_t s = 0; s < n; ++s)
        if (cls[s] >= 0 && rep[size_t(cls[s])] < 0) rep[size_t(cls[s])] = int32_t(s);
    std::vector<int32_t> order(classes, -1);
    std::deque<int32_t> queue{cls[size_t(d.start)]};
    order[size_t(cls[size_t(d.start)])] = 0;
    int32_t numbered = 1;
    std::vector<int32_t> seq;
    while (!queue.empty())
    {
        const int32_t k = queue.front();
        queue.pop_front();
        seq.push_back(k);
        const size_t s = size_t(rep[size_t(k)]);
        for (size_t c = 0; c < B; ++c)
        {
            const int32_t t = d.next[s * B + c];
            if (!live(t)) continue;
            const int32_t kt = cls[size_t(t)];
            if (order[size_t(kt)] < 0)
            {
                order[size_t(kt)] = numbered++;
                queue.push_back(kt);
            }
        }
    }
    Dfa m;
    m.base = d.base;
    m.start = 0;
    m.accept.assign(seq.size(), 0);
    m.next.assign(seq.size() * B, Dfa::kDead);
    for (int32_t k : seq)
    {
        const size_t s = size_t(rep[size_t(k)]), ns = size_t(order[size_t(k)]);
        m.accept[ns] = d.accept[s];
        for (size_t c = 0; c < B; ++c)
            if (const int32_t t = d.next[s * B + c]; live(t)) m.next[ns * B + c] = order[size_t(cls[size_t(t)])];
    }
    return m;
}

Dfa intersect(const Dfa& a, const Dfa& b)
{
    if (a.base != b.base) throw std::invalid_argument("automata over different symbols cannot be combined");
    const size_t B = a.base;
    if (a.start < 0 || b.start < 0) return empty_dfa(a.base);
    std::unordered_map<uint64_t, int32_t> id;
    std::vector<std::pair<int32_t, int32_t>> pairs;
    auto get = [&](int32_t x, int32_t y) {
        const uint64_t key = uint64_t(uint32_t(x)) << 32 | uint32_t(y);
        const auto [it, added] = id.emplace(key, int32_t(pairs.size()));
        if (added) pairs.emplace_back(x, y);
        return it->second;
    };
    Dfa p;
    p.base = a.base;
    p.start = get(a.start, b.start);
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        const auto [x, y] = pairs[i];
        p.accept.push_back(a.accept[size_t(x)] && b.accept[size_t(y)] ? 1 : 0);
        for (size_t c = 0; c < B; ++c)
        {
            const int32_t tx = a.next[size_t(x) * B + c], ty = b.next[size_t(y) * B + c];
            p.next.push_back(tx < 0 || ty < 0 ? Dfa::kDead : get(tx, ty));
        }
    }
    return minimise(p);
}

Dfa unite(const Dfa& a, const Dfa& b)
{
    if (a.base != b.base) throw std::invalid_argument("automata over different symbols cannot be combined");
    if (a.start < 0) return minimise(b);
    if (b.start < 0) return minimise(a);
    const size_t B = a.base;
    // Pairs of states, either of which may be the dead end (-1) but not both.
    std::unordered_map<uint64_t, int32_t> id;
    std::vector<std::pair<int32_t, int32_t>> pairs;
    auto get = [&](int32_t x, int32_t y) {
        const uint64_t key = uint64_t(uint32_t(x)) << 32 | uint32_t(y);
        const auto [it, added] = id.emplace(key, int32_t(pairs.size()));
        if (added) pairs.emplace_back(x, y);
        return it->second;
    };
    Dfa p;
    p.base = a.base;
    p.start = get(a.start, b.start);
    for (size_t i = 0; i < pairs.size(); ++i)
    {
        const auto [x, y] = pairs[i];
        p.accept.push_back((x >= 0 && a.accept[size_t(x)]) || (y >= 0 && b.accept[size_t(y)]) ? 1 : 0);
        for (size_t c = 0; c < B; ++c)
        {
            const int32_t tx = x < 0 ? Dfa::kDead : a.next[size_t(x) * B + c], ty = y < 0 ? Dfa::kDead : b.next[size_t(y) * B + c];
            p.next.push_back(tx < 0 && ty < 0 ? Dfa::kDead : get(tx, ty));
        }
    }
    return minimise(p);
}

Dfa complement(const Dfa& d)
{
    const size_t B = d.base;
    if (B == 0) throw std::invalid_argument("an automaton needs at least one symbol");
    // Every state, and a sink for the dead end, with acceptance flipped.
    const size_t n = d.states();
    Dfa c;
    c.base = d.base;
    const int32_t sink = int32_t(n);
    c.start = d.start < 0 ? sink : d.start;
    c.next.resize((n + 1) * B);
    c.accept.resize(n + 1);
    for (size_t s = 0; s < n; ++s)
    {
        c.accept[s] = d.accept[s] ? 0 : 1;
        for (size_t k = 0; k < B; ++k)
        {
            const int32_t t = d.next[s * B + k];
            c.next[s * B + k] = t < 0 ? sink : t;
        }
    }
    c.accept[n] = 1;
    for (size_t k = 0; k < B; ++k) c.next[n * B + k] = sink;
    return minimise(c);
}

bool subset(const Dfa& a, const Dfa& b)
{
    if (a.base != b.base) throw std::invalid_argument("automata over different symbols cannot be compared");
    if (a.start < 0) return true; // a accepts nothing
    const size_t B = a.base;
    // Pairs (state of a, state of b or dead); a pair where a accepts and b does not (or is dead)
    // is a unit a keeps and b does not.
    std::unordered_map<uint64_t, char> seen;
    std::vector<std::pair<int32_t, int32_t>> todo{{a.start, b.start}};
    auto key = [](int32_t x, int32_t y) { return uint64_t(uint32_t(x)) << 32 | uint32_t(y); };
    seen.emplace(key(a.start, b.start), 1);
    while (!todo.empty())
    {
        const auto [x, y] = todo.back();
        todo.pop_back();
        if (a.accept[size_t(x)] && (y < 0 || !b.accept[size_t(y)])) return false;
        for (size_t c = 0; c < B; ++c)
        {
            const int32_t tx = a.next[size_t(x) * B + c];
            if (tx < 0) continue;
            const int32_t ty = y < 0 ? Dfa::kDead : b.next[size_t(y) * B + c];
            if (seen.emplace(key(tx, ty), 1).second) todo.emplace_back(tx, ty);
        }
    }
    return true;
}

// ---------------------------------------------------------------- the ranker

double DfaRanker::table_bytes(size_t states, uint32_t base, uint32_t length)
{
    // On average a number in the table is half the longest (the counts grow with r).
    const double bits = double(length) * std::log2(double(std::max<uint32_t>(base, 2)));
    return double(states) * (double(length) + 1) * (bits / 16.0 + 32.0);
}

DfaRanker::DfaRanker(const Dfa& minimal, uint32_t length) : dfa_(minimal), length_(length)
{
    const size_t n = dfa_.states(), B = dfa_.base;
    // Each state's transitions grouped by where they lead, with how many symbols lead there.
    std::vector<std::vector<std::pair<int32_t, uint32_t>>> groups(n);
    for (size_t s = 0; s < n; ++s)
    {
        std::map<int32_t, uint32_t> m;
        for (size_t c = 0; c < B; ++c)
            if (const int32_t t = dfa_.next[s * B + c]; t >= 0) ++m[t];
        groups[s].assign(m.begin(), m.end());
    }
    table_.assign(size_t(length) + 1, std::vector<BigUint>(n));
    for (size_t s = 0; s < n; ++s)
        if (dfa_.accept[s]) table_[0][s] = BigUint(1);
    for (uint32_t r = 1; r <= length; ++r)
        for (size_t s = 0; s < n; ++s)
        {
            BigUint acc;
            for (const auto& [t, mult] : groups[s])
            {
                const BigUint& prev = table_[r - 1][size_t(t)];
                if (prev.is_zero()) continue;
                if (mult == 1) acc += prev;
                else
                {
                    BigUint x = prev;
                    x.mul_small(mult);
                    acc += x;
                }
            }
            table_[r][s] = std::move(acc);
        }
    set_count();
}

Ranker::State DfaRanker::next(State s, uint32_t symbol) const
{
    if (s == kDead || s >= dfa_.states() || symbol >= dfa_.base) return kDead;
    const int32_t t = dfa_.next[size_t(s) * dfa_.base + symbol];
    return t < 0 ? kDead : State(t);
}

BigUint DfaRanker::completions(State s, uint32_t remaining) const
{
    if (s == kDead || s >= dfa_.states() || remaining > length_) return BigUint();
    return table_[remaining][size_t(s)];
}

bool DfaRanker::alive(State s, uint32_t remaining) const
{
    if (s == kDead || s >= dfa_.states() || remaining > length_) return false;
    return !table_[remaining][size_t(s)].is_zero();
}

} // namespace sieve
