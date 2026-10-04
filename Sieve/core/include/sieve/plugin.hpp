// Sieve — filter plugins (docs/FILTER-PLUGINS.md): filters written as data, in `sieve-filter-v1`
// files (.sfilter), compiled to a deterministic automaton (sieve/dfa.hpp) that judges units and,
// where the table fits, counts, ranks and compacts their survivors exactly.
//
// A plugin is registered like a built-in filter (a FilterSpec named <id>-v<version>), so it
// appears in every list, ticks into stacks, and records its provenance: the file's SHA-256 and
// every parameter's value. Its parameters are declared in the file (`param`) and adjusted like a
// built-in filter's, from the menu or the filter settings, so a rule can be tried at several
// settings without editing the file; the file is still the version, and a changed file is a new
// filter.
//
// The format, line by line (`;` starts a comment, as does a line whose first character is `#`):
//
//   sieve-filter-v1
//   id        no-double-space           lower-case letters, digits and hyphens
//   version   1
//   author    Edward James Gordon
//   origin    human                     human | ai-directed | ai
//   lines     text                      the lines it applies to: text image video audio
//   symbols   lower27                   an alphabet id, palette:<id>, notes104, or any
//   describe  No two SPACEs in a row.
//   requires  clean-v1                  filters switched on with this one (by full name), one a line,
//   requires  max-run-data-v1  max=3    each with any settings it pins
//   param     max  int  3  1  64  Longest run allowed   (name, kind, default, min, max, text)
//   class     space  " "                symbols: "literal", a-z, @7, @0..@9 (by digit), or *
//   class     letter a-z                (* is every symbol in no other class)
//   states    {max + 2}                 numbers are integers, names, or {expressions}
//   start     0
//   accept    1..{max}                  states, or ranges of them
//   t         0  letter  1              from, symbols (a class, or as in a class), to
//   for       i  1  {max - 1}           repeats the lines up to `done` for i = 1 .. max-1
//   t         {i}  letter  {i + 1}
//   done
//   end
//
// The token form (instead of classes, states and transitions): tokens are runs of symbols between
// separators, each a word of some set; one separator between tokens, at most one at each end, and
// at least one token.
//
//   tokens    separator " "             the symbols that separate tokens (as in a class)
//   edges     whole                     whole | cut: cut lets a token touching the unit's start be
//                                       a suffix of a word, one touching its end a prefix, and one
//                                       touching both a substring (a page cut from running text)
//   param     dictionary dict default  a dictionary setting: a registered id, or default
//   set       word  dict:{dictionary}   a registered dictionary (dict:ID, or dict:{a dict param})
//   set       det   list:determiners.txt  a word list beside the plugin, one word a line
//   set       noun  tags:pos.tsv:Np     the words of a tagged list (word<TAB>tags) carrying any of
//                                       the tags given (Np-DP: any of N p, and none of D P),
//                                       a word judged by the tags of all its lines together;
//                                       files+joined+with+plus are read as one
//   follow    det   noun                which set may follow which (none at all: any order)
//   first     det noun                  the sets the first token may be (default: all)
//   last      noun                      the sets the last token may be (default: all)
//
// A word in several sets may be any of them: a unit passes if some reading of its tokens chains
// by `follow`. A token cut by an edge (cut) may be any set. A dictionary's words take the line's
// symbols as they are (letters it does not have are skipped, with the dictionary's own rule);
// a list's words must be the line's symbols. Each list's SHA-256 is part of the provenance.
//
// Expressions: + - * / % and brackets over integers, parameters and `for` variables; / and % are
// for non-negative numbers only.
//
// sieve-filter-v2 (a file's first line says which it is; a v1 file reads exactly as it always has)
// is v1 and, in the table form:
//
//   {abs(p - q) <= leap && !(p == 0)}   comparisons (== != < <= > >=) give 1 or 0; && || and !
//                                       take 0 as false and anything else as true; min(a, b),
//                                       max(a, b) and abs(a). Lowest to highest: || && comparisons
//                                       + - * / % and unary - !. Both sides are always worked out.
//   if        {EXPR}                    the lines up to `else` or `fi` when EXPR is not 0, and
//   else                                those after `else` (if there is one) when it is; if
//   fi                                  blocks nest with for blocks
//   param     scale choice major major,minor,blues   the default, then the choices (letters,
//                                       digits, # and -); in expressions a choice is its place
//                                       in the list, from 0
//   symbols   notes*                    every line whose symbols' id starts with what comes
//                                       before the * (here the note lines: notes104 and
//                                       every notes2 set; on several voices a plugin judges each)
//
// and constants of the line, in expressions: BASE (its number of symbols), and on a note line
// PITCHES (pitches, not counting the rest), DURATIONS and LOW (the MIDI number of the lowest
// pitch; notes104: 25, 4, 60; a notes2 set: its own): a note's symbol is pitch * DURATIONS + duration, pitch 0 the rest and pitch 1 LOW. A
// parameter or a for variable may not take any of these names, nor min, max or abs. A (state, symbol) given two different targets is an error; one
// given none is the dead end. The oracle (reference/sieve_ref.py plugin) reads the same files with
// its own parser and engine, and CI compares the two.
#pragma once

#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"

#include <deque>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kPluginFormat = "sieve-filter-v1";
inline constexpr const char* kPluginFormat2 = "sieve-filter-v2";

struct PluginDef; // the parsed file (plugin.cpp)

struct PluginHeader
{
    std::string id, author, origin, symbols, describe, sha256;
    std::string form; // "table" or "tokens"
    uint32_t version = 0;
    uint32_t format = 1; // sieve-filter-v1 or -v2
    std::vector<std::string> lines;
    std::vector<FilterParam> params;
    std::vector<FilterSpec::Prerequisite> prerequisites;
    std::string name() const { return id + "-v" + std::to_string(version); }
};

// Parses a plugin file's text; `sha256` is the file's, recorded as its identity. Throws
// std::invalid_argument naming the line on anything malformed.
// `folder` is where the file is, for word lists beside it (`set NAME list:FILE`).
std::shared_ptr<const PluginDef> parse_plugin(const std::string& text, const std::string& sha256, const std::string& folder = {});
const PluginHeader& plugin_header(const PluginDef& p);

// Whether the plugin is written for this line (its kind, and its symbols).
bool plugin_applies(const PluginDef& p, const FilterLine& line);

// The automaton for this line and these parameter values (missing ones take their defaults), as
// declared (`declared_states` receives how many the file declared; for the token form, how many
// distinct words, since the states made on the way depend on how it is built) and minimised.
// `resources` supplies registered dictionaries (the token form's `dict:` sets).
// `data` receives the hashes of the word lists and dictionaries used, for the provenance.
// `step`, if given, is told what is being done as it happens ("building the automaton"), for a
// screen that shows progress; it is called from the compiling thread.
Dfa compile_plugin(const PluginDef& p, const FilterLine& line, const FilterValues& values, const FilterResources& resources,
                   size_t* declared_states = nullptr, std::string* data = nullptr,
                   const std::function<void(const std::string&)>& step = {});

// Counting needs a table of states x (length + 1) numbers; past this many bytes a plugin judges
// only (mark, hide, excluded), and the menu says why.
// The filter memory: what one count may take for its tables, in bytes. Every filter that counts by
// a table, and the automata merged for a stack, keep within it, and judge only past it. It is a
// setting, not a constant (the setup menu's FILTER MEMORY, saved with the application's settings;
// `sieve --filter-memory MB`), 512 MB until it is set. Safe to read from any thread.
inline constexpr double kDefaultFilterMemory = 512.0 * 1024 * 1024;
double filter_memory();
void set_filter_memory(double bytes);
// The merge cache: what share of the filter memory (0 to 1) keeps the plugins' merged automata
// between counts (filter.cpp), so a stack counted again, or the same plugins on another line of
// the same shape, does not merge them again. The rest is left for the counting tables. A setting
// (the setup menu's MERGE CACHE; `sieve --merge-cache PCT`), a half until it is set. 0 keeps none.
inline constexpr double kDefaultMergeCacheShare = 0.5;
double merge_cache_share();
void set_merge_cache_share(double share);
// A size for a person: "2.1 GB", "512 MB".
std::string memory_text(double bytes);
// Why a counting table is refused, in words: "<what> needs 2.1 GB of memory at this length, over
// the filter memory (512 MB)". Without a size (bytes <= 0): "<what> would need more memory at this
// length than the filter memory (512 MB)".
std::string over_table_limit(const std::string& what, double bytes = 0);

// The registered filter for a plugin: its make() compiles for the line and values, and builds the
// ranker when the table fits the budget.
FilterSpec plugin_spec(std::shared_ptr<const PluginDef> p);

// Compiled automata are kept for the process, and, once the application names a folder, on disk
// as well (sieve-dfa-cache-v1): one file a plugin, symbols and settings, holding the minimal
// automaton and a SHA-256 of all of it, checked on every load, so a large grammar compiles once
// rather than at every start. An unreadable or mismatched file is ignored and rewritten. No folder
// (the default, and the tests): memory only.
void set_plugin_cache_dir(const std::filesystem::path& dir);
// Whether a plugin (by its SHA-256 and parameters) is compiled for the line with these values
// already, in memory or on disk: building a stack with it will then be quick.
bool plugin_compiled(const std::string& sha256, const FilterLine& line, const std::vector<FilterParam>& params, const FilterValues& values);

// A plugin filter's automaton (for combining a stack of plugins into one ranker), or nullptr.
const Dfa* plugin_dfa(const Filter& f);
// A filter whose rule is an automaton made in code rather than read from a file (not-written-v1,
// sieve/written.hpp): judged, counted and combined with plugins exactly as a plugin's is.
std::unique_ptr<Filter> make_dfa_filter(Dfa dfa, uint32_t length, std::string provenance);

// The plugins in use, registered once at start-up (the application finds and loads the files;
// tools/cli/plugins.hpp). find_filter and filters_for look here after the built-in filters.
void register_plugins(std::vector<FilterSpec> specs);
// One more, while running (the designer's save): false if a filter of that name is registered
// already. Registered filters never move, so pointers to them stay good.
bool add_plugin(FilterSpec spec);
const std::deque<FilterSpec>& plugin_registry();

} // namespace sieve
