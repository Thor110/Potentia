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
#include "sieve/packed.hpp"

#include <cstdint>
#include <memory>
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
//
// The table is packed: each row's numbers stored one after another as their limbs, with where each
// starts, rather than as a BigUint each (a vector and an allocation apiece: 358 MB held for 68 MB
// of numbers in the 236,034-state table of every text plugin at 32 characters, 99 MB packed).
// Rows 0 .. r of a table answer every length up to r, so a ranker at a shorter length can share a
// longer one's table (the constructor from a DfaRanker), and the automaton is shared, not copied.
class DfaRanker : public Ranker
{
public:
    DfaRanker(const Dfa& minimal, uint32_t length);
    DfaRanker(std::shared_ptr<const Dfa> minimal, uint32_t length);
    // The same automaton at `length` <= longer.length(), on longer's table: nothing is built.
    DfaRanker(const DfaRanker& longer, uint32_t length);
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return dfa().base; }
    State start() const override { return dfa().start < 0 ? kDead : State(dfa().start); }
    State next(State s, uint32_t symbol) const override;
    BigUint completions(State s, uint32_t remaining) const override;
    bool alive(State s, uint32_t remaining) const override;
    const Dfa& dfa() const { return *table_->dfa; } // the automaton it counts (minimal)
    uint32_t table_length() const { return uint32_t(table_->rows.rows() - 1); } // the longest it answers

    // The table's size, in bytes, before building it: states x (length + 1) numbers of up to
    // length x log2(base) bits, packed (PackedRows::estimate: an overestimate, 160 MB against
    // 98 MB measured for the table above). Used to decide whether counting fits the filter memory.
    static double table_bytes(size_t states, uint32_t base, uint32_t length);

private:
    struct Table
    {
        std::shared_ptr<const Dfa> dfa;
        PackedRows rows; // row r: each state's completions with r symbols left
    };
    void build();
    std::shared_ptr<Table> table_;
    uint32_t length_;
};

} // namespace sieve
