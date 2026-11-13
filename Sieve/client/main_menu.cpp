// Sieve hallway — the main menu's behaviour: the rows, the drop-down lists, and applying a change
// the moment it is made. See main_menu.hpp for the keys and the mouse.
//
// The rows are built fresh by items() each time they are needed rather than kept in a table,
// because which rows exist depends on which screen you are on and what the display can do; a
// stale table was the alternative, and this menu is drawn rarely enough that rebuilding it costs
// nothing. Every change is applied and saved at once, with no confirm step, so that a setting that
// makes the window unusable can be undone by changing it back rather than by editing a file.
#include "main_menu.hpp"
#include "cli/media_decode.hpp"
#include "cli/timings.hpp"
#include "cli/locate.hpp" // read_file_bytes, sha256_hex (the replay viewer's cache boxes)
#include "dialogs.hpp"
#include "speech.hpp"

#include "dimensions.hpp"
#include "menu.hpp" // line_sizes_of
#include "font.hpp"
#include "music.hpp"
#include "raid_service.hpp"
#include "sieve/paths.hpp"
#include "strings.hpp"

#include <algorithm>
#include <cmath>

namespace hallway {

namespace {

constexpr int kMinW = 1000, kMinH = 640; // smaller windows draw the menu scaled down
constexpr int kListRows = 10;            // drop-down entries shown at once
const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kAccent{0, 255, 255, 255},
                kRed{255, 70, 70, 255};

// A value cut to fit `px` at double size (8 px a character, twice), "..." where it was cut.
std::string fit_value(const std::string& v, float px)
{
    const size_t cells = size_t(std::max(4.0f, px / 16.0f));
    return v.size() <= cells ? v : v.substr(0, cells - 3) + "...";
}

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
    // The same systems as the pause menu, in the same order (IDEAS §19.18), less what needs a
    // hallway already walked (the node graph, the navigator).
    case Screen::Main:
        // (Manual Navigation after the rest, before Exit: the CI's walks through this menu count rows.)
        return {{Kind::Action, "start"},    {Kind::Action, "locator"}, {Kind::Action, "media"}, {Kind::Action, "designer"},
                {Kind::Action, "harness"}, {Kind::Action, "settings"}, {Kind::Action, "cipher"}, {Kind::Action, "manual_nav"}, {Kind::Action, "personal"}, {Kind::Action, "systems"}, {Kind::Action, "creator"}, {Kind::Action, "replays"}, {Kind::Action, "exit"}};
    // Personal Dimensions (IDEAS 19.152, 19.157): yours, built; other people's and group dimensions to come.
    case Screen::Personal: return {{Kind::Action, "personal_one"}, {Kind::Action, "personal_others"}, {Kind::Action, "personal_groups"}, {Kind::Action, "back"}};
    // The System Viewer (IDEAS 19.182, 19.183): the systems charted so far, each opened in one viewer.
    case Screen::Systems: return {{Kind::Action, "systems_formats"}, {Kind::Action, "systems_language"}, {Kind::Action, "back"}};
    case Screen::Settings:
        return {{Kind::Action, "graphics"}, {Kind::Action, "controls"}, {Kind::Action, "language"}, {Kind::Action, "programs"},
                {Kind::Action, "raid"}, {Kind::Action, "back"}};
    case Screen::Graphics:
        return {{Kind::Choice, "resolution"}, {Kind::Toggle, "fullscreen"}, {Kind::Toggle, "vsync"}, {Kind::Toggle, "edge_glow"}, {Kind::Toggle, "real_graphics"}, {Kind::Toggle, "door_portals"}, {Kind::Number, "graphics_memory"}, {Kind::Toggle, "fps_counter"},
                {Kind::Number, "view_rooms"}, {Kind::Number, "picture_rooms"},
                {Kind::Action, "back"}};
    case Screen::Controls: return {{Kind::Number, "mouse_sensitivity"}, {Kind::Toggle, "invert_mouse_y"}, {Kind::Toggle, "read_aloud"}, {Kind::Toggle, "text_to_voice"}, {Kind::Action, "back"}};
    case Screen::Language: return {{Kind::Choice, "language_choice"}, {Kind::Action, "back"}};
    case Screen::Programs: return {{Kind::Program, "ffmpeg"}, {Kind::Action, "back"}};
    // The replay viewer (IDEAS 19.184): one row a recording, its box for caching it; then Back.
    case Screen::Replays:
    {
        std::vector<Item> out;
        for (size_t i = 0; i < replays_.size(); ++i) out.push_back({Kind::Replay, "replay:" + std::to_string(i)});
        out.push_back({Kind::Action, "back"});
        return out;
    }
    // The infinite RAID between machines (IDEAS 19.176, 19.150): both halves opt in only, each its own
    // red box; the machines and the address are set in the settings file and shown here.
    case Screen::Raid:
        return {{Kind::Toggle, "raid_share"}, {Kind::Info, "raid_peers"},  {Kind::Number, "raid_copies"}, {Kind::Action, "raid_check"},
                {Kind::Toggle, "raid_serve"}, {Kind::Info, "raid_listen"}, {Kind::Action, "back"}};
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
    if (it.id == "door_portals") return onoff(s_.door_portals);
    if (it.id == "graphics_memory") return trf("value.graphics_memory", {std::to_string(s_.graphics_memory_gb)});
    if (it.id == "fps_counter") return onoff(s_.fps_counter);
    if (it.id == "view_rooms") return trf("value.view_rooms", {std::to_string(s_.view_rooms)});
    if (it.id == "picture_rooms") return trf("value.picture_rooms", {std::to_string(s_.picture_rooms)});
    if (it.id == "mouse_sensitivity") return std::to_string(s_.mouse_sensitivity) + "%";
    if (it.id == "invert_mouse_y") return onoff(s_.invert_mouse_y);
    if (it.id == "read_aloud") return speech_available() ? onoff(s_.read_aloud) : tr("value.read_aloud.none");
    if (it.id == "text_to_voice") return speech_available() || sieve_voice_available() ? onoff(s_.text_to_voice) : tr("value.text_to_voice.none");
    if (it.kind == Kind::Replay)
    {
        // Its size, and its box: ticked, decoded once into the cache (a packed recording alone).
        const size_t i = size_t(std::stoul(it.id.substr(7)));
        std::error_code ec;
        const uint64_t bytes = std::filesystem::file_size(replays_[i], ec);
        const std::string size = bytes >= 10240 ? std::to_string(bytes / 1024) + " KiB" : std::to_string(bytes) + " bytes";
        if (replays_[i].extension() != ".sieve") return size + "  " + tr("value.replay.plain");
        return size + "  [" + (replay_cached(i) ? "x" : " ") + "] " + tr("value.replay.cache");
    }
    if (it.id == "raid_share") return onoff(s_.raid_share);
    if (it.id == "raid_serve") return onoff(s_.raid_serve);
    if (it.id == "raid_copies") return trf("value.raid_copies", {std::to_string(s_.raid_copies)});
    if (it.id == "raid_peers") return s_.raid_peers.empty() ? tr("value.raid_peers.none") : s_.raid_peers;
    if (it.id == "raid_listen") return s_.raid_serve ? hallway::raid_service::serving_status() : s_.raid_listen;
    if (it.id == "ffmpeg") return ffmpeg_version_.empty() ? tr("value.ffmpeg.none") : ffmpeg_version_;
    if (it.id == "language_choice")
    {
        for (const auto& l : langs_)
            if (l.code == language_code()) return l.name;
        return language_code();
    }
    return "";
}

// The line of help under an item: "help.<id>", one for every recording's row.
std::string MainMenu::help_key(const Item& it) { return it.kind == Kind::Replay ? "help.replay_row" : "help." + it.id; }

// The recordings in `replays` beside the settings file, plain (.replay) or packed (.sieve), by name.
void MainMenu::read_replays()
{
    replays_.clear();
    std::error_code ec;
    for (const auto& e : std::filesystem::directory_iterator(path_.parent_path() / "replays", ec))
        if (e.is_regular_file() && (e.path().extension() == ".replay" || e.path().extension() == ".sieve")) replays_.push_back(e.path());
    std::sort(replays_.begin(), replays_.end());
    if (replays_.size() > 200) replays_.resize(200);
}

bool MainMenu::replay_cached(size_t i) const
{
    if (i >= replays_.size() || replays_[i].extension() != ".sieve" || s_.replays_cached.empty()) return false;
    const std::string sha = sieve::cli::sha256_hex(sieve::cli::read_file_bytes(replays_[i]));
    return s_.replays_cached.find(sha) != std::string::npos;
}

// Its box ticked or not: its SHA-256 put in [replays] cached, or taken out, and saved.
void MainMenu::toggle_replay_cache(size_t i)
{
    if (i >= replays_.size() || replays_[i].extension() != ".sieve") return;
    const std::string sha = sieve::cli::sha256_hex(sieve::cli::read_file_bytes(replays_[i]));
    std::vector<std::string> list;
    for (size_t at = 0; at <= s_.replays_cached.size();)
    {
        const size_t c = std::min(s_.replays_cached.find(',', at), s_.replays_cached.size());
        if (c > at) list.push_back(s_.replays_cached.substr(at, c - at));
        at = c + 1;
    }
    const auto f = std::find(list.begin(), list.end(), sha);
    if (f == list.end()) list.push_back(sha);
    else list.erase(f);
    std::string v;
    for (const auto& x : list) v += (v.empty() ? "" : ",") + x;
    s_.replays_cached = v;
    save();
}

void MainMenu::save()
{
    s_.save(path_); // a read-only folder still keeps the choices for this session
}

void MainMenu::refresh_ffmpeg()
{
    namespace cli = sieve::cli;
    ffmpeg_version_ = cli::ffmpeg_version();
    const std::optional<std::string> exe = cli::find_ffmpeg();
    if (!ffmpeg_version_.empty()) ffmpeg_where_ = trf(s_.ffmpeg.empty() ? "value.ffmpeg.found" : "value.ffmpeg.chosen", {*exe});
    else if (!s_.ffmpeg.empty()) ffmpeg_where_ = trf("value.ffmpeg.bad", {s_.ffmpeg});
    else if (exe) ffmpeg_where_ = trf("value.ffmpeg.bad", {*exe}); // found by its name, and it does not run
    else ffmpeg_where_ = cli::ffmpeg_looked();
}

namespace {

void SDLCALL ffmpeg_file_chosen(void* user, const char* const* files, int)
{
    if (files && files[0]) static_cast<MainMenu*>(user)->ffmpeg_chosen(files[0]);
}

} // namespace

void MainMenu::ffmpeg_chosen(const std::string& path)
{
    std::lock_guard<std::mutex> lock(chosen_mx_);
    ffmpeg_chosen_ = path;
}

void MainMenu::poll_ffmpeg_chosen()
{
    std::optional<std::string> chosen;
    {
        std::lock_guard<std::mutex> lock(chosen_mx_);
        chosen.swap(ffmpeg_chosen_);
    }
    if (!chosen) return;
    s_.ffmpeg = *chosen;
    sieve::cli::set_ffmpeg_path(s_.ffmpeg); // for the rest of this run too, over --ffmpeg: it was chosen after
    save();
    refresh_ffmpeg();
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
    case Screen::Main: row_ = int(items().size()) - 1; return; // Esc on the main screen points at Exit (the last) rather than quitting
    case Screen::Settings:
        if (settings_only_) { result_ = Result::Start; done_ = true; return; } // back to the hallway
        screen_ = Screen::Main;
        // (back on Settings' own row, wherever it is in the list)
        row_ = 0;
        for (const Item& it : items())
            if (it.id == "settings") break;
            else ++row_;
        return;
    case Screen::Graphics: screen_ = Screen::Settings; row_ = 0; return;
    case Screen::Controls: screen_ = Screen::Settings; row_ = 1; return;
    case Screen::Language: screen_ = Screen::Settings; row_ = 2; return;
    case Screen::Programs: screen_ = Screen::Settings; row_ = 3; return;
    case Screen::Raid: screen_ = Screen::Settings; row_ = 4; return;
    case Screen::Replays:
        screen_ = Screen::Main;
        row_ = 0;
        for (const Item& it : items())
            if (it.id == "replays") break;
            else ++row_;
        return;
    case Screen::Personal:
        screen_ = Screen::Main;
        row_ = 0;
        for (const Item& it : items())
            if (it.id == "personal") break;
            else ++row_;
        return;
    case Screen::Systems:
        screen_ = Screen::Main;
        row_ = 0;
        for (const Item& it : items())
            if (it.id == "systems") break;
            else ++row_;
        return;
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
        else if (it.id == "locator") { result_ = Result::Locator; done_ = true; }
        else if (it.id == "designer") { result_ = Result::Designer; done_ = true; }
        else if (it.id == "harness") { result_ = Result::Harness; done_ = true; }
        else if (it.id == "media") { result_ = Result::Media; done_ = true; }
        else if (it.id == "cipher") { result_ = Result::Cipher; done_ = true; }
        else if (it.id == "manual_nav") { result_ = Result::ManualNav; done_ = true; }
        else if (it.id == "personal") { screen_ = Screen::Personal; row_ = 0; }
        else if (it.id == "personal_one") { result_ = Result::Personal; done_ = true; }
        else if (it.id == "systems") { screen_ = Screen::Systems; row_ = 0; }
        else if (it.id == "creator") { result_ = Result::Creator; done_ = true; } // the character creator (IDEAS 19.186)
        else if (it.id == "replays") { read_replays(); screen_ = Screen::Replays; row_ = 0; }
        else if (it.id == "systems_formats") { system_root_ = "information"; result_ = Result::Systems; done_ = true; }
        else if (it.id == "systems_language") { system_root_ = "language"; result_ = Result::Systems; done_ = true; }
        else if (it.id == "personal_others" || it.id == "personal_groups") {} // (not built yet: its help line says so)
        else if (it.id == "exit") { result_ = Result::Quit; done_ = true; }
        else if (it.id == "settings") { screen_ = Screen::Settings; row_ = 0; }
        else if (it.id == "graphics") { screen_ = Screen::Graphics; row_ = 0; }
        else if (it.id == "controls") { screen_ = Screen::Controls; row_ = 0; }
        else if (it.id == "language") { screen_ = Screen::Language; row_ = 0; }
        else if (it.id == "raid") { screen_ = Screen::Raid; row_ = 0; }
        else if (it.id == "raid_check")
        {
            // Check my shares (IDEAS 19.176): on a thread; what came of it said below the rows.
            hallway::raid_service::check(s_, path_);
            if (auto n = hallway::raid_service::take_check_note()) raid_note_ = *n;
        }
        else if (it.id == "programs")
        {
            screen_ = Screen::Programs;
            row_ = 0;
            refresh_ffmpeg();
        }
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
        else if (it.id == "door_portals") s_.door_portals = !s_.door_portals;
        else if (it.id == "fps_counter") s_.fps_counter = !s_.fps_counter;
        else if (it.id == "invert_mouse_y") s_.invert_mouse_y = !s_.invert_mouse_y;
        else if (it.id == "read_aloud")
        {
            s_.read_aloud = !s_.read_aloud;
            set_read_aloud(s_.read_aloud);
        }
        else if (it.id == "text_to_voice")
        {
            s_.text_to_voice = !s_.text_to_voice;
            set_text_to_voice(s_.text_to_voice);
        }
        else if (it.id == "raid_share")
        {
            // Opt in: turned on, your key is made if there is none (said, with where), so what is sent
            // can be read by you alone.
            s_.raid_share = !s_.raid_share;
            raid_note_.clear();
            if (s_.raid_share)
            {
                bool made = false;
                const std::string why = hallway::raid_service::ensure_key(s_, path_, &made);
                if (!why.empty())
                {
                    s_.raid_share = false;
                    raid_note_ = trf("value.raid_key.failed", {why});
                }
                else if (made) raid_note_ = trf("value.raid_key.made", {sieve::u8(hallway::raid_service::key_path(s_, path_))});
            }
        }
        else if (it.id == "raid_serve")
        {
            s_.raid_serve = !s_.raid_serve;
            save();
            hallway::raid_service::apply(s_, path_); // started or stopped now
        }
        save();
        return;
    case Kind::Number:
        if (it.id == "graphics_memory") s_.graphics_memory_gb = std::clamp(s_.graphics_memory_gb + dir, 1, 128);
        else if (it.id == "view_rooms") s_.view_rooms = std::clamp(s_.view_rooms + dir, 2, 64);
        else if (it.id == "picture_rooms") s_.picture_rooms = std::clamp(s_.picture_rooms + dir, 0, 8);
        else if (it.id == "raid_copies") s_.raid_copies = std::clamp(s_.raid_copies + dir, 1, 16);
        else s_.mouse_sensitivity = std::clamp(s_.mouse_sensitivity + dir * 10, 10, 400);
        save();
        return;
    case Kind::Choice: open_list(it); return;
    case Kind::Info: return;
    case Kind::Replay: toggle_replay_cache(size_t(std::stoul(it.id.substr(7)))); return;
    case Kind::Program:
        if (dir > 0)
        {
#ifdef _WIN32
            static const SDL_DialogFileFilter programs[] = {{"Programs", "exe"}, {"All files", "*"}};
            open_file_dialog(ffmpeg_file_chosen, this, window_, programs, 2, nullptr, false, false, this);
#else
            open_file_dialog(ffmpeg_file_chosen, this, window_, nullptr, 0, nullptr, false, false, this);
#endif
            return;
        }
        // Left: back to looking for one (SIEVE_FFMPEG, beside Sieve, the PATH).
        s_.ffmpeg.clear();
        sieve::cli::set_ffmpeg_path("");
        save();
        refresh_ffmpeg();
        return;
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
    // A recording's row: Enter plays it; Space, Left and Right tick its box (below).
    const auto its = items();
    if ((e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER) && row_ >= 0 && row_ < n && its[size_t(row_)].kind == Kind::Replay)
    {
        replay_chosen_ = replays_[size_t(std::stoul(its[size_t(row_)].id.substr(7)))];
        result_ = Result::Replay;
        done_ = true;
        return;
    }
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
    music_mode(MusicMode::Menus);
    music_colours_default();
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
        poll_ffmpeg_chosen();
        {
            sieve::cli::timings::Scope timed("menu.main.frame");
            render();
        }
        present(r_);
    }
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void MainMenu::render()
{
    if (int(screen_) != spoken_screen_)
    {
        forget_announced();
        spoken_screen_ = int(screen_);
    }
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
    // The subtitle names every line, so on a narrow window it takes two lines, broken at the space
    // nearest its middle (there is room for two above the rule below).
    {
        const std::string& sub = tr("main.subtitle");
        const float sy = 50 + 8 * ts + 14;
        const size_t mid = sub.size() / 2, before = sub.rfind(' ', mid), after = sub.find(' ', mid);
        const size_t cut = before == std::string::npos ? after : after == std::string::npos ? before
                                                                 : (mid - before <= after - mid ? before : after);
        if (text_width(sub, 1) <= W - 40 || cut == std::string::npos) draw_centred(r_, cx, sy, sub, 1, kGrey);
        else
        {
            draw_centred(r_, cx, sy - 4, sub.substr(0, cut), 1, kGrey);
            draw_centred(r_, cx, sy + 8, sub.substr(cut + 1), 1, kGrey);
        }
    }
    // A thin rule under the title in the colours of the lines, as the corridor runs: binary at
    // both ends, the others in door order between.
    {
        const float rw = std::min(W - 80, 700.0f), x0 = cx - rw / 2, y = 50 + 8 * ts + 36;
        constexpr int bands = kLines + 1;
        for (int i = 0; i < bands; ++i)
        {
            const SDL_Color c = menu_ink(theme_of(i == 0 ? kBinaryLine : i - 1));
            SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
            const SDL_FRect band{x0 + rw * float(i) / bands, y, rw / bands - 4, 3};
            SDL_RenderFillRect(r_, &band);
        }
    }

    // Where you are.
    const char* heading = screen_ == Screen::Settings   ? "screen.settings"
                          : screen_ == Screen::Graphics ? "screen.graphics"
                          : screen_ == Screen::Controls ? "screen.controls"
                          : screen_ == Screen::Language ? "screen.language"
                          : screen_ == Screen::Personal ? "screen.personal"
                          : screen_ == Screen::Systems ? "screen.systems"
                          : screen_ == Screen::Programs ? "screen.programs"
                          : screen_ == Screen::Raid ? "screen.raid"
                          : screen_ == Screen::Replays ? "screen.replays"
                                                        : nullptr;
    // Everything below the title (heading, items, help, the key list) is one block, centred in
    // the space between the title and the footer; if it is taller than that, it starts at the top.
    const auto its = items();
    const bool pairs = screen_ == Screen::Graphics || screen_ == Screen::Controls || screen_ == Screen::Language || screen_ == Screen::Programs || screen_ == Screen::Raid ||
                       screen_ == Screen::Replays;
    const float rowh = screen_ == Screen::Main ? 44.0f : 36.0f, boxw = std::min(W - 60, 760.0f);
    size_t help_lines = 0, key_lines = 0;
    if (row_ >= 0 && row_ < int(its.size())) help_lines = split_lines(tr(help_key(its[size_t(row_)]))).size();
    if (screen_ == Screen::Controls) key_lines = split_lines(tr("controls.keys")).size();
    bool has_back = false;
    for (const auto& it : its) has_back = has_back || it.id == "back";
    // The help block is sized for the tallest item's help, so the block does not jump as you move.
    for (const auto& it : its) help_lines = std::max(help_lines, split_lines(tr(help_key(it))).size());
    const float block = (heading ? 40.0f : 0.0f) + float(its.size()) * rowh + (has_back ? 12.0f : 0.0f) + 16 + float(help_lines) * 14 +
                        (key_lines ? 16 + 28 + float(key_lines) * 14 : 0.0f);
    float top = 50 + 8 * ts + 60;
    const float bottom = H - 50;
    // (The main screen's completion estimate, each line's walk time, was drawn here until 9 November
    // 2026; it is now the Start Sieve map's, with the other figures of each line: Edward, "they
    // overcomplicate the main menu".)
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
        // The RAID's two opt-ins: a red box always (IDEAS 19.150: "strongly labelled and opt in only
        // red warning type data checkboxes"), filled red while on.
        const bool red = it.id == "raid_share" || it.id == "raid_serve";
        if (red)
        {
            const bool on = it.id == "raid_share" ? s_.raid_share : s_.raid_serve;
            SDL_SetRenderDrawColor(r_, 200, 30, 30, on ? 90 : 30);
            SDL_RenderFillRect(r_, &r);
            SDL_SetRenderDrawColor(r_, 230, 50, 50, 255);
            SDL_RenderRect(r_, &r);
            const SDL_FRect r2{r.x - 2, r.y - 2, r.w + 4, r.h + 4};
            SDL_RenderRect(r_, &r2);
        }
        if (sel)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 36);
            SDL_RenderFillRect(r_, &r);
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 160);
            SDL_RenderRect(r_, &r);
        }
        const std::string label = it.kind == Kind::Replay ? sieve::u8(replays_[size_t(std::stoul(it.id.substr(7)))].filename()) : tr("item." + it.id);
        const float scale = screen_ == Screen::Main ? 3.0f : 2.0f;
        if (!pairs || it.kind == Kind::Action) draw_centred(r_, cx, yy + (screen_ == Screen::Main ? -2 : 2), label, scale, sel ? kWhite : kGrey);
        else
        {
            draw_text(r_, r.x + 20, yy + 2, label, 2, sel ? kWhite : kGrey);
            std::string v = value_of(it);
            if (it.kind == Kind::Choice) v += "  v";
            if (it.kind == Kind::Number) v = "< " + v + " >";
            // A program that is not there is said in red, as every warning that stops something is.
            const bool missing = it.kind == Kind::Program && ffmpeg_version_.empty();
            draw_text(r_, r.x + r.w - 20 - text_width(fit_value(v, r.w / 2), 2), yy + 2, fit_value(v, r.w / 2), 2, missing || red ? kRed : sel ? kAccent : kGrey);
            if (it.id == list_) list_anchor = r;
        }
        item_y = yy + rowh;
        // Read aloud when chosen (speech.hpp): the item, and its value on a settings screen.
        if (sel && read_aloud()) announce(pairs && it.kind != Kind::Action ? label + ", " + value_of(it) : label);
    }

    // What the selected item does.
    if (row_ >= 0 && row_ < int(its.size()))
    {
        const std::string help = tr(help_key(its[size_t(row_)]));
        float hy = item_y + 16;
        // (the RAID's opt-ins' help in red: what ticking them does, before it is done)
        const bool red_help = its[size_t(row_)].id == "raid_share" || its[size_t(row_)].id == "raid_serve";
        for (const auto& line : split_lines(help)) { draw_centred(r_, cx, hy, line, 1, red_help ? kRed : kGrey); hy += 14; }
        if (screen_ == Screen::Raid)
            if (auto n = hallway::raid_service::take_check_note()) raid_note_ = *n; // (a check finishing)
        if (screen_ == Screen::Raid && !raid_note_.empty())
        {
            hy += 6;
            draw_centred(r_, cx, hy, raid_note_, 1, kWhite);
            hy += 14;
        }
        // The ffmpeg row: which one is used, or where none was found (in red).
        if (its[size_t(row_)].kind == Kind::Program)
        {
            // Broken at spaces to the box's width: a path, or the list of places looked in, is long.
            hy += 6;
            std::string rest = ffmpeg_where_;
            while (!rest.empty())
            {
                size_t cut = rest.size();
                while (cut > 0 && text_width(rest.substr(0, cut), 1) > boxw - 20)
                {
                    const size_t sp = rest.rfind(' ', cut - 1);
                    cut = sp == std::string::npos || sp == 0 ? cut - 1 : sp;
                }
                if (cut == 0) cut = rest.size();
                draw_centred(r_, cx, hy, rest.substr(0, cut), 1, ffmpeg_version_.empty() ? kRed : kWhite);
                hy += 14;
                rest = cut < rest.size() ? rest.substr(cut + (rest[cut] == ' ' ? 1 : 0)) : std::string();
            }
            hy += 6;
        }
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
