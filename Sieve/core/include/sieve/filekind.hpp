// Sieve — what kind of file a byte string is, read from its own first bytes ("file-kinds-v1"),
// and the binary line's filter by kind ("binary-kind-v1"), counted and ranked exactly.
//
// A file's kind is decided by its first 16 bytes (fewer if it is shorter) and its size, in this
// order, the first that applies:
//   EMPTY     no bytes at all
//   a signature, the first in the table below whose bytes all lie within the file:
//     PNG 89 50 4E 47 0D 0A 1A 0A      7Z "7z" BC AF 27 1C          XZ FD "7zXZ" 00
//     MANIFEST "sieve-manifest"         BOOK "sieve-book"             GIF "GIF87a" or "GIF89a"
//     WAV "RIFF" and "WAVE" at 8        AVI "RIFF" and "AVI " at 8    WEBP "RIFF" and "WEBP" at 8
//     MP4 "ftyp" at 4                   ZIP "PK" 03 04 or "PK" 05 06  ELF 7F "ELF"
//     PDF "%PDF"   RAR "Rar!"   OGG "OggS"   FLAC "fLaC"   MID "MThd"   JPG FF D8 FF
//     MP3 "ID3"    BZ2 "BZh"    GZ 1F 8B     EXE "MZ"      BMP "BM"
//   TXT       every one of the first bytes readable: 20-7E, 80-FF (UTF-8's bytes above 127), or
//             tab, line feed or carriage return
//   ?         anything else
// The hallway's label on a file (display only) is this; MID is new in v1 of the table, which is
// otherwise the hallway's list as it was. A signature's kind is a claim the file makes about
// itself, not a check that the rest of it is well formed.
//
// binary-kind-v1 keeps (or excludes) the files of chosen kinds on the binary line (binary-v1,
// sieve/binaryspace.hpp). Because a kind depends only on the first 16 bytes and the size, the
// survivors can be counted and ranked exactly however long the line is:
//   a head of h <= 16 bytes is walked byte by byte through a small automaton whose state is which
//   signatures are still possible and whether every byte so far is readable; from it, for every h,
//   the number of heads of h bytes whose kind is chosen. A file of L <= 16 bytes is its head; a
//   file of L > 16 bytes is a head of 16 and any L - 16 bytes after it, so
//       survivors of length L = heads(L)                    for L <= 16
//                             = heads(16) * 256^(L - 16)    for L > 16
// and the survivors are numbered as the line numbers files: shortest first, then by their bytes
// read as one big-endian number. A survivor's rank is the survivors shorter than it, plus the
// chosen heads of its length below its own head times 256^(L - 16), plus its bytes after the head
// read as a number.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/compact.hpp"
#include "sieve/filter.hpp"

#include <array>
#include <functional>
#include <optional>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kFileKindsVersion = "file-kinds-v1";
inline constexpr size_t kKindHead = 16; // the bytes a kind is read from

// Every kind, in the order they are listed: EMPTY, the signatures' kinds in table order, TXT, "?".
const std::vector<std::string>& file_kinds();
// The kind of a file whose first bytes are `head` (at least min(16, size) of them; any more are
// ignored) and whose length is `size`.
std::string file_kind(std::span<const uint8_t> head, uint64_t size);
// Whether a kind is one a signature gives (not EMPTY, TXT or "?").
bool is_signed_kind(std::string_view kind);
// The kind of the file at binary-v1 positional index `index`, from its head alone (the rest of the
// file is never written out, however long it is).
std::string file_kind_at(const BigUint& index);

// The signature table, for the head automaton and the not-written filter (sieve/written.hpp):
// entry i fixes some of the first bytes (fixed[p] >= 0 is the byte at p, else -1), and is matched
// once all of them lie within the file (end = one past the last fixed byte).
struct FileSignature
{
    std::string kind;
    std::array<int16_t, kKindHead> fixed;
    uint32_t end = 0;
};
// The table, worked out once from the kinds' signatures.
const std::vector<FileSignature>& file_signatures();

// A set of kinds: one flag per entry of file_kinds().
using KindSet = std::vector<uint8_t>;
// The kinds a binary-kind-v1 setting names ("signed", "text", "unknown", "empty",
// "signed-or-text", "any", or one kind in lower case: "png", "7z", ... "txt"). Throws on anything else.
KindSet kind_set_of(const std::string& name);
// The setting's choices, in the order the menu offers them.
const std::vector<std::string>& kind_set_names();

// A positive number as a double and a power of two: an estimate (about 15 significant figures) of a
// count far too long to hold exactly, such as the well-formed UTF-8 files of megabytes. Only sums
// and products of counts, so nothing cancels and the figures hold.
struct Scaled
{
    double m = 0;  // in [0.5, 1), or 0
    int64_t e = 0; // the value is m * 2^e
    Scaled() = default;
    explicit Scaled(double v); // zero for v <= 0
    static Scaled of(const BigUint& v);
    Scaled& operator+=(const Scaled& o);
    Scaled& operator-=(const Scaled& o); // o <= this (a part taken from its whole)
    Scaled& operator*=(const Scaled& o);
    void mul_small(uint64_t n) { *this *= Scaled(double(n)); }
    bool is_zero() const { return m == 0; }
    double log10() const; // -infinity for zero
};

// The files of 0..max_bytes bytes whose kind is in a set: counted, ranked and unranked exactly, in
// the binary line's positional order (see above).
class KindCounter
{
public:
    KindCounter(uint64_t max_bytes, KindSet kinds);

    uint64_t max_bytes() const { return max_bytes_; }
    const BigUint& count() const { return count_; }
    BigUint count_of_length(uint64_t length) const; // the files of exactly this many bytes in the set
    bool passes(std::span<const uint8_t> head, uint64_t size) const;

    BigUint rank(const std::vector<uint8_t>& file) const;   // the file must pass
    std::vector<uint8_t> unrank(const BigUint& k) const;    // k < count()
    // How many of the chosen files have a binary-v1 positional index below `index` (any index:
    // past the line, all of them). Its head is worked out, not the whole file.
    BigUint count_before(const BigUint& index) const;

    // A fixed-length set of files given byte by byte (`allowed[p]`: the bytes position p may
    // hold): the pages of a text line, as their files. How many of them are of a chosen kind, and
    // how many of those come before a file in the line's order.
    struct Pattern
    {
        std::vector<std::array<bool, 256>> allowed;
    };
    BigUint pattern_count(const Pattern& pat) const;
    BigUint pattern_below(const Pattern& pat, const std::vector<uint8_t>& file) const;
    static bool pattern_has(const Pattern& pat, const std::vector<uint8_t>& file);
    // The same with the pattern's table worked out once (pattern_table) and kept by the caller: a
    // table is every head state at every level, so building it for each call is what is slow.
    using PatternTable = std::vector<std::vector<BigUint>>;
    PatternTable pattern_table(const Pattern& pat) const; // completions from each head state after p bytes
    BigUint pattern_below(const Pattern& pat, const PatternTable& table, const std::vector<uint8_t>& file) const;

private:
    // The head automaton, level by level: states_[p] are the states after p bytes (0..16), each a
    // (signatures still possible, readable so far) pair; next_[p][s * 256 + b] the state after b.
    struct HeadState
    {
        uint32_t mask = 0;
        bool text = true;
    };
    std::array<std::vector<HeadState>, kKindHead + 1> states_;
    std::array<std::vector<uint32_t>, kKindHead> next_;
    // comp_[h][p][s]: heads of exactly h bytes, through state s after p of them, whose kind is chosen.
    std::array<std::array<std::vector<BigUint>, kKindHead + 1>, kKindHead + 1> comp_;
    KindSet kinds_;
    uint64_t max_bytes_;
    BigUint count_, short_total_; // all survivors; those of 16 bytes or fewer
    int kind_of(uint32_t h, uint32_t s) const; // index into file_kinds() of a head of h bytes ending in s

public:
    BigUint shorter_than(uint64_t length) const; // survivors shorter than `length` bytes

    // With utf8-valid-v1 (Utf8Counter): the files whose kind is chosen and that are well-formed
    // UTF-8. The head automaton and UTF-8's are walked side by side over the first 16 bytes, where
    // the kind is decided; after that only UTF-8's matters. `tail[u]`: the ways to finish a file
    // from UTF-8 state u with 1 to max_bytes - 16 more bytes, summed (Utf8Counter::tail_sums, or
    // tail_estimate). With a pattern, only its files (of its one length), and `tail` is not used.
    BigUint utf8_count(bool text_only, const std::vector<BigUint>& tail, const Pattern* pat = nullptr) const;
    Scaled utf8_estimate(bool text_only, const std::vector<Scaled>& tail, const Pattern* pat = nullptr) const;

private:
    template <class T>
    T utf8_joint(bool text_only, const std::vector<T>& tail, const Pattern* pat) const;
};

// The other lines' items as the binary line holds them (not-an-item-v1): each a file that is
// exactly an item of another line as F saves it. Pages whose symbols are one byte each (ASCII
// alphabets, and bytes256) are a Pattern and count exactly; the rest are judged file by file by the
// application, which knows the lines' shapes (tools/cli/filter_config.hpp).
struct BinaryItems
{
    std::optional<KindCounter::Pattern> pages; // counted exactly
    // Judges, by the form they recognise: "pages" (when not a pattern), "melodies", "pictures",
    // "models". A form asked for with no judge here leaves the sieve unable to count.
    std::vector<std::pair<std::string, std::function<bool(const std::vector<uint8_t>&)>>> judges;
};

// The binary line's ticked filters (at present binary-kind-v1), with what the other lines' stacks
// give the hallway: a verdict from a file's head and size, the survivors' count, and their compact
// orderings (positional: survivor k at compact address k; scrambled: shuffle-sha256-v1 over the
// survivors, keyed with the line's key, the stack id its domain, as sieve/compact.hpp does).
// utf8-valid-v1 (core/src/filters/binary.cpp): the files of 0..max_bytes bytes that are well-formed
// UTF-8 (RFC 3629: no overlong forms, no surrogates, nothing past U+10FFFF), and with
// `text_only` no control characters but tab, line feed and carriage return (C0, DEL and C1).
// Judged byte by byte through a small automaton; counted length by length from its table of
// completions, and ranked in the binary line's order (shortest first, then by the bytes), so the
// survivors compact. The table holds states x (max_bytes + 1) numbers of up to 8 max_bytes bits,
// so past the filter memory (plugin.hpp filter_memory) it judges only.
class Utf8Counter
{
public:
    Utf8Counter(uint64_t max_bytes, bool text_only);
    // Its table's memory at a line of files up to max_bytes: kStates x (n + 1) numbers of up to 8n bits.
    static double table_bytes(uint64_t max_bytes) { return double(kStates) * double(max_bytes + 1) * double(8 * max_bytes + 64) / 8.0; }

    static bool valid(std::span<const uint8_t> file, bool text_only);
    bool can_rank() const { return can_rank_; }
    const BigUint& count() const { return count_; }
    BigUint rank(const std::vector<uint8_t>& file) const;  // the file must pass
    std::vector<uint8_t> unrank(const BigUint& k) const;   // k < count()

    static constexpr uint32_t kStates = 9;
    static constexpr uint8_t kDead = 0xFF;
    static uint8_t step(uint8_t state, uint8_t byte, bool text_only); // state 0 is a whole character

    // For each state u, the ways to end on a whole character after 1 to `most` more bytes, summed:
    // exactly from the table (can_rank() only), or estimated at any length (a power of a small
    // matrix, so a line of megabytes takes microseconds).
    std::vector<BigUint> tail_sums(uint64_t most) const;
    static std::vector<Scaled> tail_estimate(bool text_only, uint64_t most);

private:
    uint64_t max_bytes_;
    bool text_only_, can_rank_ = true;
    std::vector<std::vector<BigUint>> ways_; // ways_[r][s]: completions from state s with r bytes left
    std::vector<BigUint> before_;            // before_[l]: survivors shorter than l bytes
    BigUint count_;
};

class BinarySieve
{
public:
    BinarySieve() = default;
    // The binary line's ticked filters; `items` describes the other lines for not-an-item-v1.
    BinarySieve(const BinarySpace& space, const std::vector<FilterStack::Entry>& entries, const BinaryItems* items = nullptr);

    bool empty() const { return names_.empty(); }
    size_t size() const { return names_.size(); }
    // The first filter the file fails, or "" if it passes them all. From its head and size, when
    // needs_file() is false; else it needs the whole file (first_failure_of).
    std::string first_failure(std::span<const uint8_t> head, uint64_t size) const;
    std::string first_failure_of(const std::vector<uint8_t>& file) const;
    bool passes(std::span<const uint8_t> head, uint64_t size) const { return first_failure(head, size).empty(); }
    bool needs_file() const { return !item_forms_.empty() || utf8_filter_ >= 0; }
    // Whether the survivors can be counted and ranked (not when a form is judged file by file).
    bool can_rank() const { return can_rank_; }
    // Whether they can at least be counted (for the share a stack filters): exactly, as count_of()
    // gives, or estimated, as survivors_log10() gives. utf8-valid-v1 counts with the kind filters
    // and not-an-item-v1's pages but ranks only on its own, and past its table it is estimated.
    bool can_count() const { return can_count_; }
    bool count_exact() const { return count_exact_; }
    const BigUint& survivors() const { return can_rank_ ? count_ : survivors_; } // count_exact() only
    double survivors_log10() const { return survivors_log10_; }
    // The memory its count's tables need (utf8-valid-v1's; the kinds' are small): what the filter
    // memory must hold for it to count exactly.
    double table_bytes() const { return table_bytes_; }
    std::string blocker() const { return blocker_; }

    const BigUint& count() const { return count_; }
    const KindCounter& counter() const { return *counter_; }
    const std::string& provenance() const { return provenance_; }
    const std::string& id() const { return id_; }

    // The compact orderings (can_rank() only). index_of throws std::invalid_argument when the file
    // is not a survivor.
    BigUint index_of(const std::vector<uint8_t>& file, AddressMode m) const;
    std::vector<uint8_t> file_at(const BigUint& index, AddressMode m) const;
    BigUint rank_of_index(const BigUint& index, AddressMode m) const; // its survivor number
    std::string hex_of(const BigUint& index) const;
    BigUint parse(std::string_view hex) const;

private:
    BigUint rank(const std::vector<uint8_t>& file) const;
    std::vector<uint8_t> unrank(const BigUint& k) const;
    std::vector<std::string> names_;
    std::vector<KindSet> sets_;          // binary-kind-v1 filters, by position in names_
    std::vector<int> item_filter_;       // per filter: -1, or which not-an-item-v1 it is
    std::vector<std::string> item_forms_; // the forms not-an-item-v1 asks for
    std::string item_name_;
    BinaryItems items_;
    std::unique_ptr<KindCounter> counter_; // the survivors of the kind filters
    std::unique_ptr<Utf8Counter> utf8_;    // utf8-valid-v1, when ticked
    int utf8_filter_ = -1;                 // its place in names_
    bool utf8_text_only_ = false;
    BigUint count_, pages_in_;             // survivors; pages among the kind survivors
    KindCounter::PatternTable pages_table_; // the pages' pattern table, worked out once
    bool can_rank_ = true, exclude_pages_ = false;
    bool can_count_ = true, count_exact_ = true;
    BigUint survivors_;            // counted (exactly) though not ranked
    double survivors_log10_ = 0;
    double table_bytes_ = 0;
    std::string blocker_;
    std::unique_ptr<Shuffle> shuffle_;
    size_t hex_width_ = 1;
    std::string provenance_, id_;
};

} // namespace sieve
