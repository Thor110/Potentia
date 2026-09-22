// Sieve hallway — the setup menu shown before you enter.
//
// Every line's state space can be adjusted, with no limits beyond what a setting can store
// (2^32 - 1): the state spaces are meant to scale without end. A map shows the four lines side by
// side as bars, one copy each (no looping). Bar length is proportional to the line's size in bits
// (log2 of its number of units), because the sizes differ by factors far too large for a literal
// drawing. The longest line always spans the map's full height, so no bar can run off the screen
// however large the settings grow, and a bar never gets shorter than a minimum, so even a tiny
// line stays visible next to a huge one.
#pragma once

#include "cli/args.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string>

namespace hallway {

struct Settings
{
    std::string start_line = "text";
    std::string mode = "positional"; // positional | scrambled | guided
    std::string key = "sieve";
    uint32_t length = 32;
    std::string alphabet = "lower27";
    std::string canon = "v2";
    bool model = true; // use the alphabet's default model (guided ordering)
    uint32_t image_w = 10, image_h = 10;
    std::string image_palette = "mono";
    uint32_t notes = 16;
    uint32_t video_w = 5, video_h = 5, frames = 8;
    std::string video_palette = "mono";

    static Settings from_args(const sieve::cli::Args& a);
    // Writes these settings into `a` (as the hallway's own options), keeping everything else.
    void apply(sieve::cli::Args& a) const;
};

// A line's size, for the map: bits (log2 of units) and how it fits the corridor's tiles.
struct LineSize
{
    double bits = 0;
    uint32_t padding = 0; // empty slots in the last tile of each copy
    std::string units;    // e.g. "27^32 = ~10^45.8 units"
};
LineSize line_size(uint32_t base, uint64_t length);

class Menu
{
public:
    Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings);

    enum class Result { Enter, Quit };
    Result run();                                  // interactive: until Enter or quit
    void press(SDL_Keycode key, SDL_Keymod mod);   // one key, as if typed (scripting)
    void render();
    const Settings& settings() const { return s_; }

private:
    void handle(const SDL_Event& e, bool& done, Result& result);
    std::array<LineSize, 4> line_sizes() const;
    bool too_large() const; // a line this machine cannot open
    void adjust(int dir, int step);
    int row_count() const;

    SDL_Window* window_;
    SDL_Renderer* r_;
    Settings s_;
    int row_ = 0;
};

} // namespace hallway
