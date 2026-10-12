// Sieve CLI — a language model given as a folder, as `sieve chat --model` reads one: config.json,
// model.safetensors and tokenizer.json, run by Sieve's own engine (sieve/llm.hpp). Shared by the
// setup menu's AI rows and the hallway's CHAT tab.
#pragma once

#include "sieve/llm.hpp"
#include "sieve/llm_tokenizer.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace sieve::cli {

// Whether the folder holds the three files.
bool is_model_folder(const std::filesystem::path& dir);

struct ModelFolder
{
    std::string name; // the folder's name
    std::unique_ptr<llm::Tokenizer> tokenizer;
    std::unique_ptr<llm::Model> model;
    bool chatml = false; // its tokenizer has ChatML's tokens: it is talked to as a chat (llm::Chat)
};
// Loads it (seconds for a model of hundreds of megabytes). Throws std::invalid_argument when a file
// is missing or of a kind the engine does not run.
ModelFolder load_model_folder(const std::filesystem::path& dir, unsigned threads = 0);

} // namespace sieve::cli
