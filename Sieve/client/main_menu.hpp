// Sieve hallway — the main menu: Start Sieve, the File Locator, the Media Player, the Filter
// Designer, the AI Training Harness, Settings (Graphics, Controls, Language, Programs), the
// Encryption Engine, Exit.
//
// Keyboard: Up/Down choose, Enter or Right change or open, Left change back, Esc goes back a
// screen. Mouse: point to choose, click to change (right click changes back), the wheel scrolls
// a drop-down list. Every change is applied at once and saved to sieve-hallway.ini; every
// string comes from the language files (strings.hpp).
#pragma once

#include "app_settings.hpp"
#include "strings.hpp"

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

    enum class Result { Start, Locator, Media, Designer, Harness, Cipher, Quit };
    Result run();
    // Settings alone, as the hallway's pause menu opens them: the menu starts on the Settings
    // screen, and backing out of it ends the menu instead of going to the main screen.
    Result run_settings()
    {
        screen_ = Screen::Settings;
        row_ = 0;
        settings_only_ = true;
        return run();
    }
    void press(SDL_Keycode key, SDL_Keymod mod); // one key, as if typed (scripting and tests)
    void render();

private:
    enum class Screen { Main, Settings, Graphics, Controls, Language, Programs };
    // A Program row names another program Sieve runs (ffmpeg): Enter chooses it with the system's
    // file window, Left goes back to looking for one by itself.
    enum class Kind { Action, Toggle, Choice, Number, Program };
    struct Item
    {
        Kind kind;
        std::string id; // also the language key: "item.<id>", and "help.<id>"
    };
    std::vector<Item> items() const;
    std::string value_of(const Item& it) const;
    void change(int dir);        // the selected item: act, toggle, step, or open its list
    void back();
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
    DisplayInfo display_;
    Screen screen_ = Screen::Main;
    bool settings_only_ = false;
    int row_ = 0;
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
};

} // namespace hallway
