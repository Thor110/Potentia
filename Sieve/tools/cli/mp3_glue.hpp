#pragma once

// v34 draft (8 November 2026, Edward: "lets focus on making the decoder safe for MP3"): an MP3 carried
// as packMP3 codes it (Matthias Stirner's, LGPL v3; third_party/packmp3, its decoder made safe for
// damaged input by Sieve: results/mp3-pmp-2026-11-08.md), made again byte for byte on installing
// ("pmp <inner size>").
//
// packMP3 keeps its state in globals: every call here holds one lock, so one MP3 is coded at a time.

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// Whether b starts as an MP3 packMP3 might code: an ID3v2 tag ("ID3") or an MPEG-1 Layer III frame's
// sync (FF, then FA or FB).
bool is_mp3(const std::vector<uint8_t>& b);

// b coded by packMP3, where it codes it and that gives b back exactly; else nothing, and (where `why`
// is given) why: "not an MP3's start", "not coded: <packMP3's reason>", or "not given back: <why>".
std::optional<std::vector<uint8_t>> mp3_to_pmp(const std::vector<uint8_t>& b, std::string* why = nullptr);

// The MP3 made again from packMP3's form (the reader's side): throws where the form is damaged or
// not packMP3's, never reading outside it.
std::vector<uint8_t> pmp_to_mp3(const std::vector<uint8_t>& pmp);

} // namespace sieve::cli
