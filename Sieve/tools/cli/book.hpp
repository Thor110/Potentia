// Sieve CLI — books: a work composed of units from the lines (SPECIFICATIONS §11).
//
// A book is an ordered list of labelled sections ("title", "cover", "pages", ...). Each section
// names its line and exact shape, the ordering its addresses are written in, and then lists one
// address per unit. The record ("sieve-book-v1") is plain text:
//
//     sieve-book-v1
//     section title                    a label: lowercase letters, digits and '-'
//     line text                        text | image | audio | video, then that line's shape:
//     alphabet lower27                   text:  alphabet, length, canon
//     length 3200                        image: width, height, palette
//     canon canon-text-v2                video: width, height, frames, palette
//     key sieve                          audio: length
//     mode guided                      positional | scrambled | guided
//     model gutenberg-lower27-o5 <sha256>   guided only: the pinned model it was coded with
//     units 1
//     <address>                        one per unit, in order
//     section cover
//     ...
//     end
//     id <sha256>
//
// The id is the book's identity, the SHA-256 of its content alone ("sieve-book-id-v1"): for each
// section, its label, its shape (line/symbols/L<length>), its unit count and each unit's
// positional address, i.e. the content itself. The key, the ordering and the model only choose
// how the addresses are written, so the same book written any of those ways has the same id.
// Reading a book recomputes every unit from its address and refuses it unless the id matches.
//
// Composition does not shrink the space of possible books: a book's content is exactly its
// units. Written in guided order, though, a book of real text takes far fewer digits than its
// characters, because each page's address is as long as its information (about 2 bits per
// character instead of 4.75).
#pragma once

#include "args.hpp"
#include "lines.hpp"

#include "sieve/bookspace.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace sieve::cli {

inline constexpr const char* kBookFormat = "sieve-book-v1";
inline constexpr const char* kBookIdVersion = "sieve-book-id-v1";

struct BookSection
{
    std::string role;
    Args shape;           // the line's options: line, key and the shape fields above
    std::string mode;     // positional | scrambled | guided
    std::string model_id; // guided only
    std::string model_sha256;
    std::vector<std::string> addresses;
};

struct Book
{
    std::vector<BookSection> sections;
    std::string id; // as written in the record (checked against the content when read)
};

// A section's line, built from its shape (guided sections load and check their model).
Line section_line(const BookSection& s);

// Writes the units as a section in the given ordering.
BookSection make_section(const std::string& role, const Args& shape, const std::string& mode,
                         const std::vector<std::vector<uint32_t>>& units);

// The units of every section, recomputed from their addresses (guided addresses must be the
// units' own addresses). Throws if anything does not decode.
struct DecodedSection
{
    const BookSection* section;
    Line line;
    std::vector<std::vector<uint32_t>> units;
};
std::vector<DecodedSection> decode_book(const Book& b);

// The content id of decoded sections.
std::string book_id(const std::vector<DecodedSection>& sections);

// A decoded record as a book of a books line: its cover must be on the line's cover symbols, its
// title and pages on the line's pages (same symbols and length), with at most the line's number
// of pages. A missing cover or title is blank (all zero digits); missing pages are blank pages.
// Throws std::invalid_argument naming the part that does not fit.
BookSpace::Parts record_parts(const std::vector<DecodedSection>& sections, const BookSpace& space);

std::string serialise_book(const Book& b); // computes nothing: writes b.id as given
Book parse_book(std::string_view text);    // strict; does not decode

} // namespace sieve::cli
