// Sieve CLI -- binary written as text, carried as what it is (spans-v1; sieve-manifest-v33 draft).
//
// A text holding base64 (a picture in a data: URL, a mail's attachment, a key in PEM, a font in CSS)
// is carried as v31 carries an archive: a skeleton, and members after it. The file's own entry
// ("b64 <members> <skeleton size>") carries the skeleton: the text with each span of base64 taken
// out, a table at its start saying where each span was and how it was written. Each span follows as
// a member, "<file>/<n>", its bytes decoded, so it is carried its own way (a text reaches the word
// list, a program the x86 filter). Installing, each member is written again as the table says and
// put back.
//
// The skeleton:
//   a varint, the spans (1 or more, as the recipe counts);
//   for each span, in the file's order: a varint, the text's bytes between the last span's end (the
//   file's start, for the first) and this span; a byte, how it is written (bit 0: the URL-safe
//   alphabet, "-_" for "+/"; bit 1: padded with "=" to a multiple of 4; bit 2: lines end CR LF, else
//   LF; no other bit); a varint, the characters a line (0: one line);
//   then the text, every byte not in a span, in order.
// A span written in lines has a line end between each two lines, none after the last. Varints are
// LEB128, 7 bits a byte, low first, as short as they can be written. IDEAS §19.114 (1).

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace sieve::cli::spans {

struct Span
{
    size_t at = 0, length = 0; // where in the file, and how many bytes the span is there
    uint8_t how = 0;           // bit 0 URL-safe, bit 1 padded, bit 2 CR LF
    uint32_t width = 0;        // characters a line, 0 for one line
    std::vector<uint8_t> bytes; // what it holds, decoded
};

// The bytes written as base64 the way `how` and `width` say.
std::vector<uint8_t> base64_written(std::span<const uint8_t> bytes, uint8_t how, uint32_t width);

// The file's spans of base64 that hold at least `smallest` bytes each and are given back exactly by
// base64_written (checked, every one), in the file's order, none overlapping.
std::vector<Span> base64_spans(std::span<const uint8_t> file, size_t smallest);

// The skeleton for the file with these spans taken out.
std::vector<uint8_t> skeleton_of(std::span<const uint8_t> file, const std::vector<Span>& spans);

// The file made again from its skeleton and its members' bytes (as many as the skeleton counts),
// refused where the skeleton is not as written above.
std::vector<uint8_t> made_from(std::span<const uint8_t> skeleton, const std::vector<std::span<const uint8_t>>& members);

// How many members a skeleton counts (its first varint), refused where it has none.
uint64_t members_of(std::span<const uint8_t> skeleton);

} // namespace sieve::cli::spans
