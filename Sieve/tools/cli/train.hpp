// Sieve CLI — training runs (SPECIFICATIONS §12.0e): a folder, or a .sieve made of one, holding a
// model's recipe (training.ini), the exact order of what it learnt from (order.txt), that material,
// and once trained what it became (result.txt). `sieve ai-train` records one and trains it again; the
// hallway's AI Training Harness does the same from the main menu.
#pragma once

#include "cli/args.hpp"
#include "sieve/aispace.hpp"
#include "sieve/training.hpp"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli {

struct TrainingRun
{
    std::string name;                                   // the folder's name, or the .sieve's root
    training::Recipe recipe;
    std::vector<training::Window> order;
    std::map<std::string, std::vector<uint8_t>> files;  // every file of the run, by its path in it
    std::optional<std::string> result;                  // result.txt, if it has been trained
    uint64_t steps() const { return (order.size() + recipe.batch - 1) / recipe.batch; }
};

// A run from its folder or from a .sieve (or installer program) holding one. Throws if it is not a
// run, or its order names bytes it does not hold.
TrainingRun load_training_run(const std::filesystem::path& folder_or_sieve);

struct TrainingOutcome
{
    training::Result result;
    std::string text;       // result.txt
    AiSpace::Digits digits; // the final model's
    bool cancelled = false;
};
// Told after every step: the step (from 1), how many there are, and the score since the last
// checkpoint (reset after each).
using TrainingCallback = std::function<void(uint64_t step, uint64_t steps, const training::Score& recent)>;
TrainingOutcome train_run(const TrainingRun& run, unsigned threads, const TrainingCallback& progress = {},
                          const std::atomic<bool>* cancel = nullptr);

// The first line where a result differs from the one expected ("line N: ..."), or nothing.
std::optional<std::string> result_difference(const std::string& expected, const std::string& got);

struct RecordOptions
{
    training::Recipe recipe;
    uint32_t window = 65, epochs = 30;
    uint64_t seed = 1;
};
// A new run in `out` (made, or empty): the corpus (a file, or every file under a folder) copied
// into corpus/, training.ini and order.txt. Returns the run, untrained.
TrainingRun record_training_run(const std::filesystem::path& corpus, const std::filesystem::path& out, const RecordOptions& o);
// A run folder as Sieve instructions (.sieve, packed as sieve-manifest-v4 when that is smaller).
void pack_training_run(const std::filesystem::path& folder, const std::filesystem::path& sieve_file);

int cmd_ai_train(const Args& a);

} // namespace sieve::cli
