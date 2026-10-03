// Sieve — text that is a file written out, and the filter that sets it aside ("not-written-v1").
//
// Any alphabet of two or more symbols can write out any file: as hex, as base64, as the sixteen
// letters a-p, as two symbols for the bits. The vault reads text those ways (vault-decoders-v1,
// tools/cli/vault_decode.hpp) to refuse known files. not-written-v1 reads a unit the same ways and
// asks one thing of each reading: is it a file that says what it is? That is, do its first bytes
// carry a signature (file-kinds-v1, sieve/filekind.hpp: PNG, ZIP, MID, PDF, ...)? A unit that is
// such a file, under any of the readings chosen, belongs on the binary line (where the same bytes
// have a place of their own) rather than on this one, and fails the filter.
//
// The readings, each of the unit's whole text (its padding included), as the vault reads it:
//   text     its own bytes: its code points as bytes when all are below 256, else its UTF-8; as it
//            is, without its trailing whitespace, and without it but with one line feed
//   hex      hexadecimal digits, either case, 0x and \x prefixes dropped, any of space , : ; - _
//            between; an even number of digits
//   base64   the standard or the URL-safe alphabet (not both), padded with = or not, whitespace
//            ignored, a data: URL's prefix up to "base64," dropped; not 1 more than a multiple of 4
//   base32   RFC 4648, either case, padded or not, whitespace ignored
//   decimal  byte values 0-255 of up to three digits, separated by whitespace , ; ( ) [ ] { }
//   nibbles  the letters a-p (either case), one per four bits, a being 0; an even number of them
//   spelled  zero .. fifteen as words, or the single letters a-f (10-15), one per four bits,
//            separated by whitespace , . or -; an even number of them
//   binary   any two symbols (whitespace ignored), one per bit, most significant first, eight to a
//            byte, read both ways round
// Every decoder reads the text's UTF-8 bytes except `text`, which reads its code points. The
// vault's ninth, ascii85, is not among them in v1: its groups of five symbols are base 85, so a
// byte's value depends on five symbols at once, which the automaton below does not follow.
// On a line that holds every byte (bytes256) a unit is a file already, and is read only as its
// own bytes, as the vault reads it.
//
// Counting: each reading but `binary` is walked symbol by symbol through a small machine (the
// decoder's own state, and the signature matcher's: which signatures its first bytes still fit),
// done as soon as the signature is decided, after at most 14 decoded bytes. Their states over the
// line's symbols make a deterministic automaton per reading, and their union O is minimised
// (sieve/dfa.hpp). The binary reading cannot be one small automaton: it must remember which two
// symbols it has seen as well as the bits so far. But which two they are changes nothing but the
// symbols it will accept next, so it is walked over what a symbol is to it (the first symbol, the
// other, or whitespace) with each class's number of symbols, an automaton Bn of a few thousand
// states. On an ASCII alphabet (one byte a symbol) the units a stack keeps are then counted
// exactly by inclusion and exclusion, with P the units the stack's plugins keep (every unit when
// there are none):
//     kept = |P| - |P and O| - |P and Bn| + |P and O and Bn|
// The last two are walked with the two symbols named, and only while both automata are alive
// (few units of two symbols are anything under another reading), up to a budget. On a larger
// alphabet (UTF-8 of two or more bytes a symbol) with the binary reading chosen, or past the
// budget, the filter judges only; the verdicts are the same.
#pragma once

#include "sieve/alphabet.hpp"
#include "sieve/dfa.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kWrittenVersion = "not-written-v1";
inline constexpr size_t kWrittenMaxStates = 400000;

// The readings, in the order they are tried; a mask has bit i for reading i.
const std::vector<std::string>& written_decoders(); // text hex base64 base32 decimal nibbles spelled binary
uint32_t written_mask_of(const std::string& name); // "all" or one reading; throws on anything else

struct WrittenReading
{
    std::string decoder, kind; // e.g. "nibbles", "PNG"
};

// The first reading (of those in `mask`) under which the text is a file with a signature, found
// by decoding it outright. For a line of every byte, pass the unit's bytes instead (as_bytes).
std::optional<WrittenReading> written_as(const std::u32string& text, uint32_t mask);
std::optional<WrittenReading> written_as_bytes(std::span<const uint32_t> bytes);

// The rule for one alphabet and set of readings, made once and kept for the process.
class WrittenRule;
std::shared_ptr<const WrittenRule> written_rule(const Alphabet& a, uint32_t mask);
// Whether a unit (digits of the rule's alphabet) is written out, walked through the automata (the
// same verdict as written_as, found the other way).
bool written_by_rule(const WrittenRule& rule, std::span<const uint32_t> unit);
// Why the rule cannot count ("" when it can).
std::string written_blocker(const WrittenRule& rule);
// Its automata's sizes, in words ("2590 states (readings but binary), binary 1234 states").
std::string written_summary(const WrittenRule& rule);

// A ranker for the units that pass (not written out) and that `keep` accepts (the stack's
// plugins' automaton, minimal; nullptr: every unit), at one length. Null, with `why` said, when
// the rule cannot count or the tables would pass the budget.
// `need`, if given: the memory its tables take, or would (what the filter memory must hold).
std::unique_ptr<Ranker> written_ranker(std::shared_ptr<const WrittenRule> rule, const Dfa* keep, uint32_t length, std::string& why, double* need = nullptr);

// A not-written-v1 filter's rule (for a stack that counts it with its plugins), or nullptr.
std::shared_ptr<const WrittenRule> written_rule_of(const Filter& f);

// The union of the readings in `mask` other than binary (what the rule calls O), over the
// alphabet's symbols; nullopt when it would pass `max_states` states before minimising.
std::optional<Dfa> written_dfa(const Alphabet& a, uint32_t mask, size_t max_states = kWrittenMaxStates);

// ---- not-other-line-v1 (text): a page that is another line's content written as text

// Forms: "notes" (melody notation, as canon-notes-v1 and -v2 read it: notes A-G with # or b and an
// octave digit, rests R, durations s e e. q q. h h. w or none, // between voices, separated by
// whitespace, | or ,; at least one note or rest) and "obj" (a model's .obj text: v lines of three
// numbers and f lines of three indices, apart by line feeds, at least one of each, padding spaces
// only at the end). Mask bit 0 notes, bit 1 obj.
const std::vector<std::string>& other_line_forms();
uint32_t other_line_mask_of(const std::string& name); // "all", "notes" or "obj"
// The form the text is, decided outright (not by the automaton), or nullopt.
std::optional<std::string> other_line_as(const std::u32string& text, uint32_t mask);
// The automaton of the texts in those forms, over the alphabet's symbols.
std::optional<Dfa> other_line_dfa(const Alphabet& a, uint32_t mask, size_t max_states = kWrittenMaxStates);

// ---- not-packed-v1 (image, video): a unit whose symbols, `bits` each, packed into bytes are a
// file with a signature

std::optional<std::string> packed_as(std::span<const uint32_t> digits, uint32_t bits);
Dfa packed_dfa(uint32_t base, uint32_t bits);

} // namespace sieve
