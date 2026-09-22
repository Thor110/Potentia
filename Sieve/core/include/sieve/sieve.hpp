// Sieve — M1 exhaustive sieve over the lower27 text line (SPECIFICATIONS §13, M1).
//
// Filters, each strictly tighter than the last (Words ⊆ Window ⊆ Clean):
//   Clean   — at least one letter, and no two consecutive spaces.
//   Window  — Clean, and the unit could be a window cut from running English text:
//             tokens touching neither edge are dictionary words; a token touching only the
//             left edge is a word suffix; only the right edge, a word prefix; both edges,
//             a substring of a word.
//   Words   — Clean, and every token is a complete dictionary word.
//
// Three independent methods compute the same counts, and must agree:
//   Brute   — visits every one of 27^L units and tests each with the filter directly.
//   Pruned  — walks the prefix tree, abandoning any prefix that can no longer pass
//             (blocking off noise regions), and counts the surviving units one by one.
//   Counted — exact dynamic programming over the same rules; never lists units, so it
//             reaches lengths where enumeration is impossible.
// These filters are local (finite-state), which is why exact counting works. Filters built
// on language models are not, and at scale can only be sampled.
#pragma once

#include "sieve/biguint.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace sieve {

enum class SieveFilter { Clean, Window, Words };
const char* to_string(SieveFilter f);

// Hash and word count of a word-list file, without building any lookup structures.
struct DictionaryFileInfo
{
    std::string sha256;
    size_t word_count = 0;    // distinct valid words
    size_t skipped_lines = 0; // non-empty lines containing anything other than a-z
};

class Dictionary
{
public:
    // Reads the file, hashes it and counts words only (fast; used by `sieve dicts`).
    static DictionaryFileInfo inspect_file(const std::string& path);
    // One word per line. Lines are trimmed and ASCII-lowercased; lines containing anything
    // other than a–z are skipped (and counted).
    static Dictionary load_file(const std::string& path);
    static Dictionary from_words(const std::vector<std::string>& words);

    bool is_word(std::string_view t) const { return words_.find(t) != words_.end(); }
    bool is_suffix(std::string_view t) const { return suffixes_.find(t) != suffixes_.end(); }
    bool is_word_prefix(std::string_view t) const;
    bool is_substring(std::string_view t) const;

    // Number of distinct words / word prefixes / word suffixes / word substrings of length k.
    uint32_t words_of_length(uint32_t k) const { return at(words_by_len_, k); }
    uint32_t prefixes_of_length(uint32_t k) const { return at(prefixes_by_len_, k); }
    uint32_t suffixes_of_length(uint32_t k) const { return at(suffixes_by_len_, k); }
    uint32_t substrings_of_length(uint32_t k) const { return at(substrings_by_len_, k); }

    size_t word_count() const { return sorted_words_.size(); }
    size_t skipped_lines() const { return skipped_; }
    const std::string& sha256() const { return sha256_; }

private:
    void build(std::vector<std::string> words);
    static uint32_t at(const std::vector<uint32_t>& v, uint32_t k) { return k < v.size() ? v[k] : 0; }
    std::vector<uint32_t> words_by_len_, prefixes_by_len_, suffixes_by_len_, substrings_by_len_;
    // Transparent hashing: lookups by string_view without building a std::string.
    struct Hash
    {
        using is_transparent = void;
        size_t operator()(std::string_view v) const { return std::hash<std::string_view>{}(v); }
    };
    using Set = std::unordered_set<std::string, Hash, std::equal_to<>>;
    Set words_;
    Set suffixes_;
    std::vector<std::string> sorted_words_;
    std::vector<std::string> sorted_suffixes_;
    size_t skipped_ = 0;
    std::string sha256_;
};

// Direct test of one unit (lower27 characters, i.e. ' ' and 'a'–'z').
bool unit_passes(std::string_view unit, SieveFilter f, const Dictionary& dict);

struct SieveCount
{
    BigUint clean, window, words;
};

struct PrunedResult
{
    uint64_t survivors = 0;
    uint64_t states_explored = 0; // prefix-tree nodes visited, including the root
};

SieveCount sieve_brute(uint32_t length, const Dictionary& dict, unsigned threads);
PrunedResult sieve_pruned(uint32_t length, SieveFilter f, const Dictionary& dict, unsigned threads);
SieveCount sieve_counted(uint32_t length, const Dictionary& dict);

// Total nodes in the full prefix tree of depth L over 27 symbols: 1 + 27 + ... + 27^L.
BigUint prefix_tree_nodes(uint32_t length);

} // namespace sieve
