// Sieve hallway — the AI Training Harness, opened from the main menu (SPECIFICATIONS §12.0e): the
// training runs in the `training` folder beside the programs, each a folder or Sieve instructions
// (.sieve) holding a model's recipe, the exact order of what it learns from, that material, and once
// trained what it became. Train one, and if it was trained before, see whether it came out the same,
// bit for bit; record a new run from a file or folder; pack a run as a .sieve; and go to the model on
// the AI line to talk to it.
//
// Keyboard: Up/Down choose a run, Enter trains it (or, on the last row, records a new one), G goes
// to its model, P packs a run folder as a .sieve, Esc stops a training or goes back. Recording a
// run: a path typed, then its files listed in the order they will be read, Shift+Up/Down moving
// the one chosen, M changing how they are read, Left/Right the times through, Enter to record and
// train. Mouse: click a run, a file or a button. Training runs on a worker, so the window stays
// alive and can stop it.
#pragma once

#include "cli/train.hpp"
#include "sieve/aispace.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <chrono>
#include <filesystem>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace hallway {

class Harness
{
public:
    Harness(SDL_Window* window, SDL_Renderer* renderer, std::filesystem::path runs_dir);
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
    std::vector<std::pair<SDL_FRect, int>> row_rects_, cur_rects_;
    std::vector<std::pair<SDL_FRect, Action>> button_rects_;
    bool done_ = false;
    Result result_ = Result::Back;
};

} // namespace hallway
