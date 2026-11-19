#include "sieve/space.hpp"

#include "sieve/compact.hpp"
#include "sieve/sha256.hpp"

#include <algorithm>
#include <stdexcept>
#include <string>
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
    : symbols_id_(std::move(symbols_id)), base_(base), length_(unit_length), min_length_(unit_length), key_(std::move(key))
{
    if (unit_length == 0) throw std::invalid_argument("unit length must be at least 1");
    if (base < 2) throw std::invalid_argument("a space needs at least 2 symbols");
    size_ = BigUint::pow(base, unit_length);
    // Largest address is size - 1.
    bits_ = size_.is_power_of_two() ? size_.bit_length() - 1 : size_.bit_length();
    hex_width_ = (bits_ + 3) / 4;
    if (hex_width_ == 0) hex_width_ = 1;
}

Space::Space(const Alphabet& alphabet, uint32_t min_length, uint32_t max_length, std::string key)
    : Space(alphabet, max_length, std::move(key))
{
    if (min_length == 0 || min_length > max_length) throw std::invalid_argument("a range of lengths runs from 1 or more up to its maximum");
    min_length_ = min_length;
    if (!is_range()) return;
    size_ = count_shorter(length_ + 1);
    BigUint top = size_;
    top -= BigUint(1);
    bits_ = top.bit_length();
    hex_width_ = std::max<size_t>(1, (bits_ + 3) / 4);
    shuffle_ = std::make_unique<Shuffle>(size_, key_, id());
}

Space::~Space() = default;
Space::Space(Space&&) noexcept = default;
Space& Space::operator=(Space&&) noexcept = default;

Space::Space(const Space& o)
    : alphabet_(o.alphabet_), symbols_id_(o.symbols_id_), base_(o.base_), length_(o.length_), min_length_(o.min_length_),
      shuffle_(o.shuffle_ ? std::make_unique<Shuffle>(*o.shuffle_) : nullptr), key_(o.key_), size_(o.size_), bits_(o.bits_),
      hex_width_(o.hex_width_)
{
}

Space& Space::operator=(const Space& o)
{
    if (this != &o)
    {
        Space c(o);
        *this = std::move(c);
    }
    return *this;
}

BigUint Space::count_shorter(uint32_t length) const
{
    if (length <= min_length_) return {};
    // (B^length - B^min) / (B - 1): the units of min .. length - 1 symbols.
    BigUint n = BigUint::pow(base_, length);
    n -= BigUint::pow(base_, min_length_);
    if (base_ > 2) n.divmod_small(base_ - 1);
    return n;
}

void Space::check_unit(const Digits& unit) const
{
    if (unit.size() < min_length_ || unit.size() > length_) throw std::invalid_argument("digit vector has the wrong length");
    for (uint32_t d : unit)
        if (d >= base_) throw std::invalid_argument("digit out of range for this space");
}

BigUint Space::place_of(const Digits& unit) const
{
    check_unit(unit);
    BigUint v = BigUint::from_digits(unit, base_);
    v += count_shorter(uint32_t(unit.size()));
    return v;
}

Space::Digits Space::unit_at_place(const BigUint& place) const
{
    if (!(place < size_)) throw std::out_of_range("place outside this space");
    // Its length: the last whose shorter units do not pass the place.
    uint32_t L = min_length_;
    while (L < length_ && !(place < count_shorter(L + 1))) ++L;
    BigUint v = place;
    v -= count_shorter(L);
    return v.to_digits(base_, L);
}

const Alphabet& Space::alphabet() const
{
    if (!alphabet_) throw std::logic_error("space '" + symbols_id_ + "' is not a text space");
    return *alphabet_;
}

std::string Space::id() const
{
    if (is_range())
        return symbols_id_ + "/L" + std::to_string(min_length_) + "-" + std::to_string(length_) + "/key=" + key_ + "/" + kShuffleVersion;
    return symbols_id_ + "/L" + std::to_string(length_) + "/key=" + key_ + "/" + kScrambleVersion;
}

Space::Digits Space::digits_of(std::u32string_view unit) const
{
    const Alphabet& alpha = alphabet();
    if (unit.size() < min_length_ || unit.size() > length_)
        throw std::invalid_argument("unit has " + std::to_string(unit.size()) + " symbols, expected " +
                                    (is_range() ? std::to_string(min_length_) + " to " : std::string()) + std::to_string(length_));
    Digits d(unit.size());
    for (uint32_t i = 0; i < d.size(); ++i)
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
    if (unit.size() < min_length_ || unit.size() > length_) throw std::invalid_argument("digit vector has the wrong length");
    for (uint32_t d : unit)
        if (d >= base_) throw std::invalid_argument("digit out of range");
    std::u32string s(unit.size(), U' ');
    for (uint32_t i = 0; i < unit.size(); ++i) s[i] = alpha.symbol(unit[i]);
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
    if (is_range()) throw std::logic_error("a range of lengths is scrambled by its shuffle (address_digits)");
    if (d.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t n = base_;
    const size_t h = length_ / 2;
    for (uint32_t r = 0; r < kScrambleRounds; ++r)
    {
        const bool even = (r % 2) == 0;
        const size_t src_off = even ? 0 : h, src_len = even ? h : length_ - h;
        const size_t dst_off = even ? h : 0, dst_len = even ? length_ - h : h;
        const Digits f = round_function(r, d.data() + src_off, src_len, dst_len);
        for (size_t j = 0; j < dst_len; ++j) d[dst_off + j] = uint32_t((uint64_t(d[dst_off + j]) + f[j]) % n); // (in 64 bits: a base above 2^31 wrapped)
    }
    return d;
}

Space::Digits Space::unscramble(Digits d) const
{
    if (is_range()) throw std::logic_error("a range of lengths is scrambled by its shuffle (unit_of_address)");
    if (d.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    const uint32_t n = base_;
    const size_t h = length_ / 2;
    for (uint32_t r = kScrambleRounds; r-- > 0;)
    {
        const bool even = (r % 2) == 0;
        const size_t src_off = even ? 0 : h, src_len = even ? h : length_ - h;
        const size_t dst_off = even ? h : 0, dst_len = even ? length_ - h : h;
        const Digits f = round_function(r, d.data() + src_off, src_len, dst_len);
        for (size_t j = 0; j < dst_len; ++j) d[dst_off + j] = uint32_t((uint64_t(d[dst_off + j]) + n - f[j]) % n);
    }
    return d;
}

Space::Digits Space::address_digits(const Digits& unit, AddressMode mode) const
{
    if (is_range())
    {
        const BigUint place = place_of(unit);
        return (mode == AddressMode::Scrambled ? shuffle_->forward(place) : place).to_digits(base_, address_length());
    }
    if (unit.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    for (uint32_t d : unit)
        if (d >= base_) throw std::invalid_argument("digit out of range for this space");
    return mode == AddressMode::Scrambled ? scramble(unit) : unit;
}

Space::Digits Space::unit_of_address(Digits address, AddressMode mode) const
{
    if (is_range())
    {
        if (address.size() != address_length()) throw std::invalid_argument("digit vector has the wrong length");
        const BigUint v = BigUint::from_digits(address, base_);
        if (!(v < size_)) throw std::out_of_range("address is outside this space");
        return unit_at_place(mode == AddressMode::Scrambled ? shuffle_->inverse(v) : v);
    }
    if (address.size() != length_) throw std::invalid_argument("digit vector has the wrong length");
    return mode == AddressMode::Scrambled ? unscramble(std::move(address)) : address;
}

std::string Space::hex_of(const Digits& address) const
{
    return BigUint::from_digits(address, base_).to_hex(hex_width_);
}

double Space::fraction_of(const Digits& address) const
{
    if (is_range())
    {
        // place / size, from the top 53 bits of each (display only)
        BigUint v = BigUint::from_digits(address, base_), n = size_;
        const size_t shift = n.bit_length() > 53 ? n.bit_length() - 53 : 0;
        v >>= shift, n >>= shift;
        return double(std::stoull(v.to_decimal())) / double(std::stoull(n.to_decimal()));
    }
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
    if (v >= size_)
        throw std::out_of_range("address is outside this space (must be below " +
                                (is_range() ? size_.to_hex() + " hex" : std::to_string(base_) + "^" + std::to_string(length_)) + ")");
    return v.to_digits(base_, address_length());
}

Space::Digits Space::unit_at(std::string_view hex, AddressMode mode) const
{
    return unit_of_address(parse_address(hex), mode);
}

Space::Digits Space::step_address(Digits a, int64_t offset) const
{
    if (is_range())
    {
        // A number below the size, stepped round the loop.
        if (a.size() != address_length()) throw std::invalid_argument("digit vector has the wrong length");
        BigUint v = BigUint::from_digits(a, base_);
        BigUint step(offset >= 0 ? uint64_t(offset) : 0 - uint64_t(offset));
        step = BigUint::mod(step, size_);
        if (offset >= 0)
        {
            v += step;
            if (!(v < size_)) v -= size_;
        }
        else if (v < step)
        {
            v += size_;
            v -= step;
        }
        else v -= step;
        return v.to_digits(base_, address_length());
    }
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
    if (space.is_range()) return space.unit_of_address(v.to_digits(space.base(), space.address_length()), mode);
    Space::Digits a = v.to_digits(space.base(), space.unit_length());
    return mode == AddressMode::Scrambled ? space.unscramble(std::move(a)) : a;
}

} // namespace sieve
