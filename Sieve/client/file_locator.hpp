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
#include "dialogs.hpp"
#include "cli/filter_config.hpp"
#include "cli/locate.hpp"
#include "cli/weigh.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
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
    // "Create .sieve" from Explorer's right-click menu (hallway --create PATH, IDEAS 19.100): the
    // locator opened on PATH, working as Choose a file or a folder does, the window live (the
    // estimate, the best so far, Stop); once it is read, not stopped and not failed, its Sieve
    // instructions saved to `out` as Save does. create_finished: 0 still working, 1 saved (and said
    // for two seconds: time to close), -1 stopped or failed (the locator stays open, saying why).
    // `only`: several files or folders chosen in the folder `path` (their names in it), read as a
    // folder holding just them.
    void create(const std::string& path, const std::string& out, std::vector<std::string> only = {});
    int create_finished();
    // For scripted runs, before locate_now: Stop pressed so many seconds after it starts (0: never),
    // and the instructions saved to this file once it is read (empty: not), as the buttons do.
    // With shot_after, it is left working and the picture taken so many seconds in.
    static void script(double stop_after, const std::string& save_to, double shot_after = 0, double force_after = 0);
    void install_now(const std::string& from, const std::string& to);
    // The search's time for tailoring, before a scripted run (--search-per-file, --search-total):
    // seconds a file, or for the run (`total`); the other follows, the files counted.
    void set_search(double seconds, bool total);
    // The search's time kept between runs (sieve-hallway.ini [locator]): what every File Locator
    // starts with, and what is called when the player changes it. Set once, at start.
    static void remember_search(double seconds, bool total, std::function<void(double, bool)> changed);
    // "Optimise Compressed Formats" (once "Optimise Archive Formats"; sieve-manifest-v14 on), kept
    // between runs as the search's time is:
    // off unless ticked; ticked, an archive is remade exactly from what it holds where Sieve can,
    // else carried as its contents, where either is clearly smaller. Set once, at start.
    static void remember_archives(bool temporary, bool permanent, std::function<void(bool, bool)> changed);
    void set_archives(bool temporary, bool permanent); // scripted runs (--optimise-compressed, --compressed-temporary, --compressed-permanent)
    // "Try every way" ([locator] search_every_wave), kept between runs: the search's shortcuts off
    // (the rope's early stops on text and on bytes already packed; mixing's size limits and its test
    // on a file's start). It can only make the instructions smaller, never larger; it takes longer.
    // Its state is the packer's own (sieve::rope_search_every_wave). Set once, at start.
    static void remember_every(std::function<void(bool)> changed);
    // "Optimise Search" ([locator] optimise_search, ticked unless unticked; Edward, 2 November 2026):
    // Try every way chosen for each file by its kind, from what was measured, in place of its box
    // (sieve::cli::optimise_search). Set once, at start.
    static void remember_optimise(std::function<void(bool)> changed);
    // Size, Balanced or Decoding speed ([locator] priority, Balanced unless changed; one button that
    // goes round the three), kept between runs: what the packer weighs ways by (sieve::Priority,
    // mix.hpp). Its state is the packer's own (sieve::priority).
    static void remember_priority(std::function<void(sieve::Priority)> changed);
    // More formats (0.13.78): the comparison with gzip, bzip2, zstd, brotli and lz4 beside zip and
    // 7z, as the settings have it; `changed` saves the box when it is changed here.
    static void remember_more(bool on, std::function<void(bool)> changed);
    // Compare with zip and 7z ([locator] compare, ticked unless unticked; Z), and what saves it.
    static void remember_compare(bool on, std::function<void(bool)> changed);
    // Keep a log: whether it is ticked, the folder the logs go in, and who keeps the box's setting.
    static void remember_log(bool on, std::filesystem::path folder, std::function<void(bool)> changed);
    // The share of the search's time a run has really used, packing and weighing, learned on this
    // machine ([locator] search_share_packing, search_share_weighing; 1 until a run has measured
    // it): what the first estimate counts of the search, rather than all of it. Set once, at start.
    static void remember_shares(double packing, double weighing, std::function<void(double, double)> changed);

    // From the system dialogs' callbacks (any thread): what was chosen, handed to the next frame.
    void picked(const std::string& path);
    void save_to(const std::string& path);
    void install_picked(int step, const std::string& path); // 0 the installer, 1 where; 2 a .sieve, 3 its program's name

private:
    // (create: the file it saves to, how far it has gone (0 none, 1 reading, 2 saving, 3 saved, 4
    // stopped or failed), and when it was saved; last_save_ok_: whether the last save wrote its file)
    std::string create_out_;
    std::vector<std::string> only_; // (the names chosen, for the next reading only)
    int create_state_ = 0;
    std::chrono::steady_clock::time_point create_saved_at_{};
    bool last_save_ok_ = false;
    struct Result
    {
        enum Kind { None, File, Folder } kind = None;
        std::string path, sha256, hex, table, error; // (hex: the address as shown, cli::raw_summary)
        size_t hex_digits = 1;
        std::vector<uint8_t> bytes; // a file
        sieve::cli::Manifest manifest; // a folder
        // Its files weighed against their own addresses, under the host's lines and filters
        // (cli/weigh.hpp), as a table; with tailoring, the filters found for the lines with items.
        std::string weighing;
        std::string weighing_whole; // for Keep a log: every file's row (the window shows 12), then every way weighed
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
        std::vector<uint8_t> instructions_raw; // their address's raw bytes (cli::raw_of_file)
        double made_rope_seconds = 0;
        int made_archives = 0; // the Compressed formats boxes it was made with (1 temporary, 2 permanent)
        bool made_every = false;    // Try every way, as it was when they were made
        bool made_optimise = false; // Optimise Search, as it was when they were made
        int made_priority = 0; // Size, Balanced or Decoding speed, as it was (sieve::Priority)
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
    // A warning an install gave (install_tree's notes: a RAR given as its contents only, its rar not
    // found), popped up in a box of its own on the next frame; under mx_.
    std::string pending_warning_;
    // A RAR the instructions carry to be made again by rar on installing (cli::rar_remake_warning):
    // said in a box (0.13.78), as an install's warning is; from the worker, so under mx_.
    void warn_rars(const sieve::cli::Manifest& m);
    // The job under way (reading or saving): when it began, and its estimate (seconds; -1: none
    // yet, or too small to need one), with what the estimate is made of; under mx_.
    // Stop pressed: once (sieve::kStop), the job ends with the best it has, the pass under way
    // finished; twice (kForce), the pass under way abandoned too, the last finished kept.
    std::atomic<int> stop_{0};
    // An install under way (3 November 2026, Edward): its Stop cancels it, the decoding included
    // (sieve::DecodeCancelScope), and nothing is left written (install_tree takes back what it wrote).
    std::atomic<bool> installing_{false}, cancel_install_{false};
    std::string progress_;          // what the job is doing now, and the best so far (under mx_)
    std::chrono::steady_clock::time_point job_started_{};
    double job_estimate_ = -1;
    std::string job_estimate_text_;
    std::string memory_text_; // the memory the reading needs, about, said beside the estimate (IDEAS 19.97)
    // The estimate step by step (IDEAS §19.97): what the samples predict for each step of the
    // reading (seconds, without the search's time), refined as each is done by how long the done
    // ones took against their prediction; the first estimate kept, to say beside the time taken.
    std::array<double, 6> est_steps_{}; // address and SHA-256, zip, 7z, the instructions, compared, the weighing
    bool est_on_ = false;
    // The estimate refined: its words made as it is drawn, so "left" counts down with the time so far
    // (made once, they stood still: 37 min left at 1 h 28 min in, of an estimate of 51 min), and
    // whether the search has no limit, so the estimate leaves it out and says so.
    bool est_live_ = false, est_unlimited_ = false;
    std::string estimate_text() const; // under mx_
    // The search's limits for packing and for weighing (seconds, all of it), and the share of them a
    // run uses as learned (est_share_*); est_ratio_: this run's speed against the samples, from the
    // steps with no search in them (1 to 3); est_done_: the steps done.
    double est_first_ = -1, est_pack_search_ = 0, est_weigh_search_ = 0, est_share_pack_ = 1, est_share_weigh_ = 1, est_ratio_ = 1;
    size_t est_done_ = 0;
    std::chrono::steady_clock::time_point est_work_start_{}, est_pack_start_{}, est_weigh_start_{}, est_ticked_{};
    double est_pack_share_seen_ = -1; // this run's packing: the share of its search's limit it used
    double last_took_ = -1;           // the last job's time, shown under the graph once it is done
    double read_took_ = -1;           // the last reading's whole time (its search with it), for a save that packs nothing again
    // Keep a log: the run's file, open while it works (under mx_), what it last said, and its last
    // best by step, to mark a new one.
    std::ofstream log_;
    std::filesystem::path log_path_;
    std::string log_last_, log_step_;
    double log_best_ = -1, log_best_at_ = -1, log_of_ = -1;
    // (`search`: a reading or a saving, with the search's settings and its reports; an install has neither.)
    void open_log(const std::string& what, const std::string& path, bool search = true); // under mx_
    void close_log(const std::string& result); // under mx_: the end, written, and the status says where
    void refine_estimate(size_t steps_done); // after step steps_done - 1 is done; under no lock
    void tick_estimate();                    // while packing: the search's part again, as its files are done; under no lock
    void estimate_now(double remaining);     // under mx_
    void learn_shares(bool stopped);         // at the job's end; under mx_
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
    double graph_best_at_ = -1;    // when the last best so far came, seconds into the job
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
    std::string program_src_; // the .sieve chosen, while its installer program's name is being chosen
    // An installer program from Sieve instructions already made (Edward, 29 October 2026): the
    // .sieve's bytes attached to sieve-install as they are, nothing read or packed again.
    void program_from(const std::string& sieve_file, const std::string& out);
    int save_what_ = 0;
    // The folder an installer program offers to install into (0.13.78, Edward's idea): asked for
    // once the program's name is chosen, typed as Documents/Games/Name (cli/locate.hpp,
    // offered_folder_problem); empty: none, so it offers Documents and the folder's name.
    struct FolderAsk
    {
        bool from_sieve = false; // made from a .sieve already made (program_from), else from what was read
        std::string out, text, problem;
    };
    std::optional<FolderAsk> asking_folder_;
    std::string offer_folder_; // the answer, for the program being written
    void ask_folder(bool from_sieve, const std::string& out, const std::string& name);
    void folder_event(const SDL_Event& e);
    void draw_folder_prompt(float W, float H, SDL_Color ink, SDL_Color grey);
    bool go_armed_ = false; // Go to it pressed once for a file past the BINARY length
    float mouse_x_ = -1, mouse_y_ = -1;
    std::vector<std::pair<SDL_FRect, std::string>> buttons_;
    // How long tailoring may search for shorter addresses (cli::WeighBudget): seconds a file and
    // for the whole run, 0 for no limit. Each moves the other, through the files counted: the run's
    // is the file's times the files, and the file's is the run's shared out among them.
    // Compressed formats (v31; Optimise Compressed Formats before): temporary, made again exactly on
    // installing; permanent, carried as what it holds. Both off by default; both ticked, either.
    bool temporary_ = false, permanent_ = false;
    int compressed() const { return (temporary_ ? 1 : 0) | (permanent_ ? 2 : 0); }
    double search_per_file_ = 0, search_total_ = 0;
    bool search_by_total_ = false; // which was set last: the other follows it
    size_t files_counted() const;
    void step_search(bool total, int dir);
    void relink_search();
    // Last, so it is the first let go: its dialogs' answers after it has gone are dropped (dialogs.hpp).
    hallway::DialogOwner dialog_owner_{this};
};

} // namespace hallway
