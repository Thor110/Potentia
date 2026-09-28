// The Media Player, from the pause menu: the background music's settings, the tracks it has
// played and the ones kept (music.hpp has the player itself).
//
// Three columns, as the filter designer has: on the left the last ten tracks the player chose,
// each with the mode it played in (MENUS or WORLD) and when; in the middle the controls; on the
// right the favourites. The controls show one mode's settings at a time (MODE at the top switches
// between MENUS and WORLD, which have one form and two sets of values): the music on or off, its
// volume, the tracks' length, tempo, voice, echo and the quiet between them, a new track now, and
// the mode's own stack of audio filters with their settings, apart from the setup menu's. Below
// them the actions, which act on the highlighted track of whichever list was used last: play it,
// go to it (on the audio line, in hand; a track of another length than this hallway's audio line
// builds the hallway again at that length), save it as MIDI, and add it to the favourites or take
// it off them.
//
// Keys: Tab moves between the columns, Up and Down along one, Left and Right change a setting
// (Shift: ten steps), ENTER plays a track or presses a button, G goes to the track, F saves it,
// V adds it to the favourites (or takes it off, in that list), N plays a new track now, Esc
// goes back to the pause menu.

#include "hallway.hpp"

#include <algorithm>

namespace hallway::hall {

namespace {

constexpr float kRowH = 18;

void SDLCALL media_save_chosen(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return; // cancelled
    static_cast<Hallway*>(user)->media_saved(files[0]);
}

std::string mode_name(MusicMode m) { return tr(m == MusicMode::Menus ? "music.mode.menus" : "music.mode.world"); }

std::string track_line(const MusicTrack& t)
{
    return (t.when.empty() ? "--:--" : t.when) + "  " + t.where() + "  " + t.address().substr(0, 10) + "  " + t.notation();
}

} // namespace

void Hallway::open_media_player()
{
    media_open_ = true;
    media_area_ = 1;
    media_row_ = 0;
    media_top_ = 0;
    media_mode_ = MusicMode::World;
    {
        std::lock_guard<std::mutex> lock(media_mx_);
        media_status_ = music() ? music()->status() : tr("media.no_player");
    }
    media_had_mouse_ = SDL_GetWindowRelativeMouseMode(window_);
    SDL_SetWindowRelativeMouseMode(window_, false);
}

void Hallway::close_media_player()
{
    media_open_ = false;
    SDL_SetWindowRelativeMouseMode(window_, media_had_mouse_);
}

std::vector<Hallway::MediaRow> Hallway::media_rows() const
{
    using K = MediaRow::Kind;
    std::vector<MediaRow> rows;
    for (K k : {K::Mode, K::On, K::Volume, K::Length, K::Tempo, K::Voice, K::Echo, K::Gap}) rows.push_back({k, {}, {}, -1});
    // WORLD's character: a mode for each line, and the key round the circle of fifths.
    if (media_mode_ == MusicMode::World)
    {
        rows.push_back({K::Character, {}, {}, -1});
        for (int li = 0; li < 7; ++li) rows.push_back({K::LineMode, {}, {}, li});
        rows.push_back({K::Fifths, {}, {}, -1});
    }
    for (K k : {K::Next, K::Filters}) rows.push_back({k, {}, {}, -1});
    if (MusicPlayer* p = music())
    {
        const MusicSettings s = p->settings(media_mode_);
        sieve::FilterLine fl{"audio", sieve::kNotesSymbolsId, sieve::kNoteSymbols, s.length, nullptr, 0, 0, 0};
        for (const sieve::FilterSpec* f : sieve::filters_for(fl))
        {
            rows.push_back({K::Filter, f->name(), {}, -1});
            if (s.filters.is_enabled(f->name()))
                for (const auto& prm : f->params) rows.push_back({K::Param, f->name(), prm.key, -1});
        }
    }
    for (K k : {K::Play, K::GoTo, K::Save, K::Favourite}) rows.push_back({k, {}, {}, -1});
    return rows;
}

std::optional<MusicTrack> Hallway::media_selected() const
{
    if (!music()) return std::nullopt;
    const auto list = media_list_ == 2 ? music()->favourites() : music()->recent();
    const int i = media_sel_[media_list_];
    if (i < 0 || i >= int(list.size())) return std::nullopt;
    return list[size_t(i)];
}

void Hallway::media_saved(const std::string& path)
{
    std::vector<uint32_t> notes;
    {
        std::lock_guard<std::mutex> lock(media_mx_);
        notes = media_saving_;
    }
    std::string done;
    try
    {
        cli::Args la;
        la.opts["line"] = "audio";
        la.opts["length"] = std::to_string(std::max<size_t>(1, notes.size()));
        cli::save_unit(cli::make_line(la), notes, path, 16); // refuses what the vault holds
        done = trf("hand.saved", {path});
    }
    catch (const std::exception& e)
    {
        done = trf("hand.save_failed", {e.what()});
    }
    std::lock_guard<std::mutex> lock(media_mx_);
    media_status_ = done;
}

// Walk to a track: the audio line at the track's slot, the track in hand. Only a hallway whose
// audio line is the track's length has it; otherwise the application builds one that does.
void Hallway::go_to_track(const std::vector<uint32_t>& notes)
{
    if (notes.empty()) return;
    if (lines_[2].space.unit_length() != notes.size())
    {
        track_to_go_ = notes;
        request_ = Request::GoToTrack;
        return;
    }
    if (media_open_) close_media_player();
    if (pause_open_) close_pause();
    drop_in_hand();
    trail_.clear();
    door_back_.clear();
    set_line(2);
    go_to_unit(notes, true);
    message(trf("media.went", {std::to_string(notes.size())}));
}

void Hallway::media_act(MediaRow::Kind k, bool& quit)
{
    MusicPlayer* p = music();
    if (!p) return;
    using K = MediaRow::Kind;
    auto say = [&](const std::string& s) {
        std::lock_guard<std::mutex> lock(media_mx_);
        media_status_ = s;
    };
    if (k == K::Next)
    {
        p->next(media_mode_);
        say(trf("media.next_done", {mode_name(media_mode_)}));
        return;
    }
    const auto t = media_selected();
    if (!t)
    {
        say(tr("media.nothing_selected"));
        return;
    }
    switch (k)
    {
    case K::Play:
        p->play(*t);
        say(trf("media.playing", {mode_name(t->mode)}));
        break;
    case K::GoTo:
        go_to_track(t->notes);
        if (request_ == Request::GoToTrack) quit = true; // the application builds the hallway again
        break;
    case K::Save:
    {
        {
            std::lock_guard<std::mutex> lock(media_mx_);
            media_saving_ = t->notes;
        }
        const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
        const std::string name = "sieve-audio-" + t->address().substr(0, 12) + ".mid";
        const std::string start = docs ? std::string(docs) + name : name;
        SDL_ShowSaveFileDialog(media_save_chosen, this, window_, nullptr, 0, start.c_str());
        break;
    }
    case K::Favourite:
        if (media_list_ == 2)
        {
            const std::string err = p->remove_favourite(size_t(media_sel_[2]));
            say(err.empty() ? tr("media.fav_removed") : err);
            media_sel_[2] = std::max(0, std::min(media_sel_[2], int(p->favourites().size()) - 1));
        }
        else
        {
            const std::string err = p->add_favourite(*t);
            const std::u8string where = p->favourites_path().u8string();
            say(err.empty() ? trf("media.fav_added", {std::string(where.begin(), where.end())}) : err);
        }
        break;
    default: break;
    }
}

// Left and Right on a control (and ENTER, as Right, on a switch or a button).
void Hallway::media_change(int dir, bool big, bool& quit)
{
    MusicPlayer* p = music();
    if (!p) return;
    const auto rows = media_rows();
    if (media_row_ < 0 || media_row_ >= int(rows.size())) return;
    const MediaRow& row = rows[size_t(media_row_)];
    using K = MediaRow::Kind;
    if (row.kind == K::Mode)
    {
        media_mode_ = media_mode_ == MusicMode::Menus ? MusicMode::World : MusicMode::Menus;
        return;
    }
    if (row.kind == K::Next || row.kind == K::Play || row.kind == K::GoTo || row.kind == K::Save || row.kind == K::Favourite)
    {
        media_act(row.kind, quit);
        return;
    }
    MusicSettings s = p->settings(media_mode_);
    const int by = dir * (big ? 10 : 1);
    switch (row.kind)
    {
    case K::On: s.on = !s.on; break;
    case K::Volume: s.volume = std::clamp(s.volume + by * 5, 0, 100); break;
    case K::Length: s.length = uint32_t(std::clamp(int(s.length) + by, 1, 4096)); break;
    case K::Tempo: s.tempo = std::clamp(s.tempo + by * 2, 20, 300); break;
    case K::Voice: s.voice = Voice((int(s.voice) + dir + 4) % 4); break;
    case K::Echo: s.echo = std::clamp(s.echo + by * 5, 0, 80); break;
    case K::Gap: s.gap = std::clamp(s.gap + by, 0, 600); break;
    case K::Fifths: s.fifths = !s.fifths; break;
    case K::LineMode: s.modes[row.line] = ((s.modes[row.line] + dir) % kMusicModes + kMusicModes) % kMusicModes; break;
    case K::Filter: (void)cli::tick_filter(s.filters, row.filter, !s.filters.is_enabled(row.filter)); break; // with its prerequisites
    case K::Param:
    {
        const sieve::FilterSpec* spec = sieve::find_filter(row.filter);
        if (!spec) return;
        const sieve::FilterParam* prm = nullptr;
        for (const auto& q : spec->params)
            if (q.key == row.key) prm = &q;
        if (!prm) return;
        sieve::FilterValues& vals = s.filters.values[row.filter];
        if (prm->kind == sieve::FilterParam::Kind::Integer)
        {
            int64_t v = 0;
            try { v = sieve::param_int(*spec, vals, prm->key); } catch (const std::exception&) { v = std::stoll(prm->default_value); }
            vals[prm->key] = std::to_string(std::clamp<int64_t>(v + dir * prm->step * (big ? 10 : 1), prm->min, prm->max));
        }
        else if (!prm->choices.empty())
        {
            const std::string cur = sieve::param_value(*spec, vals, prm->key);
            const auto it = std::find(prm->choices.begin(), prm->choices.end(), cur);
            const int n = int(prm->choices.size()), i = it == prm->choices.end() ? 0 : int(it - prm->choices.begin());
            vals[prm->key] = prm->choices[size_t(((i + dir) % n + n) % n)];
        }
        break;
    }
    default: return;
    }
    p->set_settings(media_mode_, s);
}

void Hallway::media_event(const SDL_Event& e, bool& quit)
{
    MusicPlayer* p = music();
    const int counts[3] = {p ? int(p->recent().size()) : 0, int(media_rows().size()), p ? int(p->favourites().size()) : 0};
    auto focus = [&](int area) {
        media_area_ = area;
        if (area != 1) media_list_ = area;
    };
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && (e.button.button == SDL_BUTTON_LEFT || e.button.button == SDL_BUTTON_RIGHT))
    {
        float x = e.button.x, y = e.button.y;
        SDL_RenderCoordinatesFromWindow(r_, x, y, &x, &y);
        for (const auto& [r, at] : media_rects_)
            if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h)
            {
                focus(at.first);
                if (at.first == 1)
                {
                    const bool again = media_row_ == at.second;
                    media_row_ = at.second;
                    // A second click on a control changes it (right-click the other way).
                    if (again) media_change(e.button.button == SDL_BUTTON_RIGHT ? -1 : 1, false, quit);
                }
                else media_sel_[at.first] = at.second;
                return;
            }
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    const bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0;
    int& row = media_area_ == 1 ? media_row_ : media_sel_[media_area_];
    const int n = counts[media_area_];
    using K = MediaRow::Kind;
    switch (e.key.key)
    {
    case SDLK_ESCAPE: close_media_player(); break; // back to the pause menu
    case SDLK_TAB: focus((media_area_ + (shift ? 2 : 1)) % 3); break;
    case SDLK_UP:
    case SDLK_W: if (n > 0) row = (row + n - 1) % n; break;
    case SDLK_DOWN:
    case SDLK_S: if (n > 0) row = (row + 1) % n; break;
    case SDLK_LEFT:
    case SDLK_A: if (media_area_ == 1) media_change(-1, shift, quit); break;
    case SDLK_RIGHT:
    case SDLK_D: if (media_area_ == 1) media_change(1, shift, quit); break;
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
    case SDLK_SPACE:
        if (media_area_ == 1) media_change(1, false, quit);
        else media_act(K::Play, quit);
        break;
    case SDLK_G: media_act(K::GoTo, quit); break;
    case SDLK_F: media_act(K::Save, quit); break;
    case SDLK_V: media_act(K::Favourite, quit); break;
    case SDLK_N: media_act(K::Next, quit); break;
    default: break;
    }
}

void Hallway::draw_media_player(float W, float H)
{
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{255, 255, 255, 255}, dim{90, 90, 90, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);
    text(20, 16, tr("media.title"), 3, ink);
    media_rects_.clear();
    MusicPlayer* p = music();
    if (!p)
    {
        text(20, 70, tr("media.no_player"), 1, grey);
        text(20, H - 26, tr("media.keys"), 1, grey);
        return;
    }
    const float gap = 20, lw = (W - 4 * gap) * 0.3f, cw = (W - 4 * gap) * 0.4f, rw = lw;
    const float lx = gap, cx = lx + lw + gap, rx = cx + cw + gap, top = 64, bottom = H - 96;
    const int shown = std::max(1, int((bottom - top - 24) / kRowH));

    auto frame = [&](float x, float w, int area, const std::string& title) {
        const bool focused = media_area_ == area;
        SDL_SetRenderDrawColor(r_, focused ? ink.r : dim.r, focused ? ink.g : dim.g, focused ? ink.b : dim.b, 255);
        const SDL_FRect box{x, top, w, bottom - top};
        SDL_RenderRect(r_, &box);
        text(x + 8, top + 6, title, 1, focused ? ink : grey);
    };
    auto highlight = [&](float x, float y, float w, bool focused) {
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, Uint8(focused ? 70 : 30));
        const SDL_FRect hl{x + 2, y - 3, w - 4, kRowH};
        SDL_RenderFillRect(r_, &hl);
    };

    // The two lists.
    auto list = [&](float x, float w, int area, const std::vector<MusicTrack>& tracks, const std::string& title, const std::string& empty) {
        frame(x, w, area, title);
        if (tracks.empty()) text(x + 8, top + 28, fit(empty, w - 16, 1), 1, dim);
        int& sel = media_sel_[area];
        sel = std::clamp(sel, 0, std::max(0, int(tracks.size()) - 1));
        const int first = std::max(0, sel - shown + 1);
        for (int i = first; i < std::min(int(tracks.size()), first + shown); ++i)
        {
            const float y = top + 28 + float(i - first) * kRowH;
            const bool chosen = i == sel && media_list_ == area;
            if (i == sel) highlight(x, y, w, media_area_ == area);
            text(x + 8, y, fit(track_line(tracks[size_t(i)]), w - 16, 1), 1, chosen ? white : ink);
            media_rects_.push_back({SDL_FRect{x, y - 3, w, kRowH}, {area, i}});
        }
    };
    list(lx, lw, 0, p->recent(), tr("media.recent"), tr("media.recent_empty"));
    list(rx, rw, 2, p->favourites(), tr("media.favourites"), tr("media.favourites_empty"));

    // The controls.
    frame(cx, cw, 1, trf("media.controls", {mode_name(media_mode_)}));
    const MusicSettings s = p->settings(media_mode_);
    const auto rows = media_rows();
    media_row_ = std::clamp(media_row_, 0, int(rows.size()) - 1);
    if (media_row_ < media_top_) media_top_ = media_row_;
    if (media_row_ >= media_top_ + shown) media_top_ = media_row_ - shown + 1;
    const bool from_favs = media_list_ == 2;
    for (int i = media_top_; i < std::min(int(rows.size()), media_top_ + shown); ++i)
    {
        const MediaRow& row = rows[size_t(i)];
        const float y = top + 28 + float(i - media_top_) * kRowH;
        using K = MediaRow::Kind;
        std::string label, value;
        bool header = false;
        switch (row.kind)
        {
        case K::Mode: label = tr("media.mode"); value = "< " + mode_name(media_mode_) + " >"; break;
        case K::On: label = tr("media.on"); value = tr(s.on ? "media.value.on" : "media.value.off"); break;
        case K::Volume: label = tr("media.volume"); value = std::to_string(s.volume) + "%"; break;
        case K::Length: label = tr("media.length"); value = trf("media.value.notes", {std::to_string(s.length)}); break;
        case K::Tempo: label = tr("media.tempo"); value = trf("media.value.bpm", {std::to_string(s.tempo)}); break;
        case K::Voice: label = tr("media.voice"); value = tr(std::string("media.voice.") + voice_id(s.voice)); break;
        case K::Echo: label = tr("media.echo"); value = std::to_string(s.echo) + "%"; break;
        case K::Gap: label = tr("media.gap"); value = trf("media.value.seconds", {std::to_string(s.gap)}); break;
        case K::Character:
            label = tr(s.filters.is_enabled("key-data-v2") || s.filters.is_enabled("key-v1") ? "media.character" : "media.character.no_key");
            header = true;
            break;
        case K::LineMode: label = "   " + tr(line_key(row.line)); value = tr(std::string("media.mode.") + music_mode_name(s.modes[row.line])); break;
        case K::Fifths: label = tr("media.fifths"); value = tr(s.fifths ? "media.value.on" : "media.value.off"); break;
        case K::Next: label = tr("media.next"); break;
        case K::Filters: label = tr("media.filters"); header = true; break;
        case K::Filter:
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            label = std::string(s.filters.is_enabled(row.filter) ? "[x] " : "[ ] ") + row.filter + (f && !f->title.empty() ? "  " + f->title : "");
            break;
        }
        case K::Param:
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            label = "      " + row.key;
            const auto it = s.filters.values.find(row.filter);
            const sieve::FilterValues none;
            if (f) value = sieve::param_value(*f, it == s.filters.values.end() ? none : it->second, row.key);
            break;
        }
        case K::Play: label = tr("media.play"); break;
        case K::GoTo: label = tr("media.goto"); break;
        case K::Save: label = tr("media.save"); break;
        case K::Favourite: label = tr(from_favs ? "media.fav_remove" : "media.fav_add"); break;
        }
        if (i == media_row_) highlight(cx, y, cw, media_area_ == 1);
        const SDL_Color c = header ? grey : i == media_row_ && media_area_ == 1 ? white : ink;
        text(cx + 8, y, fit(label, value.empty() ? cw - 16 : cw * 0.6f, 1), 1, c);
        if (!value.empty()) text(cx + cw - 8 - text_width(value, 1), y, value, 1, c);
        if (!header) media_rects_.push_back({SDL_FRect{cx, y - 3, cw, kRowH}, {1, i}});
    }

    // Underneath: what is playing in each mode, what the actions would act on, and how it went.
    float y = bottom + 10;
    for (MusicMode m : {MusicMode::Menus, MusicMode::World})
    {
        const auto now = p->now_playing(m);
        const std::string on = now ? now->address().substr(0, 16) + "  " + now->notation() : tr("media.nothing_yet");
        text(20, y, fit(trf("media.now", {mode_name(m), on}), W - 40, 1), 1, m == (in_menu() ? MusicMode::Menus : MusicMode::World) ? ink : grey);
        y += 14;
    }
    if (const auto t = media_selected())
        text(20, y, fit(trf("media.selected", {tr(from_favs ? "media.favourites" : "media.recent"), t->address().substr(0, 24), std::to_string(t->notes.size())}), W - 40, 1), 1, grey);
    y += 14;
    std::string status;
    {
        std::lock_guard<std::mutex> lock(media_mx_);
        status = media_status_;
    }
    if (status.empty()) status = p->status();
    if (!p->audio()) status = tr("media.silent") + (status.empty() ? "" : ": " + status);
    if (!status.empty()) text(20, y, fit(status, W - 40, 1), 1, white);
    text(20, H - 18, fit(tr("media.keys"), W - 40, 1), 1, grey);
}

} // namespace hallway::hall
