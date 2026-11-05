// Sieve CLI — the dictionary registry (data/dictionaries/dictionaries.tsv).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace sieve::cli {

struct DictionaryEntry
{
    std::string id, file, language, sha256, description;
    bool is_default = false;
    std::filesystem::path path; // resolved location of the file
};

struct Registry
{
    std::filesystem::path folder; // where dictionaries.tsv was found
    std::vector<DictionaryEntry> entries;
    const DictionaryEntry& default_entry() const;
    const DictionaryEntry* find(const std::string& id) const;
};

// Finds and parses the registry: ./data/dictionaries (repository root) first, then the
// "dictionaries" folder next to the executable. Throws if neither exists or it is malformed.
Registry load_registry();

struct ResolvedDictionary
{
    std::string id;       // registry id, or "file" for a path given directly
    std::string path;
    std::string expected_sha256; // empty when not pinned by the registry
};

// --dict value -> dictionary. Empty: the registry default. A registry id selects that entry
// (35, 60 and 80 are kept as short forms of scowl-en-35/60/80). Anything containing a path
// separator or ending in .txt is used as a file path directly.
ResolvedDictionary resolve_dictionary(const std::string& value);

// The program's own folder. A library loaded into another program (the thumbnail handler, in
// Windows' dllhost) names its own with set_executable_dir, so its data folders are found beside it.
std::filesystem::path executable_dir();
void set_executable_dir(const std::filesystem::path& folder);

// The dictionaries an installer program carries (unpacked by use_carried_dictionaries into a
// folder of their own): looked for there first, before data/dictionaries and the installation's.
void set_dictionaries_folder(const std::filesystem::path& folder);

// The installation's own folder, where its data folders are: the program's folder, or the one
// above it when the program is in the installation's tools folder (a release puts sieve and
// sieve-install in tools\, so that hallway is the one program at the top). Found by the name
// "tools" and a dictionaries folder beside it; a build folder, with every program together, is its
// own installation folder.
std::filesystem::path install_dir();

} // namespace sieve::cli
