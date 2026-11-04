// Sieve hallway — Manual Navigation (IDEAS §19.117; Edward, 3 November 2026): a file's bytes
// turned by hand, and where each turn puts the file on its line, opened from the main menu.
//
// The binary line to begin with: a file of n bytes is a unit of the loop of every file of 0 to n
// bytes (binary-v1, SPECIFICATIONS §12.1), its rotation the bearing the navigator gives it. Every
// byte is a wheel. With no file loaded, an example: a square of random bytes, its side the width
// (W: 2 to 16, so 4 to 256 bytes), made afresh when the width changes where FRESH RANDOM BYTES is
// ticked (F; on by default), else kept, random bytes added or the end cut (Edward, 4 November
// 2026: "4 random bytes as the default loaded example 'file' [...] where we would learn to drive
// a 'file'"). A file loaded is laid out whole, as a sheet of that many bytes a row, scrolling. The
// byte chosen is turned with the arrows at its top and bottom (or Up/Down, Shift for 16), or set
// by typing hex; bytes no longer as loaded are marked.
//
// The circle at the right: the dot's angle is the file's rotation (0 at the top, clockwise, as on
// the compass); its distance from the centre is how far that rotation is from landing exactly on a
// bearing at the Angle Precision chosen ([ and ]): 0 at the centre, a bearing landed on exactly,
// and 1 at the rim, halfway between two. The first mark is in the binary line's two colours; each
// turn after leaves a mark, green where it moved in towards the centre and red where it moved out.
//
// Above: the rotation, the shortest route to it (the navigator's variable length addressing), and
// what the steps cost: each names a byte and its new value. The net is the file's own address,
// less the route and the steps: the steps are the walk back from where the route lands to the file,
// so a net above 0 would be a file written shorter than itself.
//
// Steering (Edward, 4 November 2026: "add the steering readout"): for the byte chosen, how far one
// turn of it moves the file, in gaps between bearings, and which value of it puts the dot nearest
// the centre, how many turns up or down, shown as a hollow mark on the circle. A turn of a byte moves
// the file by a fixed amount, its place's weight (256 to the power of the bytes after it), whatever
// the bytes are; so that move, one division the size of the file, is worked out on the worker only
// when another byte is chosen, the precision changes or a file is loaded, and the nearest value is
// then found among the byte's 256 at once. A step does not ask for it again.
//
// The file is held once. Each step is kept as its offset and the byte before and after. Undo and
// redo (Ctrl+Z, Ctrl+Y) go back as far as the Undo depth says (U: all, the default, or 10, 100,
// 1,000). Keep a log (L, on by default; Edward, 4 November 2026) writes every step, undone ones
// included, to a CSV in the logs folder beside the settings as each lands, with its bearings before
// and after; its head names the file (its whole path, size and SHA-256) and the precision, so Load
// Log (Ctrl+L) can open the file again and replay it, undos and redos included, and carry on
// writing to it. Save Steps (Ctrl+S) writes just the walk: the steps from the file as loaded to the
// bytes now, to a place chosen. Where each step lands is
// worked out on a worker (a file of 64 KiB is a number of half a million bits), so the screen never
// waits. Every step is worked out, in order (the worker keeps its own copy of the bytes and takes
// each step to it), so each has its own before and after and its own mark, however fast the keys
// come. The route is worked out up to 64 KiB, and files are taken up to 1 MiB.
#pragma once

#include <SDL3/SDL.h>

#include "dialogs.hpp" // DialogOwner

#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace hallway {

class ManualNav
{
public:
    ManualNav(SDL_Window* window, SDL_Renderer* renderer, int angle_decimals);
    ~ManualNav();
    ManualNav(const ManualNav&) = delete;
    ManualNav& operator=(const ManualNav&) = delete;

    enum class Result { Back, Quit };
    Result run();

    // Keep a log and the Undo depth (0: all) as the settings have them, the logs folder, and who to
    // tell when either is changed here ([manual] in the settings).
    struct Kept
    {
        bool keep_log = true;
        int undo_depth = 0; // 0: all
        bool fresh = true;  // FRESH RANDOM BYTES
        int width = 2;      // bytes a row, and the example's side
    };
    static void remember(Kept kept, std::filesystem::path folder, std::function<void(const Kept&)> changed);
    // Scripting: the example made again from this seed (its bytes the same every run).
    void seed(uint32_t s);

    // A file's bytes to steer (Ctrl+O's, or --manual-nav FILE in scripted runs); false, with the
    // status saying why, where it cannot be taken.
    bool load(const std::string& path);
    void press(SDL_Keycode key, SDL_Keymod mod); // scripting: one key
    void type(const std::string& utf8);          // scripting: hex typed into the byte chosen
    void render();
    void wait(); // scripting: until the worker has worked out the last step
    // The walk as CSV: the steps from the file as loaded to the bytes now (Ctrl+S's, or --manual-steps
    // FILE); false, the status saying why.
    bool save_steps(const std::string& path);
    // A log opened: its file loaded from the path it names (asked for where it is not there, or not
    // the same bytes) and every step replayed (Ctrl+L's, or --manual-log FILE); false, the status
    // saying why.
    bool load_log(const std::string& path);
    const std::string& log_path() const { return log_path_; } // the log being written ("" for none)
    const std::string& status() const { return status_; }
    bool loaded() const { return !bytes_.empty() && !example_; } // a file's bytes to turn (not the example)
    // The readout above the circle, as text (for scripted runs).
    std::string readout() const;
    // The circle's marks in order, a letter each (for scripted runs): S the start, G in (green), R out
    // (red), X an exact landing.
    std::string marks_text() const;
    std::string steering() const { return steer_text(); } // the steering line, as text (for scripted runs)
    // From the file windows' callbacks, on any thread: applied on the main thread. `what`: 0 a file to
    // load, 1 where to save the walk, 2 a log to load.
    void file_chosen(const std::string& path, int what);
    // Scripting: `path` given as a file window would give it (0 load, 1 save the walk, 2 a log), through
    // the same answer as the window's (its owner checked), then taken as the next frame takes it.
    void answer_as_window(const std::string& path, int what);

private:
    // Where a file lands: its rotation, how far from landing exactly, and its route.
    struct Reading
    {
        uint64_t gen = 0;
        int decimals = 0;         // the precision it was worked out at
        char kind = 0;            // what it answers for: 't' a turn, 'u' an undo, 'r' a redo, 's' steering only, 0 the bytes whole
        double pos = 0;           // where in the gap between two bearings the file lies (0 to 1)
        // ('s': one turn of byte `sel`, in gaps between bearings, its whole and its share of a turn)
        uint64_t sel = 0;
        double gaps = 0, turn_step = 0;
        std::string bearing;      // at the precision asked
        double turn = 0;          // the rotation as a fraction of a turn (for drawing)
        double off = 1;           // 0: lands exactly on a bearing; 1: halfway between two
        bool exact = false;
        bool routed = false;      // the route worked out (64 KiB or less)
        std::string route;        // as the navigator writes it ("90.1+3f")
        double route_bits = 0;
        size_t address_bytes = 0; // the file's own address, as raw bytes
    };
    struct Step
    {
        std::string kind; // "turn", "undo", "redo"
        uint64_t offset = 0;
        uint8_t from = 0, to = 0;
        std::string bearing_before, bearing_after;
        double off_before = 1, off_after = 1;
        bool pending = true; // where it lands not worked out yet
        uint64_t gen = 0;    // the bytes' generation it makes (its reading answers for it)
        uint64_t number = 0; // its number in the log (1, 2, ...)
    };
    // A log's line, read back: a step (turn, undo, redo) or the precision changed.
    struct Logged
    {
        std::string kind;
        uint64_t offset = 0;
        uint8_t from = 0, to = 0;
        int decimals = 0; // ("precision")
    };
    // The worker's work, in order: the bytes as they are (a file loaded, the precision changed),
    // or one step taken to its own copy.
    struct Job
    {
        uint64_t gen = 0;
        int decimals = 0;
        bool whole = false;
        char kind = 0; // ('t', 'u', 'r': the step's; 's' steering for byte `sel`; 0 for the bytes whole)
        uint64_t sel = 0;
        std::vector<uint8_t> bytes; // (whole)
        uint64_t at = 0;
        uint8_t value = 0;
    };
    struct Mark
    {
        double turn = 0, off = 1;
        int kind = 0; // 0 the first, 1 in (green), 2 out (red), 3 exact
    };

    void key(SDL_Keycode k, SDL_Keymod mod);
    void handle(const SDL_Event& e);
    void set_byte(uint64_t at, uint8_t value, const char* kind); // a step: applied, logged, landed
    void undo();
    void redo();
    void start_reading();    // the bytes as they are now, queued whole for the worker
    void queue(Job j);       // a job queued, the worker started if it is not working
    void collect_reading();  // its answers taken, in order
    static Reading read(const std::vector<uint8_t>& bytes, int decimals, uint64_t gen);
    void poll_chosen();
    int depth() const;                    // the undo depth now (a large number for all)
    void open_log(bool append);           // the log begun (or carried on), as Keep a log says
    void write_landed(const Step& s);     // a step's line, once it has landed
    void replay(const std::vector<Logged>& lines);
    // New bytes to turn (a file read, an example made, an example read back from a log): the steps,
    // marks and log of the last let go.
    void take(std::vector<uint8_t> bytes, std::string name, std::string path, bool example);
    void make_example(bool keep); // a width-by-width square of random bytes (keep: the last one's, resized)
    void keep_in_view();          // the sheet scrolled to the byte chosen
    void tell_kept();             // the settings told what is kept

    SDL_Window* window_;
    SDL_Renderer* r_;
    bool done_ = false;
    Result result_ = Result::Back;
    int decimals_ = 1;

    std::vector<uint8_t> bytes_; // the file, once
    std::vector<uint8_t> loaded_; // as loaded, to mark the bytes changed since
    std::string name_;
    std::string path_;   // its whole path, as loaded
    std::string sha_;    // its SHA-256 as loaded (hex)
    bool keep_log_ = true;
    int undo_depth_ = 0; // 0: all
    bool fresh_ = true;   // FRESH RANDOM BYTES
    bool example_ = false; // the bytes an example made here, not a file
    std::mt19937 rng_;
    std::ofstream log_out_;
    std::string log_path_;
    uint64_t steps_made_ = 0;  // every step's number so far (turns, undos and redos)
    std::deque<Step> unlogged_; // steps made and not yet landed, to be written as they land
    bool replaying_ = false;
    std::optional<std::pair<std::string, std::vector<Logged>>> waiting_log_; // a log whose file is to be found (its SHA-256)
    std::string waiting_log_path_; // (where that log is, to carry on writing to it)
    bool from_log_ = false;        // load() called by load_log(): the file the log names
    size_t original_address_bytes_ = 0; // the file's own address as loaded, as raw bytes
    uint64_t sel_ = 0;    // the byte chosen
    uint64_t top_ = 0;    // the sheet's first row shown
    int width_ = 2;       // bytes a row, and the example's side: 2 to 16
    int rows_shown_ = 8;  // rows the sheet has room for (as last drawn)
    int pending_hi_ = -1; // a first hex digit typed into the chosen byte, waiting for its second

    std::vector<Step> active_; // the steps from the file as loaded to the bytes now (each a turn)
    std::vector<Step> redo_;   // undone, for redo
    int undoable_ = 0;         // how many of active_'s last steps undo may take back (the depth at most)
    std::vector<Mark> marks_;      // the walk's marks: the start, then one for each step it holds
    std::vector<Mark> redo_marks_; // marks undone, put back by redo as they were
    // Steering for the byte chosen: one turn of it, in gaps between bearings (as worked out for this
    // byte, precision and length); asked for once each, an earlier ask still queued let go.
    struct Steer
    {
        bool valid = false;
        uint64_t sel = 0;
        int decimals = -1;
        size_t length = 0;
        double gaps = 0, turn_step = 0;
        double frac = 0; // what a turn moves past whole gaps (0 to 1)
    };
    Steer steer_;
    uint64_t steer_asked_sel_ = UINT64_MAX;
    int steer_asked_dec_ = -1;
    size_t steer_asked_len_ = 0;
    void ask_steer(); // a steering job queued where the byte chosen (or the precision, or the length) changed
    // The nearest value of the byte chosen: its change in turns (0: as near as it can be), where the
    // dot would be (its share of a turn, and its distance from landing).
    struct Target
    {
        int turns = 0;
        double turn = 0, off = 1;
    };
    std::optional<Target> target() const;
    std::string steer_text() const;

    Reading now_;               // the latest reading taken
    bool have_reading_ = false;
    std::thread worker_;
    std::atomic<bool> busy_{false}; // (set under mx_)
    std::mutex mx_;
    std::deque<Job> jobs_;          // (mx_)
    std::deque<Reading> answers_;   // (mx_)
    std::vector<uint8_t> wbytes_;   // the worker's own copy of the bytes (the worker's alone)
    uint64_t gen_ = 0;              // the bytes' generation: one more at every step

    std::string status_;
    std::optional<std::pair<std::string, int>> chosen_file_; // (mx_)
    std::vector<std::pair<SDL_FRect, uint64_t>> cells_;       // a byte's box, its offset
    std::vector<std::pair<SDL_FRect, int>> arrows_;           // the chosen byte's: 0 up, 1 down
    std::vector<std::pair<SDL_FRect, int>> buttons_;
    // Last, so the first let go: while it lives, the file windows' answers reach this screen; without
    // it they were dropped (4 November 2026, Edward: "Opening a file seems to do nothing").
    hallway::DialogOwner dialog_owner_{this};
};

} // namespace hallway
