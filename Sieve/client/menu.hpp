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
#include "dimensions.hpp"
#include "display.hpp"
#include "cli/args.hpp"
#include "cli/filter_config.hpp"
#include "cli/lines.hpp" // kDefaultExportFps

#include <SDL3/SDL.h>

#include <array>
#include <memory>
#include <functional>
#include <limits>
#include <map>
#include <optional>
#include <atomic>
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
    // The notes3 set (sieve/notes3.hpp): open-ended notes. Levels, tempo and instruments are set
    // in the settings file only.
    std::string n3_low = "C-1", n3_high = "G9", n3_instruments = "0";
    uint32_t n3_tpq = 4, n3_longest = 16, n3_levels = 8, n3_voices = 1, n3_tempo = 120;
    // The pcm set (sieve/sound.hpp): sound itself, `samples` a channel per book.
    uint32_t samples = 8000, pcm_rate = 8000, pcm_bits = 8, pcm_channels = 1;
    uint32_t video_w = 5, video_h = 5, frames = 8;
    std::string video_palette = "mono";
    // The ffmpeg that reads picture and video formats stb_image does not (cli/media_decode.hpp);
    // empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH.
    std::string ffmpeg;
    uint32_t video_fps = sieve::cli::kDefaultExportFps; // a video saved as a video through ffmpeg: frames a second
    uint32_t book_pages = 4; // books: a cover (image line), a title and this many pages (pages line)
    // Tracks and movies (sieve/composition.hpp): a cover, a title and this many units of audio, of video.
    uint32_t track_units = 4, movie_units = 4;
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
    uint32_t closeup_px = kCloseUpScreen; // the widest close-up display (0: none; display.hpp)
    uint32_t binary_bytes = 32; // the binary line: every file up to this many bytes
    // Which line FIND MY LIMITS grows: "all", or one line's name, the others left as they are.
    std::string limits_focus = "all";

    // The settings the hallway's options give (--line, --length, ...), the defaults for the rest.
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
// For a line of `length` symbols of `base` each.
LineSize line_size(uint32_t base, uint64_t length);

// The view distance (Settings > Graphics): the tiles drawn and kept in the item cache either side
// of the one you are in, as many behind as ahead, and the rooms either side of yours whose items
// have pictures of their own. 7 and 1 until set.
void set_view(int view_rooms, int picture_rooms);
int view_rooms();
int tiles_kept(); // view_rooms() either side, and yours
int picture_rooms();
// The most items the hallway keeps worked out (its cache clears itself past this many): the tiles
// kept (tiles_kept) and one more either side, at the items a tile holds now (sieve::books_per_tile). What the
// budget's memory bar counts on (as the setting stands), and the cache itself (hallway.cpp book()).
size_t cached_units();
// The longest address the menu lets a shape have: one that alone would fill the memory the item
// cache is given (Budget::cache_bytes, ITEM MEMORY's share of installed memory), held as a number and as hex
// (three eighths of a byte a bit). Past it a line is "too large" and nothing of it is worked out.
double too_large_bits();
// A file long enough that opening it is noticed: one whose bytes take a quarter of the time budget
// to work out from its place on the binary line, measured here (machine_budget). Past it the
// hallway checks your room's files with the vault ahead of a look (vault_ahead), and a file taken
// there by its own length keeps only the room you stand in (thin).
uint64_t large_file_bytes();

// What this machine can open (menu.cpp: machine_budget). A shape has to fit all three.
struct Budget
{
    double bits = 4.0e6;                     // the longest address that opens in the time budget, measured here
    double cache_bytes = 256.0 * 1048576.0;  // what the hallway's cache of units may take
    double ms_per_unit_at_limit = 50;        // the time `bits` was worked out for (the time budget)
    double growth = 1.6;                     // how the time to open grows with the address, measured
    double ref_bits = 0, ref_ms = 0;         // the measurement it was worked out from
    // The binary line's conversions are hex, linear in the length, not the base conversion the
    // other lines need, so it has a measurement of its own and grows in proportion from it.
    double binary_ref_bits = 0, binary_ref_ms = 0;
};
// Measured once, the first time it is asked for (about 100 ms), and the same after.
Budget machine_budget();
// Roughly how long one unit with an address of this many bits takes to open here. For the
// budget's display only: it is an estimate from one measurement, never used to address anything.
double unit_ms(const Budget& b, double bits);

// The menu counts each line's stack on worker threads, which outlive the menu: building the
// hallway, and leaving the program, wait for them first (they share its caches).
void finish_filter_warmup();
// Whether a counting worker is still running (at exit: see app_main.cpp).
bool filter_workers_busy();
// The share of installed memory (percent, 5 to 100) the counting workers may take at once, each
// count up to the whole filter memory (the setup menu's COUNTING MEMORY; 50 until it is set), and
// how many counts that lets run at once on this machine (at most one per core but the one that
// draws), with a filter memory of `filter_bytes` (0: the one in use, sieve::filter_memory).
void set_counting_share(int percent);
int counting_slots(double filter_bytes = 0);
// The time budget as set (the setup menu's TIME BUDGET, in ms): what the budget's time bar and
// FIND MY LIMITS hold a line to. (What can rank follows sieve::unit_time_ms, which X applies.)
void set_time_budget(double ms);
// The share of installed memory (percent, 5 to 90) the hallway's cache of the items around you may
// take (the setup menu's ITEM MEMORY; 25 until it is set): the budget's memory bar and how long an
// address may be (too_large_bits) follow it.
void set_item_memory_share(int percent);

// Which tab of the filters window lists a filter: 0 built-in, 1 custom (plugins), 2 retired.
int tab_of(const sieve::FilterSpec& f);

// How many rows each line has in the setup menu, and where they start: in door order after the
// GLOBAL rows (nullopt: where the lines' rows end, FIND MY LIMITS).
inline constexpr int kSetupFirstLineRow = 16;
constexpr int setup_rows_of(Media m)
{
    switch (m)
    {
    case Media::Pages: return 4;  // length, alphabet, warp rules, model
    case Media::Image: return 3;  // width, height, palette
    case Media::Audio: return 6;  // notes (samples), the set, and its four rows
    case Media::Video: return 4;  // width, height, frames, palette
    case Media::Books: return 1;  // pages per book
    case Media::Tracks: return 1; // units per track
    case Media::Movies: return 1; // units per movie
    case Media::Models: return 3; // vertices, triangles, grid
    case Media::Binary: return 1; // length
    }
    return 0;
}
constexpr int setup_row_of(std::optional<Media> m)
{
    int r = kSetupFirstLineRow;
    for (const Dimension& d : kDimensions)
    {
        if (m && d.media == *m) return r;
        r += setup_rows_of(d.media);
    }
    return r;
}

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
    // One event (a key, the mouse, the window); sets `done` and `result` when the menu is left.
    void handle(const SDL_Event& e, bool& done, Result& result);
    std::array<LineSize, kLines> line_sizes() const; // by door (dimensions.hpp)
    bool too_large() const;
    int over_budget() const;   // which line is beyond this machine, or -1
    bool over_budget_line(int i, const Budget& b) const;
    double line_ms(int i, const Budget& b) const; // how long a unit of line i takes to open, estimated
    double line_cache_bytes(int i) const;  // what line i's cached units would take in memory
    double graphics_mb_needed() const;     // the display cache, the close-ups and the world, in megabytes
    double world_graphics_mb() const;      // the world's part of that, from what it makes (menu.cpp)
    // What line i's displays are drawn at (display.hpp), and one display on the line whose
    // displays are largest: held to what the display cache can hold (widest_px), or, with
    // `cache_held` false, as wide as the letters ask, which is what the cache would need.
    int display_px_line(int i, bool cache_held = true) const;
    double display_mb(bool cache_held = true) const;
    int widest_px() const;                 // the widest the display cache lets displays be drawn
    int closeup_px() const;                // the widest a close-up is drawn (display.hpp closeup_width)
    double closeup_one_mb() const;         // one close-up at that width (0 with them off)
    size_t closeup_count() const;          // how many the graphics memory left over holds
    double closeup_mb() const;             // what they take at most
    bool graphics_over() const;
    int model_cache_mb() const { return app_ ? app_->model_cache_mb : 64; }
    void save_app() const;
    // The budget's four bars and status line, at the menu's top right, kBudgetW wide from x; returns the y below them.
    static constexpr float kBudgetW = 600;
    float draw_budget(float x, float y);
    void find_limits();        // set every line to the largest shape this machine can open
    void reset_settings();     // every shape back to its default // a line this machine cannot open
    void adjust(int dir, int step);
    // The settings rows, in the order render() lists them and adjust() switches on; then FIND MY
    // LIMITS, RESET, and ENTER THE HALLWAY, which is the only row that opens it. The GLOBAL rows
    // come first; then each line's (setup_rows_of) in door order, from kFirstLineRow (setup_row_of),
    // so reordering the doors reorders them, and a row added to GLOBAL moves them all with one
    // change here.
    static constexpr int kAngleRow = 4, kTitleRow = 5, kLettersRow = 6, kDisplaySizeRow = 7, kDisplayCacheRow = 8,
                         kCloseUpRow = 9, kFocusRow = 10, kFilterMemoryRow = 11, kCountingMemoryRow = 12, kMergeCacheRow = 13,
                         kTimeBudgetRow = 14, kItemMemoryRow = 15;
    static constexpr int kFirstLineRow = kSetupFirstLineRow;
    // The audio rows: its notes, then its note set and the notes2 set's range, durations and voices.
    static constexpr int kPagesRows = setup_row_of(Media::Pages), kImageRows = setup_row_of(Media::Image),
                         kAudioRow = setup_row_of(Media::Audio), kVideoRows = setup_row_of(Media::Video),
                         kBooksRow = setup_row_of(Media::Books), kTracksRow = setup_row_of(Media::Tracks),
                         kMoviesRow = setup_row_of(Media::Movies), kModelsRows = setup_row_of(Media::Models),
                         kBinaryRow = setup_row_of(Media::Binary);
    static constexpr int kLimitsRow = setup_row_of(std::nullopt), kResetRow = kLimitsRow + 1, kEnterRow = kLimitsRow + 2;
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
    // Z: this tab of this line; C: both tabs of this line; X: both tabs of every line.
    enum class ToggleScope { ThisTab, BothTabs, EveryLine };
    void toggle_all_filters(ToggleScope scope);
    bool memory_pending() const; // the filter memory setting is not yet the limit the tallies use
    bool time_pending() const;   // nor the time budget (X applies both)
    double filter_memory_setting() const; // the setting, in bytes
    // The lines whose tally is still being counted (on the workers), by name; "" when none is.
    std::string counting_lines() const;
    // Still calculating the dimensions: a tally being counted, or X weighing the filters that
    // clash. ENTER THE HALLWAY waits for it, greyed, and the menu says so at its top.
    bool calculating() const { return toggling_ || !counting_lines().empty(); }
    // A filter a toggle reaches: its stack, line and part, and, where it clashes with another in
    // reach, its share kept alone (log10) once weighed.
    struct Reach
    {
        sieve::cli::LineFilters* lf;
        int li, part;
        std::string name;
        bool clashes = false;
        double kept = std::numeric_limits<double>::quiet_NaN();
    };
    std::vector<Reach> reach_of(ToggleScope scope, int overlay, int tab);
    // Ticking all waits on the weighing (poll_toggle, each frame): the toggle asked for, and where.
    struct Toggling
    {
        ToggleScope scope;
        int overlay, tab;
    };
    std::optional<Toggling> toggling_;
    void poll_toggle();

    void render_overlay(float W, float H);
    sieve::FilterLine filter_line_of(int line) const;
    sieve::FilterLine book_part_line(int part) const; // books: the line a part's filters see
    // Lines whose items have parts, each with its own filters (cover, title, and pages or units):
    // books, tracks and movies. part_line: the line a part's filters see.
    static bool has_parts(int line) { return line == kBooksLine || is_composition(line); }
    // How many parts it has (books: cover, title, pages; tracks and movies: cover, title, units,
    // joined), and whether a part is there to filter (a composition's title is not when titles are off).
    static int parts_of(int line) { return line == kBooksLine ? 3 : sieve::cli::CompositionFilters::kParts; }
    bool has_part(int line, int part) const { return !(part == 1 && is_composition(line) && s_.title_length == 0); }
    // A composition's joined part. Z, C and X leave it alone: with the units' filters ticked too,
    // nothing would count, so it is ticked by hand.
    static constexpr int kJoinedPart = 3;
    sieve::FilterLine part_line(int line, int part) const;
    sieve::cli::CompositionFilters& comp_cfg(int line) { return kDimensions[line].media == Media::Tracks ? cfg_.tracks : cfg_.movies; }
    const sieve::cli::CompositionFilters& comp_cfg(int line) const { return kDimensions[line].media == Media::Tracks ? cfg_.tracks : cfg_.movies; }
    uint32_t comp_units(int line) const { return kDimensions[line].media == Media::Tracks ? s_.track_units : s_.movie_units; }
    sieve::cli::LineFilters& filters_of(const ORow& row);
    const sieve::cli::LineFilters& filters_of(const ORow& row) const;
    sieve::cli::FilterMode& mode_of(int line);
    struct StackInfo
    {
        std::string key;      // settings it was computed for
        std::string status;   // one line for the overlay footer
        double survivor_bits = -1; // exact survivors (log2), or -1
        // The tally beside the overlay's title: how much of the line the ticked filters remove,
        // as a percentage ("99.999999999999...% (kept 10^-19.27)"), or empty when it cannot be counted.
        std::string filtered;
        // The same as a number, for comparing stacks: log10 of the share kept (0 nothing removed,
        // -infinity everything), or NaN when it cannot be counted.
        double kept_log10 = std::numeric_limits<double>::quiet_NaN();
        // The memory the count's tables need (or would): what the filter memory must hold for it.
        double table_bytes = 0;
    };
    static void survivors_of(StackInfo& out, const sieve::BigUint& n, const sieve::BigUint& total);
    static void none_of(StackInfo& out);
    const StackInfo& stack_info(int line);
    const StackInfo& parts_stack_info(int line); // books, tracks, movies: every part counted, the whole exact
    const StackInfo& book_stack_info() { return parts_stack_info(kBooksLine); }
    // The key and the work for a stack's tally (menu.cpp); `part` is the books' part for line 4.
    std::pair<std::string, std::function<StackInfo()>> stack_job(int line, const sieve::cli::LineFilters& lf, int part = -1) const;
    // One filter alone (and what ticking it ticks too): its tally, or null while it is counted on a
    // worker (filter_share).
    static sieve::cli::LineFilters alone(const sieve::cli::LineFilters& lf, const std::string& filter);
    // Stack info is worked out on worker threads (menu.cpp): the result when it is ready, else a
    // "counting" placeholder, asked again each frame.
    struct Job
    {
        std::atomic<bool> ready{false};
        std::atomic<bool> wanted{true}; // false once its settings are gone: skipped if not yet begun
        StackInfo result;
    };
    struct Pending
    {
        std::string key;
        std::shared_ptr<Job> job;
    };
    std::array<Pending, kLines> pending_;
    std::array<StackInfo, kLines> waiting_;
    const StackInfo& resolve(int i, const std::string& key, std::function<StackInfo()> work);
    // Queues `work` for a counting worker; its result (or its error, said as a status) lands in `job`.
    static void start_job(std::shared_ptr<Job> job, std::function<StackInfo()> work);
    std::map<std::string, std::shared_ptr<Job>> shares_; // filter_share's tallies, by line and settings
    const StackInfo* filter_share(int line, int part, const sieve::cli::LineFilters& lf, const std::string& filter);
    void save_filters();

    SDL_Window* window_;
    SDL_Renderer* r_;
    int texture_px_ = 1024; // the renderer's widest texture (gpu::max_texture_px)
    Settings s_;
    int row_ = 0;
    sieve::cli::FilterConfig cfg_;
    std::string cfg_path_;
    AppSettings* app_ = nullptr;
    std::filesystem::path app_path_;
    int otab_ = 0;     // the filters window's tab: 0 built-in filters, 1 custom filters (plugins), 2 retired filters
    void add_filter_rows(std::vector<ORow>& rows, const sieve::FilterLine& line, const sieve::cli::LineFilters& lf, int part) const;
    int overlay_ = -1; // the door of the line whose filters are open (dimensions.hpp), or -1
    int orow_ = 0;
    int oscroll_ = 0;
    float wheel_ = 0; // wheel movement not yet whole notches
    StackInfo info_[kLines]; // one per line of the map, by door
    // One glass per column of the map: binary, the lines between, binary again. Both binary columns
    // open the same filters, because it is one line met at both ends.
    SDL_FRect magnifier_[kLines + 1] = {};
    static int overlay_of_column(int c) { return c == 0 || c == kLines ? kBinaryLine : c - 1; }
    SDL_FRect box_ = {};
    std::vector<std::pair<SDL_FRect, int>> row_rects_; // overlay rows on screen
    bool in_game_ = false;
    bool go_anyway_armed_ = false, went_in_thin_ = false;
};

} // namespace hallway
