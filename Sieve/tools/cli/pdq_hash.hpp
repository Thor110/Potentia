// Sieve — PDQ, the perceptual hash for pictures (docs/VAULT.md), over Meta's reference code
// (third_party/pdq, BSD licence).
//
// A perceptual hash gives similar pictures similar hashes: resized, recompressed, lightly edited
// or recoloured copies of a picture land within a few bits of it, where a SHA-256 would change
// entirely. PDQ is the open one used across the industry for exactly the lists the vault is built
// to hold. A picture is reduced to luminance, blurred and cut down to 64 x 64, turned by a
// discrete cosine transform into 16 x 16 frequencies, and each is set against their median: 256
// bits. Two pictures match when their hashes differ in few bits.
//
// This header keeps PDQ's own types out of Sieve's code: hashes are 32 bytes, in the order of
// PDQ's own 64-digit hex form, so a hash from any PDQ tool reads the same here.
#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace sieve::cli::pdq {

using Hash = std::array<uint8_t, 32>;

// The hashes of an RGBA picture (width x height x 4 bytes, row major), as it is and in its seven
// other orientations (rotated a quarter, a half and three quarters; flipped about each axis and
// each diagonal), [0] as it is. `quality` is PDQ's 0-100 measure of how much there is in the
// picture to hash: below 50, it is too plain for its hash to mean much. A picture under 5 pixels
// either way cannot be hashed: then nothing is returned.
struct Hashes
{
    std::array<Hash, 8> orientations;
    int quality = 0;
};
std::optional<Hashes> hash_rgba(const uint8_t* rgba, uint32_t width, uint32_t height);

// How many of the 256 bits differ.
int distance(const Hash& a, const Hash& b);

// The 64 lower-case hex digits PDQ's tools write, and back (nothing when it is not that).
std::string to_hex(const Hash& h);
std::optional<Hash> from_hex(const std::string& hex);

} // namespace sieve::cli::pdq
