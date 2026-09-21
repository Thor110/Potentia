#include "potentia/alphabet.hpp"

#include <stdexcept>

namespace potentia {

Alphabet::Alphabet(std::string id, std::string description, std::u32string symbols)
    : id_(std::move(id)), description_(std::move(description)), symbols_(std::move(symbols))
{
    if (symbols_.empty() || symbols_[0] != U' ')
        throw std::invalid_argument("alphabet '" + id_ + "' must start with SPACE (digit 0)");
    for (uint32_t i = 0; i < symbols_.size(); ++i)
    {
        if (!index_.emplace(symbols_[i], i).second)
            throw std::invalid_argument("alphabet '" + id_ + "' contains a duplicate symbol");
        if (symbols_[i] >= U'A' && symbols_[i] <= U'Z') has_upper_ = true;
    }
}

std::optional<uint32_t> Alphabet::digit_of(char32_t cp) const
{
    auto it = index_.find(cp);
    if (it == index_.end()) return std::nullopt;
    return it->second;
}

namespace {

std::u32string ascii_range(char32_t first, char32_t last)
{
    std::u32string s;
    for (char32_t c = first; c <= last; ++c) s.push_back(c);
    return s;
}

const std::vector<Alphabet>& registry()
{
    static const std::vector<Alphabet> alphabets = {
        Alphabet("lower27", "space + a-z", U" " + ascii_range(U'a', U'z')),
        Alphabet("babel29", "space + a-z + comma + period", U" " + ascii_range(U'a', U'z') + U",."),
        Alphabet("ascii95", "printable ASCII U+0020-U+007E", ascii_range(U' ', U'~')),
    };
    return alphabets;
}

} // namespace

const Alphabet& alphabet_by_id(std::string_view id)
{
    for (const auto& a : registry())
        if (a.id() == id) return a;
    throw std::invalid_argument("unknown alphabet '" + std::string(id) + "'");
}

std::vector<std::string> alphabet_ids()
{
    std::vector<std::string> ids;
    for (const auto& a : registry()) ids.push_back(a.id());
    return ids;
}

} // namespace potentia
