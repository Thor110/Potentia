// Sieve — the ticked filters of a line of a range of lengths (sieve/rangestack.hpp, list O3).

#include "sieve/rangestack.hpp"

#include "sieve/sha256.hpp"

#include <algorithm>
#include <climits>
#include <optional>
#include <stdexcept>
#include <utility>

namespace sieve {

BigUint ranker_below(const Ranker& r, std::span<const uint32_t> unit)
{
    const uint32_t L = r.length();
    if (unit.size() != L) throw std::invalid_argument("ranker_below: a unit of another length");
    BigUint total;
    Ranker::State s = r.start();
    for (uint32_t i = 0; i < L && s != Ranker::kDead; ++i)
    {
        for (uint32_t c = 0; c < unit[i] && c < r.base(); ++c)
            if (const Ranker::State t = r.next(s, c); t != Ranker::kDead) total += r.completions(t, L - i - 1);
        s = r.next(s, unit[i]);
    }
    return total;
}

RangeStack::RangeStack(const FilterLine& line, const std::vector<FilterStack::Entry>& entries, const FilterResources& resources)
    : min_(line.min_length), max_(line.length), base_(line.base)
{
    if (min_ == 0 || min_ >= max_) throw std::invalid_argument("a range stack is for a line of a range of lengths (min below max)");
    for (uint32_t l = min_; l <= max_; ++l) places_ += BigUint::pow(base_, l);
    std::vector<FilterStack::Entry> rest;
    std::vector<int> rest_index;
    std::string windows_prov;
    for (size_t i = 0; i < entries.size(); ++i)
    {
        names_.push_back(entries[i].spec->name());
        window_filter_.push_back(-1);
        if (const std::optional<PlaceWindow> w = place_window(*entries[i].spec, entries[i].values))
        {
            window_filter_.back() = int(windows_.size());
            windows_.push_back(*w);
            windows_prov += "; " + names_.back() + "{from=" + param_value(*entries[i].spec, entries[i].values, "from") + " width=" +
                            param_value(*entries[i].spec, entries[i].values, "width") + " decimals=" +
                            param_value(*entries[i].spec, entries[i].values, "decimals") + "}";
            continue;
        }
        rest.push_back(entries[i]);
        rest_index.push_back(int(i));
    }
    stacks_.reserve(max_ - min_ + 1);
    for (uint32_t l = min_; l <= max_; ++l)
    {
        FilterLine fl = line;
        fl.length = l;
        fl.min_length = 0;
        std::vector<FilterStack::Entry> here;
        std::vector<int> map;
        for (size_t j = 0; j < rest.size(); ++j)
            if (rest[j].spec->applies(fl))
            {
                here.push_back(rest[j]);
                map.push_back(rest_index[j]);
            }
        stacks_.emplace_back(fl, here, resources);
        index_.push_back(std::move(map));
    }
    // The provenance: the longest length's stack's, its length written as the range, and the windows.
    const std::string& top = stacks_.back().provenance();
    const size_t semi = top.find(';');
    provenance_ = line.kind + "/" + line.symbols_id + "/L" + std::to_string(min_) + "-" + std::to_string(max_) +
                  (semi == std::string::npos ? std::string() : top.substr(semi)) + windows_prov;
    id_ = Sha256::hex(Sha256::hash(provenance_));
    lazy_ = std::make_unique<std::once_flag>();
}

double RangeStack::table_bytes() const
{
    double most = 0;
    for (const FilterStack& s : stacks_) most = std::max(most, s.table_bytes());
    return most;
}

BigUint RangeStack::shorter_places(uint32_t length) const
{
    BigUint n;
    for (uint32_t l = min_; l < length; ++l) n += BigUint::pow(base_, l);
    return n;
}

int RangeStack::first_failure(std::span<const uint32_t> unit) const
{
    if (names_.empty()) return -1;
    const uint32_t L = uint32_t(unit.size());
    if (L < min_ || L > max_) throw std::invalid_argument("unit is not of a length in this stack's range");
    int first = INT_MAX;
    const size_t at = L - min_;
    if (const int f = stacks_[at].first_failure(unit); f >= 0) first = index_[at][size_t(f)];
    if (!windows_.empty())
    {
        BigUint place = BigUint::from_digits(unit, base_);
        place += shorter_places(L);
        for (size_t i = 0; i < names_.size() && int(i) < first; ++i)
            if (window_filter_[i] >= 0 && !windows_[size_t(window_filter_[i])].contains(place, places_)) first = int(i);
    }
    return first == INT_MAX ? -1 : first;
}

void RangeStack::settle() const
{
    std::call_once(*lazy_, [this] {
        if (names_.empty())
        {
            blocker_ = "no filters ticked";
            return;
        }
        // Every length's stack must rank (a length with no filter of its own: every unit of it).
        before_.assign(stacks_.size(), BigUint());
        all_ = BigUint();
        for (size_t i = 0; i < stacks_.size(); ++i)
        {
            BigUint n;
            if (stacks_[i].empty()) n = BigUint::pow(base_, min_ + uint32_t(i));
            else if (const Ranker* r = stacks_[i].ranker()) n = r->count();
            else
            {
                blocker_ = "at " + std::to_string(min_ + i) + " symbols: " + stacks_[i].compact_blocker();
                return;
            }
            before_[i] = all_;
            all_ += n;
        }
        count_ = all_;
        if (!windows_.empty())
        {
            std::optional<PlaceWindow::Places> keep;
            bool across = false;
            for (const PlaceWindow& w : windows_)
            {
                const PlaceWindow::Places p = w.places_kept(places_);
                across = across || p.wrap;
                if (!keep) keep = p;
                else if (!across)
                {
                    if (keep->lo < p.lo) keep->lo = p.lo;
                    if (p.hi < keep->hi) keep->hi = p.hi;
                    if (keep->hi < keep->lo) keep->hi = keep->lo;
                }
            }
            if (windows_.size() > 1 && across)
            {
                blocker_ = "two degree windows where one crosses 0 cannot be counted together: they judge only";
                return;
            }
            window_wrap_ = keep->wrap;
            below_lo_ = below_place(keep->lo);
            below_hi_ = below_place(keep->hi);
            count_ = below_hi_;
            if (window_wrap_) count_ += all_;
            count_ -= below_lo_;
        }
        can_rank_ = true;
    });
}

bool RangeStack::can_rank() const
{
    settle();
    return can_rank_;
}

std::string RangeStack::compact_blocker() const
{
    settle();
    return blocker_;
}

const BigUint& RangeStack::count() const
{
    settle();
    if (!can_rank_) throw std::logic_error("this range stack cannot count its survivors: " + blocker_);
    return count_;
}

BigUint RangeStack::below_place(const BigUint& place) const
{
    if (!(place < places_)) return all_;
    uint32_t L = min_;
    while (L < max_ && !(place < shorter_places(L + 1))) ++L;
    BigUint v = place;
    v -= shorter_places(L);
    const std::vector<uint32_t> unit = v.to_digits(base_, L);
    const size_t i = L - min_;
    BigUint r = before_[i];
    if (stacks_[i].empty()) r += v; // (every unit of the length: those below are its value)
    else r += ranker_below(*stacks_[i].ranker(), unit);
    return r;
}

BigUint RangeStack::rank_inner(std::span<const uint32_t> unit) const
{
    const size_t i = unit.size() - min_;
    BigUint r = before_[i];
    r += stacks_[i].empty() ? BigUint::from_digits(unit, base_) : stacks_[i].ranker()->rank(unit);
    return r;
}

std::vector<uint32_t> RangeStack::unrank_inner(const BigUint& k) const
{
    size_t i = 0;
    while (i + 1 < stacks_.size() && !(k < before_[i + 1])) ++i;
    BigUint at = k;
    at -= before_[i];
    const uint32_t L = min_ + uint32_t(i);
    if (stacks_[i].empty()) return at.to_digits(base_, L);
    return stacks_[i].ranker()->unrank(at);
}

BigUint RangeStack::rank(std::span<const uint32_t> unit) const
{
    if (!can_rank()) throw std::logic_error("this range stack cannot rank: " + blocker_);
    if (!passes(unit)) throw std::invalid_argument("the unit is not a survivor");
    BigUint r = rank_inner(unit);
    if (windows_.empty()) return r;
    if (window_wrap_ && r < below_hi_) return r; // (the loop's start, then its end)
    r -= below_lo_;
    if (window_wrap_) r += below_hi_;
    return r;
}

std::vector<uint32_t> RangeStack::unrank(const BigUint& k) const
{
    if (!can_rank()) throw std::logic_error("this range stack cannot rank: " + blocker_);
    if (!(k < count_)) throw std::out_of_range("survivor number beyond the survivors");
    if (windows_.empty()) return unrank_inner(k);
    if (window_wrap_ && k < below_hi_) return unrank_inner(k);
    BigUint at = k;
    if (window_wrap_) at -= below_hi_;
    at += below_lo_;
    return unrank_inner(at);
}

} // namespace sieve
