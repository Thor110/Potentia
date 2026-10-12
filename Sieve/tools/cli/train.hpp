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
    bool cancelled = false; // stopped before the end (cancelled, or at stop_after); its state saved, if a state file was given
    uint64_t resumed_from = 0, stopped_at = 0;
};
// Told after every step: the step (from 1), how many there are, and the score since the last
// checkpoint (reset after each).
using TrainingCallback = std::function<void(uint64_t step, uint64_t steps, const training::Score& recent)>;
// With `state`: training carries on from that file if it holds this run's state (and refuses one
// of another run), saves its state there at every checkpoint and when it stops before the end, and
// deletes it once the run is done. The file is beside the run, never part of it. `stop_after`:
// stop (as if cancelled) after that many steps, for scripts and tests.
TrainingOutcome train_run(const TrainingRun& run, unsigned threads, const TrainingCallback& progress = {},
                          const std::atomic<bool>* cancel = nullptr, const std::filesystem::path& state = {}, uint64_t stop_after = 0);
// Where a run's state is kept when it stops: beside it, its name and ".resume".
std::filesystem::path state_file_of(const std::filesystem::path& run);
// The step a saved state carries on from, if `state` holds one of this run.
std::optional<uint64_t> saved_step(const TrainingRun& run, const std::filesystem::path& state);

// The first line where a result differs from the one expected ("line N: ..."), or nothing.
std::optional<std::string> result_difference(const std::string& expected, const std::string& got);

struct RecordOptions
{
    training::Recipe recipe;  // the shape, how it learns, and how it reads (window, epochs, seed, reading)
    bool write_order = false; // write order.txt out, rather than have order-v1 make it from the recipe
};
// A file of a corpus: where it is, its path in the run (corpus/...), and its size.
struct TrainingFile
{
    std::filesystem::path source;
    std::string path;
    uint64_t size = 0;
};
// Every file under a folder, in path order (the order a new run reads them in, unless it is
// changed: the harness's curriculum list). A corpus is a folder; a single file is refused.
std::vector<TrainingFile> corpus_files(const std::filesystem::path& corpus);
// A new run in `out` (made, or empty): the files copied into it, read in the order given, and
// training.ini (and, with write_order, order.txt). Returns the run, untrained.
TrainingRun record_training_run(const std::vector<TrainingFile>& files, const std::filesystem::path& out, const RecordOptions& o);
TrainingRun record_training_run(const std::filesystem::path& corpus, const std::filesystem::path& out, const RecordOptions& o);
// The learning rate to suggest for a shape and its windows: halved each time the width doubles
// (measured: 0.0005 at width 16, 0.0002 at 32, 0.0001 at 64, 0.00005 at 128, on the shipped text,
// layers mattering little), and scaled down as a step sums more predictions (batch times window
// less one, 256 at the defaults): 0.008 / width * 256 / predictions, rounded down to 1, 2 or 5 of a
// power of ten, and no lower than 0.00002 (training-v1's Q16 holds no smaller step than 1 / 65536).
std::string suggested_learning_rate(const training::Recipe& rc);
// The loss scale to suggest (training-v2's k): 6 for a width of 64 or more, or 4 layers or more,
// where the gradients of deeper layers fell below Q16 (measured: width 128 with 2 layers, 3.49
// bits a byte to 3.33; 4 layers of width 32, 3.24 to 3.14); 0 below, where scaling clipped
// gradients and cost (Micro: 3.21 to 3.29).
uint32_t suggested_loss_scale(const training::Recipe& rc);

// What a run of this recipe over these files will take, worked out before it starts.
struct TrainingEstimate
{
    uint64_t weights = 0, windows = 0, steps = 0;
    unsigned threads = 1;                       // as many as the batch can use
    uint64_t bytes_per_thread = 0, bytes = 0;   // memory: each thread's working set, and all of it
    uint64_t result_bytes = 0;                  // result.txt, about
    bool learning_rate_at_floor = false;        // the rate is within a few steps of 1 / 65536
};
TrainingEstimate estimate_training(const training::Recipe& rc, const std::vector<TrainingFile>& files, unsigned threads = 0);
// Seconds one step takes, measured: a step trained on the first windows of the files.
double measure_training_step(const training::Recipe& rc, const std::vector<TrainingFile>& files, unsigned threads = 0);

// A run folder as Sieve instructions (.sieve, packed as sieve-manifest-v4 when that is smaller).
void pack_training_run(const std::filesystem::path& folder, const std::filesystem::path& sieve_file);

// A run as the AI line's filters name it (language-v1, near-model-v1): its model's shape, its model
// (the final address; empty if untrained) and its text (its first file). `id` is a run in `dirs` (a
// folder of that name, or that name with .sieve) or a path to one.
ModelRun model_run_of(const std::string& id, const std::vector<std::filesystem::path>& dirs);
// set_model_run_loader with these folders, looked in in turn (the programs give the training folder
// beside them, then data/training, for running from the repository).
void use_training_runs(const std::vector<std::filesystem::path>& dirs);
// The folders given to use_training_runs (none before it is called).
const std::vector<std::filesystem::path>& training_run_dirs();

int cmd_ai_train(const Args& a);
// sieve diffuse: train, check and use a diffusion-v1 model of pictures (tools/cli/diffuse.cpp).
int cmd_diffuse(const Args& a);

} // namespace sieve::cli
