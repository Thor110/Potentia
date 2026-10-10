// Sieve — a language model's tokenizer: text to tokens and back, read from the model's own
// tokenizer.json (IDEAS §15; SPECIFICATIONS §12.0c). The first piece of the AI dimension.
//
// The kind covered: byte-level BPE, as GPT-2 made it and SmolLM2 uses it.
//   1. Added tokens (<|im_start|>, <|im_end|>, ...) are found in the text first, when asked for
//      (the earliest, the longest of those starting there), and stand as themselves.
//   2. The rest is split into pieces: with a "Digits" pre-tokenizer (individual_digits), every
//      number character (Unicode N) on its own; then by the GPT-2 pattern
//          's|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+
//      tried in that order at each place (Unicode letters, numbers and White_Space as
//      sieve/unicode_classes.hpp has them).
//   3. Each piece's UTF-8 bytes are written as GPT-2's byte characters (every byte a printable
//      character: the printable ASCII and Latin-1 ones as themselves, the rest from U+0100 on),
//      and merged pair by pair, the pair with the lowest merge rank first (the leftmost, on a
//      tie), until no pair in the merge list is left.
//   4. Each resulting symbol is a token of the vocabulary. A byte whose character is not a token
//      (SmolLM2 has none for 21 of the 256: control characters and bytes UTF-8 never uses) is
//      left out, as the library the tokenizer comes from does when there is no unknown token.
// Decoding writes each token's characters back as bytes; an added token is its text.
#pragma once

#include "sieve/json.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace sieve::llm {

class Tokenizer
{
public:
    // From a tokenizer.json. Throws std::invalid_argument for a kind it does not cover (another
    // model type, a normalizer, byte fallback, another pre-tokenizer).
    explicit Tokenizer(const json::Value& tokenizer_json);

    // `special`: added tokens written in the text are those tokens (as a chat needs); otherwise
    // they are text like any other.
    std::vector<uint32_t> encode(std::string_view utf8, bool special = true) const;
    std::string decode(const std::vector<uint32_t>& ids) const; // bytes (UTF-8 when the tokens make it so)
    std::string bytes_of(uint32_t id) const;                   // one token's bytes (an added token's text)

    size_t size() const { return tokens_.size(); }                // ids run from 0 to size() - 1
    std::optional<uint32_t> find(std::string_view token) const;   // a token by its written form ("<|im_end|>")
    bool is_added(uint32_t id) const { return id < added_.size() && added_[id]; }
    // The pieces step 2 makes of a text (no added tokens), for tests and for `sieve chat --encode`.
    std::vector<std::string> pieces(std::string_view utf8) const;

private:
    std::vector<std::string> tokens_;                       // id -> written form (byte characters)
    std::unordered_map<std::string, uint32_t> ids_;          // written form -> id
    std::unordered_map<std::string, uint32_t> ranks_;        // merge ("a b") -> rank
    std::vector<bool> added_;
    std::vector<std::pair<std::string, uint32_t>> added_list_; // the added tokens' text, longest first
    bool digits_ = false;                                   // a Digits pre-tokenizer, individual_digits
    std::vector<uint32_t> bpe(const std::string& piece) const;
};

// GPT-2's byte characters: byte b written as one code point, and back.
char32_t byte_char(uint8_t b);
std::optional<uint8_t> char_byte(char32_t c);

} // namespace sieve::llm
