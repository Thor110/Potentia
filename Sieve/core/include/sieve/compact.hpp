// Sieve — compact orderings: a line holding only a filter stack's survivors (SPECIFICATIONS §9).
//
// A stack that can rank (sieve/filter.hpp) numbers its N survivors 0..N-1 in positional order.
// Each ordering of the full line has a compact form over those N survivors:
//   positional  survivor k has compact address k (its survivor number)
//   scrambled   survivor k has compact address shuffle(k), a keyed permutation of [0, N)
//   guided      the guided line restricted to survivors ("sieve-restrict-v1", sieve/guided.hpp):
//               its arcs tile [0, 1) with survivors only
// Compact positional and scrambled addresses are hex numbers below N, as wide as N - 1 needs.
//
// "shuffle-sha256-v1", the keyed permutation of [0, N):
//   b = max(2, bit length of N - 1); a value x < 2^b is split into its high ceil(b/2) bits H
//   and low floor(b/2) bits Lo. Eight Feistel rounds r = 0..7: even rounds Lo ^= F(r, H), odd
//   rounds H ^= F(r, Lo), where F(r, v) is the first |target| bits of
//       SHA-256(label || k || 0) || SHA-256(label || k || 1) || ...   (read big-endian)
//   with label = "SIEVE/SHUFFLE/1", then u32le-length-prefixed: the key, the domain, N in
//   lowercase hex, then u32le(r) and v in lowercase hex (length-prefixed), and k a u32le block
//   counter. A result of N or more is permuted again (cycle walking) until it is below N, which
//   takes at most two passes of the permutation on average (2^b <= 2N). N = 1 is the identity.
// The domain is the stack id, so every stack shuffles its survivors differently.
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/filter.hpp"
#include "sieve/guided.hpp"
#include "sieve/sha256.hpp"
#include "sieve/space.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kShuffleVersion = "shuffle-sha256-v1";

class Shuffle
{
public:
    Shuffle(BigUint n, std::string key, std::string domain);
    const BigUint& size() const { return n_; }
    BigUint forward(const BigUint& k) const; // k < size()
    BigUint inverse(const BigUint& j) const; // j < size()

private:
    BigUint permute(BigUint x, bool forward) const; // one pass over [0, 2^b)
    BigUint round_value(uint32_t round, const BigUint& src, size_t bits) const;
    BigUint n_;
    size_t bits_ = 0, lo_bits_ = 0;
    std::string key_, domain_;
    std::string n_hex_; // N in hex, part of every round's input
    Sha256 prefix_;     // the hash state after the input every round shares (label, key, domain, N)
};

// A line's survivors in every ordering. The ranker must outlive this object.
class CompactLine
{
public:
    CompactLine(const Ranker& ranker, const std::string& key, const std::string& stack_id,
                std::shared_ptr<const CharModel> model = nullptr);

    const Ranker& ranker() const { return *ranker_; }
    const BigUint& count() const { return ranker_->count(); }
    size_t hex_width() const { return hex_width_; }

    // Positional and scrambled: a survivor's compact address, and back. index_of throws
    // std::invalid_argument if the unit is not a survivor; unit_at if the index is not below N.
    BigUint index_of(std::span<const uint32_t> unit, AddressMode m) const;
    std::vector<uint32_t> unit_at(const BigUint& index, AddressMode m) const;
    std::string hex_of(const BigUint& index) const;
    BigUint parse(std::string_view hex) const;

    // The sieved guided line, when the line has a model (else null).
    const GuidedLine* guided() const { return guided_.get(); }

private:
    const Ranker* ranker_;
    Shuffle shuffle_;
    size_t hex_width_;
    std::unique_ptr<GuidedLine> guided_;
};

} // namespace sieve
