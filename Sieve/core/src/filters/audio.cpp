// Filters for melodies (the notes104 audio line): key.

#include "sieve/audio.hpp"
#include "sieve/filter.hpp"

#include <array>

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
    KeyRanker(std::vector<bool> allowed, uint32_t L) : allowed_(std::move(allowed)), L_(L)
    {
        for (bool b : allowed_) a_ += b ? 1 : 0;
        set_count();
    }
    uint32_t length() const override { return L_; }
    uint32_t base() const override { return uint32_t(allowed_.size()); }
    State start() const override { return 0; }
    State next(State, uint32_t c) const override { return c < allowed_.size() && allowed_[c] ? 0 : kDead; }
    BigUint completions(State, uint32_t r) const override { return BigUint::pow(a_, r); }
    bool alive(State, uint32_t) const override { return a_ > 0; }

private:
    std::vector<bool> allowed_;
    uint32_t L_;
    uint32_t a_ = 0;
};

class Key : public Filter
{
public:
    Key(const std::string& tonic, const std::string& scale, uint32_t L) : allowed_(kNoteSymbols)
    {
        provenance_ = "tonic=" + tonic + " scale=" + scale;
        uint32_t t = 0;
        while (tonic != kTonics[t]) ++t;
        const Scale* sc = nullptr;
        for (const auto& s : scales())
            if (scale == s.name) sc = &s;
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
    key.make = [](const FilterLine& l, const FilterValues& v, const FilterResources&) {
        const FilterSpec& s = *find_filter("key-v1");
        return std::make_unique<Key>(param_value(s, v, "tonic"), param_value(s, v, "scale"), l.length);
    };
    out.push_back(key);
}

} // namespace sieve
