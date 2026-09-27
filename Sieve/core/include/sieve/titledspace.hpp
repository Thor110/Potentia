// Sieve — titled lines: every unit of a line with a title, and on some lines a cover
// (SPECIFICATIONS §11, "Titled lines").
//
// A titled unit is a cover (optional: one unit of a picture space), a title (one unit of a title
// space) and its content, which is one unit of the line as it was. The content is given by its
// positional index on its own line, so any line can be titled -- a unit line, whose index is its
// digits read as one number, or the models line, whose index is ModelSpace's -- and this class
// needs to know nothing about it but how many there are and what shape they are. So there are
//     N = |cover| * |title| * |content|
// titled units, and "titled-v1" numbers them as the books line numbers books:
//   positional  (cover * |title| + title) * |content| + content, the cover and the title each
//               read as one number of their own digits. Neighbours differ in their content.
//   scrambled   the positional index passed through shuffle-sha256-v1 over [0, N), keyed with
//               the line's key, domain = the titled line's id. Neighbours are unrelated.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/space.hpp"

#include <optional>
#include <string>
#include <string_view>

namespace sieve {

inline constexpr const char* kTitledSpaceVersion = "titled-v1";

class TitledSpace
{
public:
    using Digits = Space::Digits;
    struct Parts
    {
        Digits cover;    // empty on a line without covers
        Digits title;    // empty when the title length is 0
        BigUint content; // the content's positional index on its own line, < content_size()
        bool operator==(const Parts&) const = default;
    };

    // `content_shape` names the content's shape for the id: "<symbols>/L<n>" for a unit line,
    // "models/V<v>/F<f>/C<c>" for the models line.
    // A title length of 0 is no title: pass std::nullopt.
    TitledSpace(std::optional<Space> title, std::optional<Space> cover, BigUint content_size, std::string content_shape,
                std::string key);

    const std::optional<Space>& title_space() const { return title_; }
    const std::optional<Space>& cover_space() const { return cover_; }
    const BigUint& content_size() const { return content_size_; }
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    // "titled/<content>[+title=<title symbols>/L<T>][+cover=<cover symbols>/L<n>]/key=<key>/titled-v1"
    std::string id() const;

    BigUint index_of(const Parts& p, AddressMode m) const;
    Parts parts_at(const BigUint& index, AddressMode m) const; // index < size()
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked

    // A title or cover of all zero digits: what a unit warped in on its own carries.
    Digits blank_title() const { return title_ ? Digits(title_->unit_length(), 0) : Digits{}; }
    Digits blank_cover() const { return cover_ ? Digits(cover_->unit_length(), 0) : Digits{}; }

private:
    std::optional<Space> title_;
    std::optional<Space> cover_;
    BigUint content_size_, title_size_, size_;
    std::string content_shape_, key_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
};

} // namespace sieve
