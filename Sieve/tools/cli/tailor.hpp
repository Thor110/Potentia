// Sieve CLI -- a line's filters tailored to one item (IDEAS.md §12, "optimise all with the COST
// pages"; shared by `sieve tailor` and the hallway's COST tab).
//
// The item is the anchor: whatever is chosen, it survives. The search looks for the filters, and
// their settings, that leave the fewest survivors with the item among them, so that its compact
// address (its number among the survivors, log2 of their count in bits) is as short as the search
// can make it. In two steps:
//
//   1. Each filter on its own. Every filter the line offers that can be counted (not retired, not
//      one that judges only) is tried at the settings it has, and then each of its settings in
//      turn over the values it allows: an integer over its range, coarsely and then closer in
//      around the best value found; a text over its list of choices, or every registered
//      dictionary. A value is kept only where the item passes, and of those the one that leaves
//      the fewest survivors. A filter the item fails at every value tried is not used.
//   2. The set. The filters kept are taken strongest first (fewest survivors alone), and each is
//      added to the stack if it can be counted together with those already there (no clash,
//      filter_conflict), the stack can still be counted, and it leaves fewer survivors than before.
//
// It is a search, not a proof: the values are sampled, and the set is chosen greedily. What it
// finds is exact, though: every count is the count of the stack as built, and the item passes it.
#pragma once

#include "cli/filter_config.hpp"

#include <atomic>
#include <mutex>
#include <span>
#include <string>
#include <vector>

namespace sieve::cli {

// One filter as the search left it.
struct TailorChoice
{
    std::string name;    // its full name ("max-run-v1")
    FilterValues values; // the settings found (with the item passing), or those it has
    double bits = -1;    // log2 of its survivors alone at those settings; -1 when not counted
    bool used = false;   // in the stack chosen
    std::string why_not; // when not used: why (empty for one used)
};

struct TailorResult
{
    LineFilters filters;              // the stack chosen, in compact mode (nothing ticked: nothing helped)
    double line_bits = 0;             // log2 of the line: the item's address without filters
    double bits = 0;                  // log2 of the stack's survivors: the item's compact address
    std::vector<TailorChoice> choices; // every filter weighed, in the order they were taken
    bool finished = true;             // false: cancelled before the end (what is here was found)
};

// Where a search running on another thread has got to, and a way to stop it.
struct TailorProgress
{
    std::atomic<int> done{0}, total{0}; // filters weighed of those to weigh (then the set: total + 1)
    std::atomic<bool> cancel{false};
    std::mutex mx;
    std::string current; // the filter being weighed
};

// The search above, for `unit` on `line`, starting from `current` (its settings are the first
// tried for each filter). `unit` must be a unit of the line.
TailorResult tailor_filters(const FilterLine& line, std::span<const uint32_t> unit, const LineFilters& current, TailorProgress* progress = nullptr);

} // namespace sieve::cli
