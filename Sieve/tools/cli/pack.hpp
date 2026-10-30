// Sieve CLI -- an installer's files packed (sieve-manifest-v4, locate.hpp).
//
// Each file's way is chosen, then the two streams are compressed:
//
//   - a program (its kind, from its first bytes, EXE or ELF) goes in the x86 stream: the x86
//     branch filter turns its calls' relative addresses into absolute ones, which repeat, and LZMA2
//     then finds them;
//   - a text whose lines are some of another file's lines, in its order, is derived from it: a mask
//     of that file's lines, one bit each, where the mask packs smaller than the text (each SCOWL
//     list is a mask over the largest). The other file must be carried as it is, and one file
//     derived from is never itself derived;
//   - everything else goes in the lzma2 stream as it is.
//
// Both streams are LZMA2 at its strongest (preset 9, extreme), with a dictionary no larger than the
// stream needs, so unpacking takes no more memory than it must. What is made is read back (unpacked
// and every file compared) before it is used, so a packer fault can never make an installer that
// gives back something else.
#pragma once

#include "cli/locate.hpp"
#include "sieve/rope_wave.hpp"

#include <set>
#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sieve::cli {

struct PackReport
{
    bool kept_v3 = false;                            // packing would not have made it smaller: v3 instead
    size_t x86 = 0, raw = 0, derived = 0;            // files carried each way
    uint64_t x86_bytes = 0, raw_bytes = 0, derived_bytes = 0, mask_bytes = 0;
    uint64_t packed = 0;                             // the two streams as carried
    std::vector<std::pair<std::string, std::string>> derivations; // a derived file, and the file it is made from
    size_t items = 0;                                // v5: files carried as items of Sieve's lines
    uint64_t item_bytes = 0, item_text = 0;          // their bytes, and what naming them takes in the listing
    uint64_t item_places = 0;                        // v6: their places, as raw bytes in the items stream
    uint64_t item_carried = 0;                       // v6: the items stream as carried (raw or LZMA2)
    uint64_t item_raw_places = 0;                    // v7: the places had every line's been raw
    std::vector<std::string> coded_lines;            // v7: the lines whose places were coded
    bool compact = false;                            // v7
    uint64_t listing = 0, listing_carried = 0;       // v7: the listing, and as carried where packed (its line too)
    size_t roped = 0, rope_tried = 0;                // v8: files carried by rope-wave-v1, and the waves tried
    uint64_t rope_bytes = 0, rope_carried = 0;       // v8: their bytes, and the rope stream
    bool rope_columns = false;                       // v9: some of them rope-wave-v2's
    bool rope_lines = false;                         // v10: some of them rope-wave-v3's
    bool rope_groups = false;                        // v11: some of them rope-wave-v4's
    bool trimmed = false;                            // v12: no empty items line, or the files carried as they are
    bool lzma2_as_is = false;                        // v12: the lzma2 stream carried as it is (LZMA2 made it no smaller)
    size_t pieces = 0;                               // v13: files cut into pieces, LZMA2's and the rope's
    uint64_t pieces_lzma2 = 0, pieces_rope = 0;      // v13: their bytes, each way's
    bool pieces_made = false;                        // v13: the instructions are v13
    size_t remade = 0, unpacked = 0;                 // v14: archives remade exactly, and carried as their contents
    bool archives_made = false;                      // v14: the instructions are v14
    bool rope_above = false;                         // v15: some of them rope-wave-v5's
    bool binary = false;                             // v16: the head in bytes
    bool remake_gz = false;                          // v17: some gzip remade exactly
    bool more_archives = false;                      // v19: some bzip2 or zstd remade, or carried as its contents
    bool v20 = false;                    // v20: some lz4, brotli or lzma remade, or one of v20's carried as its contents
    bool v21 = false;                    // v21: some gzip or zip made again by preflate
    bool v22 = false;                    // v22: some RAR to be made again by rar
    bool v23 = false;                    // v23: some rope way rope-wave-v6's (the triangle)
    bool v24 = false;                    // v24: some file carried by mixing (mix-v1)
    bool v25 = false;                    // v25: some PNG made again from its skeleton
    bool v26 = false;                    // v26: some file carried by mix-v2
    bool v27 = false;                    // v27: some file carried by mix-v3
    size_t mixed3 = 0;                   // v27: files carried by mix-v3
    bool v28 = false;                    // v28: some file carried by mix-v4
    bool v29 = false;                    // v29: the lzma2 stream mixed
    uint64_t stream_mixed = 0, stream_lzma2 = 0; // v29: its mixed code's size, and LZMA2's that it replaced
    std::string stream_mixed_params;     // v29: its parameters ("B21", "B21 D60")
    size_t mixed_together = 0;           // v29: files that would be mixed one by one, mixed together in it instead
    bool v30 = false;                    // v30: files mixed together in groups
    bool v31 = false;                    // v31: archives made again from their skeleton, their files carried apart
    bool v32 = false;                    // v32 (draft): some file carried as its differences from another
    size_t deltas = 0;                   // v32: files carried so, their bytes, and their scripts' bytes
    uint64_t delta_bytes = 0, delta_scripts = 0;
    size_t jpegs = 0;                    // v32: JPEGs carried as their brunsli form, their bytes, and the forms'
    uint64_t jpeg_bytes = 0, jpeg_forms = 0;
    size_t member_archives = 0, members = 0; // v31: those archives, and their files
    size_t mix_groups = 0, mix_group_files = 0; // v30: the groups, and the files in them
    uint64_t mix_group_bytes = 0, mix_group_code = 0, mix_group_apart = 0; // v30: their bytes, codes, and what they came to apart
    uint64_t mixed_together_apart = 0;   // v29: what they and the stream came to apart (their codes, and LZMA2's estimate)
    size_t mixed4 = 0, mixed4_pictures = 0, mixed4_words = 0; // v28: files carried by mix-v4, as pictures, with a word list
    size_t mixed2 = 0;                   // v26: files carried by mix-v2 (of `mixed`), as pictures
    size_t mixed2_pictures = 0;
    size_t decoding_first = 0;           // files mixing was not tried on: Prioritise Decoding Speed
    size_t mixed = 0;                    // v24: files carried by mixing
    bool rope_v6 = false;                // v23: some rope way rope-wave-v6's
    uint64_t mix_bytes = 0, mix_carried = 0; // v24: their bytes, and their codes
    size_t rar_remade = 0;               // v22: RARs carried as their contents, the machine's rar making them again exactly
    std::vector<std::string> rar_said;   // v22: for each RAR tried, what came of it
    uint64_t head_text = 0, head_bytes = 0;          // v16: the listing as v15's text, and as carried
};

// v5: for a file's bytes, the line it is an item of (its place in the manifest's `lines`) and its
// positional address there, or nothing (items.hpp makes one from Sieve's lines).
using ItemFinder = std::function<std::optional<std::pair<size_t, std::string>>(const std::vector<uint8_t>&)>;
// v7: an item's place (hex) on line K of m.lines coded (kCodedPlaces), or nothing where the line
// has no coding.
using PlaceCoder = std::function<std::optional<std::vector<uint8_t>>(size_t line, const std::string& place)>;

// Every file's bytes packed after the manifest, making it v4, an installer's. Reads every file again
// (or takes them from a v3 manifest's contents, if it has them) and checks it against the size and
// SHA-256 the walk found, so a file changed in between is refused rather than installed wrong.
// Where packing would not make the installer smaller (a few bytes of text, files already
// compressed), the files are carried as they are instead (v3), so an installer is never larger
// than it need be.
//
// With `items` (and m.lines set to the lines it names), a file that is an item of one of them is
// carried as its line and its place there (v5), made again on installing; the lines no file uses are
// dropped. Its own SHA-256 is left out ("-") unless `item_hashes`, the tree's covering it. With
// m.minimal (and m.line_widths beside m.lines), v6: each place carried as raw bytes, and an item
// taken only where its place is shorter than the file. With m.compact too, v7: a line's places
// coded by `coder` where that is smaller than their raw bytes. With rope_seconds 0 or more (0: no
// limit), v8 where it helps: a file carried by rope-wave-v1, searched for that long, where its code
// is smaller than the file packed alone.
// For the length of a scope (the File Locator's estimate, timing a sample of a large file), the
// main streams are packed with the dictionary a stream of `whole_bytes` would have, as the whole
// will be, so the sample takes nearer as long a byte as the whole does. Its instructions are for
// timing only. One at a time, for the whole program.
struct SampleAsWhole
{
    explicit SampleAsWhole(uint64_t whole_bytes);
    ~SampleAsWhole();
    SampleAsWhole(const SampleAsWhole&) = delete;
    SampleAsWhole& operator=(const SampleAsWhole&) = delete;
};

// The newest level of the packer's ways (the rope's waves, mixing, groups, archives' members): 14,
// sieve-manifest-v31.
// Everything that makes instructions passes it unless an older version is asked for by name.
inline constexpr uint32_t kNewestRope = 14;

// v32 in the making (docs/NEXT-VERSION.md): its ways are made only with the draft switch on
// (`--v32-draft`), so every build writes v31 as before until v32 is frozen. For testing only:
// nothing made with it is to be published, and the finished v32 need not read it.
void set_v32_draft(bool on);
bool v32_draft();

// What Optimise Compressed Formats does with an archive or a compressed file, for the whole run:
// `temporary`, made again exactly on installing, so the program that wants it still finds it (only
// formats Sieve makes again byte for byte); `permanent`, carried as what it holds, installing that
// in its place. Both (the default, and Optimise Compressed Formats before v31): made again where
// Sieve can, else carried as what it holds.
void set_compressed_modes(bool temporary, bool permanent);
bool compressed_temporary();
bool compressed_permanent();

void add_packed(Manifest& m, const std::filesystem::path& root, PackReport* report = nullptr, const ItemFinder* items = nullptr,
                bool item_hashes = false, const PlaceCoder* coder = nullptr, double rope_seconds = -1, uint32_t rope_newest = 5,
                bool trim = false, bool pieces = false, bool optimise_archives = false, bool binary = false,
                bool gzip = false, bool v20 = false, bool v21 = false, bool v22 = false);

// A file's ways to be carried, each with its size in bytes as the instructions would hold it: LZMA2
// alone (estimated, preset 6), the rope over the whole file, and (v13) the file cut into pieces,
// LZMA2's and the rope's. The packer carries the smallest; the weighing weighs all three, so the
// two cannot disagree. The rope's search takes `rope_seconds` (0: no limit) and is remembered.
struct FileWays
{
    uint64_t lzma2 = 0;             // LZMA2 alone, estimated
    RopeParams rope;                // the whole file's wave
    std::vector<uint8_t> rope_code; // and its code
    uint64_t rope_bytes = 0;        // its code and the code's length (a varint)
    bool has_pieces = false;
    std::string pieces_way;          // "pieces z… r… rope <parameters>"
    std::string pieces_said;         // where it has none, why: "not tried", "under 16 KiB", "all 24415 blocks ... went to LZMA2" ...
    std::vector<uint8_t> pieces_z;   // the pieces LZMA2 packs, joined
    std::vector<uint8_t> pieces_code; // the rope's code of the rest
    RopeParams pieces_rope;
    uint64_t pieces_bytes = 0;      // LZMA2's estimate of z, the code, its length and the way's longer text
    size_t tried = 0;               // waves tried
    bool forced = false;            // a force stop came first: only `lzma2` and `tried` are set
};
FileWays file_ways(const std::vector<uint8_t>& bytes, double rope_seconds, uint32_t rope_newest = 6, bool pieces = true);
// The ways file_ways keeps for the run (the File Locator's weighing asks for them again) let go:
// when a reading is done, so a large file's are not held after it.
void forget_file_ways();
// What mixing came to on a file when the packer weighed it (its way, "mix4 B22 D60", and its code's
// bytes with the code's length), kept for the run with the ways above, so the weighing counts it too:
// file_ways does not mix (it takes minutes a megabyte), and without it the weighing's best way was
// LZMA2's estimate beside instructions a sixth smaller (enwik8: 24.9% against 21.68%).
struct MixedAs
{
    std::string way;
    uint64_t bytes = 0;
};
std::optional<MixedAs> mixed_as_found(const std::vector<uint8_t>& bytes);

// Threads for work made of separate parts (SIEVE_PACK_THREADS, else every core), and the memory free
// now in bytes (0 where it cannot be known): the packer's, and installing's (files decoded at once).
unsigned pack_threads();
uint64_t memory_free();

// v14: an archive remade from what it holds, by its recipe ("xz 6 crc64 <size>"): what installing
// does (set as the manifest's remaker).
std::vector<uint8_t> remake_archive(const std::string& recipe, const std::vector<uint8_t>& inner);

// The report as lines, for the tool and the File Locator.
std::string pack_report_text(const PackReport& r, uint64_t original);

// ---- Any archive Sieve reads, extracted (the right-click menu's Extract on an archive, as an
// archiver's; sieve-install). One layer only, as 7-Zip's: a .tar.gz gives its .tar, a .zip its files.
// The kinds: 7z, zip, xz, gzip, bzip2, zstd, lz4, lzip, and by their names lzma (.lzma) and brotli
// (.br); and through libarchive RAR 4 and 5, cab, ISO 9660, LHA, cpio, ar, WARC and tar.
struct AnyArchive
{
    struct Entry
    {
        std::string path; // relative, '/' between parts, as the archive names it (its top folder kept)
        bool dir = false;
        std::vector<uint8_t> bytes;
    };
    std::string format;   // "7z", "zip", "gz", "rar", "tar" ...
    std::vector<Entry> entries;
    size_t links_left_out = 0; // links in a tar (or another archive) are not written
    uint64_t files = 0, bytes = 0;
};
struct ArchiveKind
{
    const char* format;                     // as AnyArchive's
    std::vector<const char*> extensions;    // lower case, with the dot
};
// Every kind read_any_archive extracts: the one list the right-click menu is made from.
const std::vector<ArchiveKind>& archive_kinds();
// Nothing where it is none of them; throws where it is one this cannot read (encrypted, damaged),
// saying why. `file_name` gives a compressed stream's one file its name (notes.txt.gz: notes.txt),
// and names brotli and lzma, which have no mark of their own.
std::optional<AnyArchive> read_any_archive(const std::vector<uint8_t>& bytes, const std::string& file_name);
// The one folder every entry is in, where there is one, left out (Extract to NAME\ puts its
// contents straight into NAME\); its name in `top`.
void leave_out_top_folder(AnyArchive& a, std::string* top = nullptr);
// Writes it into `dest` (made if need be): files already there refused unless `force`, each file
// checked by the vault, and on a cancel (false) or a failure (throws) what was written removed again.
// `keep`: files already there that stay as they are (install_tree's).
bool unpack_archive(const AnyArchive& a, const std::filesystem::path& dest, bool force,
                    const std::function<void(size_t done, size_t total, const std::string& path)>& progress,
                    const std::atomic<bool>* cancel = nullptr, const std::set<std::string>* keep = nullptr);

} // namespace sieve::cli
