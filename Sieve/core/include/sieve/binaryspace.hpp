// Sieve — the binary line: every file up to N bytes long, in one loop (SPECIFICATIONS §12.1,
// "binary-v1").
//
// A unit of this line is a byte string of any length from 0 to N: every file that size or
// smaller, the empty file included. They are numbered shortest first and, within a length, by
// their bytes read as one big-endian number, so
//
//     positional index of a file of L bytes = (files shorter than L) + (its bytes as a number)
//     files shorter than L                  = 1 + 256 + ... + 256^(L-1) = (256^L - 1) / 255
//
// and (256^L - 1) / 255 is, in hex, "01" written L times. So a file's address is its own hex
// dump plus 0101...01, one "01" for each of its bytes, and every conversion here is a hex
// conversion: linear in the length, however long the file. The line holds
//
//     M = (256^(N+1) - 1) / 255 files, which is "01" written N + 1 times,
//
// and scrambled order is the positional index through shuffle-sha256-v1 over [0, M), keyed with
// the line's key and domain-separated by the space id, as every other line's is. Unlike a
// bytes256 line of fixed length (§12.1a), nothing is padded: a file of 3 bytes and the same
// three bytes followed by a NUL are two different files with two different addresses.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/compact.hpp"
#include "sieve/space.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kBinarySpaceVersion = "binary-v1";

class BinarySpace
{
public:
    using Bytes = std::vector<uint8_t>;

    // Every file of 0 to max_bytes bytes (max_bytes >= 1).
    BinarySpace(uint64_t max_bytes, std::string key);

    uint64_t max_bytes() const { return max_bytes_; }
    const std::string& key() const { return key_; }
    const BigUint& size() const { return size_; } // M, the files on the line
    size_t hex_width() const { return hex_width_; }
    // "binary/bytes256/L0-<N>/key=<key>/binary-v1"
    std::string id() const;
    // The content's shape, as a titled line names it: "binary/L0-<N>".
    std::string shape() const;

    // How many files are shorter than `length` bytes: (256^length - 1) / 255.
    static BigUint count_shorter(uint64_t length);

    BigUint index_of(const Bytes& file, AddressMode m) const; // throws if longer than max_bytes
    Bytes bytes_at(const BigUint& index, AddressMode m) const; // index < size()
    // The file at a positional place on binary-v1, whatever the line's length (the length comes from
    // the number itself): what bytes_at does in positional order, for a worker that must not hold the
    // line, which can be rebuilt meanwhile.
    static Bytes file_at_place(const BigUint& place);
    std::string hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }
    BigUint parse(std::string_view hex) const; // range-checked

private:
    uint64_t max_bytes_;
    std::string key_;
    BigUint size_;
    size_t hex_width_ = 1;
    Shuffle shuffle_;
};

} // namespace sieve
