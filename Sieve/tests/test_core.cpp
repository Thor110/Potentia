// Sieve core tests. Usage: sieve_tests <tests directory containing the vectors_*.tsv files>

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/biguint.hpp"
#include "sieve/canon.hpp"
#include "sieve/image.hpp"
#include "sieve/sha256.hpp"
#include "sieve/sieve.hpp"
#include "sieve/space.hpp"
#include "sieve/utf8.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
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

void test_door_map()
{
    const Space text(alphabet_by_id("lower27"), 3), same("other27", 27, 3);
    const Space bits("image/mono/4x4", 2, 16); // 65536 units
    for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
    {
        // Same size: a bijection that keeps the address.
        const auto u = text.digits_of(U"abc");
        const auto v = door_map(text, u, m, same);
        CHECK(same.address_of(v, m) == text.address_of(u, m));
        CHECK(door_map(same, v, m, text) == u);
    }
    // Different sizes: floor(a * 65536 / 19683), checked with plain integers.
    for (uint64_t a : {0ull, 1ull, 786ull, 9841ull, 19682ull})
    {
        const auto u = text.unit_at(BigUint(a).to_hex(4), AddressMode::Positional);
        const auto v = door_map(text, u, AddressMode::Positional, bits);
        CHECK(std::stoull(BigUint::from_hex(bits.address_of(v, AddressMode::Positional)).to_decimal()) == a * 65536 / 19683);
    }
    // Paragraph-scale text to a full-colour image and back lands within one text unit's span.
    const Space para(alphabet_by_id("lower27"), 1000), img("image/rgb24/10x10", 16777216u, 100);
    const auto u = para.digits_of(std::u32string(500, U'm') + std::u32string(500, U'q'));
    const auto there = door_map(para, u, AddressMode::Positional, img);
    const auto back = door_map(img, there, AddressMode::Positional, para);
    CHECK(para.fraction(back, AddressMode::Positional) <= para.fraction(u, AddressMode::Positional));
    CHECK(std::abs(para.fraction(back, AddressMode::Positional) - para.fraction(u, AddressMode::Positional)) < 1e-12);

    CHECK(text.address_of(unit_at_fraction(text, 5, 1, AddressMode::Positional), AddressMode::Positional) ==
          BigUint(19683 / 2).to_hex(4));                                                    // 50%
    CHECK(text.address_of(unit_at_fraction(text, 0, 0, AddressMode::Positional), AddressMode::Positional) == "0000");
    CHECK(throws([&] { unit_at_fraction(text, 10, 1, AddressMode::Positional); }));          // 100% is not on the line
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

} // namespace

int main(int argc, char** argv)
{
    test_sha256();
    test_utf8();
    test_biguint();
    test_space_basics();
    test_scramble_bijection();
    test_neighbour();
    test_door_map();
    test_canon();
    test_sieve();
    test_image();
    test_audio();
    if (argc > 1)
    {
        const std::string dir = std::string(argv[1]) + "/";
        test_vectors(dir + "vectors_v1.tsv");
        test_digit_vectors(dir + "vectors_digits_v1.tsv");
        test_canon_vectors(dir + "vectors_canon.tsv");
        test_image_vectors(dir + "vectors_image_v1.tsv");
    }
    else { std::cerr << "usage: sieve_tests <tests directory>\n"; ++g_failures; }

    std::cout << g_checks << " checks, " << g_failures << " failures\n";
    return g_failures ? 1 : 0;
}
