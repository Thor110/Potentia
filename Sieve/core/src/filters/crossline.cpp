// Sieve — not-a-file-v1: every line read as the binary line. A unit of any line is a number (its
// digits read positionally), and every number is a place on the binary line (binary-v1), holding a
// file. This filter fails a unit whose own number holds, on the binary line, a file whose first
// bytes carry a signature (file-kinds-v1): the same number, read as a file, says it is a PNG, a ZIP,
// a MIDI file. It applies to every line but binary (pages, image, audio, video, the books' parts,
// and the models line, whose number is its mixed-radix positional index: sieve/modelsieve.hpp),
// whatever its shape.
//
// Counting needs no automaton. The files of a kind sit in the binary line's order, so the units it
// fails below any number x are E(x) = the signed files at indexes below x (KindCounter::count_before,
// which works out only the file's head), and the survivors below x are S(x) = x - E(x). A unit's rank
// is S(its number); the k-th survivor is found by halving; and the walks the guided coder and the
// generic ranker take are prefixes of digits, whose completions are S(hi) - S(lo) over the range of
// numbers the prefix covers.

#include "sieve/filekind.hpp"
#include "sieve/filter.hpp"

#include <map>
#include <mutex>
#include <stdexcept>

namespace sieve {

namespace {

KindSet signed_set()
{
    KindSet s = kind_set_of("signed");
    return s;
}

class NumberFileRanker : public Ranker
{
public:
    NumberFileRanker(uint32_t base, uint32_t length) : base_(base), length_(length)
    {
        powers_.push_back(BigUint(1));
        for (uint32_t i = 0; i < length; ++i)
        {
            BigUint n = powers_.back();
            n.mul_small(base);
            powers_.push_back(n);
        }
        // The line's numbers run to base^length, whose file is the longest asked about.
        BigUint t = powers_.back();
        t.mul_small(255);
        t.add_small(1);
        signed_ = std::make_unique<KindCounter>(uint64_t((t.bit_length() - 1) / 8) + 1, signed_set());
        prefixes_.push_back({BigUint(), 0});
        set_count();
    }
    uint32_t length() const override { return length_; }
    uint32_t base() const override { return base_; }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override
    {
        if (s == kDead || symbol >= base_) return kDead;
        std::lock_guard<std::mutex> lock(mx_);
        const auto [prefix, depth] = prefixes_[size_t(s)];
        if (depth >= length_) return kDead;
        BigUint p = prefix;
        p.mul_small(base_);
        p.add_small(symbol);
        const std::string key = p.to_hex() + "/" + std::to_string(depth + 1);
        const auto [it, added] = ids_.emplace(key, State(prefixes_.size()));
        if (added) prefixes_.push_back({p, depth + 1});
        return it->second;
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return {};
        BigUint lo;
        {
            std::lock_guard<std::mutex> lock(mx_);
            lo = prefixes_[size_t(s)].first;
        }
        lo = BigUint::mul(lo, powers_[remaining]);
        BigUint hi = lo;
        hi += powers_[remaining];
        BigUint r = kept_below(hi);
        r -= kept_below(lo);
        return r;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        const BigUint v = BigUint::from_digits(unit, base_);
        if (is_signed_kind(file_kind_at(v))) throw std::invalid_argument("the unit is not a survivor");
        return kept_below(v);
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (!(k < count())) throw std::out_of_range("survivor number beyond the survivors");
        // The smallest v with more than k survivors in 0..v: between k and the line's last number.
        BigUint lo = k, hi = powers_.back();
        hi -= BigUint(1);
        while (lo < hi)
        {
            BigUint mid = lo;
            mid += hi;
            mid >>= 1;
            BigUint through = mid;
            through.add_small(1);
            if (kept_below(through) > k) hi = mid;
            else
            {
                lo = mid;
                lo.add_small(1);
            }
        }
        return lo.to_digits(base_, length_);
    }

private:
    BigUint kept_below(const BigUint& x) const // survivors among the numbers below x
    {
        BigUint r = x;
        r -= signed_->count_before(x);
        return r;
    }
    uint32_t base_, length_;
    std::vector<BigUint> powers_;
    std::unique_ptr<KindCounter> signed_;
    mutable std::mutex mx_;
    mutable std::vector<std::pair<BigUint, uint32_t>> prefixes_;
    mutable std::map<std::string, State> ids_;
};

class NotAFile : public Filter
{
public:
    NotAFile(uint32_t base, uint32_t length)
    {
        provenance_ = "not-a-file-v1 kinds=file-kinds-v1 line=binary-v1";
        ranker_ = std::make_unique<NumberFileRanker>(base, length);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        return !is_signed_kind(file_kind_at(BigUint::from_digits(unit, ranker_->base())));
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    std::unique_ptr<NumberFileRanker> ranker_;
};

} // namespace

void add_crossline_filters(std::vector<FilterSpec>& out)
{
    FilterSpec f;
    f.id = "not-a-file";
    f.title = "not-a-file";
    f.description = "Not a file by its number: every unit is a number, and every number is a place on the binary line. "
                    "Fails a unit whose own number holds, there, a file whose first bytes carry a signature (file-kinds-v1: "
                    "PNG, ZIP, MID, ...): the same number read as a file says it is one. Any line, any shape. Exact.";
    f.applies = [](const FilterLine& l) { return l.kind == "models" || (l.kind != "binary" && l.base >= 2 && l.length >= 1); };
    f.make = [](const FilterLine& l, const FilterValues&, const FilterResources&) -> std::unique_ptr<Filter> {
        if (l.kind == "models") throw std::invalid_argument("the models line is sieved by its own stack (sieve/modelsieve.hpp)");
        return std::make_unique<NotAFile>(l.base, l.length);
    };
    out.push_back(f);
}

} // namespace sieve
