// Sieve hallway -- the File Locator: `sieve locate` with a window round it, opened from the main
// menu or from the hallway's pause menu (file_locator.cpp).
//
// Choose a file or a folder (the system's picker, or drop one on the window). A file is named by
// its place on the binary line and its SHA-256, set beside what zip and 7z make of it; a folder is
// walked and listed as a manifest, compared the same way. Either is weighed against its own address
// under the lines and filters given, can have those filters tailored to it, and can be saved as
// Sieve instructions or an installer program; and an installer can be installed from. In the world
// (the pause menu) a file can also be walked to: Go to it. From the main menu there is nowhere to
// walk, so Go to it is not offered. The reading, hashing and compressing run on a worker thread, so
// the window goes on drawing; the code is the tool's own (tools/cli/locate.*, compare.*, weigh.*),
// so the two agree.
#pragma once

#include "cli/args.hpp"
#include "cli/filter_config.hpp"
#include "cli/locate.hpp"
#include "cli/weigh.hpp"

#include <SDL3/SDL.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace hallway {

class FileLocator
{
public:
    // What the locator needs from where it is opened.
    struct Host
    {
        // The lines and filters files are weighed against. Called on the worker, once a weighing
        // starts; what it points to must last as long as the locator.
        std::function<sieve::cli::WeighLines()> lines;
        // The ink of the window's rules and buttons (the line's colour in the world).
        std::function<SDL_Color()> ink;
        // In the world: walk to a file (its bytes, its name, and whether it is longer than the
        // BINARY length, so the line is made to fit). Empty: not in the world, and Go to it is not
        // offered.
        std::function<void(const std::vector<uint8_t>& bytes, const std::string& name, bool past)> go;
        // In the world: the BINARY length now, which Go to it measures a file against.
        std::function<uint64_t()> binary_bytes;
        // The filters tailored to the files, to use (the world builds a hallway with them; the main
        // menu keeps them for the next one built).
        std::function<void(const sieve::cli::FilterConfig&)> use_filters;
        // Closed (Esc, or after Go to it or using the filters).
        std::function<void()> closed;
        // The shapes of the lines an installer carries items of (sieve-manifest-v6): the pages'
        // alphabet and length, the AI line's shape, as `sieve locate --items` takes them. Empty:
        // their defaults. Called on the worker.
        std::function<sieve::cli::Args()> item_args;
    };

    FileLocator(SDL_Window* window, SDL_Renderer* renderer, Host host);
    ~FileLocator();
    FileLocator(const FileLocator&) = delete;
    FileLocator& operator=(const FileLocator&) = delete;

    void open();
    void close();
    bool is_open() const { return open_; }
    bool offers_go() const { return in_world(); } // Go to it: only in the world
    void event(const SDL_Event& e);
    void draw(float W, float H);
    void stop(); // waits for the worker
    // A line under the buttons (e.g. "filters saved for the next hallway").
    void say(const std::string& status);
    // That line as it stands (scripted runs print it).
    std::string status_text();

    // Scripted runs: open on a file or folder, measured (and tailored) before the next frame; and
    // an install, at once.
    void locate_now(const std::string& path, bool tailor = false);
    // For scripted runs, before locate_now: Stop pressed so many seconds after it starts (0: never),
    // and the instructions saved to this file once it is read (empty: not), as the buttons do.
    static void script(double stop_after, const std::string& save_to);
    void install_now(const std::string& from, const std::string& to);
    // The search's time for tailoring, before a scripted run (--search-per-file, --search-total):
    // seconds a file, or for the run (`total`); the other follows, the files counted.
    void set_search(double seconds, bool total);
    // The search's time kept between runs (sieve-hallway.ini [locator]): what every File Locator
    // starts with, and what is called when the player changes it. Set once, at start.
    static void remember_search(double seconds, bool total, std::function<void(double, bool)> changed);
    // "Optimise Archive Formats" (sieve-manifest-v14), kept between runs as the search's time is:
    // off unless ticked; ticked, an archive is remade exactly from what it holds where Sieve can,
    // else carried as its contents, where either is clearly smaller. Set once, at start.
    static void remember_archives(bool on, std::function<void(bool)> changed);
    void set_archives(bool on); // scripted runs (--optimise-archives)

    // From the system dialogs' callbacks (any thread): what was chosen, handed to the next frame.
    void picked(const std::string& path);
    void save_to(const std::string& path);
    void install_picked(int step, const std::string& path); // 0 the installer, 1 where

private:
    struct Result
    {
        enum Kind { None, File, Folder } kind = None;
        std::string path, sha256, hex, table, error;
        std::vector<uint8_t> bytes; // a file
        sieve::cli::Manifest manifest; // a folder
        // Its files weighed against their own addresses, under the host's lines and filters
        // (cli/weigh.hpp), as a table; with tailoring, the filters found for the lines with items.
        std::string weighing;
        bool tailored = false;
        std::optional<sieve::cli::FilterConfig> tailored_filters;
        // How long packing it took when it was read (seconds; -1 not measured), and the most the
        // rope's search may take of that, for the save's estimate (it packs it again).
        double pack_seconds = -1, pack_search = 0;
        bool pack_search_unlimited = false;
        // Stopped (Stop, or S): what was found by then is what it holds.
        bool stopped = false;
        // Its Sieve instructions as made when it was read, their address, and what they were made
        // with: saving writes these where nothing has changed since, rather than packing again.
        std::optional<sieve::cli::Manifest> instructions;
        sieve::BigUint instructions_address;
        double made_rope_seconds = 0;
        bool made_archives = false;
    };
    bool in_world() const { return bool(host_.go); }
    // `tailor`: the weighing tailors each line's filters to the files that are its items.
    void analyse(const std::string& path, bool sync = false, bool tailor = false);
    void save(const std::string& to);
    void install(const std::string& from, const std::string& to, bool sync = false);
    void go();

    SDL_Window* window_;
    SDL_Renderer* r_;
    Host host_;
    bool open_ = false;
    std::atomic<bool> busy_{false};
    std::thread worker_;
    std::mutex mx_;
    Result result_;
    std::string status_;
    // The job under way (reading or saving): when it began, and its estimate (seconds; -1: none
    // yet, or too small to need one), with what the estimate is made of; under mx_.
    std::atomic<bool> stop_{false}; // Stop pressed: the job ends with what it has found
    std::string progress_;          // what the job is doing now, and the best so far (under mx_)
    std::chrono::steady_clock::time_point job_started_{};
    double job_estimate_ = -1;
    std::string job_estimate_text_;
    void start_job(bool new_graph = true);
    // The graph of the best so far: each new best of a step (its bytes as a share of what it
    // packs), handed over by the worker (under mx_) and drawn onto graph_ as it comes, then let go,
    // each step's line falling from the top (its first try) as it finds smaller. The texture keeps
    // the lines; nothing keeps the values but each step's first. Past its time across, what is drawn
    // is squeezed into the left half; past its depth, into the top half.
    struct GraphPoint
    {
        double seconds, share, of;
        std::string step;
    };
    std::vector<GraphPoint> graph_pending_;
    bool graph_reset_ = false, graph_drawn_ = false;
    SDL_Texture* graph_ = nullptr;
    int graph_w_ = 0, graph_h_ = 0;
    double graph_span_ = 10;       // seconds across
    float graph_x_ = -1, graph_y_ = -1;
    double graph_of_ = -1;
    double graph_first_ = 1;    // the step's first try (a share), which its line starts from at the top
    double graph_depth_ = 0.01; // how far below its first try the bottom is (1%, doubled as needed)
    std::string graph_step_;
    std::vector<std::string> graph_steps_; // the kinds of step seen, each its colour (a few)
    void draw_graph(float x, float y, float w, float h, SDL_Color ink, SDL_Color grey);
    void report(const std::string& what, double best, double of); // the worker's word, for the line and the graph
    std::string finish_job(); // "took X" (and the estimate), the job ended
    std::optional<std::string> pending_pick_, pending_save_;
    std::optional<std::pair<int, std::string>> pending_install_;
    std::string install_src_; // the installer chosen, while its folder is being chosen
    int save_what_ = 0;
    bool go_armed_ = false; // Go to it pressed once for a file past the BINARY length
    float mouse_x_ = -1, mouse_y_ = -1;
    std::vector<std::pair<SDL_FRect, std::string>> buttons_;
    // How long tailoring may search for shorter addresses (cli::WeighBudget): seconds a file and
    // for the whole run, 0 for no limit. Each moves the other, through the files counted: the run's
    // is the file's times the files, and the file's is the run's shared out among them.
    bool optimise_archives_ = false; // the box, off by default
    double search_per_file_ = 0, search_total_ = 0;
    bool search_by_total_ = false; // which was set last: the other follows it
    size_t files_counted() const;
    void step_search(bool total, int dir);
    void relink_search();
};

} // namespace hallway
