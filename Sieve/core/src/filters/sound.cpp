// Sieve — filters for sound itself (the pcm set, sieve/sound.hpp; SPECIFICATIONS §3.3).
//
// A filter stack judges a sound's channels one at a time (FilterStack, line_voices), so each of
// these is a rule about one channel's samples, and its survivors are one channel's to the power of
// the channels. Samples are the digits' signed values (pcm_sample).
//
//   sound-peak-v1    no sample louder than a share of full scale: a sample s passes when
//                    -A <= s <= A, A = floor(2^(BITS-1) * percent / 100). Every sample on its own,
//                    so an automaton of one state, or (past the bound an automaton may take) its
//                    own ranker, a plain mixed-radix count: exact at every depth.
//   sound-step-v1    neighbouring samples differ by at most D = floor(2^BITS * percent / 100): noise
//                    jumps about the whole range, while sound that carries anything mostly moves a
//                    little from one sample to the next. An automaton whose state is the sample
//                    before, where states x symbols fit (to 11 bits); judged past that.
//   silence-run-v1   no run of more than N silent samples (digit 0): units that are mostly
//                    silence, which the start of the line in positional order is full of. An
//                    automaton whose state is the run so far, where it fits; judged past that.
//   sound-crossings-v1  at most a share of the neighbouring pairs cross zero (one sample above it,
//                    the next below, or the other way; IDEAS 19.232): noise crosses about every other
//                    sample, tones and voices far less. An automaton whose state is the last sample's
//                    sign (none, above, below; silence keeps the last) and the crossings so far, where
//                    it fits; judged past that.
//   sound-bend-v1    the second difference, s[i] - 2 s[i-1] + s[i-2], at most a share of the range
//                    (IDEAS 19.232): sound-step bounds the slope, this how sharply it bends, which
//                    real waves do gently. An automaton whose state is the two samples before, where
//                    it fits (to 7 bits a sample); judged past that.
//   sound-periodic-v1  the sound repeats itself a short way on (list Z9; IDEAS 19.232's "periodicity, the
//                    mark of a pitch"): for some period from the rate / 1,000 to the rate / 50 samples (a
//                    pitch of 50 Hz to 1 kHz), most samples are within a share of the range of the one a
//                    period before. A voice, an instrument, a hum pass; noise does not. Judged.

#include "sieve/biguint.hpp"
#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp" // make_dfa_filter
#include "sieve/sound.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <functional>
#include <string>
#include <mutex>
#include <optional>
#include <span>
#include <vector>
#include <stdexcept>

namespace sieve {

namespace {

// An automaton is built where its states times its symbols stay under this (as the picture
// filters and max-run-v1 bound theirs).
constexpr uint64_t kMaxDfaCells = uint64_t(1) << 24;
bool cells_fit(uint64_t states, uint64_t base) { return base > 0 && states <= kMaxDfaCells / base; }

bool sound_line(const FilterLine& l) { return l.kind == "audio" && is_pcm_symbols(l.symbols_id); }

// ---------------------------------------------------------------- sound-peak

// The largest magnitude allowed.
int64_t peak_limit(uint32_t bits, int64_t percent) { return int64_t((uint64_t(1) << (bits - 1)) * uint64_t(percent) / 100); }

// The digits that pass, in digit order: 0..hi (silence and the positive samples), then the
// negative samples from -lo up (digits base-lo .. base-1).
struct PeakSet
{
    uint64_t hi = 0, lo = 0, base = 0;
    uint64_t size() const { return hi + 1 + lo; }
    bool has(uint32_t d) const { return d <= hi || d >= base - lo; }
    uint64_t index(uint32_t d) const { return d <= hi ? d : hi + 1 + (d - (base - lo)); }
    uint32_t digit(uint64_t i) const { return uint32_t(i <= hi ? i : base - lo + (i - hi - 1)); }
};

PeakSet peak_set(uint32_t bits, int64_t percent)
{
    const int64_t a = peak_limit(bits, percent), half = int64_t(1) << (bits - 1);
    return PeakSet{uint64_t(std::min(a, half - 1)), uint64_t(std::min(a, half)), uint64_t(1) << bits};
}

// Every sample on its own: a survivor is a number in base |set|, read digit by digit.
class PeakRanker : public Ranker
{
public:
    PeakRanker(const PeakSet& s, uint32_t length) : s_(s), l_(length), k_(BigUint(s.size())) { set_count(); }
    uint32_t length() const override { return l_; }
    uint32_t base() const override { return uint32_t(s_.base); }
    State start() const override { return 0; }
    State next(State s, uint32_t symbol) const override { return s == kDead || symbol >= s_.base || !s_.has(symbol) ? kDead : 0; }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        return BigUint::pow(k_, remaining);
    }
    bool alive(State s, uint32_t) const override { return s != kDead; }
    // A survivor's rank is its samples' places in the set read as one number in base |set| (at
    // most 2^31), so both ways are BigUint's own digit conversions, which divide and conquer:
    // nothing grows a sample at a time.
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (k >= count()) throw std::out_of_range("rank beyond the survivors");
        if (s_.size() == 1) return std::vector<uint32_t>(l_, s_.digit(0)); // one survivor: silence
        std::vector<uint32_t> out = k.to_digits(uint32_t(s_.size()), l_);
        for (uint32_t& d : out) d = s_.digit(d);
        return out;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != l_) throw std::invalid_argument("unit has the wrong length");
        std::vector<uint32_t> places(unit.size());
        for (size_t i = 0; i < unit.size(); ++i)
        {
            if (!s_.has(unit[i])) throw std::invalid_argument("not a survivor");
            places[i] = uint32_t(s_.index(unit[i]));
        }
        return s_.size() == 1 ? BigUint() : BigUint::from_digits(places, uint32_t(s_.size()));
    }

private:
    PeakSet s_;
    uint32_t l_;
    BigUint k_;
};

class PeakFilter : public Filter
{
public:
    PeakFilter(const PeakSet& s, uint32_t length, std::string prov) : s_(s), l_(length) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        return std::all_of(unit.begin(), unit.end(), [&](uint32_t d) { return d < s_.base && s_.has(d); });
    }
    const Ranker* ranker() const override
    {
        std::call_once(once_, [&] { ranker_ = std::make_unique<PeakRanker>(s_, l_); });
        return ranker_.get();
    }
    bool can_rank() const override { return true; }

private:
    PeakSet s_;
    uint32_t l_;
    mutable std::once_flag once_;
    mutable std::unique_ptr<PeakRanker> ranker_;
};

Dfa peak_dfa(const PeakSet& s)
{
    Dfa d;
    d.base = uint32_t(s.base);
    d.start = 0;
    d.accept = {1};
    d.next.assign(size_t(s.base), Dfa::kDead);
    for (uint64_t c = 0; c < s.base; ++c)
        if (s.has(uint32_t(c))) d.next[size_t(c)] = 0;
    return d;
}

// ---------------------------------------------------------------- sound-step

int64_t step_limit(uint32_t bits, int64_t percent) { return int64_t((uint64_t(1) << bits) * uint64_t(percent) / 100); }

std::optional<Dfa> step_dfa(const PcmFormat& f, int64_t limit)
{
    const uint64_t B = f.base();
    if (!cells_fit(B + 1, B)) return std::nullopt;
    // State 0: no sample yet; state 1 + d: the last sample was digit d.
    Dfa d;
    d.base = uint32_t(B);
    d.start = 0;
    d.accept.assign(size_t(B + 1), 1);
    d.next.assign(size_t((B + 1) * B), Dfa::kDead);
    for (uint64_t c = 0; c < B; ++c) d.next[size_t(c)] = int32_t(1 + c);
    for (uint64_t last = 0; last < B; ++last)
    {
        const int64_t a = pcm_sample(f, uint32_t(last));
        for (uint64_t c = 0; c < B; ++c)
            if (std::abs(pcm_sample(f, uint32_t(c)) - a) <= limit) d.next[size_t((1 + last) * B + c)] = int32_t(1 + c);
    }
    return minimise(d);
}

class StepFilter : public Filter
{
public:
    StepFilter(const PcmFormat& f, int64_t limit, std::string prov) : f_(f), limit_(limit) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t i = 1; i < unit.size(); ++i)
            if (std::abs(int64_t(pcm_sample(f_, unit[i])) - int64_t(pcm_sample(f_, unit[i - 1]))) > limit_) return false;
        return true;
    }

private:
    PcmFormat f_;
    int64_t limit_;
};

// ---------------------------------------------------------------- silence-run

std::optional<Dfa> silence_dfa(uint32_t base, uint32_t length, uint64_t most)
{
    const uint64_t n = std::min<uint64_t>(most, length); // a run cannot be longer than the unit
    if (!cells_fit(n + 1, base)) return std::nullopt;
    // State r: the unit ends in r silent samples.
    Dfa d;
    d.base = base;
    d.start = 0;
    d.accept.assign(size_t(n + 1), 1);
    d.next.assign(size_t((n + 1) * base), Dfa::kDead);
    for (uint64_t r = 0; r <= n; ++r)
    {
        if (r + 1 <= n) d.next[size_t(r * base)] = int32_t(r + 1);
        for (uint64_t c = 1; c < base; ++c) d.next[size_t(r * base + c)] = 0;
    }
    return minimise(d);
}

class SilenceFilter : public Filter
{
public:
    SilenceFilter(uint64_t most, std::string prov) : most_(most) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        uint64_t run = 0;
        for (uint32_t d : unit)
        {
            run = d == 0 ? run + 1 : 0;
            if (run > most_) return false;
        }
        return true;
    }

private:
    uint64_t most_;
};

// ---------------------------------------------------------------- sound-crossings

// A sample's sign: 0 silent, 1 above, 2 below.
uint32_t sign_of(const PcmFormat& f, uint32_t digit)
{
    const int32_t v = pcm_sample(f, digit);
    return v > 0 ? 1 : v < 0 ? 2 : 0;
}

uint64_t crossings_allowed(uint32_t length, int64_t permille) { return length < 2 ? 0 : uint64_t(length - 1) * uint64_t(permille) / 1000; }

std::optional<Dfa> crossings_dfa(const PcmFormat& f, uint64_t most)
{
    const uint64_t B = f.base();
    // State 3 k + s: k crossings so far, the last sign that was not silence s (0: none yet).
    const uint64_t states = 3 * (most + 1);
    if (!cells_fit(states, B)) return std::nullopt;
    Dfa d;
    d.base = uint32_t(B);
    d.start = 0;
    d.accept.assign(size_t(states), 1);
    d.next.assign(size_t(states * B), Dfa::kDead);
    std::vector<uint32_t> sign(static_cast<size_t>(B));
    for (uint64_t c = 0; c < B; ++c) sign[size_t(c)] = sign_of(f, uint32_t(c));
    for (uint64_t k = 0; k <= most; ++k)
        for (uint64_t last = 0; last < 3; ++last)
            for (uint64_t c = 0; c < B; ++c)
            {
                const uint32_t now = sign[size_t(c)];
                if (now == 0) d.next[size_t((3 * k + last) * B + c)] = int32_t(3 * k + last); // (silence keeps the last sign)
                else if (last == 0 || now == last) d.next[size_t((3 * k + last) * B + c)] = int32_t(3 * k + now);
                else if (k + 1 <= most) d.next[size_t((3 * k + last) * B + c)] = int32_t(3 * (k + 1) + now);
            }
    return minimise(d);
}

class CrossingsFilter : public Filter
{
public:
    CrossingsFilter(const PcmFormat& f, uint64_t most, std::string prov) : f_(f), most_(most) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        uint32_t last = 0;
        uint64_t k = 0;
        for (const uint32_t d : unit)
        {
            const uint32_t now = sign_of(f_, d);
            if (now == 0) continue;
            if (last != 0 && now != last && ++k > most_) return false;
            last = now;
        }
        return true;
    }

private:
    PcmFormat f_;
    uint64_t most_;
};

// ---------------------------------------------------------------- loudness in blocks (sound-envelope, sound-syllables)

// A sample's loudness in four steps: 0 silent, then 1 to 4 for up to a quarter, a half, three quarters and the whole
// of full scale (its magnitude against 2^(BITS-1)). A block's loudness is its loudest sample's.
uint32_t loudness_of(const PcmFormat& f, uint32_t digit)
{
    const int64_t v = pcm_sample(f, digit), m = v < 0 ? -v : v;
    if (m == 0) return 0;
    const int64_t half = int64_t(1) << (f.bits - 1);
    return uint32_t(std::min<int64_t>(4, (4 * m + half - 1) / half));
}

// sound-envelope: block loudness rises (or holds), then falls (or holds), never rising again once it has fallen,
// the loudest at least 2 and the last block at least 2 below it. The blocks' story, outside any one block: 0
// nothing yet; 1 + L rising, the last block L; 6 + k falling, (the last L, the loudest P) the k-th pair L < P.
constexpr uint32_t kEnvStory = 16;
uint32_t env_falling(uint32_t last, uint32_t peak) { return 6 + (peak * (peak - 1)) / 2 + last; } // (P = 1..4, L < P)
// The story after a block of loudness l; kDead where it rises again after falling.
int64_t env_close(uint32_t g, uint32_t l)
{
    if (g == 0) return 1 + l;
    if (g <= 5)
    {
        const uint32_t last = g - 1;
        return l >= last ? int64_t(1 + l) : int64_t(env_falling(l, last));
    }
    for (uint32_t p = 1; p <= 4; ++p)
        for (uint32_t q = 0; q < p; ++q)
            if (env_falling(q, p) == g) return l <= q ? int64_t(env_falling(l, p)) : -1;
    return -1;
}
bool env_accepts(uint32_t g)
{
    for (uint32_t p = 2; p <= 4; ++p)
        for (uint32_t q = 0; q + 2 <= p; ++q)
            if (env_falling(q, p) == g) return true;
    return false;
}

std::optional<Dfa> envelope_dfa(const PcmFormat& f, uint32_t block)
{
    const uint64_t B = f.base(), states = uint64_t(kEnvStory) * 5 * block;
    if (!cells_fit(states, B)) return std::nullopt;
    // State ((g * 5) + cur) * block + pos: the story g, the loudest so far in this block cur, pos samples into it.
    auto id = [block](uint32_t g, uint32_t cur, uint32_t pos) { return int32_t((uint64_t(g) * 5 + cur) * block + pos); };
    Dfa d;
    d.base = uint32_t(B);
    d.start = 0;
    d.accept.assign(size_t(states), 0);
    d.next.assign(size_t(states * B), Dfa::kDead);
    std::vector<uint32_t> lev(static_cast<size_t>(B));
    for (uint64_t c = 0; c < B; ++c) lev[size_t(c)] = loudness_of(f, uint32_t(c));
    for (uint32_t g = 0; g < kEnvStory; ++g)
        for (uint32_t cur = 0; cur < 5; ++cur)
            for (uint32_t pos = 0; pos < block; ++pos)
            {
                const size_t at = size_t(id(g, cur, pos));
                // the unit ending here: a block begun is the last block
                const int64_t end = pos == 0 ? int64_t(g) : env_close(g, cur);
                d.accept[at] = end >= 0 && env_accepts(uint32_t(end)) ? 1 : 0;
                for (uint64_t c = 0; c < B; ++c)
                {
                    const uint32_t now = std::max(cur, lev[size_t(c)]);
                    if (pos + 1 < block) d.next[at * B + c] = id(g, now, pos + 1);
                    else if (const int64_t g2 = env_close(g, now); g2 >= 0) d.next[at * B + c] = id(uint32_t(g2), 0, 0);
                }
            }
    return minimise(d);
}

// How many samples are of each loudness, and so how many blocks of k samples have their loudest at each level:
// those of at most l, less those of at most l - 1.
std::array<uint64_t, 5> loudness_counts(const PcmFormat& f)
{
    std::array<uint64_t, 5> n{};
    for (uint64_t c = 0; c < f.base(); ++c) ++n[loudness_of(f, uint32_t(c))];
    return n;
}
std::array<BigUint, 5> blocks_at_level(const std::array<uint64_t, 5>& n, uint32_t k)
{
    std::array<BigUint, 5> out;
    uint64_t at_most = 0;
    BigUint below(0);
    for (uint32_t l = 0; l < 5; ++l)
    {
        at_most += n[l];
        BigUint upto = BigUint::pow(uint32_t(at_most), k);
        out[l] = upto;
        out[l] -= below;
        below = std::move(upto);
    }
    return out;
}

// A block story ranked sample by sample (list N5-10): sound-envelope and sound-syllables at any length. Each sample has
// a class (its loudness, or loud or not), classes nested so that "at most class l" is a set of N_l samples; a block's
// class is its loudest sample's; the blocks' story goes on from block to block (close) and ends accepted or not. The
// ways to finish from a place depend only on the block, the loudest class in it so far and the story: the rest of
// this block reaching class m exactly (N_m^r less N_(m-1)^r, or N_cur^r staying at cur) times the blocks after it from
// the story that m leaves (a table, block by block). So it counts, ranks and unranks with the samples taken a run of
// one class at a time (the digits of one class lie in a few runs), never the whole palette.
struct BlockStory
{
    uint32_t states = 0;
    std::function<int64_t(uint32_t, uint32_t)> close; // the story after a block of class m (-1: none)
    std::function<bool(uint32_t)> accept;
};
struct ClassRun
{
    uint32_t start, len, cls;
};

class BlockStoryRanker : public Ranker
{
public:
    // The memory its tables take: N_l^r for every class and every length to a block, and the story's table.
    static double table_bytes(uint32_t bits, uint32_t classes, uint32_t block, uint32_t length, uint32_t states)
    {
        const double powers = double(classes) * double(block + 1) * (double(block) * double(bits) / 8 + 32);
        const double story = double(length / block + 2) * double(states) * (double(length) * double(bits) / 8 + 32);
        return powers + story;
    }
    BlockStoryRanker(uint32_t base, std::vector<ClassRun> runs, uint32_t classes, uint32_t block, uint32_t length, BlockStory st)
        : base_(base), runs_(std::move(runs)), classes_(classes), block_(block), len_(length), st_(std::move(st))
    {
        std::vector<uint64_t> n(classes_, 0);
        for (const ClassRun& r : runs_) n[r.cls] += r.len;
        std::vector<uint64_t> le(classes_, 0);
        for (uint32_t l = 0; l < classes_; ++l) le[l] = n[l] + (l ? le[l - 1] : 0);
        pw_.assign(classes_, std::vector<BigUint>(block_ + 1));
        for (uint32_t l = 0; l < classes_; ++l)
        {
            pw_[l][0] = BigUint(1);
            for (uint32_t r = 1; r <= block_; ++r)
            {
                pw_[l][r] = pw_[l][r - 1];
                if (le[l] <= 0xffffffffu) pw_[l][r].mul_small(uint32_t(le[l]));
                else pw_[l][r] = BigUint::mul(pw_[l][r], BigUint(le[l]));
            }
        }
        const uint32_t blocks = (len_ + block_ - 1) / block_;
        tail_.assign(size_t(blocks) + 1, std::vector<BigUint>(st_.states));
        for (uint32_t g = 0; g < st_.states; ++g) tail_[blocks][g] = BigUint(st_.accept(g) ? 1 : 0);
        for (uint32_t b = blocks; b-- > 0;)
        {
            const uint32_t k = std::min(block_, len_ - b * block_);
            for (uint32_t g = 0; g < st_.states; ++g)
            {
                BigUint v;
                for (uint32_t m = 0; m < classes_; ++m)
                    if (const int64_t g2 = st_.close(g, m); g2 >= 0 && !tail_[b + 1][size_t(g2)].is_zero())
                        v += BigUint::mul(exact(k, 0, m), tail_[b + 1][size_t(g2)]);
                tail_[b][g] = std::move(v);
            }
        }
        set_count();
    }
    uint32_t length() const override { return len_; }
    uint32_t base() const override { return base_; }
    State start() const override { return pack(0, 0, 0); }
    State next(State s, uint32_t c) const override
    {
        if (s == kDead || c >= base_) return kDead;
        return step(s, class_of(c));
    }
    BigUint completions(State s, uint32_t remaining) const override
    {
        if (s == kDead) return BigUint();
        const auto [pos, g, cur] = unpack(s);
        if (uint64_t(pos) + remaining != len_) return BigUint();
        return ways(pos, g, cur);
    }
    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
        BigUint k = k0, q, r;
        std::vector<uint32_t> u(len_);
        State s = start();
        for (uint32_t i = 0; i < len_; ++i)
        {
            bool placed = false;
            for (const ClassRun& run : runs_)
            {
                const State t = step(s, run.cls);
                if (t == kDead) continue;
                const BigUint w = completions(t, len_ - i - 1);
                if (w.is_zero()) continue;
                const BigUint all = BigUint::mul(w, BigUint(run.len));
                if (k < all)
                {
                    BigUint::divmod(k, w, q, r);
                    u[i] = run.start + (q.is_zero() ? 0 : uint32_t(q.limbs()[0]));
                    k = std::move(r);
                    s = t;
                    placed = true;
                    break;
                }
                k -= all;
            }
            if (!placed) throw std::logic_error("the block ranker's counts are inconsistent");
        }
        return u;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != len_) throw std::invalid_argument("unit has the wrong length");
        BigUint k;
        State s = start();
        for (uint32_t i = 0; i < len_; ++i)
        {
            if (unit[i] >= base_) throw std::invalid_argument("unit is not a survivor");
            for (const ClassRun& run : runs_)
            {
                if (run.start >= unit[i]) continue;
                const uint32_t below = std::min(run.len, unit[i] - run.start);
                const State t = step(s, run.cls);
                if (t == kDead) continue;
                k += BigUint::mul(completions(t, len_ - i - 1), BigUint(below));
            }
            s = step(s, class_of(unit[i]));
            if (s == kDead) throw std::invalid_argument("unit is not a survivor");
        }
        if (completions(s, 0).is_zero()) throw std::invalid_argument("unit is not a survivor");
        return k;
    }

private:
    static State pack(uint32_t pos, uint32_t g, uint32_t cur) { return (State(pos) << 32) | (State(g) << 8) | cur; }
    static std::tuple<uint32_t, uint32_t, uint32_t> unpack(State s) { return {uint32_t(s >> 32), uint32_t((s >> 8) & 0xffffff), uint32_t(s & 0xff)}; }
    uint32_t class_of(uint32_t c) const
    {
        for (const ClassRun& r : runs_)
            if (c >= r.start && c - r.start < r.len) return r.cls;
        throw std::logic_error("a symbol in no class");
    }
    State step(State s, uint32_t cls) const
    {
        auto [pos, g, cur] = unpack(s);
        if (pos >= len_) return kDead;
        cur = std::max(cur, cls);
        ++pos;
        if (pos % block_ == 0 || pos == len_)
        {
            const int64_t g2 = st_.close(g, cur);
            if (g2 < 0) return kDead;
            g = uint32_t(g2), cur = 0;
        }
        return pack(pos, g, cur);
    }
    // r more samples taking a block whose loudest so far is cur to exactly m
    BigUint exact(uint32_t r, uint32_t cur, uint32_t m) const
    {
        if (m < cur) return BigUint();
        if (m == cur) return pw_[m][r];
        BigUint v = pw_[m][r];
        v -= pw_[m - 1][r];
        return v;
    }
    BigUint ways(uint32_t pos, uint32_t g, uint32_t cur) const
    {
        if (pos >= len_) return BigUint(st_.accept(g) ? 1 : 0);
        const uint32_t b = pos / block_, o = pos % block_, k = std::min(block_, len_ - b * block_);
        BigUint v;
        for (uint32_t m = cur; m < classes_; ++m)
            if (const int64_t g2 = st_.close(g, m); g2 >= 0 && !tail_[b + 1][size_t(g2)].is_zero())
                v += BigUint::mul(exact(k - o, cur, m), tail_[b + 1][size_t(g2)]);
        return v;
    }

    uint32_t base_;
    std::vector<ClassRun> runs_;
    uint32_t classes_, block_, len_;
    BlockStory st_;
    std::vector<std::vector<BigUint>> pw_, tail_;
};

// The runs of digits of one class, in digit order (a few: classes go by magnitude).
template <class Of>
std::vector<ClassRun> class_runs(uint64_t base, Of of)
{
    std::vector<ClassRun> out;
    for (uint64_t c = 0; c < base; ++c)
    {
        const uint32_t k = of(uint32_t(c));
        if (!out.empty() && out.back().cls == k && out.back().start + out.back().len == c) ++out.back().len;
        else out.push_back({uint32_t(c), 1, k});
    }
    return out;
}

// sound-envelope counted block by block: the units of `length` samples whose blocks tell an envelope. Exact, at
// any length (an automaton's table grows with the length; this, with the blocks only).
BigUint envelope_count(const PcmFormat& f, uint32_t block, uint32_t length)
{
    const auto n = loudness_counts(f);
    std::vector<BigUint> ways(kEnvStory, BigUint(0));
    ways[0] = BigUint(1);
    auto step = [&](uint32_t k) {
        const auto at = blocks_at_level(n, k);
        std::vector<BigUint> next(kEnvStory, BigUint(0));
        for (uint32_t g = 0; g < kEnvStory; ++g)
        {
            if (ways[g].is_zero()) continue;
            for (uint32_t l = 0; l < 5; ++l)
                if (const int64_t g2 = env_close(g, l); g2 >= 0 && !at[l].is_zero()) next[size_t(g2)] += BigUint::mul(ways[g], at[l]);
        }
        ways = std::move(next);
    };
    for (uint32_t b = 0; b < length / block; ++b) step(block);
    if (length % block) step(length % block);
    BigUint total(0);
    for (uint32_t g = 0; g < kEnvStory; ++g)
        if (env_accepts(g)) total += ways[g];
    return total;
}

class EnvelopeFilter : public Filter
{
public:
    EnvelopeFilter(const PcmFormat& f, uint32_t block, uint32_t length, std::string prov)
        : f_(f), block_(block), count_(envelope_count(f, block, length))
    {
        provenance_ = std::move(prov);
        // (ranked where its tables fit the filter memory: list N5-10)
        if (BlockStoryRanker::table_bytes(f.bits, 5, block, length, kEnvStory) <= filter_memory())
        {
            BlockStory st{kEnvStory, [](uint32_t g, uint32_t m) { return env_close(g, m); }, [](uint32_t g) { return env_accepts(g); }};
            ranker_ = std::make_unique<BlockStoryRanker>(f.base(), class_runs(f.base(), [&](uint32_t c) { return loudness_of(f, c); }), 5, block,
                                                         length, std::move(st));
        }
    }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &count_; }
    const Ranker* ranker() const override { return ranker_.get(); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        int64_t g = 0;
        uint32_t cur = 0, pos = 0;
        for (const uint32_t c : unit)
        {
            cur = std::max(cur, loudness_of(f_, c));
            if (++pos == block_)
            {
                if ((g = env_close(uint32_t(g), cur)) < 0) return false;
                cur = pos = 0;
            }
        }
        if (pos > 0 && (g = env_close(uint32_t(g), cur)) < 0) return false;
        return env_accepts(uint32_t(g));
    }

private:
    PcmFormat f_;
    uint32_t block_;
    BigUint count_;
    std::unique_ptr<Ranker> ranker_;
};

// sound-syllables: a block is loud where its loudest sample reaches the threshold; each loud block after a quiet
// one (or first) begins a syllable; between `fewest` and `most` of them in the unit.
struct SyllableRule
{
    uint32_t block = 256;
    int64_t threshold = 1; // (the smallest magnitude that is loud)
    uint64_t fewest = 0, most = 0;
};
bool loud_sample(const PcmFormat& f, uint32_t digit, int64_t threshold)
{
    const int64_t v = pcm_sample(f, digit);
    return (v < 0 ? -v : v) >= threshold;
}
SyllableRule syllable_rule(const PcmFormat& f, uint32_t length, uint32_t block, int64_t percent, int64_t per_second_low, int64_t per_second_high)
{
    SyllableRule r;
    r.block = block;
    // loud from `percent` of full scale (2^(BITS-1)), rounded up, and never silence itself
    r.threshold = std::max<int64_t>(1, ((int64_t(1) << (f.bits - 1)) * percent + 99) / 100);
    // per second, over the unit's length in seconds: at least the floor of the low rate's, at most the ceiling of the high's
    const uint64_t num = uint64_t(length), den = f.rate;
    r.fewest = uint64_t(per_second_low) * num / den;
    r.most = (uint64_t(per_second_high) * num + den - 1) / den;
    return r;
}

std::optional<Dfa> syllables_dfa(const PcmFormat& f, const SyllableRule& r)
{
    const uint64_t B = f.base(), states = (r.most + 1) * 4 * uint64_t(r.block);
    if (!cells_fit(states, B)) return std::nullopt;
    // State ((n * 2 + was) * 2 + now) * block + pos: n syllables so far, the last whole block loud (was), this
    // block loud so far (now), pos samples into it.
    auto id = [&](uint64_t n, uint32_t was, uint32_t now, uint32_t pos) { return int32_t(((n * 2 + was) * 2 + now) * r.block + pos); };
    Dfa d;
    d.base = uint32_t(B);
    d.start = 0;
    d.accept.assign(size_t(states), 0);
    d.next.assign(size_t(states * B), Dfa::kDead);
    std::vector<uint32_t> loud(static_cast<size_t>(B));
    for (uint64_t c = 0; c < B; ++c) loud[size_t(c)] = loud_sample(f, uint32_t(c), r.threshold) ? 1 : 0;
    for (uint64_t n = 0; n <= r.most; ++n)
        for (uint32_t was = 0; was < 2; ++was)
            for (uint32_t now = 0; now < 2; ++now)
                for (uint32_t pos = 0; pos < r.block; ++pos)
                {
                    const size_t at = size_t(id(n, was, now, pos));
                    const uint64_t end = n + (pos > 0 && now && !was ? 1 : 0); // (a block begun counts as the last)
                    d.accept[at] = end >= r.fewest && end <= r.most ? 1 : 0;
                    for (uint64_t c = 0; c < B; ++c)
                    {
                        const uint32_t now2 = now | loud[size_t(c)];
                        if (pos + 1 < r.block) d.next[at * B + c] = id(n, was, now2, pos + 1);
                        else
                        {
                            const uint64_t n2 = n + (now2 && !was ? 1 : 0);
                            if (n2 <= r.most) d.next[at * B + c] = id(n2, now2, 0, 0);
                        }
                    }
                }
    return minimise(d);
}

// sound-syllables counted block by block: the story is the syllables so far and whether the last block was loud.
BigUint syllables_count(const PcmFormat& f, const SyllableRule& r, uint32_t length)
{
    uint64_t quiet = 0;
    for (uint64_t c = 0; c < f.base(); ++c) quiet += loud_sample(f, uint32_t(c), r.threshold) ? 0 : 1;
    const size_t states = size_t(r.most + 1) * 2;
    std::vector<BigUint> ways(states, BigUint(0));
    ways[0] = BigUint(1); // (no syllables, the block before quiet: the first loud block begins one)
    auto step = [&](uint32_t k) {
        const BigUint q = BigUint::pow(uint32_t(quiet), k);
        BigUint loud = BigUint::pow(f.base(), k);
        loud -= q;
        std::vector<BigUint> next(states, BigUint(0));
        for (uint64_t n = 0; n <= r.most; ++n)
            for (uint32_t was = 0; was < 2; ++was)
            {
                const BigUint& w = ways[size_t(n * 2 + was)];
                if (w.is_zero()) continue;
                next[size_t(n * 2)] += BigUint::mul(w, q);
                const uint64_t n2 = n + (was ? 0 : 1);
                if (n2 <= r.most && !loud.is_zero()) next[size_t(n2 * 2 + 1)] += BigUint::mul(w, loud);
            }
        ways = std::move(next);
    };
    for (uint32_t b = 0; b < length / r.block; ++b) step(r.block);
    if (length % r.block) step(length % r.block);
    BigUint total(0);
    for (uint64_t n = r.fewest; n <= r.most; ++n) total += ways[size_t(n * 2)], total += ways[size_t(n * 2 + 1)];
    return total;
}

class SyllablesFilter : public Filter
{
public:
    SyllablesFilter(const PcmFormat& f, SyllableRule r, uint32_t length, std::string prov)
        : f_(f), r_(r), count_(syllables_count(f, r, length))
    {
        provenance_ = std::move(prov);
        const uint32_t states = uint32_t((r.most + 1) * 2);
        if (r.most < (1u << 22) && BlockStoryRanker::table_bytes(f.bits, 2, r.block, length, states) <= filter_memory())
        {
            const SyllableRule rule = r;
            BlockStory st{states,
                          [rule](uint32_t g, uint32_t m) -> int64_t {
                              const uint64_t n = g / 2, was = g % 2;
                              if (!m) return int64_t(n * 2);
                              const uint64_t n2 = n + (was ? 0 : 1);
                              return n2 > rule.most ? -1 : int64_t(n2 * 2 + 1);
                          },
                          [rule](uint32_t g) { return g / 2 >= rule.fewest && g / 2 <= rule.most; }};
            ranker_ = std::make_unique<BlockStoryRanker>(f.base(), class_runs(f.base(), [&](uint32_t c) { return loud_sample(f, c, r.threshold) ? 1u : 0u; }), 2,
                                                         r.block, length, std::move(st));
        }
    }
    const BigUint* survivors() const override { return ranker_ ? nullptr : &count_; }
    const Ranker* ranker() const override { return ranker_.get(); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        uint64_t n = 0;
        uint32_t was = 0, now = 0, pos = 0;
        for (const uint32_t c : unit)
        {
            now |= loud_sample(f_, c, r_.threshold) ? 1 : 0;
            if (++pos == r_.block)
            {
                if (now && !was && ++n > r_.most) return false;
                was = now, now = 0, pos = 0;
            }
        }
        if (pos > 0 && now && !was) ++n;
        return n >= r_.fewest && n <= r_.most;
    }

private:
    PcmFormat f_;
    SyllableRule r_;
    BigUint count_;
    std::unique_ptr<Ranker> ranker_;
};

// sound-interval: the two strongest peaks of the sound's spectrum (50 Hz to 2 kHz, the first 2,048 samples or the
// largest power of two the unit holds) a consonant interval apart, not an octave: a minor or major third, a fourth,
// a fifth, a minor or major sixth, within `cents`. The weaker at least a quarter of the stronger.
class IntervalFilter : public Filter
{
public:
    IntervalFilter(const PcmFormat& f, int64_t cents, std::string prov) : f_(f), cents_(cents) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        size_t n = 1;
        while (n * 2 <= unit.size() && n * 2 <= 2048) n *= 2;
        if (n < 256) return false;
        std::vector<double> x(n);
        for (size_t i = 0; i < n; ++i)
            x[i] = double(pcm_sample(f_, unit[i])) * (0.5 - 0.5 * std::cos(2 * 3.14159265358979323846 * double(i) / double(n - 1)));
        const double hz = double(f_.rate) / double(n);
        const size_t lo = std::max<size_t>(1, size_t(50 / hz)), hi = std::min<size_t>(n / 2 - 1, size_t(2000 / hz) + 1);
        if (lo + 2 > hi) return false;
        std::vector<double> mag(hi + 2, 0.0);
        for (size_t k = lo - 1; k <= hi + 1 && k < n / 2; ++k)
        {
            double re = 0, im = 0;
            const double w = 2 * 3.14159265358979323846 * double(k) / double(n);
            for (size_t i = 0; i < n; ++i)
            {
                re += x[i] * std::cos(w * double(i));
                im -= x[i] * std::sin(w * double(i));
            }
            mag[k] = std::sqrt(re * re + im * im);
        }
        // the two strongest local peaks
        size_t a = 0, b = 0;
        for (size_t k = lo; k <= hi; ++k)
            if (mag[k] > mag[k - 1] && mag[k] >= mag[k + 1])
            {
                if (a == 0 || mag[k] > mag[a]) b = a, a = k;
                else if (b == 0 || mag[k] > mag[b]) b = k;
            }
        if (a == 0 || b == 0 || mag[a] <= 0 || mag[b] < 0.25 * mag[a]) return false;
        const double ratio = double(std::max(a, b)) / double(std::min(a, b));
        static const double kConsonant[] = {6.0 / 5, 5.0 / 4, 4.0 / 3, 3.0 / 2, 8.0 / 5, 5.0 / 3};
        for (const double c : kConsonant)
            if (std::fabs(1200.0 * std::log2(ratio / c)) <= double(cents_)) return true;
        return false;
    }

private:
    PcmFormat f_;
    int64_t cents_;
};

// ---------------------------------------------------------------- sound-bend

std::optional<Dfa> bend_dfa(const PcmFormat& f, int64_t limit)
{
    const uint64_t B = f.base();
    // State 0: nothing yet; 1 + a: one sample, a; 1 + B + a B + b: the last two, a then b.
    const uint64_t states = 1 + B + B * B;
    if (!cells_fit(states, B)) return std::nullopt;
    Dfa d;
    d.base = uint32_t(B);
    d.start = 0;
    d.accept.assign(size_t(states), 1);
    d.next.assign(size_t(states * B), Dfa::kDead);
    for (uint64_t c = 0; c < B; ++c) d.next[size_t(c)] = int32_t(1 + c);
    for (uint64_t a = 0; a < B; ++a)
        for (uint64_t c = 0; c < B; ++c) d.next[size_t((1 + a) * B + c)] = int32_t(1 + B + a * B + c);
    for (uint64_t a = 0; a < B; ++a)
        for (uint64_t b = 0; b < B; ++b)
        {
            const int64_t sa = pcm_sample(f, uint32_t(a)), sb = pcm_sample(f, uint32_t(b));
            for (uint64_t c = 0; c < B; ++c)
                if (std::abs(int64_t(pcm_sample(f, uint32_t(c))) - 2 * sb + sa) <= limit)
                    d.next[size_t((1 + B + a * B + b) * B + c)] = int32_t(1 + B + b * B + c);
        }
    return minimise(d);
}

class BendFilter : public Filter
{
public:
    BendFilter(const PcmFormat& f, int64_t limit, std::string prov) : f_(f), limit_(limit) { provenance_ = std::move(prov); }
    bool passes(std::span<const uint32_t> unit) const override
    {
        for (size_t i = 2; i < unit.size(); ++i)
            if (std::abs(int64_t(pcm_sample(f_, unit[i])) - 2 * int64_t(pcm_sample(f_, unit[i - 1])) + int64_t(pcm_sample(f_, unit[i - 2]))) > limit_)
                return false;
        return true;
    }

private:
    PcmFormat f_;
    int64_t limit_;
};

// ---------------------------------------------------------------- sound-periodic

class PeriodicFilter : public Filter
{
public:
    PeriodicFilter(const PcmFormat& f, int64_t tolerance, int64_t share, std::string prov) : f_(f), tolerance_(tolerance), share_(share)
    {
        provenance_ = std::move(prov);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const size_t lo = std::max<size_t>(2, f_.rate / 1000), hi = std::max<size_t>(lo, f_.rate / 50);
        std::vector<int32_t> s(unit.size());
        for (size_t i = 0; i < unit.size(); ++i) s[i] = pcm_sample(f_, unit[i]);
        bool loud = false; // (silence repeats at every period: it is not a pitch)
        for (const int32_t v : s) loud = loud || std::abs(int64_t(v)) > tolerance_;
        if (!loud) return false;
        for (size_t p = lo; p <= hi && p * 2 <= s.size(); ++p)
        {
            size_t close = 0;
            for (size_t i = p; i < s.size(); ++i) close += std::abs(int64_t(s[i]) - s[i - p]) <= tolerance_;
            if (int64_t(close) * 100 >= share_ * int64_t(s.size() - p)) return true;
        }
        return false;
    }

private:
    PcmFormat f_;
    int64_t tolerance_, share_;
};

} // namespace

void add_sound_filters(std::vector<FilterSpec>& out)
{
    FilterSpec p;
    p.id = "sound-peak";
    p.title = "sound-peak";
    p.description = "Sound: no sample louder than a share of full scale. Loud noise, and sound clipped at the top and bottom of "
                    "the range, fail; a sound recorded with room to spare passes. Each channel on its own. Exact at every depth.";
    p.params = {{"percent", "the loudest a sample may be, as a percentage of full scale", FilterParam::Kind::Integer, "90", 1, 100, 1, {}}};
    p.applies = sound_line;
    p.counts_as = "automaton";
    p.make = [own = p.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pct = param_int(*find_filter(own), v, "percent");
        const PeakSet s = peak_set(f.bits, pct);
        const std::string prov = "percent=" + std::to_string(pct) + " limit=" + std::to_string(peak_limit(f.bits, pct));
        if (cells_fit(1, f.base())) return make_dfa_filter(peak_dfa(s), l.length, prov);
        return std::make_unique<PeakFilter>(s, l.length, prov);
    };
    p.counts_as_on = [](const FilterLine& l, const FilterValues*) -> std::string { return cells_fit(1, l.base) ? "automaton" : "own"; };
    out.push_back(p);

    FilterSpec s;
    s.id = "sound-step";
    s.title = "sound-step";
    s.description = "Sound: neighbouring samples differ by at most a share of the range. Noise jumps about the whole range from "
                    "one sample to the next; sound that carries anything mostly moves a little. Each channel on its own. Counted "
                    "as an automaton up to 11 bits a sample; judged past that.";
    s.params = {{"percent", "the largest step between neighbouring samples, as a percentage of the range", FilterParam::Kind::Integer, "50",
                 1, 100, 1, {}}};
    s.applies = sound_line;
    s.counts_as = "automaton";
    s.make = [own = s.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pct = param_int(*find_filter(own), v, "percent");
        const int64_t limit = step_limit(f.bits, pct);
        const std::string prov = "percent=" + std::to_string(pct) + " step=" + std::to_string(limit);
        if (auto dfa = step_dfa(f, limit)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<StepFilter>(f, limit, prov);
    };
    s.counts_as_on = [](const FilterLine& l, const FilterValues*) -> std::string { return cells_fit(uint64_t(l.base) + 1, l.base) ? "automaton" : ""; };
    out.push_back(s);

    FilterSpec r;
    r.id = "silence-run";
    r.title = "silence-run";
    r.description = "Sound: no run of more than so many silent samples. Units that are mostly silence (the start of the line in "
                    "positional order is full of them) fail. Each channel on its own. Counted as an automaton while the run's "
                    "states times the symbols fit; judged past that.";
    r.params = {{"samples", "the longest run of silent samples allowed", FilterParam::Kind::Integer, "4000", 0, 4294967295LL, 1, {}}};
    r.applies = sound_line;
    r.counts_as = "automaton";
    r.make = [own = r.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t n = param_int(*find_filter(own), v, "samples");
        const std::string prov = "samples=" + std::to_string(n);
        if (auto dfa = silence_dfa(l.base, l.length, uint64_t(n))) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<SilenceFilter>(uint64_t(n), prov);
    };
    // An automaton while the run's states (the run allowed, and no more than the line) times the
    // symbols fit; without its settings, at the longest run the line allows.
    r.counts_as_on = [own = r.name()](const FilterLine& l, const FilterValues* v) -> std::string {
        uint64_t run = l.length;
        if (v)
            try { run = std::min<uint64_t>(run, uint64_t(param_int(*find_filter(own), *v, "samples"))); }
            catch (const std::exception&) {} // a malformed setting: as without one
        return cells_fit(run + 1, l.base) ? "automaton" : "";
    };
    out.push_back(r);

    FilterSpec x;
    x.id = "sound-crossings";
    x.title = "sound-crossings";
    x.description = "Sound: at most so many of each thousand neighbouring pairs cross zero (one sample above silence, the next "
                    "below, or the other way; a silent sample keeps the sign before it). Noise crosses about every other sample; "
                    "a tone or a voice far less (a 440 Hz tone at 8,000 samples a second, 110 a thousand). Each channel on its own. "
                    "Counted as an automaton (the last sign and the crossings so far) while it fits; judged past that.";
    x.params = {{"permille", "the most crossings allowed, a thousandth of the neighbouring pairs each", FilterParam::Kind::Integer, "200", 0,
                 1000, 10, {}}};
    x.applies = sound_line;
    x.counts_as = "automaton";
    x.make = [own = x.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pm = param_int(*find_filter(own), v, "permille");
        const uint64_t most = crossings_allowed(l.length, pm);
        const std::string prov = "permille=" + std::to_string(pm) + " crossings=" + std::to_string(most);
        if (auto dfa = crossings_dfa(f, most)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<CrossingsFilter>(f, most, prov);
    };
    x.counts_as_on = [own = x.name()](const FilterLine& l, const FilterValues* v) -> std::string {
        int64_t pm = 1000;
        if (v)
            try { pm = param_int(*find_filter(own), *v, "permille"); }
            catch (const std::exception&) {} // a malformed setting: as without one
        return cells_fit(3 * (crossings_allowed(l.length, pm) + 1), l.base) ? "automaton" : "";
    };
    out.push_back(x);

    FilterSpec b;
    b.id = "sound-bend";
    b.title = "sound-bend";
    b.description = "Sound: the second difference (a sample, less twice the one before, plus the one before that) at most a share of "
                    "the range: sound-step bounds how steeply a wave moves, this how sharply it turns, which real waves do gently "
                    "and noise does not. Each channel on its own. Counted as an automaton (the two samples before) to 7 bits a "
                    "sample; judged past that.";
    b.params = {{"percent", "the largest second difference, as a percentage of the range", FilterParam::Kind::Integer, "25", 1, 200, 1, {}}};
    b.applies = sound_line;
    b.counts_as = "automaton";
    b.make = [own = b.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pct = param_int(*find_filter(own), v, "percent");
        const int64_t limit = step_limit(f.bits, pct);
        const std::string prov = "percent=" + std::to_string(pct) + " bend=" + std::to_string(limit);
        if (auto dfa = bend_dfa(f, limit)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<BendFilter>(f, limit, prov);
    };
    b.counts_as_on = [](const FilterLine& l, const FilterValues*) -> std::string {
        const uint64_t B = l.base;
        return cells_fit(1 + B + B * B, B) ? "automaton" : "";
    };
    out.push_back(b);

    FilterSpec per;
    per.id = "sound-periodic";
    per.title = "sound-periodic";
    per.description = "Sound: it repeats itself a short way on, the mark of a pitch. For some period from the rate / 1,000 to the rate / "
                      "50 samples (a pitch of 50 Hz to 1 kHz), at least `share` percent of the samples are within `tolerance` "
                      "percent of the range of the one a period before; silence alone does not pass. A voice, an instrument, a "
                      "hum pass; noise does not. Each channel on its own. Judged.";
    per.params = {{"tolerance", "how close a sample must be to the one a period before, as a percentage of the range", FilterParam::Kind::Integer,
                   "10", 1, 100, 1, {}},
                  {"share", "the percentage of samples that must be that close", FilterParam::Kind::Integer, "80", 1, 100, 5, {}}};
    per.applies = sound_line;
    per.make = [own = per.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const FilterSpec& spec = *find_filter(own);
        const int64_t tol = param_int(spec, v, "tolerance"), share = param_int(spec, v, "share");
        return std::make_unique<PeriodicFilter>(f, step_limit(f.bits, tol), share,
                                                "tolerance=" + std::to_string(tol) + " share=" + std::to_string(share));
    };
    per.judge_only = true;
    out.push_back(per);

    FilterSpec env;
    env.id = "sound-envelope";
    env.title = "sound-envelope";
    env.description = "Sound: one note's shape, loudness rising (or holding) and then falling, as a struck or plucked note's attack "
                      "and decay. Each block of samples has a loudness, its loudest sample in four steps of full scale (a quarter, a "
                      "half, three quarters, the whole); the blocks' loudness never rises again once it has fallen, the loudest is "
                      "at least the second step, and the last block is at least two steps below it. Noise, as loud at the end as "
                      "at the start, does not pass. Each channel on its own. Picks a kind of sound, so ticking every filter leaves it. "
                      "Counted exactly block by block at any length; as an automaton (the blocks so far, and this block's loudest), so "
                      "it ranks and merges, where its table fits the filter memory.";
    env.params = {{"block", "samples in a block (256 at 8,000 a second is 32 ms)", FilterParam::Kind::Integer, "256", 16, 4096, 16, {}}};
    env.applies = sound_line;
    env.counts_as = "automaton";
    env.picks_a_kind = true;
    env.make = [own = env.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const uint32_t block = uint32_t(param_int(*find_filter(own), v, "block"));
        const std::string prov = "block=" + std::to_string(block);
        if (auto dfa = envelope_dfa(f, block); dfa && DfaRanker::table_bytes(dfa->states(), dfa->base, l.length) <= filter_memory())
            return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<EnvelopeFilter>(f, block, l.length, prov); // (counted and ranked block by block)
    };
    env.counts_as_on = [own = env.name()](const FilterLine& l, const FilterValues* v) -> std::string {
        int64_t block = 256;
        if (v)
            try { block = param_int(*find_filter(own), *v, "block"); }
            catch (const std::exception&) {} // a malformed setting: as without one
        return cells_fit(uint64_t(kEnvStory) * 5 * uint64_t(block), l.base) ? "automaton" : "own"; // (list N5-10: ranked block by block past that)
    };
    out.push_back(env);

    FilterSpec syl;
    syl.id = "sound-syllables";
    syl.title = "sound-syllables";
    syl.description = "Sound: the rhythm of speech, loud and quiet in turn so many times a second (syllables, 3 to 7 a second "
                      "unless changed). A block of samples is loud where its loudest sample reaches the threshold (12% of full "
                      "scale unless changed: speech is often quiet), and each loud "
                      "block after a quiet one begins a syllable; over the unit's length in seconds there are at least `low` and at "
                      "most `high` a second. Noise and a held tone are loud throughout: one syllable. Each channel on its own. "
                      "Picks a kind of sound, so ticking every filter leaves it. Counted exactly block by block at any length; as "
                      "an automaton (the syllables so far, whether the last block was loud, and this one so far), so it ranks and "
                      "merges, where its table fits the filter memory.";
    syl.params = {{"low", "the fewest syllables a second", FilterParam::Kind::Integer, "3", 0, 30, 1, {}},
                  {"high", "the most syllables a second", FilterParam::Kind::Integer, "7", 1, 60, 1, {}},
                  {"threshold", "how loud a block's loudest sample must be, as a percentage of full scale",
                   FilterParam::Kind::Integer, "12", 1, 100, 1, {}},
                  {"block", "samples in a block (256 at 8,000 a second is 32 ms)", FilterParam::Kind::Integer, "256", 16, 4096, 16, {}}};
    syl.applies = sound_line;
    syl.counts_as = "automaton";
    syl.picks_a_kind = true;
    auto syllables_of = [](const std::string& own, const FilterLine& l, const FilterValues& v) {
        const FilterSpec& spec = *find_filter(own);
        const int64_t lo = param_int(spec, v, "low"), hi = param_int(spec, v, "high");
        if (lo > hi) throw std::invalid_argument("sound-syllables: low is more than high");
        return syllable_rule(pcm_format_of(l.symbols_id), l.length, uint32_t(param_int(spec, v, "block")), param_int(spec, v, "threshold"), lo, hi);
    };
    syl.make = [own = syl.name(), syllables_of](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const SyllableRule r = syllables_of(own, l, v);
        const std::string prov = "block=" + std::to_string(r.block) + " threshold=" + std::to_string(r.threshold) + " syllables=" +
                                 std::to_string(r.fewest) + ".." + std::to_string(r.most);
        if (auto dfa = syllables_dfa(f, r); dfa && DfaRanker::table_bytes(dfa->states(), dfa->base, l.length) <= filter_memory())
            return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<SyllablesFilter>(f, r, l.length, prov); // (counted and ranked block by block)
    };
    syl.counts_as_on = [own = syl.name(), syllables_of](const FilterLine& l, const FilterValues* v) -> std::string {
        try
        {
            const SyllableRule r = syllables_of(own, l, v ? *v : FilterValues{});
            return cells_fit((r.most + 1) * 4 * uint64_t(r.block), l.base) ? "automaton" : "own"; // (list N5-10)
        }
        catch (const std::exception&) { return ""; }
    };
    out.push_back(syl);

    FilterSpec iv;
    iv.id = "sound-interval";
    iv.title = "sound-interval";
    iv.description = "Sound: two notes sounding together a consonant interval apart. The two strongest peaks of the sound's spectrum "
                     "(50 Hz to 2 kHz, over its first 2,048 samples) are a minor or major third, a fourth, a fifth, or a minor or "
                     "major sixth apart, within so many cents, the weaker at least a quarter of the stronger. An octave does not "
                     "count, so one note and its own first overtone do not pass. Each channel on its own. Picks a kind of sound. "
                     "Judged.";
    iv.params = {{"cents", "how far from the interval's exact ratio, in hundredths of a semitone", FilterParam::Kind::Integer, "25", 1, 100, 5, {}}};
    iv.applies = sound_line;
    iv.picks_a_kind = true;
    iv.judge_only = true;
    iv.make = [own = iv.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t cents = param_int(*find_filter(own), v, "cents");
        return std::make_unique<IntervalFilter>(pcm_format_of(l.symbols_id), cents, "cents=" + std::to_string(cents));
    };
    out.push_back(iv);
}

} // namespace sieve
