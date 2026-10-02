// Sieve — deterministic automata (docs/FILTER-PLUGINS.md §5): the engine every filter plugin runs
// on. A plugin, however it is written, compiles to one of these; from it the engine judges units,
// and counts, ranks and unranks their survivors exactly, as the built-in rankers do.
//
// A Dfa reads a unit one symbol at a time: from a state, a symbol leads to another state or to the
// dead end (the unit fails), and the unit passes if the walk ends in an accepting state. It is
// minimised into a canonical form: only states that can be reached and can still reach an
// accepting state are kept, equivalent states are merged, and the rest are numbered in the order
// a breadth-first walk from the start meets them, symbols in order. Two plugins that describe the
// same rule therefore compile to the same automaton, state for state, in C++ and in the oracle.
#pragma once

#include "sieve/filter.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace sieve {

struct Dfa
{
    static constexpr int32_t kDead = -1;
    uint32_t base = 0;           // symbols
    int32_t start = kDead;       // kDead: no unit passes (an empty automaton)
    std::vector<int32_t> next;   // states * base: next[s * base + c]
    std::vector<uint8_t> accept; // per state

    size_t states() const { return accept.size(); }
    int32_t step(int32_t s, uint32_t c) const { return s < 0 || c >= base ? kDead : next[size_t(s) * base + c]; }
    bool accepts(std::span<const uint32_t> unit) const;
};

// The canonical minimal automaton for the same units (see above). Throws if the input is malformed
// (a transition to a state that does not exist, or a start out of range).
Dfa minimise(const Dfa& d);

// The units both accept (the product, minimised). Both must read the same symbols. With
// `max_pairs`, throws std::length_error once the product has more states than that before it is
// minimised (for a caller that would refuse a table that size anyway, and must not run out of
// memory finding out).
Dfa intersect(const Dfa& a, const Dfa& b, size_t max_pairs = 0);

// The units either accepts (the product, minimised). Both must read the same symbols.
Dfa unite(const Dfa& a, const Dfa& b);

// The units it does not accept, at every length (every state completed with a sink, acceptance
// flipped, minimised).
Dfa complement(const Dfa& d);

// Whether every unit a accepts, b accepts too, at every length (a's rule implies b's). Exact: a
// walk of the pairs of states the two can be in together, looking for one where a accepts and b
// does not. Two minimal automata accept the same units exactly when they are equal, state for
// state (minimise is canonical), which is how duplicate filters are found.
bool subset(const Dfa& a, const Dfa& b);

// Counts, ranks and unranks what a Dfa accepts at one length. completions(s, r) is a table built
// backwards from the accepting states, r = 0 .. length, in exact integers, with each state's
// transitions grouped by where they lead (every letter after a letter goes to one state).
class DfaRanker : public Ranker
{
public:
    DfaRanker(const Dfa& minimal, uint32_t length);
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return dfa_.base; }
    State start() const override { return dfa_.start < 0 ? kDead : State(dfa_.start); }
    State next(State s, uint32_t symbol) const override;
    BigUint completions(State s, uint32_t remaining) const override;
    bool alive(State s, uint32_t remaining) const override;

    // The table's size estimate, in bytes, before building it: states x (length + 1) numbers of
    // up to length x log2(base) bits. Used to decide whether counting fits the budget.
    static double table_bytes(size_t states, uint32_t base, uint32_t length);

private:
    Dfa dfa_;
    uint32_t length_;
    std::vector<std::vector<BigUint>> table_; // table_[r][s]
};

} // namespace sieve
