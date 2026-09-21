// Potentia CLI — the dictionary registry (data/dictionaries/dictionaries.tsv).
#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace potentia::cli {

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

std::filesystem::path executable_dir();

} // namespace potentia::cli
