// Sieve hallway — the Fractal Dimension (IDEAS 19.191, 19.235; list S, step 1): its shelves, the fractals
// charted by hand (fractals/chart.txt beside the program), listed on the left; on the right the one
// chosen, drawn at a depth you choose, with its ruleset beneath. Opened from the setup page's FRACTALS
// column (beside AI), and by `--fractals ID` for pictures and tests.
//
// Keys (Settings > Controls > Keys, "The menus" and "The Fractal Dimension"): Up and Down choose a
// fractal; Right and Left (or = and -) grow it a step deeper or take one back; Home and End its first
// and deepest; Esc goes back.
#pragma once

#include "sieve/fractal.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

class FractalLibrary
{
public:
    // The shelves in `folder` (chart.txt and its rulesets), opened on `id` where that names one.
    FractalLibrary(SDL_Window* window, SDL_Renderer* renderer, const std::filesystem::path& folder, const std::string& id = {});
    ~FractalLibrary();
    enum class Result { Back, Quit };
    Result run();
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void render();
    // For scripted runs: the fractal shown, its depth, and how many are charted.
    std::string summary() const;
    const std::string& status() const { return status_; }

    // The shelves' folder: fractals beside the program (or data/fractals in the repository).
    static std::filesystem::path folder();

private:
    void key(SDL_Keycode k, SDL_Keymod mod);
    void handle(const SDL_Event& e);
    void choose(int i);

    SDL_Window* window_;
    SDL_Renderer* r_;
    bool done_ = false;
    Result result_ = Result::Back;
    std::vector<sieve::ChartedFractal> chart_;
    int chosen_ = 0;
    std::vector<uint32_t> depth_; // each fractal's depth, as you left it
    std::string status_;
    // The drawing shown: of which fractal, at which depth and size.
    SDL_Texture* tex_ = nullptr;
    int tex_for_ = -1;
    uint32_t tex_depth_ = 0, tex_size_ = 0;
    std::vector<std::pair<SDL_FRect, int>> row_boxes_;
};

} // namespace hallway
