#include "sieve/space.hpp"

#include "sieve/sha256.hpp"

#include <stdexcept>
#include <utility>

namespace sieve {

const char* to_string(AddressMode m) { return m == AddressMode::Positional ? "positional" : "scrambled"; }

AddressMode address_mode_from_string(std::string_view s)
{
    if (s == "positional") return AddressMode::Positional;
    if (s == "scrambled") return AddressMode::Scrambled;
    throw std::invalid_argument("unknown address mode '" + std::string(s) + "' (positional|scrambled)");
}

Space::Space(const Alphabet& alphabet, uint32_t unit_length, std::string key)
    : Space(alphabet.id(), alphabet.size(), unit_length, std::move(key))
{
    alphabet_ = &alphabet;
}

Space::Space(std::string symbols_id, uint32_t base, uint32_t unit_length, std::string key)
    : symbols_id_(std::move(symbols_id)), base_(base), length_(unit_length), key_(std::move(key))
{
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
    if (base < 2) throw std::invalid_argument("a space needs at least 2 symbols");
    size_ = BigUint::pow(base, unit_length);
    // Largest address is size - 1.
    bits_ = size_.is_power_of_two() ? size_.bit_length() - 1 : size_.bit_length();
    hex_width_ = (bits_ + 3) / 4;
    if (hex_width_ == 0) hex_width_ = 1;
}

const Alphabet& Space::alphabet() const
{
    if (!alphabet_) throw std::logic_error("space '" + symbols_id_ + "' is not a text space");
    return *alphabet_;
}

std::string Space::id() const
{
    return symbols_id_ + "/L" + std::to_string(length_) + "/key=" + key_ + "/" + kScrambleVersion;
}

Space::Digits Space::digits_of(std::u32string_view unit) const
{
    const Alphabet& alpha = alphabet();
    if (unit.size() != length_)
        throw std::invalid_argument("unit has " + std::to_string(unit.size()) + " symbols, expected " +
                                    std::to_string(length_));
    Digits d(length_);
    for (uint32_t i = 0; i < length_; ++i)
    {
        auto digit = alpha.digit_of(unit[i]);
        if (!digit) throw std::invalid_argument("symbol at position " + std::to_string(i) + " is not in alphabet " + symbols_id_);
        d[i] = *digit;
    }
    return d;
}

std::u32string Space::text_of(const Digits& unit) const
{
    const Alphabet& alpha = alphabet();
    if (unit.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    for (uint32_t d : unit)
        if (d >= base_) throw std::invalid_argument("digit out of range");
    std::u32string s(length_, U' ');
    for (uint32_t i = 0; i < length_; ++i) s[i] = alpha.symbol(unit[i]);
    return s;
}

// Round function F(round, src) -> `count` digits in [0, N).
// Stream block k = SHA-256( "POTENTIA/FEISTEL/1" || u32le(len key) || key || u32le(round) || u32le(N)
//                           || u32le(L) || u32le(len src) || u32le(src[0]) ... || u32le(k) ).
// Each 32-byte block yields eight little-endian u32 words; digit = word mod N.
Space::Digits Space::round_function(uint32_t round, const uint32_t* src, size_t src_len, size_t count) const
{
    const uint32_t n = base_;
    Digits out;
    out.reserve(count);
    // The common input, built in one buffer and hashed once; each output block then clones the
    // hash state and adds only its block counter.
    auto put_u32 = [](std::string& b, uint32_t v) {
        const char c[4] = {char(v), char(v >> 8), char(v >> 16), char(v >> 24)};
        b.append(c, 4);
    };
    std::string msg;
    msg.reserve(40 + key_.size() + 4 * src_len);
    // Domain-separation label: a frozen constant of feistel-sha256-v1 (named before the project
    // became Sieve). Changing it would change every scrambled address, under every key.
    msg += "POTENTIA/FEISTEL/1";
    put_u32(msg, static_cast<uint32_t>(key_.size()));
    msg += key_;
    put_u32(msg, round);
    put_u32(msg, n);
    put_u32(msg, length_);
    put_u32(msg, static_cast<uint32_t>(src_len));
    for (size_t i = 0; i < src_len; ++i) put_u32(msg, src[i]);
    Sha256 prefix;
    prefix.update(msg);

    for (uint32_t k = 0; out.size() < count; ++k)
    {
        Sha256 h = prefix;
        h.update_u32le(k);
        const auto digest = h.finish();
        for (int w = 0; w < 8 && out.size() < count; ++w)
        {
            const uint32_t word = uint32_t(digest[4 * w]) | (uint32_t(digest[4 * w + 1]) << 8) |
                                  (uint32_t(digest[4 * w + 2]) << 16) | (uint32_t(digest[4 * w + 3]) << 24);
            out.push_back(word % n);
        }
    }
    return out;
}

// Unbalanced digit-Feistel: left = digits [0, h), right = digits [h, L), h = floor(L/2).
// Even rounds add F(round, left) to right; odd rounds add F(round, right) to left (mod N).
// Each round is invertible by subtraction, so the whole map is a permutation of the space.
Space::Digits Space::scramble(Digits d) const
{
    if (d.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t n = base_;
    const size_t h = length_ / 2;
    for (uint32_t r = 0; r < kScrambleRounds; ++r)
    {
        const bool even = (r % 2) == 0;
        const size_t src_off = even ? 0 : h, src_len = even ? h : length_ - h;
        const size_t dst_off = even ? h : 0, dst_len = even ? length_ - h : h;
        const Digits f = round_function(r, d.data() + src_off, src_len, dst_len);
        for (size_t j = 0; j < dst_len; ++j) d[dst_off + j] = (d[dst_off + j] + f[j]) % n;
    }
    return d;
}

Space::Digits Space::unscramble(Digits d) const
{
    if (d.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t n = base_;
    const size_t h = length_ / 2;
    for (uint32_t r = kScrambleRounds; r-- > 0;)
    {
        const bool even = (r % 2) == 0;
        const size_t src_off = even ? 0 : h, src_len = even ? h : length_ - h;
        const size_t dst_off = even ? h : 0, dst_len = even ? length_ - h : h;
        const Digits f = round_function(r, d.data() + src_off, src_len, dst_len);
        for (size_t j = 0; j < dst_len; ++j) d[dst_off + j] = (d[dst_off + j] + n - f[j]) % n;
    }
    return d;
}

Space::Digits Space::address_digits(const Digits& unit, AddressMode mode) const
{
    if (unit.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    for (uint32_t d : unit)
        if (d >= base_) throw std::invalid_argument("digit out of range for this space");
    return mode == AddressMode::Scrambled ? scramble(unit) : unit;
}

Space::Digits Space::unit_of_address(Digits address, AddressMode mode) const
{
    if (address.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    return mode == AddressMode::Scrambled ? unscramble(std::move(address)) : address;
}

std::string Space::hex_of(const Digits& address) const
{
    return BigUint::from_digits(address, base_).to_hex(hex_width_);
}

double Space::fraction_of(const Digits& address) const
{
    double f = 0.0, scale = 1.0;
    const double n = base_;
    // Enough leading digits to fill a double, however small the base.
    for (size_t i = 0; i < address.size() && scale > 1e-18; ++i)
    {
        scale /= n;
        f += address[i] * scale;
    }
    return f;
}

std::string Space::address_of(const Digits& unit, AddressMode mode) const
{
    return hex_of(address_digits(unit, mode));
}

Space::Digits Space::parse_address(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("address is outside this space (must be below " +
                                            std::to_string(base_) + "^" + std::to_string(length_) + ")");
    return v.to_digits(base_, length_);
}

Space::Digits Space::unit_at(std::string_view hex, AddressMode mode) const
{
    return unit_of_address(parse_address(hex), mode);
}

Space::Digits Space::step_address(Digits a, int64_t offset) const
{
    if (a.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    // Add |offset| to (or subtract it from) the address digits, least significant digit last,
    // with the carry or borrow wrapping around the loop.
    const bool forward = offset >= 0;
    uint64_t step = forward ? static_cast<uint64_t>(offset) : 0 - static_cast<uint64_t>(offset);
    uint64_t carry = 0;
    for (size_t i = length_; i-- > 0 && (step || carry);)
    {
        const uint64_t part = step % base_;
        step /= base_;
        if (forward)
        {
            const uint64_t v = uint64_t(a[i]) + part + carry;
            a[i] = static_cast<uint32_t>(v % base_);
            carry = v / base_;
        }
        else
        {
            const int64_t v = int64_t(a[i]) - int64_t(part) - int64_t(carry);
            carry = v < 0 ? 1 : 0;
            a[i] = static_cast<uint32_t>(v < 0 ? v + int64_t(base_) : v);
        }
    }
    // Remaining step/carry beyond the most significant digit wraps around the loop and is discarded.
    return a;
}

Space::Digits Space::neighbour(const Digits& unit, AddressMode mode, int64_t offset) const
{
    return unit_of_address(step_address(address_digits(unit, mode), offset), mode);
}

double Space::fraction(const Digits& unit, AddressMode mode) const
{
    return fraction_of(address_digits(unit, mode));
}

Space::Digits unit_at_fraction(const Space& space, uint64_t numerator, uint32_t decimals, AddressMode mode)
{
    BigUint limit(1);
    for (uint32_t i = 0; i < decimals; ++i) limit.mul_small(10);
    if (BigUint(numerator) >= limit) throw std::invalid_argument("fraction must be below 1 (100%)");
    // floor(size * numerator / 10^decimals)
    if (numerator >> 32) throw std::invalid_argument("too many decimal places");
    BigUint v = space.size();
    v.mul_small(static_cast<uint32_t>(numerator));
    for (uint32_t i = 0; i < decimals; ++i) v.divmod_small(10);
    Space::Digits a = v.to_digits(space.base(), space.unit_length());
    return mode == AddressMode::Scrambled ? space.unscramble(std::move(a)) : a;
}

} // namespace sieve
