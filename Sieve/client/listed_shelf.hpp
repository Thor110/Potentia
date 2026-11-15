// Sieve hallway — a listed line's shelf (dimensions.hpp `listed`): what a line whose units are a finite
// list of items needs to stand them on its shelves, put one in your hands and open it. Every listed
// line goes through this one interface, so a new dimension of the kind is a row in kDimensions and a
// shelf here, not a branch in every part of the hallway (Edward, 15 November 2026: "we should make sure
// it is dynamic so that we can more easily slip in new dimensions in the future").
//
// The first such line is the fractal dimension (IDEAS 19.191, 19.235): its first book the index of every
// fractal charted (Edward: "a single first page book that lists all of them and then the individuals"),
// each book after it one fractal, drawn at a depth.
#pragma once

#include "dimensions.hpp"

#include "sieve/image.hpp"

#include <cstddef>
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
    virtual size_t size() const = 0;
    virtual std::string id() const = 0;
    // An item's title, as its spine and hover panel show it, and a line saying what it is.
    virtual std::string title(size_t i) const = 0;
    virtual std::string summary(size_t i) const = 0;
    // An item's picture at a depth, on a picture of width x height (RGBA); nothing if it has none.
    virtual std::optional<sieve::RgbaImage> picture(size_t i, uint32_t depth, uint32_t width, uint32_t height) const = 0;
    // Its text, as the item page and the viewer's text tab show it, a line at a time. It is what the
    // item is: F saves it and the sample writes it, each line ending in a line feed, as a file of the
    // extension file_ext(i); the picture is only how it is shown (Edward, 15 November 2026: "we should
    // only really be storing the steps used to produce them, the formula, not the pngs ... displaying
    // it as an image is just a pleasantry").
    virtual std::vector<std::string> text(size_t i) const = 0;
    virtual std::string file_ext(size_t i) const = 0;
    // The depths it can be shown at: 0 to deepest(i); and the one it is shown at first.
    virtual uint32_t deepest(size_t i) const = 0;
    virtual uint32_t first_depth(size_t i) const = 0;
    // An item by a name typed (its id), for Go to: nothing if no item is called that.
    virtual std::optional<size_t> find(const std::string& name) const = 0;
    // Why it is empty, where it could not be read ("" when it is not).
    virtual std::string trouble() const = 0;
};

// The shelf of a listed line (nullptr for a line that is not one).
std::unique_ptr<ListedShelf> make_listed_shelf(Media m);

} // namespace hallway
