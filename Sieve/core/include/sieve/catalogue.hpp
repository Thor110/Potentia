// Sieve — the catalogue of formats, stage 1: what Sieve already knows, gathered into one web
// (IDEAS 19.165; docs/CATALOGUE-ROADMAP.md).
//
// A format here is anything agreed for writing something down so it can be read back: a file format,
// an alphabet, a script, a notation, a character or animation line. Stage 1 gathers the formats
// Sieve's own code already names, so the catalogue is made from the code and cannot fall behind it:
//   - the file kinds Sieve recognises by their first bytes (file-kinds-v1, sieve/filekind.hpp);
//   - the model importers (sieve/importers.hpp);
//   - the built-in alphabets and the named Unicode blocks (sieve/alphabet.hpp);
//   - Sieve's own formats (.sieve, characters, species, animations, the RAID);
//   - and a first layer of general entries above them (text, image, sound, writing, compression...),
//     each leading up to one root, "information".
//
// The wiki principle, checked: every entry has a path of "part-of" links up to "information", and a
// test fails if one does not. Any two entries are joined by some path through the links (path()).
//
// "sieve-format-v1", one entry as text (one field a line, in this order; links as "link TYPE TARGET",
// in order; empty fields left out):
//     sieve-format-v1
//     id ID
//     kind KIND            (general, file, importer, alphabet, script, sieve)
//     name NAME
//     link part-of TARGET
//     recognised HOW       (how a program tells it: a signature in hex, ".." any byte; Unicode ranges)
//     sieve WHAT           (what Sieve does with it)
//     source WHERE         (where in Sieve it was gathered from)
// Its SHA-256 is that version of the entry; a change makes a new version, the id staying the same.
// SPECIFICATIONS 12.0m writes the format out.
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace sieve {

inline constexpr const char* kFormatVersion = "sieve-format-v1";

struct FormatLink
{
    std::string type; // "part-of", "carried-by"
    std::string to;
};

struct FormatEntry
{
    std::string id, kind, name;
    std::vector<FormatLink> links;
    std::string recognised, sieve, source;
    std::string text() const;
    std::string version() const; // SHA-256 of the text
};

// Stage 2: an entry's text read back, checked: the version line first; then id (lower-case letters,
// digits, '-', '.' and '_'), kind and name, each present and on one line; then the links ("link TYPE
// TARGET", TYPE a lower-case word); then recognised, sieve and source where given, in that order;
// every line ending in a line feed, nothing else. Throws std::invalid_argument saying what is wrong.
FormatEntry read_format_entry(const std::string& text);

// Every entry gathered from the code (above), in a fixed order: the general layer first, then by
// where each was gathered from. Ids are unique.
const std::vector<FormatEntry>& format_catalogue();
const FormatEntry* format_by_id(std::string_view id); // null if there is none

// Stage 2: entries written by hand, one a file (<ID>.format) in a folder (data/catalogue), read in the
// order of their names, each checked as read_format_entry checks it and against its file's name. They
// join the entries from the code; an id already there is refused. Throws std::invalid_argument naming
// the file. A folder that is not there gives none.
std::vector<FormatEntry> read_format_folder(const std::string& dir);
// The code's entries and a folder's, together (the folder's after), checked as one: ids unique, every
// link naming an entry. Throws std::invalid_argument.
std::vector<FormatEntry> format_catalogue_with(const std::string& dir);
// The same with several folders (data/catalogue and data/languages: one web, IDEAS 19.183), read in
// the order given.
std::vector<FormatEntry> format_catalogue_with(const std::vector<std::string>& dirs);

// The chain of "part-of" links from an entry up to "information" (the entry first, the root last);
// empty if it does not reach the root. Over the code's entries, or over a given set.
std::vector<std::string> format_lineage(std::string_view id);
std::vector<std::string> format_lineage(const std::vector<FormatEntry>& all, std::string_view id);
// The shortest chain of links between two entries, followed either way (empty if none).
std::vector<std::string> format_path(std::string_view from, std::string_view to);
std::vector<std::string> format_path(const std::vector<FormatEntry>& all, std::string_view from, std::string_view to);

} // namespace sieve
