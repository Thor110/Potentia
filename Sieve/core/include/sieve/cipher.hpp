// Sieve — the Encryption Engine's ciphers, stage 1 (`cipher-v1`, SPECIFICATIONS §12.0i).
//
// A recipe is a list of layers applied to bytes in order; decrypting undoes them in reverse. The
// layers are the historical weak ciphers, which can be recognised and broken (and are here, for
// study), and one format-preserving layer: the binary line's own keyed Feistel scramble, which
// maps a file of n bytes onto another file of n bytes, so a key turns the line into a cipher of its
// own shape. None of them is a reviewed cipher for protecting secrets (IDEAS §19.34, §19.52).
//
//   reverse          the bytes in reverse order
//   caesar N         each byte plus N, mod 256 (N 0 to 255)
//   xor HEX          each byte XOR the key's bytes in turn (HEX: one byte or more)
//   vigenere TEXT    each byte plus the key's bytes in turn, mod 256 (TEXT's UTF-8 bytes)
//   feistel TEXT     the binary line's feistel-sha256-v1 scramble of the n bytes, with key TEXT
//   vigenere-hex HEX, feistel-hex HEX   the same with the key's bytes in hex (written so when the
//                    key is not printable ASCII, or holds ';' or '#')
//
// A recipe's text is one layer a line, or layers apart by ';' (TEXT then cannot hold ';'), after
// an optional first line "sieve-cipher-v1".
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::cipher {

inline constexpr const char* kCipherVersion = "cipher-v1";

struct Layer
{
    enum class Kind { Reverse, Caesar, Xor, Vigenere, Feistel } kind = Kind::Reverse;
    std::vector<uint8_t> key; // caesar: one byte; xor and vigenere: the key's bytes; feistel: the key text's bytes
    std::string text() const; // as a recipe line
};

struct Recipe
{
    std::vector<Layer> layers;
    // Throws std::invalid_argument naming what is wrong.
    static Recipe parse(std::string_view text);
    std::string text() const; // "sieve-cipher-v1", then a layer a line
};

std::vector<uint8_t> encrypt(const Recipe& r, std::vector<uint8_t> bytes);
std::vector<uint8_t> decrypt(const Recipe& r, std::vector<uint8_t> bytes);

// How English-like bytes are: the sum of a weight for each byte (letters by their frequency in
// English, the space highest, other printable characters a little, anything else a penalty).
int64_t english_score(const std::vector<uint8_t>& bytes);

// A guess at a one-layer weak recipe that made `cipher`, with what it decrypts to, its English
// score, and its rank: the score less kKeyByteCost for each key byte beyond the first, so that a
// longer key must earn its place (one fitted column by column can always score a little higher).
inline constexpr int64_t kKeyByteCost = 1500;
struct Guess
{
    Recipe recipe;
    std::vector<uint8_t> plain;
    int64_t score = 0, rank = 0;
};
// Every caesar shift, every one-byte XOR, and for each key length 2 to max_key (with at least
// eight bytes a column) the best repeating XOR and Vigenère key, column by column, leaving out a
// key that repeats a shorter one; the `top` best guesses by rank, the higher first (on a tie, in
// that order).
std::vector<Guess> break_weak(const std::vector<uint8_t>& cipher, size_t max_key = 16, size_t top = 5);

} // namespace sieve::cipher
