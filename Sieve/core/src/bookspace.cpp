#include "sieve/bookspace.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

// Digits in chunks: as many as fit a 32-bit multiplier (base^k <= 2^32 - 1), one big-number
// operation per chunk instead of one per digit.
uint32_t chunk_digits(uint32_t base)
{
    uint32_t k = 1;
    for (uint64_t p = base; p * base < (uint64_t(1) << 32); p *= base) ++k;
    return k;
}

uint32_t small_pow(uint32_t base, uint32_t k)
{
    uint64_t p = 1;
    for (uint32_t i = 0; i < k; ++i) p *= base;
    return uint32_t(p);
}

void mul_pow(BigUint& n, uint32_t base, uint64_t e)
{
    const uint32_t k = chunk_digits(base), full = small_pow(base, k);
    for (; e >= k; e -= k) n.mul_small(full);
    for (; e > 0; --e) n.mul_small(base);
}

BigUint book_count(const Space& cover, const Space& page, uint32_t pages)
{
    BigUint n(1);
    mul_pow(n, cover.base(), cover.unit_length());
    mul_pow(n, page.base(), uint64_t(pages + 1) * page.unit_length());
    return n;
}

std::string shape_id(const Space& cover, const Space& page, uint32_t pages)
{
    return "books/" + cover.symbols_id() + "/L" + std::to_string(cover.unit_length()) + "+" + page.symbols_id() + "/L" +
           std::to_string(page.unit_length()) + "x" + std::to_string(pages + 1) + "/key=" + page.key() + "/" + kBookSpaceVersion;
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
    BigUint v;
    auto put = [&](const Digits& d, const Space& sp) {
        if (d.size() != sp.unit_length()) throw std::invalid_argument("a part has the wrong length");
        const uint32_t b = sp.base(), k = chunk_digits(b);
        for (size_t i = 0; i < d.size();)
        {
            const size_t take = std::min<size_t>(k, d.size() - i);
            uint64_t chunk = 0;
            for (size_t j = 0; j < take; ++j, ++i)
            {
                if (d[i] >= b) throw std::invalid_argument("digit out of range");
                chunk = chunk * b + d[i];
            }
            v.mul_small(small_pow(b, uint32_t(take)));
            v.add_small(uint32_t(chunk));
        }
    };
    put(p.cover, cover_);
    put(p.title, page_);
    for (const auto& pg : p.pages) put(pg, page_);
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

BookSpace::Parts BookSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("book address beyond the line");
    BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    Parts p;
    auto take = [&](Digits& d, const Space& sp) { // least significant digits first, a chunk per division
        const uint32_t b = sp.base(), k = chunk_digits(b);
        d.assign(sp.unit_length(), 0);
        for (size_t i = d.size(); i > 0;)
        {
            const uint32_t n = uint32_t(std::min<size_t>(k, i));
            uint32_t r = v.divmod_small(small_pow(b, n));
            for (uint32_t j = 0; j < n; ++j)
            {
                d[--i] = r % b;
                r /= b;
            }
        }
    };
    p.pages.resize(pages_);
    for (size_t k = pages_; k-- > 0;) take(p.pages[k], page_);
    take(p.title, page_);
    take(p.cover, cover_);
    return p;
}

BigUint BookSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("book address beyond the line");
    return v;
}

} // namespace sieve
