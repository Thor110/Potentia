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

#include "cli/image_io.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "cli/pack.hpp"
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

// The limits a .sieve's listing and a picture's header are held to before anything is decoded:
// a count of files, and memory, half of what is free when it is asked (or where that cannot be
// had, 2 GiB; --budget gives it instead).
constexpr size_t kMaxFiles = 4096;
constexpr uint32_t kMaxSide = uint32_t(1) << 24; // pixels a side (stb_image's own limit)
uint64_t g_budget = 0;                           // (--budget)

uint64_t memory_budget()
{
    if (g_budget) return g_budget;
    const uint64_t free_now = memory_free();
    return free_now ? free_now / 2 : uint64_t(2) << 30;
}

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string lower(std::string s)
{
    for (char& c : s) c = char(std::tolower(static_cast<unsigned char>(c)));
    return s;
}

std::string human_bytes(uint64_t n)
{
    char b[48];
    if (n < 1024) std::snprintf(b, sizeof b, "%llu bytes", (unsigned long long)n);
    else if (n < 1048576) std::snprintf(b, sizeof b, "%.1f KB", double(n) / 1024.0);
    else if (n < 1073741824ull) std::snprintf(b, sizeof b, "%.1f MB", double(n) / 1048576.0);
    else std::snprintf(b, sizeof b, "%.2f GB", double(n) / 1073741824.0);
    return b;
}

bool picture_name(const std::string& name)
{
    const std::string e = lower(fs::path(from_u8(name)).extension().string());
    return e == ".png" || e == ".jpg" || e == ".jpeg" || e == ".jpe" || e == ".jfif" || e == ".bmp" || e == ".gif" || e == ".tga";
}

bool sieve_name(const std::string& name)
{
    const std::string e = lower(fs::path(from_u8(name)).extension().string());
    return e == ".simage" || e == ".sieve";
}

// A recipe that says the file is a picture made again: a JPEG from its brunsli form, a PNG by
// stb_image_write, from its rows unfiltered, or re-saved.
bool picture_recipe(const std::string& recipe)
{
    for (const char* k : {"jbr ", "stb ", "skr ", "pngr "})
        if (recipe.rfind(k, 0) == 0) return true;
    return false;
}

std::vector<uint8_t> read_all(const fs::path& p)
{
    std::ifstream f(p, std::ios::binary);
    if (!f) throw std::runtime_error("cannot open " + u8(p));
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// A picture to show: where it came from, its own file's bytes, and its pixels once made.
struct Picture
{
    std::string name;          // its name (in a .sieve, its path there)
    std::vector<uint8_t> file; // the picture file's own bytes
    uint32_t width = 0, height = 0; // its own size
    uint32_t scale = 1;             // 1: its pixels; 8: an eighth of them (too large to decode whole)
    std::string reduced;            // where reduced, why
    bool made = false;
    RgbaImage pixels;
};

bool is_jpeg_bytes(const std::vector<uint8_t>& b) { return b.size() >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF; }

// A PNG's size from its header (IHDR), for one stb_image will not read for its size.
std::optional<std::pair<uint32_t, uint32_t>> png_size(const std::vector<uint8_t>& b)
{
    if (b.size() < 24 || std::memcmp(b.data(), "\x89PNG\r\n\x1a\n", 8) != 0 || std::memcmp(b.data() + 12, "IHDR", 4) != 0) return std::nullopt;
    auto be32 = [&](size_t at) { return uint32_t(b[at]) << 24 | uint32_t(b[at + 1]) << 16 | uint32_t(b[at + 2]) << 8 | b[at + 3]; };
    return std::make_pair(be32(16), be32(20));
}

// The header checked, then the pixels made: the first frame (a GIF's others are not shown); a JPEG
// too large for that, an eighth from its blocks' averages; else refused, with why.
void make_pixels(Picture& p)
{
    if (p.made) return;
    uint32_t w = 0, h = 0;
    const bool jpeg = is_jpeg_bytes(p.file);
    const bool info = image_info(p.file.data(), p.file.size(), w, h);
    if (!info)
    {
        const auto size = jpeg ? jpeg_size(p.file) : png_size(p.file);
        if (!size) throw std::runtime_error(p.name + ": not a picture this viewer reads");
        std::tie(w, h) = *size;
    }
    if (w == 0 || h == 0 || w > kMaxSide || h > kMaxSide)
        throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels, over " + std::to_string(kMaxSide) + " a side");
    p.width = w, p.height = h;
    // (stb_image's pixels and the viewer's RGBA of them, 4 bytes a pixel each at most; stb_image
    // itself decodes a JPEG's of under 2 GB and a PNG's of 1 GiB at most)
    const uint64_t pixels = uint64_t(w) * h, need = pixels * 8, budget = memory_budget();
    const bool stb_can = info && pixels * 4 < (uint64_t(1) << 31);
    if (stb_can && need <= budget)
    {
        std::vector<RgbaImage> frames = decode_image_frames(p.file.data(), p.file.size(), p.name);
        if (frames.empty()) throw std::runtime_error(p.name + ": no picture in it");
        p.pixels = std::move(frames.front());
        p.scale = 1;
    }
    else
    {
        const std::string why = need > budget ? "its pixels need about " + human_bytes(need) + " to decode whole, more than half the memory free (" + human_bytes(budget) + ")"
                                              : "too large for stb_image to decode whole";
        if (!jpeg) throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels: " + why + " (only a JPEG is shown reduced)");
        if (pixels / 64 * 8 > budget) throw std::runtime_error(p.name + ": " + std::to_string(w) + " by " + std::to_string(h) + " pixels: even its eighth needs more than half the memory free");
        auto eighth = jpeg_eighth(p.file);
        if (!eighth) throw std::runtime_error(p.name + ": " + why + ", and its eighth could not be read (a baseline or progressive JPEG, grey or YCbCr, is)");
        p.pixels.width = eighth->eighth_w;
        p.pixels.height = eighth->eighth_h;
        p.pixels.rgba = std::move(eighth->rgba);
        p.scale = 8;
        p.reduced = why;
    }
    p.made = true;
}

// What a .sieve holds, read from its listing alone, and refused there if it is not pictures within
// the limits; else installed into a folder of its own and each picture's file read back.
std::vector<Picture> pictures_of_sieve(const fs::path& file, std::string* listing_said = nullptr)
{
    // (its listing alone first, nothing decoded: cli/locate.hpp listing_of_installer)
    const Manifest listed = listing_of_installer(file);
    size_t n = 0;
    uint64_t all = 0;
    for (const ManifestEntry& e : listed.entries)
    {
        if (e.dir || e.member) continue;
        if (++n > kMaxFiles) throw std::runtime_error("more than " + std::to_string(kMaxFiles) + " files: not a .sieve of pictures this viewer opens");
        const std::string name = e.path;
        if (!picture_recipe(e.remake) && !picture_name(name))
            throw std::runtime_error(name + ": not a picture (by its recipe and its name), so the .sieve is not opened");
        if (e.size == kSizeAsMade) throw std::runtime_error(name + ": its size not said");
        all += e.size;
        // (every file is held at once while it is installed: half the memory free for them all)
        if (all > memory_budget())
            throw std::runtime_error("its pictures come to more than half the memory free (" + human_bytes(memory_budget()) + "), by its listing" +
                                     (e.size > memory_budget() ? " (" + name + " alone " + human_bytes(e.size) + ")" : std::string()));
    }
    if (n == 0) throw std::runtime_error("no files in it");
    if (listing_said) *listing_said = std::to_string(n) + (n == 1 ? " picture, " : " pictures, ") + human_bytes(all);
    // (then read again to install: each file decoded, stopping at its declared size)
    const Manifest m = installable_manifest(file, false);
    // (A folder of its own, removed after: installing checks each file's size and SHA-256 first.)
    std::error_code ec;
    static std::atomic<unsigned> serial{0};
    const fs::path dest = fs::temp_directory_path(ec) / ("sieve-view-" + std::to_string(SDL_GetTicksNS()) + "-" + std::to_string(serial++));
    struct Gone
    {
        fs::path p;
        ~Gone()
        {
            std::error_code e;
            fs::remove_all(p, e);
        }
    } gone{dest};
    install_tree(m, dest, false, nullptr);
    std::vector<Picture> out;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir || e.member) continue;
        Picture p;
        p.name = e.path;
        p.file = read_all(dest / from_u8(e.path));
        out.push_back(std::move(p));
    }
    return out;
}

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
    SDL_Process* proc = SDL_CreateProcess(args, true);
    if (!proc) return {false, std::string("could not run sieve: ") + SDL_GetError()};
    size_t got = 0;
    int code = -1;
    char* said = static_cast<char*>(SDL_ReadProcess(proc, &got, &code));
    std::string all = said ? std::string(said, got) : std::string();
    SDL_free(said);
    SDL_DestroyProcess(proc);
    std::string last;
    for (size_t at = 0; at < all.size();)
    {
        const size_t nl = all.find('\n', at);
        const std::string line = all.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
        if (line.rfind("installer ", 0) == 0 || line.rfind("error", 0) == 0 || line.rfind("sieve:", 0) == 0) last = line;
        if (nl == std::string::npos) break;
        at = nl + 1;
    }
    return {code == 0, last};
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
    "memory free. A JPEG too large is shown at an eighth, from its blocks' averages alone.\n"
    "\n"
    "Keys: Right/Left next and before, wheel or +/- zoom, 0 fit, 1 one to one, drag to move,\n"
    "E export as PNG, S make NAME.simage beside it (sieve locate --prioritise-decoding),\n"
    "F save the picture's own file from a .sieve, Esc or Q close.\n"
    "\n"
    "Options (for testing):\n"
    "  --check              say what it would show (each picture, or why it is refused), and exit\n"
    "  --make-simage        make NAME.simage beside the picture and exit (0 on success)\n"
    "  --zoom Z             start at this zoom (1 is one to one) rather than fitted\n"
    "  --budget BYTES       take this as the memory it may use, not half of what is free\n"
    "  --screenshot FILE    write the window once the picture is shown, as a BMP, and exit\n"
    "  --help               this text\n";

} // namespace

int main(int argc, char** argv)
{
    std::string file_arg, shot;
    bool check = false, make = false;
    double start_zoom = 0;
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
        else if (a == "--budget" && i + 1 < argc) g_budget = std::strtoull(argv[++i], nullptr, 10);
        else if (a == "--check") check = true;
        else if (a == "--make-simage") make = true;
        else file_arg = a;
    }
    // Every file Sieve's instructions can carry made again (a JPEG from its brunsli form, a PNG by
    // stb_image_write ...), as sieve install does.
    sieve::cli::use_item_maker();

    // The pictures of one place: an ordinary picture, or every picture of a .sieve, the pixels made
    // of the one to be shown first only (the first, or coming back to it, the last).
    auto load = [](const Place& at, std::string& said, bool backwards) {
        std::vector<Picture> out;
        if (at.sieve) out = pictures_of_sieve(at.path, &said);
        else
        {
            Picture p;
            p.name = u8(at.path.filename());
            p.file = read_all(at.path);
            out.push_back(std::move(p));
        }
        make_pixels(out[backwards ? out.size() - 1 : 0]);
        return out;
    };

    if (check || make)
    {
        hallway::attach_parent_console();
        if (file_arg.empty())
        {
            std::fputs("sieve-view: name a picture or a .sieve\n", stderr);
            return 2;
        }
        const fs::path f = from_u8(file_arg);
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
        try
        {
            std::string said;
            std::vector<Picture> got = load(Place{f, sieve_name(u8(f.filename()))}, said, false);
            if (!said.empty()) std::printf("listing: %s\n", said.c_str());
            for (Picture& p : got)
            {
                make_pixels(p); // (one at a time, each let go before the next)
                std::printf("%s: %u by %u pixels, %s%s\n", p.name.c_str(), p.width, p.height, human_bytes(p.file.size()).c_str(),
                            p.scale == 8 ? (", shown at an eighth (" + std::to_string(p.pixels.width) + " by " + std::to_string(p.pixels.height) + "): " + p.reduced).c_str() : "");
                p.pixels = RgbaImage{};
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
    auto upload = [&] {
        if (tex) SDL_DestroyTexture(tex), tex = nullptr;
        if (pics.empty() || !pics[shown].made) return;
        const RgbaImage& im = pics[shown].pixels;
        tex_step = 1;
        while ((int(im.width) + tex_step - 1) / tex_step > max_texture || (int(im.height) + tex_step - 1) / tex_step > max_texture) ++tex_step;
        tex_w = int((im.width + tex_step - 1) / tex_step), tex_h = int((im.height + tex_step - 1) / tex_step);
        tex = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC, tex_w, tex_h);
        if (!tex) return;
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
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    };
    auto show = [&](size_t k) {
        // (one picture's pixels held at a time: the others let go, this one made if it is not)
        for (size_t j = 0; j < pics.size(); ++j)
            if (j != k && pics[j].made) pics[j].pixels = RgbaImage{}, pics[j].made = false;
        try
        {
            make_pixels(pics[k]);
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
        upload();
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
        const RgbaImage& im = pics[shown].pixels;
        const std::string png = stb_png(im.rgba, int(im.width), int(im.height), 4, 8, -1);
        std::ofstream o(to, std::ios::binary);
        o.write(png.data(), std::streamsize(png.size()));
        message = o ? "exported " + u8(to) + " (" + human_bytes(png.size()) + (pics[shown].scale == 8 ? ", the eighth shown" : "") + ")" : "could not write " + u8(to);
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
