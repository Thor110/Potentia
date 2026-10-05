// Sieve — open-ended notes: the notes3 family on the audio line (SPECIFICATIONS §3.4).
//
// Beside notes104, notes2 (sieve/audio.hpp) and sound itself (pcm, sieve/sound.hpp), notes3 is a
// note set chosen by its settings, with nothing capped but MIDI itself:
//   pitches   every semitone from LOW to HIGH, within MIDI 0..127 (C-1..G9), at least an octave
//   ticks     lengths are whole ticks, TPQ to a quarter note (1..960), from 1 to LONGEST (1..65535)
//   levels    K loudness levels (1..127); level k plays at velocity round(127 k / K)
//   voices    1..15, each its own line of events with its own time, on its own MIDI channel (the
//             drums' channel 10 left out)
//   tempo     quarter notes a minute (1..1000), and an instrument a voice (General MIDI 0..127):
//             how the set plays and is saved, not which units it has
// named by its id, e.g. "notes3/C-1..G9/q4/d16/v8/V1/t120/i0" (instruments comma separated, one a
// voice).
//
// A digit is one event: (length - 1) + LONGEST * class, where class 0 is a rest and class
// 1 + p * K + (k - 1) is pitch p (0 = LOW) at level k. So a set has LONGEST * (1 + PITCHES * K)
// symbols, and digit 0, a rest of one tick, is the padding symbol. A unit is VOICES x L events,
// voice by voice, as notes2's.
//
// Notation: events separated by spaces, voices by "//". A note is its name (C, C#, Db ... B, then the
// octave, -1 to 9), then ":TICKS" and "!LEVEL", either left out; a rest is R with ":TICKS". Lengths
// may instead be notes2's codes (s e e. q q. h h. w), as that many sixteenths of TPQ ticks. So
// "C#4:3!5 R:2 E4q" is C#4 for 3 ticks at level 5, a rest of 2 ticks, and E4 for a quarter.
//
// "canon-notes-v3" fits any notation to a set:
//   - a missing length is a quarter (TPQ ticks); a code's length is round(sixteenths * TPQ / 4),
//     at least 1 tick (reported when it is not whole);
//   - a missing level is the one whose velocity is nearest 96 (what notes104 and notes2 play at),
//     a tie to the louder; a level above K is K (reported);
//   - flats are written as sharps; a pitch outside LOW..HIGH moves by whole octaves into it;
//   - a length beyond LONGEST is the note (or rest) for LONGEST ticks, then rests of at most
//     LONGEST ticks for the remainder (reported);
//   - fewer voices than the set's are filled with rests, more are an error; each voice is cut into
//     runs of L, every voice padded to the same number of runs with digit 0; unit k holds run k of
//     every voice.
// Output uses sharps, every event written in full ("C#4:3!5", "R:2"), and " // " between voices.
#pragma once

#include "sieve/audio.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kNotes3CanonVersion = "canon-notes-v3";
inline constexpr uint32_t kNotes3MaxVoices = 15;

struct Notes3Set
{
    uint32_t low = 0, high = 127;   // MIDI numbers
    uint32_t tpq = 4;               // ticks a quarter note
    uint32_t longest = 16;          // the longest length, in ticks
    uint32_t levels = 8;            // loudness levels
    uint32_t voices = 1;
    uint32_t tempo = 120;           // quarter notes a minute
    std::vector<uint32_t> instruments = {0}; // General MIDI program, one a voice

    std::string id() const;
    uint32_t pitches() const { return high - low + 1; }
    uint32_t base() const { return longest * (1 + pitches() * levels); }

    // A digit's parts: its length in ticks, its MIDI number (0 and rest = true for a rest) and its
    // level (1..K; 0 for a rest).
    uint32_t ticks(uint32_t digit) const { return digit % longest + 1; }
    bool rest(uint32_t digit) const { return digit / longest == 0; }
    uint32_t midi(uint32_t digit) const { return rest(digit) ? 0 : low + (digit / longest - 1) / levels; }
    uint32_t level(uint32_t digit) const { return rest(digit) ? 0 : (digit / longest - 1) % levels + 1; }
    uint32_t velocity(uint32_t level) const; // round(127 * level / K)
    uint32_t default_level() const;          // nearest velocity 96, a tie to the louder
    uint32_t digit(bool rest, uint32_t midi, uint32_t level, uint32_t ticks) const;
};

// A set, checked; throws std::invalid_argument with the reason otherwise. `instruments` may be
// empty (piano for every voice) or one, given for every voice, or one a voice.
Notes3Set make_notes3_set(uint32_t low, uint32_t high, uint32_t tpq, uint32_t longest, uint32_t levels, uint32_t voices, uint32_t tempo,
                          std::vector<uint32_t> instruments);
bool is_notes3_symbols(std::string_view symbols_id);
Notes3Set notes3_set_of(std::string_view symbols_id); // throws if it is not a notes3 id

// "C-1" -> 0, "G9" -> 127, "Db4" -> 61; and back, with sharps: 61 -> "C#4". Throws on anything else.
uint32_t notes3_midi_of(const std::string& name);
std::string notes3_name(uint32_t midi);

struct Notes3CanonResult
{
    std::vector<std::vector<uint32_t>> units; // each exactly VOICES x L digits
    size_t events = 0;
    size_t flats_rewritten = 0, octave_shifted = 0, default_lengths = 0, default_levels = 0;
    size_t lengths_rounded = 0;  // a code's length that is not a whole number of ticks
    size_t lengths_split = 0;    // events longer than LONGEST, split into the event and rests
    size_t levels_clamped = 0;   // levels above K
    size_t padding = 0;          // digit-0 rests filling the voices
};

Notes3CanonResult canonicalise_notes3(std::string_view notation, const Notes3Set& set, uint32_t length);
std::string notes3_token(const Notes3Set& set, uint32_t digit); // "C#4:3!5", "R:2"
std::string notes3_to_notation(const Notes3Set& set, const std::vector<uint32_t>& digits);

// A format-1 MIDI file: a tempo track (the set's tempo), then a track per voice on its own channel
// (1-9, 11-16), its instrument chosen first, each note at its level's velocity; TPQ ticks a quarter.
std::string notes3_to_midi(const Notes3Set& set, const std::vector<uint32_t>& digits);

// A Standard MIDI File read back as notes3 notation for a set: as midi_to_notation reads it
// (a voice a track with notes, one note at a time), times rounded to the set's ticks, each note's
// velocity to the nearest level (a tie to the louder). The file's tempo and instruments are not
// read: they are the line's. `report` receives what was changed, in words.
std::string midi_to_notes3(const std::vector<uint8_t>& midi, const Notes3Set& set, std::vector<std::string>* report = nullptr);

} // namespace sieve
