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
                bool item_hashes = false, const PlaceCoder* coder = nullptr, double rope_seconds = -1);

// The report as lines, for the tool and the File Locator.
std::string pack_report_text(const PackReport& r, uint64_t original);

} // namespace sieve::cli
