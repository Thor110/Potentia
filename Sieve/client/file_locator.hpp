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

#include "cli/filter_config.hpp"
#include "cli/locate.hpp"
#include "cli/weigh.hpp"

#include <SDL3/SDL.h>

#include <atomic>
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
    };

    FileLocator(SDL_Window* window, SDL_Renderer* renderer, Host host);
    ~FileLocator() { stop(); }
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

    // Scripted runs: open on a file or folder, measured (and tailored) before the next frame; and
    // an install, at once.
    void locate_now(const std::string& path, bool tailor = false);
    void install_now(const std::string& from, const std::string& to);

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
    std::optional<std::string> pending_pick_, pending_save_;
    std::optional<std::pair<int, std::string>> pending_install_;
    std::string install_src_; // the installer chosen, while its folder is being chosen
    int save_what_ = 0;
    bool go_armed_ = false; // Go to it pressed once for a file past the BINARY length
    float mouse_x_ = -1, mouse_y_ = -1;
    std::vector<std::pair<SDL_FRect, std::string>> buttons_;
};

} // namespace hallway
