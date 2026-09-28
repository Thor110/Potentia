// Sieve — the vault's decoders (docs/VAULT.md): the forms a known file can take when it is written
// out as text, and the bytes each gives back.
//
// A hash matches only the exact bytes it was made from, and any alphabet of two or more symbols can
// write out any file (as hex, base64, a-p for the sixteen nibbles, "a" and "b" for bits...). So a
// unit of text is checked not only as it is but as each of a fixed set of encodings, decoded: if
// any decoding is a file the vault holds, the unit is that file written out, and it is withheld.
// Ordinary writing decodes to noise, which matches nothing, so text is never judged by what it
// says; only a known file, in a form Sieve recognises, is refused.
//
// The set is versioned, like the filters, and never edited: a new decoder is a new version. It
// cannot be complete (the encodings are endless, and an address is one more of them); what it does
// is make sure the well-known ones are not a way to have Sieve hand a known file over.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace sieve::cli::vault {

inline constexpr const char* kDecoders = "vault-decoders-v1";

// The names of the decoders in kDecoders, in the order they are tried (for `sieve vault`).
const std::vector<std::string>& decoder_names();

// Every byte string the text (UTF-8) decodes to under kDecoders, each at most once, none empty.
// Whitespace is ignored by every decoder (so a unit's padding and a page's line breaks do not
// matter), and each decoder must account for the whole text, not a part of it.
std::vector<std::vector<uint8_t>> decodings(const std::string& utf8);

// True when the text is withheld: the vault has failed closed, or a decoding is a file it holds.
bool withheld_written(const std::string& utf8);

} // namespace sieve::cli::vault
