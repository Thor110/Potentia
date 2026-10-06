// Sieve CLI — filter settings (sieve-filters.ini) and the pinned data filters use.
//
// The settings file lives next to the executable (outside the folders a rebuild copies, so a
// rebuild never overwrites it), or wherever --filters PATH points. It is plain text:
//
//   [text]
//   mode = hide                  ; off | mark | hide | compact | excluded
//   filters = words-v1, model-information-v1
//   [text.model-information-v1]
//   max_millibits = 4000
//   [books]
//   mode = compact
//   [books.title]               ; the books line: title (one page), cover (a picture) and
//   filters = title-v1          ; pages (all pages read as one text)
//   [books.pages]
//   filters = words-v2
//
// The hallway's setup menu edits and saves it; hand edits are welcome. A missing file means
// every line has no filters and mode off.
#pragma once

#include "lines.hpp"

#include "sieve/filekind.hpp"
#include "sieve/modelsieve.hpp"
#include "sieve/modelspace.hpp"
#include "sieve/filter.hpp"

#include <filesystem>
#include <optional>
#include <map>
#include <string>
#include <vector>

namespace sieve::cli {

// Excluded is Hide turned round: the units that pass are left out and the ones that fail keep
// their places, so what a stack sets aside can be walked and checked, each with the filter that
// rejected it.
enum class FilterMode { Off, Mark, Hide, Compact, Excluded };
const char* to_string(FilterMode m);
FilterMode filter_mode_from_string(const std::string& s);

struct LineFilters
{
    FilterMode mode = FilterMode::Off;
    std::vector<std::string> enabled;           // filter names, e.g. "words-v1"
    std::map<std::string, FilterValues> values; // per filter name: parameter values
    bool is_enabled(const std::string& name) const;
    void set_enabled(const std::string& name, bool on);
};

// The books line: one mode, and a stack for each part. The parts' own modes are unused.
struct BookFilters
{
    FilterMode mode = FilterMode::Off;
    LineFilters parts[3]; // cover, title, pages
    static const char* part_name(int i);
    static int part_index(const std::string& name); // -1 if unknown
};

// A composition dimension's (tracks, movies; sieve/composition.hpp): one mode, and a stack for its
// cover, its title, each of its units judged one at a time, and its units joined, judged as one.
struct CompositionFilters
{
    static constexpr int kParts = 4;
    FilterMode mode = FilterMode::Off;
    LineFilters parts[kParts]; // cover, title, units, joined
    static const char* part_name(int i);
    static int part_index(const std::string& name); // -1 if unknown
};

struct FilterConfig
{
    LineFilters lines[4]; // text, image, audio, video
    BookFilters books;
    CompositionFilters tracks, movies;
    // The models and binary lines: a mode and a stack each. The models line has no filters yet;
    // the binary line has binary-kind-v1 (sieve/filekind.hpp). The binary line is one line, met at
    // both ends of the corridor, so it has one section.
    LineFilters models, binary;
    LineFilters& of(LineKind k) { return lines[int(k)]; }
    const LineFilters& of(LineKind k) const { return lines[int(k)]; }

    static std::filesystem::path default_path(); // next to the executable
    static FilterConfig load(const std::filesystem::path& path); // missing file: defaults
    void save(const std::filesystem::path& path) const;
};

// Registries (dictionaries.tsv, models.tsv) with hash checks, cached per process.
class AppResources : public FilterResources
{
public:
    std::shared_ptr<const Dictionary> dictionary(const std::string& id) const override;
    std::shared_ptr<const CharModel> model(const std::string& id, const std::string& symbols) const override;
};

// Ticks or unticks a filter. Ticking also ticks what it requires (a plugin's `requires`), and what
// those require, each with the settings it pins; a prerequisite already ticked keeps its own
// settings (prerequisite_notes then reports the difference). Returns the filters ticked with it.
std::vector<std::string> tick_filter(LineFilters& settings, const std::string& name, bool on);
// Ticking by hand (the menus): as tick_filter, and then any ticked filter that cannot be counted
// together with what was just ticked is unticked (filter_conflict; docs/FILTERS-CONFLICTS.md).
// Returns the names unticked. Settings files are never changed this way when they are loaded.
// `line`, where known, decides how a filter whose way of counting depends on it counts (filter_conflict).
std::vector<std::string> tick_filter_by_hand(LineFilters& settings, const std::string& name, bool on, const FilterLine* line = nullptr);
// What the ticked filters' prerequisites say about the stack: one line for each prerequisite that
// is not ticked, or is ticked at a setting other than the one pinned. Empty when all is well.
std::vector<std::string> prerequisite_notes(const LineFilters& settings);

FilterLine filter_line(const Line& line);
// The line's stack from its settings (filters that do not apply to this line are skipped). A
// ticked filter's prerequisites join the stack even when a hand-edited file leaves them out.
FilterStack build_stack(const Line& line, const LineFilters& settings);
FilterStack build_stack(const FilterLine& line, const LineFilters& settings);

// The binary line's shape as filters see it (kind "binary", its length the line's most bytes),
// and its ticked filters as one sieve (sieve/filekind.hpp).
FilterLine binary_filter_line(uint64_t max_bytes);
// The other lines, for not-an-item-v1: pages of one-byte symbols as a pattern (counted exactly),
// and judges for pictures (their PNG, one pixel a pixel), melodies (their MIDI file), models
// (their .obj) and pages of other alphabets. Any line may be null (its form is then not known).
BinaryItems binary_items(const Line* pages, const Line* image, const Line* video, const Line* audio, const ModelSpace* models);
// Pages of `length` symbols of an alphabet as files: a pattern when every symbol is one byte.
std::optional<KindCounter::Pattern> page_pattern(const Alphabet& a, uint32_t length);
BinarySieve build_binary_sieve(const BinarySpace& space, const LineFilters& settings, const BinaryItems* items = nullptr);
// The models line: its FilterLine (kind "models", for listing the filters it offers) and its stack.
FilterLine models_filter_line(uint32_t vertices, uint32_t faces, uint32_t coords);
ModelSieve build_model_sieve(const ModelSpace& space, const LineFilters& settings);

// The books line's stacks: the cover on the cover (image) line, the title on one page, and the
// pages on all pages read as one text (P * L symbols; no stack when P = 0).
struct BookStacks
{
    FilterStack cover, title, pages;
};
BookStacks build_book_stacks(const Line& cover, const Line& page, uint32_t pages, const BookFilters& settings);

// A composition's four stacks: its cover on the image line, its title (a unit of `title`, the
// titled lines' title, or none), its units on their own line, each unit judged by itself, and its
// `units` units joined, on that line `units` units long (joined_filter_line).
struct CompositionStacks
{
    FilterStack cover, title, units, joined;
};
CompositionStacks build_composition_stacks(const Line& cover, const std::optional<FilterLine>& title, const Line& unit, uint32_t units,
                                           const CompositionFilters& settings);
// A unit line `n` units long, as the joined stack sees it: n times the positions (a video's
// frames n times as many). Throws if that is beyond what a unit can hold.
FilterLine joined_filter_line(const FilterLine& unit, uint32_t n);

} // namespace sieve::cli
