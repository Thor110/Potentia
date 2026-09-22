// Sieve hallway — the setup menu shown before you enter.
//
// Every line's state space can be adjusted, with no limits beyond what a setting can store
// (2^32 - 1): the state spaces are meant to scale without end. A map shows the four lines side by
// side as bars, one copy each (no looping). Bar length is proportional to the line's size in bits
// (log2 of its number of units), because the sizes differ by factors far too large for a literal
// drawing. The longest line always spans the map's full height, so no bar can run off the screen
// however large the settings grow, and a bar never gets shorter than a minimum, so even a tiny
// line stays visible next to a huge one.
//
// A magnifying glass beside each line's title (or F on that line's settings) opens its filter
// list over the map: the display mode, every filter with a tickbox and description, and each
// ticked filter's parameters. The books line's list has three groups: the cover (a picture), the
// title (one page) and the pages (all of a book's pages read as one text), under one display
// mode. The choices are saved to sieve-filters.ini. Where the stack can
// count its survivors exactly, the map shows them as a filled bar inside the line's bar.
#pragma once

#include "cli/args.hpp"
#include "cli/filter_config.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <vector>

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
    uint32_t book_pages = 4; // books: a cover (image line), a title and this many pages (pages line)

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
    Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings, sieve::cli::FilterConfig filters, std::string filters_path);

    enum class Result { Enter, Quit, Back }; // Back: Esc, to the main menu
    Result run();                                  // interactive: until Enter or quit
    void press(SDL_Keycode key, SDL_Keymod mod);   // one key, as if typed (scripting)
    void render();
    const Settings& settings() const { return s_; }
    const sieve::cli::FilterConfig& filters() const { return cfg_; }
    void open_filters(int line) { overlay_ = line; orow_ = 0; oscroll_ = 0; }

private:
    void handle(const SDL_Event& e, bool& done, Result& result);
    std::array<LineSize, 5> line_sizes() const; // pages, image, audio, video, books
    bool too_large() const; // a line this machine cannot open
    void adjust(int dir, int step);
    int row_count() const;

    // The filter overlay.
    struct ORow
    {
        enum class Kind { Mode, Header, Filter, Param } kind;
        std::string filter, key;
        int part = -1; // books: 0 cover, 1 title, 2 pages
    };
    std::vector<ORow> overlay_rows() const;
    void overlay_key(SDL_Keycode key, bool shift);
    void overlay_change(int dir, bool big);
    void render_overlay(float W, float H);
    sieve::FilterLine filter_line_of(int line) const;
    sieve::FilterLine book_part_line(int part) const; // books: the line a part's filters see
    sieve::cli::LineFilters& filters_of(const ORow& row);
    const sieve::cli::LineFilters& filters_of(const ORow& row) const;
    sieve::cli::FilterMode& mode_of(int line);
    struct StackInfo
    {
        std::string key;      // settings it was computed for
        std::string status;   // one line for the overlay footer
        double survivor_bits = -1; // exact survivors (log2), or -1
    };
    const StackInfo& stack_info(int line);
    const StackInfo& book_stack_info();
    void save_filters();

    SDL_Window* window_;
    SDL_Renderer* r_;
    Settings s_;
    int row_ = 0;
    sieve::cli::FilterConfig cfg_;
    std::string cfg_path_;
    int overlay_ = -1; // line whose filters are open, or -1
    int orow_ = 0;
    int oscroll_ = 0;
    StackInfo info_[5];
    SDL_FRect magnifier_[5] = {};
    SDL_FRect box_ = {};
    std::vector<std::pair<SDL_FRect, int>> row_rects_; // overlay rows on screen
};

} // namespace hallway
