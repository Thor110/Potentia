#include "synth.hpp"

#include <cmath>

namespace hallway {

namespace {
constexpr int kRate = 44100;
constexpr float kSeconds[4] = {0.25f, 0.5f, 1.0f, 2.0f}; // e q h w at 120 bpm
} // namespace

Synth::~Synth() { stop(); }

void Synth::stop()
{
    if (stream_)
    {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
    }
}

std::string Synth::play(const std::vector<uint32_t>& notes)
{
    stop();
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
        return std::string("audio unavailable: ") + SDL_GetError();
    const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, kRate};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream_) return std::string("audio unavailable: ") + SDL_GetError();

    std::vector<float> pcm;
    for (uint32_t d : notes)
    {
        const uint32_t pitch = d / 4;
        const int samples = int(kSeconds[d % 4] * kRate);
        const float freq = pitch ? 440.0f * std::pow(2.0f, (float(60 + int(pitch) - 1) - 69.0f) / 12.0f) : 0.0f;
        for (int i = 0; i < samples; ++i)
        {
            float v = 0;
            if (freq > 0)
            {
                const float phase = std::fmod(float(i) * freq / kRate, 1.0f);
                // Short attack and release so notes do not click.
                const float env = std::fmin(1.0f, std::fmin(float(i) / 200.0f, float(samples - i) / 800.0f));
                v = (phase < 0.5f ? 0.12f : -0.12f) * env;
            }
            pcm.push_back(v);
        }
    }
    SDL_PutAudioStreamData(stream_, pcm.data(), int(pcm.size() * sizeof(float)));
    SDL_FlushAudioStream(stream_);
    SDL_ResumeAudioStreamDevice(stream_);
    return "";
}

} // namespace hallway
