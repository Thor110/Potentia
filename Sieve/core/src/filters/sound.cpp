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

#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp" // make_dfa_filter
#include "sieve/sound.hpp"

#include <algorithm>
#include <mutex>
#include <optional>
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
        BigUint c(1), p = k_;
        for (uint32_t e = remaining; e > 0; e >>= 1)
        {
            if (e & 1) c = BigUint::mul(c, p);
            if (e > 1) p = BigUint::mul(p, p);
        }
        return c;
    }
    bool alive(State s, uint32_t) const override { return s != kDead; }
    std::vector<uint32_t> unrank(const BigUint& k0) const override
    {
        if (k0 >= count()) throw std::out_of_range("rank beyond the survivors");
        std::vector<uint32_t> out(l_);
        BigUint k = k0;
        for (uint32_t i = l_; i-- > 0;)
        {
            BigUint q, r;
            BigUint::divmod(k, k_, q, r);
            out[i] = s_.digit(r.limbs().empty() ? 0 : r.limbs()[0]); // r < k_ <= 2^31
            k = q;
        }
        return out;
    }
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != l_) throw std::invalid_argument("unit has the wrong length");
        BigUint k;
        for (uint32_t d : unit)
        {
            if (!s_.has(d)) throw std::invalid_argument("not a survivor");
            k = BigUint::mul(k, k_);
            k += BigUint(s_.index(d));
        }
        return k;
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
    p.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pct = param_int(*find_filter("sound-peak-v1"), v, "percent");
        const PeakSet s = peak_set(f.bits, pct);
        const std::string prov = "percent=" + std::to_string(pct) + " limit=" + std::to_string(peak_limit(f.bits, pct));
        if (cells_fit(1, f.base())) return make_dfa_filter(peak_dfa(s), l.length, prov);
        return std::make_unique<PeakFilter>(s, l.length, prov);
    };
    p.counts_as_on = [](const FilterLine& l) -> std::string { return cells_fit(1, l.base) ? "automaton" : "own"; };
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
    s.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const PcmFormat f = pcm_format_of(l.symbols_id);
        const int64_t pct = param_int(*find_filter("sound-step-v1"), v, "percent");
        const int64_t limit = step_limit(f.bits, pct);
        const std::string prov = "percent=" + std::to_string(pct) + " step=" + std::to_string(limit);
        if (auto dfa = step_dfa(f, limit)) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<StepFilter>(f, limit, prov);
    };
    s.counts_as_on = [](const FilterLine& l) -> std::string { return cells_fit(uint64_t(l.base) + 1, l.base) ? "automaton" : ""; };
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
    r.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const int64_t n = param_int(*find_filter("silence-run-v1"), v, "samples");
        const std::string prov = "samples=" + std::to_string(n);
        if (auto dfa = silence_dfa(l.base, l.length, uint64_t(n))) return make_dfa_filter(std::move(*dfa), l.length, prov);
        return std::make_unique<SilenceFilter>(uint64_t(n), prov);
    };
    r.counts_as_on = [](const FilterLine& l) -> std::string {
        return cells_fit(uint64_t(l.length) + 1, l.base) ? "automaton" : ""; // at the longest run the line allows
    };
    out.push_back(r);
}

} // namespace sieve
