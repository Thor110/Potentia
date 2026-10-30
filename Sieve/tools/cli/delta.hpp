// Sieve CLI -- a file as its differences from another (delta-v1; sieve-manifest-v32, in the making).
//
// A delta-v1 script makes a file from a base file: a run of operations, each a varint t:
//   t even, t/2 >= 1: t/2 new bytes follow, written as they are;
//   t odd,  t/2 >= 1: t/2 bytes copied from the base, from where the last copy ended (0 at the
//                     start) moved by a second varint, zigzag-signed (0, -1, 1, -2 ... as 0, 1, 2, 3 ...).
// The script ends where its bytes do, and what it makes must be the file's size.
//
// Finding which files are alike: each cut into blocks where its content says (a rolling hash, about
// every 4 KiB), one fingerprint a block, so a file with bytes inserted is still found alike (blocks
// cut at fixed places are not: every block after an insertion moves). IDEAS §19.106.

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace sieve::cli::delta {

// Each content-cut block's fingerprint and length, in order.
struct Block
{
    uint64_t print;
    uint32_t length;
};
std::vector<Block> blocks_of(std::span<const uint8_t> file);

// A script making `target` from `base`, with what it holds; copies of at least 64 bytes.
struct Script
{
    std::vector<uint8_t> bytes;
    uint64_t copies = 0, copied = 0, fresh = 0; // copies made, the bytes they cover, the new bytes
};
Script make(std::span<const uint8_t> base, std::span<const uint8_t> target);

// The file a script makes from `base`, refused where it is not `size` bytes or reads outside either.
std::vector<uint8_t> apply(std::span<const uint8_t> base, std::span<const uint8_t> script, uint64_t size);

} // namespace sieve::cli::delta
