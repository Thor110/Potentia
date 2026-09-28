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

#include "app_settings.hpp"
#include "cli/args.hpp"
#include "cli/filter_config.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cstdint>
#include <filesystem>
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
    uint32_t notes = 16; // events per book (per voice, on notes2)
    // The audio line's note set: notes104 (fixed: C4-C6, e q h w, one voice), or notes2 with its
    // range, durations (codes of s e E q Q h H w) and voices (sieve/audio.hpp).
    std::string note_set = "notes104", note_low = "C3", note_high = "C6", note_durations = "seEqQhHw";
    uint32_t voices = 1;
    uint32_t video_w = 5, video_h = 5, frames = 8;
    std::string video_palette = "mono";
    uint32_t book_pages = 4; // books: a cover (image line), a title and this many pages (pages line)
    // models: V vertices and F triangles, each coordinate one of C steps across [-1, 1]
    uint32_t model_vertices = 8, model_faces = 12, model_coords = 16;
    // How big the picture on the front of a crate is drawn, pixels square (a power of two).
    uint32_t model_tile = 64;
    // Global, because the corridor is shared: how many units stand on one tile's two walls.
    // A power of two, so a power-of-two line fills whole tiles (sieve/corridor.hpp).
    uint32_t items_per_wall = 128;
    // Global too: how long every line's titles are, in the pages line's alphabet (SPECIFICATIONS
    // §11, "Titled lines"). The books line keeps a page as its title.
    uint32_t title_length = 32;
    uint32_t letters_px = 8;   // the letter size item displays are drawn for; smaller letters are dashes
    uint32_t closeup_px = 1024; // the widest close-up display (0: none)
    uint32_t binary_bytes = 32; // the binary line: every file up to this many bytes
    // Which line FIND MY LIMITS grows: "all", or one line's name, the others left as they are.
    std::string limits_focus = "all";

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

// What this machine can open (menu.cpp: machine_budget). A shape has to fit all three.
struct Budget
{
    double bits = 4.0e6;                     // the longest address that opens in kUnitMs, measured here
    double cache_bytes = 256.0 * 1048576.0;  // what the hallway's cache of units may take
    double ms_per_unit_at_limit = 50;        // the time `bits` was worked out for
    double ref_bits = 0, ref_ms = 0;         // the measurement it was worked out from
    // The binary line's conversions are hex, linear in the length, not the base conversion the
    // other lines need, so it has a measurement of its own and grows in proportion from it.
    double binary_ref_bits = 0, binary_ref_ms = 0;
};
Budget machine_budget();
// Roughly how long one unit with an address of this many bits takes to open here. For the
// budget's display only: it is an estimate from one measurement, never used to address anything.
double unit_ms(const Budget& b, double bits);

class Menu
{
public:
    // `app` holds the two world settings the setup menu edits (angle precision and the model
    // image cache) and the graphics memory it budgets against; it is saved to `app_path` after
    // each change, unless the path is empty.
    Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings, sieve::cli::FilterConfig filters, std::string filters_path,
         AppSettings* app = nullptr, std::filesystem::path app_path = {});

    enum class Result { Enter, Quit, Back }; // Back: Esc, to the main menu (or, in game, the hallway)
    // Opened with F1 from a hallway that is still there: Esc goes back to it, and the footer says so.
    void set_in_game(bool on) { in_game_ = on; }
    // Whether the hallway was entered past the budget (Enter twice): it is then run thin.
    bool went_in_thin() const { return went_in_thin_; }
    Result run();                                  // interactive: until Enter or quit
    void press(SDL_Keycode key, SDL_Keymod mod);   // one key, as if typed (scripting)
    void render();
    const Settings& settings() const { return s_; }
    const sieve::cli::FilterConfig& filters() const { return cfg_; }
    void open_filters(int line) { overlay_ = line; orow_ = 0; oscroll_ = 0; otab_ = 0; }

private:
    void handle(const SDL_Event& e, bool& done, Result& result);
    std::array<LineSize, 7> line_sizes() const; // pages, image, audio, video, books, models, binary
    bool too_large() const;
    int over_budget() const;   // which line is beyond this machine, or -1
    bool over_budget_line(int i, const Budget& b) const;
    double line_ms(int i, const Budget& b) const; // how long a unit of line i takes to open, estimated
    double line_cache_bytes(int i) const;  // what line i's cached units would take in memory
    double graphics_mb_needed() const;     // the display cache, the close-ups and the world, in megabytes
    int display_px_line(int i) const;      // what line i's displays are drawn at (display.hpp)
    double display_mb() const;             // one display on the line whose displays are largest
    double closeup_mb() const;             // the close-ups at their largest
    bool graphics_over() const;
    int model_cache_mb() const { return app_ ? app_->model_cache_mb : 64; }
    void save_app() const;
    float draw_budget(float y); // the budget's three bars; returns the y below them
    void find_limits();        // set every line to the largest shape this machine can open
    void reset_settings();     // every shape back to its default // a line this machine cannot open
    void adjust(int dir, int step);
    // 0-31 are the settings rows, in the order render() lists them and adjust() switches on;
    // then FIND MY LIMITS, RESET, and ENTER THE HALLWAY, which is the only row that opens it.
    // The GLOBAL rows come first; each line's rows are counted from kFirstLineRow, so a row added
    // to GLOBAL moves them all with one change here.
    static constexpr int kAngleRow = 4, kTitleRow = 5, kLettersRow = 6, kDisplaySizeRow = 7, kDisplayCacheRow = 8,
                         kCloseUpRow = 9, kFocusRow = 10;
    static constexpr int kFirstLineRow = 11;
    // The audio rows: its notes, then its note set and the notes2 set's range, durations and voices.
    static constexpr int kPagesRows = kFirstLineRow, kImageRows = kFirstLineRow + 4, kAudioRow = kFirstLineRow + 7,
                         kVideoRows = kFirstLineRow + 13, kBooksRow = kFirstLineRow + 17, kModelsRows = kFirstLineRow + 18;
    static constexpr int kBinaryRow = kModelsRows + 3;
    static constexpr int kLimitsRow = kBinaryRow + 1, kResetRow = kLimitsRow + 1, kEnterRow = kLimitsRow + 2;
    int row_count() const;

    // The filter overlay.
    struct ORow
    {
        enum class Kind { Mode, Header, Filter, Param, Tabs, Info } kind;
        std::string filter, key;
        int part = -1; // books: 0 cover, 1 title, 2 pages
        std::string text = {}; // Info: the line to show
    };
    std::vector<ORow> overlay_rows() const;

    // The alphabet picker (A): the built-in alphabets, then every Unicode block, which can be
    // stacked. Its own overlay, because it is a list of a hundred rather than a tree of filters.
    void open_alphabets();
    std::vector<std::string> alpha_rows() const; // "" separators, built-in ids, then block ids
    std::vector<std::string> alpha_stack() const; // the parts of the current spec
    void alpha_choose(int row);
    void alpha_key(SDL_Keycode key);
    void render_alphabets(float W, float H);
    bool alpha_open_ = false;
    int arow_ = 0, ascroll_ = 0;
    std::vector<std::pair<SDL_FRect, int>> arow_rects_;
    void overlay_key(SDL_Keycode key, bool shift);
    void overlay_change(int dir, bool big);
    // Z / C: every filter on this tab / on both tabs switched on, or off if all were on already.
    void toggle_all_filters(bool both_tabs);
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
    AppSettings* app_ = nullptr;
    std::filesystem::path app_path_;
    int otab_ = 0;     // the filters window's tab: 0 built-in filters, 1 custom filters (plugins)
    void add_filter_rows(std::vector<ORow>& rows, const sieve::FilterLine& line, const sieve::cli::LineFilters& lf, int part) const;
    int overlay_ = -1; // line whose filters are open (0-3 text/image/audio/video, 4 books, 5 models, 6 binary), or -1
    int orow_ = 0;
    int oscroll_ = 0;
    float wheel_ = 0; // wheel movement not yet whole notches
    StackInfo info_[7]; // one per line of the map: text, image, audio, video, books, models, binary
    // One glass per column of the map: binary, the six lines, binary again. Both binary columns
    // open the same filters (overlay 6), because it is one line met at both ends.
    SDL_FRect magnifier_[8] = {};
    static int overlay_of_column(int c) { return c == 0 || c == 7 ? 6 : c - 1; }
    SDL_FRect box_ = {};
    std::vector<std::pair<SDL_FRect, int>> row_rects_; // overlay rows on screen
    bool in_game_ = false;
    bool go_anyway_armed_ = false, went_in_thin_ = false;
};

} // namespace hallway
