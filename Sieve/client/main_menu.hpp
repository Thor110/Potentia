// Sieve hallway — the main menu: Start Sieve, the File Locator, the Media Player, the Filter
// Designer, the AI Training Harness, Settings (Graphics, Controls, Language, Programs), the
// Encryption Engine, Manual Navigation, Exit.
//
// Keyboard: Up/Down choose, Enter or Right change or open, Left change back, Esc goes back a
// screen. Mouse: point to choose, click to change (right click changes back), the wheel scrolls
// a drop-down list. Every change is applied at once and saved to sieve-hallway.ini; every
// string comes from the language files (strings.hpp).
#pragma once

#include "favourites.hpp"

#include "app_settings.hpp"
#include "strings.hpp"
#include "dialogs.hpp" // DialogOwner

#include <SDL3/SDL.h>

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

class MainMenu
{
public:
    MainMenu(SDL_Window* window, SDL_Renderer* renderer, AppSettings& settings, std::filesystem::path settings_path, DisplayInfo display);

    enum class Result { Start, Locator, Media, Designer, Harness, Cipher, ManualNav, Personal, Systems, Creator, Replay, Quit };
    Result run();
    // The system chosen in the System Viewer (Result::Systems): the entry it stands at to begin with
    // ("information" for FORMATS, "language" for LANGUAGE).
    const std::string& system_root() const { return system_root_; }
    // The recording chosen in the replay viewer (Result::Replay), to play in the hallway.
    const std::filesystem::path& replay_chosen() const { return replay_chosen_; }
    // One screen and those under it, as the hallway's pause menu opens them: the menu starts on
    // that screen, and backing out of it ends the menu (Result::Start: back to the hallway) instead
    // of going to the main screen. Settings, and Personal Dimensions.
    Result run_settings() { return run_rooted(Screen::Settings); }
    Result run_personal() { return run_rooted(Screen::Personal); }
    void press(SDL_Keycode key, SDL_Keymod mod); // one key, as if typed (scripting and tests)
    void render();
    // (The completion estimate, IDEAS §19.49, each line's walk time, was drawn by the main menu
    // until 9 November 2026; it is now the Start Sieve map's, client/menu.cpp.)

private:
    enum class Screen { Main, Settings, Graphics, Controls, Language, Programs, Personal, Systems, Raid, Replays, Game, Features, Favourites, Keys };
    // A Program row names another program Sieve runs (ffmpeg): Enter chooses it with the system's
    // file window, Left goes back to looking for one by itself.
    // An Info row only shows a value (one set in the settings file: the RAID's machines).
    // A Replay row is one recording in the replay viewer: Enter plays it, Space (or Left/Right) ticks
    // its box, decoding it once into the cache rather than each time (a .sieve recording).
    enum class Kind { Action, Toggle, Choice, Number, Program, Info, Replay };
    struct Item
    {
        Kind kind;
        std::string id; // also the language key: "item.<id>", and "help.<id>"
    };
    std::vector<Item> items() const;
    static std::string help_key(const Item& it);
    std::string value_of(const Item& it) const;
    void change(int dir);        // the selected item: act, toggle, step, or open its list
    void back();
    bool select(const std::string& id); // the row of that name on this screen chosen; false if none
    std::string key_row_label(size_t i) const; // Settings > Controls > Keys: "section: what it does"
    void open_list(const Item& it);
    void choose_from_list(int index);
    std::vector<std::string> list_labels() const;
    void handle(const SDL_Event& e);
    void save();
    // ffmpeg, as the Programs screen shows it: worked out when the screen opens and when the
    // setting changes, not every frame (finding it looks through every folder on the PATH).
    void refresh_ffmpeg();
    void poll_ffmpeg_chosen();

public:
    void ffmpeg_chosen(const std::string& path); // from the file window's callback, on any thread

private:
    SDL_Window* window_;
    SDL_Renderer* r_;
    AppSettings& s_;
    std::filesystem::path path_;
    std::string raid_note_; // Settings > RAID: the key made, or why not
    // The replay viewer (IDEAS 19.184): the recordings in `replays` beside the settings, read as the
    // screen opens; the one chosen to play; whether each is ticked to be cached.
    std::vector<std::filesystem::path> replays_;
    // Every favourite, every dimension's and the Media Player's (IDEAS 19.190; favourites.hpp), read as the
    // Favourites screen opens.
    std::vector<hallway::FavouriteRow> favourite_rows_;
    // Settings > Controls > Keys (IDEAS 19.199): the row whose next key press becomes its key, or -1.
    int key_capture_ = -1;
    std::filesystem::path replay_chosen_;
    void read_replays();
    bool replay_cached(size_t i) const;
    void toggle_replay_cache(size_t i);
    DisplayInfo display_;
    Screen screen_ = Screen::Main;
    std::string system_root_ = "information";
    // Opened on one screen alone (run_rooted): backing out of it ends the menu.
    bool rooted_ = false;
    Screen root_{};
    Result run_rooted(Screen s)
    {
        screen_ = root_ = s;
        row_ = 0;
        rooted_ = true;
        return run();
    }
    int row_ = 0;
    int spoken_screen_ = -1; // the screen last read aloud on (speech.hpp): a new one is read from its first item
    bool done_ = false;
    Result result_ = Result::Quit;
    // An open drop-down list: which item, its highlighted entry and first entry shown.
    std::string list_;
    int list_row_ = 0, list_top_ = 0;
    std::vector<std::pair<SDL_FRect, int>> item_rects_, list_rects_;
    SDL_FRect list_box_{};
    std::vector<LanguageInfo> langs_; // the language files found when the menu opened
    float wheel_ = 0;                 // wheel movement not yet whole notches
    std::string ffmpeg_version_, ffmpeg_where_; // its version ("" if none) and the line under it
    std::mutex chosen_mx_;
    std::optional<std::string> ffmpeg_chosen_; // a file the window gave, for the main thread to apply
    // Last, so the first let go: the file window's answer reaches this menu only while it lives.
    hallway::DialogOwner dialog_owner_{this};
};

} // namespace hallway
