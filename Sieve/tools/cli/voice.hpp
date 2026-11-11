// Sieve's own voice, of units (IDEAS 19.135, 9 November 2026): speech made by joining small sounds
// cut from an hour of one reader's recordings (LJ Speech, public domain; data/anchors/public-domain/
// voice/lj-speech), the voice of Text to Voice.
//
// - The units are diphones: from the middle of one speech sound (a phone) to the middle of the next,
//   every one in the recordings, found by the alignment (data/voice/lj-speech/alignment.tsv: each
//   clip's phones and where each starts and ends, made once by tools/voice/align_voice.py).
// - A text is read into phones by CMUdict (data/voice/lj-speech/cmudict.dict; its licence beside
//   it): its words looked up, a number said as words; a word it does not have as a word it has with
//   an ending (printers: printer, -s), else by letter-to-sound rules learnt from it (lts.tsv); a
//   pause at each comma or full stop.
// - For each pair of phones in turn, one of its takes is chosen: the ones whose joins are smoothest
//   (the spectrum at the end of one against the start of the next; two takes that were next to each
//   other in the recording join at no cost), and of a usual length, all chosen at once (Viterbi).
//   Each join is lined up on the waveform (up to 5 ms) and crossfaded (3 ms).
//
// The same text gives the same samples. Measured (results/voice-units-2026-11-09.md): an offline
// recogniser understood 76.7% of the words of 60 sentences not in the recordings' transcripts.
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <utility>
#include <string>
#include <vector>

namespace sieve::cli {

class Voice
{
public:
    // The voice in a folder holding alignment.tsv and cmudict.dict, its clips in `clips` (the FLAC
    // files the alignment names). Throws std::runtime_error where any is missing or unreadable.
    static std::unique_ptr<Voice> load(const std::filesystem::path& folder, const std::filesystem::path& clips);
    // The voice where it is usually kept: `given` if not empty, else data/voice/lj-speech (here, or
    // beside the program, or in a folder above either), its clips beside it or in the anchors.
    static std::unique_ptr<Voice> find(const std::filesystem::path& given = {});
    // Where find would load it from (the folder, and its clips), without loading it; nothing where
    // it is not found.
    static std::optional<std::pair<std::filesystem::path, std::filesystem::path>> where(const std::filesystem::path& given = {});

    static constexpr uint32_t kRate = 22050; // samples a second (16-bit, one channel)

    // `text` (UTF-8) said: its samples.
    std::vector<int16_t> speak(const std::string& text) const;
    // The phones `text` is read as (CMUdict's, without stress; SIL a pause), for checking.
    std::vector<std::string> phones(const std::string& text) const;

    size_t diphones() const;
    size_t takes() const;
    ~Voice();

private:
    Voice();
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// A WAV file of 16-bit samples, one channel, at `rate`.
std::vector<uint8_t> wav_of(const std::vector<int16_t>& samples, uint32_t rate);

} // namespace sieve::cli
