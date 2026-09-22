// Sieve hallway — the setup menu shown before you enter.
//
// Every line's state space can be adjusted, and a map shows the four lines side by side as bars,
// one copy each (no looping). Bar length is proportional to the line's size in bits (log2 of its
// number of units), because the sizes differ by factors far too large for a literal drawing.
// By default the scale is fixed to the largest state space the menu's limits allow, so no bar
// ever runs off the screen and every bar keeps the same proportion whatever you change; a bar
// never gets shorter than a minimum, so even a tiny line stays visible. S switches to a scale
// that fits the current settings instead.
#pragma once

#include "cli/args.hpp"

#include <SDL3/SDL.h>

#include <cstdint>
#include <string>

namespace hallway {

// The menu's limits: they define the largest state space, and so the fixed scale of the map.
struct Limits
{
    static constexpr uint32_t kTextLength = 3200; // one Babel page
    static constexpr uint32_t kImageSide = 64;
    static constexpr uint32_t kNotes = 1024;
    static constexpr uint32_t kVideoSide = 32;
    static constexpr uint32_t kVideoFrames = 64;
};

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
    void adjust(int dir, int step);
    int row_count() const;

    SDL_Window* window_;
    SDL_Renderer* r_;
    Settings s_;
    int row_ = 0;
    bool fit_ = false; // false: scale fixed to the limits' maximum; true: fit the current settings
};

} // namespace hallway
