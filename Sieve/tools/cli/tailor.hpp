// Sieve CLI -- a line's filters tailored to the items that must keep their places (IDEAS.md §12,
// "optimise all with the COST pages"; shared by `sieve tailor`, the file weighing (weigh.hpp) and
// the hallway's COST tab).
//
// The items are the anchors: whatever is chosen, every one of them survives. The search looks for
// the filters, and their settings, under which the anchors' addresses are written shortest. An
// anchor's address under a stack is its survivor number, and the shortest way to write that is its
// shortest route (corridor.hpp shortest_path: the number with its leading zeros dropped, or a
// bearing and a walk): the cleanest digits there are for it. So the search does not push for the
// fewest survivors, which shortens the widest address; it pushes each anchor's own number towards
// 0, and takes whatever it can land on. A stack that leaves many survivors but puts an anchor at
// number 0 names it in no bits at all. In two steps:
//
//   1. Each filter on its own. Every filter the line offers that can be counted (not retired, not
//      one that judges only) is tried at the settings it has, and then each of its settings in
//      turn over the values it allows: an integer over its range, coarsely and then closer in
//      around the best value found; a text over its list of choices, or every registered
//      dictionary. A value is kept only where every anchor passes, and of those the one with the
//      shortest routes. A filter some anchor fails at every value tried is not used.
//   2. The set. From each filter kept, strongest first, a stack is grown: every other filter is
//      added where it can be counted with those there (filter_conflict) and shortens the routes.
//      The best stack grown is kept.
//
// With `count_description`, a stack also pays for being written down (description_bits): which
// filters are ticked and each setting, as a reader who does not have them must be told. That is
// the price where the stack travels with the addresses (a manifest of many files, weigh.hpp), and
// what makes a stack tailored to one item rarely worth it there (IDEAS §12).
//
// It is a search, not a proof: values are sampled, and the set is grown greedily. What it finds is
// exact, though: every count and number is that of the stack as built, and every anchor passes it.
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
    FilterValues values; // the settings found (with every anchor passing), or those it has
    double bits = -1;    // the score of it alone at those settings (as TailorResult::bits); -1 when not scored
    bool used = false;   // in the stack chosen
    std::string why_not; // when not used: why (empty for one used)
};

struct TailorResult
{
    LineFilters filters;              // the stack chosen, in compact mode (nothing ticked: nothing helped)
    double line_bits = 0;             // the anchors' shortest routes on the line unfiltered: the base
    double bits = 0;                  // the score of the stack chosen: the anchors' shortest routes under
                                      // it, and its description where that is counted (= line_bits: none)
    double route_bits = 0;            // ... of which the routes
    double description_bits = 0;      // ... and the description (counted or not)
    double count_bits = 0;            // log2 of its survivors: the width of every compact address
    std::vector<TailorChoice> choices; // every filter weighed, in the order they were taken
    bool finished = true;             // false: cancelled before the end (what is here was found)
};

struct TailorOptions
{
    bool count_description = false; // the stack's description is part of the score
};

// Where a search running on another thread has got to, and a way to stop it.
struct TailorProgress
{
    std::atomic<int> done{0}, total{0}; // filters weighed of those to weigh (then the set: total + 1)
    std::atomic<bool> cancel{false};
    std::mutex mx;
    std::string current; // the filter being weighed
};

// The search above, for `anchors` (units of `line`, at least one), starting from `current` (its
// settings are the first tried for each filter).
TailorResult tailor_filters(const FilterLine& line, const std::vector<std::vector<uint32_t>>& anchors, const LineFilters& current,
                            TailorProgress* progress = nullptr, const TailorOptions& options = {});
// One anchor.
TailorResult tailor_filters(const FilterLine& line, std::span<const uint32_t> unit, const LineFilters& current, TailorProgress* progress = nullptr,
                            const TailorOptions& options = {});

// The bits of a number's shortest route among `count` places (corridor.hpp shortest_path, with
// bearings of up to 20 places): its cleanest digits. Past 2^16 bits, the number with its leading
// zeros dropped (a bearing there saves a few bits of many thousands, and finding it takes seconds).
double route_bits(const BigUint& number, const BigUint& count);

// What writing `stack` down costs a reader who does not have it, in bits: one for each filter the
// line offers that can count (whether it is ticked), and for each ticked filter, each of its
// settings at log2 of the values it allows (an integer's range over its step, a text's choices,
// a dictionary among those registered; free text, 8 bits a character). Nothing ticked: 0.
double description_bits(const FilterLine& line, const LineFilters& stack);

} // namespace sieve::cli
