// Sieve core tests. Usage: sieve_tests <tests directory containing the vectors_*.tsv files>

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/biguint.hpp"
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
    CHECK(filters_for(text_line(8)).size() == 8);
    const FilterLine image{"image", "image/mono/10x10", 2, 100, nullptr, 10, 10, 1};
    CHECK(filters_for(image).size() == 2); // symbol-entropy, neighbour-agreement

    // Rankers, exhaustively at small lengths with a small dictionary: rank = position among the
    // survivors in address order, and unrank inverts it.
    auto small = std::make_shared<const Dictionary>(Dictionary::from_words({"a", "an", "ant", "i", "in", "tan"}));
    TestResources small_res(small, nullptr);
    for (const char* name : {"clean-v1", "words-v1", "clean-v2", "words-v2"})
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
    if (argc > 1)
    {
        const std::string dir = std::string(argv[1]) + "/";
        test_vectors(dir + "vectors_v1.tsv");
        test_digit_vectors(dir + "vectors_digits_v1.tsv");
        test_canon_vectors(dir + "vectors_canon.tsv");
        test_image_vectors(dir + "vectors_image_v1.tsv");
        test_guided_vectors(dir);
        test_filters(dir);
        test_filter_vectors(dir);
        test_compact(dir);
        test_compact_vectors(dir);
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
