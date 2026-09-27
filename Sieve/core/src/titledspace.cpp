// Titled lines (titledspace.hpp): packing a cover, a title and a content index into one number and
// back, and the keyed shuffle over the whole. The cover and the title are each converted in one go
// by BigUint's own conversions, and the three are joined by multiplication, as the books line joins
// its parts.

#include "sieve/titledspace.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

// How many units a part has: base^length, or one (the empty part) when there is no such part.
BigUint count_of(const std::optional<Space>& s) { return s ? BigUint::pow(s->base(), s->unit_length()) : BigUint(1); }

std::string shape_id(const std::optional<Space>& title, const std::optional<Space>& cover, const std::string& content,
                     const std::string& key)
{
    std::string id = "titled/" + content;
    if (title) id += "+title=" + title->symbols_id() + "/L" + std::to_string(title->unit_length());
    if (cover) id += "+cover=" + cover->symbols_id() + "/L" + std::to_string(cover->unit_length());
    return id + "/key=" + key + "/" + kTitledSpaceVersion;
}

BigUint total(const std::optional<Space>& title, const std::optional<Space>& cover, const BigUint& content)
{
    return BigUint::mul(count_of(cover), BigUint::mul(count_of(title), content));
}

} // namespace

TitledSpace::TitledSpace(std::optional<Space> title, std::optional<Space> cover, BigUint content_size, std::string content_shape,
                         std::string key)
    : title_(std::move(title)), cover_(std::move(cover)), content_size_(std::move(content_size)), title_size_(count_of(title_)),
      size_(total(title_, cover_, content_size_)), content_shape_(std::move(content_shape)), key_(std::move(key)),
      shuffle_(size_, key_, shape_id(title_, cover_, content_shape_, key_))
{
    if (content_size_.is_zero()) throw std::invalid_argument("a titled line needs at least one content");
    BigUint top = size_;
    top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::string TitledSpace::id() const { return shape_id(title_, cover_, content_shape_, key_); }

BigUint TitledSpace::index_of(const Parts& p, AddressMode m) const
{
    if (p.title.size() != (title_ ? title_->unit_length() : 0)) throw std::invalid_argument("the title has the wrong length");
    if (cover_ ? p.cover.size() != cover_->unit_length() : !p.cover.empty())
        throw std::invalid_argument("the cover does not have this line's shape");
    if (p.content >= content_size_) throw std::out_of_range("content beyond its line");
    BigUint v = title_ ? BigUint::from_digits(p.title, title_->base()) : BigUint();
    if (cover_)
    {
        BigUint with_cover = BigUint::mul(BigUint::from_digits(p.cover, cover_->base()), title_size_);
        with_cover += v;
        v = std::move(with_cover);
    }
    v = BigUint::mul(v, content_size_);
    v += p.content;
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

TitledSpace::Parts TitledSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("titled address beyond the line");
    const BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    Parts p;
    BigUint rest;
    BigUint::divmod(v, content_size_, rest, p.content);
    BigUint cover, title;
    BigUint::divmod(rest, title_size_, cover, title);
    if (title_) p.title = title.to_digits(title_->base(), title_->unit_length());
    if (cover_) p.cover = cover.to_digits(cover_->base(), cover_->unit_length());
    return p;
}

BigUint TitledSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("titled address beyond the line");
    return v;
}

} // namespace sieve
