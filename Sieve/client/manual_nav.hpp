// Sieve hallway — Manual Navigation (IDEAS §19.117; Edward, 3 November 2026): a file's bytes
// turned by hand, and where each turn puts the file on its line, opened from the main menu.
//
// The binary line to begin with: a file of n bytes is a unit of the loop of every file of 0 to n
// bytes (binary-v1, SPECIFICATIONS §12.1), its rotation the bearing the navigator gives it. Every
// byte is a wheel; a window of 4 of them (8 or 16: W) is shown at a time, each turned with the
// arrows above and below it (or Up/Down, Shift for 16), or set by typing hex.
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

#include <atomic>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <optional>
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
    static void remember(bool keep_log, int undo_depth, std::filesystem::path folder, std::function<void(bool, int)> changed);

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
    // The readout above the circle, as text (for scripted runs).
    std::string readout() const;
    // From the file windows' callbacks, on any thread: applied on the main thread. `what`: 0 a file to
    // load, 1 where to save the walk, 2 a log to load.
    void file_chosen(const std::string& path, int what);

private:
    // Where a file lands: its rotation, how far from landing exactly, and its route.
    struct Reading
    {
        uint64_t gen = 0;
        int decimals = 0;         // the precision it was worked out at
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

    SDL_Window* window_;
    SDL_Renderer* r_;
    bool done_ = false;
    Result result_ = Result::Back;
    int decimals_ = 1;

    std::vector<uint8_t> bytes_; // the file, once
    std::string name_;
    std::string path_;   // its whole path, as loaded
    std::string sha_;    // its SHA-256 as loaded (hex)
    bool keep_log_ = true;
    int undo_depth_ = 0; // 0: all
    std::ofstream log_out_;
    std::string log_path_;
    uint64_t steps_made_ = 0;  // every step's number so far (turns, undos and redos)
    std::deque<Step> unlogged_; // steps made and not yet landed, to be written as they land
    bool replaying_ = false;
    std::optional<std::pair<std::string, std::vector<Logged>>> waiting_log_; // a log whose file is to be found (its SHA-256)
    std::string waiting_log_path_; // (where that log is, to carry on writing to it)
    bool from_log_ = false;        // load() called by load_log(): the file the log names
    size_t original_address_bytes_ = 0; // the file's own address as loaded, as raw bytes
    uint64_t at_ = 0;     // the window's first byte
    int span_ = 4;        // the bytes shown: 4, 8 or 16
    int chosen_ = 0;      // the byte chosen in the window
    int pending_hi_ = -1; // a first hex digit typed into the chosen byte, waiting for its second

    std::vector<Step> active_; // the steps from the file as loaded to the bytes now (each a turn)
    std::vector<Step> redo_;   // undone, for redo
    int undoable_ = 0;         // how many of active_'s last steps undo may take back (the depth at most)
    std::vector<Mark> marks_;

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
    std::vector<std::pair<SDL_FRect, int>> cells_;            // a byte's box, its index in the window
    std::vector<std::pair<SDL_FRect, int>> arrows_;           // index * 2 (+1 for down)
    std::vector<std::pair<SDL_FRect, int>> buttons_;
};

} // namespace hallway
