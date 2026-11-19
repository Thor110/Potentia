// Sieve — degree-window-v1: the units of one stretch of a line's loop, by bearing (sieve/window.hpp).
//
// The filter judges a unit by comparing it, as a number, with the window's two bounds. It counts
// with whatever else is ticked: FilterStack (filter.cpp) counts the other filters as it would
// without the window, and lays a WindowRanker over their ranker, which keeps the part of their
// survivors between the bounds. With nothing else ticked, the ranker underneath is every unit
// (FullRanker), and the window is plain arithmetic: its k-th survivor is the unit at lo + k.
//
// The WindowRanker's walk is the one underneath, with a note of whether the unit read so far is
// still equal to a bound (tight against it): a state is the state underneath, the place, and those
// two notes. While it is tight against the lower bound a symbol below the bound's is dead; against
// the upper, a symbol above it; once it differs from a bound it is free of it, and a state free of
// both is the one underneath, unchanged. Completions from a tight state are those underneath less
// the ones below the lower bound and the ones at or past the upper, each counted by walking the
// bound's remaining digits. A state is kept as a number standing for its three parts, given out as
// they are first met.

#include "sieve/window.hpp"

#include "sieve/biguint.hpp"

#include <algorithm>
#include <deque>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace sieve {

namespace {

bool less(std::span<const uint32_t> a, std::span<const uint32_t> b) { return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()); }

class DegreeWindowFilter : public Filter
{
public:
    DegreeWindowFilter(WindowBounds b, std::string prov) : b_(std::move(b)) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override { return b_.contains(unit); }
    const WindowBounds& bounds() const { return b_; }

private:
    WindowBounds b_;
};

// Every unit: one state, every symbol leading back to it.
class FullRanker : public Ranker
{
public:
    FullRanker(uint32_t base, uint32_t length) : base_(base), len_(length) { set_count(); }
    uint32_t length() const override { return len_; }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override { return s == kDead || symbol >= base_ ? kDead : 0; }
    BigUint completions(State s, uint32_t remaining) const override { return s == kDead ? BigUint() : BigUint::pow(base_, remaining); }
    bool alive(State s, uint32_t) const override { return s != kDead; }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (k >= count()) throw std::out_of_range("rank beyond the survivors");
        return k.to_digits(base_, len_);
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != len_) throw std::invalid_argument("unit has the wrong length");
        for (uint32_t c : unit)
            if (c >= base_) throw std::invalid_argument("unit is not a survivor");
        return BigUint::from_digits(unit, base_);
    }

private:
    uint32_t base_, len_;
};

class WindowRanker : public Ranker
{
public:
    WindowRanker(const Ranker& inner, WindowBounds b)
        : in_(inner), b_(std::move(b)), n_(inner.length()), full_(dynamic_cast<const FullRanker*>(&inner) != nullptr)
    {
        if (b_.lo.size() != n_ || (b_.has_hi && b_.hi.size() != n_)) throw std::logic_error("a window's bounds are not units of its line");
        has_lo_ = std::any_of(b_.lo.begin(), b_.lo.end(), [](uint32_t c) { return c != 0; });
        empty_ = b_.empty();
        if (!empty_ && has_lo_ && in_.start() != kDead) below_lo_ = below(in_.start(), b_.lo, 0);
        set_count();
    }
    uint32_t length() const override { return n_; }
    uint32_t base() const override { return in_.base(); }
    State start() const override
    {
        if (empty_) return kDead;
        return id_of(in_.start(), 0, uint8_t((has_lo_ ? 1 : 0) | (b_.has_hi ? 2 : 0)));
    }
    State next(State s, uint32_t c) const override
    {
        if (s == kDead) return kDead;
        const auto [si, pos, f] = parts(s);
        if (!f) return id_of(in_.next(si, c), 0, 0);
        if (pos >= n_) return kDead;
        if ((f & 1) && c < b_.lo[pos]) return kDead;
        if ((f & 2) && c > b_.hi[pos]) return kDead;
        const State t = in_.next(si, c);
        if (t == kDead) return kDead;
        uint8_t g = 0;
        if ((f & 1) && c == b_.lo[pos]) g |= 1;
        if ((f & 2) && c == b_.hi[pos]) g |= 2;
        if (g && pos + 1 == n_)
        {
            if (g & 2) return kDead; // the upper bound itself: the first unit past the window
            g = 0;                   // the lower bound itself: the first unit in it
        }
        return id_of(t, pos + 1, g);
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        const auto [si, pos, f] = parts(s);
        if (!f) return in_.completions(si, remaining);
        if (uint64_t(remaining) + pos != n_) return BigUint();
        {
            std::lock_guard<std::mutex> lock(mx_);
            const Rec& r = recs_[size_t(s)];
            if (r.known) return r.value;
        }
        const BigUint all = in_.completions(si, remaining);
        BigUint v = all;
        if (f & 1) v -= below(si, b_.lo, pos);
        if (f & 2)
        {
            BigUint past = all;
            past -= below(si, b_.hi, pos);
            v -= past;
        }
        std::lock_guard<std::mutex> lock(mx_);
        Rec& r = recs_[size_t(s)];
        r.value = v;
        r.known = true;
        return v;
    }
    bool alive(State s, uint32_t remaining) const override
    {
        if (s == kDead) return false;
        const auto [si, pos, f] = parts(s);
        if (!f) return in_.alive(si, remaining);
        return !completions(s, remaining).is_zero();
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (k >= count()) throw std::out_of_range("rank beyond the survivors");
        BigUint at = k;
        at += below_lo_;
        return in_.unrank(at);
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (!b_.contains(unit)) throw std::invalid_argument("unit is not a survivor");
        BigUint r = in_.rank(unit);
        r -= below_lo_;
        return r;
    }

private:
    struct Rec
    {
        State inner;
        uint32_t pos;
        uint8_t flags;
        bool known = false;
        BigUint value;
    };

    State id_of(State inner, uint32_t pos, uint8_t flags) const
    {
        if (inner == kDead) return kDead;
        if (!flags) pos = 0;
        std::lock_guard<std::mutex> lock(mx_);
        const auto key = std::make_tuple(inner, pos, flags);
        if (const auto it = ids_.find(key); it != ids_.end()) return it->second;
        const State id = State(recs_.size());
        recs_.push_back({inner, pos, flags, false, BigUint()});
        ids_.emplace(key, id);
        return id;
    }
    std::tuple<State, uint32_t, uint8_t> parts(State s) const
    {
        std::lock_guard<std::mutex> lock(mx_);
        const Rec& r = recs_.at(size_t(s));
        return {r.inner, r.pos, r.flags};
    }
    // The survivors underneath that go on from `s` at place `pos` with digits below x's from there on.
    BigUint below(State s, const std::vector<uint32_t>& x, uint32_t pos) const
    {
        if (full_) return BigUint::from_digits(std::span<const uint32_t>(x).subspan(pos), in_.base());
        BigUint k;
        std::vector<std::pair<State, uint64_t>> leads; // (state, how many smaller symbols lead to it)
        for (uint32_t j = pos; j < n_ && s != kDead; ++j)
        {
            leads.clear();
            for (uint32_t c = 0; c < x[j]; ++c)
            {
                const State t = in_.next(s, c);
                if (t == kDead) continue;
                auto it = std::find_if(leads.begin(), leads.end(), [&](const auto& l) { return l.first == t; });
                if (it == leads.end()) leads.emplace_back(t, 1);
                else ++it->second;
            }
            for (const auto& [t, m] : leads)
            {
                const BigUint c = in_.completions(t, n_ - j - 1);
                if (m == 1) k += c;
                else k += BigUint::mul(c, BigUint(m));
            }
            s = in_.next(s, x[j]);
        }
        return k;
    }

    const Ranker& in_;
    WindowBounds b_;
    uint32_t n_;
    bool full_, has_lo_ = false, empty_ = false;
    BigUint below_lo_;
    mutable std::mutex mx_;
    mutable std::deque<Rec> recs_;
    mutable std::map<std::tuple<State, uint32_t, uint8_t>, State> ids_;
};

} // namespace

bool WindowBounds::contains(std::span<const uint32_t> unit) const
{
    if (unit.size() != lo.size()) return false;
    if (less(unit, lo)) return false;
    return !has_hi || less(unit, hi);
}

bool WindowBounds::empty() const { return has_hi && !less(lo, hi); }

WindowBounds degree_bounds(uint32_t base, uint32_t length, uint64_t from, uint64_t to, uint32_t decimals)
{
    if (decimals > 9) throw std::invalid_argument("degree-window-v1: decimals must be 0..9");
    uint64_t whole = 360;
    for (uint32_t i = 0; i < decimals; ++i) whole *= 10;
    if (!(from < to && to <= whole))
        throw std::invalid_argument("degree-window-v1: from must be below to, and to at most " + std::to_string(whole) + " (360 degrees at " +
                                    std::to_string(decimals) + " decimals)");
    if (base < 2 || length == 0) throw std::invalid_argument("degree-window-v1: a line of at least two symbols and one place");
    const BigUint n = BigUint::pow(base, length), s = BigUint(whole);
    auto first_at = [&](uint64_t x) { // the first unit whose bearing is x or more: ceil(x * N / S)
        BigUint q, r;
        BigUint::divmod(BigUint::mul(n, BigUint(x)), s, q, r);
        if (!r.is_zero()) q.add_small(1);
        return q;
    };
    const BigUint lo = first_at(from), hi = first_at(to);
    WindowBounds b;
    if (lo >= n || (hi < n && !(lo < hi))) // nothing on the line has those bearings (a line of fewer units than the window's width)
    {
        b.lo.assign(length, 0);
        b.hi.assign(length, 0);
        b.has_hi = true;
        return b;
    }
    b.lo = lo.to_digits(base, length);
    if (hi < n)
    {
        b.hi = hi.to_digits(base, length);
        b.has_hi = true;
    }
    return b;
}

const WindowBounds* window_bounds(const Filter& f)
{
    const auto* w = dynamic_cast<const DegreeWindowFilter*>(&f);
    return w ? &w->bounds() : nullptr;
}

WindowBounds intersect_windows(const WindowBounds& a, const WindowBounds& b)
{
    WindowBounds out;
    out.lo = less(a.lo, b.lo) ? b.lo : a.lo;
    if (a.has_hi && b.has_hi) out.hi = less(a.hi, b.hi) ? a.hi : b.hi;
    else if (a.has_hi) out.hi = a.hi;
    else if (b.has_hi) out.hi = b.hi;
    out.has_hi = a.has_hi || b.has_hi;
    if (out.empty()) // (one canonical empty window)
    {
        out.lo.assign(out.lo.size(), 0);
        out.hi = out.lo;
        out.has_hi = true;
    }
    return out;
}

std::unique_ptr<Ranker> full_ranker(uint32_t base, uint32_t length) { return std::make_unique<FullRanker>(base, length); }

std::unique_ptr<Ranker> window_ranker(const Ranker& inner, WindowBounds bounds) { return std::make_unique<WindowRanker>(inner, std::move(bounds)); }

void add_window_filters(std::vector<FilterSpec>& out)
{
    FilterSpec w;
    w.id = "degree-window";
    w.title = "degree-window";
    w.description =
        "Only the units whose bearing on the line's loop lies from `from` up to `from` + `width` degrees (the bearing the hallway shows: "
        "floor(address x 360 / units), the address in positional order), in whole degrees or, with decimals, in hundredths, "
        "thousandths and finer. Everything else is set aside, the meaningful with the noise, so the chosen stretch fills the "
        "whole loop when compact. Counts with whatever else is ticked: it keeps the part of their survivors between its bounds. "
        "Choosing the window costs about log2(360 x 10^decimals) bits to name, which is what it saves on a line whose survivors "
        "are spread evenly; it gains only where they are not.";
    w.params = {{"from", "the first bearing kept, in units of 10^-decimals of a degree (round the loop: 360 degrees is 0 again)",
                 FilterParam::Kind::Integer, "0", 0, 359999999999, 1},
                {"width", "how wide the window is, in the same units (one degree at 0 decimals); a window that would run past 360 "
                          "degrees stops there", FilterParam::Kind::Integer, "1", 1, 360000000000, 1},
                {"decimals", "decimal places of a degree the bounds are written to: 0 whole degrees, 1 tenths, 2 hundredths ...",
                 FilterParam::Kind::Integer, "0", 0, 9, 1}};
    w.applies = [](const FilterLine& l) { return l.kind != "binary" && !l.up_to_length && l.base >= 2 && l.length >= 1; };
    w.counts_as = "window";
    w.picks_a_kind = true; // (ticking every filter at once leaves it alone: it keeps 1/360 of everything)
    w.whole_unit = true;   // (the bearing is the whole unit's, every voice at once)
    w.window = true;
    const std::string own = w.name();
    w.make = [own](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        const int64_t from = param_int(spec, v, "from"), width = param_int(spec, v, "width"), d = param_int(spec, v, "decimals");
        uint64_t whole = 360;
        for (int64_t i = 0; i < d; ++i) whole *= 10;
        const uint64_t at = uint64_t(from) % whole; // (round the loop: 360 degrees is 0 again)
        const uint64_t to = std::min<uint64_t>(at + uint64_t(width), whole); // (stops at 360)
        WindowBounds b = degree_bounds(l.base, l.length, at, to, uint32_t(d));
        return std::make_unique<DegreeWindowFilter>(std::move(b), "from=" + std::to_string(from) + " width=" + std::to_string(width) +
                                                                      " decimals=" + std::to_string(d));
    };
    out.push_back(w);
}

} // namespace sieve
