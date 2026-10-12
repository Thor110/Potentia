// Sieve CLI — a language model given as a folder (see model_folder.hpp).
#include "cli/model_folder.hpp"

#include "sieve/json.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

bool is_model_folder(const fs::path& dir)
{
    std::error_code ec;
    return fs::is_regular_file(dir / "config.json", ec) && fs::is_regular_file(dir / "model.safetensors", ec) &&
           fs::is_regular_file(dir / "tokenizer.json", ec);
}

ModelFolder load_model_folder(const fs::path& dir, unsigned threads)
{
    auto read = [&](const char* name) {
        std::ifstream in(dir / name, std::ios::binary);
        if (!in) throw std::invalid_argument("a model folder holds config.json, model.safetensors and tokenizer.json: no " + std::string(name));
        std::ostringstream s;
        s << in.rdbuf();
        return s.str();
    };
    ModelFolder m;
    const std::u8string n = dir.filename().u8string();
    m.name = std::string(n.begin(), n.end());
    m.tokenizer = std::make_unique<llm::Tokenizer>(json::parse(read("tokenizer.json")));
    const json::Value config = json::parse(read("config.json"));
    if (!fs::is_regular_file(dir / "model.safetensors")) throw std::invalid_argument("a model folder holds config.json, model.safetensors and tokenizer.json: no model.safetensors");
    m.model = std::make_unique<llm::Model>(config, dir / "model.safetensors", threads);
    m.chatml = m.tokenizer->find("<|im_start|>") && m.tokenizer->find("<|im_end|>");
    return m;
}

} // namespace sieve::cli
