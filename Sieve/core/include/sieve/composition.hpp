// Sieve — compositions: a dimension made of another's units (SPECIFICATIONS §11.4).
//
// A composition is a cover (one unit of the image line), a title (one unit of the titled lines'
// title space, or none when titles are off) and N units of a base line, in that order: a track is N
// units of audio, a movie N units of video. So a composition dimension holds
//     |cover| * |title| * |unit|^N
// compositions, and "composition-v1" numbers them as bookspace-v1 numbers books:
//   positional  the parts read as one mixed-radix number, most significant first: the cover's digits
//               (base |cover symbols|), the title's (base |title symbols|), then every unit's in turn
//               (base |unit symbols|). Neighbours differ in the last unit.
//   scrambled   the positional index passed through shuffle-sha256-v1 (sieve/compact.hpp) over
//               [0, N), keyed with the unit space's key, domain = the composition space's id.
// Addresses are that index in hex, zero-padded to the width N - 1 needs.
//
// The units are the base line's own content (a unit of audio, of video); the cover and title are
// the composition's, so a track has its own as well as each of its units keeping theirs on the audio
// line. Books predate this and keep bookspace-v1 (a book's title is a whole page, from the page
// space); their addresses are unchanged.
//
// Filters ("composition-compact-v1"): a composition passes when its cover passes the cover stack,
// its title the title stack, and every unit the unit stack, each unit judged on its own. When every
// part with filters can rank its survivors, so can the compositions: there are
//     Nc * Nt * Nu^N
// of them, and survivor k has ranks (kc, kt, ku1 ... kuN) as one mixed-radix number, cover first,
// so survivors keep the positional order. Compact compositions are addressed like compact books:
// positional = k; scrambled = shuffle-sha256-v1 of k over [0, count), keyed with the unit space's
// key, domain "composition-compact-v1/<space id>/<cover>/<title>/<unit>", each part its stack's id or
// "-" for a part with no filters. A part with no filters accepts everything.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/filter.hpp"
#include "sieve/space.hpp"

#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kCompositionVersion = "composition-v1";
inline constexpr const char* kCompositionCompactVersion = "composition-compact-v1";

class CompositionSpace
{
public:
    using Digits = Space::Digits;
    struct Parts
    {
        Digits cover, title; // title empty when the space has none
        std::vector<Digits> units;
        bool operator==(const Parts&) const = default;
    };

    // `kind` names the dimension in the id ("tracks", "movies"); `title` is nullopt for no title.
    CompositionSpace(std::string kind, const Space& cover, std::optional<Space> title, const Space& unit, uint32_t units);

    const std::string& kind() const { return kind_; }
    const Space& cover_space() const { return cover_; }
    const std::optional<Space>& title_space() const { return title_; }
    const Space& unit_space() const { return unit_; }
    uint32_t units() const { return units_; }
    const BigUint& size() const { return size_; }
    size_t hex_width() const { return hex_width_; }
    // "<kind>/<cover symbols>/L<n>+<title symbols>/L<n>+<unit symbols>/L<n>x<N>/key=<key>/composition-v1",
    // the title written "-" when there is none.
    std::string id() const;

    BigUint index_of(const Parts& p, AddressMode m) const; // throws if p is not of this shape
    Parts parts_at(const BigUint& index, AddressMode m) const; // index < size()
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked
    void check(const Parts& p) const;          // throws std::invalid_argument if p is not of this shape

private:
    std::string kind_;
    Space cover_;
    std::optional<Space> title_;
    Space unit_;
    uint32_t units_;
    BigUint size_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
};

// A composition's units as one longer unit of their line, as it is shown, played and saved: a unit
// of audio is `strands` runs one after another (voices, or a sound's channels), so the joined unit
// is strand 1 of every unit in turn, then strand 2 of every unit, and so on. One strand (video, one
// voice) is plain concatenation. split_units() is its inverse; both throw on a length that does
// not divide.
std::vector<uint32_t> join_units(const std::vector<std::vector<uint32_t>>& units, uint32_t strands);
std::vector<std::vector<uint32_t>> split_units(std::span<const uint32_t> joined, uint32_t strands, uint32_t units);

class CompositionSieve
{
public:
    // The stacks are not owned and must outlive this object; any of them may be empty. `title` is
    // ignored when the space has no title.
    CompositionSieve(const CompositionSpace& space, const FilterStack& cover, const FilterStack& title, const FilterStack& unit);

    bool empty() const { return cover_->empty() && title_->empty() && unit_->empty(); }
    // "" if it passes, else which part fails and the filter: "cover: palette-size-v1", "unit 3: ...".
    std::string first_failure(const CompositionSpace::Parts& p) const;

    bool can_rank() const { return ranks_; }
    const std::string& blocker() const { return blocker_; } // why not, if it cannot
    const BigUint& count() const { return count_; }          // surviving compositions
    BigUint rank(const CompositionSpace::Parts& p) const;    // throws if it does not pass
    CompositionSpace::Parts unrank(const BigUint& k) const;  // k < count()

    // Compact addresses (can_rank() and count() > 0): survivor k at position k, or shuffled.
    BigUint index_of(const CompositionSpace::Parts& p, AddressMode m) const;
    BigUint index_of_rank(const BigUint& k, AddressMode m) const;
    CompositionSpace::Parts parts_at(const BigUint& index, AddressMode m) const;
    size_t hex_width() const { return hex_width_; }
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked
    std::string domain() const;

private:
    struct Part
    {
        const FilterStack* stack = nullptr;
        BigUint count;
        uint32_t base = 1, length = 0;
        BigUint rank(std::span<const uint32_t> u) const;
        std::vector<uint32_t> unrank(const BigUint& k) const;
    };
    const CompositionSpace* space_;
    const FilterStack *cover_, *title_, *unit_;
    Part parts_[3]; // cover, title (length 0 when there is none: count 1), one unit
    BigUint units_count_; // parts_[2].count ^ N
    bool ranks_ = false;
    std::string blocker_;
    BigUint count_;
    size_t hex_width_ = 1;
    std::unique_ptr<Shuffle> shuffle_;
};

} // namespace sieve
