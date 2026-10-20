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
void add_packed(Manifest& m, const std::filesystem::path& root, PackReport* report = nullptr, const ItemFinder* items = nullptr,
                bool item_hashes = false, const PlaceCoder* coder = nullptr, double rope_seconds = -1, uint32_t rope_newest = 5,
                bool trim = false, bool pieces = false, bool optimise_archives = false, bool binary = false,
                bool gzip = false);

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
    std::vector<uint8_t> pieces_z;   // the pieces LZMA2 packs, joined
    std::vector<uint8_t> pieces_code; // the rope's code of the rest
    RopeParams pieces_rope;
    uint64_t pieces_bytes = 0;      // LZMA2's estimate of z, the code, its length and the way's longer text
    size_t tried = 0;               // waves tried
};
FileWays file_ways(const std::vector<uint8_t>& bytes, double rope_seconds, uint32_t rope_newest = 5, bool pieces = true);

// v14: an archive remade from what it holds, by its recipe ("xz 6 crc64 <size>"): what installing
// does (set as the manifest's remaker).
std::vector<uint8_t> remake_archive(const std::string& recipe, const std::vector<uint8_t>& inner);

// The report as lines, for the tool and the File Locator.
std::string pack_report_text(const PackReport& r, uint64_t original);

} // namespace sieve::cli
