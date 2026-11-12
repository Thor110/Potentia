// Sieve — finding and loading filter plugins (sieve/plugin.hpp, docs/FILTER-PLUGINS.md).
//
// Every *.sfilter file (and every .sieve holding them, below) in the `filters` folder of the installation (beside the programs, or the
// folder above an installation's tools\), and in data/filters when run from the repository, is
// read once, at start-up, and registered. A file that does not load is listed with the reason,
// never skipped silently: `sieve filters --plugins` and the setup menu show the list. Two files
// with the same name and version but different bytes are both refused (a version is never
// edited), as is a plugin that takes a built-in filter's id.
#pragma once

#include <filesystem>
#include <string>
#include <cstdint>
#include <utility>
#include <vector>

namespace sieve::cli {

struct PluginFile
{
    std::string path, name, sha256, error; // error: empty when it loaded
    std::string packed;                    // the .sieve it was read from, if it came packed (path: its decoded copy)
};

// Loads and registers the plugins (the first call does the work; later calls return the list).
const std::vector<PluginFile>& load_plugins();
// Where compiled automata are kept on disk (see plugins.cpp): shared by every build.
std::filesystem::path plugin_cache_dir();

// Filters kept as .sieve (IDEAS 19.185). A .sieve in a filters folder holds a filter's .sfilter and
// the lists it reads, as the designer saves them. At start-up each is decoded once into the cache
// (plugin_cache_dir()/sfilters/<the .sieve's SHA-256>/, with an index of every member's SHA-256 and
// size, checked again at every start), and each .sfilter in it is loaded exactly as a loose one is,
// reading its lists from beside it; its identity is its own text's SHA-256, packed or not. A .sieve
// that unfolds to more than kFilterSieveRoof is refused. Returns the folder its members are in.
// Throws std::runtime_error saying what is wrong.
inline constexpr uint64_t kFilterSieveRoof = uint64_t(64) << 20;
std::filesystem::path decoded_filter_sieve(const std::filesystem::path& sieve_file);
// Every file in a folder (a filter and its lists), packed as Sieve packs a folder (the newest
// instructions), written to `out` as a .sieve. Throws.
void pack_filter_sieve(const std::filesystem::path& folder, const std::filesystem::path& out);
// The .sfilter files a filters .sieve holds, by name, with their text (decoded in memory, each
// member checked). Throws.
std::vector<std::pair<std::string, std::string>> sfilters_in_sieve(const std::filesystem::path& sieve_file);
// A filter's file, whichever way it is kept: a .sieve gives its decoded .sfilter (in the cache, beside
// its lists; the first by name if it holds several); anything else is itself. Throws.
std::filesystem::path sfilter_file(const std::filesystem::path& file);

} // namespace sieve::cli
