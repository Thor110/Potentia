// Sieve — the infinite RAID, stages 1 and 2: a store for any save, kept on several drives (IDEAS
// 19.154, 19.176). Local only: between machines is stage 3, sieve/raid_net.hpp.
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits. (A request, not a condition: whoever uses
// the RAID in a system run for profit is asked to give a share of that profit to charity, the share
// settled by open, democratic debate; IDEAS 19.177.)
//
// "sieve-raid-v1". A RAID is a set of folders (drives) the person chooses; each holds a file
// `sieve-raid` naming the folder ("sieve-raid-v1 id=HEX": 32 random hex digits, made once when the
// folder joins), a folder `chunks`, and a folder `manifests`.
//
// A save (any file: a game's save, a character, a whole dimension's file) is cut into chunks where
// its content says (stage 2, below; 1 MiB pieces in stage 1), each named by its SHA-256 and kept as chunks/AB/<SHA-256> (AB its first
// two hex digits). Each chunk is written to `copies` of the folders (2 unless asked otherwise; all of
// them if there are fewer): the folders ranked by SHA-256(chunk name | folder id), highest first, so
// the same chunk goes to the same folders on every machine and adding a folder moves only a share.
//
// A manifest says what a save is, and is kept in every folder as manifests/<SHA-256>.manifest:
//     sieve-raid-manifest-v1
//     name NAME                (the save's name: UTF-8, no line breaks)
//     size BYTES
//     previous SHA-256         (the version before, or "none")
//     chunk SHA-256 LENGTH     (one a chunk, in order)
// Its SHA-256 is the save's version id. A save stored again under the same name names the latest
// version of that name as its previous, so every save keeps its history as a chain; storing the same
// bytes again under the same name adds nothing (it is already the latest).
//
// Nothing needs to be trusted: every chunk and every manifest is checked against its name when read.
// verify() reports every chunk that is missing, damaged or short of its copies; repair() makes it
// whole again from any folder that holds a good copy, and says what it could not.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kRaidVersion = "sieve-raid-v1";
inline constexpr const char* kRaidManifestVersion = "sieve-raid-manifest-v1";
inline constexpr size_t kRaidChunk = size_t(1) << 20; // the longest chunk a manifest may name

// Stage 2 (IDEAS 19.176): where a save is cut, "sieve-raid-cut-v2". The cuts are made by the content
// itself, so an edit moves only the cuts near it and a new version shares every other chunk with the
// one before, however the bytes around the edit have shifted. A rolling hash runs over the bytes,
//     h = (h << 1) + gear[byte]    (64 bits; gear[i] the first 8 bytes, little-endian, of
//                                   SHA-256("sieve-raid-gear-v1" | the byte i))
// and a chunk ends after a byte where it is at least kRaidCutMin long and the low 16 bits of h are all
// 0 (about one in 65,536 places), or where it reaches kRaidCutMax; h starts again from 0 after each
// cut. So chunks are 16 KiB to 256 KiB, about 80 KiB on bytes that look random. A store written
// before stage 2 (1 MiB pieces) reads as before; its saves simply share no chunks with later versions.
inline constexpr size_t kRaidCutMin = size_t(16) << 10, kRaidCutMax = size_t(256) << 10;
inline constexpr uint64_t kRaidCutMask = 0xFFFF;
// The cuts of a save, as (offset, length) in order.
std::vector<std::pair<size_t, size_t>> raid_cut(const std::string& bytes);

struct RaidManifest
{
    std::string name;
    uint64_t size = 0;
    std::string previous; // empty: none
    std::vector<std::pair<std::string, uint64_t>> chunks; // SHA-256, length
    std::string text() const;
    std::string id() const; // SHA-256 of the text
};
// Reads a manifest's text, checked. Throws std::invalid_argument saying what is wrong.
RaidManifest read_raid_manifest(const std::string& text);

struct RaidReport
{
    size_t folders = 0, manifests = 0, chunks = 0; // distinct manifests and chunks named
    size_t good_copies = 0;                         // chunk copies that check out
    size_t damaged = 0;                             // copies whose bytes are not their name
    size_t missing = 0;                             // chunks with fewer good copies than asked
    size_t lost = 0;                                // chunks with no good copy anywhere
    size_t repaired = 0;                            // copies written by repair()
    std::vector<std::string> notes;                 // each problem, in a few words
    bool whole() const { return damaged == 0 && missing == 0 && lost == 0; }
};

class Raid
{
public:
    // The folders, each made a member if it is not one yet (its sieve-raid file written). Throws
    // std::invalid_argument for no folders, a folder that cannot be made, or one whose sieve-raid file
    // is not this version.
    explicit Raid(std::vector<std::filesystem::path> folders, uint32_t copies = 2);

    // Stores a save under a name; returns its version id (its manifest's SHA-256).
    std::string put(const std::string& name, const std::string& bytes);
    // A version's bytes, every chunk checked, each read from whichever folder has a good copy.
    // Throws std::runtime_error if a chunk has no good copy anywhere.
    std::string get(const std::string& version) const;
    // Every manifest found in any folder (checked), by name, oldest first along each chain.
    std::vector<RaidManifest> list() const;
    // The latest version of a name, if there is one.
    std::optional<std::string> latest(const std::string& name) const;

    RaidReport verify() const;
    RaidReport repair();

    const std::vector<std::filesystem::path>& folders() const { return folders_; }
    const std::vector<std::string>& folder_ids() const { return ids_; }
    // The folders a chunk belongs on (indices into folders(), best first).
    std::vector<size_t> homes(const std::string& chunk) const;

private:
    RaidReport check(bool fix) const;
    std::vector<std::filesystem::path> folders_;
    std::vector<std::string> ids_;
    uint32_t copies_;
};

} // namespace sieve
