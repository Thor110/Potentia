#include "sieve/corridor.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace sieve {
namespace {
// The corridor's tile size, and its log2. One value for the whole process: the corridor is
// shared by every line, so they cannot disagree about how it is divided.
uint32_t g_books_per_tile = 128;
unsigned g_books_per_tile_bits = 7;
// A LineLoop divides its length by the tile size once, when it is built, and keeps the answer. If
// the tile size changed after that, its stored tile count would be measured in one size and the
// slot it hands back in another, and the two would quietly disagree. Rather than leave that to a
// comment, the first LineLoop built latches the setting, and a later change is refused.
bool g_tile_size_in_use = false;
} // namespace

uint32_t books_per_tile() { return g_books_per_tile; }
unsigned books_per_tile_bits() { return g_books_per_tile_bits; }

void set_books_per_tile(uint32_t n)
{
    if (n < 2 || n > 4096 || (n & (n - 1)))
        throw std::invalid_argument("books per tile must be a power of two between 2 and 4096");
    if (g_tile_size_in_use && n != g_books_per_tile)
        throw std::logic_error("the corridor's tile size cannot change once a line has been built");
    unsigned bits = 0;
    while ((1u << bits) != n) ++bits;
    g_books_per_tile = n;
    g_books_per_tile_bits = bits;
}



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
    g_tile_size_in_use = true; // closes the latch above: the tile size is now baked into tiles_
    tiles_ = units_;
    tiles_ >>= books_per_tile_bits();
    const uint32_t rem = units_.low_bits(books_per_tile_bits());
    if (rem)
    {
        tiles_.add_small(1);
        padding_ = books_per_tile() - rem;
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
    if (slot >= books_per_tile()) throw std::out_of_range("slot outside the tile");
    BigUint i = loop_tile;
    i <<= books_per_tile_bits();
    i.add_small(slot);
    if (i >= units_) return std::nullopt;
    return i;
}

TileIndex LineLoop::tile_of(const BigUint& unit_index)
{
    TileIndex t;
    t.magnitude = unit_index;
    t.magnitude >>= books_per_tile_bits();
    return t;
}

uint32_t LineLoop::slot_of(const BigUint& unit_index) { return unit_index.low_bits(books_per_tile_bits()); }

std::string bearing_of(const BigUint& v, const BigUint& units, int decimals)
{
    const int d = std::max(0, decimals);
    if (units.is_zero()) return d ? "0." + std::string(size_t(d), '0') : "0";
    BigUint scale = BigUint::pow(10, uint64_t(d));
    scale.mul_small(360);
    BigUint q, r;
    BigUint::divmod(BigUint::mul(v, scale), units, q, r);
    std::string digits = q.to_decimal();
    if (d == 0) return digits;
    if (digits.size() <= size_t(d)) digits.insert(0, size_t(d) + 1 - digits.size(), '0');
    return digits.substr(0, digits.size() - size_t(d)) + "." + digits.substr(digits.size() - size_t(d));
}

BigUint unit_at_bearing(const BigUint& a, const BigUint& units, int decimals)
{
    BigUint scale = BigUint::pow(10, uint64_t(std::max(0, decimals)));
    scale.mul_small(360);
    BigUint q, r;
    BigUint::divmod(BigUint::mul(a, units), scale, q, r);
    if (!r.is_zero()) q.add_small(1);
    if (!(q < units)) q = BigUint();
    return q;
}

namespace {
size_t hex_len(const BigUint& x) { return std::max<size_t>(1, (x.bit_length() + 3) / 4); }

// A bearing as typed from its scaled value: "90.10" for 9010 at 2 places.
std::string bearing_text(const BigUint& a, int d)
{
    std::string digits = a.to_decimal();
    if (d == 0) return digits;
    if (digits.size() <= size_t(d)) digits.insert(0, size_t(d) + 1 - digits.size(), '0');
    return digits.substr(0, digits.size() - size_t(d)) + "." + digits.substr(digits.size() - size_t(d));
}

size_t decimal_digits(const std::string& bearing) { return bearing.size() - (bearing.find('.') == std::string::npos ? 0 : 1); }
} // namespace

ShortestPath shortest_path(const BigUint& v, const BigUint& units, int max_decimals, bool prune)
{
    if (!(v < units)) throw std::out_of_range("shortest_path: the unit is past the end of the loop");
    constexpr double kDecimalBits = 3.321928094887362;
    ShortestPath best;
    best.chars = hex_len(v);
    best.bits = 4.0 * double(best.chars);
    // A bearing, then the shorter way round to the unit from where it lands. Compared by length;
    // only the winner is written out (a walk on a line of files is as long as a file).
    // Where the unit stands as a share of the loop, from leading bits (for the estimates).
    auto share = [](const BigUint& x, size_t top) {
        BigUint t = x;
        const size_t drop = top > 64 ? top - 64 : 0;
        t >>= drop;
        const double lo = double(t.low_bits(32));
        t >>= 32;
        return std::ldexp(double(t.low_bits(32)) * 4294967296.0 + lo, int(drop) - int(top));
    };
    const size_t ubits = units.bit_length();
    const bool estimate = prune && ubits > 4096;
    const double at = estimate ? share(v, ubits) / share(units, ubits) : 0;
    auto consider = [&](const BigUint& a, int d, bool near) {
        if (estimate && !near)
        {
            // Far from the unit: its walk is about the gap times the loop, to a few bits. Skip it
            // when even a walk a hex digit shorter than that would not beat the best so far.
            BigUint scale = BigUint::pow(10, uint64_t(d));
            scale.mul_small(360);
            const double alpha = share(a, scale.bit_length()) / share(scale, scale.bit_length());
            double gap = std::fabs(alpha - at);
            gap = std::min(gap, 1.0 - gap);
            if (gap > 1e-9)
            {
                const double hex = (std::log2(gap) + double(ubits)) / 4.0;
                if (bearing_text(a, d).size() + 1 + size_t(std::max(0.0, hex - 1.0)) >= best.chars) return;
            }
        }
        const BigUint lands = unit_at_bearing(a, units, d);
        BigUint fwd, back; // (v - lands) and (lands - v), each mod units
        if (lands < v || lands == v)
        {
            fwd = v;
            fwd -= lands;
            back = units;
            back -= fwd;
        }
        else
        {
            back = lands;
            back -= v;
            fwd = units;
            fwd -= back;
        }
        const std::string bearing = bearing_text(a, d);
        const bool backwards = !fwd.is_zero() && hex_len(back) < hex_len(fwd);
        const size_t walk_len = fwd.is_zero() ? 0 : 1 + hex_len(backwards ? back : fwd);
        if (bearing.size() + walk_len >= best.chars) return;
        best = ShortestPath{};
        best.by_bearing = true;
        best.decimals = d;
        best.bearing = bearing;
        best.back = backwards;
        if (!fwd.is_zero()) best.walk = backwards ? std::move(back) : std::move(fwd);
        best.chars = bearing.size() + walk_len;
        best.bits = double(decimal_digits(bearing)) * kDecimalBits + (walk_len ? 1.0 + 4.0 * double(walk_len - 1) : 0.0);
    };
    for (int d = 0; d <= std::max(0, max_decimals); ++d)
    {
        const BigUint place = BigUint::pow(10, uint64_t(d));
        BigUint scale = place;
        scale.mul_small(360);
        BigUint below, r;
        BigUint::divmod(BigUint::mul(v, scale), units, below, r); // the bearing just below (or on) the unit
        BigUint above = below;
        above.add_small(1);
        consider(below, d, true);
        if (above < scale) consider(above, d, true);
        consider(BigUint(), d, false); // 0: the start, and round the loop backwards from it
        // The nearest bearings of each width (one, two or three whole degrees): a narrower bearing
        // can be worth a longer walk, so each width's nearest to the unit is tried. Within a width
        // the walk grows with the distance, so the nearest below and above, and the ends (for the
        // way round), are all the candidates there are.
        for (uint32_t width = 1; width <= 3; ++width)
        {
            BigUint lo = width == 1 ? BigUint() : BigUint::pow(10, width - 1), hi = width == 3 ? BigUint(360) : BigUint::pow(10, width);
            lo = BigUint::mul(lo, place);
            hi = BigUint::mul(hi, place);
            hi -= BigUint(1); // the width's bearings: [lo, hi]
            for (const BigUint* x : {&below, &above})
                if (*x < lo || hi < *x) consider(*x < lo ? lo : hi, d, false); // (within the width: done above)
            // And its ends, for the walk the other way round the loop.
            consider(lo, d, false);
            consider(hi, d, false);
        }
    }
    if (best.by_bearing) best.written = best.bearing + (best.walk.is_zero() ? "" : (best.back ? "-" : "+") + best.walk.to_hex(0));
    else
    {
        best.written = v.to_hex(0);
        if (best.written.empty()) best.written = "0";
    }
    return best;
}

} // namespace sieve
