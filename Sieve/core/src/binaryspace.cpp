// The binary line (binaryspace.hpp): a file and its place on the line, both ways. Everything is
// a hex conversion plus one addition or subtraction of 0101...01, so a file of a few megabytes
// costs a few megabytes of work, not the quadratic cost of a general base conversion.

#include "sieve/binaryspace.hpp"

#include <algorithm>
#include <stdexcept>

namespace sieve {

namespace {

constexpr char kHex[] = "0123456789abcdef";

// "01" written n times: (256^n - 1) / 255.
BigUint ones(uint64_t n)
{
    std::string h;
    h.reserve(size_t(2 * n));
    for (uint64_t i = 0; i < n; ++i) h += "01";
    return n == 0 ? BigUint() : BigUint::from_hex(h);
}

std::string space_id(uint64_t n, const std::string& key)
{
    return "binary/bytes256/L0-" + std::to_string(n) + "/key=" + key + "/" + kBinarySpaceVersion;
}

} // namespace

BinarySpace::BinarySpace(uint64_t max_bytes, std::string key)
    : max_bytes_(max_bytes), key_(std::move(key)), size_(ones(max_bytes + 1)),
      shuffle_(size_, key_, space_id(max_bytes, key_))
{
    if (max_bytes_ == 0) throw std::invalid_argument("the binary line needs a length of at least 1 byte");
    BigUint top = size_;
    top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
}

std::string BinarySpace::id() const { return space_id(max_bytes_, key_); }

std::string BinarySpace::shape() const { return "binary/L0-" + std::to_string(max_bytes_); }

BigUint BinarySpace::count_shorter(uint64_t length) { return ones(length); }

BigUint BinarySpace::index_of(const Bytes& file, AddressMode m) const
{
    if (file.size() > max_bytes_)
        throw std::invalid_argument("the file is " + std::to_string(file.size()) + " bytes, longer than the line's " +
                                    std::to_string(max_bytes_));
    // Its bytes as one number, first byte most significant: its hex dump.
    std::string h;
    h.reserve(file.size() * 2);
    for (uint8_t b : file)
    {
        h += kHex[b >> 4];
        h += kHex[b & 15];
    }
    BigUint v = file.empty() ? BigUint() : BigUint::from_hex(h);
    v += ones(file.size());
    return m == AddressMode::Scrambled ? shuffle_.forward(v) : v;
}

BinarySpace::Bytes BinarySpace::bytes_at(const BigUint& index, AddressMode m) const
{
    if (index >= size_) throw std::out_of_range("binary address beyond the line");
    BigUint v = m == AddressMode::Scrambled ? shuffle_.inverse(index) : index;
    // Its length: the largest L with (256^L - 1) / 255 <= v, which is 256^L <= 255 v + 1.
    BigUint t = v;
    t.mul_small(255);
    t.add_small(1);
    const uint64_t length = uint64_t((t.bit_length() - 1) / 8);
    v -= ones(length);
    Bytes out(size_t(length), 0);
    if (length == 0) return out;
    const std::string h = v.to_hex(size_t(2 * length));
    auto nibble = [](char c) { return uint8_t(c <= '9' ? c - '0' : c - 'a' + 10); };
    for (size_t i = 0; i < out.size(); ++i) out[i] = uint8_t(nibble(h[2 * i]) << 4 | nibble(h[2 * i + 1]));
    return out;
}

BigUint BinarySpace::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= size_) throw std::out_of_range("binary address beyond the line");
    return v;
}

} // namespace sieve
