// Sieve — "sieve-weights-v1": a model file (safetensors) coded under a prior over its weights,
// and rebuilt from that byte for byte (IDEAS §15, §16.8b; SPECIFICATIONS §12.0b).
//
// The noise gradient as a prior, first version: the weights of one kind (the tensors whose names
// differ only in their layer number, of one type) share one table of how often each 16-bit value
// occurs, and each tensor's values are coded under its kind's table, so a likely value costs few
// bits and an unlikely one many. It is lossless: the file comes back exactly, checked by its
// SHA-256. On SmolLM2-360M-Instruct it gives about 477 MB for 724 MB (xz: 513 MB).
//
// The format, all integers little-endian, "varint" an unsigned LEB128 number:
//   "sieve-weights-v1\n"
//   varint S, then S bytes: the file's start (its 8-byte length and JSON), as it was
//   varint G, then G tables, one for each kind of 16-bit float tensor (BF16, F16), in the order
//     their first tensor comes in the file: varint k, then k pairs (varint symbol gap, varint
//     frequency), symbols increasing (the first gap is the symbol itself), the frequencies
//     summing to 2^24
//   each tensor, in the order of its bytes in the file: a 16-bit float tensor as varint W and W
//     32-bit words (its rANS stream); any other tensor as its bytes, as they are
//   32 bytes: the SHA-256 of the whole file
// A kind is a tensor's name with "model.layers.<n>." written "model.layers.*.", and its type.
//
// The table of a kind (quantise): from the counts c_i of its k distinct values over N values,
// f_i = 1 + floor(c_i * (2^24 - k) / N), and what is left of 2^24 added to the most frequent value
// (the lowest such value, on a tie). Every value present has a frequency of at least 1.
//
// The coder (rANS, 64-bit state, 32-bit words, 24-bit frequencies): with L = 2^31, a symbol's
// frequency f and the sum b of the frequencies of the symbols below it, the values are coded
// from the last to the first, from x = L:
//   if x >= ((L >> 24) << 32) * f: put the low 32 bits of x out, x >>= 32
//   x = ((x / f) << 24) + (x mod f) + b
// and the stream is x's low word, its high word, then the words put out, last first. Reading:
// x is the first two words; for each value, the symbol whose range [b, b + f) holds x mod 2^24,
// then x = f * (x >> 24) + (x mod 2^24) - b, and while x < L, x = (x << 32) | the next word. At
// the end x is L and every word has been read.
#pragma once

#include "sieve/sha256.hpp"

#include <cstdint>
#include <istream>
#include <ostream>
#include <string>
#include <vector>

namespace sieve::weights {

inline constexpr const char* kVersion = "sieve-weights-v1";
inline constexpr uint32_t kScaleBits = 24;

// A kind's table, from its counts (65,536 of them, one a 16-bit value): the frequencies, 0 for a
// value that never occurs, summing to 2^24. Throws if there are no values.
std::vector<uint32_t> quantise(const std::vector<uint64_t>& counts);

// One tensor's values coded under a table, and back (n values).
std::vector<uint32_t> encode(const std::vector<uint16_t>& values, const std::vector<uint32_t>& freq);
std::vector<uint16_t> decode(const std::vector<uint32_t>& words, const std::vector<uint32_t>& freq, uint64_t n);

// A kind's name: the tensor's name with its layer number taken out.
std::string kind_of(const std::string& name);

struct PackReport
{
    uint64_t in_bytes = 0, out_bytes = 0;
    uint64_t start_bytes = 0, table_bytes = 0, coded_bytes = 0, raw_bytes = 0; // what the output is made of
    size_t kinds = 0, coded_tensors = 0, raw_tensors = 0;
    Sha256::Digest sha256{};
};
// Codes a safetensors file (`in` must be seekable) into `out`.
PackReport pack(std::istream& in, std::ostream& out);

struct UnpackReport
{
    uint64_t bytes = 0;
    Sha256::Digest sha256{};
};
// Rebuilds the file from a sieve-weights-v1 stream. Throws std::invalid_argument if the stream is
// malformed, or if what it rebuilds is not the file it was made from (its SHA-256).
UnpackReport unpack(std::istream& in, std::ostream& out);

} // namespace sieve::weights
