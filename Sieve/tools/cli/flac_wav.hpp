#pragma once

// v34 draft (8 November 2026, Edward: "make a start on that, use the three levels as the
// size/balanced/speed paths"): a WAV carried as its samples coded by FLAC (libFLAC 1.4.3) and the
// rest of the file as it was, made again byte for byte on installing.
//
// What is carried is the FLAC stream, then the skeleton: the WAV with its data chunk's samples
// taken out, every other byte as it was (the data chunk's own header and any pad byte after it
// included). Made again, the skeleton's chunks are walked from byte 12 to its data chunk, and the
// FLAC stream's samples are written there, as many bytes as that chunk's size says.
//
// Only plain integer PCM is opened: format 1, or 0xFFFE whose sub-format is PCM; 1 to 8 channels;
// 8, 16, 24 or 32 bits a sample (8-bit WAVs are unsigned, the rest signed little-endian); a block of
// channels times bytes a sample; a data chunk of a whole number of blocks, inside the file. Anything
// else is left to the other ways.
//
// An AIFF (Apple's, 1988: the same plain samples, each sample's bytes in the other order, in "FORM"
// chunks with big-endian sizes) is carried the same way, by the same recipe: its skeleton's first
// bytes say which it is. Its COMM chunk gives the channels, the sample frames and the bits (8, 16, 24
// or 32; 8-bit AIFF samples are signed); its SSND chunk's samples follow its offset and block size.
// AIFF-C is opened where its samples are not compressed: "NONE" or "twos" (big-endian), "sowt"
// (little-endian). FLAC's decoding is integer arithmetic, so the same FLAC stream
// always gives the same samples, on any machine; the opener makes the WAV again and compares before
// it is ever chosen.
//
// The three priorities choose how hard libFLAC works (measured on WoWRevived's Music-Martian/03,
// 54,987,452 bytes: results/flac-way-2026-11-08.md): Size, level 8 with linear prediction up to 32
// samples back and the coefficients' precision searched (30,341,223 bytes, made in 19.7 s); Balanced,
// level 8 (30,393,581, 1.1 s); Speed, level 5 (30,660,755, 0.5 s). Each decodes in about 0.3 s.

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// Whether b starts as a WAV does ("RIFF", four bytes, "WAVE").
bool is_wav(const std::vector<uint8_t>& b);

// Whether b starts as an AIFF or AIFF-C does ("FORM", four bytes, "AIFF" or "AIFC").
bool is_aiff(const std::vector<uint8_t>& b);

struct WavOpened
{
    std::vector<uint8_t> inner; // the FLAC stream, then the skeleton
    std::string recipe;         // "flac <skeleton size> <inner size>"
    uint64_t samples = 0;       // samples a channel
};

// How hard libFLAC works: 0 Speed (level 5), 1 Balanced (level 8), 2 Size (level 8, prediction up
// to 32 back, precision searched).
// The WAV opened, where making it again from what is carried gives back b exactly; else nothing.
std::optional<WavOpened> wav_open(const std::vector<uint8_t>& b, int effort);

// The WAV made again from the FLAC stream and the skeleton (the reader's side: anything not as
// wav_open writes it is refused, never followed). `too_big` is asked as the samples are written, so
// a small stream claiming gigabytes of silence is stopped.
std::vector<uint8_t> wav_remake(const std::vector<uint8_t>& inner, size_t skeleton, const std::function<bool(size_t)>& too_big);

} // namespace sieve::cli
