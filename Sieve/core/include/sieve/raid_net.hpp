// Sieve — the infinite RAID, stage 3: between machines, opt in only (IDEAS 19.176, 19.178, 19.179).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits. (A request, not a condition: whoever uses
// the RAID in a system run for profit is asked to give a share of that profit to charity, the share
// settled by open, democratic debate; IDEAS 19.177.)
//
// A person's saves (stages 1 and 2, sieve/raid.hpp) can be kept on other people's machines as well as
// their own drives. Nothing is shared unless the person asks: a machine keeps others' chunks only
// while its owner runs `sieve raid serve`, and a save goes out only on `sieve raid share`.
//
// What leaves the owner's machine is sealed (sieve/seal.hpp) under a key only the owner holds, so
// those who keep it cannot read it, and every piece is named by the SHA-256 of its sealed bytes, so
// nothing can be changed on the way or in keeping without being refused. A save goes out as:
//   - each chunk of the version, sealed, to `copies` of the peers (2 unless asked): the peers ranked by
//     SHA-256(sealed chunk's name | the peer's address), highest first, as stage 1 ranks folders;
//   - its manifest, sealed, and an index, sealed, to every peer. The index's text:
//         sieve-raid-remote-v1
//         manifest SHA-256       (the sealed manifest's name)
//         blob SHA-256           (each sealed chunk's name, in the manifest's order)
//     The sealed index's name is the save's remote id: with it and the key, the save can be fetched
//     back from any of the peers, on any machine.
// A chunk a peer already has is not sent again; since sealing gives the same bytes for the same
// chunk, versions that share chunks share what is sent.
//
// "sieve-raid-held-v1": a folder keeping others' sealed chunks. Its file `sieve-raid-held` reads
// "sieve-raid-held-v1 salt=HEX" (32 random hex digits, made once); the chunks are blobs/AB/<SHA-256>;
// the file `peers` says how much each peer has had kept, a line each, "KEY BYTES", KEY being
// SHA-256(salt | the peer's IP address): the address itself is never written down. Limits, as Edward
// set the first step of them (IDEAS 19.179: by IP address first, then by verified identity, then by
// what a character has kept for others): the space given in all, and the most kept for any one
// address. A chunk over either is refused when it is offered; nothing kept is ever deleted to make room.
//
// "sieve-raid-net-v1", the protocol, over any byte stream (TCP in `sieve raid`): lines end in a line
// feed, at most 200 bytes. The one who connects says "sieve-raid-net-v1", the one who keeps answers
// the same (or "refused WHY" and closes). Then any number of requests, each answered in turn:
//     has SHA            ->  yes | no
//     get SHA            ->  blob N, then the N bytes | no
//     put SHA N, N bytes ->  kept | had | refused WHY      (N at most kRaidNetMax)
//     bye                ->  (closed)
// A put whose bytes are not their name is refused, and so is anything not written as above, which
// also ends the connection.
#pragma once

#include "sieve/raid.hpp"
#include "sieve/seal.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kRaidNetVersion = "sieve-raid-net-v1";
inline constexpr const char* kRaidHeldVersion = "sieve-raid-held-v1";
inline constexpr const char* kRaidRemoteVersion = "sieve-raid-remote-v1";
inline constexpr size_t kRaidNetMax = (size_t(4) << 20) + kSealOverhead; // a stage 1 chunk or a long manifest, sealed
inline constexpr size_t kRaidNetLine = 200;

// A byte stream: a socket, or anything else that carries bytes both ways.
class RaidStream
{
public:
    virtual ~RaidStream() = default;
    // Reads exactly n bytes; false at the end, on a timeout or on an error.
    virtual bool read(char* to, size_t n) = 0;
    virtual bool write(std::string_view bytes) = 0;
};

struct HeldLimits
{
    uint64_t space = uint64_t(1) << 30;    // kept for everyone, in all
    uint64_t per_peer = uint64_t(64) << 20; // kept for any one address
};

// A folder keeping others' sealed chunks.
class RaidHeld
{
public:
    // Made a held folder if it is not one yet. Throws std::invalid_argument if it cannot be, or if
    // its sieve-raid-held file is not this version.
    RaidHeld(std::filesystem::path folder, HeldLimits limits = {});

    bool has(const std::string& name) const;
    std::optional<std::string> get(const std::string& name) const; // checked against its name
    struct Put
    {
        enum class Is { Kept, Had, Refused } is = Is::Refused;
        std::string why;
    };
    Put put(const std::string& name, const std::string& bytes, const std::string& peer_address);

    uint64_t used() const;
    uint64_t used_by(const std::string& peer_address) const;
    std::string peer_key(const std::string& peer_address) const; // SHA-256(salt | address)

    // Answers one connection's requests (above) until "bye", the stream's end, or a request out of
    // place; returns how many were answered.
    size_t serve(RaidStream& s, const std::string& peer_address);

    const std::filesystem::path& folder() const { return folder_; }

private:
    void save_peers() const;
    std::filesystem::path folder_;
    HeldLimits limits_;
    std::string salt_;
    std::vector<std::pair<std::string, uint64_t>> peers_; // key, bytes; in order of key
};

// One machine keeping chunks, as the owner sees it.
class RaidPeer
{
public:
    virtual ~RaidPeer() = default;
    virtual std::string address() const = 0;
    virtual bool has(const std::string& name) = 0;
    virtual std::optional<std::string> get(const std::string& name) = 0;
    // Empty if kept (or already had); otherwise why it was refused.
    virtual std::string put(const std::string& name, const std::string& bytes) = 0;
};

// A peer over a stream, speaking sieve-raid-net-v1. The greeting is exchanged on construction;
// throws std::runtime_error if it is not answered. A request not answered as the protocol says
// throws std::runtime_error.
class RaidNetPeer : public RaidPeer
{
public:
    RaidNetPeer(std::unique_ptr<RaidStream> stream, std::string address);
    ~RaidNetPeer() override;
    std::string address() const override { return address_; }
    bool has(const std::string& name) override;
    std::optional<std::string> get(const std::string& name) override;
    std::string put(const std::string& name, const std::string& bytes) override;

private:
    std::string line();
    std::unique_ptr<RaidStream> s_;
    std::string address_;
};

// A held folder on this machine as a peer (for tests, and for a drive kept for a friend).
class RaidLocalPeer : public RaidPeer
{
public:
    RaidLocalPeer(RaidHeld& held, std::string address, std::string from = "127.0.0.1") : held_(held), address_(std::move(address)), from_(std::move(from)) {}
    std::string address() const override { return address_; }
    bool has(const std::string& name) override { return held_.has(name); }
    std::optional<std::string> get(const std::string& name) override { return held_.get(name); }
    std::string put(const std::string& name, const std::string& bytes) override;

private:
    RaidHeld& held_;
    std::string address_, from_;
};

// The peers a sealed chunk belongs on (indices, best first).
std::vector<size_t> raid_peer_homes(const std::string& name, const std::vector<RaidPeer*>& peers);

struct RaidShareReport
{
    std::string remote;        // the save's remote id
    size_t chunks = 0;         // the version's chunks
    size_t sent = 0, had = 0;  // copies sent; copies a peer already had
    uint64_t bytes_sent = 0;
    size_t short_of = 0;       // chunks with fewer copies than asked (refused, or too few peers)
    std::vector<std::string> notes;
    bool whole() const { return short_of == 0; }
};
// Sends a version of a save from a RAID to the peers, sealed under the key. Throws
// std::invalid_argument if there are no peers or the version is not in the RAID.
RaidShareReport raid_share(const Raid& raid, const std::string& version, const SealKey& key, const std::vector<RaidPeer*>& peers, uint32_t copies = 2);

// A save fetched back by its remote id: every piece asked of its homes first, then of any peer, and
// each one checked (its name, its seal, and the manifest's SHA-256 of each chunk). Throws
// std::runtime_error naming what could not be had.
struct RaidFetched
{
    RaidManifest manifest;
    std::string bytes;
};
RaidFetched raid_fetch(const std::string& remote, const SealKey& key, const std::vector<RaidPeer*>& peers);

// "HOST:PORT" or "[IPv6]:PORT" into its two parts. Throws std::invalid_argument.
std::pair<std::string, std::string> raid_split_address(const std::string& address);

} // namespace sieve
