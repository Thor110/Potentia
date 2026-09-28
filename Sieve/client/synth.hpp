// Sieve hallway — plays an audio unit (notes104, or a notes2 set of 1-4 voices) as simple square-wave
// tones, at 120 bpm. Used when there is no music player (scripted runs); the player's mixer
// plays the melody in hand otherwise.
#pragma once

#include "sieve/audio.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>
#include <vector>

namespace hallway {

class Synth
{
public:
    ~Synth();
    // Starts playing the melody, replacing anything already playing. Returns "" or an error message.
    std::string play(const sieve::NoteSet& set, const std::vector<uint32_t>& notes);
    void stop();

private:
    SDL_AudioStream* stream_ = nullptr;
};

} // namespace hallway
