// Sieve hallway — plays an audio unit (notes104) as simple square-wave tones.
#pragma once

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
    std::string play(const std::vector<uint32_t>& notes);
    void stop();

private:
    SDL_AudioStream* stream_ = nullptr;
};

} // namespace hallway
