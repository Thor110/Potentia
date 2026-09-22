#include "sieve/booksieve.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

BookSieve::BookSieve(const BookSpace& space, const FilterStack& cover, const FilterStack& title, const FilterStack& pages)
    : space_(&space), cover_(&cover), title_(&title), pages_(&pages)
{
    const Space& c = space.cover_space();
    const Space& p = space.page_space();
    const uint32_t body = uint32_t(uint64_t(space.pages()) * p.unit_length());
    parts_[0] = {&cover, {}, c.base(), c.unit_length()};
    parts_[1] = {&title, {}, p.base(), p.unit_length()};
    parts_[2] = {&pages, {}, p.base(), body};
    const char* names[3] = {"cover", "title", "pages"};
    ranks_ = true;
    for (int i = 0; i < 3; ++i)
    {
        Part& part = parts_[i];
        if (part.stack->empty()) part.count = BigUint::pow(part.base, part.length);
        else if (part.stack->ranker()) part.count = part.stack->ranker()->count();
        else
        {
            ranks_ = false;
            if (blocker_.empty()) blocker_ = std::string(names[i]) + ": " + part.stack->compact_blocker();
        }
    }
    if (ranks_) count_ = BigUint::mul(BigUint::mul(parts_[0].count, parts_[1].count), parts_[2].count);
    if (ranks_ && !count_.is_zero())
    {
        BigUint top = count_;
        top -= BigUint(1);
        hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
        shuffle_ = std::make_unique<Shuffle>(count_, p.key(), domain());
    }
}

std::string BookSieve::domain() const
{
    auto part = [](const FilterStack* st) { return st->empty() ? std::string("-") : st->id(); };
    return std::string(kBooksCompactVersion) + "/" + space_->id() + "/" + part(cover_) + "/" + part(title_) + "/" + part(pages_);
}

BigUint BookSieve::index_of(const BookSpace::Parts& p, AddressMode m) const
{
    const BigUint k = rank(p);
    return m == AddressMode::Scrambled ? shuffle_->forward(k) : k;
}

BookSpace::Parts BookSieve::parts_at(const BigUint& index, AddressMode m) const
{
    if (!shuffle_) throw std::logic_error("no compact books here");
    if (index >= count_) throw std::out_of_range("beyond the surviving books");
    return unrank(m == AddressMode::Scrambled ? shuffle_->inverse(index) : index);
}

BigUint BookSieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= count_) throw std::out_of_range("beyond the surviving books");
    return v;
}

std::vector<uint32_t> BookSieve::body(const BookSpace::Parts& p)
{
    std::vector<uint32_t> b;
    for (const auto& pg : p.pages) b.insert(b.end(), pg.begin(), pg.end());
    return b;
}

std::string BookSieve::first_failure(const BookSpace::Parts& p) const
{
    const Space& ps = space_->page_space();
    bool shape = p.cover.size() == space_->cover_space().unit_length() && p.title.size() == ps.unit_length() && p.pages.size() == space_->pages();
    for (const auto& pg : p.pages) shape = shape && pg.size() == ps.unit_length();
    for (uint32_t d : p.cover) shape = shape && d < space_->cover_space().base();
    for (uint32_t d : p.title) shape = shape && d < ps.base();
    for (const auto& pg : p.pages)
        for (uint32_t d : pg) shape = shape && d < ps.base();
    if (!shape) throw std::invalid_argument("the book does not have this line's shape");
    if (int f = cover_->first_failure(p.cover); f >= 0) return "cover: " + cover_->filter_name(size_t(f));
    if (int f = title_->first_failure(p.title); f >= 0) return "title: " + title_->filter_name(size_t(f));
    if (!pages_->empty())
        if (int f = pages_->first_failure(body(p)); f >= 0) return "pages: " + pages_->filter_name(size_t(f));
    return "";
}

BigUint BookSieve::Part::rank(std::span<const uint32_t> u) const
{
    if (stack->empty()) return BigUint::from_digits(u, base);
    return stack->ranker()->rank(u);
}

std::vector<uint32_t> BookSieve::Part::unrank(const BigUint& k) const
{
    if (stack->empty()) return k.to_digits(base, length);
    return stack->ranker()->unrank(k);
}

BigUint BookSieve::rank(const BookSpace::Parts& p) const
{
    if (!ranks_) throw std::logic_error("these book filters cannot rank: " + blocker_);
    if (const std::string f = first_failure(p); !f.empty()) throw std::invalid_argument("the book does not pass (" + f + ")");
    const std::vector<uint32_t> b = body(p);
    BigUint k = BigUint::mul(parts_[0].rank(p.cover), parts_[1].count);
    k += parts_[1].rank(p.title);
    k = BigUint::mul(k, parts_[2].count);
    k += parts_[2].rank(b);
    return k;
}

BookSpace::Parts BookSieve::unrank(const BigUint& k) const
{
    if (!ranks_) throw std::logic_error("these book filters cannot rank: " + blocker_);
    if (k >= count_) throw std::out_of_range("beyond the surviving books");
    BigUint rest, kb, kc, kt;
    BigUint::divmod(k, parts_[2].count, rest, kb);
    BigUint::divmod(rest, parts_[1].count, kc, kt);
    BookSpace::Parts p;
    p.cover = parts_[0].unrank(kc);
    p.title = parts_[1].unrank(kt);
    const std::vector<uint32_t> b = parts_[2].unrank(kb);
    const uint32_t L = space_->page_space().unit_length();
    for (uint32_t i = 0; i < space_->pages(); ++i) p.pages.emplace_back(b.begin() + size_t(i) * L, b.begin() + size_t(i + 1) * L);
    return p;
}

} // namespace sieve
