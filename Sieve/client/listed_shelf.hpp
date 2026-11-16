// Sieve hallway — a listed line's shelf (dimensions.hpp `listed`): what a line whose units are a list of
// items needs to stand them on its shelves, put one in your hands and open it. Every listed line goes
// through this one interface, so a new dimension of the kind is a row in kDimensions and a shelf here,
// not a branch in every part of the hallway (Edward, 15 November 2026: "we should make sure it is
// dynamic so that we can more easily slip in new dimensions in the future").
//
// The first such line is the fractal dimension (IDEAS 19.191, 19.235; list X1): its first book the index
// of every fractal charted (Edward: "a single first page book that lists all of them and then the
// individuals"; none with the known fractals excluded), each book after it a ruleset of the whole space
// (fractal-space-v1), drawn at a depth, and judged by its noise filters ([fractals]).
// Its list may be far too long to count in a machine word, so a place in it is a BigUint, as every
// other line's address is.
#pragma once

#include "dimensions.hpp"

#include "sieve/biguint.hpp"
#include "sieve/filter.hpp"
#include "sieve/fractal_space.hpp"
#include "sieve/image.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

class ListedShelf
{
public:
    virtual ~ListedShelf() = default;
    // How many items stand on it, and the id of its space (what favourites are kept under).
    virtual sieve::BigUint size() const = 0;
    virtual std::string id() const = 0;
    // An item's title, as its spine and hover panel show it, and a line saying what it is.
    virtual std::string title(const sieve::BigUint& i) const = 0;
    virtual std::string summary(const sieve::BigUint& i) const = 0;
    // An item's picture at a depth, on a picture of width x height (RGBA); nothing if it has none.
    virtual std::optional<sieve::RgbaImage> picture(const sieve::BigUint& i, uint32_t depth, uint32_t width, uint32_t height) const = 0;
    // Its text, as the item page and the viewer's text tab show it, a line at a time. It is what the
    // item is: F saves it and the sample writes it, each line ending in a line feed, as a file of the
    // extension file_ext(i); the picture is only how it is shown (Edward, 15 November 2026: "we should
    // only really be storing the steps used to produce them, the formula, not the pngs ... displaying
    // it as an image is just a pleasantry").
    virtual std::vector<std::string> text(const sieve::BigUint& i) const = 0;
    virtual std::string file_ext(const sieve::BigUint& i) const = 0;
    // The depths it can be shown at: 0 to deepest(i); and the one it is shown at first.
    virtual uint32_t deepest(const sieve::BigUint& i) const = 0;
    virtual uint32_t first_depth(const sieve::BigUint& i) const = 0;
    // An item by a name typed (a charted fractal's id; "index" for the first), for Warp: nothing if no
    // item is called that.
    virtual std::optional<sieve::BigUint> find(const std::string& name) const = 0;
    // Why a name find() finds nothing for is not on the line, where a setting keeps it off (naming the
    // setting and where it is changed); "" where nothing is called that.
    virtual std::string not_found(const std::string& name) const = 0;
    // What the line's filters judge of an item, a symbol at a time, on filter_line(): nothing for an item
    // they never judge (the fractals' index, and the known fractals, which their own setting rules).
    virtual std::optional<std::vector<uint32_t>> unit(const sieve::BigUint& i) const = 0;
    virtual sieve::FilterLine filter_line() const = 0;
    // Why it is empty, where it could not be read ("" when it is not).
    virtual std::string trouble() const = 0;
    // What the line's figures in the setup menu's map say of it: its units in words.
    virtual std::string units_text() const = 0;
};

// The settings a listed line is shaped by (the setup menu's FRACTALS rows; --fractal-longest and
// --fractal-known).
struct ListedOptions
{
    uint32_t fractal_longest = sieve::kFractalSpaceLongestDefault; // the whole space's longest ruleset, in characters
    std::string fractal_known = "include"; // the known fractals: include (marked), exclude, or only
};
// The words fractal_known takes, in the order the setup menu steps through them.
inline const std::vector<std::string>& fractal_known_settings()
{
    static const std::vector<std::string> k{"include", "exclude", "only"};
    return k;
}

// The shelf of a listed line (nullptr for a line that is not one).
std::unique_ptr<ListedShelf> make_listed_shelf(Media m, const ListedOptions& options = {});

} // namespace hallway
