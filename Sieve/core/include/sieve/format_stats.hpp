// Sieve — the catalogue of formats, stage 4: figures measured as formats are used (IDEAS 19.165;
// docs/CATALOGUE-ROADMAP.md).
//
// Every figure an entry shows is one Sieve measured on this machine, never one claimed: each time a
// file is measured (`sieve catalogue --measure`) or a model imported (`sieve import`), a record is
// added to that format's file in the person's stats folder, outside the repository. A record names no
// file: only its kind, what was done, the figures, the date and the machine's label (what the person
// calls it; "unnamed" unless they say).
//
// "sieve-format-stat-v1", one record a line, its fields apart by single spaces, in this order:
//     sieve-format-stat-v1 date=YYYY-MM-DD machine=LABEL what=WHAT result=RESULT KEY=N ...
// LABEL, WHAT and RESULT are 1 to 64 of a-z, 0-9, '.', '_' and '-'; each KEY likewise, the keys in
// strictly increasing order, each N a whole number of at most 19 digits. WHAT is
//   pack     one file packed alone: bytes (its size), sieve (the .sieve made of it alone), lzma2 (LZMA2
//            alone, 7z's method, at Sieve's setting), deflate, make-us and install-us (microseconds);
//            RESULT lossless (installed back and the same bytes, by SHA-256) or differs;
//   import   one model imported: vertices-in, vertices-kept, faces-in, faces-kept, joints, animations,
//            left-out (the things it says it left out); RESULT whole (nothing left out and every
//            vertex and face kept) or partial.
// A file of stats is these lines, each ending in a line feed (a carriage return before it is
// ignored), kept as <stats folder>/<entry id>.stats; a line that is not a record is skipped when read,
// and said so, rather than ending the reading.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace sieve {

inline constexpr const char* kFormatStatVersion = "sieve-format-stat-v1";

struct FormatStat
{
    std::string date, machine, what, result;
    std::vector<std::pair<std::string, uint64_t>> figures; // in order of key
    uint64_t get(const std::string& key) const;            // 0 if not given
    bool has(const std::string& key) const;
    std::string text() const;                              // the line, without its line feed
};

// Throws std::invalid_argument saying what is wrong.
FormatStat read_format_stat(const std::string& line);

// The folder: `given` if not empty; else $SIEVE_STATS; else %LOCALAPPDATA%\Sieve\stats on Windows,
// $XDG_DATA_HOME/sieve/stats or ~/.local/share/sieve/stats elsewhere.
std::filesystem::path format_stats_folder(const std::string& given = "");
// The machine's label: `given` if not empty, else $SIEVE_MACHINE, else "unnamed"; made to fit LABEL
// (lower case, other characters as '-', at most 64).
std::string format_stats_machine(const std::string& given = "");
// Today's date (UTC), YYYY-MM-DD.
std::string format_stats_today();

// Adds a record to an entry's file (made, with its folder, if need be). Throws std::runtime_error if
// it cannot be written.
void add_format_stat(const std::filesystem::path& folder, const std::string& entry_id, const FormatStat& s);
// An entry's records, oldest first; `skipped` (if given) counts lines that were not records.
std::vector<FormatStat> format_stats(const std::filesystem::path& folder, const std::string& entry_id, size_t* skipped = nullptr);
// The entries that have records in the folder, in order of id.
std::vector<std::string> measured_entries(const std::filesystem::path& folder);

// What an entry's records say, a line each ("measured", "imported"), every figure added up or a
// median from them, with how many records and machines it came from; empty if none.
std::vector<std::string> format_stats_summary(const std::vector<FormatStat>& stats);
// The same in short lines for a narrow panel (the setup menu's SYSTEMS page): "packed 2x: 12.6% of
// its bytes (LZMA2 16.8%)", "2 of 2 lossless; 27.8 s a MiB to make", "imported 1x: 8 of 1,857
// vertices kept".
std::vector<std::string> format_stats_short(const std::vector<FormatStat>& stats);

} // namespace sieve
