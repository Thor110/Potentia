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
const std::vector<FileSignature>& file_signatures();

// A set of kinds: one flag per entry of file_kinds().
using KindSet = std::vector<uint8_t>;
// The kinds a binary-kind-v1 setting names ("signed", "text", "unknown", "empty",
// "signed-or-text", "any", or one kind in lower case: "png", "7z", ... "txt"). Throws on anything else.
KindSet kind_set_of(const std::string& name);
// The setting's choices, in the order the menu offers them.
const std::vector<std::string>& kind_set_names();

// The files of 0..max_bytes bytes whose kind is in a set: counted, ranked and unranked exactly, in
// the binary line's positional order (see above).
class KindCounter
{
public:
    KindCounter(uint64_t max_bytes, KindSet kinds);

    uint64_t max_bytes() const { return max_bytes_; }
    const BigUint& count() const { return count_; }
    BigUint count_of_length(uint64_t length) const;
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
    BigUint shorter_than(uint64_t length) const; // survivors shorter than `length` bytes
    // Completions of a pattern from each head state after p bytes (p <= min(length, 16)).
    std::vector<std::vector<BigUint>> pattern_table(const Pattern& pat) const;
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
    bool needs_file() const { return !item_forms_.empty(); }
    // Whether the survivors can be counted and ranked (not when a form is judged file by file).
    bool can_rank() const { return can_rank_; }
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
    BigUint count_, pages_in_;             // survivors; pages among the kind survivors
    bool can_rank_ = true, exclude_pages_ = false;
    std::string blocker_;
    std::unique_ptr<Shuffle> shuffle_;
    size_t hex_width_ = 1;
    std::string provenance_, id_;
};

} // namespace sieve
