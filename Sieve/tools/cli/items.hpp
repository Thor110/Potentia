// Sieve CLI -- files carried as items of Sieve's lines (sieve-manifest-v5, SPECIFICATIONS §12.2).
//
// A file that is exactly an item of a line (item_of: a page as its text, a picture or film as its
// PNG at one pixel a pixel, a melody as its MIDI file, byte for byte) need not be carried: its line
// and its positional address there make it again (unit_file). A v5 listing names each line once,
// as "<its space id> <the options that make it>", the options as `sieve` takes them; making the
// line again from those options must give the same space id, or the file is refused, so a change
// to a line's defaults can never make another file quietly.
//
// sieve-manifest-v6 carries the same items at the least their addresses can take: each one's
// positional place as raw bytes, at its line's width (the bytes the line's largest place needs),
// rather than in hex; and it adds the AI line, whose items are models given as the very
// model.safetensors the line writes for them (so a model trained by Sieve is carried as its
// weights' digits, 4 bits a weight at Micro, rather than its file's 32-bit floats).
//
// sieve-manifest-v7 writes less around them (a line's options only where its id does not already
// give them, so "line=ai" for Micro's AI line), and codes the AI line's places by digit-histogram-v1
// (sieve/digit_histogram.hpp: each tensor's histogram, then the model's rank among those with it)
// where that comes to less than their raw bytes: a trained model's digits are not spread as evenly
// as chance spreads them.
#pragma once

#include "cli/args.hpp"
#include "cli/lines.hpp"
#include "cli/pack.hpp"
#include "sieve/aispace.hpp"

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

struct ItemLine
{
    std::string spec; // "<space id> <options>", as the listing writes it
    std::optional<Line> line;                 // the pages, image, video and audio lines
    std::shared_ptr<const AiSpace> ai;        // the AI line (v6)
    uint32_t width = 0;                       // v6: bytes a place on the line takes (its largest place's)
};

// The lines files are carried as items of: the pages (of --alphabet and --page-length, as the
// File Locator's weighing takes them), the image, video and audio lines at their defaults; and,
// `with_ai` (v6), the AI line at the shape given (--ai-layers, --ai-width, --ai-heads, --ai-bits;
// Micro without them).
std::vector<ItemLine> item_lines(const Args& a, bool with_ai = false);
// The bytes a place on a line of `size` places takes: those of its largest place, size - 1 (0 for a
// line of one place).
uint32_t place_width(const BigUint& size);
// A place in hex as `width` raw bytes, most significant first, and back (v6's items stream).
std::vector<uint8_t> place_bytes(const std::string& hex, uint32_t width);
std::string place_hex(std::span<const uint8_t> bytes);
// A line from its options ("line=text alphabet=lower27 length=32 model=none").
Line line_of_options(const std::string& options);
// Finds, for a file's bytes, the first of `lines` it is an item of, and its positional address.
ItemFinder item_finder(const std::vector<ItemLine>& lines);
// Lets manifests with items be read (set_item_maker): each line made from its options once, and
// checked against its space id.
void use_item_maker();
// An installer's files packed with the items of Sieve's lines carried as their places: `version` 8
// (the default: v7, and v8 where a file is carried by the tightened rope, searched for
// `rope_seconds` a file, 0 for no limit), 7 (no rope), 6 (each place as raw bytes, the listing as v6
// wrote it) or 5 (in hex, without the AI line). The lines are those item_lines makes of `a`. Shared
// by `sieve locate --items` and the File Locator.
void add_packed_items(Manifest& m, const std::filesystem::path& root, const Args& a, PackReport* report = nullptr, int version = 8,
                      bool item_hashes = false, double rope_seconds = 0, uint32_t rope_newest = 12, bool trim = true,
                      bool pieces = true, bool optimise_archives = false, bool binary = true,
                      bool gzip = true, bool v20 = true, bool v21 = true, bool v22 = true);
// v7: an AI model's place (hex) on `ai` coded by digit-histogram-v1, its tensors the segments, as
// the fewest bytes that hold the number; and back.
std::vector<uint8_t> ai_place_code(const AiSpace& ai, const std::string& place);
std::vector<uint8_t> ai_digits_of_code(const AiSpace& ai, std::span<const uint8_t> code);

} // namespace sieve::cli
