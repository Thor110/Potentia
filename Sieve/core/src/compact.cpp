#include "sieve/compact.hpp"

#include "sieve/sha256.hpp"

#include <stdexcept>

namespace sieve {

namespace {

void put_str(std::string& b, std::string_view s)
{
    const uint32_t n = uint32_t(s.size());
    const char c[4] = {char(n), char(n >> 8), char(n >> 16), char(n >> 24)};
    b.append(c, 4);
    b.append(s);
}

BigUint mask_low(const BigUint& x, size_t bits)
{
    BigUint m(1);
    m <<= bits;
    return BigUint::mod(x, m);
}

} // namespace

Shuffle::Shuffle(BigUint n, std::string key, std::string domain) : n_(std::move(n)), key_(std::move(key)), domain_(std::move(domain))
{
    if (n_.is_zero()) throw std::invalid_argument("nothing to shuffle");
    BigUint top = n_;
    top -= BigUint(1);
    bits_ = std::max<size_t>(2, top.bit_length());
    lo_bits_ = bits_ / 2;
}

BigUint Shuffle::round_value(uint32_t round, const BigUint& src, size_t bits) const
{
    std::string msg = "SIEVE/SHUFFLE/1";
    put_str(msg, key_);
    put_str(msg, domain_);
    put_str(msg, n_.to_hex());
    const char r[4] = {char(round), char(round >> 8), char(round >> 16), char(round >> 24)};
    msg.append(r, 4);
    put_str(msg, src.to_hex());
    Sha256 prefix;
    prefix.update(msg);
    BigUint out;
    size_t have = 0;
    for (uint32_t k = 0; have < bits; ++k)
    {
        Sha256 h = prefix;
        h.update_u32le(k);
        const auto d = h.finish();
        for (uint8_t byte : d)
        {
            out <<= 8;
            out.add_small(byte);
        }
        have += 256;
    }
    out >>= have - bits;
    return out;
}

BigUint Shuffle::permute(BigUint x, bool forward) const
{
    const size_t hi_bits = bits_ - lo_bits_;
    BigUint lo = mask_low(x, lo_bits_), hi = x;
    hi >>= lo_bits_;
    for (uint32_t i = 0; i < 8; ++i)
    {
        const uint32_t r = forward ? i : 7 - i;
        if (r % 2 == 0) lo ^= round_value(r, hi, lo_bits_);
        else hi ^= round_value(r, lo, hi_bits);
    }
    hi <<= lo_bits_;
    hi += lo;
    return hi;
}

BigUint Shuffle::forward(const BigUint& k) const
{
    if (k >= n_) throw std::out_of_range("index beyond the shuffle");
    if (n_ == BigUint(1)) return k;
    BigUint x = permute(k, true);
    while (x >= n_) x = permute(x, true);
    return x;
}

BigUint Shuffle::inverse(const BigUint& j) const
{
    if (j >= n_) throw std::out_of_range("index beyond the shuffle");
    if (n_ == BigUint(1)) return j;
    BigUint x = permute(j, false);
    while (x >= n_) x = permute(x, false);
    return x;
}

CompactLine::CompactLine(const Ranker& ranker, const std::string& key, const std::string& stack_id,
                         std::shared_ptr<const CharModel> model)
    : ranker_(&ranker), shuffle_(ranker.count().is_zero() ? BigUint(1) : ranker.count(), key, stack_id)
{
    if (ranker.count().is_zero()) throw std::invalid_argument("no unit survives the filters");
    BigUint top = ranker.count();
    top -= BigUint(1);
    hex_width_ = std::max<size_t>(1, (top.bit_length() + 3) / 4);
    if (model) guided_ = std::make_unique<GuidedLine>(std::move(model), ranker.length(), &ranker);
}

BigUint CompactLine::index_of(std::span<const uint32_t> unit, AddressMode m) const
{
    if (!ranker_->accepts(unit)) throw std::invalid_argument("the unit does not pass the filters");
    const BigUint k = ranker_->rank(unit);
    return m == AddressMode::Scrambled ? shuffle_.forward(k) : k;
}

std::vector<uint32_t> CompactLine::unit_at(const BigUint& index, AddressMode m) const
{
    if (index >= count()) throw std::out_of_range("compact address beyond the survivors");
    return ranker_->unrank(m == AddressMode::Scrambled ? shuffle_.inverse(index) : index);
}

std::string CompactLine::hex_of(const BigUint& index) const { return index.to_hex(hex_width_); }

BigUint CompactLine::parse(std::string_view hex) const
{
    const BigUint v = BigUint::from_hex(hex);
    if (v >= count()) throw std::out_of_range("compact address beyond the survivors (" + count().to_decimal() + ")");
    return v;
}

} // namespace sieve
