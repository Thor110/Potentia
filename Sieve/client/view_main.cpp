// sieve-view: Sieve's picture viewer. One window, a picture in it, and the pictures beside it.
//
// It opens the usual pictures (PNG, JPEG, BMP, GIF, TGA: whatever stb_image reads) and Sieve's own:
// a .simage (Sieve instructions holding only pictures, as a .sfilter is a filter), or any .sieve
// whose files are all pictures. The other pictures in the same folder come next and
// before (Right and Left), and a .sieve holding several shows each in turn. "Sieve" makes the
// picture shown into Sieve's picture form beside it (NAME.simage, by `sieve locate --installer
// --prioritise-decoding`, so it opens as quickly as the picture did); "Export" writes the picture
// as a PNG; from a .sieve, "Save file" writes the picture's own file as it was carried.
//
// A .sieve may hold anything, under any name: gigabytes that pack to nothing, a file named .simage
// that is not a picture. Before anything is decoded (IDEAS, the picture viewer):
//   - the head and the listing are read first, nothing decoded (cli/locate.hpp listing_of_installer):
//     every file's name, its size as declared and its recipe, which says what it is (jbr a JPEG;
//     stb, skr, pngr a PNG); a file that is not a picture by its recipe or its name, more than
//     kMaxFiles files, or files coming to more than half the memory free, and the whole is refused
//     there, with why;
//   - installing then stops at each declared size and checks every file's SHA-256 before it is
//     written (cli/locate.hpp install_tree), into a folder of its own that is removed after;
//   - each picture's header is read (its width and height, without decoding), and its pixels made
//     only where they fit in half the memory free (and in what stb_image decodes: 2 GB of a JPEG's,
//     1 GiB of a PNG's). A JPEG too large is shown at an eighth instead, from its blocks' averages
//     alone (core's jpeg_eighth: a sixty-fourth of the memory); any other is refused, with why.
// An ordinary picture has its header checked the same way. A .sieve's pictures are made one at a
// time, as each is shown (Edward, 2 November 2026: "what happens with a real 300mb image? Rare but
// they exist").
//
// Keys: Right or Space the next picture, Left or Backspace the one before, Home and End the first
// and the last; the wheel zooms about the pointer, + and - zoom, 0 fits it to the window, 1 is one
// to one; dragging or the arrows with Shift move it; E exports, S makes the .simage, F saves the
// file from a .sieve; Esc or Q closes. A picture or a .sieve dropped on the window opens it.
//
// For testing without a person: --screenshot FILE.bmp writes the window once the picture is shown
// and exits (with --zoom Z first), --make-simage makes the .simage at once and exits (0 on success),
// --check says what it would show (each picture's name and size, or why it is refused) and exits;
// --budget BYTES takes that as the memory free (to try the limits on small pictures).

#include "picture_source.hpp"
#include "thumbs_check.hpp"
#include "cli/image_io.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "cli/pack.hpp"
#include "cli/vault.hpp"
#include "sieve/jpeg_dc.hpp"
#include "dialogs.hpp"
#include "window_icon.hpp"
#include "win_console.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

namespace {

namespace fs = std::filesystem;
using namespace sieve;
using namespace sieve::cli;
using namespace sieve::view;

// One place in the folder: a picture file, or a .sieve (its pictures read when it is reached).
struct Place
{
    fs::path path;
    bool sieve = false;
};

std::vector<Place> places_beside(const fs::path& opened)
{
    std::vector<Place> out;
    std::error_code ec;
    const fs::path dir = opened.parent_path().empty() ? fs::current_path(ec) : opened.parent_path();
    for (fs::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
    {
        if (!it->is_regular_file(ec)) continue;
        const std::string name = u8(it->path().filename());
        const std::string e = lower(it->path().extension().string());
        // (a plain .sieve may hold anything: only the one opened is taken, and each .simage)
        if (picture_name(name) || e == ".simage") out.push_back({it->path(), sieve_name(name)});
    }
    if (std::none_of(out.begin(), out.end(), [&](const Place& p) { return fs::equivalent(p.path, opened, ec); }))
        out.push_back({opened, sieve_name(u8(opened.filename()))});
    std::sort(out.begin(), out.end(), [](const Place& a, const Place& b) { return lower(u8(a.path.filename())) < lower(u8(b.path.filename())); });
    return out;
}

// ---- drawing: SDL's own 8x8 font, scaled, so the program needs no files of its own (as sieve-install)
const SDL_Color kInk{235, 235, 235, 255}, kDim{150, 150, 150, 255}, kAccent{32, 220, 80, 255}, kRed{255, 110, 110, 255};

void text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c)
{
    SDL_SetRenderScale(r, scale, scale);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
    SDL_SetRenderScale(r, 1, 1);
}

std::string tail(const std::string& s, size_t chars) { return s.size() <= chars ? s : "..." + s.substr(s.size() - (chars - 3)); }

struct Button
{
    SDL_FRect r;
    std::string label;
    char key;
    bool enabled = true;
};

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

constexpr float kBar = 34, kFoot = 22;

void save_bmp(SDL_Renderer* r, const std::string& path)
{
    if (SDL_Surface* s = SDL_RenderReadPixels(r, nullptr))
    {
        SDL_SaveBMP(s, path.c_str());
        SDL_DestroySurface(s);
    }
}

// The `sieve` program beside this one (or on the path): what makes a .simage.
std::optional<fs::path> sieve_program(const char* argv0)
{
    const fs::path self = own_executable(argv0);
    for (const char* name : {"sieve.exe", "sieve"})
    {
        std::error_code ec;
        const fs::path p = self.parent_path() / name;
        if (fs::is_regular_file(p, ec)) return p;
    }
    return std::nullopt;
}

// NAME.simage beside the picture, made by `sieve locate PICTURE --installer NAME.simage
// --prioritise-decoding`; what it said, and whether it worked.
std::pair<bool, std::string> make_simage(const fs::path& sieve, const fs::path& picture, const fs::path& out)
{
    const std::string s = u8(sieve), p = u8(picture), o = u8(out);
    const char* args[] = {s.c_str(), "locate", p.c_str(), "--installer", o.c_str(), "--prioritise-decoding", nullptr};
    // Its output and its errors in one pipe, its input none, and in the background (5 November 2026,
    // Edward on Windows: "could not run sieve: DuplicateHandle(): The handle is invalid"). The viewer
    // is a windowed program with no console, so there was no error stream of its own to hand on; and
    // in the background no console window opens for `sieve`. A background process's exit code is not
    // kept (SDL gives 0), so whether it worked is read from what it said: its "installer" line, which
    // it writes only once the file is made, and no error.
    const SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetPointerProperty(props, SDL_PROP_PROCESS_CREATE_ARGS_POINTER, const_cast<char**>(args));
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDIN_NUMBER, SDL_PROCESS_STDIO_NULL);
    SDL_SetNumberProperty(props, SDL_PROP_PROCESS_CREATE_STDOUT_NUMBER, SDL_PROCESS_STDIO_APP);
    SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_STDERR_TO_STDOUT_BOOLEAN, true);
    SDL_SetBooleanProperty(props, SDL_PROP_PROCESS_CREATE_BACKGROUND_BOOLEAN, true);
    SDL_Process* proc = SDL_CreateProcessWithProperties(props);
    SDL_DestroyProperties(props);
    if (!proc) return {false, std::string("could not run sieve: ") + SDL_GetError()};
    size_t got = 0;
    int code = -1;
    char* said = static_cast<char*>(SDL_ReadProcess(proc, &got, &code));
    std::string all = said ? std::string(said, got) : std::string();
    SDL_free(said);
    SDL_DestroyProcess(proc);
    std::string last;
    bool made = false, failed = false;
    for (size_t at = 0; at < all.size();)
    {
        const size_t nl = all.find('\n', at);
        std::string line = all.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("installer ", 0) == 0) made = true, last = line;
        else if (line.rfind("error", 0) == 0 || line.rfind("sieve:", 0) == 0) failed = true, last = line;
        if (nl == std::string::npos) break;
        at = nl + 1;
    }
    std::error_code ec;
    return {made && !failed && fs::is_regular_file(out, ec), last};
}

const char* const kUsage =
    "sieve-view - Sieve's picture viewer\n"
    "\n"
    "Usage: sieve-view [PICTURE | NAME.simage | NAME.sieve] [options]\n"
    "\n"
    "Opens a picture (PNG, JPEG, BMP, GIF, TGA) or Sieve instructions holding only pictures (.simage,\n"
    "or a .sieve whose files are all pictures), with the other pictures in its folder next\n"
    "and before. A .sieve's listing is read first and refused where a file is not a picture by its\n"
    "recipe or name, or over the limits (4,096 files; half the memory free in all); each picture's\n"
    "header is read before its pixels are made, and they are made only where they fit in half the\n"
    "memory free. A JPEG too large is shown at an eighth, from its blocks' averages alone. An\n"
    "animated GIF plays, each frame for its own delay, where all its frames fit there (else its\n"
    "first is shown, and why).\n"
    "\n"
    "Keys: Right/Left next and before, wheel or +/- zoom, 0 fit, 1 one to one, drag to move,\n"
    "E export as PNG (an animated GIF's frame shown), S make NAME.simage beside it (sieve locate\n"
    "--prioritise-decoding), P pause or play a GIF, comma and full stop a frame back or on,\n"
    "F save the picture's own file from a .sieve, Esc or Q close.\n"
    "\n"
    "Options (for testing):\n"
    "  --check              say what it would show (each picture, or why it is refused), and exit\n"
    "  --make-simage        make NAME.simage beside the picture and exit (0 on success)\n"
    "  --thumbnail SIDE OUT.png  write the picture as Explorer's thumbnail shows it (its longer\n"
    "                       side at most SIDE; sieve-thumbs.dll) and exit (0 on success)\n"
    "  --thumbnail-check    on Windows, each step Explorer takes for the file's thumbnail, and\n"
    "                       which one fails (0 when sieve-thumbs.dll gave one)\n"
    "  --zoom Z            start at this zoom (1 is one to one) rather than fitted\n"
    "  --frame K            an animated GIF paused at frame K (from 1), as for --screenshot\n"
    "  --budget BYTES       take this as the memory it may use, not half of what is free\n"
    "  --screenshot FILE    write the window once the picture is shown, as a BMP, and exit\n"
    "  --help               this text\n";

} // namespace

int main(int argc, char** argv)
{
    std::string file_arg, shot;
    bool check = false, make = false, thumb_check = false;
    uint32_t thumb_side = 0; // (--thumbnail SIDE OUT.png)
    std::string thumb_out;
    double start_zoom = 0;
    long start_frame = 0; // (--frame K: an animated GIF paused at frame K, from 1)
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--help" || a == "-h")
        {
            hallway::attach_parent_console();
            std::fputs(kUsage, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (a == "--screenshot" && i + 1 < argc) shot = argv[++i];
        else if (a == "--zoom" && i + 1 < argc) start_zoom = std::atof(argv[++i]);
        else if (a == "--frame" && i + 1 < argc) start_frame = std::max(1L, std::strtol(argv[++i], nullptr, 10));
        else if (a == "--budget" && i + 1 < argc) set_memory_budget(std::strtoull(argv[++i], nullptr, 10));
        else if (a == "--check") check = true;
        else if (a == "--make-simage") make = true;
        else if (a == "--thumbnail-check") thumb_check = true;
        else if (a == "--thumbnail" && i + 2 < argc)
        {
            thumb_side = uint32_t(std::strtoul(argv[++i], nullptr, 10)), thumb_out = argv[++i];
            if (thumb_side == 0) // (else it would open the window instead: review, 6 November 2026)
            {
                hallway::attach_parent_console();
                std::fputs("sieve-view: --thumbnail needs a side of 1 or more pixels\n", stderr);
                return 2;
            }
        }
        else file_arg = a;
    }
    // Every file Sieve's instructions can carry made again (a JPEG from its brunsli form, a PNG by
    // stb_image_write ...), as sieve install does.
    sieve::cli::use_item_maker();
    // (a picture's frames, decoded by the vault to check them as a .sieve is installed, handed on
    // to be shown rather than decoded again; no more than the viewer itself may hold)
    sieve::cli::vault::keep_checked_frames(true, memory_budget() / 2);

    // The pictures of one place: an ordinary picture, or every picture of a .sieve, the pixels made
    // of the one to be shown first only (the first, or coming back to it, the last).
    // (all_frames: an animated GIF's every frame, as the window plays it; a thumbnail takes the first)
    auto load = [](const Place& at, std::string& said, bool backwards, bool all_frames = true) {
        std::vector<Picture> out;
        if (at.sieve) out = pictures_of_sieve(at.path, &said);
        else
        {
            Picture p;
            p.name = u8(at.path.filename());
            p.file = read_all(at.path);
            out.push_back(std::move(p));
        }
        // (coming back to a .sieve, its last is shown: the first's frames, kept by the vault, let go first)
        if (backwards && out.size() > 1)
        {
            sieve::cli::vault::keep_checked_frames(false);
            sieve::cli::vault::keep_checked_frames(true, memory_budget() / 2);
        }
        make_pixels(out[backwards ? out.size() - 1 : 0], all_frames);
        // (what the vault kept of the pictures it checked, given up: taken above where it was the one shown)
        sieve::cli::vault::keep_checked_frames(false);
        sieve::cli::vault::keep_checked_frames(true, memory_budget() / 2);
        return out;
    };

    if (check || make || thumb_side || thumb_check)
    {
        hallway::attach_parent_console();
        if (file_arg.empty())
        {
            std::fputs("sieve-view: name a picture or a .sieve\n", stderr);
            return 2;
        }
        const fs::path f = from_u8(file_arg);
        if (thumb_check) return thumbnail_check(f);
        if (make)
        {
            const auto sieve = sieve_program(argc > 0 ? argv[0] : nullptr);
            if (!sieve)
            {
                std::fputs("sieve-view: the sieve program is not beside this one\n", stderr);
                return 1;
            }
            fs::path out = f;
            out.replace_extension(".simage");
            const auto [ok, said] = make_simage(*sieve, f, out);
            std::printf("%s\n", said.c_str());
            return ok ? 0 : 1;
        }
        if (thumb_side)
        {
            // The thumbnail Explorer is given (sieve-thumbs.dll), as a PNG: the first picture, its
            // longer side at most SIDE (shrink_to; the DLL then hands Windows the same pixels).
            try
            {
                std::string said;
                std::vector<Picture> got = load(Place{f, sieve_name(u8(f.filename()))}, said, false, false);
                const RgbaImage t = shrink_to(got.front().pixels, thumb_side);
                const std::string png = stb_png(t.rgba, int(t.width), int(t.height), 4, 8, -1);
                std::ofstream o(from_u8(thumb_out), std::ios::binary);
                o.write(png.data(), std::streamsize(png.size()));
                if (!o) throw std::runtime_error("cannot write " + thumb_out);
                std::printf("thumbnail: %u by %u pixels, of %s (%u by %u)\n", t.width, t.height, got.front().name.c_str(), got.front().width, got.front().height);
                return 0;
            }
            catch (const std::exception& x)
            {
                std::printf("refused: %s\n", x.what());
                return 1;
            }
        }
        try
        {
            std::string said;
            std::vector<Picture> got = load(Place{f, sieve_name(u8(f.filename()))}, said, false);
            if (!said.empty()) std::printf("listing: %s\n", said.c_str());
            for (Picture& p : got)
            {
                make_pixels(p, true); // (one at a time, each let go before the next)
                std::string frames;
                if (!p.more.empty())
                {
                    uint64_t ms = 0;
                    for (const int d : p.delays_ms) ms += uint64_t(d < 20 ? 100 : d);
                    frames = ", " + std::to_string(p.more.size() + 1) + " frames played, " + std::to_string(ms) + " ms in all";
                }
                else if (!p.one_frame.empty()) frames = ", its first frame of " + std::to_string(p.frames) + " shown: " + p.one_frame;
                std::printf("%s: %u by %u pixels, %s%s%s\n", p.name.c_str(), p.width, p.height, human_bytes(p.file.size()).c_str(),
                            p.scale == 8 ? (", shown at an eighth (" + std::to_string(p.pixels.width) + " by " + std::to_string(p.pixels.height) + "): " + p.reduced).c_str() : "",
                            frames.c_str());
                p.pixels = RgbaImage{};
                p.more.clear();
                p.made = false;
            }
            return 0;
        }
        catch (const std::exception& e)
        {
            std::printf("refused: %s\n", e.what());
            return 1;
        }
    }

    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("sieve-view: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = nullptr;
    SDL_Renderer* r = nullptr;
    if (!SDL_CreateWindowAndRenderer("Sieve Viewer", 1024, 720, SDL_WINDOW_RESIZABLE, &window, &r))
    {
        SDL_Log("sieve-view: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(r, 1);
    set_window_icon(window);
    const int max_texture = int(SDL_GetNumberProperty(SDL_GetRendererProperties(r), SDL_PROP_RENDERER_MAX_TEXTURE_SIZE_NUMBER, 8192));

    // The state: the folder's places, the one shown and its pictures, and the view.
    std::vector<Place> places;
    size_t place = 0, shown = 0;
    std::vector<Picture> pics;
    std::string message, listing_said;
    bool message_bad = false;
    SDL_Texture* tex = nullptr;
    int tex_w = 0, tex_h = 0, tex_step = 1; // (a picture larger than a texture is shown at 1/step)
    double zoom = 1, cx = 0, cy = 0;          // zoom (screen pixels a picture pixel), the picture's point at the view's centre
    bool fitted = true;
    // (An animated GIF played: the frame shown, whether it plays, and when the next is due, by
    // SDL's clock in milliseconds.)
    size_t frame = 0;
    bool playing = true;
    uint64_t next_frame_at = 0;
    auto animated = [&] { return !pics.empty() && pics[shown].made && !pics[shown].more.empty(); };
    // (A frame's delay as the GIF gives it; under 20 ms, which browsers too take as unmeant, 100.)
    auto delay_of = [&](size_t k) {
        const std::vector<int>& d = pics[shown].delays_ms;
        const int ms = k < d.size() ? d[k] : 0;
        return uint64_t(ms < 20 ? 100 : ms);
    };

    // (Loading on a thread of its own, so a large .sieve does not stop the window.)
    struct Loading
    {
        std::thread t;
        std::atomic<bool> done{false};
        std::vector<Picture> got;
        std::string said, error;
        bool backwards = false;
    };
    std::unique_ptr<Loading> loading;
    auto start_loading = [&](size_t at, bool backwards) {
        if (loading && loading->t.joinable()) loading->t.join();
        loading = std::make_unique<Loading>();
        loading->backwards = backwards;
        place = at;
        Loading* l = loading.get();
        const Place p = places[at];
        l->t = std::thread([l, p, &load] {
            try
            {
                l->got = load(p, l->said, l->backwards);
            }
            catch (const std::exception& e)
            {
                l->error = e.what();
            }
            l->done = true;
        });
        message = "opening " + u8(p.path.filename()) + " ...";
        message_bad = false;
    };

    auto view_w = [&] {
        int w = 0, h = 0;
        SDL_GetRenderOutputSize(r, &w, &h);
        return std::pair<float, float>(float(w), float(h) - kBar - kFoot);
    };
    auto fit = [&] {
        if (pics.empty() || !pics[shown].made) return;
        const auto [vw, vh] = view_w();
        const Picture& pic = pics[shown];
        zoom = std::min(1.0, std::min(double(vw) / pic.width, double(vh) / pic.height));
        cx = pic.width / 2.0, cy = pic.height / 2.0;
        fitted = true;
    };
    // (the frame shown put into the texture, which is already its size)
    auto fill = [&] {
        if (!tex) return;
        const RgbaImage& im = pics[shown].frame(frame);
        if (tex_step == 1) SDL_UpdateTexture(tex, nullptr, im.rgba.data(), int(im.width) * 4);
        else
        {
            // (each texel one picture pixel of a step-by-step block: enough to see it whole)
            std::vector<uint8_t> small(size_t(tex_w) * tex_h * 4);
            for (int y = 0; y < tex_h; ++y)
                for (int x = 0; x < tex_w; ++x)
                    std::copy_n(&im.rgba[(size_t(y) * tex_step * im.width + size_t(x) * tex_step) * 4], 4, &small[(size_t(y) * tex_w + x) * 4]);
            SDL_UpdateTexture(tex, nullptr, small.data(), tex_w * 4);
        }
    };
    auto upload = [&] {
        if (tex) SDL_DestroyTexture(tex), tex = nullptr;
        if (pics.empty() || !pics[shown].made) return;
        const RgbaImage& im = pics[shown].pixels;
        tex_step = 1;
        while ((int(im.width) + tex_step - 1) / tex_step > max_texture || (int(im.height) + tex_step - 1) / tex_step > max_texture) ++tex_step;
        tex_w = int((im.width + tex_step - 1) / tex_step), tex_h = int((im.height + tex_step - 1) / tex_step);
        tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tex_w, tex_h);
        if (!tex) return;
        fill();
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    };
    // (to frame k of an animated GIF: shown at once, the next due after its delay)
    auto to_frame = [&](size_t k) {
        if (!animated()) return;
        frame = k % (pics[shown].more.size() + 1);
        fill();
        next_frame_at = SDL_GetTicks() + delay_of(frame);
    };
    auto show = [&](size_t k) {
        // (one picture's pixels held at a time: the others let go, this one made if it is not)
        for (size_t j = 0; j < pics.size(); ++j)
            if (j != k && pics[j].made) pics[j].pixels = RgbaImage{}, pics[j].more.clear(), pics[j].made = false;
        try
        {
            make_pixels(pics[k], true);
        }
        catch (const std::exception& e)
        {
            shown = k;
            if (tex) SDL_DestroyTexture(tex), tex = nullptr;
            message = std::string("refused: ") + e.what(), message_bad = true;
            return;
        }
        shown = k;
        message.clear();
        frame = 0, playing = true;
        upload();
        if (animated()) next_frame_at = SDL_GetTicks() + delay_of(0);
        if (start_zoom > 0 && fitted)
        {
            zoom = start_zoom, fitted = false;
            cx = pics[shown].width / 2.0, cy = pics[shown].height / 2.0;
        }
        else fit();
    };

    // What to open: the file named, else nothing (a picture dropped on the window opens it).
    auto open_path = [&](const fs::path& f) {
        std::error_code ec;
        const fs::path abs = fs::absolute(f, ec);
        places = places_beside(abs);
        size_t at = 0;
        for (size_t k = 0; k < places.size(); ++k)
            if (fs::equivalent(places[k].path, abs, ec)) at = k;
        start_loading(at, false);
    };
    if (!file_arg.empty()) open_path(from_u8(file_arg));
    else message = "drop a picture or a .simage here";

    const std::optional<fs::path> sieve = sieve_program(argc > 0 ? argv[0] : nullptr);
    bool quit = false, dragging = false;
    float drag_x = 0, drag_y = 0;
    bool shot_due = !shot.empty();
    std::mutex dialog_mx;
    int dialog_kind = 0; // 1 export, 2 save file
    std::optional<std::string> dialog_answer;

    auto step = [&](int dir) {
        if (places.empty()) return;
        if (dir > 0 && shown + 1 < pics.size()) return show(shown + 1);
        if (dir < 0 && shown > 0 && !pics.empty()) return show(shown - 1);
        if (loading && !loading->done) return;
        const size_t n = places.size();
        start_loading(dir > 0 ? (place + 1) % n : (place + n - 1) % n, dir < 0);
    };
    auto export_png = [&](const fs::path& to) {
        const RgbaImage& im = pics[shown].frame(frame); // (an animated GIF's: the frame shown)
        const std::string png = stb_png(im.rgba, int(im.width), int(im.height), 4, 8, -1);
        std::ofstream o(to, std::ios::binary);
        o.write(png.data(), std::streamsize(png.size()));
        message = o ? "exported " + u8(to) + " (" + human_bytes(png.size()) + (pics[shown].scale == 8 ? ", the eighth shown" : "") +
                          (animated() ? ", frame " + std::to_string(frame + 1) + " of " + std::to_string(pics[shown].more.size() + 1) : std::string()) + ")"
                    : "could not write " + u8(to);
        message_bad = !o;
    };
    auto save_file = [&](const fs::path& to) {
        const std::vector<uint8_t>& b = pics[shown].file;
        std::ofstream o(to, std::ios::binary);
        o.write(reinterpret_cast<const char*>(b.data()), std::streamsize(b.size()));
        message = o ? "saved " + u8(to) + " (" + human_bytes(b.size()) + ")" : "could not write " + u8(to);
        message_bad = !o;
    };
    static const SDL_DialogFileFilter kPng[] = {{"PNG picture", "png"}};
    auto ask_where = [&](int kind) {
        if (pics.empty() || !pics[shown].made) return;
        dialog_kind = kind;
        const fs::path name = from_u8(pics[shown].name).filename();
        fs::path suggest = places[place].path.parent_path() / (name.empty() ? fs::path("picture") : name);
        if (kind == 1) suggest.replace_extension(".png");
        static std::string where;
        where = u8(suggest);
        hallway::save_file_dialog(
            [](void* self, const char* const* files, int) {
                const std::unique_ptr<std::pair<std::mutex*, std::optional<std::string>*>> answer(
                    static_cast<std::pair<std::mutex*, std::optional<std::string>*>*>(self));
                std::lock_guard<std::mutex> lock(*answer->first);
                *answer->second = files && files[0] ? std::string(files[0]) : std::string();
            },
            new std::pair<std::mutex*, std::optional<std::string>*>(&dialog_mx, &dialog_answer), window, kind == 1 ? kPng : nullptr, kind == 1 ? 1 : 0,
            where.c_str());
    };
    auto make_here = [&] {
        if (pics.empty() || places[place].sieve) return;
        if (!sieve)
        {
            message = "the sieve program is not beside this one, so no .simage can be made", message_bad = true;
            return;
        }
        fs::path out = places[place].path;
        out.replace_extension(".simage");
        const auto [ok, said] = make_simage(*sieve, places[place].path, out);
        std::error_code ec;
        message = ok ? "made " + u8(out.filename()) + ": " + human_bytes(fs::file_size(out, ec)) + " (the picture " + human_bytes(pics[shown].file.size()) + ")"
                     : "not made: " + said;
        message_bad = !ok;
    };

    while (!quit)
    {
        // A picture loaded: shown (the last of a .sieve's when coming back to it).
        if (loading && loading->done)
        {
            loading->t.join();
            if (loading->error.empty())
            {
                pics = std::move(loading->got);
                listing_said = loading->said;
                message.clear();
                show(loading->backwards ? pics.size() - 1 : 0);
                if (start_frame > 0 && animated())
                {
                    playing = false;
                    to_frame(size_t(start_frame - 1));
                    start_frame = 0;
                }
            }
            else
            {
                pics.clear();
                if (tex) SDL_DestroyTexture(tex), tex = nullptr;
                listing_said.clear();
                message = "refused: " + loading->error, message_bad = true;
            }
            loading.reset();
        }
        {
            std::lock_guard<std::mutex> lock(dialog_mx);
            if (dialog_answer)
            {
                if (!dialog_answer->empty() && !pics.empty()) {
                    if (dialog_kind == 1) export_png(from_u8(*dialog_answer));
                    else save_file(from_u8(*dialog_answer));
                }
                dialog_answer.reset();
            }
        }
        int ww = 0, wh = 0;
        SDL_GetRenderOutputSize(r, &ww, &wh);
        // The buttons along the top.
        std::vector<Button> buttons;
        {
            float x = float(ww) - 8;
            auto add = [&](const std::string& label, char key, bool enabled) {
                const float w = float(label.size()) * 8 * 1.5f + 16;
                x -= w;
                buttons.push_back({{x, 5, w, kBar - 10}, label, key, enabled});
                x -= 6;
            };
            add("Sieve", 's', !pics.empty() && !places.empty() && !places[place].sieve);
            add("Save file", 'f', !pics.empty() && !places.empty() && places[place].sieve);
            const bool shown_made = !pics.empty() && pics[shown].made;
            add("Export", 'e', shown_made);
            add(playing ? "Pause" : "Play", 'p', animated());
            add("1:1", '1', shown_made);
            add("Fit", '0', shown_made);
            add("Next >", '>', places.size() > 1 || pics.size() > 1);
            add("< Previous", '<', places.size() > 1 || pics.size() > 1);
        }
        auto press = [&](char key) {
            switch (key)
            {
            case '>': step(1); break;
            case '<': step(-1); break;
            case '0': fit(); break;
            case '1':
                if (!pics.empty() && pics[shown].made) zoom = 1, fitted = false;
                break;
            case 'e': ask_where(1); break;
            case 'f': ask_where(2); break;
            case 's': make_here(); break;
            case 'p':
                if (animated())
                {
                    playing = !playing;
                    if (playing) next_frame_at = SDL_GetTicks() + delay_of(frame);
                }
                break;
            case ',': // (a frame back or on, paused)
            case '.':
                if (animated())
                {
                    playing = false;
                    const size_t n = pics[shown].more.size() + 1;
                    to_frame(key == '.' ? frame + 1 : frame + n - 1);
                }
                break;
            default: break;
            }
        };
        auto zoom_at = [&](double factor, float px, float py) {
            if (pics.empty() || !pics[shown].made) return;
            const auto [vw, vh] = view_w();
            const double ox = px - vw / 2.0, oy = py - kBar - vh / 2.0;
            const double bx = cx + ox / zoom, by = cy + oy / zoom; // (the picture's point under the pointer stays there)
            zoom = std::clamp(zoom * factor, 1.0 / 64, 64.0);
            cx = bx - ox / zoom, cy = by - oy / zoom;
            fitted = false;
        };

        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_EVENT_QUIT: quit = true; break;
            case SDL_EVENT_DROP_FILE:
                if (e.drop.data) open_path(from_u8(e.drop.data));
                break;
            case SDL_EVENT_WINDOW_RESIZED:
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                if (fitted) fit();
                break;
            case SDL_EVENT_KEY_DOWN:
            {
                const SDL_Keycode k = e.key.key;
                const bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0;
                if (k == SDLK_ESCAPE || k == SDLK_Q) quit = true;
                else if ((k == SDLK_RIGHT && !shift) || k == SDLK_SPACE || k == SDLK_PAGEDOWN) press('>');
                else if ((k == SDLK_LEFT && !shift) || k == SDLK_BACKSPACE || k == SDLK_PAGEUP) press('<');
                else if (k == SDLK_HOME && !places.empty() && !(loading && !loading->done)) start_loading(0, false);
                else if (k == SDLK_END && !places.empty() && !(loading && !loading->done)) start_loading(places.size() - 1, true);
                else if (k == SDLK_0) press('0');
                else if (k == SDLK_1) press('1');
                else if (k == SDLK_EQUALS || k == SDLK_PLUS || k == SDLK_KP_PLUS) zoom_at(1.25, float(ww) / 2, kBar + (float(wh) - kBar - kFoot) / 2);
                else if (k == SDLK_MINUS || k == SDLK_KP_MINUS) zoom_at(0.8, float(ww) / 2, kBar + (float(wh) - kBar - kFoot) / 2);
                else if (k == SDLK_E) press('e');
                else if (k == SDLK_F) press('f');
                else if (k == SDLK_S) press('s');
                else if (k == SDLK_P) press('p');
                else if (k == SDLK_COMMA) press(',');
                else if (k == SDLK_PERIOD) press('.');
                else if (shift && (k == SDLK_LEFT || k == SDLK_RIGHT || k == SDLK_UP || k == SDLK_DOWN))
                {
                    const double d = 64 / zoom;
                    cx += k == SDLK_LEFT ? -d : k == SDLK_RIGHT ? d : 0;
                    cy += k == SDLK_UP ? -d : k == SDLK_DOWN ? d : 0;
                    fitted = false;
                }
                break;
            }
            case SDL_EVENT_MOUSE_WHEEL:
            {
                float mx = 0, my = 0;
                SDL_GetMouseState(&mx, &my);
                if (e.wheel.y != 0) zoom_at(std::pow(1.2, double(e.wheel.y)), mx, my);
                break;
            }
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                if (e.button.button == SDL_BUTTON_LEFT)
                {
                    bool on_button = false;
                    for (const Button& b : buttons)
                        if (inside(b.r, e.button.x, e.button.y))
                        {
                            on_button = true;
                            if (b.enabled) press(b.key);
                        }
                    if (!on_button && e.button.y > kBar) dragging = true, drag_x = e.button.x, drag_y = e.button.y;
                }
                break;
            case SDL_EVENT_MOUSE_BUTTON_UP:
                if (e.button.button == SDL_BUTTON_LEFT) dragging = false;
                break;
            case SDL_EVENT_MOUSE_MOTION:
                if (dragging)
                {
                    cx -= (e.motion.x - drag_x) / zoom, cy -= (e.motion.y - drag_y) / zoom;
                    drag_x = e.motion.x, drag_y = e.motion.y;
                    fitted = false;
                }
                break;
            default: break;
            }
        }

        // An animated GIF's next frame, once it is due (from when it was due, so the delays add up
        // as the GIF gives them; from now, where the window fell behind by more than a second).
        if (animated() && playing && !loading && SDL_GetTicks() >= next_frame_at)
        {
            const uint64_t due = next_frame_at;
            to_frame(frame + 1);
            if (SDL_GetTicks() < due + 1000) next_frame_at = due + delay_of(frame);
        }

        // The window: the picture on a chequer (so transparency shows), the bar above, the foot.
        SDL_SetRenderDrawColor(r, 24, 24, 26, 255);
        SDL_RenderClear(r);
        const auto [vw, vh] = view_w();
        if (tex && !pics.empty() && pics[shown].made)
        {
            // (drawn at the picture's own size: an eighth's pixels stretched over it)
            const Picture& pic = pics[shown];
            const SDL_FRect dst{float(vw / 2.0 - cx * zoom), float(kBar + vh / 2.0 - cy * zoom), float(pic.width * zoom), float(pic.height * zoom)};
            const SDL_Rect clip{0, int(kBar), int(vw), int(vh)};
            SDL_SetRenderClipRect(r, &clip);
            const float cell = 12;
            const SDL_FRect seen{std::max(dst.x, 0.f), std::max(dst.y, kBar), 0, 0};
            const float x1 = std::min(dst.x + dst.w, vw), y1 = std::min(dst.y + dst.h, kBar + vh);
            for (float y = seen.y; y < y1; y += cell)
                for (float x = seen.x; x < x1; x += cell)
                {
                    const bool dark = (int((x - dst.x) / cell) + int((y - dst.y) / cell)) % 2;
                    SDL_SetRenderDrawColor(r, dark ? 70 : 96, dark ? 70 : 96, dark ? 74 : 100, 255);
                    const SDL_FRect c{x, y, std::min(cell, x1 - x), std::min(cell, y1 - y)};
                    SDL_RenderFillRect(r, &c);
                }
            SDL_SetTextureScaleMode(tex, zoom * tex_step * pic.scale >= 2 ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
            SDL_RenderTexture(r, tex, nullptr, &dst);
            SDL_SetRenderClipRect(r, nullptr);
        }
        SDL_SetRenderDrawColor(r, 40, 40, 44, 255);
        const SDL_FRect bar{0, 0, float(ww), kBar}, foot{0, float(wh) - kFoot, float(ww), kFoot};
        SDL_RenderFillRect(r, &bar);
        SDL_RenderFillRect(r, &foot);
        std::string title = places.empty() ? std::string("Sieve Viewer") : u8(places[place].path.filename());
        if (!pics.empty() && places[place].sieve) title += " / " + pics[shown].name;
        const float left_room = buttons.empty() ? float(ww) : buttons.back().r.x - 16;
        text(r, 10, 12, tail(title, size_t(std::max(8.f, left_room / 12))), 1.5f, kInk);
        for (const Button& b : buttons)
        {
            SDL_SetRenderDrawColor(r, b.enabled ? 60 : 46, b.enabled ? 60 : 46, b.enabled ? 66 : 50, 255);
            SDL_RenderFillRect(r, &b.r);
            SDL_SetRenderDrawColor(r, 90, 90, 98, 255);
            SDL_RenderRect(r, &b.r);
            text(r, b.r.x + 8, b.r.y + 6, b.label, 1.5f, b.enabled ? (b.key == 's' ? kAccent : kInk) : kDim);
        }
        std::string foot_text;
        if (!message.empty()) foot_text = message;
        else if (!pics.empty() && pics[shown].made)
        {
            const Picture& pic = pics[shown];
            char z[32];
            std::snprintf(z, sizeof z, "%.0f%%", zoom * 100);
            foot_text = std::to_string(pic.width) + " x " + std::to_string(pic.height) + ", " + human_bytes(pic.file.size()) + ", " + z;
            if (pic.scale == 8) foot_text += " (shown at an eighth: " + pic.reduced + ")";
            if (!pic.more.empty())
                foot_text += ", frame " + std::to_string(frame + 1) + " of " + std::to_string(pic.more.size() + 1) + (playing ? "" : " (paused: , and . step)");
            else if (!pic.one_frame.empty()) foot_text += " (its first frame of " + std::to_string(pic.frames) + " shown: " + pic.one_frame + ")";
            if (pics.size() > 1) foot_text += ", picture " + std::to_string(shown + 1) + " of " + std::to_string(pics.size());
            if (places.size() > 1) foot_text += "; file " + std::to_string(place + 1) + " of " + std::to_string(places.size()) + " here";
            if (!listing_said.empty()) foot_text += "; its listing: " + listing_said;
        }
        text(r, 8, float(wh) - kFoot + 7, tail(foot_text, size_t(std::max(8, ww / 8 - 2))), 1.0f, message_bad ? kRed : message.empty() ? kDim : kInk);
        SDL_RenderPresent(r);

        if (shot_due && !loading)
        {
            // (once what was asked for is shown, or refused: the frame after its loading was taken in)
            save_bmp(r, shot);
            quit = true;
        }
        if (!shot_due) SDL_Delay(loading ? 10 : 0);
    }
    if (loading && loading->t.joinable()) loading->t.join();
    if (tex) SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
