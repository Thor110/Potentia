// Filters for melodies (the notes104 audio line): key; and (list Z9) melody-motif, for every notes set: a
// melody that brings a figure back. A figure is k notes in a row (rests left out), heard as its steps (the
// intervals between them) and its lengths, so a figure moved up or down is the same figure; a melody passes
// where some figure comes back, not overlapping itself, at least `times` times. Music is mostly repetition,
// a motif stated and stated again (Beethoven's fifth's four notes, a nursery rhyme's phrases); noise almost
// never repeats one. Judged: whether a figure recurs is not a rule an automaton of a few states holds.

#include "sieve/audio.hpp"
#include "sieve/filter.hpp"

#include <array>
#include <cstdint>
#include <algorithm>
#include <map>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>

namespace sieve {

namespace {

// Every note belongs to one scale; rests are always allowed. A note's pitch class is its
// semitone above C (C4 and C5 are both 0). Each position is judged on its own, so the survivors
// are every string over the allowed symbols: a of them per position, a^r ways to finish.
const std::array<const char*, 12> kTonics = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
struct Scale
{
    const char* name;
    std::vector<uint32_t> steps; // semitones above the tonic
};
const std::vector<Scale>& scales()
{
    static const std::vector<Scale> s = {
        {"major", {0, 2, 4, 5, 7, 9, 11}},          {"minor", {0, 2, 3, 5, 7, 8, 10}},
        {"harmonic-minor", {0, 2, 3, 5, 7, 8, 11}}, {"major-pentatonic", {0, 2, 4, 7, 9}},
        {"minor-pentatonic", {0, 3, 5, 7, 10}},     {"blues", {0, 3, 5, 6, 7, 10}},
    };
    return s;
}

class KeyRanker : public Ranker
{
public:
    KeyRanker(std::vector<bool> allowed, uint32_t L) : allowed_(std::move(allowed)), L_(L), index_(allowed_.size(), -1)
    {
        for (uint32_t s = 0; s < allowed_.size(); ++s)
            if (allowed_[s])
            {
                index_[s] = int32_t(symbols_.size());
                symbols_.push_back(s);
            }
        a_ = uint32_t(symbols_.size());
        set_count();
    }
    uint32_t length() const override { return L_; }
    uint32_t base() const override { return uint32_t(allowed_.size()); }
    State start() const override { return 0; }
    State next(State, uint32_t c) const override { return c < allowed_.size() && allowed_[c] ? 0 : kDead; }
    BigUint completions(State, uint32_t r) const override { return BigUint::pow(a_, r); }
    bool alive(State, uint32_t) const override { return a_ > 0; }
    // Every position is independent: a survivor is a base-a number whose digits are the
    // positions of its symbols among the allowed ones.
    BigUint rank(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != L_) throw std::invalid_argument("unit has the wrong length");
        std::vector<uint32_t> d(L_);
        for (size_t i = 0; i < L_; ++i)
        {
            if (unit[i] >= index_.size() || index_[unit[i]] < 0) throw std::invalid_argument("unit is not a survivor");
            d[i] = uint32_t(index_[unit[i]]);
        }
        return a_ >= 2 ? BigUint::from_digits(d, a_) : BigUint();
    }
    std::vector<uint32_t> unrank(const BigUint& k) const override
    {
        if (k >= count()) throw std::out_of_range("rank beyond the survivors");
        std::vector<uint32_t> d = a_ >= 2 ? k.to_digits(a_, L_) : std::vector<uint32_t>(L_, 0);
        for (auto& x : d) x = symbols_[x];
        return d;
    }

private:
    std::vector<bool> allowed_;
    uint32_t L_;
    std::vector<int32_t> index_;   // symbol -> its place among the allowed ones, or -1
    std::vector<uint32_t> symbols_; // the allowed symbols, ascending
    uint32_t a_ = 0;
};

class Key : public Filter
{
public:
    Key(const std::string& tonic, const std::string& scale, uint32_t L) : allowed_(kNoteSymbols)
    {
        provenance_ = "tonic=" + tonic + " scale=" + scale;
        // Both names arrive validated against the parameter's own list of choices, so in the tool
        // they cannot be wrong. They are checked again here because a filter is also constructible
        // straight from code, and a name that is not in the list would otherwise walk off the end
        // of kTonics or leave the scale pointer null.
        uint32_t t = 0;
        while (t < kTonics.size() && tonic != kTonics[t]) ++t;
        if (t == kTonics.size()) throw std::invalid_argument("unknown tonic '" + tonic + "'");
        const Scale* sc = nullptr;
        for (const auto& s : scales())
            if (scale == s.name) sc = &s;
        if (!sc) throw std::invalid_argument("unknown scale '" + scale + "'");
        std::array<bool, 12> in{};
        for (uint32_t step : sc->steps) in[(t + step) % 12] = true;
        for (uint32_t d = 0; d < kNoteSymbols; ++d)
        {
            const uint32_t pitch = d / 4; // 0 = rest, 1 = C4 ... 25 = C6
            allowed_[d] = pitch == 0 || in[(pitch - 1) % 12];
        }
        ranker_ = std::make_unique<KeyRanker>(allowed_, L);
    }
    bool passes(std::span<const uint32_t> u) const override
    {
        for (uint32_t d : u)
            if (d >= allowed_.size() || !allowed_[d]) return false;
        return true;
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    std::vector<bool> allowed_;
    std::unique_ptr<Ranker> ranker_;
};

class Motif : public Filter
{
public:
    Motif(const FilterLine& l, uint32_t notes, uint32_t times) : set_(note_set_of(l.symbols_id)), notes_(notes), times_(times)
    {
        provenance_ = "notes=" + std::to_string(notes) + " times=" + std::to_string(times);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        // Each voice on its own (the stack hands a filter one voice at a time): its notes, rests left out.
        std::vector<std::pair<int, uint32_t>> played; // (MIDI, sixteenths)
        for (const uint32_t d : unit)
            if (const uint32_t m = set_.midi(d)) played.push_back({int(m), set_.sixteenths(d)});
        if (played.size() < size_t(notes_) * times_) return false;
        // Each figure: its steps and lengths, where it was last met, and how many times so far (not overlapping).
        std::map<std::vector<int>, std::pair<size_t, uint32_t>> seen;
        for (size_t i = 0; i + notes_ <= played.size(); ++i)
        {
            std::vector<int> key;
            for (size_t j = 0; j < notes_; ++j)
            {
                if (j) key.push_back(played[i + j].first - played[i + j - 1].first);
                key.push_back(int(played[i + j].second));
            }
            auto [it, fresh] = seen.emplace(std::move(key), std::make_pair(i, 1u));
            if (fresh) continue;
            if (i >= it->second.first + notes_)
            {
                it->second.first = i;
                if (++it->second.second >= times_) return true;
            }
        }
        return false;
    }

private:
    NoteSet set_;
    uint32_t notes_, times_;
};

// melody-contrary: two voices moving against each other. Each voice is read as its notes in time (a note's start in
// sixteenths, its pitch; rests left out); wherever two voices both begin a note at once and both have moved from
// the note before, they have moved the same way (similar motion) or opposite ways (contrary). Every pair of voices
// is judged: at least `moves` such joint moves, and at least `percent` of them contrary.
//
// Counted (list N5-11), on two voices: a walk over both voices at once, in time. Each step takes the next digit of
// the voice that is behind (both, where they are level: the only moments both can begin a note together), and a
// state is how far each voice has gone, how far one is ahead of the other in sixteenths, each voice's last pitch
// (none yet: 0), and the moves made together and contrary so far (with percent 100 a similar move ends the walk, and
// the moves are counted up to `moves` only). A note on its own sets its voice's pitch to any of the set's, whatever it
// was, so its counts are summed over the old pitch and laid on every new one; two notes together are split three
// ways by where the old pitches lie about the new (both up or both down: similar; one up, one down: contrary;
// otherwise not a move made together), each a sum over a quarter of the old pitches, from their running sums.
// The survivors are counted, not ranked: the walk goes in time, the unit's digits voice after voice. Its work is
// the states times the pitch pairs (the set's pitches plus none, squared); past `kContraryMostWork` it counts nothing.
constexpr double kContraryMostWork = 4e8;

BigUint contrary_count(const NoteSet& set, uint32_t per, uint32_t moves, uint32_t percent, bool* fits)
{
    *fits = false;
    const uint32_t P = set.pitches(), Q = (P + 1) * (P + 1);
    std::vector<uint32_t> dur;
    uint32_t longest = 0;
    for (uint32_t d = 0; d < set.duration_count(); ++d)
    {
        dur.push_back(set.sixteenths(d));
        longest = std::max(longest, dur.back());
    }
    const bool strict = percent >= 100;
    const uint32_t jmax = strict ? moves : per;
    const double nodes = double(per + 1) * double(per + 1) * double(2 * longest + 1) * double(jmax + 1) * (strict ? 1.0 : double(jmax + 2) / 2.0);
    if (nodes * double(Q) * double(set.duration_count()) > kContraryMostWork) return {};
    *fits = true;
    using Matrix = std::vector<BigUint>; // [pa * (P + 1) + pb]
    struct Key
    {
        uint32_t s, ia;
        int32_t delta;
        uint32_t j, c;
        bool operator<(const Key& o) const { return std::tie(s, ia, delta, j, c) < std::tie(o.s, o.ia, o.delta, o.j, o.c); }
    };
    std::map<Key, Matrix> todo;
    auto add = [&](uint32_t ia, uint32_t ib, int32_t delta, uint32_t j, uint32_t c, const Matrix& m, uint32_t times) {
        if (ia == per || ib == per) delta = 0; // (a voice finished: no more moments together)
        Matrix& to = todo[{ia + ib, ia, delta, j, c}];
        if (to.empty()) to.assign(Q, BigUint());
        for (uint32_t q = 0; q < Q; ++q)
            if (!m[q].is_zero()) to[q].add_mul_small(m[q], times);
    };
    // A note of one voice on its own: summed over its old pitch, laid on every new one.
    auto alone = [&](const Matrix& m, bool voice_a) {
        Matrix out(Q);
        for (uint32_t other = 0; other <= P; ++other)
        {
            BigUint sum;
            for (uint32_t own = 0; own <= P; ++own) sum += voice_a ? m[own * (P + 1) + other] : m[other * (P + 1) + own];
            if (sum.is_zero()) continue;
            for (uint32_t own = 1; own <= P; ++own) (voice_a ? out[own * (P + 1) + other] : out[other * (P + 1) + own]) = sum;
        }
        return out;
    };
    {
        Matrix start(Q);
        start[0] = BigUint(1);
        add(0, 0, 0, 0, 0, start, 1);
    }
    BigUint total;
    while (!todo.empty())
    {
        const auto it = todo.begin();
        const Key k = it->first;
        const Matrix m = std::move(it->second);
        todo.erase(it);
        const uint32_t ia = k.ia, ib = k.s - k.ia;
        if (ia == per && ib == per)
        {
            if (k.j >= moves && uint64_t(k.c) * 100 >= uint64_t(k.j) * percent)
                for (const BigUint& x : m) total += x;
            continue;
        }
        const bool a_moves = ib == per || (ia < per && k.delta < 0), b_moves = ia == per || (ib < per && k.delta > 0);
        if (a_moves || b_moves) // one voice on its own: a rest, or a note
        {
            const Matrix after = alone(m, a_moves);
            for (uint32_t d : dur)
            {
                const int32_t nd = a_moves ? k.delta + int32_t(d) : k.delta - int32_t(d);
                add(ia + (a_moves ? 1 : 0), ib + (b_moves ? 1 : 0), nd, k.j, k.c, m, 1);
                add(ia + (a_moves ? 1 : 0), ib + (b_moves ? 1 : 0), nd, k.j, k.c, after, 1);
            }
            continue;
        }
        // Level: both take their next digit. How many pairs of lengths put one voice so far ahead of the other.
        std::map<int32_t, uint32_t> ahead;
        for (uint32_t da : dur)
            for (uint32_t db : dur) ++ahead[int32_t(da) - int32_t(db)];
        const Matrix after_a = alone(m, true), after_b = alone(m, false);
        // two notes together: the old pitches' running sums (both voices with a pitch), and each new pair's three parts
        std::vector<BigUint> pre(size_t(P + 1) * (P + 1));
        for (uint32_t a = 1; a <= P; ++a)
            for (uint32_t b = 1; b <= P; ++b)
            {
                BigUint v = m[a * (P + 1) + b];
                v += pre[(a - 1) * (P + 1) + b];
                v += pre[a * (P + 1) + b - 1];
                v -= pre[(a - 1) * (P + 1) + b - 1];
                pre[a * (P + 1) + b] = std::move(v);
            }
        auto rect = [&](uint32_t a1, uint32_t a2, uint32_t b1, uint32_t b2) { // old pitches a1..a2 and b1..b2 (1-based, may be empty)
            if (a1 > a2 || b1 > b2) return BigUint();
            BigUint v = pre[a2 * (P + 1) + b2];
            v += pre[(a1 - 1) * (P + 1) + b1 - 1];
            v -= pre[(a1 - 1) * (P + 1) + b2];
            v -= pre[a2 * (P + 1) + b1 - 1];
            return v;
        };
        BigUint all;
        for (const BigUint& x : m) all += x;
        Matrix none(Q), similar(Q), contrary(Q);
        for (uint32_t x = 1; x <= P; ++x)
            for (uint32_t y = 1; y <= P; ++y)
            {
                BigUint sim = rect(1, x - 1, 1, y - 1), con = rect(1, x - 1, y + 1, P);
                sim += rect(x + 1, P, y + 1, P);
                con += rect(x + 1, P, 1, y - 1);
                BigUint rest = all;
                rest -= sim;
                rest -= con;
                const uint32_t q = x * (P + 1) + y;
                none[q] = std::move(rest), similar[q] = std::move(sim), contrary[q] = std::move(con);
            }
        for (const auto& [nd, times] : ahead)
        {
            add(ia + 1, ib + 1, nd, k.j, k.c, m, times);       // two rests
            add(ia + 1, ib + 1, nd, k.j, k.c, after_a, times); // a note and a rest
            add(ia + 1, ib + 1, nd, k.j, k.c, after_b, times); // a rest and a note
            add(ia + 1, ib + 1, nd, k.j, k.c, none, times);    // two notes, not moving together
            if (!strict) add(ia + 1, ib + 1, nd, k.j + 1, k.c, similar, times);
            add(ia + 1, ib + 1, nd, std::min(k.j + 1, jmax), strict ? std::min(k.c + 1, jmax) : k.c + 1, contrary, times);
        }
    }
    return total;
}

class Contrary : public Filter
{
public:
    Contrary(const NoteSet& set, uint32_t per_voice, uint32_t moves, uint32_t percent)
        : set_(set), per_(per_voice), moves_(moves), percent_(percent)
    {
        provenance_ = "moves=" + std::to_string(moves) + " percent=" + std::to_string(percent);
    }
    // Two voices: counted when first asked for (contrary_count), where its work is small enough.
    const BigUint* survivors() const override
    {
        if (set_.voices != 2) return nullptr;
        std::call_once(counted_, [this] { survivors_ = contrary_count(set_, per_, moves_, percent_, &fits_); });
        return fits_ ? &survivors_ : nullptr;
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        const uint32_t voices = set_.voices;
        if (voices < 2 || unit.size() != size_t(per_) * voices) return false;
        // each voice: (start, pitch) of its notes, in time
        std::vector<std::map<uint64_t, uint32_t>> notes(voices);
        for (uint32_t v = 0; v < voices; ++v)
        {
            uint64_t t = 0;
            for (uint32_t i = 0; i < per_; ++i)
            {
                const uint32_t d = unit[size_t(v) * per_ + i];
                if (const uint32_t m = set_.midi(d)) notes[v][t] = m;
                t += set_.sixteenths(d);
            }
        }
        for (uint32_t a = 0; a < voices; ++a)
            for (uint32_t b = a + 1; b < voices; ++b)
            {
                uint64_t joint = 0, contrary = 0;
                uint32_t pa = 0, pb = 0; // the last note of each, as time goes
                auto ia = notes[a].begin(), ib = notes[b].begin();
                while (ia != notes[a].end() || ib != notes[b].end())
                {
                    const uint64_t ta = ia == notes[a].end() ? UINT64_MAX : ia->first;
                    const uint64_t tb = ib == notes[b].end() ? UINT64_MAX : ib->first;
                    if (ta == tb)
                    {
                        const int da = pa ? int(ia->second) - int(pa) : 0, db = pb ? int(ib->second) - int(pb) : 0;
                        if (da != 0 && db != 0)
                        {
                            ++joint;
                            if ((da > 0) != (db > 0)) ++contrary;
                        }
                        pa = ia->second, pb = ib->second;
                        ++ia, ++ib;
                    }
                    else if (ta < tb) pa = (ia++)->second;
                    else pb = (ib++)->second;
                }
                if (joint < moves_ || contrary * 100 < joint * percent_) return false;
            }
        return true;
    }

private:
    NoteSet set_;
    uint32_t per_, moves_, percent_;
    mutable std::once_flag counted_;
    mutable BigUint survivors_;
    mutable bool fits_ = false;
};

} // namespace

void add_audio_filters(std::vector<FilterSpec>& out)
{
    FilterSpec key;
    key.id = "key";
    key.title = "key";
    key.description = "Every note is in one key: its pitch class belongs to the scale on the tonic. Rests always pass. "
                      "Can rank (compact).";
    FilterParam tonic{"tonic", "the key's first note", FilterParam::Kind::Text, "C", 0, 0, 1, {}};
    for (const char* t : kTonics) tonic.choices.push_back(t);
    FilterParam scale{"scale", "the scale's notes, by semitone steps above the tonic", FilterParam::Kind::Text, "major", 0, 0, 1, {}};
    for (const auto& s : scales()) scale.choices.push_back(s.name);
    key.params = {tonic, scale};
    key.applies = [](const FilterLine& l) { return l.kind == "audio" && l.symbols_id == kNotesSymbolsId; };
    key.counts_as = "own";
    key.retired = true; // the same rule as an automaton, on every note set
    key.replaced_by = "key-data-v2";
    key.make = [own = key.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        const FilterSpec& s = *find_filter(own);
        return std::make_unique<Key>(param_value(s, v, "tonic"), param_value(s, v, "scale"), l.length);
    };
    out.push_back(key);

    FilterSpec motif;
    motif.id = "melody-motif";
    motif.title = "melody-motif";
    motif.description = "Melodies: a figure of k notes in a row (rests left out), heard as its steps and lengths, so moved up or "
                        "down it is the same figure, comes back at least `times` times without overlapping itself: a motif stated "
                        "and stated again, as music mostly is and noise almost never. Each voice on its own. Judged.";
    motif.params = {{"notes", "the notes in a figure", FilterParam::Kind::Integer, "4", 2, 16, 1, {}},
                    {"times", "how many times a figure is heard", FilterParam::Kind::Integer, "2", 2, 8, 1, {}}};
    motif.applies = [](const FilterLine& l) { return l.kind == "audio" && is_note_symbols(l.symbols_id); };
    motif.make = [own = motif.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        return std::make_unique<Motif>(l, uint32_t(param_int(spec, v, "notes")), uint32_t(param_int(spec, v, "times")));
    };
    motif.judge_only = true;
    out.push_back(motif);

    FilterSpec con;
    con.id = "melody-contrary";
    con.title = "melody-contrary";
    con.description = "Melodies of two voices or more: the voices move against each other. Wherever two voices both begin a note "
                      "at the same moment and both have moved from the note before, they move the same way (up together, down "
                      "together) or opposite ways (contrary motion, counterpoint's first rule of independence). Every pair of voices "
                      "has at least `moves` such moves, and at least `percent` of them contrary. Judges the voices together (the "
                      "other filters judge each voice on its own). Judged; on two voices its survivors are counted (not ranked), "
                      "by a walk over both voices at once in time, where that walk is small enough.";
    con.params = {{"moves", "the fewest moves made together", FilterParam::Kind::Integer, "2", 1, 64, 1, {}},
                  {"percent", "the share of them that must be contrary", FilterParam::Kind::Integer, "100", 1, 100, 5, {}}};
    con.applies = [](const FilterLine& l) { return l.kind == "audio" && is_note_symbols(l.symbols_id) && note_set_of(l.symbols_id).voices >= 2; };
    con.make = [own = con.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const FilterSpec& spec = *find_filter(own);
        const NoteSet set = note_set_of(l.symbols_id);
        return std::make_unique<Contrary>(set, l.length / set.voices, uint32_t(param_int(spec, v, "moves")), uint32_t(param_int(spec, v, "percent")));
    };
    con.judge_only = true;
    con.whole_unit = true;
    out.push_back(con);
}

} // namespace sieve
