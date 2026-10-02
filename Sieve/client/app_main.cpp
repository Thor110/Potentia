// Sieve hallway -- the program: its options, the menus, the screenshot and scripting paths used by
// the checks, and the event loop. The hallway itself is in hallway.hpp and the files beside it.

#include "window_icon.hpp"
#include "cli/plugins.hpp"
#include "designer.hpp"
#include "cli/timings.hpp"
#include "hallway.hpp"

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
    "  --line pages|image|audio|video|books|models|binary  line to start in (default pages)\n"
    "  --length L          text: characters per book (default 32)\n"
    "  --alphabet ID       text: lower27 (default), babel29, ascii95\n"
    "  --canon v2|v1       text: warp rules (default v2)\n"
    "  --image-width W  --image-height H  --image-palette ID    image line (10, 10, mono)\n"
    "  --notes N           audio: notes per book (default 16; per voice on notes2)\n"
    "  --note-set notes104|notes2  --note-low C3  --note-high C6  --note-durations seEqQhHw  --voices 1..4\n"
    "                      audio: the note set (default notes104; the rest are notes2's)\n"
    "  --video-width W  --video-height H  --video-frames F  --video-palette ID   video (5, 5, 8, mono)\n"
    "  --title-length T    every line's titles: characters (default 32; 0: no titles; books keep a page)\n"
    "  --binary-length N   binary: every file of up to N bytes (default 32)\n"
    "  --book-pages N      books: pages per book (default 4); a book is a cover (an image of the\n"
    "                      image line), a title and N pages (pages of the pages line)\n"
    "  --key K             scramble key (default sieve)\n"
    "  --mode positional|scrambled|guided   starting ordering (default positional)\n"
    "  --model ID|PATH|none  text: model for the guided ordering (default: the alphabet's default)\n"
    "  --zoom D            guided: books 2^-D of the line apart (after --warp/--goto set it)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, a picture file for image/video, a .book file for books)\n"
    "  --goto ADDR|P%|@T   go to an address, a percentage, or corridor tile T on start\n\n"
    "Menu:\n"
    "  The main menu opens first: Start Sieve, Settings (graphics, controls, language; saved to\n"
    "  sieve-hallway.ini), the Filter Designer (make filter plugins as nodes) and Exit Sieve. Start Sieve opens the setup menu (Esc: back to the\n"
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
    "  --save-item PATH    then save it as a file (F on the item page), to PATH\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n"
    "  --press K,K,...     then press these keys (e.g. M,M,-,Shift+=), printing where you are\n"
    "  --edge-glow         draw with Geometry Edge Glow (or --settings a file that has it on)\n"
    "  --real-graphics     draw with Real Graphics: the models in the meshes folder\n"
    "  --door-portals      fill the doorways with procedural data noise (Real Graphics turns this on)\n"
    "  --model-cache MB    display cache: memory for the pictures on items (8-4096, default 64)\n"
    "  --model-tile PX     display size: how wide each item's picture is drawn (16-1024, default 64)\n"
    "  --item-letters PX   letter size on item displays: pages and titles are drawn wide enough\n"
    "                      for it, and smaller letters are dashes (default 8)\n"
    "  --close-up PX       the displays nearest you are drawn again up to this wide (0: off;\n"
    "                      256, 512 or 1024, the default)\n"
    "  --items-per-wall N  units on one tile of the corridor (128 or 256; changes no address,\n"
    "                      only the tile and slot that name a unit's place in the corridor)\n"
    "  --fps-counter       show the FPS counter\n"
    "  --bench N           before the screenshot, time N frames and print the frame rate\n"
    "  --settle N          before the screenshot, draw N frames standing still, so the item pictures arrive\n"
    "  --locate PATH       open the File Locator on a file or folder\n"
    "  --install FILE --install-to DIR   the File Locator's install, at once\n"
    "  --map PATH          choose a map for the node graph: a .map file, or a folder to map\n"
    "  --graph             open the node graph (on --map, or on this installation)\n"
    "  --new-map PATH      the node graph's New map..., at once\n"
    "  --thin              keep only the room you stand in (what going in past the budget does)\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering (positional, scrambled, guided), - and = zoom out/in (guided; Shift: 8x),\n"
    "wheel/PgUp/PgDn/[ ] jump 1/1000/1000000 tiles, Home to corridor tile 0 (every line's start line),\n"
    "N/B next/previous unit of a warped trail (or page of a book in hand), P play an audio book, F save the item in hand as a file, F1 the setup menu, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit.\n\n"
    "All five lines share one corridor, 128 books per tile. Each line repeats along it; a\n"
    "checkered start line marks where each repeat begins. Black doors lead to the next line\n"
    "(left wall) or the previous line (right wall) at the same corridor position.\n";

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
            if (a.get("note-set", "notes104") == "notes2")
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

// "M,M,-,Shift+=" -> key presses.
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
        const SDL_Keycode key = name == "-" ? SDLK_MINUS : name == "=" ? SDLK_EQUALS : SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN) throw std::invalid_argument("--press: unknown key '" + name + "'");
        out.emplace_back(key, mod);
        start = end + 1;
    }
    return out;
}

// Builds the hallway from options, and applies the start-up and scripting options.
std::unique_ptr<Hallway> make_hallway(SDL_Window* window, SDL_Renderer* renderer, const Args& a, bool scripted,
                                      const FilterConfig& filters)
{
    // The corridor's tile size, before any line's loop is worked out from it. It changes no
    // address: only the tile and slot that name a unit's place in the corridor (corridor.hpp).
    if (a.has("items-per-wall")) sieve::set_books_per_tile(a.get_u32("items-per-wall", 128));
    std::vector<Line> lines = make_lines(a);
    int start_line = 0;
    if (a.get("line") == "books") start_line = kBooksLine;
    else if (a.get("line") == "models") start_line = kModelsLine;
    else if (a.get("line") == "binary") start_line = kBinaryLine;
    else
    {
        const LineKind wanted = line_from_string(a.get("line", "text")); // "pages" is the text line
        for (int i = 0; i < 4; ++i)
            if (wanted == kLineOrder[i]) start_line = i;
    }
    const bool text_has_model = lines[0].guided != nullptr;
    // Books and models are not made of one alphabet, so neither starts with the text greeting.
    const LineKind start_kind = start_line == kBooksLine || start_line == kModelsLine || start_line == kBinaryLine
                                    ? LineKind::Image
                                    : lines[size_t(start_line)].kind;

    const Hallway::ModelShape shape{a.get_positive("vertices", 8), a.get_positive("faces", 12), a.get_positive("coords", 16),
                                    a.has("title-length") ? a.get_u32("title-length", 32) : 32u,
                                    a.has("binary-length") ? a.get_positive("binary-length", 32) : 32u};
    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines), filters,
                                          a.has("book-pages") ? a.get_u32("book-pages", 4) : 4, shape);
    hall->set_line(start_line);
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
    if (a.has("locate")) hall->locate_now(a.get("locate"));
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
        finish_filter_warmup();     // a picture shows the counts, not "counting..."
        menu.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot)
    {
        auto hall = [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, a, true, filters);
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
            hall->set_closeup_px(a.has("close-up") ? a.get_u32("close-up", 1024) : 1024);
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
        if (a.has("take")) hall->take_hovered();
        if (a.has("save-item")) hall->save_in_hand_to(a.get("save-item")); // F, without the dialog
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
    std::vector<uint32_t> pending_track; // a music track to go to in the next hallway built
    sieve::NoteSet pending_set;          // and its note set
    while (true)
    {
        if (show_main)
        {
            MainMenu mm(window, renderer, app, app_path, display);
            const MainMenu::Result r = mm.run();
            if (r == MainMenu::Result::Quit) break;
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
        auto building = std::async(std::launch::async, [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, ha, first, filters);
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
        hall->set_closeup_px(ha.has("close-up") ? ha.get_u32("close-up", 1024) : 1024);
        hall->set_fps_counter(app.fps_counter);
        hall->set_thin(went_in_thin || ha.has("thin"));
        first = false;
        if (!pending_track.empty())
        {
            hall->go_to_track(pending_set, pending_track);
            pending_track.clear();
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
        if (hall->request() == Hallway::Request::GoToTrack)
        {
            // A music track on an audio line of another length: the hallway again at that length,
            // on the audio line, and there the track.
            const std::vector<uint32_t> track = hall->track_to_go();
            const sieve::NoteSet set = hall->track_set_to_go();
            settings.notes = uint32_t(track.size() / std::max<uint32_t>(1, set.voices));
            settings.note_set = set.legacy ? "notes104" : "notes2";
            if (!set.legacy)
            {
                settings.note_low = sieve::note_name(set.low);
                settings.note_high = sieve::note_name(set.high);
                settings.note_durations = set.durations;
                settings.voices = set.voices;
            }
            settings.start_line = "audio";
            pending_track = track;
            pending_set = set;
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
