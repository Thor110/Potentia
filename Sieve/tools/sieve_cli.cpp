// Sieve — command-line interface to the core.
//
//   sieve info   [--line LINE] [line options] [--key K]
//   sieve warp   [--line LINE] [line options] [--key K] [--mode MODE] [--short] (TEXT... | --file PATH)
//   sieve read   [--line LINE] [line options] [--key K] --mode MODE [--out PATH] [--scale S] ADDRESS
//   sieve browse [--line LINE] [line options] [--key K] [--count N] [--short]
//   sieve read   ... ADDRESS --around N      (the N units either side)
//   sieve dicts  [--hash FILE]
//   sieve version
//   sieve sift  [--dict ID|PATH] [--lengths SPEC] [--brute-max N] [--pruned-max N] [--threads T] [--csv PATH]
//
// Run "sieve help", "sieve help <command>" or "sieve help lines" for explanations and examples.

#include "cli/args.hpp"
#include "cli/dictionaries.hpp"
#include "cli/help.hpp"
#include "cli/lines.hpp"

#include "sieve/audio.hpp"
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
    return 0;
}

// ---------------------------------------------------------------- warp

int cmd_warp(const Args& a)
{
    const Line line = make_line(a);
    const std::string mode = a.get("mode", "both");
    if (mode != "both") address_mode_from_string(mode); // validate
    const bool abbreviate = a.has("short");

    const WarpInput w = read_warp_input(line, a);
    print_header(line);
    std::cout << "canon        " << w.report.front() << "\n";
    for (size_t i = 1; i < w.report.size(); ++i) std::cout << w.report[i] << "\n";

    const bool multiline = line.kind == LineKind::Image || line.kind == LineKind::Video;
    for (size_t i = 0; i < w.units.size(); ++i)
    {
        const auto& digits = w.units[i];
        std::cout << "\nunit " << i + 1 << "/" << w.units.size();
        if (multiline) { std::cout << "\n"; print_indented(preview(line, digits), "  "); }
        else std::cout << "  " << preview(line, digits) << "\n";
        for (AddressMode m : {AddressMode::Positional, AddressMode::Scrambled})
        {
            if (mode != "both" && address_mode_from_string(mode) != m) continue;
            const auto addr = line.space.address_digits(digits, m); // scramble once, reuse below
            char pct[32];
            std::snprintf(pct, sizeof pct, "%.10f%%", line.space.fraction_of(addr) * 100.0);
            std::cout << "  " << to_string(m) << (m == AddressMode::Positional ? "  " : "   ")
                      << show_address(line.space.hex_of(addr), abbreviate) << "\n"
                      << "  " << std::string(12, ' ') << "at " << pct << " along the line\n";
        }
    }
    return 0;
}

// ---------------------------------------------------------------- read

int cmd_read(const Args& a)
{
    const Line line = make_line(a);
    if (!a.has("mode")) throw std::invalid_argument("missing --mode positional|scrambled");
    if (a.positional.size() != 1) throw std::invalid_argument("give exactly one ADDRESS");
    const AddressMode m = address_mode_from_string(a.get("mode"));
    const auto address = line.space.parse_address(a.positional[0]);
    const auto digits = line.space.unit_of_address(address, m);
    if (a.has("around"))
    {
        // The neighbouring shelves: what the hallway shows either side of this unit.
        const int64_t n = a.get_u32("around", 0);
        const bool multiline = line.kind == LineKind::Image || line.kind == LineKind::Video;
        for (int64_t off = -n; off <= n; ++off)
        {
            // Step the address digits and unscramble once per shelf.
            const auto step = line.space.step_address(address, off);
            const auto u = off == 0 ? digits : line.space.unit_of_address(step, m);
            char label[24];
            std::snprintf(label, sizeof label, "%+lld", static_cast<long long>(off));
            const std::string addr = show_address(line.space.hex_of(step), a.has("short"));
            if (multiline)
            {
                std::cout << (off == 0 ? "> " : "  ") << label << "  " << addr << "\n";
                print_indented(preview(line, u), "    ");
            }
            else
                std::cout << (off == 0 ? "> " : "  ") << std::string(6 - std::min<size_t>(6, std::strlen(label)), ' ')
                          << label << "  " << preview(line, u) << "  " << addr << "\n";
        }
    }
    else if (line.kind == LineKind::Text) std::cout << utf8_encode(line.space.text_of(digits)) << "\n";
    else std::cout << preview(line, digits) << "\n";
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
    std::random_device rd;
    std::mt19937_64 rng((uint64_t(rd()) << 32) ^ rd());
    std::uniform_int_distribution<uint32_t> digit(0, sp.base() - 1);
    const bool multiline = line.kind == LineKind::Image || line.kind == LineKind::Video;
    for (uint32_t i = 0; i < count; ++i)
    {
        // A uniformly random scrambled address is a uniformly random unit.
        std::vector<uint32_t> addr(sp.unit_length());
        for (auto& d : addr) d = digit(rng);
        const std::string hex = sp.hex_of(addr);
        const auto unit = sp.unit_of_address(std::move(addr), AddressMode::Scrambled);
        if (i) std::cout << "\n";
        if (multiline) print_indented(preview(line, unit), "");
        else std::cout << preview(line, unit) << "\n";
        std::cout << "  scrambled " << show_address(hex, abbreviate) << "\n";
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
    std::cout << "\n  note symbols            " << kNotesSymbolsId << "\n";
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

bool is_command(const std::string& name)
{
    return name == "info" || name == "warp" || name == "read" || name == "browse" || name == "sift" || name == "sieve" || name == "dicts" ||
           name == "version";
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
