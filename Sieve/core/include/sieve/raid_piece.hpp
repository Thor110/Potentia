// Sieve — the infinite RAID's pieces: a save cut into parts with parity, each kept by a different
// machine, within limits each machine sets (IDEAS 19.210). Fixed and versioned before anyone keeps a
// piece; nothing here is switched on (sharing between machines still sends whole copies, raid_net.hpp).
//
// Additional term under section 7(b) of the GNU AGPL v3 (docs/RAID-NOTICE.md): whoever conveys this
// file, or a work based on it, must keep in it the notice "The infinite RAID save and data
// preservation system was designed by Edward James Gordon (Potentia)", and must show that notice
// wherever the work shows its legal notices or its credits. (A request, not a condition: whoever uses
// the RAID in a system run for profit is asked to give a share of that profit to charity, the share
// settled by open, democratic debate; IDEAS 19.177.)
//
// The whole is what leaves the owner's machine: a save sieved (made as small as Sieve makes it), then
// sealed (sieve/seal.hpp), so those who keep its pieces cannot read it. It is cut into K pieces of
// data and M of parity, any K of the K + M enough to make it again (erasure coding, as RAID 6 is with
// M = 2):
//   - S = ceil(LENGTH / K) bytes each; data piece i is the whole's bytes from i x S, the last one
//     filled out with zero bytes;
//   - parity piece j (0 to M - 1) is, byte by byte, the sum over i of C(j, i) x data piece i, in
//     GF(2^8) (the polynomial x^8 + x^4 + x^3 + x^2 + 1, 0x11D), C(j, i) = 1 / ((K + j) xor i): a
//     Cauchy matrix, so any K rows of the identity over it can be inverted, and any K pieces give the
//     whole back. K + M is at most 256.
//
// "sieve-raid-piece-v1", a piece's bytes: three lines of text, then its S bytes.
//     sieve-raid-piece-v1
//     of SHA LENGTH          (the whole's SHA-256 and its length in bytes, 1 or more)
//     code K M INDEX         (K at least 1, K + M at most 256; INDEX 0 to K - 1 data, then parity)
// Numbers in decimal without sign or leading zero. A piece is named by the SHA-256 of its bytes, as
// every blob between machines is (raid_net.hpp), so it travels and is kept as they are.
//
// "sieve-raid-tally-v1", who keeps pieces and how full each is, checked before a piece is placed:
//     sieve-raid-tally-v1
//     keeper ADDRESS SPACE LARGEST USED    (a line a machine, in order of ADDRESS, each once)
// ADDRESS as the machine is reached (HOST:PORT, no spaces), SPACE the bytes it gives in all, LARGEST
// the largest piece it keeps, USED what it keeps now (at most SPACE).
//
// Placement, a rule every machine works out the same from the tally: piece INDEX of a whole goes to
// the first keeper, in the order raid_rank gives for the name "SHA/INDEX" (the whole's SHA-256, a
// slash, the index), that keeps no other piece of the same whole, takes a piece of S bytes, and has
// room for it; each piece placed is counted in its keeper's USED before the next is placed. A whole
// with a piece no keeper can take is refused, said, rather than spread thin.
#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kRaidPieceVersion = "sieve-raid-piece-v1";
inline constexpr const char* kRaidTallyVersion = "sieve-raid-tally-v1";
inline constexpr uint32_t kRaidPiecesMost = 256; // K + M at most: GF(2^8) has 256 elements

struct RaidPiece
{
    std::string whole;   // the whole's SHA-256 (64 hex digits, small letters)
    uint64_t length = 0; // the whole's length in bytes
    uint32_t k = 1, m = 0, index = 0;
    std::string data;    // S = ceil(length / k) bytes

    std::string bytes() const; // the piece as it is kept
    std::string name() const;  // the SHA-256 of bytes()
};
// Reads a piece, checked (its head written as above, its data S bytes long). Throws
// std::invalid_argument saying what is wrong.
RaidPiece read_raid_piece(const std::string& bytes);

// The whole cut into k + m pieces, in order of index. Throws std::invalid_argument for an empty whole,
// k of 0, or k + m over 256.
std::vector<RaidPiece> raid_pieces(const std::string& whole, uint32_t k, uint32_t m);
// The whole made again from any k pieces of it (others given are checked against the rest, and one of
// another whole, or twice the same index, is refused); checked against its SHA-256. Throws
// std::invalid_argument saying what is wrong or missing.
std::string raid_join(const std::vector<RaidPiece>& pieces);

// How a whole is cut (IDEAS 19.210): while saves are small, one piece of data and copies of it (as
// sharing does today); above a size, k of data and m of parity. The figures are chosen, not measured:
// where the split pays (a lookup's cost against copies kept) is still to be measured.
struct RaidPieceRule
{
    uint64_t split_above = uint64_t(256) << 10; // a whole up to this is one piece and its copies
    uint32_t small_m = 2;                        // copies of a small whole (3 in all, as kRaidPeerCopies)
    uint32_t k = 4, m = 2;                       // above it
};
std::pair<uint32_t, uint32_t> raid_piece_shape(uint64_t length, const RaidPieceRule& rule = {});

struct RaidKeeper
{
    std::string address;
    uint64_t space = 0, largest = 0, used = 0;
};
struct RaidTally
{
    std::vector<RaidKeeper> keepers; // in order of address
    std::string text() const;
};
// Reads a tally, checked. Throws std::invalid_argument saying what is wrong.
RaidTally read_raid_tally(const std::string& text);

// Where each of a whole's n pieces of `piece_bytes` goes (indices into tally.keepers, by piece index),
// placed by the rule above; the tally's USED counts them. Nothing if a piece has nowhere to go, `why`
// saying which and why.
std::optional<std::vector<size_t>> raid_place(const std::string& whole, uint32_t n, uint64_t piece_bytes, RaidTally& tally, std::string* why = nullptr);

// A keeper leaving: the whole's pieces placed again by the same rule without it (from a tally that
// still counts them where `homes` puts them), and the moves that makes, (index, from, to) as addresses;
// the leaving keeper hands its own on first. Nothing if a piece then has nowhere to go.
struct RaidMove
{
    uint32_t index = 0;
    std::string from, to;
};
std::optional<std::vector<RaidMove>> raid_leave(const std::string& whole, uint32_t n, uint64_t piece_bytes, const RaidTally& tally, const std::vector<size_t>& homes,
                                                const std::string& leaving, std::string* why = nullptr);

} // namespace sieve
