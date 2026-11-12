// Sieve — the infinite RAID, stage 1: a store for any save, kept on several drives (IDEAS 19.154,
// 19.176). Local only: no network.
//
// "sieve-raid-v1". A RAID is a set of folders (drives) the person chooses; each holds a file
// `sieve-raid` naming the folder ("sieve-raid-v1 id=HEX": 32 random hex digits, made once when the
// folder joins), a folder `chunks`, and a folder `manifests`.
//
// A save (any file: a game's save, a character, a whole dimension's file) is cut into chunks of
// 1 MiB (the last shorter), each named by its SHA-256 and kept as chunks/AB/<SHA-256> (AB its first
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
inline constexpr size_t kRaidChunk = size_t(1) << 20;

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
