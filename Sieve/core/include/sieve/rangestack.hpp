// Sieve — the ticked filters of a line of a range of lengths (list O3; sieve/space.hpp's range space): every unit of
// MIN to MAX symbols, the shorter first.
//
// A FilterStack is made for one length, so a RangeStack holds one for each length of the range, each made with the
// ticked filters that apply at that length (a filter that cannot judge a unit so short, as one that needs two symbols,
// says nothing about the units of that length). A unit is judged by its own length's stack. Where every length's
// stack ranks, the range does: its survivors are the sum of theirs, and a survivor's number is the survivors of
// every shorter length plus its number among its own, so survivors keep the line's positional order.
//
// A degree window (degree-window-v2) is judged by the unit's place on the whole line, as on the binary line
// (sieve/window.hpp PlaceWindow), not by its digits: it is left out of the lengths' stacks and laid over the range,
// the survivors below each end of the window counted by walking the stacks' rankers.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/filter.hpp"
#include "sieve/window.hpp"

#include <cstdint>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace sieve {

class RangeStack
{
public:
    RangeStack() = default;
    // `line`: the range's line (its length the maximum, min_length its minimum, below the maximum).
    RangeStack(const FilterLine& line, const std::vector<FilterStack::Entry>& entries, const FilterResources& resources);

    bool empty() const { return names_.empty(); }
    size_t size() const { return names_.size(); }
    uint32_t min_length() const { return min_; }
    uint32_t max_length() const { return max_; }
    const std::string& filter_name(size_t i) const { return names_[i]; }
    // The first ticked filter the unit (min to max symbols) fails, as an index into the ticked filters, or -1.
    int first_failure(std::span<const uint32_t> unit) const;
    bool passes(std::span<const uint32_t> unit) const { return first_failure(unit) < 0; }

    // Counted and ranked where every length's stack ranks (and any window is one, or several none crossing 0).
    bool can_rank() const;
    std::string compact_blocker() const; // why not ("" when it can)
    const BigUint& count() const;        // can_rank() only
    BigUint rank(std::span<const uint32_t> unit) const;     // a survivor's number
    std::vector<uint32_t> unrank(const BigUint& k) const;   // k < count()

    // The provenance: the line's kind, symbols and range, then each ticked filter's (from the longest length's
    // stack); and its SHA-256, the domain of the range's compact scrambled order.
    const std::string& provenance() const { return provenance_; }
    const std::string& id() const { return id_; }

private:
    uint32_t min_ = 0, max_ = 0, base_ = 0;
    std::vector<std::string> names_;
    std::vector<FilterStack> stacks_;          // [length - min]
    std::vector<std::vector<int>> index_;      // [length - min][its stack's filter] -> the ticked filter's index
    std::vector<int> window_filter_;           // per ticked filter: -1, or which window
    std::vector<PlaceWindow> windows_;
    BigUint places_;                           // every unit of the range
    std::string provenance_, id_;
    // Counted once, when first asked for.
    mutable std::unique_ptr<std::once_flag> lazy_;
    mutable bool can_rank_ = false;
    mutable std::string blocker_;
    mutable std::vector<BigUint> before_;      // [length - min]: survivors of the shorter lengths (no window)
    mutable BigUint all_, count_, below_lo_, below_hi_;
    mutable bool window_wrap_ = false;
    void settle() const;
    BigUint shorter_places(uint32_t length) const; // the line's units shorter than `length`
    BigUint rank_inner(std::span<const uint32_t> unit) const;
    std::vector<uint32_t> unrank_inner(const BigUint& k) const;
    BigUint below_place(const BigUint& place) const; // survivors (no window) with a place below this one
};

// The units a ranker keeps that come before `unit` (of its length) in address order, whether `unit` is kept or not:
// the walk's completions of every smaller symbol at each place, stopping where no survivor goes on as `unit` does.
BigUint ranker_below(const Ranker& r, std::span<const uint32_t> unit);

} // namespace sieve
