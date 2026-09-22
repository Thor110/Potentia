#include "main_menu.hpp"

#include "font.hpp"
#include "strings.hpp"

#include <algorithm>
#include <cmath>

namespace hallway {

namespace {

constexpr int kMinW = 1000, kMinH = 640; // smaller windows draw the menu scaled down
constexpr int kListRows = 10;            // drop-down entries shown at once
const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kAccent{0, 255, 255, 255};

void draw_centred(SDL_Renderer* r, float cx, float y, const std::string& s, float scale, SDL_Color c)
{
    draw_text(r, cx - text_width(s, scale) / 2, y, s, scale, c);
}

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

std::vector<std::string> split_lines(const std::string& s)
{
    std::vector<std::string> out;
    size_t a = 0;
    for (size_t b; (b = s.find('\n', a)) != std::string::npos; a = b + 1) out.push_back(s.substr(a, b - a));
    out.push_back(s.substr(a));
    return out;
}

} // namespace

MainMenu::MainMenu(SDL_Window* window, SDL_Renderer* renderer, AppSettings& settings, std::filesystem::path settings_path,
                   DisplayInfo display)
    : window_(window), r_(renderer), s_(settings), path_(std::move(settings_path)), display_(std::move(display)),
      langs_(available_languages()) // read once: the list is drawn every frame
{
}

std::vector<MainMenu::Item> MainMenu::items() const
{
    switch (screen_)
    {
    case Screen::Main: return {{Kind::Action, "start"}, {Kind::Action, "settings"}, {Kind::Action, "exit"}};
    case Screen::Settings:
        return {{Kind::Action, "graphics"}, {Kind::Action, "controls"}, {Kind::Action, "language"}, {Kind::Action, "back"}};
    case Screen::Graphics:
        return {{Kind::Choice, "resolution"}, {Kind::Toggle, "fullscreen"}, {Kind::Toggle, "vsync"}, {Kind::Toggle, "edge_glow"}, {Kind::Toggle, "real_graphics"}, {Kind::Toggle, "fps_counter"},
                {Kind::Action, "back"}};
    case Screen::Controls: return {{Kind::Number, "mouse_sensitivity"}, {Kind::Toggle, "invert_mouse_y"}, {Kind::Action, "back"}};
    case Screen::Language: return {{Kind::Choice, "language_choice"}, {Kind::Action, "back"}};
    }
    return {};
}

std::string MainMenu::value_of(const Item& it) const
{
    auto onoff = [](bool b) { return tr(b ? "value.on" : "value.off"); };
    if (it.id == "resolution") return s_.resolution.w > 0 ? s_.resolution.str() : tr("value.none");
    if (it.id == "fullscreen") return onoff(s_.fullscreen);
    if (it.id == "vsync") return onoff(s_.vsync);
    if (it.id == "edge_glow") return onoff(s_.edge_glow);
    if (it.id == "real_graphics") return onoff(s_.real_graphics);
    if (it.id == "fps_counter") return onoff(s_.fps_counter);
    if (it.id == "mouse_sensitivity") return std::to_string(s_.mouse_sensitivity) + "%";
    if (it.id == "invert_mouse_y") return onoff(s_.invert_mouse_y);
    if (it.id == "language_choice")
    {
        for (const auto& l : langs_)
            if (l.code == language_code()) return l.name;
        return language_code();
    }
    return "";
}

void MainMenu::save()
{
    s_.save(path_); // a read-only folder still keeps the choices for this session
}

std::vector<std::string> MainMenu::list_labels() const
{
    std::vector<std::string> out;
    if (list_ == "resolution")
        for (const Resolution& r : display_.modes)
            out.push_back(r.str() + (r == display_.desktop ? "  " + tr("value.desktop") : std::string()));
    else if (list_ == "language_choice")
        for (const auto& l : langs_) out.push_back(l.name);
    return out;
}

void MainMenu::open_list(const Item& it)
{
    list_ = it.id;
    list_row_ = 0;
    if (it.id == "resolution")
    {
        const auto f = std::find(display_.modes.begin(), display_.modes.end(), s_.resolution);
        list_row_ = f == display_.modes.end() ? 0 : int(f - display_.modes.begin());
    }
    else
    {
        const auto& langs = langs_;
        for (size_t i = 0; i < langs.size(); ++i)
            if (langs[i].code == language_code()) list_row_ = int(i);
    }
    list_top_ = std::max(0, list_row_ - kListRows / 2);
}

void MainMenu::choose_from_list(int index)
{
    if (list_ == "resolution" && index >= 0 && index < int(display_.modes.size()))
    {
        s_.resolution = display_.modes[size_t(index)];
        apply_video(window_, s_);
    }
    else if (list_ == "language_choice")
    {
        const auto& langs = langs_;
        if (index >= 0 && index < int(langs.size()) && set_language(langs[size_t(index)].code)) s_.language = langs[size_t(index)].code;
    }
    list_.clear();
    save();
}

void MainMenu::back()
{
    if (!list_.empty()) { list_.clear(); return; }
    switch (screen_)
    {
    case Screen::Main: row_ = 2; return; // Esc on the main screen points at Exit rather than quitting
    case Screen::Settings: screen_ = Screen::Main; row_ = 1; return;
    case Screen::Graphics: screen_ = Screen::Settings; row_ = 0; return;
    case Screen::Controls: screen_ = Screen::Settings; row_ = 1; return;
    case Screen::Language: screen_ = Screen::Settings; row_ = 2; return;
    }
}

void MainMenu::change(int dir)
{
    const auto its = items();
    if (row_ < 0 || row_ >= int(its.size())) return;
    const Item& it = its[size_t(row_)];
    switch (it.kind)
    {
    case Kind::Action:
        if (dir < 0) return;
        if (it.id == "start") { result_ = Result::Start; done_ = true; }
        else if (it.id == "exit") { result_ = Result::Quit; done_ = true; }
        else if (it.id == "settings") { screen_ = Screen::Settings; row_ = 0; }
        else if (it.id == "graphics") { screen_ = Screen::Graphics; row_ = 0; }
        else if (it.id == "controls") { screen_ = Screen::Controls; row_ = 0; }
        else if (it.id == "language") { screen_ = Screen::Language; row_ = 0; }
        else if (it.id == "back") back();
        return;
    case Kind::Toggle:
        if (it.id == "fullscreen")
        {
            s_.fullscreen = !s_.fullscreen;
            apply_video(window_, s_);
        }
        else if (it.id == "vsync")
        {
            s_.vsync = !s_.vsync;
            SDL_SetRenderVSync(r_, s_.vsync ? 1 : 0);
        }
        else if (it.id == "edge_glow") s_.set_edge_glow(!s_.edge_glow);
        else if (it.id == "real_graphics") s_.set_real_graphics(!s_.real_graphics);
        else if (it.id == "fps_counter") s_.fps_counter = !s_.fps_counter;
        else if (it.id == "invert_mouse_y") s_.invert_mouse_y = !s_.invert_mouse_y;
        save();
        return;
    case Kind::Number:
        s_.mouse_sensitivity = std::clamp(s_.mouse_sensitivity + dir * 10, 10, 400);
        save();
        return;
    case Kind::Choice: open_list(it); return;
    }
}

void MainMenu::press(SDL_Keycode key, SDL_Keymod mod)
{
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = key;
    e.key.mod = mod;
    handle(e);
}

void MainMenu::handle(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT) { result_ = Result::Quit; done_ = true; return; }
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e); // the menu may be drawn scaled (see render)
    const int n = int(items().size());
    if (!list_.empty())
    {
        const int count = int(list_labels().size());
        auto keep_visible = [&] {
            list_row_ = std::clamp(list_row_, 0, std::max(0, count - 1));
            if (list_row_ < list_top_) list_top_ = list_row_;
            if (list_row_ >= list_top_ + kListRows) list_top_ = list_row_ - kListRows + 1;
        };
        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            switch (e.key.key)
            {
            case SDLK_UP: --list_row_; break;
            case SDLK_DOWN: ++list_row_; break;
            case SDLK_PAGEUP: list_row_ -= kListRows; break;
            case SDLK_PAGEDOWN: list_row_ += kListRows; break;
            case SDLK_HOME: list_row_ = 0; break;
            case SDLK_END: list_row_ = count - 1; break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
            case SDLK_SPACE: choose_from_list(list_row_); return;
            case SDLK_ESCAPE:
            case SDLK_LEFT: list_.clear(); return;
            default: break;
            }
            keep_visible();
        }
        else if (e.type == SDL_EVENT_MOUSE_WHEEL)
        {
            wheel_ += e.wheel.y; // touchpads send fractions of a notch
            const int notches = int(wheel_);
            wheel_ -= float(notches);
            list_top_ = std::clamp(list_top_ - notches, 0, std::max(0, count - kListRows));
        }
        else if (e.type == SDL_EVENT_MOUSE_MOTION)
        {
            for (const auto& [r, i] : list_rects_)
                if (inside(r, e.motion.x, e.motion.y)) list_row_ = i;
        }
        else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
        {
            if (!inside(list_box_, e.button.x, e.button.y)) { list_.clear(); return; }
            for (const auto& [r, i] : list_rects_)
                if (inside(r, e.button.x, e.button.y)) { choose_from_list(i); return; }
        }
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        for (const auto& [r, i] : item_rects_)
            if (inside(r, e.motion.x, e.motion.y)) row_ = i;
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        for (const auto& [r, i] : item_rects_)
            if (inside(r, e.button.x, e.button.y))
            {
                row_ = i;
                change(e.button.button == SDL_BUTTON_RIGHT ? -1 : 1);
            }
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    switch (e.key.key)
    {
    case SDLK_UP: row_ = (row_ + n - 1) % n; break;
    case SDLK_DOWN: row_ = (row_ + 1) % n; break;
    case SDLK_LEFT: change(-1); break;
    case SDLK_RIGHT:
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE: change(1); break;
    case SDLK_ESCAPE:
    case SDLK_BACKSPACE: back(); break;
    default: break;
    }
}

MainMenu::Result MainMenu::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    done_ = false;
    while (!done_)
    {
        SDL_Event e;
        // Wait for input (up to a frame), so the menu does not spin a core when VSync is off.
        if (SDL_WaitEventTimeout(&e, 16))
        {
            handle(e);
            while (SDL_PollEvent(&e)) handle(e);
        }
        render();
        SDL_RenderPresent(r_);
    }
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void MainMenu::render()
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h), cx = W / 2;
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    // Title, scaled down if the language makes it too wide.
    const std::string& title = tr("main.title");
    float ts = 3;
    while (ts > 1 && text_width(title, ts) > W - 40) ts -= 0.5f;
    draw_centred(r_, cx, 50, title, ts, kWhite);
    draw_centred(r_, cx, 50 + 8 * ts + 14, tr("main.subtitle"), 1, kGrey);
    // A thin rule under the title in the colours of the five lines.
    {
        const SDL_Color bands[5] = {{255, 255, 255, 255}, {0, 255, 255, 255}, {255, 176, 0, 255}, {255, 255, 0, 255}, {150, 150, 150, 255}};
        const float rw = std::min(W - 80, 700.0f), x0 = cx - rw / 2, y = 50 + 8 * ts + 36;
        for (int i = 0; i < 5; ++i)
        {
            SDL_SetRenderDrawColor(r_, bands[i].r, bands[i].g, bands[i].b, 255);
            const SDL_FRect band{x0 + rw * float(i) / 5, y, rw / 5 - 4, 3};
            SDL_RenderFillRect(r_, &band);
        }
    }

    // Where you are.
    const char* heading = screen_ == Screen::Settings   ? "screen.settings"
                          : screen_ == Screen::Graphics ? "screen.graphics"
                          : screen_ == Screen::Controls ? "screen.controls"
                          : screen_ == Screen::Language ? "screen.language"
                                                        : nullptr;
    // Everything below the title (heading, items, help, the key list) is one block, centred in
    // the space between the title and the footer; if it is taller than that, it starts at the top.
    const auto its = items();
    const bool pairs = screen_ == Screen::Graphics || screen_ == Screen::Controls || screen_ == Screen::Language;
    const float rowh = screen_ == Screen::Main ? 44 : 36, boxw = std::min(W - 60, 760.0f);
    size_t help_lines = 0, key_lines = 0;
    if (row_ >= 0 && row_ < int(its.size())) help_lines = split_lines(tr("help." + its[size_t(row_)].id)).size();
    if (screen_ == Screen::Controls) key_lines = split_lines(tr("controls.keys")).size();
    bool has_back = false;
    for (const auto& it : its) has_back = has_back || it.id == "back";
    // The help block is sized for the tallest item's help, so the block does not jump as you move.
    for (const auto& it : its) help_lines = std::max(help_lines, split_lines(tr("help." + it.id)).size());
    const float block = (heading ? 40.0f : 0.0f) + float(its.size()) * rowh + (has_back ? 12.0f : 0.0f) + 16 + float(help_lines) * 14 +
                        (key_lines ? 16 + 28 + float(key_lines) * 14 : 0.0f);
    const float top = 50 + 8 * ts + 60, bottom = H - 50;
    float y = std::max(top, top + (bottom - top - block) / 2);
    if (heading)
    {
        draw_centred(r_, cx, y, tr(heading), 2, kAccent);
        y += 40;
    }

    // Items: on the main screen, centred; on settings screens, label left and value right.
    item_rects_.clear();
    float item_y = y;
    SDL_FRect list_anchor{};
    for (int i = 0; i < int(its.size()); ++i)
    {
        const Item& it = its[size_t(i)];
        const bool sel = i == row_;
        const float yy = y + float(i) * rowh + (it.id == "back" ? 12 : 0);
        const SDL_FRect r{cx - boxw / 2, yy - 8, boxw, rowh - 6};
        item_rects_.emplace_back(r, i);
        if (sel)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 36);
            SDL_RenderFillRect(r_, &r);
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 160);
            SDL_RenderRect(r_, &r);
        }
        const std::string label = tr("item." + it.id);
        const float scale = screen_ == Screen::Main ? 3 : 2;
        if (!pairs || it.kind == Kind::Action) draw_centred(r_, cx, yy + (screen_ == Screen::Main ? -2 : 2), label, scale, sel ? kWhite : kGrey);
        else
        {
            draw_text(r_, r.x + 20, yy + 2, label, 2, sel ? kWhite : kGrey);
            std::string v = value_of(it);
            if (it.kind == Kind::Choice) v += "  v";
            if (it.kind == Kind::Number) v = "< " + v + " >";
            draw_text(r_, r.x + r.w - 20 - text_width(v, 2), yy + 2, v, 2, sel ? kAccent : kGrey);
            if (it.id == list_) list_anchor = r;
        }
        item_y = yy + rowh;
    }

    // What the selected item does.
    if (row_ >= 0 && row_ < int(its.size()))
    {
        const std::string help = tr("help." + its[size_t(row_)].id);
        float hy = item_y + 16;
        for (const auto& line : split_lines(help)) { draw_centred(r_, cx, hy, line, 1, kGrey); hy += 14; }
        item_y = hy;
    }

    // Controls: the keys, from the language file.
    if (screen_ == Screen::Controls)
    {
        float ky = item_y + 16;
        draw_centred(r_, cx, ky, tr("controls.keys.title"), 2, kAccent);
        ky += 28;
        const float kx = cx - boxw / 2 + 20;
        for (const auto& line : split_lines(tr("controls.keys")))
        {
            if (ky > H - 60) break;
            const size_t bar = line.find('|');
            if (bar == std::string::npos) draw_text(r_, kx, ky, line, 1, kGrey);
            else
            {
                draw_text(r_, kx, ky, line.substr(0, bar), 1, kWhite);
                draw_text(r_, kx + 200, ky, line.substr(bar + 1), 1, kGrey);
            }
            ky += 14;
        }
    }

    // Footer.
    draw_centred(r_, cx, H - 30, tr(list_.empty() ? "menu.footer" : "menu.footer.list"), 1, kDim);

    // An open drop-down list, over everything.
    list_rects_.clear();
    if (!list_.empty())
    {
        const auto labels = list_labels();
        const int shown = std::min<int>(kListRows, int(labels.size()));
        const float lh = 24, lw = std::max(260.0f, list_anchor.w / 2);
        float lx = list_anchor.x + list_anchor.w - lw, ly = list_anchor.y + list_anchor.h + 2;
        if (ly + lh * float(shown) + 8 > H) ly = std::max(4.0f, list_anchor.y - lh * float(shown) - 10);
        list_box_ = {lx, ly, lw, lh * float(shown) + 8};
        SDL_SetRenderDrawColor(r_, 10, 10, 10, 250);
        SDL_RenderFillRect(r_, &list_box_);
        SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
        SDL_RenderRect(r_, &list_box_);
        for (int k = 0; k < shown; ++k)
        {
            const int i = list_top_ + k;
            if (i >= int(labels.size())) break;
            const SDL_FRect r{lx + 2, ly + 4 + lh * float(k), lw - 4, lh};
            list_rects_.emplace_back(r, i);
            if (i == list_row_)
            {
                SDL_SetRenderDrawColor(r_, 255, 255, 255, 50);
                SDL_RenderFillRect(r_, &r);
            }
            draw_text(r_, r.x + 10, r.y + 4, labels[size_t(i)], 2, i == list_row_ ? kWhite : kGrey);
        }
        if (list_top_ > 0) draw_text(r_, lx + lw - 20, ly + 6, "^", 2, kGrey);
        if (list_top_ + shown < int(labels.size())) draw_text(r_, lx + lw - 20, ly + lh * float(shown) - 14, "v", 2, kGrey);
    }
}

} // namespace hallway
