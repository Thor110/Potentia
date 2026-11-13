// Sieve hallway — the music visualiser's rings (IDEAS 19.175, its first step): a lens into sound, for
// Sieve's own melodies, whose notes are known exactly.
//
// A circle of rings, the lowest octave innermost and the highest outermost (eleven, MIDI octaves -1 to
// 9), each ring twelve sectors, one a note of the octave, coloured round a colour wheel (C red, then
// round the hues to B), lit as loud as the note sounds now and fading after it (half its light every
// 0.35 s), so the circle shifts with the music; the whole turns slowly. Zoomed in, it is a lens: one
// step opens every note into its overtones (rings of its first eight harmonics, each lit in the
// colour of the note it falls on, fainter as they rise, 1/k for the k-th); the next shows each note's
// own wave, drawn round a circle of its own. The rings show only what the analysis gives: for
// Sieve's own notes that is exact; recorded sound (a spectrum over time) is a later step.
#pragma once

#include <SDL3/SDL.h>

#include <array>
#include <string>
#include <vector>

namespace hallway {

struct RingNote
{
    float midi = 0;  // the pitch (a fraction between notes where the music's character moves it)
    float level = 0; // 0 to 1
};

class Rings
{
public:
    static constexpr int kOctaves = 11, kHarmonics = 8, kZooms = 3;
    // Light rises to what sounds now and fades after it; `dt` in seconds.
    void step(float dt, const std::vector<RingNote>& now);
    void zoom(int d);
    int zoom_level() const { return zoom_; }
    void draw(SDL_Renderer* r, float W, float H) const;
    // For a picture's report: the zoom and every lit note, loudest first ("C4 0.80").
    std::string report() const;
    static std::string note_name(int midi);

private:
    std::array<std::array<float, 12>, kOctaves> light_{};    // by octave and note
    std::array<std::array<float, 12>, kHarmonics> over_{};   // by harmonic and the note it falls on
    std::vector<RingNote> now_;
    int zoom_ = 0;
    float turn_ = 0, time_ = 0;
};

} // namespace hallway
