#include "models.hpp"

#include "dictionaries.hpp" // executable_dir

#include "sieve/canon.hpp"
#include "sieve/sha256.hpp"
#include "sieve/utf8.hpp"

#include <fstream>
#include <map>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

std::string read_bytes(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) throw std::runtime_error("cannot read " + p.string());
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::vector<std::vector<std::string>> read_tsv(const fs::path& path, size_t fields)
{
    std::istringstream in(read_bytes(path));
    std::vector<std::vector<std::string>> rows;
    std::string line;
    int line_no = 0;
    while (std::getline(in, line))
    {
        ++line_no;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#') continue;
        std::vector<std::string> f;
        std::stringstream ss(line);
        std::string field;
        while (std::getline(ss, field, '\t')) f.push_back(field);
        if (f.size() != fields)
            throw std::runtime_error(path.string() + " line " + std::to_string(line_no) + ": expected " +
                                     std::to_string(fields) + " tab-separated fields, found " + std::to_string(f.size()));
        rows.push_back(std::move(f));
    }
    return rows;
}

bool looks_like_path(const std::string& v)
{
    return v.find('/') != std::string::npos || v.find('\\') != std::string::npos ||
           (v.size() > 6 && v.compare(v.size() - 6, 6, ".model") == 0);
}

// Models are several megabytes; parse each file once per process.
std::shared_ptr<const CharModel> cached_load(const std::string& path)
{
    static std::mutex mu;
    static std::map<std::string, std::shared_ptr<const CharModel>> cache;
    std::lock_guard<std::mutex> lock(mu);
    auto& slot = cache[path];
    if (!slot) slot = std::make_shared<const CharModel>(CharModel::load_file(path));
    return slot;
}

} // namespace

const ModelEntry* ModelRegistry::find(const std::string& id) const
{
    for (const auto& e : entries)
        if (e.id == id) return &e;
    return nullptr;
}

const ModelEntry* ModelRegistry::default_for(const std::string& symbols) const
{
    for (const auto& e : entries)
        if (e.is_default && e.symbols == symbols) return &e;
    return nullptr;
}

ModelRegistry load_model_registry()
{
    std::vector<fs::path> folders = {fs::path("data") / "models"};
    if (const fs::path exe = executable_dir(); !exe.empty()) folders.push_back(exe / "models");
    for (const auto& folder : folders)
    {
        const fs::path manifest = folder / "models.tsv";
        if (!fs::exists(manifest)) continue;
        ModelRegistry r;
        r.folder = folder;
        std::map<std::string, int> defaults;
        for (auto& f : read_tsv(manifest, 6))
        {
            ModelEntry e{f[0], f[1], f[2], f[4], f[5], f[3] == "yes", folder / f[1]};
            if (e.sha256.size() != 64) throw std::runtime_error(manifest.string() + ": sha256 of '" + e.id + "' must be 64 hex digits");
            if (r.find(e.id)) throw std::runtime_error(manifest.string() + ": duplicate id '" + e.id + "'");
            if (e.is_default && ++defaults[e.symbols] > 1)
                throw std::runtime_error(manifest.string() + ": more than one default model for " + e.symbols);
            r.entries.push_back(std::move(e));
        }
        return r;
    }
    return {};
}

LoadedModel resolve_model(const std::string& value, const std::string& symbols)
{
    LoadedModel out;
    std::string expected;
    if (looks_like_path(value))
    {
        out.id = "file";
        out.path = value;
    }
    else
    {
        const ModelRegistry reg = load_model_registry();
        const ModelEntry* e = value.empty() ? reg.default_for(symbols) : reg.find(value);
        if (!e)
        {
            if (value.empty()) return out; // no default model for these symbols
            std::string msg = "unknown model '" + value + "'. Available:";
            for (const auto& m : reg.entries) msg += " " + m.id;
            throw std::invalid_argument(msg + " (see: sieve models), or pass a .model file.");
        }
        if (!fs::exists(e->path)) throw std::runtime_error("model '" + e->id + "' is listed but its file is missing: " + e->path.string());
        out.id = e->id;
        out.path = e->path.string();
        expected = e->sha256;
    }
    out.model = cached_load(out.path);
    if (!expected.empty() && out.model->sha256() != expected)
        throw std::runtime_error("model '" + out.id + "' does not match its pinned SHA-256 (file changed?): " + out.path);
    if (out.model->params().symbols_id != symbols)
        throw std::invalid_argument("model '" + out.id + "' is for " + out.model->params().symbols_id + ", not " + symbols);
    return out;
}

Corpus load_corpus(const std::string& manifest_path)
{
    Corpus c;
    c.manifest = manifest_path;
    c.manifest_sha256 = Sha256::hex(Sha256::hash(read_bytes(manifest_path)));
    for (auto& f : read_tsv(manifest_path, 4))
    {
        CorpusFile cf{f[0], f[1], f[2], f[3]};
        if (cf.encoding != "utf-8" && cf.encoding != "latin-1")
            throw std::runtime_error(manifest_path + ": encoding of " + cf.file + " must be utf-8 or latin-1");
        if (cf.role != "train" && cf.role != "test")
            throw std::runtime_error(manifest_path + ": role of " + cf.file + " must be train or test");
        c.files.push_back(std::move(cf));
    }
    if (c.files.empty()) throw std::runtime_error(manifest_path + " lists no files");
    return c;
}

std::string read_corpus_file(const fs::path& dir, const CorpusFile& f)
{
    const fs::path p = dir / f.file;
    if (!fs::exists(p)) throw std::runtime_error("corpus file missing: " + p.string() + " (fetch it with tools/fetch_corpus.py)");
    const std::string bytes = read_bytes(p);
    if (Sha256::hex(Sha256::hash(bytes)) != f.sha256) throw std::runtime_error("corpus file does not match its SHA-256: " + p.string());
    if (f.encoding == "utf-8") return bytes;
    std::u32string cps;
    cps.reserve(bytes.size());
    for (unsigned char b : bytes) cps.push_back(char32_t(b)); // ISO-8859-1 is the first 256 codepoints
    return utf8_encode(cps);
}

std::vector<uint32_t> canonical_stream(const std::string& utf8, const Alphabet& alphabet)
{
    // Canonicalise in large units, then drop the padding of the last one.
    constexpr uint32_t kChunk = 1u << 20;
    const CanonResult c = canonicalise_text(utf8, alphabet, kChunk, CanonVersion::V2);
    std::vector<uint32_t> out;
    out.reserve(c.canonical_length);
    for (const auto& u : c.units)
        for (char32_t cp : u)
        {
            if (out.size() == c.canonical_length) break;
            out.push_back(*alphabet.digit_of(cp));
        }
    return out;
}

std::vector<uint32_t> training_stream(const Corpus& corpus, const fs::path& dir, const Alphabet& alphabet, std::string& description)
{
    const auto space = alphabet.digit_of(U' ');
    if (!space) throw std::invalid_argument("training needs an alphabet with SPACE");
    std::vector<uint32_t> stream;
    size_t files = 0;
    for (const auto& f : corpus.files)
    {
        if (f.role != "train") continue;
        const auto part = canonical_stream(read_corpus_file(dir, f), alphabet);
        if (part.empty()) continue;
        if (!stream.empty()) stream.push_back(*space);
        stream.insert(stream.end(), part.begin(), part.end());
        ++files;
    }
    description = corpus.manifest.filename().string() + " sha256=" + corpus.manifest_sha256 + " train_files=" +
                  std::to_string(files) + " canon=canon-text-v2";
    return stream;
}

} // namespace sieve::cli
