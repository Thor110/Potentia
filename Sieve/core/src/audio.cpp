#include "sieve/audio.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <stdexcept>
#include <utility>
#include <string>

namespace sieve {

namespace {

constexpr int kLowMidi = 60;  // C4
constexpr int kHighMidi = 84; // C6
constexpr const char* kDurations = "eqhw";
constexpr uint32_t kTicks[4] = {240, 480, 960, 1920};
constexpr const char* kSharpNames[12] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};

int letter_semitone(char c)
{
    switch (c)
    {
    case 'C': return 0; case 'D': return 2; case 'E': return 4; case 'F': return 5;
    case 'G': return 7; case 'A': return 9; case 'B': return 11;
    default: return -1;
    }
}

int duration_index(char c)
{
    for (int i = 0; i < 4; ++i)
        if (kDurations[i] == c) return i;
    return -1;
}

} // namespace

NotesCanonResult canonicalise_notes(std::string_view text, uint32_t unit_length)
{
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
    NotesCanonResult r;
    std::vector<uint32_t> events;

    size_t i = 0;
    while (i < text.size())
    {
        const char c = text[i];
        if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ',') { ++i; continue; }

        size_t j = i;
        while (j < text.size() && !std::isspace(static_cast<unsigned char>(text[j])) && text[j] != '|' && text[j] != ',') ++j;
        const std::string tok(text.substr(i, j - i));
        i = j;
        auto bad = [&] { return std::invalid_argument("cannot read note '" + tok + "' (examples: C4q  F#5e  Bb4h  Rw)"); };

        size_t k = 0;
        int pitch_index; // 0 = rest
        const char head = static_cast<char>(std::toupper(static_cast<unsigned char>(tok[k++])));
        if (head == 'R') pitch_index = 0;
        else
        {
            int semitone = letter_semitone(head);
            if (semitone < 0) throw bad();
            if (k < tok.size() && tok[k] == '#') { ++semitone; ++k; }
            else if (k < tok.size() && tok[k] == 'b') { --semitone; ++k; ++r.flats_rewritten; }
            if (k >= tok.size() || !std::isdigit(static_cast<unsigned char>(tok[k]))) throw bad();
            const int octave = tok[k++] - '0';
            int midi = (octave + 1) * 12 + semitone;
            bool shifted = false;
            while (midi < kLowMidi) { midi += 12; shifted = true; }
            while (midi > kHighMidi) { midi -= 12; shifted = true; }
            if (shifted) ++r.octave_shifted;
            pitch_index = midi - kLowMidi + 1;
        }
        int dur = 1; // quarter
        if (k < tok.size())
        {
            dur = duration_index(static_cast<char>(std::tolower(static_cast<unsigned char>(tok[k++]))));
            if (dur < 0) throw bad();
        }
        else ++r.default_durations;
        if (k != tok.size()) throw bad();
        events.push_back(static_cast<uint32_t>(pitch_index) * 4 + static_cast<uint32_t>(dur));
    }
    r.events = events.size();

    for (size_t s = 0; s < events.size(); s += unit_length)
    {
        std::vector<uint32_t> unit(events.begin() + s, events.begin() + std::min(events.size(), s + unit_length));
        if (unit.size() < unit_length)
        {
            r.padding = unit_length - unit.size();
            unit.resize(unit_length, 0);
        }
        r.units.push_back(std::move(unit));
    }
    return r;
}

std::string note_token(uint32_t d)
{
    if (d >= kNoteSymbols) throw std::out_of_range("note symbol out of range");
    const uint32_t pitch = d / 4, dur = d % 4;
    std::string s;
    if (pitch == 0) s = "R";
    else
    {
        const int midi = kLowMidi + static_cast<int>(pitch) - 1;
        s = std::string(kSharpNames[midi % 12]) + std::to_string(midi / 12 - 1);
    }
    s.push_back(kDurations[dur]);
    return s;
}

std::string notes_to_notation(const std::vector<uint32_t>& digits)
{
    std::string s;
    for (size_t i = 0; i < digits.size(); ++i)
    {
        if (i) s.push_back(' ');
        s += note_token(digits[i]);
    }
    return s;
}

std::string notes_to_midi(const std::vector<uint32_t>& digits)
{
    std::string track;
    auto vlq = [&](uint32_t v) {
        char buf[5];
        int n = 0;
        buf[n++] = static_cast<char>(v & 0x7F);
        while (v >>= 7) buf[n++] = static_cast<char>(0x80 | (v & 0x7F));
        while (n--) track.push_back(buf[n]);
    };
    auto bytes = [&](std::initializer_list<int> b) { for (int x : b) track.push_back(static_cast<char>(x)); };

    vlq(0); bytes({0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20}); // tempo 500000 us/quarter = 120 bpm
    vlq(0); bytes({0xC0, 0x00});                         // program 0: piano
    uint32_t pending = 0;                                // delta before the next event
    for (uint32_t d : digits)
    {
        if (d >= kNoteSymbols) throw std::out_of_range("note symbol out of range");
        const uint32_t pitch = d / 4, ticks = kTicks[d % 4];
        if (pitch == 0) { pending += ticks; continue; }
        const int midi = kLowMidi + static_cast<int>(pitch) - 1;
        vlq(pending); bytes({0x90, midi, 96});
        vlq(ticks);   bytes({0x80, midi, 0});
        pending = 0;
    }
    vlq(pending); bytes({0xFF, 0x2F, 0x00});

    std::string file = "MThd";
    auto u32be = [&](uint32_t v) { for (int s = 24; s >= 0; s -= 8) file.push_back(static_cast<char>(v >> s)); };
    auto u16be = [&](uint32_t v) { file.push_back(static_cast<char>(v >> 8)); file.push_back(static_cast<char>(v)); };
    u32be(6); u16be(0); u16be(1); u16be(480);
    file += "MTrk";
    u32be(static_cast<uint32_t>(track.size()));
    return file + track;
}

// ---------------------------------------------------------------- notes2, and notes104 as a NoteSet

namespace {

constexpr uint32_t kCodeSixteenths[8] = {1, 2, 3, 4, 6, 8, 12, 16}; // s e E q Q h H w
constexpr const char* kCodeNotation[8] = {"s", "e", "e.", "q", "q.", "h", "h.", "w"};

int code_index(char c)
{
    for (int i = 0; i < 8; ++i)
        if (kNoteDurationCodes[i] == c) return i;
    return -1;
}

std::string midi_name(uint32_t midi) { return std::string(kSharpNames[midi % 12]) + std::to_string(int(midi / 12) - 1); }

} // namespace

std::string note_name(uint32_t midi) { return midi_name(midi); }

uint32_t note_midi_of(const std::string& name)
{
    auto bad = [&] { return std::invalid_argument("cannot read the note '" + name + "' (examples: C3, F#5)"); };
    if (name.size() < 2) throw bad();
    int semitone = letter_semitone(static_cast<char>(std::toupper(static_cast<unsigned char>(name[0]))));
    if (semitone < 0) throw bad();
    size_t k = 1;
    if (name[k] == '#') { ++semitone; ++k; }
    else if (name[k] == 'b') { --semitone; ++k; }
    if (k + 1 != name.size() || !std::isdigit(static_cast<unsigned char>(name[k]))) throw bad();
    const int midi = (name[k] - '0' + 1) * 12 + semitone;
    if (midi < 0) throw bad();
    return uint32_t(midi);
}

NoteSet make_note_set(uint32_t low, uint32_t high, const std::string& durations, uint32_t voices)
{
    if (low < kNoteLowest || high > kNoteHighest)
        throw std::invalid_argument("the notes2 range lies within C2..C7 (MIDI 36..96)");
    if (high < low + 11) throw std::invalid_argument("the notes2 range spans at least an octave (12 notes)");
    if (durations.empty()) throw std::invalid_argument("the notes2 durations need at least one of s e E q Q h H w");
    int last = -1;
    for (char c : durations)
    {
        const int i = code_index(c);
        if (i < 0) throw std::invalid_argument(std::string("'") + c + "' is not a duration (s e E q Q h H w)");
        if (i <= last) throw std::invalid_argument("the durations are listed once each, shortest first (s e E q Q h H w)");
        last = i;
    }
    if (voices < 1 || voices > kMaxVoices) throw std::invalid_argument("notes2 has 1 to 4 voices");
    NoteSet s;
    s.low = low;
    s.high = high;
    s.durations = durations;
    s.voices = voices;
    s.legacy = false;
    return s;
}

std::string NoteSet::id() const
{
    if (legacy) return kNotesSymbolsId;
    return "notes2/" + midi_name(low) + "-" + midi_name(high) + "/" + durations + "/V" + std::to_string(voices);
}

uint32_t NoteSet::midi(uint32_t digit) const
{
    const uint32_t pitch = digit / duration_count();
    return pitch == 0 ? 0 : low + pitch - 1;
}

uint32_t NoteSet::sixteenths(uint32_t digit) const
{
    return kCodeSixteenths[code_index(durations[digit % duration_count()])];
}

bool is_note_symbols(std::string_view id)
{
    try
    {
        (void)note_set_of(id);
        return true;
    }
    catch (const std::invalid_argument&)
    {
        return false;
    }
}

NoteSet note_set_of(std::string_view id)
{
    if (id == kNotesSymbolsId) return NoteSet{};
    // notes2/LOW-HIGH/DURATIONS/VN
    const std::string s(id);
    auto bad = [&] { return std::invalid_argument("'" + s + "' is not a note set (notes104, or notes2/C3-C6/seEqQhHw/V1)"); };
    if (s.rfind("notes2/", 0) != 0) throw bad();
    const size_t a = s.find('/', 7), b = a == std::string::npos ? a : s.find('/', a + 1);
    if (a == std::string::npos || b == std::string::npos) throw bad();
    const std::string range = s.substr(7, a - 7), durations = s.substr(a + 1, b - a - 1), v = s.substr(b + 1);
    const size_t dash = range.find('-');
    if (dash == std::string::npos || v.size() != 2 || v[0] != 'V' || !std::isdigit(static_cast<unsigned char>(v[1]))) throw bad();
    const NoteSet set = make_note_set(note_midi_of(range.substr(0, dash)), note_midi_of(range.substr(dash + 1)), durations, uint32_t(v[1] - '0'));
    if (set.id() != s) throw bad(); // one spelling only (sharps, no flats)
    return set;
}

NotesCanonResult canonicalise_notes2(std::string_view text, const NoteSet& set, uint32_t length)
{
    if (length == 0) throw std::invalid_argument("unit length must be at least 1");
    NotesCanonResult r;
    const uint32_t D = set.duration_count();
    // The voices, split on "//".
    std::vector<std::string_view> parts;
    for (size_t at = 0;;)
    {
        const size_t sl = text.find("//", at);
        parts.push_back(text.substr(at, sl == std::string_view::npos ? std::string_view::npos : sl - at));
        if (sl == std::string_view::npos) break;
        at = sl + 2;
    }
    if (parts.size() > set.voices)
        throw std::invalid_argument("the notation has " + std::to_string(parts.size()) + " voices; the line has " + std::to_string(set.voices));
    std::vector<std::vector<uint32_t>> voices(set.voices);
    for (size_t v = 0; v < parts.size(); ++v)
    {
        const std::string_view part = parts[v];
        size_t i = 0;
        while (i < part.size())
        {
            const char c = part[i];
            if (std::isspace(static_cast<unsigned char>(c)) || c == '|' || c == ',') { ++i; continue; }
            size_t j = i;
            while (j < part.size() && !std::isspace(static_cast<unsigned char>(part[j])) && part[j] != '|' && part[j] != ',') ++j;
            const std::string tok(part.substr(i, j - i));
            i = j;
            auto bad = [&] { return std::invalid_argument("cannot read note '" + tok + "' (examples: C4q  F#5e.  Bb3s  Rh.)"); };
            size_t k = 0;
            uint32_t pitch_index = 0;
            const char head = static_cast<char>(std::toupper(static_cast<unsigned char>(tok[k++])));
            if (head != 'R')
            {
                int semitone = letter_semitone(head);
                if (semitone < 0) throw bad();
                if (k < tok.size() && tok[k] == '#') { ++semitone; ++k; }
                else if (k < tok.size() && tok[k] == 'b') { --semitone; ++k; ++r.flats_rewritten; }
                if (k >= tok.size() || !std::isdigit(static_cast<unsigned char>(tok[k]))) throw bad();
                int midi = (tok[k++] - '0' + 1) * 12 + semitone;
                bool shifted = false;
                while (midi < int(set.low)) { midi += 12; shifted = true; }
                while (midi > int(set.high)) { midi -= 12; shifted = true; }
                if (shifted) ++r.octave_shifted;
                pitch_index = uint32_t(midi) - set.low + 1;
            }
            // The duration: a letter, dotted with '.', or none (a quarter).
            uint32_t want = 4;
            if (k < tok.size())
            {
                const char l = static_cast<char>(std::tolower(static_cast<unsigned char>(tok[k++])));
                const bool dotted = k < tok.size() && tok[k] == '.';
                if (dotted) ++k;
                const std::string code = std::string(1, l) + (dotted ? "." : "");
                int found = -1;
                for (int n = 0; n < 8; ++n)
                    if (code == kCodeNotation[n]) found = n;
                if (found < 0) throw bad();
                want = kCodeSixteenths[found];
            }
            else ++r.default_durations;
            if (k != tok.size()) throw bad();
            // The set's nearest duration (by length; a tie to the longer).
            uint32_t best = 0, best_gap = ~0u;
            for (uint32_t n = 0; n < D; ++n)
            {
                const uint32_t len = kCodeSixteenths[code_index(set.durations[n])];
                const uint32_t gap = len > want ? len - want : want - len;
                if (gap < best_gap || (gap == best_gap && len > kCodeSixteenths[code_index(set.durations[best])]))
                {
                    best = n;
                    best_gap = gap;
                }
            }
            if (best_gap != 0) ++r.durations_changed;
            voices[v].push_back(pitch_index * D + best);
            ++r.events;
        }
    }
    size_t longest = 0;
    for (const auto& v : voices) longest = std::max(longest, v.size());
    const size_t runs = std::max<size_t>(1, (longest + length - 1) / length);
    for (size_t k = 0; k < runs; ++k)
    {
        std::vector<uint32_t> unit;
        unit.reserve(size_t(length) * set.voices);
        for (const auto& v : voices)
            for (size_t e = 0; e < length; ++e)
            {
                const size_t at = k * length + e;
                if (at < v.size()) unit.push_back(v[at]);
                else
                {
                    unit.push_back(0);
                    ++r.padding;
                }
            }
        r.units.push_back(std::move(unit));
    }
    return r;
}

std::string note_token(const NoteSet& set, uint32_t d)
{
    if (set.legacy) return note_token(d);
    if (d >= set.base()) throw std::out_of_range("note symbol out of range");
    const uint32_t midi = set.midi(d);
    return (midi == 0 ? std::string("R") : midi_name(midi)) + kCodeNotation[code_index(set.durations[d % set.duration_count()])];
}

std::string notes_to_notation(const NoteSet& set, const std::vector<uint32_t>& digits)
{
    if (set.legacy) return notes_to_notation(digits);
    const size_t per = digits.size() / set.voices;
    std::string s;
    for (size_t i = 0; i < digits.size(); ++i)
    {
        if (i) s += per && i % per == 0 ? " // " : " ";
        s += note_token(set, digits[i]);
    }
    return s;
}

std::string notes_to_midi(const NoteSet& set, const std::vector<uint32_t>& digits)
{
    if (set.legacy) return notes_to_midi(digits);
    std::string file = "MThd";
    auto u32be = [&](std::string& out, uint32_t v) { for (int s = 24; s >= 0; s -= 8) out.push_back(static_cast<char>(v >> s)); };
    auto u16be = [&](std::string& out, uint32_t v) { out.push_back(static_cast<char>(v >> 8)); out.push_back(static_cast<char>(v)); };
    auto vlq = [](std::string& out, uint32_t v) {
        char buf[5];
        int n = 0;
        buf[n++] = static_cast<char>(v & 0x7F);
        while (v >>= 7) buf[n++] = static_cast<char>(0x80 | (v & 0x7F));
        while (n--) out.push_back(buf[n]);
    };
    auto chunk = [&](const std::string& track) {
        file += "MTrk";
        u32be(file, static_cast<uint32_t>(track.size()));
        file += track;
    };
    u32be(file, 6);
    u16be(file, 1);
    u16be(file, 1 + set.voices);
    u16be(file, 480);
    {
        std::string t;
        vlq(t, 0);
        for (int b : {0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20}) t.push_back(static_cast<char>(b)); // 120 bpm
        vlq(t, 0);
        for (int b : {0xFF, 0x2F, 0x00}) t.push_back(static_cast<char>(b));
        chunk(t);
    }
    const size_t per = digits.size() / set.voices;
    for (uint32_t v = 0; v < set.voices; ++v)
    {
        std::string t;
        vlq(t, 0);
        t.push_back(static_cast<char>(0xC0 | v)); // piano, on the voice's own channel
        t.push_back(0);
        uint32_t pending = 0;
        for (size_t e = 0; e < per; ++e)
        {
            const uint32_t d = digits[v * per + e];
            if (d >= set.base()) throw std::out_of_range("note symbol out of range");
            const uint32_t ticks = set.sixteenths(d) * 120, midi = set.midi(d);
            if (midi == 0) { pending += ticks; continue; }
            vlq(t, pending);
            for (int b : {int(0x90 | v), int(midi), 96}) t.push_back(static_cast<char>(b));
            vlq(t, ticks);
            for (int b : {int(0x80 | v), int(midi), 0}) t.push_back(static_cast<char>(b));
            pending = 0;
        }
        vlq(t, pending);
        for (int b : {0xFF, 0x2F, 0x00}) t.push_back(static_cast<char>(b));
        chunk(t);
    }
    return file;
}

} // namespace sieve
