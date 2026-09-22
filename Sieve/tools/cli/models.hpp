// Sieve CLI — the model registry (data/models/models.tsv) and training corpora.
#pragma once

#include "sieve/alphabet.hpp"
#include "sieve/model.hpp"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace sieve::cli {

struct ModelEntry
{
    std::string id, file, symbols, sha256, description;
    bool is_default = false; // the default model for its symbols
    std::filesystem::path path;
};

struct ModelRegistry
{
    std::filesystem::path folder;
    std::vector<ModelEntry> entries;
    const ModelEntry* find(const std::string& id) const;
    const ModelEntry* default_for(const std::string& symbols) const;
};

// ./data/models first, then "models" next to the executable. Returns an empty registry if
// neither exists (guided addressing is then simply unavailable). Throws if one is malformed.
ModelRegistry load_model_registry();

// --model value -> a loaded model checked against its pinned hash. Empty value: the default
// model for `symbols` (nullptr if there is none). A value with a path separator or ending in
// .model is loaded as a file directly (unpinned). Throws on unknown ids, missing files, a hash
// mismatch, or a model for different symbols.
struct LoadedModel
{
    std::string id; // registry id, or "file"
    std::string path;
    std::shared_ptr<const CharModel> model;
};
LoadedModel resolve_model(const std::string& value, const std::string& symbols);

// A training corpus manifest: "file <TAB> encoding <TAB> role <TAB> sha256" per line,
// encoding utf-8 or latin-1, role train or test.
struct CorpusFile
{
    std::string file, encoding, role, sha256;
};
struct Corpus
{
    std::filesystem::path manifest;
    std::string manifest_sha256;
    std::vector<CorpusFile> files;
};
Corpus load_corpus(const std::string& manifest_path);

// Reads one corpus file from `dir`, checks its hash and returns it as UTF-8.
std::string read_corpus_file(const std::filesystem::path& dir, const CorpusFile& f);

// Text as one canonical symbol stream (canon-text-v2, no unit splitting or padding).
std::vector<uint32_t> canonical_stream(const std::string& utf8, const Alphabet& alphabet);

// The training stream of a corpus: its "train" files in order, canonicalised, joined by one
// SPACE. Returns the stream; `description` receives the provenance recorded in the model.
std::vector<uint32_t> training_stream(const Corpus& corpus, const std::filesystem::path& dir, const Alphabet& alphabet,
                                      std::string& description);

} // namespace sieve::cli
