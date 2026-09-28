// Sieve — the symbolic audio line (SPECIFICATIONS §3.1).
//
// An audio unit is UNIT_LENGTH note events. Each event is one of 104 symbols:
//   pitch: rest, or one of the 25 semitones C4..C6 (MIDI 60..84)       -> 26 choices
//   duration: e (eighth), q (quarter), h (half), w (whole)              ->  4 choices
//   digit = pitch_index * 4 + duration_index, pitch_index 0 = rest, 1 = C4 ... 25 = C6.
// Digit 0 is an eighth rest, which is also the padding symbol.
//
// Notation (input and output): events separated by spaces, e.g. "C4q D4q E4h Rq G#4e Bb4e".
//   Note: letter A-G, optional # or b, octave digit, then e|q|h|w. Rest: R then e|q|h|w.
//   A missing duration means q. Bar lines "|" and commas are ignored.
// "canon-notes-v1": flats are written as the equivalent sharp; pitches outside C4..C6 move by
// whole octaves into range (reported). Output always uses sharps.
//
// The notes2 family (SPECIFICATIONS §3.2): a larger set of note events, chosen by its range, its
// durations and its voices, named by its symbols' id, e.g. "notes2/C3-C6/seEqQhHw/V2":
//   pitches: every semitone from LOW to HIGH (MIDI 36 = C2 .. 96 = C7, at least an octave apart);
//   durations: any of s (sixteenth), e (eighth), E (dotted eighth), q (quarter), Q (dotted
//     quarter), h (half), H (dotted half), w (whole), in that order; in notation the dotted ones
//     are written "e.", "q." and "h.";
//   voices: 1 to 4, each its own line of events with its own time.
//   digit = pitch_index * DURATIONS + duration_index, pitch_index 0 = rest, 1 = LOW; digit 0, a
//   rest of the shortest duration, is the padding symbol.
// A unit is VOICES x L events, voice by voice (voice 1's L, then voice 2's): L is the line's
// length, in events per voice. notes104 is kept exactly as it was, a set of its own; a NoteSet
// describes it too (C4-C6, e q h w, one voice), so code that plays or shows notes has one path.
//
// "canon-notes-v2": as v1, with the durations above (s, e, e., q, q., h, h., w; a missing one is
// q), voices separated by "//" (fewer voices than the line's are filled with rests; more are an
// error), pitches moved by whole octaves into LOW..HIGH, and a duration the set lacks replaced by
// the nearest it has (by length; a tie goes to the longer), reported. Each voice is cut into runs
// of L, and every voice padded to the same number of runs with digit 0: unit k holds run k of
// every voice. Output uses sharps and " // " between voices.
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kNotesSymbolsId = "notes104";
inline constexpr const char* kNotesCanonVersion = "canon-notes-v1";
inline constexpr uint32_t kNoteSymbols = 104;

struct NotesCanonResult
{
    std::vector<std::vector<uint32_t>> units; // each exactly UNIT_LENGTH digits
    size_t events = 0;
    size_t flats_rewritten = 0;
    size_t octave_shifted = 0;
    size_t default_durations = 0;
    size_t padding = 0; // eighth rests added to fill the last unit (notes2: digit-0 rests, every voice)
    size_t durations_changed = 0; // notes2: durations the set lacks, replaced by the nearest
};

NotesCanonResult canonicalise_notes(std::string_view notation, uint32_t unit_length);

// One event -> notation token, e.g. 13*4+1 -> "C5q", 0 -> "Re".
std::string note_token(uint32_t digit);
std::string notes_to_notation(const std::vector<uint32_t>& digits);

// A format-0 Standard MIDI File (120 bpm, 480 ticks per quarter, piano) as raw bytes.
std::string notes_to_midi(const std::vector<uint32_t>& digits);

// ---- the notes2 family, and notes104 described the same way

inline constexpr const char* kNotes2CanonVersion = "canon-notes-v2";
inline constexpr const char* kNoteDurationCodes = "seEqQhHw"; // s e e. q q. h h. w
inline constexpr uint32_t kNoteLowest = 36, kNoteHighest = 96, kMaxVoices = 4;

struct NoteSet
{
    uint32_t low = 60, high = 84;   // MIDI numbers of pitch 1 and of the last pitch
    std::string durations = "eqhw"; // codes from kNoteDurationCodes, in its order
    uint32_t voices = 1;
    bool legacy = true;             // notes104 itself

    std::string id() const;         // "notes104", or "notes2/C3-C6/seEqQhHw/V1"
    uint32_t pitches() const { return high - low + 1; }
    uint32_t duration_count() const { return uint32_t(durations.size()); }
    uint32_t base() const { return (pitches() + 1) * duration_count(); }
    // A digit's MIDI number (0 for a rest) and its length in sixteenths.
    uint32_t midi(uint32_t digit) const;
    uint32_t sixteenths(uint32_t digit) const;
};

// A notes2 set, checked (range within C2..C7 and at least an octave, durations in order, 1-4
// voices); throws with the reason otherwise.
NoteSet make_note_set(uint32_t low, uint32_t high, const std::string& durations, uint32_t voices);
// "C3" -> 48; throws on anything else. And back: 48 -> "C3" (sharps).
uint32_t note_midi_of(const std::string& name);
std::string note_name(uint32_t midi);
bool is_note_symbols(std::string_view symbols_id); // notes104, or a notes2 id
NoteSet note_set_of(std::string_view symbols_id);  // throws if it is neither

// canon-notes-v2: `length` events per voice; each unit is voices x length digits.
NotesCanonResult canonicalise_notes2(std::string_view notation, const NoteSet& set, uint32_t length);
std::string note_token(const NoteSet& set, uint32_t digit); // "C#4q.", "Rs"
// A unit (voices x length digits) as notation, voices joined by " // ".
std::string notes_to_notation(const NoteSet& set, const std::vector<uint32_t>& digits);
// A format-1 MIDI file: a tempo track, then one track per voice on its own channel (120 bpm, 480
// ticks per quarter, piano). notes104 keeps notes_to_midi above; this is for notes2.
std::string notes_to_midi(const NoteSet& set, const std::vector<uint32_t>& digits);

} // namespace sieve
