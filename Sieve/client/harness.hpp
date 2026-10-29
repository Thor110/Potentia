// Sieve hallway — the AI Training Harness, opened from the main menu (SPECIFICATIONS §12.0e): the
// training runs in the `training` folder beside the programs, each a folder or Sieve instructions
// (.sieve) holding a model's recipe, the exact order of what it learns from, that material, and once
// trained what it became. Train one, and if it was trained before, see whether it came out the same,
// bit for bit; record a new run from a file or folder; pack a run as a .sieve; and go to the model on
// the AI line to talk to it.
//
// Keyboard: Up/Down choose a run, Enter trains it (or, on the last row, records a new one), G goes
// to its model, P packs a run folder as a .sieve, Esc stops a training or goes back. Recording a
// run: a folder chosen (the system's folder dialog), then its settings (the model's shape, how it
// learns, how it reads), each with what it will cost (weights, steps, memory, time measured on a
// step, the result's size), and its files listed in the order they will be read: Tab between the
// two, Up/Down choose, Left/Right change a setting, Shift+Up/Down move a file, M how they are read,
// Enter to record and train. Mouse: click a run, a setting, a file or a button. Training runs on a
// worker, so the window stays alive and can stop it.
#pragma once

#include "cli/diffusion_run.hpp"
#include "dialogs.hpp"
#include "cli/train.hpp"
#include "sieve/aispace.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

// Where a new picture model (diffusion-v1) draws its training pictures from: the image line's shape
// and palette, as the setup menu has them, and the settings file whose image stack keeps them.
struct PictureSource
{
    uint32_t width = 10, height = 10;
    std::string palette = "mono";
    std::filesystem::path filters;
};

class Harness
{
public:
    // `shape`: the AI line's, as the setup menu has it, which a new run starts from; `pictures`:
    // where a new picture model's pictures come from.
    Harness(SDL_Window* window, SDL_Renderer* renderer, std::filesystem::path runs_dir, sieve::AiShape shape = {},
            PictureSource pictures = {});
    ~Harness();
    enum class Result { Back, Quit, GoTo };
    Result run();
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void type(const std::string& utf8);          // scripting: text typed (the corpus prompt)
    void settle();                               // scripting: wait for a training to finish
    void render();
    const std::string& verdict() const { return verdict_; }
    const std::string& status() const { return status_; }
    bool going() const { return done_ && result_ == Result::GoTo; } // GO TO IT was chosen
    void set_scripted(bool s) { scripted_ = s; } // no folder dialog: the folder's path is typed
    void picked(const std::string& folder);      // the folder dialog's choice (from any thread)
    // With Result::GoTo: the model to go to, and the shape of the line it is on.
    sieve::AiShape go_shape;
    sieve::AiSpace::Digits go_digits;

private:
    struct Entry
    {
        std::filesystem::path path;
        std::string name;
        bool folder = false;
        bool pictures = false; // a picture model (diffusion-v1), not a text model
    };
    enum class Action { Train, GoTo, Pack, Record, RecordPictures, MoveUp, MoveDown, Reading, Start, Cancel, StartOver };
    void say(std::string text, bool bad = false); // the status line
    void open_curriculum(const std::string& folder);
    void change_setting(int row, int dir, bool big); // a new run's setting, a step either way
    void start_trial();                              // a step measured, for the time estimate
    void scan();
    void select(int row);
    void act(Action a);
    void start_training();
    void poll();
    void handle(const SDL_Event& e);
    void key(SDL_Keycode k, SDL_Keymod mod);
    bool training() const { return job_.valid() || djob_.valid(); }
    // Picture models (diffusion-v1): the settings page for a new one, training one, and what it makes.
    void open_pictures();
    void change_picture_setting(int row, int dir, bool big);
    void start_picture_training();
    void poll_pictures();
    void make_pictures(const sieve::AiSpace::Digits& digits);
    void render_pictures(float x, float& ry, float colW, float H);
    void draw_picture(const std::vector<uint32_t>& p, float x, float y, float cell) const;
    std::vector<std::pair<Action, bool>> buttons() const; // each, and whether it can be pressed now

    SDL_Window* window_;
    SDL_Renderer* r_;
    std::filesystem::path dir_;
    std::vector<Entry> entries_;
    int row_ = 0; // entries_.size() is "record a new run"
    std::optional<sieve::cli::TrainingRun> run_;
    std::string load_error_;
    // The training on the worker, and what it has told.
    std::future<sieve::cli::TrainingOutcome> job_;
    std::atomic<bool> cancel_{false};
    std::mutex mu_;
    uint64_t step_ = 0, steps_ = 0;
    std::vector<float> curve_; // bits a byte at each checkpoint so far
    std::chrono::steady_clock::time_point started_;
    double took_ = 0;
    std::optional<sieve::cli::TrainingOutcome> outcome_;
    // A run stopped before its end keeps its state beside it (RUN.resume): the step it stopped at,
    // which Enter carries on from (S starts it over).
    std::optional<uint64_t> saved_step_;
    std::filesystem::path state_path() const;
    std::string verdict_, says_, status_;
    bool status_bad_ = false;
    SDL_Color verdict_colour_{255, 255, 255, 255};
    // The corpus prompt (record a new run).
    bool prompting_ = false;
    std::string prompt_;
    // A new run's curriculum: its files in the order they will be read, moved up and down, and how
    // they are read; Start records the run and trains it.
    std::optional<std::vector<sieve::cli::TrainingFile>> curriculum_;
    int cur_row_ = 0;
    sieve::training::Recipe recipe_;
    std::string new_name_;
    sieve::AiShape line_shape_;
    bool scripted_ = false;
    std::optional<std::string> picked_; // the folder dialog's choice, under mu_
    // The new run's settings: the rows, which has the keys (the settings or the files), whether the
    // learning rate is still the one suggested, and a step measured for the shape and windows.
    bool focus_settings_ = false;
    int set_row_ = 0;
    bool lr_suggested_ = true, scale_suggested_ = true;
    // A trial runs on a thread of its own, never waited for (a large shape's step can take a minute),
    // and leaves its answer here. It holds its own copies of the recipe and files, and what it reads
    // of the training code's shared state (rope_for's cache) is never destroyed, so the program may
    // exit while it runs.
    struct Trial
    {
        std::mutex m;
        bool done = false;
        double secs = -1;
    };
    std::shared_ptr<Trial> trial_;
    std::string trial_for_, measured_for_;
    double step_secs_ = -1;
    std::vector<std::pair<SDL_FRect, int>> set_rects_;
    std::vector<std::pair<SDL_FRect, int>> row_rects_, cur_rects_;
    std::vector<std::pair<SDL_FRect, Action>> button_rects_;
    bool done_ = false;
    Result result_ = Result::Back;

    PictureSource pic_source_;
    std::optional<sieve::cli::DiffusionRun> drun_;            // the picture run chosen
    std::future<sieve::cli::DiffusionOutcome> djob_;          // training it
    std::optional<sieve::cli::DiffusionOutcome> doutcome_;
    std::vector<std::vector<uint32_t>> made_;                 // pictures its model made from noise
    // A new picture model's settings, and the stack its pictures are drawn from (worked out once,
    // when the page opens): its id and survivors, or why it cannot give pictures.
    std::optional<sieve::cli::DiffusionRecipe> pic_recipe_;
    uint32_t pic_count_ = 4000;
    std::string pic_stack_id_, pic_stack_note_;
    std::shared_ptr<const sieve::FilterStack> pic_stack_;
    int pic_row_ = 0;    // Last, so it is the first let go: its dialogs' answers after it has gone are dropped (dialogs.hpp).
    hallway::DialogOwner dialog_owner_{this};
};

} // namespace hallway
