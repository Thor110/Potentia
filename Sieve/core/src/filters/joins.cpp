// Sieve — join filters (list N5-14, IDEAS §13 "Filters made for seams"): filters that look only at the joins
// between the parts of a longer unit (FilterLine::part_length: a composition's units joined, the joined stack of
// tracks and movies; a book's pages read as one text), and leave the rest of each part alone. Edward's examples
// (6 October 2026): "a note held across the boundary, a cut between frames". Judging only the joins, they count
// and rank where a filter of the whole joined unit does not.
//
// join-leap-v1 (melodies joined, each voice on its own): where one part ends on a note and the next begins on one,
// the step between them is at most `semitones`; a rest on either side passes. An automaton over the joined voice,
// its states the place within a part and, at a join, the pitch the part before ended on (rest: none), so it counts,
// ranks and merges with the other automata of the joined stack.
//
// join-cut-v1 (videos joined): the first frame of each part after the first differs from the last frame of the
// part before in at most `changes` pixels, every other frame free: no cut at a join. Ranked by ChangesRanker
// (changes_ranker.hpp), whose parts are the frames, free but for the first of each part after the first.

#include "changes_ranker.hpp"

#include "sieve/audio.hpp"
#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve {

namespace {

// join-leap's automaton: states 0 (the start: nothing before), 1 .. P - 1 (after that many symbols of a part, P the
// part's length), and P + p (a join just passed, the part before ending on pitch p: 0 a rest, else 1 .. pitches).
Dfa leap_dfa(const NoteSet& set, uint32_t base, uint32_t part, uint32_t semitones)
{
    const uint32_t pitches = set.pitches();
    const size_t states = size_t(part) + pitches + 1;
    Dfa d;
    d.base = base;
    d.start = 0;
    d.accept.assign(states, 1);
    d.next.assign(states * base, Dfa::kDead);
    auto pitch_of = [&](uint32_t c) { const uint32_t m = set.midi(c); return m == 0 ? 0u : m - set.low + 1; };
    // after a symbol at place `at` within a part (0-based), the state for the next one
    auto after = [&](uint32_t at, uint32_t c) -> int32_t { return at + 1 == part ? int32_t(part + pitch_of(c)) : int32_t(at + 1); };
    for (uint32_t c = 0; c < base; ++c)
    {
        d.next[size_t(0) * base + c] = after(0, c);
        for (uint32_t s = 1; s < part; ++s) d.next[size_t(s) * base + c] = after(s, c);
        for (uint32_t p = 0; p <= pitches; ++p)
        {
            const uint32_t q = pitch_of(c);
            const bool ok = p == 0 || q == 0 || (p > q ? p - q : q - p) <= semitones;
            if (ok) d.next[size_t(part + p) * base + c] = after(0, c);
        }
    }
    return minimise(d);
}

class JoinCut : public Filter
{
public:
    JoinCut(const FilterLine& l, uint32_t changes) : n_(size_t(l.width) * l.height), changes_(changes)
    {
        provenance_ = "changes=" + std::to_string(changes) + " part=" + std::to_string(l.part_length);
        frames_ = l.length / uint32_t(n_);
        per_ = l.part_length / uint32_t(n_);
        std::vector<char> free_parts(frames_, 1);
        for (uint32_t f = per_; f < frames_; f += per_) free_parts[f] = 0;
        // (its table only where the filter memory holds it, as rows-alike and video-smooth build theirs: past it, judged)
        const size_t most = std::min<size_t>(changes, n_);
        if (const double bytes = ChangesRanker::table_bytes(l.base, n_, most); bytes <= filter_memory())
            ranker_ = std::make_unique<ChangesRanker>(l.base, n_, std::move(free_parts), most, false);
        else set_table_bytes(bytes);
    }
    bool passes(std::span<const uint32_t> unit) const override
    {
        if (unit.size() != size_t(frames_) * n_) return false;
        for (uint32_t f = per_; f < frames_; f += per_)
        {
            size_t changed = 0;
            for (size_t i = 0; i < n_; ++i) changed += unit[size_t(f) * n_ + i] != unit[size_t(f - 1) * n_ + i] ? 1 : 0;
            if (changed > changes_) return false;
        }
        return true;
    }
    const Ranker* ranker() const override { return ranker_.get(); }

private:
    size_t n_;
    uint32_t changes_, frames_ = 0, per_ = 0;
    std::unique_ptr<Ranker> ranker_;
};

} // namespace

void add_join_filters(std::vector<FilterSpec>& out)
{
    FilterSpec leap;
    leap.id = "join-leap";
    leap.title = "join-leap";
    leap.description = "Melodies joined (a track's units, the joined filters): where one unit ends on a note and the next begins on "
                       "one, the step between them is at most `semitones`, so the melody runs on across the join rather than jumping; "
                       "a rest on either side passes. Only the joins are judged, each voice on its own. An automaton: counted, ranked "
                       "and merged with the joined stack's other automata.";
    leap.params = {{"semitones", "the largest step across a join, in semitones", FilterParam::Kind::Integer, "7", 0, 48, 1, {}}};
    leap.applies = [](const FilterLine& l) { return l.kind == "audio" && is_note_symbols(l.symbols_id) && l.part_length > 0 && l.part_length < l.length; };
    leap.counts_as = "automaton";
    leap.make = [own = leap.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        const uint32_t k = uint32_t(param_int(*find_filter(own), v, "semitones"));
        if (l.part_length == 0 || l.length % l.part_length != 0) throw std::invalid_argument(own + ": a unit made of whole parts");
        return make_dfa_filter(leap_dfa(note_set_of(l.symbols_id), l.base, l.part_length, k), l.length,
                               "semitones=" + std::to_string(k) + " part=" + std::to_string(l.part_length));
    };
    out.push_back(leap);

    FilterSpec cut;
    cut.id = "join-cut";
    cut.title = "join-cut";
    cut.description = "Videos joined (a movie's units, the joined filters): the first frame of each unit differs from the last frame "
                      "of the unit before in at most `changes` pixels, so the picture carries on across the join rather than cutting; "
                      "every other frame is free. Only the joins are judged. Counted and ranked exactly.";
    cut.params = {{"changes", "the most pixels that change across a join", FilterParam::Kind::Integer, "2", 0, 1 << 20, 1, {}}};
    cut.applies = [](const FilterLine& l) {
        const uint64_t n = uint64_t(l.width) * l.height;
        return l.kind == "video" && n > 0 && l.part_length > 0 && l.part_length < l.length && l.part_length % n == 0 && l.length % l.part_length == 0;
    };
    cut.counts_as = "own";
    cut.make = [own = cut.name()](const FilterLine& l, const FilterValues& v, const FilterResources&) -> std::unique_ptr<Filter> {
        return std::make_unique<JoinCut>(l, uint32_t(param_int(*find_filter(own), v, "changes")));
    };
    out.push_back(cut);
}

} // namespace sieve
