#include "synth.hpp"

#include <algorithm>
#include <cmath>

namespace hallway {

namespace {
constexpr int kRate = int(kSynthRate);
constexpr float kSixteenth = 0.125f; // seconds, at 120 bpm
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

std::string Synth::play(const sieve::NoteSet& set, const std::vector<uint32_t>& notes)
{
    stop();
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
        return std::string("audio unavailable: ") + SDL_GetError();
    const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, kRate};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream_) return std::string("audio unavailable: ") + SDL_GetError();

    // At most ten minutes of sound (a line's melodies can be as long as the menu allows).
    constexpr size_t kMaxSamples = size_t(kRate) * 600;
    // Each voice rendered from the start, and the voices added together.
    std::vector<float> pcm;
    const size_t voices = std::max<uint32_t>(1, set.voices), per = notes.size() / voices;
    for (size_t v = 0; v < voices; ++v)
    {
        size_t at = 0;
        for (size_t e = v * per; e < (v + 1) * per && at < kMaxSamples; ++e)
        {
            const uint32_t d = notes[e], midi = set.midi(d);
            const int samples = int(float(set.sixteenths(d)) * kSixteenth * kRate);
            const float freq = midi ? 440.0f * std::pow(2.0f, (float(midi) - 69.0f) / 12.0f) : 0.0f;
            if (pcm.size() < at + size_t(samples)) pcm.resize(at + size_t(samples), 0.0f);
            for (int i = 0; i < samples && freq > 0; ++i)
            {
                const float phase = std::fmod(float(i) * freq / kRate, 1.0f);
                // Short attack and release so notes do not click.
                const float env = std::fmin(1.0f, std::fmin(float(i) / 200.0f, float(samples - i) / 800.0f));
                pcm[at + size_t(i)] += (phase < 0.5f ? 0.12f : -0.12f) * env / std::sqrt(float(voices));
            }
            at += size_t(samples);
        }
    }
    pcm.resize(std::min(pcm.size(), kMaxSamples));
    SDL_PutAudioStreamData(stream_, pcm.data(), int(pcm.size() * sizeof(float)));
    SDL_FlushAudioStream(stream_);
    SDL_ResumeAudioStreamDevice(stream_);
    return "";
}

std::string Synth::play_sound(const sieve::PcmFormat& f, const std::vector<uint32_t>& samples)
{
    stop();
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
        return std::string("audio unavailable: ") + SDL_GetError();
    constexpr uint32_t kDeviceChannels = 8;
    std::vector<int16_t> pcm = sieve::pcm_to_s16(f, samples);
    uint32_t channels = f.channels;
    if (channels > kDeviceChannels)
    {
        const size_t frames = pcm.size() / channels;
        std::vector<int16_t> mono(frames);
        for (size_t i = 0; i < frames; ++i)
        {
            int64_t v = 0;
            for (uint32_t c = 0; c < channels; ++c) v += pcm[i * channels + c];
            mono[i] = int16_t(v / int64_t(channels));
        }
        pcm = std::move(mono);
        channels = 1;
    }
    const SDL_AudioSpec spec{SDL_AUDIO_S16, int(channels), int(std::min<uint32_t>(f.rate, uint32_t(INT32_MAX)))};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream_) return std::string("audio unavailable: ") + SDL_GetError();
    SDL_PutAudioStreamData(stream_, pcm.data(), int(std::min<size_t>(pcm.size() * sizeof(int16_t), size_t(INT32_MAX))));
    SDL_FlushAudioStream(stream_);
    SDL_ResumeAudioStreamDevice(stream_);
    return "";
}

std::vector<float> render_notes3(const sieve::Notes3Set& set, const std::vector<uint32_t>& digits, uint32_t rate)
{
    const double seconds_a_tick = 60.0 / double(set.tempo) / double(set.tpq);
    const size_t most = size_t(rate) * 600;
    const size_t voices = std::max<uint32_t>(1, set.voices), per = digits.size() / voices;
    std::vector<float> pcm;
    for (size_t v = 0; v < voices; ++v)
    {
        double at = 0; // seconds
        for (size_t e = v * per; e < (v + 1) * per; ++e)
        {
            const uint32_t d = digits[e];
            const double len = double(set.ticks(d)) * seconds_a_tick;
            const size_t from = size_t(at * rate), n = size_t(len * rate);
            at += len;
            if (from >= most) break;
            if (set.rest(d) || n == 0) continue;
            const float freq = 440.0f * std::pow(2.0f, (float(set.midi(d)) - 69.0f) / 12.0f);
            const float loud = 0.12f * float(set.velocity(set.level(d))) / 127.0f / std::sqrt(float(voices));
            if (pcm.size() < std::min(most, from + n)) pcm.resize(std::min(most, from + n), 0.0f);
            for (size_t i = 0; i < n && from + i < most; ++i)
            {
                const float phase = std::fmod(float(i) * freq / float(rate), 1.0f);
                const float env = std::fmin(1.0f, std::fmin(float(i) / 200.0f, float(n - i) / 800.0f));
                pcm[from + i] += (phase < 0.5f ? loud : -loud) * env;
            }
        }
    }
    return pcm;
}

std::string Synth::play_samples(const std::vector<float>& mono, uint32_t rate)
{
    stop();
    if (!SDL_WasInit(SDL_INIT_AUDIO) && !SDL_InitSubSystem(SDL_INIT_AUDIO))
        return std::string("audio unavailable: ") + SDL_GetError();
    const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, int(rate)};
    stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, nullptr, nullptr);
    if (!stream_) return std::string("audio unavailable: ") + SDL_GetError();
    SDL_PutAudioStreamData(stream_, mono.data(), int(std::min<size_t>(mono.size() * sizeof(float), size_t(INT32_MAX))));
    SDL_FlushAudioStream(stream_);
    SDL_ResumeAudioStreamDevice(stream_);
    return "";
}

} // namespace hallway
