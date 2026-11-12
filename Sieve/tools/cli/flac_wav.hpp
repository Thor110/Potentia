#pragma once

// v34 (8 November 2026, Edward: "make a start on that, use the three levels as the
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
// (little-endian).
//
// Likewise, by the same recipe (8 November 2026, Edward: "work through all of those planned steps and
// formats"): RF64 and BW64 (a WAV whose sizes past 4 GiB are in its "ds64" chunk), Sony's Wave64 (GUIDs
// for chunk names, 64-bit sizes), Sun's .au (a 24-byte head, big-endian samples) and Apple's CAF
// ("lpcm" samples, either order). An .au or a CAF may leave its samples' size unwritten, the samples
// running to the end of the file: its skeleton then ends where they start, and the FLAC stream says
// how many there are. And floating-point samples (32 or 64 bits: a WAV of format 3, AIFF-C "fl32" or
// "fl64", an .au of encoding 6 or 7, a CAF so flagged), where every value is a whole number n over 2^23
// (as a float made from 16- or 24-bit integers is): FLAC codes the numbers at 24 bits, and each is
// written back as that float, bit for bit, by integer arithmetic (8 November 2026). FLAC's decoding is integer arithmetic, so the same FLAC stream
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

// Whether b starts as any sound file this carries does: WAV, RF64 or BW64, AIFF or AIFF-C, Wave64,
// Sun's .au, Apple's CAF; and its name ("WAV", "RF64", "AIFF", "Wave64", ".au", "CAF"; "" for none).
bool is_sound(const std::vector<uint8_t>& b);
const char* sound_name(const std::vector<uint8_t>& b);

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

// A .flac file of b's samples, for installing in b's place ("Sound as FLAC", Edward, 8 November 2026):
// FLAC's streamable subset (any player opens it), b's own rate, at most 24 bits, one or two channels or
// 3 to 8 whose WAVE_FORMAT_EXTENSIBLE channel mask is FLAC's own order; checked to give back b's
// samples exactly. A WAV's LIST INFO tags and an AIFF's NAME, AUTH, ANNO and "(c) " chunks are kept
// as Vorbis comments (TITLE, ARTIST, ALBUM, COMMENT, DATE, GENRE, TRACKNUMBER, COPYRIGHT); its other
// chunks are not. Nothing where b is not so.
std::optional<std::vector<uint8_t>> sound_as_flac(const std::vector<uint8_t>& b, int effort);

// Whether b starts as a FLAC file does ("fLaC").
bool is_flac(const std::vector<uint8_t>& b);

// A FLAC of one channel of 16-bit samples, decoded (its MD5 checked); nothing for any other.
std::optional<std::vector<int16_t>> flac_mono16(const std::vector<uint8_t>& f, uint32_t* rate = nullptr);

// A FLAC file re-coded (Sound as FLAC): its samples decoded and coded again by libFLAC at the effort
// asked, in the streamable subset; its own metadata blocks kept (tags, pictures, cue sheet, application
// blocks) but its padding and seek table (whose points would point into the old frames), after the new
// STREAMINFO; checked to decode to the same samples. Nothing where b is not so (an Ogg FLAC, a FLAC
// after an ID3 tag, one past the subset's bits or rates).
std::optional<std::vector<uint8_t>> flac_recoded(const std::vector<uint8_t>& b, int effort);

// The WAV made again from the FLAC stream and the skeleton (the reader's side: anything not as
// wav_open writes it is refused, never followed). `too_big` is asked as the samples are written, so
// a small stream claiming gigabytes of silence is stopped.
std::vector<uint8_t> wav_remake(const std::vector<uint8_t>& inner, size_t skeleton, const std::function<bool(size_t)>& too_big);

} // namespace sieve::cli
