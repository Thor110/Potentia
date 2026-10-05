// Sieve CLI — the four lines (text, image, audio, video): options, input, preview, output.
#pragma once

#include "args.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/canon.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
#include "sieve/sound.hpp"
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

// An audio unit's file: a MIDI file for a note set, a WAV file for a pcm set (sieve/sound.hpp).
std::string audio_file(const Line& line, const std::vector<uint32_t>& digits);
// A pcm unit as text: each channel a row of `columns` shades, each the loudest sample over its
// share of the unit.
std::string pcm_preview(const PcmFormat& f, const std::vector<uint32_t>& digits, uint32_t columns);

// The vault (vault.hpp): whether a unit is withheld. A unit of a byte line by its bytes (it is a
// file), a melody by its MIDI file, a sound by its WAV file, other text as a file written out (its own bytes, and each
// well-known encoding decoded: vault_decode.hpp), never by what it says; a picture or a video by
// each frame, by PDQ (picture_withheld).
bool unit_withheld(const Line& line, const std::vector<uint32_t>& digits);
// A picture in this format (a unit of the image or video line, or a cover), each frame by PDQ.
bool picture_withheld(const ImageFormat& format, const std::vector<uint32_t>& digits);

// The file a unit saves as, in bytes: a PNG for image/video (video frames side by side, `scale`
// pixels a pixel), MIDI for a note set and WAV for a pcm set, UTF-8 text for text. Refuses (VaultWithheld) what the vault
// holds. save_unit writes it to a file.
std::vector<uint8_t> unit_file(const Line& line, const std::vector<uint32_t>& digits, uint32_t scale);
// save_unit chooses by the path's extension: one of export_formats, or any other for the line's own.
inline constexpr uint32_t kDefaultExportFps = 8; // a video saved as video: frames a second, unless given
void save_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& path, uint32_t scale,
               uint32_t fps = kDefaultExportFps);

// ---- saving as other formats

// A format a unit can be saved as: its extension (".mp3"), a name for a file dialog, and the
// ffmpeg encoder that writes it (empty: Sieve writes it itself).
struct ExportFormat
{
    std::string ext, name, encoder;
};
// The formats a unit of this line can be saved as: its own first (PNG, MIDI, WAV, TXT), then those
// the ffmpeg found has an encoder for (none without one).
std::vector<ExportFormat> export_formats(const Line& line);
// A unit as a file of one of those formats (by its extension, case aside): its own through
// unit_file, else written through ffmpeg from its own (pictures `scale` pixels a pixel, video at
// `fps` frames a second). Refuses what the vault holds, as unit_file does.
std::vector<uint8_t> export_unit(const Line& line, const std::vector<uint32_t>& digits, const std::string& ext, uint32_t scale, uint32_t fps);

} // namespace sieve::cli
