// sieve-install: the installer. One small window, the steps every installer has, and one number
// behind it.
//
// An installer made with `sieve locate FOLDER --installer NAME.sieve` is the address of the
// folder's installer manifest (sieve-manifest-v4), stored as raw bytes: one number, which read
// back is the manifest, which holds the folder structure and then every file packed (programs
// through the x86 filter, texts made from other texts' lines as masks, all of it LZMA2), unpacked
// by the LZMA SDK's decoder in the locator's code (cli/locate.hpp). Older installers (v3: every
// file's bytes as they are) install the same way. This program does only that. It opens the .sieve file (given on the command
// line, attached to this program's own end (an installer program: sieve locate --program), dropped
// on the window, or the one lying beside the program), says what it will install
// and where, and on Install reads every file back and checks each against its SHA-256 before it
// writes any, so a damaged installer leaves nothing behind; Cancel part way removes what was
// written. It carries the core's addressing and the manifest code and nothing else: no
// dictionaries, models, fonts or pictures, which is what makes it the trimmed build of the tool.
//
// When what it carries is one 7z archive (as releases did before v4 packed their files itself),
// it unpacks the archive instead of writing it: into a folder named after the file (sieve.7z ->
// sieve), with the archive's one top folder, if it has one, left out (client/unpack_7z.hpp). The
// archive is checked against its SHA-256 first, and 7z checks each file's CRC as it unpacks. Only
// this program does that; the locator gives back exactly the file it located.
//
// For testing without a person: --to FOLDER sets where, --yes installs at once and exits (0 on
// success), and --screenshot FILE.bmp writes the window as it stands just before exiting.

#include "cli/locate.hpp"
#include "unpack_7z.hpp"
#include "window_icon.hpp"

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace {

namespace fs = std::filesystem;
using namespace sieve;
using namespace sieve::cli;

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string files_n(uint64_t n) { return std::to_string(n) + (n == 1 ? " file" : " files"); }

std::string human_bytes(uint64_t n)
{
    char b[48];
    if (n < 1024) std::snprintf(b, sizeof b, "%llu bytes", (unsigned long long)n);
    else if (n < 1048576) std::snprintf(b, sizeof b, "%.1f KB", double(n) / 1024.0);
    else if (n < 1073741824ull) std::snprintf(b, sizeof b, "%.1f MB", double(n) / 1048576.0);
    else std::snprintf(b, sizeof b, "%.2f GB", double(n) / 1073741824.0);
    return b;
}

// ---- drawing: SDL's own 8x8 font, scaled, so the program needs no files of its own
const SDL_Color kInk{235, 235, 235, 255}, kDim{150, 150, 150, 255}, kAccent{32, 220, 80, 255}, kRed{255, 90, 90, 255};

void text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c)
{
    SDL_SetRenderScale(r, scale, scale);
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderDebugText(r, x / scale, y / scale, s.c_str());
    SDL_SetRenderScale(r, 1, 1);
}

float text_w(const std::string& s, float scale) { return float(s.size()) * 8.0f * scale; }

// The end of a long string, so a path shows where it goes rather than where it starts.
std::string tail(const std::string& s, size_t chars) { return s.size() <= chars ? s : "..." + s.substr(s.size() - (chars - 3)); }

struct Button
{
    SDL_FRect r;
    std::string label;
    bool enabled = true;
};

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

void draw_button(SDL_Renderer* r, const Button& b, bool hot)
{
    SDL_SetRenderDrawColor(r, hot && b.enabled ? 40 : 24, hot && b.enabled ? 70 : 24, hot && b.enabled ? 48 : 24, 255);
    SDL_RenderFillRect(r, &b.r);
    const SDL_Color c = b.enabled ? kAccent : kDim;
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 255);
    SDL_RenderRect(r, &b.r);
    text(r, b.r.x + (b.r.w - text_w(b.label, 2)) / 2, b.r.y + (b.r.h - 16) / 2, b.label, 2, b.enabled ? kInk : kDim);
}

// ---- the installer's state
enum class Screen { NoInstaller, Ready, Installing, Done, Cancelled, Failed };

struct Installer
{
    fs::path source;            // the .sieve file
    std::optional<Manifest> manifest;
    // One 7z archive: unpacked into a folder named after it, rather than written as it is.
    std::optional<install::SevenZip> archive;
    std::string error;          // why the installer could not be read, or the install failed
    std::string dest;           // where, as UTF-8, editable
    bool replace = false;       // replace files that are already there
    bool editing = false;       // typing into the destination
    Screen screen = Screen::NoInstaller;
    // The install runs on a worker; these are what the window shows of it.
    std::thread worker;
    std::atomic<bool> cancel{false}, finished{false}, ok{false};
    std::atomic<int> phase{0};
    std::atomic<size_t> done{0}, total{0};
    std::mutex mx;
    std::string current, worker_error;
    // A folder chosen in the system's dialog, handed over from its callback.
    std::mutex dialog_mx;
    std::optional<std::string> chosen;

    // An installer file, or a program with one attached (an installer program, or this one).
    void open(const fs::path& file)
    {
        source = file;
        manifest.reset();
        archive.reset();
        error.clear();
        try
        {
            manifest = installable_manifest(file, false); // an installer, a program, or a v3 or v4 manifest
            screen = Screen::Ready;
            // A 7z archive on its own is unpacked; checked against its SHA-256 first, so a damaged
            // one is refused here, before its listing is trusted.
            if (one_file() && install::is_7z(manifest->contents))
            {
                if (sha256_hex(manifest->contents) != manifest->entries[0].sha256)
                    throw std::runtime_error(manifest->root + " does not match its SHA-256: the installer is damaged");
                archive = install::list_7z(manifest->contents);
            }
            // Where it goes unless you say otherwise: into your documents (see where_in).
            const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
            dest = where_in(docs ? from_u8(docs) : fs::current_path());
        }
        catch (const std::exception& e)
        {
            error = e.what();
            screen = Screen::NoInstaller;
        }
    }

    // A single file (Sieve instructions for one file).
    bool one_file() const
    {
        return manifest && manifest->with_contents && manifest->entries.size() == 1 && !manifest->entries[0].dir &&
               manifest->entries[0].path == manifest->root;
    }

    // Where it goes in a folder chosen: a folder of its own name; a single file straight into the
    // folder; a 7z archive into a folder named after the file (sieve.7z -> sieve).
    std::string where_in(const fs::path& base) const
    {
        if (archive) return u8(base / from_u8(manifest->root).stem());
        if (one_file()) return u8(base);
        return u8(base / from_u8(manifest->root));
    }

    void start()
    {
        if (!manifest || dest.empty()) return;
        screen = Screen::Installing;
        cancel = false;
        finished = false;
        done = 0;
        total = archive ? size_t(archive->files) : size_t(manifest->files);
        worker = std::thread([this] {
            try
            {
                if (archive)
                {
                    phase = 1; // checked already: the archive's SHA-256 when it was opened
                    ok = install::unpack_7z(manifest->contents, from_u8(dest), replace,
                                            [this](size_t d, size_t t, const std::string& path) {
                                                done = d;
                                                total = t;
                                                std::lock_guard<std::mutex> lock(mx);
                                                current = path;
                                            },
                                            &cancel);
                    finished = true;
                    return;
                }
                const bool whole = install_tree(*manifest, from_u8(dest), replace,
                                                [this](int ph, size_t d, size_t t, const std::string& path) {
                                                    phase = ph;
                                                    done = d;
                                                    total = t;
                                                    std::lock_guard<std::mutex> lock(mx);
                                                    current = path;
                                                },
                                                &cancel);
                ok = whole;
            }
            catch (const std::exception& e)
            {
                std::lock_guard<std::mutex> lock(mx);
                worker_error = e.what();
                ok = false;
            }
            finished = true;
        });
    }

    // Called every frame: the worker's end turns into the next screen.
    void poll()
    {
        if (screen != Screen::Installing || !finished) return;
        worker.join();
        if (ok) screen = Screen::Done;
        else if (cancel) screen = Screen::Cancelled;
        else
        {
            std::lock_guard<std::mutex> lock(mx);
            error = worker_error;
            screen = Screen::Failed;
        }
    }
};

void SDLCALL folder_chosen(void* user, const char* const* files, int)
{
    auto* in = static_cast<Installer*>(user);
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    std::lock_guard<std::mutex> lock(in->dialog_mx);
    in->chosen = std::string(files[0]);
}

// The .sieve file beside the program, if there is exactly one.
std::optional<fs::path> installer_beside_me()
{
    const char* base = SDL_GetBasePath();
    if (!base) return std::nullopt;
    std::optional<fs::path> found;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(from_u8(base), ec))
        if (e.is_regular_file() && e.path().extension() == ".sieve")
        {
            if (found) return std::nullopt; // more than one: which is meant is not ours to guess
            found = e.path();
        }
    return found;
}

void save_bmp(SDL_Renderer* r, const std::string& path)
{
    if (SDL_Surface* s = SDL_RenderReadPixels(r, nullptr))
    {
        SDL_SaveBMP(s, path.c_str());
        SDL_DestroySurface(s);
    }
}

} // namespace

int main(int argc, char** argv)
{
    // Options for testing (see the top); anything else is the installer to open.
    std::string file_arg, to_arg, shot;
    bool yes = false;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--to" && i + 1 < argc) to_arg = argv[++i];
        else if (a == "--screenshot" && i + 1 < argc) shot = argv[++i];
        else if (a == "--yes") yes = true;
        else file_arg = a;
    }
    if (!SDL_Init(SDL_INIT_VIDEO))
    {
        SDL_Log("sieve-install: %s", SDL_GetError());
        return 1;
    }
    SDL_Window* window = nullptr;
    SDL_Renderer* r = nullptr;
    if (!SDL_CreateWindowAndRenderer("Sieve Installer", 760, 440, 0, &window, &r))
    {
        SDL_Log("sieve-install: %s", SDL_GetError());
        return 1;
    }
    SDL_SetRenderVSync(r, 1);
    set_window_icon(window);
    Installer in;
    // What to install: the file named, else the installer attached to this program itself (an
    // installer program, made with sieve locate --program or the File Locator), else the one
    // .sieve file beside it.
    const fs::path self = own_executable(argc > 0 ? argv[0] : nullptr);
    std::optional<BigUint> mine;
    try
    {
        if (file_arg.empty()) mine = attached_address(self);
    }
    catch (const std::exception& e)
    {
        in.error = e.what();
    }
    if (!file_arg.empty()) in.open(from_u8(file_arg));
    else if (mine) in.open(self);
    else if (!in.error.empty()) {}
    else if (auto beside = installer_beside_me()) in.open(*beside);
    else in.error = "no installer: drop a .sieve file on this window, or open one with this program";
    if (!to_arg.empty()) in.dest = to_arg;
    if (yes && in.screen == Screen::Ready) in.start();

    bool quit = false;
    float mx = -1, my = -1;
    std::vector<Button> buttons;
    SDL_FRect field{}, tick{};
    while (!quit)
    {
        in.poll();
        if (yes && in.screen != Screen::Installing) quit = true; // unattended: the result is the exit code
        {
            std::lock_guard<std::mutex> lock(in.dialog_mx);
            if (in.chosen && in.manifest)
            {
                in.dest = in.where_in(from_u8(*in.chosen));
                in.chosen.reset();
            }
        }
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT && in.screen != Screen::Installing) quit = true;
            if (e.type == SDL_EVENT_DROP_FILE && in.screen != Screen::Installing && e.drop.data) in.open(from_u8(e.drop.data));
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = e.motion.x;
                my = e.motion.y;
            }
            if (e.type == SDL_EVENT_TEXT_INPUT && in.editing) in.dest += e.text.text;
            std::string pressed;
            if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
            {
                for (const Button& b : buttons)
                    if (b.enabled && inside(b.r, e.button.x, e.button.y)) pressed = b.label;
                if (in.screen == Screen::Ready)
                {
                    const bool on_field = inside(field, e.button.x, e.button.y);
                    if (on_field != in.editing)
                    {
                        in.editing = on_field;
                        if (on_field) SDL_StartTextInput(window);
                        else SDL_StopTextInput(window);
                    }
                    if (inside(tick, e.button.x, e.button.y)) in.replace = !in.replace;
                }
            }
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                if (in.editing && e.key.key == SDLK_BACKSPACE && !in.dest.empty())
                {
                    do in.dest.pop_back();
                    while (!in.dest.empty() && (static_cast<unsigned char>(in.dest.back()) & 0xC0) == 0x80);
                }
                else if (in.editing && e.key.key == SDLK_V && (e.key.mod & SDL_KMOD_CTRL))
                {
                    if (char* clip = SDL_GetClipboardText()) { in.dest += clip; SDL_free(clip); }
                }
                else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                    pressed = in.screen == Screen::Ready ? "Install" : in.screen == Screen::Installing ? "" : "Close";
                else if (e.key.key == SDLK_ESCAPE) pressed = in.screen == Screen::Installing ? "Cancel" : in.screen == Screen::Ready ? "Cancel" : "Close";
            }
            if (pressed == "Install") { in.editing = false; SDL_StopTextInput(window); in.start(); }
            else if (pressed == "Browse...")
            {
                const std::string start = in.dest;
                SDL_ShowOpenFolderDialog(folder_chosen, &in, window, start.empty() ? nullptr : start.c_str(), false);
            }
            else if (pressed == "Cancel" && in.screen == Screen::Installing) in.cancel = true;
            else if (pressed == "Cancel" || pressed == "Close" || pressed == "Finish") quit = true;
        }

        // ---- the window
        SDL_SetRenderDrawColor(r, 12, 12, 12, 255);
        SDL_RenderClear(r);
        buttons.clear();
        const float W = 760, H = 440;
        text(r, 24, 20, "SIEVE INSTALLER", 3, kAccent);
        auto row = [&](float y, const std::string& label, const std::string& value) {
            text(r, 24, y, label, 1.5f, kDim);
            text(r, 170, y, value, 1.5f, kInk);
        };
        if (in.screen == Screen::NoInstaller)
        {
            text(r, 24, 90, "Nothing to install yet.", 2, kInk);
            text(r, 24, 130, tail(in.error, 80), 1, kRed);
            buttons.push_back({{W - 164, H - 60, 140, 40}, "Close"});
        }
        else
        {
            const Manifest& m = *in.manifest;
            if (in.archive)
            {
                row(80, "Unpacks", m.root + " (7z, " + human_bytes(m.bytes) + ")");
                row(104, "Contents", files_n(in.archive->files) + ", " + human_bytes(in.archive->bytes) +
                                         (in.archive->top.empty() ? "" : ", from " + in.archive->top));
            }
            else
            {
                row(80, "Installs", m.root);
                row(104, "Contents", files_n(m.files) + ", " + human_bytes(m.bytes));
            }
            row(128, "From", tail(u8(in.source.filename()), 60));
            if (in.screen == Screen::Ready)
            {
                text(r, 24, 170, "Install to:", 1.5f, kDim);
                field = {24, 192, W - 206, 34};
                SDL_SetRenderDrawColor(r, 28, 28, 28, 255);
                SDL_RenderFillRect(r, &field);
                SDL_SetRenderDrawColor(r, in.editing ? 255 : 110, in.editing ? 255 : 110, in.editing ? 255 : 110, 255);
                SDL_RenderRect(r, &field);
                const std::string shown = tail(in.dest, size_t((field.w - 16) / 12)) + (in.editing && (SDL_GetTicks() / 400) % 2 ? "_" : "");
                text(r, field.x + 8, field.y + 9, shown, 1.5f, kInk);
                buttons.push_back({{W - 172, 192, 148, 34}, "Browse..."});
                tick = {24, 244, 18, 18};
                SDL_SetRenderDrawColor(r, 160, 160, 160, 255);
                SDL_RenderRect(r, &tick);
                if (in.replace)
                {
                    const SDL_FRect in_tick{28, 248, 10, 10};
                    SDL_SetRenderDrawColor(r, kAccent.r, kAccent.g, kAccent.b, 255);
                    SDL_RenderFillRect(r, &in_tick);
                }
                text(r, 52, 247, "Replace files that are already there", 1.5f, kInk);
                if (in.archive)
                {
                    text(r, 24, 290, "The archive was read back from its address and checked against its SHA-256;", 1, kDim);
                    text(r, 24, 304, "each file's CRC is checked as it unpacks. Cancel removes whatever was written.", 1, kDim);
                }
                else
                {
                    text(r, 24, 290, "Every file is read back from its address and checked against its SHA-256", 1, kDim);
                    text(r, 24, 304, "before anything is written. Cancel removes whatever was written.", 1, kDim);
                }
                buttons.push_back({{W - 328, H - 60, 140, 40}, "Install", !in.dest.empty()});
                buttons.push_back({{W - 164, H - 60, 140, 40}, "Cancel"});
            }
            else
            {
                row(152, "To", tail(in.dest, 60));
                const size_t d = in.done, t = std::max<size_t>(1, in.total);
                if (in.screen == Screen::Installing)
                {
                    const bool writing = in.phase == 1;
                    text(r, 24, 196, in.archive ? "Unpacking" : writing ? "Writing" : "Checking", 2, kInk);
                    // Checking is the first half of the bar, writing the second (unpacking, the whole).
                    const float f = in.archive ? float(d) / float(t) : (float(d) / float(t) + (writing ? 1.0f : 0.0f)) / 2.0f;
                    const SDL_FRect bar{24, 228, W - 48, 22}, fill{26, 230, (W - 52) * f, 18};
                    SDL_SetRenderDrawColor(r, 110, 110, 110, 255);
                    SDL_RenderRect(r, &bar);
                    SDL_SetRenderDrawColor(r, kAccent.r, kAccent.g, kAccent.b, 255);
                    SDL_RenderFillRect(r, &fill);
                    std::string cur;
                    {
                        std::lock_guard<std::mutex> lock(in.mx);
                        cur = in.current;
                    }
                    text(r, 24, 262, std::to_string(d) + " / " + std::to_string(in.total) + "   " + tail(cur, 70), 1, kDim);
                    buttons.push_back({{W - 164, H - 60, 140, 40}, "Cancel", !in.cancel});
                }
                else if (in.screen == Screen::Done)
                {
                    text(r, 24, 196, "Installed.", 2, kAccent);
                    if (in.archive)
                        text(r, 24, 228, files_n(in.archive->files) + " unpacked from " + in.manifest->root + ", every one checked.", 1.5f, kInk);
                    else
                        text(r, 24, 228, files_n(in.manifest->files) + (in.manifest->files == 1 ? ", checked against its SHA-256." : ", every one checked against its SHA-256."), 1.5f, kInk);
                    buttons.push_back({{W - 164, H - 60, 140, 40}, "Finish"});
                }
                else if (in.screen == Screen::Cancelled)
                {
                    text(r, 24, 196, "Cancelled.", 2, kInk);
                    text(r, 24, 228, "Nothing was left behind.", 1.5f, kDim);
                    buttons.push_back({{W - 164, H - 60, 140, 40}, "Close"});
                }
                else
                {
                    text(r, 24, 196, "Not installed.", 2, kRed);
                    text(r, 24, 228, tail(in.error, 90), 1, kInk);
                    text(r, 24, 246, "Nothing was left behind.", 1, kDim);
                    buttons.push_back({{W - 164, H - 60, 140, 40}, "Close"});
                }
            }
        }
        for (const Button& b : buttons) draw_button(r, b, inside(b.r, mx, my));
        if (quit && !shot.empty()) save_bmp(r, shot);
        SDL_RenderPresent(r);
    }
    if (in.worker.joinable())
    {
        in.cancel = true;
        in.worker.join();
    }
    const int code = in.screen == Screen::Done || (!yes && in.screen != Screen::Failed) ? 0 : 1;
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return code;
}
