#pragma once

// v34 (9 November 2026, Edward: "let's move on to the other formats"): Sound as FLAC for the
// other lossless sound formats, WavPack (.wv), Monkey's Audio (.ape), TTA (.tta) and Apple Lossless
// (ALAC, in an .m4a). Sieve links none of their decoders: each file is decoded by ffmpeg, run as its
// own process (cli/media_decode.hpp: --ffmpeg, the hallway's [paths] ffmpeg, SIEVE_FFMPEG, beside
// Sieve, the PATH), into a FLAC file with its tags and cover pictures, which libFLAC then codes
// again at the priority's effort (flac_recoded). Only the packer needs ffmpeg: what is installed is
// a plain .flac.
//
// Checked before it is ever chosen: ffmpeg reports no error while decoding (CRCs checked: WavPack's,
// Monkey's Audio's and TTA's frames carry them; ALAC's do not); the source is 16 to 24-bit integer
// samples (8-bit, floating-point, 32-bit, WavPack's lossy hybrid and DSD files are left as they are);
// and the MD5 of the FLAC's samples, decoded, is the MD5 of the source's.

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// The lossless sound format b is ("WavPack", "Monkey's Audio", "TTA", "ALAC"), by its first bytes
// (after an ID3v2 tag) or, for ALAC, its MP4 boxes; "" where it is none of them.
const char* lossless_sound_name(const std::vector<uint8_t>& b);

// A .flac file of b's samples, its tags and cover pictures, checked as above; else nothing, and
// (where `why` is given) why. Needs ffmpeg ("needs ffmpeg" where none is found).
std::optional<std::vector<uint8_t>> lossless_as_flac(const std::vector<uint8_t>& b, int effort, std::string* why = nullptr);

} // namespace sieve::cli
