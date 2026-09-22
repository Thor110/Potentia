// Sieve — filters for the books line (SPECIFICATIONS §11).
//
// A book passes when its cover passes the cover stack (image filters), its title the title stack
// (page filters, on one page), and its pages the pages stack, applied to all its pages read as one
// continuous text of P * L symbols. Reading the pages as one text is what lets real books through:
// a fixed page length cuts words in two at every page break, and a word cut there is judged whole.
// A part with no filters ticked accepts everything.
//
// When every part with filters can rank its survivors (sieve/filter.hpp), so can the books: the
// survivors are all (cover, title, body) combinations of surviving parts, counted as
//     N = Nc * Nt * Nb,
// and survivor k is the combination with ranks (kc, kt, kb), k = (kc * Nt + kt) * Nb + kb. Each
// part's ranks follow its own address order, and a book's positional address puts the cover
// first, then the title, then the pages, so survivors keep the books line's positional order.
//
// Compact books ("books-compact-v1") are addressed like a compact line (sieve/compact.hpp):
// positional = the survivor number k; scrambled = shuffle-sha256-v1 of k over [0, N), keyed with
// the page space's key, domain "books-compact-v1/<book space id>/<cover>/<title>/<pages>" where
// each part is its stack's id, or "-" for a part with no filters.
#pragma once

#include "sieve/bookspace.hpp"
#include "sieve/compact.hpp"
#include "sieve/filter.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

class BookSieve
{
public:
    // The stacks are not owned and must outlive this object; any of them may be empty.
    BookSieve(const BookSpace& space, const FilterStack& cover, const FilterStack& title, const FilterStack& pages);

    bool empty() const { return cover_->empty() && title_->empty() && pages_->empty(); }
    // "" if the book passes, else which part fails and the filter, e.g. "pages: words-v2".
    std::string first_failure(const BookSpace::Parts& p) const;

    bool can_rank() const { return ranks_; }
    const std::string& blocker() const { return blocker_; } // why not, if it cannot
    const BigUint& count() const { return count_; }          // surviving books
    BigUint rank(const BookSpace::Parts& p) const;           // throws if the book does not pass
    BookSpace::Parts unrank(const BigUint& k) const;         // k < count()

    static std::vector<uint32_t> body(const BookSpace::Parts& p); // the pages as one text

    // Compact addresses (can_rank() and count() > 0): survivor k at position k, or shuffled.
    BigUint index_of(const BookSpace::Parts& p, AddressMode m) const; // throws if it does not pass
    BookSpace::Parts parts_at(const BigUint& index, AddressMode m) const;
    size_t hex_width() const { return hex_width_; }
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked
    std::string domain() const;

private:
    struct Part
    {
        const FilterStack* stack;
        BigUint count;
        uint32_t base, length;
        BigUint rank(std::span<const uint32_t> u) const;
        std::vector<uint32_t> unrank(const BigUint& k) const;
    };
    const BookSpace* space_;
    const FilterStack *cover_, *title_, *pages_;
    Part parts_[3]; // cover, title, body
    bool ranks_ = false;
    std::string blocker_;
    BigUint count_;
    size_t hex_width_ = 1;
    std::unique_ptr<Shuffle> shuffle_;
};

inline constexpr const char* kBooksCompactVersion = "books-compact-v1";

} // namespace sieve
