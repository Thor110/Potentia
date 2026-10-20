// Sieve CLI — the file locator and the manifest creator (SPECIFICATIONS §12.2).
//
// Locating a file is naming its place on the binary line (§12.1, binary-v1): every file of every
// length is a unit there, and its positional address is its own hex dump plus 0101...01, one
// "01" for each of its bytes. That address depends only on the file, not on the line's length
// setting, so it is the file's name in the space; the SHA-256 beside it is its short identity.
//
// A manifest is a folder located: the folder walked to the bottom, every file and directory in it
// listed with the file's size and SHA-256, in one canonical text ("sieve-manifest-v1"). Canonical
// means that the same tree always gives the same bytes, so the manifest is itself a file with an
// identity and a place on the binary line, and one line of text names the whole tree:
//
//     sieve-manifest-v1
//     root <the folder's name>
//     files <how many files>
//     bytes <their sizes added up>
//     d	<path>/                      a directory, its path relative to the root, ending in '/'
//     f	<size>	<sha256>	<path>     a file
//     end
//
// Paths are UTF-8 with '/' between their parts, whatever the system writes; entries are sorted
// by their paths' bytes; the separators are single tabs and the lines end in a line feed. Paths
// come last on their line so that a path with spaces needs no quoting. Symbolic links are not
// followed or listed (they are counted, and reported, as skipped), since a link is a claim about
// somewhere else rather than content of its own. The addresses themselves are not in a v1
// manifest: each is the file, and would make the manifest as large as the tree it describes.
//
// "sieve-manifest-v2" is v1 with every file's address in it, between its SHA-256 and its path:
//
//     f	<size>	<sha256>	<address>	<path>
//
// a listing for reading, twice the tree's size and more (two hex digits a byte). Installers were
// once made from it, which made them twice as large as they need be; they are not any more.
//
// A packed installer's manifest, "sieve-manifest-v4" (the installers made now), is the same listing
// with each file's way of being carried between its SHA-256 and its path, and two lines naming the
// two streams its files are carried in:
//
//     stream x86 <packed bytes> <bytes> <dictionary>     programs (EXE, ELF), x86 filter then LZMA2
//     stream lzma2 <packed bytes> <bytes> <dictionary>   everything else, LZMA2
//     f	<size>	<sha256>	<way>	<path>
//
// and after "end" the two streams, the x86 first, each raw LZMA2 (its dictionary size gives its
// property byte as the LZMA2 format defines it). A file's way is one of:
//
//     x86       its bytes, next in the x86 stream
//     raw       its bytes, next in the lzma2 stream
//     lines K   derived from file K (counting the "f" lines from 0, a file carried x86 or raw): its
//               lines (split at each line feed; a file ending in one has an empty last line) are
//               some of file K's, in its order, and a mask of them, one bit for each of file K's
//               lines (the first in the low bit of the first byte), is next in the lzma2 stream
//
// so each file is carried as compactly as the packer found (pack.hpp), the whole of it as small as
// the strongest general compression, and smaller where one file is made from another (the
// dictionaries: each SCOWL list but the largest is a mask over the largest). Unpacking gives back
// every file exactly, each checked against its size and SHA-256 before anything is written.
//
// An installer's manifest, "sieve-manifest-v3", is the v1 text under its own first line, and then,
// straight after "end" and its line feed, the raw bytes of every file one after another, in the
// manifest's order: file k is the `size` bytes after the files before it. Nothing separates them
// and nothing more follows; the sizes say where each ends. It holds everything needed to put the
// tree back, so it is the tree's size and the text's, no more: the least an address of the tree
// can be without compressing it first. Its own address, one number, is then the whole
// installation: `sieve install` reads that number back into the manifest, and the manifest into
// the files, checking each against its size and SHA-256 before anything is written. (A v2
// manifest's address still installs: its files come from their addresses.)
#pragma once

#include "sieve/biguint.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::cli {

inline constexpr const char* kManifestVersion = "sieve-manifest-v1";
inline constexpr const char* kAddressManifestVersion = "sieve-manifest-v2";
inline constexpr const char* kInstallManifestVersion = "sieve-manifest-v3";
inline constexpr const char* kPackedManifestVersion = "sieve-manifest-v4";
inline constexpr const char* kItemsManifestVersion = "sieve-manifest-v5";
// v5 with every item's place carried at its least: as raw bytes, at its line's width, in a third
// stream after the other two, rather than in hex in the listing; and with the AI line among the
// lines offered (SPECIFICATIONS §12.2).
inline constexpr const char* kMinimalManifestVersion = "sieve-manifest-v6";
// v6 with less around its places: no line the rest gives (files, bytes, an empty stream, a single
// file's root), a line's options only where its id does not already give them, an item's entry as
// "i K path"; and a line's places coded where that is smaller (the AI line's by digit-histogram-v1,
// SPECIFICATIONS §12.2).
inline constexpr const char* kCompactManifestVersion = "sieve-manifest-v7";
// v7: how a line's places are coded, where they are not raw bytes of its width.
inline constexpr const char* kCodedPlaces = "digit-histogram-v1";
// v7 with files carried by the tightened rope (rope-wave-v1, IDEAS §19.87): a file's way "rope
// <parameters>", its code in a fourth stream (SPECIFICATIONS §12.2).
inline constexpr const char* kRopeManifestVersion = "sieve-manifest-v8";
// v8 whose rope ways may be rope-wave-v2's (columns, " C1"): made only where some file uses them.
inline constexpr const char* kRopeColumnsManifestVersion = "sieve-manifest-v9";
// v9 whose rope ways may be rope-wave-v3's (columns by line, " C2"): made only where some file's is.
inline constexpr const char* kRopeLinesManifestVersion = "sieve-manifest-v10";
// v10 whose rope ways may be rope-wave-v4's (columns sharing tables, " G1"): made only where some
// file's is.
inline constexpr const char* kRopeGroupsManifestVersion = "sieve-manifest-v11";
// v11 with less around a file's bytes: no items line where there are no items, and the lzma2 stream
// carried as it is ("stream lzma2 B B 0") where LZMA2 makes it no smaller. Made only where one of
// these applies.
inline constexpr const char* kTrimmedManifestVersion = "sieve-manifest-v12";
// v12 whose files may be cut into pieces: a way "pieces z17733 r4121949 rope <parameters>", the z
// pieces next in the lzma2 stream at the file's turn (as a raw file's bytes are), the r pieces
// joined and coded by the rope, next in the rope stream; made only where some file's is.
inline constexpr const char* kPiecesManifestVersion = "sieve-manifest-v13";
// v13 with archives optimised ("Optimise Archive Formats", off unless asked for): a file remade
// exactly from what it holds, its way "xz <preset> <check> <inner size> <way of what it holds>"; or
// an archive carried as its contents, a line "u\t<format>\t<size>\t<archive>\t<folder>" after the
// tree's saying so. Made only where some file's is.
inline constexpr const char* kArchivesManifestVersion = "sieve-manifest-v14";

std::vector<uint8_t> read_file_bytes(const std::filesystem::path& file);

// A file's place on the binary line in positional order (binary-v1), and its SHA-256 in hex.
BigUint binary_address(const std::vector<uint8_t>& bytes);
// And back: the file at a positional address on the binary line.
std::vector<uint8_t> file_at(const BigUint& address);
std::string sha256_hex(const std::vector<uint8_t>& bytes);
std::string sha256_file_hex(const std::filesystem::path& file); // streamed, for large files

struct ManifestEntry
{
    bool dir = false;
    std::string path;    // relative, '/' between parts; a directory's ends in '/'
    uint64_t size = 0;   // files
    std::string sha256;  // files
    std::string address; // files, in a v2 manifest: the file's positional address in hex
    std::string remake;  // files, in v14: "xz <preset> <check> <inner size>", the file remade from what its way gives
    std::string way;     // files, in a v4 manifest: "x86", "raw" or "lines K"; in v5 and v6 also "item K <index>"
                         // (v6 writes "item K" alone, the index carried in its items stream); in v7, on
                         // a line whose places are coded, "item K h<the code's bytes in hex>"; in v8
                         // also "rope L8 W949 S1 P3 K0", its code in the rope stream
};
// v7: an item's size, until it is made (its entry gives none).
inline constexpr uint64_t kSizeAsMade = ~uint64_t(0);

// One of a v4 manifest's two streams.
struct PackedStream
{
    uint64_t packed = 0, bytes = 0; // its size as carried, and unpacked
    uint32_t dictionary = 0;        // the LZMA2 dictionary it was packed with
};

struct Manifest
{
    std::string root;                    // the folder's own name
    std::vector<ManifestEntry> entries;  // sorted by path
    uint64_t files = 0, bytes = 0;
    uint64_t skipped = 0;                // symbolic links and other entries that are not files
    bool with_addresses = false;         // v2: every file's address in its line
    bool with_contents = false;          // v3 and v4: an installer's manifest; contents holds every file
    std::vector<uint8_t> contents;       // v3 and v4: every file's bytes, in manifest order (v4: unpacked)
    bool packed = false;                 // v4: carried in the two streams below
    PackedStream x86, lzma2;             // v4: the streams
    std::vector<uint8_t> streams;        // v4: the two streams as carried, the x86 first
    bool items = false;                  // v5: v4 with files carried as items of Sieve's lines
    std::vector<std::string> lines;      // v5: line K, "<its space id> <the options that make it>"
    std::string tree;                    // v5: the SHA-256 of the whole tree (tree_sha256)
    bool minimal = false;                // v6: v5 with its items' places as raw bytes (the items stream)
    std::vector<uint32_t> line_widths;   // v6: the bytes a place on line K takes
    // v6: every item's place, line_widths[K] bytes each, in the listing's order: the third stream,
    // as its places are (items_stream), and as carried (raw, dictionary 0, or LZMA2, whichever is
    // smaller: set by the packer, and read back by parse).
    std::vector<uint8_t> items_stream() const;
    PackedStream item_stream;
    std::vector<uint8_t> items_carried;
    bool compact = false;                // v7: v6 with less around its places (above); minimal too
    // v7: how line K's places are coded: "" raw, line_widths[K] bytes each; kCodedPlaces, each item's
    // code as a varint of its length and then its bytes.
    std::vector<std::string> line_codings;
    bool one_file_root() const; // v7 writes no root: one file, named as the root
    // v8: files carried by rope-wave-v1, each a varint of its code's length and the code, in the
    // listing's order: the fourth stream, carried as it is after the items stream.
    bool roped = false;
    bool rope_columns = false; // v9: some rope way is rope-wave-v2's
    bool rope_lines = false;   // v10: some rope way is rope-wave-v3's (and rope_columns is set too)
    bool rope_groups = false;  // v11: some rope way is rope-wave-v4's (and rope_lines is set too)
    bool trimmed = false;      // v12: no empty items line; the lzma2 stream perhaps carried as it is
    bool pieces = false;       // v13: some file is cut into pieces (and trimmed is set too)
    bool archives = false;     // v14: some archive is remade or carried as its contents (and pieces is set too)
    // v14: each archive carried as its contents: its format ("7z", "xz"), its size, its path, and the
    // folder (or file) its contents are in.
    struct UnpackedArchive
    {
        std::string format, archive, folder;
        uint64_t size = 0;
    };
    std::vector<UnpackedArchive> unpacked;
    PackedStream rope_stream;
    std::vector<uint8_t> rope_carried;
    // v7: the listing (every line after the first, up to and including "end") carried as LZMA2 where
    // that is smaller, after a line "listing <packed> <bytes> <dictionary>": set by the packer for
    // the text it packed (listing_of), which file() checks is still the text.
    PackedStream listing_stream;
    std::vector<uint8_t> listing_carried;
    std::string listing_of;
    const char* version() const; // its first line ("sieve-manifest-v7")
    std::string text() const;            // the canonical text above (for v3 and v4, its text part)
    std::vector<uint8_t> file() const;   // the whole manifest as a file: the text, and for v3 the contents, for v4 the streams
    // v1 to v4; throws if it is none of them. A v4 manifest's streams are unpacked into contents and
    // every file's way followed, so what is installed from it is checked as a v3's is.
    static Manifest parse(std::string_view whole);
};

// v5's check on the whole tree: the SHA-256 of, for every file in the listing's order, its path, a
// line feed, its bytes' SHA-256 in hex and a line feed. An item's own SHA-256 is left out of the
// listing ("-"), this covering it.
std::string tree_sha256(const Manifest& m);
// What makes an item's bytes from its line ("<space id> <options>") and its positional address
// (hex): set by the programs that hold every line's code (sieve, the hallway). Without it a v5
// manifest with items is refused with a word to open it with Sieve (the small installer has none).
// v14: what remakes an archive exactly from what it holds ("xz 6 crc64 <size>"); the full Sieve sets
// it (sieve-install has no compressor in it, and asks for Sieve, as it does for items).
using Remaker = std::function<std::vector<uint8_t>(const std::string& recipe, const std::vector<uint8_t>& inner)>;
void set_remaker(Remaker remaker);
using ItemMaker = std::function<std::vector<uint8_t>(const std::string& line, const std::string& address)>;
void set_item_maker(ItemMaker maker);
bool has_item_maker();

// Every file's address put into the manifest, making it v2. Reads every file again.
void add_addresses(Manifest& m, const std::filesystem::path& root);
// Every file's bytes put after the manifest, making it v3, an installer's. Reads every file again
// and checks it against the size and SHA-256 the walk found, so a file changed in between is
// refused rather than installed wrong.
void add_contents(Manifest& m, const std::filesystem::path& root);

// A single file as a manifest: as a folder holding just that file would be, its root named after
// the file. Sieve instructions for one file are this with the file's bytes after it (add_contents,
// with the file's own folder as the root), so a file and a folder are installed the same way and a
// file keeps its name and its SHA-256 check.
Manifest manifest_of_file(const std::filesystem::path& file);

// Walks `root` to the bottom. Throws on anything it cannot read, or a path that cannot be written
// in the manifest (one with a tab or a line break in it).
Manifest walk_folder(const std::filesystem::path& root);

// An address stored in a file: as raw bytes (the number in base 256, most significant first:
// exactly as long as the file it names, or one byte longer when adding 0101...01 carries past the
// first byte; the empty file's address, 0, is no bytes at all), or as hex text. An installer is
// its manifest's address as raw bytes, conventionally named <name>.sieve.
BigUint read_address_file(const std::filesystem::path& file, bool hex);
void write_address_file(const std::filesystem::path& file, const BigUint& address, bool hex);

// An installer program: a copy of sieve-install with an installer attached to its end, one file
// to hand to someone. Programs are read from their start, so what follows the program itself is
// left alone by the system that runs it; sieve-install looks at its own end for it:
//
//     <sieve-install, as built> <the installer: the address as raw bytes> <its length: 8 bytes,
//     least significant first> "sieve-attached-1"
//
// A program that already has one attached has it replaced, not a second added.
inline constexpr char kAttachedMagic[] = "sieve-attached-1"; // 16 bytes, no terminator used

// The running program's own file (for sieve-install, to find what is attached to it; for sieve
// and the hallway, to find the sieve-install beside them).
std::filesystem::path own_executable(const char* argv0);
// The sieve-install program beside `dir`, or in the tools folder in it (an installation, where
// sieve and sieve-install sit in tools\ below the hallway), if there is one.
std::optional<std::filesystem::path> installer_program_beside(const std::filesystem::path& dir);
// Writes `program` (sieve-install) with `address` attached as `out`, marked runnable.
void write_installer_program(const std::filesystem::path& program, const BigUint& address, const std::filesystem::path& out);
// The address attached to a program, if it has one.
std::optional<BigUint> attached_address(const std::filesystem::path& program);
// An address from its raw bytes (base 256, most significant first; no bytes is 0).
BigUint address_of_raw(const std::vector<uint8_t>& bytes);

// The manifest an installer holds, read back from it and checked to be one that can install (v3,
// or the older v2).
Manifest manifest_of_installer(const BigUint& address);

// The manifest to install from, whatever the file is: an installer (.sieve, raw bytes, or with
// `hex` written in hex), an installer program (sieve-install with one attached), or an installer's
// manifest itself (v3, or v2: it begins "sieve-manifest-v", which no raw address does, since an
// address is its manifest with every byte one higher). A v1 manifest is refused: it lists the
// files but does not hold them.
Manifest installable_manifest(const std::filesystem::path& file, bool hex);

// Puts a v3 (or v2) manifest's tree into `dest`. Every file is cut from the contents (v3) or read
// back from its address (v2), and checked
// against its size and SHA-256 before anything is written, so a bad installer leaves `dest` as
// it was; files already there are refused unless `force`. `progress` is told of each file as it
// is checked (phase 0) and written (phase 1). If `cancel` becomes true, what was written so far is
// removed again, the folders it made included, and the call returns false. Throws on errors.
using InstallProgress = std::function<void(int phase, size_t done, size_t total, const std::string& path)>;
bool install_tree(const Manifest& m, const std::filesystem::path& dest, bool force, const InstallProgress& progress,
                  const std::atomic<bool>* cancel = nullptr);

// A path, as a manifest writes it: relative to `root`, UTF-8, '/' between the parts.
std::string manifest_path(const std::filesystem::path& p, const std::filesystem::path& root);

} // namespace sieve::cli
