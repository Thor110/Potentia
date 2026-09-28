// Sieve CLI — the four lines (text, image, audio, video): options, input, preview, output.
#pragma once

#include "args.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/canon.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
#include "sieve/space.hpp"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::cli {

enum class LineKind { Text, Image, Audio, Video };
const char* to_string(LineKind k);
LineKind line_from_string(std::string_view s); // text|image|audio|video

struct Line
{
    LineKind kind = LineKind::Text;
    const Alphabet* alphabet = nullptr;  // text
    CanonVersion canon = kDefaultCanon;  // text
    ImageFormat image;                   // image, video
    Space space;
    // Text lines with a model (see sieve models): the guided, entropy-ordered view of the same
    // units. Null when the alphabet has no model or --model none was given.
    std::shared_ptr<const GuidedLine> guided;
    std::string model_id;

    // Human-readable description of the symbols a unit is made of.
    std::string describe_symbols() const;
};

// Builds the line and its space from --line and the line's own options (and --model).
Line make_line(const Args& a);

struct WarpInput
{
    std::vector<std::vector<uint32_t>> units; // digit vectors, one per unit
    std::vector<std::string> report;          // canonicalisation report lines
};

// Reads TEXT... or --file for the line and canonicalises it into units.
WarpInput read_warp_input(const Line& line, const Args& a);

// Multi-line console preview of one unit (quoted text, ASCII-art image/frames, note list).
std::string preview(const Line& line, const std::vector<uint32_t>& digits);

// The vault (vault.hpp): whether a unit is withheld. A unit of a byte line by its bytes (it is a
// file), a melody by its MIDI file, other text as a file written out (its own bytes, and each
// well-known encoding decoded: vault_decode.hpp), never by what it says; a picture or a video by
// each frame, by PDQ (picture_withheld).
bool unit_withheld(const Line& line, const std::vector<uint32_t>& digits);
// A picture in this format (a unit of the image or video line, or a cover), each frame by PDQ.
bool picture_withheld(const ImageFormat& format, const std::vector<uint32_t>& digits);

// Writes a unit to a file: .png for image/video (video frames side by side), .mid for audio,
// UTF-8 text for text. `scale` enlarges image pixels. Refuses (VaultWithheld) what the vault holds.
void save_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& path, uint32_t scale);

} // namespace sieve::cli
