// The pause menu: Esc in the hallway (with nothing in your hands) stops the world and offers the
// ways out of it and the tools beside it. Resume; Restart, the setup menu over the hallway (F1); the Node Graph Viewer
// (node_graph.cpp, also O); the Navigation System (the address navigator, also X); the
// File Locator (file_locator.cpp); Settings, the same screens as the main menu's, opened over the
// hallway and returning to it; and Exit Sieve, which asks whether you mean the main menu or
// leaving altogether.
//
// Every tool opened from here comes back to it when left (Esc): the node graph, the navigator (whose
// ENTER goes to the address, and so out of the pause as well), the locator, Settings, and Restart's
// setup menu when Esc there keeps this hallway.
//
// Settings and the main menu are not drawn by the hallway: it asks for them (request()) and ends
// its loop, and the application opens them and, for Settings, comes back to the same hallway
// with the pause menu still open (app_main.cpp).

#include "hallway.hpp"

namespace hallway::hall {

namespace {

struct PauseItem
{
    const char* id; // "pause.<id>" is its name, "pause.help.<id>" the line under the list
    bool built;
};

constexpr PauseItem kPauseItems[] = {
    {"resume", true}, {"restart", true}, {"graph", true}, {"navigator", true}, {"locator", true}, {"settings", true}, {"exit", true},
};
constexpr int kPauseCount = int(sizeof kPauseItems / sizeof kPauseItems[0]);

} // namespace

void Hallway::open_pause()
{
    pause_open_ = true;
    pause_confirm_ = false;
    pause_row_ = 0;
    SDL_SetWindowRelativeMouseMode(window_, false);
}

void Hallway::close_pause()
{
    pause_open_ = false;
    pause_confirm_ = false;
    SDL_SetWindowRelativeMouseMode(window_, true);
}

void Hallway::pause_choose(int row, bool& quit)
{
    if (row < 0 || row >= kPauseCount) return;
    const PauseItem& it = kPauseItems[row];
    if (!it.built)
    {
        message(tr("pause.not_built"));
        return;
    }
    const std::string id = it.id;
    if (id == "resume") close_pause();
    else if (id == "restart")
    {
        // The setup menu, as F1 opens it: Esc there comes back to this hallway as it was, and to
        // this pause menu (back_from_menu).
        close_pause();
        menu_requested_ = true;
        restart_from_pause_ = true;
        quit = true;
    }
    else if (id == "graph") open_graph(); // over the pause menu, which is there again after it
    else if (id == "navigator") open_navigator(); // over the pause menu: Esc comes back to it, ENTER goes
    else if (id == "locator") open_locator(); // over the pause menu, which is there again after it
    else if (id == "settings")
    {
        request_ = Request::Settings;
        quit = true;
    }
    else if (id == "exit") pause_confirm_ = true;
}

void Hallway::pause_event(const SDL_Event& e, bool& quit)
{
    if (pause_confirm_)
    {
        // Return to the main menu? Y: the main menu. N: leave Sieve. Esc: back to the list.
        if (e.type != SDL_EVENT_KEY_DOWN) return;
        if (e.key.key == SDLK_Y)
        {
            request_ = Request::MainMenu;
            quit = true;
        }
        else if (e.key.key == SDLK_N) quit = true;
        else if (e.key.key == SDLK_ESCAPE) pause_confirm_ = false;
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION || e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        float x = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.x : e.button.x;
        float y = e.type == SDL_EVENT_MOUSE_MOTION ? e.motion.y : e.button.y;
        SDL_RenderCoordinatesFromWindow(r_, x, y, &x, &y);
        for (int i = 0; i < int(pause_rects_.size()); ++i)
        {
            const SDL_FRect& r = pause_rects_[size_t(i)];
            if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            {
                pause_row_ = i;
                if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT) pause_choose(i, quit);
            }
        }
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    switch (e.key.key)
    {
    case SDLK_ESCAPE: close_pause(); break;
    case SDLK_UP:
    case SDLK_W: pause_row_ = (pause_row_ + kPauseCount - 1) % kPauseCount; break;
    case SDLK_DOWN:
    case SDLK_S: pause_row_ = (pause_row_ + 1) % kPauseCount; break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE: pause_choose(pause_row_, quit); break;
    default: break;
    }
}

void Hallway::draw_pause(float W, float H)
{
    const SDL_Color ink = theme().edge, grey{130, 130, 130, 255}, white{255, 255, 255, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 200);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);
    const float bw = 460, bh = 370, bx = (W - bw) / 2, by = (H - bh) / 2;
    panel(bx, by, bw, bh, 240);
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 255);
    const SDL_FRect border{bx, by, bw, bh};
    SDL_RenderRect(r_, &border);
    const std::string title = tr("pause.title");
    text(bx + (bw - text_width(title, 3)) / 2, by + 18, title, 3, ink);
    if (pause_confirm_)
    {
        const std::string q = tr("pause.confirm"), keys = tr("pause.confirm.keys");
        text(bx + (bw - text_width(q, 2)) / 2, by + 130, q, 2, white);
        text(bx + (bw - text_width(keys, 1)) / 2, by + 170, keys, 1, ink);
        return;
    }
    pause_rects_.clear();
    float y = by + 70;
    for (int i = 0; i < kPauseCount; ++i)
    {
        const PauseItem& it = kPauseItems[i];
        const std::string label = tr(std::string("pause.") + it.id);
        const SDL_FRect row{bx + 20, y - 4, bw - 40, 28};
        pause_rects_.push_back(row);
        if (i == pause_row_)
        {
            SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 60);
            SDL_RenderFillRect(r_, &row);
        }
        const SDL_Color c = !it.built ? grey : i == pause_row_ ? white : ink;
        text(bx + (bw - text_width(label, 2)) / 2, y, label, 2, c);
        y += 36;
    }
    // What the chosen item does, and what is not built yet.
    const std::string help = tr(std::string("pause.help.") + kPauseItems[pause_row_].id);
    text(bx + (bw - text_width(fit(help, bw - 30, 1), 1)) / 2, by + bh - 26, fit(help, bw - 30, 1), 1, grey);
}

} // namespace hallway::hall
