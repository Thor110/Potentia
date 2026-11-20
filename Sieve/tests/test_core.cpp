// Sieve core tests. Usage: sieve_tests <tests directory containing the vectors_*.tsv files>

#include "sieve/cipher.hpp"
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
#include "sieve/character.hpp"
#include "sieve/character_voxels.hpp"
#include "sieve/character_skin.hpp"
#include "sieve/character_edit.hpp"
#include "sieve/key_registry.hpp"
#include "sieve/footer_wrap.hpp"
#include "sieve/favourites.hpp"
#include "sieve/importers.hpp"
#include "sieve/dimensions.hpp"
#include "sieve/animation.hpp"
#include "sieve/raid.hpp"
#include "sieve/raid_net.hpp"
#include "sieve/raid_piece.hpp"
#include "sieve/fractal.hpp"
#include "sieve/fractal_count.hpp"
#include "sieve/fractal_space.hpp"
#include "sieve/seal.hpp"
#include "sieve/catalogue.hpp"
#include "sieve/format_stats.hpp"
#include "sieve/save.hpp"
#include "sieve/actions.hpp"
#include "sieve/filekind.hpp"
#include "sieve/json.hpp"
#include "sieve/safetensors.hpp"
#include "sieve/weights.hpp"
#include "sieve/llm.hpp"
#include "sieve/aispace.hpp"
#include "sieve/digit_histogram.hpp"
#include "sieve/rope_wave.hpp"
#include "sieve/mix.hpp"
#include "sieve/training.hpp"
#include "sieve/diffusion.hpp"
#include "sieve/llm_tokenizer.hpp"
#include "sieve/unicode_classes.hpp"
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
#include "sieve/jpeg_dc.hpp"
#include "sieve/sha256.hpp"
#include "sieve/sieve.hpp"
#include "sieve/space.hpp"
#include "sieve/rangestack.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/titledspace.hpp"
#include "sieve/utf8.hpp"
#include "sieve/varint.hpp"

#include <algorithm>
#include <atomic>
#include <bitset>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <filesystem>
#include <map>
#include <memory>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
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
        // gcd (Lehmer's, used by Manual Navigation's exact check): the same as Euclid's by long
        // division, for numbers sharing a factor of many sizes, edge limbs, 0 and equal operands.
        auto euclid = [](BigUint a, BigUint b) {
            while (!b.is_zero())
            {
                BigUint t = BigUint::mod(a, b);
                a = std::move(b);
                b = std::move(t);
            }
            return a;
        };
        bool same = true;
        for (int t = 0; t < 600; ++t)
        {
            const bool edge = t % 2;
            const BigUint f = rnd(1 + rng() % 6, edge);
            const BigUint a = BigUint::mul(rnd(1 + rng() % 40, edge), f), b = BigUint::mul(rnd(1 + rng() % 40, edge), f);
            same = same && BigUint::gcd(a, b) == euclid(a, b) && BigUint::gcd(b, a) == euclid(a, b);
        }
        same = same && BigUint::gcd(BigUint(), BigUint()).is_zero() && BigUint::gcd(x, BigUint()) == x && BigUint::gcd(x, x) == x;
        same = same && BigUint::gcd(BigUint::pow(6, 300), BigUint::pow(10, 200)) == BigUint::pow(2, 200);
        CHECK(same);
        // Stopped when asked: false, the answer untouched.
        BigUint g(5);
        CHECK(!BigUint::gcd(BigUint::pow(3, 5000), BigUint::pow(7, 3000), g, [](size_t) { return false; }) && g == BigUint(5));
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

// A space of a range of lengths (list O1): every unit of min to max symbols, the shorter first. Every unit of small
// ranges walked both ways in both orders (the scrambled order a permutation of the places), its place the units
// shorter than it plus its digits' value, steps round the loop and the fraction; min = max is the fixed space.
void test_space_range(const std::string& dir)
{
    const Alphabet& a = alphabet_by_id("lower27");
    const Space r(a, 1, 3, "sieve");
    CHECK(r.is_range() && r.min_length() == 1 && r.unit_length() == 3 && r.address_length() == 4);
    CHECK(r.size() == BigUint(27 + 729 + 19683));
    CHECK(r.id() == "lower27/L1-3/key=sieve/shuffle-sha256-v1");
    CHECK(r.count_shorter(1).is_zero() && r.count_shorter(2) == BigUint(27) && r.count_shorter(3) == BigUint(27 + 729));
    CHECK(r.address_of(r.digits_of(U"a"), AddressMode::Positional) == "0001");
    CHECK(r.address_of(r.digits_of(U"  "), AddressMode::Positional) == "001b"); // the first of two letters: 27
    CHECK(r.text_of(r.unit_at("001b", AddressMode::Positional)) == U"  ");
    CHECK(throws([&] { r.unit_at(r.size().to_hex(), AddressMode::Positional); })); // just past the line
    CHECK(throws([&] { r.digits_of(U"abcd"); }) && throws([&] { r.digits_of(U""); }));
    // every unit, both ways, both orders
    bool ok = true;
    std::vector<char> hit(size_t(27 + 729 + 19683), 0);
    for (uint64_t p = 0; ok && p < 27 + 729 + 19683; ++p)
    {
        const auto u = r.unit_at_place(BigUint(p));
        ok = r.place_of(u) == BigUint(p) && u.size() == (p < 27 ? 1u : p < 27 + 729 ? 2u : 3u);
        const auto pos = r.address_digits(u, AddressMode::Positional);
        ok = ok && BigUint::from_digits(pos, 27) == BigUint(p) && r.unit_of_address(pos, AddressMode::Positional) == u;
        const auto scr = r.address_digits(u, AddressMode::Scrambled);
        const BigUint s = BigUint::from_digits(scr, 27);
        ok = ok && s < r.size() && r.unit_of_address(scr, AddressMode::Scrambled) == u;
        if (ok)
        {
            const uint64_t i = std::stoull(s.to_decimal());
            ok = !hit[i];
            hit[i] = 1;
        }
        // a step of one, and back
        ok = ok && r.neighbour(r.neighbour(u, AddressMode::Positional, 1), AddressMode::Positional, -1) == u;
    }
    CHECK(ok);
    // round the loop: the last unit's next is the first; a step of the whole size comes back
    BigUint top = r.size();
    top -= BigUint(1);
    const auto last = r.unit_at_place(top);
    CHECK(r.neighbour(last, AddressMode::Positional, 1) == r.unit_at_place(BigUint()));
    CHECK(r.neighbour(r.digits_of(U"ab"), AddressMode::Scrambled, int64_t(27 + 729 + 19683)) == r.digits_of(U"ab"));
    CHECK(r.neighbour(r.digits_of(U"a"), AddressMode::Positional, -2) == last);
    // the fraction, and a unit at a fraction
    CHECK(r.fraction(r.unit_at_place(BigUint()), AddressMode::Positional) == 0.0);
    CHECK(unit_at_fraction(r, 5, 1, AddressMode::Positional) == r.unit_at_place(BigUint((27 + 729 + 19683) / 2)));
    // min = max: the fixed space, address for address
    const Space one(a, 3, 3, "sieve"), fixed(a, 3, "sieve");
    CHECK(!one.is_range() && one.id() == fixed.id() && one.size() == fixed.size());
    for (const char32_t* t : {U"abc", U"   ", U"zz "})
        for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
            CHECK(one.address_of(one.digits_of(t), m) == fixed.address_of(fixed.digits_of(t), m));
    // the oracle's vectors (reference/sieve_ref.py range-vectors): places and both orders, both ways
    {
        int n = 0;
        bool agree = true;
        std::ifstream in(dir + "vectors_range_v1.tsv");
        CHECK(static_cast<bool>(in));
        for (std::string line; std::getline(in, line);)
        {
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> f;
            size_t start = 0;
            for (size_t i = 0; i <= line.size(); ++i)
                if (i == line.size() || line[i] == '\t') f.push_back(line.substr(start, i - start)), start = i + 1;
            if (f.size() != 7) continue;
            const Space sp(alphabet_by_id(f[0]), uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), f[3]);
            const std::u32string unit = utf8_decode(f[4]);
            const auto d = sp.digits_of(unit);
            agree = agree && sp.address_of(d, AddressMode::Positional) == f[5] && sp.address_of(d, AddressMode::Scrambled) == f[6] &&
                    sp.text_of(sp.unit_at(f[5], AddressMode::Positional)) == unit && sp.text_of(sp.unit_at(f[6], AddressMode::Scrambled)) == unit;
            ++n;
        }
        CHECK(agree && n >= 80);
    }
    // a copy is the same space (its shuffle with it)
    const Space copy = r;
    CHECK(copy.address_of(copy.digits_of(U"ab"), AddressMode::Scrambled) == r.address_of(r.digits_of(U"ab"), AddressMode::Scrambled));
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

    // An alphabet with no space (dna4, IDEAS 19.162): capitals fold to the bases, and whatever it cannot
    // hold is dropped, there being no space to separate with; padding is digit 0 (a). The reference
    // oracle gives the same units (reference/sieve_ref.py).
    {
        const Alphabet& dna = alphabet_by_id("dna4");
        CHECK(dna.size() == 4 && dna.symbol(0) == U'a' && dna.symbol(3) == U't' && !dna.contains(U' '));
        const auto d = canonicalise_text("GATTACA, gattaca!", dna, 12);
        CHECK(d.units.size() == 2 && d.units[0] == U"gattacagatta" && d.units[1] == U"caaaaaaaaaaa");
        CHECK(d.dropped == 3 && d.separated == 0 && d.padding == 10 && d.case_folded == 7);
        // A block on its own (no space) can now be read onto, where before it could not.
        const auto g = canonicalise_text("\u03b1\u03b2 \u03b3", alphabet_of("greek"), 4);
        CHECK(g.units.size() == 1 && g.units[0].substr(0, 3) == U"\u03b1\u03b2\u03b3");
    }

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

// Canonicalisation on a range line (list O2, reference/sieve_ref.py canon-range-vectors): the last unit keeps its own
// length, padded only up to the shortest; and a warp on a range space gives the greeting its own 20 symbols.
void test_canon_range_vectors(const std::string& path)
{
    int n = 0;
    for (const auto& f : read_tsv(path, 6))
    {
        const auto r = canonicalise_text(from_hex(f[4]), alphabet_by_id(f[1]), uint32_t(std::stoul(f[3])), canon_version_from_string(f[0]),
                                         uint32_t(std::stoul(f[2])));
        std::string got;
        for (size_t i = 0; i < r.units.size(); ++i)
        {
            if (i) got += ",";
            for (unsigned char c : utf8_encode(r.units[i]))
            {
                char b[3];
                std::snprintf(b, sizeof b, "%02x", c);
                got += b;
            }
        }
        CHECK(got == f[5]);
        ++n;
    }
    CHECK(n == 18);
    const auto g = canonicalise_text("welcome to the sieve", alphabet_by_id("lower27"), 32, CanonVersion::V2, 1);
    CHECK(g.units.size() == 1 && g.units[0] == U"welcome to the sieve" && g.padding == 0);
    const auto b = canonicalise_bytes("abc", alphabet_by_id("bytes256"), 8, 1);
    CHECK(b.units.size() == 1 && b.units[0].size() == 3 && b.padding == 0);
}

void test_image_vectors(const std::string& path, ImageFormat::Fit fit = ImageFormat::Fit::Average, int expected = 36)
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
        fmt.fit = fit;
        const bool ok = canonicalise_frame(img, fmt) == split_u32(f[6]);
        CHECK(ok);
        if (!ok) std::cerr << "  image mismatch: " << f[0] << " " << f[1] << "x" << f[2] << " -> " << f[3] << "x" << f[4] << "\n";
        ++n;
    }
    std::cout << "image vectors checked (" << (fit == ImageFormat::Fit::Levels ? kImageCanonVersion2 : kImageCanonVersion) << "): " << n << "\n";
    CHECK(n == expected); // a truncated or emptied vector file must fail, not pass quietly
    CHECK(n > 30);
}

// cipher-v1 (SPECIFICATIONS §12.0i): every recipe applied as the oracle applies it, undone again,
// and the weak ciphers broken to the oracle's guesses, rank for rank.
void test_cipher_vectors(const std::string& path)
{
    using namespace sieve::cipher;
    auto bytes = [](const std::string& h) {
        std::vector<uint8_t> b;
        for (size_t i = 0; i + 1 < h.size(); i += 2) b.push_back(uint8_t(std::stoi(h.substr(i, 2), nullptr, 16)));
        return b;
    };
    auto hex = [](const std::vector<uint8_t>& b) {
        static const char k[] = "0123456789abcdef";
        std::string s;
        for (const uint8_t c : b) s += {k[c >> 4], k[c & 15]};
        return s;
    };
    int enc = 0, brk = 0;
    std::map<std::string, std::vector<std::vector<std::string>>> breaks; // cipher hex -> its guesses, in order
    std::vector<std::string> order;
    std::ifstream in(path);
    CHECK(bool(in));
    std::string line;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
        if (f[0] == "encrypt" && f.size() >= 3)
        {
            const Recipe r = Recipe::parse(f[1]);
            const std::vector<uint8_t> plain = bytes(f[2]), want = bytes(f.size() > 3 ? f[3] : "");
            const std::vector<uint8_t> got = encrypt(r, plain);
            CHECK(got == want);
            CHECK(decrypt(r, got) == plain);
            if (got != want) std::cerr << "  cipher mismatch: " << f[1] << " on " << f[2].substr(0, 16) << "\n";
            ++enc;
        }
        else if (f[0] == "break" && f.size() == 6)
        {
            if (!breaks.count(f[1])) order.push_back(f[1]);
            breaks[f[1]].push_back(f);
        }
    }
    for (const std::string& c : order)
    {
        const auto& want = breaks[c];
        const std::vector<Guess> got = break_weak(bytes(c), 16, want.size());
        CHECK(got.size() == want.size());
        for (size_t i = 0; i < got.size() && i < want.size(); ++i)
        {
            const bool ok = std::to_string(got[i].rank) == want[i][2] && std::to_string(got[i].score) == want[i][3] &&
                            got[i].recipe.layers.front().text() == want[i][4] && hex(got[i].plain) == want[i][5];
            CHECK(ok);
            if (!ok) std::cerr << "  break mismatch: guess " << i + 1 << " " << got[i].recipe.layers.front().text() << " vs " << want[i][4] << "\n";
            ++brk;
        }
    }
    std::cout << "cipher vectors checked: " << enc << " recipes, " << brk << " guesses\n";
    CHECK(enc == 55 && brk == 15); // a truncated or emptied vector file must fail, not pass quietly
    // A recipe reads back as itself, and bad ones say what is wrong.
    const Recipe r = Recipe::parse("sieve-cipher-v1\nreverse\ncaesar 7\nxor 0a0b\nvigenere-hex 00ff\nfeistel key;vigenere lemon");
    CHECK(Recipe::parse(r.text()).text() == r.text());
    CHECK(r.layers.size() == 6 && r.layers[3].text() == "vigenere-hex 00ff");
    for (const char* badr : {"", "caesar 256", "caesar", "xor 5", "xor zz", "rot13", "reverse x", "vigenere"})
    {
        bool threw = false;
        try { Recipe::parse(badr); } catch (const std::invalid_argument&) { threw = true; }
        CHECK(threw);
    }
    // The catalogue: one-byte keys give 257 + 3 * 256 = 1,025 layers; sixteen-byte keys the
    // number sieve cipher --catalogue prints (CI step 3), and depth d is layers^d.
    const Catalogue one = catalogue(1, 2);
    CHECK(one.layers == "1025" && one.layer_bits == 11 && one.depths.size() == 2 && one.depths[1].recipes == "1050625");
    CHECK(one.total_bits == 21); // 1,025 + 1,050,625 = 1,051,650 < 2^21
    const Catalogue c16 = catalogue(16, 2);
    CHECK(c16.layers == "1024850422726591137019104700029796025345" && c16.layer_bits == 130 && c16.depths[1].bits == 260);
    CHECK(short_decimal(c16.depths[1].recipes).find("... (") != std::string::npos && short_decimal("1025") == "1025");
}

// The system prompt sieve-system-v1 (docs/SYSTEM-PROMPT.md) is never edited: its bytes are the
// published ones, and a change is a new version.
void test_system_prompt(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    CHECK(bool(in));
    std::ostringstream all;
    all << in.rdbuf();
    const std::string text = all.str();
    CHECK(text.size() == 3370);
    CHECK(Sha256::hex(Sha256::hash(text)) == "04bd46c89c16f86fbccffac8055636b03df1d1b7de2270a343bd20ddd74239a9");
    std::cout << "system prompt checked: sieve-system-v1, " << text.size() << " bytes\n";
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

    // Faint ink on paper: v1 averages it to white, v2's levels keep the ink.
    {
        RgbaImage paper{40, 40, {}};
        for (uint32_t y = 0; y < 40; ++y)
            for (uint32_t x = 0; x < 40; ++x)
            {
                const bool ink = x % 10 < 2 && y < 20; // thin strokes in the top half
                const uint8_t v = ink ? 60 : 230;
                paper.rgba.insert(paper.rgba.end(), {v, v, uint8_t(v - 20), 255});
            }
        ImageFormat f4{4, 4, 1, &mono};
        f4.fit = ImageFormat::Fit::Average;
        CHECK(canonicalise_frame(paper, f4) == std::vector<uint32_t>(16, 1));
        f4.fit = ImageFormat::Fit::Levels;
        CHECK((canonicalise_frame(paper, f4) == std::vector<uint32_t>{0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1}));
        CHECK(std::string(f4.canon_version()) == "canon-image-v2");
    }

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
    // The least and most any symbol is given in any table (the metrics list's bound on a guided
    // address): 1, as every table gives every symbol, and the padding table's 2^16 - 26, as the
    // oracle's tables give them.
    CHECK(model->frequency_range() == std::make_pair(uint32_t(1), uint32_t(65510)));
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
        const FilterSpec t1_spec = plugin_spec(t1), t2_spec = plugin_spec(t2); // clean_spec: clean-data-v1, above
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

// List Z: the filters added on 17 November 2026: the pages' few-values, two-symbol-run and golden-word
// (Z7), the sound line's sound-crossings and sound-bend (Z6), the video line's video-moves and video-smooth
// (Z5), and the fractals in the pages and audio lines, fractal-written and fractal-scored (Z4).
extern std::string g_data; // (the data folder: defined below)
// List M2: the sound filters (envelope, syllables, interval) and the melody filters (cadence, contrary) against their
// rules written plainly; the envelope's and the syllables' counts block by block against their automata's and against
// every unit of a small set.
void test_m2_filters(const std::string& dir)
{
    static const TestResources none(nullptr, nullptr);
    // the rules, plainly: a sample's loudness in quarters of full scale, and the blocks'
    auto loud = [](const PcmFormat& f, uint32_t d) {
        const int64_t v = pcm_sample(f, d), m = v < 0 ? -v : v, half = int64_t(1) << (f.bits - 1);
        return m == 0 ? 0 : int(std::min<int64_t>(4, (4 * m + half - 1) / half));
    };
    auto blocks = [&](const PcmFormat& f, const std::vector<uint32_t>& u, uint32_t k) {
        std::vector<int> out;
        for (size_t i = 0; i < u.size(); i += k)
        {
            int m = 0;
            for (size_t j = i; j < std::min(u.size(), i + k); ++j) m = std::max(m, loud(f, u[j]));
            out.push_back(m);
        }
        return out;
    };
    auto envelope_rule = [&](const PcmFormat& f, const std::vector<uint32_t>& u, uint32_t k) {
        const auto b = blocks(f, u, k);
        size_t i = 1;
        while (i < b.size() && b[i] >= b[i - 1]) ++i;
        const int peak = b[i - 1];
        for (; i < b.size(); ++i)
            if (b[i] > b[i - 1]) return false;
        return peak >= 2 && b.back() + 2 <= peak;
    };
    auto syllables_rule = [&](const PcmFormat& f, const std::vector<uint32_t>& u, uint32_t k, int64_t threshold, uint64_t lo, uint64_t hi) {
        uint64_t n = 0;
        bool was = false;
        for (size_t i = 0; i < u.size(); i += k)
        {
            bool now = false;
            for (size_t j = i; j < std::min(u.size(), i + k); ++j)
            {
                const int64_t v = pcm_sample(f, u[j]);
                now = now || (v < 0 ? -v : v) >= threshold;
            }
            n += now && !was;
            was = now;
        }
        return n >= lo && n <= hi;
    };
    auto pcm_line = [](const PcmFormat& f, uint32_t L) {
        FilterLine l;
        l.kind = "audio", l.symbols_id = f.id(), l.base = f.base(), l.length = L;
        return l;
    };
    std::mt19937_64 rng(2245);
    // Every unit of one-bit sound, 20 samples (a block of 16 and one of 4): judged both ways, and counted.
    {
        const PcmFormat f = make_pcm_format(8, 1, 1);
        const FilterLine l = pcm_line(f, 20);
        const FilterValues ev{{"block", "16"}}, sv{{"block", "16"}, {"low", "1"}, {"high", "2"}};
        const FilterStack env(l, {{find_filter("sound-envelope-v1"), ev}}, none), syl(l, {{find_filter("sound-syllables-v1"), sv}}, none);
        uint64_t ne = 0, ns = 0;
        for (uint32_t i = 0; i < (1u << 20); ++i)
        {
            std::vector<uint32_t> u(20);
            for (uint32_t j = 0; j < 20; ++j) u[j] = (i >> j) & 1;
            const bool e = envelope_rule(f, u, 16), s2 = syllables_rule(f, u, 16, 1, 2, 5); // (1 and 2 a second over 2.5 s)
            CHECK(env.passes(u) == e && syl.passes(u) == s2);
            ne += e, ns += s2;
        }
        CHECK(env.ranker() && env.ranker()->count() == BigUint(ne));
        CHECK(syl.ranker() && syl.ranker()->count() == BigUint(ns));
    }
    // Four-bit sound, 200 samples in blocks of 16: the automaton's count and the count block by block (the filter
    // memory set too small for the automaton's table) agree; and random sounds, and notes that swell and fade, judged
    // the same by both and by the rule.
    {
        const PcmFormat f = make_pcm_format(800, 4, 1);
        const FilterLine l = pcm_line(f, 200);
        const FilterValues ev{{"block", "16"}}, sv{{"block", "16"}, {"low", "8"}, {"high", "30"}, {"threshold", "30"}};
        const FilterStack env(l, {{find_filter("sound-envelope-v1"), ev}}, none), syl(l, {{find_filter("sound-syllables-v1"), sv}}, none);
        const double was = filter_memory();
        set_filter_memory(1024);
        const FilterStack env2(l, {{find_filter("sound-envelope-v1"), ev}}, none), syl2(l, {{find_filter("sound-syllables-v1"), sv}}, none);
        // (review 4: what they count as follows what make builds, so a conflict with an automaton shows)
        const std::string env_low = find_filter("sound-envelope-v1")->counts_as_on(l, &ev), syl_low = find_filter("sound-syllables-v1")->counts_as_on(l, &sv);
        set_filter_memory(was);
        CHECK(env_low == "own" && syl_low == "own");
        CHECK(find_filter("sound-envelope-v1")->counts_as_on(l, &ev) == "automaton" && find_filter("sound-syllables-v1")->counts_as_on(l, &sv) == "automaton");
        // (since list N5-10 the counted route ranks too, block by block: past the automaton, its numbers the same)
        CHECK(env.ranker() && env2.ranker() && dynamic_cast<const DfaRanker*>(env.ranker()) && !dynamic_cast<const DfaRanker*>(env2.ranker()));
        CHECK(env.ranker() && env2.ranker() && env.ranker()->count() == env2.ranker()->count());
        CHECK(syl.ranker() && syl2.ranker() && syl.ranker()->count() == syl2.ranker()->count());
        if (env.ranker() && env2.ranker() && syl.ranker() && syl2.ranker())
            for (int k = 0; k < 40; ++k)
                for (const auto& [a, b] : {std::pair{env.ranker(), env2.ranker()}, std::pair{syl.ranker(), syl2.ranker()}})
                {
                    std::vector<uint8_t> bytes(40);
                    for (auto& x : bytes) x = uint8_t(rng());
                    const BigUint at = BigUint::mod(BigUint::from_bytes(bytes), a->count());
                    const auto u = a->unrank(at);
                    CHECK(b->unrank(at) == u && b->rank(u) == at && b->accepts(u));
                    CHECK(b->Ranker::unrank(at) == u); // (and the walk by next and completions)
                }
        int envelopes = 0;
        for (int k = 0; k < 3000; ++k)
        {
            std::vector<uint32_t> u(200);
            const double a = double(rng() % 8), period = 4 + double(rng() % 20);
            for (uint32_t i = 0; i < 200; ++i)
            {
                double v;
                if (k % 3 == 0) v = double(int64_t(rng() % 16) - 8);                            // noise
                else if (k % 3 == 1) v = a * std::sin(i * 6.283 / period) * std::exp(-double(i) / 60.0) * std::min(1.0, i / 10.0); // a struck note
                else v = (i / 25) % 2 ? 0.0 : a * std::sin(i * 6.283 / period);               // bursts, as syllables
                u[i] = pcm_digit(f, int32_t(std::lround(std::clamp(v, -8.0, 7.0))));
            }
            const bool e = envelope_rule(f, u, 16), s2 = syllables_rule(f, u, 16, 3, 2, 8); // (8 to 30 a second over a quarter of a second; 30% of 8 is 2.4, so 3)
            envelopes += e;
            CHECK(env.passes(u) == e && env2.passes(u) == e);
            CHECK(syl.passes(u) == s2 && syl2.passes(u) == s2);
        }
        CHECK(envelopes > 0);
    }
    // The interval: two tones a fifth apart pass, and a major third; one tone with its octave above does not, nor noise.
    {
        const PcmFormat f = make_pcm_format(8000, 8, 1);
        const FilterLine l = pcm_line(f, 8000);
        const FilterStack iv(l, {{find_filter("sound-interval-v1"), {}}}, none);
        auto tones = [&](double a, double b, double ga, double gb) {
            std::vector<uint32_t> u(8000);
            for (uint32_t i = 0; i < 8000; ++i)
                u[i] = pcm_digit(f, int32_t(std::lround(60 * ga * std::sin(i * 6.2831853 * a / 8000) + 60 * gb * std::sin(i * 6.2831853 * b / 8000))));
            return u;
        };
        CHECK(iv.passes(tones(440, 660, 1, 1)));
        CHECK(iv.passes(tones(440, 550, 1, 0.8)));
        CHECK(!iv.passes(tones(440, 880, 1, 0.5)));
        std::vector<uint32_t> noise(8000);
        for (auto& d : noise) d = uint32_t(rng() & 255);
        CHECK(!iv.passes(noise));
    }
    // The cadence (a plugin): a melody ending on the tonic from a step; counted against every melody of 3 notes.
    {
        const std::string filters = dir + "../data/filters/";
        const auto def = load_plugin_file(filters + "melody-cadence-v1.sfilter");
        auto notes = [](const std::string& notation) {
            return canonicalise_notes(notation, uint32_t(std::count(notation.begin(), notation.end(), ' ') + 1)).units.front();
        };
        auto audio = [](uint32_t L) { return FilterLine{"audio", kNotesSymbolsId, kNoteSymbols, L, nullptr, 0, 0, 0}; };
        auto judge = [&](const std::string& n, const FilterValues& v = {}) {
            const auto u = notes(n);
            return compile_plugin(*def, audio(uint32_t(u.size())), v, none).accepts(u);
        };
        CHECK(judge("E4q D4q C4h") && judge("G4q B4q C5h") && judge("C5q C#5q C5w") && judge("D4q C4e C4h"));
        CHECK(!judge("G4q C4q C4h"));
        CHECK(!judge("E4q C4h") && !judge("D4q Rq C4h") && !judge("C4q D4h") && !judge("E4q D4q C4h Rq"));
        CHECK(!judge("D4q C4h", {{"approach", "half"}}) && judge("B4q C5h", {{"approach", "half"}}));
        CHECK(judge("C4q B4q A4h", {{"tonic", "A"}}));
        const NoteSet n104 = note_set_of(kNotesSymbolsId);
        const Dfa d = compile_plugin(*def, audio(3), {}, none);
        uint64_t brute = 0;
        for (uint32_t a = 0; a < kNoteSymbols; ++a)
            for (uint32_t b = 0; b < kNoteSymbols; ++b)
                for (uint32_t c = 0; c < kNoteSymbols; ++c)
                {
                    // the rule: the last note C, reached by a step, or repeated after a C reached by a step
                    const uint32_t pa = n104.midi(a), pb = n104.midi(b), pc = n104.midi(c);
                    auto step = [](uint32_t x, uint32_t y) { return x != 0 && y != 0 && (x > y ? x - y : y - x) >= 1 && (x > y ? x - y : y - x) <= 2; };
                    const bool want = pc != 0 && pc % 12 == 0 && (step(pb, pc) || (pb == pc && step(pa, pb)));
                    const std::vector<uint32_t> u{a, b, c};
                    CHECK(d.accepts(u) == want);
                    brute += want;
                }
        CHECK(DfaRanker(std::make_shared<const Dfa>(d), 3).count() == BigUint(brute));
    }
    // Contrary motion, two voices on notes2 judged together, beside a filter that judges each voice.
    {
        const NoteSet set = make_note_set(48, 84, "eqhw", 2);
        FilterLine l;
        l.kind = "audio", l.symbols_id = set.id(), l.base = set.base(), l.length = 8;
        auto unit = [&](const std::string& n) { return canonicalise_notes2(n, set, 4).units.front(); };
        const FilterStack con(l, {{find_filter("melody-contrary-v1"), {}}}, none);
        CHECK(con.passes(unit("C4q D4q E4q F4q // G4q F4q E4q D4q")));
        CHECK(!con.passes(unit("C4q D4q E4q F4q // E4q F4q G4q A4q")));  // in parallel
        CHECK(!con.passes(unit("C4q D4q E4q F4q // G4w")));             // one voice holds: no joint moves
        CHECK(con.passes(unit("C4q D4q E4q F4q // G4q Rq E4q D4q")));    // a rest: E4 is from G4, down, against E4 up
        // beside melody-motif, which judges each voice on its own: the stack passes where both do
        const FilterValues mv{{"notes", "2"}, {"times", "2"}};
        const FilterStack motif(l, {{find_filter("melody-motif-v1"), mv}}, none);
        const FilterStack mixed(l, {{find_filter("melody-contrary-v1"), {}}, {find_filter("melody-motif-v1"), mv}}, none);
        for (const char* n : {"C4q D4q E4q F4q // G4q F4q E4q D4q", "C4q D4q C4q D4q // G4q F4q G4q F4q", "C4q D4q E4q F4q // E4q F4q G4q A4q"})
        {
            const auto u = unit(n);
            CHECK(mixed.passes(u) == (con.passes(u) && motif.passes(u)));
        }
        CHECK(mixed.passes(unit("C4q D4q C4q D4q // G4q F4q G4q F4q")));
        CHECK(find_filter("melody-contrary-v1")->whole_unit && find_filter("melody-contrary-v1")->judge_only);
    }
    // Counted (list N5-11): a walk over both voices at once, in time, against every unit of small lines judged one by one.
    {
        struct Small
        {
            const char* durations;
            uint32_t per;
        };
        for (const Small sm : {Small{"eq", 2}, Small{"eqh", 2}, Small{"q", 3}})
            for (const auto& [moves, percent] : std::vector<std::pair<int, int>>{{1, 100}, {2, 100}, {1, 50}, {2, 60}})
            {
                const NoteSet set = make_note_set(48, 60, sm.durations, 2);
                FilterLine l;
                l.kind = "audio", l.symbols_id = set.id(), l.base = set.base(), l.length = 2 * sm.per;
                const FilterStack con(l, {{find_filter("melody-contrary-v1"), {{"moves", std::to_string(moves)}, {"percent", std::to_string(percent)}}}}, none);
                uint64_t brute = 0;
                std::vector<uint32_t> u(l.length, 0);
                for (;;)
                {
                    brute += con.passes(u) ? 1 : 0;
                    size_t i = u.size();
                    while (i > 0 && ++u[i - 1] == l.base) u[--i] = 0;
                    if (i == 0) break;
                }
                CHECK(con.counted() && *con.counted() == BigUint(brute));
            }
        // three voices judge only; and the default two-voice line of 16 notes a voice is counted
        const NoteSet three = make_note_set(48, 60, "q", 3);
        FilterLine l3;
        l3.kind = "audio", l3.symbols_id = three.id(), l3.base = three.base(), l3.length = 6;
        CHECK(!FilterStack(l3, {{find_filter("melody-contrary-v1"), {}}}, none).counted());
    }
    CHECK(find_filter("sound-envelope-v1")->picks_a_kind && find_filter("sound-syllables-v1")->picks_a_kind && find_filter("sound-interval-v1")->picks_a_kind);
}

// List Q: border, specks and gradient against their rules written plainly: every picture of small sizes counted
// one by one and against the automaton's count, and random larger ones judged both ways.
void test_q_filters()
{
    static const TestResources none(nullptr, nullptr);
    using Pic = std::vector<uint32_t>;
    auto border_rule = [](const Pic& p, uint32_t W, uint32_t H) {
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x)
                if ((x == 0 || y == 0 || x + 1 == W || y + 1 == H) && p[y * W + x] != p[0]) return false;
        return true;
    };
    auto specks_rule = [](const Pic& p, uint32_t W, uint32_t H) {
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x)
            {
                const uint32_t c = p[y * W + x];
                const bool ok = (x > 0 && p[y * W + x - 1] == c) || (x + 1 < W && p[y * W + x + 1] == c) || (y > 0 && p[(y - 1) * W + x] == c) ||
                                (y + 1 < H && p[(y + 1) * W + x] == c);
                if (!ok) return false;
            }
        return true;
    };
    auto gradient_rule = [](const Pic& p, uint32_t W, uint32_t H, bool down) {
        int way = 0;
        for (uint32_t y = 0; y < H; ++y)
            for (uint32_t x = 0; x < W; ++x)
            {
                if (down ? y == 0 : x == 0) continue;
                const uint32_t a = down ? p[(y - 1) * W + x] : p[y * W + x - 1], b = p[y * W + x];
                if (a == b) continue;
                const int w = b > a ? 1 : -1;
                if (way && way != w) return false;
                way = w;
            }
        return true;
    };
    struct Shape
    {
        uint32_t W, H, B;
    };
    for (const Shape sh : {Shape{3, 3, 2}, Shape{4, 3, 2}, Shape{2, 4, 3}, Shape{3, 2, 4}, Shape{1, 3, 3}})
    {
        const Palette& pal = sh.B == 2 ? palette_by_id("mono") : palette_by_id("ega16");
        (void)pal;
        FilterLine l;
        l.kind = "image", l.symbols_id = "test", l.base = sh.B, l.length = sh.W * sh.H, l.width = sh.W, l.height = sh.H;
        const FilterStack border(l, {{find_filter("border-v1"), {}}}, none), specks(l, {{find_filter("specks-v1"), {}}}, none);
        const FilterStack across(l, {{find_filter("gradient-v1"), {}}}, none), down(l, {{find_filter("gradient-v1"), {{"way", "down"}}}}, none);
        uint64_t nb = 0, ns = 0, na = 0, nd = 0, total = 1;
        for (uint32_t k = 0; k < l.length; ++k) total *= sh.B;
        for (uint64_t i = 0; i < total; ++i)
        {
            Pic p(l.length);
            uint64_t v = i;
            for (auto& c : p) c = uint32_t(v % sh.B), v /= sh.B;
            const bool b = border_rule(p, sh.W, sh.H), s = specks_rule(p, sh.W, sh.H), a = gradient_rule(p, sh.W, sh.H, false), d = gradient_rule(p, sh.W, sh.H, true);
            CHECK(border.passes(p) == b && specks.passes(p) == s && across.passes(p) == a && down.passes(p) == d);
            nb += b, ns += s, na += a, nd += d;
        }
        CHECK(border.ranker() && border.ranker()->count() == BigUint(nb));
        CHECK(specks.ranker() && specks.ranker()->count() == BigUint(ns));
        CHECK(across.ranker() && across.ranker()->count() == BigUint(na));
        CHECK(down.ranker() && down.ranker()->count() == BigUint(nd));
    }
    // The default line, 10 x 10 in black and white: random pictures, half of them smoothed into patches so each rule
    // meets both answers.
    {
        FilterLine l;
        l.kind = "image", l.symbols_id = "test", l.base = 2, l.length = 100, l.width = 10, l.height = 10;
        const FilterStack border(l, {{find_filter("border-v1"), {}}}, none), specks(l, {{find_filter("specks-v1"), {}}}, none);
        const FilterStack across(l, {{find_filter("gradient-v1"), {}}}, none), down(l, {{find_filter("gradient-v1"), {{"way", "down"}}}}, none);
        std::mt19937_64 rng(43);
        for (int k = 0; k < 4000; ++k)
        {
            Pic p(100);
            for (auto& c : p) c = uint32_t(rng() & 1);
            if (k % 2)
                for (uint32_t i = 1; i < 100; ++i)
                    if (rng() % 4) p[i] = p[i - 1];
            if (k % 4 == 1)
                for (uint32_t y = 0; y < 10; ++y) // (a gradient across: each row 0s then 1s)
                    for (uint32_t x = 0; x < 10; ++x) p[y * 10 + x] = x >= (rng() % 11) ? 1 : 0;
            CHECK(border.passes(p) == border_rule(p, 10, 10));
            CHECK(specks.passes(p) == specks_rule(p, 10, 10));
            CHECK(across.passes(p) == gradient_rule(p, 10, 10, false));
            CHECK(down.passes(p) == gradient_rule(p, 10, 10, true));
        }
    }
    // Border and gradient pick a kind of picture: ticking every filter at once leaves them; specks sets aside noise.
    CHECK(find_filter("border-v1")->picks_a_kind && find_filter("gradient-v1")->picks_a_kind && !find_filter("specks-v1")->picks_a_kind);
}

// List P: word-shape, punctuation, letters-and-digits, each its automaton against its rule written plainly, on
// prose and on 20,000 random pages, and counted.
void test_p_filters()
{
    static const TestResources none(nullptr, nullptr);
    const Alphabet& a96 = alphabet_by_id("ascii96");
    const Alphabet& a27 = alphabet_by_id("lower27");
    auto unit_of = [](const Alphabet& a, const std::string& t) {
        std::vector<uint32_t> u;
        for (const char c : t) u.push_back(*a.digit_of(char32_t(uint8_t(c))));
        return u;
    };
    auto letter = [](char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); };
    auto digit = [](char c) { return c >= '0' && c <= '9'; };
    auto shape_rule = [&](const std::string& t, size_t longest) {
        std::vector<size_t> ws;
        size_t n = 0;
        for (const char c : t + " ")
            if (letter(c)) ++n;
            else if (n) ws.push_back(n), n = 0;
        for (size_t k = 0; k + 1 < ws.size(); ++k)
            if (ws[k] > longest && ws[k + 1] > longest) return false;
        return true;
    };
    auto punct_rule = [&](const std::string& t) {
        const std::string close = ")]}\"'";
        for (size_t i = 0; i < t.size(); ++i)
        {
            const char c = t[i];
            const bool mid = c == ',' || c == ';' || c == ':', end = c == '.' || c == '!' || c == '?';
            if (!mid && !end) continue;
            if (i > 0 && (t[i - 1] == ' ' || t[i - 1] == '\n')) return false;
            if (i + 1 == t.size()) continue;
            const char x = t[i + 1];
            if (x == ' ' || x == '\n' || close.find(x) != std::string::npos) continue;
            if (end && (x == '.' || x == '!' || x == '?')) continue;
            if (c != ';' && c != '!' && c != '?' && digit(x) && i > 0 && digit(t[i - 1])) continue;
            return false;
        }
        return true;
    };
    auto ld_rule = [&](const std::string& t, size_t turns) {
        size_t n = 0;
        for (size_t i = 0; i < t.size(); ++i)
        {
            const bool in = letter(t[i]) || digit(t[i]);
            if (!in) { n = 0; continue; }
            if (i > 0 && (letter(t[i - 1]) || digit(t[i - 1])) && digit(t[i]) != digit(t[i - 1]) && ++n > turns) return false;
        }
        return true;
    };
    const FilterLine p96{"text", "ascii96", 96, 40, &a96, 0, 0, 0};
    const FilterLine p27{"text", "lower27", 27, 40, &a27, 0, 0, 0};
    const FilterStack shape96(p96, {{find_filter("word-shape-v1"), {}}}, none), shape27(p27, {{find_filter("word-shape-v1"), {{"longest", "4"}}}}, none);
    const FilterStack punct(p96, {{find_filter("punctuation-v1"), {}}}, none), ld(p96, {{find_filter("letters-and-digits-v1"), {}}}, none);
    const char* prose[] = {"The quick brown fox, 3rd of its kind, ran.", "Prices: 1,000 at 12:30; 3.14 (or so)! Yes", "x86 and 1990s",
                           "a1b2 is not a word , nor this", "internationalisation organisations", "Hello.World and e.g.this"};
    for (const char* q : prose)
    {
        std::string t(q);
        t.resize(40, ' ');
        CHECK(shape96.passes(unit_of(a96, t)) == shape_rule(t, 10));
        CHECK(punct.passes(unit_of(a96, t)) == punct_rule(t));
        CHECK(ld.passes(unit_of(a96, t)) == ld_rule(t, 1));
    }
    std::mt19937_64 rng(41);
    // (random pages weighted toward the marks, digits and spaces each rule turns on)
    const std::string pool96 = "abcdeABC xyz  012,.;:!?)\"'\n([", pool27 = "abcdefghij   ";
    for (int k = 0; k < 20000; ++k)
    {
        std::string t(40, ' '), w(40, ' ');
        for (char& c : t) c = pool96[rng() % pool96.size()];
        for (char& c : w) c = pool27[rng() % pool27.size()];
        CHECK(shape96.passes(unit_of(a96, t)) == shape_rule(t, 10));
        CHECK(shape27.passes(unit_of(a27, w)) == shape_rule(w, 4));
        CHECK(punct.passes(unit_of(a96, t)) == punct_rule(t));
        CHECK(ld.passes(unit_of(a96, t)) == ld_rule(t, 1));
    }
    // Each counts and ranks (an automaton), and merges with the pages' other automata.
    CHECK(shape96.ranker() && punct.ranker() && ld.ranker());
    const FilterStack all(p96, {{find_filter("word-shape-v1"), {}}, {find_filter("punctuation-v1"), {}}, {find_filter("letters-and-digits-v1"), {}}, {find_filter("brackets-v1"), {}}}, none);
    CHECK(all.ranker() != nullptr);
    // (lower27 has no punctuation and no digits: only word-shape applies there)
    CHECK(find_filter("word-shape-v1")->applies(p27) && !find_filter("punctuation-v1")->applies(p27) && !find_filter("letters-and-digits-v1")->applies(p27));
}

void test_z_filters()
{
    static const TestResources none(nullptr, nullptr);
    auto count = [&](const FilterLine& l, const char* name, const FilterValues& v = {}) {
        const FilterStack st(l, {{find_filter(name), v}}, none);
        if (st.ranker()) return st.ranker()->count().to_decimal();
        if (const BigUint* n = st.counted()) return n->to_decimal() + " judged";
        return std::string("none");
    };
    auto unit_of = [](const Alphabet& a, const std::string& t) {
        std::vector<uint32_t> u;
        for (const char c : t) u.push_back(*a.digit_of(char32_t(uint8_t(c))));
        return u;
    };
    const Alphabet& a27 = alphabet_by_id("lower27");
    const FilterLine page{"text", "lower27", 27, 32, &a27, 0, 0, 0};
    // few-values: 27 pages of one letter and 351 x (2^32 - 2) of two set aside, as an automaton.
    {
        BigUint few(351);
        few = BigUint::mul(few, BigUint::pow(2, 32));
        few -= BigUint(702 - 27);
        BigUint kept = BigUint::pow(27, 32);
        kept -= few;
        CHECK(count(page, "few-values-v1") == kept.to_decimal());
        const FilterStack st(page, {{find_filter("few-values-v1"), {}}}, none);
        CHECK(!st.passes(unit_of(a27, "abababababababababababababababab")) && st.passes(unit_of(a27, "the quick brown fox jumps over t")));
        // (k = 4, sixteen values: past what an automaton holds at 27 letters, counted exactly all the same)
        CHECK(count(page, "few-values-v1", {{"bits", "4"}}).find("judged") != std::string::npos);
        // (review 4: what it counts as, worked out by counting its states, is what make builds)
        bool same = true;
        for (const uint32_t base : {2u, 4u, 16u, 27u, 40u, 96u, 256u})
            for (int bits = 1; bits <= 4; ++bits)
            {
                FilterLine l = page;
                l.base = base, l.alphabet = nullptr, l.symbols_id = "test" + std::to_string(base);
                const FilterValues v{{"bits", std::to_string(bits)}};
                const FilterSpec& fv = *find_filter("few-values-v1");
                const bool automaton = plugin_dfa(*fv.make(l, v, none)) != nullptr;
                same = same && (fv.counts_as_on(l, &v) == "automaton") == automaton;
            }
        CHECK(same);
    }
    // two-symbol-run: English passes, padding does not.
    {
        const FilterStack st(page, {{find_filter("two-symbol-run-v1"), {}}}, none);
        CHECK(st.ranker() != nullptr);
        CHECK(st.passes(unit_of(a27, "welcome to the sieve and its lin")));
        CHECK(!st.passes(unit_of(a27, "welcome to the sieaaaaabbbbbaaaa"))); // (14 on a and b)
        CHECK(!st.passes(unit_of(a27, "xyz ababababababababababababab q")));
    }
    // golden-word: 33 windows at length 32, each either way round, over 351 pairs; every Sturmian word, 3,648.
    {
        CHECK(count(page, "golden-word-v1") == "23166 judged");
        CHECK(count(page, "golden-word-v1", {{"slopes", "every"}}) == "1280448 judged");
        const FilterStack st(page, {{find_filter("golden-word-v1"), {}}}, none), every(page, {{find_filter("golden-word-v1"), {{"slopes", "every"}}}}, none);
        // (the Fibonacci word's first 32 symbols, 0 as a and 1 as b; then as 1 and 0; then a balanced word of another slope)
        CHECK(st.passes(unit_of(a27, "abaababaabaababaababaabaababaaba")) && st.passes(unit_of(a27, "babbababbabbababbababbabbababbab")));
        CHECK(!st.passes(unit_of(a27, "aabaabaabaabaabaabaabaabaabaabaa")) && every.passes(unit_of(a27, "aabaabaabaabaabaabaabaabaabaabaa")));
        CHECK(!every.passes(unit_of(a27, "aabbaabbaabbaabbaabbaabbaabbaabb")));
    }
    // The sound line: a 440 Hz tone passes both, noise neither.
    {
        const PcmFormat f = make_pcm_format(8000, 8, 1);
        const FilterLine sound{"audio", f.id(), f.base(), 800, nullptr, 0, 0, 0};
        std::vector<uint32_t> tone, noise;
        uint64_t r = 12345;
        for (int i = 0; i < 800; ++i)
        {
            tone.push_back(pcm_digit(f, int32_t(std::lround(100 * std::sin(2 * 3.14159265358979 * 440 * i / 8000.0)))));
            r = r * 6364136223846793005ull + 1442695040888963407ull;
            noise.push_back(uint32_t(r >> 56));
        }
        const FilterStack cross(sound, {{find_filter("sound-crossings-v1"), {}}}, none), bend(sound, {{find_filter("sound-bend-v1"), {}}}, none);
        CHECK(cross.ranker() != nullptr); // (8 bits, 800 samples: the last sign and 159 crossings, an automaton)
        CHECK(cross.passes(tone) && bend.passes(tone) && !cross.passes(noise) && !bend.passes(noise));
        const PcmFormat f4 = make_pcm_format(8000, 4, 1);
        const FilterLine small{"audio", f4.id(), f4.base(), 64, nullptr, 0, 0, 0};
        CHECK(count(small, "sound-bend-v1") == "22739424633651186841132108843799674201872012366146772");
    }
    // The video line: still videos set aside, and frames that change a little kept, counted exactly.
    {
        ImageFormat vf;
        vf.width = 10, vf.height = 10, vf.frames = 4, vf.palette = &palette_by_id("mono");
        const FilterLine video{"video", vf.symbols_id(), 2, 400, nullptr, 10, 10, 4};
        BigUint moving = BigUint::pow(2, 400);
        moving -= BigUint::pow(2, 100);
        CHECK(count(video, "video-moves-v1") == moving.to_decimal()); // (ranked since list N)
        // (5%: five pixels a frame; within five of one frame, 1 + 100 + ... + C(100, 5) = 79,375,496)
        BigUint smooth = BigUint::mul(BigUint::pow(2, 100), BigUint::pow(BigUint(79375496), 3));
        CHECK(count(video, "video-smooth-v1", {{"percent", "5"}}) == smooth.to_decimal() + " judged");
        std::vector<uint32_t> still(400, 0), flicker(400, 0);
        for (size_t i = 100; i < 200; ++i) flicker[i] = 1;
        still[5] = still[105] = still[205] = still[305] = 1;
        const FilterStack mv(video, {{find_filter("video-moves-v1"), {}}}, none), sm(video, {{find_filter("video-smooth-v1"), {}}}, none);
        CHECK(!mv.passes(still) && mv.passes(flicker) && sm.passes(still) && !sm.passes(flicker));
    }
    // The fractals in the pages and audio lines (list Z4): counted and ranked exactly.
    {
        set_fractal_chart_folder(std::string(g_data) + "/fractals");
        CHECK(count(page, "fractal-written-v1") == "43");
        const Alphabet& a96 = alphabet_by_id("ascii96");
        const FilterLine page96{"text", "ascii96", 96, 32, &a96, 0, 0, 0};
        CHECK(count(page96, "fractal-written-v1") == "531");
        const FilterLine notes{"audio", kNotesSymbolsId, kNoteSymbols, 16, nullptr, 0, 0, 0};
        CHECK(count(notes, "fractal-scored-v1") == "3548");
        // (the Fibonacci word's first symbols as a page: 0 and 1 are not lower27's, but are ascii96's)
        const FilterStack w96(page96, {{find_filter("fractal-written-v1"), {}}}, none);
        std::string fib = "01001010010010100101001001010010";
        CHECK(w96.passes(unit_of(a96, fib)));
    }
    // List Z8 and Z9: pronounceable pages (an automaton, counted and ranked), mirror-image pictures, films of
    // the Game of Life and of a camera panning (counted, judged), melodies that bring a figure back, and
    // sound with a pitch (judged).
    {
        const FilterStack sp(page, {{find_filter("pronounceable-v1"), {}}}, none);
        CHECK(sp.ranker() != nullptr);
        CHECK(sp.passes(unit_of(a27, "strengths and queueing are words")));
        CHECK(!sp.passes(unit_of(a27, "xqzk wrtp mnbv cxzl kjhg fdsa qw")));
        CHECK(!sp.passes(unit_of(a27, "rhythms are fine but strrrngths ")));
        const BigUint n = sp.ranker()->count();
        CHECK(sp.ranker()->accepts(sp.ranker()->unrank(BigUint(12345))));
        CHECK(!(n < BigUint::pow(10, 44)) && n < BigUint::pow(27, 32));
    }
    {
        // letter-case and brackets, on ascii96 (both cases, and brackets to close)
        const Alphabet& a96 = alphabet_by_id("ascii96");
        const FilterLine page96{"text", "ascii96", 96, 32, &a96, 0, 0, 0};
        CHECK(!find_filter("letter-case-v1")->applies(page) && !find_filter("brackets-v1")->applies(page));
        const FilterStack lc(page96, {{find_filter("letter-case-v1"), {}}}, none), br(page96, {{find_filter("brackets-v1"), {}}}, none);
        CHECK(lc.ranker() && br.ranker());
        CHECK(lc.passes(unit_of(a96, "Sieve reads NASA files. It works")));
        CHECK(!lc.passes(unit_of(a96, "Sieve reads NASA files. it works")));
        CHECK(!lc.passes(unit_of(a96, "sIeve reads nasa files, it works")));
        CHECK(br.passes(unit_of(a96, "f(x) = [a (b {c}) d] and (e) ok.")));
        CHECK(!br.passes(unit_of(a96, "f(x) = [a (b {c)} d] and (e) ok.")));
        CHECK(!br.passes(unit_of(a96, "f(x) = [a (b {c}) d] and (e ok..")));
        CHECK(!br.passes(unit_of(a96, ")(x) = [a (b {c}) d] and (e) ok.")));
    }
    {
        const FilterStack wl(page, {{find_filter("word-length-v2"), {{"longest", "10"}}}}, none);
        CHECK(find_filter("word-length-v1")->retired && find_filter("word-length-v1")->replaced_by == "word-length-v2");
        CHECK(param_value(*find_filter("word-length-v2"), {}, "longest") == "28");
        CHECK(throws([&] { (void)FilterStack(page, {{find_filter("word-length-v2"), {{"longest", "4"}}}}, none); }));
        CHECK(FilterStack(page, {{find_filter("word-length-v2"), {{"longest", "100"}}}}, none).ranker() != nullptr);
        CHECK(wl.ranker() && wl.passes(unit_of(a27, "words of ordinary lengths pass i")) && !wl.passes(unit_of(a27, "antidisestablishmentarianism ok ")));
        // (lower27, 32 letters, no word past 20: every page but those with a run of 21 letters or more)
        CHECK(count(page, "word-length-v2") != "none");
    }
    {
        ImageFormat f;
        f.width = 10, f.height = 10, f.palette = &palette_by_id("mono");
        const FilterLine pic{"image", f.symbols_id(), 2, 100, nullptr, 10, 10, 1};
        CHECK(count(pic, "image-mirror-v1") == BigUint::pow(2, 50).to_decimal());
        CHECK(count(pic, "image-mirror-v1", {{"axis", "both"}}) == BigUint::pow(2, 25).to_decimal());
        CHECK(count(pic, "image-mirror-v1", {{"axis", "turn"}}) == BigUint::pow(2, 50).to_decimal());
        std::vector<uint32_t> face(100, 0);
        face[2 * 10 + 3] = face[2 * 10 + 6] = 1; // (two eyes, a mouth)
        for (int x = 3; x <= 6; ++x) face[7 * 10 + x] = 1;
        const FilterStack across(pic, {{find_filter("image-mirror-v1"), {}}}, none), down(pic, {{find_filter("image-mirror-v1"), {{"axis", "down"}}}}, none);
        CHECK(across.passes(face) && !down.passes(face));
        ImageFormat vf = f;
        vf.frames = 4;
        const FilterLine video{"video", vf.symbols_id(), 2, 400, nullptr, 10, 10, 4};
        CHECK(count(video, "life-causal-v1") == BigUint::pow(2, 100).to_decimal());
        CHECK(count(video, "video-pan-v1") == BigUint::pow(2, 130).to_decimal());
        // (a blinker: three across, then three down, then across again)
        std::vector<uint32_t> blink(400, 0);
        for (int k = 0; k < 4; ++k)
            for (int j = -1; j <= 1; ++j) blink[size_t(k) * 100 + (k % 2 ? size_t(5 + j) * 10 + 5 : 5 * 10 + size_t(5 + j))] = 1;
        const FilterStack life(video, {{find_filter("life-causal-v1"), {}}}, none), pan(video, {{find_filter("video-pan-v1"), {}}}, none);
        CHECK(life.passes(blink) && !pan.passes(blink));
        // (a bar moving left a pixel a frame, the column coming in black)
        std::vector<uint32_t> bar(400, 0);
        for (int k = 0; k < 4; ++k)
            for (int y = 0; y < 10; ++y) bar[size_t(k) * 100 + size_t(y) * 10 + size_t(7 - k)] = 1;
        CHECK(pan.passes(bar) && !life.passes(bar));
        CHECK(throws([&] { (void)FilterStack(video, {{find_filter("life-causal-v1"), {{"rule", "B3S23"}}}}, none); }));
    }
    {
        const FilterLine notes{"audio", kNotesSymbolsId, kNoteSymbols, 16, nullptr, 0, 0, 0};
        const FilterStack motif(notes, {{find_filter("melody-motif-v1"), {}}}, none);
        auto melody = [](const char* text) { return canonicalise_notes(text, 16).units.at(0); };
        // (a figure, then the same figure a fifth higher: one motif twice)
        CHECK(motif.passes(melody("C4q D4q E4q C4q G4q A4q B4q G4q C5q C5q C5q C5q C5q C5q C5q C5q")));
        CHECK(!motif.passes(melody("C4q D4e F4h E4q G#4e A4w B4q C5e D#4h F#4q C4e B4h D5q A#4e G4w E5q")));
        const PcmFormat f = make_pcm_format(8000, 8, 1);
        const FilterLine sound{"audio", f.id(), f.base(), 800, nullptr, 0, 0, 0};
        const FilterStack periodic(sound, {{find_filter("sound-periodic-v1"), {}}}, none);
        std::vector<uint32_t> tone, noise, quiet(800, pcm_digit(f, 0));
        uint64_t r = 99;
        for (int i = 0; i < 800; ++i)
        {
            tone.push_back(pcm_digit(f, int32_t(std::lround(100 * std::sin(2 * 3.14159265358979 * 220 * i / 8000.0)))));
            r = r * 6364136223846793005ull + 1442695040888963407ull;
            noise.push_back(uint32_t(r >> 56));
        }
        CHECK(periodic.passes(tone) && !periodic.passes(noise) && !periodic.passes(quiet));
    }
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
    CHECK(filters_for(text_line(8)).size() == 26); // (word-shape-v1, list P, the 24th: punctuation and letters-and-digits need marks and digits;
                                                    // degree-window-v1 and -v2, list N, the 25th and 26th)
                                                    // not-written-v1, not-a-file-v1, not-other-line-v1 and not-a-pattern-v1 joined the ten;
                                                    // symbol-entropy-v2 and model-information-v2 (IDEAS §2.1, §2.2) joined those;
                                                    // few-values, two-symbol-run, golden-word, fractal-written, pronounceable and word-length (v1 and v2) (list Z) these
    const FilterLine image{"image", "image/mono/10x10", 2, 100, nullptr, 10, 10, 1};
    CHECK(filters_for(image).size() == 18); // symbol-entropy (v1 and v2), neighbour-agreement, not-a-file, not-packed,
                                            // not-a-pattern, palette-size, row-runs, figure, fractal-drawn (v1 and v2), image-mirror,
                                            // border, specks and gradient (list Q), and rows-alike and degree-window v1 and v2 (list N)
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
    // Version 2 (IDEAS §2.1, §2.2): against the line's own most, log2 min(base, n), and trailing
    // padding (two or more SPACEs) not counted.
    {
        auto ent = [&](uint32_t L, const char* lo, const char* hi) {
            return FilterStack(text_line(L), {{find_filter("symbol-entropy-v2"), {{"min_permille", lo}, {"max_permille", hi}}}}, res);
        };
        CHECK(ent(4, "500", "500").passes(digits27("abab")));  // 1 bit of log2 4 = 2
        CHECK(!ent(4, "501", "1000").passes(digits27("abab")));
        CHECK(ent(4, "1000", "1000").passes(digits27("abcd"))); // the most there is: bounds inclusive
        CHECK(!ent(4, "1", "1000").passes(digits27("aaaa")));
        // "ab  " is "ab": 1 bit of log2 2 = 1, so 1000; v1 counts the SPACEs (1.5 bits)
        CHECK(ent(4, "1000", "1000").passes(digits27("ab  ")));
        CHECK(!ent(4, "0", "999").passes(digits27("ab  ")));
        const FilterStack v1(text_line(4), {{find_filter("symbol-entropy-v1"), {{"min_millibits", "1500"}, {"max_millibits", "1500"}}}}, res);
        CHECK(v1.passes(digits27("ab  ")));
        CHECK(!ent(4, "0", "1000").passes(digits27("    "))); // padding alone
        // one SPACE at the end is not padding: "abc " is judged whole (log2 4 of log2 4)
        CHECK(ent(4, "1000", "1000").passes(digits27("abc ")));
        // model-information: noise padded out with SPACEs passed v1 (SPACEs are cheap), not v2
        const auto padded = digits27("qzxvjk                          ");
        const FilterStack info1(text_line(32), {{find_filter("model-information-v1"), {}}}, res);
        const FilterStack info2(text_line(32), {{find_filter("model-information-v2"), {}}}, res);
        CHECK(info1.passes(padded));
        CHECK(!info2.passes(padded));
        CHECK(info2.passes(digits27("it was the best of times        ")));
        CHECK(!info2.passes(digits27("                                ")));
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
// floor(a * b / c) exactly, for the bearings below (BigUint: MSVC has no 128-bit integer).
uint64_t mul_div(uint64_t a, uint64_t b, uint64_t c)
{
    BigUint q, r;
    BigUint::divmod(BigUint::mul(BigUint(a), BigUint(b)), BigUint(c), q, r);
    return std::stoull(q.to_decimal());
}

// degree-window-v1 (sieve/window.hpp): the units whose bearing floor(v * 360 * 10^d / N) lies in [from, to), alone and
// over other filters, against every unit of small lines: counted, ranked and walked (the generic walk, by next and
// completions, as well as the window's own rank and unrank), the bounds exactly the hallway's bearings.
void test_degree_window(const std::string& dir)
{
    static const TestResources none(nullptr, nullptr);
    const FilterSpec* w = find_filter("degree-window-v2");
    CHECK(w && w->window && w->picks_a_kind && w->whole_unit && w->category == "soft");
    CHECK(find_filter("degree-window-v1")->retired && find_filter("degree-window-v1")->replaced_by == "degree-window-v2");
    if (!w) return;
    const auto wv = [](uint64_t from, uint64_t to, uint32_t d) { // (written as from and to; the filter takes a width)
        return FilterValues{{"from", std::to_string(from)}, {"width", std::to_string(to - from)}, {"decimals", std::to_string(d)}};
    };
    // the rule, plainly
    const auto bearing_in = [](uint64_t v, uint64_t n, uint64_t from, uint64_t to, uint32_t d) {
        uint64_t s = 360;
        for (uint32_t i = 0; i < d; ++i) s *= 10;
        const uint64_t b = mul_div(v, s, n);
        return b >= from && b < to;
    };
    const auto walk_both = [](const Ranker& rk, bool& ok) { // the generic walk agrees with the window's own
        const uint64_t c = rk.count().is_zero() ? 0 : uint64_t(rk.count().limbs()[0]);
        for (uint64_t k = 0; k < c; k += 1 + c / 61)
        {
            const auto u = rk.unrank(BigUint(k));
            ok = ok && rk.Ranker::unrank(BigUint(k)) == u && rk.Ranker::rank(u) == BigUint(k);
        }
    };
    // Alone, on the pages line at 4 letters (531,441 units).
    const FilterLine l4 = text_line(4);
    const uint64_t n4 = 27ull * 27 * 27 * 27;
    for (auto [from, to, d] : std::vector<std::tuple<uint64_t, uint64_t, uint32_t>>{
             {0, 1, 0}, {1, 2, 0}, {1, 5, 0}, {359, 360, 0}, {0, 360, 0}, {100, 101, 0}, {1234, 1240, 2}, {35999, 36000, 2}, {7, 8, 3}})
    {
        const FilterStack st(l4, {{w, wv(from, to, d)}}, none);
        bool ok = true;
        uint64_t n = 0;
        for (uint64_t v = 0; v < n4; ++v)
        {
            const auto u = BigUint(v).to_digits(27, 4);
            const bool in = bearing_in(v, n4, from, to, d);
            ok = ok && st.passes(u) == in;
            n += in;
        }
        CHECK(ok);
        CHECK(st.ranker() && st.ranker()->count() == BigUint(n));
        check_ranker_exhaustive(st, 27, 4, ok);
        if (st.ranker()) walk_both(*st.ranker(), ok);
        CHECK(ok);
    }
    // A window narrower than one unit's share keeps nothing (a line of 27 units, a thousandth of a degree).
    {
        const FilterStack st(text_line(1), {{w, wv(5, 6, 3)}}, none);
        CHECK(st.ranker() && st.ranker()->count().is_zero());
    }
    // Over plugins: max-run-data alone, with a second plugin merged, and two windows at once (the part both keep).
    const std::string filters = dir + "../data/filters/";
    const FilterSpec run = plugin_spec(load_plugin_file(filters + "max-run-data-v1.sfilter"));
    const FilterSpec pairs = plugin_spec(load_plugin_file(filters + "letter-pairs-v1.sfilter"));
    for (auto [from, to] : std::vector<std::pair<uint64_t, uint64_t>>{{0, 1}, {10, 40}, {200, 360}, {359, 360}})
    {
        const FilterStack plain(l4, {{&run, {{"max", "1"}}}}, none);
        const FilterStack st(l4, {{&run, {{"max", "1"}}}, {w, wv(from, to, 0)}}, none);
        bool ok = true;
        check_ranker_exhaustive(st, 27, 4, ok);
        uint64_t n = 0;
        for (uint64_t v = 0; v < n4; ++v)
        {
            const auto u = BigUint(v).to_digits(27, 4);
            n += plain.passes(u) && bearing_in(v, n4, from, to, 0);
        }
        CHECK(st.ranker() && st.ranker()->count() == BigUint(n));
        if (st.ranker()) walk_both(*st.ranker(), ok);
        const FilterStack merged(l4, {{w, wv(from, to, 0)}, {&run, {{"max", "2"}}}, {&pairs, {}}}, none);
        check_ranker_exhaustive(merged, 27, 4, ok);
        CHECK(ok);
    }
    {
        const FilterStack two(l4, {{w, wv(10, 40, 0)}, {&run, {{"max", "1"}}}, {w, wv(30, 90, 0)}}, none),
            one(l4, {{&run, {{"max", "1"}}}, {w, wv(30, 40, 0)}}, none);
        bool ok = true;
        check_ranker_exhaustive(two, 27, 4, ok);
        CHECK(ok && two.ranker() && one.ranker() && two.ranker()->count() == one.ranker()->count());
    }
    // A stack that cannot count still cannot with a window, and says why (not the window).
    {
        const FilterStack st(l4, {{find_filter("symbol-entropy-v2"), {}}, {w, wv(0, 90, 0)}}, none);
        CHECK(!st.ranker() && st.compact_blocker().find("degree-window") == std::string::npos);
        CHECK(filter_conflict(*w, *find_filter("not-a-file-v1")).empty() && filter_conflict(*find_filter("not-a-file-v1"), *w).empty());
    }
    // Two voices (stereo one-bit sound, 6 samples a channel): the bearing is the whole unit's.
    {
        const PcmFormat f = make_pcm_format(8, 1, 2);
        FilterLine l;
        l.kind = "audio", l.symbols_id = f.id(), l.base = f.base(), l.length = 12;
        const FilterStack st(l, {{w, wv(90, 200, 0)}}, none);
        bool ok = true;
        check_ranker_exhaustive(st, f.base(), 12, ok);
        uint64_t n = 0;
        const uint64_t all = uint64_t(1) << 12;
        for (uint64_t v = 0; v < all; ++v) n += bearing_in(v, all, 90, 200, 0);
        CHECK(ok && st.ranker() && st.ranker()->count() == BigUint(n));
    }
    // Across 0 (v2, list N5-2): 350 for 20 keeps 350 to 360 and 0 to 10, the stretch at the loop's start first; alone,
    // over a plugin, and to two decimals. (v1 stopped at 360.)
    for (auto [from, width, d] : std::vector<std::tuple<uint64_t, uint64_t, uint32_t>>{{350, 20, 0}, {359, 2, 0}, {300, 100, 0}, {35990, 25, 2}, {200, 360, 0}})
    {
        uint64_t whole = 360;
        for (uint32_t i = 0; i < d; ++i) whole *= 10;
        const FilterValues v{{"from", std::to_string(from)}, {"width", std::to_string(width)}, {"decimals", std::to_string(d)}};
        const FilterStack alone(l4, {{w, v}}, none), over(l4, {{&run, {{"max", "1"}}}, {w, v}}, none);
        const FilterStack plain(l4, {{&run, {{"max", "1"}}}}, none);
        bool ok = true;
        uint64_t n = 0, m = 0;
        for (uint64_t x = 0; x < n4; ++x)
        {
            const auto u = BigUint(x).to_digits(27, 4);
            const uint64_t b = mul_div(x, whole, n4);
            const bool in = width >= whole || b >= from || b < from + width - whole;
            ok = ok && alone.passes(u) == in;
            n += in;
            m += in && plain.passes(u);
        }
        check_ranker_exhaustive(alone, 27, 4, ok);
        check_ranker_exhaustive(over, 27, 4, ok);
        if (alone.ranker()) walk_both(*alone.ranker(), ok);
        if (over.ranker()) walk_both(*over.ranker(), ok);
        CHECK(ok && alone.ranker() && alone.ranker()->count() == BigUint(n) && over.ranker() && over.ranker()->count() == BigUint(m));
    }
    {
        const FilterStack v1(l4, {{find_filter("degree-window-v1"), {{"from", "350"}, {"width", "20"}}}}, none),
            to_end(l4, {{w, wv(350, 360, 0)}}, none);
        CHECK(v1.ranker() && to_end.ranker() && v1.ranker()->count() == to_end.ranker()->count());
        const FilterStack two(l4, {{w, {{"from", "350"}, {"width", "20"}}}, {w, wv(0, 5, 0)}}, none);
        CHECK(!two.ranker() && two.compact_blocker().find("crosses 0") != std::string::npos);
    }
    // A width of 0 is refused; from goes round the loop.
    bool threw = false;
    try { (void)FilterStack(l4, {{w, wv(5, 5, 0)}}, none); } catch (const std::invalid_argument&) { threw = true; }
    CHECK(threw);
    {
        const FilterStack round(l4, {{w, wv(361, 362, 0)}}, none), one(l4, {{w, wv(1, 2, 0)}}, none); // (361 degrees is 1 again)
        CHECK(round.ranker() && one.ranker() && round.ranker()->count() == one.ranker()->count());
    }
}

// List N3: the video line's filters that rank (life-causal, video-pan, video-smooth-v2) and image-mirror, against
// every unit of small lines: each survivor's number, the k-th survivor, the walk by next and completions, and
// the count, all as the filter judges.
void test_video_rankers(const std::string&)
{
    static const TestResources none(nullptr, nullptr);
    const auto video = [](uint32_t base, uint32_t w, uint32_t h, uint32_t f) {
        return FilterLine{"video", "video/test" + std::to_string(base), base, w * h * f, nullptr, w, h, f};
    };
    const auto exhaustive = [](const FilterLine& l, const char* name, const FilterValues& v) {
        const FilterStack st(l, {{find_filter(name), v}}, none);
        bool ok = st.ranker() != nullptr;
        if (ok) check_ranker_exhaustive(st, l.base, l.length, ok);
        if (ok)
        {
            const Ranker& rk = *st.ranker();
            const uint64_t c = rk.count().is_zero() ? 0 : uint64_t(rk.count().limbs()[0]);
            for (uint64_t k = 0; k < c; k += 1 + c / 37) // (the generic walk, by next and completions)
                ok = ok && rk.Ranker::unrank(BigUint(k)) == rk.unrank(BigUint(k)) && rk.Ranker::rank(rk.unrank(BigUint(k))) == BigUint(k);
        }
        if (!ok) std::cerr << "  " << name << " on " << l.width << "x" << l.height << "x" << l.frames << " (" << l.base << " colours) failed\n";
        return ok;
    };
    CHECK(find_filter("video-smooth-v1")->retired && find_filter("video-smooth-v1")->replaced_by == "video-smooth-v2");
    for (const FilterLine& l : {video(2, 2, 2, 3), video(3, 2, 1, 3), video(2, 3, 1, 4), video(4, 1, 2, 2)})
    {
        for (const char* pct : {"0", "25", "50", "100"}) CHECK(exhaustive(l, "video-smooth-v2", {{"percent", pct}}));
        CHECK(exhaustive(l, "video-moves-v1", {}));
        for (const char* way : {"left", "right", "up", "down"}) CHECK(exhaustive(l, "video-pan-v1", {{"direction", way}}));
        if (l.base == 2)
            for (const char* rule : {"B3/S23", "B36/S23", "B1/S012345678", "B/S"}) CHECK(exhaustive(l, "life-causal-v1", {{"rule", rule}}));
    }
    for (auto [w, h, base] : std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>{{3, 3, 2}, {4, 3, 2}, {3, 2, 3}, {2, 3, 3}})
    {
        const FilterLine pic{"image", "image/test" + std::to_string(base), base, w * h, nullptr, w, h, 1};
        for (const char* axis : {"across", "down", "both", "turn"}) CHECK(exhaustive(pic, "image-mirror-v1", {{"axis", axis}}));
    }
    // v2 sets the still videos aside too: v1's count less P^n, and with video-moves (which it implies) it compacts.
    {
        ImageFormat vf;
        vf.width = 10, vf.height = 10, vf.frames = 4, vf.palette = &palette_by_id("mono");
        const FilterLine l{"video", vf.symbols_id(), 2, 400, nullptr, 10, 10, 4};
        const FilterStack v1(l, {{find_filter("video-smooth-v1"), {{"percent", "5"}}}}, none),
            v2(l, {{find_filter("video-smooth-v2"), {{"percent", "5"}}}, {find_filter("video-moves-v1"), {}}}, none);
        CHECK(v1.counted() && v2.ranker());
        if (v1.counted() && v2.ranker())
        {
            BigUint want = *v1.counted();
            want -= BigUint::pow(2, 100);
            CHECK(v2.ranker()->count() == want);
            std::mt19937_64 rng(1803);
            for (int i = 0; i < 20; ++i)
            {
                BigUint k = BigUint::from_bytes(std::vector<uint8_t>{uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng()), uint8_t(rng())});
                k = BigUint::mod(BigUint::mul(k, BigUint::pow(10, 100)), v2.ranker()->count());
                const auto u = v2.ranker()->unrank(k);
                CHECK(v2.passes(u) && v2.ranker()->rank(u) == k);
            }
        }
    }
}

// Code-made automata whose settings read alike (word-length-v2 and word-shape-v1 both "longest=12") are merged and
// kept apart: a stack's merge is kept under each filter's name as well as its settings (list N).
void test_merge_keys(const std::string&)
{
    static const TestResources none(nullptr, nullptr);
    const FilterLine l = text_line(12);
    const FilterValues lv{{"longest", "6"}};
    const FilterStack a(l, {{find_filter("word-length-v2"), lv}, {find_filter("few-values-v1"), {}}}, none),
        b(l, {{find_filter("word-shape-v1"), lv}, {find_filter("few-values-v1"), {}}}, none);
    CHECK(a.ranker() && b.ranker());
    if (!a.ranker() || !b.ranker()) return;
    (void)a.ranker()->count();
    std::mt19937_64 rng(1811);
    bool same = true;
    for (int i = 0; i < 20000; ++i)
    {
        std::vector<uint32_t> u(12);
        for (auto& c : u) c = rng() % 3 == 0 ? 0 : uint32_t(1 + rng() % 26);
        same = same && b.ranker()->accepts(u) == b.passes(u) && a.ranker()->accepts(u) == a.passes(u);
    }
    CHECK(same);
}

// rows-alike-v1 (list N4): each row differs from the row above in at most k pixels (a video's frames each on their
// own), against the rule written plainly on every unit of small pictures and videos, and counted and ranked.
void test_rows_alike(const std::string&)
{
    static const TestResources none(nullptr, nullptr);
    const auto rule = [](const std::vector<uint32_t>& u, uint32_t w, uint32_t h, uint32_t k) {
        for (size_t f = 0; f < u.size(); f += size_t(w) * h)
            for (uint32_t y = 1; y < h; ++y)
            {
                uint32_t ch = 0;
                for (uint32_t x = 0; x < w; ++x) ch += u[f + size_t(y) * w + x] != u[f + size_t(y - 1) * w + x];
                if (ch > k) return false;
            }
        return true;
    };
    for (auto [kind, w, h, frames, base] : std::vector<std::tuple<const char*, uint32_t, uint32_t, uint32_t, uint32_t>>{
             {"image", 3, 3, 1, 2}, {"image", 4, 3, 1, 2}, {"image", 3, 2, 1, 3}, {"image", 2, 4, 1, 3}, {"video", 2, 2, 3, 2}, {"video", 3, 2, 2, 2}})
    {
        const FilterLine l{kind, std::string(kind) + "/test", base, w * h * frames, nullptr, w, h, frames};
        CHECK(find_filter("rows-alike-v1")->applies(l));
        for (uint32_t k = 0; k <= w; ++k)
        {
            const FilterStack st(l, {{find_filter("rows-alike-v1"), {{"changes", std::to_string(k)}}}}, none);
            bool ok = true;
            uint64_t total = 1;
            for (uint32_t i = 0; i < l.length; ++i) total *= base;
            for (uint64_t v = 0; v < total; ++v)
            {
                const auto u = BigUint(v).to_digits(base, l.length);
                ok = ok && st.passes(u) == rule(u, w, h, k);
            }
            check_ranker_exhaustive(st, base, l.length, ok);
            CHECK(ok);
        }
    }
    // On rgb24 the automaton does not fit: counted and ranked by changes so far (changes_ranker.hpp), its count
    // P^W g^(H-1) with g the rows within 5 changes of one row, every survivor drawn at random back to its number.
    {
        ImageFormat f;
        f.width = 10, f.height = 10, f.palette = &palette_by_id("rgb24");
        const FilterLine pic{"image", f.symbols_id(), 1u << 24, 100, nullptr, 10, 10, 1};
        CHECK(find_filter("rows-alike-v1")->counts_as_on(pic, nullptr) == "own");
        const FilterStack st(pic, {{find_filter("rows-alike-v1"), {}}}, none);
        CHECK(st.ranker() != nullptr);
        if (st.ranker())
        {
            BigUint g, term(1);
            for (uint32_t j = 0; j <= 5; ++j)
            {
                g += term;
                term.mul_small(10 - j);
                term.mul_small((1u << 24) - 1);
                (void)term.divmod_small(j + 1);
            }
            CHECK(st.ranker()->count() == BigUint::mul(BigUint::pow(1u << 24, 10), BigUint::pow(g, 9)));
            std::mt19937_64 rng(1813);
            for (int i = 0; i < 12; ++i)
            {
                std::vector<uint8_t> bytes(400);
                for (auto& b : bytes) b = uint8_t(rng());
                const BigUint k = BigUint::mod(BigUint::from_bytes(bytes), st.ranker()->count());
                const auto u = st.ranker()->unrank(k);
                CHECK(st.passes(u) && st.ranker()->rank(u) == k && st.ranker()->accepts(u));
            }
        }
    }
    // With row-runs, both automata merged: smooth both ways (and the merge kept apart from row-runs alone's).
    {
        ImageFormat f;
        f.width = 10, f.height = 10, f.palette = &palette_by_id("mono");
        const FilterLine pic{"image", f.symbols_id(), 2, 100, nullptr, 10, 10, 1};
        const FilterStack both(pic, {{find_filter("rows-alike-v1"), {}}, {find_filter("row-runs-v1"), {}}}, none);
        CHECK(both.ranker() != nullptr);
        std::vector<uint32_t> disc(100, 0);
        for (int y = 2; y < 8; ++y)
            for (int x = 3; x < 7; ++x) disc[size_t(y) * 10 + size_t(x)] = 1;
        CHECK(both.passes(disc));
        std::mt19937_64 rng(1812);
        std::vector<uint32_t> noise(100);
        for (auto& c : noise) c = uint32_t(rng() & 1);
        CHECK(!both.passes(noise));
    }
}

// quotes-v1 (list N5-12): against its rule written plainly, on random pages of a few symbols where quotation marks are
// common, and its count walked back.
void test_quotes(const std::string&)
{
    static const TestResources none(nullptr, nullptr);
    const Alphabet& a = alphabet_by_id("ascii96");
    const FilterLine l{"text", "ascii96", 96, 12, &a, 0, 0, 0};
    const FilterStack st(l, {{find_filter("quotes-v1"), {}}}, none);
    CHECK(st.ranker() != nullptr);
    const auto rule = [](const std::u32string& p) {
        int prev = 0, pend = 0, par = 0;
        for (char32_t x : p)
        {
            const bool space = x == U' ', quote = x == U'"', opener = x == U'(';
            const bool after = space || x == U'.' || x == U')';
            if (pend == 1 && (space || quote)) return false;
            if (pend == 2 && !after) return false;
            if (quote)
            {
                if (prev == 0) { if (par == 1) return false; prev = 1, pend = 1, par = 1; }
                else { if (par == 2) return false; prev = 1, pend = 2, par = 2; }
            }
            else prev = space || opener ? 0 : 1, pend = 0;
        }
        return pend != 1;
    };
    const std::u32string pool = U"a \".()";
    std::mt19937_64 rng(1819);
    bool same = true;
    for (int i = 0; i < 50000; ++i)
    {
        std::u32string p;
        std::vector<uint32_t> u;
        for (int k = 0; k < 12; ++k)
        {
            p.push_back(pool[rng() % pool.size()]);
            u.push_back(*a.digit_of(p.back()));
        }
        same = same && st.passes(u) == rule(p);
        if (st.ranker()) same = same && st.ranker()->accepts(u) == rule(p);
        if (st.ranker() && rule(p)) same = same && st.ranker()->unrank(st.ranker()->rank(u)) == u;
    }
    CHECK(same);
}

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
    for (uint32_t L : {1u, 2u, 5u, 9u, 12u}) // v2: thousandths of 1 bit, the most in black and white
        for (auto [lo, hi] : {std::pair<int, int>{0, 500}, {0, 900}, {600, 1000}, {950, 1000}})
        {
            const FilterLine img{"image", "image/mono/" + std::to_string(L) + "x1", 2, L, nullptr, L, 1, 1};
            const FilterStack st(img, {{find_filter("symbol-entropy-v2"), {{"min_permille", std::to_string(lo)}, {"max_permille", std::to_string(hi)}}}}, none);
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
    CHECK(n == 270); // a truncated or emptied vector file must fail, not pass quietly
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
void test_json()
{
    using sieve::json::parse;
    const auto v = parse(" {\"a\": [1, 2.5, -3e2, true, false, null], \"s\": \"x\\u00e9\\ud83d\\ude00\\n\\\"\", \"big\": 18446744073709551615} ");
    CHECK(v.is_object() && v.object().size() == 3);
    CHECK(v.at("a").array().size() == 6);
    CHECK(v.at("a").array()[0].u64() == 1);
    CHECK(v.at("a").array()[1].number() == 2.5);
    CHECK(v.at("a").array()[2].number() == -300);
    CHECK(throws([&] { (void)v.at("a").array()[1].u64(); })); // not a whole number
    CHECK(v.at("a").array()[3].boolean() && !v.at("a").array()[4].boolean() && v.at("a").array()[5].is_null());
    CHECK(v.at("s").string() == "x\xc3\xa9\xf0\x9f\x98\x80\n\"");
    CHECK(v.at("big").u64() == UINT64_MAX);
    CHECK(throws([&] { (void)parse("18446744073709551616").u64(); }));
    CHECK(v.find("missing") == nullptr);
    // Written compactly, in the order read, escaped as serde_json does.
    CHECK(sieve::json::write(v) == "{\"a\":[1,2.5,-3e2,true,false,null],\"s\":\"x\xc3\xa9\xf0\x9f\x98\x80\\n\\\"\",\"big\":18446744073709551615}");
    for (const char* badtext : {"", "{", "[1,]", "{\"a\":1,}", "01", "1 2", "\"\\x\"", "\"\x01\"", "\"\\ud800\"", "tru", "{1:2}"})
        CHECK(throws([&] { (void)parse(badtext); }));
}

// A JPEG at an eighth of its size from its blocks' averages (sieve-view's reduced view): the public
// domain Vitruvian (baseline, 949 by 1300) read by its frame alone and then whole, its eighth pinned;
// cut short or not a JPEG, nothing (and no crash).
void test_jpeg_eighth(const std::string& dir)
{
    std::ifstream f(dir + "../data/anchors/public-domain/Vitruvian.jpg", std::ios::binary);
    const std::vector<uint8_t> jpeg((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    CHECK(jpeg.size() > 1000);
    const auto size = jpeg_size(jpeg);
    CHECK(size && size->first == 949 && size->second == 1300);
    const auto e = jpeg_eighth(jpeg);
    CHECK(e && e->eighth_w == 119 && e->eighth_h == 163 && e->rgba.size() == size_t(119) * 163 * 4);
    if (e) CHECK(Sha256::hex(Sha256::hash(std::string(e->rgba.begin(), e->rgba.end()))) == "7f9dad61cffe05f1c4ff1215371598df30b3f5ab79f852b03c65d880574d98f3");
    bool quiet = true;
    for (const size_t cut : {size_t(2), size_t(100), jpeg.size() / 3, jpeg.size() - 10})
    {
        const std::vector<uint8_t> part(jpeg.begin(), jpeg.begin() + std::ptrdiff_t(cut));
        (void)jpeg_eighth(part); // (whatever it gives, it gives without a crash)
        quiet = quiet && !jpeg_eighth(std::vector<uint8_t>(10, 0xFF));
    }
    CHECK(quiet && !jpeg_size(std::vector<uint8_t>{0x89, 'P', 'N', 'G'}));
}

void test_safetensors()
{
    namespace st = sieve::safetensors;
    const auto tiny = sieve::json::parse(R"({"model_type":"llama","hidden_size":64,"intermediate_size":160,"num_attention_heads":4,
        "num_hidden_layers":2,"num_key_value_heads":2,"tie_word_embeddings":true,"torch_dtype":"bfloat16","vocab_size":300})");
    const auto t = st::llama_tensors(tiny);
    CHECK(t.size() == 20); // the embedding, 9 a layer, the final norm; tied, so no output layer
    CHECK(t[0].name == "model.embed_tokens.weight" && t[0].shape == (std::vector<uint64_t>{300, 64}) && t[0].dtype == "BF16");
    // An untied model has an output layer of its own; biases are not covered.
    {
        std::string untied = sieve::json::write(tiny);
        untied.replace(untied.find("true"), 4, "false");
        CHECK(st::llama_tensors(sieve::json::parse(untied)).size() == 21);
        CHECK(throws([] { (void)st::llama_tensors(sieve::json::parse(R"({"model_type":"llama","attention_bias":true})")); }));
        CHECK(throws([] { (void)st::llama_tensors(sieve::json::parse(R"({"model_type":"gpt2"})")); }));
    }
    // Sizes held to bounds, one reader for the layout and the model (7 November 2026): a config.json
    // claiming 10^18 layers was listed until the memory ran out.
    {
        std::string huge = sieve::json::write(tiny);
        huge.replace(huge.find("\"num_hidden_layers\":2"), 21, "\"num_hidden_layers\":1000000000000000000");
        CHECK(throws([&] { (void)st::llama_tensors(sieve::json::parse(huge)); }));
        std::string zero = sieve::json::write(tiny);
        zero.replace(zero.find("\"num_key_value_heads\":2"), 23, "\"num_key_value_heads\":0");
        CHECK(throws([&] { (void)st::llama_shape(sieve::json::parse(zero)); }));
        const st::LlamaShape sh = st::llama_shape(tiny);
        CHECK(sh.layers == 2 && sh.head_dim == 16 && sh.kv_heads == 2 && sh.tied);
    }
    // The layout: sorted by name, offsets end to end, padded to 8; read back, canonical.
    std::vector<st::Tensor> laid;
    const std::string start = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, t, &laid);
    CHECK(start.size() % 8 == 0);
    CHECK(laid.front().name == "model.embed_tokens.weight" && laid.front().begin == 0 && laid.back().name == "model.norm.weight");
    const std::span<const uint8_t> bytes(reinterpret_cast<const uint8_t*>(start.data()), start.size());
    const st::Header h = st::parse_header(bytes);
    CHECK(h.start_bytes() == start.size() && h.tensors.size() == 20 && st::is_canonical(h));
    CHECK(h.data_bytes() == 211072); // as the oracle's stand-in model has it
    CHECK(h.metadata && h.metadata->size() == 1 && (*h.metadata)[0].second == "pt");
    // A valid header written in another order is not canonical; bad offsets are refused.
    auto header = [](const std::string& j) {
        std::string s(8, '\0');
        uint64_t n = j.size();
        for (int i = 0; i < 8; ++i, n >>= 8) s[size_t(i)] = char(n & 0xFF);
        return s + j;
    };
    auto parse_text = [](const std::string& s) { return st::parse_header(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(s.data()), s.size())); };
    const std::string two = header(R"({"b":{"dtype":"F16","shape":[2],"data_offsets":[0,4]},"a":{"dtype":"F16","shape":[1],"data_offsets":[4,6]}})");
    CHECK(parse_text(two).tensors.size() == 2 && !st::is_canonical(parse_text(two)));
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"F16","shape":[2],"data_offsets":[0,6]}})")); }));          // offsets not its shape
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"F16","shape":[1],"data_offsets":[2,4]}})")); }));          // a gap
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"F16","shape":[2],"data_offsets":[0,4]},"b":{"dtype":"F16","shape":[1],"data_offsets":[2,4]}})")); })); // overlap
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"Q4","shape":[1],"data_offsets":[0,1]}})")); }));            // unknown type
    // One name twice, side by side in the data and with another between (the second was let through
    // when the names were compared in the data's order).
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"F16","shape":[1],"data_offsets":[0,2]},"a":{"dtype":"F16","shape":[1],"data_offsets":[2,4]}})")); }));
    CHECK(throws([&] { (void)parse_text(header(R"({"a":{"dtype":"F16","shape":[1],"data_offsets":[0,2]},"b":{"dtype":"F16","shape":[1],"data_offsets":[2,4]},"a":{"dtype":"F16","shape":[1],"data_offsets":[4,6]}})")); }));
    // The real model's start, from its config alone: the first 32,672 bytes of SmolLM2-360M-Instruct's
    // model.safetensors (commit a10cc15), whose SHA-256 is pinned here.
    const auto smol = sieve::json::parse(R"({"hidden_size":960,"intermediate_size":2560,"model_type":"llama","num_attention_heads":15,
        "num_hidden_layers":32,"num_key_value_heads":5,"tie_word_embeddings":true,"torch_dtype":"bfloat16","vocab_size":49152,
        "attention_bias":false,"mlp_bias":false})");
    const std::string smol_start = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, st::llama_tensors(smol));
    CHECK(smol_start.size() == 32672);
    CHECK(Sha256::hex(Sha256::hash(smol_start)) == "3f17a5d8f1cc3c6a2d2a7b4ca43d526608b2d7c8c772c70939d4753883a9bf35");
    // Statistics: 1, 1, -1, 0 in BF16.
    {
        st::StatsBuilder b("BF16");
        const uint8_t v[] = {0x80, 0x3F, 0x80, 0x3F, 0x80, 0xBF, 0x00, 0x00};
        b.add(v);
        const st::TensorStats s = b.finish();
        CHECK(s.values == 4 && s.distinct == 3);
        CHECK(std::fabs(s.h_value - 1.5) < 1e-12 && std::fabs(s.h_high - 1.5) < 1e-12);
        CHECK(std::fabs(s.h_low - (-(0.75 * std::log2(0.75) + 0.25 * std::log2(0.25)))) < 1e-12);
        CHECK(s.mean == 0.25 && std::fabs(s.sd - std::sqrt(0.6875)) < 1e-12);
        CHECK(throws([] { st::StatsBuilder bad("F32"); }));
    }
    CHECK(st::half_to_double(0x3C00, false) == 1.0 && st::half_to_double(0x0001, false) == std::ldexp(1.0, -24) && st::half_to_double(0xC000, false) == -2.0);
    CHECK(st::half_to_double(0x3F80, true) == 1.0);
}

void test_weights()
{
    namespace sw = sieve::weights;
    // The table: every value present at least 1, the rest of 2^24 to the most frequent (the lowest on a tie).
    {
        std::vector<uint64_t> c(65536, 0);
        c[7] = 5;
        c[9] = 5;
        c[300] = 1;
        const auto f = sw::quantise(c);
        uint64_t sum = 0;
        for (const uint32_t v : f) sum += v;
        CHECK(sum == (uint64_t(1) << 24));
        CHECK(f[300] >= 1 && f[7] > f[9] && f[0] == 0);
        CHECK(f[9] == 1 + 5 * ((uint64_t(1) << 24) - 3) / 11);
        CHECK(throws([] { (void)sw::quantise(std::vector<uint64_t>(65536, 0)); }));
    }
    // Coding and back: skewed values, one value alone, and none.
    {
        std::mt19937 rng(5);
        std::vector<uint16_t> v(20000);
        std::vector<uint64_t> c(65536, 0);
        for (auto& x : v)
        {
            const double g = std::normal_distribution<double>(0, 40)(rng);
            x = uint16_t(int(std::lround(g)) & 0xFFFF);
            ++c[x];
        }
        const auto f = sw::quantise(c);
        const auto words = sw::encode(v, f);
        CHECK(sw::decode(words, f, v.size()) == v);
        auto broken = words;
        broken[words.size() / 2] ^= 1;
        CHECK(throws([&] { (void)sw::decode(broken, f, v.size()); }) || sw::decode(broken, f, v.size()) != v);
        std::vector<uint64_t> one(65536, 0);
        one[42] = 9;
        const std::vector<uint16_t> same(9, 42);
        CHECK(sw::decode(sw::encode(same, sw::quantise(one)), sw::quantise(one), 9) == same);
        CHECK(sw::encode(std::vector<uint16_t>{}, sw::quantise(one)).size() == 2);
        CHECK(throws([&] { (void)sw::encode(std::vector<uint16_t>{43}, sw::quantise(one)); })); // a value the table lacks
    }
    CHECK(sw::kind_of("model.layers.12.mlp.up_proj.weight") == "model.layers.*.mlp.up_proj.weight");
    CHECK(sw::kind_of("model.norm.weight") == "model.norm.weight");
    // A whole file: a small Llama model with an F32 tensor kept as it is, packed and rebuilt.
    {
        namespace st = sieve::safetensors;
        auto tensors = st::llama_tensors(sieve::json::parse(R"({"model_type":"llama","hidden_size":16,"intermediate_size":32,"num_attention_heads":2,
            "num_hidden_layers":2,"num_key_value_heads":1,"tie_word_embeddings":false,"torch_dtype":"bfloat16","vocab_size":40})"));
        st::Tensor extra;
        extra.name = "extra.scale";
        extra.dtype = "F32";
        extra.shape = {3};
        tensors.push_back(extra);
        std::vector<st::Tensor> laid;
        std::string file = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, tensors, &laid);
        std::mt19937 rng(11);
        for (const st::Tensor& t : laid)
            for (uint64_t i = 0; i < t.bytes(); i += 2)
            {
                const float x = float(std::normal_distribution<double>(0, 0.1)(rng));
                uint32_t u;
                std::memcpy(&u, &x, 4);
                file.push_back(char((u >> 16) & 0xFF));
                file.push_back(char(u >> 24));
            }
        std::istringstream in(file);
        std::ostringstream packed;
        const sw::PackReport r = sw::pack(in, packed);
        CHECK(r.in_bytes == file.size() && r.out_bytes == packed.str().size());
        CHECK(r.kinds == 12 && r.raw_tensors == 1 && r.coded_tensors == tensors.size() - 1); // untied: lm_head is a kind of its own
        CHECK(Sha256::hex(r.sha256) == Sha256::hex(Sha256::hash(file)));
        std::istringstream back(packed.str());
        std::ostringstream rebuilt;
        const sw::UnpackReport u = sw::unpack(back, rebuilt);
        CHECK(rebuilt.str() == file && u.bytes == file.size());
        // A changed byte in the coded part, a missing tail or a byte too many are all refused.
        std::string bad = packed.str();
        bad[bad.size() - 40] ^= 0x10;
        std::istringstream b1(bad);
        std::ostringstream o1;
        CHECK(throws([&] { (void)sw::unpack(b1, o1); }));
        std::istringstream b2(packed.str().substr(0, packed.str().size() - 1));
        std::ostringstream o2;
        CHECK(throws([&] { (void)sw::unpack(b2, o2); }));
        std::istringstream b3(packed.str() + "x");
        std::ostringstream o3;
        CHECK(throws([&] { (void)sw::unpack(b3, o3); }));
        std::istringstream b4(std::string("not weights"));
        std::ostringstream o4;
        CHECK(throws([&] { (void)sw::unpack(b4, o4); }));
    }
}

void test_llm(const std::string& dir)
{
    namespace L = sieve::llm;
    // Unicode classes.
    CHECK(sieve::unicode::is_letter(U'a') && sieve::unicode::is_letter(U'é') && sieve::unicode::is_letter(U'日') && !sieve::unicode::is_letter(U'1'));
    CHECK(sieve::unicode::is_number(U'7') && sieve::unicode::is_number(U'½') && sieve::unicode::is_number(U'٣') && !sieve::unicode::is_number(U'x'));
    CHECK(sieve::unicode::is_white_space(U'　') && !sieve::unicode::is_white_space(U'​'));
    // GPT-2's byte characters.
    CHECK(L::byte_char(' ') == U'Ġ' && L::byte_char('A') == U'A' && L::byte_char(0) == U'Ā');
    for (int b = 0; b < 256; ++b) CHECK(L::char_byte(L::byte_char(uint8_t(b))) == uint8_t(b));
    // The stand-in tokenizer (the oracle's llm-tokenizer-fixture).
    std::ifstream f(dir + "llama_tiny_tokenizer.json", std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    const L::Tokenizer tok(sieve::json::parse(ss.str()));
    CHECK(tok.size() == 300);
    CHECK(tok.find("<|im_end|>") == 2u && tok.is_added(2) && !tok.is_added(3));
    const std::vector<std::string> pieces = tok.pieces("it's 2026,  the  end\n\nok");
    // The last of a run of spaces goes with what follows; a newline cannot, so it stands alone.
    CHECK((pieces == std::vector<std::string>{"it", "'s", " ", "2", "0", "2", "6", ",", " ", " the", " ", " end", "\n", "\n", "ok"}));
    for (const char* text : {"The library holds every book.", "  spaces\tand\nlines  ", "caf\xc3\xa9 \xe6\x97\xa5\xe6\x9c\xac \xf0\x9f\x98\x80", "1,000.5", ""})
        CHECK(tok.decode(tok.encode(text, true)) == text);
    const auto ids = tok.encode("<|im_start|>user\nhi<|im_end|>", true);
    CHECK(ids.front() == 1u && ids.back() == 2u);
    CHECK(tok.encode("<|im_end|>", false).size() > 1); // as text, not the token
    CHECK(tok.encode(" the", true).size() == 1);         // learnt merges: " the" is one token
    CHECK(throws([] { (void)L::Tokenizer(sieve::json::parse(R"({"normalizer":{"type":"NFC"},"pre_tokenizer":{"type":"ByteLevel"},"model":{"type":"BPE","vocab":{},"merges":[]}})")); }));

    // The stand-in model: one written to a file, run with one thread and with four.
    namespace st = sieve::safetensors;
    const auto config = sieve::json::parse(R"({"model_type":"llama","hidden_size":32,"intermediate_size":48,"num_attention_heads":4,
        "num_hidden_layers":2,"num_key_value_heads":2,"tie_word_embeddings":true,"torch_dtype":"bfloat16","vocab_size":300,"max_position_embeddings":64})");
    std::vector<st::Tensor> laid;
    std::string file = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, st::llama_tensors(config), &laid);
    std::mt19937 rng(3);
    for (const st::Tensor& t : laid)
        for (uint64_t i = 0; i < t.bytes(); i += 2)
        {
            const bool norm = t.name.find("norm") != std::string::npos;
            const float x = float((norm ? 1.0 : 0.0) + std::normal_distribution<double>(0, norm ? 0.05 : 0.1)(rng));
            uint32_t u;
            std::memcpy(&u, &x, 4);
            file.push_back(char((u >> 16) & 0xFF));
            file.push_back(char(u >> 24));
        }
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "sieve_test_llm.safetensors";
    {
        std::ofstream out(path, std::ios::binary);
        out.write(file.data(), std::streamsize(file.size()));
    }
    {
        L::Model one(config, path, 1), four(config, path, 4);
        CHECK(one.config().head_dim == 8 && one.config().kv_heads == 2);
        std::vector<float> a, b;
        for (const uint32_t t : tok.encode("The library holds every book.", true))
        {
            a = one.step(t);
            b = four.step(t);
        }
        CHECK(a.size() == 300 && a == b); // the same to the bit, whatever the threads
        one.reset();
        std::vector<float> again;
        for (const uint32_t t : tok.encode("The library holds every book.", true)) again = one.step(t);
        CHECK(again == a);
        CHECK(throws([&] { (void)one.step(300); }));
        // Sampling: greedy is the likeliest; a seed gives the same choices.
        L::Sampling greedy;
        greedy.temperature = 0;
        L::Sampler g(greedy);
        CHECK(g.pick(a, {}) == uint32_t(std::max_element(a.begin(), a.end()) - a.begin()));
        L::Sampling warm;
        warm.temperature = 1.0f;
        warm.top_p = 1.0f;
        warm.seed = 9;
        L::Sampler s1(warm), s2(warm);
        std::vector<uint32_t> p1, p2;
        for (int i = 0; i < 20; ++i)
        {
            p1.push_back(s1.pick(a, {}));
            p2.push_back(s2.pick(a, {}));
        }
        CHECK(p1 == p2);
        L::Sampling k1 = warm;
        k1.top_k = 1;
        L::Sampler s3(k1);
        CHECK(s3.pick(a, {}) == g.pick(a, {}));
        // A chat: the reply stops within its budget, and the turn is closed.
        L::Model m(config, path, 2);
        L::Chat chat(m, tok, "test");
        L::Sampler sg(greedy);
        std::string streamed;
        const std::string r = chat.reply("hi", sg, 6, [&](const std::string& piece) {
            streamed += piece;
            return true;
        });
        CHECK(streamed == r && chat.last_reply_tokens() <= 6);
        CHECK(throws([&] { (void)L::Model(sieve::json::parse(R"({"model_type":"gpt2"})"), path, 1); }));
        // Kept as stored or widened as they load (widen_weights): the same logits to the bit, at
        // half the memory stored.
        L::set_widen_weights(true);
        L::Model wide(config, path, 1);
        L::set_widen_weights(false);
        std::vector<float> w;
        for (const uint32_t t : tok.encode("The library holds every book.", true)) w = wide.step(t);
        CHECK(w == a);
        CHECK(wide.weight_bytes() == 2 * one.weight_bytes());
        CHECK(wide.widened() && !one.widened());
        // "auto": widened where the widened model fits the budget, as stored where it does not.
        L::set_widen_weights_auto(uint64_t(1) << 40);
        L::Model roomy(config, path, 1);
        L::set_widen_weights_auto(one.weight_bytes()); // (twice that is needed)
        L::Model tight(config, path, 1);
        L::set_widen_weights(false);
        CHECK(roomy.widened() && roomy.weight_bytes() == wide.weight_bytes());
        CHECK(!tight.widened() && tight.weight_bytes() == one.weight_bytes());
    }
    std::filesystem::remove(path);
    // The same model in F16: kept as stored (two bytes a value, no longer widened as it loads) and
    // widened give the same logits to the bit, every F16 value being a 32-bit float exactly.
    {
        const auto config16 = sieve::json::parse(R"({"model_type":"llama","hidden_size":32,"intermediate_size":48,"num_attention_heads":4,
            "num_hidden_layers":2,"num_key_value_heads":2,"tie_word_embeddings":true,"torch_dtype":"float16","vocab_size":300,"max_position_embeddings":64})");
        std::vector<st::Tensor> laid16;
        std::string file16 = st::layout_start(std::vector<std::pair<std::string, std::string>>{{"format", "pt"}}, st::llama_tensors(config16), &laid16);
        CHECK(!laid16.empty() && laid16.front().dtype == "F16");
        // A float to the nearest F16 (normal values only, which these all are; smaller ones to 0).
        auto to_half = [](float x) {
            uint32_t u;
            std::memcpy(&u, &x, 4);
            const uint32_t sign = (u >> 16) & 0x8000u, m = u & 0x7FFFFFu;
            const int e = int((u >> 23) & 0xFFu) - 127 + 15;
            if (e <= 0) return uint16_t(sign);
            uint32_t h = (uint32_t(e) << 10) | (m >> 13);
            if (m & 0x1000u) ++h;
            return uint16_t(sign | h);
        };
        std::mt19937 rng16(3);
        for (const st::Tensor& t : laid16)
            for (uint64_t i = 0; i < t.bytes(); i += 2)
            {
                const bool norm = t.name.find("norm") != std::string::npos;
                const uint16_t h = to_half(float((norm ? 1.0 : 0.0) + std::normal_distribution<double>(0, norm ? 0.05 : 0.1)(rng16)));
                file16.push_back(char(h & 0xFF));
                file16.push_back(char(h >> 8));
            }
        const std::filesystem::path path16 = std::filesystem::temp_directory_path() / "sieve_test_llm16.safetensors";
        {
            std::ofstream out(path16, std::ios::binary);
            out.write(file16.data(), std::streamsize(file16.size()));
        }
        {
            L::Model stored(config16, path16, 1);
            L::set_widen_weights(true);
            L::Model widened(config16, path16, 3);
            L::set_widen_weights(false);
            std::vector<float> a16, b16;
            for (const uint32_t t : tok.encode("The library holds every book.", true))
            {
                a16 = stored.step(t);
                b16 = widened.step(t);
            }
            CHECK(a16.size() == 300 && a16 == b16);
            CHECK(widened.weight_bytes() == 2 * stored.weight_bytes());
        }
        std::filesystem::remove(path16);
    }
}

// training-v1 (SPECIFICATIONS §12.0e): the oracle's steps on small shapes, the functions, the same
// result for any number of threads, and the files of a run read back.
void test_training(const std::string& dir)
{
    using namespace sieve::training;
    std::ifstream cin(dir + "training_corpus.txt", std::ios::binary);
    CHECK(bool(cin));
    const std::string corpus((std::istreambuf_iterator<char>(cin)), std::istreambuf_iterator<char>());
    // Every inner-loop path, plain and (where the processor has them) AVX2 and AVX-512: the same steps.
    for (const int level : {0, 1, 2})
    {
        if (!use_kernels(level)) continue;
        std::ifstream in(dir + "vectors_training_v1.tsv");
        CHECK(bool(in));
        struct Case
        {
            std::string shape;
            std::unique_ptr<AiSpace> space;
            std::unique_ptr<Trainer> one, many;
            std::vector<Window> order;
            uint32_t batch = 0, at = 0;
        };
        std::vector<Case> cases;
        auto split = [](const std::string& line, char sep) {
            std::vector<std::string> f;
            std::stringstream ss(line);
            std::string x;
            while (std::getline(ss, x, sep)) f.push_back(x);
            return f;
        };
        std::string line;
        int steps = 0, functions = 0;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#') continue;
            const std::vector<std::string> f = split(line, '\t');
            if (f[0] == "exp" || f[0] == "sigmoid" || f[0] == "sincos" || f[0] == "pow2q32")
            {
                std::vector<std::string> got;
                if (f[0] == "exp")
                    for (int64_t z : {int64_t(0), int64_t(-1), int64_t(-65536), int64_t(-300000), int64_t(-5000000)})
                        got.push_back(std::to_string(fx::exp_q16(z)));
                if (f[0] == "sigmoid")
                    for (int64_t x : {int64_t(0), int64_t(70000), int64_t(-70000), int64_t(8000000), int64_t(-8000000)})
                        got.push_back(std::to_string(fx::sigmoid_q16(x)));
                if (f[0] == "sincos")
                    for (int64_t a : {int64_t(0), int64_t(1) << 32, int64_t(7) << 32, int64_t(511) << 32, int64_t(6746518852)})
                    {
                        int64_t s, c;
                        fx::sincos_q16(a, s, c);
                        got.push_back(std::to_string(s) + "," + std::to_string(c));
                    }
                if (f[0] == "pow2q32")
                    for (int64_t y : {int64_t(0), int64_t(-1), -(int64_t(1) << 31), int64_t(-57070290109)})
                        got.push_back(std::to_string(fx::pow2_q32(y)));
                CHECK(split(f[1], ' ') == got);
                ++functions;
                continue;
            }
            CHECK(f.size() == 9);
            if (f.size() != 9) continue;
            if (cases.empty() || cases.back().shape != f[0] + f[1])
            {
                Case c;
                c.shape = f[0] + f[1];
                AiShape shape;
                CHECK(std::sscanf(f[0].c_str(), "L%u/H%u/A%u/B%u", &shape.layers, &shape.width, &shape.heads, &shape.bits) == 4);
                c.space = std::make_unique<AiSpace>(shape, "sieve");
                Recipe r;
                r.version = 2; // (training-v2 at a loss scale of 0: training-v1's arithmetic, its vectors)
                r.shape = shape;
                r.batch = c.batch = uint32_t(std::stoul(f[2]));
                r.learning_rate = f[3];
                r.momentum = f[4];
                const AiSpace::Digits start = c.space->digits_at(BigUint(0), AddressMode::Scrambled);
                c.one = std::make_unique<Trainer>(*c.space, start, r, 1);
                c.many = std::make_unique<Trainer>(*c.space, start, r, 3);
                c.order = make_order({{"c", corpus.size()}}, uint32_t(std::stoul(f[1])), 1, 1);
                cases.push_back(std::move(c));
            }
            Case& c = cases.back();
            std::vector<std::span<const uint8_t>> batch;
            for (uint32_t i = c.at; i < std::min<size_t>(c.order.size(), c.at + c.batch); ++i)
                batch.push_back({reinterpret_cast<const uint8_t*>(corpus.data()) + c.order[i].offset, c.order[i].length});
            c.at += c.batch;
            const Score s1 = c.one->step(batch), s2 = c.many->step(batch);
            CHECK(std::to_string(s1.prob_sum) == f[6] && std::to_string(s1.predictions) == f[7]);
            CHECK(s2.prob_sum == s1.prob_sum && s2.predictions == s1.predictions);
            const AiSpace::Digits& d = c.one->digits();
            CHECK(Sha256::hex(Sha256::hash(std::string(d.begin(), d.end()))) == f[8]);
            CHECK(c.many->digits() == d);
            ++steps;
        }
        CHECK(steps == 9 && functions == 4);
        std::cout << "training vectors checked: " << steps << " steps, " << functions << " functions, inner loops "
                  << kernels_name() << std::endl;
    }
    use_vector_kernels(true);

    // training-v2: the loss scaled by 2^k and the rate held to 16 + k bits (one rate below v1's floor);
    // training-v3, the same cases with every byte's probability floored and capped.
    for (const auto& [vectors, version] : {std::pair<const char*, int>{"vectors_training_v2.tsv", 2}, {"vectors_training_v3.tsv", 3}})
    {
        std::ifstream in2(dir + vectors);
        CHECK(bool(in2));
        std::string key;
        std::unique_ptr<AiSpace> space;
        std::unique_ptr<Trainer> one, many;
        std::vector<Window> order2;
        uint32_t batch = 0, at = 0;
        int rows = 0;
        std::string row;
        while (std::getline(in2, row))
        {
            if (row.empty() || row[0] == '#') continue;
            std::vector<std::string> f;
            {
                std::stringstream ss(row);
                std::string x;
                while (std::getline(ss, x, '\t')) f.push_back(x);
            }
            CHECK(f.size() == 10);
            if (f.size() != 10) continue;
            if (key != f[0] + f[1])
            {
                key = f[0] + f[1];
                AiShape shape;
                CHECK(std::sscanf(f[0].c_str(), "L%u/H%u/A%u/B%u", &shape.layers, &shape.width, &shape.heads, &shape.bits) == 4);
                space = std::make_unique<AiSpace>(shape, "sieve");
                Recipe r;
                r.version = version;
                r.shape = shape;
                r.batch = batch = uint32_t(std::stoul(f[2]));
                r.learning_rate = f[3];
                r.momentum = f[4];
                r.loss_scale_bits = uint32_t(std::stoul(f[5]));
                const AiSpace::Digits start = space->digits_at(BigUint(0), AddressMode::Scrambled);
                one = std::make_unique<Trainer>(*space, start, r, 1);
                many = std::make_unique<Trainer>(*space, start, r, 3);
                order2 = make_order({{"c", corpus.size()}}, uint32_t(std::stoul(f[1])), 1, 1);
                at = 0;
            }
            std::vector<std::span<const uint8_t>> b;
            for (uint32_t i = at; i < std::min<size_t>(order2.size(), at + batch); ++i)
                b.push_back({reinterpret_cast<const uint8_t*>(corpus.data()) + order2[i].offset, order2[i].length});
            at += batch;
            const Score s1 = one->step(b), s2 = many->step(b);
            CHECK(std::to_string(s1.prob_sum) == f[7] && std::to_string(s1.predictions) == f[8] && s2.prob_sum == s1.prob_sum);
            const AiSpace::Digits& d = one->digits();
            CHECK(Sha256::hex(Sha256::hash(std::string(d.begin(), d.end()))) == f[9] && many->digits() == d);
            ++rows;
        }
        CHECK(rows == 10);
        std::cout << "training-v" << version << " vectors checked: " << rows << " steps" << std::endl;
    }
    // The update's product, exact past 64 bits.
    CHECK(fx::mul_shift(int64_t(1) << 53, int64_t(1) << 23, 23) == int64_t(1) << 53);
    // -(2^53 + 1)(2^23 + 5) / 2^23 = -(2^53 + 5 * 2^30 + 1 + 5 / 2^23), so its floor is -(2^53 + 5 * 2^30 + 2).
    CHECK(fx::mul_shift(-(int64_t(1) << 53) - 1, (int64_t(1) << 23) + 5, 23) == -9007204623450114LL);
    CHECK(fx::mul_shift(-7, 3, 1) == -11 && fx::mul_shift(7, 3, 1) == 10 && fx::mul_shift(-6, 1, 1) == -3);

    // A run's files, read back as written.
    Recipe r;
    r.start = "1f";
    r.start_mode = AddressMode::Positional;
    r.learning_rate = "0.0005";
    r.files = {"corpus/a.txt"};
    CHECK(Recipe::parse(r.text()).text() == r.text());
    CHECK(Recipe::parse(r.text()).learning_rate_q16() == 33); // round(0.0005 * 65536) = 32.768
    bool refused = false;
    // (9 November 2026: training-v3, the probabilities floored, is the default and is read back; a
    // version past it is refused)
    CHECK(r.version == 3 && Recipe::parse(r.text()).version == 3);
    std::string v4 = r.text();
    v4.replace(v4.find("training-v3"), 11, "training-v4");
    try { Recipe::parse(v4); }
    catch (const std::invalid_argument&) { refused = true; }
    CHECK(refused);
    refused = false;
    r.batch = 33;
    try { r.check(); }
    catch (const std::invalid_argument&) { refused = true; }
    CHECK(refused);
    const std::vector<Window> order = make_order({{"corpus/a.txt", 100}, {"corpus/b.txt", 3}}, 33, 2, 7);
    CHECK(order.size() == 2 * (4 + 1)); // 100 bytes: 0, 32, 64, 96 (4 bytes); 3 bytes: 0
    CHECK(parse_order(order_text(order)).size() == order.size());
    CHECK(order_text(parse_order(order_text(order))) == order_text(order));
    // The three readings: every window once an epoch, the files in the order given.
    const std::vector<std::pair<std::string, uint64_t>> two = {{"corpus/b.txt", 100}, {"corpus/a.txt", 40}};
    const std::vector<Window> in_order = make_order(two, 33, 2, 7, Reading::InOrder);
    const std::vector<Window> within = make_order(two, 33, 2, 7, Reading::ShuffledWithinFiles);
    const std::vector<Window> mixed = make_order(two, 33, 2, 7, Reading::Shuffled);
    CHECK(in_order.size() == 2 * (4 + 2) && within.size() == in_order.size() && mixed.size() == in_order.size());
    CHECK(in_order[0].path == "corpus/b.txt" && in_order[0].offset == 0 && in_order[3].offset == 96 && in_order[4].path == "corpus/a.txt");
    bool blocks = true; // shuffled within files: b's four windows, then a's two, each epoch
    for (size_t i = 0; i < within.size(); ++i) blocks = blocks && within[i].path == (i % 6 < 4 ? "corpus/b.txt" : "corpus/a.txt");
    CHECK(blocks);
    CHECK(order_text(make_order(two, 33, 2, 7)) == order_text(mixed)); // shuffled is the default, as the first runs had it
    // A recipe of run format 2, read back; one of the first runs' format keeps its form.
    Recipe r2;
    r2.reading = Reading::ShuffledWithinFiles;
    r2.files = {"corpus/b.txt", "corpus/a.txt"};
    r2.checkpoint_addresses = true;
    CHECK(Recipe::parse(r2.text()).text() == r2.text());
    CHECK(Recipe::parse(r2.text()).files == r2.files && Recipe::parse(r2.text()).result_version() == 2);
    Recipe r1 = r2;
    r1.order.clear();
    CHECK(Recipe::parse(r1.text()).result_version() == 1 && Recipe::parse(r1.text()).text() == r1.text());
    Result res;
    res.version = 1;
    res.steps = 3;
    res.checkpoints = {{0, {}, {}, "00ff"}, {3, {12345, 64}, {}, "0100"}};
    res.final_address = "0100";
    res.model_sha256 = std::string(64, 'a');
    CHECK(Result::parse(res.text()).text() == res.text());
    res.version = 2;
    res.checkpoints = {{0, {}, std::string(64, 'b'), {}}, {3, {12345, 64}, std::string(64, 'c'), "0100"}};
    CHECK(Result::parse(res.text()).text() == res.text());
    CHECK(Result::parse(res.text()).checkpoints[0].address.empty() && Result::parse(res.text()).checkpoints[1].address == "0100");
}

void test_ai_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_ai_v1.tsv");
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
        CHECK(f.size() == 6);
        if (f.size() != 6) continue;
        AiShape shape;
        CHECK(std::sscanf(f[0].c_str(), "L%u/H%u/A%u/B%u", &shape.layers, &shape.width, &shape.heads, &shape.bits) == 4);
        const AiSpace space(shape, "sieve");
        const AddressMode m = address_mode_from_string(f[1]);
        const BigUint index = unit_at_bearing(BigUint(uint64_t(std::stoul(f[2]))), space.size(), 0);
        CHECK(Sha256::hex(Sha256::hash(space.hex_of(index))) == f[3]);
        const AiSpace::Digits d = space.digits_at(index, m);
        CHECK(Sha256::hex(Sha256::hash(std::string(d.begin(), d.end()))) == f[4]);
        const std::string file = space.safetensors_of(d);
        CHECK(Sha256::hex(Sha256::hash(file)) == f[5]);
        CHECK(space.index_of(d, m) == index);
        const auto back = space.digits_of_safetensors(file);
        CHECK(back && *back == d);
        ++n;
    }
    CHECK(n == 30);
    // A model off the grid, or of another shape, has no address; a bad shape is refused.
    {
        const AiSpace space(AiShape{}, "sieve");
        CHECK(space.weights() == 8192 && space.bits() == 32768 && space.hex_width() == 8192);
        AiSpace::Digits d(static_cast<size_t>(space.weights()), 3);
        std::string file = space.safetensors_of(d);
        CHECK(space.digits_of_safetensors(file) == d);
        file[file.size() - 1] ^= 0x01; // a weight nudged off its value
        CHECK(!space.digits_of_safetensors(file));
        const AiSpace other(AiShape{1, 8, 2, 4}, "sieve");
        CHECK(!other.digits_of_safetensors(space.safetensors_of(d)));
        CHECK(throws([] { AiShape{1, 16, 3, 4}.check(); }));
        CHECK(throws([] { AiShape{1, 16, 2, 9}.check(); }));
        CHECK(throws([&] { (void)space.index_of(AiSpace::Digits(5, 0), AddressMode::Positional); }));
        d[0] = 16;
        CHECK(throws([&] { (void)space.index_of(d, AddressMode::Positional); }));
        // The values: evenly spaced, the embedding over [-1, 1].
        const AiSpace::Tensor& e = space.tensors().front();
        CHECK(e.name == "model.embed_tokens.weight" && space.value(e, 0) == -1.0f && space.value(e, 15) == 1.0f);
        // A model off the shelf talks: the engine runs it from memory.
        sieve::llm::Model model(space.config(), space.tensors_of(space.digits_at(BigUint(12345), AddressMode::Scrambled)), 1);
        const sieve::llm::Tokenizer tok(sieve::json::parse(AiSpace::tokenizer_json()));
        CHECK(tok.size() == 256 && tok.encode("ab", false) == (std::vector<uint32_t>{97, 98}));
        sieve::llm::Sampling greedy;
        greedy.temperature = 0;
        sieve::llm::Sampler sampler(greedy);
        CHECK(sieve::llm::continue_text(model, tok, "hello", sampler, 10).size() >= 1);
    }
}

// The AI line's filters (SPECIFICATIONS §12.0f). weight-rows-v1 against brute force: a shape whose
// rows are 2 and 8 digits of 4 (every row of 8 enumerated), at several settings; its ranker's own
// rank and unrank against the generic walk over next and completions. canonical-model-v1: the
// canonical form is canonical, every symmetry leaves the logits as they were (to a float's last
// bits), and the count agrees with its log.
void test_ai_filters()
{
    const AiSpace space(AiShape{1, 2, 1, 2}, "sieve"); // rows of 2 (embedding, attention, gate, up) and 8 (down)
    sieve::FilterLine line;
    line.kind = "ai";
    line.symbols_id = space.id();
    line.base = 4;
    line.length = uint32_t(space.weights());
    line.ai = &space;
    struct Rule
    {
        uint32_t run, period;
        bool ramps;
    };
    for (const Rule rule : {Rule{2, 3, true}, Rule{0, 4, true}, Rule{3, 0, false}, Rule{16, 4, true}, Rule{1, 1, true}})
    {
        auto passes_row = [&](const std::vector<uint32_t>& r) {
            uint32_t run = 0;
            for (size_t i = 0; i < r.size(); ++i)
            {
                run = i > 0 && r[i] == r[i - 1] ? run + 1 : 1;
                if (rule.run && run > rule.run) return false;
            }
            for (uint32_t p = 1; p <= rule.period && 2 * p <= r.size(); ++p)
            {
                bool rep = true;
                for (size_t i = p; i < r.size(); ++i) rep = rep && r[i] == r[i - p];
                if (rep) return false;
            }
            if (rule.ramps && r.size() >= 3)
            {
                const int d = int(r[1]) - int(r[0]);
                bool ramp = d != 0;
                for (size_t i = 2; i < r.size(); ++i) ramp = ramp && int(r[i]) - int(r[i - 1]) == d;
                if (ramp) return false;
            }
            return true;
        };
        auto brute = [&](uint32_t c) {
            uint64_t n = 0;
            std::vector<uint32_t> r(c);
            for (uint64_t v = 0; v < (uint64_t(1) << (2 * c)); ++v)
            {
                for (uint32_t i = 0; i < c; ++i) r[i] = uint32_t(v >> (2 * (c - 1 - i))) & 3;
                n += passes_row(r) ? 1 : 0;
            }
            return n;
        };
        uint64_t rows2 = 0, rows8 = 0;
        for (const AiSpace::Tensor& t : space.tensors()) (t.cols == 2 ? rows2 : rows8) += t.rows;
        CHECK(rows2 == 280 && rows8 == 2);
        const sieve::FilterValues v{{"max_run", std::to_string(rule.run)}, {"period", std::to_string(rule.period)}, {"ramps", rule.ramps ? "on" : "off"}};
        const sieve::FilterStack st(line, {{find_filter("weight-rows-v1"), v}}, TestResources(nullptr, nullptr));
        const sieve::Ranker* r = st.ranker();
        CHECK(r != nullptr);
        if (!r) continue;
        CHECK(r->count() == BigUint::mul(BigUint::pow(BigUint(brute(2)), rows2), BigUint::pow(BigUint(brute(8)), rows8)));
        std::mt19937_64 rng(rule.run * 31 + rule.period);
        BigUint last;
        for (int i = 0; i < 12; ++i)
        {
            BigUint k = BigUint::from_hex(std::to_string(rng() % 1000003));
            k = BigUint::mod(BigUint::mul(k, BigUint::pow(BigUint(7), 300)), r->count());
            const std::vector<uint32_t> u = r->unrank(k);
            CHECK(st.passes(u) && r->accepts(u));
            CHECK(r->rank(u) == k && r->Ranker::rank(u) == k && r->Ranker::unrank(k) == u);
        }
        // Survivors in address order: unrank of k and k + 1 increase.
        const std::vector<uint32_t> a = r->unrank(BigUint(41)), b = r->unrank(BigUint(42));
        CHECK(std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end()));
    }
    // weight-signs-v1 (list N5-16): every row of 2 and of 8 digits counted one by one, and the ranker's rank and unrank
    // against the generic walk.
    for (const auto& [name, percent] : std::vector<std::pair<std::string, int>>{{"weight-signs-v1", 10}, {"weight-signs-v1", 20},
                                                                                 {"weight-signs-v1", 50}, {"weight-signs-v1", 0}})
    {
        const bool signs = name == "weight-signs-v1";
        auto passes_row = [&](const std::vector<uint32_t>& r) {
            uint32_t k = 0;
            for (uint32_t d : r) k += signs ? (d < 2 ? 1u : 0u) : (d == 0 || d == 3 ? 1u : 0u);
            const uint32_t c = uint32_t(r.size());
            if (signs)
            {
                const uint32_t lo = (c * uint32_t(percent) + 99) / 100;
                return k >= lo && c - k >= lo;
            }
            return k <= c * uint32_t(percent) / 100;
        };
        auto brute = [&](uint32_t c) {
            uint64_t n = 0;
            std::vector<uint32_t> r(c);
            for (uint64_t v = 0; v < (uint64_t(1) << (2 * c)); ++v)
            {
                for (uint32_t i = 0; i < c; ++i) r[i] = uint32_t(v >> (2 * (c - 1 - i))) & 3;
                n += passes_row(r) ? 1 : 0;
            }
            return n;
        };
        const sieve::FilterStack st(line, {{find_filter(name), {{"percent", std::to_string(percent)}}}}, TestResources(nullptr, nullptr));
        const sieve::Ranker* r = st.ranker();
        CHECK(r != nullptr);
        if (!r) continue;
        CHECK(r->count() == BigUint::mul(BigUint::pow(BigUint(brute(2)), 280), BigUint::pow(BigUint(brute(8)), 2)));
        if (r->count().is_zero()) continue;
        std::mt19937_64 rng(uint64_t(percent) * 7 + (signs ? 1 : 2));
        for (int i = 0; i < 12; ++i)
        {
            BigUint k = BigUint::from_hex(std::to_string(rng() % 1000003));
            k = BigUint::mod(BigUint::mul(k, BigUint::pow(BigUint(7), 300)), r->count());
            const std::vector<uint32_t> u = r->unrank(k);
            CHECK(st.passes(u) && r->accepts(u));
            CHECK(r->rank(u) == k && r->Ranker::rank(u) == k && r->Ranker::unrank(k) == u);
            // (review 4: the filter judges by its rows' counts alone, without the ranker's tables; a digit changed
            // here and there, the verdict the ranker's)
            std::vector<uint32_t> w = u;
            for (int j = 0; j < 3; ++j) w[size_t(rng() % w.size())] = uint32_t(rng() % 4);
            CHECK(st.passes(w) == r->accepts(w));
        }
    }
    // A trained-looking row passes at the defaults; a ramp, a repeat and a long run do not.
    {
        const AiSpace micro(AiShape{}, "sieve");
        sieve::FilterLine ml = line;
        ml.symbols_id = micro.id();
        ml.base = 16;
        ml.length = uint32_t(micro.weights());
        ml.ai = &micro;
        const sieve::FilterStack st(ml, {{find_filter("weight-rows-v1"), {}}}, TestResources(nullptr, nullptr));
        std::mt19937_64 rng(5);
        std::vector<uint32_t> u(ml.length);
        for (uint32_t& x : u) x = uint32_t(rng() % 16);
        for (size_t i = 0; i + 1 < u.size(); ++i)
            if (u[i] == u[i + 1]) u[i + 1] = (u[i + 1] + 1) % 16; // no runs at all
        CHECK(st.passes(u));
        CHECK(st.ranker() && st.ranker()->unrank(st.ranker()->rank(u)) == u);
        auto with = [&](std::vector<uint32_t> row) {
            std::vector<uint32_t> w = u;
            std::copy(row.begin(), row.end(), w.begin() + 16 * 7); // the eighth row of the embedding
            return st.passes(w);
        };
        std::vector<uint32_t> ramp(16), rep(16);
        for (uint32_t i = 0; i < 16; ++i) ramp[i] = i, rep[i] = (i % 3) * 5;
        CHECK(!with(ramp) && !with(rep) && !with(std::vector<uint32_t>(16, 9)));
        std::reverse(ramp.begin(), ramp.end());
        CHECK(!with(ramp));
        rep[15] = 1;
        CHECK(with(rep));
    }
    // canonical-model-v1.
    for (const AiShape shape : {AiShape{1, 4, 1, 2}, AiShape{2, 8, 2, 3}, AiShape{1, 16, 2, 4}})
    {
        const AiSpace sp(shape, "sieve");
        std::mt19937_64 rng(shape.width);
        for (int i = 0; i < 4; ++i)
        {
            AiSpace::Digits d(static_cast<size_t>(sp.weights()));
            for (uint8_t& x : d) x = uint8_t(rng() & ((1u << shape.bits) - 1));
            const AiSpace::Digits c = sp.canonical_of(d);
            CHECK(sp.is_canonical(c) && sp.canonical_of(c) == c && (c == d) == sp.is_canonical(d));
            CHECK(!sp.is_canonical(d)); // a random model is all but never canonical
            sieve::llm::Model m1(sp.config(), sp.tensors_of(d), 1), m2(sp.config(), sp.tensors_of(c), 1);
            for (uint32_t tok : {72u, 105u, 33u, 10u})
            {
                const std::vector<float> l1 = m1.step(tok), l2 = m2.step(tok);
                float worst = 0, size = 0;
                for (size_t j = 0; j < l1.size(); ++j) worst = std::max(worst, std::fabs(l1[j] - l2[j])), size = std::max(size, std::fabs(l1[j]));
                CHECK(worst <= 1e-4f * std::max(1.0f, size));
            }
        }
        const double exact = double(sp.canonical_count().bit_length());
        CHECK(std::fabs(exact - sp.canonical_log2_count()) < 1.5);
        std::vector<uint32_t> u(size_t(sp.weights()));
        sieve::FilterLine cl = line;
        cl.ai = &sp;
        cl.base = 1u << shape.bits;
        cl.length = uint32_t(sp.weights());
        const sieve::FilterStack st(cl, {{find_filter("canonical-model-v1"), {}}}, TestResources(nullptr, nullptr));
        AiSpace::Digits d(static_cast<size_t>(sp.weights()));
        for (uint8_t& x : d) x = uint8_t(rng() & ((1u << shape.bits) - 1));
        const AiSpace::Digits c = sp.canonical_of(d);
        for (size_t j = 0; j < u.size(); ++j) u[j] = c[j];
        CHECK(st.passes(u));
        for (size_t j = 0; j < u.size(); ++j) u[j] = d[j];
        CHECK(!st.passes(u));
    }
}

// The AI line's filters against the oracle's vectors (sieve_ref.py ai-filter-vectors), which counts
// another way: weight-rows-v1's survivors at several shapes and settings, survivors unranked and
// ranked again, and canonical-model-v1's count and the canonical forms of models off the line.
void test_ai_filter_vectors(const std::string& dir)
{
    std::ifstream in(dir + "vectors_ai_filters_v1.tsv");
    CHECK(bool(in));
    auto shape_of = [](const std::string& t) {
        AiShape sh;
        CHECK(std::sscanf(t.c_str(), "L%u/H%u/A%u/B%u", &sh.layers, &sh.width, &sh.heads, &sh.bits) == 4);
        return sh;
    };
    auto line_of = [](const AiSpace& sp) {
        sieve::FilterLine l;
        l.kind = "ai";
        l.symbols_id = sp.id();
        l.base = 1u << sp.shape().bits;
        l.length = uint32_t(sp.weights());
        l.ai = &sp;
        return l;
    };
    auto rows_values = [](const std::string& t) {
        unsigned run = 0, period = 0;
        char ramps[8] = {};
        CHECK(std::sscanf(t.c_str(), "%u/%u/%7s", &run, &period, ramps) == 3);
        return sieve::FilterValues{{"max_run", std::to_string(run)}, {"period", std::to_string(period)}, {"ramps", ramps}};
    };
    auto sha_hex = [](const BigUint& n) { return Sha256::hex(Sha256::hash(n.to_hex())); };
    std::string line;
    int rows = 0, ranks = 0, counts = 0, models = 0;
    while (std::getline(in, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string x;
        while (std::getline(ss, x, '\t')) f.push_back(x);
        const AiSpace sp(shape_of(f.at(1)), "sieve");
        const sieve::FilterLine fl = line_of(sp);
        if (f[0] == "rows" || f[0] == "rank")
        {
            const sieve::FilterStack st(fl, {{find_filter("weight-rows-v1"), rows_values(f.at(2))}}, TestResources(nullptr, nullptr));
            CHECK(st.ranker() != nullptr);
            if (!st.ranker()) continue;
            if (f[0] == "rows")
            {
                CHECK(std::to_string(st.ranker()->count().bit_length()) == f.at(3) && sha_hex(st.ranker()->count()) == f.at(4));
                ++rows;
            }
            else
            {
                const BigUint k = BigUint::from_hex(f.at(3));
                const std::vector<uint32_t> u = st.ranker()->unrank(k);
                CHECK(Sha256::hex(Sha256::hash(std::string(u.begin(), u.end()))) == f.at(4));
                CHECK(st.passes(u) && st.ranker()->rank(u) == k);
                ++ranks;
            }
        }
        else if (f[0] == "canonical")
        {
            CHECK(std::to_string(sp.canonical_count().bit_length()) == f.at(2) && sha_hex(sp.canonical_count()) == f.at(3));
            ++counts;
        }
        else if (f[0] == "model")
        {
            const BigUint index = unit_at_bearing(BigUint(uint64_t(std::stoul(f.at(3)))), sp.size(), 0);
            const AiSpace::Digits c = sp.canonical_of(sp.digits_at(index, address_mode_from_string(f.at(2))));
            CHECK(Sha256::hex(Sha256::hash(std::string(c.begin(), c.end()))) == f.at(4));
            const sieve::FilterStack st(fl, {{find_filter("canonical-model-v1"), {}}}, TestResources(nullptr, nullptr));
            CHECK(st.passes(std::vector<uint32_t>(c.begin(), c.end())));
            ++models;
        }
    }
    CHECK(rows == 6 && ranks == 5 && counts == 4 && models == 12);
}

// language-v1 and near-model-v1, on a run given by a loader of the test's own: the score is
// Trainer::evaluate's; near-model-v1 counts sum C(W, i) (K - 1)^i, its own rank and unrank agree with
// the generic walk, and survivors come in address order.
void test_ai_run_filters(const std::string& dir)
{
    const AiSpace space(AiShape{1, 2, 1, 2}, "sieve");
    const AiSpace::Digits anchor = space.digits_at(BigUint(987654321), AddressMode::Scrambled);
    const std::string text = "the library holds every book that ever was and ever will be";
    sieve::set_model_run_loader([&](const std::string& id) {
        if (id != "test") throw std::invalid_argument("no run " + id);
        sieve::ModelRun r;
        r.id = id;
        r.layers = 1, r.width = 2, r.heads = 1, r.bits = 2;
        r.final_address = space.hex_of(space.index_of(anchor, AddressMode::Positional));
        r.text_name = "corpus/t.txt";
        r.text.assign(text.begin(), text.end());
        r.sha256 = "x";
        return r;
    });
    sieve::FilterLine line;
    line.kind = "ai";
    line.symbols_id = space.id();
    line.base = 4;
    line.length = uint32_t(space.weights());
    line.ai = &space;
    const TestResources res(nullptr, nullptr);
    // The score is the trainer's, and the threshold is exact.
    {
        sieve::training::Recipe r;
        r.version = 2; // (the filter scores as training-v2's softmax does: its definition, kept)
        r.shape = space.shape();
        const sieve::training::Trainer tr(space, anchor, r, 1);
        const std::vector<std::span<const uint8_t>> w{std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(text.data()), text.size())};
        const sieve::training::Score a = tr.evaluate(w), b = sieve::training::evaluate_model(space, anchor, w);
        CHECK(a.prob_sum == b.prob_sum && a.predictions == b.predictions && a.predictions == text.size() - 1);
        const uint64_t permille = a.prob_sum * 1000 / (a.predictions * 65536); // floor of the mean, in thousandths
        auto passes_at = [&](uint64_t min) {
            const sieve::FilterStack st(line, {{find_filter("language-v1"), {{"run", "test"}, {"bytes", "513"}, {"min_permille", std::to_string(min)}}}}, res);
            return st.passes(std::vector<uint32_t>(anchor.begin(), anchor.end()));
        };
        CHECK(passes_at(permille) && !passes_at(permille + 1));
    }
    for (const uint32_t k : {0u, 1u, 2u, 3u})
    {
        const sieve::FilterStack st(line, {{find_filter("near-model-v1"), {{"run", "test"}, {"changes", std::to_string(k)}}}}, res);
        const sieve::Ranker* r = st.ranker();
        CHECK(r != nullptr);
        if (!r) continue;
        BigUint want, term(1);
        for (uint32_t i = 0; i <= k; ++i)
        {
            if (i) term = BigUint::mul(term, BigUint(uint64_t(space.weights() - i + 1) * 3)), term = [&] { BigUint q, m; BigUint::divmod(term, BigUint(i), q, m); return q; }();
            want += term;
        }
        CHECK(r->count() == want);
        const std::vector<uint32_t> a(anchor.begin(), anchor.end());
        CHECK(st.passes(a) && r->accepts(a));
        std::mt19937_64 rng(k + 3);
        std::vector<uint32_t> prev;
        for (int i = 0; i < 10; ++i)
        {
            const BigUint idx = BigUint::mod(BigUint(rng()), r->count());
            const std::vector<uint32_t> u = r->unrank(idx);
            uint32_t d = 0;
            for (size_t j = 0; j < u.size(); ++j) d += u[j] != a[j] ? 1 : 0;
            CHECK(d <= k && st.passes(u));
            CHECK(r->rank(u) == idx && r->Ranker::rank(u) == idx && r->Ranker::unrank(idx) == u);
        }
        if (k > 0)
        {
            const std::vector<uint32_t> x = r->unrank(BigUint(5)), y = r->unrank(BigUint(6));
            CHECK(std::lexicographical_compare(x.begin(), x.end(), y.begin(), y.end()));
            std::vector<uint32_t> far = a;
            for (uint32_t j = 0; j <= k; ++j) far[j * 7] = (far[j * 7] + 1) % 4;
            CHECK(!st.passes(far));
        }
    }
    // The oracle's vectors (sieve_ref.py ai-run-filter-vectors), written apart from the C++.
    {
        std::ifstream in(dir + "vectors_ai_run_filters_v1.tsv"), corpus(dir + "training_corpus.txt", std::ios::binary);
        CHECK(bool(in) && bool(corpus));
        std::vector<uint8_t> head(65);
        corpus.read(reinterpret_cast<char*>(head.data()), 65);
        std::string row;
        int nears = 0, langs = 0;
        while (std::getline(in, row))
        {
            if (row.empty() || row[0] == '#') continue;
            std::vector<std::string> f;
            std::stringstream ss(row);
            for (std::string x; std::getline(ss, x, '\t');) f.push_back(x);
            if (f[0] == "near")
            {
                const sieve::FilterStack st(line, {{find_filter("near-model-v1"), {{"run", "test"}, {"changes", f.at(2)}}}}, res);
                const BigUint& n = st.ranker()->count();
                CHECK(std::to_string(n.bit_length()) == f.at(3) && Sha256::hex(Sha256::hash(n.to_hex())) == f.at(4));
                const int js[3] = {1, 4, 9};
                for (int j = 0; j < 3; ++j)
                {
                    BigUint idx = n, q, m;
                    idx.mul_small(uint32_t(js[j]));
                    BigUint::divmod(idx, BigUint(10), q, m);
                    const std::vector<uint32_t> u = st.ranker()->unrank(q);
                    CHECK(Sha256::hex(Sha256::hash(std::string(u.begin(), u.end()))) == f.at(size_t(5 + j)));
                }
                ++nears;
            }
            else if (f[0] == "language")
            {
                AiShape sh;
                CHECK(std::sscanf(f.at(1).c_str(), "L%u/H%u/A%u/B%u", &sh.layers, &sh.width, &sh.heads, &sh.bits) == 4);
                const AiSpace sp(sh, "sieve");
                const AiSpace::Digits d = sp.digits_at(unit_at_bearing(BigUint(uint64_t(std::stoul(f.at(3)))), sp.size(), 0), AddressMode::Scrambled);
                const sieve::training::Score sc = sieve::training::evaluate_model(sp, d, {std::span<const uint8_t>(head)});
                CHECK(std::to_string(sc.prob_sum) == f.at(4) && std::to_string(sc.predictions) == f.at(5));
                ++langs;
            }
        }
        CHECK(nears == 4 && langs == 3);
    }
    // A run of another shape, or one never named, is refused.
    const AiSpace micro(AiShape{}, "sieve");
    sieve::FilterLine ml = line;
    ml.ai = &micro, ml.base = 16, ml.length = uint32_t(micro.weights()), ml.symbols_id = micro.id();
    CHECK(throws([&] { sieve::FilterStack(ml, {{find_filter("near-model-v1"), {{"run", "test"}}}}, res); }));
    CHECK(throws([&] { sieve::FilterStack(line, {{find_filter("near-model-v1"), {{"run", "nothing"}}}}, res); }));
    sieve::set_model_run_loader({});
}

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
    CHECK(n == 1285); // a truncated or emptied vector file must fail, not pass quietly
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
    // Several kinds apart by commas (7 November 2026): the files of any of them, the union of each alone.
    {
        const KindSet png = kind_set_of("png"), jpg = kind_set_of("jpg"), both = kind_set_of("png,jpg");
        bool union_ok = both.size() == png.size();
        for (size_t i = 0; i < both.size() && union_ok; ++i) union_ok = both[i] == uint8_t(png[i] | jpg[i]);
        CHECK(union_ok);
        CHECK(throws([] { (void)kind_set_of("png,,jpg"); }));
        CHECK(throws([] { (void)kind_set_of("png,nothing"); }));
        const FilterSpec* bk = find_filter("binary-kind-v1");
        CHECK(bk && param_value(*bk, FilterValues{{"kinds", "png,jpg"}}, "kinds") == "png,jpg");
        CHECK(bk && throws([&] { (void)param_value(*bk, FilterValues{{"kinds", "png,"}}, "kinds"); }));
    }
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
    // The readings decoded outright, as the vault takes them (written_readings: one copy of each
    // decoder since 7 November 2026): hex, base64 and the text's own three; text not UTF-8 read as
    // its bytes.
    {
        const auto has = [](const std::vector<std::vector<uint8_t>>& all, std::vector<uint8_t> b) {
            return std::find(all.begin(), all.end(), b) != all.end();
        };
        CHECK(has(sieve::written_readings("0x89 0x50 0x4E 0x47"), {0x89, 'P', 'N', 'G'}));
        CHECK(has(sieve::written_readings("iVBORw=="), {0x89, 'P', 'N', 'G'}));
        CHECK(has(sieve::written_readings("caf\xC3\xA9 \n"), {'c', 'a', 'f', 0xE9, '\n'}));
        CHECK(has(sieve::written_readings(std::string("\xC3\x41", 2)), {0xC3, 0x41}));
    }
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
            items.pages = {pattern_of(alphabet_of(f[4]), uint32_t(std::stoul(f[5])))};
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
        const ModelSieve unfiltered(space, {});
        CHECK(unfiltered.empty() && unfiltered.count() == space.size());
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
        items.pages = {pattern_of(alphabet_of("u+0061-u+0062"), 1)};
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

// diffusion-v1 against the oracle's vectors (tests/vectors_diffusion_v1.tsv): training steps on
// examples that attend everywhere and are scored on every clean cell, each step's score and digits
// as the oracle's, and a picture made from noise, level by level.
void test_diffusion_vectors(const std::string& dir)
{
    int n = 0;
    std::string cur;
    std::unique_ptr<AiSpace> space;
    std::unique_ptr<training::Trainer> trainer;
    std::unique_ptr<diffusion::SplitMix> draws;
    std::vector<std::vector<uint32_t>> pictures;
    diffusion::Setup setup;
    uint32_t batch = 0;
    for (const auto& f : read_rows(dir + "vectors_diffusion_v1.tsv"))
    {
        if (!((f[0] == "step" && f.size() == 14) || (f[0] == "picture" && f.size() == 12))) continue;
        std::string key;
        for (size_t i = 1; i <= 9; ++i) key += f[i] + "|";
        if (key != cur)
        {
            cur = key;
            uint32_t L = 0, H = 0, A = 0, B = 0;
            CHECK(std::sscanf(f[1].c_str(), "L%u/H%u/A%u/B%u", &L, &H, &A, &B) == 4);
            AiShape shape;
            shape.layers = L, shape.width = H, shape.heads = A, shape.bits = B;
            space = std::make_unique<AiSpace>(shape, "sieve");
            setup.width = uint32_t(std::stoul(f[2])), setup.height = uint32_t(std::stoul(f[3]));
            setup.colours = uint32_t(std::stoul(f[4])), setup.levels = uint32_t(std::stoul(f[5]));
            batch = uint32_t(std::stoul(f[6]));
            diffusion::SplitMix start_draws(std::stoull(f[9]));
            AiSpace::Digits start(space->weights());
            for (auto& d : start) d = uint8_t(start_draws.next() % (uint64_t(1) << B));
            training::Recipe recipe;
            recipe.version = 1;
            recipe.shape = shape;
            recipe.learning_rate = f[7];
            recipe.momentum = f[8];
            trainer = std::make_unique<training::Trainer>(*space, start, recipe, 2);
            diffusion::SplitMix picture_draws(1);
            pictures.assign(4, std::vector<uint32_t>(setup.cells()));
            for (auto& p : pictures)
                for (auto& c : p) c = uint32_t(picture_draws.next() % setup.colours);
            draws = std::make_unique<diffusion::SplitMix>(3);
        }
        bool ok;
        if (f[0] == "step")
        {
            const training::Score sc = diffusion::step(*trainer, setup, pictures, *draws, batch);
            const std::string d(trainer->digits().begin(), trainer->digits().end());
            ok = std::to_string(sc.prob_sum) == f[11] && std::to_string(sc.predictions) == f[12] && Sha256::hex(Sha256::hash(d)) == f[13];
        }
        else ok = diffusion::make_picture(*space, trainer->digits(), setup, split_u32(f[10]), 77) == split_u32(f[11]);
        CHECK(ok);
        if (!ok) std::cerr << "  diffusion vector mismatch: " << f[0] << " " << f[1] << " " << (f[0] == "step" ? f[10] : "") << "\n";
        ++n;
    }
    std::cout << "diffusion vectors checked: " << n << "\n";
    CHECK(n >= 7);
    // A window of text is the causal example of its bytes: the same score either way.
    {
        AiShape shape;
        shape.width = 8, shape.heads = 2, shape.bits = 3;
        const AiSpace sp(shape, "sieve");
        AiSpace::Digits d(sp.weights());
        diffusion::SplitMix g(4);
        for (auto& x : d) x = uint8_t(g.next() % 8);
        const std::string text = "the sieve keeps what reads";
        const std::span<const uint8_t> win(reinterpret_cast<const uint8_t*>(text.data()), text.size());
        const training::Score a = training::evaluate_model(sp, d, {win});
        training::Recipe recipe;
        recipe.version = 2; // (evaluate_model scores with training-v2's softmax)
        recipe.shape = shape;
        const training::Trainer t(sp, d, recipe, 1);
        const std::vector<int16_t> target(win.begin() + 1, win.end());
        const training::Score b = t.evaluate(std::vector<training::Example>{{win.first(win.size() - 1), target, true}});
        CHECK(a.prob_sum == b.prob_sum && a.predictions == b.predictions);
        // Not causal, the same example scores otherwise: every position now sees the bytes after it.
        const training::Score c = t.evaluate(std::vector<training::Example>{{win.first(win.size() - 1), target, false}});
        CHECK(c.predictions == b.predictions && c.prob_sum != b.prob_sum);
    }
}

// figure-v1 against the oracle's vectors (tests/vectors_figure_v1.tsv): counts, ranks both ways
// and verdicts, on small lines checked there picture by picture with a flood fill, and at 10 by 10
// on every palette (rgb24 with its own ranker), with a taller and a wider picture.
void test_figure_vectors(const std::string& dir)
{
    const TestResources none(nullptr, nullptr);
    const FilterSpec* figure = find_filter("figure-v1");
    CHECK(figure != nullptr);
    if (!figure) return;
    int n = 0;
    for (const auto& f : read_rows(dir + "vectors_figure_v1.tsv"))
    {
        if (!((f[0] == "count" && f.size() == 8) || (f[0] == "unit" && f.size() == 9))) continue;
        const Palette& pal = palette_by_id(f[1]);
        const uint32_t w = uint32_t(std::stoul(f[2])), h = uint32_t(std::stoul(f[3]));
        const FilterLine line{"image", "image/" + f[1] + "/" + f[2] + "x" + f[3], pal.size(), w * h, nullptr, w, h, 0};
        const FilterValues v{{"shapes", f[4]}, {"connect", f[5]}, {"border", f[6]}};
        const FilterStack st(line, {{figure, v}}, none);
        bool ok = st.ranker() != nullptr;
        if (ok && f[0] == "count") ok = st.ranker()->count().to_decimal() == f[7];
        else if (ok)
        {
            const BigUint k = BigUint::from_decimal(f[7]);
            const std::vector<uint32_t> want = split_u32(f[8]);
            ok = st.ranker()->unrank(k) == want && st.ranker()->rank(want) == k && st.passes(want) && st.ranker()->accepts(want);
        }
        CHECK(ok);
        if (!ok)
        {
            std::cerr << "  figure vector mismatch:";
            for (const auto& x : f) std::cerr << " " << x.substr(0, 60);
            std::cerr << "\n";
        }
        ++n;
    }
    std::cout << "figure vectors checked: " << n << "\n";
    CHECK(n >= 150);
    // On mono it is an automaton, so it counts together with row-runs: every picture of a small
    // line judged by both agrees with the stack's count. On rgb24 it counts with its own ranker.
    {
        const FilterSpec* runs = find_filter("row-runs-v1");
        const FilterLine line{"image", "image/mono/4x4", 2, 16, nullptr, 4, 4, 0};
        const FilterValues fv{{"border", "any"}, {"connect", "four"}}, rv{{"changes", "1"}};
        const FilterStack both(line, {{figure, fv}, {runs, rv}}, none), one(line, {{figure, fv}}, none), other(line, {{runs, rv}}, none);
        uint64_t kept = 0;
        for (uint32_t bits = 0; bits < (1u << 16); ++bits)
        {
            std::vector<uint32_t> u(16);
            for (uint32_t i = 0; i < 16; ++i) u[i] = (bits >> i) & 1;
            kept += one.passes(u) && other.passes(u);
        }
        CHECK(both.ranker() && both.ranker()->count() == BigUint(kept));
        CHECK(figure->counts_as_on(line, &fv) == "automaton");
        const FilterLine big{"image", "image/rgb24/10x10", 1u << 24, 100, nullptr, 10, 10, 0};
        CHECK(figure->counts_as_on(big, nullptr) == "own");
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

// degree-window-v2 on the lines whose units are of any length (list N5-3): on the binary line a file's bearing is its
// place among every file of the line, and BinarySieve counts and ranks the window's part of the other filters'
// survivors; checked against every file of a line of 2 bytes, alone and over the kinds, not-an-item's pages and
// utf8-valid, across 0 and two at once. On the fractals line it judges a ruleset by its place (first_failure_at).
void test_place_window()
{
    const FilterSpec* w = find_filter("degree-window-v2");
    const FilterSpec* utf8 = find_filter("utf8-valid-v1");
    const FilterSpec* not_item = find_filter("not-an-item-v1");
    CHECK(w && utf8 && not_item);
    if (!w || !utf8 || !not_item) return;
    FilterLine bl;
    bl.kind = "binary", bl.symbols_id = "bytes256", bl.base = 256, bl.length = 2;
    FilterLine fl;
    fl.kind = "fractals", fl.symbols_id = "fractals/96", fl.base = 128, fl.length = 96, fl.up_to_length = true;
    CHECK(w->applies(bl) && w->applies(fl) && !find_filter("degree-window-v1")->applies(bl));
    const auto wv = [](uint64_t from, uint64_t width, uint32_t d) {
        return FilterValues{{"from", std::to_string(from)}, {"width", std::to_string(width)}, {"decimals", std::to_string(d)}};
    };
    const BinarySpace space(2, "sieve");
    const uint64_t n = 1 + 256 + 65536;
    BinaryItems items;
    items.pages.emplace_back();
    items.pages[0].allowed.assign(1, std::array<bool, 256>{});
    items.pages[0].allowed[0]['a'] = items.pages[0].allowed[0]['b'] = true; // (the pages of a line of one letter, a or b)
    const auto in = [&](uint64_t v, uint64_t from, uint64_t width, uint32_t d) {
        uint64_t s = 360;
        for (uint32_t i = 0; i < d; ++i) s *= 10;
        const uint64_t b = mul_div(v, s, n);
        return width >= s || (b + s - from) % s < width;
    };
    struct Case
    {
        std::vector<FilterStack::Entry> others;
        std::vector<std::array<uint64_t, 3>> windows; // from, width, decimals
    };
    const std::vector<Case> cases = {
        {{}, {{{10, 20, 0}}}},
        {{}, {{{350, 20, 0}}}},                               // across 0
        {{}, {{{1234, 7, 1}}}},                               // tenths
        {{kind_entry("text", "keep")}, {{{90, 45, 0}}}},
        {{kind_entry("text", "keep")}, {{{300, 100, 0}}}},    // across 0, over a kind
        {{kind_entry("any", "keep"), {not_item, {{"items", "pages"}}}}, {{{0, 1, 0}}}}, // the pages are among the first places
        {{{utf8, {{"controls", "text"}}}}, {{{200, 30, 0}}}},
        {{{utf8, {{"controls", "any"}}}}, {{{355, 10, 0}}}},  // across 0, over utf8-valid
        {{kind_entry("signed-or-text", "keep")}, {{{10, 100, 0}}, {{50, 100, 0}}}}, // two windows: 50 to 110
    };
    for (const Case& c : cases)
    {
        std::vector<FilterStack::Entry> entries = c.others;
        for (const auto& x : c.windows) entries.push_back({w, wv(x[0], x[1], uint32_t(x[2]))});
        const BinarySieve others(space, c.others, &items), bs(space, entries, &items);
        bool ok = bs.can_rank() && bs.needs_file();
        uint64_t k = 0;
        for (uint64_t v = 0; ok && v < n; ++v)
        {
            const std::vector<uint8_t> file = BinarySpace::file_at_place(BigUint(v));
            bool pass = others.first_failure_of(file).empty();
            for (const auto& x : c.windows) pass = pass && in(v, x[0], x[1], uint32_t(x[2]));
            ok = bs.first_failure_of(file).empty() == pass;
            if (pass && ok) ok = bs.index_of(file, AddressMode::Positional) == BigUint(k) && bs.file_at(BigUint(k), AddressMode::Positional) == file;
            k += pass ? 1 : 0;
        }
        CHECK(ok);
        CHECK(bs.count() == BigUint(k));
    }
    // Two windows where one crosses 0: they judge only.
    {
        const BinarySieve bs(space, {{w, wv(350, 20, 0)}, {w, wv(0, 5, 0)}});
        CHECK(!bs.can_rank() && bs.blocker().find("crosses 0") != std::string::npos);
        CHECK(bs.first_failure_of({}).empty() && !bs.first_failure_of({0x80, 0}).empty()); // (the empty file is at place 0)
    }
    // The fractals line: judged by place, and first_failure without one refused.
    {
        static const TestResources none(nullptr, nullptr);
        const FilterStack st(fl, {{w, wv(90, 90, 0)}}, none);
        CHECK(st.by_place() && throws([&] { (void)st.first_failure(std::vector<uint32_t>{1, 2, 3}); }));
        const BigUint places(1000);
        bool ok = true;
        for (uint64_t v = 0; v < 1000; ++v)
        {
            const bool want = v * 360 / 1000 >= 90 && v * 360 / 1000 < 180;
            ok = ok && (st.first_failure_at(std::vector<uint32_t>{1, 2, 3}, BigUint(v), places) < 0) == want;
        }
        CHECK(ok);
    }
}

// A range line's filters (list O3, sieve/rangestack.hpp): every unit of 1 to 3 letters judged against each length's own
// stack (and a window by its place on the whole range), counted, ranked and unranked in the line's order; a filter
// that does not apply at a length (not-a-pattern needs two symbols) says nothing about the units of that length.
void test_range_stack()
{
    static const TestResources none(nullptr, nullptr);
    const Alphabet& a = alphabet_by_id("lower27");
    const Space sp(a, 1, 3, "sieve");
    FilterLine line;
    line.kind = "text", line.symbols_id = a.id(), line.base = 27, line.length = 3, line.min_length = 1, line.alphabet = &a;
    const FilterSpec* run = find_filter("max-run-v1");
    const FilterSpec* pat = find_filter("not-a-pattern-v1");
    const FilterSpec* win = find_filter("degree-window-v2");
    CHECK(run && pat && win);
    if (!run || !pat || !win) return;
    auto wv = [](int from, int width) { return FilterValues{{"from", std::to_string(from)}, {"width", std::to_string(width)}}; };
    struct Case
    {
        std::vector<FilterStack::Entry> entries;
        std::vector<std::pair<int, int>> windows;
    };
    const std::vector<Case> cases = {
        {{{run, {{"max_run", "1"}}}}, {}},
        {{{pat, {}}}, {}},                        // not-a-pattern: no stack of its own at one letter
        {{{run, {{"max_run", "1"}}}}, {{100, 50}}},
        {{{run, {{"max_run", "1"}}}}, {{350, 30}}}, // across 0
        {{}, {{10, 20}}},                           // a window alone
    };
    const uint64_t n = 27 + 729 + 19683;
    for (const Case& c : cases)
    {
        std::vector<FilterStack::Entry> entries = c.entries;
        for (const auto& [f, w] : c.windows) entries.push_back({win, wv(f, w)});
        const RangeStack rs(line, entries, none);
        // the reference: each length's own stack, with the filters that apply at that length, and the window by place
        std::vector<FilterStack> per;
        for (uint32_t L = 1; L <= 3; ++L)
        {
            FilterLine fl = line;
            fl.length = L, fl.min_length = 0;
            std::vector<FilterStack::Entry> here;
            for (const auto& e : c.entries)
                if (e.spec->applies(fl)) here.push_back(e);
            per.emplace_back(fl, here, none);
        }
        bool ok = rs.can_rank();
        uint64_t k = 0;
        for (uint64_t p = 0; ok && p < n; ++p)
        {
            const auto u = sp.unit_at_place(BigUint(p));
            bool want = per[u.size() - 1].passes(u);
            for (const auto& [f, w] : c.windows) want = want && (p * 360 / n + 360 - uint64_t(f)) % 360 < uint64_t(w);
            ok = rs.passes(u) == want;
            if (ok && want) ok = rs.rank(u) == BigUint(k) && rs.unrank(BigUint(k)) == u;
            k += want ? 1 : 0;
        }
        CHECK(ok);
        CHECK(rs.can_rank() && rs.count() == BigUint(k));
    }
    // Review 4: the tables of every length together must fit the filter memory (each length's stack checks only its
    // own), the longest length's table counted with the rest; and a range with a degree window gives no ranker per
    // length, which a compact line asks before it builds a guided order (it would throw).
    {
        FilterLine wide = line;
        wide.length = 150;
        const RangeStack fits(wide, {{run, {{"max_run", "1"}}}}, none);
        CHECK(fits.can_rank() && fits.table_bytes() > 4.0 * 1024 * 1024);
        FilterLine top = wide;
        top.min_length = 0;
        CHECK(FilterStack(top, {{run, {{"max_run", "1"}}}}, none).table_bytes() < 2.0 * 1024 * 1024);
        const double was = filter_memory();
        set_filter_memory(2.0 * 1024 * 1024); // more than any one length's table, less than them all
        const RangeStack over(wide, {{run, {{"max_run", "1"}}}}, none);
        CHECK(!over.can_rank() && over.compact_blocker().find("together, over the filter memory") != std::string::npos);
        set_filter_memory(was);
        const FilterStack plain(line, {{run, {{"max_run", "1"}}}}, none);
        const FilterStack windowed(line, {{run, {{"max_run", "1"}}}, {win, wv(100, 200)}}, none);
        CHECK(plain.ranker() && plain.ranker()->has_length_rankers());
        CHECK(windowed.ranker() && !windowed.ranker()->has_length_rankers());
    }
    // not-a-pattern does not apply at one letter: those pass it
    const RangeStack only_pat(line, {{pat, {}}}, none);
    CHECK(only_pat.passes(std::vector<uint32_t>{5}));
    CHECK(only_pat.provenance().rfind("text/lower27/L1-3", 0) == 0);

    // A FilterStack on a line of a range of lengths (list O4) hands every question to its RangeStack: the same
    // verdicts, a ranker that counts, ranks and unranks across the range, and a compact line on it in both orders.
    {
        const std::vector<FilterStack::Entry> entries = {{run, {{"max_run", "1"}}}, {win, wv(100, 200)}};
        const RangeStack rs(line, entries, none);
        const FilterStack st(line, entries, none);
        CHECK(st.range() != nullptr && st.size() == 2 && st.filter_name(1) == rs.filter_name(1) && st.id() == rs.id());
        const Ranker* r = st.ranker();
        CHECK(r && r->count() == rs.count() && r->length() == 3 && r->base() == 27);
        if (r)
        {
            const CompactLine cl(*r, "sieve", st.id());
            bool ok = true;
            uint64_t k = 0;
            std::set<std::string> seen;
            for (uint64_t p = 0; ok && p < n; ++p)
            {
                const auto u = sp.unit_at_place(BigUint(p));
                ok = st.first_failure(u) == rs.first_failure(u);
                if (!ok || !rs.passes(u)) continue;
                ok = r->rank(u) == BigUint(k) && r->unrank(BigUint(k)) == u && cl.index_of(u, AddressMode::Positional) == BigUint(k);
                const BigUint s = cl.index_of(u, AddressMode::Scrambled);
                ok = ok && s < r->count() && cl.unit_at(s, AddressMode::Scrambled) == u && seen.insert(s.to_hex()).second;
                ++k;
            }
            CHECK(ok);
            CHECK(BigUint(k) == r->count());
        }
        // a line of one length is a plain stack, as before
        FilterLine one = line;
        one.min_length = 0;
        CHECK(FilterStack(one, {{run, {{"max_run", "1"}}}}, none).range() == nullptr);
    }
    // not-an-item-v1's pages on the binary line when pages are of several lengths: those of every length (1 and 2
    // letters, a or b) are set aside, and the survivors, in order, are the kind survivors that are none of them.
    {
        const FilterSpec* not_item = find_filter("not-an-item-v1");
        const FilterSpec* kind = find_filter("binary-kind-v1");
        CHECK(not_item && kind);
        if (!not_item || !kind) return;
        const BinarySpace space(2, "sieve");
        BinaryItems items;
        for (uint32_t L = 1; L <= 2; ++L)
        {
            KindCounter::Pattern pt;
            std::array<bool, 256> ab{};
            ab['a'] = ab['b'] = true;
            pt.allowed.assign(L, ab);
            items.pages.push_back(pt);
        }
        const BinarySieve bs(space, {{kind, {{"kinds", "any"}, {"keep", "keep"}}}, {not_item, {{"items", "pages"}}}}, &items);
        bool ok = bs.can_rank();
        const uint64_t files = 1 + 256 + 65536;
        CHECK(ok && bs.count() == BigUint(files - 2 - 4));
        uint64_t k = 0;
        for (uint64_t p = 0; ok && p < files; ++p)
        {
            const auto f = BinarySpace::file_at_place(BigUint(p));
            bool page = !f.empty();
            for (uint8_t c : f) page = page && (c == 'a' || c == 'b');
            ok = bs.first_failure_of(f).empty() == !page;
            if (ok && !page) ok = bs.index_of(f, AddressMode::Positional) == BigUint(k) && bs.file_at(BigUint(k), AddressMode::Positional) == f;
            k += page ? 0 : 1;
        }
        CHECK(ok);
    }
}

// The guided order on a line of a range of lengths (guided-range-v1, list O5): the oracle's vectors; every unit of 1 to
// 3 letters, its arc tiling the line in order (the lengths in turn, each in its guided order) and read back from its
// address; and sieved, by a stack on the range, its survivors' arcs tiling the line and a compact line's guided view.
void test_guided_range(const std::string& dir)
{
    const auto model = std::make_shared<const CharModel>(CharModel::load_file(dir + "../data/models/gutenberg-lower27-o5.model"));
    const Alphabet& a = alphabet_by_id("lower27");
    {
        std::ifstream in(dir + "vectors_guided_range_v1.tsv");
        CHECK(bool(in));
        std::string line;
        std::getline(in, line);
        std::getline(in, line);
        CHECK(line == "# model sha256 " + model->sha256());
        std::map<std::pair<uint32_t, uint32_t>, std::unique_ptr<GuidedLine>> lines;
        int n = 0;
        while (std::getline(in, line))
        {
            if (line.empty() || line[0] == '#') continue;
            std::vector<std::string> f;
            std::stringstream ss(line);
            std::string x;
            while (std::getline(ss, x, '\t')) f.push_back(x);
            CHECK(f.size() == 6);
            if (f.size() != 6) continue;
            const uint32_t lo = uint32_t(std::stoul(f[1])), hi = uint32_t(std::stoul(f[2]));
            auto& g = lines[{lo, hi}];
            if (!g) g = std::make_unique<GuidedLine>(model, lo, hi);
            std::vector<uint32_t> unit;
            for (char c : f[3]) unit.push_back(*a.digit_of(char32_t(c)));
            if (f[0] == "code")
            {
                const auto c = g->code(unit);
                CHECK(std::to_string(c.bits) == f[4]);
                CHECK(c.hex == f[5]);
            }
            CHECK(g->unit_at(g->point_of(f[5])) == unit);
            ++n;
        }
        std::cout << "guided range vectors checked: " << n << "\n";
        CHECK(n == 46);
    }
    // Every unit of 1 to 3 letters, in place order: arcs that meet end to end from 0 to 2^S, each read back.
    static const TestResources none(nullptr, model);
    const Space sp(a, 1, 3, "sieve");
    const uint64_t places = 27 + 729 + 19683;
    auto tiles = [&](const GuidedLine& g, const std::function<bool(const std::vector<uint32_t>&)>& kept) {
        BigUint at;
        bool ok = true;
        uint64_t k = 0;
        for (uint64_t p = 0; ok && p < places; ++p)
        {
            const auto u = sp.unit_at_place(BigUint(p));
            if (!kept(u))
            {
                ok = throws([&] { (void)g.interval(u); });
                continue;
            }
            const auto iv = g.interval(u);
            ok = iv.low == at && !iv.width.is_zero() && g.unit_at(iv.low) == u && g.unit_at(g.code_of(iv).point) == u;
            at = iv.low;
            at += iv.width;
            ++k;
        }
        BigUint end(1);
        end <<= g.scale_bits();
        return ok && at == end && k > 0;
    };
    {
        const GuidedLine g(model, 1, 3);
        CHECK(g.scale_bits() == 64 && g.is_range());
        CHECK(tiles(g, [](const std::vector<uint32_t>&) { return true; }));
    }
    FilterLine fl;
    fl.kind = "text", fl.symbols_id = a.id(), fl.base = 27, fl.length = 3, fl.min_length = 1, fl.alphabet = &a;
    const FilterSpec* run = find_filter("max-run-v1");
    const FilterSpec* pat = find_filter("not-a-pattern-v1");
    CHECK(run && pat);
    if (!run || !pat) return;
    for (const auto& entries : std::vector<std::vector<FilterStack::Entry>>{{{run, {{"max_run", "1"}}}}, {{pat, {}}}})
    {
        const FilterStack st(fl, entries, none);
        const Ranker* r = st.ranker();
        CHECK(r != nullptr);
        if (!r) continue;
        const GuidedLine g(model, *r);
        CHECK(tiles(g, [&](const std::vector<uint32_t>& u) { return st.passes(u); }));
        const CompactLine cl(*r, "sieve", st.id(), model);
        CHECK(cl.guided() && cl.guided()->is_range() && cl.guided()->code(digits27("ab")).hex == g.code(digits27("ab")).hex);
    }
    // a degree window keeps a stretch of the whole range: no guided order within each length
    {
        const FilterStack st(fl, {{run, {{"max_run", "1"}}}, {find_filter("degree-window-v2"), {{"from", "10"}, {"width", "100"}}}}, none);
        CHECK(st.ranker() && throws([&] { (void)GuidedLine(model, *st.ranker()); }));
    }
    // a line of one length is guided-ac-v1, as before
    CHECK(GuidedLine(model, 3).scale_bits() == 48);
}

// The join filters (list N5-14): join-leap-v1 and join-cut-v1 judge only where one part of a joined unit meets the
// next, and count and rank: against every unit of small joined lines, and over two voices.
void test_join_filters()
{
    static const TestResources none(nullptr, nullptr);
    const FilterSpec* leap = find_filter("join-leap-v1");
    const FilterSpec* cut = find_filter("join-cut-v1");
    CHECK(leap && cut);
    if (!leap || !cut) return;
    auto walk_all = [](const FilterStack& st, const FilterLine& l, const std::function<bool(const std::vector<uint32_t>&)>& rule) {
        std::vector<uint32_t> u(l.length, 0);
        uint64_t kept = 0;
        bool agree = true;
        for (;;)
        {
            const bool want = rule(u);
            agree = agree && st.passes(u) == want;
            if (want && agree && st.ranker()) agree = st.ranker()->rank(u) == BigUint(kept) && st.ranker()->unrank(BigUint(kept)) == u;
            kept += want ? 1 : 0;
            size_t i = u.size();
            while (i > 0 && ++u[i - 1] == l.base) u[--i] = 0;
            if (i == 0) break;
        }
        return std::make_pair(agree, kept);
    };
    // join-leap on notes2 C3-C4 of quarter notes (14 symbols, 14^4 units each): parts of 1 joined four times, parts of 2
    // joined twice, and two voices of parts of 1 a voice joined twice
    struct Shape
    {
        uint32_t voices, part, units;
    };
    for (const Shape sh : {Shape{1, 1, 4}, Shape{1, 2, 2}, Shape{2, 2, 2}})
    {
        const uint32_t voices = sh.voices;
        const NoteSet set = make_note_set(48, 60, "q", voices);
        FilterLine part;
        part.kind = "audio", part.symbols_id = set.id(), part.base = set.base(), part.length = sh.part;
        const uint32_t units = sh.units;
        FilterLine l = part;
        l.length = part.length * units, l.part_length = part.length;
        CHECK(leap->applies(l) && !leap->applies(part));
        const FilterStack st(l, {{leap, {{"semitones", "2"}}}}, none);
        const uint32_t per = l.length / voices, part_per = part.length / voices;
        const auto [agree, kept] = walk_all(st, l, [&](const std::vector<uint32_t>& u) {
            for (uint32_t v = 0; v < voices; ++v)
                for (uint32_t j = part_per; j < per; j += part_per)
                {
                    const uint32_t a = set.midi(u[v * per + j - 1]), b = set.midi(u[v * per + j]);
                    if (a && b && (a > b ? a - b : b - a) > 2) return false;
                }
            return true;
        });
        CHECK(agree && st.ranker() && st.ranker()->count() == BigUint(kept));
    }
    // join-cut on 2 x 1 pictures of 3 colours, units of 2 frames joined three times
    {
        FilterLine l;
        l.kind = "video", l.symbols_id = "video-test", l.base = 3, l.width = 2, l.height = 1, l.frames = 6, l.length = 12, l.part_length = 4;
        for (int changes : {0, 1})
        {
            const FilterStack st(l, {{cut, {{"changes", std::to_string(changes)}}}}, none);
            const auto [agree, kept] = walk_all(st, l, [&](const std::vector<uint32_t>& u) {
                for (uint32_t f = 2; f < 6; f += 2)
                    if (int((u[f * 2] != u[f * 2 - 2]) + (u[f * 2 + 1] != u[f * 2 - 1])) > changes) return false;
                return true;
            });
            CHECK(agree && st.ranker() && st.ranker()->count() == BigUint(kept));
        }
    }
}

// The models line's mesh filters (list N5-15): a tetrahedron, wound one way, is closed; turned inside out on one
// face it is not; three faces on one edge are a fin; corners on one line have no area. They judge only.
void test_mesh_filters()
{
    const ModelSpace space(4, 4, 4);
    ModelSpace::Parts p;
    p.verts = {0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3};
    p.faces = {0, 2, 1, 0, 1, 3, 0, 3, 2, 1, 2, 3}; // every edge run once each way
    const ModelSieve closed(space, {{find_filter("mesh-closed-v1"), {}}}), edges(space, {{find_filter("mesh-edges-v1"), {}}}),
        area(space, {{find_filter("mesh-area-v1"), {}}});
    CHECK(!closed.can_rank() && closed.blocker().find("judge only") != std::string::npos);
    auto at = [&](const ModelSpace::Parts& q) { return space.index_of(q, AddressMode::Positional); };
    CHECK(closed.first_failure(at(p)).empty() && edges.first_failure(at(p)).empty() && area.first_failure(at(p)).empty());
    ModelSpace::Parts flipped = p;
    std::swap(flipped.faces[10], flipped.faces[11]); // the last face wound the other way
    CHECK(closed.first_failure(at(flipped)) == "mesh-closed-v1" && edges.first_failure(at(flipped)).empty());
    ModelSpace::Parts fin = p;
    fin.faces = {0, 1, 2, 0, 1, 3, 1, 0, 3, 0, 1, 2}; // edge 0-1 in four faces
    CHECK(edges.first_failure(at(fin)) == "mesh-edges-v1" && closed.first_failure(at(fin)) == "mesh-closed-v1");
    ModelSpace::Parts flat = p;
    flat.verts = {0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 3}; // the first three on one line
    CHECK(area.first_failure(at(flat)) == "mesh-area-v1" && closed.first_failure(at(flat)).empty());
    CHECK(find_filter("mesh-closed-v1")->judge_only && find_filter("mesh-area-v1")->judge_only);
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
                items.pages = {p};
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
                    items.pages = {pat};
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
        items.pages = {pages};
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
        const ModelSpace thirteen(13, 20, 16);
        const std::vector<FilterStack::Entry> both{{find_filter("canonical-mesh-v1"), {}}, {find_filter("every-vertex-used-v1"), {}}};
        const ModelSieve fits(thirteen, both);
        CHECK(fits.can_rank() && fits.table_bytes() > 8.0 * 1024 * 1024);
        BigUint k = fits.count();
        k.divmod_small(3);
        CHECK(fits.index_of(fits.model_at(k, AddressMode::Positional), AddressMode::Positional) == k);
        set_filter_memory(fits.table_bytes() * 0.99);
        const ModelSieve over(thirteen, both);
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
// mix-v1: the parameters; every shape of file given back; a code cut short or with more after it
// refused; and the code of one text pinned by its SHA-256, so every machine must make the same.
void test_mix()
{
    CHECK(MixParams::parse("B22").table_bits == 22);
    CHECK(MixParams::parse("B16").text() == "B16");
    CHECK(throws([] { MixParams::parse("B15"); }));
    CHECK(throws([] { MixParams::parse("B23"); }));
    CHECK(throws([] { MixParams::parse("B022"); }));
    CHECK(throws([] { MixParams::parse("B22 "); }));
    CHECK(MixParams::for_size(0).table_bits == 16 && MixParams::for_size(1u << 20).table_bits == 21 && MixParams::for_size(1u << 30).table_bits == 22);
    std::mt19937 rng(5);
    std::vector<std::vector<uint8_t>> files{{}, {0}, {255}, std::vector<uint8_t>(5000, 'a')};
    std::vector<uint8_t> noise(3000), all(256), text;
    for (auto& b : noise) b = uint8_t(rng());
    for (int i = 0; i < 256; ++i) all[size_t(i)] = uint8_t(i);
    // A text that repeats with changes: sentences of words drawn from a few.
    const char* words[] = {"the", "rope", "triangle", "pulls", "tight", "through", "every", "value", "Sieve", "mixes"};
    for (int i = 0; i < 2500; ++i)
    {
        const std::string w = words[rng() % 10];
        text.insert(text.end(), w.begin(), w.end());
        text.push_back(rng() % 9 ? ' ' : '\n');
    }
    files.push_back(noise);
    files.push_back(all);
    files.push_back(text);
    bool back = true;
    for (const auto& f : files)
        for (const uint32_t b : {16u, 18u})
        {
            MixParams p;
            p.table_bits = b;
            back = back && mix_file(mix_code(f, p), p, f.size()) == f;
        }
    CHECK(back);
    MixParams p;
    p.table_bits = 16;
    const std::vector<uint8_t> c = mix_code(text, p);
    CHECK(c.size() * 4 < text.size()); // a text of ten words: well under a quarter
    std::vector<uint8_t> more = c;
    more.insert(more.end(), {1, 2, 3, 4, 5, 6});
    CHECK(throws([&] { mix_file(more, p, text.size()); }));
    CHECK(throws([&] { mix_file(std::span<const uint8_t>(c.data(), c.size() / 2), p, text.size()); }));
    CHECK(mix_file(c, p, text.size()) == text);
    const std::string pinned = Sha256::hex(Sha256::hash(std::string(c.begin(), c.end())));
    std::cout << "mix-v1: a text of " << text.size() << " bytes in " << c.size() << ", sha256 " << pinned << "\n";
    CHECK(pinned == "503f61a3778a1f75a568e2bde90ffd6f6ea8f430ce9c87ac1357cf6175db302a");
}

// mix-v2: the parameters (a picture's shape among them); every shape of file given back, as text
// and as pictures of each width and bytes a pixel; a code cut short or with more after it refused;
// and the codes of one text and one picture pinned by their SHA-256.
void test_mix2()
{
    CHECK(Mix2Params::parse("B22").width == 0);
    CHECK(Mix2Params::parse("B20 I949x1").width == 949 && Mix2Params::parse("B20 I949x1").channels == 1);
    CHECK(Mix2Params::parse("B16 I1280x3").text() == "B16 I1280x3");
    CHECK(throws([] { Mix2Params::parse("B15"); }));
    CHECK(throws([] { Mix2Params::parse("B23"); }));
    CHECK(throws([] { Mix2Params::parse("B22 I1x1"); }));    // a picture one pixel wide
    CHECK(throws([] { Mix2Params::parse("B22 I949x5"); }));  // five bytes a pixel
    CHECK(throws([] { Mix2Params::parse("B22 I949x0"); }));
    CHECK(throws([] { Mix2Params::parse("B22 I0949x1"); })); // not written as it would be
    CHECK(throws([] { Mix2Params::parse("B22 I949"); }));
    CHECK(throws([] { Mix2Params::parse("B22 "); }));
    CHECK(Mix2Params::for_size(1u << 20).text() == "B21" && Mix2Params::for_size(4096, 64, 3).text() == "B16 I64x3");
    std::mt19937 rng(9);
    std::vector<uint8_t> noise(3000), all(256), text, picture;
    for (auto& b : noise) b = uint8_t(rng());
    for (int i = 0; i < 256; ++i) all[size_t(i)] = uint8_t(i);
    const char* words[] = {"the", "rope", "triangle", "pulls", "tight", "through", "every", "value", "Sieve", "mixes"};
    for (int i = 0; i < 2500; ++i)
    {
        const std::string w = words[rng() % 10];
        text.insert(text.end(), w.begin(), w.end());
        text.push_back(rng() % 9 ? (rng() % 5 ? ' ' : '\t') : '\n');
    }
    // A picture: 60 by 40, three bytes a pixel, smooth ramps with a little noise and an edge.
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 60; ++x)
            for (int c = 0; c < 3; ++c) picture.push_back(uint8_t((x * 3 + y * 2 + c * 40 + (x > 30 ? 90 : 0) + int(rng() % 5)) & 255));
    const std::vector<std::vector<uint8_t>> files{{}, {0}, {255}, std::vector<uint8_t>(5000, 'a'), noise, all, text, picture};
    bool back = true;
    for (const auto& f : files)
        for (const char* params : {"B16", "B18", "B16 I2x1", "B16 I60x3", "B16 I7x4", "B16 I3000x1"})
        {
            const Mix2Params p = Mix2Params::parse(params);
            back = back && mix2_file(mix2_code(f, p), p, f.size()) == f;
        }
    CHECK(back);
    const Mix2Params p = Mix2Params::parse("B16");
    const std::vector<uint8_t> c = mix2_code(text, p);
    CHECK(c.size() * 4 < text.size());
    std::vector<uint8_t> more = c;
    more.insert(more.end(), {1, 2, 3, 4, 5, 6});
    CHECK(throws([&] { mix2_file(more, p, text.size()); }));
    CHECK(throws([&] { mix2_file(std::span<const uint8_t>(c.data(), c.size() / 2), p, text.size()); }));
    CHECK(mix2_file(c, p, text.size()) == text);
    const Mix2Params pp = Mix2Params::parse("B16 I60x3");
    const std::vector<uint8_t> cp = mix2_code(picture, pp);
    CHECK(cp.size() < mix2_code(picture, p).size()); // as a picture, smaller than as bytes
    const std::string pinned = Sha256::hex(Sha256::hash(std::string(c.begin(), c.end())));
    const std::string pinned_picture = Sha256::hex(Sha256::hash(std::string(cp.begin(), cp.end())));
    std::cout << "mix-v2: a text of " << text.size() << " bytes in " << c.size() << ", sha256 " << pinned << "; a picture of "
              << picture.size() << " in " << cp.size() << ", sha256 " << pinned_picture << "\n";
    CHECK(pinned == "2a3b7d5ec944128c74ab0946fe8c2c36ba13714c674dec4ee97e3d704b1e6626");
    CHECK(pinned_picture == "cc0a6fe8d16932d05fcd967de60566e9c025605383fec73144d2c72dd0446251");
}

// mix-v3: a word list made ready (its lines, a carriage return left out, case folded, lines of
// anything but letters left out); the parameters; every shape of file given back; mix-v2's code
// when the list knows nothing of the text; the code of one text pinned by its SHA-256.
void test_mix3()
{
    const std::string list_text = "the\r\nrope\nTriangle\npulls\ntight\nthrough\nevery\nvalue\nSieve\nmixes\nit's\n\n1990s\nthen\n";
    const MixDictionary words(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(list_text.data()), list_text.size()));
    CHECK(words.words() == 11); // (it's, 1990s and the empty line left out)
    const MixDictionary::Node& root = words.nodes()[0];
    CHECK(root.pre[27] == 11);                                   // every word goes on from the root by a letter
    CHECK(root.pre['t' - 'a' + 2] - root.pre['t' - 'a' + 1] == 5); // the, triangle, tight, through, then
    CHECK(Mix3Params::parse("B22 D60").dictionary == 60 && Mix3Params::parse("B16 D35").text() == "B16 D35");
    CHECK(throws([] { Mix3Params::parse("B22"); }));
    CHECK(throws([] { Mix3Params::parse("B22 D61"); }));
    CHECK(throws([] { Mix3Params::parse("B22 D060"); }));
    CHECK(throws([] { Mix3Params::parse("B15 D60"); }));
    CHECK(Mix3Params::for_size(1u << 20).text() == "B21 D60");
    std::mt19937 rng(11);
    std::vector<uint8_t> noise(3000), all(256), text;
    for (auto& b : noise) b = uint8_t(rng());
    for (int i = 0; i < 256; ++i) all[size_t(i)] = uint8_t(i);
    const char* said[] = {"the", "rope", "triangle", "pulls", "tight", "through", "every", "value", "Sieve", "mixes", "TIGHTER", "ropes"};
    for (int i = 0; i < 2000; ++i)
    {
        const std::string w = said[rng() % 12];
        text.insert(text.end(), w.begin(), w.end());
        text.push_back(rng() % 7 ? ' ' : (rng() % 2 ? '\n' : '.'));
    }
    const std::vector<std::vector<uint8_t>> files{{}, {0}, {'a'}, std::vector<uint8_t>(5000, 'e'), noise, all, text};
    bool back = true;
    for (const auto& f : files)
        for (const char* params : {"B16 D60", "B17 D80"})
        {
            const Mix3Params p = Mix3Params::parse(params);
            back = back && mix3_file(mix3_code(f, p, words), p, words, f.size()) == f;
        }
    CHECK(back);
    const Mix3Params p = Mix3Params::parse("B16 D60");
    const std::vector<uint8_t> c = mix3_code(text, p, words);
    // Its start, before mixing has learnt the words: smaller with the list, which knows them.
    const std::vector<uint8_t> start(text.begin(), text.begin() + 200);
    CHECK(mix3_code(start, p, words).size() < mix2_code(start, Mix2Params::parse("B16")).size());
    std::vector<uint8_t> more = c;
    more.insert(more.end(), {1, 2, 3, 4, 5, 6}); // (the reader reads four bytes ahead: fewer read as the code's own)
    CHECK(throws([&] { mix3_file(more, p, words, text.size()); }));
    CHECK(throws([&] { mix3_file(std::span<const uint8_t>(c.data(), c.size() / 2), p, words, text.size()); }));
    const std::string pinned = Sha256::hex(Sha256::hash(std::string(c.begin(), c.end())));
    std::cout << "mix-v3: a text of " << text.size() << " bytes in " << c.size() << ", sha256 " << pinned << "\n";
    CHECK(pinned == "21fe39717297673e2bdc3a9644a9a3e9c3bc26830cb2378b328899589fc915c3");
}

// mix-v4: mix-v2 and mix-v3 without sse-v1's stages (retired): the parameters (text, a picture, a
// word list, never both of the last two); every shape of file given back each way; the same models
// as mix-v2's but no SSE, so a different code; three codes pinned by their SHA-256.
// A mix-v4 run fed in parts (Mix4Run, the mixing speed pass of 12 November 2026): its code is
// mix4_code's for the same bytes, wherever they are cut, a program's through the x86 filter too; its
// size before the end is the code's size were it ended there; and what it refuses.
void test_mix4_run()
{
    std::string text;
    for (int i = 0; i < 2000; ++i) text += "the run is carried on, line " + std::to_string(i * 7919 % 1000) + " of it\n";
    const std::vector<uint8_t> bytes(text.begin(), text.end());
    Mix4Params p = Mix4Params::for_size(bytes.size());
    p.recover = true, p.min_match = 4, p.candidates = 2;
    const std::vector<uint8_t> whole = mix4_code(bytes, p);
    for (const size_t cut : {size_t(0), size_t(1), size_t(4096), bytes.size() / 2, bytes.size() - 1, bytes.size()})
    {
        Mix4Run r(p, bytes.size());
        r.feed(std::span<const uint8_t>(bytes).first(cut));
        if (cut == bytes.size()) CHECK(r.size_if_ended() == whole.size());
        r.feed(std::span<const uint8_t>(bytes).subspan(cut));
        CHECK(r.size_if_ended() == whole.size());
        CHECK(r.end() == whole);
    }
    {
        // The start's size, ended there, is mix4_code's for the start alone (the trial's size).
        const size_t cut = bytes.size() / 3;
        Mix4Run r(p, bytes.size());
        r.feed(std::span<const uint8_t>(bytes).first(cut));
        CHECK(r.size_if_ended() == mix4_code(std::span<const uint8_t>(bytes).first(cut), p).size());
    }
    // A program: the caller filters, the code is mix4_code's with X (and E1).
    std::vector<uint8_t> prog;
    std::mt19937 g(41);
    for (int i = 0; i < 60000; ++i)
        prog.push_back(i % 5 == 0 ? 0xE8 : uint8_t(g() % 7 == 0 ? g() : 0x48 + i % 3));
    Mix4Params px = Mix4Params::for_size(prog.size());
    px.x86 = true, px.instructions = true;
    std::vector<uint8_t> filtered = prog;
    x86_filter(filtered, true);
    Mix4Run rx(px, prog.size());
    rx.feed(std::span<const uint8_t>(filtered).first(20000));
    rx.feed(std::span<const uint8_t>(filtered).subspan(20000));
    CHECK(rx.end() == mix4_code(prog, px));
    // Refused: more than it was made for, a palette, a primer.
    CHECK(throws([&] { Mix4Run r(p, 10); r.feed(std::span<const uint8_t>(bytes).first(11)); }));
    Mix4Params pp = p;
    pp.palette = 4, pp.width = 16, pp.channels = 3;
    CHECK(throws([&] { Mix4Run r(pp, 100); }));
    Mix4Params pw = p;
    pw.primer = 1;
    CHECK(throws([&] { Mix4Run r(pw, 100); }));
}

void test_mix4()
{
    CHECK(Mix4Params::parse("B22").text() == "B22" && Mix4Params::parse("B20 I949x1").width == 949 && Mix4Params::parse("B16 D60").dictionary == 60);
    CHECK(throws([] { Mix4Params::parse("B22 I949x1 D60"); }));
    // v34 draft (E1): the x86 instruction model, after the ways and before the filter; a program's only.
    CHECK(Mix4Params::parse("B20 R1 M4 C2 E1 X").instructions && Mix4Params::parse("B20 R1 M4 C2 E1 X").text() == "B20 R1 M4 C2 E1 X");
    CHECK(Mix4Params::parse("B20 E1").instructions && !Mix4Params::parse("B20 X").instructions);
    CHECK(throws([] { Mix4Params::parse("B20 X E1"); }));
    CHECK(throws([] { Mix4Params::parse("B20 I60x3 E1"); }));
    CHECK(throws([] { Mix4Params::parse("B20 D60 E1"); }));
    {
        // Instructions of every part (prefixes, REX, 0F and 0F 38 opcodes, ModRM with SIB and
        // displacements, immediates of 1 to 8 bytes) and bytes that are none, given back the same.
        std::mt19937 r(34);
        std::vector<uint8_t> prog;
        const std::vector<std::vector<uint8_t>> pieces = {{0x48, 0x89, 0xE5}, {0xE8, 1, 2, 3, 4}, {0x48, 0xB8, 1, 2, 3, 4, 5, 6, 7, 8},
                                                           {0x8B, 0x44, 0x24, 0x08}, {0x0F, 0x84, 9, 0, 0, 0}, {0x66, 0x0F, 0x38, 0x00, 0xC1},
                                                           {0xF3, 0x0F, 0x10, 0x05, 1, 2, 3, 4}, {0xC3}, {0x90}, {0xC7, 0x45, 0xF8, 1, 0, 0, 0}};
        for (int i = 0; i < 4000; ++i)
        {
            const auto& pc = pieces[r() % pieces.size()];
            prog.insert(prog.end(), pc.begin(), pc.end());
            if (r() % 50 == 0) prog.push_back(uint8_t(r())); // (a byte that throws the reading out)
        }
        for (const char* params : {"B16 E1", "B16 R1 M4 C2 E1 X"})
        {
            const Mix4Params p = Mix4Params::parse(params);
            CHECK(mix4_file(mix4_code(prog, p), p, nullptr, prog.size()) == prog);
        }
        CHECK(mix4_code(prog, Mix4Params::parse("B16 R1 M4 C2 E1 X")).size() < mix4_code(prog, Mix4Params::parse("B16 R1 M4 C2 X")).size());
    }
    CHECK(throws([] { Mix4Params::parse("B22 D61"); }));
    CHECK(throws([] { Mix4Params::parse("B22 I1x1"); }));
    CHECK(throws([] { Mix4Params::parse("B23"); }));
    CHECK(throws([] { Mix4Params::parse("B22 "); }));
    CHECK(Mix4Params::for_size(4096, 64, 3).text() == "B16 I64x3" && Mix4Params::for_size(1u << 20, 0, 1, 60).text() == "B21 D60");
    CHECK(throws([] { mix4_code(std::vector<uint8_t>{1, 2}, Mix4Params::parse("B16 D60"), nullptr); })); // a list named, none given
    const std::string list_text = "the\nrope\ntriangle\npulls\ntight\nthrough\nevery\nvalue\nsieve\nmixes\n";
    const MixDictionary words(std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(list_text.data()), list_text.size()));
    std::mt19937 rng(13);
    std::vector<uint8_t> noise(3000), text, picture;
    for (auto& b : noise) b = uint8_t(rng());
    const char* said[] = {"the", "rope", "triangle", "pulls", "tight", "through", "every", "value", "Sieve", "mixes"};
    for (int i = 0; i < 2000; ++i)
    {
        const std::string w = said[rng() % 10];
        text.insert(text.end(), w.begin(), w.end());
        text.push_back(rng() % 8 ? ' ' : '\n');
    }
    for (int y = 0; y < 40; ++y)
        for (int x = 0; x < 60; ++x)
            for (int c = 0; c < 3; ++c) picture.push_back(uint8_t((x * 3 + y * 2 + c * 40 + (x > 30 ? 90 : 0) + int(rng() % 5)) & 255));
    const std::vector<std::vector<uint8_t>> files{{}, {0}, {255}, std::vector<uint8_t>(5000, 'a'), noise, text, picture};
    bool back = true;
    for (const auto& f : files)
        for (const char* params : {"B16", "B16 I60x3", "B16 I2x1", "B16 D60"})
        {
            const Mix4Params p = Mix4Params::parse(params);
            back = back && mix4_file(mix4_code(f, p, &words), p, &words, f.size()) == f;
        }
    CHECK(back);
    const Mix4Params pt = Mix4Params::parse("B16"), pp = Mix4Params::parse("B16 I60x3"), pw = Mix4Params::parse("B16 D60");
    const std::vector<uint8_t> ct = mix4_code(text, pt), cp = mix4_code(picture, pp), cw = mix4_code(text, pw, &words);
    CHECK(ct != mix2_code(text, Mix2Params::parse("B16"))); // no SSE stages: not mix-v2's code
    std::vector<uint8_t> more = ct;
    more.insert(more.end(), {1, 2, 3, 4, 5, 6});
    CHECK(throws([&] { mix4_file(more, pt, nullptr, text.size()); }));
    CHECK(throws([&] { mix4_file(std::span<const uint8_t>(ct.data(), ct.size() / 2), pt, nullptr, text.size()); }));
    auto sha = [](const std::vector<uint8_t>& c) { return Sha256::hex(Sha256::hash(std::string(c.begin(), c.end()))); };
    std::cout << "mix-v4: a text of " << text.size() << " bytes in " << ct.size() << ", sha256 " << sha(ct) << "; with the list in " << cw.size()
              << ", sha256 " << sha(cw) << "; a picture of " << picture.size() << " in " << cp.size() << ", sha256 " << sha(cp) << "\n";
    CHECK(sha(ct) == "41e1ecd2da3a84f4c7230125ff73ed390b5b08d5cde7fb84925959713c21d116");
    CHECK(sha(cw) == "be848447a86d69e0603c8659c9c548d39c2d4e89d0fac5ddb4837af6fa3702e8");
    CHECK(sha(cp) == "300960c718b6a53ab2cf6b1837e6e29798f455ede671303747875f40731d535f");
    // The v33 draft's match ways (R1, M4, C2): written after the rest, each at most once and in that
    // order, refused otherwise; off unless written (the codes above, pinned, are as they were); every
    // file given back with each and all of them; a repetitive program-like file smaller with them.
    CHECK(Mix4Params::parse("B22 R1 M4 C2").text() == "B22 R1 M4 C2" && Mix4Params::parse("B16 D60 C2").candidates == 2);
    CHECK(Mix4Params::parse("B16 I60x3 R1").recover && Mix4Params::parse("B16 M4").min_match == 4 && !match_ways(Mix4Params::parse("B22")));
    CHECK(throws([] { Mix4Params::parse("B22 C2 R1"); }));
    CHECK(throws([] { Mix4Params::parse("B22 R1 R1"); }));
    CHECK(throws([] { Mix4Params::parse("B22 M5"); }));
    CHECK(throws([] { Mix4Params::parse("B22 C3"); }));
    std::vector<uint8_t> code_like;
    for (int k = 0; k < 40; ++k)
        for (int i = 0; i < 400; ++i) code_like.push_back(uint8_t((i * 7 + (i % 23 == k % 23 ? k : 0)) & 255)); // (blocks alike but for a byte here and there)
    bool ways_back = true;
    for (const auto& f : {files[3], noise, text, picture, code_like})
        for (const char* params : {"B16 R1", "B16 M4", "B16 C2", "B16 R1 M4 C2", "B16 I60x3 R1 M4 C2", "B16 D60 R1 M4 C2"})
        {
            const Mix4Params p = Mix4Params::parse(params);
            ways_back = ways_back && mix4_file(mix4_code(f, p, &words), p, &words, f.size()) == f;
        }
    CHECK(ways_back);
    CHECK(mix4_code(code_like, Mix4Params::parse("B16 R1 M4 C2")).size() < mix4_code(code_like, pt).size());
    // The v33 draft's x86 filter (X): written last, not with a picture or a word list; the filter
    // as the LZMA SDK's (a call E8 at 0 to 0 bytes on is made the place it goes, 5; one whose
    // distance's top byte is neither 00 nor FF is left), in place and back; every file given back
    // mixed with it; calls to the same few places, at many places, smaller with it.
    CHECK(Mix4Params::parse("B22 R1 M4 C2 X").text() == "B22 R1 M4 C2 X" && Mix4Params::parse("B16 X").x86 && match_ways(Mix4Params::parse("B16 X")));
    CHECK(throws([] { Mix4Params::parse("B22 X R1"); }));
    CHECK(throws([] { Mix4Params::parse("B22 X X"); }));
    CHECK(throws([] { Mix4Params::parse("B16 I60x3 X"); }));
    CHECK(throws([] { Mix4Params::parse("B16 D60 X"); }));
    {
        std::vector<uint8_t> call{0xE8, 0, 0, 0, 0, 0xE9, 0x10, 0x20, 0x30, 0x12, 0x90, 0x90};
        const std::vector<uint8_t> was = call;
        x86_filter(call, true);
        CHECK((call == std::vector<uint8_t>{0xE8, 5, 0, 0, 0, 0xE9, 0x10, 0x20, 0x30, 0x12, 0x90, 0x90}));
        x86_filter(call, false);
        CHECK(call == was);
        std::vector<uint8_t> tiny{0xE8, 1, 2, 3};
        x86_filter(tiny, true);
        CHECK((tiny == std::vector<uint8_t>{0xE8, 1, 2, 3}));
    }
    std::vector<uint8_t> calls;
    uint32_t seed = 7;
    while (calls.size() < 60000)
    {
        seed = seed * 1103515245u + 12345u;
        const uint32_t target = 0x4000 + ((seed >> 16) % 12) * 0x100; // (twelve places called)
        const uint32_t from = uint32_t(calls.size()) + 5;
        const uint32_t rel = target - from;
        calls.insert(calls.end(), {0x55, 0x89, uint8_t(seed >> 24), 0xE8, uint8_t(rel), uint8_t(rel >> 8), uint8_t(rel >> 16), uint8_t(rel >> 24)});
        // (the call's distance counted from its E8 at calls.size() - 5; the filter counts from there too)
    }
    bool x86_back = true;
    for (const auto& f : {files[3], noise, text, code_like, calls})
        for (const char* params : {"B16 X", "B16 R1 M4 C2 X"})
        {
            const Mix4Params p = Mix4Params::parse(params);
            x86_back = x86_back && mix4_file(mix4_code(f, p), p, nullptr, f.size()) == f;
        }
    CHECK(x86_back);
    CHECK(mix4_code(calls, Mix4Params::parse("B16 X")).size() < mix4_code(calls, Mix4Params::parse("B16")).size());
    // The v33 draft's palette (P): written after a picture's shape, before the match model's ways,
    // only for pixels of 2 to 4 bytes and 1 to 256 colours; the colours as first seen, the rest of
    // the palette zero; more colours than it, or not whole pixels, refused; an index past it
    // refused on the way back; every picture given back mixed with it, and a picture of few colours
    // smaller with it.
    CHECK(Mix4Params::parse("B16 I60x3 P6 R1 C2").text() == "B16 I60x3 P6 R1 C2" && Mix4Params::parse("B16 I60x3 P6").palette == 6 &&
          match_ways(Mix4Params::parse("B16 I60x3 P6")));
    CHECK(throws([] { Mix4Params::parse("B16 I60x1 P6"); }));
    CHECK(throws([] { Mix4Params::parse("B16 I60x3 P257"); }));
    CHECK(throws([] { Mix4Params::parse("B16 P6"); }));
    CHECK(throws([] { Mix4Params::parse("B16 I60x3 R1 P6"); }));
    {
        const std::vector<uint8_t> px{9, 9, 9, 1, 2, 3, 9, 9, 9, 4, 5, 6};
        const auto split = palette_split(px, 3, 4);
        CHECK(split && (*split == std::vector<uint8_t>{0, 1, 0, 2, 9, 9, 9, 1, 2, 3, 4, 5, 6, 0, 0, 0}));
        CHECK(split && palette_join(*split, 3, 4) == px);
        CHECK(!palette_split(px, 3, 2));
        CHECK(!palette_split(std::vector<uint8_t>{1, 2, 3, 4}, 3, 4));
        CHECK(colours_of(px, 3) == 3 && colours_of(std::vector<uint8_t>{1, 2, 3, 4}, 3) == 0);
        CHECK(throws([] { palette_join(std::vector<uint8_t>{0, 2, 9, 9, 9, 1, 2, 3}, 3, 2); }));
    }
    std::vector<uint8_t> drawing;
    {
        uint32_t r = 11;
        const uint8_t cols[5][3] = {{250, 250, 245}, {30, 30, 40}, {200, 60, 50}, {40, 120, 200}, {90, 170, 90}};
        std::vector<uint8_t> idx(96 * 64);
        for (size_t i = 0; i < idx.size(); ++i)
        {
            r = r * 1103515245u + 12345u;
            const uint32_t v = (r >> 16) % 100;
            idx[i] = v < 70 && i % 96 ? idx[i - 1] : v < 93 && i >= 96 ? idx[i - 96] : uint8_t(v % 5);
        }
        for (const uint8_t k : idx) drawing.insert(drawing.end(), cols[k], cols[k] + 3);
    }
    bool palette_back = true;
    for (const char* params : {"B16 I96x3 P5", "B16 I96x3 P5 R1 M4 C2", "B16 I96x3 P200"})
    {
        const Mix4Params p = Mix4Params::parse(params);
        palette_back = palette_back && mix4_file(mix4_code(drawing, p), p, nullptr, drawing.size()) == drawing;
    }
    CHECK(palette_back);
    CHECK(throws([&] { mix4_code(drawing, Mix4Params::parse("B16 I96x3 P4")); }));
    CHECK(mix4_code(drawing, Mix4Params::parse("B16 I96x3 P5")).size() < mix4_code(drawing, Mix4Params::parse("B16 I96x3")).size());
    // v33 (reopened 3 November 2026): W1, the model primed on a dictionary first. Written after the
    // word list and before the match ways; text only; refused where no primer can be had. Here a
    // primer of the test's own (the hook the CLI sets reads the registered file): a page primed on
    // pages like it given back exactly, with and without the word list and the ways, and smaller.
    CHECK(Mix4Params::parse("B16 F80 W1 R1 C2").text() == "B16 F80 W1 R1 C2" && Mix4Params::parse("B16 W1").primer == 1 && match_ways(Mix4Params::parse("B16 W1")));
    CHECK(throws([] { Mix4Params::parse("B16 W2"); }));
    CHECK(throws([] { Mix4Params::parse("B16 R1 W1"); }));
    CHECK(throws([] { Mix4Params::parse("B16 I60x3 W1"); }));
    CHECK(throws([] { Mix4Params::parse("B16 W1 X"); }));
    std::string pages;
    for (int i = 0; i < 40; ++i)
        pages += "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <title>Page " + std::to_string(i) +
                 "</title>\n</head>\n<body>\n  <div class=\"box\"><p>the rope pulls tight through every value</p></div>\n</body>\n</html>\n";
    const std::vector<uint8_t> primer_bytes(pages.begin(), pages.end());
    std::string page_text = "<!DOCTYPE html>\n<html lang=\"en\">\n<head>\n  <meta charset=\"utf-8\">\n  <title>Sieve</title>\n</head>\n<body>\n";
    for (int i = 0; i < 12; ++i) page_text += "  <div class=\"box\"><p>" + std::string(said[rng() % 10]) + " " + said[rng() % 10] + "</p></div>\n";
    page_text += "</body>\n</html>\n";
    const std::vector<uint8_t> page(page_text.begin(), page_text.end());
    CHECK(throws([&] { mix4_code(page, Mix4Params::parse("B16 W1")); })); // no primer to be had
    set_mix_primer([&](uint32_t) { return std::span<const uint8_t>(primer_bytes); });
    bool primed_back = true;
    for (const char* params : {"B16 W1", "B16 D60 W1", "B16 W1 R1 M4 C2"})
    {
        const Mix4Params p = Mix4Params::parse(params);
        primed_back = primed_back && mix4_file(mix4_code(page, p, &words), p, &words, page.size()) == page;
        primed_back = primed_back && mix4_file(mix4_code(std::vector<uint8_t>{}, p, &words), p, &words, 0).empty();
    }
    CHECK(primed_back);
    CHECK(mix4_code(page, Mix4Params::parse("B16 W1")).size() * 2 < mix4_code(page, Mix4Params::parse("B16")).size());
    set_mix_primer(nullptr);
    {
        // An install's Cancel (DecodeCancelScope): a mixing decode stops with DecodeCancelled once
        // the flag is set, and decodes as before while it is not, or with no scope at all.
        std::vector<uint8_t> big(200000);
        for (size_t i = 0; i < big.size(); ++i) big[i] = uint8_t(i * 7 % 251);
        const Mix4Params bp = Mix4Params::parse("B16");
        const std::vector<uint8_t> code = mix4_code(big, bp, nullptr);
        std::atomic<bool> flag{false};
        {
            const DecodeCancelScope scope(&flag);
            CHECK(mix4_file(code, bp, nullptr, big.size()) == big);
            flag = true;
            bool cancelled = false;
            try { (void)mix4_file(code, bp, nullptr, big.size()); }
            catch (const DecodeCancelled&) { cancelled = true; }
            CHECK(cancelled);
        }
        CHECK(!decoding_cancelled() && mix4_file(code, bp, nullptr, big.size()) == big);
    }
}

void test_rope_wave()
{
    // The one reader of Sieve's own numbers (sieve/varint.hpp): only the shortest form is read.
    {
        auto read = [](std::vector<uint8_t> b, uint64_t& v) {
            size_t at = 0;
            return sieve::read_varint(std::span<const uint8_t>(b.data(), b.size()), at, v);
        };
        uint64_t v = 0;
        CHECK(read({0x00}, v) == sieve::VarintFault::none && v == 0);
        CHECK(read({0xE5, 0x8E, 0x26}, v) == sieve::VarintFault::none && v == 624485);
        CHECK(read({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01}, v) == sieve::VarintFault::none && v == UINT64_MAX);
        CHECK(read({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02}, v) == sieve::VarintFault::too_long); // (bit 64)
        CHECK(read({0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01}, v) == sieve::VarintFault::too_long);
        CHECK(read({0x85, 0x00}, v) == sieve::VarintFault::not_canonical); // (5, written in two bytes)
        CHECK(read({0x85}, v) == sieve::VarintFault::ends);
        size_t at = 0;
        const std::vector<uint8_t> nine = {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x01};
        CHECK(sieve::read_varint(std::span<const uint8_t>(nine.data(), nine.size()), at, v, 9) == sieve::VarintFault::too_long);
    }
    // The parameters as written, and refused when written otherwise or out of range.
    CHECK(RopeParams::parse("L8 W949 S1 P3 K0").text() == "L8 W949 S1 P3 K0");
    CHECK((RopeParams::parse("L16 W1 S1 P2 K1") == RopeParams{16, 1, 1, 2, true}));
    CHECK(throws([] { RopeParams::parse("L12 W1 S1 P0 K0"); }));
    CHECK(throws([] { RopeParams::parse("L8 W0 S1 P0 K0"); }));
    CHECK(throws([] { RopeParams::parse("L8 W1 S1 P5 K0"); }));
    CHECK(throws([] { RopeParams::parse("L8 W01 S1 P0 K0"); }));
    // rope-wave-v2: columns, written " C1"; lines of two values or more.
    CHECK((RopeParams::parse("L8 W111 S1 P0 K0 C1") == RopeParams{8, 111, 1, 0, false, 1}));
    CHECK(throws([] { RopeParams::parse("L8 W1 S1 P0 K0 C1"); }));
    CHECK(throws([] { RopeParams::parse("L8 W5 S1 P0 K0 C0"); }));
    // rope-wave-v3: columns by line, bytes with no wave only.
    CHECK((RopeParams::parse("L8 W56 S1 P0 K0 C2") == RopeParams{8, 56, 1, 0, false, 2}));
    CHECK(throws([] { RopeParams::parse("L16 W56 S1 P0 K0 C2"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P1 K0 C2"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K1 C2"); }));
    // rope-wave-v4: the columns in groups, " G1" after C1 or C2, and nothing else.
    CHECK((RopeParams::parse("L8 W56 S1 P0 K0 C2 G1") == RopeParams{8, 56, 1, 0, false, 2, true}));
    CHECK(RopeParams::parse("L16 W12 S1 P2 K0 C1 G1").text() == "L16 W12 S1 P2 K0 C1 G1");
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 G1"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 G0"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 G2"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 G1 "); }));
    // rope-wave-v5: the byte above, " A1" after C2 G1 only.
    CHECK((RopeParams::parse("L8 W56 S1 P0 K0 C2 G1 A1") == RopeParams{8, 56, 1, 0, false, 2, true, true}));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 A1"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C1 G1 A1"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 G1 A2"); }));
    CHECK(throws([] { RopeParams::parse("L8 W56 S1 P0 K0 C2 G1 A0"); }));
    // rope-wave-v6: the triangle (P4), and its contexts (" T1" after K, with P3 or P4, no columns).
    {
        RopeParams t{8, 949, 1, 4, false};
        t.triangle = true;
        CHECK(RopeParams::parse("L8 W949 S1 P4 K0 T1") == t);
        CHECK(RopeParams::parse("L16 W7 S2 P3 K1 T1").text() == "L16 W7 S2 P3 K1 T1");
        CHECK(RopeParams::parse("L8 W949 S1 P4 K0").text() == "L8 W949 S1 P4 K0");
        CHECK(rope_v6(RopeParams::parse("L8 W949 S1 P4 K0")) && !rope_v6(RopeParams::parse("L8 W949 S1 P3 K0")));
        CHECK(throws([] { RopeParams::parse("L8 W9 S1 P2 K0 T1"); }));
        CHECK(throws([] { RopeParams::parse("L8 W9 S1 P4 K0 C1 T1"); }));
        CHECK(throws([] { RopeParams::parse("L8 W9 S1 P4 K0 T0"); }));
        CHECK(throws([] { RopeParams::parse("L8 W9 S1 P4 K0 T1 "); }));
    }
    // A file that is the rope exactly (x_i = i, 100 values, kept at 0, 33, 66 and 99: an even count
    // has no centre value): nothing is left, so its code is the table of one byte (1 + 1 + 4), its
    // stream's two words (1 + 8) and the four kept bytes.
    std::vector<uint8_t> ramp(100);
    for (size_t i = 0; i < ramp.size(); ++i) ramp[i] = uint8_t(i);
    CHECK(rope_code(ramp, RopeParams{8, 1, 1, 0, true}).size() == 19);
    // Every parameter, on files of every kind of shape: random, a smooth picture in lines, 16-bit
    // values with a trailing byte, the empty file and one byte; each comes back exactly.
    std::mt19937 rng(11);
    std::vector<std::vector<uint8_t>> files{{}, {7}, std::vector<uint8_t>(1, 255)};
    {
        std::vector<uint8_t> r(257);
        for (auto& b : r) b = uint8_t(rng());
        files.push_back(r);
        std::vector<uint8_t> pic(24 * 17);
        for (size_t y = 0; y < 17; ++y)
            for (size_t x = 0; x < 24; ++x) pic[y * 24 + x] = uint8_t(x * 7 + y * 3 + (rng() % 3));
        files.push_back(pic);
        std::vector<uint8_t> sound;
        for (int i = 0; i < 301; ++i)
        {
            const int16_t v = int16_t(8000 * std::sin(i * 0.07));
            sound.push_back(uint8_t(v & 0xFF));
            sound.push_back(uint8_t((v >> 8) & 0xFF));
        }
        sound.push_back(42); // a byte past the last whole value
        files.push_back(sound);
    }
    bool back = true;
    for (const auto& f : files)
        for (const uint32_t bits : {8u, 16u, 32u})
            for (const uint32_t width : {1u, 3u, 24u})
                for (uint32_t step = 1; step <= 4; ++step)
                    for (uint32_t phases = 0; phases <= 3; ++phases)
                        for (const bool keep : {false, true})
                            for (const uint32_t columns : {0u, 1u})
                                for (const bool grouped : {false, true})
                                {
                                    if ((columns && width < 2) || (grouped && !columns)) continue;
                                    const RopeParams p{bits, width, step, phases, keep, columns, grouped};
                                    back = back && rope_file(rope_code(f, p), p, f.size()) == f;
                                }
    CHECK(back);
    // rope-wave-v6: the triangle and its contexts, on every shape of file too.
    bool back6 = true;
    for (const auto& f : files)
        for (const uint32_t bits : {8u, 16u, 32u})
            for (const uint32_t width : {1u, 3u, 24u})
                for (uint32_t step = 1; step <= 4; ++step)
                    for (const uint32_t phases : {3u, 4u})
                        for (const bool keep : {false, true})
                            for (const bool tri : {false, true})
                                for (const uint32_t columns : {0u, 1u})
                                {
                                    if ((columns && (width < 2 || tri)) || (phases == 3 && !tri)) continue;
                                    RopeParams p{bits, width, step, phases, keep, columns};
                                    p.triangle = tri;
                                    back6 = back6 && rope_file(rope_code(f, p), p, f.size()) == f;
                                }
    CHECK(back6);
    {
        // On a picture with an edge across it (160 by 120, smooth either side, a little noise) the
        // triangle's contexts code it smaller than the plane (P3) does; on the small one, where nine
        // tables cost more than they save, the search need not pick them. A code cut short, or a
        // size not its, is refused.
        std::vector<uint8_t> edge(160 * 120);
        for (size_t y = 0; y < 120; ++y)
            for (size_t x = 0; x < 160; ++x) edge[y * 160 + x] = uint8_t((x > y ? 150 : 20) + x / 4 + y / 5 + (rng() % 4));
        RopeParams t{8, 160, 1, 4, false};
        t.triangle = true;
        CHECK(rope_code(edge, t).size() < rope_code(edge, RopeParams{8, 160, 1, 3, false}).size());
        CHECK(rope_file(rope_code(edge, t), t, edge.size()) == edge);
        t.width = 24;
        const std::vector<uint8_t> c = rope_code(files[4], t);
        CHECK(throws([&] { rope_file(std::span<const uint8_t>(c.data(), c.size() - 1), t, files[4].size()); }));
        CHECK(throws([&] { rope_file(c, t, files[4].size() + 1); }));
    }
    // A code that does not give the file is refused: one byte short, one too many, a size not its.
    {
        const RopeParams p{8, 24, 1, 3, true};
        const std::vector<uint8_t> c = rope_code(files[4], p);
        CHECK(throws([&] { rope_file(std::span<const uint8_t>(c.data(), c.size() - 1), p, files[4].size()); }));
        std::vector<uint8_t> longer = c;
        longer.push_back(0);
        CHECK(throws([&] { rope_file(longer, p, files[4].size()); }));
        CHECK(throws([&] { rope_file(c, p, files[4].size() + 1); }));
    }
    // Columns: digits in lines of ten with a space and a line feed after each line, every column of
    // separators one byte value, so a table alone (no stream); the digits about log2(10) bits each.
    {
        std::vector<uint8_t> table;
        for (int line = 0; line < 4000; ++line)
        {
            for (int d = 0; d < 10; ++d) table.push_back(uint8_t('0' + rng() % 10));
            table.push_back(' ');
            table.push_back('\n');
        }
        const RopeParams flat{8, 1, 1, 0, false, 0}, cols{8, 12, 1, 0, false, 1};
        const size_t c1 = rope_code(table, cols).size(), c0 = rope_code(table, flat).size();
        CHECK(c1 < c0);
        CHECK(c1 < size_t(40000 * std::log2(10.0) / 8) + 12 * 50);
        CHECK(rope_file(rope_code(table, cols), cols, table.size()) == table);
        const RopeSearch found = rope_search(table);
        CHECK(found.best.columns == 1 && found.best.width % 12 == 0);
    }
    // Columns by line: the same digits in lines of 54 and 55 bytes, a blank line or a form feed
    // after every twenty, which shift any fixed width; counted from each line's start, the
    // separators cost almost nothing again. Every file of the round trips above comes back too.
    {
        std::vector<uint8_t> text;
        for (int block = 0; block < 300; ++block)
        {
            for (int line = 0; line < 20; ++line)
            {
                for (int g = 0; g < 5; ++g)
                {
                    for (int d = 0; d < 10; ++d) text.push_back(uint8_t('0' + rng() % 10));
                    if (g < 4 || line % 2 == 0) text.push_back(' ');
                }
                text.push_back('\n');
            }
            if (block % 10 == 9) text.push_back(12);
            text.push_back('\n');
        }
        const RopeParams byline{8, 56, 1, 0, false, 2}, flat{8, 1, 1, 0, false, 0};
        const size_t c2 = rope_code(text, byline).size();
        CHECK(c2 < rope_code(text, flat).size());
        CHECK(c2 < size_t(300 * 20 * 50 * std::log2(10.0) / 8) + 56 * 70); // each column's table, count and stream: about 65 bytes
        CHECK(rope_file(rope_code(text, byline), byline, text.size()) == text);
        // rope-wave-v4: the digits' columns share one table, the separators' columns theirs.
        const RopeParams grouped{8, 56, 1, 0, false, 2, true};
        const size_t g2 = rope_code(text, grouped).size();
        CHECK(g2 + 56 * 30 < c2); // measured: 2,187 bytes smaller (128,145 and 125,958)
        CHECK(rope_file(rope_code(text, grouped), grouped, text.size()) == text);
        // rope-wave-v5: the byte above sees what the column alone cannot (the last column's space
        // and line feed taking turns, the line after a blank one); smaller again, and the search's.
        const RopeParams above{8, 56, 1, 0, false, 2, true, true};
        const size_t a2 = rope_code(text, above).size();
        CHECK(a2 + 200 < g2);
        CHECK(rope_file(rope_code(text, above), above, text.size()) == text);
        CHECK(rope_search(text).best == above);
        CHECK(rope_search(text, 0, 4).best == grouped); // the newest version 4: v4's
        std::cout << "rope-wave-v3, v4 and v5 on lines of digits: " << c2 << ", " << g2 << " and " << a2 << " bytes\n";
        bool all = true;
        for (const auto& f : files)
            for (const uint32_t w : {2u, 7u, 300u})
                for (const int g : {0, 1, 2}) // v3, v4, v5
                {
                    const RopeParams p{8, w, 1, 0, false, 2, g >= 1, g == 2};
                    all = all && rope_file(rope_code(f, p), p, f.size()) == f;
                }
        CHECK(all);
        // A column's group number past the groups started before it is refused: the first column's
        // must be 0.
        std::vector<uint8_t> past = rope_code(text, grouped);
        past[0] = 1;
        CHECK(throws([&] { rope_file(past, grouped, text.size()); }));
        // v5: one pair fewer than the file meets is refused, as is a code cut short.
        std::vector<uint8_t> fewer = rope_code(text, above);
        fewer[0] = uint8_t(fewer[0] - 1); // the pairs' count (a varint; its low byte here is not 0)
        CHECK(throws([&] { rope_file(fewer, above, text.size()); }));
        std::vector<uint8_t> cut = rope_code(text, above);
        cut.pop_back();
        CHECK(throws([&] { rope_file(cut, above, text.size()); }));
        std::vector<uint8_t> shorter = rope_code(text, byline);
        shorter.pop_back();
        CHECK(throws([&] { rope_file(shorter, byline, text.size()); }));
    }
    // Pulling tight: on the smooth picture the search finds its lines and propagates through them,
    // and its code is far smaller than the picture.
    const RopeSearch s = rope_search(files[4]);
    CHECK(s.best.phases >= 2 && s.best.width == 24);
    CHECK(rope_code(files[4], s.best).size() * 2 < files[4].size());
    CHECK(s.tried > 10 && !s.timed_out);
}

void test_digit_histogram()
{
    // Every row of 5 digits of 3 values, cut 2 + 3: one number each, all different, and back.
    {
        const std::vector<uint64_t> seg{2, 3};
        std::set<std::string> seen;
        bool back = true;
        for (uint32_t v = 0; v < 243; ++v)
        {
            std::vector<uint8_t> d(5);
            for (uint32_t i = 0, x = v; i < 5; ++i, x /= 3) d[4 - i] = uint8_t(x % 3);
            const BigUint z = digit_histogram_number(d, seg, 3);
            seen.insert(z.to_hex());
            back = back && digit_histogram_digits(z, seg, 3) == d;
        }
        CHECK(seen.size() == 243);
        CHECK(back);
        // C = C(4, 2) * C(5, 2) = 60; the rows with A = 0 (c = (0,0,2), (0,0,3)) are 1 * 1, so
        // Z = 60 names none.
        CHECK(throws([&] { digit_histogram_digits(BigUint(60), seg, 3); }));
        CHECK(small_binomial(5, 2) == BigUint(10));
        CHECK(small_binomial(2, 5).is_zero());
    }
    // A model's shape at 4 bits: digits spread evenly by chance cost a little more than their
    // place, digits spread unevenly less; and every one comes back.
    {
        const std::vector<uint64_t> seg{4096, 1024, 1024, 1024, 256, 256, 256, 256};
        std::mt19937 rng(7);
        std::vector<uint8_t> even(8192), uneven(8192);
        for (auto& d : even) d = uint8_t(rng() % 16);
        for (auto& d : uneven) d = uint8_t(rng() % 3 == 0 ? (rng() % 2 ? 0 : 15) : rng() % 16);
        const BigUint ze = digit_histogram_number(even, seg, 16), zu = digit_histogram_number(uneven, seg, 16);
        CHECK(ze.bit_length() > 32768 - 64 && ze.bit_length() < 32768 + 1024);
        CHECK(zu.bit_length() < 32768);
        CHECK(digit_histogram_digits(ze, seg, 16) == even);
        CHECK(digit_histogram_digits(zu, seg, 16) == uneven);
        CHECK(throws([&] { digit_histogram_number(std::vector<uint8_t>{16}, std::vector<uint64_t>{1}, 16); }));
        CHECK(throws([&] { digit_histogram_number(even, std::vector<uint64_t>{8191}, 16); }));
    }
}

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

// Characters (sieve-character-v1, IDEAS 19.148): the same seed and settings give the same tree, a
// different seed another; every parent comes before its child; bilateral characters are their own
// mirror; radial ones keep to the horizontal ring; the sphere count follows the settings; and the
// settings out of range are refused. Two characters are pinned by their SHA-256, so any change to
// the unfolding shows (it would be a new version).
void test_character()
{
    CharacterSettings s;
    s.seed = 7;
    const Character a = unfold_character(s), b = unfold_character(s);
    CHECK(a.text() == b.text());
    s.seed = 8;
    CHECK(unfold_character(s).text() != a.text());
    for (uint64_t seed = 0; seed < 40; ++seed)
        for (const Symmetry sym : {Symmetry::None, Symmetry::Bilateral, Symmetry::Radial})
        {
            CharacterSettings t;
            t.seed = seed;
            t.symmetry = sym;
            t.limbs = uint32_t(seed % 9);
            t.joints = uint32_t(1 + seed % 6);
            t.branches = uint32_t(seed % 4);
            t.head = seed % 3 != 0;
            const Character c = unfold_character(t);
            const size_t branch_joints = (t.joints + 1) / 2;
            CHECK(c.spheres.size() == 1 + (t.head ? 1u : 0u) + t.limbs * (t.joints + t.branches * branch_joints));
            for (size_t i = 0; i < c.spheres.size(); ++i)
            {
                CHECK(c.spheres[i].parent < int32_t(i));
                CHECK(c.spheres[i].r >= 8);
                CHECK((i == 0) == (c.spheres[i].parent == -1));
            }
            if (sym == Symmetry::Bilateral)
                for (const Sphere& p : c.spheres)
                {
                    bool found = false;
                    for (const Sphere& q : c.spheres) found = found || (q.x == -p.x && q.y == p.y && q.z == p.z && q.r == p.r);
                    CHECK(found);
                }
            if (sym == Symmetry::Radial && t.limbs > 0)
                for (size_t i = t.head ? 2 : 1; i < c.spheres.size(); i += t.joints + t.branches * branch_joints)
                    CHECK(c.spheres[i].y == 0); // each limb's first joint, on the ring
            int32_t ex, ey, ez;
            c.extent(ex, ey, ez);
            CHECK(ex >= c.spheres[0].r && ey >= c.spheres[0].r && ez >= c.spheres[0].r);
        }
    CharacterSettings bad;
    bad.limbs = 17;
    CHECK(throws([&] { (void)unfold_character(bad); }));
    bad.limbs = 4;
    bad.joints = 0;
    CHECK(throws([&] { (void)unfold_character(bad); }));
    bad.joints = 3;
    bad.branches = 4;
    CHECK(throws([&] { (void)unfold_character(bad); }));
    CHECK(throws([] { (void)symmetry_from_name("lopsided"); }));
    CHECK(symmetry_from_name("radial") == Symmetry::Radial);
    // Read back: the whole text, the settings line alone, and with Windows line endings; a changed
    // sphere, a missing setting and another version refused.
    {
        CharacterSettings t;
        t.seed = 99;
        t.limbs = 5;
        t.branches = 1;
        const Character c = unfold_character(t);
        CHECK(read_character(c.text()).id() == c.id());
        CHECK(read_character(c.text().substr(0, c.text().find('\n') + 1)).id() == c.id());
        std::string crlf;
        for (const char ch : c.text()) crlf += ch == '\n' ? std::string("\r\n") : std::string(1, ch);
        CHECK(read_character(crlf).id() == c.id());
        std::string changed = c.text();
        changed[changed.rfind(" joint") - 1] = changed[changed.rfind(" joint") - 1] == '9' ? '8' : '9';
        CHECK(throws([&] { (void)read_character(changed); }));
        CHECK(throws([] { (void)read_character("sieve-character-v1 seed=1 limbs=4 joints=3 branches=0 head=yes\n"); }));
        CHECK(throws([] { (void)read_character("sieve-character-v2 seed=1\n"); }));
        CHECK(throws([] { (void)read_character("sieve-character-v1 seed=1 limbs=40 joints=3 branches=0 symmetry=none head=yes\n"); }));
    }
    // v2: models held in spheres. A character holding none is v1 still (its identity unchanged);
    // one holding models reads back as v2 with them; a model on a missing sphere, a bad address,
    // two models in one sphere, and a v2 holding nothing are refused. The held models' address width
    // is reported, for the specification.
    {
        CharacterSettings t;
        t.seed = 3;
        Character c = unfold_character(t);
        const std::string v1 = c.text(), id1 = c.id();
        const ModelSpace m(kHeldVertices, kHeldFaces, kHeldCoords);
        const std::string a = m.hex_of(BigUint(12345)), b = m.hex_of(BigUint(678));
        c.hold(0, a);
        c.hold(1, b);
        CHECK(c.text().rfind("sieve-character-v2 ", 0) == 0);
        CHECK(c.id() != id1);
        const Character back = read_character(c.text());
        CHECK(back.id() == c.id() && back.holds.size() == 2 && *back.held(0) == a);
        CHECK(read_character(c.text().substr(0, c.text().find('\n') + 1) + "h 0 " + a + "\nh 1 " + b + "\n").id() == c.id());
        c.hold(0, "");
        c.hold(1, "");
        CHECK(c.text() == v1 && c.id() == id1);
        CHECK(throws([&] { c.hold(9999, a); }));
        CHECK(throws([&] { c.hold(0, "xyz"); }));
        const std::string head2 = "sieve-character-v2" + v1.substr(v1.find(' '), v1.find('\n') - v1.find(' ')) + "\n";
        CHECK(throws([&] { (void)read_character(head2); }));
        CHECK(throws([&] { (void)read_character(head2 + "h 0 " + a + "\nh 0 " + b + "\n"); }));
    }
    // The seed walk: the same start walks the same way; two steps are one step twice; each step a
    // valid character; pinned at ten steps from the default.
    {
        const CharacterSettings start;
        CHECK(unfold_character(walk_character(start, 2)).id() == unfold_character(walk_character(walk_character(start, 1), 1)).id());
        CharacterSettings w = start;
        for (int k = 0; k < 50; ++k)
        {
            w = walk_character(w);
            CHECK(unfold_character(w).spheres.size() >= 1);
        }
        CHECK(unfold_character(walk_character(start, 10)).id() == "775f9b13e7bb87599c40b20aa700f7444da6aca7a09c4ff2a2a2101593c08688");
    }
    // Species: the seed free, the rest fixed; two seeds of one species share it, another limb count
    // does not.
    {
        CharacterSettings a1, a2, b;
        a1.seed = 5;
        a2.seed = 6;
        b.seed = 5;
        b.limbs = 5;
        CHECK(species_id(a1) == species_id(a2));
        CHECK(species_id(a1) != species_id(b));
        CHECK(species_text(a1) == "sieve-species-v1 limbs=4 joints=3 branches=0 symmetry=bilateral head=yes");
    }
    // Importing an .obj into a sphere: a cube of 8 vertices and 12 triangles comes in whole (8 of 8,
    // 12 of 12); the same file gives the same model; a file with no vertices is refused.
    {
        const std::string cube = "v -1 -1 -1\nv 1 -1 -1\nv 1 1 -1\nv -1 1 -1\nv -1 -1 1\nv 1 -1 1\nv 1 1 1\nv -1 1 1\n"
                                 "f 1 2 3 4\nf 5 8 7 6\nf 1 5 6 2\nf 2 6 7 3\nf 3 7 8 4\nf 5 1 4 8\n";
        const HeldImport a = import_obj_for_hold(cube), b = import_obj_for_hold(cube);
        CHECK(a.hex == b.hex && a.vertices_in == 8 && a.faces_in == 12 && a.faces_kept == 12);
        CHECK(held_model_edges(a.hex).size() >= 12);
        CHECK(throws([] { (void)import_obj_for_hold("o empty\n"); }));

        // The same cube as glTF (IDEAS 19.162's importer layer): its buffer embedded as a data: URI,
        // then as a .glb, then beside it as a file; each comes in as the .obj did, to the same model.
        std::string bin;
        auto put_f = [&](float f) { char b[4]; std::memcpy(b, &f, 4); bin.append(b, 4); };
        auto put_u16 = [&](uint16_t v) { bin.push_back(char(v & 0xFF)); bin.push_back(char(v >> 8)); };
        const float P[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
        for (const auto& v : P) put_f(v[0]), put_f(v[1]), put_f(v[2]);
        const uint16_t quads[6][4] = {{0, 1, 2, 3}, {4, 7, 6, 5}, {0, 4, 5, 1}, {1, 5, 6, 2}, {2, 6, 7, 3}, {4, 0, 3, 7}};
        for (const auto& q : quads) put_u16(q[0]), put_u16(q[1]), put_u16(q[2]), put_u16(q[0]), put_u16(q[2]), put_u16(q[3]);
        CHECK(bin.size() == 96 + 72);
        auto b64 = [](const std::string& in) {
            static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string o;
            for (size_t i = 0; i < in.size(); i += 3)
            {
                uint32_t n = uint32_t(uint8_t(in[i])) << 16;
                if (i + 1 < in.size()) n |= uint32_t(uint8_t(in[i + 1])) << 8;
                if (i + 2 < in.size()) n |= uint32_t(uint8_t(in[i + 2]));
                o += t[n >> 18 & 63];
                o += t[n >> 12 & 63];
                o += i + 1 < in.size() ? t[n >> 6 & 63] : '=';
                o += i + 2 < in.size() ? t[n & 63] : '=';
            }
            return o;
        };
        auto gltf = [&](const std::string& buffer, const std::string& extra, const std::string& node) {
            return "{\"asset\":{\"version\":\"2.0\"}," + extra + "\"buffers\":[" + buffer +
                   "],\"bufferViews\":[{\"buffer\":0,\"byteLength\":96},{\"buffer\":0,\"byteOffset\":96,\"byteLength\":72}],"
                   "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":8,\"type\":\"VEC3\"},"
                   "{\"bufferView\":1,\"componentType\":5123,\"count\":36,\"type\":\"SCALAR\"},"
                   "{\"componentType\":5126,\"count\":5,\"type\":\"SCALAR\"}],"
                   "\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"indices\":1}]}],"
                   "\"nodes\":[" + node + "],\"scenes\":[{\"nodes\":[0]}]}";
        };
        const std::string embedded = "{\"byteLength\":168,\"uri\":\"data:application/octet-stream;base64," + b64(bin) + "\"}";
        const std::string plain = gltf(embedded, "", "{\"mesh\":0}");
        const HeldImport g = import_model_for_hold(plain);
        CHECK(g.format == "gltf" && g.hex == a.hex && g.vertices_in == 8 && g.faces_in == 12 && g.faces_kept == 12 && g.left_out.empty());
        // As a .glb: the JSON chunk (padded with spaces to 4 bytes) and the BIN chunk.
        std::string js = gltf("{\"byteLength\":168}", "", "{\"mesh\":0}");
        while (js.size() % 4) js += ' ';
        auto le = [](uint32_t v) { std::string o(4, '\0'); for (int i = 0; i < 4; ++i) o[size_t(i)] = char(v >> (8 * i) & 0xFF); return o; };
        const std::string glb = "glTF" + le(2) + le(uint32_t(12 + 8 + js.size() + 8 + bin.size())) + le(uint32_t(js.size())) + "JSON" + js +
                                le(uint32_t(bin.size())) + std::string("BIN\0", 4) + bin;
        const HeldImport gb = import_model_for_hold(glb);
        CHECK(gb.format == "glb" && gb.hex == a.hex);
        // Beside it as a file, read through the loader; a URI that leaves the folder is refused.
        const std::string beside = gltf("{\"byteLength\":168,\"uri\":\"cube%20data.bin\"}", "", "{\"mesh\":0}");
        std::string asked;
        const HeldImport gf = import_model_for_hold(beside, [&](const std::string& uri) -> std::optional<std::string> { asked = uri; return bin; });
        CHECK(gf.hex == a.hex && asked == "cube data.bin");
        CHECK(throws([&] { (void)import_model_for_hold(gltf("{\"byteLength\":168,\"uri\":\"../cube.bin\"}", "", "{\"mesh\":0}"),
                                                       [&](const std::string&) -> std::optional<std::string> { return bin; }); }));
        CHECK(throws([&] { (void)import_model_for_hold(beside); })); // no loader: the buffer cannot be reached
        CHECK(!import_uri_is_local("/etc/x.bin") && !import_uri_is_local("c:x.bin") && !import_uri_is_local("a/../../x") &&
              !import_uri_is_local("http://x/y.bin") && import_uri_is_local("parts/body.bin"));
        // Placed by its node: moved 10 along x and turned a quarter about z, (1, -1, -1) lands at (11, 1, -1).
        const ImportedModel placed = read_model(gltf(embedded, "", "{\"mesh\":0,\"translation\":[10,0,0],\"rotation\":[0,0,0.70710678,0.70710678]}"));
        CHECK(std::abs(placed.mesh.verts[1].x - 11) < 1e-4 && std::abs(placed.mesh.verts[1].y - 1) < 1e-4 && std::abs(placed.mesh.verts[1].z + 1) < 1e-4);
        // Skins and animations are counted.
        const ImportedModel rigged = read_model(gltf(embedded, "\"skins\":[{\"joints\":[0]}],\"animations\":[{\"channels\":[{},{}],\"samplers\":[{\"input\":2}]}],",
                                                     "{\"mesh\":0}"));
        // (Since IDEAS 19.170 the skin comes in as a skeleton of its one joint, and the animation, whose
        // channels name no target, is sampled as one frame of no turn.)
        CHECK(rigged.skins == 1 && rigged.joints == 1 && rigged.animations == 1 && rigged.channels == 2 && rigged.keyframes == 5 && rigged.left_out.empty());
        CHECK(rigged.skeleton.size() == 1 && rigged.sampled.size() == 1 && rigged.sampled[0].frames == 1);
        // Refused: Draco compression it cannot undo; an accessor past its buffer; not glTF 2.0.
        CHECK(throws([&] { (void)read_model(gltf(embedded, "\"extensionsRequired\":[\"KHR_draco_mesh_compression\"],", "{\"mesh\":0}")); }));
        std::string past = plain;
        past.replace(past.find("\"count\":36"), 11, "\"count\":37");
        CHECK(throws([&] { (void)read_model(past); }));
        std::string v1 = plain;
        v1.replace(v1.find("\"2.0\""), 5, "\"1.0\"");
        CHECK(throws([&] { (void)read_model(v1); }));
        // A strip of 4 vertices is 2 triangles, the second turned to keep the winding.
        const std::string strip = "{\"asset\":{\"version\":\"2.0\"},\"buffers\":[" + embedded +
                                  "],\"bufferViews\":[{\"buffer\":0,\"byteLength\":48}],\"accessors\":[{\"bufferView\":0,\"componentType\":5126,"
                                  "\"count\":4,\"type\":\"VEC3\"}],\"meshes\":[{\"primitives\":[{\"attributes\":{\"POSITION\":0},\"mode\":5}]}]}";
        const ImportedModel st = read_model(strip);
        CHECK(st.mesh.faces.size() == 2 && st.mesh.faces[1].a == 2 && st.mesh.faces[1].b == 1 && st.mesh.faces[1].c == 3);
    }
    // Characters of given spheres (sieve-character-v3) and the animation line (IDEAS 19.170).
    {
        // The arm: a body, an elbow 1 m up, a hand 1 m above it.
        std::vector<Sphere> sp(3);
        sp[0] = {-1, 0, 0, 0, 1365, Sphere::Body};
        sp[1] = {0, 0, 4096, 0, 1024, Sphere::Joint};
        sp[2] = {1, 0, 8192, 0, 1024, Sphere::Joint};
        const Character arm = given_character(sp);
        CHECK(arm.given && arm.text() == "sieve-character-v3 spheres=3\ns 0 -1 0 0 0 1365 body\ns 1 0 0 4096 0 1024 joint\ns 2 1 0 8192 0 1024 joint\n");
        CHECK(read_character(arm.text()).id() == arm.id() && read_character(arm.one_line()).id() == arm.id());
        // Every version reads back from its one-line form, the same character.
        Character v2 = unfold_character(CharacterSettings{});
        v2.hold(1, held_model_from(7));
        CHECK(read_character(v2.one_line()).id() == v2.id() && read_character(unfold_character(CharacterSettings{}).one_line()).id() == unfold_character(CharacterSettings{}).id());
        // Refused: a parent after its child, a second body, a radius of 0, text not canonical.
        auto bad = sp;
        bad[1].parent = 2;
        CHECK(throws([&] { (void)given_character(bad); }));
        bad = sp;
        bad[2].parent = -1;
        CHECK(throws([&] { (void)given_character(bad); }));
        bad = sp;
        bad[2].r = 0;
        CHECK(throws([&] { (void)given_character(bad); }));
        CHECK(throws([] { (void)read_character("sieve-character-v3 spheres=1\ns 0 -1 00 0 0 5 body\n"); }));
        CHECK(throws([] { (void)read_character("sieve-character-v3 spheres=2\ns 0 -1 0 0 0 5 body\n"); }));

        // An animation of the arm: the elbow turns a quarter about z over 30 frames.
        Animation an;
        an.bones = 3;
        an.frames = 31;
        for (uint32_t f = 0; f <= 30; ++f)
        {
            const double h = 3.14159265358979323846 / 4 * f / 30; // half the angle
            an.turns.push_back({});
            an.turns.push_back(turn_of_quaternion(0, 0, std::sin(h), std::cos(h)));
            an.turns.push_back(turn_of_quaternion(0, 0, std::sin(h), std::cos(h)));
        }
        CHECK(an.at(30, 1) == (BoneTurn{128, 128, 192}) && an.at(0, 1) == BoneTurn{});
        CHECK(read_animation(an.text()).text() == an.text() && an.address().size() == size_t(31) * 3 * 6);
        CHECK(an.address().substr(0, 18) == "808080808080808080");
        // At frame 30 the hand is a metre to the left of the elbow; at frame 0 it is at rest.
        const auto p30 = pose(arm, an, 30), p0 = pose(arm, an, 0);
        CHECK(std::abs(p30[2][0] + 4096) < 1 && std::abs(p30[2][1] - 4096) < 1 && std::abs(p30[1][1] - 4096) < 1e-6);
        CHECK(std::abs(p0[2][1] - 8192) < 1e-6 && std::abs(p0[2][0]) < 1e-6);
        // A character with more spheres than bones plays it too (the extra turn as their parent).
        CHECK(pose(unfold_character(CharacterSettings{}), an, 30).size() == unfold_character(CharacterSettings{}).spheres.size());
        CHECK(throws([] { (void)read_animation("sieve-animation-v1 bones=1 frames=1 rate=25\nf 0 808080\n"); }));
        CHECK(throws([] { (void)read_animation("sieve-animation-v1 bones=1 frames=2 rate=30\nf 0 808080\n"); }));
        CHECK(throws([] { (void)read_animation("sieve-animation-v1 bones=1 frames=1 rate=30\nf 0 80808G\n"); }));
        // sieve-animation-v2 (glTF translation channels): the same turns and a move a bone; the body
        // walks a metre forward over the 30 frames, the hand slides half a metre up. Read back the
        // same; with no move anywhere it is written as v1 (so v1's identities hold), and a v2 text
        // with none is refused.
        Animation mv = an;
        CHECK(!mv.moving() && mv.text().rfind("sieve-animation-v1 ", 0) == 0);
        mv.moves.assign(mv.turns.size(), BoneMove{});
        CHECK(mv.text() == an.text() && mv.id() == an.id() && mv.address() == an.address());
        for (uint32_t f = 0; f <= 30; ++f)
        {
            mv.moves[size_t(f) * 3 + 0] = {0, 0, int32_t(4096 * f / 30)};
            mv.moves[size_t(f) * 3 + 2] = {0, int32_t(2048 * f / 30), 0};
        }
        CHECK(mv.moving() && mv.text().rfind("sieve-animation-v2 bones=3 frames=31 rate=30\nf 0 808080800000800000800000 ", 0) == 0);
        CHECK(read_animation(mv.text()).text() == mv.text() && mv.address().size() == size_t(31) * 3 * 24);
        const auto m30 = pose(arm, mv, 30);
        CHECK(std::abs(m30[0][2] - 4096) < 1e-6 && std::abs(m30[1][2] - 4096) < 1e-6 && std::abs(m30[2][0] + 4096) < 1 && std::abs(m30[2][1] - 6144) < 1);
        Animation least = mv;
        least.moves[0] = {kAnimationMoveLeast, kAnimationMoveMost, -1};
        CHECK(read_animation(least.text()).move(0, 0) == least.moves[0]);
        CHECK(throws([] { (void)read_animation("sieve-animation-v2 bones=1 frames=1 rate=30\nf 0 808080800000800000800000\n"); }));
        CHECK(throws([] { (void)read_animation("sieve-animation-v2 bones=1 frames=1 rate=30\nf 0 808080\n"); }));
        CHECK(read_animation("sieve-animation-v2 bones=1 frames=1 rate=30\nf 0 808080800001800000800000\n").move(0, 0) == (BoneMove{1, 0, 0}));
        // A held model turns with its sphere: the elbow's turn at frame 30, a quarter about z, takes
        // a point at +x to +y.
        const auto tt = pose_turns(arm, an, 30);
        const float px[3] = {1, 0, 0};
        const auto q = turned(&tt[1], px);
        CHECK(tt.size() == 3 && std::abs(q[0]) < 1e-3 && std::abs(q[1] - 1) < 1e-3 && turned(nullptr, px)[0] == 1);

        // The same arm from a glTF: its skin's joints as the skeleton, its animation sampled.
        std::string bin;
        auto putf = [&](float f) { char b[4]; std::memcpy(b, &f, 4); bin.append(b, 4); };
        for (float f : {0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.70710678f, 0.70710678f}) putf(f);
        auto b64 = [](const std::string& in) {
            static const char* t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string o;
            for (size_t i = 0; i < in.size(); i += 3)
            {
                uint32_t n = uint32_t(uint8_t(in[i])) << 16;
                if (i + 1 < in.size()) n |= uint32_t(uint8_t(in[i + 1])) << 8;
                if (i + 2 < in.size()) n |= uint32_t(uint8_t(in[i + 2]));
                o += t[n >> 18 & 63];
                o += t[n >> 12 & 63];
                o += i + 1 < in.size() ? t[n >> 6 & 63] : '=';
                o += i + 2 < in.size() ? t[n & 63] : '=';
            }
            return o;
        };
        const std::string doc = "{\"asset\":{\"version\":\"2.0\"},\"buffers\":[{\"byteLength\":40,\"uri\":\"data:application/octet-stream;base64," + b64(bin) +
                                "\"}],\"bufferViews\":[{\"buffer\":0,\"byteLength\":8},{\"buffer\":0,\"byteOffset\":8,\"byteLength\":32}],"
                                "\"accessors\":[{\"bufferView\":0,\"componentType\":5126,\"count\":2,\"type\":\"SCALAR\"},"
                                "{\"bufferView\":1,\"componentType\":5126,\"count\":2,\"type\":\"VEC4\"}],"
                                "\"nodes\":[{\"name\":\"root\",\"children\":[1]},{\"name\":\"elbow\",\"translation\":[0,1,0],\"children\":[2]},"
                                "{\"name\":\"hand\",\"translation\":[0,1,0]}],\"skins\":[{\"joints\":[0,1,2]}],"
                                "\"animations\":[{\"name\":\"wave\",\"samplers\":[{\"input\":0,\"output\":1}],"
                                "\"channels\":[{\"sampler\":0,\"target\":{\"node\":1,\"path\":\"rotation\"}}]}],\"scenes\":[{\"nodes\":[0]}]}";
        const ImportedModel m = read_model(doc);
        CHECK(m.skeleton.size() == 3 && m.mesh.verts.empty() && m.sampled.size() == 1 && m.sampled[0].frames == 31);
        CHECK(skeleton_character(m).text() == arm.text());
        const Animation ia = imported_animation(m, 0);
        CHECK(ia.at(30, 1) == (BoneTurn{128, 128, 192}) && ia.at(30, 2) == (BoneTurn{128, 128, 192}) && ia.at(0, 1) == BoneTurn{});
        CHECK(ia.at(15, 1) == an.at(15, 1)); // halfway, slerped as made by hand
        CHECK(throws([&] { (void)imported_animation(m, 1); }));
        CHECK(throws([&] { (void)import_model_for_hold(doc); })); // no mesh to hold
    }

    // The catalogue of dimensions (IDEAS 19.162): ids unique, every link names a dimension that exists.
    {
        std::set<std::string> ids;
        for (const auto& d : dimensions()) CHECK(ids.insert(std::string(d.id)).second);
        for (const auto& d : dimensions())
        {
            std::istringstream links{std::string(d.links)};
            std::string l;
            while (links >> l)
            {
                if (!dimension_by_id(l)) std::cerr << "dimension " << d.id << " links to " << l << ", which is not there\n";
                CHECK(dimension_by_id(l) != nullptr);
            }
        }
        CHECK(dimension_by_id("dna") && dimension_by_id("research") && !dimension_by_id("nowhere"));
        // The wiki principle (IDEAS 19.165), checked: every dimension is linked from another, and
        // following links from any of them reaches the pages line.
        std::map<std::string, std::vector<std::string>> out;
        std::set<std::string> linked;
        for (const auto& d : dimensions())
        {
            std::istringstream links{std::string(d.links)};
            std::string l;
            while (links >> l) out[std::string(d.id)].push_back(l), linked.insert(l);
        }
        for (const auto& d : dimensions())
        {
            if (!linked.count(std::string(d.id))) std::cerr << "dimension " << d.id << " is linked from none\n";
            CHECK(linked.count(std::string(d.id)) == 1);
            std::set<std::string> seen{std::string(d.id)};
            std::vector<std::string> todo{std::string(d.id)};
            while (!todo.empty())
            {
                const std::string x = todo.back();
                todo.pop_back();
                for (const auto& y : out[x])
                    if (seen.insert(y).second) todo.push_back(y);
            }
            if (!seen.count("pages")) std::cerr << "dimension " << d.id << " does not reach the pages line\n";
            CHECK(seen.count("pages") == 1);
        }
    }
    CharacterSettings p;
    p.seed = 1;
    CHECK(unfold_character(p).id() == "607ec057ef34f8c040ab5f72ae2f0401f05f0675e8d13da656973d0ff4b0f352");
    p.seed = 2;
    p.limbs = 6;
    p.joints = 4;
    p.branches = 2;
    p.symmetry = Symmetry::Radial;
    CHECK(unfold_character(p).id() == "42f354a2d11c87a26cc0c2d877c9ba21efed1ba648abff66e39379334ec48293");
}

// The infinite RAID, stage 1 (IDEAS 19.176): saves on several folders, every piece checked by its
// name; damage found by verify and mended by repair; a chunk with no good copy said to be lost.
void test_raid()
{
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / ("sieve-raid-test-" + std::to_string(std::random_device{}()));
    std::error_code ec;
    fs::remove_all(root, ec);
    const std::vector<fs::path> drives = {root / "a", root / "b", root / "c"};
    std::string bytes;
    std::mt19937 rng(11);
    for (size_t i = 0; i < kRaidChunk * 2 + 12345; ++i) bytes.push_back(char(rng() & 0xFF));
    std::string v1, v2;
    {
        Raid raid(drives);
        v1 = raid.put("game.sav", bytes);
        CHECK(raid.put("game.sav", bytes) == v1); // the same bytes again: nothing new
        CHECK(raid.get(v1) == bytes);
        bytes[5] ^= 1;
        v2 = raid.put("game.sav", bytes);
        CHECK(v2 != v1 && raid.latest("game.sav") == v2);
        const auto all = raid.list();
        const size_t pieces = raid_cut(bytes).size();
        CHECK(all.size() == 2 && all[0].id() == v1 && all[1].previous == v1 && all[1].chunks.size() == pieces);
        CHECK(raid.verify().whole() && raid.verify().chunks == pieces + 1); // the first chunk changed, the rest shared
        CHECK(throws([&] { (void)raid.put("two\nlines", bytes); }));
    }
    {
        // Opened again: the folders keep their ids, so the chunks are where they were.
        Raid raid(drives);
        const auto homes = raid.homes(read_raid_manifest(raid.list()[1].text()).chunks[0].first);
        CHECK(homes.size() == 3);
        // Damage one copy of a chunk and delete one copy of another; take a manifest out of one folder.
        const auto m = raid.list()[1];
        const std::string c0 = m.chunks[0].first, c1 = m.chunks[1].first;
        const auto h0 = raid.homes(c0), h1 = raid.homes(c1);
        auto path = [&](size_t d, const std::string& h) { return drives[d] / "chunks" / h.substr(0, 2) / h; };
        std::ofstream(path(h0[0], c0), std::ios::binary) << "damaged";
        fs::remove(path(h1[1], c1), ec);
        fs::remove(drives[2] / "manifests" / (v1 + ".manifest"), ec);
        const RaidReport r = raid.verify();
        CHECK(!r.whole() && r.damaged == 1 && r.missing == 2 && r.lost == 0);
        CHECK(raid.get(v2) == bytes); // still readable: a good copy of everything remains
        const RaidReport fixed = raid.repair();
        CHECK(fixed.repaired >= 3 && raid.verify().whole());
        // Both copies of a chunk gone: lost, said so, and get refuses rather than give wrong bytes.
        fs::remove(path(h0[0], c0), ec);
        fs::remove(path(h0[1], c0), ec);
        const RaidReport gone = raid.verify();
        CHECK(gone.lost == 1 && !gone.whole());
        CHECK(throws([&] { (void)raid.get(v2); }));
    }
    CHECK(throws([] { (void)read_raid_manifest("sieve-raid-manifest-v1\nname x\nsize 2\nprevious none\nchunk " + std::string(64, 'a') + " 1\n"); }));
    // Stage 2: cuts made by the content. Every piece between 16 KiB and 256 KiB (the last may be
    // shorter), and ten bytes inserted near the start move only the pieces up to the first cut that
    // the content makes again, so a new version shares the rest.
    {
        std::string data;
        std::mt19937 g(21);
        for (int i = 0; i < 3 << 20; ++i) data.push_back(char(g() & 0xFF));
        const auto cuts = raid_cut(data);
        size_t total = 0;
        for (size_t k = 0; k < cuts.size(); ++k)
        {
            CHECK(cuts[k].first == total && cuts[k].second <= kRaidCutMax && (k + 1 == cuts.size() || cuts[k].second >= kRaidCutMin));
            total += cuts[k].second;
        }
        CHECK(total == data.size() && cuts.size() > 12 && cuts.size() < 192);
        std::string edited = data;
        edited.insert(100, "0123456789");
        std::set<std::string> before, after;
        for (const auto& [at, n] : cuts) before.insert(Sha256::hex(Sha256::hash(data.substr(at, n))));
        size_t shared = 0;
        const auto cuts2 = raid_cut(edited);
        for (const auto& [at, n] : cuts2) shared += before.count(Sha256::hex(Sha256::hash(edited.substr(at, n))));
        CHECK(shared + 2 >= cuts2.size()); // at most two pieces new
        CHECK(raid_cut(data) == cuts);      // the same bytes, the same cuts
        CHECK(raid_cut(std::string()).empty());
    }
    CHECK(throws([] { (void)Raid(std::vector<std::filesystem::path>{}); }));
    fs::remove_all(root, ec);
}

// Stage 3 of the RAID (IDEAS 19.176): sealing checked against the RFCs' vectors; a held folder's
// limits; the protocol answered as written; a save shared to peers and fetched back, a changed
// version sending only what changed, and anything tampered with refused.
// The RAID's pieces (IDEAS 19.210, SPECIFICATIONS 12.0s): any K of K + M make the whole again, a damaged
// piece is found, and placement keeps one piece of a whole a machine, within each machine's limits.
void test_raid_pieces()
{
    std::mt19937 rng(210);
    auto random_bytes = [&](size_t n) {
        std::string b(n, '\0');
        for (char& c : b) c = char(rng() & 0xFF);
        return b;
    };
    for (const auto& [k, m, n] : std::vector<std::tuple<uint32_t, uint32_t, size_t>>{{1, 2, 1}, {1, 2, 5000}, {4, 2, 100003}, {3, 3, 7}, {5, 0, 64}, {2, 1, 2}})
    {
        const std::string whole = random_bytes(n);
        const auto pieces = raid_pieces(whole, k, m);
        CHECK(pieces.size() == k + m);
        for (const auto& p : pieces)
        {
            CHECK(p.data.size() == (n + k - 1) / k);
            const RaidPiece back = read_raid_piece(p.bytes());
            CHECK(back.bytes() == p.bytes());
            CHECK(back.name() == p.name());
        }
        // every choice of K pieces gives the whole back
        for (uint32_t mask = 0; mask < (1u << (k + m)); ++mask)
        {
            if (uint32_t(std::bitset<32>(mask).count()) != k) continue;
            std::vector<RaidPiece> some;
            for (uint32_t i = 0; i < k + m; ++i)
                if (mask >> i & 1) some.push_back(pieces[i]);
            CHECK(raid_join(some) == whole);
        }
        if (m > 0)
        {
            // one too few, and a damaged one, are refused
            std::vector<RaidPiece> few(pieces.begin(), pieces.begin() + (k - 1));
            CHECK(throws([&] { (void)raid_join(few); }));
            std::vector<RaidPiece> bad(pieces.begin() + 1, pieces.begin() + 1 + k);
            bad[0].data[0] = char(bad[0].data[0] ^ 1);
            CHECK(throws([&] { (void)raid_join(bad); }));
        }
    }
    // The format pinned: this whole's pieces are these, so a change to the code or the head is caught.
    {
        const auto p = raid_pieces("sieve-raid-piece-v1 pinned", 4, 2);
        // (the names an independent implementation gives, GF(2^8) multiplied bit by bit, not by tables)
        CHECK(p[0].bytes().substr(0, 102) == "sieve-raid-piece-v1\nof 13ae1ecf1e257fabe0209ee07dc80dca26a8c483c69da7747184ebe28e50cddb 26\ncode 4 2 0\n");
        CHECK(p[0].name() == "376e3605c67b829eb313fad410819b281b164170cb33228172af5a0952856b42");
        CHECK(p[4].name() == "7c0f3c7dd2e27235d5b13f43d7e2fd0db4ce25a98dd013d1ba4aa8816385138a");
        CHECK(p[5].name() == "2d57a9f063c15b46c2d277987711a4941663ab28c1a73d21e326c0f80ed7c0aa");
    }
    // Heads not as written are refused.
    const std::string good = raid_pieces("abcdefg", 2, 1)[2].bytes();
    CHECK(throws([&] { (void)read_raid_piece(good.substr(0, good.size() - 1)); }));
    CHECK(throws([&] { (void)read_raid_piece(good + "x"); }));
    for (const auto& [from, to] : std::vector<std::pair<std::string, std::string>>{{"code 2 1 2", "code 2 1 3"}, {"code 2 1 2", "code 02 1 2"}, {"code 2 1 2", "code 0 1 2"}, {" 7\n", " 07\n"}, {"sieve-raid-piece-v1", "sieve-raid-piece-v2"}})
    {
        std::string b = good;
        b.replace(b.find(from), from.size(), to);
        CHECK(throws([&] { (void)read_raid_piece(b); }));
    }
    CHECK(throws([&] { (void)raid_pieces("", 1, 1); }));
    CHECK(throws([&] { (void)raid_pieces("x", 0, 1); }));
    CHECK(throws([&] { (void)raid_pieces("x", 200, 57); }));
    CHECK(raid_pieces("x", 200, 56).size() == 256);
    CHECK(raid_piece_shape(1) == std::make_pair(1u, 2u));
    CHECK(raid_piece_shape(256 << 10) == std::make_pair(1u, 2u));
    CHECK(raid_piece_shape((256 << 10) + 1) == std::make_pair(4u, 2u));

    // The tally, and placement.
    std::string text = "sieve-raid-tally-v1\n";
    for (int i = 0; i < 8; ++i) text += "keeper 198.51.100." + std::to_string(10 + i) + ":47100 1000000 300000 " + std::to_string(i * 1000) + "\n";
    RaidTally tally = read_raid_tally(text);
    CHECK(tally.text() == text);
    CHECK(throws([&] { (void)read_raid_tally("sieve-raid-tally-v1\nkeeper b:1 10 10 0\nkeeper a:1 10 10 0\n"); })); // out of order
    CHECK(throws([&] { (void)read_raid_tally("sieve-raid-tally-v1\nkeeper a:1 10 10 11\n"); }));                  // more than it gives
    CHECK(throws([&] { (void)read_raid_tally("sieve-raid-tally-v1\nkeeper a:1 10 10 0"); }));                      // last line not ended
    const std::string sha = Sha256::hex_hash("a whole");
    RaidTally t1 = tally, t2 = tally;
    const auto h1 = raid_place(sha, 6, 100000, t1), h2 = raid_place(sha, 6, 100000, t2);
    CHECK(h1 && h2 && *h1 == *h2); // the same rule, the same homes
    CHECK(std::set<size_t>(h1->begin(), h1->end()).size() == 6); // one piece a machine
    uint64_t before = 0, after = 0;
    for (const auto& k : tally.keepers) before += k.used;
    for (const auto& k : t1.keepers) after += k.used;
    CHECK(after == before + 600000);
    std::string why;
    RaidTally t3 = tally;
    CHECK(!raid_place(sha, 6, 300001, t3, &why)); // larger than any machine keeps
    CHECK(why.find("piece 0 of 6") == 0);
    CHECK(t3.text() == tally.text()); // nothing counted for a whole refused
    CHECK(!raid_place(sha, 9, 1000, t3)); // more pieces than machines
    // A machine leaving: its piece moves first, and every piece ends on a machine still there, one each.
    const std::string leaving = t1.keepers[(*h1)[2]].address;
    const auto moves = raid_leave(sha, 6, 100000, t1, *h1, leaving);
    CHECK(moves && !moves->empty() && (*moves)[0].from == leaving && (*moves)[0].index == 2);
    std::vector<std::string> now;
    for (size_t i = 0; i < 6; ++i) now.push_back(t1.keepers[(*h1)[i]].address);
    for (const auto& mv : *moves)
    {
        CHECK(now[mv.index] == mv.from);
        now[mv.index] = mv.to;
    }
    CHECK(std::set<std::string>(now.begin(), now.end()).size() == 6);
    CHECK(std::find(now.begin(), now.end(), leaving) == now.end());
}

extern std::string g_data; // (the data folder: defined below, set from the tests folder)

// The fractal dimension's rulesets (IDEAS 19.191, 19.235): read as written, grown, drawn, and the
// shelves' chart read whole.
// The fractals' structural filters counted (list F, fractal_count.hpp): against the filters themselves on every
// ruleset of the smallest lengths, against the space's own counts, against the prototype's figures (counted
// there by the reach and checked against every body of small alphabets), and against a sample of the space.
void test_fractal_count()
{
    using F = FractalStructureFilters;
    auto set_of = [](const std::string& bits) {
        F f;
        f.draws = bits[0] == '1', f.grows = bits[1] == '1', f.no_idle = bits[2] == '1', f.canonical = bits[3] == '1';
        return f;
    };
    // Unfiltered, the counts are the space's.
    {
        const FractalSpace space(256);
        CHECK(space.count_v1_passing(F{}) == space.count_v1());
        CHECK(space.count_v2_passing(F{}) == space.count_v2());
        CHECK(space.count_v3_passing(F{}) == space.count_v3());
    }
    // The prototype's figures (L-systems and substitutions, bodies of 40 and 60 characters, every
    // combination of the four: draws, grows, no-idle, canonical).
    static const char* const ref[] = {
        "L 0000 98420592945317914665633198458843339160591940176 23649639520036478924985839625019871447085463645245115667161331110073148528847860141",
        "L 0001 12597993001936297324679977773 67012544609824111221381416120081114611492105639602792",
        "L 0010 18916154112002317707298917228485799575181180304 7359631518329260620521519090966782878169382108241303709139547795660975316532583784",
        "L 0011 12541328518892693954145436824 66896392884355496663074261116116924264030269551322998",
        "L 0100 17521776230820238635261532447405813336495894500 7073931409328999550642893153438903702167223971491317276000230845543902808432872700",
        "L 0101 11804058675908487447826588410 64815726798883776518682988034676884407207578104234552",
        "L 0110 17521776230806050015022315762600461515259674256 7073931409312115506747444363219912517853851257053771982449948339776923309686120936",
        "L 0111 11804058673649526670366580481 64815726796514243085157979580063662217261705667460021",
        "L 1000 36644110481986348841420290333094971231047199816 13299296410578507376740882176234713243376073811122592649594283870635854725449473406",
        "L 1001 12337282580396477933302551776 66548805335381901939837716171079106495540302401443616",
        "L 1010 11233892279698111067113064744694888198987405004 5901618121162990563576455170129376561209553961658860479515268865235868391544874484",
        "L 1011 12281861766452504218387626588 66433483487317579268738990159071067703782587912065460",
        "L 1100 10404154576837655222544780060479508229799451650 5672256594344082836968378902422192911690525967860354996884548411781522974217880650",
        "L 1101 11559536622069900338006134736 64367049409511717271883183364639792784005260535046128",
        "L 1110 10404154576832546891867686356512036432725268556 5672256594333945773348420596566615285785361768062955499654261077574070877493662836",
        "L 1111 11559536619898844893495247376 64367049407165464153874044219652781751507367787866514",
        "S 0000 698355023751216440472543347174482749947904 17114637772298140040263309203570095381928754149565816588115077472723140608",
        "S 0001 749334085145788427057 66898438743087941752277881879811212491809556",
        "S 0010 197495214712677371262617476450941139978788 7434594492267717208635933262624615283261865657474346191208171599167811708",
        "S 0011 745141449244695046808 66767882186713769575102709472921194650899441",
        "S 0100 183593475370110724037625369172713853765248 7165917637162675961354308730531548986801159910311005616167309393044883328",
        "S 0101 704787385961390728422 64780980988219736103322961885994033253762375",
        "S 0110 183593475366397935247852267369321824523128 7165917636788885895679946493500097071445099636156940813561387824534643248",
        "S 0111 704787381931694051557 64780980974251561525833480800665767393959579",
        "S 1000 669256897759859471131383150455799816252960 16725668731971210311338784250414566923128024639663631052719215587310698640",
        "S 1001 749334085145788427033 66898438743087941752277881879811212491809512",
        "S 1010 197495214712677371262617476450941139977960 7434594492267717208635933262624615283261865657474346191208171599167810160",
        "S 1011 745141449244695046785 66767882186713769575102709472921194650899398",
        "S 1100 183593475370110715851573941555446141872420 7165917637162675851927310963975634696447195702186271445874865338714945940",
        "S 1101 704787385961390728399 64780980988219736103322961885994033253762332",
        "S 1110 183593475366397935247852267369321824522300 7165917636788885895679946493500097071445099636156940813561387824534641700",
        "S 1111 704787381931694051534 64780980974251561525833480800665767393959536",
    };
    for (const char* line : ref)
    {
        std::istringstream in(line);
        std::string fam, bits, a40, a60;
        in >> fam >> bits >> a40 >> a60;
        const auto v = fractal_bodies_passing(fam == "L" ? fractal_lsystem_kinds() : fractal_substitution_kinds(), 60, set_of(bits));
        CHECK(v[40].to_decimal() == a40);
        CHECK(v[60].to_decimal() == a60);
    }
    // v3's bodies (any number of rules a symbol, each with context letters or none), at 30 and 40 characters: the
    // prototype's figures, it checked against every body of small alphabets with those rules.
    static const char* const ref3[] = {
        "V 0000 8084395968535363825926669461760 68318033607948636336183007116356760577445168013312",
        "V 0001 35155121677188756019362 21356461569614859021679598804566744358",
        "V 0010 770349573538586989381231456360 10404951408803978423356831281827206007601944737280",
        "V 0011 34991546653188659727197 21301744765886833684017767636413451124",
        "V 0100 671529401997831392500949647040 9622487037060241467537031287088394018442692583888",
        "V 0101 30745602595288060064692 19840160805516125291629482456024800010",
        "V 0110 671529401997823185643760817104 9622487037059235164760213506702465802522025567564",
        "V 0111 30745602595286679528137 19840160805512205710074996087721464945",
        "V 1000 1541911817430681806799304334768 20427510034345216847581337326035167090256801713356",
        "V 1001 28204053770264710267012 19090883176261086848652305843868758606",
        "V 1010 266035003445282784742940781860 5231100915478329320783917225517843333854498625880",
        "V 1011 28074871090389931756088 19042291355564297445282872285257236560",
        "V 1100 231843205004554067162635602640 4837075755324278955606995373456493590861242037488",
        "V 1101 24664075924303067815240 17734468596376414930394807074074812222",
        "V 1110 231843205004552894594621054804 4837075755323980781003404522319191537542562977914",
        "V 1111 24664075924302129535052 17734468596373100976304204255571942226",
    };
    for (const char* line : ref3)
    {
        std::istringstream in(line);
        std::string fam, bits, a30, a40;
        in >> fam >> bits >> a30 >> a40;
        const auto v = fractal_bodies_passing(fractal_v3_kinds(), 40, set_of(bits));
        CHECK(v[30].to_decimal() == a30);
        CHECK(v[40].to_decimal() == a40);
    }
    // Every ruleset of the smallest lengths, judged by the filters: the L-systems of 44 characters (an angle
    // of one digit, an axiom and a rule of one symbol each) and the substitutions of 47 and 48.
    FilterLine fl;
    fl.kind = "fractals", fl.symbols_id = "fractals/96", fl.base = 128, fl.length = 96, fl.up_to_length = true;
    static const TestResources none(nullptr, nullptr);
    const char* names[] = {"fractal-draws-v1", "fractal-grows-v1", "fractal-no-idle-v1", "fractal-canonical-v1"};
    std::vector<std::unique_ptr<Filter>> judge;
    for (const char* n : names) judge.push_back(find_filter(n)->make(fl, {}, none));
    // passed[combination]: how many pass every filter of it (combination's bit k: filter k ticked)
    auto tally = [&](const std::string& text, std::array<uint64_t, 16>& passed) {
        const std::vector<uint32_t> u(text.begin(), text.end());
        unsigned ok = 0;
        for (unsigned k = 0; k < 4; ++k)
            if (judge[k]->passes(u)) ok |= 1u << k;
        for (unsigned c = 0; c < 16; ++c)
            if ((c & ok) == c) ++passed[c];
    };
    auto bits_of = [](unsigned c) {
        std::string b;
        for (unsigned k = 0; k < 4; ++k) b += (c >> k) & 1 ? '1' : '0';
        return b;
    };
    {
        const std::string syms = "+-ABCDEFGHIJKLMNOPQRSTUVWXYZ[]abcdefghijklmnopqrstuvwxyz|";
        const std::string rulers = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::array<uint64_t, 16> passed{};
        for (const char x : syms)
            for (const char y : rulers)
                for (const char z : syms)
                    tally(std::string("sieve-fractal-v1\nlsystem 7\naxiom ") + x + "\nrule " + y + " " + z + "\n", passed);
        for (unsigned c = 0; c < 16; ++c) CHECK(fractal_bodies_passing(fractal_lsystem_kinds(), 17, set_of(bits_of(c)))[17] == BigUint(passed[c]));
    }
    {
        const std::string syms = "0123456789abcdefghijklmnopqrstuvwxyz";
        std::array<uint64_t, 16> p17{}, p18{};
        for (const char x : syms)
            for (const char y : syms)
                for (const char z : syms)
                {
                    tally(std::string("sieve-fractal-v1\nsubstitution\naxiom ") + x + "\nrule " + y + " " + z + "\n", p17);
                    for (const char w : syms)
                    {
                        tally(std::string("sieve-fractal-v1\nsubstitution\naxiom ") + x + w + "\nrule " + y + " " + z + "\n", p18);
                        tally(std::string("sieve-fractal-v1\nsubstitution\naxiom ") + x + "\nrule " + y + " " + z + w + "\n", p18);
                    }
                }
        for (unsigned c = 0; c < 16; ++c)
        {
            const auto v = fractal_bodies_passing(fractal_substitution_kinds(), 18, set_of(bits_of(c)));
            CHECK(v[17] == BigUint(p17[c]));
            CHECK(v[18] == BigUint(p18[c]));
        }
    }
    // Every v3 body of 17 characters (an axiom and a rule of one symbol each, over its 74 symbols), judged.
    {
        std::string syms;
        for (int c = 33; c < 127; ++c)
            if (std::isalnum(c) || std::string("+-[]|!&^/\"'\\").find(char(c)) != std::string::npos) syms += char(c);
        CHECK(syms.size() == 74);
        std::array<uint64_t, 16> passed{};
        for (const char x : syms)
            for (const char y : syms)
                if (std::isalpha(static_cast<unsigned char>(y)))
                    for (const char z : syms)
                        tally(std::string("sieve-fractal-v3\nlsystem 7\naxiom ") + x + "\nrule " + y + " " + z + "\n", passed);
        for (unsigned c = 0; c < 16; ++c) CHECK(fractal_bodies_passing(fractal_v3_kinds(), 17, set_of(bits_of(c)))[17] == BigUint(passed[c]));
    }
    // A sample of v3 to 90 characters (several rules for a symbol, contexts, header lines): as v1's below.
    {
        const FractalSpace space(90);
        std::mt19937_64 rng(37);
        std::array<uint64_t, 16> passed{};
        const unsigned n = 3000;
        BigUint base = space.count_v1();
        base += space.count_v2();
        for (unsigned k = 0; k < n; ++k)
        {
            BigUint r;
            for (int w = 0; w < 6; ++w)
            {
                r <<= 64;
                r += BigUint(rng());
            }
            BigUint i = BigUint::mod(r, space.count_v3());
            i += base;
            tally(space.at(i), passed);
        }
        for (unsigned c : {1u, 2u, 4u, 8u, 7u})
        {
            const double p = std::pow(10.0, space.count_v3_passing(set_of(bits_of(c))).log10_approx() - space.count_v3().log10_approx());
            const double sd = std::sqrt(p * (1 - p) / n), got = double(passed[c]) / n;
            CHECK(std::abs(got - p) <= 4 * sd + 1.0 / n);
        }
    }
    // A sample of v1 to 80 characters (several rules, words of several symbols): each filter's share among
    // 3,000 rulesets drawn at random is within four standard deviations of its count's.
    {
        const FractalSpace space(80);
        std::mt19937_64 rng(31);
        std::array<uint64_t, 16> passed{};
        const unsigned n = 3000;
        for (unsigned k = 0; k < n; ++k)
        {
            BigUint r;
            for (int w = 0; w < 6; ++w)
            {
                r <<= 64;
                r += BigUint(rng());
            }
            tally(space.at(BigUint::mod(r, space.count_v1())), passed);
        }
        for (unsigned c : {1u, 2u, 4u, 8u, 7u})
        {
            const double p = std::pow(10.0, space.count_v1_passing(set_of(bits_of(c))).log10_approx() - space.count_v1().log10_approx());
            const double sd = std::sqrt(p * (1 - p) / n), got = double(passed[c]) / n;
            CHECK(std::abs(got - p) <= 4 * sd + 1.0 / n);
        }
    }
}

void test_fractals()
{
    const std::string fib = "sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 01\nrule 1 0\n";
    const auto f = read_fractal(fib);
    CHECK(f && f->text() == fib);
    // the Fibonacci word: its lengths are the Fibonacci numbers, its ones at 1/phi^2 of its places
    CHECK(*f->grow(0) == "0" && *f->grow(1) == "01" && *f->grow(5) == "0100101001001");
    CHECK(f->grow(20)->size() == 17711);
    {
        const std::string w = *f->grow(25);
        const double ones = double(std::count(w.begin(), w.end(), '1')) / double(w.size());
        CHECK(std::fabs(ones - 0.3819660112501051) < 1e-6);
    }
    CHECK(f->deepest() == 29); // 1,346,269 symbols at 29; 2,178,309 at 30, past the limit of 2,097,152
    // The dragon curve: 2^n lines at depth n, each a step long.
    const auto d = read_fractal("sieve-fractal-v1\nlsystem 90\naxiom FX\nrule X X+YF+\nrule Y -FX-Y\n");
    CHECK(d && d->angle_tenths == 900 && d->version == 1);
    CHECK(fractal_lines(*d, *d->grow(10)).size() == 1024);
    for (const auto& l : fractal_lines(*d, *d->grow(6))) CHECK(std::fabs(std::hypot(l[2] - l[0], l[3] - l[1]) - 1) < 1e-9);
    const auto img = draw_fractal(*d, 8, 64, 48);
    CHECK(img && img->width == 64 && img->height == 48 && img->rgba.size() == 64u * 48 * 4);
    CHECK(!draw_fractal(*d, d->deepest() + 1, 64, 64)); // past its limit: nothing, not a word of millions
    // Not as written: each refused with a reason.
    for (const std::string& bad : {std::string("sieve-fractal-v1\nsubstitution\naxiom 0\n"), std::string("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 1 0\nrule 0 01\n"),
                                  std::string("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 01\nrule 0 1\n"), std::string("sieve-fractal-v1\nlsystem 0\naxiom F\nrule F FF\n"),
                                  std::string("sieve-fractal-v1\nlsystem 090\naxiom F\nrule F FF\n"), std::string("sieve-fractal-v1\nlsystem 90\naxiom F\nrule + FF\n"),
                                  std::string("sieve-fractal-v1\nsubstitution\naxiom A\nrule A 0\n"), std::string("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 01"),
                                  std::string("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 ") + std::string(65, '0') + "\n"})
    {
        std::string why;
        CHECK(!read_fractal(bad, &why) && !why.empty());
    }
    // sieve-fractal-v2 (list X2): an angle with a tenth, a scale and its " and ', each written one way.
    {
        const std::string plant = "sieve-fractal-v2\nlsystem 22.5\naxiom F\nrule F FF-[-F+F+F]+[+F-F-F]\n";
        const auto p = read_fractal(plant);
        CHECK(p && p->version == 2 && p->angle_tenths == 225 && p->scale == 0 && p->text() == plant);
        const std::string scaled = "sieve-fractal-v2\nlsystem 90 scale 0.5\naxiom F\nrule F F[\"F]'F\n";
        const auto s = read_fractal(scaled);
        CHECK(s && s->scale == 500 && s->angle_tenths == 900 && s->text() == scaled);
        // (a step, then half a step kept within [ ], then the step again, then twice it: [ keeps the step)
        const auto ls = fractal_lines(*s, *s->grow(1));
        CHECK(ls.size() == 3);
        if (ls.size() == 3)
        {
            const double l0 = std::hypot(ls[0][2] - ls[0][0], ls[0][3] - ls[0][1]), l1 = std::hypot(ls[1][2] - ls[1][0], ls[1][3] - ls[1][1]),
                         l2 = std::hypot(ls[2][2] - ls[2][0], ls[2][3] - ls[2][1]);
            CHECK(std::fabs(l0 - 1) < 1e-9 && std::fabs(l1 - 0.5) < 1e-9 && std::fabs(l2 - 2) < 1e-9);
        }
        CHECK(fractal_angle_text(1) == "0.1" && fractal_angle_text(3599) == "359.9" && fractal_angle_text(900) == "90");
        CHECK(fractal_scale_text(707) == "0.707" && fractal_scale_text(500) == "0.5" && fractal_scale_text(10) == "0.01");
        for (const std::string& bad : {std::string("sieve-fractal-v2\nlsystem 90\naxiom F\nrule F FF\n"), std::string("sieve-fractal-v2\nlsystem 22.0\naxiom F\nrule F FF\n"),
                                      std::string("sieve-fractal-v2\nlsystem 360.5\naxiom F\nrule F FF\n"), std::string("sieve-fractal-v2\nlsystem 00.5\naxiom F\nrule F FF\n"),
                                      std::string("sieve-fractal-v2\nlsystem 90 scale 0.70\naxiom F\nrule F FF\n"), std::string("sieve-fractal-v2\nlsystem 90 scale 1.5\naxiom F\nrule F FF\n"),
                                      std::string("sieve-fractal-v2\nlsystem 22.5\naxiom F\nrule F F\"F\n"), std::string("sieve-fractal-v1\nlsystem 90\naxiom F\nrule F F\"F\n"),
                                      std::string("sieve-fractal-v2\nsubstitution\naxiom 0\nrule 0 01\n")})
        {
            std::string why;
            CHECK(!read_fractal(bad, &why) && !why.empty());
        }
        // Its box-counting dimension, measured: the Sierpinski triangle near log 3 / log 2, the Koch
        // curve near log 4 / log 3 (its few depths drawn read a little above it), the Hilbert curve
        // filling the square, a straight line 1; nothing for a substitution.
        auto measured = [](const std::string& t) { return fractal_box_dimension(*read_fractal(t)); };
        const auto sier = measured("sieve-fractal-v1\nlsystem 120\naxiom F-G-G\nrule F F-G+F+G-F\nrule G GG\n");
        CHECK(sier && std::fabs(sier->value - std::log(3.0) / std::log(2.0)) < 0.03);
        const auto koch = measured("sieve-fractal-v1\nlsystem 60\naxiom F\nrule F F+F--F+F\n");
        CHECK(koch && std::fabs(koch->value - std::log(4.0) / std::log(3.0)) < 0.05 && koch->grids >= 3);
        const auto hil = measured("sieve-fractal-v1\nlsystem 90\naxiom A\nrule A +BF-AFA-FB+\nrule B -AF+BFB+FA-\n");
        CHECK(hil && std::fabs(hil->value - 2) < 0.05);
        const auto line = measured("sieve-fractal-v1\nlsystem 90\naxiom F\nrule F FF\n");
        CHECK(line && std::fabs(line->value - 1) < 0.02);
        CHECK(!measured(fib) && !measured("sieve-fractal-v1\nlsystem 90\naxiom F\nrule A F\n"));
    }
    // sieve-fractal-v3 (list X3): read and written back as it was, every optional line; refusals; its
    // turtle's new symbols; several rules for a symbol chosen from the seed, the same every time;
    // contexts; and its rulesets in the whole space after v2's.
    {
        const std::string full = "sieve-fractal-v3\nlsystem 360/34\nscale 0.618034\nseed 7\nignore +-F\ndefine 1 turn 42\ndefine 2 turn -84.5\ndefine 3 scale 4.783\n"
                                 "axiom F\nrule a<F>b F12F3\nrule F F!+F--F+F\nrule F F&F^F\\F/F\nrule G G\n";
        const auto f = read_fractal(full);
        CHECK(f && f->version == 3 && f->text() == full && f->rules.size() == 4 && f->defines.size() == 3 && f->seed == 7 && f->ignore == "+-F");
        CHECK(f && std::fabs(f->angle_degrees - 360.0 / 34) < 1e-12 && std::fabs(f->scale_ratio - 0.618034) < 1e-12 && f->contexts[0] == std::make_pair('a', 'b'));
        CHECK(f && f->defines[1].value == -84.5 && !f->defines[2].turn && f->angle_written() == "360/34" && f->scale_written() == "0.618034");
        for (const std::string& bad : {std::string("sieve-fractal-v3\nlsystem 360\naxiom F\nrule F F\n"), std::string("sieve-fractal-v3\nlsystem 360/1\naxiom F\nrule F F\n"),
                                      std::string("sieve-fractal-v3\nlsystem 22.50\naxiom F\nrule F F\n"), std::string("sieve-fractal-v3\nlsystem 90\nseed 0\naxiom F\nrule F F\n"),
                                      std::string("sieve-fractal-v3\nlsystem 90\nignore F+\naxiom F\nrule F F\n"),
                                      std::string("sieve-fractal-v3\nlsystem 90\ndefine 2 turn 1\ndefine 1 turn 1\naxiom F\nrule F F\n"),
                                      std::string("sieve-fractal-v3\nlsystem 90\ndefine 1 scale 1\naxiom F\nrule F F\n"),
                                      std::string("sieve-fractal-v3\nlsystem 90\naxiom F\nrule G F\nrule F F\n"), std::string("sieve-fractal-v3\nlsystem 90\naxiom F\nrule F ") + std::string(256, 'F') + "\n",
                                      std::string("sieve-fractal-v3\nlsystem 90\nseed 3\nscale 0.5\naxiom F\nrule F F\n")})
        {
            std::string why;
            CHECK(!read_fractal(bad, &why) && !why.empty());
        }
        auto lines_of = [](const std::string& t, uint32_t depth) { const auto g = read_fractal(t); return fractal_lines(*g, *g->grow(depth)); };
        auto same = [](const std::vector<std::array<double, 4>>& a, const std::vector<std::array<double, 4>>& b) {
            if (a.size() != b.size()) return false;
            for (size_t k = 0; k < a.size(); ++k)
                for (size_t j = 0; j < 4; ++j)
                    if (std::fabs(a[k][j] - b[k][j]) > 1e-9) return false;
            return true;
        };
        // A digit turning 60 draws as + at 60; ! swaps + and - (so the Koch curve swapped is its mirror,
        // y turned over); v3 at 60 draws the Koch curve as v1 does.
        const std::string koch1 = "sieve-fractal-v1\nlsystem 60\naxiom F\nrule F F+F--F+F\n";
        CHECK(same(lines_of("sieve-fractal-v3\nlsystem 60\naxiom F\nrule F F+F--F+F\n", 3), lines_of(koch1, 3)));
        CHECK(same(lines_of("sieve-fractal-v3\nlsystem 7\ndefine 1 turn 60\ndefine 2 turn -60\naxiom F\nrule F F1F22F1F\n", 3), lines_of(koch1, 3)));
        {
            auto mirror = lines_of(koch1, 3);
            for (auto& l : mirror) l[1] = -l[1], l[3] = -l[3];
            CHECK(same(lines_of("sieve-fractal-v3\nlsystem 60\naxiom !F\nrule F F+F--F+F\n", 3), mirror));
        }
        // Out of the plane: & pitches the next step out of it, so it is drawn seen from the side.
        const auto bent = lines_of("sieve-fractal-v3\nlsystem 90\naxiom F&F\nrule A A\n", 0);
        CHECK(bent.size() == 2 && std::fabs(bent[1][2] - bent[1][0]) > 1e-3 && std::fabs(bent[1][3] - bent[1][1]) > 1e-3);
        // Several rules: the same word every time from one seed, another from another, each place one of them.
        const std::string bush = "sieve-fractal-v3\nlsystem 22.5\nseed 1\naxiom F\nrule F F[+F]F\nrule F F[-F]F\nrule F FF\n";
        const auto b1 = read_fractal(bush);
        CHECK(b1 && *b1->grow(5) == *b1->grow(5));
        auto b2 = *b1;
        b2.seed = 2;
        CHECK(*b2.grow(5) != *b1->grow(5));
        CHECK(b1->deepest() >= 8); // (sized by the longest rule: 7 symbols a step at most)
        // A context: B becomes C only where A stands before it (the branch passed over), else stays.
        const auto ctx = read_fractal("sieve-fractal-v3\nlsystem 90\nignore +\naxiom A[X]+B[A]B\nrule A<B C\n");
        CHECK(ctx && *ctx->grow(1) == "A[X]+C[A]B");
        // In the whole space: after every v1 and v2 ruleset, read back from its address.
        const FractalSpace space(96);
        BigUint first3 = space.count();
        first3 -= space.count_v3();
        first3 -= space.count_v4(); // (v4's after v3's, list X4)
        first3 -= space.count_v5(); // (and v5's, list Y2)
        first3 -= space.count_v6(); // (and v6's, list X5)
        // (the rules symbol by symbol, an earlier symbol's shorter first: so the first has its rule for z)
        CHECK(space.at(first3) == "sieve-fractal-v3\nlsystem 1\naxiom !\nrule z !\n");
        const FractalSpace whole(kFractalSpaceLongestDefault);
        BigUint first3w = whole.count();
        first3w -= whole.count_v3();
        first3w -= whole.count_v4();
        first3w -= whole.count_v5();
        first3w -= whole.count_v6();
        for (const std::string& t : {full, bush, std::string("sieve-fractal-v3\nlsystem 0.000001\naxiom A\nrule A A\n")})
        {
            const auto i = whole.index_of(t);
            CHECK(i && *i >= first3w && whole.at(*i) == t);
        }
    }
    // sieve-fractal-v4 (list X4): an IFS, an escape-time and a convergence formula, read and written back;
    // refusals; drawn; and their rulesets in the whole space after v3's.
    {
        const std::string fern = "sieve-fractal-v4\nifs\nmap 0 0 0 0.16 0 0\nmap 0.85 0.04 -0.04 0.85 0 1.6\nmap 0.2 -0.26 0.23 0.22 0 1.6\nmap -0.15 0.28 0.26 0.24 0 0.44\n";
        const std::string mandel = "sieve-fractal-v4\nescape + ^ z 2 c\nview -0.5 0 3\n";
        const std::string newton = "sieve-fractal-v4\nconverge - z / - ^ z 3 1 * 3 ^ z 2\njulia 0 0\n";
        const auto f = read_fractal(fern), m = read_fractal(mandel), n = read_fractal(newton);
        CHECK(f && f->family == Fractal::Family::Ifs && f->maps.size() == 4 && f->maps[1].value[5] == 1.6 && f->text() == fern && !f->grow(1));
        CHECK(m && m->family == Fractal::Family::Escape && m->formula.size() == 5 && m->view && m->view_value[0] == -0.5 && m->text() == mandel);
        CHECK(n && n->family == Fractal::Family::Converge && n->julia && n->text() == newton);
        for (const std::string& bad : {std::string("sieve-fractal-v4\nifs\n"), std::string("sieve-fractal-v4\nifs\nmap 0 0 0 0 0\n"),
                                      std::string("sieve-fractal-v4\nifs\nmap -0 0 0 0 0 0\n"), std::string("sieve-fractal-v4\nifs\nmap 1000 0 0 0 0 0\n"),
                                      std::string("sieve-fractal-v4\nescape + z\n"), std::string("sieve-fractal-v4\nescape z z\n"),
                                      std::string("sieve-fractal-v4\nescape 0\n"), std::string("sieve-fractal-v4\nescape tan z\n"),
                                      std::string("sieve-fractal-v4\nescape  z\n"), std::string("sieve-fractal-v4\nescape z\nview 0 0 0\n"),
                                      std::string("sieve-fractal-v4\nescape z\nview 0 0 1\njulia 0 0\n"),
                                      std::string("sieve-fractal-v4\nescape + + + + + + + + z z z z z z z z z\n")})
            CHECK(!read_fractal(bad));
        CHECK(read_fractal("sieve-fractal-v4\nescape + + + + + + + z z z z z z z z\n")); // (8 awaited after the seventh +: the most)
        // Drawn: the fern's points with every level kept, from its first map's fixed point (0, 0); the
        // Mandelbrot set's centre (-0.5, 0) inside it, its corner (-2, 1.5) escaped at once.
        const auto pts = fractal_points(*f, 2);
        CHECK(pts && pts->size() == 1 + 4 + 16 && (*pts)[0][0] == 0 && (*pts)[0][1] == 0);
        const auto img = draw_fractal(*m, 4, 33, 33);
        auto pixel = [&](uint32_t x, uint32_t y) { return std::array<uint8_t, 3>{img->rgba[(y * 33 + x) * 4], img->rgba[(y * 33 + x) * 4 + 1], img->rgba[(y * 33 + x) * 4 + 2]}; };
        CHECK(img && pixel(16, 16) == pixel(16, 17) && pixel(0, 0) != pixel(16, 16) && m->deepest() == 10 && fractal_iterations(4) == 64);
        const auto sierpinski = read_fractal("sieve-fractal-v4\nifs\nmap 0.5 0 0 0.5 0 0\nmap 0.5 0 0 0.5 0.5 0\nmap 0.5 0 0 0.5 0.25 0.433013\n");
        const auto dim = fractal_box_dimension(*sierpinski);
        CHECK(dim && std::fabs(dim->value - std::log(3.0) / std::log(2.0)) < 0.03 && !fractal_box_dimension(*m));
        // In the whole space: after v3's, from the shortest (26 characters: escape and one token), each
        // read back from its address; the first of them "escape c" (the leaves c i z, then a number).
        const FractalSpace space(96);
        BigUint first4 = space.count();
        first4 -= space.count_v4();
        first4 -= space.count_v5(); // (list Y2: v5's after v4's)
        first4 -= space.count_v6(); // (and v6's, list X5)
        CHECK(space.at(first4) == "sieve-fractal-v4\nescape c\n" && space.count_at(26) == BigUint(12) && space.count_at(30) == BigUint(28770 + 10 + 90)); // (and v5's elementary 0 to 9, v6's aggregate 10 to 99)
        const FractalSpace whole(kFractalSpaceLongestDefault); // (the fern is 133 characters)
        BigUint first4w = whole.count();
        first4w -= whole.count_v4();
        first4w -= whole.count_v5();
        first4w -= whole.count_v6();
        for (const std::string& t : {fern, mandel, newton, std::string("sieve-fractal-v4\nescape c\n")})
        {
            const auto i = whole.index_of(t);
            CHECK(i && *i >= first4w && whole.at(*i) == t);
        }
        // Against format-fractal-v5 (v4's and v5's): at 26 to 30 characters theirs alone, which the grammar
        // passes exactly.
        const Alphabet& a96 = alphabet_by_id("ascii96");
        const auto plugin = load_plugin_file(std::string(g_data) + "/filters/format-fractal-v6.sfilter"); // (v5's and v6's too)
        static const TestResources none(nullptr, nullptr);
        for (uint32_t l = 26; l <= 30; ++l)
            CHECK(space.count_at(l) == DfaRanker(compile_plugin(*plugin, FilterLine{"text", "ascii96", 96, l, &a96, 0, 0, 0}, {}, none), l).count());
    }
    // sieve-fractal-v5 (list Y2): cellular automata as a family of the fractal dimension, read and written
    // back; refusals; generations worked out; rule 90 from one cell Sierpinski's triangle (dimension
    // log 3 / log 2, its cells on a lattice); a glider the same five cells four generations on, moved; a
    // rule that wakes empty space waking all of it; and in the space, after v4's.
    {
        const std::string r90 = "sieve-fractal-v5\nelementary 90\n", glider = "sieve-fractal-v5\nlife B3/S23\nstart 010/001/111\n";
        const auto e = read_fractal(r90), l = read_fractal(glider);
        CHECK(e && l && e->text() == r90 && l->text() == glider && e->family == Fractal::Family::Elementary && l->family == Fractal::Family::Life && e->is_automaton() && !e->rewrites());
        CHECK(l->birth == (1u << 3) && l->survival == ((1u << 2) | (1u << 3)) && l->life_rule_text() == "B3/S23" && l->start_rows.size() == 3);
        for (const std::string& bad : {std::string("sieve-fractal-v5\nelementary 256\n"), std::string("sieve-fractal-v5\nelementary 090\n"),
                                       std::string("sieve-fractal-v5\nelementary 30\nstart \n"), std::string("sieve-fractal-v5\nelementary 30\nstart 012\n"),
                                       std::string("sieve-fractal-v5\nelementary 30\nstart ") + std::string(65, '1') + "\n", std::string("sieve-fractal-v5\nlife B33/S23\nstart 1\n"),
                                       std::string("sieve-fractal-v5\nlife B9/S23\nstart 1\n"), std::string("sieve-fractal-v5\nlife B3/S23\n"),
                                       std::string("sieve-fractal-v5\nlife B3/S23\nstart 1//1\n"), std::string("sieve-fractal-v5\nlife B3S23\nstart 1\n"),
                                       std::string("sieve-fractal-v5\nelementary 30\nstart 1\nstart 1\n"), std::string("sieve-fractal-v5\nifs\nmap 0 0 0 0 0 0\n")})
            CHECK(!read_fractal(bad));
        CHECK(read_fractal("sieve-fractal-v5\nlife B/S\nstart 1\n") && read_fractal(std::string("sieve-fractal-v5\nelementary 30\nstart ") + std::string(64, '1') + "\n"));
        const auto c90 = fractal_cells(*e, 3);
        // (generations 0 to 3 of rule 90 from one cell: 1, 101, 10001, 1010101, a row each, 7 wide)
        const std::vector<uint8_t> want = {0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1};
        CHECK(c90 && c90->width == 7 && c90->height == 4 && c90->cells == want && c90->beyond == 0);
        const auto d90 = fractal_box_dimension(*e);
        CHECK(d90 && std::fabs(d90->value - std::log(3.0) / std::log(2.0)) < 0.01 && !fractal_box_dimension(*l));
        auto live = [](const AutomatonCells& c) {
            std::vector<std::pair<uint32_t, uint32_t>> at;
            for (uint32_t y = 0; y < c.height; ++y)
                for (uint32_t x = 0; x < c.width; ++x)
                    if (c.cells[size_t(y) * c.width + x]) at.push_back({x, y});
            return at;
        };
        const auto g0 = fractal_cells(*l, 0), g4 = fractal_cells(*l, 4);
        auto a = live(*g0), b = live(*g4);
        CHECK(a.size() == 5 && b.size() == 5);
        for (auto& [x, y] : b) x -= 4 + 1, y -= 4 + 1; // (the window 4 bigger each way, and the glider a cell right and down)
        CHECK(a == b && g4->width == 11 && g4->height == 11);
        const auto wake = fractal_cells(*read_fractal("sieve-fractal-v5\nelementary 1\nstart 0\n"), 1);
        CHECK(wake && wake->beyond == 1 && wake->cells[0] == 0 && wake->cells[size_t(wake->width) * 1] == 1); // (000 makes 1: rule 1 wakes it)
        CHECK(e->deepest() == kFractalGenerationsMost && draw_fractal(*e, 31, 64, 64) && draw_fractal(*l, 8, 32, 32) && !draw_fractal(*l, l->deepest() + 1, 32, 32));
        const FractalSpace space(96);
        BigUint first5 = space.count();
        first5 -= space.count_v5();
        first5 -= space.count_v6();
        CHECK(space.at(first5) == "sieve-fractal-v5\nelementary 0\n" && space.index_of(r90) && space.at(*space.index_of(r90)) == r90 && space.at(*space.index_of(glider)) == glider);
        BigUint last = space.count();
        last -= BigUint(1);
        CHECK(read_fractal(space.at(last))->version == 6 && space.index_of(space.at(last)) == last); // (v6's after v5's, list X5)
    }
    // sieve-fractal-v6 (list X5): an attractor, a flow and an aggregate, read and written back; refusals;
    // Henon's attractor measured near its known 1.26; Lorenz's flow followed, finite and staying within its
    // butterfly; an aggregate the same from its seed every time, every particle stuck; and in the space,
    // after v5's, from "aggregate 1".
    {
        const std::string henon = "sieve-fractal-v6\nattractor\nx + - 1 * 1.4 ^ x 2 y\ny * 0.3 x\n";
        const std::string lorenz = "sieve-fractal-v6\nflow 0.01\nx * 10 - y x\ny - * x - 28 z y\nz - * x y * 2.666667 z\n";
        const std::string dla = "sieve-fractal-v6\naggregate 1\n";
        const auto h = read_fractal(henon), lz = read_fractal(lorenz), a = read_fractal(dla);
        CHECK(h && lz && a && h->text() == henon && lz->text() == lorenz && a->text() == dla && h->family == Fractal::Family::Attractor && lz->family == Fractal::Family::Flow &&
              a->family == Fractal::Family::Aggregate && h->is_path() && !a->is_path() && lz->from_value[0] == 1 && a->seed == 1);
        for (const std::string& bad : {std::string("sieve-fractal-v6\naggregate 0\n"), std::string("sieve-fractal-v6\naggregate 4294967296\n"),
                                       std::string("sieve-fractal-v6\nattractor\nx x\ny z\n"), std::string("sieve-fractal-v6\nattractor\nx conj x\ny x\n"),
                                       std::string("sieve-fractal-v6\nattractor\nx x\n"), std::string("sieve-fractal-v6\nattractor\nx x\ny x\nfrom 0 0 0\n"),
                                       std::string("sieve-fractal-v6\nflow 0\nx x\ny y\nz z\n"), std::string("sieve-fractal-v6\nflow 1\nx x\ny y\n"),
                                       std::string("sieve-fractal-v6\nattractor\nx + + + + + + + + x x x x x x x x x\ny x\n")})
            CHECK(!read_fractal(bad));
        const auto hd = fractal_box_dimension(*h);
        CHECK(hd && std::fabs(hd->value - 1.26) < 0.05 && !fractal_box_dimension(*lz));
        const auto path = fractal_path(*lz, 3);
        bool within = path && path->size() == size_t(1024) << 3;
        for (const auto& p : *path) within = within && std::fabs(p[0]) < 30 && std::fabs(p[1]) < 40 && p[2] > -1 && p[2] < 60;
        CHECK(within);
        const auto c1 = fractal_cells(*a, 2), c2 = fractal_cells(*a, 2);
        size_t stuck = 0;
        for (const uint8_t v : c1->cells) stuck += v;
        CHECK(c1 && c2 && c1->cells == c2->cells && c1->when == c2->when && stuck == 16u << 2 && draw_fractal(*a, 2, 32, 32) && draw_fractal(*h, 2, 32, 32) && draw_fractal(*lz, 2, 32, 32));
        const FractalSpace space(96);
        BigUint first6 = space.count();
        first6 -= space.count_v6();
        CHECK(space.at(first6) == dla && space.at(*space.index_of(henon)) == henon && space.at(*space.index_of(lorenz)) == lorenz);
    }
    // The shelves as charted in the repository: every entry read, and each grows at least a step.
    const auto chart = read_fractal_chart(std::filesystem::path(g_data) / "fractals");
    // A v2 chart: each fractal's "source" line after it, once; a fractal without one refused.
    {
        const auto dir = std::filesystem::temp_directory_path() / "sieve-test-chart-v2";
        std::filesystem::create_directories(dir);
        auto write = [&](const std::string& name, const std::string& text) { std::ofstream(dir / name, std::ios::binary) << text; };
        write("a.fractal", "sieve-fractal-v1\nlsystem 90\naxiom F\nrule F FF\n");
        write("chart.txt", "sieve-fractal-chart-v2\nfractal a A line\nsource a book, its figure 1\n");
        const auto c = read_fractal_chart(dir);
        CHECK(c.size() == 1 && c[0].source == "a book, its figure 1" && c[0].title == "A line");
        write("chart.txt", "sieve-fractal-chart-v2\nfractal a A line\n");
        CHECK(throws([&] { (void)read_fractal_chart(dir); }));
        write("chart.txt", "sieve-fractal-chart-v2\nfractal a A line\nsource one\nsource two\n");
        CHECK(throws([&] { (void)read_fractal_chart(dir); }));
        write("chart.txt", "sieve-fractal-chart-v1\nfractal a A line\n");
        CHECK(read_fractal_chart(dir).at(0).source.empty());
        std::filesystem::remove_all(dir);
    }
    // (list X2: 75, each with where it was transcribed from; the first 11 as they were)
    CHECK(chart.size() == 197 && chart[0].id == "fibonacci-word" && chart[10].id == "fractal-plant"); // (X3: 107; X4: 155; Y2: 187; X5: 197)
    for (const auto& c : chart) CHECK(c.fractal.deepest() >= 3 && c.fractal.text() == c.text && !c.source.empty());
    // The whole space (fractal-space-v1, list X1): every ruleset the reader takes, at an address of its
    // own. Up to 52 characters a ruleset has one rule, so the count is format-fractal-v1's exactly; at 53
    // two rules first fit, and the grammar also counts each pair the other way round and a symbol twice
    // (52 x 52 pairs against the reader's 52 x 51 / 2), with one-symbol words, a one-symbol axiom and a
    // one-digit angle.
    {
        const FractalSpace space(96);
        const Alphabet& a96 = alphabet_by_id("ascii96");
        // (fractal-space-v2, list X2: v1's first, at the addresses v1 gave them, then v2's; the grammar
        // format-fractal-v2 passes both)
        const auto plugin = load_plugin_file(std::string(g_data) + "/filters/format-fractal-v6.sfilter"); // (v3's to v6's too, lists X3, X4, Y2 and X5)
        static const TestResources none(nullptr, nullptr);
        for (uint32_t l = kFractalSpaceShortest; l <= 52; ++l)
        {
            const Dfa d = compile_plugin(*plugin, FilterLine{"text", "ascii96", 96, l, &a96, 0, 0, 0}, {}, none);
            CHECK(space.count_at(l) == DfaRanker(d, l).count());
        }
        {
            // v1's alone, against format-fractal-v1: at 53 two rules first fit, and the grammar also counts
            // each pair the other way round and a symbol twice (52 x 52 pairs against the reader's
            // 52 x 51 / 2), with one-symbol words, a one-symbol axiom and a one-digit angle.
            const FractalSpace v1(53);
            const auto plugin1 = load_plugin_file(std::string(g_data) + "/filters/format-fractal-v1.sfilter");
            BigUint grammar;
            for (uint32_t l = kFractalSpaceShortest; l <= 53; ++l)
                grammar += DfaRanker(compile_plugin(*plugin1, FilterLine{"text", "ascii96", 96, l, &a96, 0, 0, 0}, {}, none), l).count();
            BigUint ours = v1.count_v1();
            BigUint extra = BigUint::pow(57, 3);
            extra.mul_small(9 * (52 * 52 - 52 * 51 / 2));
            ours += extra;
            CHECK(ours == grammar);
        }
        CHECK(space.count_at(44) == BigUint::from_decimal("20010504322671072")); // (v1's 1,520,532, v3's 2,562,768, v4's, v5's and v6's)
        CHECK(space.at(BigUint(0)) == "sieve-fractal-v1\nlsystem 1\naxiom +\nrule A +\n");
        CHECK(space.at(space.count_v1()) == "sieve-fractal-v2\nlsystem 0.1\naxiom +\nrule A +\n");
        {
            BigUint last = space.count();
            last -= BigUint(1);
            CHECK(read_fractal(space.at(last)) && space.at(last).size() == 96 && space.index_of(space.at(last)) == last);
        }
        // Every charted fractal but four is in it at the default longest (256), and its
        // address gives it back; a v1 one's address is the same whatever the longest, a v2 one's follows
        // every v1 ruleset to the longest, so moves with it.
        {
            const FractalSpace whole(kFractalSpaceLongestDefault);
            for (const auto& c : chart)
            {
                if (c.text.size() > kFractalSpaceLongestDefault)
                {
                    // (Voderberg's tiling 590 characters, and X4's Fractint koch3 272, the Sierpinski carpet 331 and
                    // Fractint's own name 690: the four past the default)
                    CHECK(c.id == "voderberg-spiral-tiling" || c.id == "fractint-ifs-fractint" || c.id == "fractint-ifs-koch3" || c.id == "sierpinski-carpet-ifs" ||
                          c.id == "life-gosper-gun"); // (and Y2's Gosper glider gun, 368)
                    continue;
                }
                const auto i = whole.index_of(c.text);
                CHECK(i && whole.at(*i) == c.text && (c.text.size() > 96 || c.fractal.version >= 2 || space.index_of(c.text) == i));
            }
        }
        // Addresses across the space read back the same, each a ruleset the reader takes, the shorter
        // first; and texts it does not hold have none.
        std::mt19937_64 g(7);
        size_t last_length = 0;
        bool v2_met = false, v3_met = false, v4_met = false, v5_met = false, v6_met = false;
        BigUint v6_from = space.count();
        v6_from -= space.count_v6();
        BigUint v5_from = v6_from;
        v5_from -= space.count_v5();
        BigUint v4_from = v5_from;
        v4_from -= space.count_v4();
        BigUint v3_from = v4_from;
        v3_from -= space.count_v3();
        BigUint step = space.count();
        step.divmod_small(997);
        BigUint at;
        for (int k = 0; k < 997; ++k, at += step)
        {
            BigUint i = at;
            i.add_small(uint32_t(g() % 1000));
            if (i >= space.count()) break;
            const std::string t = space.at(i);
            if (!v2_met && i >= space.count_v1()) v2_met = true, last_length = 0; // (v2's begin again from the shortest)
            if (!v3_met && i >= v3_from) v3_met = true, last_length = 0;          // (and v3's)
            if (!v4_met && i >= v4_from) v4_met = true, last_length = 0;          // (and v4's)
            if (!v5_met && i >= v5_from) v5_met = true, last_length = 0;          // (and v5's)
            if (!v6_met && i >= v6_from) v6_met = true, last_length = 0;          // (and v6's)
            CHECK(read_fractal(t).has_value() && space.index_of(t) == i && t.size() >= last_length);
            last_length = t.size();
        }
        CHECK(!space.index_of("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 1 0\nrule 0 01\n")); // (rules out of order)
        CHECK(!space.index_of(std::string("sieve-fractal-v1\nlsystem 90\naxiom F\nrule F ") + std::string(64, 'F') + "\n" + "rule G " + std::string(30, 'G') + "\n")); // (longer than 96)
        CHECK(throws([&] { (void)space.at(space.count()); }) && throws([] { FractalSpace(43); }));
    }
    // Its noise filters (core/src/filters/fractals.cpp): each judges a ruleset's text, of any length up
    // to the line's, by what its axiom reaches.
    {
        FilterLine fl;
        fl.kind = "fractals", fl.symbols_id = "fractals/96", fl.base = 128, fl.length = 96, fl.up_to_length = true;
        static const TestResources none(nullptr, nullptr);
        const char* names[] = {"fractal-draws-v1", "fractal-grows-v1", "fractal-no-idle-v1", "fractal-canonical-v1"};
        // Which of the four a ruleset passes, as four characters (1 passes, 0 fails).
        auto judged = [&](const std::string& t) {
            std::string r;
            const std::vector<uint32_t> u(t.begin(), t.end());
            for (const char* n : names) r += FilterStack(fl, {{find_filter(n), {}}}, none).passes(u) ? '1' : '0';
            return r;
        };
        auto text_of = [&](const std::string& id) {
            for (const auto& c : chart)
                if (c.id == id) return c.text;
            return std::string();
        };
        CHECK(judged(text_of("koch-curve")) == "1111");
        CHECK(judged(text_of("hilbert-curve")) == "1111");
        CHECK(judged(text_of("dragon-curve")) == "1110"); // (X and Y: a renaming of A and B)
        CHECK(judged("sieve-fractal-v1\nlsystem 90\naxiom FA\nrule A A+BF+\nrule B -FA-B\n") == "1111");
        CHECK(judged(text_of("cantor-set")) == "1110"); // (1 met first: a renaming of 0)
        CHECK(judged(text_of("thue-morse")) == "1111");
        CHECK(judged("sieve-fractal-v1\nlsystem 1\naxiom +\nrule A +\n") == "0000"); // (draws nothing; A never met)
        CHECK(judged("sieve-fractal-v1\nlsystem 90\naxiom F\nrule F F\n") == "1001"); // (never grows; F to itself)
        CHECK(judged("sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 00\n") == "0111"); // (one colour)
        CHECK(judged("not a ruleset") == "0000");
        const FilterStack all(fl, {{find_filter(names[0]), {}}, {find_filter(names[1]), {}}}, none);
        const std::string draws_not = "sieve-fractal-v1\nlsystem 1\naxiom +\nrule A +\n";
        CHECK(all.first_failure(std::vector<uint32_t>(draws_not.begin(), draws_not.end())) == 0);
        CHECK(throws([&] { (void)all.first_failure(std::vector<uint32_t>(97, 'F')); })); // (longer than the line)
        for (const char* n : names) CHECK(find_filter(n)->category == "hard" && !find_filter(n)->judge_only); // (counted since list F)
        {
            // List Z3: fractal-strange-v1 (attractors and flows) and fractal-lasts-v1 (automata), on the charted
            // ones and on generated ones from the v5 and v6 blocks.
            FilterLine wide = fl;
            wide.symbols_id = "fractals/256", wide.length = 256;
            const FilterStack strange_st(wide, {{find_filter("fractal-strange-v1"), {}}}, none), lasts_st(wide, {{find_filter("fractal-lasts-v1"), {}}}, none);
            auto passes = [](const FilterStack& st, const std::string& t) { return st.passes(std::vector<uint32_t>(t.begin(), t.end())); };
            std::string strange_out, lasts_out;
            for (const auto& c : chart)
            {
                if (c.text.size() > 256) continue; // (past the line: charted, not generated)
                if (!passes(strange_st, c.text)) strange_out += c.id + " ";
                if (!passes(lasts_st, c.text)) lasts_out += c.id + " ";
            }
            // (Bogdanov's map at the charted settings traces loops, not chaos; the Game of Life's oscillators,
            // gliders and spaceship, rule 184's traffic and the charted starts that settle do not last)
            CHECK(strange_out == "bogdanov-attractor ");
            CHECK(lasts_out == "rule-184 life-glider life-blinker life-toad life-beacon life-pulsar life-lwss diamoeba morley two-by-two anneal ");
            const FractalSpace big(256);
            BigUint v6_from = big.count();
            v6_from -= big.count_v6();
            BigUint v5_from = v6_from;
            v5_from -= big.count_v5();
            uint64_t state = 1;
            for (const auto& [from, span, st, name] : {std::tuple{v5_from, big.count_v5(), &lasts_st, "v5"}, std::tuple{v6_from, big.count_v6(), &strange_st, "v6"}})
            {
                size_t kept = 0, paths = 0, n = 0;
                for (int k = 0; k < 300; ++k)
                {
                    // (a spread of the block: a 60-bit number scaled to it)
                    state = state * 6364136223846793005ull + 1442695040888963407ull;
                    BigUint i = span;
                    i.mul_small(uint32_t(state >> 40));
                    i >>= 24;
                    i += from;
                    const std::string t = big.at(i);
                    const auto f = read_fractal(t);
                    if (!f) continue;
                    if (std::string(name) == "v6" && !f->is_path()) continue;
                    ++n, ++paths;
                    if (passes(*st, t)) ++kept;
                }
                // (of 300 spread over each block: v5's automata, 195 last; v6's attractors and flows, none strange)
                CHECK(n == 300 && kept == (std::string(name) == "v5" ? 195u : 0u));
            }
        }
    }
    // In other lines (list X1c): fractal-drawn-v1 on the image line, the charted fractals drawn at each
    // depth and fitted as a picture is. Counted and ranked exactly, both ways round.
    {
        set_fractal_chart_folder(std::string(g_data) + "/fractals");
        static const TestResources none(nullptr, nullptr);
        ImageFormat fmt;
        fmt.width = 10, fmt.height = 10, fmt.palette = &palette_by_id("mono");
        FilterLine il{"image", fmt.symbols_id(), 2, 100, nullptr, 10, 10, 1};
        const FilterSpec* spec = find_filter("fractal-drawn-v1");
        const FilterStack in(il, {{spec, {{"keep", "include"}}}}, none), out(il, {{spec, {{"keep", "exclude"}}}}, none);
        CHECK(in.ranker() && out.ranker());
        const BigUint kept = in.ranker()->count();
        BigUint rest = BigUint::pow(2, 100);
        rest -= kept;
        // (112 with X1's 11 charted, 567 with X2's 75, 782 with X3's 107, 1,057 with X4's 155; Y2's automata, an
        // elementary one's history another picture at each of its 512 depths, bring it to 5,244)
        CHECK(kept == BigUint(5279) && out.ranker()->count() == rest); // (X5's attractors, flows and aggregate: 5,279)
        {
            // In the video line (list Y4): a charted fractal growing, its frames its depths in turn; at 10 x 10
            // and 4 frames in black and white, 1,774 of them, counted and ranked exactly both ways round.
            ImageFormat vf = fmt;
            vf.frames = 4;
            FilterLine vl{"video", vf.symbols_id(), 2, 400, nullptr, 10, 10, 4};
            const FilterSpec* mv = find_filter("fractal-moving-v1");
            CHECK(mv && mv->applies(vl) && !mv->applies(il) && !spec->applies(vl));
            const FilterStack vin(vl, {{mv, {{"keep", "include"}}}}, none), vout(vl, {{mv, {{"keep", "exclude"}}}}, none);
            BigUint vrest = BigUint::pow(2, 400);
            vrest -= vin.ranker()->count();
            CHECK(vin.ranker() && vout.ranker() && vin.ranker()->count() == BigUint(1774) && vout.ranker()->count() == vrest);
        }
        {
            // Version 2 (IDEAS 19.244): turned, mirrored and coloured other ways. As drawn it is v1's set; its
            // defaults (every turn, inverted too) and any colouring (on black and white, the same as inverted:
            // two colours, two ways) count and rank exactly; on ega16, any colouring is counted and judged.
            const FilterSpec* d2 = find_filter("fractal-drawn-v2");
            CHECK(d2 && !d2->retired && spec->retired && spec->replaced_by == "fractal-drawn-v2");
            const auto count_of = [&](const FilterLine& line, const FilterSpec* s, FilterValues v) {
                const FilterStack st(line, {{s, v}}, none);
                return st.ranker() ? st.ranker()->count().to_decimal() : std::string("judged");
            };
            CHECK(count_of(il, d2, {{"keep", "include"}, {"turns", "drawn"}, {"colours", "drawn"}}) == "5279"); // (v1's own)
            CHECK(count_of(il, d2, {{"keep", "include"}, {"colours", "drawn"}}) == "31190");                       // (turned)
            CHECK(count_of(il, d2, {{"keep", "include"}}) == "62356");                                             // (and inverted)
            CHECK(count_of(il, d2, {{"keep", "include"}, {"colours", "any"}}) == "62356");
            {
                const FilterStack all_out(il, {{d2, {{"keep", "exclude"}}}}, none);
                BigUint others = BigUint::pow(2, 100);
                others -= BigUint(62356);
                CHECK(all_out.ranker() && all_out.ranker()->count() == others);
            }
            ImageFormat vf = fmt;
            vf.frames = 4;
            FilterLine vl{"video", vf.symbols_id(), 2, 400, nullptr, 10, 10, 4};
            CHECK(count_of(vl, find_filter("fractal-moving-v2"), {{"keep", "include"}}) == "23830");
            ImageFormat ef = fmt;
            ef.palette = &palette_by_id("ega16");
            FilterLine el{"image", ef.symbols_id(), 16, 100, nullptr, 10, 10, 1};
            CHECK(count_of(el, d2, {{"keep", "include"}}) == "91206");
            const auto any16 = d2->make(el, {{"keep", "include"}, {"colours", "any"}}, none);
            // (45,180 shapes, each in every colouring: counted exactly, judged)
            CHECK(!any16->ranker() && any16->survivors() && any16->survivors()->to_decimal() == "12830096076621376");
            {
                // (a picture of the chart, its colours swapped about, passes: the shape is what is kept)
                const auto img = draw_fractal(chart.front().fractal, 2, 40, 40);
                CHECK(img.has_value());
                ImageFormat f16 = ef;
                auto u = canonicalise_frame(*img, f16);
                CHECK(any16->passes(u));
                for (auto& p : u) p = (p + 5) % 16;
                CHECK(any16->passes(u));
                std::vector<uint32_t> flat(100, 3);
                flat[0] = 4, flat[1] = 4, flat[2] = 9; // (three colours in no drawn shape)
                CHECK(!any16->passes(flat));
            }
        }
        {
            // The models line (list Z2): the charted fractals' lines as meshes, a line a face (a, b, b), where a
            // depth fits the line's 8 vertices and 12 faces, in each of the 24 turns of a cube.
            {
                // (a v3 ruleset's lines leave the plane where it pitches; v1's stay in it)
                const auto f3 = read_fractal("sieve-fractal-v3\nlsystem 90\naxiom F&F\nrule F F\n");
                const auto f1 = read_fractal("sieve-fractal-v1\nlsystem 90\naxiom F+F\nrule F F\n");
                CHECK(f3 && f1);
                if (f3 && f1)
                {
                    const auto l3 = fractal_lines3(*f3, "F&F"), l1 = fractal_lines3(*f1, "F+F");
                    CHECK(l3.size() == 2 && l1.size() == 2 && std::abs(l3[1][5]) > 0.5 && l1[1][5] == 0);
                }
            }
            const ModelSpace space(8, 12, 16);
            const FilterSpec* fm = find_filter("fractal-modelled-v1");
            CHECK(fm && fm->applies(sieve::FilterLine{"models", "", 0, 0}));
            const ModelSieve in(space, {{fm, {{"keep", "include"}}}}), out(space, {{fm, {{"keep", "exclude"}}}});
            const ModelSieve drawn(space, {{fm, {{"keep", "include"}, {"turns", "drawn"}}}});
            // (69 fitted, each twice: its lacking faces as the line fills them and as its own lines again; 3,216 turned)
            CHECK(in.count() == BigUint(3216) && drawn.count() == BigUint(138));
            CHECK(in.can_rank() && out.can_rank());
            BigUint rest = space.size();
            rest -= in.count();
            CHECK(out.count() == rest);
            // Ranked both ways round: each survivor's address and back.
            for (uint64_t k : {uint64_t(0), uint64_t(1), uint64_t(7)})
            {
                const BigUint m = in.model_at(BigUint(k), AddressMode::Positional);
                CHECK(in.first_failure(m).empty() && out.first_failure(m) == "fractal-modelled-v1");
                CHECK(in.index_of(m, AddressMode::Positional) == BigUint(k));
                const BigUint o = out.model_at(m, AddressMode::Positional); // (the survivor at the same number)
                CHECK(out.first_failure(o).empty() && out.index_of(o, AddressMode::Positional) == m);
            }
            // With the line's own rules: no degenerate faces sets every one aside already (a line is a face
            // (a, b, b)); with distinct vertices, the others' survivors without the fractals', still ranked.
            const FilterSpec* di = find_filter("distinct-indices-v1");
            const FilterSpec* dv = find_filter("distinct-vertices-v1");
            const ModelSieve none_left(space, {{di, {}}, {fm, {{"keep", "include"}}}});
            CHECK(none_left.can_rank() && none_left.count().is_zero());
            const ModelSieve base(space, {{dv, {}}}), culled(space, {{dv, {}}, {fm, {{"keep", "exclude"}}}}),
                kept(space, {{dv, {}}, {fm, {{"keep", "include"}}}});
            BigUint left = base.count();
            left -= kept.count();
            CHECK(kept.count() == BigUint(864));
            CHECK(culled.can_rank() && culled.count() == left);
            for (uint64_t k : {uint64_t(0), uint64_t(12345)})
            {
                const BigUint m = culled.model_at(BigUint(k), AddressMode::Scrambled);
                CHECK(culled.first_failure(m).empty() && culled.index_of(m, AddressMode::Scrambled) == BigUint(k));
            }
        }
        if (!(kept == BigUint(5279)) || std::getenv("SIEVE_FRACTAL_DIGEST"))
        {
            // (where the count differs from machine to machine, each charted fractal's pictures as one hash,
            // drawn and fitted as fractal-drawn-v1 draws and fits them, to compare against another machine's)
            std::cout << "fractal-drawn-v1 at 10x10: " << kept.to_decimal() << " pictures; each fractal's, depth by depth:\n";
            for (const auto& c : chart)
            {
                std::string frames;
                const uint32_t deepest = fractal_deepest_within(c.fractal, size_t(1) << 18);
                for (uint32_t d = 0; d <= deepest; ++d)
                    if (const auto img = draw_fractal(c.fractal, d, 40, 40))
                    {
                        const auto u = canonicalise_frame(*img, fmt);
                        frames += std::to_string(d) + ":";
                        for (const uint32_t p : u) frames += char('0' + p);
                        frames += "\n";
                    }
                std::cout << "  " << c.id << " " << Sha256::hex_hash(frames).substr(0, 12) << "\n";
            }
        }
        // The Koch curve at depth 2, drawn and fitted, is one of them; stripes are not. (A picture all of
        // one colour is: a substitution's axiom of one symbol fills it.)
        const auto koch = std::find_if(chart.begin(), chart.end(), [](const ChartedFractal& c) { return c.id == "koch-curve"; });
        const auto drawn = canonicalise_frame(*draw_fractal(koch->fractal, 2, 40, 40), fmt);
        CHECK(in.passes(drawn) && !out.passes(drawn));
        std::vector<uint32_t> stripes(100);
        for (size_t p = 0; p < 100; ++p) stripes[p] = uint32_t(p % 2);
        CHECK(!in.passes(stripes) && out.passes(stripes));
        CHECK(in.passes(std::vector<uint32_t>(100, 1)));
        // Each survivor's rank gives it back, in address order.
        std::vector<uint32_t> last;
        for (uint64_t k = 0; k < 782; ++k)
        {
            const auto u = in.ranker()->unrank(BigUint(k));
            CHECK(in.passes(u) && in.ranker()->rank(u) == BigUint(k) && (k == 0 || last < u));
            last = u;
        }
        const auto u = out.ranker()->unrank(BigUint(5));
        CHECK(out.passes(u) && out.ranker()->rank(u) == BigUint(5));
    }
}

void test_raid_net()
{
    namespace fs = std::filesystem;
    auto hex = [](std::string_view b) {
        static const char* k = "0123456789abcdef";
        std::string t;
        for (const char c : b) t += k[uint8_t(c) >> 4], t += k[uint8_t(c) & 15];
        return t;
    };
    auto view = [](const std::array<uint8_t, 32>& a) { return std::string_view(reinterpret_cast<const char*>(a.data()), a.size()); };
    // RFC 8439 2.4.2 (ChaCha20, counter 1) and RFC 4231 cases 2 and 6 (HMAC-SHA-256).
    {
        std::array<uint8_t, 32> key{};
        for (int i = 0; i < 32; ++i) key[size_t(i)] = uint8_t(i);
        const std::array<uint8_t, 12> nonce = {0, 0, 0, 0, 0, 0, 0, 0x4a, 0, 0, 0, 0};
        const std::string plain = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        CHECK(hex(chacha20(key, nonce, 1, plain)) ==
              "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0bf91b65c5524733ab8f593dabcd62b3571639d624e65152ab8f530c359f0861d807ca0dbf500d6a6156a38e088a22b65e52bc514d16ccf806818ce91ab77937365af90bbf74a35be6b40b8eedf2785e42874d");
        CHECK(chacha20(key, nonce, 1, chacha20(key, nonce, 1, plain)) == plain);
        CHECK(hex(view(hmac_sha256("Jefe", "what do ya want for nothing?"))) == "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
        CHECK(hex(view(hmac_sha256(std::string(131, '\xaa'), "Test Using Larger Than Block-Size Key - Hash Key First"))) ==
              "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
        // sieve-seal-v1, the same as an independent reckoning (Python's cryptography package).
        SealKey k{};
        for (int i = 0; i < 32; ++i) k[size_t(i)] = uint8_t(i);
        const std::string sealed = seal(k, "The infinite RAID");
        CHECK(hex(sealed) == "4b1885e43f0cd8187f9aa35fdc52a31b529821d45c5ad3a363ea456c2714e2944a632516ef452d97855021c039e97fa322e0792abfd2a6054774afcdcc");
        CHECK(unseal(k, sealed) == std::optional<std::string>("The infinite RAID"));
        CHECK(seal(k, "").size() == kSealOverhead && unseal(k, seal(k, "")) == std::optional<std::string>(""));
        for (size_t i = 0; i < sealed.size(); ++i) // any one bit changed: refused
        {
            std::string t = sealed;
            t[i] = char(t[i] ^ (1 << (i % 8)));
            CHECK(!unseal(k, t));
        }
        CHECK(!unseal(k, sealed.substr(1)) && !unseal(k, "short"));
        SealKey other = k;
        other[0] ^= 1;
        CHECK(!unseal(other, sealed));
        CHECK(read_seal_key(seal_key_text(k)) == k && new_seal_key() != new_seal_key());
        CHECK(throws([] { (void)read_seal_key("sieve-raid-key-v1 00"); }) && throws([] { (void)read_seal_key("sieve-raid-key-v1 " + std::string(64, 'G')); }));
    }
    const fs::path root = fs::temp_directory_path() / ("sieve-raid-net-test-" + std::to_string(std::random_device{}()));
    std::error_code ec;
    fs::remove_all(root, ec);
    auto sha = [](const std::string& b) { return Sha256::hex(Sha256::hash(b)); };
    // A held folder: limits per address and in all; the address itself never written down.
    {
        HeldLimits lim;
        lim.space = 3000;
        lim.per_peer = 2000;
        RaidHeld held(root / "held", lim);
        const std::string a(1500, 'a'), b(1000, 'b'), c(900, 'c');
        CHECK(held.put(sha(a), a, "198.51.100.7").is == RaidHeld::Put::Is::Kept);
        CHECK(held.put(sha(a), a, "198.51.100.7").is == RaidHeld::Put::Is::Had); // had already: not counted twice
        CHECK(held.put(sha(b), b, "198.51.100.7").is == RaidHeld::Put::Is::Refused); // over this address's 2000
        CHECK(held.put(sha(b), b, "203.0.113.9").is == RaidHeld::Put::Is::Kept);
        CHECK(held.put(sha(c), c, "192.0.2.1").is == RaidHeld::Put::Is::Refused); // over the 3000 in all
        CHECK(held.put(sha(c), c + "x", "192.0.2.1").is == RaidHeld::Put::Is::Refused); // bytes not their name
        CHECK(held.used() == 2500 && held.used_by("198.51.100.7") == 1500 && held.get(sha(a)) == a);
        std::ifstream pf(root / "held" / "peers", std::ios::binary);
        const std::string peers((std::istreambuf_iterator<char>(pf)), std::istreambuf_iterator<char>());
        CHECK(peers.find("198.51") == std::string::npos && peers.find(held.peer_key("198.51.100.7")) != std::string::npos);
        RaidHeld again(root / "held", lim); // opened again: the same salt, the same counts
        CHECK(again.used_by("198.51.100.7") == 1500 && again.peer_key("x") == held.peer_key("x"));
    }
    // The protocol, answered from a script of requests.
    {
        struct Script : RaidStream
        {
            std::string in, out;
            size_t at = 0;
            bool read(char* to, size_t n) override
            {
                if (in.size() - at < n) return false;
                std::memcpy(to, in.data() + at, n);
                at += n;
                return true;
            }
            bool write(std::string_view b) override
            {
                out.append(b);
                return true;
            }
        };
        RaidHeld held(root / "proto");
        const std::string blob = "a sealed chunk", name = sha(blob);
        Script s;
        s.in = "sieve-raid-net-v1\nhas " + name + "\nput " + name + " " + std::to_string(blob.size()) + "\n" + blob + "has " + name + "\nget " + name + "\nput " + name + " 3\nabcbye\n";
        CHECK(held.serve(s, "192.0.2.5") == 5);
        CHECK(s.out == "sieve-raid-net-v1\nno\nkept\nyes\nblob " + std::to_string(blob.size()) + "\n" + blob + "refused its bytes are not its name\n");
        Script bad;
        bad.in = "hello\n";
        CHECK(held.serve(bad, "192.0.2.5") == 0 && bad.out.rfind("refused", 0) == 0);
        Script odd;
        odd.in = "sieve-raid-net-v1\nput " + name + " 99999999999\n";
        CHECK(held.serve(odd, "192.0.2.5") == 0 && odd.out.find("refused too long") != std::string::npos);
        Script lng;
        lng.in = "sieve-raid-net-v1\n" + std::string(500, 'x') + "\n";
        CHECK(held.serve(lng, "192.0.2.5") == 0);
        // Asked together (OPTIMISATIONS 2): a peer's `has` lines go in one write, the answers read in
        // turn; the same answers as one at a time.
        struct Counted : Script
        {
            size_t writes = 0;
            bool write(std::string_view b) override
            {
                ++writes;
                return Script::write(b);
            }
        };
        auto counted = std::make_unique<Counted>();
        Counted* c = counted.get();
        c->in = "sieve-raid-net-v1\nyes\nno\nyes\n";
        {
            RaidNetPeer peer(std::move(counted), "p:1");
            const std::vector<std::string> names = {std::string(64, 'a'), std::string(64, 'b'), std::string(64, 'c')};
            CHECK(peer.has_each(names) == std::vector<bool>({true, false, true}));
            CHECK(c->writes == 2 && c->out == "sieve-raid-net-v1\nhas " + names[0] + "\nhas " + names[1] + "\nhas " + names[2] + "\n");
        }
    }
    // Written outside the lock (OPTIMISATIONS 6): eight puts at once of one piece keep it once and say
    // had seven times; of eight pieces with room for five, five are kept; the counts and the peers file
    // as if one after another, and no part file left.
    {
        HeldLimits lim;
        lim.space = 5000;
        RaidHeld held(root / "together", lim);
        const std::string same(700, 's');
        std::vector<std::string> own;
        for (int i = 0; i < 8; ++i) own.push_back(std::string(1000, char('A' + i)));
        std::atomic<int> kept{0}, had{0}, own_kept{0}, own_refused{0};
        std::vector<std::thread> ts;
        for (int i = 0; i < 8; ++i)
            ts.emplace_back([&, i] {
                const auto a = held.put(sha(same), same, "192.0.2.9");
                if (a.is == RaidHeld::Put::Is::Kept) ++kept;
                if (a.is == RaidHeld::Put::Is::Had) ++had;
                const auto b = held.put(sha(own[size_t(i)]), own[size_t(i)], "192.0.2." + std::to_string(i));
                (b.is == RaidHeld::Put::Is::Kept ? own_kept : own_refused) += 1;
            });
        for (auto& t : ts) t.join();
        CHECK(kept == 1 && had == 7);
        CHECK(own_kept == 4 && own_refused == 4); // (700 + 4 x 1000 within the 5000)
        CHECK(held.used() == 4700 && held.used_by("192.0.2.9") == 700);
        RaidHeld again(root / "together", lim); // the peers file as the counts
        CHECK(again.used() == 4700);
        size_t parts = 0;
        for (const auto& e : fs::recursive_directory_iterator(root / "together"))
            parts += e.path().filename().string().find(".part") != std::string::npos ? 1 : 0;
        CHECK(parts == 0);
    }
    // Shared to three peers and fetched back; a new version sends only its changed chunks.
    {
        Raid raid({root / "d1", root / "d2"});
        std::string bytes;
        std::mt19937 g(5);
        for (int i = 0; i < 900000; ++i) bytes.push_back(char(g() & 0xFF));
        const std::string v1 = raid.put("game.sav", bytes);
        RaidHeld h1(root / "p1"), h2(root / "p2"), h3(root / "p3");
        RaidLocalPeer p1(h1, "p1:1"), p2(h2, "p2:1"), p3(h3, "p3:1");
        const std::vector<RaidPeer*> peers = {&p1, &p2, &p3};
        SealKey key = new_seal_key();
        const auto r1 = raid_share(raid, v1, key, peers);
        const size_t n = raid.list()[0].chunks.size();
        // (each chunk on 3 of the 3, the default since 13 November 2026; the manifest and the index too)
        CHECK(r1.whole() && r1.chunks == n && r1.sent == 3 * n + 3 + 3 && r1.had == 0);
        CHECK(raid_share(raid, v1, key, peers).sent == 0); // all there already
        const auto f = raid_fetch(r1.remote, key, peers);
        CHECK(f.bytes == bytes && f.manifest.id() == v1);
        // Nothing a peer holds can be read without the key: no chunk of the save appears in it.
        for (const auto& e : fs::recursive_directory_iterator(root / "p1"))
            if (e.is_regular_file() && e.path().parent_path().parent_path().filename() == "blobs")
            {
                std::ifstream in(e.path(), std::ios::binary);
                const std::string b((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                CHECK(b.size() < 64 || bytes.find(b.substr(12, 32)) == std::string::npos);
            }
        bytes[400000] ^= 1;
        const std::string v2 = raid.put("game.sav", bytes);
        const auto r2 = raid_share(raid, v2, key, peers);
        CHECK(r2.whole() && r2.sent <= 3 + 3 + 3 && raid_fetch(r2.remote, key, peers).bytes == bytes);
        // Kept whole by its owner: one peer loses everything it kept; check finds every piece short,
        // puts each back from the others, and then finds it whole, sending nothing more.
        {
            const auto before = raid_check(r2.remote, key, peers, 3, false);
            CHECK(before.whole_now() && before.short_of == 0 && before.pieces == n + 2);
            std::error_code e2;
            fs::remove_all(root / "p3" / "blobs", e2);
            const auto seen = raid_check(r2.remote, key, peers, 3, false);
            CHECK(!seen.whole_now() && seen.short_of == n + 2 && seen.repaired == 0 && seen.lost == 0);
            const auto mended = raid_check(r2.remote, key, peers, 3, true);
            CHECK(mended.whole_now() && mended.repaired == n + 2 && mended.bytes_sent > 0);
            const auto after = raid_check(r2.remote, key, peers);
            CHECK(after.whole_now() && after.short_of == 0 && after.bytes_sent == 0);
            // Asked for 2: each piece on 3 is more than enough.
            CHECK(raid_check(r2.remote, key, peers, 2).whole == n + 2);
        }
        // One peer gone: still whole from the others. Every copy of a piece changed: refused, not wrong.
        const std::vector<RaidPeer*> two = {&p1, &p2};
        CHECK(raid_fetch(r2.remote, key, two).bytes == bytes);
        for (const fs::path& d : {root / "p1", root / "p2", root / "p3"})
            for (const auto& e : fs::recursive_directory_iterator(d))
                if (e.is_regular_file() && e.path().parent_path().parent_path().filename() == "blobs" && e.path().filename() != r2.remote)
                    std::ofstream(e.path(), std::ios::binary | std::ios::app) << "x";
        CHECK(throws([&] { (void)raid_fetch(r2.remote, key, peers); }));
        SealKey wrong = key;
        wrong[31] ^= 1;
        CHECK(throws([&] { (void)raid_fetch(r1.remote, wrong, peers); }));
        CHECK(throws([&] { (void)raid_share(raid, v1, key, {}); }));
    }
    CHECK(raid_split_address("127.0.0.1:47100") == std::make_pair(std::string("127.0.0.1"), std::string("47100")));
    CHECK(raid_split_address("[::1]:9") == std::make_pair(std::string("::1"), std::string("9")));
    CHECK(throws([] { (void)raid_split_address("::1:9"); }) && throws([] { (void)raid_split_address("host:0"); }) && throws([] { (void)raid_split_address("host"); }));
    fs::remove_all(root, ec);
}

// Stage 4 of the catalogue (IDEAS 19.165): a measured figure read back as written; anything else
// refused; records kept per entry, a line that is not one skipped; and added up as said.
void test_format_stats()
{
    namespace fs = std::filesystem;
    FormatStat s;
    s.date = "2026-11-12";
    s.machine = "desk";
    s.what = "pack";
    s.result = "lossless";
    s.figures = {{"bytes", 1000}, {"deflate", 600}, {"install-us", 1000}, {"lzma2", 500}, {"make-us", 20000}, {"sieve", 400}};
    const std::string line = "sieve-format-stat-v1 date=2026-11-12 machine=desk what=pack result=lossless bytes=1000 deflate=600 install-us=1000 lzma2=500 make-us=20000 sieve=400";
    CHECK(s.text() == line && read_format_stat(line).text() == line && read_format_stat(line).get("lzma2") == 500 && read_format_stat(line).get("none") == 0);
    for (const std::string& bad : {std::string("sieve-format-stat-v2 date=2026-11-12 machine=desk what=pack result=lossless"),
                                  std::string("sieve-format-stat-v1 date=2026-1-12 machine=desk what=pack result=lossless"),
                                  std::string("sieve-format-stat-v1 date=2026-11-12 machine=Desk what=pack result=lossless"),
                                  std::string("sieve-format-stat-v1 date=2026-11-12 machine=desk what=pack result=lossless b=1 a=2"),
                                  std::string("sieve-format-stat-v1 date=2026-11-12 machine=desk what=pack result=lossless a=1 a=2"),
                                  std::string("sieve-format-stat-v1 date=2026-11-12 machine=desk what=pack result=lossless a=-1"),
                                  std::string("sieve-format-stat-v1 date=2026-11-12 machine=desk what=pack result=lossless  a=1"),
                                  std::string("sieve-format-stat-v1 machine=desk date=2026-11-12 what=pack result=lossless")})
        CHECK(throws([&] { (void)read_format_stat(bad); }));
    CHECK(format_stats_machine("My Desk PC!") == "my-desk-pc-" && format_stats_machine("") .size() > 0 && format_stats_today().size() == 10);
    const fs::path dir = fs::temp_directory_path() / ("sieve-stats-test-" + std::to_string(std::random_device{}()));
    std::error_code ec;
    fs::remove_all(dir, ec);
    CHECK(format_stats(dir, "file-png").empty() && measured_entries(dir).empty() && format_stats_summary({}).empty());
    add_format_stat(dir, "file-png", s);
    FormatStat t = s;
    t.machine = "laptop";
    t.result = "differs";
    t.figures = {{"bytes", 3000}, {"deflate", 1800}, {"install-us", 9000}, {"lzma2", 1500}, {"make-us", 60000}, {"sieve", 1200}};
    add_format_stat(dir, "file-png", t);
    std::ofstream(dir / "file-png.stats", std::ios::binary | std::ios::app) << "not a record\r\n";
    FormatStat imp;
    imp.date = "2026-11-12";
    imp.machine = "desk";
    imp.what = "import";
    imp.result = "partial";
    imp.figures = {{"faces-in", 3552}, {"faces-kept", 9}, {"vertices-in", 1857}, {"vertices-kept", 8}};
    add_format_stat(dir, "model-obj", imp);
    CHECK(throws([&] { add_format_stat(dir, "../escape", s); }));
    size_t skipped = 0;
    const auto png = format_stats(dir, "file-png", &skipped);
    CHECK(png.size() == 2 && skipped == 1 && png[1].machine == "laptop");
    CHECK(measured_entries(dir) == std::vector<std::string>({"file-png", "model-obj"}));
    const auto sum = format_stats_summary(png);
    CHECK(sum.size() == 2 && sum[0].find("packed 2 times on 2 machines, 2026-11-12: 4,000 bytes to 1,600 (40.0%); LZMA2 alone 2,000 (50.0%), deflate 2,400 (60.0%)") != std::string::npos);
    CHECK(sum[1].find("round trips 1 of 2 lossless") != std::string::npos && sum[1].find("made in 20,971 ms a MiB") != std::string::npos);
    const auto im = format_stats_summary(format_stats(dir, "model-obj"));
    CHECK(im.size() == 2 && im[0].find("8 of 1,857 vertices kept, 9 of 3,552 faces") != std::string::npos && im[1].find("0 of 1 whole") != std::string::npos);
    const auto sh = format_stats_short(png);
    CHECK(sh.size() == 2 && sh[0] == "packed 2x: 40.0% of its bytes (LZMA2 50.0%)" && sh[1] == "1 of 2 lossless; 21.0 s a MiB to make");
    CHECK(format_id_of_file_kind("PNG") == "file-png" && format_id_of_file_kind("?") == "file-unknown-bytes" && format_by_id(format_id_of_file_kind("JPG")));
    // The File Locator's readings (what=read): LZMA2 and deflate added up over the compared alone.
    FormatStat rd;
    rd.date = "2026-11-13";
    rd.machine = "desk";
    rd.what = "read";
    rd.result = "read";
    rd.figures = {{"bytes", 1000}, {"deflate", 700}, {"lzma2", 600}, {"make-us", 1000}, {"sieve", 500}};
    FormatStat rd2 = rd;
    rd2.figures = {{"bytes", 3000}, {"make-us", 3000}, {"sieve", 1500}}; // (Compare unticked)
    const auto rs = format_stats_summary({rd, rd2});
    CHECK(rs.size() == 2 && rs[0].find("read       in the File Locator 2 times on 1 machine, 2026-11-13: 4,000 bytes to 2,000 (50.0%)") != std::string::npos);
    CHECK(rs[1].find("of the 1 compared: LZMA2 alone 60.0%, deflate 70.0%") != std::string::npos);
    CHECK(format_stats_short({rd, rd2}) == std::vector<std::string>({"read 2x in the File Locator: 50.0% of its bytes"}));
    fs::remove_all(dir, ec);
}

// A character's save (sieve-save-v1, IDEAS 19.183): the character, its animation and its languages,
// named together; the languages kept in order, each once; read back the same.
void test_save()
{
    CharacterSave s;
    s.character = unfold_character(CharacterSettings{}).id();
    s.speak("language-welsh", true);
    s.speak("language-english", true);
    s.speak("language-welsh", true); // twice: still once
    CHECK(s.speaks == (std::vector<std::string>{"language-english", "language-welsh"}));
    CHECK(s.text() == "sieve-save-v1\ncharacter " + s.character + "\nspeaks language-english\nspeaks language-welsh\n");
    CHECK(read_save(s.text()).id() == s.id());
    const std::string before = s.id();
    s.speak("language-welsh", false);
    CHECK(s.id() != before && s.speaks.size() == 1);
    s.animation = std::string(64, 'a');
    CHECK(read_save(s.text()).animation == s.animation);
    CHECK(throws([] { (void)read_save("sieve-save-v1\ncharacter x\n"); }));
    CHECK(throws([&] { (void)read_save("sieve-save-v1\ncharacter " + std::string(64, 'b') + "\nspeaks z\nspeaks a\n"); }));          // out of order
    CHECK(throws([&] { (void)read_save("sieve-save-v1\ncharacter " + std::string(64, 'b') + "\nspeaks a\nspeaks a\n"); }));          // twice
    CHECK(throws([&] { (void)read_save("sieve-save-v1\ncharacter " + std::string(64, 'b') + "\nspeaks a\nanimation " + std::string(64, 'c') + "\n"); }));
    // sieve-save-v2 (IDEAS 19.186): its actions' map after the animation; none, and it is v1 as before.
    const std::string v1 = s.text();
    s.actions = std::string(64, 'd');
    CHECK(s.text() == "sieve-save-v2\ncharacter " + s.character + "\nanimation " + std::string(64, 'a') + "\nactions " + std::string(64, 'd') + "\nspeaks language-english\n");
    CHECK(read_save(s.text()).actions == s.actions && read_save(s.text()).id() == s.id());
    s.actions.clear();
    CHECK(s.text() == v1);
    CHECK(throws([&] { (void)read_save("sieve-save-v2\ncharacter " + std::string(64, 'b') + "\n"); }));           // v2 with no map
    CHECK(throws([&] { (void)read_save("sieve-save-v1\ncharacter " + std::string(64, 'b') + "\nactions " + std::string(64, 'd') + "\n"); })); // a map in v1
}

// Actions to animations (IDEAS 19.186, sieve-actions-v1): every action once, in order, each a SHA-256
// or none; read back the same; an animation fits a character with as many spheres as it has bones.
void test_actions()
{
    ActionMap m;
    CHECK(m.empty() && m.text().rfind("sieve-actions-v1\naction idle none\naction walk none\n", 0) == 0);
    CHECK(read_actions(m.text()).text() == m.text() && m.text().size() == std::string("sieve-actions-v1\n").size() + 118);
    m.set(Action::Walk, std::string(64, 'a'));
    m.set(Action::Wave, std::string(64, 'b'));
    CHECK(!m.empty() && m.of(Action::Walk) == std::string(64, 'a') && read_actions(m.text()).id() == m.id());
    CHECK(action_from_name("run") == Action::Run && std::string(action_name(Action::Read)) == "read");
    CHECK(throws([] { (void)action_from_name("fly"); }));
    const std::string t = m.text();
    CHECK(throws([&] { (void)read_actions(t.substr(0, t.size() - 1)); }));                                         // no last line feed
    std::string swapped = t;
    swapped.replace(swapped.find("action idle"), 11, "action walk");
    CHECK(throws([&] { (void)read_actions(swapped); }));                                                             // out of order
    CHECK(throws([&] { (void)read_actions("sieve-actions-v1\naction idle none\n"); }));                             // too few
    std::string bad = t;
    bad.replace(bad.find(std::string(64, 'a')), 64, std::string(64, 'A'));
    CHECK(throws([&] { (void)read_actions(bad); }));                                                                 // not a SHA-256
    // Fitting: the arm (3 spheres) plays a 3-bone animation, not a 4-bone one.
    Character arm = unfold_character(CharacterSettings{});
    Animation a;
    a.bones = uint32_t(arm.spheres.size());
    a.frames = 1;
    a.turns.assign(a.bones, BoneTurn{});
    CHECK(animation_fits(arm, a));
    a.bones += 1;
    a.turns.push_back(BoneTurn{});
    CHECK(!animation_fits(arm, a));
}

// The catalogue of formats, stage 1 (IDEAS 19.165): gathered from the code; ids unique; every link
// names an entry; every entry reaches "information" (the wiki principle); one entry pinned.
// The Sieve folder's data/, found from the tests folder main is given (its sibling), so the tests read
// it wherever they are run from: ctest runs them in the build folder (step 01), not the Sieve folder.
std::string g_data = "data/";

void test_catalogue()
{
    const auto& all = format_catalogue();
    std::set<std::string> ids;
    for (const auto& e : all) CHECK(ids.insert(e.id).second);
    for (const auto& e : all)
    {
        for (const auto& l : e.links)
        {
            if (!format_by_id(l.to)) std::cerr << "format " << e.id << " links to " << l.to << ", which is not there\n";
            CHECK(format_by_id(l.to) != nullptr);
        }
        if (format_lineage(e.id).empty()) std::cerr << "format " << e.id << " does not reach information\n";
        CHECK(!format_lineage(e.id).empty());
    }
    // Every file kind, importer and built-in alphabet is in it.
    for (const auto& k : file_kinds())
    {
        std::string id = k == "?" ? "unknown-bytes" : k;
        for (char& c : id) c = char(std::tolower(static_cast<unsigned char>(c)));
        const FormatEntry* e = format_by_id("file-" + id);
        if (!e) std::cerr << "file kind " << k << " has no entry\n";
        CHECK(e && e->kind == "file");
    }
    CHECK(format_by_id("file-png") && format_by_id("model-gltf") && format_by_id("alphabet-dna4") && format_by_id("script-greek") && format_by_id(kAnimationVersion));
    CHECK(format_lineage("file-flac") == (std::vector<std::string>{"file-flac", "sound", "data", "information"}));
    CHECK(format_by_id("file-flac")->recognised == "first bytes 66 4C 61 43");
    CHECK(format_by_id("file-flac")->version() == "22b664d61e80e56a39b304716ed25c849dcfb3b783d662b5f3842736f90b4081");
    const auto path = format_path("file-png", "alphabet-dna4");
    CHECK(path.size() >= 3 && path.front() == "file-png" && path.back() == "alphabet-dna4");
    CHECK(format_path("file-png", "nowhere").empty() && !format_by_id("nowhere"));
    // Stage 2: every entry reads back from its own text, the same entry.
    for (const auto& e : all) CHECK(read_format_entry(e.text()).text() == e.text());
    CHECK(throws([] { (void)read_format_entry("sieve-format-v1\nid Bad Id\nkind x\nname y\n"); }));
    CHECK(throws([] { (void)read_format_entry("sieve-format-v1\nid a\nkind x\nname y\nsieve s\nlink part-of data\n"); })); // out of order
    CHECK(throws([] { (void)read_format_entry("sieve-format-v1\nid a\nkind x\nname y"); }));                              // no last line feed
    CHECK(read_format_entry("sieve-format-v1\nid a\nkind x\nname y\nlink part-of data\nsource z\n").links.size() == 1);
    // The hand-written entries (data/catalogue) join the code's, every one reaching information.
    CHECK(std::filesystem::is_directory(g_data + "catalogue") && std::filesystem::is_directory(g_data + "languages")); // (found, wherever the tests run)
    const auto with = format_catalogue_with(g_data + "catalogue");
    CHECK(with.size() > all.size() && std::any_of(with.begin(), with.end(), [](const FormatEntry& e) { return e.id == "encoding-utf-8"; }));
    for (const auto& e : with) CHECK(!format_lineage(with, e.id).empty());
    // The language catalogue (data/languages, IDEAS 19.183) joins the same web: every language reaches
    // information through its family and "language", and links to the scripts it is written in.
    const auto both = format_catalogue_with(std::vector<std::string>{g_data + "catalogue", g_data + "languages"});
    size_t languages = 0;
    for (const auto& e : both)
    {
        CHECK(!format_lineage(both, e.id).empty());
        if (e.kind != "language") continue;
        ++languages;
        CHECK(std::any_of(e.links.begin(), e.links.end(), [](const FormatLink& l) { return l.type == "written-in" && l.to.rfind("script-", 0) == 0; }));
    }
    CHECK(languages >= 13);
    CHECK(format_lineage(both, "language-welsh") == (std::vector<std::string>{"language-welsh", "family-indo-european", "language", "information"}));
    // Refused: an id already in the catalogue, a link to nothing, a file named otherwise.
    namespace fs = std::filesystem;
    const fs::path dir = fs::temp_directory_path() / ("sieve-catalogue-test-" + std::to_string(std::random_device{}()));
    std::error_code ec;
    auto put = [&](const std::string& name, const std::string& text) {
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);
        std::ofstream(dir / name, std::ios::binary) << text;
    };
    put("file-png.format", "sieve-format-v1\nid file-png\nkind file\nname again\nlink part-of image\n");
    CHECK(throws([&] { (void)format_catalogue_with(dir.string()); }));
    put("x.format", "sieve-format-v1\nid x\nkind k\nname n\nlink part-of nothing-here\n");
    CHECK(throws([&] { (void)format_catalogue_with(dir.string()); }));
    put("y.format", "sieve-format-v1\nid z\nkind k\nname n\nlink part-of data\n");
    CHECK(throws([&] { (void)format_catalogue_with(dir.string()); }));
    put("z.format", "sieve-format-v1\r\nid z\r\nkind k\r\nname n\r\nlink part-of data\r\n"); // written on Windows: still read
    CHECK(format_catalogue_with(dir.string()).size() == all.size() + 1);
    fs::remove_all(dir, ec);
}

// `--part NAME[,NAME...]` (Edward, IDEAS 19.236: "splitting the test scripts down even further ... that
// way we can more easily pinpoint what is going wrong"): only the tests of those parts of the suite, each
// part one ctest entry (CMakeLists.txt, core-NAME), so a red step 01 names the part that failed and that
// part can be run alone. Every test is in exactly one part: the PART(...) lines in run_all() mark where
// each begins, and --list names each test's part.
const char* const kParts[] = {"spaces", "characters", "hallway", "fractals", "models", "vectors", "filters", "cross", "books", "text-sound", "mixing"};
std::vector<std::string> g_parts;
std::string g_part;

#define PART(name) g_part = name

// `sieve_tests DIR --only NAME[,NAME...]` (Edward, IDEAS 19.10: to check one area without the whole
// suite): only the tests whose call names one of them (as part of it: "mix" runs test_mix to
// test_mix4); `--list` names them all and runs none.
std::vector<std::string> g_only;
bool g_list = false;

bool wanted(const char* call)
{
    if (g_list)
    {
        std::cout << g_part << ": " << call << "\n";
        return false;
    }
    if (!g_parts.empty() && std::find(g_parts.begin(), g_parts.end(), g_part) == g_parts.end()) return false;
    if (g_only.empty()) return true;
    const std::string c = call;
    for (const std::string& o : g_only)
        if (c.find(o) != std::string::npos) return true;
    return false;
}

// `--times`: each test's seconds after it, to see which part of the suite the time goes to.
bool g_times = false;

#define RUN(call) \
    do \
    { \
        if (wanted(#call)) \
        { \
            const auto run_from = std::chrono::steady_clock::now(); \
            call; \
            if (g_times) std::cout << "time " << std::chrono::duration<double>(std::chrono::steady_clock::now() - run_from).count() << " s " << #call << "\n"; \
        } \
    } while (false)

// A character's voxels (IDEAS 19.214, sieve/character_voxels.hpp), the cubes every space draws: none
// for no spheres or no cell; one sphere's cubes alike every way; every face flat across the way it faces,
// a cell wide, and between a filled cell and an empty one (so as many face each way as face back, the
// surface being closed); a cell in two spheres belonging to the one it lies deeper in; and the cell
// from the character's height. One character's face count is pinned, so any change to the cubes shows.
void test_character_voxels()
{
    CHECK(voxel_faces({}, 4.0f).empty());
    Sphere a;
    a.r = 40;
    CHECK(voxel_faces({a}, 0.0f).empty());
    {
        const auto faces = voxel_faces({a}, 10.0f);
        size_t by_dir[6] = {};
        for (const VoxelFace& f : faces)
        {
            ++by_dir[f.dir];
            CHECK(f.sphere == 0);
            const int* dv = kVoxelDirs[f.dir];
            const int ax = dv[0] ? 0 : dv[1] ? 1 : 2;
            for (int k = 1; k < 4; ++k) CHECK(f.corner[k][ax] == f.corner[0][ax]);
            // (the face a cell's side: half a cell from its middle, the way it faces)
            CHECK(std::fabs((f.corner[0][ax] - f.mid[ax]) - float(dv[ax]) * 5.0f) < 1e-4f);
            float span[3] = {};
            for (int j = 0; j < 3; ++j)
                for (int k = 0; k < 4; ++k) span[j] = std::max(span[j], std::fabs(f.corner[k][j] - f.corner[0][j]));
            CHECK(span[ax] == 0 && span[(ax + 1) % 3] == 10.0f && span[(ax + 2) % 3] == 10.0f);
        }
        CHECK(!faces.empty());
        for (int d = 1; d < 6; ++d) CHECK(by_dir[d] == by_dir[0]);
    }
    {
        // Two spheres that overlap: the surface closed, and the far ends each its own sphere's.
        Sphere b = a;
        b.x = 50;
        b.r = 30;
        const auto faces = voxel_faces({a, b}, 8.0f);
        size_t by_dir[6] = {};
        float least_x = 1e9f, most_x = -1e9f;
        uint32_t least_owner = 9, most_owner = 9;
        for (const VoxelFace& f : faces)
        {
            ++by_dir[f.dir];
            if (f.mid[0] < least_x) least_x = f.mid[0], least_owner = f.sphere;
            if (f.mid[0] > most_x) most_x = f.mid[0], most_owner = f.sphere;
        }
        CHECK(by_dir[0] == by_dir[1] && by_dir[2] == by_dir[3] && by_dir[4] == by_dir[5]);
        CHECK(least_owner == 0 && most_owner == 1);
    }
    CharacterSettings t;
    t.seed = 99;
    t.limbs = 5;
    t.branches = 1;
    const Character c = unfold_character(t);
    int32_t ex, ey, ez;
    c.extent(ex, ey, ez);
    CHECK(voxel_cell(c) == std::max(1.0f, float(ey) / 16.0f));
    const auto faces = voxel_faces(c.spheres, voxel_cell(c));
    size_t by_dir[6] = {};
    for (const VoxelFace& f : faces)
    {
        ++by_dir[f.dir];
        CHECK(f.sphere < c.spheres.size());
    }
    CHECK(by_dir[0] == by_dir[1] && by_dir[2] == by_dir[3] && by_dir[4] == by_dir[5]);
    std::printf("  the character of seed 99 (5 limbs, 1 branch): %zu voxel faces at a cell of %.3f units\n", faces.size(), double(voxel_cell(c)));
    CHECK(faces.size() == 2896);
    // A character far wider than it is tall (two spheres of radius 40, one set 100,000,000 units out, as
    // a store's v3 file can say): under the height rule its cells grow until its widest side is within
    // kVoxelLatticeMostCells, so its grid stays small, rather than asking for one too large to hold.
    {
        Character wide;
        Sphere a, b;
        a.r = b.r = 40;
        a.kind = Sphere::Body;
        b.parent = 0;
        b.x = 100000000;
        wide.spheres = {a, b};
        const VoxelCell vc = voxel_cell_of(wide, VoxelSizing{});
        int32_t wx, wy, wz;
        wide.extent(wx, wy, wz);
        CHECK(vc.coarser);
        CHECK(2.0 * double(wx) / double(vc.cell) <= double(kVoxelLatticeMostCells) + 1e-3);
        CHECK(voxel_cell(wide) == vc.cell);
        const auto wf = voxel_faces(wide.spheres, vc.cell);
        CHECK(wf.size() < 100000);
        CHECK(!voxel_cell_of(c, VoxelSizing{}).coarser);
    }
}

// How big a voxel is (IDEAS 19.216): the height rule as voxel_cell; the lattice's cell a power of two of
// its base, the coarsest at which the smallest sphere is still `across` cells wide (or the base itself),
// coarser only to keep within kVoxelLatticeMostCells; its grid anchored at whole cells from the centre.
// Greedy meshing (IDEAS 19.215): the rectangles cover exactly the faces they join (as many cells, the same
// area each way), never join two colours, and are no more than the faces; one sphere in one colour joins
// into far fewer.
void test_voxel_lattice_and_joining()
{
    CharacterSettings t;
    t.seed = 99;
    t.limbs = 5;
    t.branches = 1;
    const Character c = unfold_character(t);
    CHECK(voxel_cell(c, VoxelSizing{}) == voxel_cell(c));
    int32_t least = 0;
    for (const Sphere& p : c.spheres)
        if (p.r > 0 && (least == 0 || p.r < least)) least = p.r;
    for (uint32_t base : {1u, 5u, 10u, 40u})
        for (uint32_t across : {1u, 2u, 4u})
        {
            VoxelSizing s;
            s.rule = VoxelSizing::Lattice;
            s.base_mm = base, s.across = across;
            const VoxelCell vc = voxel_cell_of(c, s);
            const float b = float(base) * float(kCharacterUnitsPerMetre) / 1000.0f;
            CHECK(std::fabs(vc.cell - b * float(1 << vc.level)) < 1e-3f * vc.cell);
            const float asks = 2.0f * float(least) / float(across);
            if (!vc.coarser)
            {
                CHECK(vc.level == 0 || vc.cell <= asks * 1.0001f);
                CHECK(vc.cell * 2.0f > asks * 0.9999f || vc.level == 24);
            }
            int32_t ex, ey, ez;
            c.extent(ex, ey, ez);
            CHECK(2.0f * float(std::max({ex, ey, ez})) / vc.cell <= float(kVoxelLatticeMostCells));
        }
    {
        VoxelSizing s;
        s.rule = VoxelSizing::Lattice;
        const float cell = voxel_cell(c, s);
        for (const VoxelFace& f : voxel_faces(c.spheres, cell, true))
            for (int k = 0; k < 4; ++k)
                for (int j = 0; j < 3; ++j)
                {
                    const float n = f.corner[k][j] / cell;
                    CHECK(std::fabs(n - std::round(n)) < 1e-3f);
                }
    }
    CHECK(join_voxel_faces({}, {}).empty());
    auto check_join = [&](const std::vector<VoxelFace>& faces, const std::vector<uint32_t>& colours, float cell) {
        const auto quads = join_voxel_faces(faces, colours);
        CHECK(quads.size() <= faces.size());
        size_t cells = 0;
        double area[6] = {}, joined[6] = {};
        for (const VoxelFace& f : faces) area[f.dir] += double(cell) * double(cell);
        for (const VoxelQuad& q : quads)
        {
            cells += q.cells;
            CHECK(q.dir == faces[q.face].dir);
            const int* dv = kVoxelDirs[q.dir];
            const int ax = dv[0] ? 0 : dv[1] ? 1 : 2;
            for (int k = 1; k < 4; ++k) CHECK(q.corner[k][ax] == q.corner[0][ax]);
            const double su = std::fabs(double(q.corner[1][(ax + 1) % 3]) - q.corner[0][(ax + 1) % 3]),
                         sv = std::fabs(double(q.corner[2][(ax + 2) % 3]) - q.corner[1][(ax + 2) % 3]);
            CHECK(std::fabs(su * sv - double(q.cells) * cell * cell) < 1e-2 * cell * cell);
            joined[q.dir] += su * sv;
        }
        CHECK(cells == faces.size());
        for (int d = 0; d < 6; ++d) CHECK(std::fabs(area[d] - joined[d]) < 1e-2 * double(cell) * double(cell) * (1.0 + area[d] / (double(cell) * cell)));
        return quads.size();
    };
    {
        Sphere a;
        a.r = 400;
        const auto faces = voxel_faces({a}, 10.0f);
        const std::vector<uint32_t> one(faces.size(), 7u);
        const size_t n = check_join(faces, one, 10.0f);
        std::printf("  one sphere 80 cells across: %zu voxel faces joined into %zu rectangles\n", faces.size(), n);
        CHECK(n * 2 < faces.size());
        // Every face a colour of its own: nothing joins.
        std::vector<uint32_t> own(faces.size());
        for (size_t i = 0; i < own.size(); ++i) own[i] = uint32_t(i);
        CHECK(check_join(faces, own, 10.0f) == faces.size());
    }
    {
        const float cell = voxel_cell(c);
        const auto faces = voxel_faces(c.spheres, cell);
        std::vector<uint32_t> by_sphere(faces.size());
        for (size_t i = 0; i < faces.size(); ++i) by_sphere[i] = faces[i].sphere;
        const size_t n = check_join(faces, by_sphere, cell);
        std::printf("  the character of seed 99: its %zu voxel faces joined, one colour a sphere, into %zu rectangles\n", faces.size(), n);
        CHECK(n < faces.size());
    }
}

// A character's skin (IDEAS 19.225, sieve/character_skin.hpp): nothing for no spheres or no cell; one
// sphere's skin closed (every edge between exactly two triangles, and as many vertices less edges plus
// triangles as a sphere has: two), lying on the sphere to within a cell, its normals outwards and its
// triangles wound to face them; two overlapping spheres' neck where they meet as the plain union makes it
// with no blend, and filled out as the smooth minimum says with one, the far ends each its own sphere's.
// One character's triangles are pinned, so any change to the skin shows.
void check_skin_quads(const SkinMesh& m, bool pieces_allowed);
void test_character_skin()
{
    CHECK(skin_mesh({}, 4.0f, 0.0f).vertices.empty());
    Sphere a;
    a.r = 100;
    CHECK(skin_mesh({a}, 0.0f, 0.0f).tris.empty());
    // Closed: each edge (its two ends, either way round) between two triangles, and V - E + F = 2 (one
    // piece, no handles). `pieces_allowed` for a character, whose limbs may stand apart and whose skin
    // may touch itself in a cell: then only no edge left open (each between an even number of triangles).
    auto closed = [](const SkinMesh& m, bool pieces_allowed = false) {
        std::map<std::pair<uint32_t, uint32_t>, int> edges;
        for (size_t t = 0; t < m.triangles(); ++t)
            for (int k = 0; k < 3; ++k)
            {
                const uint32_t i = m.tris[t * 3 + size_t(k)], j = m.tris[t * 3 + size_t((k + 1) % 3)];
                ++edges[{std::min(i, j), std::max(i, j)}];
            }
        for (const auto& e : edges)
            if (pieces_allowed ? e.second % 2 != 0 : e.second != 2) return false;
        return pieces_allowed || int64_t(m.vertices.size()) - int64_t(edges.size()) + int64_t(m.triangles()) == 2;
    };
    {
        const SkinMesh m = skin_mesh({a}, 10.0f, 0.0f);
        CHECK(m.triangles() > 0);
        CHECK(closed(m));
        for (const SkinVertex& v : m.vertices)
        {
            const float d = std::sqrt(v.p[0] * v.p[0] + v.p[1] * v.p[1] + v.p[2] * v.p[2]);
            CHECK(std::fabs(d - 100.0f) < 10.0f);
            CHECK(v.p[0] * v.n[0] + v.p[1] * v.n[1] + v.p[2] * v.n[2] > 0); // (outwards)
            CHECK(v.sphere == 0);
        }
        for (size_t t = 0; t < m.triangles(); ++t)
        {
            const float* p0 = m.vertices[m.tris[t * 3]].p;
            const float* p1 = m.vertices[m.tris[t * 3 + 1]].p;
            const float* p2 = m.vertices[m.tris[t * 3 + 2]].p;
            const float e1[3] = {p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]}, e2[3] = {p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2]};
            const float n[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
            CHECK(n[0] * (p0[0] + p1[0] + p2[0]) + n[1] * (p0[1] + p1[1] + p2[1]) + n[2] * (p0[2] + p1[2] + p2[2]) > 0); // (wound to face out)
        }
        // A blend changes nothing for one sphere.
        const SkinMesh b = skin_mesh({a}, 10.0f, 50.0f);
        CHECK(b.vertices.size() == m.vertices.size() && b.tris == m.tris);
    }
    {
        // Two spheres of radius 100, 160 apart: the plain union's neck is 60 wide at the middle (100^2 =
        // 80^2 + 60^2); with a blend of 100, where both are 0 at the middle the smooth minimum is a quarter
        // of the blend below them, so the neck reaches out to sqrt(125^2 - 80^2), about 96.
        Sphere l = a, r = a;
        l.x = -80, r.x = 80;
        auto neck = [&](float blend) {
            const SkinMesh m = skin_mesh({l, r}, 4.0f, blend);
            CHECK(closed(m));
            float most = 0;
            for (const SkinVertex& v : m.vertices)
                if (std::fabs(v.p[0]) < 2.0f) most = std::max(most, std::sqrt(v.p[1] * v.p[1] + v.p[2] * v.p[2]));
            float least_x = 1e9f, most_x = -1e9f;
            uint32_t least_owner = 9, most_owner = 9;
            for (const SkinVertex& v : m.vertices)
            {
                if (v.p[0] < least_x) least_x = v.p[0], least_owner = v.sphere;
                if (v.p[0] > most_x) most_x = v.p[0], most_owner = v.sphere;
            }
            CHECK(least_owner == 0 && most_owner == 1);
            return most;
        };
        const float hard = neck(0.0f), soft = neck(100.0f);
        std::printf("  two spheres' neck: %.1f units across with no blend, %.1f with a blend of 100\n", double(hard), double(soft));
        CHECK(std::fabs(hard - 60.0f) < 4.0f);
        CHECK(std::fabs(soft - 96.0f) < 4.0f);
    }
    // The blend a percentage of the spheres' mean radius; the cell the voxels' halved, doubled to fit.
    {
        Sphere s = a;
        s.r = 300;
        CHECK(skin_blend({a, s}, 50) == 100.0f);
        CHECK(skin_blend({}, 50) == 0.0f);
        CHECK(skin_cell({a}, 8.0f, 1).cell == 4.0f && !skin_cell({a}, 8.0f, 1).coarser);
        const SkinCell big = skin_cell({a}, 1.0f, 3); // (200 across in eighths: 1,600 cells, doubled to fit)
        CHECK(big.coarser && 200.0f / big.cell <= float(kSkinMostCells) && 200.0f / big.cell > float(kSkinMostCells) / 2);
    }
    CharacterSettings t;
    t.seed = 99;
    t.limbs = 5;
    t.branches = 1;
    const Character c = unfold_character(t);
    const SkinCell sc = skin_cell(c.spheres, voxel_cell(c), 1);
    const SkinMesh m = skin_mesh(c.spheres, sc.cell, skin_blend(c.spheres, 50));
    CHECK(closed(m, true));
    for (const SkinVertex& v : m.vertices) CHECK(v.sphere < c.spheres.size());
    std::printf("  the character of seed 99 (5 limbs, 1 branch): %zu skin triangles, %zu vertices, at a cell of %.3f units\n", m.triangles(), m.vertices.size(), double(sc.cell));
    CHECK(m.triangles() == 23180);
    check_skin_quads(m, true);
}

// The skin's quads (IDEAS 19.214): one four-sided face for every two triangles, face q being triangles 2q
// and 2q + 1 with the same corners and winding, so the quads are the same surface; one sphere's quads
// closed as a surface of quads (each side between exactly two faces, V - E + F = 2), a character's with
// no side left open. And the smooth drawing's normals, averaged across the faces: one a vertex, of length
// one, outwards on a sphere and within a few degrees of its true normal (how near, and how near the
// field's slope is, printed).
void check_skin_quads(const SkinMesh& m, bool pieces_allowed)
{
    CHECK(m.quads.size() * 6 == m.tris.size() * 4);
    for (size_t q = 0; q < m.quad_count(); ++q)
    {
        const uint32_t* c = &m.quads[q * 4];
        const uint32_t* t = &m.tris[q * 6];
        CHECK(t[0] == c[0] && t[1] == c[1] && t[2] == c[2] && t[3] == c[0] && t[4] == c[2] && t[5] == c[3]);
    }
    std::map<std::pair<uint32_t, uint32_t>, int> sides;
    for (size_t q = 0; q < m.quad_count(); ++q)
        for (int k = 0; k < 4; ++k)
        {
            const uint32_t i = m.quads[q * 4 + size_t(k)], j = m.quads[q * 4 + size_t((k + 1) % 4)];
            ++sides[{std::min(i, j), std::max(i, j)}];
        }
    bool closed = true;
    for (const auto& e : sides)
        if (pieces_allowed ? e.second % 2 != 0 : e.second != 2) closed = false;
    CHECK(closed);
    if (!pieces_allowed) CHECK(int64_t(m.vertices.size()) - int64_t(sides.size()) + int64_t(m.quad_count()) == 2);
}

void test_skin_quads_and_smooth()
{
    Sphere a;
    a.r = 100;
    const SkinMesh m = skin_mesh({a}, 10.0f, 0.0f);
    CHECK(m.quad_count() > 0);
    check_skin_quads(m, false);
    const std::vector<float> n = skin_smooth_normals(m);
    CHECK(n.size() == m.vertices.size() * 3);
    double off_smooth = 0, off_field = 0;
    for (size_t v = 0; v < m.vertices.size(); ++v)
    {
        const float* p = m.vertices[v].p;
        const float* s = &n[v * 3];
        const float d = std::sqrt(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
        CHECK(std::fabs(s[0] * s[0] + s[1] * s[1] + s[2] * s[2] - 1.0f) < 1e-4f);
        const float along = (s[0] * p[0] + s[1] * p[1] + s[2] * p[2]) / d;
        CHECK(along > 0.95f); // (outwards, and near the sphere's own normal)
        off_smooth += 1.0 - double(along);
        off_field += 1.0 - double((m.vertices[v].n[0] * p[0] + m.vertices[v].n[1] * p[1] + m.vertices[v].n[2] * p[2]) / d);
    }
    std::printf("  one sphere's skin: %zu quads; its normals' mean distance from the true (1 - cosine): averaged %.5f, the field's %.5f\n", m.quad_count(),
                off_smooth / double(m.vertices.size()), off_field / double(m.vertices.size()));
    CHECK(skin_smooth_normals(SkinMesh{}).empty());
}

// The skin's limbs (IDEAS 19.229, the pipeline's first stage): the rounded cone's distance exact (outside
// it, the nearest of the spheres swept along the bone, found by trying a thousand of them; inside,
// negative); no limbs, the skin exactly as before; one sphere has none; two spheres apart are two pieces
// of skin alone and one closed piece with their limb, as wide at its middle as the cone; and characters
// of many limbs and one that stands apart closed, in as many pieces as the limbs leave them.
static size_t skin_pieces(const SkinMesh& m)
{
    std::vector<uint32_t> up(m.vertices.size());
    for (size_t i = 0; i < up.size(); ++i) up[i] = uint32_t(i);
    auto top = [&](uint32_t i) {
        while (up[i] != i) i = up[i] = up[up[i]];
        return i;
    };
    for (size_t t = 0; t < m.triangles(); ++t)
        for (int k = 1; k < 3; ++k) up[top(m.tris[t * 3 + size_t(k)])] = top(m.tris[t * 3]);
    size_t n = 0;
    for (size_t i = 0; i < up.size(); ++i) n += top(uint32_t(i)) == i;
    return n;
}

static bool skin_closed(const SkinMesh& m, bool pieces_allowed)
{
    std::map<std::pair<uint32_t, uint32_t>, int> edges;
    for (size_t t = 0; t < m.triangles(); ++t)
        for (int k = 0; k < 3; ++k)
        {
            const uint32_t i = m.tris[t * 3 + size_t(k)], j = m.tris[t * 3 + size_t((k + 1) % 3)];
            ++edges[{std::min(i, j), std::max(i, j)}];
        }
    for (const auto& e : edges)
        if (pieces_allowed ? e.second % 2 != 0 : e.second != 2) return false;
    return pieces_allowed || int64_t(m.vertices.size()) - int64_t(edges.size()) + int64_t(m.triangles()) == 2;
}

static bool same_skin(const SkinMesh& x, const SkinMesh& y)
{
    if (x.tris != y.tris || x.quads != y.quads || x.vertices.size() != y.vertices.size()) return false;
    for (size_t i = 0; i < x.vertices.size(); ++i)
        if (std::memcmp(x.vertices[i].p, y.vertices[i].p, sizeof x.vertices[i].p) != 0 || std::memcmp(x.vertices[i].n, y.vertices[i].n, sizeof x.vertices[i].n) != 0 ||
            x.vertices[i].sphere != y.vertices[i].sphere)
            return false;
    return true;
}

void test_skin_limbs()
{
    // The distance: a cone from radius 100 at the origin to 50 at 300 along x.
    SkinLimb l;
    l.ra = 100, l.b[0] = 300, l.rb = 50;
    auto swept = [&](const float p[3]) {
        float best = 1e30f;
        for (int i = 0; i <= 1000; ++i)
        {
            const float t = float(i) / 1000.0f, c = 300.0f * t, r = 100.0f + (50.0f - 100.0f) * t;
            best = std::min(best, std::sqrt((p[0] - c) * (p[0] - c) + p[1] * p[1] + p[2] * p[2]) - r);
        }
        return best;
    };
    uint64_t rng = 12345;
    auto next = [&]() {
        rng = rng * 6364136223846793005ull + 1442695040888963407ull;
        return float(int32_t(rng >> 33) % 1000) - 500.0f + 150.0f * float(rng >> 63);
    };
    float worst = 0;
    for (int i = 0; i < 2000; ++i)
    {
        const float p[3] = {next(), next(), next()};
        const float d = skin_limb_distance(l, p), want = swept(p);
        if (want > 0) worst = std::max(worst, std::fabs(d - want));
        else CHECK(d <= 0.5f);
    }
    std::printf("  a limb's distance against a thousand swept spheres: at most %.3f units apart outside it\n", double(worst));
    CHECK(worst < 0.5f);
    {
        const float behind[3] = {-150, 0, 0}, beyond[3] = {400, 0, 0}, middle[3] = {150, 0, 0};
        CHECK(std::fabs(skin_limb_distance(l, behind) - 50.0f) < 1e-3f);
        CHECK(std::fabs(skin_limb_distance(l, beyond) - 50.0f) < 1e-3f);
        CHECK(skin_limb_distance(l, middle) < -50.0f);
        // One end's sphere holding the other's: the larger sphere.
        SkinLimb in = l;
        in.b[0] = 20, in.rb = 30;
        const float p[3] = {0, 130, 0};
        CHECK(std::fabs(skin_limb_distance(in, p) - 30.0f) < 1e-3f);
    }
    // One sphere: no limbs, and the skin the same with them as without.
    Sphere a;
    a.r = 100;
    CHECK(skin_limbs({a}, 50.0f).empty());
    CHECK(same_skin(skin_mesh({a}, 10.0f, 50.0f, skin_limbs({a}, 50.0f)), skin_mesh({a}, 10.0f, 50.0f)));
    // Two spheres 400 apart (radius 100 and 60, the second the first's child): two pieces without their
    // limb; one closed piece with it, its middle as wide as the cone there.
    {
        Sphere b;
        b.parent = 0, b.x = 400, b.r = 60;
        const std::vector<Sphere> two = {a, b};
        const std::vector<SkinLimb> limbs = skin_limbs(two, 0.0f);
        CHECK(limbs.size() == 1 && limbs[0].parent == 0 && limbs[0].child == 1 && limbs[0].ra == 100.0f && limbs[0].rb == 60.0f);
        const SkinMesh apart = skin_mesh(two, 5.0f, 0.0f), joined = skin_mesh(two, 5.0f, 0.0f, limbs);
        CHECK(skin_pieces(apart) == 2);
        CHECK(skin_pieces(joined) == 1);
        CHECK(skin_closed(joined, false));
        check_skin_quads(joined, false);
        float most = 0;
        for (const SkinVertex& v : joined.vertices)
            if (std::fabs(v.p[0] - 200.0f) < 2.5f) most = std::max(most, std::sqrt(v.p[1] * v.p[1] + v.p[2] * v.p[2]));
        // (the side's radius at the middle: the tangent's slope sin = 40 / 400, so 80 / cos at the middle)
        const float want = 80.0f / std::sqrt(1.0f - 0.01f);
        std::printf("  two spheres 400 apart joined by their limb: %zu triangles, its middle %.1f units from the bone (the cone's %.1f)\n", joined.triangles(), double(most), double(want));
        CHECK(std::fabs(most - want) < 5.0f);
        // With a blend the far ends are still each its own sphere's.
        const SkinMesh soft = skin_mesh(two, 5.0f, 50.0f, skin_limbs(two, 50.0f));
        CHECK(skin_pieces(soft) == 1);
        CHECK(skin_closed(soft, false));
    }
    // Characters: off is the skin as before (seed 99 pinned at its triangles), and with limbs every
    // skin has no edge left open, every vertex's sphere one of the character's.
    struct Case
    {
        uint64_t seed;
        uint32_t limbs, joints, branches;
        Symmetry symmetry;
        bool head;
    };
    const Case cases[] = {{99, 5, 3, 1, Symmetry::Bilateral, true}, {12345, 3, 6, 0, Symmetry::Bilateral, false}, {42, 16, 12, 3, Symmetry::Bilateral, true}, {7, 8, 4, 2, Symmetry::Radial, true}};
    for (const Case& k : cases)
    {
        CharacterSettings t;
        t.seed = k.seed, t.limbs = k.limbs, t.joints = k.joints, t.branches = k.branches, t.symmetry = k.symmetry, t.head = k.head;
        const Character c = unfold_character(t);
        const SkinCell sc = skin_cell(c.spheres, voxel_cell(c), 1);
        const float blend = skin_blend(c.spheres, 50);
        const SkinMesh off = skin_mesh(c.spheres, sc.cell, blend, {});
        CHECK(same_skin(off, skin_mesh(c.spheres, sc.cell, blend)));
        const std::vector<SkinLimb> limbs = skin_limbs(c.spheres, blend);
        CHECK(limbs.size() + 1 == c.spheres.size());
        const SkinMesh on = skin_mesh(c.spheres, sc.cell, blend, limbs);
        CHECK(skin_closed(on, true));
        check_skin_quads(on, true);
        for (const SkinVertex& v : on.vertices) CHECK(v.sphere < c.spheres.size());
        std::printf("  seed %llu (%zu spheres): %zu triangles in %zu pieces without limbs, %zu in %zu with %zu limbs\n", (unsigned long long)k.seed, c.spheres.size(), off.triangles(),
                    skin_pieces(off), on.triangles(), skin_pieces(on), limbs.size());
        if (k.seed == 99) CHECK(off.triangles() == 23180);
        CHECK(skin_pieces(on) <= skin_pieces(off));
        if (k.seed == 12345) CHECK(skin_pieces(on) == 1);
        // On the world lattice (IDEAS 19.216), where seed 12345's voxels fall apart, its skin's cell the
        // lattice's halved.
        VoxelSizing lattice;
        lattice.rule = VoxelSizing::Lattice;
        const SkinCell lc = skin_cell(c.spheres, voxel_cell_of(c, lattice).cell, 1);
        const SkinMesh loff = skin_mesh(c.spheres, lc.cell, blend), lon = skin_mesh(c.spheres, lc.cell, blend, limbs);
        CHECK(skin_closed(lon, true));
        std::printf("    on the lattice (cell %.1f): %zu triangles in %zu pieces without limbs, %zu in %zu with them\n", double(lc.cell), loff.triangles(), skin_pieces(loff), lon.triangles(),
                    skin_pieces(lon));
        CHECK(skin_pieces(lon) == 1);
    }
}

// Smooth plus, the surface (IDEAS 19.229, the pipeline's second stage): Taubin's smoothing of the skin.
// No rounds is the skin exactly; with rounds only the vertices move (the same triangles and quads, the
// mesh still closed, every normal of length one and outwards on a sphere); one sphere's skin comes nearer
// its sphere and keeps its volume, where the shrink step alone loses volume; and characters of one
// sphere, many limbs and radial symmetry, with and without limbs, keep their volume to within a
// percent, measured with the time a round takes.
void test_skin_taubin()
{
    CHECK(skin_taubin(SkinMesh{}, 4).vertices.empty());
    CHECK(skin_volume(SkinMesh{}) == 0.0);
    Sphere a;
    a.r = 100;
    const SkinMesh m = skin_mesh({a}, 10.0f, 0.0f);
    CHECK(same_skin(skin_taubin(m, 0), m));
    auto spread = [](const SkinMesh& s) { // (the root mean square of each vertex's distance from radius 100)
        double sum = 0;
        for (const SkinVertex& v : s.vertices)
        {
            const double d = std::sqrt(double(v.p[0]) * v.p[0] + double(v.p[1]) * v.p[1] + double(v.p[2]) * v.p[2]) - 100.0;
            sum += d * d;
        }
        return std::sqrt(sum / double(s.vertices.size()));
    };
    const SkinMesh t = skin_taubin(m, 10);
    CHECK(t.tris == m.tris && t.quads == m.quads && t.vertices.size() == m.vertices.size());
    CHECK(skin_closed(t, false));
    check_skin_quads(t, false);
    for (size_t i = 0; i < t.vertices.size(); ++i)
    {
        const SkinVertex& v = t.vertices[i];
        CHECK(v.sphere == m.vertices[i].sphere);
        const float len = std::sqrt(v.n[0] * v.n[0] + v.n[1] * v.n[1] + v.n[2] * v.n[2]);
        CHECK(std::fabs(len - 1.0f) < 1e-4f);
        CHECK(v.p[0] * v.n[0] + v.p[1] * v.n[1] + v.p[2] * v.n[2] > 0); // (outwards)
    }
    const double ball = 4.0 / 3.0 * 3.14159265358979 * 1e6, v0 = skin_volume(m), v1 = skin_volume(t);
    const double shrunk = skin_volume(skin_taubin(m, 10, kTaubinShrink, 0.0f));
    std::printf("  one sphere's skin (%zu triangles): volume %.0f (the ball's %.0f), after 10 rounds %.0f (%.2f%%), the shrink step alone %.0f (%.2f%%); "
                "its corners' distance from the sphere %.3f units, after %.3f\n",
                m.triangles(), v0, ball, v1, 100.0 * v1 / v0, shrunk, 100.0 * shrunk / v0, spread(m), spread(t));
    CHECK(std::fabs(v0 - ball) < 0.02 * ball);
    CHECK(std::fabs(v1 - v0) < 0.01 * v0);
    CHECK(shrunk < 0.95 * v0);
    CHECK(spread(t) < spread(m));
    // Characters (one sphere, seed 99, many limbs, radial, one that stands apart), with limbs and without.
    struct Case
    {
        uint64_t seed;
        uint32_t limbs, joints, branches;
        Symmetry symmetry;
        bool head;
    };
    const Case cases[] = {{1, 0, 1, 0, Symmetry::Bilateral, false}, {99, 5, 3, 1, Symmetry::Bilateral, true}, {42, 16, 12, 3, Symmetry::Bilateral, true},
                          {7, 8, 4, 2, Symmetry::Radial, true}, {12345, 3, 6, 0, Symmetry::Bilateral, false}};
    for (const Case& k : cases)
    {
        CharacterSettings st;
        st.seed = k.seed, st.limbs = k.limbs, st.joints = k.joints, st.branches = k.branches, st.symmetry = k.symmetry, st.head = k.head;
        const Character c = unfold_character(st);
        const SkinCell sc = skin_cell(c.spheres, voxel_cell(c), 1);
        const float blend = skin_blend(c.spheres, 50);
        for (int with_limbs = 0; with_limbs < 2; ++with_limbs)
        {
            const SkinMesh s = skin_mesh(c.spheres, sc.cell, blend, with_limbs ? skin_limbs(c.spheres, blend) : std::vector<SkinLimb>{});
            const double before = skin_volume(s);
            std::printf("  seed %llu (%zu spheres%s): %zu triangles, volume %.0f;", (unsigned long long)k.seed, c.spheres.size(), with_limbs ? ", limbs" : "", s.triangles(), before);
            double ms_of[11] = {0};
            for (uint32_t rounds : {1u, 4u, 10u})
            {
                const auto t0 = std::chrono::steady_clock::now();
                const SkinMesh r = skin_taubin(s, rounds);
                const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                const double after = skin_volume(r);
                double moved = 0;
                for (size_t i = 0; i < s.vertices.size(); ++i)
                {
                    const float* p = s.vertices[i].p;
                    const float* q = r.vertices[i].p;
                    moved += std::sqrt(double(p[0] - q[0]) * (p[0] - q[0]) + double(p[1] - q[1]) * (p[1] - q[1]) + double(p[2] - q[2]) * (p[2] - q[2]));
                }
                ms_of[rounds] = ms;
                std::printf(" %u rounds in %.2f ms, %.0f (%.2f%%, corners moved %.3f cells on average);", unsigned(rounds), ms, after,
                            100.0 * after / before, moved / double(s.vertices.size()) / double(sc.cell));
                CHECK(r.tris == s.tris && r.quads == s.quads);
                CHECK(std::fabs(after - before) < 0.01 * before);
                if (rounds == 4) CHECK(skin_closed(r, true));
            }
            // (a round's own time: what nine more rounds added, the neighbours and normals made once either way)
            std::printf(" %.3f ms a round\n", (ms_of[10] - ms_of[1]) / 9.0);
        }
    }
}

// Smooth plus, the outline (IDEAS 19.229, the pipeline's third stage): Catmull and Clark's subdivision of
// the skin. No steps is the skin exactly; each step makes four quads of each (each triangle of a skin
// given as triangles alone three), the mesh still closed (every edge between two triangles, V - E + F = 2
// for one sphere), wound outwards; one sphere's outline comes nearer the sphere; a cube's corners move as
// the rule says; the bound on quads takes fewer steps; and characters of one sphere, many limbs and radial
// symmetry, with and without limbs and smoothing, stay closed, measured with the time each step takes.
void test_skin_subdivide()
{
    CHECK(skin_subdivide(SkinMesh{}, 2).vertices.empty());
    // A cube of side 2 as six quads (wound anticlockwise from outside): one step moves each corner from
    // (1, 1, 1) to (5/9, 5/9, 5/9), as the rule gives for three faces round it.
    {
        SkinMesh cube;
        const float c[8][3] = {{-1, -1, -1}, {1, -1, -1}, {1, 1, -1}, {-1, 1, -1}, {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1}};
        for (const auto& p : c)
        {
            SkinVertex v{};
            for (int k = 0; k < 3; ++k) v.p[k] = p[k];
            cube.vertices.push_back(v);
        }
        const uint32_t f[6][4] = {{0, 3, 2, 1}, {4, 5, 6, 7}, {0, 1, 5, 4}, {2, 3, 7, 6}, {1, 2, 6, 5}, {0, 4, 7, 3}};
        for (const auto& q : f)
        {
            cube.quads.insert(cube.quads.end(), {q[0], q[1], q[2], q[3]});
            cube.tris.insert(cube.tris.end(), {q[0], q[1], q[2], q[0], q[2], q[3]});
        }
        CHECK(skin_closed(cube, false));
        CHECK(same_skin(skin_subdivide(cube, 0), cube));
        const SkinMesh one = skin_subdivide(cube, 1);
        CHECK(one.quad_count() == 24 && one.triangles() == 48 && one.vertices.size() == 26);
        CHECK(skin_closed(one, false));
        check_skin_quads(one, false);
        CHECK(std::fabs(one.vertices[6].p[0] - 5.0f / 9.0f) < 1e-5f && std::fabs(one.vertices[6].p[2] - 5.0f / 9.0f) < 1e-5f);
        CHECK(skin_volume(one) > 0);
        // Given as triangles alone (no quads): the rule's general form, three quads a triangle.
        SkinMesh tri = cube;
        tri.quads.clear();
        const SkinMesh t1 = skin_subdivide(tri, 1);
        CHECK(t1.quad_count() == 36 && skin_closed(t1, false));
        CHECK(skin_subdivided_quads(tri, 1) == 36 && skin_subdivided_quads(tri, 2) == 144);
        CHECK(skin_subdivide(tri, 2).quad_count() == 144);
        CHECK(skin_volume(t1) > 0);
        // The bound: the steps whose quads stay within it.
        CHECK(skin_subdivision_steps(cube, 3, 384) == 3 && skin_subdivision_steps(cube, 3, 383) == 2 && skin_subdivision_steps(cube, 3, 23) == 0);
        CHECK(skin_subdivision_steps(cube, 0) == 0);
    }
    // One sphere of radius 100 on a 10-unit grid: rounder for the subdivision (its corners' distances from
    // the middle spread less about their mean than the skin's), and the sharpest bend between two
    // neighbouring triangles, which is what an outline shows, smaller with each step, every normal of length one and outwards, still one closed
    // piece. (Catmull and Clark's surface lies inside the faces it is made from, so the corners come a
    // little nearer the middle: the volume measures how much.)
    Sphere a;
    a.r = 100;
    const SkinMesh m = skin_mesh({a}, 10.0f, 0.0f);
    CHECK(same_skin(skin_subdivide(m, 0), m));
    auto roundness = [](const SkinMesh& s) { // (the root mean square of each corner's distance from the mean distance)
        double sum = 0, sum2 = 0;
        for (const SkinVertex& v : s.vertices)
        {
            const double d = std::sqrt(double(v.p[0]) * v.p[0] + double(v.p[1]) * v.p[1] + double(v.p[2]) * v.p[2]);
            sum += d, sum2 += d * d;
        }
        const double n = double(s.vertices.size()), mean = sum / n;
        return std::sqrt(std::max(0.0, sum2 / n - mean * mean));
    };
    auto sharpest = [](const SkinMesh& s) { // (the largest angle between two triangles that share a side, in degrees)
        std::map<std::pair<uint32_t, uint32_t>, std::vector<size_t>> sides;
        std::vector<double> n(s.triangles() * 3);
        for (size_t t = 0; t < s.triangles(); ++t)
        {
            const float* p0 = s.vertices[s.tris[t * 3]].p;
            const float* p1 = s.vertices[s.tris[t * 3 + 1]].p;
            const float* p2 = s.vertices[s.tris[t * 3 + 2]].p;
            const double e1[3] = {double(p1[0]) - p0[0], double(p1[1]) - p0[1], double(p1[2]) - p0[2]}, e2[3] = {double(p2[0]) - p0[0], double(p2[1]) - p0[1], double(p2[2]) - p0[2]};
            double g[3] = {e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0]};
            const double l = std::sqrt(g[0] * g[0] + g[1] * g[1] + g[2] * g[2]);
            for (int k = 0; k < 3; ++k) n[t * 3 + size_t(k)] = l > 0 ? g[k] / l : 0;
            for (int k = 0; k < 3; ++k)
            {
                const uint32_t i = s.tris[t * 3 + size_t(k)], j = s.tris[t * 3 + size_t((k + 1) % 3)];
                sides[{std::min(i, j), std::max(i, j)}].push_back(t);
            }
        }
        double most = 0;
        for (const auto& e : sides)
            if (e.second.size() == 2)
            {
                const double* x = &n[e.second[0] * 3];
                const double* y = &n[e.second[1] * 3];
                most = std::max(most, std::acos(std::clamp(x[0] * y[0] + x[1] * y[1] + x[2] * y[2], -1.0, 1.0)) * 180.0 / 3.14159265358979);
            }
        return most;
    };
    const double ball = 4.0 / 3.0 * 3.14159265358979 * 1e6;
    std::printf("  one sphere's skin: %zu quads, its corners' distances spread %.3f units, the sharpest bend %.1f degrees, volume %.2f%% of the ball's;", m.quad_count(), roundness(m),
                sharpest(m), 100.0 * skin_volume(m) / ball);
    double was_sharp = sharpest(m);
    for (uint32_t steps = 1; steps <= 2; ++steps)
    {
        const SkinMesh s = skin_subdivide(m, steps);
        CHECK(s.quad_count() == m.quad_count() << (2 * steps));
        CHECK(skin_closed(s, false));
        check_skin_quads(s, false);
        for (const SkinVertex& v : s.vertices)
        {
            const float len = std::sqrt(v.n[0] * v.n[0] + v.n[1] * v.n[1] + v.n[2] * v.n[2]);
            CHECK(std::fabs(len - 1.0f) < 1e-4f);
            CHECK(v.p[0] * v.n[0] + v.p[1] * v.n[1] + v.p[2] * v.n[2] > 0);
            CHECK(v.sphere == 0);
        }
        const double round = roundness(s), sharp = sharpest(s);
        std::printf(" %u %s %zu quads, %.3f units, %.1f degrees, %.2f%%;", unsigned(steps), steps == 1 ? "step" : "steps", s.quad_count(), round, sharp, 100.0 * skin_volume(s) / ball);
        CHECK(round < roundness(m));
        CHECK(sharp < was_sharp);
        CHECK(std::fabs(skin_volume(s) - ball) < 0.03 * ball);
        was_sharp = sharp;
    }
    std::printf("\n");
    // Characters (one sphere, seed 99, many limbs, radial, one that stands apart), with and without
    // limbs, with and without four rounds of smoothing before it.
    struct Case
    {
        uint64_t seed;
        uint32_t limbs, joints, branches;
        Symmetry symmetry;
        bool head;
    };
    const Case cases[] = {{1, 0, 1, 0, Symmetry::Bilateral, false}, {99, 5, 3, 1, Symmetry::Bilateral, true}, {42, 16, 12, 3, Symmetry::Bilateral, true},
                          {7, 8, 4, 2, Symmetry::Radial, true}, {12345, 3, 6, 0, Symmetry::Bilateral, false}};
    for (const Case& k : cases)
    {
        CharacterSettings st;
        st.seed = k.seed, st.limbs = k.limbs, st.joints = k.joints, st.branches = k.branches, st.symmetry = k.symmetry, st.head = k.head;
        const Character c = unfold_character(st);
        const SkinCell sc = skin_cell(c.spheres, voxel_cell(c), 1);
        const float blend = skin_blend(c.spheres, 50);
        for (int with_limbs = 0; with_limbs < 2; ++with_limbs)
            for (uint32_t rounds : {0u, 4u})
            {
                const SkinMesh s0 = skin_mesh(c.spheres, sc.cell, blend, with_limbs ? skin_limbs(c.spheres, blend) : std::vector<SkinLimb>{});
                const SkinMesh s = skin_taubin(s0, rounds);
                CHECK(same_skin(skin_subdivide(s, 0), s));
                const double before = skin_volume(s);
                std::printf("  seed %llu (%zu spheres%s, %u rounds): %zu quads;", (unsigned long long)k.seed, c.spheres.size(), with_limbs ? ", limbs" : "", unsigned(rounds), s.quad_count());
                for (uint32_t steps = 1; steps <= 2; ++steps)
                {
                    const auto t0 = std::chrono::steady_clock::now();
                    const SkinMesh r = skin_subdivide(s, steps);
                    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
                    CHECK(r.quad_count() == s.quad_count() << (2 * steps));
                    CHECK(r.quad_count() == skin_subdivided_quads(s, steps));
                    CHECK(skin_closed(r, true));
                    for (const SkinVertex& v : r.vertices) CHECK(v.sphere < c.spheres.size());
                    const double after = skin_volume(r);
                    CHECK(after > 0.9 * before && after < 1.01 * before);
                    std::printf(" %u %s %zu quads in %.2f ms (volume %.2f%%);", unsigned(steps), steps == 1 ? "step" : "steps", r.quad_count(), ms, 100.0 * after / before);
                }
                std::printf(" at most %u of 3 steps within %zu quads\n", unsigned(skin_subdivision_steps(s, 3)), kSkinMostSubdividedQuads);
            }
    }
}

// Editing a character by its joints (IDEAS 19.203, sieve/character_edit.hpp): a seed-made character kept
// as its spheres with its symmetry (sieve-character-v4), its mirrored spheres found and kept mirrored
// through every edit, bones keeping their lengths, and each edit undone by its opposite to the same text.
void test_character_edit()
{
    CharacterSettings st;
    st.seed = 99;
    st.limbs = 5;
    st.joints = 3;
    st.branches = 1;
    const Character seed = unfold_character(st);
    const std::string seed_text = seed.text();
    const Character e = editable_character(seed);
    CHECK(seed.text() == seed_text); // (the seed-made character itself is not changed)
    CHECK(e.given && e.given_symmetry == Symmetry::Bilateral && e.text().rfind("sieve-character-v4 spheres=" + std::to_string(seed.spheres.size()) + " symmetry=bilateral\n", 0) == 0);
    CHECK(read_character(e.text()).id() == e.id() && read_character(e.one_line()).id() == e.id());
    // Every sphere off the centre plane is paired with its mirror.
    auto mirrored = [](const Character& c) {
        size_t pairs = 0;
        for (size_t i = 0; i < c.spheres.size(); ++i)
        {
            if (c.images[i].of < 0) continue;
            const Sphere &a = c.spheres[i], &b = c.spheres[size_t(c.images[i].of)];
            if (a.x != -b.x || a.y != b.y || a.z != b.z || a.r != b.r) return size_t(0);
            ++pairs;
        }
        return pairs;
    };
    size_t off_centre = 0;
    for (const Sphere& p : seed.spheres) off_centre += p.x != 0 ? 1 : 0;
    CHECK(mirrored(e) * 2 == off_centre && mirrored(e) > 0);
    // A limb's first joint dragged: its mirror follows, both bones keep their lengths, the limbs beyond move.
    uint32_t root = 0;
    for (uint32_t i = 1; i < e.spheres.size(); ++i)
        if (e.spheres[i].parent == 0 && e.spheres[i].x != 0 && e.images[i].of < 0) { root = i; break; }
    CHECK(root > 0);
    Character moved = e;
    const double to[3] = {double(e.spheres[root].x) * 2, double(e.spheres[root].y) - 500, double(e.spheres[root].z) + 300};
    CHECK(edit_move(moved, root, to, false) == root);
    CHECK(moved.text() != e.text() && mirrored(moved) == mirrored(e) && moved.spheres.size() == e.spheres.size());
    auto bone = [](const Character& c, size_t i) {
        const Sphere &a = c.spheres[i], &p = c.spheres[size_t(a.parent)];
        return std::sqrt(double(a.x - p.x) * (a.x - p.x) + double(a.y - p.y) * (a.y - p.y) + double(a.z - p.z) * (a.z - p.z));
    };
    for (size_t i = 1; i < e.spheres.size(); ++i) CHECK(std::fabs(bone(moved, i) - bone(e, i)) < 2.0);
    // Stretched, the joint goes exactly where it is put.
    Character stretched = e;
    edit_move(stretched, root, to, true);
    CHECK(stretched.spheres[root].x == int32_t(to[0]) && stretched.spheres[root].y == int32_t(to[1]) && mirrored(stretched) == mirrored(e));
    // The body is never moved or taken away.
    Character body = e;
    CHECK(throws([&] { edit_move(body, 0, to, false); }) && throws([&] { edit_remove_limb(body, 0); }) && body.text() == e.text());
    // A limb grown from the body: on the centre plane, one; off it, a pair. Taken away (with its mirror),
    // the character is as it was.
    Character grown = e;
    const uint32_t g = edit_grow(grown, 0);
    const bool centre = grown.spheres[g].x == 0;
    CHECK(grown.spheres.size() == e.spheres.size() + (centre ? 1 : 2) && mirrored(grown) == mirrored(e) + (centre ? 0 : 1) && grown.spheres[g].parent == 0);
    // From a limb's end, the new joints come in a pair, mirrored.
    Character end = e;
    uint32_t tip = root;
    while (true)
    {
        uint32_t next = tip;
        for (uint32_t i = tip + 1; i < e.spheres.size(); ++i)
            if (e.spheres[i].parent == int32_t(tip)) { next = i; break; }
        if (next == tip) break;
        tip = next;
    }
    edit_grow(end, tip);
    CHECK(end.spheres.size() == e.spheres.size() + 2 && mirrored(end) == mirrored(e) + 1);
    edit_remove_limb(grown, g);
    CHECK(grown.text() == e.text());
    // A joint added along a limb and taken away again: the same character.
    Character jointed = e;
    const uint32_t j = edit_add_joint(jointed, root);
    CHECK(jointed.spheres.size() == e.spheres.size() + 2 && jointed.spheres[j].parent == int32_t(root) && mirrored(jointed) == mirrored(e) + 1);
    CHECK(edit_remove_joint(jointed, j) == root && jointed.text() == e.text());
    // A whole limb taken away: it and all beyond it, on both sides.
    Character cut = e;
    edit_remove_limb(cut, root);
    CHECK(cut.spheres.size() + 2 * (size_t(st.joints) + st.branches * ((st.joints + 1) / 2)) == e.spheres.size());
    CHECK(read_character(cut.text()).id() == cut.id());
    // Radial: each limb's spheres the first limb's turned; a limb grown from the body grows on every turn.
    st.symmetry = Symmetry::Radial;
    st.limbs = 4;
    const Character r = editable_character(unfold_character(st));
    size_t turned = 0;
    for (const SphereImage& m : r.images) turned += m.of >= 0 ? 1 : 0;
    CHECK(turned > 0 && read_character(r.text()).id() == r.id());
    Character rg = r;
    edit_grow(rg, 0);
    CHECK(rg.spheres.size() == r.spheres.size() + 4);
    // No symmetry: kept as v3, its spheres given.
    st.symmetry = Symmetry::None;
    Character n = unfold_character(st);
    edit_add_joint(n, uint32_t(n.spheres.size() - 1));
    CHECK(n.text().rfind("sieve-character-v3 ", 0) == 0 && n.spheres.size() == unfold_character(st).spheres.size() + 1);
    // Refused: a v4 of no symmetry, an image of a later sphere, a bilateral image turned.
    CHECK(throws([] { (void)read_character("sieve-character-v4 spheres=1 symmetry=none\ns 0 -1 0 0 0 5 body\n"); }));
    CHECK(throws([] { (void)read_character("sieve-character-v4 spheres=3 symmetry=bilateral\ns 0 -1 0 0 0 5 body\ns 1 0 9 0 0 2 joint\ns 2 0 -9 0 0 2 joint\ni 1 2 1 0\n"); }));
    CHECK(throws([] { (void)read_character("sieve-character-v4 spheres=3 symmetry=bilateral\ns 0 -1 0 0 0 5 body\ns 1 0 9 0 0 2 joint\ns 2 0 -9 0 0 2 joint\ni 2 1 1 2\n"); }));
    CHECK(read_character("sieve-character-v4 spheres=3 symmetry=bilateral\ns 0 -1 0 0 0 5 body\ns 1 0 9 0 0 2 joint\ns 2 0 -9 0 0 2 joint\ni 2 1 1 0\n").images[2].of == 1);
    std::printf("  an edited character: %zu spheres, %zu mirrored pairs found; %s\n", e.spheres.size(), mirrored(e), e.id().substr(0, 16).c_str());
}

// The one footer's layout (IDEAS 19.224, sieve/footer_wrap.hpp): a footer that fits comes out exactly as
// written, one too wide is broken only between keys with each line within the room, the last lines last
// (the screen draws them upwards from its foot), an entry too long alone broken between words, a word too
// long between characters (never inside one), and the room measured by the caller's own widths.
void test_footer_wrap()
{
    using sieve::wrap_footer;
    const std::string keys = "Up/Down choose   Enter select   Left/Right change   Esc back   (the mouse works too)";
    CHECK(wrap_footer(keys, 200) == std::vector<std::string>{keys});
    CHECK(wrap_footer(keys, keys.size()) == std::vector<std::string>{keys});
    const auto two = wrap_footer(keys, 60);
    CHECK(two.size() == 2 && two[0] == "Up/Down choose   Enter select   Left/Right change   Esc back" && two[1] == "(the mouse works too)");
    for (size_t room : {10, 14, 20, 33, 40, 61})
    {
        const auto lines = wrap_footer(keys, room);
        std::string joined;
        for (const std::string& l : lines)
        {
            CHECK(l.size() <= room);
            joined += (joined.empty() ? "" : " ") + l;
        }
        // Nothing lost or added but spacing: every word of the footer is there, in order.
        std::string words, w2;
        for (char c : keys) words += c == ' ' && !words.empty() && words.back() == ' ' ? std::string() : std::string(1, c);
        for (char c : joined) w2 += c == ' ' && !w2.empty() && w2.back() == ' ' ? std::string() : std::string(1, c);
        CHECK(words == w2);
    }
    // At a room that holds each key, no key is parted between lines.
    for (const std::string& l : wrap_footer(keys, 30))
        CHECK(l == "Up/Down choose   Enter select" || l == "Left/Right change   Esc back" || l == "(the mouse works too)");
    // Two spaces part entries as three do (the hallway's own footer is written so).
    CHECK((wrap_footer("W move  Shift run  [ ] jump", 20) == std::vector<std::string>{"W move  Shift run", "[ ] jump"}));
    // Gaps are kept as written (four spaces stay four), and spaces at the ends dropped.
    CHECK(wrap_footer("  A: one    B: two  ", 80) == std::vector<std::string>{"A: one    B: two"});
    CHECK(wrap_footer("", 10) == std::vector<std::string>{""});
    // An entry too long alone: between words, then a word too long between characters.
    CHECK((wrap_footer("one two three", 7) == std::vector<std::string>{"one two", "three"}));
    CHECK((wrap_footer("abcdefghij", 4) == std::vector<std::string>{"abcd", "efgh", "ij"}));
    // Measured in the caller's units: here two units a character, and a two-byte character one unit.
    const auto halves = wrap_footer("A: one   B: two", 12.0, [](const std::string& t) { return 2 * t.size(); });
    CHECK((halves == std::vector<std::string>{"A: one", "B: two"}));
    const auto cells = [](const std::string& t) {
        size_t n = 0;
        for (unsigned char c : t) n += (c & 0xC0) != 0x80;
        return n;
    };
    const auto acc = wrap_footer("\xc3\xa9\xc3\xa9\xc3\xa9", 2.0, cells); // three e-acutes, two to a line
    CHECK((acc == std::vector<std::string>{"\xc3\xa9\xc3\xa9", "\xc3\xa9"}));
}

// The key registry (IDEAS 19.199, sieve/key_registry.hpp): chords read and named, actions declared and
// asked, keys set and kept as overrides, clashes found, and footers' keys filled in. Without a naming a
// single character names its key; a few of SDL's names are given here as the hallway gives SDL's.
void test_key_registry()
{
    CHECK(parse_chord("E").key == 'e' && parse_chord("E").mod == 0 && chord_name(parse_chord("E")) == "E");
    CHECK(parse_chord("  e ").key == 'e');
    CHECK(parse_chord("Shift+R").mod == kKeyShift && chord_name(parse_chord("Shift+R")) == "Shift+R");
    CHECK(chord_name(parse_chord("Shift+Alt+Ctrl+X")) == "Ctrl+Alt+Shift+X"); // (always written in one order)
    CHECK(parse_chord("Comma").key == ',' && chord_name(parse_chord("Comma")) == "Comma");
    CHECK(parse_chord("Plus").key == '+' && chord_name(parse_chord("Plus")) == "+");
    CHECK(parse_chord("Return").key == 0 && parse_chord("").key == 0 && parse_chord("Shift+").key == 0);
    CHECK(plain_mods(0x0001) == kKeyShift && plain_mods(0x0080) == kKeyCtrl && plain_mods(0x0200 | 0x1000) == kKeyAlt);

    const uint32_t kReturn = 13, kUp = 0x40000052, kLShift = 0x400000E1, kKpPlus = 0x40000057;
    set_key_naming({[&](const std::string& n) -> uint32_t {
                        if (n == "Return") return kReturn;
                        if (n == "Up") return kUp;
                        if (n == "Left Shift") return kLShift;
                        if (n == "Keypad +") return kKpPlus;
                        return n.size() == 1 ? uint32_t(std::tolower(static_cast<unsigned char>(n[0]))) : 0;
                    },
                    [&](uint32_t c) -> std::string {
                        if (c == kReturn) return "Return";
                        if (c == kUp) return "Up";
                        if (c == kLShift) return "Left Shift";
                        if (c == kKpPlus) return "Keypad +";
                        return c > ' ' && c <= '~' ? std::string(1, char(std::toupper(int(c)))) : std::string();
                    }});
    CHECK(parse_chords("W, ,Up,Bogus,Comma,Keypad +").size() == 4);
    CHECK(chord_name(parse_chord("Ctrl+Return")) == "Ctrl+Return");

    KeyRegistry k;
    const std::string& fwd = k.declare({"walk", "forward", "keys.walk.forward", "W,Up"});
    CHECK(fwd == "walk/forward");
    CHECK(&k.declare({"walk", "forward", "other", "Q"}) == &fwd && k.names(fwd) == "W / Up"); // (a second declaration is ignored)
    const std::string& zoom = k.declare({"view", "zoom_out", "keys.view.zoom_out", "-"});
    const std::string& far = k.declare({"view", "zoom_out_far", "keys.view.zoom_out_far", "Shift+-"});
    const std::string& go = k.declare({"view", "go", "keys.view.go", "Return,Ctrl+G"});
    CHECK(k.is(fwd, 'w', 0) && k.is(fwd, kUp, 0) && !k.is(fwd, 'w', 0x0001) && !k.is(fwd, 's', 0) && !k.is("no/such", 'w', 0));
    CHECK(k.within(fwd, 'w', 0x0001) && !k.within(far, '-', 0) && k.within(far, '-', 0x0002 | 0x0040));
    CHECK(k.is(go, 'g', 0x0040) && !k.is(go, 'g', 0) && k.within(go, kReturn, 0x0100));
    CHECK(k.which({zoom, far}, '-', 0) == zoom && k.which({zoom, far}, '-', 0x0001) == far && k.which({zoom, far}, '-', 0x0001 | 0x0040) == far &&
          k.which({zoom, far}, '-', 0x0040) == zoom && k.which({zoom, far}, '=', 0).empty());
    CHECK(k.first(fwd) == "W" && k.names(go) == "Return / Ctrl+G" && k.names("no/such") == "?" && k.first("no/such") == "-");
    CHECK(k.chords(fwd).size() == 2 && k.chords(fwd)[1].key == kUp && k.chords("no/such").empty());

    // A screen's keys asked together (the setup menu's right-hand panel): a chord with Ctrl is the tab before
    // rather than Left, while Shift held with Left is still Left (ten steps), as the menus always took it.
    {
        KeyRegistry s;
        const std::string& left = s.declare({"menus", "left", "keys.menus.left", "A"});
        const std::string& tab = s.declare({"side", "previous_tab", "keys.side.previous_tab", "Ctrl+A"});
        CHECK(s.which({tab, left}, 'a', 0) == left && s.which({tab, left}, 'a', 0x0001) == left && s.which({tab, left}, 'a', 0x0040) == tab &&
              s.which({tab, left}, 'a', 0x0040 | 0x0001) == tab && s.which({left}, 'a', 0x0040) == left);
        s.together("menus", "side");
        CHECK(s.clashes().empty()); // (Ctrl+A is not A)
    }

    // Footers' keys: as written while the defaults stand, as set once changed.
    const std::string footer = "{key1:walk/forward|W} walk  {key:walk/forward|W/Up}  {key:no/such|N} x  {keyboard}  {key:open";
    CHECK(k.keyed(footer) == "W walk  W/Up  N x  {keyboard}  {key:open");
    CHECK(!k.changed(fwd) && k.overrides().empty());
    k.set(fwd, "I,Up");
    CHECK(k.changed(fwd) && k.is(fwd, 'i', 0) && !k.is(fwd, 'w', 0) && k.names(fwd) == "I / Up");
    CHECK(k.keyed(footer) == "I walk  I / Up  N x  {keyboard}  {key:open");
    CHECK(k.overrides().size() == 1 && k.overrides().at("walk/forward") == "I,Up");
    k.set(fwd, "Bogus"); // (no key in it: the default again, as an empty one is)
    CHECK(!k.changed(fwd) && k.names(fwd) == "W / Up");
    k.set(fwd, "I");
    k.set(fwd, "");
    CHECK(!k.changed(fwd) && k.overrides().empty());

    // Set before its system declares it (the settings file is read first): kept, then taken at declaring.
    k.set("late/act", "L");
    CHECK(k.overrides().size() == 1 && k.overrides().at("late/act") == "L" && !k.is("late/act", 'l', 0));
    const std::string& late = k.declare({"late", "act", "keys.late.act", "K"});
    CHECK(k.is(late, 'l', 0) && k.changed(late) && k.overrides().size() == 1);
    const auto rows = k.listing();
    CHECK(rows.size() == 5 && rows[0].full_id == "walk/forward" && rows[0].section == "walk" && rows[0].name_key == "keys.walk.forward" && rows[0].keys == "W / Up" &&
          !rows[0].changed && rows[4].full_id == "late/act" && rows[4].changed && rows[4].keys == "L");

    // Clashes: within one section, and between two sections only once they are said to be read together.
    CHECK(k.clashes().empty());
    k.set(far, "-");
    auto c = k.clashes();
    CHECK(c.size() == 1 && c[0].chord == "-" && c[0].a == "view/zoom_out" && c[0].b == "view/zoom_out_far");
    k.set(far, "");
    k.set(late, "W");
    CHECK(k.clashes().empty());
    k.together("walk", "late");
    c = k.clashes();
    CHECK(c.size() == 1 && c[0].chord == "W" && c[0].a == "walk/forward" && c[0].b == "late/act");
    set_key_naming({}); // (the plain naming again, for the tests after)
    CHECK(parse_chord("Return").key == 0 && parse_chord("q").key == 'q');
}

// Favourites' files (IDEAS 19.190, sieve/favourites.hpp): a favourite written and read back, named by its
// SHA-256, one changed by hand refused, and a dimension's list kept in order, each once.
void test_favourites()
{
    namespace fs = std::filesystem;
    const fs::path base = fs::temp_directory_path() / ("sieve-favourites-test-" + std::to_string(std::random_device{}()));
    std::error_code ec;
    fs::remove_all(base, ec);
    CHECK(favourite_sha("titled-v1", {1, 2, 3}) == Sha256::hex(Sha256::hash(std::string("titled-v1\n1,2,3"))));
    CHECK(favourite_sha("titled-v1", {}, "1f") == Sha256::hex(Sha256::hash(std::string("titled-v1\n\n1f"))));
    CHECK(favourites_dir(base) == base / "favourites");
    CHECK(favourites_list_of(fs::path("worlds") / "personal.world") == fs::path("worlds") / "personal.favourites");

    Favourite f;
    f.line = "pages";
    f.space = "titled-v1";
    f.title = "100% a title\nover two lines\r";
    f.digits = {1, 2, 3};
    f.sha = favourite_sha(f.space, f.digits);
    std::string why;
    CHECK(save_favourite(base, f, why) && why.empty());
    std::string text;
    {
        std::ifstream in(favourites_dir(base) / (f.sha + ".item"), std::ios::binary);
        text.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    CHECK(text == "sieve-favourite-v1\nline pages\nspace titled-v1\ntitle 100%25 a title%0Aover two lines%0D\ndigits 1,2,3\n");
    const auto back = read_favourite(base, f.sha);
    CHECK(back && back->sha == f.sha && back->line == "pages" && back->space == "titled-v1" && back->title == f.title && back->digits == f.digits && back->index.empty());
    CHECK(!read_favourite(base, std::string(64, '0')));

    // One kept by its place (a model, a world, a book): no digits, its address in hex.
    Favourite m;
    m.line = "models";
    m.space = "titled-v1";
    m.title = "a model";
    m.index = "1f";
    m.sha = favourite_sha(m.space, {}, m.index);
    CHECK(m.sha != favourite_sha(m.space, {}) && save_favourite(base, m, why));
    const auto mb = read_favourite(base, m.sha);
    CHECK(mb && mb->digits.empty() && mb->index == "1f" && mb->line == "models");

    // Changed by hand, so it is no longer the favourite its name says; or not a favourite at all.
    std::ofstream(favourites_dir(base) / (f.sha + ".item"), std::ios::binary) << "sieve-favourite-v1\nline pages\nspace titled-v1\ntitle x\ndigits 1,2,4\n";
    CHECK(!read_favourite(base, f.sha));
    std::ofstream(favourites_dir(base) / (f.sha + ".item"), std::ios::binary) << "sieve-favourite-v1\nline pages\nspace titled-v1\ntitle x\ndigits 1,x,3\n";
    CHECK(!read_favourite(base, f.sha));
    std::ofstream(favourites_dir(base) / (f.sha + ".item"), std::ios::binary) << "sieve-favourite-v2\nline pages\nspace titled-v1\ndigits 1,2,3\n";
    CHECK(!read_favourite(base, f.sha));
    std::ofstream(favourites_dir(base) / (f.sha + ".item"), std::ios::binary) << "sieve-favourite-v1\r\nline pages\r\nspace titled-v1\r\ntitle x\r\ndigits 1,2,3\r\n";
    CHECK(read_favourite(base, f.sha) && read_favourite(base, f.sha)->title == "x"); // (CR LF read as LF)

    // A dimension's list: in the order kept, each once; a line not a SHA-256 is passed over.
    const fs::path list = favourites_list_of(base / "worlds" / "personal.world");
    CHECK(dimension_favourites(list).empty());
    CHECK(add_to_dimension(list, f.sha, why) && !add_to_dimension(list, f.sha, why) && add_to_dimension(list, m.sha, why));
    CHECK(dimension_favourites(list) == std::vector<std::string>({f.sha, m.sha}));
    CHECK(remove_from_dimension(list, f.sha) && !remove_from_dimension(list, f.sha));
    CHECK(dimension_favourites(list) == std::vector<std::string>({m.sha}));
    std::ofstream(list, std::ios::binary | std::ios::app) << "not a sha\n" << std::string(64, 'A') << "\n";
    CHECK(dimension_favourites(list) == std::vector<std::string>({m.sha}));
    std::ofstream(list, std::ios::binary) << "sieve-favourites-v2\n" << m.sha << "\n";
    CHECK(dimension_favourites(list).empty());
    fs::remove_all(base, ec);
}

void run_all(int argc, char** argv)
{
    PART("spaces");
    RUN(test_sha256());
    RUN(test_utf8());
    RUN(test_biguint());
    RUN(test_space_basics());
    RUN(test_range_stack());
    RUN(test_scramble_bijection());
    RUN(test_neighbour());
    RUN(test_fraction());
    RUN(test_corridor());
    RUN(test_canon());
    RUN(test_sieve());
    RUN(test_image());
    RUN(test_audio());
    RUN(test_midi_read());
    PART("characters");
    RUN(test_character());
    RUN(test_character_voxels());
    RUN(test_voxel_lattice_and_joining());
    RUN(test_character_skin());
    RUN(test_skin_quads_and_smooth());
    RUN(test_skin_limbs());
    RUN(test_skin_taubin());
    RUN(test_skin_subdivide());
    RUN(test_character_edit());
    PART("hallway");
    RUN(test_key_registry());
    RUN(test_footer_wrap());
    RUN(test_favourites());
    RUN(test_raid());
    RUN(test_raid_net());
    RUN(test_raid_pieces());
    PART("fractals");
    RUN(test_fractals());
    RUN(test_fractal_count());
    PART("hallway");
    RUN(test_format_stats());
    RUN(test_catalogue());
    RUN(test_save());
    RUN(test_actions());
    PART("models");
    RUN(test_model());
    RUN(test_guided());
    RUN(test_modelspace());
    if (argc > 1)
    {
        const std::string dir = std::string(argv[1]) + "/";
        PART("vectors");
        RUN(test_vectors(dir + "vectors_v1.tsv"));
        RUN(test_space_range(dir));
        RUN(test_biguint_vectors(dir + "vectors_biguint_v1.tsv"));
        RUN(test_biguint_large_vectors(dir + "vectors_biguint_large_v1.tsv"));
        RUN(test_digit_vectors(dir + "vectors_digits_v1.tsv"));
        RUN(test_canon_vectors(dir + "vectors_canon.tsv"));
        RUN(test_canon_range_vectors(dir + "vectors_canon_range_v1.tsv"));
        RUN(test_bytes_vectors(dir + "vectors_bytes_v1.tsv"));
        RUN(test_alphabet_vectors(dir + "vectors_alphabets_v1.tsv"));
        RUN(test_model_vectors(dir + "vectors_models_v1.tsv"));
        RUN(test_image_vectors(dir + "vectors_image_v1.tsv"));
        RUN(test_image_vectors(dir + "vectors_image_v2.tsv", ImageFormat::Fit::Levels, 32));
        PART("models");
        RUN(test_system_prompt(dir + "../data/prompts/sieve-system-v1.txt"));
        PART("text-sound");
        RUN(test_cipher_vectors(dir + "vectors_cipher_v1.tsv"));
        PART("models");
        RUN(test_guided_vectors(dir));
        RUN(test_guided_range(dir));
        PART("filters");
        RUN(test_filters(dir));
        RUN(test_z_filters());
        RUN(test_p_filters());
        RUN(test_q_filters());
        RUN(test_m2_filters(dir));
        RUN(test_degree_window(dir));
        RUN(test_place_window());
        RUN(test_join_filters());
        RUN(test_mesh_filters());
        RUN(test_video_rankers(dir));
        RUN(test_merge_keys(dir));
        RUN(test_rows_alike(dir));
        RUN(test_quotes(dir));
        RUN(test_filter_vectors(dir));
        RUN(test_compact(dir));
        RUN(test_compact_vectors(dir));
        PART("books");
        RUN(test_bookspace());
        RUN(test_booksieve());
        RUN(test_review_additions());
        RUN(test_book_vectors(dir));
        RUN(test_composition_vectors(dir));
        RUN(test_composition_sieve());
        RUN(test_world_vectors(dir));
        RUN(test_world_sieve());
        PART("models");
        RUN(test_json());
        RUN(test_safetensors());
        PART("vectors");
        RUN(test_jpeg_eighth(dir));
        PART("models");
        RUN(test_weights());
        RUN(test_llm(dir));
        RUN(test_ai_vectors(dir));
        RUN(test_ai_filters());
        RUN(test_ai_filter_vectors(dir));
        RUN(test_ai_run_filters(dir));
        RUN(test_training(dir));
        RUN(test_diffusion_vectors(dir));
        PART("text-sound");
        RUN(test_titled_vectors(dir + "vectors_titled_v1.tsv"));
        RUN(test_binary_vectors(dir + "vectors_binary_v1.tsv"));
        RUN(test_chunk_vectors(dir + "vectors_chunks_v1.tsv"));
        RUN(test_notes2_vectors(dir + "vectors_notes2_v1.tsv"));
        RUN(test_pcm_vectors(dir + "vectors_pcm_v1.tsv"));
        RUN(test_pcm_digits());
        RUN(test_notes3_vectors(dir + "vectors_notes3_v1.tsv"));
        RUN(test_sound_vectors(dir));
        RUN(test_notes2(dir));
        PART("filters");
        RUN(test_plugins(dir));
        RUN(test_book_filter_vectors(dir));
        PART("filters");
        RUN(test_kinds());
        RUN(test_kind_vectors(dir + "vectors_kinds_v1.tsv"));
        RUN(test_written_vectors(dir));
        PART("cross");
        RUN(test_cross_vectors(dir));
        PART("filters");
        RUN(test_picture_vectors(dir));
        RUN(test_figure_vectors(dir));
        PART("text-sound");
        RUN(test_utf8_vectors(dir));
        RUN(test_utf8_joint());
        RUN(test_shortest_path());
        PART("filters");
        RUN(test_filter_memory(dir));
        PART("text-sound");
        RUN(test_byte_conversions());
        RUN(test_digit_histogram());
        PART("mixing");
        RUN(test_rope_wave());
        RUN(test_mix());
        RUN(test_mix2());
        RUN(test_mix3());
        RUN(test_mix4());
        RUN(test_mix4_run());
    }
}

} // namespace

int main(int argc, char** argv)
{
    if (argc < 2) { std::cerr << "usage: sieve_tests <tests directory> [--part NAME[,NAME...]] [--only NAME[,NAME...]] [--list] [--times]\n"; return 1; }
    g_data = std::string(argv[1]) + "/../data/";
    for (int i = 2; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--list") g_list = true;
        else if (a == "--times") g_times = true;
        else if (a == "--part" && i + 1 < argc)
        {
            std::string list = argv[++i];
            for (size_t at = 0; at <= list.size();)
            {
                const size_t e = std::min(list.find(',', at), list.size());
                const std::string name = list.substr(at, e - at);
                if (std::find(std::begin(kParts), std::end(kParts), name) == std::end(kParts))
                {
                    std::cerr << "sieve_tests: no part " << name << " (";
                    for (const char* k : kParts) std::cerr << k << (k == kParts[std::size(kParts) - 1] ? ")\n" : ", ");
                    return 1;
                }
                g_parts.push_back(name);
                at = e + 1;
            }
        }
        else if (a == "--only" && i + 1 < argc)
        {
            std::string list = argv[++i];
            for (size_t at = 0; at <= list.size();)
            {
                const size_t e = std::min(list.find(',', at), list.size());
                if (e > at) g_only.push_back(list.substr(at, e - at));
                at = e + 1;
            }
        }
        else { std::cerr << "sieve_tests: unknown option " << a << "\n"; return 1; }
    }
    if (g_list)
    {
        run_all(argc, argv);
        return 0;
    }

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
