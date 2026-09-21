// Potentia — pinned symbol alphabets (SPECIFICATIONS §3).
//
// Every text alphabet places SPACE at digit 0, so the all-space unit is address 0
// and padding is always digit 0.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace potentia {

class Alphabet
{
public:
    Alphabet(std::string id, std::string description, std::u32string symbols);

    const std::string& id() const { return id_; }
    const std::string& description() const { return description_; }
    uint32_t size() const { return static_cast<uint32_t>(symbols_.size()); }
    const std::u32string& symbols() const { return symbols_; }

    char32_t symbol(uint32_t digit) const { return symbols_.at(digit); }
    std::optional<uint32_t> digit_of(char32_t cp) const;
    bool contains(char32_t cp) const { return index_.count(cp) != 0; }

    // True if the alphabet contains any ASCII uppercase letter (disables case folding).
    bool has_uppercase() const { return has_upper_; }

private:
    std::string id_;
    std::string description_;
    std::u32string symbols_;
    std::unordered_map<char32_t, uint32_t> index_;
    bool has_upper_ = false;
};

// Built-in alphabets, versioned with the spec:
//   lower27  — space + a–z
//   babel29  — space + a–z + comma + period (libraryofbabel.info character set)
//   ascii95  — printable ASCII U+0020–U+007E
const Alphabet& alphabet_by_id(std::string_view id); // throws std::invalid_argument
std::vector<std::string> alphabet_ids();

} // namespace potentia
