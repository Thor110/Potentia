// Sieve — the corridor at a tile size other than its default of 128 (REVIEW, the first pass, item 6).
//
// The size is set once at start-up and latched by the first line built (corridor.hpp), so it
// cannot be changed within sieve_tests, which builds lines at 128 throughout: this program sets
// the size it is given (2 and 4096, the two ends, are run by ctest) before anything else, and
// checks the corridor's arithmetic at it. No address depends on the size; only where a unit
// stands (its tile and slot) does.
//
//   sieve_tile_tests N        N a power of two, 2 to 4096

#include "sieve/biguint.hpp"
#include "sieve/corridor.hpp"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <vector>

using namespace sieve;

static int failures = 0, checks = 0;
#define CHECK(c)                                                                \
    do                                                                          \
    {                                                                           \
        ++checks;                                                               \
        if (!(c))                                                               \
        {                                                                       \
            ++failures;                                                         \
            std::fprintf(stderr, "FAILED %s:%d: %s\n", __FILE__, __LINE__, #c); \
        }                                                                       \
    } while (0)

// (BigUint has its arithmetic in place: these give a value.)
static BigUint plus(BigUint a, const BigUint& b) { return a += b; }
static BigUint minus(BigUint a, const BigUint& b) { return a -= b; }
static BigUint over(BigUint a, uint32_t n) { a.divmod_small(n); return a; }
static BigUint times(BigUint a, uint32_t n) { a.mul_small(n); return a; }

int main(int argc, char** argv)
{
    if (argc != 2) return std::fprintf(stderr, "usage: sieve_tile_tests N (a power of two, 2 to 4096)\n"), 2;
    const uint32_t n = uint32_t(std::strtoul(argv[1], nullptr, 10));
    set_books_per_tile(n); // first of all: before any line is built
    CHECK(books_per_tile() == n);
    CHECK((uint32_t(1) << books_per_tile_bits()) == n);
    // Refused outright: a size that is not a power of two, or out of range.
    for (const uint32_t bad : {0u, 1u, 3u, 6u, 8192u})
    {
        bool refused = false;
        try
        {
            set_books_per_tile(bad);
        }
        catch (const std::invalid_argument&)
        {
            refused = true;
        }
        CHECK(refused);
    }
    // Lines of every kind of length: under a tile, a tile less one, a tile, a tile and one, an
    // odd number of them (lower27 at length 2), and large powers of two.
    const std::vector<BigUint> lengths = {BigUint(1), BigUint(n - 1 ? n - 1 : 1), BigUint(n), BigUint(n + 1), BigUint(27 * 27),
                                          BigUint::pow(2, 20), BigUint::pow(2, 100), BigUint::pow(27, 32)};
    for (const BigUint& units : lengths)
    {
        const LineLoop loop(units);
        // Its tiles, rounded up, and the padding that leaves at the end.
        const BigUint tiles = over(plus(units, BigUint(n - 1)), n);
        CHECK(loop.tiles() == tiles);
        CHECK(BigUint(loop.padding()) == minus(times(tiles, n), units));
        CHECK(loop.fills_whole_tiles() == (loop.padding() == 0));
        // Units from the start, the middle and the end: each in tile v / n, slot v % n, and back.
        std::vector<BigUint> some = {BigUint(0), minus(units, BigUint(1)), over(units, 2)};
        if (units > BigUint(n)) some.push_back(BigUint(n)), some.push_back(BigUint(n - 1));
        for (const BigUint& v : some)
        {
            const TileIndex t = LineLoop::tile_of(v);
            const uint32_t slot = LineLoop::slot_of(v);
            CHECK(!t.negative && t.magnitude == over(v, n));
            CHECK(slot == v.mod_small(n));
            const auto back = loop.unit_index(loop.loop_tile(t), slot);
            CHECK(back && *back == v);
        }
        // The last tile's padding slots hold no unit.
        const BigUint last = minus(tiles, BigUint(1));
        for (uint32_t k = n - loop.padding(); k < n; ++k) CHECK(!loop.unit_index(last, k));
        // The loop: tile `tiles` is tile 0 again, and tile -1 the last.
        CHECK(loop.loop_tile(TileIndex::of(0) + int64_t(0)) == BigUint(0));
        if (tiles.bit_length() < 40)
        {
            const int64_t tc = std::stoll(tiles.to_decimal());
            CHECK(loop.loop_tile(TileIndex::of(tc)) == BigUint(0));
            CHECK(loop.loop_tile(TileIndex::of(-1)) == last);
        }
    }
    // Latched: once a line is built, the same size may be said again, another may not.
    set_books_per_tile(n);
    bool latched = false;
    try
    {
        set_books_per_tile(n == 2 ? 4 : 2);
    }
    catch (const std::logic_error&)
    {
        latched = true;
    }
    CHECK(latched);
    CHECK(books_per_tile() == n);
    std::printf("tiles of %u: %d checks, %d failures\n", n, checks, failures);
    return failures ? 1 : 0;
}
