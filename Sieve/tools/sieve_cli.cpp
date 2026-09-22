// Sieve — command-line interface to the core.
//
//   sieve info   [--line LINE] [line options] [--key K]
//   sieve warp   [--line LINE] [line options] [--key K] [--mode MODE] [--short] (TEXT... | --file PATH)
//   sieve read   [--line LINE] [line options] [--key K] --mode MODE [--out PATH] [--scale S] ADDRESS
//   sieve browse [--line LINE] [line options] [--key K] [--count N] [--short]
//   sieve read   ... ADDRESS --around N      (the N units either side)
//   sieve dicts  [--hash FILE]
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
#include "cli/dictionaries.hpp"
#include "cli/help.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/models.hpp"

#include "sieve/audio.hpp"
#include "sieve/corridor.hpp"
#include "sieve/guided.hpp"
#include "sieve/image.hpp"
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
#include <iostream>
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

// ---------------------------------------------------------------- warp

int cmd_warp(const Args& a)
{
    const Line line = make_line(a);
    const Modes modes = parse_modes(line, a.get("mode", "all"));
    const bool abbreviate = a.has("short");

    const WarpInput w = read_warp_input(line, a);
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
    const uint32_t slot = uint32_t(std::stoul(at.substr(colon + 1)));
    if (slot >= kBooksPerTile) throw std::invalid_argument("SLOT must be 0.." + std::to_string(kBooksPerTile - 1));
    const bool guided = mode == "guided";
    uint32_t zoom = 0;
    if (guided)
    {
        need_guided(line);
        if (!a.has("zoom")) throw std::invalid_argument("--mode guided --at needs --zoom D (books 2^-D apart), as shown in the hallway");
        zoom = a.get_positive("zoom", 20);
    }
    const LineLoop loop(guided ? BigUint::pow(2, zoom) : line.space.size());
    const auto index = loop.unit_index(loop.loop_tile(tile), slot);
    if (!index)
    {
        std::cout << "(empty: padding at the end of a loop)\n";
        return 0;
    }
    std::vector<uint32_t> digits;
    if (guided)
    {
        BigUint point = *index;
        point <<= line.guided->scale_bits() - zoom;
        digits = line.guided->unit_at(point);
        std::cerr << "point " << line.guided->hex_of(point, zoom) << "\n";
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
    std::vector<uint32_t> digits;
    if (mode == "guided")
    {
        const GuidedLine& g = need_guided(line);
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
    if (mode == "guided")
        std::cerr << "random points on the guided line: text the model finds likely. It is fluent by\n"
                     "construction, which is evidence of nothing (SPECIFICATIONS 4.2).\n\n";
    for (uint32_t i = 0; i < count; ++i)
    {
        std::vector<uint32_t> unit;
        std::string label;
        if (mode == "guided")
        {
            // A uniformly random point is a sample from the model.
            const GuidedLine& g = need_guided(line);
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
        const uint32_t a = static_cast<uint32_t>(std::stoul(part.substr(0, dash)));
        const uint32_t b = dash == std::string::npos ? a : static_cast<uint32_t>(std::stoul(part.substr(dash + 1)));
        if (a < 1 || b < a) throw std::invalid_argument("bad length range '" + part + "'");
        for (uint32_t L = a; L <= b; ++L) out.push_back(L);
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
              << kModelTotalBits << ")\n";
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

int cmd_train(const Args& a)
{
    if (!a.has("out")) throw std::invalid_argument("missing --out FILE (the model to write)");
    const Corpus corpus = load_corpus(a.get("corpus", "data/models/corpus/gutenberg-nltk.tsv"));
    const fs::path dir = a.get("texts", "corpus/gutenberg");
    const Alphabet& alpha = alphabet_by_id(a.get("alphabet", "lower27"));
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
    const Alphabet& alpha = alphabet_by_id(a.get("alphabet", "lower27"));
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
        const Corpus corpus = load_corpus(a.get("corpus", "data/models/corpus/gutenberg-nltk.tsv"));
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

int cmd_filters(const Args& a)
{
    const Line line = make_line(a.has("length") || a.get("line", "text") != "text" ? a : [&] {
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

bool is_command(const std::string& name)
{
    return name == "info" || name == "warp" || name == "read" || name == "browse" || name == "sift" || name == "sieve" || name == "dicts" ||
           name == "version" || name == "models" || name == "train" || name == "measure" ||
           name == "filters" || name == "check";
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
        if (a.command == "info") return cmd_info(a);
        if (a.command == "warp") return cmd_warp(a);
        if (a.command == "read") return cmd_read(a);
        if (a.command == "browse") return cmd_browse(a);
        if (a.command == "sift" || a.command == "sieve") return cmd_sieve(a); // "sieve" kept as an alias
        if (a.command == "dicts") return cmd_dicts(a);
        if (a.command == "version") return cmd_version();
        if (a.command == "models") return cmd_models();
        if (a.command == "train") return cmd_train(a);
        if (a.command == "measure") return cmd_measure(a);
        if (a.command == "filters") return cmd_filters(a);
        if (a.command == "check") return cmd_check(a);
        std::cerr << "unknown command '" << a.command << "'\n\n";
        print_usage();
        return 1;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n";
        if (argc >= 2 && is_command(argv[1])) std::cerr << "(see: sieve help " << argv[1] << ")\n";
        return 1;
    }
}
