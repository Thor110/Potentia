// Sieve — entropy-ordered ("guided") addresses (SPECIFICATIONS §4.2, milestone M3).
//
// An exact arithmetic code over a pinned model. With T = 2^16 and the model's table F_i for
// the history u_0..u_{i-1} (cumulative C_i), a unit u of length L gets the interval
//     low_0 = 0, w_0 = 1
//     low_{i+1} = low_i * T + C_i(u_i) * w_i,     w_{i+1} = w_i * F_i(u_i)
//     I(u) = [low_L, low_L + w_L)  inside  [0, 2^S),  S = 16 L.
// The intervals of all N^L units tile [0, 2^S) exactly, in positional (dictionary) order, and
// each is as wide as the model thinks the unit is likely. So the line [0, 1) is divided into
// arcs of width P(u): probable units take wide arcs, noise takes hairlines.
//
// A point is any binary fraction in [0, 1). Every point lies in exactly one unit's arc.
// A unit's guided address is the shortest aligned binary block [m/2^b, (m+1)/2^b) that fits
// inside its arc (the lowest one, if two fit), written as the b-bit fraction m/2^b. So
//     -log2 P(u)  <=  b  <  -log2 P(u) + 2,
// the addresses form a prefix code, and an address's length is an honest measure of the
// unit's information: for English text roughly 2 bits per character instead of 4.75.
// Addresses are written as ceil(b/4) hexadecimal fraction digits ("8" is one half, "c" three
// quarters). Reading accepts any hex fraction: it is a point, and decodes to the unit whose arc
// contains it.
//
// Guided decoding is sampling from the model: a random point decodes to text the model finds
// plausible. Fluency is therefore evidence of nothing (§4.2 warning).
#pragma once

#include "sieve/biguint.hpp"
#include "sieve/model.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kGuidedVersion = "guided-ac-v1";

class GuidedLine
{
public:
    using Digits = std::vector<uint32_t>;

    GuidedLine(std::shared_ptr<const CharModel> model, uint32_t unit_length);

    const CharModel& model() const { return *model_; }
    uint32_t unit_length() const { return length_; }
    size_t scale_bits() const { return scale_; } // S = 16 L: points are integers in [0, 2^S)

    struct Interval
    {
        BigUint low, width;
    };
    Interval interval(const Digits& unit) const;

    struct Code
    {
        BigUint point;  // the address as a point in [0, 2^S)
        size_t bits;    // its length in bits (0 for the point 0)
        std::string hex;
    };
    // The unit's guided address (see above).
    Code code(const Digits& unit) const;
    Code code_of(const Interval& iv) const;

    // The unit whose arc contains the point.
    Digits unit_at(const BigUint& point) const;
    // Hex fraction digits -> point (digits beyond S bits are dropped). Throws on bad hex.
    BigUint point_of(std::string_view hex) const;
    // The point's first `bits` bits as ceil(bits/4) hex fraction digits ("0" for 0 bits).
    std::string hex_of(const BigUint& point, size_t bits) const;

    // point + offset * 2^(S - depth), wrapping around the line.
    BigUint step(const BigUint& point, int64_t offset, uint32_t depth) const;

    // DISPLAY ONLY (floating point): position along the line in [0, 1), and -log2 of a width.
    double fraction(const BigUint& point) const;
    double information_bits(const Interval& iv) const;

private:
    std::shared_ptr<const CharModel> model_;
    uint32_t length_;
    size_t scale_;
};

} // namespace sieve
