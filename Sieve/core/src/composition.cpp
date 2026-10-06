#include "sieve/composition.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

BigUint unit_count(const Space& s) { return BigUint::pow(s.base(), s.unit_length()); }

std::string part_id(const Space& s) { return s.symbols_id() + "/L" + std::to_string(s.unit_length()); }

std::string shape_id(const std::string& kind, const Space& cover, const std::optional<Space>& title, const Space& unit, uint32_t units)
{
    return kind + "/" + part_id(cover) + "+" + (title ? part_id(*title) : std::string("-")) + "+" + part_id(unit) + "x" +
           std::to_string(units) + "/key=" + unit.key() + "/" + kCompositionVersion;
}

// b^e by squaring: the surviving units' count to the power of the units.
BigUint power(const BigUint& b, uint32_t e)
{
    BigUint result(1), sq = b;
    for (; e; e >>= 1)
    {
        if (e & 1) result = BigUint::mul(result, sq);
        if (e > 1) sq = BigUint::mul(sq, sq);
    }
    return result;
}

size_t width_of(const BigUint& size)
{
    BigUint top = size;
    top -= BigUint(1);
    return std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

BigUint size_of(const Space& cover, const std::optional<Space>& title, const Space& unit, uint32_t units)
{
    BigUint n = unit_count(cover);
    if (title) n = BigUint::mul(n, unit_count(*title));
    return BigUint::mul(n, BigUint::pow(unit.base(), uint64_t(units) * unit.unit_length()));
}

} // namespace

CompositionSpace::CompositionSpace(std::string kind, const Space& cover, std::optional<Space> title, const Space& unit, uint32_t units)
    : kind_(std::move(kind)), cover_(cover), title_(std::move(title)), unit_(unit), units_(units),
      size_(size_of(cover_, title_, unit_, units_)), hex_width_(width_of(size_)), shuffle_(size_, unit_.key(), shape_id(kind_, cover_, title_, unit_, units_))
{
    if (units_ == 0) throw std::invalid_argument("a composition holds at least one unit");
}

std::string CompositionSpace::id() const { return shape_id(kind_, cover_, title_, unit_, units_); }

void CompositionSpace::check(const Parts& p) const
{
    auto fits = [](const Digits& d, const Space& s) {
        if (d.size() != s.unit_length()) return false;
        return std::all_of(d.begin(), d.end(), [&](uint32_t x) { return x < s.base(); });
    };
    bool ok = fits(p.cover, cover_) && (title_ ? fits(p.title, *title_) : p.title.empty()) && p.units.size() == units_;
    for (const Digits& u : p.units) ok = ok && fits(u, unit_);
    if (!ok) throw std::invalid_argument("the composition does not have this dimension's shape");
}

BigUint CompositionSpace::index_of(const Parts& p, AddressMode m) const
{
    check(p);
    BigUint v = BigUint::from_digits(p.cover, cover_.base());
    if (title_)
    {
        v = BigUint::mul(v, unit_count(*title_));
        v += BigUint::from_digits(p.title, title_->base());
    }
    Digits run;
    run.reserve(size_t(units_) * unit_.unit_length());
    for (const Digits& u : p.units) run.insert(run.end(), u.begin(), u.end());
    v = BigUint::mul(v, BigUint::pow(unit_.base(), run.size()));
    v += BigUint::from_digits(run, unit_.base());
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

CompositionSpace::Parts CompositionSpace::parts_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("address beyond the dimension");
    const BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    const uint64_t run_length = uint64_t(units_) * unit_.unit_length();
    BigUint head, rest;
    BigUint::divmod(v, BigUint::pow(unit_.base(), run_length), head, rest);
    Parts p;
    if (title_)
    {
        BigUint cover, title;
        BigUint::divmod(head, unit_count(*title_), cover, title);
        p.cover = cover.to_digits(cover_.base(), cover_.unit_length());
        p.title = title.to_digits(title_->base(), title_->unit_length());
    }
    else p.cover = head.to_digits(cover_.base(), cover_.unit_length());
    const Digits run = rest.to_digits(unit_.base(), size_t(run_length));
    const auto L = std::ptrdiff_t(unit_.unit_length());
    p.units.resize(units_);
    for (size_t k = 0; k < units_; ++k) p.units[k].assign(run.begin() + L * std::ptrdiff_t(k), run.begin() + L * std::ptrdiff_t(k + 1));
    return p;
}

BigUint CompositionSpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("address beyond the dimension");
    return v;
}

std::vector<uint32_t> join_units(const std::vector<std::vector<uint32_t>>& units, uint32_t strands)
{
    if (strands == 0) throw std::invalid_argument("a unit has at least one strand");
    std::vector<uint32_t> out;
    if (units.empty()) return out;
    const size_t len = units[0].size();
    if (len % strands) throw std::invalid_argument("a unit's length is not a whole number of strands");
    const size_t each = len / strands;
    out.reserve(len * units.size());
    for (uint32_t s = 0; s < strands; ++s)
        for (const auto& u : units)
        {
            if (u.size() != len) throw std::invalid_argument("the units differ in length");
            out.insert(out.end(), u.begin() + std::ptrdiff_t(s * each), u.begin() + std::ptrdiff_t((s + 1) * each));
        }
    return out;
}

std::vector<std::vector<uint32_t>> split_units(std::span<const uint32_t> joined, uint32_t strands, uint32_t units)
{
    if (strands == 0 || units == 0 || joined.size() % (size_t(strands) * units))
        throw std::invalid_argument("the joined unit is not a whole number of units and strands");
    const size_t each = joined.size() / (size_t(strands) * units);
    std::vector<std::vector<uint32_t>> out(units);
    for (uint32_t s = 0; s < strands; ++s)
        for (uint32_t k = 0; k < units; ++k)
        {
            const auto from = joined.begin() + std::ptrdiff_t((size_t(s) * units + k) * each);
            out[k].insert(out[k].end(), from, from + std::ptrdiff_t(each));
        }
    return out;
}

// ---------------------------------------------------------------- the filters

CompositionSieve::CompositionSieve(const CompositionSpace& space, const FilterStack& cover, const FilterStack& title, const FilterStack& unit)
    : space_(&space), cover_(&cover), title_(&title), unit_(&unit)
{
    const Space& c = space.cover_space();
    const Space& u = space.unit_space();
    const std::optional<Space>& t = space.title_space();
    parts_[0] = {&cover, {}, c.base(), c.unit_length()};
    parts_[1] = {&title, {}, t ? t->base() : 1, t ? t->unit_length() : 0};
    parts_[2] = {&unit, {}, u.base(), u.unit_length()};
    const char* names[3] = {"cover", "title", "unit"};
    ranks_ = true;
    for (int i = 0; i < 3; ++i)
    {
        Part& part = parts_[i];
        if (i == 1 && !t) part.count = BigUint(1); // no title: one way to have none
        else if (part.stack->empty()) part.count = BigUint::pow(part.base, part.length);
        else if (part.stack->ranker()) part.count = part.stack->ranker()->count();
        else
        {
            ranks_ = false;
            if (blocker_.empty()) blocker_ = std::string(names[i]) + ": " + part.stack->compact_blocker();
        }
    }
    if (!ranks_) return;
    units_count_ = power(parts_[2].count, space.units());
    count_ = BigUint::mul(BigUint::mul(parts_[0].count, parts_[1].count), units_count_);
    if (!count_.is_zero())
    {
        hex_width_ = width_of(count_);
        shuffle_ = std::make_unique<Shuffle>(count_, u.key(), domain());
    }
}

std::string CompositionSieve::domain() const
{
    auto part = [](const FilterStack* st) { return st->empty() ? std::string("-") : st->id(); };
    const std::string title = space_->title_space() ? part(title_) : std::string("-");
    return std::string(kCompositionCompactVersion) + "/" + space_->id() + "/" + part(cover_) + "/" + title + "/" + part(unit_);
}

std::string CompositionSieve::first_failure(const CompositionSpace::Parts& p) const
{
    space_->check(p);
    if (int f = cover_->first_failure(p.cover); f >= 0) return "cover: " + cover_->filter_name(size_t(f));
    if (space_->title_space())
        if (int f = title_->first_failure(p.title); f >= 0) return "title: " + title_->filter_name(size_t(f));
    for (size_t k = 0; k < p.units.size(); ++k)
        if (int f = unit_->first_failure(p.units[k]); f >= 0) return "unit " + std::to_string(k + 1) + ": " + unit_->filter_name(size_t(f));
    return "";
}

BigUint CompositionSieve::Part::rank(std::span<const uint32_t> u) const
{
    if (stack->empty()) return BigUint::from_digits(u, base);
    return stack->ranker()->rank(u);
}

std::vector<uint32_t> CompositionSieve::Part::unrank(const BigUint& k) const
{
    if (stack->empty()) return k.to_digits(base, length);
    return stack->ranker()->unrank(k);
}

BigUint CompositionSieve::rank(const CompositionSpace::Parts& p) const
{
    if (!ranks_) throw std::logic_error("these filters cannot rank: " + blocker_);
    if (const std::string f = first_failure(p); !f.empty()) throw std::invalid_argument("it does not pass (" + f + ")");
    BigUint k = parts_[0].rank(p.cover);
    if (space_->title_space())
    {
        k = BigUint::mul(k, parts_[1].count);
        k += parts_[1].rank(p.title);
    }
    for (const auto& u : p.units)
    {
        k = BigUint::mul(k, parts_[2].count);
        k += parts_[2].rank(u);
    }
    return k;
}

CompositionSpace::Parts CompositionSieve::unrank(const BigUint& k) const
{
    if (!ranks_) throw std::logic_error("these filters cannot rank: " + blocker_);
    if (k >= count_) throw std::out_of_range("beyond the surviving compositions");
    CompositionSpace::Parts p;
    p.units.resize(space_->units());
    BigUint rest = k;
    for (size_t i = p.units.size(); i-- > 0;)
    {
        BigUint q, r;
        BigUint::divmod(rest, parts_[2].count, q, r);
        p.units[i] = parts_[2].unrank(r);
        rest = std::move(q);
    }
    if (space_->title_space())
    {
        BigUint kc, kt;
        BigUint::divmod(rest, parts_[1].count, kc, kt);
        p.title = parts_[1].unrank(kt);
        rest = std::move(kc);
    }
    p.cover = parts_[0].unrank(rest);
    return p;
}

BigUint CompositionSieve::index_of(const CompositionSpace::Parts& p, AddressMode m) const { return index_of_rank(rank(p), m); }

BigUint CompositionSieve::index_of_rank(const BigUint& k, AddressMode m) const
{
    if (!shuffle_) throw std::logic_error("no compact compositions here");
    if (k >= count_) throw std::out_of_range("beyond the surviving compositions");
    return m == AddressMode::Scrambled ? shuffle_->forward(k) : k;
}

CompositionSpace::Parts CompositionSieve::parts_at(const BigUint& index, AddressMode m) const
{
    if (!shuffle_) throw std::logic_error("no compact compositions here");
    if (index >= count_) throw std::out_of_range("beyond the surviving compositions");
    return unrank(m == AddressMode::Scrambled ? shuffle_->inverse(index) : index);
}

BigUint CompositionSieve::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= count_) throw std::out_of_range("beyond the surviving compositions");
    return v;
}

} // namespace sieve
