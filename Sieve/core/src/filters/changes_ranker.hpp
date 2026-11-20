// Sieve — two rankers shared by the filters of pictures and video (list N): History, the walk's states
// kept as they are first met, and ChangesRanker, which counts and ranks units made of parts of n
// places, each part either free or within `most` changes of the part before it.
//
// ChangesRanker serves video-smooth-v2 (the parts are frames, the first free, and something must
// change somewhere) and rows-alike-v1 on large palettes (the parts are rows, each frame's first row
// free). At each place of a part that follows another, one colour is the one a part before (no
// change) and the other P - 1 are changes, so the ways to finish a unit depend only on the place,
// the changes made so far in the part, and whether anything has changed yet, whatever the colours
// are: g[r][t], the ways to fill r places with at most t changes, times the parts still to come
// (P^n a free part, g[n][most] another), less, where a change is needed and none has been made, the
// one way with no change at all. The k-th survivor is found place by place from those counts, and a
// survivor's number summed from them: no walk through the palette, so 2^24 colours cost what two do.
#pragma once

#include "sieve/filter.hpp"

#include <cstdint>
#include <deque>
#include <map>
#include <mutex>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

namespace sieve {

// The walk's states: the place, a small note (a ranker's own), and the symbols just before the place
// (as many as the ranker looks back), each given a number as it is first met.
class History
{
public:
    explicit History(size_t lag) : lag_(lag) {}
    struct Rec
    {
        uint32_t pos;
        uint64_t aux;
        std::vector<uint32_t> tail; // the symbols before pos, the last of them pos - 1
    };
    Ranker::State id_of(uint32_t pos, uint64_t aux, std::vector<uint32_t> tail) const
    {
        if (tail.size() > lag_) tail.erase(tail.begin(), tail.end() - std::ptrdiff_t(lag_));
        // Each state's tail kept once, in recs_, and found again by a hash of the state (until review 4 the map
        // held a second copy of every tail as its key, and a guided walk on video meets millions of states).
        uint64_t h = 0x9e3779b97f4a7c15ull ^ (uint64_t(pos) * 0xff51afd7ed558ccdull) ^ (aux * 0xc4ceb9fe1a85ec53ull);
        for (const uint32_t c : tail) h = (h ^ c) * 0x100000001b3ull;
        std::lock_guard<std::mutex> lock(mx_);
        for (auto [it, end] = ids_.equal_range(h); it != end; ++it)
        {
            const Rec& r = recs_[size_t(it->second)];
            if (r.pos == pos && r.aux == aux && r.tail == tail) return it->second;
        }
        const Ranker::State id = recs_.size();
        recs_.push_back({pos, aux, std::move(tail)});
        ids_.emplace(h, id);
        return id;
    }
    Rec get(Ranker::State s) const
    {
        std::lock_guard<std::mutex> lock(mx_);
        return recs_.at(size_t(s));
    }

private:
    size_t lag_;
    mutable std::mutex mx_;
    mutable std::deque<Rec> recs_;
    mutable std::unordered_multimap<uint64_t, Ranker::State> ids_; // a state's hash -> its number
};

class ChangesRanker : public Ranker
{
public:
    // free_parts: per part, whether it is free (the first always is); most: the changes a part may make from
    // the one before; must_change: some part must differ from the one before it.
    ChangesRanker(uint32_t base, size_t n, std::vector<char> free_parts, size_t most, bool must_change)
        : P_(base), n_(n), free_(std::move(free_parts)), most_(most), must_(must_change), states_(n)
    {
        if (free_.empty() || n_ == 0) throw std::invalid_argument("a unit of no parts");
        free_[0] = 1;
        g_.assign(n_ + 1, std::vector<BigUint>(most_ + 1, BigUint(1)));
        for (size_t r = 1; r <= n_; ++r)
            for (size_t t = 0; t <= most_; ++t)
            {
                BigUint v = g_[r - 1][t];
                if (t > 0)
                {
                    BigUint ch = g_[r - 1][t - 1];
                    ch.mul_small(P_ - 1);
                    v += ch;
                }
                g_[r][t] = std::move(v);
            }
        // tail_[s]: the ways to fill parts s on; still_[s]: the ways with no change in any of them
        const BigUint whole = BigUint::pow(P_, n_);
        tail_.assign(free_.size() + 1, BigUint(1));
        still_.assign(free_.size() + 1, BigUint(1));
        for (size_t s = free_.size(); s-- > 0;)
        {
            tail_[s] = BigUint::mul(tail_[s + 1], free_[s] ? whole : g_[n_][most_]);
            still_[s] = free_[s] ? BigUint::mul(still_[s + 1], whole) : still_[s + 1];
        }
        set_count();
    }
    // The memory its table g takes (n + 1 rows of most + 1 numbers of up to n log2 P bits): a filter builds it only
    // where the filter memory holds it, and otherwise counts its survivors by formula and judges.
    static double table_bytes(uint32_t base, size_t n, size_t most)
    {
        double bits = 1;
        for (uint32_t b = base; b > 1; b >>= 1) bits += 1;
        return double(n + 1) * double(most + 1) * (double(n) * bits / 8 + 32);
    }
    uint32_t length() const override { return uint32_t(n_ * free_.size()); }
    uint32_t base() const override { return P_; }
    State start() const override { return states_.id_of(0, 0, {}); }
    State next(State s, uint32_t c) const override
    {
        if (s == kDead || c >= P_) return kDead;
        History::Rec r = states_.get(s);
        if (r.pos >= length()) return kDead;
        size_t ch = size_t(r.aux >> 1);
        bool moved = r.aux & 1;
        if (!free_[r.pos / n_] && c != r.tail.at(r.tail.size() - n_)) // (the same place a part before)
        {
            if (++ch > most_) return kDead;
            moved = true;
        }
        const uint32_t pos = r.pos + 1;
        if (pos % n_ == 0) ch = 0;
        r.tail.push_back(c);
        return states_.id_of(pos, (uint64_t(ch) << 1) | (moved ? 1 : 0), std::move(r.tail));
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        const History::Rec r = states_.get(s);
        if (uint64_t(r.pos) + remaining != length()) return BigUint();
        return ways(r.pos, size_t(r.aux >> 1), r.aux & 1);
    }
    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
        BigUint k = k0, q, rem;
        std::vector<uint32_t> u(length());
        size_t ch = 0;
        bool moved = false;
        auto small = [](const BigUint& x) { return x.is_zero() ? 0u : uint32_t(x.limbs()[0]); };
        for (size_t i = 0; i < u.size(); ++i)
        {
            const bool ends = (i + 1) % n_ == 0;
            if (free_[i / n_])
            {
                BigUint::divmod(k, ways(i + 1, 0, moved), q, rem);
                u[i] = small(q);
                k = std::move(rem);
                continue;
            }
            const uint32_t prev = u[i - n_];
            const BigUint same = ways(i + 1, ends ? 0 : ch, moved);
            const BigUint other = ch + 1 <= most_ ? ways(i + 1, ends ? 0 : ch + 1, true) : BigUint();
            const BigUint before = BigUint::mul(other, BigUint(prev));
            bool changed = true;
            if (k < before)
            {
                BigUint::divmod(k, other, q, rem);
                u[i] = small(q);
                k = std::move(rem);
            }
            else
            {
                k -= before;
                if (k < same)
                {
                    u[i] = prev;
                    changed = false;
                }
                else
                {
                    k -= same;
                    if (other.is_zero()) throw std::logic_error("the changes ranker's counts are inconsistent");
                    BigUint::divmod(k, other, q, rem);
                    u[i] = prev + 1 + small(q);
                    k = std::move(rem);
                }
            }
            if (changed)
            {
                ++ch;
                moved = true;
            }
            if (ends) ch = 0;
        }
        return u;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != length()) throw std::invalid_argument("unit has the wrong length");
        BigUint k;
        size_t ch = 0;
        bool moved = false;
        for (size_t i = 0; i < unit.size(); ++i)
        {
            const uint32_t c = unit[i];
            if (c >= P_) throw std::invalid_argument("unit is not a survivor");
            const bool ends = (i + 1) % n_ == 0;
            if (free_[i / n_])
            {
                if (c) k += BigUint::mul(ways(i + 1, 0, moved), BigUint(c));
                continue;
            }
            const uint32_t prev = unit[i - n_];
            const uint32_t same_below = prev < c ? 1 : 0, other_below = c - same_below;
            if (same_below) k += ways(i + 1, ends ? 0 : ch, moved);
            if (other_below && ch + 1 <= most_) k += BigUint::mul(ways(i + 1, ends ? 0 : ch + 1, true), BigUint(other_below));
            if (c != prev)
            {
                if (++ch > most_) throw std::invalid_argument("unit is not a survivor");
                moved = true;
            }
            if (ends) ch = 0;
        }
        if (must_ && !moved) throw std::invalid_argument("unit is not a survivor");
        return k;
    }

private:
    // The ways to finish from place pos, `ch` places of its part changed so far (0 at a part's start).
    BigUint ways(size_t pos, size_t ch, bool moved) const
    {
        if (pos >= length()) return BigUint(!must_ || moved ? 1 : 0);
        const size_t s = pos / n_, o = pos % n_;
        if (!free_[s] && ch > most_) return BigUint();
        BigUint v = BigUint::mul(free_[s] ? BigUint::pow(P_, n_ - o) : g_[n_ - o][most_ - ch], tail_[s + 1]);
        if (must_ && !moved) v -= BigUint::mul(free_[s] ? BigUint::pow(P_, n_ - o) : BigUint(1), still_[s + 1]); // (no change from here on)
        return v;
    }

    uint32_t P_;
    size_t n_;
    std::vector<char> free_;
    size_t most_;
    bool must_;
    History states_;
    std::vector<std::vector<BigUint>> g_;
    std::vector<BigUint> tail_, still_;
};

} // namespace sieve
