// Sieve hallway — the main menu: Start Sieve, Settings (Graphics, Controls, Language), Exit.
//
// Keyboard: Up/Down choose, Enter or Right change or open, Left change back, Esc goes back a
// screen. Mouse: point to choose, click to change (right click changes back), the wheel scrolls
// a drop-down list. Every change is applied at once and saved to sieve-hallway.ini; every
// string comes from the language files (strings.hpp).
#pragma once

#include "app_settings.hpp"

#include <SDL3/SDL.h>

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace hallway {

class MainMenu
{
public:
    MainMenu(SDL_Window* window, SDL_Renderer* renderer, AppSettings& settings, std::filesystem::path settings_path, DisplayInfo display);

    enum class Result { Start, Quit };
    Result run();
    void press(SDL_Keycode key, SDL_Keymod mod); // one key, as if typed (scripting and tests)
    void render();

private:
    enum class Screen { Main, Settings, Graphics, Controls, Language };
    enum class Kind { Action, Toggle, Choice, Number };
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

    SDL_Window* window_;
    SDL_Renderer* r_;
    AppSettings& s_;
    std::filesystem::path path_;
    DisplayInfo display_;
    Screen screen_ = Screen::Main;
    int row_ = 0;
    bool done_ = false;
    Result result_ = Result::Quit;
    // An open drop-down list: which item, its highlighted entry and first entry shown.
    std::string list_;
    int list_row_ = 0, list_top_ = 0;
    std::vector<std::pair<SDL_FRect, int>> item_rects_, list_rects_;
    SDL_FRect list_box_{};
};

} // namespace hallway
