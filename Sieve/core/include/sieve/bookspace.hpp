// Sieve — the books line: every possible book of one shape (SPECIFICATIONS §11).
//
// A book of this line is a cover (one unit of a picture space), a title (one unit of the page
// space) and P pages (P more units of the page space), in that order. So there are
//     N = |cover| * |page|^(P + 1)
// books, and "bookspace-v1" numbers them:
//   positional  the book's index is its parts read as one mixed-radix number, most significant
//               first: the cover's digits (base |cover symbols|), then the title's and every
//               page's digits (base |page symbols|). Neighbouring books differ in the last page.
//   scrambled   the positional index passed through shuffle-sha256-v1 (sieve/compact.hpp) over
//               [0, N), keyed with the page space's key, domain = the book space's id. Neighbours
//               are unrelated books.
// Addresses are that index in hex, zero-padded to the width N - 1 needs. The line holds exactly
// the books a book record (tools/cli/book.hpp) of the same shape can hold, so a record opens on
// its shelf when its shape matches.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/space.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kBookSpaceVersion = "bookspace-v1";

class BookSpace
{
public:
    using Digits = Space::Digits;
    struct Parts
    {
        Digits cover, title;
        std::vector<Digits> pages;
        bool operator==(const Parts&) const = default;
    };

    BookSpace(const Space& cover, const Space& page, uint32_t pages);

    const Space& cover_space() const { return cover_; }
    const Space& page_space() const { return page_; }
    uint32_t pages() const { return pages_; }
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    // "books/<cover symbols>/L<n>+<page symbols>/L<n>x<P+1>/key=<key>/bookspace-v1"
    std::string id() const;

    BigUint index_of(const Parts& p, AddressMode m) const;
    Parts parts_at(const BigUint& index, AddressMode m) const; // index < size()
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked

private:
    Space cover_, page_;
    uint32_t pages_;
    BigUint size_;
    size_t hex_width_;
    Shuffle shuffle_;
};

} // namespace sieve
