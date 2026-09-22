// Sieve CLI — filter settings (sieve-filters.ini) and the pinned data filters use.
//
// The settings file lives next to the executable (outside the folders a rebuild copies, so a
// rebuild never overwrites it), or wherever --filters PATH points. It is plain text:
//
//   [text]
//   mode = hide                  ; off | mark | hide | compact
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

#include "sieve/filter.hpp"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace sieve::cli {

enum class FilterMode { Off, Mark, Hide, Compact };
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

struct FilterConfig
{
    LineFilters lines[4]; // text, image, audio, video
    BookFilters books;
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

FilterLine filter_line(const Line& line);
// The line's stack from its settings (filters that do not apply to this line are skipped).
FilterStack build_stack(const Line& line, const LineFilters& settings);
FilterStack build_stack(const FilterLine& line, const LineFilters& settings);

// The books line's stacks: the cover on the cover (image) line, the title on one page, and the
// pages on all pages read as one text (P * L symbols; no stack when P = 0).
struct BookStacks
{
    FilterStack cover, title, pages;
};
BookStacks build_book_stacks(const Line& cover, const Line& page, uint32_t pages, const BookFilters& settings);

} // namespace sieve::cli
