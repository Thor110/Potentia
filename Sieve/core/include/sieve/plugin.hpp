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
// Expressions: + - * / % and brackets over integers, parameters and `for` variables; / and % are
// for non-negative numbers only. A (state, symbol) given two different targets is an error; one
// given none is the dead end. The oracle (reference/sieve_ref.py plugin) reads the same files with
// its own parser and engine, and CI compares the two.
#pragma once

#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"

#include <memory>
#include <string>
#include <vector>

namespace sieve {

inline constexpr const char* kPluginFormat = "sieve-filter-v1";

struct PluginDef; // the parsed file (plugin.cpp)

struct PluginHeader
{
    std::string id, author, origin, symbols, describe, sha256;
    uint32_t version = 0;
    std::vector<std::string> lines;
    std::vector<FilterParam> params;
    std::string name() const { return id + "-v" + std::to_string(version); }
};

// Parses a plugin file's text; `sha256` is the file's, recorded as its identity. Throws
// std::invalid_argument naming the line on anything malformed.
std::shared_ptr<const PluginDef> parse_plugin(const std::string& text, const std::string& sha256);
const PluginHeader& plugin_header(const PluginDef& p);

// Whether the plugin is written for this line (its kind, and its symbols).
bool plugin_applies(const PluginDef& p, const FilterLine& line);

// The automaton for this line and these parameter values (missing ones take their defaults), as
// declared (`declared_states` receives how many the file declared) and minimised.
Dfa compile_plugin(const PluginDef& p, const FilterLine& line, const FilterValues& values, size_t* declared_states = nullptr);

// Counting needs a table of states x (length + 1) numbers; past this many bytes a plugin judges
// only (mark, hide, excluded), and the menu says why.
inline constexpr double kPluginTableBudget = 512.0 * 1024 * 1024;

// The registered filter for a plugin: its make() compiles for the line and values, and builds the
// ranker when the table fits the budget.
FilterSpec plugin_spec(std::shared_ptr<const PluginDef> p);

// A plugin filter's automaton (for combining a stack of plugins into one ranker), or nullptr.
const Dfa* plugin_dfa(const Filter& f);

// The plugins in use, registered once at start-up (the application finds and loads the files;
// tools/cli/plugins.hpp). find_filter and filters_for look here after the built-in filters.
void register_plugins(std::vector<FilterSpec> specs);
const std::vector<FilterSpec>& plugin_registry();

} // namespace sieve
