#include "sieve/bookspace.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

// Every page after the cover, the title included, is a unit of the same page space, so the title
// and the pages together are one run of digits in one base, and a book's index is the cover's
// digits followed by that run: cover * |page|^run + run. Packing and unpacking a book is one
// conversion of each part, with one multiplication or division between them.
uint64_t run_length(const Space& page, uint32_t pages) { return (uint64_t(pages) + 1) * page.unit_length(); }

BigUint book_count(const Space& cover, const Space& page, uint32_t pages)
{
    return BigUint::mul(BigUint::pow(cover.base(), cover.unit_length()), BigUint::pow(page.base(), run_length(page, pages)));
}

std::string shape_id(const Space& cover, const Space& page, uint32_t pages)
{
    return "books/" + cover.symbols_id() + "/L" + std::to_string(cover.unit_length()) + "+" + page.symbols_id() + "/L" +
           std::to_string(page.unit_length()) + "x" + std::to_string(uint64_t(pages) + 1) + "/key=" + page.key() + "/" + kBookSpaceVersion;
}

} // namespace

BookSpace::BookSpace(const Space& cover, const Space& page, uint32_t pages)
    : cover_(cover), page_(page), pages_(pages), size_(book_count(cover, page, pages)),
      hex_width_(0), shuffle_(size_, page.key(), shape_id(cover, page, pages))
{
    BigUint top = size_;
    top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::string BookSpace::id() const { return shape_id(cover_, page_, pages_); }

BigUint BookSpace::index_of(const Parts& p, AddressMode m) const
{
    if (p.cover.size() != cover_.unit_length() || p.title.size() != page_.unit_length() || p.pages.size() != pages_)
        throw std::invalid_argument("the book does not have this line's shape");
    Digits run;
    run.reserve(size_t(run_length(page_, pages_)));
    run.insert(run.end(), p.title.begin(), p.title.end());
    for (const auto& pg : p.pages)
    {
        if (pg.size() != page_.unit_length()) throw std::invalid_argument("a part has the wrong length");
        run.insert(run.end(), pg.begin(), pg.end());
    }
    BigUint v = BigUint::mul(BigUint::from_digits(p.cover, cover_.base()), BigUint::pow(page_.base(), run.size()));
    v += BigUint::from_digits(run, page_.base());
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

BookSpace::Parts BookSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("book address beyond the line");
    const BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    BigUint cover, rest;
    BigUint::divmod(v, BigUint::pow(page_.base(), run_length(page_, pages_)), cover, rest);
    Parts p;
    p.cover = cover.to_digits(cover_.base(), cover_.unit_length());
    const Digits run = rest.to_digits(page_.base(), size_t(run_length(page_, pages_)));
    const auto L = std::ptrdiff_t(page_.unit_length());
    p.title.assign(run.begin(), run.begin() + L);
    p.pages.resize(pages_);
    for (size_t k = 0; k < pages_; ++k)
        p.pages[k].assign(run.begin() + L * std::ptrdiff_t(k + 1), run.begin() + L * std::ptrdiff_t(k + 2));
    return p;
}

BigUint BookSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("book address beyond the line");
    return v;
}

} // namespace sieve
