#include "sieve/corridor.hpp"

#include <stdexcept>
#include <utility>

namespace sieve {

TileIndex TileIndex::of(int64_t t)
{
    TileIndex r;
    r.negative = t < 0;
    r.magnitude = BigUint(t < 0 ? 0 - uint64_t(t) : uint64_t(t));
    return r;
}

TileIndex& TileIndex::operator+=(int64_t delta)
{
    if (delta == 0) return *this;
    const BigUint d(delta < 0 ? 0 - uint64_t(delta) : uint64_t(delta));
    const bool d_negative = delta < 0;
    if (negative == d_negative) magnitude += d;
    else if (magnitude >= d) magnitude -= d;
    else
    {
        BigUint m = d;
        m -= magnitude;
        magnitude = m;
        negative = d_negative;
    }
    if (magnitude.is_zero()) negative = false;
    return *this;
}

std::string TileIndex::to_decimal() const { return (negative ? "-" : "") + magnitude.to_decimal(); }

TileIndex TileIndex::parse(std::string_view dec)
{
    TileIndex t;
    if (!dec.empty() && dec[0] == '-')
    {
        t.negative = true;
        dec.remove_prefix(1);
    }
    t.magnitude = BigUint::from_decimal(dec);
    if (t.magnitude.is_zero()) t.negative = false;
    return t;
}

LineLoop::LineLoop(BigUint units) : units_(std::move(units))
{
    if (units_.is_zero()) throw std::invalid_argument("a line needs at least one unit");
    tiles_ = units_;
    tiles_ >>= kBooksPerTileBits;
    const uint32_t rem = units_.low_bits(kBooksPerTileBits);
    if (rem)
    {
        tiles_.add_small(1);
        padding_ = kBooksPerTile - rem;
    }
}

BigUint LineLoop::loop_tile(const TileIndex& t) const
{
    const BigUint r = BigUint::mod(t.magnitude, tiles_);
    if (!t.negative || r.is_zero()) return r;
    BigUint back = tiles_;
    back -= r;
    return back;
}

std::optional<BigUint> LineLoop::unit_index(const BigUint& loop_tile, uint32_t slot) const
{
    if (slot >= kBooksPerTile) throw std::out_of_range("slot outside the tile");
    BigUint i = loop_tile;
    i <<= kBooksPerTileBits;
    i.add_small(slot);
    if (i >= units_) return std::nullopt;
    return i;
}

TileIndex LineLoop::tile_of(const BigUint& unit_index)
{
    TileIndex t;
    t.magnitude = unit_index;
    t.magnitude >>= kBooksPerTileBits;
    return t;
}

uint32_t LineLoop::slot_of(const BigUint& unit_index) { return unit_index.low_bits(kBooksPerTileBits); }

} // namespace sieve
