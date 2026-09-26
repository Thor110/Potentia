// Sieve core tests. Usage: sieve_tests <tests directory containing the vectors_*.tsv files>

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/biguint.hpp"
#include "sieve/booksieve.hpp"
#include "sieve/bookspace.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/canon.hpp"
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
#include "sieve/utf8.hpp"

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
    CHECK(filters_for(text_line(8)).size() == 10);
    const FilterLine image{"image", "image/mono/10x10", 2, 100, nullptr, 10, 10, 1};
    CHECK(filters_for(image).size() == 2); // symbol-entropy, neighbour-agreement

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
    // Black-and-white entropy at its limit: the fast walk agrees with the generic one, both ways.
    {
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
        const FilterLine img2{"image", "image/mono/2049x1", 2, 2049, nullptr, 2049, 1, 1};
        CHECK(FilterStack(img2, {{find_filter("symbol-entropy-v1"), {}}}, none).ranker() == nullptr);
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
        const FilterStack pages(text_line(4), {{find_filter("max-run-v1"), {}}}, res);
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
    test_model();
    test_guided();
    test_modelspace();
    if (argc > 1)
    {
        const std::string dir = std::string(argv[1]) + "/";
        test_vectors(dir + "vectors_v1.tsv");
        test_biguint_vectors(dir + "vectors_biguint_v1.tsv");
        test_digit_vectors(dir + "vectors_digits_v1.tsv");
        test_canon_vectors(dir + "vectors_canon.tsv");
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
        test_book_filter_vectors(dir);
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
