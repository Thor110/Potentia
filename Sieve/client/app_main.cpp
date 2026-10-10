// Sieve hallway -- the program: its options, the menus, the screenshot and scripting paths used by
// the checks, and the event loop. The hallway itself is in hallway.hpp and the files beside it.

#include "window_icon.hpp"
#include "cli/media_decode.hpp"
#include "cli/plugins.hpp"
#include "designer.hpp"
#include "cli/timings.hpp"
#include "hallway.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sound.hpp"

#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <fstream>

using namespace hallway;
using namespace hallway::hall;

namespace {

bool save_render(SDL_Renderer* r, const std::string& path)
{
    SDL_Surface* s = SDL_RenderReadPixels(r, nullptr);
    if (!s) return false;
    SDL_Surface* rgb = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGB24);
    SDL_DestroySurface(s);
    if (!rgb) return false;
    std::vector<Rgb> px(size_t(rgb->w) * rgb->h);
    for (int y = 0; y < rgb->h; ++y)
    {
        const auto* row = static_cast<const Uint8*>(rgb->pixels) + size_t(y) * rgb->pitch;
        for (int x = 0; x < rgb->w; ++x) px[size_t(y) * rgb->w + x] = {row[x * 3], row[x * 3 + 1], row[x * 3 + 2]};
    }
    write_png(path, uint32_t(rgb->w), uint32_t(rgb->h), px, 1);
    SDL_DestroySurface(rgb);
    return true;
}

// ---------------------------------------------------------------- options

const char* kUsage =
    "hallway - walk the Sieve's five lines in 3D\n\n"
    "Usage: hallway [options]\n\n"
    "Lines (the same meaning as in the sieve tool):\n"
    "  --line LINE         the line to start in: image, pages (default), books, audio, tracks,\n"
    "                      video, movies, models or binary\n"
    "  --length L          text: characters per book (default 32)\n"
    "  --alphabet ID       text: lower27 (default), babel29, ascii95\n"
    "  --canon v2|v1       text: warp rules (default v2)\n"
    "  --image-width W  --image-height H  --image-palette ID    image line (10, 10, mono)\n"
    "  --notes N           audio: notes per book (default 16; per voice on notes2)\n"
    "  --note-set notes104|notes2|notes3|pcm  --note-low C3  --note-high C6  --note-durations seEqQhHw  --voices 1..4\n"
    "                      audio: the note set (default notes104; low to voices are notes2's)\n"
    "  --notes3-low C-1  --notes3-high G9  --notes3-tpq 4  --notes3-longest 16  --notes3-levels 8\n"
    "  --notes3-voices 1..15  --notes3-tempo 120  --notes3-instruments 0[,0...]\n"
    "                      audio, --note-set notes3: open-ended notes (lengths in ticks, loudness levels)\n"
    "  --samples N  --pcm-rate HZ  --pcm-bits B  --pcm-channels C\n"
    "                      audio, --note-set pcm: sound itself, N samples a channel per book\n"
    "                      (default 8000), at 8000 a second, 8 bits (1-31), 1 channel\n"
    "  --video-width W  --video-height H  --video-frames F  --video-palette ID   video (5, 5, 8, mono)\n"
    "  --title-length T    every line's titles: characters (default 32; 0: no titles; books keep a page)\n"
    "  --binary-length N   binary: every file of up to N bytes (default 32)\n"
    "  --book-pages N      books: pages per book (default 4); a book is a cover (an image of the\n"
    "                      image line), a title and N pages (pages of the pages line)\n"
    "  --track-units N     tracks: units of audio per track (default 4); a track is a cover, a title\n"
    "                      and N units of the audio line (composition-v1)\n"
    "  --movie-units N     movies: units of video per movie (default 4), as tracks are of audio\n"
    "  --ffmpeg PATH       the ffmpeg that reads and writes picture, video and sound formats beyond\n"
    "                      PNG, JPEG, BMP, GIF, TGA, WAV and MIDI (default: SIEVE_FFMPEG, then beside\n"
    "                      the hallway, then the PATH); kept in the settings\n"
    "  --video-fps N       a video saved as GIF, MP4 or WebM (with ffmpeg): frames a second (default 8)\n"
    "  --key K             scramble key (default sieve)\n"
    "  --mode positional|scrambled|guided   starting ordering (default positional)\n"
    "  --model ID|PATH|none  text: model for the guided ordering (default: the alphabet's default)\n"
    "  --zoom D            guided: books 2^-D of the line apart (after --warp/--goto set it)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, a picture file for image/video, a .book file for books)\n"
    "  --goto ADDR|P%|@T   go to an address, a percentage, or corridor tile T on start\n\n"
    "Menu:\n"
    "  The main menu opens first: Start Sieve, Settings (graphics, controls, language; saved to\n"
    "  sieve-hallway.ini), the File Locator (as in the pause menu, without Go to it), the Filter Designer\n"
    "  (make filter plugins as nodes) and Exit Sieve. Start Sieve opens the setup menu (Esc: back to the\n"
    "  main menu): adjust every line's state space and see the five lines as a map.\n"
    "  The magnifying glass beside a line's title (or F) opens its filters: tick filters, set\n"
    "  their parameters, and choose the mode: off, mark (failures faint), hide (failures left\n"
    "  out), compact (only survivors, packed together, in every ordering) or excluded (only\n"
    "  failures, in their places, to check what the filters set aside). Saved to the\n"
    "  --filters file. In compact mode, G takes a compact address, as the books show it.\n"
    "  --no-menu           go straight into the hallway (F1 opens the setup menu from the hallway)\n"
    "  --settings PATH     application settings (default: sieve-hallway.ini next to the executable)\n"
    "  --language CODE     menu language for this run (a file in the lang folder, e.g. en)\n"
    "  --menu              with --screenshot: a picture of the setup menu (--press keys go to it)\n"
    "                      (--busy: while the dimensions are still being calculated, if that is caught)\n"
    "  --main-menu         with --screenshot: a picture of the main menu (--press keys go to it)\n"
    "  --designer          with --screenshot: a picture of the filter designer; --design FILE opens a\n"
    "                      plugin, --script \"Down,Right,=text,Return\" runs keys and typed text, and\n"
    "                      --design-out FILE writes the filter as the designer would save it\n"
    "  --no-music          no background music this run (the Media Player is still there, silent)\n"
    "  --media-player      with --screenshot: a silent music player, so --press can open the Media\n"
    "                      Player from the pause menu (Escape, then Down to it, Return)\n"
    "  --filters PATH      filter settings (default: sieve-filters.ini next to the executable)\n"
    "  --timings           time each phase (building, items, filters, vault, faces, frames, menus);\n"
    "                      written at exit to sieve-timings.txt beside the settings, and to stderr\n\n"
    "Screenshots (for documentation and testing):\n"
    "  --screenshot PATH   render one frame to a PNG and exit\n"
    "  --size WxH          window size (default 1280x720)\n"
    "  --pose X,Z,YAW,PITCH  camera position and angles in degrees\n"
    "  --tile N            then move N tiles along the corridor\n"
    "  --take              take the book you are looking at off the shelf\n"
    "  --tailor PATH       with --screenshot, the item in hand on COST: K, waited for, and what Return\n"
    "                      would apply saved to PATH as a settings file\n"
    "  --save-item PATH    then save it as a file (F on the item page), to PATH\n"
    "  --save-view PATH    with the viewer open (--press E,Z,Tab...): save what it shows (F), to PATH\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n"
    "  --press K,K,...     then press these keys (e.g. M,M,-,Shift+=; Click: a left click where the\n"
    "                      crosshair is), printing where you are\n"
    "  --edge-glow         draw with Geometry Edge Glow (or --settings a file that has it on)\n"
    "  --real-graphics     draw with Real Graphics: the models in the meshes folder\n"
    "  --door-portals      fill the doorways with procedural data noise (Real Graphics turns this on)\n"
    "  --model-cache MB    display cache: memory for the pictures on items (8 up, default 64)\n"
    "  --model-tile PX     display size: how wide each item's picture is drawn (16 up to the\n"
    "                      renderer's widest texture, default 64)\n"
    "  --item-letters PX   letter size on item displays: pages and titles are drawn wide enough\n"
    "                      for it, and smaller letters are dashes (default 8)\n"
    "  --close-up PX       the displays nearest you are drawn again up to this wide (0: off;\n"
    "                      \"screen\", the default, for the screen's width rounded up to a power of\n"
    "                      two; or a power of two from 256 to the renderer's widest texture)\n"
    "  --items-per-wall N  units on one tile of the corridor (128 or 256; changes no address,\n"
    "                      only the tile and slot that name a unit's place in the corridor)\n"
    "  --fps-counter       show the FPS counter\n"
    "  --bench N           before the screenshot, time N frames and print the frame rate\n"
    "  --settle N          before the screenshot, draw N frames standing still, so the item pictures arrive\n"
    "  --sample-degrees DIR   every dimension's item at each whole degree 0-359, saved into DIR as its\n"
    "                      file named by its title (pictures one pixel a pixel, as J reads them), and DIR\n"
    "                      made into Sieve instructions (--sample-out FILE, else DIR.sieve)\n"
    "  --locate PATH       open the File Locator on a file or folder (--tailored: and tailor the filters to its files;\n"
    "                      with --main-menu: the main menu's, with no world to walk to)\n"
    "  --install FILE --install-to DIR   the File Locator's install, at once\n"
    "  --map PATH          choose a map for the node graph: a .map file, or a folder to map\n"
    "  --graph             open the node graph (on --map, or on this installation)\n"
    "  --new-map PATH      the node graph's New map..., at once\n"
    "  --thin              keep only the room you stand in (what going in past the budget does)\n"
    "  --filter-memory MB  the most one count of the filters' survivors may take for its tables (default\n"
    "                      the setup menu's, 512 MB at first)\n"
    "  --counting-memory PCT  the share of installed memory the setup menu's counts may take at once\n"
    "                      (5 to 100; the setup menu's, 50 at first)\n"
    "  --merge-cache PCT   the share of the filter memory kept for merged filters between counts (0 to\n"
    "                      100; the setup menu's, 50 at first)\n"
    "  --unit-time MS      the time budget: the longest one unit may take to open (5 to 60000; the\n"
    "                      setup menu's, 50 at first)\n"
    "  --item-memory PCT   the share of installed memory the items around you may take (5 to 90; the\n"
    "                      setup menu's, 25 at first)\n"
    "  --view-rooms N      rooms drawn and kept either side of you (2 to 64; 7 at first)\n"
    "  --picture-rooms N   rooms either side of yours with item pictures of their own (0 to 8; 1)\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering (positional, scrambled, guided), - and = zoom out/in (guided; Shift: 8x),\n"
    "wheel/PgUp/PgDn/[ ] jump 1/1000/1000000 tiles, Home to corridor tile 0 (every line's start line),\n"
    "N/B next/previous unit of a warped trail (or page of a book in hand), P play an audio book, F save the item in hand as a file, F1 the setup menu, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit.\n\n"
    "All five lines share one corridor, 128 books per tile. Each line repeats along it; a\n"
    "checkered start line marks where each repeat begins. Black doors lead to the next line\n"
    "(left wall) or the previous line (right wall) at the same corridor position.\n";

// Whether the settings make a pcm set (a set they cannot make falls back to notes104, as the setup
// menu says).
bool pcm_settings_ok(const Args& a)
{
    try
    {
        sieve::make_pcm_format(a.get_positive("pcm-rate", 8000), a.get_positive("pcm-bits", 8), a.get_positive("pcm-channels", 1));
        return uint64_t(a.get_positive("samples", 8000)) * a.get_positive("pcm-channels", 1) <= 0xFFFFFFFFull;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// Whether the settings make a notes3 set (one they cannot make falls back to notes104).
bool notes3_settings_ok(const Args& a)
{
    try
    {
        Args la;
        la.opts["line"] = "audio";
        la.opts["note-set"] = "notes3";
        for (const char* k : {"low", "high", "tpq", "longest", "levels", "voices", "tempo", "instruments"})
            if (a.has(std::string("notes3-") + k)) la.opts[k] = a.get(std::string("notes3-") + k);
        (void)make_line(la);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

std::vector<Line> make_lines(const Args& a)
{
    std::vector<Line> lines;
    for (LineKind k : kLineOrder)
    {
        Args la;
        la.opts["line"] = to_string(k);
        la.opts["key"] = a.get("key", "sieve");
        switch (k)
        {
        case LineKind::Text:
            la.opts["length"] = a.get("length", "32");
            la.opts["alphabet"] = a.get("alphabet", "lower27");
            la.opts["canon"] = a.get("canon", "v2");
            if (a.has("model")) la.opts["model"] = a.get("model");
            break;
        case LineKind::Image:
            la.opts["width"] = a.get("image-width", "10");
            la.opts["height"] = a.get("image-height", "10");
            la.opts["palette"] = a.get("image-palette", "mono");
            break;
        case LineKind::Audio:
            la.opts["length"] = a.get("notes", "16");
            if (a.get("note-set", "notes104") == "pcm" && pcm_settings_ok(a))
            {
                la.opts["note-set"] = "pcm";
                la.opts["length"] = a.get("samples", "8000");
                la.opts["rate"] = a.get("pcm-rate", "8000");
                la.opts["bits"] = a.get("pcm-bits", "8");
                la.opts["channels"] = a.get("pcm-channels", "1");
            }
            else if (a.get("note-set", "notes104") == "notes3" && notes3_settings_ok(a))
            {
                la.opts["note-set"] = "notes3";
                la.opts["low"] = a.get("notes3-low", "C-1");
                la.opts["high"] = a.get("notes3-high", "G9");
                la.opts["tpq"] = a.get("notes3-tpq", "4");
                la.opts["longest"] = a.get("notes3-longest", "16");
                la.opts["levels"] = a.get("notes3-levels", "8");
                la.opts["voices"] = a.get("notes3-voices", "1");
                la.opts["tempo"] = a.get("notes3-tempo", "120");
                la.opts["instruments"] = a.get("notes3-instruments", "0");
            }
            else if (a.get("note-set", "notes104") == "notes2")
            {
                la.opts["note-set"] = "notes2";
                la.opts["low"] = a.get("note-low", "C3");
                la.opts["high"] = a.get("note-high", "C6");
                la.opts["durations"] = a.get("note-durations", "seEqQhHw");
                la.opts["voices"] = a.get("voices", "1");
            }
            break;
        case LineKind::Video:
            la.opts["width"] = a.get("video-width", "5");
            la.opts["height"] = a.get("video-height", "5");
            la.opts["frames"] = a.get("video-frames", "8");
            la.opts["palette"] = a.get("video-palette", "mono");
            break;
        }
        lines.push_back(make_line(la));
    }
    return lines;
}

// The BINARY length (bytes): the binary line's, as the setup menu sets it.
uint32_t binary_length(const Args& a) { return a.has("binary-length") ? a.get_positive("binary-length", 32) : 32u; }

// The File Locator from the main menu: files weighed under the lines the setup menu's settings
// would build (made when the first weighing starts), and nowhere to walk to, so no Go to it.
// Tailored filters, when used, are saved for the next hallway built. Esc comes back to the main
// menu; `path` (scripted runs, --main-menu --locate PATH) opens it on a file or folder at once.
// Returns false if the window was closed.
std::vector<std::pair<SDL_Keycode, SDL_Keymod>> parse_presses(const std::string& spec); // below

struct MenuLocator
{
    Args la;
    std::vector<Line> lines;
    std::once_flag made;
    FilterConfig filters;
};

bool run_locator(SDL_Window* window, SDL_Renderer* renderer, const Settings& settings, FilterConfig& filters, const std::string& filters_path,
                 const std::string& path = "", bool tailor = false, const std::string& screenshot = "", const std::string& presses = "")
{
    MenuLocator m;
    settings.apply(m.la);
    m.filters = filters;
    FileLocator::Host host;
    host.lines = [&m] {
        std::call_once(m.made, [&m] { m.lines = make_lines(m.la); });
        sieve::cli::WeighLines wl;
        wl.pages = &m.lines[size_t(LineKind::Text)];
        wl.image = &m.lines[size_t(LineKind::Image)];
        wl.audio = &m.lines[size_t(LineKind::Audio)];
        wl.video = &m.lines[size_t(LineKind::Video)];
        wl.filters = m.filters;
        wl.binary_bytes = binary_length(m.la);
        return wl;
    };
    FileLocator* self = nullptr;
    host.use_filters = [&](const FilterConfig& f) {
        filters = m.filters = f;
        try
        {
            f.save(filters_path);
            self->say(tr("loc.filters_kept"));
        }
        catch (const std::exception& e)
        {
            self->say(trf("loc.filters_not_saved", {e.what()}));
        }
    };
    FileLocator loc(window, renderer, host);
    self = &loc;
    if (!path.empty()) loc.locate_now(path, tailor);
    else loc.open();
    auto frame = [&] {
        int ww = 0, wh = 0;
        SDL_GetCurrentRenderOutputSize(renderer, &ww, &wh);
        SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        loc.draw(float(ww), float(wh));
    };
    if (!screenshot.empty())
    {
        for (const auto& [key, mod] : parse_presses(presses))
        {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = key;
            e.key.mod = mod;
            loc.event(e);
        }
        std::cout << "file locator (main menu): " << (loc.is_open() ? "open" : "closed") << ", "
                  << (loc.offers_go() ? "Go to it offered" : "no Go to it") << "\n";
        frame();
        if (!save_render(renderer, screenshot)) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        loc.stop();
        return true;
    }
    bool quit = false;
    while (loc.is_open() && !quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT) quit = true;
            else loc.event(e);
        }
        music_mode(MusicMode::Menus);
        frame();
        present(renderer);
    }
    loc.stop();
    return !quit;
}

// "M,M,-,Shift+=" -> key presses.
// "Click" in --press: a left click in the middle of the window, where the crosshair is (the hallway
// only; a menu takes it as a key it does not know).
constexpr SDL_Keycode kClickPress = SDLK_SCANCODE_MASK | 0xFFFF;

std::vector<std::pair<SDL_Keycode, SDL_Keymod>> parse_presses(const std::string& spec)
{
    std::vector<std::pair<SDL_Keycode, SDL_Keymod>> out;
    for (size_t start = 0; start < spec.size();)
    {
        const size_t end = std::min(spec.find(',', start), spec.size());
        std::string name = spec.substr(start, end - start);
        SDL_Keymod mod = SDL_KMOD_NONE;
        if (name.rfind("Shift+", 0) == 0) { mod = SDL_KMOD_LSHIFT; name = name.substr(6); }
        if (name.rfind("Ctrl+", 0) == 0) { mod = SDL_KMOD_LCTRL; name = name.substr(5); }
        const SDL_Keycode key = name == "-" ? SDLK_MINUS : name == "=" ? SDLK_EQUALS : name == "Click" ? kClickPress : SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN) throw std::invalid_argument("--press: unknown key '" + name + "'");
        out.emplace_back(key, mod);
        start = end + 1;
    }
    return out;
}

// Builds the hallway from options, and applies the start-up and scripting options.
std::unique_ptr<Hallway> make_hallway(SDL_Window* window, SDL_Renderer* renderer, const Args& a, bool scripted,
                                      const FilterConfig& filters, int angle_decimals)
{
    // The corridor's tile size, before any line's loop is worked out from it. It changes no
    // address: only the tile and slot that name a unit's place in the corridor (corridor.hpp).
    if (a.has("items-per-wall")) sieve::set_books_per_tile(a.get_u32("items-per-wall", 128));
    std::vector<Line> lines = make_lines(a);
    // The door to start at, by name: an id ("pages") or a --line name ("text"). Any other name is
    // refused, by line_from_string(), with the names it takes.
    int start_line = line_named(a.get("line", "text"));
    if (start_line < 0) start_line = line_of(line_from_string(a.get("line", "text")));
    const bool text_has_model = lines[size_t(LineKind::Text)].guided != nullptr;
    // Books and models are not made of one alphabet, so neither starts with the text greeting.
    const LineKind start_kind = kDimensions[start_line].unit.value_or(LineKind::Image);

    const Hallway::ModelShape shape{a.get_positive("vertices", 8), a.get_positive("faces", 12), a.get_positive("coords", 16),
                                    a.has("title-length") ? a.get_u32("title-length", 32) : 32u,
                                    binary_length(a),
                                    a.has("track-units") ? a.get_positive("track-units", 4) : 4u,
                                    a.has("movie-units") ? a.get_positive("movie-units", 4) : 4u};
    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines), filters,
                                          a.has("book-pages") ? a.get_u32("book-pages", 4) : 4, shape);
    hall->set_line(start_line);
    hall->set_angle_decimals(angle_decimals); // before any scripted keys, which may type a bearing
    if (a.has("mode"))
    {
        if (a.get("mode") == "guided")
        {
            if (text_has_model) hall->enable_guided();
            else
            {
                std::cerr << "guided ordering needs a model for the text line (see: sieve models); using positional\n";
                hall->message("guided ordering needs a text model: using positional");
            }
        }
        else hall->set_mode(address_mode_from_string(a.get("mode")));
    }
    if (a.has("warp")) hall->warp(a.get("warp"));
    else if (a.has("goto")) hall->go_to(a.get("goto"));
    else if (start_kind == LineKind::Text)
    {
        // The greeting is Latin letters, so on an alphabet that cannot hold them (Greek, kana,
        // hieroglyphs) nothing survives canonicalisation: start halfway along instead.
        if (!hall->warp("welcome to the sieve")) hall->go_to("50%");
        hall->put_back();
    }
    else
    {
        // Halfway along, and on the shelf: going to an address hands you the unit, and holding
        // one holds you still, so starting a line holding a record would start it unable to move.
        hall->go_to("50%");
        hall->put_back();
    }
    if (!scripted) return hall;

    if (a.has("zoom")) hall->zoom_to(a.get_positive("zoom", 20));
    if (a.has("locate")) hall->locate_now(a.get("locate"), a.has("tailored"));
    if (a.has("install") && a.has("install-to")) hall->install_now(a.get("install"), a.get("install-to"));
    if (a.has("new-map")) hall->graph_picked(4, a.get("new-map")); // the viewer's New map..., at once
    if (a.has("map") || a.has("graph")) hall->graph_map_now(a.has("map") ? a.get("map") : "", a.has("graph"));
    if (a.has("tile"))
    {
        const std::string t = a.get("tile");
        const bool neg = !t.empty() && t[0] == '-';
        const int64_t n = int64_t(parse_whole(neg ? t.substr(1) : t, "--tile", uint64_t(1) << 62));
        hall->move_tiles(neg ? -n : n);
    }
    if (a.has("pose"))
    {
        float v[4] = {};
        if (!hallway::parse_floats(a.get("pose"), v, 4)) throw std::invalid_argument("--pose expects X,Z,YAW,PITCH");
        const float x = v[0], z = v[1], yaw = v[2], pitch = v[3];
        hall->camera().pos = {x, 1.6f, z};
        hall->camera().yaw = yaw * kPi / 180;
        hall->camera().pitch = pitch * kPi / 180;
    }
    if (a.has("walk"))
    {
        // "DX,DZ;DX,DZ;..." in metres, applied in 5 cm steps so walls and doors behave as when walking.
        // They are in the walker's own frame: a door that turns you round (into or out of the
        // binary line from models) turns the rest of the walk round with you, as it turns a
        // person walking forward.
        std::string spec = a.get("walk");
        float frame = 1.0f;
        for (size_t start = 0; start < spec.size();)
        {
            const size_t end = std::min(spec.find(';', start), spec.size());
            float v[2] = {};
            if (!hallway::parse_floats(spec.substr(start, end - start), v, 2)) throw std::invalid_argument("--walk expects DX,DZ;DX,DZ;...");
            const float dx = v[0], dz = v[1];
            const int steps = std::max(1, int(std::ceil(std::sqrt(dx * dx + dz * dz) / 0.05f)));
            for (int i = 0; i < steps; ++i)
            {
                const float yaw = hall->camera().yaw;
                hall->move_by({frame * dx / steps, 0, frame * dz / steps});
                if (std::abs(std::remainder(hall->camera().yaw - yaw, 2 * kPi)) > 3.0f) frame = -frame;
            }
            std::cout << hall->status() << "\n";
            start = end + 1;
        }
    }
    if (a.has("press"))
        for (const auto& [key, mod] : parse_presses(a.get("press")))
        {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = key;
            e.key.mod = mod;
            if (key == kClickPress)
            {
                int w = 0, h = 0;
                SDL_GetWindowSize(window, &w, &h);
                e = SDL_Event{};
                e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                e.button.button = SDL_BUTTON_LEFT;
                e.button.x = float(w) * 0.5f;
                e.button.y = float(h) * 0.5f;
            }
            bool quit = false;
            hall->render(); // so the key sees what you are looking at
            hall->handle(e, quit);
            std::cout << hall->status() << "\n";
        }
    return hall;
}

int run(const Args& a)
{
    int w = 1280, h = 720;
    if (a.has("size") && !hallway::parse_size(a.get("size"), w, h))
        throw std::invalid_argument("--size expects WxH, e.g. 1280x720");
    const bool shot = a.has("screenshot");

    if (!SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    // The application's settings (sieve-hallway.ini, or --settings PATH). On the first start the
    // resolution is chosen from the display. Screenshots keep --size (default 1280x720) and the
    // default settings, so they come out the same on every machine.
    const std::filesystem::path app_path = a.has("settings") ? std::filesystem::path(a.get("settings")) : AppSettings::default_path();
    // --timings: how long each phase takes, written at exit to standard error and to
    // sieve-timings.txt beside the settings (tools/cli/timings.hpp).
    if (a.has("timings")) sieve::cli::timings::enable(app_path.parent_path() / "sieve-timings.txt");
    sieve::cli::load_plugins(); // the filter plugins in the filters folder, beside the built-in filters
    AppSettings app = shot ? AppSettings{} : AppSettings::load(app_path);
    if (shot && a.has("settings")) app = AppSettings::load(app_path);
    // The filter memory (the setup menu's FILTER MEMORY): what one count's tables may take.
    if (a.has("filter-memory")) app.filter_memory_mb = std::clamp(int(a.get_positive("filter-memory", 512)), 64, 1048576);
    sieve::set_filter_memory(double(app.filter_memory_mb) * 1024 * 1024);
    // How much of installed memory the setup menu's counts may take at once (COUNTING MEMORY), and
    // how much of the filter memory keeps merged automata between counts (MERGE CACHE).
    if (a.has("counting-memory")) app.counting_memory_pct = std::clamp(int(a.get_positive("counting-memory", 50)), 5, 100);
    if (a.has("merge-cache")) app.merge_cache_pct = std::clamp(int(a.get_u32("merge-cache", 50)), 0, 100);
    set_counting_share(app.counting_memory_pct);
    sieve::set_merge_cache_share(app.merge_cache_pct / 100.0);
    // The time budget (TIME BUDGET): the longest one unit may take to open.
    if (a.has("unit-time")) app.unit_time_ms = std::clamp(int(a.get_positive("unit-time", 50)), 5, 60000);
    sieve::set_unit_time_ms(app.unit_time_ms);
    set_time_budget(app.unit_time_ms);
    // The items around you (ITEM MEMORY): what the hallway's cache of them may take.
    if (a.has("item-memory")) app.item_memory_pct = std::clamp(int(a.get_positive("item-memory", 25)), 5, 90);
    set_item_memory_share(app.item_memory_pct);
    // The view distance (Settings > Graphics): the rooms drawn and kept, and those with pictures.
    if (a.has("view-rooms")) app.view_rooms = std::clamp(int(a.get_positive("view-rooms", 7)), 2, 64);
    if (a.has("picture-rooms")) app.picture_rooms = std::clamp(int(a.get_u32("picture-rooms", 1)), 0, 8);
    set_view(app.view_rooms, app.picture_rooms);
    const DisplayInfo display = detect_display();
    if (!shot && app.resolution.w <= 0)
    {
        app.resolution = default_resolution(display);
        app.save(app_path);
    }
    if (!shot && !a.has("size")) { w = app.resolution.w; h = app.resolution.h; }
    if (shot && app.resolution.w <= 0) app.resolution = {w, h}; // the screenshot's own size
    set_language(a.has("language") ? a.get("language") : app.language);
    SDL_Window* window = SDL_CreateWindow("Sieve - hallway", w, h, SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));
    set_window_icon(window);
    if (!window) throw std::runtime_error(std::string("cannot open a window: ") + SDL_GetError());
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) throw std::runtime_error(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(renderer, app.vsync ? 1 : 0);
    if (!shot && !a.has("size") && app.fullscreen) apply_video(window, app);
    // The music player (music.hpp): melodies from the audio line behind every screen. Scripted
    // pictures have none, unless --media-player asks for one (silent) to draw the Media Player;
    // --no-music leaves it silent in the app too.
    std::unique_ptr<MusicPlayer> player;
    if (!shot || a.has("media-player"))
    {
        const std::filesystem::path folder = app_path.parent_path().empty() ? std::filesystem::path(".") : app_path.parent_path();
        player = std::make_unique<MusicPlayer>(folder, !shot && !a.has("no-music"));
        set_music(player.get());
    }
    auto finish = [&] {
        player.reset(); // its audio stream goes before SDL does
        release_fonts(); // glyph textures belong to the renderer
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    };

    Settings settings = Settings::from_args(a);
    sieve::cli::set_ffmpeg_path(settings.ffmpeg);
    // Filter settings: --filters PATH, or sieve-filters.ini next to the executable.
    const std::string filters_path = a.has("filters") ? a.get("filters") : FilterConfig::default_path().string();
    if (a.has("filters") && !std::filesystem::exists(filters_path))
        std::cerr << "note: " << filters_path << " does not exist yet: no filters ticked (the menu saves your choices there)\n";
    FilterConfig filters = FilterConfig::load(filters_path);
    if (shot && a.has("designer"))
    {
        // A picture of the filter designer (for documentation and testing): --design FILE opens a
        // plugin in it; --script runs keys and typed text ("Down,Right,=some words,Return").
        Designer d(window, renderer, a.get("design"));
        // --busy: the picture while the first test is still running (the progress window).
        const bool busy = a.has("busy");
        if (!busy) d.settle();
        if (a.has("script"))
        {
            std::string script = a.get("script");
            size_t at = 0;
            while (at <= script.size())
            {
                const size_t comma = script.find(',', at);
                const std::string item = script.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                if (!item.empty() && item[0] == '=') d.type(item.substr(1));
                else if (!item.empty())
                    for (const auto& [key, mod] : parse_presses(item)) d.press(key, mod);
                d.settle();
                if (comma == std::string::npos) break;
                at = comma + 1;
            }
        }
        if (busy)
        {
            d.press(SDLK_F5, SDL_KMOD_NONE); // starts the test on the worker
            SDL_Delay(400);
        }
        else d.settle();
        d.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        if (busy) d.settle(); // never leave the worker running as the program ends
        std::cout << "designer: " << d.doc().name() << "\n";
        if (!d.status().empty()) std::cout << "status: " << d.status() << "\n";
        if (a.has("design-out"))
        {
            std::ofstream out(std::filesystem::path(a.get("design-out")), std::ios::binary);
            out << d.doc().to_text();
        }
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot && a.has("main-menu") && a.has("locate"))
    {
        // A picture of the File Locator opened from the main menu, on a file or folder (--tailored:
        // and its filters tailored): no world, so no Go to it.
        run_locator(window, renderer, settings, filters, filters_path, a.get("locate"), a.has("tailored"), a.get("screenshot"), a.get("press", ""));
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot && a.has("main-menu"))
    {
        // A picture of the main menu; --press keys go to it (e.g. Down,Enter for Settings).
        MainMenu mm(window, renderer, app, a.has("settings") ? app_path : std::filesystem::path(), display);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) mm.press(key, mod);
        mm.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "main menu: language " << language_code() << ", font " << font_name() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot && a.has("menu"))
    {
        // A picture of the setup menu (for documentation and testing); --press keys go to the menu.
        Menu menu(window, renderer, settings, filters, filters_path, &app, a.has("settings") ? app_path : std::filesystem::path());
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) menu.press(key, mod);
        menu.render();              // starts the counting workers
        // A picture shows the counts, not "counting..." (--busy: while they are still being
        // counted, if they take long enough to be caught).
        if (!a.has("busy"))
        {
            finish_filter_warmup();
            menu.render();
        }
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        if (a.has("busy")) finish_filter_warmup(); // never leave the workers counting as the program ends
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot)
    {
        auto hall = [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, a, true, filters, app.angle_decimals);
        }();
        {
            const bool glow = app.edge_glow || a.has("edge-glow"), real = app.real_graphics || a.has("real-graphics");
            // --real-graphics brings Door Portals with it, as turning it on in the menu does. The
            // saved setting is taken as it stands: a settings file that turns portals off with
            // Real Graphics on means exactly that, since nothing may turn them off but you.
            const bool portals = app.door_portals || a.has("real-graphics") || a.has("door-portals");
            hall->set_graphics(glow && !real, real, portals);
            hall->set_model_cache(a.has("model-cache") ? int(a.get_u32("model-cache", 64)) : app.model_cache_mb);
            hall->set_angle_decimals(app.angle_decimals);
            hall->set_face_px(a.get_u32("model-tile", 64));
            hall->set_letters_px(a.has("item-letters") ? a.get_u32("item-letters", 8) : 8);
            hall->set_export_fps(a.has("video-fps") ? a.get_u32("video-fps", sieve::cli::kDefaultExportFps) : sieve::cli::kDefaultExportFps);
            hall->set_closeup_px(closeup_setting(a.get("close-up", "screen"), kCloseUpScreen));
            hall->set_graphics_memory(app.graphics_memory_gb);
            hall->set_fps_counter(app.fps_counter || a.has("fps-counter"));
            if (a.has("thin")) hall->set_thin(true); // (a walk to a long file in --press may have made it thin already)
            // On stderr: stdout is where the readout goes, which scripts read line by line.
            std::cerr << "graphics: edge glow " << (glow && !real ? "on" : "off") << ", real graphics " << (real ? "on" : "off")
                      << ", door portals " << (portals ? "on" : "off")
                      << ", fps counter " << (app.fps_counter || a.has("fps-counter") ? "on" : "off") << "\n";
        }
        if (a.has("bench"))
        {
            // Frame timing: render N frames (turning slowly, so nothing is cached between them).
            const uint32_t n = a.get_positive("bench", 60);
            hall->render();
            const Uint64 t0 = SDL_GetTicksNS();
            for (uint32_t i = 0; i < n; ++i)
            {
                hall->camera().yaw += 0.002f;
                hall->render();
                SDL_RenderPresent(renderer); // includes drawing the batched geometry
            }
            const double ms = double(SDL_GetTicksNS() - t0) / 1e6 / n;
            std::cout << "bench: " << n << " frames, " << ms << " ms per frame (" << 1000.0 / ms << " fps), renderer "
                      << SDL_GetRendererName(renderer) << "\n";
        }
        if (a.has("settle"))
        {
            // Draw N frames standing still, a few milliseconds apart, so that what is drawn in the
            // background (the pictures on the items) has time to arrive before the picture is taken.
            const uint32_t n = a.get_positive("settle", 30);
            for (uint32_t i = 0; i < n; ++i)
            {
                hall->render();
                SDL_RenderPresent(renderer);
                SDL_Delay(10);
            }
        }
        hall->render(); // computes what you are looking at
        hall->settle_vault(); // the vault's verdicts on the pictures in view, so the picture shows them
        if (a.has("take")) hall->take_hovered();
        // K on the item in hand, waited for, and what Return would apply saved as a settings file.
        if (a.has("tailor") && !hall->tailor_now(a.get("tailor")))
            std::cerr << "--tailor: nothing tailored (no item in hand on the pages, image, audio or video line)\n";
        if (a.has("save-item")) hall->save_in_hand_to(a.get("save-item")); // F, without the dialog
        if (a.has("save-view")) hall->save_view_to(a.get("save-view"));   // F in the viewer, without the dialog
        if (a.has("sample-degrees"))
            std::cout << hall->sample_degrees(a.get("sample-degrees"), a.get("sample-out", a.get("sample-degrees") + ".sieve"));
        hall->render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        hall->release_textures();
        hall.reset(); // before SDL goes (its audio stream, its textures)
        return finish();
    }

    // Interactive: the main menu, then the setup menu, then the hallway (--no-menu goes straight
    // in). F1 in the hallway returns to the setup menu, and Esc there to the main menu.
    bool show_menu = !a.has("no-menu");
    bool settings_chosen = false; // settings came from a setup menu (F1 in game), even with --no-menu
    bool went_in_thin = false;    // the setup menu was told to go in past the budget
    bool show_main = show_menu;
    bool first = true;
    std::optional<MusicMelody> pending_melody; // a melody of the music to go to in the next hallway built
    std::optional<Space::Digits> pending_unit; // an item to hold on its COST tab there (COST's tailoring)
    while (true)
    {
        if (show_main)
        {
            MainMenu mm(window, renderer, app, app_path, display);
            const MainMenu::Result r = mm.run();
            if (r == MainMenu::Result::Quit) break;
            if (r == MainMenu::Result::Locator)
            {
                // The File Locator, and back to the main menu from it.
                if (!run_locator(window, renderer, settings, filters, filters_path)) break;
                continue;
            }
            if (r == MainMenu::Result::Designer)
            {
                // The filter designer, and back to the main menu from it.
                Designer designer(window, renderer);
                if (designer.run() == Designer::Result::Quit) break;
                continue;
            }
            show_main = false;
        }
        if (show_menu)
        {
            Menu menu(window, renderer, settings, filters, filters_path, &app, app_path);
            const Menu::Result r = menu.run();
            if (r == Menu::Result::Quit) break;
            went_in_thin = menu.went_in_thin();
            settings = menu.settings();
            filters = menu.filters();
            if (r == Menu::Result::Back)
            {
                show_main = true;
                continue;
            }
        }
        Args ha = a;
        if (show_menu || settings_chosen) settings.apply(ha);
        if (!first)
            for (const char* k : {"warp", "goto", "zoom", "tile", "pose", "walk", "press"}) ha.opts.erase(k);
        // The hallway is built on a worker thread -- a large shape takes a while, and none of
        // building it touches the renderer -- while this thread keeps the window alive and says
        // it is still working: a red CALCULATING, its dots counting up and starting again. It
        // only appears if the building takes long enough to be noticed.
        bool quit_while_building = false;
        music_mode(MusicMode::Menus); // building is still the menus
        music_colours_default();
        // The hallway counts with the filter memory as set, pressed X or not (and the setup menu,
        // built again when it next opens, counts with it too).
        sieve::set_filter_memory(double(app.filter_memory_mb) * 1024 * 1024);
        sieve::set_unit_time_ms(app.unit_time_ms); // (and the time budget, as set)
        set_view(app.view_rooms, app.picture_rooms); // (and the view distance, as Settings > Graphics has it)
        auto building = std::async(std::launch::async, [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, ha, first, filters, app.angle_decimals);
        });
        const Uint64 building_since = SDL_GetTicks();
        while (building.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                if (e.type == SDL_EVENT_QUIT) quit_while_building = true;
            const Uint64 waited = SDL_GetTicks() - building_since;
            if (waited < 200) continue;
            int ww = 0, wh = 0;
            SDL_GetCurrentRenderOutputSize(renderer, &ww, &wh);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            const float scale = 4.0f;
            const std::string word = tr("calculating");
            // Centred on the word alone, so the dots grow to the right and it does not jump.
            const float x = (float(ww) - float(word.size()) * 8.0f * scale) * 0.5f, y = (float(wh) - 8.0f * scale) * 0.5f;
            draw_text(renderer, x, y, word + std::string(size_t((waited / 400) % 4), '.'), scale, SDL_Color{255, 60, 60, 255});
            present(renderer);
        }
        auto hall = building.get(); // rethrows here anything the building threw
        if (quit_while_building) break;
        hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
        hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
        hall->set_model_cache(app.model_cache_mb);
        hall->set_angle_decimals(app.angle_decimals);
        hall->set_face_px(ha.get_u32("model-tile", 64));
        hall->set_letters_px(ha.has("item-letters") ? ha.get_u32("item-letters", 8) : 8);
        hall->set_export_fps(ha.has("video-fps") ? ha.get_u32("video-fps", sieve::cli::kDefaultExportFps) : sieve::cli::kDefaultExportFps);
        hall->set_closeup_px(closeup_setting(ha.get("close-up", "screen"), kCloseUpScreen));
        hall->set_graphics_memory(app.graphics_memory_gb);
        hall->set_fps_counter(app.fps_counter);
        hall->set_thin(went_in_thin || ha.has("thin"));
        first = false;
        if (pending_melody)
        {
            hall->go_to_melody(*pending_melody);
            pending_melody.reset();
        }
        if (pending_unit)
        {
            hall->hold_on_cost(*pending_unit);
            pending_unit.reset();
        }
        SDL_SetWindowRelativeMouseMode(window, true);
        Menu::Result in_game_menu = Menu::Result::Back;
        for (;;)
        {
            hall->music_line(); // back from a menu (or in for the first time): the line's colours and character
            bool quit = false;
            Uint64 last = SDL_GetTicksNS();
            while (!quit)
            {
                SDL_Event e;
                while (SDL_PollEvent(&e)) hall->handle(e, quit);
                const Uint64 now = SDL_GetTicksNS();
                const float dt = std::min(0.1f, float(now - last) / 1e9f);
                last = now;
                {
                    sieve::cli::timings::Scope timed("hallway.frame.update");
                    hall->update(dt, SDL_GetKeyboardState(nullptr));
                }
                {
                    sieve::cli::timings::Scope timed("hallway.frame.render");
                    hall->render();
                }
                music_mode(hall->in_menu() ? MusicMode::Menus : MusicMode::World);
                {
                    sieve::cli::timings::Scope timed("hallway.frame.present"); // waits for vsync when it is on
                    present(renderer);
                }
            }
            // F1: the setup menu over the hallway, which is kept. Esc there comes back to it just
            // as it was; ENTER THE HALLWAY builds a new one from the new settings.
            if (hall->menu_requested())
            {
                Menu menu(window, renderer, settings, filters, filters_path, &app, app_path);
                menu.set_in_game(true);
                in_game_menu = menu.run();
                if (in_game_menu == Menu::Result::Back)
                {
                    hall->set_model_cache(app.model_cache_mb);
                    hall->set_graphics_memory(app.graphics_memory_gb);
                    SDL_SetWindowRelativeMouseMode(window, !hall->back_from_menu());
                    continue;
                }
                if (in_game_menu == Menu::Result::Enter)
                {
                    went_in_thin = menu.went_in_thin();
                    settings = menu.settings();
                    filters = menu.filters();
                }
                break;
            }
            // Settings from the pause menu: the main menu's screens over the hallway, and then
            // the same hallway again, still paused, with what was changed applied to it.
            if (hall->request() != Hallway::Request::Settings) break;
            hall->clear_request();
            MainMenu mm(window, renderer, app, app_path, display);
            if (mm.run_settings() == MainMenu::Result::Quit) break;
            hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
            hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
            hall->set_fps_counter(app.fps_counter);
            hall->settings_changed();
            SDL_SetWindowRelativeMouseMode(window, false);
        }
        if (hall->request() == Hallway::Request::GoToMelody && hall->melody_to_go())
        {
            // A melody of the music on an audio line of another length or note set (or a track of
            // another number of units): the hallway again with that audio line (and tracks), on
            // the audio or tracks line, and there the melody.
            const MusicMelody melody = *hall->melody_to_go();
            const sieve::NoteSet& set = melody.set;
            const uint32_t units = std::max<uint32_t>(1, melody.units);
            settings.notes = uint32_t(melody.notes.size() / std::max<uint32_t>(1, set.voices) / units);
            if (melody.units) settings.track_units = melody.units;
            settings.note_set = set.legacy ? "notes104" : "notes2";
            if (!set.legacy)
            {
                settings.note_low = sieve::note_name(set.low);
                settings.note_high = sieve::note_name(set.high);
                settings.note_durations = set.durations;
                settings.voices = set.voices;
            }
            settings.start_line = melody.units ? "tracks" : "audio";
            pending_melody = melody;
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::Tailored && hall->tailored_filters())
        {
            // Tailored filters (Return on COST, or the File Locator's "use"): kept, as the setup
            // menu keeps what it ticks, and a hallway built with them on the same line, with the
            // item from COST in hand again on its COST tab.
            filters = *hall->tailored_filters();
            try
            {
                filters.save(filters_path);
            }
            catch (const std::exception&)
            {
                // Read-only folder: the filters still apply to this session.
            }
            settings.start_line = hall->line_name();
            pending_unit = hall->tailored_unit();
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::MainMenu)
        {
            // Exit Sieve, then Y: back to the main menu, and from there the setup menu as ever.
            show_main = show_menu = true;
            continue;
        }
        if (!hall->menu_requested()) break;
        if (in_game_menu == Menu::Result::Quit) break;
        // ENTER THE HALLWAY from the F1 menu: a new hallway from its settings, without the menu again.
        show_menu = false;
        settings_chosen = true;
    }
    return finish();
}

} // namespace

int main(int argc, char** argv)
{
    try
    {
        // Reuse the sieve argument parser: it expects a command first.
        std::vector<char*> args(argv, argv + argc);
        std::string command = "hallway";
        args.insert(args.begin() + 1, command.data());
        const Args a = parse_args(int(args.size()), args.data());
        if (a.help)
        {
            std::cout << kUsage;
            return 0;
        }
        const int status = run(a);
        // A menu count still running at exit (a large stack in a Debug build can take minutes)
        // would use statics that are being destroyed, and waiting for it would hold the program
        // open: so leave at once. Everything is saved as it changes, and the disk cache is written
        // by renaming a finished file, so nothing is left half done.
        if (filter_workers_busy())
        {
            std::cout.flush();
            std::cerr.flush();
            std::_Exit(status);
        }
        finish_filter_warmup();
        return status;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n\n" << kUsage;
        std::cerr.flush();
        if (filter_workers_busy()) std::_Exit(1);
        finish_filter_warmup();
        return 1;
    }
}
