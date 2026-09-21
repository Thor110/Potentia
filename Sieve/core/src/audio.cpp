#include "sieve/audio.hpp"

#include <algorithm>
#include <cctype>
#include <initializer_list>
#include <stdexcept>

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

} // namespace sieve
