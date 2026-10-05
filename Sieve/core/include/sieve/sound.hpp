// Sieve — sound as samples on the audio line (SPECIFICATIONS §3.3).
//
// Beside the note sets (notes104, notes2: sieve/audio.hpp), the audio line can hold sound itself:
// the "pcm" set, chosen by its sample rate, its bit depth and its channels, and named by its id,
// e.g. "pcm/8000/8/C1" (8000 samples a second, 8 bits a sample, one channel). Nothing is capped
// but what one unit can hold: any rate from 1 Hz, any depth from 1 to 31 bits (a digit is a
// 32-bit number, and 2^32 is not one), and any number of channels a WAV file can carry (65535).
//
// A unit is CHANNELS x L samples, channel by channel (channel 1's L samples, then channel 2's), as
// a notes2 unit is voice by voice: L is the line's length, in samples per channel. Each sample is
// one digit, the sample's own bits read as an unsigned number (two's complement), so digit 0 is
// silence and is the padding symbol. Addresses read the whole unit as one number, as every line
// does.
//
// "canon-pcm-v1" fits any sound to a pcm set by fixed rules, so the same sound always lands in the
// same place. Every sample is first a signed 32-bit number (8-bit WAV's unsigned bytes centred,
// shorter samples shifted up, floating-point samples scaled by 2^31, rounded half up and clamped).
// Then, in this order:
//   1. channels: to one channel, the mean of all of them, rounded half up; to C of more, the
//      first C; to more than there are, the last repeated;
//   2. rate: exact area-averaging, as pictures are stretched: target sample t spans [t/R, (t+1)/R)
//      seconds, and is the mean of the source samples over that span, each weighted by how much
//      of it they cover, rounded half up (the same rule up and down; upward it holds each sample);
//   3. depth: rounded half up to the set's bits, and clamped (reported as clipped);
//   4. units: each channel cut into runs of L samples, and every channel padded with silence to the
//      same number of runs; unit k holds run k of every channel.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kPcmCanonVersion = "canon-pcm-v1";
inline constexpr uint32_t kPcmMaxBits = 31;
inline constexpr uint32_t kPcmMaxChannels = 65535;

struct PcmFormat
{
    uint32_t rate = 8000;   // samples a second
    uint32_t bits = 8;      // bits a sample, 1..31
    uint32_t channels = 1;

    std::string id() const; // "pcm/8000/8/C1"
    uint32_t base() const { return uint32_t(1) << bits; }
};

// A pcm set, checked (rate at least 1 and below 2^31, bits 1-31, channels 1-65535); throws with the
// reason otherwise.
PcmFormat make_pcm_format(uint32_t rate, uint32_t bits, uint32_t channels);
bool is_pcm_symbols(std::string_view symbols_id);
PcmFormat pcm_format_of(std::string_view symbols_id); // throws if it is not a pcm id

// A digit's sample, as a signed number of the set's bits, and back.
int32_t pcm_sample(const PcmFormat& f, uint32_t digit);
uint32_t pcm_digit(const PcmFormat& f, int32_t sample);

struct PcmCanonResult
{
    std::vector<std::vector<uint32_t>> units; // each exactly CHANNELS x L digits
    uint32_t source_rate = 0, source_channels = 0;
    uint64_t source_frames = 0;  // samples per channel read
    uint64_t samples = 0;        // samples per channel made, before padding
    uint64_t clipped = 0;        // samples clamped to the set's range
    uint64_t padding = 0;        // silent samples added, every channel counted
};

// canon-pcm-v1, a block of frames at a time: give the source's rate and channels, then its frames
// in order (interleaved signed 32-bit samples), then finish.
class PcmCanoniser
{
public:
    PcmCanoniser(const PcmFormat& target, uint32_t length, uint32_t source_rate, uint32_t source_channels);
    void add(std::span<const int32_t> interleaved);
    PcmCanonResult finish();

private:
    int32_t depth(int64_t v);
    PcmFormat f_;
    uint32_t length_, src_rate_, src_ch_;
    uint64_t num_, den_;                 // target span / source span, in 1/(rate x source rate) seconds, reduced
    std::vector<std::vector<int32_t>> out_; // per target channel: samples made
    std::vector<int64_t> acc_;           // per target channel: the weighted sum of the sample being made
    uint64_t filled_ = 0;                // how much of the current target span the sums cover
    PcmCanonResult r_;
};

// A whole sound in memory: interleaved signed 32-bit samples.
struct PcmAudio
{
    uint32_t rate = 0, channels = 0;
    std::vector<int32_t> samples;
};

// A WAV file's fmt chunk, read (PCM of 8, 16, 24 or 32 bits, or 32- or 64-bit floating point, plain
// or WAVE_FORMAT_EXTENSIBLE); throws std::invalid_argument on any other.
struct WavFormat
{
    uint32_t rate = 0, channels = 0, bits = 0, align = 0; // align: bytes a frame
    bool is_float = false;
};
WavFormat wav_format(std::span<const uint8_t> fmt_body);
// One sample at p, as signed 32 bits (8-bit centred, shorter ones shifted up, floats scaled by
// 2^31, rounded half up and clamped).
int32_t wav_sample(const WavFormat& f, const uint8_t* p);

// A WAV file's sound: PCM of 8, 16, 24 or 32 bits, or 32- or 64-bit floating point, plain or
// WAVE_FORMAT_EXTENSIBLE. A data chunk whose size says 0 or 0xFFFFFFFF (as a stream writes it) runs
// to the end. Throws std::invalid_argument on anything else.
PcmAudio read_wav(std::span<const uint8_t> bytes);
// Whether these are a WAV file's first bytes ("RIFF" .... "WAVE").
bool is_wav(std::span<const uint8_t> head);

// A unit as a WAV file: interleaved, in the smallest container of whole bytes that holds the set's
// bits (8-bit unsigned, else signed little-endian), each sample shifted up to fill it. Plain PCM for
// 8, 16 and 24 bits and one or two channels; WAVE_FORMAT_EXTENSIBLE otherwise, with the set's own
// bits as its valid bits.
std::string pcm_to_wav(const PcmFormat& f, const std::vector<uint32_t>& digits);

// The interleaved signed 16-bit samples of a unit, for playing it.
std::vector<int16_t> pcm_to_s16(const PcmFormat& f, const std::vector<uint32_t>& digits);

} // namespace sieve
