// Sieve — the vault (docs/VAULT.md): hashes of content Sieve refuses to show, emit or pass on.
//
// The space is untouched: every address still exists and every count stays exact. What the vault
// changes is what the tool does. Content that matches an entry is *withheld*: the hallway draws it
// blank and will not let it be taken, saved, installed, mapped or exported, and the command line
// prints "withheld" in its place. It is not a filter: it is not in the stack, it has no mode, and
// nothing in the settings turns it off.
//
// Entries are hashes only, in `sieve-vault-v3` files (conceptually a map's cousin, holding nothing
// but the hashes; v1 files, sha256 entries only, and v2, no chunks, are still read):
//
//     sieve-vault-v3
//     entries <N>
//     v	sha256	<64 hex digits>      the exact bytes of a file
//     v	pdq	<64 hex digits>         a picture, by PDQ (pdq_hash.hpp): its near copies match too
//     v	chunks	<hex>,<hex>,...     one file, by its cdc-v1 chunks (sieve/chunks.hpp): pieces of it
//     end
//
// A `chunks` entry matches under `chunk-match-v1`: bytes are withheld when they hold two different
// chunks of one entry (or its only chunk, for a file of one chunk), wherever they sit; chunks of a
// single repeated byte are never counted. So a piece of a listed file, cut out anywhere and
// written anywhere (a unit of a line, a page of a book, bytes decoded from text, another file), is
// recognised once it holds two whole chunks of it after falling into step with its cuts: measured
// on random data, a piece of 1 KB is caught about two times in three, 1.5 KB 86%, 2 KB 98%.
//
// A picture matches a pdq entry under `pdq-match-v1`: PDQ's quality at least 50 (plainer pictures
// hash too alike to tell apart), and any of its eight orientations within 31 bits, the threshold
// PDQ's authors give for a match. The built-in
// entries are compiled in, so a copy of a program on its own (an installer program, say) still has
// them; every *.vault file in the `vault` folder beside the programs (or the folder above, in an
// installation's tools\) adds to them. A vault file that exists but cannot be read makes the vault
// *fail closed*: everything is withheld until it is fixed or removed. Never open.
//
// What is hashed: a file's bytes, exactly, and if it is a picture (any format stb_image reads), each
// of its frames by PDQ; a melody by its MIDI file; a model by its .obj; every picture Sieve draws
// (image and video units, covers) by PDQ; text as a file written out (vault_decode.hpp), never by
// what it says. It is for known files and pictures.
// The built-in entries are harmless tests (as antivirus software ships the EICAR test file), so the
// mechanism can be seen working and checked: the 16 bytes "sieve vault test", a test picture
// (test_picture below) by PDQ, and a test file (test_file_bytes) by its chunks.
#pragma once

#include <cstdint>
#include <filesystem>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace sieve::cli {

// Thrown where content is refused for matching the vault. The message says only that it is
// withheld (and whose path, where it is the user's own file), never which entry matched.
struct VaultWithheld : std::runtime_error
{
    using std::runtime_error::runtime_error;
};

namespace vault {

inline constexpr const char* kVersion = "sieve-vault-v3";
inline constexpr const char* kVersion2 = "sieve-vault-v2"; // still read: no chunks
inline constexpr const char* kVersion1 = "sieve-vault-v1"; // still read: sha256 entries only
inline constexpr const char* kChunkMatch = "chunk-match-v1";
inline constexpr const char* kPdqMatch = "pdq-match-v1";
inline constexpr int kPdqMaxDistance = 31, kPdqMinQuality = 50;

// True when the content with this SHA-256 (lower-case hex) is withheld. With the vault failed
// closed, everything is.
bool withheld_sha256(const std::string& sha256_hex);
bool withheld_bytes(const std::vector<uint8_t>& bytes);
bool withheld_text(const std::string& utf8); // text already without its padding
// A picture, RGBA (width x height x 4 bytes, row major), by PDQ. False for one too small to hash
// (under 5 pixels either way) or too plain (quality under 50), unless failed closed.
bool withheld_picture(const uint8_t* rgba, uint32_t width, uint32_t height);

// The PDQ hash (64 hex digits, as PDQ's own tools write it) and quality of each frame of a picture
// file's bytes; empty when they are not a picture or it is too small. For `sieve vault --pdq`, so a
// deployment can check its hashes agree with its list's.
struct PictureHash
{
    std::string hex;
    int quality = 0;
};
std::vector<PictureHash> picture_hashes(const std::vector<uint8_t>& bytes);

// The test picture: 64 x 64 grey, in 4 x 4 blocks whose shades are the bytes of SHA-256 chained
// from "sieve vault test picture" (the digest, then the digest of that, eight times over: 256
// blocks). Noise no photograph resembles, so it withholds nothing real; its PDQ hash is built in.
std::vector<uint8_t> test_picture_rgba(); // 64 * 64 * 4 bytes

// The test file: 4,096 bytes, SHA-256 chained from "sieve vault test file" (the digest, then the
// digest of that, 128 times). Its chunks are a built-in `chunks` entry (its whole-file hash is not
// listed), so any piece of it of a few hundred bytes is withheld: the chunk check, seen working.
std::vector<uint8_t> test_file_bytes();

// A file's `chunks` entry line (without its end of line): its cdc-v1 chunks, less those of one
// repeated byte and less any in `common` (chunks found in ordinary files, which would match
// unrelated things). Empty when no chunk is left.
std::string chunks_entry(const std::vector<uint8_t>& bytes, const std::set<std::string>& common = {});

// Throws VaultWithheld when withheld; `what` names what is refused (a path, or "the unit").
void check_sha256(const std::string& sha256_hex, const std::string& what);
void check_bytes(const std::vector<uint8_t>& bytes, const std::string& what);
// Where the SHA-256 is already known: checks it, then the bytes as a picture (when they are one).
void check_known(const std::vector<uint8_t>& bytes, const std::string& sha256_hex, const std::string& what);
// A file on disk, read once: its SHA-256 (returned) and its chunks worked out from the same blocks
// and checked; then, if the file is a picture (by its first bytes, or a .tga name), its frames by
// PDQ. Throws VaultWithheld when withheld.
std::string hash_checked_file(const std::filesystem::path& file, const std::string& what);

// The state, for `sieve vault` and the hallway: how many entries, from which files, and whether it
// has failed closed (and why).
struct Status
{
    size_t builtin = 0, loaded = 0;          // entries compiled in, and from files
    size_t sha256 = 0, pdq = 0;              // all entries, of each kind
    size_t chunk_files = 0, chunks = 0;      // `chunks` entries, and the chunks in them
    std::vector<std::string> files;          // the vault files read
    bool failed_closed = false;
    std::string error;                       // why, when it has
};
Status status();

// A vault file's text, parsed (throws on anything malformed). For `sieve vault --parse`.
struct Entries
{
    std::vector<std::string> sha256, pdq;       // lower-case hex
    std::vector<std::vector<std::string>> chunks; // one per `chunks` entry: its chunks' SHA-256s
    size_t size() const { return sha256.size() + pdq.size() + chunks.size(); }
};
Entries parse(const std::string& text);

} // namespace vault
} // namespace sieve::cli
