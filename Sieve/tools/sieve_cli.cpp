// Sieve — command-line interface to the core.
//
//   sieve info   [--line LINE] [line options] [--key K]
//   sieve warp   [--line LINE] [line options] [--key K] [--mode MODE] [--short] (TEXT... | --file PATH)
//   sieve read   [--line LINE] [line options] [--key K] --mode MODE [--out PATH] [--scale S] ADDRESS
//   sieve browse [--line LINE] [line options] [--key K] [--count N] [--short]
//   sieve read   ... ADDRESS --around N      (the N units either side)
//   sieve dicts  [--hash FILE]
//   sieve alphabets [--spec SPEC]
//   sieve mesh   [--vertices V] [--faces F] [--coords C] [--warp FILE | --read ADDR | --browse N]
//   sieve version
//   sieve models
//   sieve filters [--line LINE] [line options] [--filters INI]
//   sieve check   [--line LINE] [line options] [--filters INI] (TEXT... | --file PATH)
//   sieve train   --out FILE [--corpus MANIFEST] [--texts DIR] [--alphabet A] [--order K] [--min-count M]
//   sieve measure [--model ID|PATH] [--length L] [FILE... | --corpus MANIFEST --texts DIR [--role test|train|all]]
//   sieve sift  [--dict ID|PATH] [--lengths SPEC] [--brute-max N] [--pruned-max N] [--threads T] [--csv PATH]
//
// Run "sieve help", "sieve help <command>" or "sieve help lines" for explanations and examples.

#include "cli/args.hpp"
#include "cli/book.hpp"
#include "cli/dictionaries.hpp"
#include "cli/help.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/models.hpp"

#include "sieve/booksieve.hpp"
#include "sieve/audio.hpp"
#include "sieve/compact.hpp"
#include "sieve/corridor.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/sha256.hpp"
#include "sieve/sieve.hpp"
#include "sieve/utf8.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>


using namespace sieve;
using namespace sieve::cli;

namespace {

namespace fs = std::filesystem;

// ---------------------------------------------------------------- shared output

std::string show_address(const std::string& hex, bool abbreviate)
{
    if (!abbreviate || hex.size() <= 28) return hex;
    return hex.substr(0, 12) + "..." + hex.substr(hex.size() - 12) + " (" + std::to_string(hex.size()) + " digits)";
}

void print_indented(const std::string& text, const std::string& indent)
{
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) std::cout << indent << line << "\n";
}

void print_header(const Line& line)
{
    std::cout << "line         " << to_string(line.kind) << "\n"
              << "space        " << line.space.id() << "\n";
}

FilterConfig load_filter_config(const Args& a)
{
    // A file named with --filters must exist (a typo would otherwise mean "nothing ticked").
    if (a.has("filters") && !fs::exists(fs::path(a.get("filters"))))
        throw std::invalid_argument("no filter settings file at " + a.get("filters"));
    return FilterConfig::load(a.has("filters") ? fs::path(a.get("filters")) : FilterConfig::default_path());
}

std::string filters_path(const Args& a) { return a.has("filters") ? a.get("filters") : FilterConfig::default_path().string(); }

// ---------------------------------------------------------------- info

int cmd_info(const Args& a)
{
    const Line line = make_line(a);
    const Space& sp = line.space;
    print_header(line);
    std::cout << "unit         " << line.describe_symbols() << "\n"
              << "units        ~10^" << std::floor(sp.size().log10_approx() * 100) / 100 << "\n"
              << "address      " << sp.address_bits() << " bits, " << sp.hex_width() << " hex digits\n";
    if (sp.size().log10_approx() < 60) std::cout << "exact        " << sp.size().to_decimal() << "\n";
    {
        const LineLoop loop(sp.size());
        std::cout << "hallway      one loop is " << (loop.tiles().log10_approx() < 30 ? loop.tiles().to_decimal() : "~10^" + std::to_string(int(loop.tiles().log10_approx())))
                  << " tiles of " << kBooksPerTile << " books; "
                  << (loop.fills_whole_tiles() ? std::string("fills whole tiles exactly")
                                               : "its last tile has " + std::to_string(loop.padding()) +
                                                     " empty slots (a size that is a power of two, at least 128, would have none)")
                  << "\n";
    }
    if (line.guided)
        std::cout << "guided       model " << line.model_id << " (order " << line.guided->model().order() << ", "
                  << line.guided->model().context_count() << " contexts): addresses cost about as many bits as the\n"
                  << "             unit carries information; the line has 2^" << line.guided->scale_bits() << " points\n";
    return 0;
}

// Which orderings a command shows. "all" (or the older "both"): positional, scrambled and, for
// a line with a model, guided.
struct Modes
{
    bool positional = false, scrambled = false, guided = false;
};

const GuidedLine& need_guided(const Line& line)
{
    if (!line.guided)
    {
        if (line.kind != LineKind::Text) throw std::invalid_argument("guided ordering is only available on the text line so far");
        throw std::invalid_argument("no model for alphabet " + line.alphabet->id() +
                                    " (see: sieve models; build one with sieve train, or pass --model FILE)");
    }
    return *line.guided;
}

Modes parse_modes(const Line& line, const std::string& mode)
{
    Modes m;
    if (mode == "all" || mode == "both") { m.positional = m.scrambled = true; m.guided = line.guided != nullptr; }
    else if (mode == "guided") { need_guided(line); m.guided = true; }
    else if (address_mode_from_string(mode) == AddressMode::Positional) m.positional = true;
    else m.scrambled = true;
    return m;
}

std::string percent(double f)
{
    char pct[32];
    std::snprintf(pct, sizeof pct, "%.10f%%", f * 100.0);
    return pct;
}

std::string fixed(double v, int decimals)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", decimals, v);
    return b;
}

// Uniformly random point on a guided line (S random bits).
BigUint random_point(const GuidedLine& g, std::mt19937_64& rng)
{
    BigUint p;
    const size_t words = (g.scale_bits() + 31) / 32;
    for (size_t i = 0; i < words; ++i)
    {
        p <<= 32;
        p.add_small(static_cast<uint32_t>(rng()));
    }
    p >>= words * 32 - g.scale_bits();
    return p;
}

// --compact: the line's survivors under the ticked filter stack (--filters), in every ordering.
struct Compact
{
    FilterStack stack;
    std::unique_ptr<CompactLine> line;
};
std::unique_ptr<Compact> make_compact(const Line& line, const Args& a)
{
    auto c = std::make_unique<Compact>();
    c->stack = build_stack(line, load_filter_config(a).of(line.kind));
    if (c->stack.empty()) throw std::invalid_argument("--compact needs ticked filters (see: sieve filters)");
    if (!c->stack.ranker()) throw std::invalid_argument("this stack cannot rank its survivors: " + c->stack.compact_blocker());
    c->line = std::make_unique<CompactLine>(*c->stack.ranker(), line.space.key(), c->stack.id(), line.guided ? line.guided->model_ptr() : nullptr);
    std::cerr << "compact: " << c->line->count().to_decimal() << " survivors of " << c->stack.provenance() << "\n";
    return c;
}

// A uniformly random number below n (64 extra random bits make the bias negligible).
BigUint random_below(const BigUint& n, std::mt19937_64& rng)
{
    BigUint v;
    for (size_t b = 0; b < n.bit_length() + 64; b += 32)
    {
        v <<= 32;
        v.add_small(static_cast<uint32_t>(rng()));
    }
    return BigUint::mod(v, n);
}

// ---------------------------------------------------------------- warp

int cmd_warp(const Args& a)
{
    const Line line = make_line(a);
    const Modes modes = parse_modes(line, a.get("mode", "all"));
    const bool abbreviate = a.has("short");

    const WarpInput w = read_warp_input(line, a);
    const auto compact = a.has("compact") ? make_compact(line, a) : nullptr;
    print_header(line);
    if (modes.guided) std::cout << "model        " << line.model_id << " (" << line.guided->model().sha256().substr(0, 16) << "...)\n";
    std::cout << "canon        " << w.report.front() << "\n";
    for (size_t i = 1; i < w.report.size(); ++i) std::cout << w.report[i] << "\n";

    const bool multiline = line.kind == LineKind::Image || line.kind == LineKind::Video;
    const double raw_bits = std::log2(double(line.space.base()));
    for (size_t i = 0; i < w.units.size(); ++i)
    {
        const auto& digits = w.units[i];
        std::cout << "\nunit " << i + 1 << "/" << w.units.size();
        if (multiline) { std::cout << "\n"; print_indented(preview(line, digits), "  "); }
        else std::cout << "  " << preview(line, digits) << "\n";
        for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            if (!(m == AddressMode::Positional ? modes.positional : modes.scrambled)) continue;
            const auto addr = line.space.address_digits(digits, m); // scramble once, reuse below
            std::cout << "  " << to_string(m) << (m == AddressMode::Positional ? "  " : "   ")
                      << show_address(line.space.hex_of(addr), abbreviate) << "\n"
                      << "  " << std::string(12, ' ') << "at " << percent(line.space.fraction_of(addr)) << " along the line\n";
        }
        if (modes.guided)
        {
            const GuidedLine& g = *line.guided;
            const auto c = g.code(digits);
            const double per = double(c.bits) / digits.size();
            std::cout << "  guided      " << show_address(c.hex, abbreviate) << "\n"
                      << "  " << std::string(12, ' ') << c.bits << " bits: " << fixed(per, 2) << " bits/symbol (raw "
                      << fixed(raw_bits, 2) << "), at " << percent(g.fraction(c.point)) << " along the guided line\n";
        }
        if (compact)
        {
            const CompactLine& cl = *compact->line;
            const int fail = compact->stack.first_failure(digits);
            if (fail >= 0)
            {
                std::cout << "  compact     not a survivor: fails " << compact->stack.filter_name(size_t(fail)) << "\n";
                continue;
            }
            const BigUint k = cl.index_of(digits, AddressMode::Positional);
            std::cout << "  compact     survivor number " << k.to_decimal() << " of " << cl.count().to_decimal() << "\n";
            if (modes.positional) std::cout << "    positional  " << show_address(cl.hex_of(k), abbreviate) << "\n";
            if (modes.scrambled) std::cout << "    scrambled   " << show_address(cl.hex_of(cl.index_of(digits, AddressMode::Scrambled)), abbreviate) << "\n";
            if (modes.guided && cl.guided())
            {
                const auto c = cl.guided()->code(digits);
                std::cout << "    guided      " << show_address(c.hex, abbreviate) << "  (" << c.bits << " bits: "
                          << fixed(double(c.bits) / digits.size(), 2) << " bits/symbol)\n";
            }
        }
    }
    return 0;
}

// ---------------------------------------------------------------- read

void print_shelf_row(const Line& line, int64_t off, const std::vector<uint32_t>& u, const std::string& addr)
{
    char label[24];
    std::snprintf(label, sizeof label, "%+lld", static_cast<long long>(off));
    if (line.kind == LineKind::Image || line.kind == LineKind::Video)
    {
        std::cout << (off == 0 ? "> " : "  ") << label << "  " << addr << "\n";
        print_indented(preview(line, u), "    ");
    }
    else
        std::cout << (off == 0 ? "> " : "  ") << std::string(6 - std::min<size_t>(6, std::strlen(label)), ' ') << label
                  << "  " << preview(line, u) << "  " << addr << "\n";
}

// --at TILE:SLOT : the book in that place of the shared corridor, as the hallway shows it.
int read_at(const Line& line, const Args& a, const std::string& mode)
{
    const std::string at = a.get("at");
    const size_t colon = at.find(':');
    if (colon == std::string::npos) throw std::invalid_argument("--at expects TILE:SLOT, for example 4626:0 or -1:127");
    const TileIndex tile = TileIndex::parse(at.substr(0, colon));
    const uint32_t slot = uint32_t(parse_whole(at.substr(colon + 1), "--at SLOT"));
    if (slot >= kBooksPerTile) throw std::invalid_argument("SLOT must be 0.." + std::to_string(kBooksPerTile - 1));
    const bool guided = mode == "guided";
    const auto compact = a.has("compact") ? make_compact(line, a) : nullptr;
    uint32_t zoom = 0;
    if (guided)
    {
        need_guided(line);
        if (!a.has("zoom")) throw std::invalid_argument("--mode guided --at needs --zoom D (books 2^-D apart), as shown in the hallway");
        zoom = a.get_positive("zoom", 20);
        if (zoom > line.guided->scale_bits())
            throw std::invalid_argument("--zoom is finer than the line (" + std::to_string(line.guided->scale_bits()) + " bits)");
    }
    const LineLoop loop(guided ? BigUint::pow(2, zoom) : compact ? compact->line->count() : line.space.size());
    const auto index = loop.unit_index(loop.loop_tile(tile), slot);
    if (!index)
    {
        std::cout << "(empty: padding at the end of a loop)\n";
        return 0;
    }
    std::vector<uint32_t> digits;
    if (guided)
    {
        const GuidedLine& g = compact ? *compact->line->guided() : *line.guided;
        BigUint point = *index;
        point <<= g.scale_bits() - zoom;
        digits = g.unit_at(point);
        std::cerr << "point " << g.hex_of(point, zoom) << "\n";
    }
    else if (compact)
    {
        digits = compact->line->unit_at(*index, address_mode_from_string(mode));
        std::cerr << "compact address " << compact->line->hex_of(*index) << "\n";
    }
    else
    {
        const auto address = index->to_digits(line.space.base(), line.space.unit_length());
        digits = line.space.unit_of_address(address, address_mode_from_string(mode));
        std::cerr << "address " << line.space.hex_of(address) << "\n";
    }
    if (line.kind == LineKind::Text) std::cout << utf8_encode(line.space.text_of(digits)) << "\n";
    else std::cout << preview(line, digits) << "\n";
    if (a.has("out"))
    {
        save_unit(line, digits, a.get("out"), a.get_positive("scale", 16));
        std::cerr << "saved " << a.get("out") << "\n";
    }
    return 0;
}

int cmd_read(const Args& a)
{
    const Line line = make_line(a);
    if (!a.has("mode")) throw std::invalid_argument("missing --mode positional|scrambled|guided");
    const std::string mode = a.get("mode");
    if (a.has("at")) return read_at(line, a, mode);
    if (a.has("survivor"))
    {
        // The K-th unit (from 0) that passes the line's filter stack, in address order.
        const FilterStack st = build_stack(line, load_filter_config(a).of(line.kind));
        if (st.empty()) throw std::invalid_argument("--survivor needs ticked filters (see: sieve filters)");
        if (!st.ranker()) throw std::invalid_argument("this stack cannot rank its survivors: " + st.compact_blocker());
        const BigUint k = BigUint::from_decimal(a.get("survivor"));
        if (k >= st.ranker()->count()) throw std::out_of_range("there are only " + st.ranker()->count().to_decimal() + " survivors");
        const auto digits = st.ranker()->unrank(k);
        std::cerr << "survivor " << k.to_decimal() << " of " << st.ranker()->count().to_decimal() << ", address "
                  << line.space.hex_of(line.space.address_digits(digits, address_mode_from_string(mode))) << "\n";
        if (line.kind == LineKind::Text) std::cout << utf8_encode(line.space.text_of(digits)) << "\n";
        else std::cout << preview(line, digits) << "\n";
        return 0;
    }
    if (a.positional.size() != 1) throw std::invalid_argument("give exactly one ADDRESS");
    const auto compact = a.has("compact") ? make_compact(line, a) : nullptr;
    std::vector<uint32_t> digits;
    if (compact && mode != "guided")
    {
        // A compact address: the survivor number (positional) or its shuffle (scrambled).
        const CompactLine& cl = *compact->line;
        const AddressMode m = address_mode_from_string(mode);
        const BigUint index = cl.parse(a.positional[0]);
        digits = cl.unit_at(index, m);
        std::cerr << "survivor number " << (m == AddressMode::Positional ? index : cl.ranker().rank(digits)).to_decimal() << " of "
                  << cl.count().to_decimal() << "\n";
        if (a.has("around"))
        {
            const int64_t n = a.get_u32("around", 0);
            for (int64_t off = -n; off <= n; ++off)
            {
                // Neighbouring compact addresses, around the loop of survivors.
                BigUint i = index;
                if (off >= 0) i += BigUint(uint64_t(off));
                else
                {
                    i += cl.count();
                    i -= BigUint(uint64_t(-off));
                }
                i = BigUint::mod(i, cl.count());
                print_shelf_row(line, off, cl.unit_at(i, m), show_address(cl.hex_of(i), a.has("short")));
            }
        }
    }
    else if (mode == "guided")
    {
        const GuidedLine& g = compact ? *compact->line->guided() : need_guided(line);
        if (compact && !compact->line->guided()) need_guided(line);
        const BigUint point = g.point_of(a.positional[0]);
        digits = g.unit_at(point);
        const auto own = g.code(digits);
        if (!(own.point == point))
            std::cerr << "note: that point lies inside the arc of a unit whose own address is " << own.hex << " ("
                      << own.bits << " bits)\n";
        if (a.has("around"))
        {
            // Books spaced 2^-zoom apart along the guided line, as the hallway shows them.
            const uint32_t zoom = a.has("zoom") ? a.get_u32("zoom", 0) : uint32_t(std::max<size_t>(own.bits, 1));
            if (zoom > g.scale_bits()) throw std::invalid_argument("--zoom is finer than the line (" + std::to_string(g.scale_bits()) + " bits)");
            std::cerr << "books 2^-" << zoom << " apart: each shows its point, then the length of the address of the\n"
                         "unit found there (its information content, to within 2 bits)\n";
            // Book 0 is the given point, snapped down to the zoom grid, so every label is exact.
            BigUint base = point;
            base >>= g.scale_bits() - zoom;
            base <<= g.scale_bits() - zoom;
            const int64_t n = a.get_u32("around", 0);
            for (int64_t off = -n; off <= n; ++off)
            {
                const BigUint p = g.step(base, off, zoom);
                const auto u = g.unit_at(p);
                print_shelf_row(line, off, u, show_address(g.hex_of(p, zoom), a.has("short")) + "  (" + std::to_string(g.code(u).bits) + " bits)");
            }
        }
    }
    else
    {
        const AddressMode m = address_mode_from_string(mode);
        const auto address = line.space.parse_address(a.positional[0]);
        digits = line.space.unit_of_address(address, m);
        if (a.has("around"))
        {
            // The neighbouring shelves: what the hallway shows either side of this unit.
            const int64_t n = a.get_u32("around", 0);
            for (int64_t off = -n; off <= n; ++off)
            {
                // Step the address digits and unscramble once per shelf.
                const auto step = line.space.step_address(address, off);
                const auto u = off == 0 ? digits : line.space.unit_of_address(step, m);
                print_shelf_row(line, off, u, show_address(line.space.hex_of(step), a.has("short")));
            }
        }
    }
    if (!a.has("around"))
    {
        if (line.kind == LineKind::Text) std::cout << utf8_encode(line.space.text_of(digits)) << "\n";
        else std::cout << preview(line, digits) << "\n";
    }
    if (a.has("out"))
    {
        save_unit(line, digits, a.get("out"), a.get_positive("scale", 16));
        std::cerr << "saved " << a.get("out") << "\n";
    }
    return 0;
}

// ---------------------------------------------------------------- browse

int cmd_browse(const Args& a)
{
    const Line line = make_line(a);
    const Space& sp = line.space;
    const uint32_t count = a.get_positive("count", 5);
    const bool abbreviate = a.has("short");
    const std::string mode = a.get("mode", "scrambled");
    if (mode != "scrambled" && mode != "guided") throw std::invalid_argument("--mode must be scrambled or guided");
    std::random_device rd;
    std::mt19937_64 rng((uint64_t(rd()) << 32) ^ rd());
    if (a.has("seed")) rng.seed(a.get_u32("seed", 0));
    std::uniform_int_distribution<uint32_t> digit(0, sp.base() - 1);
    const bool multiline = line.kind == LineKind::Image || line.kind == LineKind::Video;
    const auto compact = a.has("compact") ? make_compact(line, a) : nullptr;
    if (mode == "guided")
        std::cerr << "random points on the guided line: text the model finds likely. It is fluent by\n"
                     "construction, which is evidence of nothing (SPECIFICATIONS 4.2).\n\n";
    for (uint32_t i = 0; i < count; ++i)
    {
        std::vector<uint32_t> unit;
        std::string label;
        if (compact && mode != "guided")
        {
            // A uniformly random compact address is a uniformly random survivor.
            const CompactLine& cl = *compact->line;
            const BigUint j = random_below(cl.count(), rng);
            unit = cl.unit_at(j, AddressMode::Scrambled);
            label = "  compact scrambled " + show_address(cl.hex_of(j), abbreviate) + "  (survivor number " +
                    cl.ranker().rank(unit).to_decimal() + ")";
        }
        else if (mode == "guided")
        {
            // A uniformly random point is a sample from the model (restricted to survivors with --compact).
            if (compact && !compact->line->guided()) need_guided(line);
            const GuidedLine& g = compact ? *compact->line->guided() : need_guided(line);
            unit = g.unit_at(random_point(g, rng));
            const auto c = g.code(unit);
            label = "  guided " + show_address(c.hex, abbreviate) + "  (" + std::to_string(c.bits) + " bits)";
        }
        else
        {
            // A uniformly random scrambled address is a uniformly random unit.
            std::vector<uint32_t> addr(sp.unit_length());
            for (auto& d : addr) d = digit(rng);
            label = "  scrambled " + show_address(sp.hex_of(addr), abbreviate);
            unit = sp.unit_of_address(std::move(addr), AddressMode::Scrambled);
        }
        if (i) std::cout << "\n";
        if (multiline) print_indented(preview(line, unit), "");
        else std::cout << preview(line, unit) << "\n";
        std::cout << label << "\n";
    }
    return 0;
}

// ---------------------------------------------------------------- sieve

// "1-12,16,100" -> {1..12, 16, 100}
std::vector<uint32_t> parse_lengths(const std::string& spec)
{
    std::vector<uint32_t> out;
    std::stringstream ss(spec);
    std::string part;
    while (std::getline(ss, part, ','))
    {
        const auto dash = part.find('-');
        // Lengths up to a million: far beyond what sift can count in any reasonable time.
        const uint64_t a = parse_whole(part.substr(0, dash), "--lengths", 1000000);
        const uint64_t b = dash == std::string::npos ? a : parse_whole(part.substr(dash + 1), "--lengths", 1000000);
        if (a < 1 || b < a) throw std::invalid_argument("bad length range '" + part + "'");
        for (uint64_t L = a; L <= b; ++L) out.push_back(uint32_t(L));
    }
    if (out.empty()) throw std::invalid_argument("no lengths given");
    return out;
}
std::string fmt_log(double v)
{
    if (std::isinf(v)) return "-inf";
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.4f", v);
    return buf;
}

int cmd_sieve(const Args& a)
{
    const ResolvedDictionary rd = resolve_dictionary(a.get("dict"));
    const std::string& dict_path = rd.path;
    const std::vector<uint32_t> lengths = parse_lengths(a.get("lengths", "1-12"));
    const uint32_t brute_max = a.get_u32("brute-max", 5), pruned_max = a.get_u32("pruned-max", 6);
    const unsigned threads = a.get_u32("threads", std::max(1u, std::thread::hardware_concurrency()));

    const Dictionary dict = Dictionary::load_file(dict_path);
    if (!rd.expected_sha256.empty() && dict.sha256() != rd.expected_sha256)
        throw std::runtime_error("dictionary '" + rd.id + "' does not match its registered SHA-256 (the file has changed).\n"
                                 "  expected " + rd.expected_sha256 + "\n  found    " + dict.sha256() +
                                 "\nRestore the original file, or register the new one under a new id.");
    std::cerr << "dictionary  " << rd.id << " (" << dict_path << ")\n            " << dict.word_count() << " words (" << dict.skipped_lines()
              << " lines skipped), sha256 " << dict.sha256() << "\n";

    std::ofstream csv_file;
    std::ostream* csv = &std::cout;
    if (a.has("csv"))
    {
        csv_file.open(a.get("csv"));
        if (!csv_file) throw std::runtime_error("cannot write '" + a.get("csv") + "'");
        csv = &csv_file;
    }
    *csv << "# sieve sift, alphabet lower27, dictionary " << rd.id << " sha256 " << dict.sha256() << "\n"
         << "length,log10_units,clean,window,words,log10_frac_clean,log10_frac_window,log10_frac_words,"
            "verified_by,pruned_window_states,log10_frac_prefix_tree_explored\n";

    bool all_ok = true;
    for (uint32_t L : lengths)
    {
        const auto t0 = std::chrono::steady_clock::now();
        const SieveCount exact = sieve_counted(L, dict);
        std::string verified = "counted";
        std::string states, explored;

        if (L <= brute_max)
        {
            const SieveCount b = sieve_brute(L, dict, threads);
            const bool ok = b.clean == exact.clean && b.window == exact.window && b.words == exact.words;
            verified += ok ? "+brute" : "+BRUTE_MISMATCH";
            all_ok &= ok;
        }
        if (L <= pruned_max)
        {
            bool ok = true;
            PrunedResult pw{};
            for (SieveFilter f : {SieveFilter::Window, SieveFilter::Words})
            {
                const PrunedResult p = sieve_pruned(L, f, dict, threads);
                const BigUint& want = f == SieveFilter::Window ? exact.window : exact.words;
                ok &= BigUint(p.survivors) == want;
                if (f == SieveFilter::Window) pw = p;
            }
            verified += ok ? "+pruned" : "+PRUNED_MISMATCH";
            all_ok &= ok;
            states = std::to_string(pw.states_explored);
            explored = fmt_log(std::log10(static_cast<double>(pw.states_explored)) - prefix_tree_nodes(L).log10_approx());
        }

        const double units = L * std::log10(27.0);
        *csv << L << "," << fmt_log(units) << "," << exact.clean.to_decimal() << "," << exact.window.to_decimal()
             << "," << exact.words.to_decimal() << "," << fmt_log(exact.clean.log10_approx() - units) << ","
             << fmt_log(exact.window.log10_approx() - units) << "," << fmt_log(exact.words.log10_approx() - units)
             << "," << verified << "," << states << "," << explored << "\n";
        csv->flush();
        const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        std::cerr << "L=" << L << " done in " << secs << "s (" << verified << ")\n";
    }
    if (!all_ok)
    {
        std::cerr << "ERROR: methods disagree — see verified_by column\n";
        return 2;
    }
    return 0;
}

// ---------------------------------------------------------------- version

int cmd_version()
{
    std::cout << "sieve " << SIEVE_VERSION << "  (specification 2.0)\n\n"
              << "Pinned rules - results record these, and changing any of them means a new version:\n"
              << "  address scramble        " << kScrambleVersion << " (" << kScrambleRounds << " rounds)\n"
              << "  text canonicalisation   " << to_string(kDefaultCanon) << " (default), canon-text-v1\n"
              << "  image canonicalisation  " << kImageCanonVersion << "\n"
              << "  note canonicalisation   " << kNotesCanonVersion << "\n"
              << "  alphabets               ";
    for (const auto& id : alphabet_ids()) std::cout << id << " ";
    std::cout << "\n  palettes                ";
    for (const auto& id : palette_ids()) std::cout << id << " ";
    std::cout << "\n  note symbols            " << kNotesSymbolsId << "\n"
              << "  guided addresses        " << kGuidedVersion << " over " << kCharModelFormat << " (" << kSmoothing << ", total 2^"
              << kModelTotalBits << ")\n"
              << "  compact orderings       " << kShuffleVersion << " (scrambled), " << kSieveRestrictVersion << " (guided)\n"
              << "  filters                 ";
    for (const auto& f : filter_registry()) std::cout << f.name() << " ";
    std::cout << "\n";
    try
    {
        const ModelRegistry mr = load_model_registry();
        if (const ModelEntry* m = mr.default_for("lower27"))
            std::cout << "  default model           " << m->id << " (sha256 " << m->sha256 << ")\n";
    }
    catch (const std::exception&)
    {
    }
    try
    {
        const Registry reg = load_registry();
        const auto& d = reg.default_entry();
        std::cout << "  default dictionary      " << d.id << " (sha256 " << d.sha256 << ")\n";
    }
    catch (const std::exception&)
    {
        std::cout << "  default dictionary      (registry not found)\n";
    }
    std::cout << "\nThis machine:\n"
              << "  SHA-256                 "
              << (Sha256::using_hardware() ? "hardware (SHA-NI)" : "portable") << " - both give identical results\n";
    return 0;
}

// ---------------------------------------------------------------- dicts

int cmd_dicts(const Args& a)
{
    if (a.has("hash"))
    {
        const DictionaryFileInfo d = Dictionary::inspect_file(a.get("hash"));
        std::cout << "file      " << a.get("hash") << "\n"
                  << "words     " << d.word_count << " (" << d.skipped_lines << " lines skipped: not a-z only)\n"
                  << "sha256    " << d.sha256 << "\n"
                  << "\nRegistry line (edit id, language and description; tab-separated):\n"
                  << "my-dictionary\t" << fs::path(a.get("hash")).filename().string() << "\ten\tno\t" << d.sha256
                  << "\tDescription of this word list\n";
        return 0;
    }
    const Registry reg = load_registry();
    std::cout << "registry  " << (reg.folder / "dictionaries.tsv").string() << "\n\n";
    for (const auto& e : reg.entries)
    {
        std::string status;
        if (!fs::exists(e.path)) status = "MISSING FILE";
        else
        {
            const DictionaryFileInfo d = Dictionary::inspect_file(e.path.string());
            status = d.sha256 == e.sha256 ? std::to_string(d.word_count) + " words, hash ok" : "HASH MISMATCH";
        }
        std::string id = e.id;
        id.resize(std::max<size_t>(id.size() + 2, 14), ' ');
        std::cout << (e.is_default ? "* " : "  ") << id << e.language << "  " << status << "\n    " << e.description << "\n";
    }
    std::cout << "\n* = default. Use one with --dict ID. To add a dictionary, see: sieve help dicts\n";
    return 0;
}

// ---------------------------------------------------------------- the models line

// Reads a Wavefront .obj: "v x y z" and "f" faces of any arity, split into a fan. Indices may be
// negative (counting back from the newest vertex), and may carry /vt/vn, which is ignored here:
// the models line holds geometry, and a texture is an address on the image line.
struct ObjMesh
{
    std::vector<ModelSpace::Vertex> verts;
    std::vector<ModelSpace::Face> faces;
};

ObjMesh read_obj(const std::string& path)
{
    std::ifstream in(path);
    if (!in) throw std::invalid_argument("cannot read " + path);
    ObjMesh m;
    std::string line;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::istringstream ls(line);
        std::string tag;
        if (!(ls >> tag)) continue;
        if (tag == "v")
        {
            ModelSpace::Vertex v;
            if (ls >> v.x >> v.y >> v.z) m.verts.push_back(v);
        }
        else if (tag == "f")
        {
            std::vector<uint32_t> idx;
            std::string tok;
            while (ls >> tok)
            {
                const std::string first = tok.substr(0, tok.find('/'));
                if (first.empty()) continue;
                long n = 0;
                try { n = std::stol(first); } catch (...) { continue; }
                if (n < 0) n = long(m.verts.size()) + n; // -1 is the newest vertex
                else --n;                                 // .obj counts from 1
                if (n >= 0) idx.push_back(uint32_t(n));
            }
            for (size_t i = 2; i < idx.size(); ++i) m.faces.push_back({idx[0], idx[i - 1], idx[i]});
        }
    }
    if (m.verts.empty()) throw std::invalid_argument(path + " has no vertices");
    return m;
}

ModelSpace make_model_space(const Args& a)
{
    return ModelSpace(a.get_positive("vertices", 8), a.get_positive("faces", 12), a.get_positive("coords", 16),
                      a.get("key", "sieve"));
}

// A short readable summary of a model: its bounding box on the grid and its first faces.
std::string mesh_preview(const ModelSpace& sp, const ModelSpace::Parts& p)
{
    const auto verts = sp.mesh_of(p);
    const auto faces = sp.faces_of(p);
    uint32_t used = 0, degenerate = 0;
    std::vector<bool> seen(sp.vertices(), false);
    for (const auto& f : faces)
    {
        seen[f.a] = seen[f.b] = seen[f.c] = true;
        if (f.a == f.b || f.b == f.c || f.a == f.c) ++degenerate;
    }
    for (bool b : seen)
        if (b) ++used;
    std::string out = "  " + std::to_string(sp.vertices()) + " vertices (" + std::to_string(used) + " used), " +
                      std::to_string(sp.face_count()) + " faces (" + std::to_string(degenerate) + " degenerate)\n";
    for (uint32_t i = 0; i < std::min<uint32_t>(3, sp.vertices()); ++i)
        out += "  v" + std::to_string(i + 1) + " " + fixed(double(verts[i].x), 4) + " " + fixed(double(verts[i].y), 4) + " " +
               fixed(double(verts[i].z), 4) + "\n";
    if (sp.vertices() > 3) out += "  ...\n";
    return out;
}

int cmd_mesh(const Args& a)
{
    const ModelSpace sp = make_model_space(a);
    auto shape = [&] {
        std::cout << "line         models\n"
                  << "space        " << sp.id() << "\n"
                  << "unit         " << sp.vertices() << " vertices on a grid of " << sp.coords() << " points per axis, "
                  << sp.face_count() << " triangles\n";
    };
    // --read ADDRESS: the model at an address, as canonical .obj.
    if (a.has("read"))
    {
        const AddressMode m = address_mode_from_string(a.get("mode", "positional"));
        const ModelSpace::Parts p = sp.parts_at(sp.parse(a.get("read")), m);
        const std::string obj = sp.to_obj(p);
        if (a.has("out"))
        {
            std::ofstream out(a.get("out"), std::ios::binary);
            out << obj;
            if (!out) throw std::invalid_argument("cannot write " + a.get("out"));
            std::cerr << "wrote " << a.get("out") << " (" << obj.size() << " bytes)\n";
        }
        else std::cout << obj;
        std::cerr << mesh_preview(sp, p);
        return 0;
    }
    // --warp FILE: where a mesh lives on this line.
    if (a.has("warp"))
    {
        const ObjMesh mesh = read_obj(a.get("warp"));
        const ModelSpace::Fitted fit = sp.fit(mesh.verts, mesh.faces);
        shape();
        std::cout << "fitted       " << mesh.verts.size() << " vertices in, " << mesh.faces.size() << " triangles in; scaled by "
                  << fixed(double(fit.scale), 4) << "\n";
        if (fit.vertices_dropped) std::cout << "             " << fit.vertices_dropped << " vertices dropped (past --vertices)\n";
        if (fit.faces_dropped) std::cout << "             " << fit.faces_dropped << " faces dropped (past --faces)\n";
        if (fit.faces_added) std::cout << "             " << fit.faces_added << " filler faces added\n";
        if (fit.indices_clamped) std::cout << "             " << fit.indices_clamped << " indices clamped into range\n";
        for (const char* m : {"positional", "scrambled"})
        {
            const BigUint k = sp.index_of(fit.parts, address_mode_from_string(m));
            std::cout << "  " << std::setw(11) << std::left << m << show_address(sp.hex_of(k), a.has("short")) << "\n";
        }
        std::cout << mesh_preview(sp, fit.parts);
        return 0;
    }
    // --browse N: models off the shelf.
    if (a.has("browse"))
    {
        const uint32_t n = a.get_positive("browse", 5);
        std::mt19937_64 rng(a.has("seed") ? a.get_u32("seed", 0) : std::random_device{}());
        const AddressMode m = address_mode_from_string(a.get("mode", "positional"));
        for (uint32_t i = 0; i < n; ++i)
        {
            const BigUint k = random_below(sp.size(), rng);
            std::cout << show_address(sp.hex_of(k), a.has("short")) << "\n" << mesh_preview(sp, sp.parts_at(k, m));
        }
        return 0;
    }
    shape();
    std::cout << "models       ~10^" << std::floor(sp.size().log10_approx() * 100) / 100 << "\n"
              << "address      " << sp.size().bit_length() << " bits, " << sp.hex_width() << " hex digits\n";
    if (sp.size().log10_approx() < 60) std::cout << "exact        " << sp.size().to_decimal() << "\n";
    std::cout << "as text      " << sp.obj_length() << " characters of canonical .obj, so a page of that length on an\n"
              << "             ascii96 line holds the same model (see: sieve help alphabets)\n";
    {
        const LineLoop loop(sp.size());
        std::cout << "hallway      one loop is "
                  << (loop.tiles().log10_approx() < 30 ? loop.tiles().to_decimal()
                                                       : "~10^" + std::to_string(int(loop.tiles().log10_approx())))
                  << " tiles of " << kBooksPerTile << " models\n";
    }
    return 0;
}

// ---------------------------------------------------------------- alphabets

int cmd_alphabets(const Args& a)
{
    auto hex = [](char32_t c) {
        std::string s;
        for (int i = 20; i >= 0; i -= 4)
        {
            const int d = int(c >> i) & 15;
            if (!s.empty() || d || i <= 12) s += "0123456789ABCDEF"[d];
        }
        return "U+" + s;
    };
    // One alphabet in detail: what a spec works out to, before a line is built with it.
    if (a.has("spec"))
    {
        const sieve::Alphabet& al = sieve::alphabet_of(a.get("spec"));
        std::cout << "alphabet  " << al.id() << "\n"
                  << "symbols   " << al.size() << "\n"
                  << "holds     " << al.description() << "\n";
        for (const sieve::CodeRange& r : al.ranges())
            std::cout << "range     " << hex(r.first) << " - " << hex(r.last) << "  (" << r.count() << ")\n";
        std::cout << "digit 0   " << hex(al.symbol(0)) << "\n";
        if (!al.text_is_encodable())
            std::cout << "note      holds surrogates: units over it can be addressed and drawn, but have no text form\n";
        return 0;
    }
    std::cout << "Built in, pinned with the specification:\n\n";
    for (const std::string& id : sieve::alphabet_ids())
    {
        const sieve::Alphabet& al = sieve::alphabet_by_id(id);
        std::string name = id;
        name.resize(std::max<size_t>(name.size() + 2, 12), ' ');
        std::cout << "  " << name << std::setw(8) << al.size() << "  " << al.description() << "\n";
    }
    std::cout << "\nUnicode blocks, to use on their own or stacked with '+':\n\n";
    for (const sieve::Block& b : sieve::blocks())
    {
        std::string id(b.id);
        id.resize(std::max<size_t>(id.size() + 2, 34), ' ');
        std::cout << "  " << id << std::setw(8) << b.range.count() << "  " << hex(b.range.first) << " - " << hex(b.range.last)
                  << "\n    " << b.name << ". " << b.description << "\n";
    }
    std::cout << "\nStack them with '+', in any order: --alphabet greek+cyrillic. A raw range works too\n"
                 "(--alphabet u+0370-u+03ff), and stacked code points are unioned, so blocks that\n"
                 "overlap never give a symbol twice. To see what a stack works out to:\n"
                 "  sieve alphabets --spec greek+cyrillic\n";
    return 0;
}

// ---------------------------------------------------------------- models

int cmd_models()
{
    const ModelRegistry reg = load_model_registry();
    if (reg.entries.empty())
    {
        std::cout << "no model registry found (data/models/models.tsv). Guided addressing is unavailable.\n";
        return 0;
    }
    std::cout << "registry  " << (reg.folder / "models.tsv").string() << "\n\n";
    for (const auto& e : reg.entries)
    {
        std::string status;
        if (!fs::exists(e.path)) status = "MISSING FILE";
        else
        {
            try
            {
                const CharModel m = CharModel::load_file(e.path.string());
                if (m.sha256() != e.sha256) status = "HASH MISMATCH";
                else
                    status = "order " + std::to_string(m.order()) + ", " + std::to_string(m.context_count()) + " contexts, trained on " +
                             std::to_string(m.trained_symbols()) + " symbols, hash ok";
            }
            catch (const std::exception& ex) { status = std::string("UNREADABLE: ") + ex.what(); }
        }
        std::string id = e.id;
        id.resize(std::max<size_t>(id.size() + 2, 22), ' ');
        std::cout << (e.is_default ? "* " : "  ") << id << e.symbols << "  " << status << "\n    " << e.description << "\n";
    }
    std::cout << "\n* = default for its alphabet. Use one with --model ID. Build one with: sieve train\n";
    return 0;
}

// ---------------------------------------------------------------- train

// The corpus manifest: --corpus, else the repository's copy (from the project folder), else the
// copy the build places next to the executable.
std::string default_corpus(const Args& a)
{
    if (a.has("corpus")) return a.get("corpus");
    const fs::path here = "data/models/corpus/gutenberg-nltk.tsv";
    if (fs::exists(here)) return here.string();
    return (executable_dir() / "models" / "corpus" / "gutenberg-nltk.tsv").string();
}

int cmd_train(const Args& a)
{
    if (!a.has("out")) throw std::invalid_argument("missing --out FILE (the model to write)");
    const Corpus corpus = load_corpus(default_corpus(a));
    const fs::path dir = a.get("texts", "corpus/gutenberg");
    const Alphabet& alpha = alphabet_of(a.get("alphabet", "lower27"));
    ModelParams p;
    p.symbols_id = alpha.id();
    p.base = alpha.size();
    p.order = a.get_u32("order", 5);
    p.min_count = a.get_positive("min-count", 8);
    p.padding = alpha.digit_of(U' ');

    const auto t0 = std::chrono::steady_clock::now();
    const auto stream = training_stream(corpus, dir, alpha, p.corpus);
    const auto t1 = std::chrono::steady_clock::now();
    const CharModel m = CharModel::train(stream, p);
    const auto t2 = std::chrono::steady_clock::now();
    const std::string text = m.serialise();
    std::ofstream out(a.get("out"), std::ios::binary);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!out) throw std::runtime_error("cannot write " + a.get("out"));

    const auto secs = [](auto d) { return fixed(std::chrono::duration<double>(d).count(), 2) + " s"; };
    std::cout << "corpus    " << corpus.manifest.string() << " (" << p.corpus << ")\n"
              << "stream    " << stream.size() << " symbols of " << alpha.id() << " (canonicalised in " << secs(t1 - t0) << ")\n"
              << "model     order " << p.order << ", min count " << p.min_count << ": " << m.context_count()
              << " contexts (counted in " << secs(t2 - t1) << ")\n"
              << "wrote     " << a.get("out") << " (" << text.size() << " bytes)\n"
              << "sha256    " << m.sha256() << "\n"
              << "\nRegistry line for data/models/models.tsv (edit id and description; tab-separated):\n"
              << "my-model\t" << fs::path(a.get("out")).filename().string() << "\t" << alpha.id() << "\tno\t" << m.sha256()
              << "\tDescription of this model\n";
    return 0;
}

// ---------------------------------------------------------------- measure

int cmd_measure(const Args& a)
{
    const Alphabet& alpha = alphabet_of(a.get("alphabet", "lower27"));
    const LoadedModel lm = resolve_model(a.get("model"), alpha.id());
    if (!lm.model) throw std::invalid_argument("no model for alphabet " + alpha.id() + " (see: sieve models)");
    const uint32_t length = a.get_positive("length", 1000);
    const GuidedLine g(lm.model, length);

    std::vector<std::pair<std::string, std::string>> inputs; // (name, utf-8 text)
    for (const auto& f : a.positional)
    {
        std::ifstream in(f, std::ios::binary);
        if (!in) throw std::runtime_error("cannot read " + f);
        std::ostringstream ss;
        ss << in.rdbuf();
        inputs.emplace_back(f, ss.str());
    }
    if (inputs.empty())
    {
        // The corpus's held-out files: text the model has never seen.
        const Corpus corpus = load_corpus(default_corpus(a));
        const fs::path dir = a.get("texts", "corpus/gutenberg");
        const std::string role = a.get("role", "test");
        for (const auto& f : corpus.files)
            if (role == "all" || f.role == role) inputs.emplace_back(f.file, read_corpus_file(dir, f));
    }

    const double raw = std::log2(double(alpha.size()));
    std::cout << "model     " << lm.id << " (" << lm.model->sha256().substr(0, 16) << "..., order " << lm.model->order() << ")\n"
              << "raw       " << fixed(raw, 3) << " bits/symbol (every unit equally likely)\n"
              << "stream    bits/symbol coding each text as one long history\n"
              << "guided    bits/symbol of the guided addresses of its " << length << "-symbol units\n\n";
    std::printf("%-28s %10s %8s %8s %9s\n", "text", "symbols", "stream", "guided", "vs raw");
    uint64_t all_symbols = 0, all_bits = 0;
    double all_stream = 0;
    for (const auto& [name, text] : inputs)
    {
        const auto s = canonical_stream(text, alpha);
        if (s.empty()) continue;
        const double stream_bits = lm.model->stream_bits(s);
        uint64_t bits = 0;
        for (size_t i = 0; i < s.size(); i += length)
        {
            std::vector<uint32_t> unit(s.begin() + std::ptrdiff_t(i), s.begin() + std::ptrdiff_t(std::min(s.size(), i + length)));
            unit.resize(length, *alpha.digit_of(U' '));
            bits += g.code(unit).bits;
        }
        const double per = double(bits) / double(s.size());
        std::printf("%-28s %10zu %8.3f %8.3f %8.2fx\n", name.c_str(), s.size(), stream_bits / double(s.size()), per, raw / per);
        all_symbols += s.size();
        all_bits += bits;
        all_stream += stream_bits;
    }
    if (inputs.size() > 1 && all_symbols)
        std::printf("%-28s %10llu %8.3f %8.3f %8.2fx\n", "all", static_cast<unsigned long long>(all_symbols), all_stream / double(all_symbols),
                    double(all_bits) / double(all_symbols), raw / (double(all_bits) / double(all_symbols)));
    return 0;
}

// ---------------------------------------------------------------- filters

int cmd_check_book(const Args& a);
int cmd_filters_books(const Args& a);

int cmd_filters(const Args& a)
{
    if (a.get("line", "text") == "books") return cmd_filters_books(a);
    const Line line = make_line(a.has("length") || line_from_string(a.get("line", "text")) != LineKind::Text ? a : [&] {
        Args b = a;
        b.opts["length"] = "32";
        return b;
    }());
    const FilterConfig cfg = load_filter_config(a);
    const LineFilters& lf = cfg.of(line.kind);
    const FilterLine fl = filter_line(line);
    std::cout << "line         " << to_string(line.kind) << "  (" << line.space.id() << ")\n"
              << "settings     " << filters_path(a) << "\n"
              << "mode         " << to_string(lf.mode) << "\n\n";
    const auto list = filters_for(fl);
    if (list.empty()) std::cout << "No filters for this line yet.\n";
    for (const FilterSpec* f : list)
    {
        std::cout << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << "\n";
        print_indented(f->description, "      ");
        const auto vit = lf.values.find(f->name());
        for (const auto& p : f->params)
        {
            const std::string v = param_value(*f, vit == lf.values.end() ? FilterValues{} : vit->second, p.key);
            std::cout << "      " << p.key << " = " << (v.empty() ? "(default)" : v) << "   " << p.description << "\n";
        }
        if (!f->implies.empty())
        {
            std::cout << "      implies:";
            for (const auto& i : f->implies) std::cout << " " << i;
            std::cout << "\n";
        }
    }
    const FilterStack st = build_stack(line, lf);
    if (!st.empty())
    {
        std::cout << "\nstack        " << st.id().substr(0, 16) << "...  " << st.provenance() << "\n";
        if (st.ranker()) std::cout << "survivors    " << st.ranker()->count().to_decimal() << " (exact; compact mode available)\n";
        else std::cout << "compact      unavailable: " << st.compact_blocker() << "\n";
    }
    return 0;
}

int cmd_check(const Args& a)
{
    if (a.has("book")) return cmd_check_book(a);
    const Line line = make_line(a);
    const FilterConfig cfg = load_filter_config(a);
    const LineFilters& lf = cfg.of(line.kind);
    const WarpInput w = read_warp_input(line, a);
    // Every filter available for the line, with its settings, whether ticked or not.
    const FilterLine fl = filter_line(line);
    std::vector<std::pair<std::string, FilterStack>> each;
    for (const FilterSpec* f : filters_for(fl))
    {
        LineFilters one = lf;
        one.enabled = {f->name()};
        each.emplace_back(f->name(), build_stack(line, one));
    }
    const FilterStack st = build_stack(line, lf);
    print_header(line);
    std::cout << "canon        " << w.report.front() << "\n"
              << "stack        " << (st.empty() ? std::string("(none ticked)") : st.id().substr(0, 16) + "...  " + st.provenance()) << "\n";
    for (size_t i = 0; i < w.units.size(); ++i)
    {
        const auto& u = w.units[i];
        std::cout << "\nunit " << i + 1 << "/" << w.units.size() << "  " << preview(line, u) << "\n";
        for (const auto& [name, one] : each)
            std::cout << "  " << (lf.is_enabled(name) ? "[x] " : "[ ] ") << name << std::string(std::max<size_t>(1, 24 - name.size()), ' ')
                      << (one.passes(u) ? "pass" : "FAIL") << "\n";
        if (!st.empty())
        {
            const int fail = st.first_failure(u);
            std::cout << "  stack: " << (fail < 0 ? "passes" : "fails at " + st.filter_name(size_t(fail)));
            if (fail < 0 && st.ranker()) std::cout << ", survivor number " << st.ranker()->rank(u).to_decimal() << " of " << st.ranker()->count().to_decimal();
            std::cout << "\n";
        }
    }
    return 0;
}

// ---------------------------------------------------------------- books

std::string slurp(const std::string& path)
{
    std::ifstream in(std::filesystem::path(path), std::ios::binary);
    if (!in) throw std::runtime_error("cannot open '" + path + "'");
    return std::string(std::istreambuf_iterator<char>(in), {});
}

// The text of a section's units, joined, without the padding at the end.
std::string section_text(const DecodedSection& d)
{
    std::u32string t;
    for (const auto& u : d.units) t += d.line.space.text_of(u);
    while (!t.empty() && t.back() == U' ') t.pop_back();
    return utf8_encode(t);
}

int cmd_bind(const Args& a)
{
    if (!a.has("out")) throw std::invalid_argument("missing --out FILE.book");
    if (!a.has("pages") && !a.has("title") && !a.has("cover")) throw std::invalid_argument("give --title TEXT, --cover PICTURE and/or --pages FILE");
    const std::string mode = a.get("mode", "scrambled");
    const std::string key = a.get("key", "sieve");
    const uint32_t page_length = a.get_positive("length", 3200);
    Book b;
    auto text_shape = [&](uint32_t length) {
        Args s;
        s.opts = {{"line", "text"}, {"alphabet", a.get("alphabet", "lower27")}, {"length", std::to_string(length)},
                  {"canon", a.get("canon", "v2")}, {"key", key}};
        if (a.has("model")) s.opts["model"] = a.get("model");
        return s;
    };
    auto text_units = [&](const Args& shape, const Args& input) {
        Args full = shape;
        if (mode != "guided") full.opts["model"] = "none";
        const Line line = make_line(full);
        const WarpInput w = read_warp_input(line, input);
        std::cerr << "  " << w.report.front() << "\n";
        return w.units;
    };
    if (a.has("title"))
    {
        // A title is a page like any other, of its own length (default: the page length).
        const Args shape = text_shape(a.get_positive("title-length", page_length));
        Args in;
        in.positional = {a.get("title")};
        std::cerr << "title\n";
        b.sections.push_back(make_section("title", shape, mode, text_units(shape, in)));
    }
    if (a.has("cover"))
    {
        // A cover is an image. The image line has no guided ordering, so a guided book writes
        // its cover scrambled.
        Args shape;
        shape.opts = {{"line", "image"}, {"width", std::to_string(a.get_positive("cover-width", 10))},
                      {"height", std::to_string(a.get_positive("cover-height", 10))}, {"palette", a.get("cover-palette", "mono")},
                      {"key", key}, {"model", "none"}};
        Args in;
        in.opts["file"] = a.get("cover");
        const Line line = make_line(shape);
        const WarpInput w = read_warp_input(line, in);
        std::cerr << "cover\n  " << w.report.front() << "\n";
        b.sections.push_back(make_section("cover", shape, mode == "guided" ? "scrambled" : mode, w.units));
    }
    if (a.has("pages"))
    {
        const Args shape = text_shape(page_length);
        Args in;
        in.opts["file"] = a.get("pages");
        std::cerr << "pages\n";
        b.sections.push_back(make_section("pages", shape, mode, text_units(shape, in)));
    }
    // Read it back before writing: every address must decode to its unit.
    const auto decoded = decode_book(b);
    b.id = book_id(decoded);
    const std::string record = serialise_book(b);
    // And read back the very text that will be written (a key with a line break, say, would not).
    if (book_id(decode_book(parse_book(record))) != b.id) throw std::runtime_error("the record does not read back as the same book");
    std::ofstream out(std::filesystem::path(a.get("out")), std::ios::binary);
    out << record;
    if (!out) throw std::runtime_error("cannot write " + a.get("out"));
    size_t chars = 0;
    for (const auto& d : decoded)
        if (d.line.kind == LineKind::Text) chars += d.units.size() * d.line.space.unit_length();
    std::cout << "book         " << b.id << "\n";
    for (const auto& d : decoded)
        std::cout << "  " << d.section->role << std::string(std::max<size_t>(1, 11 - d.section->role.size()), ' ') << d.units.size()
                  << " unit(s) on " << d.line.space.id() << ", " << d.section->mode << "\n";
    std::cout << "record       " << record.size() << " bytes -> " << a.get("out") << "\n";
    return 0;
}

// The books line's lines: the cover (image) line and the page (text) line, from the options.
std::pair<Line, Line> book_lines(const Args& a)
{
    Args c = a, t = a;
    c.opts["line"] = "image";
    c.opts["model"] = "none";
    t.opts["line"] = "text";
    if (!t.has("length")) t.opts["length"] = "3200";
    return {make_line(c), make_line(t)};
}

void print_book_stacks(const BookStacks& st, const BookSieve& sieve)
{
    const char* names[3] = {"cover", "title", "pages"};
    const FilterStack* parts[3] = {&st.cover, &st.title, &st.pages};
    for (int i = 0; i < 3; ++i)
    {
        const FilterStack& s = *parts[i];
        std::cout << names[i] << std::string(13 - std::string(names[i]).size(), ' ')
                  << (s.empty() ? std::string("(none ticked)") : s.id().substr(0, 16) + "...  " + s.provenance()) << "\n";
    }
    if (sieve.empty()) return;
    if (sieve.can_rank()) std::cout << "survivors    " << sieve.count().to_decimal() << " books (exact; compact mode available)\n";
    else std::cout << "compact      unavailable: " << sieve.blocker() << "\n";
}

int cmd_filters_books(const Args& a)
{
    const auto [cover, page] = book_lines(a);
    const uint32_t pages = a.has("book-pages") ? a.get_positive("book-pages", 4) : 4;
    const FilterConfig cfg = load_filter_config(a);
    const BookSpace space(cover.space, page.space, pages);
    std::cout << "line         books  (" << space.id() << ")\n"
              << "settings     " << filters_path(a) << "\n"
              << "mode         " << to_string(cfg.books.mode) << "\n";
    const char* heads[3] = {"COVER: a picture of the image line", "TITLE: one page of the text line",
                            "PAGES: all pages read as one text"};
    for (int i = 0; i < 3; ++i)
    {
        FilterLine fl = filter_line(i == 0 ? cover : page);
        if (i == 2) fl.length = uint32_t(std::min<uint64_t>(uint64_t(fl.length) * pages, UINT32_MAX));
        const LineFilters& lf = cfg.books.parts[i];
        std::cout << "\n" << heads[i] << " (" << fl.length << " symbols)\n";
        for (const FilterSpec* f : filters_for(fl)) std::cout << "  " << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << "\n";
    }
    const BookStacks st = build_book_stacks(cover, page, pages, cfg.books);
    const BookSieve sieve(space, st.cover, st.title, st.pages);
    std::cout << "\n";
    print_book_stacks(st, sieve);
    return 0;
}

// A book record judged by the books line's filters. The line takes the record's own shape: its
// cover's picture, its pages' length, and as many pages as it has (or --book-pages).
int cmd_check_book(const Args& a)
{
    const Book b = parse_book(slurp(a.get("book")));
    const auto decoded = decode_book(b);
    if (book_id(decoded) != b.id) throw std::runtime_error("the book's content does not match its id (" + b.id + ")");
    auto [cover, page] = book_lines(a);
    size_t record_pages = 0;
    bool have_pages = false;
    for (const auto& d : decoded)
    {
        if (d.section->role == "cover") cover = d.line;
        else if (d.section->role == "pages")
        {
            page = d.line; // the pages set the page shape (a title may be bound shorter)
            record_pages = d.units.size();
            have_pages = true;
        }
        else if (d.section->role == "title" && !have_pages) page = d.line;
    }
    const uint32_t pages = a.has("book-pages") ? a.get_positive("book-pages", 4) : uint32_t(record_pages);
    const FilterConfig cfg = load_filter_config(a);
    const BookSpace space(cover.space, page.space, pages);
    const BookSpace::Parts parts = record_parts(decoded, space);
    const BookStacks st = build_book_stacks(cover, page, pages, cfg.books);
    const BookSieve sieve(space, st.cover, st.title, st.pages);
    std::cout << "book         " << b.id << "\n"
              << "line         " << space.id() << "\n"
              << "settings     " << filters_path(a) << "\n";
    print_book_stacks(st, sieve);
    // Every filter each part is offered, ticked or not, with the part's settings.
    const char* names[3] = {"cover", "title", "pages"};
    const std::vector<uint32_t> body = BookSieve::body(parts);
    const std::vector<uint32_t>* units[3] = {&parts.cover, &parts.title, &body};
    for (int i = 0; i < 3; ++i)
    {
        FilterLine fl = filter_line(i == 0 ? cover : page);
        if (i == 2) fl.length = uint32_t(body.size());
        if (fl.length == 0) continue;
        std::cout << "\n" << names[i] << "\n";
        for (const FilterSpec* f : filters_for(fl))
        {
            LineFilters one = cfg.books.parts[i];
            one.enabled = {f->name()};
            const FilterStack s = build_stack(fl, one);
            std::cout << "  " << (cfg.books.parts[i].is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name()
                      << std::string(std::max<size_t>(1, 24 - f->name().size()), ' ') << (s.passes(*units[i]) ? "pass" : "FAIL") << "\n";
        }
    }
    const std::string fail = sieve.first_failure(parts);
    std::cout << "\nbook: " << (sieve.empty() ? std::string("no filters ticked") : fail.empty() ? std::string("passes") : "fails at " + fail);
    if (!sieve.empty() && fail.empty() && sieve.can_rank())
    {
        const BigUint k = sieve.rank(parts); // ranked once; both compact addresses follow from k
        std::cout << ", survivor number " << k.to_decimal() << " of " << sieve.count().to_decimal() << "\n"
                  << "compact      positional " << sieve.hex_of(sieve.index_of_rank(k, AddressMode::Positional)) << "\n"
                  << "             scrambled  " << sieve.hex_of(sieve.index_of_rank(k, AddressMode::Scrambled));
    }
    std::cout << "\n";
    return 0;
}

int cmd_unbind(const Args& a)
{
    if (a.positional.size() != 1) throw std::invalid_argument("give one BOOK file");
    const Book b = parse_book(slurp(a.positional[0]));
    const auto decoded = decode_book(b);
    const std::string id = book_id(decoded);
    if (id != b.id) throw std::runtime_error("the book's content does not match its id (" + b.id + ")");
    std::cerr << "book " << id << " (id checked)\n";
    for (const auto& d : decoded)
    {
        const std::string& role = d.section->role;
        std::cerr << "  " << role << ": " << d.units.size() << " unit(s) on " << d.line.space.id() << ", " << d.section->mode << "\n";
        if (d.line.kind == LineKind::Text)
        {
            const std::string text = section_text(d);
            if (role == "pages" && a.has("pages"))
            {
                std::ofstream out(std::filesystem::path(a.get("pages")), std::ios::binary);
                out << text << "\n";
                if (!out) throw std::runtime_error("cannot write " + a.get("pages"));
                std::cerr << "  saved " << a.get("pages") << "\n";
            }
            else std::cout << text << "\n";
        }
        else
        {
            if (role == "cover" && a.has("cover") && d.units.empty()) std::cerr << "  the cover section is empty: nothing to save\n";
            else if (role == "cover" && a.has("cover"))
            {
                save_unit(d.line, d.units.front(), a.get("cover"), a.get_positive("scale", 16));
                std::cerr << "  saved " << a.get("cover") << "\n";
            }
            else
                for (const auto& u : d.units) print_indented(preview(d.line, u), "");
        }
    }
    return 0;
}

bool is_command(const std::string& name)
{
    return name == "info" || name == "warp" || name == "read" || name == "browse" || name == "sift" || name == "sieve" || name == "dicts" || name == "alphabets" || name == "mesh" ||
           name == "version" || name == "models" || name == "train" || name == "measure" ||
           name == "filters" || name == "check" || name == "bind" || name == "unbind";
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
        {
            print_usage();
            return 1;
        }
        const std::string first = argv[1];
        if (first == "--version") return cmd_version();
        if (first == "help" || first == "--help" || first == "-h")
        {
            if (argc >= 3)
            {
                if (print_help(argv[2])) return 0;
                std::cerr << "no help for '" << argv[2] << "'\n\n";
                print_usage();
                return 1;
            }
            print_usage();
            return 0;
        }
        const Args a = parse_args(argc, argv);
        if (a.help && print_help(a.command == "sieve" ? "sift" : a.command)) return 0;
        // A mistyped option would otherwise be ignored without a word.
        {
            const auto known = documented_options(a.command == "sieve" ? "sift" : a.command);
            if (!known.empty())
                for (const auto& [key, value] : a.opts)
                    if (std::find(known.begin(), known.end(), key) == known.end())
                        std::cerr << "warning: --" << key << " is not an option of 'sieve " << a.command << "' (see: sieve help " << a.command
                                  << "); ignored\n";
        }
        if (a.command == "info") return cmd_info(a);
        if (a.command == "warp") return cmd_warp(a);
        if (a.command == "read") return cmd_read(a);
        if (a.command == "browse") return cmd_browse(a);
        if (a.command == "sift" || a.command == "sieve") return cmd_sieve(a); // "sieve" kept as an alias
        if (a.command == "dicts") return cmd_dicts(a);
        if (a.command == "alphabets") return cmd_alphabets(a);
        if (a.command == "mesh") return cmd_mesh(a);
        if (a.command == "version") return cmd_version();
        if (a.command == "models") return cmd_models();
        if (a.command == "train") return cmd_train(a);
        if (a.command == "measure") return cmd_measure(a);
        if (a.command == "filters") return cmd_filters(a);
        if (a.command == "check") return cmd_check(a);
        if (a.command == "bind") return cmd_bind(a);
        if (a.command == "unbind") return cmd_unbind(a);
        std::cerr << "unknown command '" << a.command << "'\n\n";
        print_usage();
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        if (argc >= 2 && is_command(argv[1]))
            std::cerr << "(see: sieve help " << (std::string(argv[1]) == "sieve" ? "sift" : argv[1]) << ")\n";
        return 1;
    }
}
