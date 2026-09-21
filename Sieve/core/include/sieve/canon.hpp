// Sieve — canonicalisation of pasted text for warping (SPECIFICATIONS §6.2).
//
// "canon-text-v2" (default). For each codepoint of strict UTF-8 input:
//   1. Whitespace (TAB, LF, VT, FF, CR, SPACE, NBSP) becomes SPACE.
//   2. Typographic punctuation becomes its ASCII form: curly quotes -> ' and ", dashes and
//      minus -> -, ellipsis -> ..., guillemets -> ".
//   3. Latin accented letters (U+00C0-U+017F) fold to ASCII: é -> e, ß -> ss, æ -> ae, ø -> o.
//   4. If the alphabet has no uppercase letters, A-Z fold to a-z.
//   Then each resulting character is kept if the alphabet has it; otherwise an apostrophe is
//   removed (so "didn't" -> "didnt"), and anything else becomes SPACE (so "well-known" ->
//   "well known", "author/editor" -> "author editor").
//   5. Runs of SPACE collapse to one; leading and trailing SPACE are trimmed.
//   6. The result is split into units of UNIT_LENGTH symbols; the last is padded with SPACE.
//
// "canon-text-v1" (kept so earlier results stay reproducible): case folding and whitespace as
// above, but every other out-of-alphabet codepoint is dropped, which glues words together
// ("well-known" -> "wellknown").
#pragma once

#include "sieve/alphabet.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

enum class CanonVersion { V1, V2 };
inline constexpr CanonVersion kDefaultCanon = CanonVersion::V2;

const char* to_string(CanonVersion v);                     // "canon-text-v1" | "canon-text-v2"
CanonVersion canon_version_from_string(std::string_view s); // accepts "v1", "v2", or the full names

struct CanonResult
{
    CanonVersion version = kDefaultCanon;
    std::vector<std::u32string> units;   // each exactly UNIT_LENGTH symbols
    size_t input_codepoints = 0;
    size_t whitespace_mapped = 0;         // non-SPACE whitespace mapped to SPACE
    size_t transliterated = 0;            // typographic punctuation replaced by ASCII (v2)
    size_t accents_folded = 0;            // accented Latin letters folded to ASCII (v2)
    size_t case_folded = 0;
    size_t dropped = 0;                   // removed: apostrophes (v2) or any unknown codepoint (v1)
    std::u32string dropped_examples;      // first distinct removed codepoints (up to 16)
    size_t separated = 0;                 // unknown codepoints turned into SPACE (v2)
    std::u32string separated_examples;    // first distinct separators (up to 16)
    size_t spaces_collapsed = 0;          // SPACE removed by collapsing and trimming
    size_t padding = 0;                   // SPACE appended to fill the last unit
    size_t canonical_length = 0;          // symbols before padding
};

CanonResult canonicalise_text(std::string_view utf8, const Alphabet& alphabet, uint32_t unit_length,
                              CanonVersion version = kDefaultCanon);

} // namespace sieve
