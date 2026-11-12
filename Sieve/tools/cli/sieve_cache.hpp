// Sieve — Sieve's own files kept as .sieve, read on the spot and cached (IDEAS 19.185).
//
// Filters, catalogue entries and anything else Sieve reads from its own folders may be kept there as
// Sieve instructions (a .sieve holding the files), as maps already are. Each is read the same way:
// decoded once into the cache, plugin_cache_dir()/<kind>/<the .sieve's SHA-256>/, beside an `index` of
// every member's SHA-256 and size; at each later start every member is checked against the index and
// the .sieve decoded again where anything differs. A .sieve that unfolds to more than its roof is
// refused. The files are then read from that folder exactly as loose ones are.
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace sieve::cli {

// The folder a .sieve's files are decoded into, under `kind` ("sfilters", "catalogue"). Throws
// std::runtime_error saying what is wrong.
std::filesystem::path decoded_sieve(const std::filesystem::path& sieve_file, const std::string& kind, uint64_t most);
// Every file in a folder packed as Sieve packs a folder (the newest instructions), read back and
// checked, then written to `out`. Throws.
void pack_folder_sieve(const std::filesystem::path& folder, const std::filesystem::path& out, uint64_t most);
// A .sieve's files in memory, each checked, nothing written. Throws.
std::vector<std::pair<std::string, std::vector<uint8_t>>> files_in_sieve(const std::filesystem::path& sieve_file, uint64_t most);

// The catalogue's folders (data/catalogue and data/languages: one web, IDEAS 19.183), each where it
// is found first: in data/ here (running from the repository), beside the program, in the folder above
// it (a release keeps sieve in tools/) or that folder's data/. Then, for each, the decoded copy of every
// .sieve in it (catalogue entries kept packed, decoded at start-up and cached as above, kind
// "catalogue"), in order of name. A .sieve that does not decode is left out and said in `notes`.
inline constexpr uint64_t kCatalogueSieveRoof = uint64_t(64) << 20;
std::vector<std::string> catalogue_folders(std::vector<std::string>* notes = nullptr);
// One folder of entries as the same: itself, then the decoded copy of every .sieve in it.
std::vector<std::string> catalogue_folder(const std::filesystem::path& folder, std::vector<std::string>* notes = nullptr);

} // namespace sieve::cli
