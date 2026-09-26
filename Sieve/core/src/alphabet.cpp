#include "sieve/alphabet.hpp"

#include <algorithm>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <utility>

namespace sieve {

namespace {

constexpr char32_t kMaxCodePoint = 0x10FFFF;
constexpr char32_t kSurrogateFirst = 0xD800, kSurrogateLast = 0xDFFF;

// One code point, as a range.
constexpr CodeRange one(char32_t c) { return {c, c}; }

} // namespace

Alphabet::Alphabet(std::string id, std::string description, std::vector<CodeRange> ranges)
    : id_(std::move(id)), description_(std::move(description)), ranges_(std::move(ranges))
{
    if (ranges_.empty()) throw std::invalid_argument("alphabet '" + id_ + "' has no symbols");
    uint64_t total = 0;
    for (const CodeRange& r : ranges_)
    {
        if (r.first > r.last || r.last > kMaxCodePoint)
            throw std::invalid_argument("alphabet '" + id_ + "' has a range outside Unicode");
        first_digit_.push_back(uint32_t(total));
        total += r.count();
        if (total > (uint64_t(1) << 31)) throw std::invalid_argument("alphabet '" + id_ + "' has too many symbols");
        if (r.first <= U'Z' && r.last >= U'A') has_upper_ = true;
        if (r.first <= kSurrogateLast && r.last >= kSurrogateFirst) encodable_ = false;
    }
    size_ = uint32_t(total);
    // The ranges in code point order, for digit_of. They are the digit order as given, which is
    // not always ascending (babel29), so the two orders are kept separately.
    by_code_.resize(ranges_.size());
    for (uint32_t i = 0; i < by_code_.size(); ++i) by_code_[i] = i;
    std::sort(by_code_.begin(), by_code_.end(), [&](uint32_t a, uint32_t b) { return ranges_[a].first < ranges_[b].first; });
    for (size_t i = 1; i < by_code_.size(); ++i)
        if (ranges_[by_code_[i]].first <= ranges_[by_code_[i - 1]].last)
            throw std::invalid_argument("alphabet '" + id_ + "' has overlapping ranges");
}

char32_t Alphabet::symbol(uint32_t digit) const
{
    if (digit >= size_) throw std::out_of_range("digit " + std::to_string(digit) + " is not in alphabet '" + id_ + "'");
    // The last range whose first digit is at or below this one.
    const auto it = std::upper_bound(first_digit_.begin(), first_digit_.end(), digit);
    const size_t i = size_t(it - first_digit_.begin()) - 1;
    return ranges_[i].first + (digit - first_digit_[i]);
}

std::optional<uint32_t> Alphabet::digit_of(char32_t cp) const
{
    // The last range (in code point order) that starts at or below cp.
    const auto it = std::upper_bound(by_code_.begin(), by_code_.end(), cp,
                                     [&](char32_t c, uint32_t r) { return c < ranges_[r].first; });
    if (it == by_code_.begin()) return std::nullopt;
    const uint32_t i = *(it - 1);
    if (cp > ranges_[i].last) return std::nullopt;
    return first_digit_[i] + uint32_t(cp - ranges_[i].first);
}

// ---------------------------------------------------------------- the built-in alphabets

namespace {

const std::vector<Alphabet>& registry()
{
    static const std::vector<Alphabet> alphabets = {
        Alphabet("lower27", "space + a-z", {one(U' '), {U'a', U'z'}}),
        // Pinned in libraryofbabel.info's order, which is not code point order: the comma and
        // the period come after the letters. Never reorder this one.
        Alphabet("babel29", "space + a-z + comma + period", {one(U' '), {U'a', U'z'}, one(U','), one(U'.')}),
        Alphabet("ascii95", "printable ASCII U+0020-U+007E", {{U' ', U'~'}}),
        Alphabet("ascii96", "line feed + printable ASCII: text that can hold a file, such as an .obj",
                 {one(U'\n'), {U' ', U'~'}}),
        // One symbol per byte value, in byte order: digit d is byte d. A file of N bytes is one
        // unit of a line of length N, and the unit's address is the file's bytes read as one
        // number. Nothing is folded, dropped or collapsed over it (canon-bytes-v1).
        Alphabet("bytes256", "U+0000-U+00FF: one symbol per byte value, so any file is a unit",
                 {{char32_t(0), char32_t(0xFF)}}),
    };
    return alphabets;
}

} // namespace

bool holds_all_bytes(const Alphabet& a)
{
    return a.ranges().size() == 1 && a.ranges()[0].first == char32_t(0) && a.ranges()[0].last == char32_t(0xFF);
}

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

// ---------------------------------------------------------------- stacking

namespace {

// "u+0370" or "0370" (hex, 1 to 6 digits).
char32_t parse_code_point(std::string_view s, std::string_view whole)
{
    if (s.size() > 2 && (s[0] == 'u' || s[0] == 'U') && s[1] == '+') s.remove_prefix(2);
    if (s.empty() || s.size() > 6) throw std::invalid_argument("'" + std::string(whole) + "' is not a code point or a block name");
    char32_t cp = 0;
    for (const char c : s)
    {
        const int d = c >= '0' && c <= '9'   ? c - '0'
                      : c >= 'a' && c <= 'f' ? c - 'a' + 10
                      : c >= 'A' && c <= 'F' ? c - 'A' + 10
                                             : -1;
        if (d < 0) throw std::invalid_argument("'" + std::string(whole) + "' is not a code point or a block name");
        cp = cp * 16 + char32_t(d);
    }
    if (cp > kMaxCodePoint) throw std::invalid_argument("'" + std::string(whole) + "' is past the end of Unicode");
    return cp;
}

// One part of a stack: a block id, a single code point, or "first-last". Because a code point
// may be written "u+0370", the dash is looked for after any "u+" prefixes.
CodeRange parse_part(std::string_view part)
{
    if (const Block* b = block_by_id(part)) return b->range;
    size_t dash = std::string_view::npos;
    for (size_t i = 1; i < part.size(); ++i)
        if (part[i] == '-' && part[i - 1] != '+') { dash = i; break; }
    if (dash == std::string_view::npos)
    {
        const char32_t cp = parse_code_point(part, part);
        return {cp, cp};
    }
    const char32_t a = parse_code_point(part.substr(0, dash), part), b = parse_code_point(part.substr(dash + 1), part);
    if (a > b) throw std::invalid_argument("'" + std::string(part) + "' runs backwards");
    return {a, b};
}

} // namespace

const Alphabet& alphabet_of(std::string_view spec)
{
    static std::mutex lock;
    static std::deque<Alphabet> made; // never moves what it already holds, so references stay good
    {
        const std::lock_guard<std::mutex> held(lock);
        for (const Alphabet& a : made)
            if (a.id() == spec) return a;
    }
    for (const Alphabet& a : registry())
        if (a.id() == spec) return a;
    if (spec.empty()) throw std::invalid_argument("no alphabet given");

    // Split on '+', but not the '+' of a "u+XXXX" code point.
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= spec.size(); ++i)
    {
        const bool end = i == spec.size();
        const bool sep = !end && spec[i] == '+' && !(i > 0 && (spec[i - 1] == 'u' || spec[i - 1] == 'U'));
        if (!end && !sep) continue;
        if (i > start) parts.emplace_back(spec.substr(start, i - start));
        start = i + 1;
    }
    if (parts.empty()) throw std::invalid_argument("no alphabet given");

    std::vector<CodeRange> got;
    for (const std::string& p : parts) got.push_back(parse_part(p));
    // Union: sort by first code point, then merge anything that touches or overlaps, so a block
    // stacked twice, or a block inside another, adds each symbol exactly once.
    std::sort(got.begin(), got.end(), [](const CodeRange& a, const CodeRange& b) {
        return a.first != b.first ? a.first < b.first : a.last < b.last;
    });
    std::vector<CodeRange> merged;
    for (const CodeRange& r : got)
    {
        if (!merged.empty() && r.first <= merged.back().last + 1) merged.back().last = std::max(merged.back().last, r.last);
        else merged.push_back(r);
    }
    // The canonical id: the parts sorted and deduplicated, so the same alphabet is always named
    // the same however it was asked for.
    std::sort(parts.begin(), parts.end());
    parts.erase(std::unique(parts.begin(), parts.end()), parts.end());
    std::string id, description;
    for (const std::string& p : parts)
    {
        if (!id.empty()) id += '+';
        id += p;
        if (!description.empty()) description += " + ";
        const Block* b = block_by_id(p);
        description += b ? std::string(b->name) : p;
    }
    {
        const std::lock_guard<std::mutex> held(lock);
        for (const Alphabet& a : made)
            if (a.id() == id) return a;
        made.emplace_back(id, description, std::move(merged));
        return made.back();
    }
}

} // namespace sieve
