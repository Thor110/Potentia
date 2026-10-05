// Sieve CLI — pictures and video from any format, by way of ffmpeg.
//
// stb_image reads PNG, JPEG, BMP, GIF and TGA itself, and always reads them first, so every address
// made from those files stays as it was. Anything else (MP4, WebM, MKV, MOV, AVI, WebP, TIFF, ...)
// is handed to an ffmpeg program, run as its own process: Sieve links none of it. Each frame comes
// back as RGBA pixels (PAM), exactly as ffmpeg decodes it, in the order the file stores the frames,
// one per stored frame (as a GIF's are), with bit-exact colour conversion; then canon-image-v1 fits
// them to the line as it fits any picture.
//
// Which ffmpeg: the one set (the CLI's --ffmpeg, the hallway's "ffmpeg" setting), else the
// SIEVE_FFMPEG environment variable, else an ffmpeg beside Sieve's own program, else the first on
// the PATH. Sieve works without one; only these other formats need it.
#pragma once

#include "sieve/image.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sieve::cli {

// Sets the ffmpeg to use (empty: look for one as above).
void set_ffmpeg_path(const std::string& path);

// The ffmpeg that would be used, or nothing if there is none.
std::optional<std::string> find_ffmpeg();

// Why no ffmpeg can be run ("there is no ffmpeg at '...'", or that none was found and where to put
// one), or empty if one can.
std::string ffmpeg_missing();

// Its version, as "ffmpeg 7.0.2", or empty if there is none.
std::string ffmpeg_version();

// How a picture or video file was read.
struct MediaRead
{
    std::string decoder; // "stb_image", or ffmpeg's version ("ffmpeg 7.0.2")
    bool more = false;   // ffmpeg stopped at max_frames, and the file has more
    std::string complaints; // what ffmpeg wrote of errors while reading (a damaged file), if anything
};

// Hands every frame of a picture or video file to `each`, in order: stb_image's reading where it
// reads the file (all its frames, as before), else ffmpeg's, up to max_frames, one frame held at a
// time. `what` names the file in an error.
using EachFrame = std::function<void(const RgbaImage&)>;
MediaRead read_media_frames(const std::string& path, uint32_t max_frames, const EachFrame& each);
MediaRead read_media_frames(const uint8_t* data, size_t size, const std::string& what, uint32_t max_frames, const EachFrame& each);

// How a sound file was read.
struct AudioRead
{
    std::string decoder;    // "sieve-wav" (Sieve's own WAV reader), or ffmpeg's version
    std::string complaints; // what ffmpeg wrote of errors while reading, if anything
    uint32_t rate = 0, channels = 0;
};

// Hands a sound file's samples to `block` in order, as interleaved signed 32-bit samples (whole
// frames), after `start` is told its rate and channels: a WAV file by Sieve's own reader
// (sieve/sound.hpp), any other sound (MP3, FLAC, OGG, Opus, AAC, a video's sound, ...) by ffmpeg,
// its first audio stream decoded as it stores it (`-flags +bitexact`), never resampled or mixed
// there (canon-pcm-v1 does that, the same way everywhere).
using AudioStart = std::function<void(uint32_t rate, uint32_t channels)>;
using AudioBlock = std::function<void(std::span<const int32_t>)>;
AudioRead read_media_audio(const std::string& path, const AudioStart& start, const AudioBlock& block);
AudioRead read_media_audio(const uint8_t* data, size_t size, const std::string& what, const AudioStart& start, const AudioBlock& block);

// ---- writing other formats

// Whether the ffmpeg found can encode with this encoder (by `ffmpeg -encoders`, asked once per ffmpeg).
bool ffmpeg_has_encoder(const std::string& name);
// Runs ffmpeg from one file to another, with options before the input and before the output;
// throws with what ffmpeg said if it fails.
void ffmpeg_convert(const std::vector<std::string>& input_options, const std::string& input, const std::vector<std::string>& output_options,
                    const std::string& output);

} // namespace sieve::cli
