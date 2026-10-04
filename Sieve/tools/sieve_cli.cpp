// Sieve — command-line interface to the core.
//
//   sieve info   [--line LINE] [line options] [--key K]
//   sieve warp   [--line LINE] [line options] [--key K] [--mode MODE] [--short] (TEXT... | --file PATH)
//   sieve read   [--line LINE] [line options] [--key K] --mode MODE [--out PATH] [--scale S] ADDRESS
//   sieve browse [--line LINE] [line options] [--key K] [--count N] [--short]
//   sieve read   ... ADDRESS --around N      (the N units either side)
//   sieve dicts  [--hash FILE]
//   sieve alphabets [--spec SPEC]
//   sieve mesh   [--vertices V] [--faces F] [--coords C] [--warp FILE [--canonical] | --read ADDR | --browse N]
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
#include "cli/compare.hpp"
#include "cli/dictionaries.hpp"
#include "cli/help.hpp"
#include "cli/image_io.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/locate.hpp"
#include "cli/vault.hpp"
#include "cli/vault_decode.hpp"
#include "cli/plugins.hpp"
#include "cli/timings.hpp"
#include "sieve/dfa.hpp"
#include "sieve/plugin.hpp"
#include "sieve/chunks.hpp"
#include "cli/map.hpp"
#include "cli/models.hpp"

#include "sieve/booksieve.hpp"
#include "sieve/audio.hpp"
#include "sieve/compact.hpp"
#include "sieve/filekind.hpp"
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
#include <set>
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
                  << " tiles of " << sieve::books_per_tile() << " books; "
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
    if (slot >= sieve::books_per_tile()) throw std::invalid_argument("SLOT must be 0.." + std::to_string(sieve::books_per_tile() - 1));
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
    if (line.kind == LineKind::Text) std::cout << (unit_withheld(line, digits) ? std::string("(withheld)") : utf8_encode(line.space.text_of(digits))) << "\n";
    else std::cout << preview(line, digits) << "\n";
    if (a.has("out"))
    {
        save_unit(line, digits, a.get("out"), a.get_positive("scale", 16));
        std::cerr << "saved " << a.get("out") << "\n";
    }
    return 0;
}

std::string slurp(const std::string& path); // defined below, with the book commands

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
        if (line.kind == LineKind::Text) std::cout << (unit_withheld(line, digits) ? std::string("(withheld)") : utf8_encode(line.space.text_of(digits))) << "\n";
        else std::cout << preview(line, digits) << "\n";
        return 0;
    }
    // An address of a file-sized unit is far too long for a command line -- a 100 KB file on a
    // bytes256 line has a 200,000 digit address -- so it can be given in a file instead.
    std::string given;
    if (a.has("address-file"))
    {
        if (!a.positional.empty()) throw std::invalid_argument("give an ADDRESS or --address-file PATH, not both");
        // All whitespace goes, not just the trailing newline: an address of forty thousand digits
        // is the sort of thing an editor or a pipe through `fold` will have wrapped, and a line
        // break in the middle of it is not a different address.
        const std::string raw = slurp(a.get("address-file"));
        given.reserve(raw.size());
        for (char c : raw)
            if (c != '\n' && c != '\r' && c != ' ' && c != '\t') given.push_back(c);
    }
    else
    {
        if (a.positional.size() != 1) throw std::invalid_argument("give exactly one ADDRESS (or --address-file PATH)");
        given = a.positional[0];
    }
    const auto compact = a.has("compact") ? make_compact(line, a) : nullptr;
    std::vector<uint32_t> digits;
    if (compact && mode != "guided")
    {
        // A compact address: the survivor number (positional) or its shuffle (scrambled).
        const CompactLine& cl = *compact->line;
        const AddressMode m = address_mode_from_string(mode);
        const BigUint index = cl.parse(given);
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
        const BigUint point = g.point_of(given);
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
        const auto address = line.space.parse_address(given);
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
        if (line.kind == LineKind::Text) std::cout << (unit_withheld(line, digits) ? std::string("(withheld)") : utf8_encode(line.space.text_of(digits))) << "\n";
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
              << "  note canonicalisation   " << kNotesCanonVersion << " (notes104), " << kNotes2CanonVersion << " (notes2)\n"
              << "  alphabets               ";
    for (const auto& id : alphabet_ids()) std::cout << id << " ";
    std::cout << "\n  palettes                ";
    for (const auto& id : palette_ids()) std::cout << id << " ";
    std::cout << "\n  note symbols            " << kNotesSymbolsId << ", notes2/LOW-HIGH/DURATIONS/VN (e.g. " << make_note_set(48, 84, kNoteDurationCodes, 1).id() << ")\n"
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
        vault::check_bytes(std::vector<uint8_t>(obj.begin(), obj.end()), "the model"); // the vault
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
        ModelSpace::Fitted fit = sp.fit(mesh.verts, mesh.faces);
        // --canonical: the one encoding canonical-mesh-v1 keeps (the same mesh, reordered).
        std::string canonical_note;
        if (a.has("canonical"))
        {
            if (const auto c = canonical_mesh(sp, fit.parts))
            {
                canonical_note = c->verts == fit.parts.verts && c->faces == fit.parts.faces ? "already canonical" : "vertices and faces reordered";
                fit.parts = *c;
            }
            else canonical_note = "none (two vertices on one grid point, a face naming a vertex twice, or a face twice): left as fitted";
        }
        {
            const std::string obj = sp.to_obj(fit.parts);
            vault::check_bytes(std::vector<uint8_t>(obj.begin(), obj.end()), "the model given"); // the vault
        }
        shape();
        std::cout << "fitted       " << mesh.verts.size() << " vertices in, " << mesh.faces.size() << " triangles in; scaled by "
                  << fixed(double(fit.scale), 4) << "\n";
        if (fit.vertices_dropped) std::cout << "             " << fit.vertices_dropped << " vertices dropped (past --vertices)\n";
        if (fit.faces_dropped) std::cout << "             " << fit.faces_dropped << " faces dropped (past --faces)\n";
        if (fit.faces_added) std::cout << "             " << fit.faces_added << " filler faces added\n";
        if (fit.indices_clamped) std::cout << "             " << fit.indices_clamped << " indices clamped into range\n";
        if (!canonical_note.empty()) std::cout << "canonical    " << canonical_note << "\n";
        for (const char* m : {"positional", "scrambled"})
        {
            const BigUint k = sp.index_of(fit.parts, address_mode_from_string(m));
            std::cout << "  " << std::setw(11) << std::left << m << show_address(sp.hex_of(k), a.has("short")) << "\n";
        }
        // The models line's filters ([models] in the settings), when any are ticked.
        const ModelSieve ms = build_model_sieve(sp, load_filter_config(a).models);
        if (!ms.empty())
        {
            const BigUint k = sp.index_of(fit.parts, AddressMode::Positional);
            const std::string fail = ms.first_failure(k);
            if (!fail.empty()) std::cout << "stack: fails at " << fail << "\n";
            else if (!ms.can_rank()) std::cout << "stack: passes (compact unavailable: " << ms.blocker() << ")\n";
            else
            {
                std::cout << "stack: passes, survivor number " << ms.index_of(k, AddressMode::Positional).to_decimal() << "\n";
                for (const auto m : {AddressMode::Positional, AddressMode::Scrambled})
                    std::cout << "  compact " << std::setw(11) << std::left << to_string(m) << show_address(ms.hex_of(ms.index_of(k, m)), a.has("short")) << "\n";
            }
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
            const ModelSpace::Parts p = sp.parts_at(k, m);
            const std::string obj = sp.to_obj(p);
            if (vault::withheld_bytes(std::vector<uint8_t>(obj.begin(), obj.end()))) { std::cout << "(withheld)\n"; continue; }
            std::cout << show_address(sp.hex_of(k), a.has("short")) << "\n" << mesh_preview(sp, p);
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
                  << " tiles of " << sieve::books_per_tile() << " models\n";
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
    return (install_dir() / "models" / "corpus" / "gutenberg-nltk.tsv").string();
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

// --params "a=1,b=2" into values.
FilterValues parse_params(const std::string& s)
{
    FilterValues v;
    size_t at = 0;
    while (at < s.size())
    {
        const size_t comma = s.find(',', at);
        const std::string item = s.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
        const size_t eq = item.find('=');
        if (eq == std::string::npos || eq == 0) throw std::invalid_argument("--params expects NAME=VALUE, commas between: '" + item + "'");
        v[item.substr(0, eq)] = item.substr(eq + 1);
        if (comma == std::string::npos) break;
        at = comma + 1;
    }
    return v;
}

// What a plugin's rule is to every other custom filter for the same line, at their default
// settings: the same (a duplicate), stricter (it implies the other), looser (implied by it), or
// neither. Exact, over every length (sieve/dfa.hpp: subset).
void print_relations(const std::string& self_name, const Dfa& d, const FilterLine& fl)
{
    bool any = false;
    for (const FilterSpec& other : plugin_registry())
    {
        if (other.name() == self_name || !other.applies(fl)) continue;
        std::unique_ptr<Filter> f;
        try
        {
            static const AppResources resources;
            f = other.make(fl, {}, resources);
        }
        catch (const std::exception&)
        {
            continue;
        }
        const Dfa* o = plugin_dfa(*f);
        if (!o) continue;
        const bool in = subset(d, *o), out = subset(*o, d);
        const char* what = in && out ? "the same rule as" : in ? "stricter than (implies)" : out ? "looser than (implied by)" : nullptr;
        if (!what) continue;
        std::cout << "relation   " << what << " " << other.name() << (in && out ? "  -- a duplicate" : "") << "\n";
        any = true;
    }
    if (!any) std::cout << "relation   none: no other custom filter for this line is the same, stricter or looser\n";
}

// One plugin file: its header, its automaton and, at the line's length, what it keeps. The same
// report as the oracle's `sieve_ref.py plugin`, line for line, so CI can compare them.
int cmd_filters_plugin(const Args& a)
{
    const std::string file = a.get("plugin");
    const std::vector<uint8_t> bytes = read_file_bytes(std::filesystem::path(std::u8string(file.begin(), file.end())));
    const std::string text(bytes.begin(), bytes.end());
    const std::string sha = Sha256::hex(Sha256::hash(text));
    const std::filesystem::path file_path(std::u8string(file.begin(), file.end()));
    const std::u8string folder = file_path.parent_path().u8string();
    const auto def = parse_plugin(text, sha, std::string(folder.begin(), folder.end()));
    const PluginHeader& h = plugin_header(*def);
    // The line: as given, or the one the plugin's symbols name.
    Args b = a;
    if (!a.has("line"))
    {
        if (h.symbols == "notes104" || h.symbols == "notes*") b.opts["line"] = "audio"; // notes*: every note line (v2)
        else if (h.symbols.rfind("palette:", 0) == 0)
        {
            b.opts["line"] = "image";
            b.opts["palette"] = h.symbols.substr(8);
        }
        else if (h.symbols != "any" && !a.has("alphabet")) b.opts["alphabet"] = h.symbols;
    }
    const LineKind kind = line_from_string(b.get("line", "text"));
    if (kind == LineKind::Audio && a.has("length")) b.opts["notes"] = a.get("length"); // a melody's length is its notes
    if (!b.has("length") && kind == LineKind::Text) b.opts["length"] = "32";
    const Line line = make_line(b);
    const FilterLine fl = filter_line(line);
    const FilterValues values = a.has("params") ? parse_params(a.get("params")) : FilterValues{};
    for (const auto& [k, v] : values)
        if (std::none_of(h.params.begin(), h.params.end(), [&](const FilterParam& p) { return p.key == k; }))
            throw std::invalid_argument(h.name() + " has no parameter '" + k + "'");
    size_t declared = 0;
    static const AppResources resources;
    std::string data;
    // A note line of several voices: the plugin judges each voice, as in a stack (FilterStack), so
    // it is compiled for one voice's line and counted through the voices.
    uint32_t voices = 1;
    FilterLine one = fl;
    if (fl.kind == "audio" && is_note_symbols(fl.symbols_id))
    {
        voices = note_set_of(fl.symbols_id).voices;
        one.length = fl.length / voices;
    }
    const Dfa d = compile_plugin(*def, one, values, resources, &declared, &data);
    std::string params;
    for (const auto& p : h.params)
    {
        const auto it = values.find(p.key);
        params += (params.empty() ? "" : " ") + p.key + "=" + (it == values.end() ? p.default_value : it->second);
    }
    std::cout << "plugin     " << h.name() << "\n"
              << "sha256     " << sha << "\n"
              << "origin     " << h.origin << "\n"
              << "author     " << h.author << "\n"
              << "symbols    " << h.symbols << "\n"
              << "params     " << (params.empty() ? "(none)" : params) << "\n";
    std::string reqs;
    for (const auto& r : h.prerequisites)
    {
        reqs += (reqs.empty() ? "" : "; ") + r.name;
        for (const auto& [k, v] : r.values) reqs += " " + k + "=" + v;
    }
    std::cout << "requires   " << (reqs.empty() ? "(none)" : reqs) << "\n"
              << "form       " << h.form << (data.empty() ? std::string() : "  " + data) << "\n"
              << "states     " << (h.form == "tokens" ? std::to_string(declared) + " words, " : std::to_string(declared) + " declared, ") << d.states()
              << " minimal\n"
              << "length     " << fl.length << "\n";
    if (a.has("relations")) print_relations(h.name(), d, fl);
    // --judge FILE: each line of a text file judged as one unit of its own length (the oracle's
    // `plugin --judge` prints the same, so CI compares verdicts on real and shuffled sentences).
    auto judge_lines = [&]() {
        if (!a.has("judge")) return;
        if (!fl.alphabet) throw std::invalid_argument("--judge reads text: the plugin is not for a text line");
        const std::string jf = a.get("judge");
        std::ifstream in(std::filesystem::path(std::u8string(jf.begin(), jf.end())), std::ios::binary);
        if (!in) throw std::invalid_argument("cannot read " + jf);
        std::string l;
        while (std::getline(in, l))
        {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            if (l.empty() || l[0] == '#') continue; // a comment
            std::vector<uint32_t> u;
            bool spelled = true;
            for (char32_t cp : utf8_decode(l))
            {
                const auto dg = fl.alphabet->digit_of(cp);
                if (!dg) { spelled = false; break; }
                u.push_back(*dg);
            }
            std::cout << "judge      " << (!spelled ? "unspellable" : d.accepts(u) ? "pass" : "FAIL") << "  " << l << "\n";
        }
    };
    if (DfaRanker::table_bytes(d.states(), d.base, one.length) > filter_memory())
    {
        std::cout << "survivors  (judge only: " << over_table_limit("the table") << ")\n";
        judge_lines();
        return 0;
    }
    const DfaRanker one_voice(d, one.length);
    const std::unique_ptr<Ranker> all_voices = voices > 1 ? voices_ranker(one_voice, voices) : nullptr;
    const Ranker& r = all_voices ? *all_voices : static_cast<const Ranker&>(one_voice);
    BigUint excluded = BigUint::pow(fl.base, fl.length);
    excluded -= r.count();
    std::cout << "survivors  " << r.count().to_decimal() << "\n"
              << "excluded   " << excluded.to_decimal() << "\n";
    if (!r.count().is_zero())
    {
        BigUint third = r.count();
        third.divmod_small(3);
        BigUint last = r.count();
        last -= BigUint(1);
        std::vector<BigUint> ks{BigUint(0)};
        if (!(third == BigUint(0)) && !(third == last)) ks.push_back(third);
        if (!(last == BigUint(0))) ks.push_back(last);
        for (const BigUint& k : ks)
        {
            std::string digits;
            for (uint32_t c : r.unrank(k)) digits += (digits.empty() ? "" : ",") + std::to_string(c);
            std::cout << "rank       " << k.to_decimal() << "  " << (digits.empty() ? "-" : digits) << "\n";
        }
    }
    judge_lines();
    return 0;
}


// The binary line's filters (binary-kind-v1), its sieve, and what each kind is of the line: the
// calculator for where the binary line's files stand, exact at any length.
// The other lines, as not-an-item-v1 sees them from the command line: the pages line of --alphabet
// and --page-length (default lower27, 32), and the other lines at their default shapes.
struct OtherLines
{
    static Line line(const Args& a, const char* kind)
    {
        Args b;
        b.opts["line"] = kind;
        b.opts["model"] = "none";
        if (std::string(kind) == "text")
        {
            b.opts["alphabet"] = a.get("alphabet", "lower27");
            b.opts["length"] = a.get("page-length", "32");
        }
        return make_line(b);
    }
    Line pages, image, video, audio;
    ModelSpace models{8, 12, 16};
    BinaryItems items;
    explicit OtherLines(const Args& a)
        : pages(line(a, "text")), image(line(a, "image")), video(line(a, "video")), audio(line(a, "audio"))
    {
        items = binary_items(&pages, &image, &video, &audio, &models);
    }
};

// The models line's filters: what it offers, what is ticked ([models] in the settings), and the
// stack's survivors, exact (sieve/modelsieve.hpp).
int cmd_filters_models(const Args& a)
{
    const ModelSpace space = make_model_space(a);
    const FilterConfig cfg = load_filter_config(a);
    const LineFilters& lf = cfg.models;
    const FilterLine fl = models_filter_line(space.vertices(), space.face_count(), space.coords());
    std::cout << "line         models  (" << space.id() << ")\n"
              << "settings     " << filters_path(a) << "\n"
              << "mode         " << to_string(lf.mode) << "\n\n";
    for (const FilterSpec* f : filters_for(fl))
    {
        std::cout << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << (f->category.empty() ? "" : "   (" + f->category + ")") << "\n";
        print_indented(f->description, "      ");
    }
    const ModelSieve ms = build_model_sieve(space, lf);
    if (ms.empty()) return 0;
    if (!ms.can_rank())
    {
        std::cout << "\nstack        " << ms.id().substr(0, 16) << "...  " << ms.provenance() << "\n"
                  << "compact      unavailable: " << ms.blocker() << "\n";
        return 0;
    }
    const BigUint& all = space.size();
    auto share = [&](const BigUint& part) {
        if (part.is_zero()) return std::string("none");
        std::ostringstream o;
        const double x = part.log10_approx() - all.log10_approx();
        if (x > -0.005) return std::string("about all");
        o << "10^" << std::fixed << std::setprecision(2) << x;
        return o.str();
    };
    BigUint excluded = all;
    excluded -= ms.count();
    std::cout << "\nstack        " << ms.id().substr(0, 16) << "...  " << ms.provenance() << "\n"
              << "survivors    " << ms.count().to_decimal() << " (exact; compact mode available)\n"
              << "excluded     " << excluded.to_decimal() << " (exact)\n"
              << "share        the stack keeps " << share(ms.count()) << " of the line and sets aside " << share(excluded) << "\n";
    return 0;
}

int cmd_filters_binary(const Args& a)
{
    const uint64_t n = a.has("length") ? std::stoull(a.get("length")) : 32;
    const BinarySpace space(std::max<uint64_t>(1, n), a.get("key", "sieve"));
    const FilterConfig cfg = load_filter_config(a);
    const LineFilters& lf = cfg.binary;
    const OtherLines others(a);
    const FilterLine fl = binary_filter_line(space.max_bytes());
    std::cout << "line         binary  (" << space.id() << ")\n"
              << "settings     " << filters_path(a) << "\n"
              << "mode         " << to_string(lf.mode) << "\n\n";
    for (const FilterSpec* f : filters_for(fl))
    {
        std::cout << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << (f->category.empty() ? "" : "   (" + f->category + ")") << "\n";
        print_indented(f->description, "      ");
        const auto vit = lf.values.find(f->name());
        for (const auto& p : f->params)
            std::cout << "      " << p.key << " = " << param_value(*f, vit == lf.values.end() ? FilterValues{} : vit->second, p.key) << "   "
                      << p.description << "\n";
    }
    const BigUint& all = space.size();
    auto share = [&](const BigUint& part) {
        if (part.is_zero()) return std::string("none");
        std::ostringstream o;
        const double x = part.log10_approx() - all.log10_approx();
        if (x > -0.005) return std::string("about all");
        o << "10^" << std::fixed << std::setprecision(2) << x;
        return o.str();
    };
    const BinarySieve bs = build_binary_sieve(space, lf, &others.items);
    if (!bs.empty() && !bs.can_rank())
        std::cout << "\nstack        " << bs.id().substr(0, 16) << "...  " << bs.provenance() << "\n"
                  << "compact      unavailable: " << bs.blocker() << "\n";
    else if (!bs.empty())
    {
        BigUint excluded = all;
        excluded -= bs.count();
        std::cout << "\nstack        " << bs.id().substr(0, 16) << "...  " << bs.provenance() << "\n"
                  << "survivors    " << bs.count().to_decimal() << " (exact; compact mode available)\n"
                  << "excluded     " << excluded.to_decimal() << " (exact)\n"
                  << "share        the stack keeps " << share(bs.count()) << " of the line and sets aside " << share(excluded) << "\n";
    }
    // Every kind's share of the line (display: its power of ten; --exact for the counts).
    std::cout << "\nkinds        every file of 0.." << space.max_bytes() << " bytes by " << kFileKindsVersion << ", of "
              << all.to_decimal().size() << "-digit total\n";
    const auto& kinds = file_kinds();
    for (size_t i = 0; i < kinds.size(); ++i)
    {
        KindSet one(kinds.size(), 0);
        one[i] = 1;
        const KindCounter k(space.max_bytes(), one);
        std::cout << "  " << std::left << std::setw(10) << kinds[i] << std::right << std::setw(12) << share(k.count());
        if (a.has("exact")) std::cout << "  " << k.count().to_decimal();
        std::cout << "\n";
    }
    return 0;
}

// A file on the binary line: its kind, each binary filter's verdict, and its survivor number and
// compact addresses where the stack keeps it.
int cmd_check_binary(const Args& a)
{
    if (!a.has("file")) throw std::invalid_argument("check --line binary needs --file PATH");
    const std::string file = a.get("file");
    const auto bytes = read_file_bytes(std::filesystem::path(std::u8string(file.begin(), file.end())));
    vault::check_bytes(bytes, file); // the vault: a withheld file is never placed
    const uint64_t n = a.has("length") ? std::stoull(a.get("length")) : std::max<uint64_t>(32, bytes.size());
    if (bytes.size() > n) throw std::invalid_argument("the file is " + std::to_string(bytes.size()) + " bytes, longer than the line's " + std::to_string(n));
    const BinarySpace space(std::max<uint64_t>(1, n), a.get("key", "sieve"));
    const FilterConfig cfg = load_filter_config(a);
    const FilterLine fl = binary_filter_line(space.max_bytes());
    const OtherLines others(a);
    const BinarySieve bs = build_binary_sieve(space, cfg.binary, &others.items);
    std::cout << "line         binary  (" << space.id() << ")\n"
              << "file         " << file << ", " << bytes.size() << " bytes\n"
              << "kind         " << file_kind(bytes, bytes.size()) << " (" << kFileKindsVersion << ")\n"
              << "stack        " << (bs.empty() ? std::string("(none ticked)") : bs.id().substr(0, 16) + "...  " + bs.provenance()) << "\n";
    for (const FilterSpec* f : filters_for(fl))
    {
        LineFilters one = cfg.binary;
        one.enabled = {f->name()};
        const BinarySieve single = build_binary_sieve(space, one, &others.items);
        std::cout << "  " << (cfg.binary.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name()
                  << std::string(std::max<size_t>(1, 24 - f->name().size()), ' ') << (single.first_failure_of(bytes).empty() ? "pass" : "FAIL") << "\n";
    }
    if (!bs.empty())
    {
        const std::string fail = bs.first_failure_of(bytes);
        std::cout << "  stack: " << (fail.empty() ? "passes" : "fails at " + fail);
        if (fail.empty() && bs.can_rank())
            std::cout << ", survivor number " << bs.index_of(bytes, AddressMode::Positional).to_decimal() << " of " << bs.count().to_decimal()
                      << "\n  compact      positional " << bs.hex_of(bs.index_of(bytes, AddressMode::Positional)) << "\n"
                      << "               scrambled  " << bs.hex_of(bs.index_of(bytes, AddressMode::Scrambled));
        std::cout << "\n";
    }
    return 0;
}

int cmd_filters(const Args& a)
{
    if (a.has("plugin")) return cmd_filters_plugin(a);
    if (a.has("plugins"))
    {
        const auto& list = load_plugins();
        if (list.empty()) std::cout << "No plugin files found (a filters folder beside the programs, or data/filters).\n";
        for (const PluginFile& p : list)
            std::cout << (p.error.empty() ? "loaded   " : "REFUSED  ") << (p.name.empty() ? std::string("?") : p.name) << "  "
                      << (p.sha256.empty() ? std::string("-") : p.sha256.substr(0, 16)) << "  " << p.path
                      << (p.error.empty() ? std::string() : "\n         " + p.error) << "\n";
        return 0;
    }
    if (a.get("line", "text") == "books") return cmd_filters_books(a);
    if (a.get("line", "text") == "binary") return cmd_filters_binary(a);
    if (a.get("line", "text") == "models") return cmd_filters_models(a);
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
        std::cout << (lf.is_enabled(f->name()) ? "[x] " : "[ ] ") << f->name() << (f->category.empty() ? "" : "   (" + f->category + ")")
                  << (f->plugin_sha256.empty() ? "" : "   (custom)") << (!f->retired ? "" : f->replaced_by.empty() ? "   (retired: judges only)" : "   (retired: replaced by " + f->replaced_by + ")") << "\n";
        print_indented(f->description, "      ");
        if (!f->plugin_sha256.empty())
        {
            std::cout << "      by " << f->author << " (" << f->origin << "), file " << f->plugin_sha256.substr(0, 12) << "\n";
            for (const auto& r : f->prerequisites)
            {
                std::cout << "      requires " << r.name;
                for (const auto& [k, v] : r.values) std::cout << " " << k << "=" << v;
                std::cout << " (ticked with it)\n";
            }
        }
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
    for (const std::string& n : prerequisite_notes(lf)) std::cout << "\nnote         " << n;
    const FilterStack st = build_stack(line, lf);
    if (!st.empty())
    {
        std::cout << "\nstack        " << st.id().substr(0, 16) << "...  " << st.provenance() << "\n";
        if (st.ranker())
        {
            std::cout << "survivors    " << st.ranker()->count().to_decimal() << " (exact; compact mode available)\n";
            // What the stack sets aside: the rest of the line (the hallway's excluded mode shows them).
            BigUint excluded = line.space.size();
            excluded -= st.ranker()->count();
            std::cout << "excluded     " << excluded.to_decimal() << " (exact)\n";
            // As powers of ten of the line (display only): what the numbers above come to.
            auto share = [&](const BigUint& part) {
                if (part.is_zero()) return std::string("none");
                std::ostringstream o;
                const double x = part.log10_approx() - line.space.size().log10_approx();
                if (x > -0.005) return std::string("about all");
                o << "10^" << std::fixed << std::setprecision(2) << x;
                return o.str();
            };
            std::cout << "share        the stack keeps " << share(st.ranker()->count()) << " of the line and sets aside " << share(excluded) << "\n";
        }
        else std::cout << "compact      unavailable: " << st.compact_blocker() << "\n";
    }
    return 0;
}

int cmd_check(const Args& a)
{
    if (a.has("book")) return cmd_check_book(a);
    if (a.get("line", "text") == "binary") return cmd_check_binary(a);
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

// locate: a file's place on the binary line, or a folder's manifest (SPECIFICATIONS §12.2).
// The program as it was run, for finding what lies beside it (sieve-install, for --program).
const char* g_argv0 = nullptr;

int cmd_locate(const Args& a)
{
    namespace fs = std::filesystem;
    if (a.positional.empty()) throw std::invalid_argument("locate needs a file or a folder (see: sieve help locate)");
    const std::string arg = a.positional[0];
    const fs::path target(std::u8string(arg.begin(), arg.end()));
    auto u8 = [](const fs::path& p) {
        const std::u8string u = p.generic_u8string();
        return std::string(u.begin(), u.end());
    };
    auto address_bytes = [](const BigUint& v) { return uint64_t((v.bit_length() + 7) / 8); };
    auto head_tail = [](const std::string& h) { return h.size() <= 64 ? h : h.substr(0, 32) + "..." + h.substr(h.size() - 32); };
    Comparison cmp;
    if (fs::is_regular_file(target))
    {
        const auto bytes = read_file_bytes(target);
        vault::check_bytes(bytes, u8(target)); // the vault: a withheld file is never located
        const BigUint addr = binary_address(bytes);
        const std::string hex = addr.is_zero() ? "0" : addr.to_hex();
        std::cout << "file      " << u8(target) << "\n"
                  << "bytes     " << bytes.size() << "\n"
                  << "sha256    " << sha256_hex(bytes) << "\n"
                  << "kind      " << file_kind(bytes, bytes.size()) << " (" << kFileKindsVersion << ", from its first bytes)\n"
                  << "line      binary (binary-v1), positional; on every binary line of length " << bytes.size() << " or more\n"
                  << "address   " << hex.size() << " hex digits: " << head_tail(hex) << "\n";
        if (a.has("out"))
        {
            const std::string o = a.get("out");
            std::ofstream out(fs::path(std::u8string(o.begin(), o.end())), std::ios::binary);
            out << hex << "\n";
            if (!out) throw std::runtime_error("cannot write " + o);
            std::cout << "wrote     " << o << "\n";
        }
        if (a.has("compare"))
        {
            cmp.original = bytes.size();
            cmp.deflate = deflate_size(bytes);
            cmp.lzma2 = lzma2_size(bytes);
            cmp.address_bytes = address_bytes(addr);
            cmp.address_hex = addr.is_zero() ? 0 : hex.size();
        }
        // Sieve instructions for the one file, exactly as for a folder holding just it: its name,
        // size and SHA-256, then its bytes, as one number. And as an installer program.
        if (a.has("installer") || a.has("program") || a.has("compare"))
        {
            Manifest m = manifest_of_file(target);
            add_contents(m, fs::absolute(target).parent_path());
            const std::vector<uint8_t> ibytes = m.file();
            const BigUint iaddr = binary_address(ibytes);
            if (a.has("installer"))
            {
                const std::string o = a.get("installer");
                write_address_file(fs::path(std::u8string(o.begin(), o.end())), iaddr, a.has("hex"));
                std::cout << "installer " << o << ": Sieve instructions for the file (its name, size and SHA-256, then its bytes), as "
                          << (a.has("hex") ? "hex" : "raw bytes") << "\n";
            }
            if (a.has("program"))
            {
                const std::string o = a.get("program");
                const auto stub = installer_program_beside(own_executable(g_argv0).parent_path());
                if (!stub) throw std::runtime_error("--program needs sieve-install beside sieve (build it with the client)");
                write_installer_program(*stub, iaddr, fs::path(std::u8string(o.begin(), o.end())));
                std::cout << "program   " << o << ": sieve-install with the instructions attached; run it to install\n";
            }
            if (a.has("compare"))
            {
                cmp.manifest = ibytes.size();
                cmp.manifest_deflate = deflate_size(ibytes);
                cmp.manifest_lzma2 = lzma2_size(ibytes);
                cmp.installer_hex = iaddr.is_zero() ? 1 : iaddr.to_hex().size();
                cmp.installer_raw = address_bytes(iaddr);
            }
        }
        if (a.has("compare")) std::cout << "\n" << comparison_table(cmp);
        return 0;
    }
    if (!fs::is_directory(target)) throw std::invalid_argument(arg + " is neither a file nor a folder");
    Manifest m = walk_folder(target);
    // Which manifest: v2 lists every file's address (--with-addresses); an installer's (v3) has
    // every file's bytes after its text, and the installer is its address; otherwise v1.
    const bool installer = a.has("installer") || a.has("program");
    if (a.has("with-addresses")) add_addresses(m, target);
    else if (installer) add_contents(m, target);
    const std::string text = m.text();
    const std::vector<uint8_t> mbytes = m.file();
    // An installer is always made from v3, whichever manifest is written out.
    Manifest inst;
    if (installer && !m.with_contents)
    {
        inst = m;
        add_contents(inst, target);
    }
    const std::vector<uint8_t> ibytes = !installer ? std::vector<uint8_t>{} : m.with_contents ? mbytes : inst.file();
    const BigUint maddr = binary_address(mbytes);
    // The manifest to --manifest, or to standard output (a v3 manifest's text part only: its
    // files' bytes are no use on a terminal); what it says about itself to standard error when
    // the manifest is on standard output, so the two never mix.
    std::ostream* info = &std::cout;
    if (a.has("manifest"))
    {
        const std::string o = a.get("manifest");
        std::ofstream out(fs::path(std::u8string(o.begin(), o.end())), std::ios::binary);
        out.write(reinterpret_cast<const char*>(mbytes.data()), std::streamsize(mbytes.size()));
        if (!out) throw std::runtime_error("cannot write " + o);
    }
    else
    {
        std::cout << text;
        info = &std::cerr;
    }
    size_t dirs = 0;
    for (const auto& e : m.entries) dirs += e.dir ? 1 : 0;
    *info << "folder    " << u8(target) << ": " << m.files << " files, " << dirs << " folders, " << m.bytes << " bytes";
    if (m.skipped) *info << " (" << m.skipped << " links and other entries skipped)";
    *info << "\nmanifest  " << text.substr(0, text.find('\n')) << ", " << mbytes.size() << " bytes" << (a.has("manifest") ? ", written to " + a.get("manifest") : std::string())
          << "\nsha256    " << sha256_hex(mbytes) << (m.with_addresses || m.with_contents ? "\n" : "  (the tree's identity)\n")
          << "address   " << (maddr.is_zero() ? 1 : maddr.to_hex().size()) << " hex digits: the manifest's own place on the binary line\n";
    // Every file's address, as <DIR>/<path>.hex, and the comparison over all of them.
    const fs::path root = fs::absolute(target).lexically_normal();
    std::vector<uint8_t> all; // every file, in manifest order, for the solid LZMA2 stream
    const bool compare = a.has("compare");
    if (a.has("addresses") || compare)
    {
        const std::string d = a.get("addresses");
        const fs::path dir(std::u8string(d.begin(), d.end()));
        for (const auto& e : m.entries)
        {
            if (e.dir) continue;
            const fs::path file = root / fs::path(std::u8string(e.path.begin(), e.path.end()));
            const auto bytes = read_file_bytes(file);
            const BigUint addr = binary_address(bytes);
            const std::string hex = addr.is_zero() ? "0" : addr.to_hex();
            if (a.has("addresses"))
            {
                const fs::path out_path = dir / fs::path(std::u8string(e.path.begin(), e.path.end()) + u8".hex");
                fs::create_directories(out_path.parent_path());
                std::ofstream out(out_path, std::ios::binary);
                out << hex << "\n";
                if (!out) throw std::runtime_error("cannot write " + u8(out_path));
            }
            if (compare)
            {
                cmp.original += bytes.size();
                cmp.deflate += deflate_size(bytes);
                cmp.address_bytes += address_bytes(addr);
                cmp.address_hex += addr.is_zero() ? 0 : hex.size();
                all.insert(all.end(), bytes.begin(), bytes.end());
            }
        }
        if (a.has("addresses")) *info << "addresses " << m.files << " files' addresses written under " << d << "\n";
    }
    const BigUint iaddr = installer ? (m.with_contents ? maddr : binary_address(ibytes)) : BigUint();
    if (a.has("program"))
    {
        // The installer attached to a copy of sieve-install: one program to hand to someone.
        const std::string o = a.get("program");
        const auto stub = installer_program_beside(own_executable(g_argv0).parent_path());
        if (!stub) throw std::runtime_error("--program needs sieve-install beside sieve (build it with the client)");
        write_installer_program(*stub, iaddr, fs::path(std::u8string(o.begin(), o.end())));
        *info << "program   " << o << ": sieve-install with the installer attached (" << fs::file_size(fs::path(std::u8string(o.begin(), o.end())))
              << " bytes); run it to install\n";
        std::error_code ec;
        if (fs::exists(stub->parent_path() / "SDL3.dll", ec))
            *info << "          note: this sieve-install uses SDL3.dll, so the program needs SDL3.dll beside it too;\n"
                     "          build with a static SDL (the fetched one is) for a program that stands alone\n";
    }
    if (a.has("installer"))
    {
        const std::string o = a.get("installer");
        write_address_file(fs::path(std::u8string(o.begin(), o.end())), iaddr, a.has("hex"));
        *info << "installer " << o << ": the address of the installer's manifest (v3: the listing, then every file's bytes), as "
              << (a.has("hex") ? "hex" : "raw bytes") << "; install it with\n"
              << "          sieve-install " << o << "   (or: sieve install " << o << (a.has("hex") ? " --hex" : "") << " --to FOLDER)\n";
    }
    if (compare)
    {
        cmp.lzma2 = lzma2_size(all);
        cmp.manifest = text.size();
        if (installer)
        {
            // The installer's manifest itself, compressed: the tree's bytes and a little text.
            cmp.manifest = ibytes.size();
            cmp.manifest_deflate = deflate_size(ibytes);
            cmp.manifest_lzma2 = lzma2_size(ibytes);
            cmp.installer_hex = iaddr.is_zero() ? 1 : iaddr.to_hex().size();
            cmp.installer_raw = address_bytes(iaddr);
        }
        *info << "\n" << comparison_table(cmp);
    }
    return 0;
}

// map: a folder's map (node graph of verified anchors), a map of some files (a release's), or a
// map read back; changed an anchor at a time (--add, --remove) or sealed; written as the map
// itself, as DOT or as GraphML.
int cmd_map(const Args& a)
{
    namespace fs = std::filesystem;
    if (a.has("new"))
    {
        // An empty map: only its root (sealed with --seal, as the checks use for a read-only map).
        const std::string name = a.get("new");
        Map empty = Map::empty(name);
        empty.sealed = a.has("seal");
        const auto bytes = empty.file();
        const std::string to = a.has("out") ? a.get("out") : name + ".map";
        std::ofstream out(fs::path(std::u8string(to.begin(), to.end())), std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        if (!out) throw std::runtime_error("cannot write " + to);
        std::cerr << "map       " << name << (empty.sealed ? " (sealed)" : "") << ": empty, written to " << to << "\n";
        return 0;
    }
    if (a.positional.empty()) throw std::invalid_argument("map needs a FOLDER, FILEs, or a MAP file (see: sieve help map)");
    auto path_of = [](const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); };
    auto name_of = [](const fs::path& p) {
        fs::path r = fs::absolute(p).lexically_normal();
        if (!r.has_filename()) r = r.parent_path();
        const std::u8string u = r.filename().u8string();
        return std::string(u.begin(), u.end());
    };
    auto is_map = [&](const fs::path& p) {
        std::ifstream in(p, std::ios::binary);
        char head[11] = {};
        in.read(head, sizeof head);
        return in.gcount() == 11 && std::string(head, 11) == "sieve-map-v";
    };
    const fs::path first = path_of(a.positional[0]);
    Map m;
    if (a.positional.size() == 1 && fs::is_directory(first)) m = map_of_folder(first, a.has("name") ? a.get("name") : name_of(first));
    else if (a.positional.size() == 1 && is_map(first))
    {
        const auto bytes = read_file_bytes(first);
        m = Map::parse(std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size()));
        if (a.has("name")) m.name = a.get("name");
    }
    else
    {
        std::vector<fs::path> files;
        for (const auto& f : a.positional) files.push_back(path_of(f));
        m = map_of_files(files, a.has("name") ? a.get("name") : "files");
    }
    if (a.has("remove"))
    {
        const uint32_t id = uint32_t(std::stoul(a.get("remove")));
        const std::string gone = id < m.nodes.size() ? m.label(id) : "";
        m.remove(id);
        std::cerr << "removed   node " << id << " (" << gone << ")\n";
    }
    if (a.has("add"))
    {
        const fs::path f = path_of(a.get("add"));
        const std::string label = a.has("as") ? a.get("as") : name_of(f);
        const uint32_t id = m.add_held(label, read_file_bytes(f));
        std::cerr << "added     " << label << " as node " << id << ", held in the map\n";
    }
    if (a.has("meta"))
    {
        // NODE:key=value (an empty value removes the key)
        const std::string spec = a.get("meta");
        const size_t colon = spec.find(':'), eq = spec.find('=');
        if (colon == std::string::npos || eq == std::string::npos || eq < colon) throw std::invalid_argument("--meta takes NODE:key=value");
        m.set_meta(uint32_t(std::stoul(spec.substr(0, colon))), spec.substr(colon + 1, eq - colon - 1), spec.substr(eq + 1));
        std::cerr << "metadata  node " << spec.substr(0, colon) << ": " << spec.substr(colon + 1) << "\n";
    }
    if (a.has("seal")) m.sealed = true;
    auto write = [&](const std::string& to, const std::vector<uint8_t>& bytes) {
        std::ofstream out(path_of(to), std::ios::binary);
        out.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        if (!out) throw std::runtime_error("cannot write " + to);
    };
    const std::vector<uint8_t> bytes = m.file();
    size_t files = 0, held = 0;
    for (const auto& n : m.nodes)
    {
        files += n.folder() ? 0 : 1;
        held += n.kind == MapNode::Held ? 1 : 0;
    }
    std::ostream* info = &std::cout;
    if (a.has("out")) write(a.get("out"), bytes);
    else if (!a.has("dot") && !a.has("graphml"))
    {
        const std::string text = m.text(); // the text part: held bytes are no use on a terminal
        std::cout << text;
        info = &std::cerr;
    }
    auto as_bytes = [](const std::string& t) { return std::vector<uint8_t>(t.begin(), t.end()); };
    if (a.has("dot")) write(a.get("dot"), as_bytes(m.dot()));
    if (a.has("graphml")) write(a.get("graphml"), as_bytes(m.graphml()));
    *info << "map       " << m.name << (m.sealed ? " (sealed)" : "") << ": " << m.nodes.size() << " nodes (" << files << " files, " << held
          << " held; " << m.nodes.size() - files << " folders), " << m.edges.size() << " edges, " << bytes.size() << " bytes\n"
          << "sha256    " << sha256_hex(bytes) << "  (the map's identity)\n";
    if (a.has("out")) *info << "wrote     " << a.get("out") << "\n";
    if (a.has("dot")) *info << "wrote     " << a.get("dot") << " (Graphviz DOT)\n";
    if (a.has("graphml")) *info << "wrote     " << a.get("graphml") << " (GraphML)\n";
    return 0;
}

// install: a folder put back from one address, the address of its installer's manifest.
int cmd_install(const Args& a)
{
    namespace fs = std::filesystem;
    if (a.positional.empty() || !a.has("to")) throw std::invalid_argument("install needs an address file and --to FOLDER (see: sieve help install)");
    const std::string in = a.positional[0], to = a.get("to");
    // An installer, an installer program, or an installer's manifest itself.
    const Manifest m = installable_manifest(fs::path(std::u8string(in.begin(), in.end())), a.has("hex"));
    std::cout << "manifest  " << m.root << ": " << m.files << " files, " << m.bytes << " bytes\n";
    const fs::path dest(std::u8string(to.begin(), to.end()));
    install_tree(m, dest, a.has("force"), [&](int phase, size_t done, size_t total, const std::string& path) {
        if (phase == 1) std::cout << "  " << done << "/" << total << "  " << path << "\n";
    });
    std::cout << "installed " << m.files << (m.files == 1 ? " file into " : " files into ") << to
              << (m.files == 1 ? ", checked against its SHA-256\n" : ", every one checked against its SHA-256\n");
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

// vault: what the vault holds (how many entries, from which files, which decoders) and whether files
// are withheld, as they are or (--written) as text that is a known file written out.
int cmd_vault(const Args& a)
{
    namespace fs = std::filesystem;
    if (a.has("chunks"))
    {
        // A whole vault file on standard output, one `chunks` entry per FILE (docs/VAULT.md).
        if (a.positional.empty()) throw std::invalid_argument("give the files to list: sieve vault --chunks FILE...");
        std::set<std::string> common;
        if (a.has("common"))
        {
            const std::string c = a.get("common");
            const fs::path folder(std::u8string(c.begin(), c.end()));
            if (!fs::is_directory(folder)) throw std::invalid_argument("--common expects a folder");
            for (const auto& e : fs::recursive_directory_iterator(folder))
                if (e.is_regular_file())
                {
                    const std::vector<uint8_t> b = read_file_bytes(e.path());
                    for (const cdc::Chunk& ch : cdc::chunks(b)) common.insert(Sha256::hex(ch.sha256));
                }
        }
        std::vector<std::string> lines;
        for (const std::string& arg : a.positional)
        {
            const std::string line = vault::chunks_entry(read_file_bytes(fs::path(std::u8string(arg.begin(), arg.end()))), common);
            if (line.empty()) std::cerr << "note: " << arg << " has no chunks left to list (too small, one repeated byte, or all common)\n";
            else lines.push_back(line);
        }
        std::cout << vault::kVersion << "\nentries " << lines.size() << "\n";
        for (const std::string& l : lines) std::cout << l << "\n";
        std::cout << "end\n";
        return 0;
    }
    if (a.has("test-file"))
    {
        const std::vector<uint8_t> b = vault::test_file_bytes();
        const std::string out = a.get("test-file");
        std::ofstream f(fs::path(std::u8string(out.begin(), out.end())), std::ios::binary);
        if (!f) throw std::runtime_error("cannot write '" + out + "'");
        f.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
        std::cout << "wrote     " << out << "\n";
        return 0;
    }
    const vault::Status st = vault::status();
    std::cout << "vault     " << st.builtin << " built in, " << st.loaded << " from files (" << st.sha256 << " sha256, " << st.pdq << " pdq, "
              << st.chunk_files << " chunks: " << st.chunks << " chunks in all)\n";
    for (const auto& f : st.files) std::cout << "file      " << f << "\n";
    {
        std::cout << "decoders  " << vault::kDecoders << ":";
        for (const auto& n : vault::decoder_names()) std::cout << " " << n;
        std::cout << "\n";
        std::cout << "pictures  " << vault::kPdqMatch << ": quality " << vault::kPdqMinQuality << " or more, within "
                  << vault::kPdqMaxDistance << " bits, any of 8 orientations\n";
        std::cout << "pieces    " << vault::kChunkMatch << ": " << cdc::kVersion << " chunks (" << cdc::kMin << " to " << cdc::kMax
                  << " bytes), two of one entry\n";
    }
    if (st.failed_closed) std::cout << "FAILED CLOSED: everything is withheld until this is fixed or removed:\n  " << st.error << "\n";
    if (a.has("test-picture"))
    {
        // The test picture, 64 x 64, for seeing the perceptual check work (docs/VAULT.md).
        const std::vector<uint8_t> px = vault::test_picture_rgba();
        std::vector<Rgb> rgb(size_t(64) * 64);
        for (size_t i = 0; i < rgb.size(); ++i) rgb[i] = Rgb{px[i * 4], px[i * 4 + 1], px[i * 4 + 2]};
        write_png(a.get("test-picture"), 64, 64, rgb, a.has("scale") ? a.get_positive("scale", 1) : 1);
        std::cout << "wrote     " << a.get("test-picture") << "\n";
        return 0;
    }
    int withheld = 0;
    for (const std::string& arg : a.positional)
    {
        const fs::path p(std::u8string(arg.begin(), arg.end()));
        if (a.has("parse"))
        {
            std::ifstream in(p, std::ios::binary);
            std::ostringstream text;
            text << in.rdbuf();
            const auto entries = vault::parse(text.str());
            std::cout << "parsed    " << arg << ": " << entries.size() << " entries (" << entries.sha256.size() << " sha256, "
                      << entries.pdq.size() << " pdq, " << entries.chunks.size() << " chunks), well formed\n";
            continue;
        }
        if (a.has("pdq"))
        {
            const auto hs = vault::picture_hashes(read_file_bytes(p));
            if (hs.empty()) std::cout << "not a picture (or under 5 pixels either way)  " << arg << "\n";
            for (size_t i = 0; i < hs.size(); ++i)
                std::cout << hs[i].hex << "  quality " << hs[i].quality << "  " << arg << (hs.size() > 1 ? "  frame " + std::to_string(i + 1) : std::string()) << "\n";
            continue;
        }
        // --written: the file read as text that may be a known file written out (vault_decode.hpp).
        const std::vector<uint8_t> bytes = read_file_bytes(p);
        const bool w = a.has("written") ? vault::withheld_written(std::string(bytes.begin(), bytes.end()))
                                        : vault::withheld_bytes(bytes);
        std::cout << (w ? "withheld  " : "clear     ") << arg << "\n";
        if (w) ++withheld;
    }
    return withheld ? 3 : 0;
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
    g_argv0 = argv[0];
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
        // --timings, on any command: how long each phase took, to standard error at exit.
        if (a.has("timings")) timings::enable();
        // --filter-memory MB, on any command: what one count's tables may take (the setup menu's
        // FILTER MEMORY; 512 MB unless given).
        if (a.has("filter-memory")) set_filter_memory(double(a.get_positive("filter-memory", 512)) * 1024 * 1024);
        // --merge-cache PCT: the share of it kept for merged automata between counts (the setup
        // menu's MERGE CACHE; 50 unless given).
        if (a.has("merge-cache")) set_merge_cache_share(std::min(a.get_u32("merge-cache", 50), 100u) / 100.0);
        load_plugins(); // the filter plugins, registered beside the built-in filters
        const std::string command_phase = "sieve " + a.command;
        timings::Scope timed(command_phase.c_str());
        // A mistyped option would otherwise be ignored without a word.
        {
            const auto known = documented_options(a.command == "sieve" ? "sift" : a.command);
            if (!known.empty())
                for (const auto& [key, value] : a.opts)
                    if (key != "timings" && key != "filter-memory" && key != "merge-cache" && std::find(known.begin(), known.end(), key) == known.end())
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
        if (a.command == "locate") return cmd_locate(a);
        if (a.command == "install") return cmd_install(a);
        if (a.command == "map") return cmd_map(a);
        if (a.command == "vault") return cmd_vault(a);
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
