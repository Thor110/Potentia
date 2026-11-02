// Sieve CLI -- binary written as text, carried as what it is (spans-v1; sieve-manifest-v33 draft).
//
// A text holding binary written out as text (base64: a picture in a data: URL, a mail's attachment,
// a key in PEM; hex: a dump, a blob; a list of byte values: a C array, an escaped string, a JSON
// list) is carried as v31 carries an archive: a skeleton, and members after it. The file's own entry
// ("spn <members> <skeleton size>") carries the skeleton: the text with each span taken out, a table
// at its start saying where each span was and how it was written. Each span follows as a member,
// "<file>/<n>", its bytes decoded, so it is carried its own way (a text reaches the word list, a
// JPEG brunsli). Installing, each member is written again as the table says and put back.
//
// The skeleton:
//   a varint, the spans (1 or more, as the recipe counts);
//   for each span, in the file's order:
//     a varint, the text's bytes between the last span's end (the file's start, for the first) and it;
//     a byte, how it is written: bits 3 and 4 its kind, the rest by kind (no other bit set):
//       kind 0, base64: bit 0 the URL-safe alphabet ("-_" for "+/"); bit 1 padded with "=" to a
//         multiple of 4 characters; bit 2 lines end CR LF, else LF;
//       kind 1, hex, two digits a byte, nothing between: bit 0 capitals (A-F), else small letters;
//         bit 2 lines end CR LF, else LF;
//       kind 2, a list of byte values, each its prefix and then its value: bits 0 and 1 the
//         value's form (0: two hex digits, small letters; 1: two hex digits, capitals; 2: decimal,
//         0 to 255, no leading zero);
//     a varint, its width: kinds 0 and 1, the characters a line (0: one line), a line end between
//       each two lines, none after the last; kind 2, the values a line (0: one line);
//     kind 2 only: its prefix, its separator and (where its width is not 0) its line break, each a
//       varint of its length (at most 64) and its bytes: the values are written each after its
//       prefix, the separator between each two, but for the line break after every width-th value
//       (none after the last);
//   then the text, every byte not in a span, in order.
// Varints are LEB128, 7 bits a byte, low first, as short as they can be written. IDEAS §19.114 (1).

#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sieve::cli::spans {

// How a span is written (the table's entry, but for where it is).
struct Form
{
    uint8_t how = 0;    // its kind in bits 3 and 4, the rest by kind (above)
    uint32_t width = 0; // characters a line (kinds 0 and 1), values a line (kind 2); 0: one line
    std::string prefix, separator, line; // kind 2 only
};

struct Span
{
    size_t at = 0, length = 0;  // where in the file, and how many bytes the span is there
    Form form;
    std::vector<uint8_t> bytes; // what it holds, decoded
};

// The bytes written as `form` says.
std::vector<uint8_t> written(std::span<const uint8_t> bytes, const Form& form);

// The file's spans that hold at least `smallest` bytes each and are given back exactly by written
// (checked, every one), in the file's order, none overlapping: base64, hex, and lists of byte values.
std::vector<Span> spans_of(std::span<const uint8_t> file, size_t smallest);

// The skeleton for the file with these spans taken out.
std::vector<uint8_t> skeleton_of(std::span<const uint8_t> file, const std::vector<Span>& spans);

// The file made again from its skeleton and its members' bytes (as many as the skeleton counts),
// refused where the skeleton is not as written above.
std::vector<uint8_t> made_from(std::span<const uint8_t> skeleton, const std::vector<std::span<const uint8_t>>& members);

// How many members a skeleton counts (its first varint), refused where it has none.
uint64_t members_of(std::span<const uint8_t> skeleton);

} // namespace sieve::cli::spans
