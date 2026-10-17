// Sieve CLI -- files weighed: every way found to name each one, against its own address (shared by
// `sieve locate --weigh` and the hallway's File Locator).
//
// A file's own name in the space is its place on the binary line (locate.hpp): its bytes, so as
// long as the file. That is the base. Other ways to name it, under the lines and filters given:
//
//   - its number among the binary line's survivors, where that line has filters that rank and the
//     file passes them;
//   - where the file is exactly an item of another line (item_of: a page, a picture or film as its
//     PNG, a melody as its MIDI file, byte for byte), its place on that line, and its number among
//     that line's survivors where the line has filters that rank and it passes them.
//
// Each number is weighed as its shortest route (tailor.hpp route_bits): the cleanest digits there
// are for it, leading zeros dropped or a bearing and a walk. The best way is the shortest. A way
// other than the base needs the reader to know what it was taken under: the line's shape (its
// symbols and length, "lower27/L32", priced as the text it is) and, where it uses filters, the
// stack (description_bits).
// Each is paid once however many files use it, so the totals weigh the files' best ways together
// with what they share, against the files' own addresses.
//
// With tailoring, the files that are items of a line are its anchors: the line's filters are
// tailored to keep every one of them and write them shortest, the description paid for (tailor.hpp),
// and the totals weighed again with what that finds.
#pragma once

#include "cli/filter_config.hpp"
#include "cli/lines.hpp"
#include "cli/tailor.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// The lines files are weighed against (any may be null: not weighed there), and their filters.
struct WeighLines
{
    const Line* pages = nullptr;
    const Line* image = nullptr;
    const Line* audio = nullptr;
    const Line* video = nullptr;
    FilterConfig filters;
    uint64_t binary_bytes = 0; // the binary line's length, for its filters (0: as long as the longest file weighed)
};

struct Way
{
    std::string how;      // "binary", "binary, compact", "pages", "pages, compact", ...
    double bits = 0;      // its shortest route
    std::string written;  // the route as written (empty for the base, which is the file's own hex)
};

struct FileWeight
{
    std::string path;       // as given (relative to a folder walked)
    uint64_t bytes = 0;
    double base_bits = 0;   // its address on the binary line, its hex digits at 4 bits each
    std::vector<Way> ways;  // every way found, the base first
    size_t best = 0;        // the shortest of them
    int line = -1;          // the line (LineKind) it is exactly an item of, or -1
    std::vector<uint32_t> unit; // its unit there
    std::string error;      // could not be read
};

// One line's part in the totals: what the files that use it share.
struct LineWeight
{
    std::string name;            // "binary", "pages", ...
    std::string shape;           // its symbols and length ("lower27/L32"); binary: its shape
    size_t files = 0;            // files whose best way is on it
    double shape_bits = 0;       // the shape written down (8 bits a character)
    double description_bits = 0; // its filters written down, where a best way uses them
    std::optional<TailorResult> tailored; // with tailoring: what the search found for its items
    // With tailoring and a time given, a search that found nothing it could use because its time ran
    // out (or the run's was spent before it began): said in the table, so a line left as it was is
    // not taken for one that had nothing shorter.
    std::string search_stopped;
};

// How long the tailoring may search for shorter addresses (seconds; 0: no limit). Each file is
// given `per_file`, so a line's search has that times its items; and the run as a whole `total`,
// which caps every line's search at what is left of it. Each is the other's with the files counted:
// total = per_file * files (budget_total, budget_per_file), and the File Locator moves them together.
struct WeighBudget
{
    double per_file = 0, total = 0;
};
inline double budget_total(double per_file, size_t files) { return per_file * double(files); }
inline double budget_per_file(double total, size_t files) { return files ? total / double(files) : total; }

struct Weighing
{
    std::vector<FileWeight> files;
    WeighBudget budget;            // the time given (with tailoring)
    double searched = 0;           // the seconds the tailoring took, every line together
    std::vector<LineWeight> lines;
    double base_bits = 0;   // every file's own address
    double best_bits = 0;   // every file's best way
    double shared_bits = 0; // the shapes and stacks those ways need, each once
    double total_bits() const { return best_bits + shared_bits; }
};

// Weighs `files` (paths under `root`, or a single file with an empty root), reading each once.
// With `tailor`, each line's items are its anchors and the line's filters are tailored to them
// (description paid for); a line is weighed under what the search found where that is better.
// `budget` bounds that search (no limit by default); a line whose search runs out of time keeps the
// best stack it found by then.
Weighing weigh_files(const std::filesystem::path& root, const std::vector<std::string>& files, const WeighLines& lines, bool tailor,
                     TailorProgress* progress = nullptr, const WeighBudget& budget = {});

// The weighing as a table, one row a file and the totals.
std::string weighing_table(const Weighing& w, size_t max_rows = 0);

} // namespace sieve::cli
