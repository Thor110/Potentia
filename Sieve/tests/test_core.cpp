// Sieve core tests. Usage: sieve_tests <tests directory containing the vectors_*.tsv files>

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"
#include "sieve/biguint.hpp"
#include "sieve/booksieve.hpp"
#include "sieve/bookspace.hpp"
#include "sieve/composition.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/canon.hpp"
#include "sieve/chunks.hpp"
#include "sieve/dfa.hpp"
#include "sieve/filekind.hpp"
#include "sieve/modelsieve.hpp"
#include "sieve/worldspace.hpp"
#include "sieve/written.hpp"
#include "sieve/packed.hpp"
#include "sieve/plugin.hpp"
#include "sieve/compact.hpp"
#include "sieve/corridor.hpp"
#include "sieve/filter.hpp"
#include "sieve/intlog.hpp"
#include "sieve/guided.hpp"
#include "sieve/model.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"
#include "sieve/sieve.hpp"
#include "sieve/space.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/titledspace.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace sieve;

namespace {

int g_failures = 0;
int g_checks = 0;

#define CHECK(cond)                                                                            \
    do {                                                                                       \
        ++g_checks;                                                                            \
        if (!(cond)) {                                                                         \
            ++g_failures;                                                                      \
            std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #cond "\n";         \
        }                                                                                      \
    } while (0)

template <class F>
bool throws(F&& f)
{
    try { f(); } catch (const std::exception&) { return true; }
    return false;
}

std::u32string u32(const std::string& s) { return utf8_decode(s); }

void test_sha256()
{
    CHECK(Sha256::hex(Sha256::hash("")) == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK(Sha256::hex(Sha256::hash("abc")) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string million(1000000, 'a');
    CHECK(Sha256::hex(Sha256::hash(million)) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    // Incremental updates across block boundaries must match a single update.
    Sha256 s;
    for (size_t i = 0; i < million.size(); i += 777) s.update(std::string_view(million).substr(i, 777));
    CHECK(Sha256::hex(s.finish()) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    // Padding boundaries (one or two final blocks).
    CHECK(Sha256::hex(Sha256::hash(std::string(55, 'x'))) == "d5e285683cd4efc02d021a5c62014694958901005d6f71e89e0989fac77e4072");
    CHECK(Sha256::hex(Sha256::hash(std::string(56, 'x'))) == "04c26261370ee7541549d16dee320c723e3fd14671e66a099afe0a377c16888e");
    CHECK(Sha256::hex(Sha256::hash(std::string(63, 'x'))) == "75220b47218278e656f2013bb8f0c455a25eaf01e86c64924e9d48d89776d6f2");
    CHECK(Sha256::hex(Sha256::hash(std::string(64, 'x'))) == "7ce100971f64e7001e8fe5a51973ecdfe1ced42befe7ee8d5fd6219506b5393c");
    CHECK(Sha256::hex(Sha256::hash(std::string(65, 'x'))) == "9537c5fdf120482f7d58d25e9ed583f52c02b4e304ea814db1633ad565aed7e9");
    CHECK(Sha256::hex(Sha256::hash(std::string(119, 'x'))) == "000b48d4edf0fa7bee3c6236ecd2785baa5db4eeb8bb54341b029e0d9fa5fb0c");
    CHECK(Sha256::hex(Sha256::hash(std::string(120, 'x'))) == "13f05a0b594787f5ecd315edc96141bd3243203d1b7d4f0836f37308b276ba98");
    CHECK(Sha256::hex(Sha256::hash(std::string(128, 'x'))) == "24da1b81d0b16df6428eee73c69fcb2a93c76bc6df706f0c6670fe6bfe800464");
}

void test_utf8()
{
    CHECK(utf8_encode(u32("héllo 蹪ꡎ孺徨 😀")) == "héllo 蹪ꡎ孺徨 😀");
    CHECK(throws([] { utf8_decode("\xC0\xAF"); }));          // overlong
    CHECK(throws([] { utf8_decode("\xED\xA0\x80"); }));      // surrogate
    CHECK(throws([] { utf8_decode("\xE2\x82"); }));          // truncated
    CHECK(throws([] { utf8_decode("\x80"); }));              // stray continuation
}

void test_biguint()
{
    // Multiplication and long division: a = q * b + r, 0 <= r < b, for random sizes (including
    // the cases algorithm D must correct), checked against repeated small operations.
    {
        std::mt19937_64 rng(23);
        auto rnd = [&](size_t limbs, bool edge) {
            BigUint v;
            for (size_t i = 0; i < limbs; ++i)
            {
                v <<= 32;
                v.add_small(edge ? (rng() % 3 == 0 ? 0xFFFFFFFFu : rng() % 3 == 0 ? 0u : uint32_t(rng())) : uint32_t(rng()));
            }
            return v;
        };
        bool ok = true;
        for (int t = 0; t < 3000; ++t)
        {
            const bool edge = t % 2;
            BigUint a = rnd(1 + rng() % 12, edge), b = rnd(1 + rng() % 6, edge);
            if (b.is_zero()) b = BigUint(7);
            BigUint q, r;
            BigUint::divmod(a, b, q, r);
            BigUint back = BigUint::mul(q, b);
            back += r;
            ok = ok && back == a && r < b;
        }
        CHECK(ok);
        // mul agrees with mul_small on one-limb factors, and with pow.
        BigUint x = BigUint::from_hex("123456789abcdef0fedcba9876543210");
        BigUint y = x;
        y.mul_small(0xdeadbeef);
        CHECK(BigUint::mul(x, BigUint(0xdeadbeef)) == y);
        // add_mul_small: the same as a copy multiplied and added, for shorter, longer and equal
        // lengths, a carry that runs through every limb, m = 0 and 1, and a number added to itself.
        {
            auto slow = [](BigUint acc, const BigUint& v, uint32_t m) {
                BigUint t = v;
                t.mul_small(m);
                return acc += t;
            };
            const BigUint ones = BigUint::from_hex("ffffffffffffffffffffffffffffffffffffffffffffffff");
            bool same = true;
            for (const BigUint& acc : {BigUint(), BigUint(7), x, ones, BigUint::pow(27, 100)})
                for (const BigUint& v : {BigUint(), BigUint(1), x, ones, BigUint::pow(3, 200)})
                    for (uint32_t m : {0u, 1u, 2u, 255u, 0xffffffffu})
                    {
                        BigUint fast = acc;
                        fast.add_mul_small(v, m);
                        same = same && fast == slow(acc, v, m);
                    }
            BigUint self = ones;
            self.add_mul_small(self, 3);
            same = same && self == slow(ones, ones, 3);
            CHECK(same);
        }
        CHECK(BigUint::mul(BigUint::pow(27, 40), BigUint::pow(27, 60)) == BigUint::pow(27, 100));
        BigUint q, r;
        BigUint::divmod(BigUint::pow(27, 100), BigUint::pow(27, 60), q, r);
        CHECK(q == BigUint::pow(27, 40) && r.is_zero());
    }

    // Fast paths agree with the generic ones: power-of-two bases (bits placed directly), pow by
    // shifting, linear hex parsing, and mod when a < 2m.
    {
        std::mt19937_64 rng(17);
        bool ok = true;
        for (uint32_t base : {2u, 4u, 8u, 16u, 256u, 65536u, 1u << 31, 3u, 27u, 104u})
            for (size_t len : {1u, 5u, 31u, 32u, 33u, 200u})
            {
                std::vector<uint32_t> d(len);
                for (auto& x : d) x = uint32_t(rng() % base);
                const BigUint v = BigUint::from_digits(d, base);
                // Generic Horner, one digit at a time.
                BigUint h;
                for (uint32_t x : d)
                {
                    if (base == (1u << 31)) { h <<= 31; }
                    else h.mul_small(base);
                    h.add_small(x);
                }
                ok = ok && v == h && v.to_digits(base, len) == d;
                ok = ok && throws([&] { (void)BigUint::pow(base, uint32_t(len)).to_digits(base, len); });
            }
        CHECK(ok);
        for (uint32_t base : {2u, 16u, 256u, 3u})
        {
            BigUint slow(1);
            for (int e = 0; e < 300; ++e) slow.mul_small(base);
            CHECK(BigUint::pow(base, 300) == slow);
        }
        CHECK(BigUint::pow(0, 0) == BigUint(1));
        CHECK(BigUint::pow(0, 5).is_zero());
        CHECK(BigUint::pow(1, 1000) == BigUint(1));
        std::string hex = "1";
        for (int i = 0; i < 99; ++i) hex += "0123456789abcdef"[rng() % 16];
        CHECK(BigUint::from_hex(hex).to_hex() == hex);
        CHECK(BigUint::from_hex("00ff") == BigUint(255));
        const BigUint m = BigUint::from_hex("123456789abcdef123456789");
        BigUint a = m;
        a += BigUint::from_hex("fedcba987654321");
        CHECK(BigUint::mod(a, m) == BigUint::from_hex("fedcba987654321"));
        BigUint big = m;
        big <<= 70;
        big.add_small(12345);
        BigUint expect = BigUint::mod(big, m); // shift-and-subtract path
        BigUint check = big;
        while (check >= m) { BigUint t = m; size_t sh = check.bit_length() - m.bit_length(); t <<= sh; if (t > check) { t = m; t <<= sh - 1; } check -= t; }
        CHECK(expect == check);
    }

    const BigUint p = BigUint::pow(27, 13);
    CHECK(p.to_decimal() == "4052555153018976267");
    CHECK(BigUint::pow(2, 100).to_hex() == "10000000000000000000000000");
    CHECK(BigUint::pow(2, 100).is_power_of_two());
    CHECK(!p.is_power_of_two());
    CHECK(BigUint::from_hex("00ff").to_decimal() == "255");
    CHECK(BigUint::from_hex("FF") == BigUint(255));
    CHECK(throws([] { BigUint::from_hex("xyz"); }));
    CHECK(throws([] { BigUint(256).to_hex(1); }));

    // Chunked conversions against a naive digit-at-a-time Horner/peel, across bases where the
    // chunk size and edge cases differ (powers of two, base^k == 2^32, one-digit chunks).
    {
        std::mt19937 r(7);
        for (uint32_t base : {2u, 3u, 4u, 16u, 27u, 95u, 104u, 255u, 256u, 65535u, 65536u, 65537u, 16777216u, 4294967295u})
            for (int t = 0; t < 20; ++t)
            {
                std::vector<uint32_t> d(1 + r() % 90);
                for (auto& x : d) x = t == 0 ? base - 1 : static_cast<uint32_t>(r() % base);
                BigUint naive;
                for (uint32_t x : d) { naive.mul_small(base); naive.add_small(x); }
                CHECK(BigUint::from_digits(d, base) == naive);
                CHECK(naive.to_digits(base, d.size()) == d);
                CHECK(throws([&] { (void)naive.to_digits(base, d.size() - 1); }) ==
                      !(d.front() == 0));
            }
        for (uint32_t base : {2u, 27u, 256u, 16777216u})
            for (uint32_t e : {0u, 1u, 5u, 13u, 40u})
            {
                BigUint naive(1);
                for (uint32_t i = 0; i < e; ++i) naive.mul_small(base);
                CHECK(BigUint::pow(base, e) == naive);
            }
    }

    std::mt19937 rng(1);
    for (int i = 0; i < 200; ++i)
    {
        const uint32_t base = 2 + rng() % 200;
        std::vector<uint32_t> d(1 + rng() % 60);
        for (auto& x : d) x = rng() % base;
        const BigUint v = BigUint::from_digits(d, base);
        CHECK(v.to_digits(base, d.size()) == d);
        CHECK(BigUint::from_hex(v.to_hex()) == v);
    }
    BigUint a(0xFFFFFFFFull), b(1);
    a += b;
    CHECK(a.to_hex() == "100000000");
    CHECK(std::abs(BigUint::pow(10, 50).log10_approx() - 50.0) < 1e-9);
}

void test_space_basics()
{
    const Alphabet& a = alphabet_by_id("lower27");
    CHECK(a.size() == 27 && a.symbol(0) == U' ' && a.symbol(1) == U'a');
    const Space sp(a, 3);
    // 27^3 = 19683, largest address 19682 needs 15 bits -> 4 hex digits.
    CHECK(sp.address_bits() == 15 && sp.hex_width() == 4);
    const auto d = sp.digits_of(U"abc");
    CHECK((d == std::vector<uint32_t>{1, 2, 3}));
    CHECK(sp.address_of(d, AddressMode::Positional) == "0312"); // 1*729 + 2*27 + 3 = 786
    CHECK(sp.text_of(sp.unit_at("0312", AddressMode::Positional)) == U"abc");
    CHECK(sp.address_of(sp.digits_of(U"   "), AddressMode::Positional) == "0000");
    CHECK(throws([&] { sp.unit_at("4ce3", AddressMode::Positional); })); // 19683 is outside
    CHECK(!throws([&] { sp.unit_at("4ce2", AddressMode::Positional); }));
    CHECK(throws([&] { sp.digits_of(U"ab"); }));
    CHECK(throws([&] { sp.digits_of(U"aBc"); }));

    const Space big(alphabet_by_id("lower27"), 1000);
    CHECK(big.address_bits() == 4755 && big.hex_width() == 1189);
}

void test_scramble_bijection()
{
    // Exhaustive: every unit of a small space maps to a distinct address, and back.
    for (uint32_t L : {1u, 2u, 3u})
    {
        const Space sp(alphabet_by_id("lower27"), L);
        std::set<std::string> seen;
        const uint64_t total = [&] { uint64_t t = 1; for (uint32_t i = 0; i < L; ++i) t *= 27; return t; }();
        std::vector<uint32_t> d(L, 0);
        for (uint64_t n = 0; n < total; ++n)
        {
            uint64_t v = n;
            for (uint32_t i = L; i-- > 0;) { d[i] = static_cast<uint32_t>(v % 27); v /= 27; }
            const std::string addr = sp.address_of(d, AddressMode::Scrambled);
            seen.insert(addr);
            if (n % 97 == 0) CHECK(sp.unit_at(addr, AddressMode::Scrambled) == d);
        }
        CHECK(seen.size() == total);
    }
    // Random round trips on larger spaces, all alphabets.
    std::mt19937 rng(7);
    for (const auto& id : alphabet_ids())
        for (uint32_t L : {4u, 17u, 256u})
        {
            const Space sp(alphabet_by_id(id), L, "test-key");
            for (int i = 0; i < 20; ++i)
            {
                std::vector<uint32_t> d(L);
                for (auto& x : d) x = rng() % sp.alphabet().size();
                CHECK(sp.unscramble(sp.scramble(d)) == d);
                const std::string addr = sp.address_of(d, AddressMode::Scrambled);
                CHECK(addr.size() == sp.hex_width());
                CHECK(sp.unit_at(addr, AddressMode::Scrambled) == d);
            }
        }
    // Different keys give different scrambles.
    const Space k1(alphabet_by_id("lower27"), 32, "a"), k2(alphabet_by_id("lower27"), 32, "b");
    const auto d = k1.digits_of(U"it was the best of times        ");
    CHECK(k1.address_of(d, AddressMode::Scrambled) != k2.address_of(d, AddressMode::Scrambled));
}

void test_neighbour()
{
    const Space sp(alphabet_by_id("lower27"), 3);
    const auto abc = sp.digits_of(U"abc");
    CHECK(sp.text_of(sp.neighbour(abc, AddressMode::Positional, 1)) == U"abd");
    CHECK(sp.text_of(sp.neighbour(abc, AddressMode::Positional, -3)) == U"ab ");  // 'c' - 3 = ' ' (digit 0)
    CHECK(sp.text_of(sp.neighbour(abc, AddressMode::Positional, -4)) == U"aaz");  // borrow
    CHECK(sp.text_of(sp.neighbour(sp.digits_of(U"zzz"), AddressMode::Positional, 1)) == U"   "); // wraps
    CHECK(sp.text_of(sp.neighbour(sp.digits_of(U"   "), AddressMode::Positional, -1)) == U"zzz");
    CHECK(sp.text_of(sp.neighbour(abc, AddressMode::Positional, 27 * 27)) == U"bbc");
    CHECK(sp.neighbour(abc, AddressMode::Positional, 19683) == abc); // a full loop

    // Scrambled: the neighbour's scrambled address is one more than this unit's.
    for (int64_t off : {-1000, -1, 1, 7, 12345})
    {
        const auto n = sp.neighbour(abc, AddressMode::Scrambled, off);
        CHECK(sp.neighbour(n, AddressMode::Scrambled, -off) == abc);
        const BigUint a = BigUint::from_hex(sp.address_of(abc, AddressMode::Scrambled));
        const BigUint b = BigUint::from_hex(sp.address_of(n, AddressMode::Scrambled));
        const uint64_t size = 19683;
        const uint64_t av = std::stoull(a.to_decimal()), bv = std::stoull(b.to_decimal());
        CHECK(bv == static_cast<uint64_t>((static_cast<int64_t>(av) + off % int64_t(size) + int64_t(size)) % int64_t(size)));
    }
    // Large offsets on a large base (rgb24 images).
    const Space big("image/rgb24/2x2", 16777216u, 4);
    const Space::Digits zero(4, 0);
    const auto far = big.neighbour(zero, AddressMode::Positional, int64_t(1) << 40);
    CHECK((far == Space::Digits{0, 0, 65536u, 0}));
    CHECK(big.neighbour(far, AddressMode::Positional, -(int64_t(1) << 40)) == zero);
}

void test_fraction()
{
    const Space text(alphabet_by_id("lower27"), 3);
    CHECK(text.address_of(unit_at_fraction(text, 5, 1, AddressMode::Positional), AddressMode::Positional) ==
          BigUint(19683 / 2).to_hex(4));                                                    // 50%
    CHECK(text.address_of(unit_at_fraction(text, 0, 0, AddressMode::Positional), AddressMode::Positional) == "0000");
    CHECK(throws([&] { unit_at_fraction(text, 10, 1, AddressMode::Positional); }));          // 100% is not on the line
}

void test_corridor()
{
    // BigUint helpers used by the corridor.
    std::mt19937_64 r(11);
    for (int i = 0; i < 300; ++i)
    {
        const uint64_t a = r() >> (r() % 60), m = 1 + (r() >> (r() % 63 + 1));
        CHECK(BigUint::mod(BigUint(a), BigUint(m)) == BigUint(a % m));
    }
    CHECK(BigUint::mod(BigUint::pow(2, 200), BigUint::pow(2, 7)).is_zero());
    CHECK(BigUint::mod(BigUint::pow(27, 32), BigUint(128)) == BigUint(uint64_t(BigUint::pow(27, 32).low_bits(7))));
    CHECK(BigUint::from_decimal("4052555153018976267") == BigUint::pow(27, 13));
    CHECK(throws([] { BigUint::from_decimal("12a"); }));

    // Signed tiles.
    TileIndex t = TileIndex::of(2);
    t += -5;
    CHECK(t.to_decimal() == "-3");
    t += 3;
    CHECK(t.to_decimal() == "0" && !t.negative);
    CHECK(TileIndex::parse("-42") == TileIndex::of(-42));

    // A power-of-two line fills whole tiles; others are padded at the end of each copy.
    const LineLoop image(BigUint::pow(2, 100));
    CHECK(image.fills_whole_tiles());
    CHECK(image.tiles() == BigUint::pow(2, 93));
    const LineLoop tiny(BigUint(27 * 27)); // lower27 at length 2: 729 units = 5 tiles + 89
    CHECK(tiny.tiles() == BigUint(6));
    CHECK(tiny.padding() == 6 * 128 - 729);
    CHECK(tiny.unit_index(BigUint(5), 88) == std::optional<BigUint>(BigUint(728)));
    CHECK(!tiny.unit_index(BigUint(5), 89));
    // Loops: tile 6 is the start of the second copy, tile -1 the last tile of the copy before.
    CHECK(tiny.loop_tile(TileIndex::of(6)).is_zero());
    CHECK(tiny.loop_tile(TileIndex::of(-1)) == BigUint(5));
    CHECK(tiny.loop_tile(TileIndex::of(-6)).is_zero());
    CHECK(tiny.loop_tile(TileIndex::of(13)) == BigUint(1));
    // Warp position: a unit's slot in the first copy.
    CHECK(LineLoop::tile_of(BigUint(728)) == TileIndex::of(5));
    CHECK(LineLoop::slot_of(BigUint(728)) == 88);

    // The two-door loop: text and image share the position, so going through a door, one tile
    // along, and back through the next door lands one tile along in text too, at any sizes.
    const LineLoop text(BigUint::pow(27, 32));
    const TileIndex start = TileIndex::of(123456789);
    const BigUint text_here = text.loop_tile(start);
    const BigUint text_next = text.loop_tile(start + 1);
    BigUint expect = text_here;
    expect.add_small(1);
    CHECK(text_next == expect);
    BigUint image_next = image.loop_tile(start);
    image_next.add_small(1);
    CHECK(image.loop_tile(start + 1) == image_next);
    // Nesting: with power-of-two sizes, every start line of the bigger line is one of the smaller's.
    const LineLoop small(BigUint::pow(2, 20)), big(BigUint::pow(2, 30));
    for (int64_t c : {1, 2, 7})
    {
        const TileIndex s = TileIndex::of(c * (int64_t(1) << (30 - 7)));
        CHECK(big.loop_tile(s).is_zero() && small.loop_tile(s).is_zero());
    }
}

void test_canon()
{
    const Alphabet& a = alphabet_by_id("lower27");
    auto r = canonicalise_text("  It was\tthe BEST\n\nof times!  ", a, 10, CanonVersion::V1);
    CHECK(r.units.size() == 3);
    CHECK(r.units[0] == U"it was the");
    CHECK(r.units[1] == U" best of t");
    CHECK(r.units[2] == U"imes      ");
    CHECK(r.dropped == 1 && r.dropped_examples == U"!");
    CHECK(r.case_folded == 5);
    CHECK(r.whitespace_mapped == 3);
    CHECK(r.padding == 6);
    CHECK(r.canonical_length == 24);

    // Dropping a character can create a double space, which is then collapsed.
    r = canonicalise_text("a - b", a, 8, CanonVersion::V1);
    CHECK(r.units.size() == 1 && r.units[0] == U"a b     ");

    // Accented letters are dropped and reported in v1.
    r = canonicalise_text("café", a, 4, CanonVersion::V1);
    CHECK(r.units[0] == U"caf " && r.dropped == 1 && r.dropped_examples == U"é");

    // v2: punctuation separates words, apostrophes join, accents fold.
    r = canonicalise_text("A well-known author/editor didn\u2019t stop \u2014 na\u00efve caf\u00e9!", a, 64);
    CHECK(r.units.size() == 1);
    CHECK(r.units[0].substr(0, 45) == U"a well known author editor didnt stop naive c");
    CHECK(r.transliterated == 2 && r.accents_folded == 2 && r.dropped == 1);
    CHECK(r.separated == 4); // - / — !
    r = canonicalise_text("Stra\u00dfe \u00c6sop \u0141\u00f3d\u017a", a, 20);
    CHECK(r.units[0] == U"strasse aesop lodz  ");
    // ascii95 keeps the ASCII forms of typographic punctuation.
    r = canonicalise_text("\u201cHi\u201d \u2014 it\u2019s\u2026", alphabet_by_id("ascii95"), 16);
    CHECK(r.units[0] == U"\"Hi\" - it's...  ");
    CHECK(canon_version_from_string("v1") == CanonVersion::V1);
    CHECK(throws([] { canon_version_from_string("v9"); }));

    // ascii95 keeps case and punctuation.
    r = canonicalise_text("Hello, World!", alphabet_by_id("ascii95"), 13);
    CHECK(r.units.size() == 1 && r.units[0] == U"Hello, World!" && r.case_folded == 0);

    r = canonicalise_text("!!!", a, 4);
    CHECK(r.units.empty());
    CHECK(throws([&] { canonicalise_text("\xFF", a, 4); }));
}

std::vector<std::vector<std::string>> read_tsv(const std::string& path, size_t fields)
{
    std::vector<std::vector<std::string>> rows;
    std::ifstream in(path);
    CHECK(static_cast<bool>(in));
    if (!in) { std::cerr << "cannot open " << path << "\n"; return rows; }
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        size_t start = 0;
        for (size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || line[i] == '\t') { f.push_back(line.substr(start, i - start)); start = i + 1; }
        CHECK(f.size() == fields);
        if (f.size() == fields) rows.push_back(std::move(f));
    }
    return rows;
}

std::vector<uint32_t> split_u32(const std::string& s)
{
    std::vector<uint32_t> v;
    std::stringstream ss(s);
    std::string part;
    while (std::getline(ss, part, ',')) v.push_back(static_cast<uint32_t>(std::stoul(part)));
    return v;
}

std::string from_hex(const std::string& h)
{
    std::string s;
    for (size_t i = 0; i + 1 < h.size(); i += 2) s.push_back(static_cast<char>(std::stoul(h.substr(i, 2), nullptr, 16)));
    return s;
}

// BigUint against Python's own arbitrary-precision integers: the address path rests entirely on
// this arithmetic, so it is pinned the same way every other rule is. The vectors are generated by
// reference/sieve_ref.py, which shares no code with the core (SPECIFICATIONS 4.3).
void test_biguint_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 4))
    {
        const std::string& op = f[0];
        const BigUint a = BigUint::from_hex(f[1]), b = BigUint::from_hex(f[2]);
        const std::string& want = f[3];
        std::string got;
        if (op == "add") { BigUint v = a; v += b; got = v.to_hex(); }
        else if (op == "sub") { BigUint v = a; v -= b; got = v.to_hex(); }
        else if (op == "mul") got = BigUint::mul(a, b).to_hex();
        else if (op == "xor") { BigUint v = a; v ^= b; got = v.to_hex(); }
        else if (op == "cmp") { const int c = compare(a, b); got = c < 0 ? "-1" : c > 0 ? "1" : "0"; }
        else if (op == "bits") got = std::to_string(a.bit_length());
        else if (op == "dec") got = a.to_decimal();
        else if (op == "divmod")
        {
            BigUint q, r;
            BigUint::divmod(a, b, q, r);
            got = q.to_hex() + "," + r.to_hex();
            // and the identity that division has to satisfy, whatever the implementation
            BigUint back = BigUint::mul(q, b);
            back += r;
            CHECK(back == a && r < b);
        }
        else if (op.rfind("shl", 0) == 0) { BigUint v = a; v <<= size_t(std::stoul(op.substr(3))); got = v.to_hex(); }
        else if (op.rfind("shr", 0) == 0) { BigUint v = a; v >>= size_t(std::stoul(op.substr(3))); got = v.to_hex(); }
        else if (op.rfind("digits", 0) == 0)
        {
            const uint32_t base = static_cast<uint32_t>(std::stoul(op.substr(6)));
            size_t len = 1;
            for (BigUint p(base); p <= a; p = BigUint::mul(p, BigUint(base))) ++len;
            const auto d = a.to_digits(base, len);
            for (size_t i = 0; i < d.size(); ++i) got += (i ? "," : "") + std::to_string(d[i]);
            // and back again, since the pair of them is what an address is
            CHECK(BigUint::from_digits(d, base) == a);
        }
        else
        {
            std::cerr << "  unknown biguint op '" << op << "'\n";
            CHECK(false);
            continue;
        }
        const bool ok = got == want;
        CHECK(ok);
        if (!ok) std::cerr << "  biguint " << op << "(" << f[1] << ", " << f[2] << "): got " << got << " want " << want << "\n";
        ++n;
    }
    std::cout << "biguint vectors checked: " << n << "\n";
    CHECK(n > 3000); // a truncated or emptied vector file must fail, not pass quietly
}

// The same pinning at the lengths where the fast algorithms run (Karatsuba, division by a
// reciprocal, conversion by halves), up to 400,000 bits. The operands are named rather than
// written out, and each result is compared by its SHA-256, both as reference/sieve_ref.py's
// large_operand() and cmd_biguint_large_vectors() describe; the builder below follows that
// description, not the C++ it is testing.
BigUint large_operand(const std::string& spec)
{
    std::vector<std::string> f;
    for (size_t i = 0, j; i <= spec.size(); i = j + 1)
    {
        j = spec.find(':', i);
        if (j == std::string::npos) j = spec.size();
        f.push_back(spec.substr(i, j - i));
    }
    const auto two_to = [](size_t bits) { BigUint v(1); v <<= bits; return v; };
    if (f[0] == "r")
    {
        const size_t bits = std::stoul(f[1]), bytes = (bits + 7) / 8;
        std::vector<uint8_t> raw;
        for (uint32_t k = 0; raw.size() < bytes; ++k)
            for (uint8_t b : Sha256::hash("biguint-large/v1/" + f[2] + "/" + std::to_string(k))) raw.push_back(b);
        raw.resize(bytes);
        std::vector<uint32_t> words((bytes + 3) / 4, 0);
        for (size_t i = 0; i < bytes; ++i) words[i / 4] |= uint32_t(raw[i]) << (8 * (i % 4));
        BigUint v = BigUint::mod(BigUint::from_limbs(words), two_to(bits));
        if (!v.bit(bits - 1)) v += two_to(bits - 1);
        return v;
    }
    if (f[0] == "m") { BigUint v = two_to(std::stoul(f[1])); v -= BigUint(1); return v; }
    if (f[0] == "s") { BigUint v = two_to(std::stoul(f[1]) - 1); v += BigUint(1); return v; }
    if (f[0] == "p")
    {
        BigUint v = BigUint::pow(uint32_t(std::stoul(f[1])), std::stoull(f[2]));
        if (f[3] == "-1") v -= BigUint(1);
        else if (f[3] == "1") v += BigUint(1);
        return v;
    }
    throw std::invalid_argument("unknown large operand " + spec);
}

void test_biguint_large_vectors(const std::string& path)
{
    int n = 0;
    std::map<std::string, BigUint> made;
    auto operand = [&](const std::string& spec) -> const BigUint& {
        auto it = made.find(spec);
        if (it == made.end()) it = made.emplace(spec, large_operand(spec)).first;
        return it->second;
    };
    for (const auto& f : read_tsv(path, 4))
    {
        const std::string& op = f[0];
        const BigUint& a = operand(f[1]);
        std::string got;
        if (op == "mul") got = BigUint::mul(a, operand(f[2])).to_hex();
        else if (op == "hex") got = a.to_hex();
        else if (op == "dec")
        {
            got = a.to_decimal();
            CHECK(BigUint::from_decimal(got) == a);
        }
        else if (op == "divmod")
        {
            const BigUint& b = operand(f[2]);
            BigUint q, r;
            BigUint::divmod(a, b, q, r);
            got = q.to_hex() + "," + r.to_hex();
            BigUint back = BigUint::mul(q, b);
            back += r;
            CHECK(back == a && r < b);
        }
        else if (op.rfind("digits", 0) == 0)
        {
            const uint32_t base = static_cast<uint32_t>(std::stoul(op.substr(6)));
            // The smallest len with base^len > a, found by doubling and then bisecting, since a
            // stepwise search at these lengths would be the slowest thing in the file.
            size_t lo = 0, hi = 1;
            while (BigUint::pow(base, hi) <= a) { lo = hi; hi *= 2; }
            while (hi - lo > 1)
            {
                const size_t mid = lo + (hi - lo) / 2;
                (BigUint::pow(base, mid) <= a ? lo : hi) = mid;
            }
            const size_t len = hi;
            const auto d = a.to_digits(base, len);
            got.reserve(len * 3);
            for (size_t i = 0; i < d.size(); ++i)
            {
                if (i) got += ',';
                got += std::to_string(d[i]);
            }
            CHECK(BigUint::from_digits(d, base) == a);
            CHECK(throws([&] { (void)a.to_digits(base, len - 1); }));
        }
        else
        {
            std::cerr << "  unknown large biguint op '" << op << "'\n";
            CHECK(false);
            continue;
        }
        const bool ok = Sha256::hex(Sha256::hash(got)) == f[3];
        CHECK(ok);
        if (!ok) std::cerr << "  large biguint " << op << "(" << f[1] << ", " << f[2] << ") does not match\n";
        ++n;
    }
    std::cout << "large biguint vectors checked: " << n << "\n";
    CHECK(n >= 60);
}

void test_digit_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 6))
    {
        const Space sp("test", static_cast<uint32_t>(std::stoul(f[0])), static_cast<uint32_t>(std::stoul(f[1])), f[2]);
        const auto d = split_u32(f[3]);
        const bool ok = sp.address_of(d, AddressMode::Positional) == f[4] && sp.address_of(d, AddressMode::Scrambled) == f[5] &&
                        sp.unit_at(f[5], AddressMode::Scrambled) == d;
        CHECK(ok);
        if (!ok) std::cerr << "  digit vector mismatch: base " << f[0] << " L=" << f[1] << "\n";
        ++n;
    }
    std::cout << "digit vectors checked: " << n << "\n";
    CHECK(n == 120); // a truncated or emptied vector file must fail, not pass quietly
    CHECK(n > 100);
}

// The binary line against the oracle: a file cut into bytes256 units by canon-bytes-v1, and each
// unit's addresses. Until these existed the binary line was checked only by the engine agreeing
// with itself, in a round trip; these are answers the oracle worked out from the specification.
void test_bytes_vectors(const std::string& path)
{
    int n = 0;
    const auto hex_of = [](const std::u32string& u) {
        std::string h;
        for (char32_t c : u)
        {
            char b[3];
            std::snprintf(b, sizeof b, "%02x", unsigned(c));
            h += b;
        }
        return h;
    };
    const auto split = [](const std::string& s) {
        std::vector<std::string> out;
        for (size_t i = 0, j; i <= s.size(); i = j + 1)
        {
            j = s.find(',', i);
            if (j == std::string::npos) j = s.size();
            out.push_back(s.substr(i, j - i));
        }
        return out;
    };
    for (const auto& f : read_tsv(path, 7))
    {
        const Alphabet& alpha = alphabet_of(f[0]);
        const uint32_t L = static_cast<uint32_t>(std::stoul(f[1]));
        const std::string raw = f[3] == "-" ? std::string() : from_hex(f[3]);
        CanonResult r;
        try
        {
            r = canonicalise_bytes(raw, alpha, L);
        }
        catch (const std::exception& e)
        {
            // A row the engine refuses is a disagreement with the oracle like any other, and
            // must fail this check rather than end the whole run.
            CHECK(false);
            std::cerr << "  bytes vector refused: " << f[0] << " L=" << f[1] << ": " << e.what() << "\n";
            ++n;
            continue;
        }
        const auto want_units = split(f[4]), want_pos = split(f[5]), want_scr = split(f[6]);
        bool ok = r.units.size() == want_units.size() && want_pos.size() == want_units.size() &&
                  want_scr.size() == want_units.size();
        const Space sp(alpha, L, f[2]);
        for (size_t i = 0; ok && i < r.units.size(); ++i)
        {
            const auto d = sp.digits_of(r.units[i]);
            ok = hex_of(r.units[i]) == want_units[i] && sp.address_of(d, AddressMode::Positional) == want_pos[i] &&
                 sp.address_of(d, AddressMode::Scrambled) == want_scr[i] &&
                 sp.unit_at(want_scr[i], AddressMode::Scrambled) == d;
        }
        CHECK(ok);
        if (!ok) std::cerr << "  bytes vector mismatch: " << f[0] << " L=" << f[1] << " file " << f[3].substr(0, 16) << "\n";
        ++n;
    }
    std::cout << "binary-line vectors checked: " << n << "\n";
    CHECK(n >= 33); // a truncated or emptied vector file must fail, not pass quietly
}

// Titled lines against the oracle (SPECIFICATIONS §11): each row names a shape and an address in
// one ordering, and gives the cover, title and content that address holds; the engine has to find
// the same parts, and the same address from them.
void test_titled_vectors(const std::string& path)
{
    int n = 0;
    const auto digits = [](const std::string& s) {
        Space::Digits d;
        if (s == "-") return d;
        for (size_t i = 0, j; i <= s.size(); i = j + 1)
        {
            j = s.find(',', i);
            if (j == std::string::npos) j = s.size();
            d.push_back(static_cast<uint32_t>(std::stoul(s.substr(i, j - i))));
        }
        return d;
    };
    for (const auto& f : read_tsv(path, 13))
    {
        const std::string& key = f[7];
        std::optional<Space> title;
        if (std::stoul(f[3]) > 0) title.emplace(alphabet_of(f[2]), static_cast<uint32_t>(std::stoul(f[3])), key);
        std::optional<Space> cover;
        if (f[4] != "-") cover.emplace(f[4], static_cast<uint32_t>(std::stoul(f[5])), static_cast<uint32_t>(std::stoul(f[6])), key);
        const TitledSpace ts(title, cover, BigUint::from_hex(f[1]), f[0], key);
        const AddressMode m = f[8] == "scrambled" ? AddressMode::Scrambled : AddressMode::Positional;
        const TitledSpace::Parts want{digits(f[10]), digits(f[11]), BigUint::from_hex(f[12])};
        const BigUint address = ts.parse(f[9]);
        const bool ok = ts.parts_at(address, m) == want && ts.index_of(want, m) == address && ts.hex_of(address) == f[9];
        CHECK(ok);
        if (!ok) std::cerr << "  titled vector mismatch: " << f[0] << " " << f[8] << " " << f[9] << "\n";
        ++n;
    }
    std::cout << "titled-line vectors checked: " << n << "\n";
    CHECK(n >= 60); // a truncated or emptied vector file must fail, not pass quietly
}

void test_binary_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 6))
    {
        const BinarySpace bs(std::stoull(f[0]), f[1]);
        const AddressMode m = f[2] == "scrambled" ? AddressMode::Scrambled : AddressMode::Positional;
        BinarySpace::Bytes want;
        if (f[5] != "-")
            for (size_t i = 0; i + 1 < f[5].size(); i += 2) want.push_back(uint8_t(std::stoul(f[5].substr(i, 2), nullptr, 16)));
        const BigUint address = bs.parse(f[3]);
        const bool ok = want.size() == std::stoull(f[4]) && bs.bytes_at(address, m) == want && bs.index_of(want, m) == address &&
                        bs.hex_of(address) == f[3];
        CHECK(ok);
        if (!ok) std::cerr << "  binary vector mismatch: " << f[0] << " " << f[2] << " " << f[3] << "\n";
        ++n;
    }
    // Longer than the line is refused, not wrapped or cut.
    bool refused = false;
    try
    {
        (void)BinarySpace(2, "sieve").index_of({1, 2, 3}, AddressMode::Positional);
    }
    catch (const std::invalid_argument&)
    {
        refused = true;
    }
    CHECK(refused);
    std::cout << "binary-line vectors checked: " << n << "\n";
    CHECK(n >= 120); // a truncated or emptied vector file must fail, not pass quietly
}

// cdc-v1 (chunks.hpp) against the oracle's own cutting: the gear table's ends, then each input's
// chunks, by offset, length, SHA-256 and whether uniform.
// canon-notes-v2 against the oracle: every unit's digits, the report, the first unit's notation
// and its MIDI file's SHA-256.
void test_notes2_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 7))
    {
        const NoteSet set = note_set_of(f[0]);
        CHECK(set.id() == f[0] && !set.legacy);
        const NotesCanonResult r = canonicalise_notes2(f[2], set, uint32_t(std::stoul(f[1])));
        std::string units;
        for (const auto& u : r.units)
        {
            if (!units.empty()) units += ";";
            for (size_t i = 0; i < u.size(); ++i) units += (i ? "," : "") + std::to_string(u[i]);
        }
        const std::string report = std::to_string(r.events) + "," + std::to_string(r.flats_rewritten) + "," + std::to_string(r.octave_shifted) + "," +
                                   std::to_string(r.default_durations) + "," + std::to_string(r.durations_changed) + "," + std::to_string(r.padding);
        const std::string midi = notes_to_midi(set, r.units.front());
        const bool ok = units == f[3] && report == f[4] && notes_to_notation(set, r.units.front()) == f[5] && Sha256::hex(Sha256::hash(midi)) == f[6];
        CHECK(ok);
        if (!ok) std::cerr << "  notes2 vector mismatch: " << f[0] << " " << f[2] << " -> " << units << " | " << report << "\n";
        // The notation written out reads back to the same unit.
        const auto again = canonicalise_notes2(notes_to_notation(set, r.units.front()), set, uint32_t(std::stoul(f[1])));
        CHECK(again.units.front() == r.units.front());
        ++n;
    }
    std::cout << "notes2 vectors checked: " << n << "\n";
    CHECK(n >= 12);
}

void test_pcm_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 6))
    {
        const PcmFormat fmt = pcm_format_of(f[0]);
        CHECK(fmt.id() == f[0]);
        const uint32_t L = uint32_t(std::stoul(f[1]));
        std::vector<uint8_t> wav;
        for (size_t i = 0; i + 1 < f[2].size(); i += 2) wav.push_back(uint8_t(std::stoul(f[2].substr(i, 2), nullptr, 16)));
        const PcmAudio a = read_wav(wav);
        // Handed over in blocks of 3 frames, so the spans cross the blocks' edges.
        PcmCanoniser c(fmt, L, a.rate, a.channels);
        for (size_t at = 0; at < a.samples.size(); at += 3 * a.channels)
            c.add(std::span<const int32_t>(a.samples.data() + at, std::min<size_t>(3 * a.channels, a.samples.size() - at)));
        const PcmCanonResult r = c.finish();
        std::string units;
        for (const auto& u : r.units)
        {
            if (!units.empty()) units += ";";
            for (size_t i = 0; i < u.size(); ++i) units += (i ? "," : "") + std::to_string(u[i]);
        }
        const std::string report = std::to_string(r.source_frames) + "," + std::to_string(r.samples) + "," + std::to_string(r.clipped) + "," +
                                   std::to_string(r.padding);
        const std::string out = pcm_to_wav(fmt, r.units.front());
        const bool ok = units == f[3] && report == f[4] && Sha256::hex(Sha256::hash(out)) == f[5];
        CHECK(ok);
        if (!ok) std::cerr << "  pcm vector mismatch: " << f[0] << " L=" << f[1] << " -> " << units << " | " << report << "\n";
        // The WAV file written out reads back to the same unit, at the set's own rate and channels.
        const PcmAudio back = read_wav(std::vector<uint8_t>(out.begin(), out.end()));
        CHECK(back.rate == fmt.rate && back.channels == fmt.channels);
        PcmCanoniser again(fmt, L, back.rate, back.channels);
        again.add(back.samples);
        CHECK(again.finish().units.front() == r.units.front());
        ++n;
    }
    std::cout << "pcm vectors checked: " << n << "\n";
    CHECK(n >= 13);
}

void test_notes3_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 7))
    {
        const Notes3Set set = notes3_set_of(f[0]);
        CHECK(set.id() == f[0]);
        const Notes3CanonResult r = canonicalise_notes3(f[2], set, uint32_t(std::stoul(f[1])));
        std::string units;
        for (const auto& u : r.units)
        {
            if (!units.empty()) units += ";";
            for (size_t i = 0; i < u.size(); ++i) units += (i ? "," : "") + std::to_string(u[i]);
        }
        const std::string report = std::to_string(r.events) + "," + std::to_string(r.flats_rewritten) + "," + std::to_string(r.octave_shifted) + "," +
                                   std::to_string(r.default_lengths) + "," + std::to_string(r.default_levels) + "," + std::to_string(r.lengths_rounded) +
                                   "," + std::to_string(r.lengths_split) + "," + std::to_string(r.levels_clamped) + "," + std::to_string(r.padding);
        const std::string midi = notes3_to_midi(set, r.units.front());
        const bool ok = units == f[3] && report == f[4] && notes3_to_notation(set, r.units.front()) == f[5] && Sha256::hex(Sha256::hash(midi)) == f[6];
        CHECK(ok);
        if (!ok) std::cerr << "  notes3 vector mismatch: " << f[0] << " " << f[2] << " -> " << units << " | " << report << "\n";
        // The notation written out reads back to the same unit.
        CHECK(canonicalise_notes3(notes3_to_notation(set, r.units.front()), set, uint32_t(std::stoul(f[1]))).units.front() == r.units.front());
        ++n;
    }
    std::cout << "notes3 vectors checked: " << n << "\n";
    CHECK(n >= 8);

    // A MIDI file saved reads back as the same music: here, with no two rests in a row (MIDI has no
    // rest events, so a run of rests comes back as one), the same unit.
    const Notes3Set set = make_notes3_set(0, 127, 4, 16, 8, 2, 100, {0, 40});
    const Notes3CanonResult r = canonicalise_notes3("C#4:3!5 R:1 D4:16!8 E4:1!1 // C2:16!3 R:2 G9:1!8 C-1:3!2", set, 4);
    std::vector<std::string> notes;
    const std::string file = notes3_to_midi(set, r.units.front());
    const std::string back = midi_to_notes3(std::vector<uint8_t>(file.begin(), file.end()), set, &notes);
    CHECK(canonicalise_notes3(back, set, 4).units.front() == r.units.front());
    // Ids: one spelling, and the limits.
    for (const char* bad : {"notes3/C-1..G9/q4/d16/v8/V1/t120", "notes3/C-1..G9/q0/d16/v8/V1/t120/i0", "notes3/C4..C4/q4/d16/v8/V1/t120/i0",
                            "notes3/C-1..G9/q4/d16/v8/V16/t120/i0", "notes3/C-1..G9/q4/d16/v8/V2/t120/i0,1,2", "notes3/Db4..G9/q4/d16/v8/V1/t120/i0",
                            "notes3/C-1..G9/q4/d16/v128/V1/t120/i0"})
        CHECK(!is_notes3_symbols(bad));
    CHECK(!is_note_symbols("notes3/C-1..G9/q4/d16/v8/V1/t120/i0") && !is_notes3_symbols("notes104"));
    // An event asking to be split into more than 2^24 rests is refused, not built.
    {
        const Notes3Set tiny = make_notes3_set(48, 72, 4, 1, 8, 1, 120, {0});
        bool refused = false;
        try
        {
            (void)canonicalise_notes3("C4:999999999", tiny, 16);
        }
        catch (const std::invalid_argument&)
        {
            refused = true;
        }
        CHECK(refused);
    }
    // The kind checks answer quickly and the same either way.
    CHECK(is_notes3_symbols("notes3/C-1..G9/q4/d16/v8/V1/t120/i0") && !is_notes3_symbols("notes104") && !is_notes3_symbols("notes3/x"));
    CHECK(is_pcm_symbols("pcm/8000/8/C1") && !is_pcm_symbols("notes104") && !is_pcm_symbols("pcm/8000"));
    CHECK(is_note_symbols("notes104") && !is_note_symbols("pcm/8000/8/C1") && !is_note_symbols("notes2/x"));
    // A power of a big number is the same however it is reached.
    CHECK(BigUint::pow(BigUint(27), 40) == BigUint::pow(27u, 40) && BigUint::pow(BigUint::pow(3u, 50), 7) == BigUint::pow(3u, 350) &&
          BigUint::pow(BigUint(5), 0) == BigUint(1));
    std::cout << "notes3 MIDI and ids checked\n";
}

void test_pcm_digits()
{
    // A digit is the sample's own bits: digit 0 is silence, and every value comes back.
    for (uint32_t bits : {1u, 2u, 8u, 16u, 24u, 31u})
    {
        const PcmFormat f = make_pcm_format(8000, bits, 1);
        const int64_t lo = -(int64_t(1) << (bits - 1)), hi = (int64_t(1) << (bits - 1)) - 1;
        for (int64_t s : {lo, lo + 1, int64_t(-1), int64_t(0), int64_t(1), hi - 1, hi})
            if (s >= lo && s <= hi)
            {
                CHECK(pcm_sample(f, pcm_digit(f, int32_t(s))) == s);
                CHECK(pcm_digit(f, int32_t(s)) < f.base());
            }
        CHECK(pcm_digit(f, 0) == 0);
    }
    // Ids: written one way only, and checked.
    CHECK(pcm_format_of("pcm/44100/16/C2").id() == "pcm/44100/16/C2");
    for (const char* bad : {"pcm/0/8/C1", "pcm/8000/0/C1", "pcm/8000/32/C1", "pcm/8000/8/C0", "pcm/8000/8/C65536", "pcm/08000/8/C1", "pcm/8000/8/1",
                            "pcm/8000/8"})
        CHECK(!is_pcm_symbols(bad));
    CHECK(!is_pcm_symbols("notes104") && !is_note_symbols("pcm/8000/8/C1"));
    // No sound at all is refused, not made into silence.
    PcmCanoniser c(make_pcm_format(8000, 8, 1), 4, 8000, 1);
    bool threw = false;
    try
    {
        c.finish();
    }
    catch (const std::invalid_argument&)
    {
        threw = true;
    }
    CHECK(threw);
    std::cout << "pcm digits and ids checked\n";
}

void test_chunk_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 3))
    {
        if (f[0] == "gear")
        {
            char want[17];
            std::snprintf(want, sizeof want, "%016llx", static_cast<unsigned long long>(sieve::cdc::gear()[std::stoul(f[1])]));
            CHECK(f[2] == want);
            ++n;
            continue;
        }
        const std::string in = f[1] == "-" ? std::string() : from_hex(f[1]);
        std::string got;
        for (const auto& c : sieve::cdc::chunks(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(in.data()), in.size())))
        {
            if (!got.empty()) got += ",";
            got += std::to_string(c.offset) + ":" + std::to_string(c.length) + ":" + Sha256::hex(c.sha256) + ":" + (c.uniform ? "1" : "0");
        }
        if (got.empty()) got = "-";
        // Fed in pieces (37 bytes at a time), the Chunker must cut exactly as in one go.
        sieve::cdc::Chunker ck;
        for (size_t i = 0; i < in.size(); i += 37)
            ck.update(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(in.data()) + i, std::min<size_t>(37, in.size() - i)));
        ck.finish();
        std::string pieces;
        for (const auto& c : ck.ready())
        {
            if (!pieces.empty()) pieces += ",";
            pieces += std::to_string(c.offset) + ":" + std::to_string(c.length) + ":" + Sha256::hex(c.sha256) + ":" + (c.uniform ? "1" : "0");
        }
        if (pieces.empty()) pieces = "-";
        CHECK(pieces == got);
        const bool ok = got == f[2];
        CHECK(ok);
        if (!ok) std::cerr << "  chunk vector mismatch: " << f[0] << "\n";
        ++n;
    }
    std::cout << "chunk vectors checked: " << n << "\n";
    CHECK(n >= 13); // a truncated or emptied vector file must fail, not pass quietly
}

void test_canon_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 5))
    {
        const auto r = canonicalise_text(from_hex(f[3]), alphabet_by_id(f[1]), static_cast<uint32_t>(std::stoul(f[2])),
                                         canon_version_from_string(f[0]));
        std::string got;
        for (size_t i = 0; i < r.units.size(); ++i)
        {
            if (i) got += ",";
            const std::string u = utf8_encode(r.units[i]);
            for (unsigned char c : u)
            {
                char b[3];
                std::snprintf(b, sizeof b, "%02x", c);
                got += b;
            }
        }
        CHECK(got == f[4]);
        if (got != f[4]) std::cerr << "  canon mismatch: " << f[0] << " " << f[1] << " input " << from_hex(f[3]) << "\n";
        ++n;
    }
    std::cout << "canonicalisation vectors checked: " << n << "\n";
    CHECK(n == 80); // a truncated or emptied vector file must fail, not pass quietly
    CHECK(n > 50);
}

void test_image_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 7))
    {
        RgbaImage img;
        img.width = static_cast<uint32_t>(std::stoul(f[1]));
        img.height = static_cast<uint32_t>(std::stoul(f[2]));
        const std::string bytes = from_hex(f[5]);
        img.rgba.assign(bytes.begin(), bytes.end());
        ImageFormat fmt;
        fmt.width = static_cast<uint32_t>(std::stoul(f[3]));
        fmt.height = static_cast<uint32_t>(std::stoul(f[4]));
        fmt.palette = &palette_by_id(f[0]);
        const bool ok = canonicalise_frame(img, fmt) == split_u32(f[6]);
        CHECK(ok);
        if (!ok) std::cerr << "  image mismatch: " << f[0] << " " << f[1] << "x" << f[2] << " -> " << f[3] << "x" << f[4] << "\n";
        ++n;
    }
    std::cout << "image vectors checked: " << n << "\n";
    CHECK(n == 36); // a truncated or emptied vector file must fail, not pass quietly
    CHECK(n > 30);
}

void test_image()
{
    const Palette& mono = palette_by_id("mono");
    CHECK(mono.size() == 2 && mono.nearest({200, 200, 200}) == 1 && mono.nearest({50, 50, 50}) == 0);
    const Palette& rgb24 = palette_by_id("rgb24");
    CHECK(rgb24.size() == 16777216u && rgb24.nearest({1, 2, 3}) == 0x010203u && rgb24.colour(0x010203u) == (Rgb{1, 2, 3}));
    const Palette& p332 = palette_by_id("rgb332");
    for (uint32_t i = 0; i < 256; ++i) CHECK(p332.nearest(p332.colour(i)) == i);
    for (const auto& id : palette_ids()) CHECK(palette_by_id(id).colour(0) == (Rgb{0, 0, 0}));

    // Video: frames padded with black, extras dropped, all reported.
    ImageFormat vf{2, 2, 3, &mono};
    RgbaImage white{2, 2, std::vector<uint8_t>(16, 255)};
    ImageCanonReport rep;
    auto d = canonicalise_image({white}, vf, &rep);
    CHECK((d == std::vector<uint32_t>{1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0}) && rep.frames_padded == 2);
    d = canonicalise_image({white, white, white, white}, vf, &rep);
    CHECK(rep.frames_dropped == 1 && d.size() == 12);
    CHECK(vf.symbols_id() == "video/mono/2x2x3");
    CHECK(render_image(d, vf).size() == 12);
}

void test_audio()
{
    auto r = canonicalise_notes("C4q D#4e | Eb4h, Rw C6 B7q A2e", 4);
    CHECK(r.events == 7 && r.units.size() == 2 && r.padding == 1);
    CHECK(r.flats_rewritten == 1 && r.octave_shifted == 2 && r.default_durations == 1);
    CHECK(notes_to_notation(r.units[0]) == "C4q D#4e D#4h Rw");
    CHECK(notes_to_notation(r.units[1]) == "C6q B5q A4e Re");
    CHECK(note_token(0) == "Re" && note_token(103) == "C6w");
    CHECK(throws([] { canonicalise_notes("H4q", 4); }));
    CHECK(throws([] { canonicalise_notes("C4x", 4); }));
    const std::string midi = notes_to_midi(r.units[0]);
    CHECK(midi.substr(0, 4) == "MThd" && midi.substr(14, 4) == "MTrk");
    CHECK(static_cast<unsigned char>(midi[midi.size() - 3]) == 0xFF && midi.substr(midi.size() - 2) == std::string("\x2F\x00", 2));
}

// Alphabets built from Unicode blocks and ranges. The units are written as code points, not as
// UTF-8, because these lines can hold line feeds and surrogates, which a text form cannot carry.
// The models line, against the Python oracle's vectors: the shape's size and addresses in both
// orderings, and the SHA-256 of the canonical .obj text of each model.
void test_model_vectors(const std::string& path)
{
    std::ifstream in(path);
    CHECK(in.good());
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        size_t start = 0;
        for (size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || line[i] == '\t') { f.push_back(line.substr(start, i - start)); start = i + 1; }
        CHECK(f.size() == 9);
        if (f.size() != 9) continue;
        auto digits = [](const std::string& s) {
            std::vector<uint32_t> d;
            size_t p = 0;
            for (size_t i = 0; i <= s.size(); ++i)
                if (i == s.size() || s[i] == ',') { d.push_back(uint32_t(std::stoul(s.substr(p, i - p)))); p = i + 1; }
            return d;
        };
        const ModelSpace sp(uint32_t(std::stoul(f[0])), uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), f[3]);
        const AddressMode m = f[4] == "scrambled" ? AddressMode::Scrambled : AddressMode::Positional;
        ModelSpace::Parts p;
        p.verts = digits(f[5]);
        p.faces = digits(f[6]);
        const bool addr_ok = sp.hex_of(sp.index_of(p, m)) == f[7];
        const bool back_ok = sp.parts_at(sp.parse(f[7]), m) == p;
        const std::string obj = sp.to_obj(p);
        const bool obj_ok = Sha256::hex(Sha256::hash(obj)) == f[8] && obj.size() == sp.obj_length() &&
                            obj.find("  ") == std::string::npos && sp.from_obj(obj) == p;
        CHECK(addr_ok && back_ok && obj_ok);
        if (!(addr_ok && back_ok && obj_ok))
            std::cerr << "  model vector mismatch: V" << f[0] << " F" << f[1] << " C" << f[2] << " " << f[4] << "\n";
        ++n;
    }
    std::cout << "model vectors checked: " << n << "\n";
    CHECK(n == 50); // a truncated or emptied vector file must fail, not pass quietly
}

void test_modelspace()
{
    const ModelSpace sp(8, 12, 16);
    CHECK(sp.id() == "models/V8/F12/C16/key=sieve/modelspace-v1");
    CHECK(sp.decimals() == 4);
    // N = 16^24 * 8^36 = 2^96 * 2^108 = 2^204.
    CHECK(sp.size() == BigUint::pow(2, 204));
    CHECK(sp.obj_length() == 8 * 26 + 12 * 8);

    // Address zero: every coordinate at the low end of the grid, every face (1, 1, 1).
    const ModelSpace::Parts zero = sp.parts_at(BigUint(0), AddressMode::Positional);
    CHECK(zero.verts.size() == 24 && zero.faces.size() == 36);
    CHECK(std::all_of(zero.verts.begin(), zero.verts.end(), [](uint32_t d) { return d == 0; }));
    const std::string obj0 = sp.to_obj(zero);
    CHECK(obj0.size() == sp.obj_length());
    CHECK(obj0.substr(0, 27) == "v -0.9375 -0.9375 -0.9375\nv");
    CHECK(sp.from_obj(obj0) == zero);

    // Digits, address and canonical text all agree, in both orderings.
    for (uint32_t seed = 1; seed <= 40; ++seed)
    {
        ModelSpace::Parts p;
        uint32_t x = seed * 2654435761u;
        auto next = [&] { x ^= x << 13; x ^= x >> 17; x ^= x << 5; return x; };
        for (uint32_t i = 0; i < 24; ++i) p.verts.push_back(next() % 16);
        for (uint32_t i = 0; i < 36; ++i) p.faces.push_back(next() % 8);
        for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            const BigUint k = sp.index_of(p, m);
            CHECK(k < sp.size());
            CHECK(sp.parts_at(k, m) == p);
            CHECK(sp.hex_of(k).size() == sp.hex_width());
        }
        const std::string obj = sp.to_obj(p);
        CHECK(obj.size() == sp.obj_length());
        CHECK(sp.from_obj(obj) == p);
        // The mesh reads back on the grid it was written from.
        const auto mesh = sp.mesh_of(p);
        CHECK(mesh.size() == 8);
        for (uint32_t i = 0; i < 8; ++i)
            CHECK(std::fabs(mesh[i].x - (2.0f * float(p.verts[size_t(i) * 3]) + 1.0f - 16.0f) / 16.0f) < 1e-6f);
    }

    // A cube fitted to the line comes back as a cube, centred and scaled to the grid.
    std::vector<ModelSpace::Vertex> cube;
    for (int i = 0; i < 8; ++i)
        cube.push_back({i & 1 ? 3.0f : 1.0f, i & 2 ? 3.0f : 1.0f, i & 4 ? 3.0f : 1.0f});
    std::vector<ModelSpace::Face> tris;
    for (uint32_t i = 0; i < 12; ++i) tris.push_back({i % 8, (i + 1) % 8, (i + 2) % 8});
    const ModelSpace::Fitted fitted = sp.fit(cube, tris);
    CHECK(fitted.vertices_dropped == 0 && fitted.faces_dropped == 0 && fitted.faces_added == 0);
    const auto back = sp.mesh_of(fitted.parts);
    // The cube is symmetric on the grid: the two extremes are equal and opposite.
    CHECK(std::fabs(back[0].x + 0.9375f) < 1e-6f && std::fabs(back[7].x - 0.9375f) < 1e-6f);
    CHECK(sp.faces_of(fitted.parts)[3].a == 3);
    // Too few vertices and faces: the rest are the origin and a filler face.
    const ModelSpace::Fitted thin = sp.fit({{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}, {{0, 1, 2}});
    CHECK(thin.faces_added == 11 && thin.parts.verts[23] == 8);
    // Too many: the extras are dropped and counted.
    cube.push_back({0, 0, 0});
    tris.push_back({0, 1, 2});
    const ModelSpace::Fitted fat = sp.fit(cube, tris);
    CHECK(fat.vertices_dropped == 1 && fat.faces_dropped == 1);

    // Shapes the line cannot have.
    CHECK(throws([] { ModelSpace(2, 1, 16); }));
    CHECK(throws([] { ModelSpace(8, 0, 16); }));
    CHECK(throws([] { ModelSpace(8, 12, 12); }));  // not a power of two
    CHECK(throws([] { ModelSpace(8, 12, 8192); })); // past the grid limit
    // Text that is not the canonical form.
    CHECK(throws([&] { sp.from_obj(obj0.substr(1)); }));
    CHECK(throws([&] { ModelSpace(8, 12, 16).from_obj(std::string(obj0.size(), 'x')); }));
    // A different shape is a different line, and its addresses are a different width.
    const ModelSpace other(8, 12, 32);
    CHECK(other.id() != sp.id() && other.size() > sp.size() && other.decimals() == 5);
}

void test_alphabet_vectors(const std::string& path)
{
    std::ifstream in(path);
    CHECK(in.good());
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        size_t start = 0;
        for (size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || line[i] == '\t') { f.push_back(line.substr(start, i - start)); start = i + 1; }
        CHECK(f.size() == 7);
        if (f.size() != 7) continue;
        const Alphabet& al = alphabet_of(f[0]);
        const bool size_ok = al.size() == std::stoul(f[1]);
        // The unit, as comma-separated hex code points.
        std::u32string unit;
        for (size_t i = 0, p = 0; i <= f[4].size(); ++i)
            if (i == f[4].size() || f[4][i] == ',')
            {
                unit.push_back(char32_t(std::stoul(f[4].substr(p, i - p), nullptr, 16)));
                p = i + 1;
            }
        const Space sp(al, static_cast<uint32_t>(std::stoul(f[2])), f[3]);
        const auto d = sp.digits_of(unit);
        const bool pos_ok = sp.address_of(d, AddressMode::Positional) == f[5];
        const bool scr_ok = sp.address_of(d, AddressMode::Scrambled) == f[6];
        const bool back_ok = sp.unit_at(f[5], AddressMode::Positional) == d && sp.unit_at(f[6], AddressMode::Scrambled) == d;
        CHECK(size_ok && pos_ok && scr_ok && back_ok);
        if (!(size_ok && pos_ok && scr_ok && back_ok))
            std::cerr << "  alphabet vector mismatch: " << f[0] << " L=" << f[2] << " key=" << f[3] << "\n";
        ++n;
    }
    std::cout << "alphabet vectors checked: " << n << "\n";
    CHECK(n == 270); // a truncated or emptied vector file must fail, not pass quietly
}

void test_vectors(const std::string& path)
{
    std::ifstream in(path);
    CHECK(static_cast<bool>(in));
    if (!in) { std::cerr << "cannot open vectors file " << path << "\n"; return; }
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        size_t start = 0;
        for (size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || line[i] == '\t') { f.push_back(line.substr(start, i - start)); start = i + 1; }
        CHECK(f.size() == 6);
        if (f.size() != 6) continue;
        const Space sp(alphabet_by_id(f[0]), static_cast<uint32_t>(std::stoul(f[1])), f[2]);
        const auto d = sp.digits_of(utf8_decode(f[3]));
        const bool pos_ok = sp.address_of(d, AddressMode::Positional) == f[4];
        const bool scr_ok = sp.address_of(d, AddressMode::Scrambled) == f[5];
        const bool back_ok = sp.unit_at(f[4], AddressMode::Positional) == d && sp.unit_at(f[5], AddressMode::Scrambled) == d;
        CHECK(pos_ok && scr_ok && back_ok);
        if (!(pos_ok && scr_ok && back_ok)) std::cerr << "  vector mismatch: " << f[0] << " L=" << f[1] << " key=" << f[2] << "\n";
        ++n;
    }
    std::cout << "conformance vectors checked: " << n << "\n";
    CHECK(n == 160); // a truncated or emptied vector file must fail, not pass quietly
    CHECK(n > 100);
}

void test_sieve()
{
    const Dictionary dict = Dictionary::from_words(
        {"a", "i", "at", "cat", "the", "then", "hen", "he", "in", "tin", "at", "ate", "eat", "tea", "ten", "net", "no"});
    CHECK(dict.is_word("cat") && !dict.is_word("ca"));
    CHECK(dict.is_word_prefix("ca") && !dict.is_word_prefix("cx"));
    CHECK(dict.is_suffix("en") && dict.is_substring("he") && dict.is_substring("h") && !dict.is_substring("q"));

    CHECK(unit_passes("the cat", SieveFilter::Words, dict));
    CHECK(!unit_passes("the  cat", SieveFilter::Clean, dict));
    CHECK(!unit_passes("       ", SieveFilter::Clean, dict));
    CHECK(unit_passes("en the ca", SieveFilter::Window, dict));  // suffix, word, prefix
    CHECK(!unit_passes("en the ca", SieveFilter::Words, dict));
    CHECK(unit_passes("he", SieveFilter::Window, dict));         // substring of "the"
    CHECK(!unit_passes("xq at", SieveFilter::Window, dict));

    for (uint32_t L = 1; L <= 5; ++L)
    {
        const SieveCount exact = sieve_counted(L, dict);
        const SieveCount brute = sieve_brute(L, dict, 2);
        CHECK(exact.clean == brute.clean);
        CHECK(exact.window == brute.window);
        CHECK(exact.words == brute.words);
        CHECK(BigUint(sieve_pruned(L, SieveFilter::Window, dict, 2).survivors) == exact.window);
        CHECK(BigUint(sieve_pruned(L, SieveFilter::Words, dict, 2).survivors) == exact.words);
        CHECK(BigUint(sieve_pruned(L, SieveFilter::Clean, dict, 2).survivors) == exact.clean);
    }
    CHECK(prefix_tree_nodes(2).to_decimal() == "757"); // 1 + 27 + 729
}


// A small lower27 model trained on a few sentences, for exhaustive checks at tiny lengths.
std::shared_ptr<const CharModel> small_model(uint32_t order = 3, uint32_t min_count = 2)
{
    const std::string text =
        "it was the best of times it was the worst of times it was the age of wisdom it was the age of "
        "foolishness it was the epoch of belief it was the epoch of incredulity it was the season of light";
    const Alphabet& a = alphabet_by_id("lower27");
    std::vector<uint32_t> stream;
    for (char c : text) stream.push_back(*a.digit_of(char32_t(c)));
    ModelParams p{"lower27", 27, order, min_count, 0u, "test sentences"};
    return std::make_shared<const CharModel>(CharModel::train(stream, p));
}

void test_model()
{
    const auto m = small_model();
    // Every table is a full distribution over 27 symbols summing to 2^16, each entry >= 1.
    for (const std::vector<uint32_t>& h : std::vector<std::vector<uint32_t>>{{}, {20}, {20, 8}, {20, 8, 5}, {0, 0}, {7, 0, 0}})
    {
        const uint32_t* cum = m->cumulative(h);
        bool ok = cum[0] == 0 && cum[27] == kModelTotal;
        for (int s = 0; s < 27; ++s) ok = ok && cum[s + 1] > cum[s];
        CHECK(ok);
    }
    // After "th", 'e' is the likeliest next symbol; the padding context strongly predicts SPACE.
    {
        const uint32_t* cum = m->cumulative(std::vector<uint32_t>{20, 8});
        uint32_t best = 0;
        for (uint32_t s = 1; s < 27; ++s)
            if (cum[s + 1] - cum[s] > cum[best + 1] - cum[best]) best = s;
        CHECK(best == 5);
        const uint32_t* pad = m->cumulative(std::vector<uint32_t>{5, 0, 0});
        CHECK(pad[1] - pad[0] == kModelTotal - 26);
    }
    // The file form round-trips exactly, and is the only accepted spelling.
    const std::string file = m->serialise();
    const CharModel back = CharModel::parse(file);
    CHECK(back.serialise() == file);
    CHECK(back.sha256() == m->sha256());
    CHECK(back.context_count() == m->context_count());
    CHECK(throws([&] { CharModel::parse(file + "\n"); }));
    std::string altered = file;
    altered.replace(altered.find("min_count 2"), 11, "min_count 02");
    CHECK(throws([&] { CharModel::parse(altered); }));
}

void test_guided()
{
    const auto m = small_model();
    const Alphabet& a = alphabet_by_id("lower27");
    // Exhaustive at L = 1..3: the intervals of all 27^L units tile [0, 2^S) in dictionary order,
    // every unit's address lies inside its own interval, is the shortest fraction that does,
    // decodes back to the unit, and no two units share one.
    for (uint32_t L = 1; L <= 3; ++L)
    {
        const GuidedLine g(m, L);
        Space sp(a, L);
        const uint64_t count = uint64_t(std::pow(27.0, L) + 0.5);
        BigUint expected_low;
        std::set<std::string> codes;
        bool tiled = true, inside = true, shortest = true, decodes = true;
        for (uint64_t i = 0; i < count; ++i)
        {
            const auto unit = BigUint(i).to_digits(27, L);
            const auto iv = g.interval(unit);
            tiled = tiled && iv.low == expected_low;
            expected_low = iv.low;
            expected_low += iv.width;
            const auto c = g.code_of(iv);
            BigUint hi = iv.low;
            hi += iv.width;
            inside = inside && iv.low <= c.point && c.point < hi;
            // The block of 2^-bits at the address lies inside the arc, and no block one bit
            // coarser fits anywhere in it.
            {
                const size_t t = g.scale_bits() - c.bits;
                BigUint end = c.point, block(1);
                block <<= t;
                end += block;
                inside = inside && end <= hi;
                if (t < g.scale_bits())
                {
                    BigUint coarse = BigUint(1);
                    coarse <<= t + 1;
                    BigUint up = coarse;
                    up -= BigUint(1);
                    up += iv.low;
                    up >>= t + 1;
                    up <<= t + 1;
                    up += coarse;
                    shortest = shortest && up > hi;
                }
            }
            decodes = decodes && g.unit_at(c.point) == unit && g.unit_at(g.point_of(c.hex)) == unit;
            codes.insert(c.hex);
        }
        BigUint full(1);
        full <<= g.scale_bits();
        CHECK(tiled);
        CHECK(expected_low == full);
        CHECK(inside);
        CHECK(shortest);
        CHECK(decodes);
        CHECK(codes.size() == count);
    }
    // Longer units: round trip, and the address costs about the model's information content.
    const GuidedLine g(m, 40);
    auto digits = [&](const std::string& t) {
        std::vector<uint32_t> d;
        for (char c : t) d.push_back(*a.digit_of(char32_t(c)));
        d.resize(40, 0);
        return d;
    };
    const auto likely = digits("it was the best of times it was the wors");
    const auto noise = digits("qzxv jjkw pqpq zzzz xkcd vvvv wqwq kkkk ");
    const auto cl = g.code(likely), cn = g.code(noise);
    CHECK(g.unit_at(cl.point) == likely);
    CHECK(g.unit_at(cn.point) == noise);
    CHECK(cl.bits < 40);          // memorised text is nearly free
    CHECK(cn.bits > 6 * cl.bits); // noise is expensive
    const double info = g.information_bits(g.interval(likely));
    CHECK(double(cl.bits) >= info - 1e-9 && double(cl.bits) < info + 2.0); // -log2 P <= bits < -log2 P + 2
    // Padding costs almost nothing after its first SPACE.
    const auto short_text = digits("it was");
    CHECK(g.code(short_text).bits < 60);
    // Any point decodes; random points land inside the arc of the unit they decode to.
    std::mt19937_64 rng(3);
    bool ok = true;
    for (int i = 0; i < 200; ++i)
    {
        BigUint p;
        for (size_t b = 0; b < g.scale_bits(); b += 32)
        {
            p <<= 32;
            p.add_small(uint32_t(rng()));
        }
        p >>= (g.scale_bits() + 31) / 32 * 32 - g.scale_bits();
        const auto u = g.unit_at(p);
        const auto iv = g.interval(u);
        BigUint hi = iv.low;
        hi += iv.width;
        ok = ok && iv.low <= p && p < hi;
    }
    CHECK(ok);
    // Stepping wraps around the line exactly.
    BigUint one(1);
    const BigUint last = g.step(BigUint(), -1, 8);
    CHECK(g.step(last, 1, 8).is_zero());
    CHECK(g.step(BigUint(), 256, 8).is_zero());
    CHECK(g.hex_of(g.step(BigUint(), 128, 8), 1) == "8");
    CHECK(g.hex_of(g.step(BigUint(), 128, 8), 8) == "80");
    CHECK(g.point_of("8") == g.step(BigUint(), 1, 1));
    CHECK(throws([&] { (void)g.point_of("xyz"); }));
}

// Guided vectors from the Python oracle, which derives every table itself from the model file.
void test_guided_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_guided_v1.tsv");
    CHECK(bool(in));
    std::string line, sha;
    std::getline(in, line);
    std::getline(in, line);
    const std::string tag = "# model sha256 ";
    CHECK(line.rfind(tag, 0) == 0);
    sha = line.substr(tag.size());
    const auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    CHECK(model->sha256() == sha);
    const Alphabet& a = alphabet_by_id("lower27");
    std::map<uint32_t, std::unique_ptr<GuidedLine>> lines;
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        CHECK(f.size() == 5);
        if (f.size() != 5) continue;
        const uint32_t L = uint32_t(std::stoul(f[1]));
        auto& g = lines[L];
        if (!g) g = std::make_unique<GuidedLine>(model, L);
        std::vector<uint32_t> unit;
        for (char c : f[2]) unit.push_back(*a.digit_of(char32_t(c)));
        CHECK(unit.size() == L);
        if (f[0] == "code")
        {
            const auto c = g->code(unit);
            CHECK(std::to_string(c.bits) == f[3]);
            CHECK(c.hex == f[4]);
            CHECK(g->unit_at(g->point_of(f[4])) == unit);
        }
        else CHECK(g->unit_at(g->point_of(f[4])) == unit);
        ++n;
    }
    std::cout << "guided vectors checked: " << n << "\n";
    CHECK(n == 51); // a truncated or emptied vector file must fail, not pass quietly
}

// Resources for filter tests: one dictionary and the default model.
class TestResources : public FilterResources
{
public:
    TestResources(std::shared_ptr<const Dictionary> d, std::shared_ptr<const CharModel> m) : d_(std::move(d)), m_(std::move(m)) {}
    std::shared_ptr<const Dictionary> dictionary(const std::string&) const override { return d_; }
    std::shared_ptr<const CharModel> model(const std::string&, const std::string&) const override { return m_; }

private:
    std::shared_ptr<const Dictionary> d_;
    std::shared_ptr<const CharModel> m_;
};

FilterLine text_line(uint32_t L)
{
    const Alphabet& a = alphabet_by_id("lower27");
    return FilterLine{"text", "lower27", 27, L, &a, 0, 0, 0};
}

std::vector<uint32_t> digits27(const std::string& t)
{
    std::vector<uint32_t> d;
    for (char c : t) d.push_back(c == ' ' ? 0 : uint32_t(c - 'a' + 1));
    return d;
}

void check_ranker_exhaustive(const FilterStack& st, uint32_t base, uint32_t L, bool& ok);

// ---------------------------------------------------------------- filter plugins

std::shared_ptr<const PluginDef> load_plugin_file(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream s;
    s << in.rdbuf();
    const std::string text = s.str();
    CHECK(!text.empty());
    const size_t slash = path.find_last_of('/');
    return parse_plugin(text, Sha256::hex(Sha256::hash(text)), slash == std::string::npos ? std::string(".") : path.substr(0, slash));
}

bool plugin_parse_fails(const std::string& text, const std::string& expect)
{
    try
    {
        const auto p = parse_plugin(text, "0");
        static const TestResources none(nullptr, nullptr);
        (void)compile_plugin(*p, text_line(4), {}, none);
    }
    catch (const std::invalid_argument& e)
    {
        if (std::string(e.what()).find(expect) != std::string::npos) return true;
        std::cerr << "  plugin error was: " << e.what() << "\n";
        return false;
    }
    return false;
}

// The engine against the built-in filters its reference plugins port, and its own rules.
void test_plugins(const std::string& dir)
{
    const std::string filters = dir + "../data/filters/";
    const TestResources none(nullptr, nullptr);
    std::mt19937_64 rng(20260928);

    // clean-data-v1 counts exactly as clean-v1, and judges every unit the same.
    const auto clean = load_plugin_file(filters + "clean-data-v1.sfilter");
    const FilterSpec clean_spec = plugin_spec(clean);
    for (uint32_t L : {1u, 2u, 3u, 6u, 32u, 200u})
    {
        const FilterStack plug(text_line(L), {{&clean_spec, {}}}, none), built(text_line(L), {{find_filter("clean-v1"), {}}}, none);
        CHECK(plug.ranker() && built.ranker() && plug.ranker()->count() == built.ranker()->count());
        for (int i = 0; i < 300; ++i)
        {
            std::vector<uint32_t> u(L);
            for (auto& c : u) c = uint32_t(rng() % 4 == 0 ? 0 : rng() % 27);
            CHECK(plug.passes(u) == built.passes(u));
            if (plug.passes(u)) CHECK(plug.ranker()->unrank(plug.ranker()->rank(u)) == u);
        }
    }

    // clean-data-v2 is clean-v2 (trailing SPACE padding) as a table: the same counts, and every unit
    // of up to 4 symbols, and padding-heavy units at 32 and 200, judged the same.
    const auto clean2 = load_plugin_file(filters + "clean-data-v2.sfilter");
    const FilterSpec clean2_spec = plugin_spec(clean2);
    for (uint32_t L : {1u, 2u, 3u, 4u, 6u, 32u, 200u})
    {
        const FilterStack plug(text_line(L), {{&clean2_spec, {}}}, none), built(text_line(L), {{find_filter("clean-v2"), {}}}, none);
        CHECK(plug.ranker() && built.ranker() && plug.ranker()->count() == built.ranker()->count());
        if (L <= 4)
        {
            bool same = true;
            std::vector<uint32_t> u(L);
            uint64_t all = 1;
            for (uint32_t i = 0; i < L; ++i) all *= 27;
            for (uint64_t x = 0; x < all; ++x)
            {
                uint64_t y = x;
                for (uint32_t i = L; i-- > 0; y /= 27) u[i] = uint32_t(y % 27);
                same = same && plug.passes(u) == built.passes(u);
            }
            CHECK(same);
        }
        for (int i = 0; i < 300; ++i)
        {
            std::vector<uint32_t> u(L);
            const uint32_t pad = uint32_t(rng() % (L + 1)); // a run of SPACEs at the end, of any length
            for (uint32_t k = 0; k < L; ++k) u[k] = k >= L - pad ? 0 : uint32_t(rng() % 4 == 0 ? 0 : rng() % 27);
            CHECK(plug.passes(u) == built.passes(u));
            if (plug.passes(u)) CHECK(plug.ranker()->unrank(plug.ranker()->rank(u)) == u);
        }
    }

    // max-run-data-v1 judges as max-run-v1 at every setting, exhaustively at length 3 and on
    // run-heavy random units at 24, and (unlike the built-in) counts.
    const auto run = load_plugin_file(filters + "max-run-data-v1.sfilter");
    const FilterSpec run_spec = plugin_spec(run);
    for (int max = 1; max <= 4; ++max)
    {
        const FilterValues pv{{"max", std::to_string(max)}}, bv{{"max_run", std::to_string(max)}};
        const FilterStack p3(text_line(3), {{&run_spec, pv}}, none), b3(text_line(3), {{find_filter("max-run-v1"), bv}}, none);
        uint64_t n = 0;
        bool same = true;
        for (uint32_t x = 0; x < 27 * 27 * 27; ++x)
        {
            const std::vector<uint32_t> u{x / 729, x / 27 % 27, x % 27};
            same = same && p3.passes(u) == b3.passes(u);
            n += b3.passes(u) ? 1 : 0;
        }
        CHECK(same);
        CHECK(p3.ranker() && p3.ranker()->count() == BigUint(n));
        const FilterStack p24(text_line(24), {{&run_spec, pv}}, none), b24(text_line(24), {{find_filter("max-run-v1"), bv}}, none);
        for (int i = 0; i < 400; ++i)
        {
            std::vector<uint32_t> u(24);
            for (size_t k = 0; k < u.size(); ++k) u[k] = k > 0 && rng() % 2 ? u[k - 1] : uint32_t(rng() % 27);
            CHECK(p24.passes(u) == b24.passes(u));
        }
    }

    // tidy-data-v2 is tidy-data-v1 with the built-in max-run-v1 in place of max-run-data-v1: with the
    // prerequisites each brings, the same count at every length tried and the same verdicts, so
    // max-run-data-v1 and tidy-data-v1 can retire for them.
    {
        const auto t1 = load_plugin_file(filters + "tidy-data-v1.sfilter"), t2 = load_plugin_file(filters + "tidy-data-v2.sfilter");
        const auto clean = load_plugin_file(filters + "clean-data-v1.sfilter");
        const FilterSpec t1_spec = plugin_spec(t1), t2_spec = plugin_spec(t2), clean_spec = plugin_spec(clean);
        CHECK(t1_spec.retired && t1_spec.replaced_by == "tidy-data-v2" && !t2_spec.retired);
        CHECK(run_spec.retired && run_spec.replaced_by == "max-run-v1");
        CHECK(t2_spec.prerequisites.size() == 2);
        for (uint32_t L : {1u, 2u, 5u, 12u, 32u, 200u})
            for (const char* longest : {"3", "20"})
            {
                const FilterValues lv{{"longest", longest}};
                const FilterStack v1(text_line(L), {{&t1_spec, lv}, {&clean_spec, {}}, {&run_spec, {{"max", "3"}}}}, none);
                const FilterStack v2(text_line(L), {{&t2_spec, lv}, {&clean_spec, {}}, {find_filter("max-run-v1"), {{"max_run", "3"}}}}, none);
                CHECK(v1.ranker() && v2.ranker() && v1.ranker()->count() == v2.ranker()->count());
                for (int i = 0; i < (L <= 12 ? 200 : 40); ++i)
                {
                    std::vector<uint32_t> u(L);
                    for (size_t k = 0; k < u.size(); ++k) u[k] = k > 0 && rng() % 3 == 0 ? u[k - 1] : uint32_t(rng() % 5 == 0 ? 0 : rng() % 27);
                    CHECK(v1.passes(u) == v2.passes(u));
                }
            }
    }

    // The token form's padding and within: words-data-v2, window-data-v2 and title-data-v1 judge as
    // words-v2, window-v2 and title-v1 do (padding-heavy units, cut words at both ends), count the
    // same, and a padded plugin still merges into a stack's automaton (title-data-v1, with within,
    // counts on its own, as title-v1 does).
    {
        struct Pair { const char* plugin; const char* builtin; FilterValues pv, bv; };
        // (A dictionary id of its own: compiled automata are kept by the plugin, the line and the
        // settings, a dictionary by its id, and the other tests' dictionaries come by the default.)
        const FilterValues own{{"dictionary", "padtest"}};
        const std::vector<Pair> pairs = {
            {"words-data-v2", "words-v2", own, own},
            {"window-data-v2", "window-v2", own, own},
            {"title-data-v1", "title-v1", {{"dictionary", "padtest"}, {"max_length", "9"}}, {{"dictionary", "padtest"}, {"max_length", "9"}}},
        };
        static const char* const words[] = {"the", "cat", "sat", "on", "a", "mat", "dog", "ran", "of", "and", "zq", "xx"};
        auto pdict = std::make_shared<const Dictionary>(Dictionary::from_words({"the", "cat", "sat", "on", "a", "mat", "dog", "ran", "of", "and"}));
        const TestResources pres(pdict, nullptr);
        for (const Pair& pr : pairs)
        {
            const auto def = load_plugin_file(filters + std::string(pr.plugin) + ".sfilter");
            const FilterSpec spec = plugin_spec(def);
            CHECK(spec.counts_as == (std::string(pr.plugin) == "title-data-v1" ? "own" : "automaton"));
            for (uint32_t L : {1u, 4u, 9u, 16u})
            {
                const FilterStack p(text_line(L), {{&spec, pr.pv}}, pres), b(text_line(L), {{find_filter(pr.builtin), pr.bv}}, pres);
                CHECK(p.ranker() && b.ranker() && p.ranker()->count() == b.ranker()->count());
                if (L <= 4) // every unit
                {
                    uint64_t total = 1;
                    for (uint32_t k = 0; k < L; ++k) total *= 27;
                    std::vector<uint32_t> u(L);
                    bool same = true;
                    for (uint64_t x = 0; x < total; ++x)
                    {
                        uint64_t y = x;
                        for (uint32_t k = 0; k < L; ++k, y /= 27) u[k] = uint32_t(y % 27);
                        same = same && p.passes(u) == b.passes(u);
                    }
                    CHECK(same);
                }
                for (int i = 0; i < 300; ++i)
                {
                    // Words and SPACEs (sometimes two together), cut anywhere, then padding of any length.
                    std::vector<uint32_t> u;
                    while (u.size() < L)
                    {
                        const char* w = words[rng() % 12];
                        for (const char* c = w; *c; ++c) u.push_back(uint32_t(*c - 'a' + 1));
                        for (uint32_t sp = 1 + (rng() % 7 == 0); sp-- > 0;) u.push_back(0);
                    }
                    const size_t from = rng() % 3 == 0 ? rng() % 3 : 0;
                    std::vector<uint32_t> v(u.begin() + std::ptrdiff_t(from), u.begin() + std::ptrdiff_t(std::min(u.size(), from + L)));
                    while (v.size() < L) v.push_back(0);
                    const uint32_t pad = uint32_t(rng() % (L + 1));
                    for (uint32_t k = L - pad; k < L; ++k) v[k] = 0;
                    CHECK(p.passes(v) == b.passes(v));
                }
            }
        }
        // Long pages: where a plain dictionary plugin's automaton table does not fit the filter
        // memory, it counts by its words' lengths, as the built-ins do (plugin.hpp word_counting),
        // with the same count, and ranks and unranks its survivors.
        {
            const double was = filter_memory();
            set_filter_memory(1024.0 * 1024); // 1 MB: the automaton's table at 1,500 symbols is far over it
            for (const char* port : {"words-data-v1", "window-data-v1", "words-data-v2", "window-data-v2"})
            {
                const auto def = load_plugin_file(filters + std::string(port) + ".sfilter");
                const FilterSpec spec = plugin_spec(def);
                const std::string builtin = std::string(port).substr(0, std::string(port).find("-data")) + std::string(port).substr(std::string(port).size() - 3);
                const FilterStack p(text_line(1500), {{&spec, own}}, pres), b(text_line(1500), {{find_filter(builtin), own}}, pres);
                const Ranker* pr = p.ranker();
                CHECK(pr && b.ranker() && pr->count() == b.ranker()->count());
                CHECK(pr && dynamic_cast<const DfaRanker*>(pr) == nullptr); // counted by word lengths, not the table
                if (pr && !pr->count().is_zero())
                {
                    BigUint k = pr->count();
                    k.divmod_small(7);
                    const auto u = pr->unrank(k);
                    CHECK(p.passes(u) && b.passes(u) && pr->rank(u) == k);
                }
            }
            set_filter_memory(was);
        }
        // Padding keeps a plugin an automaton: words-data-v2 and clean-data-v2 merge, and the stack counts as words-v2 alone.
        const auto w2 = load_plugin_file(filters + "words-data-v2.sfilter"), c2 = load_plugin_file(filters + "clean-data-v2.sfilter");
        const FilterSpec w2s = plugin_spec(w2), c2s = plugin_spec(c2);
        const FilterStack both(text_line(20), {{&w2s, own}, {&c2s, {}}}, pres), alone(text_line(20), {{find_filter("words-v2"), own}}, pres);
        CHECK(both.ranker() && alone.ranker() && both.ranker()->count() == alone.ranker()->count());
    }

    // melody-lengths-v2 and melody-ending-v2 (every note set, by real lengths: LENGTH, SIXTEENTHS)
    // count as the v1s on notes104 at every setting of their choices, so the v1s retire.
    {
        const auto l1 = load_plugin_file(filters + "melody-lengths-v1.sfilter"), l2 = load_plugin_file(filters + "melody-lengths-v2.sfilter");
        const auto e1 = load_plugin_file(filters + "melody-ending-v1.sfilter"), e2 = load_plugin_file(filters + "melody-ending-v2.sfilter");
        const FilterSpec l1s = plugin_spec(l1), l2s = plugin_spec(l2), e1s = plugin_spec(e1), e2s = plugin_spec(e2);
        CHECK(l1s.retired && l1s.replaced_by == "melody-lengths-v2" && e1s.retired && e1s.replaced_by == "melody-ending-v2");
        const FilterLine audio{"audio", "notes104", 104, 4, nullptr, 0, 0, 0};
        static const char* const codes[] = {"e", "q", "h", "w"};
        bool same = true;
        for (const char* lo : codes)
            for (const char* hi : codes)
                for (const char* n : {"0", "1", "3"})
                {
                    const FilterValues v{{"shortest", lo}, {"longest", hi}, {"eighths", n}};
                    const FilterStack a1(audio, {{&l1s, v}}, none), a2(audio, {{&l2s, v}}, none);
                    same = same && a1.ranker() && a2.ranker() && a1.ranker()->count() == a2.ranker()->count();
                }
        for (const char* t : {"C", "F#", "B"})
            for (const char* h : codes)
            {
                const FilterValues v{{"tonic", t}, {"hold", h}};
                const FilterStack a1(audio, {{&e1s, v}}, none), a2(audio, {{&e2s, v}}, none);
                same = same && a1.ranker() && a2.ranker() && a1.ranker()->count() == a2.ranker()->count();
            }
        CHECK(same);
    }

    // key-data-v1 counts as key-v1 in the major key on every tonic.
    static const char* const tonics[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    const auto key = load_plugin_file(filters + "key-data-v1.sfilter");
    const FilterSpec key_spec = plugin_spec(key);
    for (int t = 0; t < 12; ++t)
        for (uint32_t L : {1u, 7u})
        {
            const FilterLine audio{"audio", "notes104", 104, L, nullptr, 0, 0, 0};
            const FilterStack p(audio, {{&key_spec, {{"tonic", std::to_string(t)}}}}, none);
            const FilterStack b(audio, {{find_filter("key-v1"), {{"tonic", tonics[t]}, {"scale", "major"}}}}, none);
            CHECK(p.ranker() && b.ranker() && p.ranker()->count() == b.ranker()->count());
        }

    // A stack of two plugins ranks through their combined automaton, exactly.
    {
        const FilterStack both(text_line(3), {{&clean_spec, {}}, {&run_spec, {{"max", "1"}}}}, none);
        CHECK(both.ranker() != nullptr);
        uint64_t n = 0;
        for (uint32_t x = 0; x < 27 * 27 * 27; ++x)
            n += both.passes(std::vector<uint32_t>{x / 729, x / 27 % 27, x % 27}) ? 1 : 0;
        CHECK(both.ranker() && both.ranker()->count() == BigUint(n));
    }

    // The token form: words-data-v1 and window-data-v1 judge and count exactly as words-v1 and
    // window-v1 with the same dictionary, exhaustively at lengths 1 to 4 and on random units.
    {
        auto dict = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan", "tin", "at", "nit"}));
        const TestResources dres(dict, nullptr);
        for (const char* port : {"words", "window"})
        {
            const auto plug = load_plugin_file(filters + port + "-data-v1.sfilter");
            const FilterSpec spec = plugin_spec(plug);
            for (uint32_t L = 1; L <= 9; ++L)
            {
                const FilterStack p(text_line(L), {{&spec, {}}}, dres), b(text_line(L), {{find_filter(std::string(port) + "-v1"), {}}}, dres);
                CHECK(p.ranker() && b.ranker() && p.ranker()->count() == b.ranker()->count());
                bool same = true;
                if (L <= 4)
                {
                    uint64_t total = 1;
                    for (uint32_t k = 0; k < L; ++k) total *= 27;
                    std::vector<uint32_t> u(L);
                    for (uint64_t x = 0; x < total; ++x)
                    {
                        uint64_t y = x;
                        for (uint32_t k = 0; k < L; ++k, y /= 27) u[k] = uint32_t(y % 27);
                        same = same && p.passes(u) == b.passes(u);
                    }
                }
                for (int i = 0; i < 300; ++i)
                {
                    std::vector<uint32_t> u(L);
                    const std::string letters = "aint ";
                    for (auto& c : u) c = digits27(std::string(1, letters[rng() % letters.size()]))[0];
                    same = same && p.passes(u) == b.passes(u);
                }
                CHECK(same);
            }
        }
    }

    // A grammar: sets from word lists, follow, first and last, and a word in two sets.
    {
        const auto g = load_plugin_file(dir + "plugins/toy-grammar-v1.sfilter");
        const FilterSpec gs = plugin_spec(g);
        auto judge = [&](const std::string& t) {
            const FilterStack st(text_line(uint32_t(t.size())), {{&gs, {}}}, none);
            return st.passes(digits27(t));
        };
        CHECK(judge("the big red cat sat"));
        CHECK(judge("the dog run"));         // run as a verb
        CHECK(judge("a cat sees the run"));  // run as a noun
        CHECK(judge("cat run "));            // one trailing SPACE
        CHECK(!judge("big cat sat"));        // first must be det or noun
        CHECK(!judge("the cat the"));        // last must be noun or verb
        CHECK(!judge("the  cat"));           // two SPACEs
        CHECK(!judge("the cats"));           // not a word of any set
        CHECK(!judge("the sat"));            // det is not followed by a verb
    }

    // Tagged lists: two files as one, an excluded tag, capitals folded, and a word judged by the
    // tags of all its lines ("Sat", a noun in the second file, makes "sat" no verb).
    {
        const auto g = load_plugin_file(dir + "plugins/toy-tags-v1.sfilter");
        const FilterSpec gs = plugin_spec(g);
        auto judge = [&](const std::string& t) {
            const FilterStack st(text_line(uint32_t(t.size())), {{&gs, {}}}, none);
            return st.passes(digits27(t));
        };
        CHECK(judge("the cat sees a mat"));    // "a" from "A", "mat" from the second file
        CHECK(judge("it sees the old hill"));
        CHECK(!judge("it sees the green hill")); // "green" (AN) is a noun, not an adjective (A-N)
        CHECK(judge("run"));                   // "Run" folded
        CHECK(!judge("the cat sat"));          // "sat" is a noun too, so no verb
        CHECK(judge("the hill"));
        CHECK(!judge("the it"));               // "it" is a pronoun, never a noun (Np-r), in either file
    }

    // The Moby grammars (data/filters): real word order passes, the same words shuffled do not,
    // and a sentence must start and end as the grammar says. Judged on the automaton directly,
    // compiled once each in the run (the lists are large; the second SHA-256 pass reuses them).
    for (const std::string name : {"moby-grammar-v1", "moby-grammar-strict-v1"})
    {
        static std::map<std::string, Dfa> compiled;
        if (!compiled.count(name)) compiled[name] = compile_plugin(*load_plugin_file(filters + name + ".sfilter"), text_line(8), {}, none);
        const Dfa& d = compiled[name];
        const bool strict = name == "moby-grammar-strict-v1";
        CHECK(d.accepts(digits27("the old man sat by the fire")));
        CHECK(d.accepts(digits27("she walked slowly to the door")));
        CHECK(d.accepts(digits27("he had never seen such a thing")));
        CHECK(d.accepts(digits27("and so they went home together")));
        CHECK(!d.accepts(digits27("fire the by sat man old the")));
        CHECK(!d.accepts(digits27("door the to slowly walked she")));
        CHECK(!d.accepts(digits27("the the the")));
        CHECK(!d.accepts(digits27("quickly the of"))); // Moby's "OF" (a noun) does not make "of" one
        CHECK(!d.accepts(digits27("the old  man")));   // two SPACEs
        CHECK(!d.accepts(digits27("the old man zqxv"))); // not a word of the lists
        CHECK(d.accepts(digits27("sat")) == !strict);  // strict: a sentence starts with a determiner, pronoun, conjunction or interjection
    }

    // subset is exact: max-run at 1 keeps only what max-run at 3 keeps, not the other way round,
    // and a rule is a subset of itself.
    {
        const Dfa r1 = compile_plugin(*run, text_line(8), {{"max", "1"}}, none), r3 = compile_plugin(*run, text_line(8), {{"max", "3"}}, none);
        CHECK(subset(r1, r3) && !subset(r3, r1) && subset(r3, r3));
        const Dfa c = compile_plugin(*clean, text_line(8), {}, none);
        CHECK(!subset(c, r1) && !subset(r1, c));
    }

    // Minimising is canonical: the same rule with its states numbered differently compiles to
    // the same automaton.
    {
        const std::string head = "sieve-filter-v1\nid x\nversion 1\nauthor t\norigin human\nlines text\nsymbols lower27\n"
                                 "class space \" \"\nclass letter a-z\nstates 5\n";
        const auto a = parse_plugin(head + "start 0\naccept 2 3\nt 0 space 1\nt 0 letter 2\nt 1 letter 2\nt 2 letter 2\nt 2 space 3\nt 3 letter 2\nend\n", "a");
        const auto b = parse_plugin(head + "start 4\naccept 1 0\nt 4 space 2\nt 4 letter 1\nt 2 letter 1\nt 1 letter 1\nt 1 space 0\nt 0 letter 1\nend\n", "b");
        const Dfa da = compile_plugin(*a, text_line(4), {}, none), db = compile_plugin(*b, text_line(4), {}, none);
        CHECK(da.next == db.next && da.accept == db.accept && da.states() == 4);
    }

    // Mistakes are refused with their line.
    const std::string h = "sieve-filter-v1\nid x\nversion 1\nauthor t\norigin human\nlines text\nsymbols lower27\nstates 2\nstart 0\naccept 1\n";
    CHECK(plugin_parse_fails(h + "t 0 @1 1\nt 0 @1 0\nend\n", "line 12: state 0 on symbol @1 goes to both 1 and 0"));
    CHECK(plugin_parse_fails(h + "t 0 vowel 1\nend\n", "'vowel' is not a class"));
    CHECK(plugin_parse_fails(h + "t 0 @1 2\nend\n", "state 2 does not exist"));
    CHECK(plugin_parse_fails(h + "t 0 @27 1\nend\n", "@27 is not a symbol"));
    CHECK(plugin_parse_fails(h + "t 0 \"A\" 1\nend\n", "is not a symbol of lower27"));
    CHECK(plugin_parse_fails(h + "t 0 @1 1\n", "ends with end"));
    CHECK(plugin_parse_fails(h + "for i 0 3\nt 0 @1 1\nend\n", "not closed by done"));
    CHECK(plugin_parse_fails(h + "t 0 @{n} 1\nend\n", "'n' is not a parameter"));
    CHECK(plugin_parse_fails(h + "t 0 @{5 / 0} 1\nend\n", "/ is for"));
    CHECK(plugin_parse_fails("sieve-filter-v1\nid x\nversion 1\nrequires clean\nend\n", "requires names a filter with its version"));
    CHECK(plugin_parse_fails("sieve-filter-v1\nid x\nversion 1\nrequires clean-v1 max\nend\n", "a pinned setting is NAME=VALUE"));
    CHECK(plugin_parse_fails("sieve-filter-v1\nid x\nversion 1\nauthor t\norigin human\nlines text\nsymbols lower27\nparam n int 9 1 5\nend\n",
                             "default must lie between"));

    // sieve-filter-v2: what v1 does not have is refused there, and v2's own mistakes are named.
    const std::string h2 = "sieve-filter-v2\nid x\nversion 1\nauthor t\norigin human\nlines text\nsymbols lower27\n";
    CHECK(plugin_parse_fails(h + "if {1}\nt 0 @1 1\nfi\nend\n", "unknown keyword 'if'"));
    CHECK(plugin_parse_fails("sieve-filter-v1\nid x\nversion 1\nauthor t\norigin human\nlines text\nsymbols lower27\nparam m choice a a,b\nend\n", "a choice parameter needs sieve-filter-v2"));
    CHECK(plugin_parse_fails("sieve-filter-v1\nid x\nversion 1\nsymbols notes*\nend\n", "need sieve-filter-v2"));
    CHECK(plugin_parse_fails(h + "t 0 @{1 < 2} 1\nend\n", "cannot read the expression"));
    CHECK(plugin_parse_fails(h2 + "states 2\nstart 0\nfi\nend\n", "fi without its if"));
    CHECK(plugin_parse_fails(h2 + "states 2\nstart 0\nif {1}\nelse\nelse\nfi\nend\n", "an if has one else"));
    CHECK(plugin_parse_fails(h2 + "states 2\nstart 0\nif {1}\nend\n", "an if is not closed by fi"));
    CHECK(plugin_parse_fails(h2 + "param BASE int 1 0 2\nstates 2\nstart 0\nend\n", "kept for the format"));
    CHECK(plugin_parse_fails(h2 + "param m choice c a,b\nstates 2\nstart 0\nend\n", "is not one of the choices"));
    CHECK(plugin_parse_fails(h2 + "states 2\nstart 0\nt 0 @{abs(1, 2)} 1\nend\n", "abs takes 1 argument"));
    CHECK(plugin_parse_fails(h2 + "states 2\nstart 0\nfor min 0 1\ndone\nend\n", "kept for the format"));
    {
        // The v2 expressions, worked out: comparisons give 1 or 0, && || ! on truth, min max abs.
        const auto p = parse_plugin(h2 + "states 40\nstart 0\naccept 0\n"
                                         "t 0 @{(3 < 5) + (5 <= 5) * 2 + (2 == 3) * 4 + (1 != 0) * 8} {abs(0 - 7) + min(4, 9) + max(-3, 1)}\n"
                                         "t 12 @{!0 + !5 * 2 + (0 || 3) * 4 + (2 && 0) * 8} 0\nend\n", "v2");
        const Dfa d = compile_plugin(*p, text_line(2), {}, none);
        // @11 from 0 to state 12 (7 + 4 + 1), then @5 back to 0: of every two-letter unit, only that.
        CHECK(d.accepts(std::vector<uint32_t>{11, 5}));
        const Dfa m = minimise(d);
        CHECK(DfaRanker(m, 2).count() == BigUint(1));
    }

    // PackedRows: numbers of every size back exactly (zeros known without reading them), a row
    // read while the next is worked out, and its estimate never under what it holds.
    {
        PackedRows rows;
        rows.add_row(5, [](size_t i, BigUint& v) {
            if (i % 2) v = BigUint::pow(3, uint64_t(i) * 50);
        });
        rows.add_row(5, [&](size_t i, BigUint& v) {
            v.add_mul_small(rows.limbs(0, i), 7);
            v.add_mul_small(rows.limbs(0, 4 - i), 2);
        });
        rows.finish();
        bool same = rows.rows() == 2;
        for (size_t i = 0; i < 5; ++i)
        {
            const BigUint a = i % 2 ? BigUint::pow(3, uint64_t(i) * 50) : BigUint();
            const BigUint b4 = (4 - i) % 2 ? BigUint::pow(3, uint64_t(4 - i) * 50) : BigUint();
            BigUint want = BigUint::mul(a, BigUint(7));
            want += BigUint::mul(b4, BigUint(2));
            same = same && rows.value(0, i) == a && rows.is_zero(0, i) == a.is_zero() && rows.value(1, i) == want;
        }
        CHECK(same);
        CHECK(PackedRows::estimate(2, 5, 3 * std::log2(3.0) * 150) >= rows.bytes());
    }
    // DfaRanker's packed table, and a ranker on a longer one's table: the same counts, ranks and
    // units as a table built at that length, at every length up to the longer one's.
    {
        const auto p = parse_plugin(h2 + "states 3\nstart 0\naccept 0 1\nt 0 @1 1\nt 0 @2 0\nt 1 @2 0\nt 1 @3 2\nend\n", "packed");
        const Dfa m = minimise(compile_plugin(*p, text_line(30), {}, none));
        const DfaRanker longer(m, 30);
        bool same = longer.table_length() == 30;
        for (uint32_t L = 0; L <= 30; ++L)
        {
            const DfaRanker own(m, L), view(longer, L);
            same = same && own.count() == view.count() && view.table_length() == 30;
            if (own.count().is_zero()) continue;
            BigUint k = own.count();
            k.divmod_small(3);
            const auto u = own.unrank(k);
            same = same && view.unrank(k) == u && view.rank(u) == k;
        }
        CHECK(same);
        bool threw = false;
        try
        {
            (void)DfaRanker(longer, 31);
        }
        catch (const std::invalid_argument&)
        {
            threw = true;
        }
        CHECK(threw);
    }

    // The melody plugins (data/filters, sieve-filter-v2 on notes104): what they let through, in
    // notation, and a stack of three counted exactly, against every unit at length 3.
    {
        auto notes = [](const std::string& notation) {
            const auto c = canonicalise_notes(notation, uint32_t(std::count(notation.begin(), notation.end(), ' ') + 1));
            return c.units.front();
        };
        auto audio = [](uint32_t L) { return FilterLine{"audio", kNotesSymbolsId, kNoteSymbols, L, nullptr, 0, 0, 0}; };
        std::map<std::string, std::shared_ptr<const PluginDef>> m;
        for (const char* f : {"key-data-v2", "melody-leap-v1", "melody-lengths-v1", "melody-rests-v1", "melody-ending-v1", "melody-range-v1"})
            m[f] = load_plugin_file(filters + f + ".sfilter");
        auto judge = [&](const std::string& f, const std::string& notation, const FilterValues& v = {}) {
            const auto u = notes(notation);
            return compile_plugin(*m[f], audio(uint32_t(u.size())), v, none).accepts(u);
        };
        CHECK(judge("key-data-v2", "C4q E4q G4h Rq B5w"));
        CHECK(!judge("key-data-v2", "C4q Eb4q G4h"));
        CHECK(judge("key-data-v2", "A4q C5q E5h G5q", {{"tonic", "A"}, {"scale", "minor"}}));
        CHECK(!judge("key-data-v2", "A4q C#5q", {{"tonic", "A"}, {"scale", "minor"}}));
        CHECK(judge("melody-leap-v1", "C4q E4q D4q F4h"));        // steps of 4, 2 and 3
        CHECK(!judge("melody-leap-v1", "C4q E4q D4q G4h"));       // D4 to G4 is 5
        CHECK(!judge("melody-leap-v1", "C4q C5q"));               // 12 semitones
        CHECK(judge("melody-leap-v1", "C4q Rq C5q", {{"rest_resets", "yes"}}));
        CHECK(!judge("melody-leap-v1", "C4q Rq C5q"));            // a rest is passed over
        CHECK(judge("melody-lengths-v1", "C4e D4e E4q"));
        CHECK(!judge("melody-lengths-v1", "C4e D4e E4e"));        // three eighths in a row
        CHECK(!judge("melody-lengths-v1", "C4q D4w", {{"longest", "h"}}));
        CHECK(judge("melody-rests-v1", "C4q Rq Rq D4q"));
        CHECK(!judge("melody-rests-v1", "Rq C4q"));               // no rest first
        CHECK(judge("melody-rests-v1", "Rq C4q", {{"leading", "yes"}}));
        CHECK(!judge("melody-rests-v1", "C4q Rq Rq Rq"));         // three in a row
        CHECK(judge("melody-ending-v1", "E4q D4q C5h"));
        CHECK(!judge("melody-ending-v1", "E4q D4q C5h Rq"));      // it must end on the note
        CHECK(!judge("melody-ending-v1", "E4q C4e", {{"hold", "q"}}));
        CHECK(judge("melody-range-v1", "C4q G5q", {{"low", "60"}, {"high", "79"}}));
        CHECK(!judge("melody-range-v1", "C4q A5q", {{"low", "60"}, {"high", "79"}}));
        // A stack of three plugins counts through their product, exactly: every unit at length 3.
        const FilterSpec a = plugin_spec(m["key-data-v2"]), b = plugin_spec(m["melody-leap-v1"]), c = plugin_spec(m["melody-ending-v1"]);
        const FilterStack st(audio(3), {{&a, {}}, {&b, {{"leap", "2"}}}, {&c, {}}}, none);
        CHECK(st.ranker() != nullptr);
        if (st.ranker())
        {
            uint64_t brute = 0;
            std::vector<uint32_t> u(3);
            for (u[0] = 0; u[0] < 104; ++u[0])
                for (u[1] = 0; u[1] < 104; ++u[1])
                    for (u[2] = 0; u[2] < 104; ++u[2]) brute += st.passes(u);
            CHECK(st.ranker()->count() == BigUint(brute) && brute > 0);
            bool ok = true;
            check_ranker_exhaustive(st, 104, 3, ok);
            CHECK(ok);
        }
    }
    std::cout << "filter plugins checked\n";
    // (Last: it compiles words-data-v1 with a real dictionary, and compiled automata are kept for the
    // run by their settings, which name the dictionary only as the default.)
    // not-written-v1 with a dictionary's automaton at page length: its tables are measured, not
    // bounded (the product with the readings is far smaller than either), so the stack ranks.
    // Survivors drawn by number are kept by both filters and come back to the same number, and
    // not-written takes a few away from the dictionary's own count.
    {
        auto scowl = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/scowl-2020.12.07-en-60.txt"));
        const TestResources dres(scowl, nullptr);
        const auto words = load_plugin_file(filters + "words-data-v1.sfilter");
        const FilterSpec wspec = plugin_spec(words);
        const FilterStack alone(text_line(32), {{&wspec, {}}}, dres);
        const FilterStack both(text_line(32), {{&wspec, {}}, {find_filter("not-written-v1"), {}}}, dres);
        CHECK(alone.ranker() && both.ranker());
        if (alone.ranker() && both.ranker())
        {
            CHECK(both.ranker()->count() < alone.ranker()->count());
            bool ok = true;
            for (int i = 0; i < 200; ++i)
            {
                BigUint k = BigUint(rng()) , q, r;
                k <<= 64;
                k += BigUint(rng());
                BigUint::divmod(k, both.ranker()->count(), q, r);
                const auto u = both.ranker()->unrank(r);
                ok = ok && both.passes(u) && alone.passes(u) && both.ranker()->rank(u) == r;
            }
            CHECK(ok);
        }
        // The product's own cap: refused early rather than built.
        Dfa a, b;
        a.base = b.base = 2;
        a.start = b.start = 0;
        a.accept = {1, 1, 1, 1};
        a.next = {1, 2, 3, 0, 0, 1, 2, 3}; // a 4-cycle on each symbol, so the product grows
        b.accept = {1, 1, 1};
        b.next = {1, 2, 2, 0, 0, 1};
        CHECK(throws([&] { (void)intersect(a, b, 2); }));
        CHECK(!throws([&] { (void)intersect(a, b, 1000); }));
    }
}


void test_notes2(const std::string& dir)
{
    // notes104 described as a NoteSet: the same tokens and pitches as its own functions.
    const NoteSet old = note_set_of(kNotesSymbolsId);
    CHECK(old.legacy && old.base() == kNoteSymbols && old.id() == kNotesSymbolsId && old.voices == 1);
    bool same = true;
    for (uint32_t d = 0; d < kNoteSymbols; ++d)
        same = same && note_token(old, d) == note_token(d) && old.midi(d) == (d / 4 ? 59 + d / 4 : 0) && old.sixteenths(d) == (2u << (d % 4));
    CHECK(same);
    const std::vector<uint32_t> tune{53, 69, 85, 0, 103};
    CHECK(notes_to_midi(old, tune) == notes_to_midi(tune) && notes_to_notation(old, tune) == notes_to_notation(tune));
    // Sets are checked, and have one spelling.
    auto refused = [](auto f) {
        try { f(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    CHECK(refused([] { (void)make_note_set(35, 60, "q", 1); }));        // below C2
    CHECK(refused([] { (void)make_note_set(60, 70, "q", 1); }));        // less than an octave
    CHECK(refused([] { (void)make_note_set(48, 84, "qe", 1); }));       // out of order
    CHECK(refused([] { (void)make_note_set(48, 84, "qx", 1); }));       // not a duration
    CHECK(refused([] { (void)make_note_set(48, 84, "q", 5); }));        // five voices
    CHECK(refused([] { (void)note_set_of("notes2/Db3-C6/q/V1"); }));    // flats are written as sharps
    CHECK(refused([] { (void)note_set_of("notes2/C3-C6/q"); }));
    CHECK(!is_note_symbols("lower27") && is_note_symbols("notes2/C3-C6/q/V1") && is_note_symbols("notes104"));
    const NoteSet two = make_note_set(48, 84, kNoteDurationCodes, 2);
    CHECK(refused([&] { (void)canonicalise_notes2("C4 // D4 // E4", two, 2); })); // three voices on a line of two
    CHECK(refused([&] { (void)canonicalise_notes2("C4w.", two, 2); }));           // no dotted whole
    // The MIDI file: format 1, a tempo track and one per voice.
    const std::string midi = notes_to_midi(two, canonicalise_notes2("C4q // E4h", two, 1).units.front());
    CHECK(midi.substr(0, 4) == "MThd" && midi[9] == 1 && midi[11] == 3);

    // Filters on several voices judge each voice, and count and rank as one voice's to the power
    // of the voices: every unit of a small set, against the stack.
    const NoteSet small = make_note_set(60, 71, "sq", 2); // 12 pitches and a rest, x 2 durations: 26 symbols
    const FilterLine line{"audio", small.id(), small.base(), 4, nullptr, 0, 0, 0};
    const TestResources none(nullptr, nullptr);
    const auto key = plugin_spec(load_plugin_file(dir + "../data/filters/key-data-v2.sfilter"));
    const auto leap = plugin_spec(load_plugin_file(dir + "../data/filters/melody-leap-v1.sfilter"));
    const FilterStack st(line, {{&key, {{"tonic", "D"}}}, {&leap, {{"leap", "2"}}}}, none);
    CHECK(st.voices() == 2 && st.ranker() != nullptr);
    if (st.ranker())
    {
        uint64_t brute = 0;
        std::vector<uint32_t> u(4);
        auto voice_ok = [&](uint32_t a, uint32_t b) {
            // D major, and neighbouring notes at most 2 apart (rests passed over).
            auto in = [&](uint32_t d) {
                const uint32_t m = small.midi(d);
                if (m == 0) return true;
                const uint32_t r = (m + 12 - 2) % 12;
                return r == 0 || r == 2 || r == 4 || r == 5 || r == 7 || r == 9 || r == 11;
            };
            const uint32_t ma = small.midi(a), mb = small.midi(b);
            return in(a) && in(b) && (ma == 0 || mb == 0 || (ma > mb ? ma - mb : mb - ma) <= 2);
        };
        bool agree = true;
        for (u[0] = 0; u[0] < 26; ++u[0])
            for (u[1] = 0; u[1] < 26; ++u[1])
                for (u[2] = 0; u[2] < 26; ++u[2])
                    for (u[3] = 0; u[3] < 26; ++u[3])
                    {
                        const bool want = voice_ok(u[0], u[1]) && voice_ok(u[2], u[3]);
                        brute += want;
                        agree = agree && st.passes(u) == want;
                    }
        CHECK(agree);
        CHECK(st.ranker()->count() == BigUint(brute) && brute > 0);
        bool ok = true;
        check_ranker_exhaustive(st, 26, 4, ok);
        CHECK(ok);
    }
    std::cout << "notes2 checked\n";
}

void test_filters(const std::string& dir)
{
    // Exact logarithms.
    CHECK(log2_q16(1) == 0);
    CHECK(log2_q16(2) == 65536);
    CHECK(log2_q16(3) == 103872);          // floor(1.58496... * 65536)
    CHECK(log2_q16(65536) == 16 * 65536);
    CHECK(log2_q16(uint64_t(1) << 63) == 63 * 65536);

    // The registry: unique names, and the lists each line offers.
    CHECK(find_filter("words") == find_filter("words-v2")); // the newest version
    CHECK(find_filter("words-v1") != find_filter("words-v2"));
    CHECK(find_filter("nonsense") == nullptr);
    // Which filters cannot be counted together (docs/FILTERS-CONFLICTS.md).
    auto clash = [](const char* a, const char* b) { return filter_conflict(*find_filter(a), *find_filter(b)); };
    CHECK(clash("words-v2", "clean-v2").empty());                // words implies clean
    CHECK(clash("title-v1", "words-v2").empty());                // title implies words-v2
    CHECK(clash("not-written-v1", "not-other-line-v1").empty()); // merged today
    CHECK(clash("distinct-indices-v1", "every-vertex-used-v1").empty());
    CHECK(clash("symbol-entropy-v1", "not-a-file-v1").empty());  // judges only: not in the rule
    CHECK(clash("max-run-v1", "not-written-v1").empty());        // max-run is an automaton now
    CHECK(clash("max-run-v1", "not-a-file-v1") == "conflict");
    CHECK(clash("words-v2", "not-written-v1") == "merge");
    CHECK(clash("clean-v1", "clean-v2") == "merge");
    CHECK(clash("neighbour-agreement-v1", "not-packed-v1") == "merge");
    CHECK(clash("not-a-file-v1", "words-v2") == "conflict");
    CHECK(clash("not-a-pattern-v1", "not-other-line-v1") == "conflict");
    CHECK(clash("not-a-file-v1", "distinct-vertices-v1") == "conflict");
    CHECK(filters_for(text_line(8)).size() == 14); // not-written-v1, not-a-file-v1, not-other-line-v1 and not-a-pattern-v1 joined the ten
    const FilterLine image{"image", "image/mono/10x10", 2, 100, nullptr, 10, 10, 1};
    CHECK(filters_for(image).size() == 7); // symbol-entropy, neighbour-agreement, not-a-file, not-packed, not-a-pattern,
                                           // palette-size, row-runs
    CHECK(clash("palette-size-v1", "row-runs-v1") == "merge");
    CHECK(clash("palette-size-v1", "not-a-pattern-v1") == "conflict");

    // Rankers, exhaustively at small lengths with a small dictionary: rank = position among the
    // survivors in address order, and unrank inverts it.
    auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan"}));
    TestResources small_res(small, nullptr);
    for (const char* name : {"clean-v1", "words-v1", "clean-v2", "words-v2", "window-v1", "window-v2"})
        for (uint32_t L = 1; L <= 4; ++L)
        {
            const FilterStack st(text_line(L), {{find_filter(name), {}}}, small_res);
            const Ranker* rk = st.ranker();
            CHECK(rk != nullptr);
            if (!rk) continue;
            uint64_t k = 0;
            bool ok = true;
            const uint64_t total = uint64_t(std::pow(27.0, L) + 0.5);
            for (uint64_t i = 0; i < total; ++i)
            {
                const auto u = BigUint(i).to_digits(27, L);
                if (!st.passes(u)) continue;
                ok = ok && rk->rank(u) == BigUint(k) && rk->unrank(BigUint(k)) == u;
                ++k;
            }
            CHECK(ok);
            CHECK(rk->count() == BigUint(k));
        }

    // Window rankers with words longer than the unit and shared suffixes (the edge token is then a
    // cut word), exhaustively at L = 5 (14 million units, so on a 3-letter alphabet slice only:
    // every unit over the symbols SPACE, a, n, t).
    {
        auto d = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "tan", "nat", "natant", "tartan", "at"}));
        TestResources dres(d, nullptr);
        for (const char* name : {"window-v1", "window-v2"})
            for (uint32_t L : {5u, 6u})
            {
                const FilterStack st(text_line(L), {{find_filter(name), {}}}, dres);
                const Ranker* rk = st.ranker();
                const uint32_t sym[4] = {0, 1, 14, 20}; // SPACE a n t
                bool ok = rk != nullptr;
                uint64_t total = 1;
                for (uint32_t i = 0; i < L; ++i) total *= 4;
                for (uint64_t i = 0; ok && i < total; ++i)
                {
                    std::vector<uint32_t> u(L);
                    uint64_t x = i;
                    for (uint32_t k = L; k-- > 0; x /= 4) u[k] = sym[x % 4];
                    const bool pass = st.passes(u);
                    ok = ok && rk->accepts(u) == pass;
                    if (pass) ok = ok && rk->unrank(rk->rank(u)) == u;
                }
                CHECK(ok);
            }
    }

    // title-v1: words in the first max_length symbols, then SPACEs; exhaustively at small sizes.
    for (uint32_t L = 1; L <= 4; ++L)
        for (const char* max : {"1", "2", "3", "9"})
        {
            const FilterStack st(text_line(L), {{find_filter("title-v1"), {{"max_length", max}}}}, small_res);
            bool ok = true;
            check_ranker_exhaustive(st, 27, L, ok);
            CHECK(ok);
            // Every title is a words-v2 unit (so the two can be ticked together and still rank).
            const FilterStack both(text_line(L), {{find_filter("title-v1"), {{"max_length", max}}}, {find_filter("words-v2"), {}}}, small_res);
            CHECK(both.ranker() != nullptr && both.ranker()->count() == st.ranker()->count());
        }

    // Version 2 allows trailing SPACE padding and nothing else new: a v2 survivor either passes
    // v1 or is a v1 survivor's prefix followed by two or more SPACEs; words-v2 implies clean-v2.
    for (uint32_t L = 1; L <= 4; ++L)
    {
        const FilterStack c1(text_line(L), {{find_filter("clean-v1"), {}}}, small_res);
        const FilterStack c2(text_line(L), {{find_filter("clean-v2"), {}}}, small_res);
        const FilterStack w1(text_line(L), {{find_filter("words-v1"), {}}}, small_res);
        const FilterStack w2(text_line(L), {{find_filter("words-v2"), {}}}, small_res);
        std::vector<FilterStack> hc, hw; // v1 stacks for every shorter length
        for (uint32_t n = 1; n <= L; ++n)
        {
            hc.emplace_back(text_line(n), std::vector<FilterStack::Entry>{{find_filter("clean-v1"), {}}}, small_res);
            hw.emplace_back(text_line(n), std::vector<FilterStack::Entry>{{find_filter("words-v1"), {}}}, small_res);
        }
        bool ok = true;
        const uint64_t total = uint64_t(std::pow(27.0, L) + 0.5);
        for (uint64_t i = 0; i < total; ++i)
        {
            const auto u = BigUint(i).to_digits(27, L);
            size_t n = u.size();
            while (n > 0 && u[n - 1] == 0) --n;
            const bool padded = u.size() - n >= 2;
            const std::vector<uint32_t> head(u.begin(), u.begin() + long(n));
            const bool head_c = n > 0 && hc[n - 1].passes(head);
            const bool head_w = n > 0 && hw[n - 1].passes(head);
            ok = ok && c2.passes(u) == (c1.passes(u) || (padded && head_c));
            ok = ok && w2.passes(u) == (w1.passes(u) || (padded && head_w));
            ok = ok && (!w2.passes(u) || c2.passes(u));
        }
        CHECK(ok);
    }
    // Against M1's exact counts (themselves checked by brute force and the pruned walk) with the
    // real default dictionary, and round trips at paragraph scale.
    auto scowl = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/scowl-2020.12.07-en-60.txt"));
    auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    TestResources res(scowl, model);
    for (uint32_t L : {1u, 2u, 5u, 12u, 32u})
    {
        const SieveCount c = sieve_counted(L, *scowl);
        const FilterStack w(text_line(L), {{find_filter("words-v1"), {}}}, res);
        const FilterStack cl(text_line(L), {{find_filter("clean-v1"), {}}}, res);
        CHECK(w.ranker()->count() == c.words);
        CHECK(cl.ranker()->count() == c.clean);
    }
    std::mt19937_64 rng(5);
    for (uint32_t L : {32u, 1000u})
    {
        const FilterStack w(text_line(L), {{find_filter("words-v1"), {}}}, res);
        const Ranker* rk = w.ranker();
        bool ok = true;
        for (int t = 0; t < (L == 32 ? 200 : 20); ++t)
        {
            BigUint k;
            for (size_t b = 0; b < rk->count().bit_length() + 64; b += 32)
            {
                k <<= 32;
                k.add_small(uint32_t(rng()));
            }
            k = BigUint::mod(k, rk->count());
            const auto u = rk->unrank(k);
            ok = ok && w.passes(u) && rk->rank(u) == k;
        }
        CHECK(ok);
        // Consecutive ranks are consecutive survivors: nothing in between passes.
        const auto a = rk->unrank(BigUint(1000)), b = rk->unrank(BigUint(1001));
        CHECK(BigUint::from_digits(a, 27) < BigUint::from_digits(b, 27));
    }

    // Warped text shorter than a unit is padded with SPACEs: only version 2 shelves it.
    {
        const auto u = digits27("it was the best of times        ");
        const FilterStack w1(text_line(32), {{find_filter("words-v1"), {}}}, res);
        const FilterStack w2(text_line(32), {{find_filter("words-v2"), {}}}, res);
        CHECK(!w1.passes(u));
        CHECK(w2.passes(u));
        CHECK(w2.ranker()->unrank(w2.ranker()->rank(u)) == u);
        CHECK(w1.ranker()->count() < w2.ranker()->count());
    }

    // Review fixes: words with one dictionary does not imply window with another (compact would
    // otherwise list non-survivors); an empty "word" is not a word.
    {
        struct TwoDicts : FilterResources
        {
            std::shared_ptr<const Dictionary> a = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant"}));
            std::shared_ptr<const Dictionary> b = std::make_shared<const Dictionary>(Dictionary::from_words({"i", "in"}));
            std::shared_ptr<const Dictionary> dictionary(const std::string& id) const override { return id == "b" ? b : a; }
            std::shared_ptr<const CharModel> model(const std::string&, const std::string&) const override { return nullptr; }
        } two;
        const FilterStack mixed(text_line(4), {{find_filter("words-v1"), {{"dictionary", "a"}}}, {find_filter("window-v1"), {{"dictionary", "b"}}}}, two);
        CHECK(mixed.ranker() == nullptr);
        const FilterStack same(text_line(4), {{find_filter("words-v1"), {{"dictionary", "a"}}}, {find_filter("window-v1"), {{"dictionary", "a"}}}}, two);
        CHECK(same.ranker() != nullptr);
        auto with_empty = std::make_shared<const Dictionary>(Dictionary::from_words({"", "a"}));
        TestResources er(with_empty, nullptr);
        bool ok = true;
        for (uint32_t L = 1; L <= 3; ++L)
        {
            const FilterStack st(text_line(L), {{find_filter("words-v1"), {}}}, er);
            check_ranker_exhaustive(st, 27, L, ok);
        }
        CHECK(ok);
        CHECK(throws([&] { (void)FilterStack(text_line(4), {{find_filter("words-v1"), {}}}, er).passes(digits27("abc")); }));
    }

    // Stacks: compact needs one ranking filter implying the rest.
    {
        const FilterStack s1(text_line(32), {{find_filter("clean-v1"), {}}, {find_filter("words-v1"), {}}}, res);
        CHECK(s1.ranker() != nullptr); // words implies clean
        const FilterStack s2(text_line(32), {{find_filter("words-v1"), {}}, {find_filter("max-run-v1"), {}}}, res);
        CHECK(s2.ranker() == nullptr);
        CHECK(!s2.compact_blocker().empty());
        CHECK(s1.id() != s2.id());
        const FilterStack s3(text_line(32), {{find_filter("words-v1"), {{"dictionary", ""}}}}, res);
        CHECK(s3.provenance().find(scowl->sha256()) != std::string::npos);
    }

    // The statistical filters on English and on noise.
    {
        const auto english = digits27("it was the best of times it was t");
        const auto noise = digits27("qzxvjjkwpqpqzzzzxkcdvvvvwqwqkkkk");
        const FilterStack info(text_line(33), {{find_filter("model-information-v1"), {}}}, res);
        CHECK(info.passes(english));
        const FilterStack info32(text_line(32), {{find_filter("model-information-v1"), {}}}, res);
        CHECK(!info32.passes(noise));
        const FilterStack run(text_line(32), {{find_filter("max-run-v1"), {}}}, res);
        CHECK(!run.passes(noise)); // "zzzz"
        CHECK(run.passes(digits27("aaa bbb ccc                     ")));
        const FilterStack ent(text_line(4), {{find_filter("symbol-entropy-v1"), {{"min_millibits", "1000"}, {"max_millibits", "2000"}}}}, res);
        CHECK(ent.passes(digits27("abab")));  // exactly 1 bit
        CHECK(!ent.passes(digits27("aaaa"))); // 0 bits
        CHECK(ent.passes(digits27("abcd")));  // exactly 2 bits: bounds are inclusive
        const FilterStack ent2(text_line(4), {{find_filter("symbol-entropy-v1"), {{"max_millibits", "1999"}}}}, res);
        CHECK(!ent2.passes(digits27("abcd")));
    }
    {
        const FilterStack n(image, {{find_filter("neighbour-agreement-v1"), {}}}, res);
        std::vector<uint32_t> flat(100, 1), checker(100);
        for (uint32_t i = 0; i < 100; ++i) checker[i] = ((i % 10) + (i / 10)) % 2;
        CHECK(n.passes(flat));
        CHECK(!n.passes(checker));
    }
    CHECK(throws([&] { FilterStack(text_line(8), {{find_filter("max-run-v1"), {{"max_run", "0"}}}}, res); }));
    CHECK(throws([&] { FilterStack(image, {{find_filter("words-v1"), {}}}, res); }));
}

// Filter vectors from the Python oracle (independent implementations of every filter and ranker).
// Every ranker, exhaustively: rank is the position among the survivors in address order,
// unrank inverts it, the walk accepts exactly what the filter passes, and alive() agrees with
// completions().
void check_ranker_exhaustive(const FilterStack& st, uint32_t base, uint32_t L, bool& ok)
{
    const Ranker* rk = st.ranker();
    if (!rk) { ok = false; return; }
    uint64_t total = 1;
    for (uint32_t i = 0; i < L; ++i) total *= base;
    uint64_t k = 0;
    for (uint64_t i = 0; i < total; ++i)
    {
        const auto u = BigUint(i).to_digits(base, L);
        const bool pass = st.passes(u);
        ok = ok && rk->accepts(u) == pass;
        if (!pass) continue;
        ok = ok && rk->rank(u) == BigUint(k) && rk->unrank(BigUint(k)) == u;
        ++k;
    }
    ok = ok && rk->count() == BigUint(k);
}

void test_compact(const std::string& dir)
{
    // Rankers on the other lines: black-and-white entropy (image, video) and key (audio).
    TestResources none(nullptr, nullptr);
    for (uint32_t L : {1u, 2u, 5u, 9u, 12u})
        for (auto [lo, hi] : {std::pair<int, int>{0, 500}, {0, 900}, {600, 1000}, {950, 32000}})
        {
            const FilterLine img{"image", "image/mono/" + std::to_string(L) + "x1", 2, L, nullptr, L, 1, 1};
            const FilterStack st(img, {{find_filter("symbol-entropy-v1"), {{"min_millibits", std::to_string(lo)}, {"max_millibits", std::to_string(hi)}}}}, none);
            bool ok = true;
            check_ranker_exhaustive(st, 2, L, ok);
            CHECK(ok);
        }
    // neighbour-agreement by transfer matrix: pictures and video of several shapes and palettes.
    {
        struct Shape { uint32_t base, w, h, frames; };
        for (const Shape sh : {Shape{2, 1, 1, 1}, Shape{2, 2, 2, 1}, Shape{2, 3, 2, 1}, Shape{2, 4, 3, 1}, Shape{3, 3, 3, 1},
                               Shape{2, 2, 2, 2}, Shape{3, 2, 1, 3}, Shape{2, 1, 3, 4}, Shape{4, 2, 2, 1}})
            for (int pm : {0, 500, 600, 750, 1000})
            {
                const uint32_t L = sh.w * sh.h * sh.frames;
                const FilterLine fl{sh.frames > 1 ? "video" : "image", "test", sh.base, L, nullptr, sh.w, sh.h, sh.frames};
                const FilterStack st(fl, {{find_filter("neighbour-agreement-v1"), {{"min_permille", std::to_string(pm)}}}}, none);
                bool ok = true;
                check_ranker_exhaustive(st, sh.base, L, ok);
                CHECK(ok);
            }
        // Too many colours for the width: no ranker (compact falls back to hide).
        const FilterLine wide{"image", "image/rgb24/10x10", 16777216, 100, nullptr, 10, 10, 1};
        CHECK(FilterStack(wide, {{find_filter("neighbour-agreement-v1"), {}}}, none).ranker() == nullptr);
    }
    {
        const FilterLine img{"image", "image/ega16/3x1", 16, 3, nullptr, 3, 1, 1};
        CHECK(FilterStack(img, {{find_filter("symbol-entropy-v1"), {}}}, none).ranker() == nullptr); // 16 colours: no ranker
    }
    for (uint32_t L : {1u, 2u, 3u})
        for (const char* scale : {"major", "minor-pentatonic", "blues"})
        {
            const FilterLine au{"audio", kNotesSymbolsId, kNoteSymbols, L, nullptr, 0, 0, 0};
            const FilterStack st(au, {{find_filter("key-v1"), {{"tonic", "D"}, {"scale", scale}}}}, none);
            bool ok = true;
            check_ranker_exhaustive(st, kNoteSymbols, L, ok);
            CHECK(ok);
        }
    {
        // C major: C4q passes, C#4q fails, rests pass; E minor pentatonic has 5 notes per octave.
        const FilterLine au{"audio", kNotesSymbolsId, kNoteSymbols, 4, nullptr, 0, 0, 0};
        const FilterStack cmaj(au, {{find_filter("key-v1"), {}}}, none);
        CHECK(cmaj.passes(std::vector<uint32_t>{1 * 4 + 1, 0, 3, 13 * 4 + 2}));
        CHECK(!cmaj.passes(std::vector<uint32_t>{2 * 4 + 1, 0, 0, 0}));
        const FilterStack em(au, {{find_filter("key-v1"), {{"tonic", "E"}, {"scale", "minor-pentatonic"}}}}, none);
        // Pitches C4..C6: 25 semitones; E minor pentatonic (E G A B D) has 10 of them, plus the rest: 11 pitches, 4 durations.
        CHECK(em.ranker()->count() == BigUint::pow(44, 4));
        CHECK(throws([&] { FilterStack(au, {{find_filter("key-v1"), {{"tonic", "H"}}}}, none); }));
    }

    // The survivor shuffle is a permutation of [0, N) for every N, and its inverse undoes it.
    {
        bool ok = true;
        for (uint32_t n = 1; n <= 300; ++n)
        {
            const Shuffle sh(BigUint(n), "sieve", "test");
            std::vector<bool> seen(n);
            for (uint32_t k = 0; k < n; ++k)
            {
                const BigUint j = sh.forward(BigUint(k));
                ok = ok && j < BigUint(n) && !seen[j.low_bits(32)] && sh.inverse(j) == BigUint(k);
                if (j < BigUint(n)) seen[j.low_bits(32)] = true;
            }
        }
        CHECK(ok);
        // Different keys and domains shuffle differently; a huge N round-trips.
        const Shuffle a(BigUint(1000000), "sieve", "x"), b(BigUint(1000000), "other", "x"), c(BigUint(1000000), "sieve", "y");
        CHECK(!(a.forward(BigUint(5)) == b.forward(BigUint(5))) || !(a.forward(BigUint(6)) == b.forward(BigUint(6))));
        CHECK(!(a.forward(BigUint(5)) == c.forward(BigUint(5))) || !(a.forward(BigUint(6)) == c.forward(BigUint(6))));
        BigUint big = BigUint::pow(27, 300);
        big -= BigUint(12345);
        const Shuffle h(big, "sieve", "big");
        const BigUint k = BigUint::pow(3, 500);
        CHECK(h.inverse(h.forward(k)) == k);
    }

    // The sieved guided line: exhaustive at L = 1..3 with a small model and dictionary. The arcs of
    // the survivors tile [0, 2^S) in order, every survivor's address decodes to it, points only
    // ever decode to survivors, and a non-survivor has no arc.
    const auto m = small_model();
    auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan", "it", "was"}));
    TestResources small_res(small, nullptr);
    for (const char* name : {"clean-v2", "words-v2", "words-v1"})
        for (uint32_t L = 1; L <= 3; ++L)
        {
            const FilterStack st(text_line(L), {{find_filter(name), {}}}, small_res);
            const CompactLine cl(*st.ranker(), "sieve", st.id(), m);
            const GuidedLine& g = *cl.guided();
            BigUint expected;
            bool tiled = true, decodes = true, refuses = true, orders = true;
            uint64_t k = 0;
            const uint64_t total = uint64_t(std::pow(27.0, L) + 0.5);
            for (uint64_t i = 0; i < total; ++i)
            {
                const auto u = BigUint(i).to_digits(27, L);
                if (!st.passes(u))
                {
                    refuses = refuses && throws([&] { (void)g.interval(u); }) && throws([&] { (void)cl.index_of(u, AddressMode::Positional); });
                    continue;
                }
                const auto iv = g.interval(u);
                tiled = tiled && iv.low == expected && !iv.width.is_zero();
                expected = iv.low;
                expected += iv.width;
                const auto c = g.code_of(iv);
                decodes = decodes && g.unit_at(c.point) == u && g.unit_at(iv.low) == u;
                // Compact positional is the survivor number; scrambled round-trips.
                orders = orders && cl.index_of(u, AddressMode::Positional) == BigUint(k) &&
                         cl.unit_at(cl.index_of(u, AddressMode::Scrambled), AddressMode::Scrambled) == u;
                ++k;
            }
            BigUint full(1);
            full <<= g.scale_bits();
            CHECK(tiled);
            CHECK(expected == full);
            CHECK(decodes);
            CHECK(refuses);
            CHECK(orders);
        }

    // At full scale: the pinned model and words-v2. Warped text keeps (nearly) its guided address
    // length, and noise has none.
    auto scowl = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/scowl-2020.12.07-en-60.txt"));
    auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    TestResources res(scowl, model);
    const FilterStack st(text_line(32), {{find_filter("words-v2"), {}}}, res);
    const CompactLine cl(*st.ranker(), "sieve", st.id(), model);
    const GuidedLine plain(model, 32);
    const auto u = digits27("it was the best of times        ");
    const auto sieved = cl.guided()->code(u), unsieved = plain.code(u);
    CHECK(cl.guided()->unit_at(sieved.point) == u);
    CHECK(sieved.bits <= unsieved.bits + 2);
    CHECK(throws([&] { (void)cl.guided()->code(digits27("qzx vvk                         ")); }));
    std::mt19937_64 rng(11);
    bool all_survive = true;
    for (int i = 0; i < 100; ++i)
    {
        BigUint p;
        for (int w = 0; w < 16; ++w)
        {
            p <<= 32;
            p.add_small(uint32_t(rng()));
        }
        all_survive = all_survive && st.passes(cl.guided()->unit_at(p));
    }
    CHECK(all_survive);
    // Compact scrambled round-trips at full scale.
    const BigUint j = cl.index_of(u, AddressMode::Scrambled);
    CHECK(cl.unit_at(j, AddressMode::Scrambled) == u);
    CHECK(cl.parse(cl.hex_of(j)) == j);
    CHECK(cl.hex_of(j).size() == cl.hex_width());
}

// Compact vectors from the oracle: the survivor shuffle, rankers on the other lines, and the
// sieved guided line (words and clean with scowl-en-35 and the pinned model).
void test_compact_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_compact_v1.tsv");
    CHECK(bool(in));
    std::string line;
    auto dict = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/scowl-2020.12.07-en-35.txt"));
    auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    TestResources res(dict, model);
    auto values = [](const std::string& spec) {
        FilterValues v;
        std::stringstream ss(spec);
        std::string kv;
        while (std::getline(ss, kv, ','))
            if (const size_t eq = kv.find('='); eq != std::string::npos) v[kv.substr(0, eq)] = kv.substr(eq + 1);
        return v;
    };
    auto digits = [](const std::string& list) {
        std::vector<uint32_t> d;
        std::stringstream ss(list);
        std::string x;
        while (std::getline(ss, x, ',')) d.push_back(uint32_t(std::stoul(x)));
        return d;
    };
    std::map<std::string, std::unique_ptr<FilterStack>> stacks; // sieved guided lines, by filter and length
    std::map<std::string, std::unique_ptr<CompactLine>> lines;
    auto compact = [&](const std::string& filter, uint32_t L) -> const GuidedLine& {
        const std::string k = filter + "/" + std::to_string(L);
        if (!lines.count(k))
        {
            stacks[k] = std::make_unique<FilterStack>(text_line(L), std::vector<FilterStack::Entry>{{find_filter(filter), {}}}, res);
            lines[k] = std::make_unique<CompactLine>(*stacks[k]->ranker(), "sieve", stacks[k]->id(), model);
        }
        return *lines[k]->guided();
    };
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        ++n;
        if (f[0] == "shuffle")
        {
            const Shuffle sh(BigUint::from_decimal(f[1]), f[2], f[3]);
            const BigUint k = BigUint::from_decimal(f[4]), j = BigUint::from_decimal(f[5]);
            CHECK(sh.forward(k) == j);
            CHECK(sh.inverse(j) == k);
        }
        else if (f[0] == "rank")
        {
            const uint32_t L = uint32_t(std::stoul(f[3]));
            const FilterLine fl = f[1] == "key-v1" ? FilterLine{"audio", kNotesSymbolsId, kNoteSymbols, L, nullptr, 0, 0, 0}
                                                   : FilterLine{"image", "image/mono/" + std::to_string(L) + "x1", 2, L, nullptr, L, 1, 1};
            const FilterStack st(fl, {{find_filter(f[1]), values(f[2])}}, res);
            const Ranker* rk = st.ranker();
            CHECK(rk && rk->count() == BigUint::from_decimal(f[4]));
            const auto u = digits(f[6]);
            CHECK(rk && rk->unrank(BigUint::from_decimal(f[5])) == u);
            CHECK(rk && rk->rank(u) == BigUint::from_decimal(f[5]));
            CHECK(st.passes(u));
        }
        else if (f[0] == "nrank")
        {
            const uint32_t w = uint32_t(std::stoul(f[1])), h = uint32_t(std::stoul(f[2])), fr = uint32_t(std::stoul(f[3])),
                           B = uint32_t(std::stoul(f[4]));
            const FilterLine fl{fr > 1 ? "video" : "image", "test", B, w * h * fr, nullptr, w, h, fr};
            const FilterStack st(fl, {{find_filter("neighbour-agreement-v1"), {{"min_permille", f[5]}}}}, res);
            const Ranker* rk = st.ranker();
            const auto u = digits(f[8]);
            CHECK(rk && rk->count() == BigUint::from_decimal(f[6]));
            CHECK(rk && rk->unrank(BigUint::from_decimal(f[7])) == u);
            CHECK(rk && rk->rank(u) == BigUint::from_decimal(f[7]));
            CHECK(st.passes(u));
        }
        else if (f[0] == "sguided")
        {
            const GuidedLine& g = compact(f[1], uint32_t(std::stoul(f[2])));
            const auto c = g.code(digits27(f[3]));
            CHECK(std::to_string(c.bits) == f[4]);
            CHECK(c.hex == f[5]);
            CHECK(g.unit_at(g.point_of(f[5])) == digits27(f[3]));
        }
        else if (f[0] == "spoint")
        {
            const GuidedLine& g = compact(f[1], uint32_t(std::stoul(f[2])));
            CHECK(g.unit_at(g.point_of(f[3])) == digits27(f[4]));
        }
        else CHECK(false);
    }
    std::cout << "compact vectors checked: " << n << "\n";
    CHECK(n == 246); // a truncated or emptied vector file must fail, not pass quietly
}

// The books line: every book of a tiny shape, in both orderings, is a bijection with [0, N).
void test_bookspace()
{
    const Space cover("image/mono/2x1", 2, 2, "sieve");
    const Space page(alphabet_by_id("lower27"), 1, "sieve");
    for (uint32_t pages : {0u, 1u, 2u})
    {
        const BookSpace bs(cover, page, pages);
        uint64_t n = 4;
        for (uint32_t i = 0; i <= pages; ++i) n *= 27;
        CHECK(bs.size() == BigUint(n));
        bool ok = true;
        std::vector<bool> seen(n);
        for (uint64_t k = 0; k < n; ++k)
        {
            const auto pos = bs.parts_at(BigUint(k), AddressMode::Positional);
            ok = ok && bs.index_of(pos, AddressMode::Positional) == BigUint(k);
            const auto scr = bs.parts_at(BigUint(k), AddressMode::Scrambled);
            const BigUint back = bs.index_of(scr, AddressMode::Scrambled);
            ok = ok && back == BigUint(k);
            const uint32_t j = bs.index_of(scr, AddressMode::Positional).low_bits(32);
            ok = ok && !seen[j];
            seen[j] = true;
        }
        CHECK(ok);
    }
    // Positional order: the cover is most significant, the last page least.
    const BookSpace bs(cover, page, 2);
    BookSpace::Parts p{{1, 0}, {3}, {{0}, {5}}};
    CHECK(bs.index_of(p, AddressMode::Positional) == BigUint(((2 * 27 + 3) * 27 + 0) * 27 + 5));
    CHECK(bs.hex_of(BigUint(5)).size() == bs.hex_width());
    CHECK(throws([&] { (void)bs.parse("fffff"); })); // five digits, as wide as the line's addresses, but past its end
    // At full scale: a 10x10 cover, 32-character pages, four of them; scrambled round-trips.
    const Space big_cover("image/mono/10x10", 2, 100, "sieve");
    const Space big_page(alphabet_by_id("lower27"), 32, "sieve");
    const BookSpace big(big_cover, big_page, 4);
    BookSpace::Parts q;
    q.cover.assign(100, 1);
    q.title = digits27("a tale of two cities            ");
    for (const char* t : {"it was the best of times it was ", "the worst of times it was the ag", "e of wisdom it was the age of fo", "olishness                       "})
        q.pages.push_back(digits27(t));
    const BigUint a = big.index_of(q, AddressMode::Scrambled);
    CHECK(big.parts_at(a, AddressMode::Scrambled) == q);
    CHECK(big.parse(big.hex_of(a)) == a);
}

void test_booksieve()
{
    // Tiny shapes, checked against brute force: a 2x2 two-colour cover, pages of L = 2 letters,
    // one or two of them. Count = the product of each part's brute-force survivors; unrank walks
    // the survivors in increasing positional address, and rank inverts it.
    auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan"}));
    TestResources res(small, nullptr);
    const Space cover("image/mono/2x2", 2, 4, "sieve");
    const Space page(alphabet_by_id("lower27"), 2, "sieve");
    const FilterLine image{"image", "image/mono/2x2", 2, 4, nullptr, 2, 2, 1};
    auto brute = [](const FilterStack& st, uint32_t base, uint32_t n) {
        uint64_t total = 1, c = 0;
        for (uint32_t i = 0; i < n; ++i) total *= base;
        for (uint64_t v = 0; v < total; ++v)
        {
            std::vector<uint32_t> u(n);
            uint64_t x = v;
            for (uint32_t i = n; i-- > 0; x /= base) u[i] = uint32_t(x % base);
            if (st.passes(u)) ++c;
        }
        return c;
    };
    for (uint32_t P : {1u, 2u})
    {
        const BookSpace bs(cover, page, P);
        const FilterStack none;
        const FilterStack cv(image, {{find_filter("neighbour-agreement-v1"), {{"min_permille", "1000"}}}}, res);
        const FilterStack ti(text_line(2), {{find_filter("title-v1"), {}}}, res);
        const FilterStack pg(text_line(2 * P), {{find_filter("words-v2"), {}}}, res);
        for (int mask = 0; mask < 8; ++mask)
        {
            const FilterStack& c = mask & 1 ? cv : none;
            const FilterStack& t = mask & 2 ? ti : none;
            const FilterStack& b = mask & 4 ? pg : none;
            const BookSieve s(bs, c, t, b);
            CHECK(s.empty() == (mask == 0));
            CHECK(s.can_rank());
            const uint64_t n = (c.empty() ? 16 : brute(c, 2, 4)) * (t.empty() ? 729 : brute(t, 27, 2)) *
                               (b.empty() ? (P == 1 ? 729 : 531441) : brute(b, 27, 2 * P));
            CHECK(s.count() == BigUint(n));
            if (n > 20000) continue;
            bool ok = true;
            BigUint prev;
            for (uint64_t k = 0; k < n; ++k)
            {
                const auto parts = s.unrank(BigUint(k));
                ok = ok && s.first_failure(parts).empty() && s.rank(parts) == BigUint(k);
                const BigUint at = bs.index_of(parts, AddressMode::Positional);
                ok = ok && (k == 0 || prev < at);
                prev = at;
            }
            CHECK(ok);
        }
    }
    // A word cut by the page break is judged whole: "an|t " passes words-v2 on the body.
    const BookSpace bs(cover, page, 2);
    const FilterStack none;
    const FilterStack pg(text_line(4), {{find_filter("words-v2"), {}}}, res);
    const BookSieve s(bs, none, none, pg);
    BookSpace::Parts p{{0, 0, 0, 0}, digits27("zz"), {digits27("an"), digits27("t ")}};
    CHECK(s.first_failure(p).empty());
    p.pages[1] = digits27("q ");
    CHECK(s.first_failure(p) == "pages: words-v2");
    CHECK(throws([&] { (void)s.rank(p); }));
    CHECK(throws([&] { (void)s.unrank(s.count()); }));
    CHECK(throws([&] { (void)s.first_failure(BookSpace::Parts{{0, 0, 0}, digits27("zz"), {digits27("an"), digits27("t ")}}); }));
    // Compact addresses: a bijection on [0, N) in both orderings, with a fixed domain.
    const FilterStack ti(text_line(2), {{find_filter("title-v1"), {}}}, res);
    const BookSieve c(bs, none, ti, pg);
    CHECK(c.domain().rfind("books-compact-v1/books/", 0) == 0);
    const uint64_t n = c.count().low_bits(32);
    CHECK(c.count() == BigUint(n) && n > 0 && n < 200000);
    std::vector<bool> seen(n);
    bool ok = true;
    for (uint64_t k = 0; k < n; ++k)
    {
        const auto q = c.parts_at(BigUint(k), AddressMode::Scrambled);
        ok = ok && c.index_of(q, AddressMode::Scrambled) == BigUint(k);
        const uint64_t j = c.index_of(q, AddressMode::Positional).low_bits(32);
        ok = ok && j < n && !seen[j];
        if (j < n) seen[j] = true;
        ok = ok && c.parts_at(BigUint(k), AddressMode::Positional) == c.unrank(BigUint(k));
    }
    CHECK(ok);
    CHECK(c.parse(c.hex_of(BigUint(n - 1))) == BigUint(n - 1));
    CHECK(throws([&] { (void)c.parse(BigUint(n).to_hex()); }));
}

// Checks added in the 0.12 review: the fast rank/unrank paths at their limits, compact books with
// a part that cannot rank, and the implications title-v1 and words-v2 rely on.
void test_review_additions()
{
    TestResources none(nullptr, nullptr);
    // Black-and-white entropy at 2048 symbols, with time to spare (the longest it ranks at follows
    // the time budget): the fast walk agrees with the generic one, both ways.
    {
        set_unit_time_ms(1e9);
        const FilterLine img{"image", "image/mono/2048x1", 2, 2048, nullptr, 2048, 1, 1};
        const FilterStack st(img, {{find_filter("symbol-entropy-v1"), {{"max_millibits", "500"}}}}, none);
        const Ranker* rk = st.ranker();
        CHECK(rk != nullptr);
        if (rk)
        {
            bool ok = true;
            BigUint k;
            for (int i = 0; i < 6; ++i)
            {
                // 0, N-1, and a spread of survivor numbers in between (from SHA-256, mod N).
                if (i == 0) k = BigUint();
                else if (i == 1) { k = rk->count(); k -= BigUint(1); }
                else k = BigUint::mod(BigUint::from_hex(Sha256::hex(Sha256::hash("entropy/" + std::to_string(i)))), rk->count());
                const auto u = rk->unrank(k);
                ok = ok && u == rk->Ranker::unrank(k) && rk->rank(u) == k && rk->Ranker::rank(u) == k && st.passes(u);
            }
            CHECK(ok);
            std::vector<uint32_t> ones(2048, 1); // all one colour at the other end: entropy 0, survives
            CHECK(!throws([&] { (void)rk->rank(ones); }));
            std::vector<uint32_t> half(2048, 0);
            for (size_t i = 0; i < half.size(); i += 2) half[i] = 1; // one bit per symbol: fails max 0.5
            CHECK(throws([&] { (void)rk->rank(half); }));
        }
        // With a millisecond allowed, a unit that long takes far longer to rank than that: none offered.
        set_unit_time_ms(1);
        const FilterLine img2{"image", "image/mono/2049x1", 2, 2049, nullptr, 2049, 1, 1};
        CHECK(FilterStack(img2, {{find_filter("symbol-entropy-v1"), {}}}, none).ranker() == nullptr);
        // ...and a short one still ranks (the limit follows the budget, by the cube of the length).
        const FilterLine img3{"image", "image/mono/8x1", 2, 8, nullptr, 8, 1, 1};
        CHECK(FilterStack(img3, {{find_filter("symbol-entropy-v1"), {}}}, none).ranker() != nullptr);
        set_unit_time_ms(kDefaultUnitTimeMs);
    }
    // clean-v2 at paragraph length: round trips through the slimmed tables.
    {
        const FilterStack st(text_line(1000), {{find_filter("clean-v2"), {}}}, none);
        const Ranker* rk = st.ranker();
        CHECK(rk != nullptr);
        bool ok = rk != nullptr;
        for (int i = 0; ok && i < 5; ++i)
        {
            BigUint k = i == 0 ? BigUint() : BigUint::mod(BigUint::from_hex(Sha256::hex(Sha256::hash("clean/" + std::to_string(i)))), rk->count());
            if (i == 4) { k = rk->count(); k -= BigUint(1); }
            const auto u = rk->unrank(k);
            ok = ok && rk->rank(u) == k && st.passes(u) && rk->accepts(u);
        }
        CHECK(ok);
    }
    // Implications: title-v1 and words-v2 each imply window-v2, so ticking both still compacts,
    // with exactly the stronger filter's survivors.
    {
        auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan"}));
        TestResources res(small, nullptr);
        for (const char* strong : {"title-v1", "words-v2"})
        {
            const FilterStack one(text_line(6), {{find_filter(strong), {}}}, res);
            const FilterStack both(text_line(6), {{find_filter(strong), {}}, {find_filter("window-v2"), {}}}, res);
            CHECK(one.ranker() != nullptr && both.ranker() != nullptr);
            if (one.ranker() && both.ranker()) CHECK(one.ranker()->count() == both.ranker()->count());
        }
        // A book whose pages stack cannot rank: no compact books, and it says why.
        const Space cover("image/mono/2x2", 2, 4, "sieve");
        const Space page(alphabet_by_id("lower27"), 2, "sieve");
        const BookSpace bs(cover, page, 2);
        const FilterStack none_stack;
        const FilterStack pages(text_line(4), {{find_filter("max-run-v1"), {}}, {find_filter("symbol-entropy-v1"), {}}}, res); // entropy judges only on 27 symbols
        const BookSieve s(bs, none_stack, none_stack, pages);
        CHECK(!s.can_rank());
        CHECK(s.blocker().rfind("pages: ", 0) == 0);
        CHECK(throws([&] { (void)s.unrank(BigUint()); }));
        CHECK(throws([&] { (void)s.parts_at(BigUint(), AddressMode::Positional); }));
        // Filtering still works without ranking.
        BookSpace::Parts p{{0, 0, 0, 0}, digits27("an"), {digits27("aa"), digits27("aa")}};
        CHECK(s.first_failure(p) == "pages: max-run-v1");
        // A title of length 0 has no survivors (it needs a letter).
        const FilterStack t0(text_line(0), {{find_filter("title-v1"), {}}}, res);
        CHECK(t0.ranker() == nullptr || t0.ranker()->count().is_zero());
    }
}

void test_book_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_books_v1.tsv");
    CHECK(bool(in));
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        while (f.size() < 13) f.push_back("");
        const Space cover(f[1], uint32_t(std::stoul(f[2])), uint32_t(std::stoul(f[3])), f[7]);
        const Space page(alphabet_by_id(f[4]), uint32_t(std::stoul(f[5])), f[7]);
        const BookSpace bs(cover, page, uint32_t(std::stoul(f[6])));
        const AddressMode m = address_mode_from_string(f[8]);
        BookSpace::Parts p;
        for (char c : f[10]) p.cover.push_back(uint32_t(c - '0'));
        p.title = digits27(f[11]);
        std::stringstream ps(f[12]);
        std::string pg;
        while (std::getline(ps, pg, '|')) p.pages.push_back(digits27(pg));
        const BigUint k = BigUint::from_hex(f[9]);
        CHECK(bs.parts_at(k, m) == p);
        CHECK(bs.hex_of(bs.index_of(p, m)) == f[9]);
        ++n;
    }
    std::cout << "book vectors checked: " << n << "\n";
    CHECK(n == 60); // a truncated or emptied vector file must fail, not pass quietly
}

// composition-v1 (tracks, movies): the oracle's vectors, both orderings, with and without a title.
void test_composition_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_compositions_v1.tsv");
    CHECK(bool(in));
    auto digits = [](const std::string& t) {
        std::vector<uint32_t> d;
        std::stringstream ss(t);
        std::string x;
        while (std::getline(ss, x, '.'))
            if (!x.empty()) d.push_back(uint32_t(std::stoul(x)));
        return d;
    };
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        while (f.size() < 18) f.push_back("");
        auto u32 = [&](size_t i) { return uint32_t(std::stoul(f[i])); };
        const std::string& key = f[12];
        std::optional<Space> title;
        if (f[5] != "-") title = Space(f[5], u32(6), u32(7), key);
        const CompositionSpace cs(f[1], Space(f[2], u32(3), u32(4), key), title, Space(f[8], u32(9), u32(10), key), u32(11));
        const AddressMode m = address_mode_from_string(f[13]);
        CompositionSpace::Parts p;
        p.cover = digits(f[15]);
        p.title = digits(f[16]);
        std::stringstream us(f[17]);
        std::string u;
        while (std::getline(us, u, '|')) p.units.push_back(digits(u));
        const BigUint k = BigUint::from_hex(f[14]);
        CHECK(cs.parts_at(k, m) == p);
        CHECK(cs.hex_of(cs.index_of(p, m)) == f[14]);
        CHECK(cs.hex_of(k) == f[14]); // the width is the space's
        ++n;
    }
    std::cout << "composition vectors checked: " << n << "\n";
    CHECK(n == 48); // a truncated or emptied vector file must fail, not pass quietly
}

// Worlds (worldspace-v1): the oracle's addresses, each slot's model and place, the turns table and
// the world's .obj text, against reference/sieve_ref.py world-vectors.
void test_world_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_worlds_v1.tsv");
    CHECK(bool(in));
    auto digits = [](const std::string& t) {
        std::vector<uint32_t> d;
        std::stringstream ss(t);
        std::string x;
        while (std::getline(ss, x, '.'))
            if (!x.empty()) d.push_back(uint32_t(std::stoul(x)));
        return d;
    };
    std::string line;
    int n = 0;
    bool turns_seen = false;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        if (f[0] == "turns")
        {
            std::string want;
            for (const Turn& t : turns())
            {
                if (!want.empty()) want += ' ';
                for (int i = 0; i < 3; ++i) want += char('0' + t.perm[size_t(i)]);
                for (int i = 0; i < 3; ++i) want += t.sign[size_t(i)] > 0 ? '+' : '-';
            }
            CHECK(f[1] == want);
            turns_seen = true;
            continue;
        }
        while (f.size() < 20) f.push_back("");
        auto u32 = [&](size_t i) { return uint32_t(std::stoul(f[i])); };
        const std::string& key = f[12];
        std::optional<Space> title;
        if (f[4] != "-") title = Space(f[4], u32(5), u32(6), key);
        const ModelSpace models(u32(7), u32(8), u32(9), key);
        const WorldSpace ws(Space(f[1], u32(2), u32(3), key), title, models, u32(10), u32(11));
        const AddressMode m = address_mode_from_string(f[13]);
        WorldSpace::Parts p;
        p.cover = digits(f[15]);
        p.title = digits(f[16]);
        std::stringstream us(f[17]);
        std::string u;
        while (std::getline(us, u, '|'))
        {
            const size_t colon = u.find(':');
            const std::vector<uint32_t> place = digits(u.substr(colon + 1));
            p.slots.push_back({BigUint::from_hex(u.substr(0, colon)), place[0], place[1], place[2], place[3]});
        }
        const BigUint k = BigUint::from_hex(f[14]);
        CHECK(ws.parts_at(k, m) == p);
        CHECK(ws.hex_of(ws.index_of(p, m)) == f[14]);
        CHECK(ws.hex_of(k) == f[14]); // the width is the space's
        const std::string obj = ws.to_obj(p);
        CHECK(Sha256::hex(Sha256::hash(obj)) == f[18]);
        // The mesh is the .obj's: as many vertices and faces, and each slot's vertices inside its cell.
        const WorldSpace::Mesh mesh = ws.mesh_of(p);
        CHECK(mesh.vertices.size() == size_t(models.vertices()) * ws.slots() && mesh.faces.size() == size_t(models.face_count()) * ws.slots());
        for (size_t s = 0; s < p.slots.size(); ++s)
        {
            const float cx = float(2 * int(p.slots[s].x) + 1 - int(ws.grid()));
            for (uint32_t i = 0; i < models.vertices(); ++i)
                CHECK(std::abs(mesh.vertices[s * models.vertices() + i].x - cx) < 1.0f);
        }
        ++n;
    }
    std::cout << "world vectors checked: " << n << "\n";
    CHECK(turns_seen);
    CHECK(n == 48); // a truncated or emptied vector file must fail, not pass quietly
}

// The world filters against brute force: a 1x1 two-colour cover, no title, one slot of the smallest
// models line (3 vertices, 1 face, 2 coordinates) on a grid of 1, under canonical-mesh-v1 and with
// none. Survivors walk in positional order, rank inverts unrank, and compact addresses round-trip.
void test_world_sieve()
{
    const Space cover("image/mono/1x1", 2, 1, "sieve");
    const ModelSpace models(3, 1, 2, "sieve");
    const WorldSpace ws(cover, std::nullopt, models, 1, 1);
    const FilterStack none;
    const ModelSieve canon(models, {{find_filter("canonical-mesh-v1"), {}}});
    const ModelSieve plain(models, {});
    for (const ModelSieve* ms : {&canon, &plain})
    {
        const WorldSieve sv(ws, none, none, *ms);
        CHECK(sv.can_rank());
        uint64_t k = 0;
        const uint64_t total = std::stoull(ws.size().to_decimal()); // 2 x 13824 x 24
        for (uint64_t v = 0; v < total; v += 101) // every 101st (prime to 24 and to the models): every place, many models
        {
            const WorldSpace::Parts p = ws.parts_at(BigUint(v), AddressMode::Positional);
            if (!sv.first_failure(p).empty())
            {
                CHECK(ms == &canon && sv.first_failure(p).rfind("model 1: ", 0) == 0);
                continue;
            }
            const BigUint r = sv.rank(p);
            CHECK(sv.unrank(r) == p);
            for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled}) CHECK(sv.parts_at(sv.index_of(p, m), m) == p);
            ++k;
        }
        CHECK(k > 0);
        const BigUint want = BigUint::mul(BigUint::mul(BigUint(2), ms == &canon ? canon.count() : models.size()), BigUint(24));
        CHECK(sv.count() == want);
        // Survivors in positional order: rank k and k + 1 are increasing worlds.
        const BigUint a = ws.index_of(sv.unrank(BigUint(5)), AddressMode::Positional), b = ws.index_of(sv.unrank(BigUint(6)), AddressMode::Positional);
        CHECK(a < b);
    }
    CHECK(WorldSieve(ws, none, none, plain).domain() == "world-compact-v1/" + ws.id() + "/-/-/-");
}

// The composition filters against brute force: a 2x2 two-colour cover and units of 2x1 two-colour
// video of 2 frames, two units a movie, no title, neighbour-agreement-v1 on cover and units. The
// count is the cover's survivors times the unit's squared; unrank walks the survivors in
// increasing positional address, rank inverts it, and the compact addresses round-trip.
void test_composition_sieve()
{
    TestResources res(nullptr, nullptr);
    const Space cover("image/mono/2x2", 2, 4, "sieve"), unit("video/mono/2x1x2", 2, 4, "sieve");
    const CompositionSpace cs("movies", cover, std::nullopt, unit, 2);
    const FilterLine image{"image", "image/mono/2x2", 2, 4, nullptr, 2, 2, 1};
    const FilterLine video{"video", "video/mono/2x1x2", 2, 4, nullptr, 2, 1, 2};
    const FilterStack none;
    const FilterStack cst(image, {{find_filter("neighbour-agreement-v1"), {}}}, res);
    const FilterStack ust(video, {{find_filter("neighbour-agreement-v1"), {}}}, res);
    for (int variant = 0; variant < 3; ++variant)
    {
        const FilterStack& c = variant == 1 ? none : cst;
        const FilterStack& u = variant == 2 ? none : ust;
        const CompositionSieve sv(cs, c, none, u);
        CHECK(sv.can_rank());
        uint64_t k = 0;
        const uint64_t total = std::stoull(cs.size().to_decimal()); // 16 x 16^2
        for (uint64_t v = 0; v < total; ++v)
        {
            const CompositionSpace::Parts p = cs.parts_at(BigUint(v), AddressMode::Positional);
            if (!sv.first_failure(p).empty()) continue;
            CHECK(sv.rank(p) == BigUint(k));
            CHECK(sv.unrank(BigUint(k)) == p);
            for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled}) CHECK(sv.parts_at(sv.index_of(p, m), m) == p);
            ++k;
        }
        CHECK(sv.count() == BigUint(k));
        if (variant == 0) CHECK(sv.domain().rfind("composition-compact-v1/movies/image/mono/2x2/L4+-+video/mono/2x1x2/L4x2/key=sieve/composition-v1/", 0) == 0);
    }
    // The joined stack: the same agreement judged across the two units joined (4 frames), the
    // per-unit stack empty. One strand: the survivors keep the positional order.
    const FilterLine video4{"video", "video/mono/2x1x4", 2, 8, nullptr, 2, 1, 4};
    const FilterStack jst(video4, {{find_filter("neighbour-agreement-v1"), {}}}, res);
    for (uint32_t strands : {1u, 2u})
    {
        const CompositionSieve sv(cs, cst, none, none, &jst, strands);
        CHECK(sv.has_joined() && !sv.empty() && sv.can_rank());
        uint64_t k = 0;
        const uint64_t total = std::stoull(cs.size().to_decimal());
        std::set<std::string> seen;
        for (uint64_t v = 0; v < total; ++v)
        {
            const CompositionSpace::Parts p = cs.parts_at(BigUint(v), AddressMode::Positional);
            const std::string f = sv.first_failure(p);
            const bool joined_ok = jst.first_failure(join_units(p.units, strands)) < 0, cover_ok = cst.first_failure(p.cover) < 0;
            CHECK(f.empty() == (joined_ok && cover_ok));
            if (cover_ok && !joined_ok) CHECK(f.rfind("joined: ", 0) == 0);
            if (!f.empty()) continue;
            const BigUint r = sv.rank(p);
            if (strands == 1) CHECK(r == BigUint(k)); // one strand: positional order
            CHECK(sv.unrank(r) == p);
            CHECK(seen.insert(r.to_decimal()).second);
            for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled}) CHECK(sv.parts_at(sv.index_of(p, m), m) == p);
            ++k;
        }
        CHECK(sv.count() == BigUint(k));
        CHECK(sv.domain().size() > jst.id().size() && sv.domain().substr(sv.domain().size() - jst.id().size() - 1) == "/" + jst.id());
    }
    {
        // Both unit stacks at once: judged, but not counted.
        const CompositionSieve sv(cs, none, none, ust, &jst, 1);
        CHECK(!sv.can_rank() && sv.blocker().rfind("units and joined", 0) == 0);
        const CompositionSpace::Parts p = cs.parts_at(BigUint(0), AddressMode::Positional);
        CHECK(sv.first_failure(p).empty() == (ust.first_failure(p.units[0]) < 0 && ust.first_failure(p.units[1]) < 0 &&
                                              jst.first_failure(join_units(p.units, 1)) < 0));
        // An empty joined stack leaves the domain as it was.
        const CompositionSieve plain(cs, none, none, ust), with_empty(cs, none, none, ust, &none, 1);
        CHECK(plain.domain() == with_empty.domain() && !with_empty.has_joined());
    }
    {
        // Five units, ranked by halves (2 + 3, then 1 + 2): every survivor's rank is its place in
        // positional order, and unrank inverts it, as one unit at a time would give.
        const CompositionSpace five("movies", cover, std::nullopt, unit, 5);
        const CompositionSieve sv(five, none, none, ust);
        const BigUint per = ust.ranker()->count();
        CHECK(sv.count() == BigUint::mul(BigUint(16), BigUint::pow(per, 5)));
        for (uint64_t step : {1ull, 7ull, 1234567ull})
            for (uint64_t k = 0; BigUint(k) < sv.count() && k < 200 * step; k += step)
            {
                const CompositionSpace::Parts p = sv.unrank(BigUint(k));
                // The units' ranks, read one unit at a time, cover first: what the halves must make.
                BigUint want = BigUint::from_digits(p.cover, 2);
                for (const auto& u : p.units)
                {
                    want = BigUint::mul(want, per);
                    want += ust.ranker()->rank(u);
                }
                CHECK(want == BigUint(k));
                CHECK(sv.rank(p) == BigUint(k));
            }
    }
    // Joining: two units of two voices (a1 a2 | b1 b2, each voice two events) join voice by voice.
    const std::vector<std::vector<uint32_t>> two = {{1, 2, 3, 4}, {5, 6, 7, 8}};
    CHECK((join_units(two, 2) == std::vector<uint32_t>{1, 2, 5, 6, 3, 4, 7, 8}));
    CHECK((join_units(two, 1) == std::vector<uint32_t>{1, 2, 3, 4, 5, 6, 7, 8}));
    const std::vector<uint32_t> j = join_units(two, 2);
    CHECK(split_units(j, 2, 2) == two);
    std::cout << "composition filters checked against brute force\n";
}

void test_book_filter_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_book_filters_v1.tsv");
    CHECK(bool(in));
    auto dict = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/scowl-2020.12.07-en-35.txt"));
    TestResources res(dict, nullptr);
    struct Case
    {
        std::unique_ptr<BookSpace> space;
        FilterStack cover, title, pages;
        std::unique_ptr<BookSieve> sieve;
        uint32_t L = 0, P = 0;
    };
    std::map<std::string, Case> cases;
    std::string line;
    int n = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        if (f[0] == "bookdomain")
        {
            Case& c = cases[f[1]];
            const uint32_t W = uint32_t(std::stoul(f[2])), H = uint32_t(std::stoul(f[3]));
            c.L = uint32_t(std::stoul(f[4]));
            c.P = uint32_t(std::stoul(f[5]));
            const std::string sym = "image/mono/" + f[2] + "x" + f[3];
            c.space = std::make_unique<BookSpace>(Space(sym, 2, W * H, "sieve"), Space(alphabet_by_id("lower27"), c.L, "sieve"), c.P);
            const FilterLine image{"image", sym, 2, W * H, nullptr, W, H, 1};
            if (f[6] != "-") c.cover = FilterStack(image, {{find_filter(f[6]), {}}}, res);
            if (f[7] != "-") c.title = FilterStack(text_line(c.L), {{find_filter("title-v1"), {{"dictionary", "scowl-en-35"}, {"max_length", f[7]}}}}, res);
            if (f[8] != "-") c.pages = FilterStack(text_line(c.L * c.P), {{find_filter(f[8]), {{"dictionary", "scowl-en-35"}}}}, res);
            c.sieve = std::make_unique<BookSieve>(*c.space, c.cover, c.title, c.pages);
            CHECK(c.sieve->count().to_decimal() == f[9]);
            CHECK(c.sieve->domain() == f[10]);
            continue;
        }
        const Case& c = cases.at(f[1]);
        const AddressMode m = address_mode_from_string(f[2]);
        BookSpace::Parts p;
        for (char ch : f[4]) p.cover.push_back(uint32_t(ch - '0'));
        p.title = digits27(f[5]);
        std::stringstream ps(f[6]);
        std::string pg;
        while (std::getline(ps, pg, '|')) p.pages.push_back(digits27(pg));
        const BigUint a = BigUint::from_hex(f[3]);
        CHECK(c.sieve->parts_at(a, m) == p);
        CHECK(c.sieve->hex_of(c.sieve->index_of(p, m)) == f[3]);
        ++n;
    }
    std::cout << "book filter vectors checked: " << n << "\n";
    CHECK(n == 48); // a truncated or emptied vector file must fail, not pass quietly
}

void test_filter_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_filters_v1.tsv");
    CHECK(bool(in));
    std::string line, dict_file;
    std::vector<std::vector<std::string>> rows;
    while (std::getline(in, line))
    {
        if (line.rfind("# dictionary ", 0) == 0) dict_file = line.substr(13);
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        rows.push_back(f);
    }
    auto dict = std::make_shared<const Dictionary>(Dictionary::load_file(dir + "../data/dictionaries/" + dict_file));
    auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    TestResources res(dict, model);
    auto values = [](const std::string& spec) {
        FilterValues v;
        std::stringstream ss(spec);
        std::string kv;
        while (std::getline(ss, kv, ','))
            if (const size_t eq = kv.find('='); eq != std::string::npos && kv.substr(0, eq) != "dictionary") v[kv.substr(0, eq)] = kv.substr(eq + 1);
        return v;
    };
    int n = 0;
    for (const auto& f : rows)
    {
        ++n;
        if (f[0] == "log2") CHECK(std::to_string(log2_q16(std::stoull(f[1]))) == f[2]);
        else if (f[0] == "verdict")
        {
            const FilterStack st(text_line(uint32_t(std::stoul(f[3]))), {{find_filter(f[1]), values(f[2])}}, res);
            CHECK(st.passes(digits27(f[4])) == (f[5] == "1"));
        }
        else if (f[0] == "image")
        {
            const uint32_t w = uint32_t(std::stoul(f[1])), h = uint32_t(std::stoul(f[2])), fr = uint32_t(std::stoul(f[3]));
            const FilterLine l{fr > 1 ? "video" : "image", "px", 2, w * h * fr, nullptr, w, h, fr};
            const FilterStack st(l, {{find_filter("neighbour-agreement-v1"), {{"min_permille", f[4]}}}}, res);
            std::vector<uint32_t> px;
            for (char c : f[5]) px.push_back(uint32_t(c - '0'));
            CHECK(st.passes(px) == (f[6] == "1"));
        }
        else if (f[0] == "trank")
        {
            const FilterStack st(text_line(uint32_t(std::stoul(f[2]))), {{find_filter("title-v1"), {{"max_length", f[1]}}}}, res);
            const Ranker* rk = st.ranker();
            CHECK(rk && rk->count() == BigUint::from_decimal(f[3]));
            CHECK(rk && rk->unrank(BigUint::from_decimal(f[4])) == digits27(f[5]));
            CHECK(rk && rk->rank(digits27(f[5])) == BigUint::from_decimal(f[4]));
        }
        else if (f[0] == "rank")
        {
            const FilterStack st(text_line(uint32_t(std::stoul(f[2]))), {{find_filter(f[1]), {}}}, res);
            const Ranker* rk = st.ranker();
            CHECK(rk && rk->count() == BigUint::from_decimal(f[3]));
            CHECK(rk && rk->unrank(BigUint::from_decimal(f[4])) == digits27(f[5]));
            CHECK(rk && rk->rank(digits27(f[5])) == BigUint::from_decimal(f[4]));
        }
        else { CHECK(false); }
    }
    std::cout << "filter vectors checked: " << n << "\n";
    CHECK(n == 997); // a truncated or emptied vector file must fail, not pass quietly
}


// ---------------------------------------------------------------- file kinds and not-written

std::vector<std::vector<std::string>> read_rows(const std::string& path)
{
    std::vector<std::vector<std::string>> rows;
    std::ifstream in(path);
    CHECK(static_cast<bool>(in));
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        size_t start = 0;
        for (size_t i = 0; i <= line.size(); ++i)
            if (i == line.size() || line[i] == '\t') { f.push_back(line.substr(start, i - start)); start = i + 1; }
        rows.push_back(std::move(f));
    }
    return rows;
}

std::vector<uint8_t> from_hex_bytes(const std::string& h)
{
    std::vector<uint8_t> b;
    if (h == "-") return b;
    for (size_t i = 0; i + 1 < h.size(); i += 2) b.push_back(uint8_t(std::stoul(h.substr(i, 2), nullptr, 16)));
    return b;
}

FilterStack::Entry kind_entry(const std::string& kinds, const std::string& keep)
{
    return {find_filter("binary-kind-v1"), {{"kinds", kinds}, {"keep", keep}}};
}

// file-kinds-v1 and binary-kind-v1 against the oracle: kinds of chosen and random files, the
// survivors of each kind set on lines of several lengths, survivors by rank, and the compact
// scrambled order.
void test_kind_vectors(const std::string& path)
{
    int n = 0;
    std::map<std::string, std::unique_ptr<KindCounter>> counters;
    auto counter = [&](const std::string& N, const std::string& kinds, const std::string& keep) -> const KindCounter& {
        auto& c = counters[N + "/" + kinds + "/" + keep];
        if (!c)
        {
            KindSet s = kind_set_of(kinds);
            if (keep == "exclude")
                for (auto& f : s) f = f ? 0 : 1;
            c = std::make_unique<KindCounter>(std::stoull(N), s);
        }
        return *c;
    };
    for (const auto& f : read_rows(path))
    {
        bool ok = false;
        if (f[0] == "kind" && f.size() == 4)
        {
            const auto b = from_hex_bytes(f[1]);
            ok = file_kind(b, std::stoull(f[2])) == f[3];
        }
        else if (f[0] == "count" && f.size() == 5) ok = counter(f[1], f[2], f[3]).count().to_decimal() == f[4];
        else if (f[0] == "file" && f.size() == 6)
        {
            const KindCounter& c = counter(f[1], f[2], f[3]);
            const BigUint k = BigUint::from_decimal(f[4]);
            const auto want = from_hex_bytes(f[5]);
            ok = c.unrank(k) == want && c.rank(want) == k;
        }
        else if (f[0] == "compact" && f.size() == 8)
        {
            const BinarySpace space(std::stoull(f[1]), f[2]);
            const BinarySieve bs(space, {kind_entry(f[3], f[4])});
            const BigUint index = BigUint::from_decimal(f[6]), number = BigUint::from_decimal(f[7]);
            const auto file = bs.counter().unrank(number);
            ok = bs.id() == f[5] && bs.index_of(file, AddressMode::Scrambled) == index && bs.file_at(index, AddressMode::Scrambled) == file &&
                 bs.rank_of_index(index, AddressMode::Scrambled) == number && bs.index_of(file, AddressMode::Positional) == number;
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  kind vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x;
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "file-kind vectors checked: " << n << "\n";
    CHECK(n >= 500);
}

// The kinds' counts, every file of up to 2 bytes walked (the oracle's vectors reach further); a BinarySieve's verdicts, and its
// compact orderings round the survivors both ways.
void test_kinds()
{
    for (const std::string set : {"signed", "text", "unknown", "empty", "exe", "gz", "any", "signed-or-text"})
    {
        const KindCounter k(2, kind_set_of(set));
        uint64_t n = 0;
        bool ok = true;
        auto visit = [&](const std::vector<uint8_t>& f) {
            if (!k.passes(f, f.size())) return;
            if (n % 257 == 0) ok = ok && k.rank(f) == BigUint(n) && k.unrank(BigUint(n)) == f;
            ++n;
        };
        visit({});
        for (int L = 1; L <= 2; ++L)
        {
            std::vector<uint8_t> f(size_t(L), 0);
            for (;;)
            {
                visit(f);
                int i = L;
                while (i > 0 && ++f[size_t(i - 1)] == 0) --i;
                if (i == 0) break;
            }
        }
        CHECK(ok);
        CHECK(k.count() == BigUint(n));
    }
    // A sieve on a 40-byte line: every survivor it hands out passes, and comes back to its place.
    const BinarySpace space(40, "sieve");
    const BinarySieve bs(space, {kind_entry("signed", "keep")});
    std::mt19937_64 rng(40);
    for (int i = 0; i < 200; ++i)
    {
        std::string h;
        for (int d = 0; d < 110; ++d) h += "0123456789abcdef"[rng() % 16];
        const BigUint index = BigUint::mod(BigUint::from_hex(h), bs.count());
        for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            const auto f = bs.file_at(index, m);
            CHECK(bs.passes(f, f.size()) && is_signed_kind(file_kind(f, f.size())));
            CHECK(bs.index_of(f, m) == index);
        }
    }
    CHECK(bs.first_failure(std::vector<uint8_t>{'a', 'b'}, 2) == "binary-kind-v1");
    CHECK(throws([&] { (void)bs.index_of({'a', 'b'}, AddressMode::Positional); }));
}

FilterLine alphabet_line(const Alphabet& a, uint32_t L) { return {"text", a.id(), a.size(), L, &a, 0, 0, 0}; }

// not-written-v1 against the oracle (verdicts, and survivors counted by its own walk and closed
// form), then against itself: the automata's verdict and the readings decoded outright agree on
// every unit of short lines, the count is the brute force's, and survivors round-trip by rank.
void test_written_vectors(const std::string& dir)
{
    const TestResources none(nullptr, nullptr);
    int n = 0;
    const FilterSpec* spec = find_filter("not-written-v1");
    CHECK(spec != nullptr);
    if (!spec) return;
    std::map<std::string, std::shared_ptr<const PluginDef>> plugins;
    for (const auto& f : read_rows(dir + "vectors_written_v1.tsv"))
    {
        bool ok = false;
        if (f[0] == "judge" && f.size() == 5)
        {
            const Alphabet& a = alphabet_of(f[1]);
            const auto bytes = from_hex_bytes(f[3]);
            const std::u32string text = utf8_decode(std::string(bytes.begin(), bytes.end()));
            std::optional<WrittenReading> r;
            std::vector<uint32_t> unit;
            for (char32_t c : text) unit.push_back(*a.digit_of(c));
            if (holds_all_bytes(a)) r = written_as_bytes(unit);
            else r = written_as(text, written_mask_of(f[2]));
            const std::string got = r ? r->decoder + ":" + r->kind : "-";
            ok = got == f[4];
            // The automata give the same verdict.
            const bool by_rule = written_by_rule(*written_rule(a, written_mask_of(f[2])), unit);
            ok = ok && by_rule == r.has_value();
        }
        else if (f[0] == "count" && (f.size() == 5 || f.size() == 7))
        {
            const Alphabet& a = alphabet_of(f[1]);
            const uint32_t L = uint32_t(std::stoul(f[3]));
            std::vector<FilterStack::Entry> entries{{spec, {{"readings", f[2]}}}};
            std::unique_ptr<FilterSpec> plug;
            if (f.size() == 7)
            {
                auto& p = plugins[f[5]];
                if (!p) p = load_plugin_file(dir + "../data/filters/" + f[5] + ".sfilter");
                plug = std::make_unique<FilterSpec>(plugin_spec(p));
                FilterValues v;
                if (f[6] != "-") v[f[6].substr(0, f[6].find('='))] = f[6].substr(f[6].find('=') + 1);
                entries.push_back({plug.get(), v});
            }
            const FilterStack st(alphabet_line(a, L), entries, none);
            ok = st.ranker() && st.ranker()->count().to_decimal() == f[4];
            if (!ok) std::cerr << "  (" << (st.ranker() ? st.ranker()->count().to_decimal() : "no ranker: " + st.compact_blocker()) << ")\n";
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  written vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x;
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "not-written vectors checked: " << n << "\n";
    CHECK(n >= 550);

    // Every unit of short lines: the verdicts agree, and the count is the brute force's.
    for (const auto& [id, L] : std::vector<std::pair<std::string, uint32_t>>{{"lower27", 4}, {"ascii95", 3}, {"bytes256", 2}})
    {
        const Alphabet& a = alphabet_of(id);
        const FilterStack st(alphabet_line(a, L), {{spec, {}}}, none);
        CHECK(st.ranker() != nullptr);
        if (!st.ranker()) continue;
        const auto rule = written_rule(a, written_mask_of("all"));
        std::vector<uint32_t> u(L, 0);
        uint64_t kept = 0;
        bool agree = true, round = true;
        for (;;)
        {
            const bool pass = st.passes(u);
            agree = agree && pass == !written_by_rule(*rule, u);
            if (pass)
            {
                if (kept % 1013 == 0) round = round && st.ranker()->rank(u) == BigUint(kept) && st.ranker()->unrank(BigUint(kept)) == u;
                ++kept;
            }
            size_t i = L;
            while (i > 0 && ++u[i - 1] == a.size()) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(agree);
        CHECK(round);
        CHECK(st.ranker()->count() == BigUint(kept));
    }

    // At the hallway's length, on each alphabet: survivors by rank pass and come back to their
    // rank; units written out under each reading fail, and are found again by the rule.
    std::mt19937_64 rng(2026);
    for (const std::string id : {"lower27", "babel29", "ascii95"})
    {
        const Alphabet& a = alphabet_of(id);
        const FilterStack st(alphabet_line(a, 32), {{spec, {}}}, none);
        CHECK(st.ranker() != nullptr);
        if (!st.ranker()) continue;
        for (int i = 0; i < 40; ++i)
        {
            std::string h;
            for (int d = 0; d < 60; ++d) h += "0123456789abcdef"[rng() % 16];
            const BigUint k = BigUint::mod(BigUint::from_hex(h), st.ranker()->count());
            const auto u = st.ranker()->unrank(k);
            CHECK(st.passes(u));
            CHECK(st.ranker()->rank(u) == k);
        }
    }
    // Written out, on purpose: a page of 32 letters with "ftyp" at 4 is an MP4 file as text; the
    // same page reads as "MZ" in nibbles when it is "enfk" and padding; bits in two letters.
    const Alphabet& lower = alphabet_of("lower27");
    auto unit_of = [&](const std::string& t) {
        std::vector<uint32_t> u;
        for (char c : t) u.push_back(*lower.digit_of(char32_t(uint8_t(c))));
        u.resize(32, 0);
        return u;
    };
    const FilterStack st32(alphabet_line(lower, 32), {{spec, {}}}, none);
    CHECK(!st32.passes(unit_of("abcdftyp")));
    CHECK(!st32.passes(unit_of("enfk")));
    CHECK(!st32.passes(unit_of("abaabbab" "ababbaba"))); // 4d 5a as bits, a = 0: "MZ", an EXE
    CHECK(!st32.passes(unit_of("aaaaaaaa" "aabaabbb" "aaaaaaaa"))); // ff d8 ff with a = 1: a JPG, the other way round
    CHECK(st32.passes(unit_of("hello world")));
    std::cout << "not-written checked\n";
}


// The cross-line filters (not-a-file-v1, not-other-line-v1, not-packed-v1, and not-an-item-v1's
// pages), against the oracle's vectors (tests/vectors_cross_v1.tsv), then a few brute-force checks.
void test_cross_vectors(const std::string& dir)
{
    const TestResources none(nullptr, nullptr);
    const FilterSpec* not_file = find_filter("not-a-file-v1");
    const FilterSpec* not_other = find_filter("not-other-line-v1");
    const FilterSpec* not_packed = find_filter("not-packed-v1");
    const FilterSpec* not_item = find_filter("not-an-item-v1");
    CHECK(not_file && not_other && not_packed && not_item);
    if (!not_file || !not_other || !not_packed || !not_item) return;
    auto number_line = [](uint32_t base, uint32_t L) { return FilterLine{"image", "cross", base, L, nullptr, L, 1, 0}; };
    std::map<std::string, std::unique_ptr<FilterStack>> stacks;
    auto file_stack = [&](uint32_t base, uint32_t L) -> const FilterStack& {
        auto& st = stacks[std::to_string(base) + "/" + std::to_string(L)];
        if (!st) st = std::make_unique<FilterStack>(number_line(base, L), std::vector<FilterStack::Entry>{{not_file, {}}}, none);
        return *st;
    };
    // A page's files as the binary line holds them: exactly its symbols' bytes (every byte, for
    // bytes256).
    auto pattern_of = [](const Alphabet& a, uint32_t L) {
        KindCounter::Pattern p;
        std::array<bool, 256> at{};
        for (uint32_t d = 0; d < a.size(); ++d) at[a.symbol(d) & 0xff] = true;
        if (a.size() == 256) at.fill(true);
        p.allowed.assign(L, at);
        return p;
    };
    int n = 0;
    for (const auto& f : read_rows(dir + "vectors_cross_v1.tsv"))
    {
        bool ok = false;
        if (f[0] == "not-a-file" && f.size() == 4)
        {
            const FilterStack& st = file_stack(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])));
            ok = st.ranker() && st.ranker()->count().to_decimal() == f[3];
        }
        else if (f[0] == "a-file-unit" && f.size() == 5)
        {
            const FilterStack& st = file_stack(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])));
            const std::vector<uint32_t> want = split_u32(f[4]);
            const BigUint k = BigUint::from_decimal(f[3]);
            ok = st.ranker() && st.ranker()->unrank(k) == want && st.ranker()->rank(want) == k && st.passes(want);
        }
        else if (f[0] == "other-judge" && f.size() == 5)
        {
            const Alphabet& a = alphabet_of(f[1]);
            const auto bytes = from_hex_bytes(f[3]);
            const std::u32string text = utf8_decode(std::string(bytes.begin(), bytes.end()));
            const auto v = other_line_as(text, other_line_mask_of(f[2]));
            std::vector<uint32_t> unit;
            for (char32_t c : text) unit.push_back(*a.digit_of(c));
            const FilterStack st(alphabet_line(a, uint32_t(unit.size())), {{not_other, {{"forms", f[2]}}}}, none);
            ok = (v ? *v : std::string("-")) == f[4] && st.passes(unit) == !v.has_value();
        }
        else if (f[0] == "other" && f.size() == 5)
        {
            const FilterStack st(alphabet_line(alphabet_of(f[1]), uint32_t(std::stoul(f[3]))), {{not_other, {{"forms", f[2]}}}}, none);
            ok = st.ranker() && st.ranker()->count().to_decimal() == f[4];
        }
        else if (f[0] == "packed" && f.size() == 4)
        {
            const FilterStack st(number_line(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2]))), {{not_packed, {}}}, none);
            ok = st.ranker() && st.ranker()->count().to_decimal() == f[3];
        }
        else if ((f[0] == "models" && f.size() == 5) || (f[0] == "model-unit" && f.size() == 6))
        {
            const ModelSpace space(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), uint32_t(std::stoul(f[3])));
            const ModelSieve ms(space, {{not_file, {}}});
            if (f[0] == "models") ok = ms.count().to_decimal() == f[4];
            else
            {
                const BigUint k = BigUint::from_decimal(f[4]), v = BigUint::from_decimal(f[5]);
                ok = ms.model_at(k, AddressMode::Positional) == v && ms.index_of(v, AddressMode::Positional) == k && ms.first_failure(v).empty();
            }
        }
        else if ((f[0] == "mesh" && f.size() == 6) || (f[0] == "mesh-unit" && f.size() == 7))
        {
            const ModelSpace space(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), uint32_t(std::stoul(f[3])));
            std::vector<FilterStack::Entry> entries;
            for (char r : f[4])
                entries.push_back({find_filter(r == 'v' ? "distinct-vertices-v1" : r == 'i' ? "distinct-indices-v1" : r == 'c' ? "canonical-mesh-v1" : "every-vertex-used-v1"), {}});
            const ModelSieve ms(space, entries);
            if (f[0] == "mesh") ok = ms.can_rank() && ms.count().to_decimal() == f[5];
            else
            {
                const BigUint k = BigUint::from_decimal(f[5]), v = BigUint::from_decimal(f[6]);
                ok = ms.model_at(k, AddressMode::Positional) == v && ms.index_of(v, AddressMode::Positional) == k && ms.first_failure(v).empty();
            }
        }
        else if (f[0] == "max-run" && f.size() == 5)
        {
            const FilterStack st(alphabet_line(alphabet_of(f[1]), uint32_t(std::stoul(f[2]))), {{find_filter("max-run-v1"), {{"max_run", f[3]}}}}, none);
            ok = st.ranker() && st.ranker()->count().to_decimal() == f[4];
        }
        else if ((f[0] == "pattern" && f.size() == 8) || (f[0] == "pattern-unit" && f.size() == 9))
        {
            const FilterValues v{{"width", f[3]}, {"order", f[4]}, {"period", f[5]}, {"ramps", f[6]}};
            const FilterStack st(number_line(uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2]))), {{find_filter("not-a-pattern-v1"), v}}, none);
            if (!st.ranker()) ok = false;
            else if (f[0] == "pattern") ok = st.ranker()->count().to_decimal() == f[7];
            else
            {
                const BigUint k = BigUint::from_decimal(f[7]);
                const std::vector<uint32_t> want = split_u32(f[8]);
                ok = st.ranker()->unrank(k) == want && st.ranker()->rank(want) == k && st.passes(want);
            }
        }
        else if (f[0] == "item-pages" && f.size() == 7)
        {
            const BinarySpace space(std::stoull(f[1]), "sieve");
            BinaryItems items;
            items.pages = pattern_of(alphabet_of(f[4]), uint32_t(std::stoul(f[5])));
            const BinarySieve bs(space, {kind_entry(f[2], f[3]), {not_item, {{"items", "pages"}}}}, &items);
            ok = bs.can_rank() && bs.count().to_decimal() == f[6];
            if (!ok) std::cerr << "  (" << bs.count().to_decimal() << " " << bs.blocker() << ")\n";
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  cross vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x;
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "cross-line vectors checked: " << n << "\n";
    CHECK(n >= 270);

    // max-run-v1 as an automaton: every unit of short lines judged as the rule says (no symbol but
    // SPACE more than max_run times in a row), counted, and ranked in order; and it merges with a
    // custom filter and not-written (one automaton), so that stack compacts.
    for (const auto& [id, L, R] : std::vector<std::tuple<std::string, uint32_t, uint32_t>>{{"lower27", 4, 1}, {"lower27", 5, 2}, {"u+0020-u+0022", 9, 2}})
    {
        const Alphabet& a = alphabet_of(id);
        const FilterStack st(alphabet_line(a, L), {{find_filter("max-run-v1"), {{"max_run", std::to_string(R)}}}}, none);
        CHECK(st.ranker() != nullptr);
        if (!st.ranker()) continue;
        const auto sp = a.digit_of(U' ');
        std::vector<uint32_t> u(L, 0);
        uint64_t kept = 0;
        bool agree = true, round = true;
        for (;;)
        {
            uint32_t run = 0;
            bool rule = true;
            for (size_t i = 0; i < L; ++i)
            {
                run = (i > 0 && u[i] == u[i - 1] && (!sp || u[i] != *sp)) ? run + 1 : 1;
                rule = rule && run <= R;
            }
            agree = agree && st.passes(u) == rule;
            if (rule)
            {
                round = round && st.ranker()->rank(u) == BigUint(kept) && st.ranker()->unrank(BigUint(kept)) == u;
                ++kept;
            }
            size_t i = L;
            while (i > 0 && ++u[i - 1] == a.size()) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(agree);
        CHECK(round);
        CHECK(st.ranker()->count() == BigUint(kept));
    }
    {
        const FilterStack both(alphabet_line(alphabet_of("lower27"), 16), {{find_filter("max-run-v1"), {}}, {find_filter("not-written-v1"), {}}}, none);
        CHECK(both.ranker() != nullptr); // merged with not-written: compact
        CHECK(filter_conflict(*find_filter("max-run-v1"), *find_filter("not-written-v1")).empty());
    }

    // not-a-pattern-v1: every unit of short lines at several settings, ranked in order, and the
    // generic walk (completions from each prefix) agreeing with the filter's own rank.
    for (const auto& [base, L, width, order] : std::vector<std::tuple<uint32_t, uint32_t, std::string, std::string>>{
             {2, 14, "1", "big"}, {3, 8, "1", "big"}, {2, 16, "2", "little"}, {2, 16, "4", "big"}, {4, 7, "2", "big"}})
    {
        const FilterStack st(number_line(base, L), {{find_filter("not-a-pattern-v1"), {{"width", width}, {"order", order}}}}, none);
        CHECK(st.ranker() != nullptr);
        if (!st.ranker()) continue;
        std::vector<uint32_t> u(L, 0);
        uint64_t kept = 0;
        bool round = true;
        for (;;)
        {
            if (st.passes(u))
            {
                if (kept % 37 == 0) round = round && st.ranker()->rank(u) == BigUint(kept) && st.ranker()->unrank(BigUint(kept)) == u &&
                                            st.ranker()->Ranker::rank(u) == BigUint(kept);
                ++kept;
            }
            size_t i = L;
            while (i > 0 && ++u[i - 1] == base) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(round);
        CHECK(st.ranker()->count() == BigUint(kept));
    }
    {
        // A ramp through every byte, a 16-bit counter and a repeated block fail; text passes.
        const FilterStack bytes(number_line(256, 32), {{find_filter("not-a-pattern-v1"), {}}}, none);
        std::vector<uint32_t> ramp(32), rep(32), word(32);
        for (uint32_t i = 0; i < 32; ++i)
        {
            ramp[i] = (200 + i * 7) % 256;
            rep[i] = std::vector<uint32_t>{1, 2, 3}[i % 3];
        }
        CHECK(!bytes.passes(ramp));
        CHECK(!bytes.passes(rep));
        const FilterStack words16(number_line(256, 32), {{find_filter("not-a-pattern-v1"), {{"width", "2"}, {"order", "little"}}}}, none);
        for (uint32_t k = 0; k < 16; ++k)
        {
            const uint32_t v = (0xfff0 + k * 3) % 65536;
            word[2 * k] = v % 256;
            word[2 * k + 1] = v / 256;
        }
        CHECK(!words16.passes(word));
        CHECK(bytes.passes(word)); // not a pattern of single bytes
        std::string t = "it was the best of times it was ";
        CHECK(bytes.passes(std::vector<uint32_t>(t.begin(), t.end())));
    }

    // The models line: every model of a small shape, against file_kind_at, in both compact orders.
    {
        const ModelSpace space(3, 1, 2);
        const ModelSieve ms(space, {{not_file, {}}});
        uint64_t kept = 0;
        bool agree = true, round = true;
        for (uint64_t v = 0; BigUint(v) < space.size(); ++v)
        {
            const bool pass = ms.first_failure(BigUint(v)).empty();
            agree = agree && pass == !is_signed_kind(file_kind_at(BigUint(v)));
            if (!pass) continue;
            for (const AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
            {
                const BigUint c = ms.index_of(BigUint(v), m);
                round = round && ms.model_at(c, m) == BigUint(v) && ms.rank_of_index(c, m) == BigUint(kept);
            }
            ++kept;
        }
        CHECK(agree);
        CHECK(round);
        CHECK(ms.count() == BigUint(kept));
        CHECK(kept < 13824); // one model's number is a signed file
        const ModelSieve none(space, {});
        CHECK(none.empty() && none.count() == space.size());
        // The line's own rules at the same shape: every model judged, the survivors in positional
        // order, and every one round trip through both compact orders.
        const ModelSieve rules(space, {{find_filter("distinct-vertices-v1"), {}}, {find_filter("distinct-indices-v1"), {}},
                                       {find_filter("every-vertex-used-v1"), {}}});
        uint64_t n_rules = 0;
        bool order = true;
        for (uint64_t v = 0; BigUint(v) < space.size(); ++v)
        {
            if (!rules.first_failure(BigUint(v)).empty()) continue;
            order = order && rules.index_of(BigUint(v), AddressMode::Positional) == BigUint(n_rules) &&
                    rules.model_at(rules.index_of(BigUint(v), AddressMode::Scrambled), AddressMode::Scrambled) == BigUint(v);
            ++n_rules;
        }
        CHECK(order);
        CHECK(rules.count() == BigUint(n_rules));
        // With not-a-file-v1 too: judged, not counted.
        const ModelSieve both(space, {{not_file, {}}, {find_filter("distinct-indices-v1"), {}}});
        CHECK(!both.can_rank() && !both.blocker().empty());
    }
    // canonical-mesh-v1: every model of a small shape judged against the rule as written, the
    // survivors in positional order through both compact orders, alone and with every-vertex-used;
    // and a mesh's canonical form is a survivor holding the same points and the same faces.
    {
        const ModelSpace space(3, 2, 2);
        for (const bool used : {false, true})
        {
            std::vector<FilterStack::Entry> entries{{find_filter("canonical-mesh-v1"), {}}};
            if (used) entries.push_back({find_filter("every-vertex-used-v1"), {}});
            const ModelSieve cm(space, entries);
            uint64_t kept = 0;
            bool agree = true, order = true;
            for (uint64_t v = 0; BigUint(v) < space.size(); ++v)
            {
                const ModelSpace::Parts p = space.parts_at(BigUint(v), AddressMode::Positional);
                auto point = [&](int i) { return (p.verts[3 * i] * 2 + p.verts[3 * i + 1]) * 2 + p.verts[3 * i + 2]; };
                auto face = [&](int k) { return std::array<uint32_t, 3>{p.faces[3 * k], p.faces[3 * k + 1], p.faces[3 * k + 2]}; };
                bool rule = point(0) < point(1) && point(1) < point(2) && face(0) < face(1);
                for (int k = 0; k < 2; ++k)
                    rule = rule && face(k)[0] < face(k)[1] && face(k)[0] < face(k)[2] && face(k)[1] != face(k)[2];
                // With three vertices every face names all of them, so every-vertex-used adds nothing here.
                const bool pass = cm.first_failure(BigUint(v)).empty();
                agree = agree && pass == rule;
                if (!pass) continue;
                order = order && cm.index_of(BigUint(v), AddressMode::Positional) == BigUint(kept) &&
                        cm.model_at(cm.index_of(BigUint(v), AddressMode::Scrambled), AddressMode::Scrambled) == BigUint(v);
                ++kept;
            }
            CHECK(agree);
            CHECK(order);
            CHECK(cm.can_rank() && cm.count() == BigUint(kept));
            CHECK(kept == 56); // C(8, 3) * C(2, 2)
        }
        // A cube, its corners listed in any order: its canonical form is a survivor, with the same
        // points and the same faces, each started at its smallest corner.
        const ModelSpace big(8, 12, 16);
        std::vector<ModelSpace::Vertex> corners;
        for (int i = 0; i < 8; ++i) corners.push_back({i & 4 ? 1.0f : -1.0f, i & 1 ? 1.0f : -1.0f, i & 2 ? 1.0f : -1.0f});
        const uint32_t quads[6][4] = {{0, 1, 3, 2}, {4, 6, 7, 5}, {0, 4, 5, 1}, {2, 3, 7, 6}, {0, 2, 6, 4}, {1, 5, 7, 3}};
        std::vector<ModelSpace::Face> tris;
        for (const auto& q : quads)
        {
            tris.push_back({q[2], q[0], q[1]}); // not started at the smallest corner
            tris.push_back({q[0], q[2], q[3]});
        }
        const ModelSpace::Parts raw = big.fit(corners, tris).parts;
        const auto canon = canonical_mesh(big, raw);
        CHECK(canon.has_value());
        const ModelSieve cms(big, {{find_filter("canonical-mesh-v1"), {}}, {find_filter("every-vertex-used-v1"), {}}});
        const BigUint at = big.index_of(*canon, AddressMode::Positional);
        CHECK(cms.first_failure(at).empty() && !cms.first_failure(big.index_of(raw, AddressMode::Positional)).empty());
        CHECK(cms.can_rank() && cms.model_at(cms.index_of(at, AddressMode::Positional), AddressMode::Positional) == at);
        auto sorted_points = [&](const ModelSpace::Parts& p) {
            std::vector<std::array<uint32_t, 3>> v;
            for (int i = 0; i < 8; ++i) v.push_back({p.verts[3 * i], p.verts[3 * i + 1], p.verts[3 * i + 2]});
            std::sort(v.begin(), v.end());
            return v;
        };
        auto face_points = [&](const ModelSpace::Parts& p) {
            // Each face as the points of its corners, rotated to start at its smallest point.
            std::vector<std::array<uint64_t, 3>> f;
            for (int k = 0; k < 12; ++k)
            {
                std::array<uint64_t, 3> t;
                for (int j = 0; j < 3; ++j)
                {
                    const uint32_t i = p.faces[3 * k + j];
                    t[j] = (uint64_t(p.verts[3 * i]) * 16 + p.verts[3 * i + 1]) * 16 + p.verts[3 * i + 2];
                }
                std::rotate(t.begin(), std::min_element(t.begin(), t.end()), t.end());
                f.push_back(t);
            }
            std::sort(f.begin(), f.end());
            return f;
        };
        CHECK(sorted_points(*canon) == sorted_points(raw));
        CHECK(face_points(*canon) == face_points(raw));
        // A face given twice has no canonical form.
        ModelSpace::Parts twice = raw;
        std::copy(twice.faces.begin(), twice.faces.begin() + 3, twice.faces.begin() + 3);
        CHECK(!canonical_mesh(big, twice).has_value());
        // Counts in closed form at the default shape: C(4096, 8) x C(112, 12).
        const ModelSieve alone(big, {{find_filter("canonical-mesh-v1"), {}}});
        CHECK(alone.count().to_decimal() == "8619984089677964781973927273325038295040");
    }

    // not-a-file: every unit of a short line, against file_kind_at, ranked in order.
    for (const auto& [base, L] : std::vector<std::pair<uint32_t, uint32_t>>{{2, 16}, {27, 3}, {256, 2}})
    {
        const FilterStack& st = file_stack(base, L);
        std::vector<uint32_t> u(L, 0);
        uint64_t kept = 0;
        bool agree = true, round = true;
        for (BigUint v; ; v += BigUint(1))
        {
            const bool pass = st.passes(u);
            agree = agree && pass == !is_signed_kind(file_kind_at(v));
            if (pass)
            {
                if (kept % 97 == 0) round = round && st.ranker()->rank(u) == BigUint(kept) && st.ranker()->unrank(BigUint(kept)) == u;
                ++kept;
            }
            size_t i = L;
            while (i > 0 && ++u[i - 1] == base) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(agree);
        CHECK(round);
        CHECK(st.ranker()->count() == BigUint(kept));
    }
    // not-packed: every unit of a short line, against packed_as.
    for (const auto& [base, L] : std::vector<std::pair<uint32_t, uint32_t>>{{2, 16}, {16, 4}, {4, 9}})
    {
        const FilterStack st(number_line(base, L), {{not_packed, {}}}, none);
        const uint32_t bits = base == 2 ? 1 : base == 4 ? 2 : 4;
        std::vector<uint32_t> u(L, 0);
        uint64_t kept = 0;
        bool agree = true;
        for (;;)
        {
            const bool pass = st.passes(u);
            agree = agree && pass == !packed_as(u, bits).has_value();
            kept += pass;
            size_t i = L;
            while (i > 0 && ++u[i - 1] == base) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(agree);
        CHECK(st.ranker() && st.ranker()->count() == BigUint(kept));
    }
    // not-an-item's pages: the survivors, in order, are the kind survivors that are not pages.
    {
        const BinarySpace space(2, "sieve");
        BinaryItems items;
        items.pages = pattern_of(alphabet_of("u+0061-u+0062"), 1);
        const BinarySieve bs(space, {kind_entry("any", "keep"), {not_item, {{"items", "pages"}}}}, &items);
        bool ok = bs.can_rank() && bs.needs_file(); // a page is judged by its whole file
        BigUint last;
        for (uint64_t k = 0; ok && BigUint(k) < bs.count(); k += 211)
        {
            const auto file = bs.file_at(BigUint(k), AddressMode::Positional);
            const bool page = file.size() == 1 && (file[0] == 'a' || file[0] == 'b');
            ok = !page && bs.first_failure_of(file).empty() && bs.index_of(file, AddressMode::Positional) == BigUint(k);
            if (!ok) std::cerr << "  (pages: k " << k << " size " << file.size() << " page " << page << " fail '" << bs.first_failure_of(file) << "')\n";
        }
        CHECK(ok);
        CHECK(!bs.first_failure_of({'a'}).empty());
        CHECK(bs.first_failure_of({'a', '\n'}).empty()); // a page's text only, nothing appended
        CHECK(bs.first_failure_of({'c'}).empty());
    }
    std::cout << "cross-line checked\n";
}


// MIDI read back (midi_to_notation): a melody saved as MIDI and read again, then fitted to its
// set, sounds the same: every voice, sixteenth by sixteenth (which pitch, and where each note
// starts), up to the rests at the end. Rests may be written with fewer events.
std::vector<std::vector<int>> sounding(const NoteSet& set, const std::vector<uint32_t>& unit)
{
    std::vector<std::vector<int>> v(set.voices);
    const size_t per = unit.size() / set.voices;
    for (uint32_t k = 0; k < set.voices; ++k)
    {
        for (size_t e = 0; e < per; ++e)
        {
            const uint32_t d = unit[k * per + e], m = set.midi(d), n = set.sixteenths(d);
            for (uint32_t i = 0; i < n; ++i) v[k].push_back(m == 0 ? -1 : int(m) * 2 + (i == 0 ? 1 : 0));
        }
        while (!v[k].empty() && v[k].back() == -1) v[k].pop_back();
    }
    return v;
}

// palette-size-v1 and row-runs-v1 against the oracle's vectors (tests/vectors_picture_v1.tsv):
// counts, ranks both ways and verdicts, on small lines checked there by brute force and on the
// image line's palettes (rgb24 included) and video.
void test_sound_vectors(const std::string& dir)
{
    const TestResources none(nullptr, nullptr);
    int n = 0;
    for (const auto& f : read_rows(dir + "vectors_sound_v1.tsv"))
    {
        if (!((f[0] == "count" && f.size() == 7) || (f[0] == "unit" && f.size() == 8))) continue;
        const std::string id = f[1] == "sound-silence" ? "silence-run-v1" : f[1] + "-v1";
        const FilterSpec* spec = find_filter(id);
        CHECK(spec != nullptr);
        if (!spec) continue;
        const uint32_t bits = uint32_t(std::stoul(f[2])), ch = uint32_t(std::stoul(f[3])), L = uint32_t(std::stoul(f[4]));
        const PcmFormat fmt = make_pcm_format(8000, bits, ch);
        const FilterLine line{"audio", fmt.id(), fmt.base(), ch * L, nullptr, 0, 0, 0};
        const FilterValues v{{f[1] == "sound-silence" ? "samples" : "percent", f[5]}};
        const FilterStack st(line, {{spec, v}}, none);
        bool ok = st.ranker() != nullptr;
        if (ok && f[0] == "count") ok = st.ranker()->count().to_decimal() == f[6];
        else if (ok)
        {
            const BigUint k = BigUint::from_decimal(f[6]);
            const std::vector<uint32_t> want = split_u32(f[7]);
            ok = st.ranker()->unrank(k) == want && st.ranker()->rank(want) == k && st.passes(want) && st.ranker()->accepts(want);
        }
        CHECK(ok);
        if (!ok) std::cerr << "  sound vector mismatch: " << f[0] << " " << f[1] << " bits=" << f[2] << " ch=" << f[3] << " L=" << f[4] << "\n";
        ++n;
    }
    std::cout << "sound filter vectors checked: " << n << "\n";
    CHECK(n >= 60);
    // How silence-run counts depends on the run it is set to, not only on the line: on a second of
    // 16-bit sound the longest run the line allows is too many states for an automaton, but a run
    // of ten is not, and there it is one, and says so (counts_as_on with its settings), so it is
    // weighed beside the other automata rather than as judging only.
    {
        const FilterSpec* silence = find_filter("silence-run-v1");
        const PcmFormat fmt = make_pcm_format(8000, 16, 1);
        const FilterLine line{"audio", fmt.id(), fmt.base(), 8000, nullptr, 0, 0, 0};
        const FilterValues ten{{"samples", "10"}}, bad{{"samples", "x"}};
        CHECK(silence->counts_as_on(line, nullptr).empty());
        CHECK(silence->counts_as_on(line, &ten) == "automaton");
        CHECK(silence->counts_as_on(line, &bad).empty()); // a malformed setting: as without one
    }
}

void test_picture_vectors(const std::string& dir)
{
    const TestResources none(nullptr, nullptr);
    const FilterSpec* palette = find_filter("palette-size-v1");
    const FilterSpec* runs = find_filter("row-runs-v1");
    CHECK(palette && runs);
    if (!palette || !runs) return;
    int n = 0;
    for (const auto& f : read_rows(dir + "vectors_picture_v1.tsv"))
    {
        bool ok = false;
        const bool is_palette = f[0] == "palette" || f[0] == "palette-unit";
        if ((f[0] == "palette" && f.size() == 8) || (f[0] == "palette-unit" && f.size() == 9) || (f[0] == "runs" && f.size() == 7) ||
            (f[0] == "runs-unit" && f.size() == 8))
        {
            const uint32_t base = uint32_t(std::stoul(f[1])), w = uint32_t(std::stoul(f[2])), h = uint32_t(std::stoul(f[3])),
                           frames = uint32_t(std::stoul(f[4]));
            const FilterLine line{frames > 1 ? "video" : "image", "picture", base, w * h * frames, nullptr, w, h, frames > 1 ? frames : 0};
            const FilterValues v = is_palette ? FilterValues{{"scope", f[5]}, {"colours", f[6]}} : FilterValues{{"changes", f[5]}};
            const FilterStack st(line, {{is_palette ? palette : runs, v}}, none);
            const size_t at = is_palette ? 7 : 6;
            if (!st.ranker()) ok = false;
            else if (f[0] == "palette" || f[0] == "runs") ok = st.ranker()->count().to_decimal() == f[at];
            else
            {
                const BigUint k = BigUint::from_decimal(f[at]);
                const std::vector<uint32_t> want = split_u32(f[at + 1]);
                ok = st.ranker()->unrank(k) == want && st.ranker()->rank(want) == k && st.passes(want) && st.ranker()->accepts(want);
            }
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  picture vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x.substr(0, 60);
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "picture vectors checked: " << n << "\n";
    CHECK(n >= 90);
    // Every unit of a short line judged by the filter and walked by its ranker agree, in a
    // black-and-white video judged frame by frame.
    {
        const FilterLine line{"video", "picture", 3, 2 * 1 * 3, nullptr, 2, 1, 3};
        const FilterStack st(line, {{palette, {{"scope", "frame"}, {"colours", "1"}}}, {runs, {}}}, none);
        // On a small palette both are automata, so together they count (each frame one colour: 27).
        CHECK(st.ranker() && st.ranker()->count() == BigUint(27));
        CHECK(filter_conflict(*palette, *runs, &line).empty() && filter_conflict(*palette, *runs) == "merge");
        // On rgb24 neither automaton could be built: they count their own ways, so together they
        // are judged and not counted, and the line says they clash.
        const FilterLine big{"video", "picture", 1u << 24, 2 * 1 * 3, nullptr, 2, 1, 3};
        const FilterStack both(big, {{palette, {{"scope", "frame"}, {"colours", "1"}}}, {runs, {}}}, none);
        CHECK(!both.ranker() && !both.compact_blocker().empty());
        CHECK(filter_conflict(*palette, *runs, &big) == "merge");
        const FilterStack alone(big, {{palette, {{"scope", "frame"}, {"colours", "1"}}}}, none);
        CHECK(alone.ranker() && alone.ranker()->count() == BigUint::pow(1u << 24, 3));
        const FilterStack one(line, {{palette, {{"scope", "frame"}, {"colours", "1"}}}}, none);
        std::vector<uint32_t> u(6, 0);
        uint64_t kept = 0;
        bool agree = true;
        for (;;)
        {
            const bool pass = one.passes(u);
            agree = agree && pass == one.ranker()->accepts(u) && pass == (u[0] == u[1] && u[2] == u[3] && u[4] == u[5]);
            if (pass) agree = agree && one.ranker()->rank(u) == BigUint(kept++);
            size_t i = 6;
            while (i > 0 && ++u[i - 1] == 3) u[--i] = 0;
            if (i == 0) break;
        }
        CHECK(agree);
        CHECK(kept == 27 && one.ranker()->count() == BigUint(27));
    }
}

// utf8-valid-v1 on the binary line against the oracle's vectors (tests/vectors_utf8_v1.tsv): the
// survivors' counts, and files by rank both ways through the compact positional order.
void test_utf8_vectors(const std::string& dir)
{
    const FilterSpec* utf8 = find_filter("utf8-valid-v1");
    CHECK(utf8);
    if (!utf8) return;
    int n = 0;
    for (const auto& f : read_rows(dir + "vectors_utf8_v1.tsv"))
    {
        bool ok = false;
        if ((f[0] == "utf8" && f.size() == 4) || (f[0] == "utf8-unit" && f.size() == 5))
        {
            const BinarySpace space(std::stoull(f[1]), "sieve");
            const BinarySieve bs(space, {{utf8, {{"controls", f[2]}}}});
            if (!bs.can_rank()) ok = false;
            else if (f[0] == "utf8") ok = bs.count().to_decimal() == f[3];
            else
            {
                const BigUint k = BigUint::from_decimal(f[3]);
                const std::vector<uint8_t> file = f[4] == "-" ? std::vector<uint8_t>{} : from_hex_bytes(f[4]);
                ok = bs.file_at(k, AddressMode::Positional) == file && bs.index_of(file, AddressMode::Positional) == k &&
                     bs.first_failure_of(file).empty() && bs.needs_file();
            }
        }
        else if (f[0] == "utf8-joint" && f.size() == 7)
        {
            // Counted with binary-kind-v1, and not-an-item-v1's pages when a page shape is given.
            const BinarySpace space(std::stoull(f[1]), "sieve");
            std::vector<FilterStack::Entry> entries{{utf8, {{"controls", f[2]}}}, kind_entry(f[3], f[4])};
            BinaryItems items;
            if (f[5] != "-")
            {
                const size_t colon = f[5].find(':');
                const Alphabet& a = alphabet_of(f[5].substr(0, colon));
                KindCounter::Pattern p;
                std::array<bool, 256> at{};
                for (uint32_t d = 0; d < a.size(); ++d) at[a.symbol(d) & 0xff] = true;
                p.allowed.assign(std::stoul(f[5].substr(colon + 1)), at);
                items.pages = p;
                entries.push_back({find_filter("not-an-item-v1"), {{"items", "pages"}}});
            }
            const BinarySieve bs(space, entries, &items);
            ok = bs.can_count() && bs.count_exact() && !bs.can_rank() && bs.survivors().to_decimal() == f[6];
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  utf8 vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x.substr(0, 60);
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "utf8 vectors checked: " << n << "\n";
    CHECK(n >= 300);
    // Malformed and unwanted files fail; with a kind filter the stack judges only.
    const BinarySpace space(8, "sieve");
    const BinarySieve text(space, {{utf8, {{"controls", "text"}}}});
    for (const std::vector<uint8_t>& bad : std::vector<std::vector<uint8_t>>{
             {0xC0, 0xAF}, {0xED, 0xA0, 0x80}, {0xF4, 0x90, 0x80, 0x80}, {0xE2, 0x82}, {0x00}, {0x7F}, {0xC2, 0x85}, {0x80}})
        CHECK(text.first_failure_of(bad) == "utf8-valid-v1");
    CHECK(text.first_failure_of({'h', 0xC3, 0xA9, '\n'}).empty());
    const BinarySieve both(space, {{utf8, {}}, {find_filter("binary-kind-v1"), {{"kinds", "text"}}}});
    CHECK(!both.can_rank() && !both.blocker().empty() && both.can_count() && both.count_exact());
}

// utf8-valid-v1 counted with binary-kind-v1 and not-an-item-v1's pages (KindCounter::utf8_count):
// against every file of up to 2 bytes judged one by one; with every kind it is
// utf8-valid-v1's own count; and the estimate past the table agrees with the exact count where
// both can be had.
void test_utf8_joint()
{
    const FilterSpec* utf8 = find_filter("utf8-valid-v1");
    const FilterSpec* not_item = find_filter("not-an-item-v1");
    CHECK(utf8 && not_item);
    if (!utf8 || !not_item) return;
    auto pattern = [](std::initializer_list<int> bytes, size_t length) {
        KindCounter::Pattern p;
        std::array<bool, 256> at{};
        for (int b : bytes) at[size_t(b)] = true;
        p.allowed.assign(length, at);
        return p;
    };
    std::array<bool, 256> lower{};
    lower[' '] = true;
    for (int c = 'a'; c <= 'z'; ++c) lower[size_t(c)] = true;
    KindCounter::Pattern pages;
    pages.allowed.assign(2, lower);
    const std::vector<KindCounter::Pattern> patterns{pages, pattern({0xC3, 0xA9, 'a', 0x80}, 2)};
    int mismatches = 0, checked = 0;
    auto brute = [&](uint32_t most, const std::vector<FilterStack::Entry>& entries, const BinaryItems* items) {
        const BinarySpace space(most, "sieve");
        const BinarySieve bs(space, entries, items);
        uint64_t kept = 0;
        std::vector<uint8_t> f;
        for (uint32_t len = 0; len <= most; ++len)
        {
            f.assign(len, 0);
            for (uint64_t v = 0; v < (uint64_t(1) << (8 * len)); ++v)
            {
                for (uint32_t i = 0; i < len; ++i) f[i] = uint8_t(v >> (8 * (len - 1 - i)));
                kept += bs.first_failure_of(f).empty() ? 1 : 0;
            }
        }
        ++checked;
        if (!bs.can_count() || !bs.count_exact() || bs.survivors() != BigUint(kept))
        {
            if (++mismatches <= 5)
                std::cerr << "  utf8 joint mismatch at " << most << ": counted " << (bs.can_count() ? bs.survivors().to_decimal() : "-") << ", judged " << kept << "\n";
        }
    };
    for (const std::string ctl : {"any", "text"})
    {
        for (const std::string& kinds : kind_set_names())
            for (const std::string keep : {"keep", "exclude"})
            {
                brute(2, {{utf8, {{"controls", ctl}}}, kind_entry(kinds, keep)}, nullptr);
                for (const auto& pat : patterns)
                {
                    BinaryItems items;
                    items.pages = pat;
                    brute(2, {kind_entry(kinds, keep), {utf8, {{"controls", ctl}}}, {not_item, {{"items", "pages"}}}}, &items);
                }
            }
    }
    CHECK(mismatches == 0);
    std::cout << "utf8 joint counts checked against every file: " << checked << "\n";
    // Every kind: utf8-valid-v1's own count, past the head too.
    for (uint32_t most : {17u, 40u, 1000u})
        for (const std::string ctl : {"any", "text"})
        {
            const BinarySpace space(most, "sieve");
            const BinarySieve alone(space, {{utf8, {{"controls", ctl}}}});
            const BinarySieve with(space, {{utf8, {{"controls", ctl}}}, kind_entry("any", "keep")});
            CHECK(alone.can_rank() && !with.can_rank() && with.can_count() && with.count_exact());
            CHECK(with.survivors() == alone.count());
        }
    // The estimate (a matrix power in doubles) against the exact count, with kinds and pages.
    for (uint32_t most : {20u, 300u, 3000u})
        for (const std::string ctl : {"any", "text"})
            for (const std::string kinds : {"any", "text", "signed", "pdf"})
            {
                const KindCounter kc(most, kind_set_of(kinds));
                const Utf8Counter u(most, ctl == "text");
                const BigUint exact = kc.utf8_count(ctl == "text", u.tail_sums(most - 16));
                const Scaled est = kc.utf8_estimate(ctl == "text", Utf8Counter::tail_estimate(ctl == "text", most - 16));
                const bool close = exact.is_zero() ? est.is_zero() : std::abs(est.log10() - exact.log10_approx()) < 1e-9;
                if (!close) std::cerr << "  utf8 estimate off at " << most << " " << ctl << " " << kinds << ": " << est.log10() << " vs " << exact.log10_approx() << "\n";
                CHECK(close);
            }
    // Past the table: estimated, not ranked, and said why.
    {
        const BinarySpace space(1000000, "sieve");
        BinaryItems items;
        items.pages = pages;
        const BinarySieve bs(space, {kind_entry("text", "keep"), {utf8, {{"controls", "text"}}}, {not_item, {{"items", "pages"}}}}, &items);
        CHECK(bs.can_count() && !bs.count_exact() && !bs.can_rank() && !bs.blocker().empty());
        const double share = bs.survivors_log10() - space.size().log10_approx();
        CHECK(std::isfinite(share) && share < -1000 && share > -8.0 * 1000000);
    }
}

// The filter memory (plugin.hpp): a setting, not a constant. Each kind of table counts with it set
// just above what it needs and judges only just below, says so naming it, and reports its size.
void test_filter_memory(const std::string& dir)
{
    const std::string filters = dir + "../data/filters/";
    const TestResources none(nullptr, nullptr);
    const double before = filter_memory();
    CHECK(before == kDefaultFilterMemory);
    // Two plugins merged into one automaton.
    const auto clean = load_plugin_file(filters + "clean-data-v1.sfilter");
    const auto pairs = load_plugin_file(filters + "letter-pairs-v1.sfilter");
    const FilterSpec cs = plugin_spec(clean), ps = plugin_spec(pairs);
    auto merged = [&] { return FilterStack(text_line(2000), {{&cs, {}}, {&ps, {}}}, none); };
    const double need = merged().table_bytes();
    CHECK(need > 4.0 * 1024 * 1024); // past the setting's least (1 MB), so both sides of it can be set
    set_filter_memory(need * 1.01);
    CHECK(merged().ranker() != nullptr);
    set_filter_memory(need * 0.99);
    {
        const FilterStack st = merged();
        CHECK(st.ranker() == nullptr && st.compact_blocker().find("the filter memory") != std::string::npos);
        CHECK(st.table_bytes() == need);
    }
    // utf8-valid-v1's table on the binary line: exact above, estimated below.
    const FilterSpec* utf8 = find_filter("utf8-valid-v1");
    const BinarySpace space(4000, "sieve");
    set_filter_memory(Utf8Counter::table_bytes(4000) * 1.01);
    {
        const BinarySieve bs(space, {{utf8, {}}});
        CHECK(bs.can_rank() && bs.table_bytes() == Utf8Counter::table_bytes(4000));
    }
    set_filter_memory(Utf8Counter::table_bytes(4000) * 0.99);
    {
        const BinarySieve bs(space, {{utf8, {}}});
        CHECK(!bs.can_rank() && bs.can_count() && !bs.count_exact() && bs.blocker().find("the filter memory") != std::string::npos);
    }
    // A word filter whose row would not fit at 27 symbols a character, but does at the growth its
    // dictionary really has (measured on 512 characters): it ranks, with the same count as with
    // room to spare, and says what it needs by that growth.
    {
        const auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "the", "cat", "sat", "on", "mat"}));
        const TestResources few(small, nullptr);
        set_filter_memory(kDefaultFilterMemory);
        const FilterStack roomy(text_line(2000), {{find_filter("words-v2"), {}}}, few);
        const double bound = roomy.table_bytes();
        set_filter_memory(1.2 * 1024 * 1024);
        const FilterStack tight(text_line(2000), {{find_filter("words-v2"), {}}}, few);
        CHECK(bound > filter_memory() && tight.table_bytes() < filter_memory());
        CHECK(tight.ranker() && roomy.ranker() && tight.ranker()->count() == roomy.ranker()->count());
        set_filter_memory(kDefaultFilterMemory);
    }
    // A built-in word filter's rows (clean-v1): ranks above, judges below.
    {
        set_filter_memory(kDefaultFilterMemory);
        const double m1 = FilterStack(text_line(3000), {{find_filter("clean-v1"), {}}}, none).table_bytes();
        CHECK(m1 > 0);
        set_filter_memory(m1 * 1.01);
        CHECK(FilterStack(text_line(3000), {{find_filter("clean-v1"), {}}}, none).ranker() != nullptr);
        set_filter_memory(m1 * 0.99);
        CHECK(FilterStack(text_line(3000), {{find_filter("clean-v1"), {}}}, none).ranker() == nullptr);
    }
    // The plugins merged the same in either order (smallest first, and kept): the same survivors in
    // the same order.
    {
        set_filter_memory(kDefaultFilterMemory);
        const FilterStack ab(text_line(40), {{&cs, {}}, {&ps, {}}}, none), ba(text_line(40), {{&ps, {}}, {&cs, {}}}, none);
        CHECK(ab.ranker() && ba.ranker() && ab.ranker()->count() == ba.ranker()->count());
        BigUint k = ab.ranker()->count();
        k.divmod_small(7);
        CHECK(ab.ranker()->unrank(k) == ba.ranker()->unrank(k));
        // One table for both, built once: the same plugins at the same length, in either order.
        CHECK(ab.ranker() == ba.ranker());
        // The merge cache's share of the filter memory: kept within 0..1, and with none kept the
        // same count.
        CHECK(merge_cache_share() == kDefaultMergeCacheShare);
        set_merge_cache_share(2.0);
        CHECK(merge_cache_share() == 1.0);
        set_merge_cache_share(-1.0);
        CHECK(merge_cache_share() == 0.0);
        const FilterStack uncached(text_line(40), {{&cs, {}}, {&ps, {}}}, none);
        CHECK(uncached.ranker() && uncached.ranker()->count() == ab.ranker()->count());
        // ...and a table still in use is shared even with nothing kept between counts.
        CHECK(uncached.ranker() == ab.ranker());
        // A shorter stack of the same plugins is served by that longer table, with nothing built,
        // and counts and ranks exactly as a table of its own would.
        const FilterStack shorter(text_line(25), {{&cs, {}}, {&ps, {}}}, none);
        const auto* view = dynamic_cast<const DfaRanker*>(shorter.ranker());
        // (The longest table of them in use or kept serves: here the 2,000-character one above.)
        const auto* longer = dynamic_cast<const DfaRanker*>(ab.ranker());
        CHECK(view && longer && view->length() == 25 && view->table_length() == longer->table_length() && longer->table_length() >= 40);
        set_merge_cache_share(kDefaultMergeCacheShare);
    }
    // not-written-v1's automaton walks as many states as the filter memory holds: refused in 1 MB,
    // made again (not the refusal kept) once there is more. Rules are kept for the process and the
    // suite runs twice (portable and hardware SHA-256), so each run asks for readings not asked
    // for before; a rule that was made is kept whatever the memory later is.
    {
        static int run = 0;
        const Alphabet& a = alphabet_of("ascii96");
        const uint32_t mask = written_mask_of("hex") | written_mask_of(run++ == 0 ? "base64" : "base32");
        // (Each on its own line: the two sides of one comparison may be worked out in either order.)
        set_filter_memory(1024 * 1024);
        const size_t one_mb = written_max_states(a.size());
        set_filter_memory(2.0 * 1024 * 1024);
        const size_t two_mb = written_max_states(a.size());
        CHECK(one_mb * 2 <= two_mb);
        set_filter_memory(1024 * 1024);
        const auto small = written_rule(a, mask);
        CHECK(written_blocker(*small).find("the filter memory (1 MB)") != std::string::npos);
        set_filter_memory(kDefaultFilterMemory);
        const auto big = written_rule(a, mask);
        CHECK(written_blocker(*big).empty() && big != small && written_rule(a, mask) == big);
    }
    // canonical-mesh-v1 with every-vertex-used-v1 ranks while its table (2^V rows) fits: 13
    // vertices in the default, not in less than its table.
    {
        set_filter_memory(kDefaultFilterMemory);
        const ModelSpace space(13, 20, 16);
        const std::vector<FilterStack::Entry> both{{find_filter("canonical-mesh-v1"), {}}, {find_filter("every-vertex-used-v1"), {}}};
        const ModelSieve fits(space, both);
        CHECK(fits.can_rank() && fits.table_bytes() > 8.0 * 1024 * 1024);
        BigUint k = fits.count();
        k.divmod_small(3);
        CHECK(fits.index_of(fits.model_at(k, AddressMode::Positional), AddressMode::Positional) == k);
        set_filter_memory(fits.table_bytes() * 0.99);
        const ModelSieve over(space, both);
        CHECK(!over.can_rank() && over.blocker().find("the filter memory") != std::string::npos);
    }
    set_filter_memory(before);
    CHECK(memory_text(512.0 * 1024 * 1024) == "512 MB" && memory_text(2.5 * 1024 * 1024 * 1024) == "2.5 GB");
}

// Variable length addressing (corridor.hpp shortest_path): on small loops, against every bearing
// of up to the given places and every walk either way round, worked out with plain integers; and
// the path it gives, followed as the navigator would, leads to the unit.
void test_shortest_path()
{
    auto hexlen = [](uint64_t x) { size_t n = 1; while (x >>= 4) ++n; return n; };
    auto bearing_len = [](uint64_t a, int d) {
        size_t n = std::to_string(a).size();
        return d == 0 ? n : std::max<size_t>(n, size_t(d) + 1) + 1;
    };
    int bad = 0, n = 0;
    for (uint64_t units : {1ull, 2ull, 7ull, 50ull, 128ull, 360ull, 1000ull, 19683ull})
        for (int maxd : {0, 1, 2})
        {
            if (units > 1000 && maxd > 1) continue;
            uint64_t scale0 = 360;
            std::vector<size_t> best(units, SIZE_MAX);
            for (uint64_t v = 0; v < units; ++v) best[v] = hexlen(v);
            for (int d = 0; d <= maxd; ++d, scale0 *= 10)
                for (uint64_t a = 0; a < scale0; ++a)
                {
                    uint64_t lands = (a * units + scale0 - 1) / scale0;
                    if (lands >= units) lands = 0;
                    const size_t len = bearing_len(a, d);
                    for (uint64_t v = 0; v < units; ++v)
                    {
                        const uint64_t f = (v + units - lands) % units, b = (lands + units - v) % units;
                        const size_t c = len + (f == 0 ? 0 : 1 + std::min(hexlen(f), hexlen(b)));
                        best[v] = std::min(best[v], c);
                    }
                }
            for (uint64_t v = 0; v < units; ++v)
            {
                const ShortestPath p = shortest_path(BigUint(v), BigUint(units), maxd);
                bool ok = p.chars == best[v] && p.chars == p.written.size();
                if (p.by_bearing)
                {
                    // Followed: the bearing typed, then the walk, round the loop.
                    const size_t dot = p.bearing.find('.');
                    const std::string digits = dot == std::string::npos ? p.bearing : p.bearing.substr(0, dot) + p.bearing.substr(dot + 1);
                    const uint64_t lands = std::stoull(unit_at_bearing(BigUint::from_decimal(digits), BigUint(units), p.decimals).to_decimal());
                    const uint64_t w = p.walk.is_zero() ? 0 : std::stoull(p.walk.to_decimal()) % units;
                    ok = ok && (p.back ? (lands + units - w) % units : (lands + w) % units) == v;
                }
                else ok = ok && BigUint::from_hex(p.written) == BigUint(v);
                ++n;
                if (!ok && ++bad <= 5)
                    std::cerr << "  shortest path: units " << units << " v " << v << " maxd " << maxd << " got " << p.written << " (" << p.chars << "), best " << best[v] << "\n";
            }
        }
    CHECK(bad == 0);
    std::cout << "shortest paths checked against every bearing: " << n << "\n";
    // On a long loop the far bearings are estimated and skipped: never a different answer.
    {
        std::mt19937_64 rng(31);
        int differ = 0;
        for (int i = 0; i < 40; ++i)
        {
            std::vector<uint8_t> ub(700 + i), vb(700 + i);
            for (auto& x : ub) x = uint8_t(rng());
            for (auto& x : vb) x = uint8_t(rng());
            BigUint units = BigUint::from_bytes(ub), v = BigUint::from_bytes(vb);
            units.add_small(1);
            // Some near the start, the end, and exact bearings, where the routes are short.
            if (i % 4 == 1) v >>= 8 * (650 + i);
            else if (i % 4 == 2) { v = units; v -= BigUint(uint64_t(rng() % 5000) + 1); }
            else if (i % 4 == 3) v = unit_at_bearing(BigUint(uint64_t(rng() % 3600000)), units, 4);
            if (!(v < units)) v = BigUint();
            const ShortestPath a = shortest_path(v, units, 20, true), b = shortest_path(v, units, 20, false);
            differ += a.written == b.written && a.chars == b.chars ? 0 : 1;
        }
        CHECK(differ == 0);
    }
    // An exact landing: the unit a bearing names is that bearing alone.
    const BigUint big = BigUint::pow(27, 32);
    const BigUint at = unit_at_bearing(BigUint(9010), big, 2); // 90.10 degrees
    const ShortestPath p = shortest_path(at, big, 20);
    CHECK(p.by_bearing && p.walk.is_zero() && p.written == "90.1");
    // A unit beside it: the bearing, and a step.
    BigUint next = at;
    next.add_small(1);
    CHECK(shortest_path(next, big, 20).written == "90.1+1");
    // bearing_of is the compass's reading.
    CHECK(bearing_of(at, big, 2) == "90.10");
}

// BigUint's byte conversions (a file read as one number and back), against the hex path they
// replaced, and BinarySpace::file_at_place against bytes_at.
void test_byte_conversions()
{
    CHECK(BigUint::from_bytes(std::vector<uint8_t>{}).is_zero());
    CHECK(BigUint::from_bytes(std::vector<uint8_t>{0, 0, 1}) == BigUint(1));
    CHECK(BigUint(0x0102).to_bytes(4) == (std::vector<uint8_t>{0, 0, 1, 2}));
    CHECK(BigUint().to_bytes(0).empty());
    CHECK(throws([] { BigUint(0x10000).to_bytes(2); }));
    uint64_t seed = 12345;
    bool same = true;
    for (size_t n : {1u, 7u, 8u, 9u, 15u, 16u, 17u, 255u, 1000u, 4097u})
    {
        std::vector<uint8_t> bytes(n);
        for (auto& b : bytes) b = uint8_t((seed = seed * 6364136223846793005ULL + 1442695040888963407ULL) >> 56);
        if (n % 3 == 0) bytes[0] = 0; // a leading zero byte
        std::string h;
        for (uint8_t b : bytes) h += "0123456789abcdef"[b >> 4], h += "0123456789abcdef"[b & 15];
        const BigUint v = BigUint::from_bytes(bytes);
        same = same && v == BigUint::from_hex(h) && v.to_bytes(n) == bytes;
        const BinarySpace bs(n, "sieve");
        const BigUint place = bs.index_of(bytes, AddressMode::Positional);
        same = same && BinarySpace::file_at_place(place) == bytes && bs.bytes_at(place, AddressMode::Positional) == bytes;
    }
    CHECK(same);
}

void test_midi_read()
{
    std::mt19937_64 rng(1028);
    const std::vector<NoteSet> sets = {note_set_of("notes104"), make_note_set(48, 84, "seEqQhHw", 1), make_note_set(48, 84, "seEqQhHw", 3),
                                       make_note_set(36, 96, "eqhw", 2), make_note_set(60, 72, "qh", 2)};
    int n = 0;
    for (const NoteSet& set : sets)
        for (int t = 0; t < 60; ++t)
        {
            const uint32_t L = 16;
            std::vector<uint32_t> u(size_t(L) * set.voices);
            for (auto& d : u) d = rng() % 3 == 0 ? uint32_t(rng() % set.duration_count()) : uint32_t(rng() % set.base()); // rests often
            if (t == 0) std::fill(u.begin(), u.end(), 0u);                                                                   // all rests
            const std::string midi = notes_to_midi(set, u);
            std::vector<std::string> report;
            const std::string notation = midi_to_notation(std::vector<uint8_t>(midi.begin(), midi.end()), set, &report);
            const NotesCanonResult c = set.legacy ? canonicalise_notes(notation, L) : canonicalise_notes2(notation, set, L);
            const bool ok = c.units.size() == 1 && sounding(set, c.units[0]) == sounding(set, u) && report.empty() && c.octave_shifted == 0 &&
                            c.durations_changed == 0;
            CHECK(ok);
            if (!ok) std::cerr << "  midi read back differs: " << set.id() << " " << notes_to_notation(set, u) << "\n    read: " << notation << "\n";
            ++n;
        }
    // Not MIDI, or cut short: refused with a reason.
    const NoteSet set = note_set_of("notes104");
    CHECK(throws([&] { (void)midi_to_notation({'M', 'Z', 0, 0}, set); }));
    const std::string midi = notes_to_midi(set, std::vector<uint32_t>(8, 13 * 4 + 1));
    CHECK(throws([&] { (void)midi_to_notation(std::vector<uint8_t>(midi.begin(), midi.begin() + 20), set); }));
    std::cout << "MIDI read back: " << n << " melodies\n";
}

void run_all(int argc, char** argv)
{
    test_sha256();
    test_utf8();
    test_biguint();
    test_space_basics();
    test_scramble_bijection();
    test_neighbour();
    test_fraction();
    test_corridor();
    test_canon();
    test_sieve();
    test_image();
    test_audio();
    test_midi_read();
    test_model();
    test_guided();
    test_modelspace();
    if (argc > 1)
    {
        const std::string dir = std::string(argv[1]) + "/";
        test_vectors(dir + "vectors_v1.tsv");
        test_biguint_vectors(dir + "vectors_biguint_v1.tsv");
        test_biguint_large_vectors(dir + "vectors_biguint_large_v1.tsv");
        test_digit_vectors(dir + "vectors_digits_v1.tsv");
        test_canon_vectors(dir + "vectors_canon.tsv");
        test_bytes_vectors(dir + "vectors_bytes_v1.tsv");
        test_alphabet_vectors(dir + "vectors_alphabets_v1.tsv");
        test_model_vectors(dir + "vectors_models_v1.tsv");
        test_image_vectors(dir + "vectors_image_v1.tsv");
        test_guided_vectors(dir);
        test_filters(dir);
        test_filter_vectors(dir);
        test_compact(dir);
        test_compact_vectors(dir);
        test_bookspace();
        test_booksieve();
        test_review_additions();
        test_book_vectors(dir);
        test_composition_vectors(dir);
        test_composition_sieve();
        test_world_vectors(dir);
        test_world_sieve();
        test_titled_vectors(dir + "vectors_titled_v1.tsv");
        test_binary_vectors(dir + "vectors_binary_v1.tsv");
        test_chunk_vectors(dir + "vectors_chunks_v1.tsv");
        test_notes2_vectors(dir + "vectors_notes2_v1.tsv");
        test_pcm_vectors(dir + "vectors_pcm_v1.tsv");
        test_pcm_digits();
        test_notes3_vectors(dir + "vectors_notes3_v1.tsv");
        test_sound_vectors(dir);
        test_notes2(dir);
        test_plugins(dir);
        test_book_filter_vectors(dir);
        test_kinds();
        test_kind_vectors(dir + "vectors_kinds_v1.tsv");
        test_written_vectors(dir);
        test_cross_vectors(dir);
        test_picture_vectors(dir);
        test_utf8_vectors(dir);
        test_utf8_joint();
        test_shortest_path();
        test_filter_memory(dir);
        test_byte_conversions();
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: sieve_tests <tests directory>\n"; return 1; }

    // Everything runs on the portable SHA-256 path, then again on SHA-NI where the CPU has it.
    Sha256::use_hardware(false);
    run_all(argc, argv);
    std::cout << "portable SHA-256: " << g_checks << " checks, " << g_failures << " failures\n";
    if (Sha256::hardware_available())
    {
        const int before = g_checks;
        Sha256::use_hardware(true);
        run_all(argc, argv);
        std::cout << "hardware SHA-256: " << g_checks - before << " checks, " << g_failures << " failures\n";
    }
    else std::cout << "hardware SHA-256: not available on this CPU\n";

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures ? 1 : 0;
}
