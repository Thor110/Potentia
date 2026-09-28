// Sieve hallway — the filter designer's model (docs/FILTER-PLUGINS.md section 12): the filter being
// made, as parts the designer's nodes edit, written to and read from a sieve-filter-v1 file.
//
// The file is the source of truth: the designer reads a .sfilter into these parts, and writes them
// back as one, so a filter made here and one written by hand are the same kind of thing, hashed,
// versioned and checked by the oracle alike. Where each node sits on the canvas is kept in the file
// too, as comments the parser skips ("; designer: node header 40 60").
//
// Everything the test panel shows is worked out here from the written file: it is compiled from a
// scratch folder (with any word lists being edited), so nothing touches the filters folder until
// Save, which writes <id>-v<version>.sfilter and its lists there and registers the filter at once.
// A saved version is never overwritten with different contents: Save then offers the next version.
#pragma once

#include "sieve/dfa.hpp"
#include "sieve/filter.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace hallway::design {

struct Pos
{
    float x = 0, y = 0;
};

struct Doc
{
    // Header.
    std::string id = "my-filter", author = "", origin = "human", symbols = "lower27", describe = "";
    uint32_t version = 1;
    std::vector<std::string> lines{"text"};
    // The entry node: filters switched on with this one, each with the settings it pins.
    struct Prerequisite
    {
        std::string name;
        std::vector<std::pair<std::string, std::string>> pins;
    };
    std::vector<Prerequisite> prerequisites;
    // Parameters: int (default, min, max) or dict (default only).
    struct Param
    {
        std::string name, kind = "int", def = "1", min = "1", max = "10", text;
    };
    std::vector<Param> params;
    // The rule: "tokens" (word sets) or "table" (states and transitions, as text).
    std::string form = "tokens";
    std::string separator = "\" \"";
    bool cut = false;
    struct Set
    {
        std::string name, source; // dict:ID, dict:{param}, list:FILE
        bool first = true, last = true;
    };
    std::vector<Set> sets;
    std::vector<std::pair<std::string, std::string>> follows;
    std::vector<std::string> table; // the table form's lines, as written
    // Word lists being edited (file name -> words, one a line), written beside the plugin on save.
    std::map<std::string, std::string> lists;
    // Where each node sits: "header", "requires", "params", "rule", "set:<name>", "test".
    std::map<std::string, Pos> layout;

    std::string name() const { return id + "-v" + std::to_string(version); }
    std::string file_name() const { return name() + ".sfilter"; }

    // The whole file.
    std::string to_text() const;
    // A file read into parts (throws with the reason when it is not a sieve-filter-v1 file); lists
    // it names are read from `folder`.
    static Doc from_text(const std::string& text, const std::filesystem::path& folder);
    // Starting points: a token filter over one dictionary, and a small table.
    static Doc new_tokens();
    static Doc new_table();
};

// What the test panel shows, worked out from the file at one length.
struct TestResult
{
    std::string error;          // the file does not compile: why (with its line), or empty
    size_t declared = 0, minimal = 0;
    std::string data;           // the lists' and dictionaries' hashes
    bool counted = false;       // the table fitted the budget
    std::string survivors, excluded;
    std::vector<std::string> samples; // survivors by rank, as text
    std::shared_ptr<const sieve::Dfa> dfa;
    sieve::FilterLine line;
};

// The line a filter written for these symbols is tested on, at this length.
sieve::FilterLine test_line(const std::string& symbols, uint32_t length);

// Compiles the file (from a scratch folder with the lists) and counts at `length`.
TestResult test(const Doc& d, uint32_t length);

// Whether the text passes, and if not, where and why, in words.
std::string judge(const Doc& d, const TestResult& t, const std::string& text);

// How the rule compares with every custom filter registered for the line (same, stricter,
// looser), at their default settings; slow for large dictionaries, so the screen runs it in the
// background.
std::vector<std::string> relations(const Doc& d, const TestResult& t);

// Where Save writes: the installation's filters folder.
std::filesystem::path filters_folder();

// Saves the file and its lists to the filters folder and registers it. Returns what happened, for
// the status line; `next_version` receives the version to save as instead when this one exists
// with other contents.
std::string save(const Doc& d, uint32_t& next_version);

} // namespace hallway::design
