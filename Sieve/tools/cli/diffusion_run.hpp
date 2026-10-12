// Sieve CLI — diffusion runs (diffusion-v1, sieve/diffusion.hpp; SPECIFICATIONS §12.0g): the run
// folder, training it on the record, and the pictures it draws its training from. Shared by
// `sieve diffuse` and the hallway's AI Training Harness.
//
// A run is a folder:
//   diffusion.ini   the recipe
//   pictures.txt    the training pictures, one a line, each cell a digit in base 36 (a palette of
//                   up to 36 colours), row after row
//   result.txt      what training gave: a checkpoint a line, then the final model's address
// so that anyone can train it again and get the same model, digit for digit.
#pragma once

#include "sieve/aispace.hpp"
#include "sieve/diffusion.hpp"
#include "sieve/filter.hpp"
#include "sieve/space.hpp"
#include "sieve/training.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

// The recipe: diffusion.ini.
struct DiffusionRecipe
{
    training::Recipe train; // shape, learning rate, momentum, version and loss scale
    diffusion::Setup setup;
    std::string palette = "mono";
    uint32_t batch = 8, steps = 4000, checkpoint_every = 500;
    uint64_t seed = 1, start_seed = 7;
    std::string pictures_sha256;

    std::string text() const;
    static DiffusionRecipe parse(const std::string& text);
    void check() const; // throws std::invalid_argument
};

struct DiffusionRun
{
    std::filesystem::path path;
    std::string name;
    DiffusionRecipe recipe;
    std::vector<std::vector<uint32_t>> pictures;
    std::optional<std::string> result; // result.txt, when trained
};

// Whether a folder is a diffusion run (it holds diffusion.ini).
bool is_diffusion_run(const std::filesystem::path& dir);
// Reads a run, checking its pictures against the recipe's SHA-256. Throws if it is not one.
DiffusionRun load_diffusion_run(const std::filesystem::path& dir);
// Writes a new run's diffusion.ini and pictures.txt (the recipe's pictures_sha256 set from them).
DiffusionRun record_diffusion_run(const std::filesystem::path& dir, DiffusionRecipe recipe, std::vector<std::vector<uint32_t>> pictures);

// `n` pictures drawn at random from a stack's survivors: two splitmix64 draws from `seed` a picture,
// read as one 128-bit number, taken modulo the survivors.
std::vector<std::vector<uint32_t>> draw_pictures(const Ranker& stack, uint32_t n, uint64_t seed);

std::string pictures_text(const std::vector<std::vector<uint32_t>>& pictures);
std::vector<std::vector<uint32_t>> parse_pictures(const std::string& text, const diffusion::Setup& s);

struct DiffusionOutcome
{
    std::string text;         // result.txt
    AiSpace::Digits digits;   // the model it became
    bool cancelled = false;
    uint64_t stopped_at = 0;
};
// Trains the run. `progress(step, steps, score since the last call)` after each step; `cancel`
// stops it (nothing is written: a diffusion run starts again from the beginning).
DiffusionOutcome train_diffusion_run(const DiffusionRun& run, unsigned threads,
                                     const std::function<void(uint64_t, uint64_t, const training::Score&)>& progress = {},
                                     const std::atomic<bool>* cancel = nullptr);

// The final model in a result.txt, or nothing if it has none.
std::optional<std::string> final_address_of(const std::string& result_text);

// The image line a run's pictures are units of (its width, height and palette), positional
// addresses written as `sieve diffuse` writes them.
Space image_line_of(const DiffusionRecipe& r);

// The k-th picture a model makes (k = 1, 2, ...): from the k-th noise drawn from splitmix64 seeded
// 1 (each cell next() mod B, cell after cell, picture after picture), with seed k. So the harness's
// pictures, `sieve diffuse --make --count N` and the chat window's figures are the same ones.
std::vector<uint32_t> nth_picture(const AiSpace& space, const AiSpace::Digits& digits, const diffusion::Setup& s, uint64_t k);

// A trained run's final model, ready to make pictures with.
struct PictureModel
{
    std::filesystem::path path;
    std::string name;
    DiffusionRecipe recipe;
    std::shared_ptr<const AiSpace> space;
    AiSpace::Digits digits;
    std::vector<uint32_t> picture(uint64_t k) const { return nth_picture(*space, digits, recipe.setup, k); }
};
// Reads a run's recipe and final model (not its pictures). Throws if it is not a trained run.
PictureModel load_picture_model(const std::filesystem::path& dir);
// The trained diffusion runs in these folders, by name; a name in an earlier folder hides the same
// name in a later one (as the training runs' loader looks).
std::vector<std::filesystem::path> picture_models_in(const std::vector<std::filesystem::path>& dirs);

} // namespace sieve::cli
