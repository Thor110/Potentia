// Sieve — pinned symbol alphabets (SPECIFICATIONS §3).
//
// An alphabet is an ordered list of Unicode code point ranges. Digit 0 is the first code point
// of the first range, and the digits run on through each range in turn, so the ranges are the
// digit order and not merely a set. That order is PINNED: it is what every address on a line
// means, so an alphabet's ranges may never be edited once published. A correction is a new
// alphabet under a new id, exactly as a changed filter is a new version.
//
// Alphabets built from Unicode blocks (see block_ids) order their code points ascending, so
// digit 0 is the lowest of them. The three original alphabets keep the order they were pinned
// with: lower27 and ascii95 happen to be ascending; babel29 is not, because it follows the
// order libraryofbabel.info uses (space, a-z, comma, period).
//
// Surrogates (U+D800..U+DFFF) are code points like any other here. They can be addressed,
// counted, filtered and drawn (as the replacement glyph), but they have no UTF-8 encoding, so
// a unit holding one has no text form to warp in from or write out. See text_is_encodable.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

// An inclusive range of code points. first <= last, and last <= U+10FFFF.
struct CodeRange
{
    char32_t first = 0, last = 0;
    uint32_t count() const { return uint32_t(last - first) + 1; }
    bool operator==(const CodeRange&) const = default;
};

class Alphabet
{
public:
    // The ranges are the digit order. They may not overlap, and together must hold at least one
    // code point and no more than 2^31. Throws std::invalid_argument otherwise.
    Alphabet(std::string id, std::string description, std::vector<CodeRange> ranges);

    const std::string& id() const { return id_; }
    const std::string& description() const { return description_; }
    uint32_t size() const { return size_; }
    const std::vector<CodeRange>& ranges() const { return ranges_; }

    char32_t symbol(uint32_t digit) const; // throws std::out_of_range past the end
    std::optional<uint32_t> digit_of(char32_t cp) const;
    bool contains(char32_t cp) const { return digit_of(cp).has_value(); }

    // True if the alphabet contains any ASCII uppercase letter (disables case folding).
    bool has_uppercase() const { return has_upper_; }
    // False if any of its code points is a surrogate, so units over it may have no text form.
    bool text_is_encodable() const { return encodable_; }

private:
    std::string id_;
    std::string description_;
    std::vector<CodeRange> ranges_;
    std::vector<uint32_t> first_digit_; // digit of each range's first code point, ascending
    std::vector<uint32_t> by_code_;     // indices into ranges_, ordered by first code point
    uint32_t size_ = 0;
    bool has_upper_ = false;
    bool encodable_ = true;
};

// A named Unicode block that can be chosen on its own or stacked with others.
struct Block
{
    std::string_view id;          // "greek", "cjk-unified"
    std::string_view name;        // "Greek"
    CodeRange range;
    std::string_view description; // what it is, for the listing and the menu
};
const std::vector<Block>& blocks();
const Block* block_by_id(std::string_view id); // null if there is no such block
std::vector<std::string> block_ids();

// Built-in alphabets, versioned with the spec:
//   lower27  — space + a–z
//   babel29  — space + a–z + comma + period (libraryofbabel.info character set)
//   ascii95  — printable ASCII U+0020–U+007E
//   ascii96  — line feed + printable ASCII (text that can hold a file, such as an .obj)
//   bytes256 — U+0000-U+00FF: one symbol per byte value, so a file of N bytes is one unit of
//              a line of length N, exactly and reversibly, with nothing folded or dropped
std::vector<std::string> alphabet_ids();

// True if an alphabet is exactly the 256 code points U+0000-U+00FF in that order, so that digit
// d is byte d. Content for such a line is read and written as raw bytes rather than as UTF-8
// text, and canonicalisation over it is the identity (see canon.hpp: canon-bytes-v1).
bool holds_all_bytes(const Alphabet& a);

// A built-in id ("lower27"), or blocks and ranges stacked with '+' ("greek+cyrillic",
// "latin-1-supplement+u+0370-u+03ff"). A stack's code points are unioned, sorted ascending and
// deduplicated, so the order the parts are given in does not change the result, and its id is
// the parts sorted and deduplicated, so the same parts always give the same name.
// Throws std::invalid_argument on an unknown part or a malformed range.
// The result is kept for the life of the process, so a Space may hold on to it as the built-in
// alphabets are held on to.
const Alphabet& alphabet_of(std::string_view spec);
// The built-ins, by id. Throws std::invalid_argument if the id is not one of them.
const Alphabet& alphabet_by_id(std::string_view id);

} // namespace sieve
