// Sieve — mixing (`mix-v1`, IDEAS §19.102, SPECIFICATIONS §12.2): a file coded bit by bit, every bit
// predicted by several models at once and their predictions mixed by weights that learn as the file
// goes, the mixed prediction then coded exactly by a binary arithmetic coder. The address is the
// code, an exact integer; every step is integer arithmetic, so the same on every machine.
//
// The models (each a table of counters, found by a hash of its context and the bits of the byte so
// far): the bytes before, 0 to 6 of them (orders 0 to 6), and the word so far (its letters, case
// aside). A counter holds a probability (16 bits) and how often it has been seen (to 255): it moves
// a 1/(n + 2) of the way to each bit seen, so it learns fast at first and settles. The mixer: the
// models' probabilities stretched (ln(p / (1 - p)), in a fixed table of 4,096), weighted (16.16
// fixed point, one set of weights for each partial byte), summed and squashed back; each weight then
// moves by its input times the error, times 2, divided by 2^12, held within +-64. The coder: 32-bit
// ranges, a byte out whenever the top bytes agree (lpaq's). The one parameter, B, the tables' size (2^B counters
// each, 16 to 22), is chosen by the file's size and written with the way ("B22").
//
// Measured (26 October 2026): Don Quixote (pg996.txt, 2,391,678 bytes), 565,599 bytes, LZMA2 (one
// 7z stream) 685,794: 17.5% smaller; about 0.4 MB/s, coding and decoding alike.
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kMixVersion = "mix-v1";

struct MixParams
{
    uint32_t table_bits = 22; // B: 16 to 22
    void check() const;       // throws std::invalid_argument out of range
    std::string text() const; // "B22"
    static MixParams parse(std::string_view text);
    // The size for a file: 2^B counters at least twice its bytes, from 2^16 to 2^22.
    static MixParams for_size(uint64_t bytes);
    bool operator==(const MixParams&) const = default;
};

// The code for a file, and the file back from its code (its size given). mix_file throws
// std::invalid_argument on a code that ends before the file does or holds more than it.
std::vector<uint8_t> mix_code(std::span<const uint8_t> file, const MixParams& p);
std::vector<uint8_t> mix_file(std::span<const uint8_t> code, const MixParams& p, uint64_t size);

// mix-v2 (27 October 2026, IDEAS §19.102, SPECIFICATIONS §12.2): mix-v1 grown, measured on the
// repository's own files. More models: a match model (the longest earlier stretch that the last six
// bytes repeat, and the byte it expects next), and, for text and tables, the column (the byte above
// in the line before, the field) and sparse contexts (bytes before, skipping one); or, for a picture
// (its width and bytes per pixel given), Edward's triangle and its neighbours as contexts in place of
// those. Counters found in slots of 16 a half-byte (two places in memory a byte, not eight), two
// mixers (by the partial byte; by the match's length, the bit's place and the byte before), averaged,
// then refined by two SSE stages. Measured (27 October 2026): Don Quixote 532,168 bytes (mix-v1
// 565,596; LZMA2 685,794); the Vitruvian picture as raw grey pixels 727,842 (rope-wave-v6 762,766);
// a 1280 by 720 screenshot's pixels 62,925 (LZMA2 93,936). About as fast as mix-v1; about 240 MB at B22.
inline constexpr const char* kMix2Version = "mix-v2";

struct Mix2Params
{
    uint32_t table_bits = 22; // B: 16 to 22
    uint32_t width = 0;       // a picture's width in pixels, 2 to 2^24; 0: not a picture
    uint32_t channels = 1;    // its bytes per pixel, 1 to 4 (1 where it is not a picture)
    void check() const;       // throws std::invalid_argument out of range
    std::string text() const; // "B22", or a picture's "B22 I949x1"
    static Mix2Params parse(std::string_view text);
    // The size for a file (as mix-v1's), and a picture's shape where it is one.
    static Mix2Params for_size(uint64_t bytes, uint32_t width = 0, uint32_t channels = 1);
    bool operator==(const Mix2Params&) const = default;
};

// mix-v3 (27 October 2026, IDEAS §19.102, SPECIFICATIONS §12.2): mix-v2 for text with one model
// more, a word list's: from the word so far, the letters the list has next (and the word's end), each
// by how many of its words go on so, give every bit a probability; it is an input of its own and its
// place in the word a context. Edward's grammar as a prediction, not a rule: a word not in the list
// costs a little, never fails. The list is one of Sieve's registered dictionaries, pinned by its
// SHA-256 (D60: scowl-en-60), which every reader has. Measured (27 October 2026): Don Quixote's first
// 800 KB 187,650 bytes (mix-v2 191,190), a 3 KB text 1,212 (1,303), a word list 27,138 (41,770); on a
// PDF 0.45% larger, so the packer weighs both.
inline constexpr const char* kMix3Version = "mix-v3";

// A word list made ready: its lines (a carriage return at the end left out), A to Z folded to a to z,
// those of letters only, as a trie with each node's count of words going on by each letter or ending.
class MixDictionary
{
public:
    explicit MixDictionary(std::span<const uint8_t> list);
    struct Node
    {
        int32_t child[26];
        uint32_t pre[28]; // the counts summed in byte order: the end (as a space, 0x20), then a to z
    };
    const std::vector<Node>& nodes() const { return nodes_; }
    uint64_t words() const { return words_; }

private:
    std::vector<Node> nodes_;
    uint64_t words_ = 0;
};

struct Mix3Params
{
    uint32_t table_bits = 22; // B: 16 to 22
    uint32_t dictionary = 60; // D: the registered SCOWL list of this size, 35, 60 or 80
    void check() const;
    std::string text() const; // "B22 D60"
    static Mix3Params parse(std::string_view text);
    static Mix3Params for_size(uint64_t bytes, uint32_t dictionary = 60);
    bool operator==(const Mix3Params&) const = default;
};

std::vector<uint8_t> mix3_code(std::span<const uint8_t> file, const Mix3Params& p, const MixDictionary& words);
std::vector<uint8_t> mix3_file(std::span<const uint8_t> code, const Mix3Params& p, const MixDictionary& words, uint64_t size);

// Prioritise Decoding Speed (--prioritise-decoding, the File Locator's box, [locator]
// prioritise_decoding): the packer carries no file by mixing, whose decoding (0.3 to 1 MB/s,
// measured 27 October 2026) is 15 to 60 times slower than the rope's (15 to 20 MB/s) or LZMA2's;
// of the other ways, the smallest. For the whole run.
void set_prioritise_decoding(bool on);
bool prioritise_decoding();

std::vector<uint8_t> mix2_code(std::span<const uint8_t> file, const Mix2Params& p);
std::vector<uint8_t> mix2_file(std::span<const uint8_t> code, const Mix2Params& p, uint64_t size);

} // namespace sieve
