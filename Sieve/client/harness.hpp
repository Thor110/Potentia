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

class Harness
{
public:
    // `shape`: the AI line's, as the setup menu has it, which a new run starts from.
    Harness(SDL_Window* window, SDL_Renderer* renderer, std::filesystem::path runs_dir, sieve::AiShape shape = {});
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
    };
    enum class Action { Train, GoTo, Pack, Record, MoveUp, MoveDown, Reading, Start, Cancel };
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
    bool training() const { return job_.valid(); }
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
    bool lr_suggested_ = true;
    // A trial runs on a thread of its own, never waited for (a large shape's step can take a minute),
    // and leaves its answer here.
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
};

} // namespace hallway
