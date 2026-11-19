// Sieve — degree windows (degree-window-v1, core/src/filters/window.cpp).
//
// Every line is a loop, and where a unit stands on it is a bearing (client/hallway.hpp's
// exact_degrees): its positional address v of the N = base^length units, read as
// floor(v * 360 / N) degrees. A degree window keeps the units whose bearing lies in
// [from, from + width) (stopping at 360), in whole degrees or to as many decimals as are asked
// for, and sets aside everything else.
//
// It is a filter like any other (it judges a unit by its own digits), and it counts on top of
// whatever else is ticked: the stack's other filters are counted as they would be without it,
// and the window keeps the part of their survivors that falls between its two bounds (a
// WindowRanker, below). Its survivors then fill the whole loop in compact, as every stack's do:
// the chosen degrees are spread round all 360.
//
// Edward, 18 November 2026: "a single filter that has the ability to exclude everything but
// whatever lies within a single chosen degree range 1-2, 1-5 etc ... this filter could work for
// all lines ... the result would be everything except the chosen degree is filtered and thus that
// chosen degree becomes the vast majority of the state space wrapping back around".
#pragma once

#include "sieve/filter.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace sieve {

// A window's two bounds as units of the line (digits, most significant first): the first unit
// kept, and the first unit past the window (none when the window runs to the end of the line).
struct WindowBounds
{
    std::vector<uint32_t> lo;
    std::vector<uint32_t> hi; // (has_hi only)
    bool has_hi = false;

    bool contains(std::span<const uint32_t> unit) const;
    bool empty() const; // nothing between the bounds
};

// The bounds of [from, to) in units of 10^-decimals of a degree, on a line of base^length units:
// the units whose bearing floor(v * 360 * 10^decimals / N) is from or more and less than to.
// Exact, so they agree with the bearing the hallway shows. Throws unless from < to <= 360 * 10^decimals.
WindowBounds degree_bounds(uint32_t base, uint32_t length, uint64_t from, uint64_t to, uint32_t decimals);

// The bounds a degree-window filter keeps, or null for any other filter.
const WindowBounds* window_bounds(const Filter& f);

// Both windows at once (the units both keep).
WindowBounds intersect_windows(const WindowBounds& a, const WindowBounds& b);

// Every unit of base^length (the stack with nothing else ticked, under a window).
std::unique_ptr<Ranker> full_ranker(uint32_t base, uint32_t length);

// The survivors of `inner` that lie between the bounds, ranked in address order (k-th of them the
// (k + below)-th of inner's, `below` being inner's survivors under the window's first unit). Keeps a
// reference to inner, which must outlive it.
std::unique_ptr<Ranker> window_ranker(const Ranker& inner, WindowBounds bounds);

} // namespace sieve
