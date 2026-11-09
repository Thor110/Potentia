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

#include <atomic>
#include <bit>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
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
    // v32: the same, each word counted by how common it is: `base` to the power of how
    // many of the `commoner` lists hold it (folded as the list is). With scowl-en-80, and scowl-en-35
    // and scowl-en-60 as the commoner lists at 64: 4,096 a word of en-35, 64 one of en-60 only, 1 the
    // rest (mix-v4's "F80"). Refused where a node's counts would pass 2^32.
    MixDictionary(std::span<const uint8_t> list, const std::vector<std::span<const uint8_t>>& commoner, uint32_t base);
    // A node: its counts summed in byte order (the end, as a space 0x20, then a to z), and its
    // children, which are next to one another: those a to z it has (a bit each), from `first`.
    struct Node
    {
        uint32_t pre[28];
        uint32_t letters = 0; // bit c: it has a child by 'a' + c
        int32_t first = -1;   // the child of its lowest letter
    };
    const std::vector<Node>& nodes() const { return nodes_; }
    // Node n's child by 'a' + c (c 0 to 25), or -1 where it has none.
    int32_t child(int32_t n, int c) const
    {
        const Node& nd = nodes_[size_t(n)];
        if (!(nd.letters >> c & 1)) return -1;
        return nd.first + int32_t(std::popcount(nd.letters & ((uint32_t(1) << c) - 1)));
    }
    uint64_t words() const { return words_; }

private:
    void build(std::vector<std::pair<std::string, uint32_t>>& words);
    std::vector<Node> nodes_;
    uint64_t words_ = 0;
};

// v32: with a list's size, asks mix_words-style hooks for that list counted by how
// common each word is (mix-v4's "F80"), not the plain list.
inline constexpr uint32_t kMixWordsByUse = 0x100;

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

// mix-v4 (27 October 2026, SPECIFICATIONS §12.2): mix-v2 and mix-v3 in one, without the two SSE
// stages, which Edward retired (sse-v1): the mixers' average is what is coded. Text ("B22"), a
// picture ("B22 I949x1") or text with a word list ("B22 D60"). Measured: 9% to 29% faster to decode
// than with them, 0.07% (a photograph) to 2.3% (a screenshot) larger (mix.cpp says more).
inline constexpr const char* kMix4Version = "mix-v4";

struct Mix4Params
{
    uint32_t table_bits = 22;
    uint32_t width = 0, channels = 1; // a picture's shape, as mix-v2's (0: not a picture)
    uint32_t dictionary = 0;          // a word list, as mix-v3's (0: none; else 35, 60 or 80)
    bool by_use = false;              // v32: that list counted by how common each word is ("F80": 80 only)
    // v33 draft (2 November 2026): the match model's ways, each off as v32 makes it. R1: a match goes
    // on past one wrong byte at the same distance; M4: a match taken after 4 bytes, not 6; C2: two
    // places a context was seen, the longer match kept.
    bool recover = false;
    uint32_t min_match = 6;
    uint32_t candidates = 1;
    // v33 draft (2 November 2026): X, the file's bytes through the x86 filter (x86_filter) before
    // they are mixed, and back through it after: a program's calls and jumps then name the places
    // they go, not how far away those are, so the same call is the same bytes wherever it is.
    bool x86 = false;
    // v33 draft (2 November 2026): P<n>, a picture of `channels` bytes a pixel and at most 256 colours
    // mixed as its palette's indices (one byte a pixel, a picture of the same width), then the
    // palette itself, n entries of `channels` bytes each: the colours in the order the picture first
    // shows them, entries past its last colour all zero (palette_split). 0: none.
    uint32_t palette = 0;
    // v33 (reopened 3 November 2026): W<n>, the model primed on registered dictionary n before the file:
    // the dictionary's bytes read through it, every bit predicted and learnt as the file's are, none
    // coded, so the file starts with what the dictionary taught it. 0: none; 1: web-mdn-2026-11-03-64k
    // (MDN's CC0 examples, HTML, CSS and JavaScript; mix_primer). Text only, not a picture or a program.
    uint32_t primer = 0;
    // v34 draft (7 November 2026): E1, the x86 instruction model: in place of the three contexts
    // made for text's lines and fields, three from where each byte falls in its x86 instruction (its
    // part, its opcode, the ModRM and SIB bytes, the instruction before's opcode). A program's bytes.
    bool instructions = false;
    // The list the hooks are asked for: its size, with kMixWordsByUse where it is counted by use.
    uint32_t list() const { return dictionary | (by_use ? kMixWordsByUse : 0); }
    void check() const;
    std::string text() const;
    static Mix4Params parse(std::string_view text);
    static Mix4Params for_size(uint64_t bytes, uint32_t width = 0, uint32_t channels = 1, uint32_t dictionary = 0, bool by_use = false);
    bool operator==(const Mix4Params&) const = default;
};

// Whether mix-v4's parameters name any of the match model's ways or the x86 filter (v33 draft).
// (And the palette, P, and a primer, W: all v33's.)
inline bool match_ways(const Mix4Params& p) { return p.recover || p.min_match != 6 || p.candidates != 1 || p.x86 || p.palette || p.primer || p.instructions; }

// v33 (W<n>): the dictionaries mixing may be primed on, each by its number. The bytes are asked of a
// hook the program sets (the CLI's reads the registered file and checks its SHA-256); mix4_code and
// mix4_file refuse parameters naming a primer where none is set, or where the hook refuses.
inline constexpr uint32_t kMixPrimers = 1;
inline constexpr const char* kMixPrimerFile[kMixPrimers + 1] = {"", "web-mdn-2026-11-03-64k.bin"};
inline constexpr const char* kMixPrimerSha256[kMixPrimers + 1] = {"", "3902a57215dcd52f0dbc585c53cad608431d1808ecfbdebf8a738ef83df7fa0b"};
using MixPrimer = std::function<std::span<const uint8_t>(uint32_t)>;
void set_mix_primer(MixPrimer primer);
std::span<const uint8_t> mix_primer(uint32_t n); // refused where no hook is set

// The x86 filter (BCJ), as the LZMA SDK's and xz's --x86 make it, starting at 0: each E8 or E9 byte
// (a call or a jump) whose four bytes after it read as a near distance has them made the place it
// goes (encoding) or made a distance again (decoding). The same length in and out; in place.
void x86_filter(std::span<uint8_t> data, bool encoding);

// v33 draft (P): a picture's pixels of `channels` bytes as their indices into a palette of `colours`
// entries, then the palette (the colours as first seen, the rest zero); none where the pixels hold
// more colours than that or are not whole pixels. And back: refused where an index is past the
// palette or the bytes are not that shape.
std::optional<std::vector<uint8_t>> palette_split(std::span<const uint8_t> pixels, uint32_t channels, uint32_t colours);
std::vector<uint8_t> palette_join(std::span<const uint8_t> split, uint32_t channels, uint32_t colours);
// How many colours a picture's pixels hold, counted up to 257 (more: 257); 0 where not whole pixels.
uint32_t colours_of(std::span<const uint8_t> pixels, uint32_t channels);

// `words`: the word list the parameters name (nullptr where they name none).
std::vector<uint8_t> mix4_code(std::span<const uint8_t> file, const Mix4Params& p, const MixDictionary* words = nullptr);
std::vector<uint8_t> mix4_file(std::span<const uint8_t> code, const Mix4Params& p, const MixDictionary* words, uint64_t size);

// Prioritise Decoding Speed (--prioritise-decoding, the File Locator's box, [locator]
// prioritise_decoding): the packer carries no file by mixing, whose decoding (0.3 to 1 MB/s,
// measured 27 October 2026) is 15 to 60 times slower than the rope's (15 to 20 MB/s) or LZMA2's;
// of the other ways, the smallest. For the whole run.
void set_prioritise_decoding(bool on);
bool prioritise_decoding();

// What the packer weighs a file's ways by (7 November 2026, Edward: "a middle ground option ...
// Balance Size and Decoding speed"; --priority, the File Locator's choice, [locator] priority):
//   size      the smallest, whatever it costs to install (mixing's rising bar over 16 MiB aside);
//   balanced  a way that decodes slowly (mixing) only where it saves at least kBalancedShare of the
//             bytes it carries over the quickest of the others: mixing installs at about 1.2 s a MB
//             (measured on 23 files and folders, results/balanced-2026-11-07.md), so 4% is about
//             33 KB saved for each second it adds; the Size setting's gain kept where it pays most;
//   speed     Prioritise Decoding Speed: no file mixed.
// Each chooses among the ways the format has: none changes what a reader reads.
enum class Priority
{
    size,
    balanced,
    speed
};
inline constexpr uint32_t kBalancedShare = 4; // (per cent of the bytes carried)
void set_priority(Priority p);
Priority priority();
const char* priority_name(Priority p);               // "size", "balanced", "speed"
bool priority_from_name(std::string_view s, Priority& p); // false where it is none of them
// Under balanced, the bytes a slow way must save over the quickest for `bytes` carried; 0 else.
inline uint64_t balanced_floor(uint64_t bytes) { return priority() == Priority::balanced ? bytes * kBalancedShare / 100 : 0; }

// An install cancelled while mixing decodes (3 November 2026, Edward: the File Locator's Stop while
// installing). Mixing decodes at 0.3 to 1 MB/s, so a large file can take minutes; for the length of
// a DecodeCancelScope, every mixing decode, on any thread, looks at the flag about every 64 KiB it
// makes and throws DecodeCancelled once it is set. One install at a time sets it (the hallway's);
// nothing else does, so packing's own decoding (its read-back) never sees it.
struct DecodeCancelled : std::runtime_error
{
    DecodeCancelled() : std::runtime_error("cancelled") {}
};
class DecodeCancelScope
{
public:
    explicit DecodeCancelScope(const std::atomic<bool>* flag);
    ~DecodeCancelScope();
    DecodeCancelScope(const DecodeCancelScope&) = delete;
    DecodeCancelScope& operator=(const DecodeCancelScope&) = delete;

private:
    const std::atomic<bool>* flag_;
};
bool decoding_cancelled();

std::vector<uint8_t> mix2_code(std::span<const uint8_t> file, const Mix2Params& p);
std::vector<uint8_t> mix2_file(std::span<const uint8_t> code, const Mix2Params& p, uint64_t size);

} // namespace sieve
