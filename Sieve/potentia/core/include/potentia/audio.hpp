// Potentia — the symbolic audio line (SPECIFICATIONS §3.1).
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
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace potentia {

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
    size_t padding = 0; // eighth rests added to fill the last unit
};

NotesCanonResult canonicalise_notes(std::string_view notation, uint32_t unit_length);

// One event -> notation token, e.g. 13*4+1 -> "C5q", 0 -> "Re".
std::string note_token(uint32_t digit);
std::string notes_to_notation(const std::vector<uint32_t>& digits);

// A format-0 Standard MIDI File (120 bpm, 480 ticks per quarter, piano) as raw bytes.
std::string notes_to_midi(const std::vector<uint32_t>& digits);

} // namespace potentia
