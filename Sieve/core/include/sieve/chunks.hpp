// Sieve — content-defined chunks (cdc-v1): cutting bytes into pieces at places the content itself
// chooses, so a piece of something, cut out anywhere, yields the same chunks as the whole.
//
// A rolling hash runs over the bytes: h = 2h + gear[byte] (mod 2^64), where gear is a fixed table
// of 256 numbers. Shifting left by one each byte means the top eight bits of h depend only on the
// last 64 bytes. A chunk ends after the byte where those eight bits are all zero, once the chunk is
// at least kMin bytes long, or when it reaches kMax; then h starts again from 0. Because the cut
// test looks only at the last 64 bytes and kMin is 64, where a cut falls depends on the content
// and on where the chunk began, and nothing else: a fragment of a file falls into step with the
// file's own cuts within a chunk or two, and from there its chunks are the file's chunks. Each
// chunk is named by its SHA-256.
//
// The gear table: gear[i] is the first 8 bytes, read big-endian, of SHA-256 of the 11 bytes
// "cdc-v1 gear" followed by the byte i. Chunks average about kMin + 256 bytes.
//
// Versioned and never edited, like the filters: the vault's `chunks` entries are made with it, so
// a list-holder's tools must cut exactly as Sieve does (the Python oracle does it independently;
// tests/vectors_chunks_v1.tsv pins both). Other uses (filters that recognise known material, maps
// that recognise versions of a file) may use other versions, with other sizes, later.
#pragma once

#include "sieve/sha256.hpp"

#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sieve::cdc {

inline constexpr const char* kVersion = "cdc-v1";
inline constexpr size_t kMin = 64, kMax = 1024;

struct Chunk
{
    size_t offset = 0, length = 0;
    Sha256::Digest sha256{};
    // Every byte the same (a run of zeros, say): such chunks turn up in unrelated files, so they
    // never identify anything.
    bool uniform = false;
};

// The chunks of the bytes, in order, covering them exactly (none for no bytes).
std::vector<Chunk> chunks(std::span<const uint8_t> bytes);

// The same cutting, fed a piece at a time (a large file read in blocks): the chunks come out as
// they close, and finish() closes the last. Fed the same bytes, in any pieces, it gives exactly
// what chunks() gives.
class Chunker
{
public:
    void update(std::span<const uint8_t> data);
    void finish();
    // The chunks closed so far and not yet taken; take them by moving from this.
    std::vector<Chunk>& ready() { return ready_; }

private:
    void close();
    uint64_t h_ = 0;
    size_t start_ = 0, len_ = 0;
    Sha256 sha_;
    bool uniform_ = true;
    uint8_t first_ = 0;
    std::vector<Chunk> ready_;
};

// The gear table (for tests).
const std::array<uint64_t, 256>& gear();

} // namespace sieve::cdc
