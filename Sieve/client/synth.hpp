// Sieve hallway — plays an audio unit (notes104, or a notes2 set of 1-4 voices) as simple square-wave
// tones, at 120 bpm. Used when there is no music player (scripted runs); the player's mixer
// plays the melody in hand otherwise.
#pragma once

#include "sieve/audio.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hallway {

// The rate the synth makes sound at.
inline constexpr uint32_t kSynthRate = 44100;

// Open-ended notes (sieve/notes3.hpp) as sound: each voice square tones at its level's loudness,
// at the set's tempo, the voices added together; one channel at `rate`, at most ten minutes. The
// instruments are what a MIDI file asks for; this plays every voice the same way.
std::vector<float> render_notes3(const sieve::Notes3Set& set, const std::vector<uint32_t>& digits, uint32_t rate);

class Synth
{
public:
    ~Synth();
    // Starts playing the melody, replacing anything already playing. Returns "" or an error message.
    std::string play(const sieve::NoteSet& set, const std::vector<uint32_t>& notes);
    // Plays a unit of a pcm set as it is, at its own rate (its channels as they are, up to the
    // eight a device takes; more are mixed to one).
    std::string play_sound(const sieve::PcmFormat& f, const std::vector<uint32_t>& samples);
    // Plays sound already made: one channel, -1..1, at `rate` samples a second.
    std::string play_samples(const std::vector<float>& mono, uint32_t rate);
    void stop();

private:
    SDL_AudioStream* stream_ = nullptr;
};

} // namespace hallway
