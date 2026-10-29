// sieve-install: the installer. One small window, the steps every installer has, and one number
// behind it.
//
// An installer made with `sieve locate FOLDER --installer NAME.sieve` is the address of the
// folder's installer manifest (sieve-manifest-v4, or with --items the newest), stored as raw bytes: one number, which read
// back is the manifest, which holds the folder structure and then every file packed (programs
// through the x86 filter, texts made from other texts' lines as masks, all of it LZMA2), unpacked
// by the LZMA SDK's decoder in the locator's code (cli/locate.hpp). Older installers (v3: every
// file's bytes as they are) install the same way. This program does only that. It opens the .sieve file (given on the command
// line, attached to this program's own end (an installer program: sieve locate --program), dropped
// on the window, or the one lying beside the program), says what it will install
// and where, and on Install reads every file back and checks each against its SHA-256 before it
// writes any, so a damaged installer leaves nothing behind; Cancel part way removes what was
// written. It is the full installer: it makes again the files carried as items of Sieve's lines
// and remakes archives exactly (cli/items.cpp), as sieve install does, so anything Sieve makes
// installs with it. It needs no data files: no dictionaries, models, fonts or pictures.
//
// When what it carries is one 7z archive (as releases did before v4 packed their files itself),
// it unpacks the archive instead of writing it: into a folder named after the file (sieve.7z ->
// sieve), with the archive's one top folder, if it has one, left out (client/unpack_7z.hpp). The
// archive is checked against its SHA-256 first, and 7z checks each file's CRC as it unpacks. Only
// this program does that; the locator gives back exactly the file it located.
//
// Any other archive Sieve reads (a .zip, .rar, .tar.gz ... : cli/pack.hpp's archive_kinds) is
// extracted as an archiver extracts it, one layer only (a .tar.gz gives its .tar): the right-click
// menu's Extract on those files (context/install_context.bat).
//
// For testing without a person: --to FOLDER sets where, --yes installs at once and exits (0 on
// success), and --screenshot FILE.bmp writes the window as it stands just before exiting.
// --answers LETTERS installs at once, as --yes does, but asks about files already there, each
// letter answering one question in turn: y Yes, n No, a Yes to all, x No to all, c Cancel.

#include "dialogs.hpp"
#include "cli/locate.hpp"
#include "cli/items.hpp"
#include "cli/pack.hpp"
#include "unpack_7z.hpp"
#include "window_icon.hpp"
#include "win_console.hpp"

#include <SDL3/SDL.h>

#include <cstdio>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
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

// The address bar's editing, on UTF-8 text: the character boundary before and after a byte offset,
// how many characters lie between two offsets, and the offset of the k-th character.
size_t utf8_prev(const std::string& s, size_t i)
{
    if (i == 0) return 0;
    do --i;
    while (i > 0 && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80);
    return i;
}
size_t utf8_next(const std::string& s, size_t i)
{
    if (i >= s.size()) return s.size();
    do ++i;
    while (i < s.size() && (static_cast<unsigned char>(s[i]) & 0xC0) == 0x80);
    return i;
}
size_t utf8_chars(const std::string& s, size_t from, size_t to)
{
    size_t n = 0;
    for (size_t i = from; i < to && i < s.size(); ++i) n += (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80;
    return n;
}
size_t utf8_at(const std::string& s, size_t k)
{
    size_t i = 0;
    while (k-- > 0 && i < s.size()) i = utf8_next(s, i);
    return i;
}

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
enum class Screen { NoInstaller, Reading, Ready, Asking, Installing, Done, Cancelled, Failed };

struct Installer
{
    fs::path source;            // the .sieve file
    std::optional<Manifest> manifest;
    // One 7z archive: unpacked into a folder named after it, rather than written as it is.
    std::optional<install::SevenZip> archive;
    // Not Sieve's: an archive of another kind, extracted (read_any_archive).
    std::optional<AnyArchive> any;
    bool keep_top = false;      // Extract here: its top folder kept, as an archiver's is
    bool top_left_out = false;  // its top folder already left out
    std::string error;          // why the installer could not be read, or the install failed
    std::string dest;           // where, as UTF-8, editable
    // The destination as a text field (Edward, 29 October 2026: "select it all, paste over it"):
    // the cursor and the selection's other end, byte offsets on UTF-8 boundaries (the selection is
    // between them), the first character shown, and whether the mouse is dragging a selection.
    size_t cur = 0, anchor = 0, view = 0;
    bool dragging = false;
    void set_dest(std::string to)
    {
        dest = std::move(to);
        cur = anchor = dest.size();
    }
    bool replace = false;       // replace files that are already there
    // Files already there, Replace not ticked: asked about one at a time, as other archivers do
    // (Edward, 29 October 2026: "an individual prompt per file unless all are accepted"). Each
    // one answered No stays as it is; unattended (--yes), they are refused as before.
    bool ask = true;
    std::vector<std::string> clashes; // their paths, as the installer names them
    size_t asking = 0;                // the one being asked about
    std::set<std::string> keep;       // those answered No
    bool editing = false;       // typing into the destination
    Screen screen = Screen::NoInstaller;
    // The install runs on a worker; these are what the window shows of it.
    std::thread worker;
    std::atomic<bool> cancel{false}, finished{false}, ok{false};
    std::atomic<int> phase{0};
    std::atomic<size_t> done{0}, total{0};
    std::mutex mx;
    std::string current, worker_error;
    // What the install had to say (install_tree's notes: an archive given as its contents, a RAR
    // made again or not): shown when it is done, a warning in a box of its own as well.
    std::vector<InstallNote> notes;
    bool warned = false;
    // A folder chosen in the system's dialog, handed over from its callback.
    std::mutex dialog_mx;
    std::optional<std::string> chosen;

    // An installer file, or a program with one attached (an installer program, or this one).
    // What reading an installer found: done on a thread of its own (a gigabyte's takes seconds, and
    // the window answers meanwhile), handed to the window by poll_open.
    struct Opened
    {
        std::optional<Manifest> manifest;
        std::optional<install::SevenZip> archive;
        std::optional<AnyArchive> any;
        std::string error;
    };
    struct Reading
    {
        std::mutex mx;
        std::optional<Opened> done;
    };
    std::shared_ptr<Reading> reading; // (shared with the reader, which may outlive a window closed meanwhile)
    bool keep_dest = false;           // the destination given (--to): reading leaves it as it is

    static Opened read_installer(const fs::path& file)
    {
        Opened o;
        // An archive of another kind, by its name (a .sieve, or a program with one attached, is
        // Sieve's own, below).
        if (is_other_archive(file))
        {
            try
            {
                const std::vector<uint8_t> bytes = read_file_bytes(file); // (at once: by the byte, 400 MB took 4.6 s)
                o.any = read_any_archive(bytes, u8(file.filename()));
                if (!o.any) throw std::runtime_error(u8(file.filename()) + " is not an archive this can extract");
            }
            catch (const std::exception& e)
            {
                o.any.reset();
                o.error = u8(file.filename()) + ": " + e.what();
            }
            return o;
        }
        try
        {
            o.manifest = installable_manifest(file, false); // an installer, a program, or a v3 or v4 manifest
            // A 7z archive on its own is unpacked; checked against its SHA-256 first, so a damaged
            // one is refused here, before its listing is trusted.
            const Manifest& m = *o.manifest;
            const bool one = m.with_contents && m.entries.size() == 1 && !m.entries[0].dir && m.entries[0].path == m.root;
            if (one && install::is_7z(m.contents))
            {
                if (sha256_hex(m.contents) != m.entries[0].sha256)
                    throw std::runtime_error(m.root + " does not match its SHA-256: the installer is damaged");
                o.archive = install::list_7z(m.contents);
            }
        }
        catch (const std::exception& e)
        {
            o.manifest.reset();
            o.archive.reset();
            o.error = e.what();
        }
        return o;
    }

    void open(const fs::path& file)
    {
        if (reading) return; // (one at a time: a file dropped meanwhile is left)
        source = file;
        manifest.reset();
        archive.reset();
        any.reset();
        top_left_out = false;
        error.clear();
        screen = Screen::Reading;
        reading = std::make_shared<Reading>();
        std::thread([r = reading, file] {
            Opened o = read_installer(file);
            std::lock_guard<std::mutex> lock(r->mx);
            r->done = std::move(o);
        }).detach();
    }

    // What the reader found, taken in once it is done; true then.
    bool poll_open()
    {
        if (!reading) return false;
        std::optional<Opened> o;
        {
            std::lock_guard<std::mutex> lock(reading->mx);
            o.swap(reading->done);
        }
        if (!o) return false;
        reading.reset();
        manifest = std::move(o->manifest);
        archive = std::move(o->archive);
        any = std::move(o->any);
        error = std::move(o->error);
        screen = (manifest || any) ? Screen::Ready : Screen::NoInstaller;
        if (screen == Screen::Ready && !keep_dest)
        {
            // Where it goes unless you say otherwise: into your documents (see where_in).
            const char* docs = SDL_GetUserFolder(SDL_FOLDER_DOCUMENTS);
            set_dest(where_in(docs ? from_u8(docs) : fs::current_path()));
        }
        return true;
    }

    // A file whose name is one of archive_kinds()'s (case aside), and not a .sieve.
    static bool is_other_archive(const fs::path& file)
    {
        std::string name = u8(file.filename());
        for (char& c : name) c = char(std::tolower(static_cast<unsigned char>(c)));
        for (const ArchiveKind& k : archive_kinds())
            for (const char* ext : k.extensions)
            {
                const std::string e(ext);
                if (name.size() > e.size() && name.compare(name.size() - e.size(), e.size(), e) == 0) return true;
            }
        return false;
    }

    // What it is, for the window: "Installs", or "Unpacks" for an archive.
    std::string root_name() const { return any ? u8(source.filename()) : manifest->root; }

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
        if (any) return u8(base / source.stem());
        if (archive) return u8(base / from_u8(manifest->root).stem());
        if (one_file()) return u8(base);
        return u8(base / from_u8(manifest->root));
    }

    // What would be written over: the files (and RARs made again) already in the destination.
    std::vector<std::string> already_there() const
    {
        std::vector<std::string> out;
        const fs::path to = from_u8(dest);
        auto there = [&](const std::string& p) {
            std::error_code ec;
            return fs::exists(to / from_u8(p), ec);
        };
        if (any)
        {
            for (const AnyArchive::Entry& e : any->entries)
                if (!e.dir && there(e.path)) out.push_back(e.path);
        }
        else if (archive)
        {
            for (const install::Unpacked& u : archive->entries)
                if (!u.dir && there(u.path)) out.push_back(u.path);
        }
        else
        {
            // (A RAR's pieces are written inside its folder and gone again once it is made: it is
            // the RAR itself that would be written over.)
            for (const ManifestEntry& e : manifest->entries)
                if (!e.dir && !e.member && there(e.path)) out.push_back(e.path);
            for (const Manifest::UnpackedArchive& u : manifest->unpacked)
                if (!u.program.empty() && there(u.archive)) out.push_back(u.archive);
        }
        return out;
    }

    // The size of what the installer gives at a path (for the question), where it is known.
    std::optional<uint64_t> given_size(const std::string& p) const
    {
        if (any)
        {
            for (const AnyArchive::Entry& e : any->entries)
                if (e.path == p) return e.bytes.size();
        }
        else if (archive)
        {
            for (const install::Unpacked& u : archive->entries)
                if (u.path == p) return u.size;
        }
        else
            for (const ManifestEntry& e : manifest->entries)
                if (e.path == p) return e.size;
        return std::nullopt;
    }

    // Install: first asking about each file already there, unless Replace is ticked.
    void start()
    {
        if ((!manifest && !any) || dest.empty()) return;
        // (Into a folder of its own: its one top folder left out, so NAME\NAME\ is not made.)
        if (any && !keep_top && !top_left_out)
        {
            leave_out_top_folder(*any);
            top_left_out = true; // once: Back, then Install again, must not take a second folder off
        }
        keep.clear();
        clashes.clear();
        if (!replace && ask) clashes = already_there();
        if (!clashes.empty())
        {
            asking = 0;
            screen = Screen::Asking;
            return;
        }
        launch(replace);
    }

    // An answer to the file being asked about; the install starts once there are no more.
    void answer(bool write_over, bool to_all)
    {
        if (screen != Screen::Asking) return;
        for (size_t k = asking; k < (to_all ? clashes.size() : asking + 1); ++k)
            if (!write_over) keep.insert(clashes[k]);
        asking = to_all ? clashes.size() : asking + 1;
        if (asking >= clashes.size()) launch(true); // (each one answered: none is refused now)
    }

    void launch(bool force)
    {
        screen = Screen::Installing;
        cancel = false;
        finished = false;
        done = 0;
        phase = 0; // (a second try starts from nothing: not the first one's last file and phase)
        {
            std::lock_guard<std::mutex> lock(mx);
            current.clear();
        }
        notes.clear();
        warned = false;
        const std::set<std::string>* kept = keep.empty() ? nullptr : &keep;
        total = any ? size_t(any->files) : archive ? size_t(archive->files) : size_t(manifest->files);
        worker = std::thread([this, force, kept] {
            try
            {
                if (any)
                {
                    phase = 1;
                    ok = unpack_archive(*any, from_u8(dest), force,
                                        [this](size_t d, size_t t, const std::string& path) {
                                            done = d;
                                            total = t;
                                            std::lock_guard<std::mutex> lock(mx);
                                            current = path;
                                        },
                                        &cancel, kept);
                    finished = true;
                    return;
                }
                if (archive)
                {
                    phase = 1; // checked already: the archive's SHA-256 when it was opened
                    ok = install::unpack_7z(manifest->contents, from_u8(dest), force,
                                            [this](size_t d, size_t t, const std::string& path) {
                                                done = d;
                                                total = t;
                                                std::lock_guard<std::mutex> lock(mx);
                                                current = path;
                                            },
                                            &cancel, kept);
                    finished = true;
                    return;
                }
                const bool whole = install_tree(*manifest, from_u8(dest), force,
                                                [this](int ph, size_t d, size_t t, const std::string& path) {
                                                    phase = ph;
                                                    done = d;
                                                    total = t;
                                                    std::lock_guard<std::mutex> lock(mx);
                                                    current = path;
                                                },
                                                &cancel, &notes, kept);
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

namespace {

const char* kInstallerUsage =
    "sieve-install - the installer: puts a folder back from its .sieve\n"
    "\n"
    "Usage:\n"
    "  sieve-install [INSTALLER.sieve] [--to FOLDER] [--yes] [--screenshot FILE.bmp]\n"
    "\n"
    "Opens the installer given, else the one attached to this program's own end (an installer\n"
    "program, made with sieve locate --program or the hallway's File Locator), else a .sieve\n"
    "dropped on its window or lying beside it. It says what it will install and where; Install\n"
    "checks every file against its SHA-256 before writing any, and Cancel part way removes what\n"
    "was written. It installs anything Sieve makes, as sieve install does: files packed, pulled\n"
    "tight by the rope or cut into pieces, items of Sieve's lines made again, archives remade\n"
    "exactly or given back as their contents, and the files a map holds. Any other archive Sieve\n"
    "reads (7z, zip, RAR, tar, gzip, xz and the rest) it extracts, as an archiver does, one layer\n"
    "at a time: a .tar.gz gives its .tar.\n"
    "\n"
    "Options:\n"
    "  --to FOLDER          where to install (else it asks)\n"
    "  --extract-here       install beside the .sieve at once, and close when done (the right-click\n"
    "                       menu's Extract here); a failure stays open, to say why\n"
    "  --extract-folder     the same, into a folder named after the .sieve (Extract to NAME\\)\n"
    "  --yes                install at once and exit: 0 on success (files already there are\n"
    "                       refused, unless --replace is given)\n"
    "  --replace            replace files that are already there (the Replace box ticked)\n"
    "  --answers LETTERS    as --yes, but each file already there is asked about, the letters\n"
    "                       answering in turn: y Yes, n No, a Yes to all, x No to all, c Cancel\n"
    "  --archive-extensions the names of the other archives it extracts (a .zip, .rar, .tar.gz ...),\n"
    "                       one a line: what context\\install_context.bat adds the menu to\n"
    "  --screenshot FILE    write the window, as it stands just before exiting, as a BMP\n"
    "  --help               this text\n";

} // namespace

int main(int argc, char** argv)
{
    // Options for testing (see the top); anything else is the installer to open.
    std::string file_arg, to_arg, shot;
    std::string answers;
    bool yes = false, replace = false;
    int extract = 0; // 1: beside the file; 2: in a folder named after it
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--help" || a == "-h")
        {
            hallway::attach_parent_console();
            std::fputs(kInstallerUsage, stdout);
            std::fflush(stdout);
            return 0;
        }
        if (a == "--archive-extensions")
        {
            hallway::attach_parent_console();
            for (const ArchiveKind& k : archive_kinds())
                for (const char* ext : k.extensions) std::printf("%s\n", ext);
            std::fflush(stdout);
            return 0;
        }
        if (a == "--to" && i + 1 < argc) to_arg = argv[++i];
        else if (a == "--screenshot" && i + 1 < argc) shot = argv[++i];
        else if (a == "--yes") yes = true;
        else if (a == "--replace") replace = true;
        else if (a == "--answers" && i + 1 < argc) answers = argv[++i], yes = true;
        else if (a == "--extract-here") extract = 1;
        else if (a == "--extract-folder") extract = 2;
        else file_arg = a;
    }
    // From the context menu (context/install_context.bat): where to put it, from the file's own
    // place, as an archiver's "Extract here" and "Extract to NAME\" do.
    if (extract && !file_arg.empty() && to_arg.empty())
    {
        namespace fs = std::filesystem;
        const fs::path f = fs::absolute(fs::path(std::u8string(file_arg.begin(), file_arg.end())));
        const fs::path where = extract == 1 ? f.parent_path() : f.parent_path() / f.stem();
        const std::u8string u = where.u8string();
        to_arg.assign(u.begin(), u.end());
    }
    // Every file Sieve's instructions can carry: items of Sieve's lines made again, archives
    // remade exactly (as sieve install and the hallway do).
    sieve::cli::use_item_maker();
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
    // (Whether one is attached is read from the program's last bytes; reading what is attached is
    // open's, once.)
    bool mine = false;
    try
    {
        if (file_arg.empty()) mine = has_attached(self);
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
    if (!to_arg.empty())
    {
        in.set_dest(to_arg);
        in.keep_dest = true;
    }
    in.keep_top = extract == 1; // Extract here: as an archiver's, the top folder kept
    in.replace = replace;
    in.ask = !yes || !answers.empty(); // (unattended, files already there are refused, as before)
    size_t answered = 0;
    // The right-click menu's extract options work as an archiver's do: at once, and the window
    // closes when it is done; only a failure stays open, to say why (Back, then Replace).
    const bool quick = extract != 0;
    bool start_when_read = yes || quick; // (once the installer is read, on its own thread)

    bool quit = false;
    float mx = -1, my = -1;
    std::vector<Button> buttons;
    SDL_FRect field{}, tick{};
    while (!quit)
    {
        in.poll();
        if (in.poll_open() && start_when_read)
        {
            start_when_read = false;
            if (in.screen == Screen::Ready) in.start();
        }
        // The questions answered from --answers, while there are letters left (then, unattended,
        // the rest are left unanswered and the run ends: refused).
        if (in.screen == Screen::Asking && answered < answers.size())
        {
            const char c = answers[answered++];
            if (c == 'c') in.screen = Screen::Cancelled;
            else in.answer(c == 'y' || c == 'a', c == 'a' || c == 'x');
        }
        if (yes && in.screen != Screen::Installing && in.screen != Screen::Reading && !(in.screen == Screen::Asking && answered < answers.size()))
            quit = true; // unattended: the result is the exit code
        // A warning (a RAR its contents only, its rar not found) pops up once the install is done,
        // before the window closes; unattended, it goes to the error output instead.
        if (in.screen == Screen::Done && !in.warned)
        {
            in.warned = true;
            std::string w;
            for (const InstallNote& n : in.notes)
                if (n.warning) w += (w.empty() ? "" : "\n\n") + n.text;
            for (const InstallNote& n : in.notes) std::fprintf(n.warning ? stderr : stdout, "%s%s\n", n.warning ? "warning: " : "note: ", n.text.c_str());
            for (const std::string& p : in.keep) std::printf("note: %s was already there and is left as it was\n", p.c_str());
            if (!w.empty() && !yes) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Sieve Installer", w.c_str(), window);
        }
        if (quick && (in.screen == Screen::Done || in.screen == Screen::Cancelled)) quit = true;
        {
            std::lock_guard<std::mutex> lock(in.dialog_mx);
            if (in.chosen && (in.manifest || in.any) && in.screen == Screen::Ready) // (a dialog the system leaves open
            {                                                                    // while installing: its answer kept for Ready)
                in.set_dest(in.where_in(from_u8(*in.chosen)));
                in.chosen.reset();
            }
        }
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT && in.screen != Screen::Installing) quit = true;
            if (e.type == SDL_EVENT_DROP_FILE && in.screen != Screen::Installing && e.drop.data) in.open(from_u8(e.drop.data));
            // The address bar's editing: the selection taken out, and what is typed or pasted put in its
            // place at the cursor (line ends and tabs left out: a path has none).
            const auto sel_lo = [&] { return std::min(in.cur, in.anchor); };
            const auto sel_hi = [&] { return std::max(in.cur, in.anchor); };
            const auto erase_selection = [&] {
                if (in.cur == in.anchor) return false;
                in.dest.erase(sel_lo(), sel_hi() - sel_lo());
                in.cur = in.anchor = sel_lo();
                return true;
            };
            const auto put = [&](std::string t) {
                t.erase(std::remove_if(t.begin(), t.end(), [](char c) { return c == '\n' || c == '\r' || c == '\t'; }), t.end());
                // A path pasted in quotes (Explorer's "Copy as path") is the path without them.
                if (t.size() >= 2 && t.front() == '"' && t.back() == '"') t = t.substr(1, t.size() - 2);
                erase_selection();
                in.dest.insert(in.cur, t);
                in.cur = in.anchor = in.cur + t.size();
            };
            // The character a point of the field is at (each 12 pixels wide, from the first shown).
            const auto char_at = [&](float x) {
                const float k = (x - field.x - 8 + 6) / 12;
                return utf8_at(in.dest, in.view + size_t(k < 0 ? 0 : k));
            };
            if (e.type == SDL_EVENT_MOUSE_MOTION)
            {
                mx = e.motion.x;
                my = e.motion.y;
                if (in.dragging && in.editing) in.cur = char_at(e.motion.x);
            }
            if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT) in.dragging = false;
            if (e.type == SDL_EVENT_TEXT_INPUT && in.editing) put(e.text.text);
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
                    if (on_field)
                    {
                        // A click puts the cursor there (Shift: the selection to there); a double
                        // click selects it all; dragging selects.
                        if (e.button.clicks >= 2)
                        {
                            in.anchor = 0;
                            in.cur = in.dest.size();
                        }
                        else
                        {
                            in.cur = char_at(e.button.x);
                            if (!(SDL_GetModState() & SDL_KMOD_SHIFT)) in.anchor = in.cur;
                            in.dragging = true;
                        }
                    }
                    if (inside(tick, e.button.x, e.button.y)) in.replace = !in.replace;
                }
            }
            if (e.type == SDL_EVENT_KEY_DOWN)
            {
                // (Ctrl without Alt: AltGr arrives as Ctrl+Alt on Windows, and AltGr+A types a letter.)
                const bool ctrl = (e.key.mod & SDL_KMOD_CTRL) != 0 && (e.key.mod & SDL_KMOD_ALT) == 0, shift = (e.key.mod & SDL_KMOD_SHIFT) != 0;
                const SDL_Keycode k = e.key.key;
                // Moves the cursor; Shift keeps the selection's other end where it is.
                const auto move_to = [&](size_t at) {
                    in.cur = at;
                    if (!shift) in.anchor = at;
                };
                if (in.editing && k == SDLK_BACKSPACE)
                {
                    if (!erase_selection() && in.cur > 0)
                    {
                        const size_t at = utf8_prev(in.dest, in.cur);
                        in.dest.erase(at, in.cur - at);
                        in.cur = in.anchor = at;
                    }
                }
                else if (in.editing && k == SDLK_DELETE)
                {
                    if (!erase_selection() && in.cur < in.dest.size()) in.dest.erase(in.cur, utf8_next(in.dest, in.cur) - in.cur);
                }
                else if (in.editing && ctrl && k == SDLK_A)
                {
                    in.anchor = 0;
                    in.cur = in.dest.size();
                }
                else if (in.editing && ctrl && (k == SDLK_C || k == SDLK_X))
                {
                    if (in.cur != in.anchor)
                    {
                        SDL_SetClipboardText(in.dest.substr(sel_lo(), sel_hi() - sel_lo()).c_str());
                        if (k == SDLK_X) erase_selection();
                    }
                }
                else if (in.editing && ctrl && k == SDLK_V)
                {
                    if (char* clip = SDL_GetClipboardText())
                    {
                        put(clip);
                        SDL_free(clip);
                    }
                }
                else if (in.editing && k == SDLK_LEFT)
                    move_to(!shift && in.cur != in.anchor ? sel_lo() : utf8_prev(in.dest, in.cur));
                else if (in.editing && k == SDLK_RIGHT)
                    move_to(!shift && in.cur != in.anchor ? sel_hi() : utf8_next(in.dest, in.cur));
                else if (in.editing && k == SDLK_HOME) move_to(0);
                else if (in.editing && k == SDLK_END) move_to(in.dest.size());
                else if (in.screen == Screen::Asking)
                {
                    // Y, N, A (yes to all), Shift+N (no to all); Escape cancels. Enter answers
                    // nothing: a file is not written over by a key pressed for something else.
                    if (k == SDLK_Y) pressed = "Yes";
                    else if (k == SDLK_N) pressed = shift ? "No to all" : "No";
                    else if (k == SDLK_A) pressed = "Yes to all";
                    else if (k == SDLK_ESCAPE) pressed = "Cancel";
                }
                else if (e.key.key == SDLK_RETURN || e.key.key == SDLK_KP_ENTER)
                    pressed = in.screen == Screen::Ready ? "Install" : in.screen == Screen::Installing || in.screen == Screen::Reading ? "" : "Close";
                else if (e.key.key == SDLK_ESCAPE && in.editing)
                {
                    in.editing = false; // (Escape while typing leaves the field, not the installer)
                    SDL_StopTextInput(window);
                }
                else if (e.key.key == SDLK_ESCAPE) pressed = in.screen == Screen::Installing ? "Cancel" : in.screen == Screen::Ready ? "Cancel" : "Close";
            }
            if (pressed == "Install") { in.editing = false; SDL_StopTextInput(window); in.start(); }
            else if (pressed == "Browse...")
            {
                const std::string start = in.dest;
                hallway::open_folder_dialog(folder_chosen, &in, window, start.empty() ? nullptr : start.c_str(), false);
            }
            else if (pressed == "Cancel" && in.screen == Screen::Installing) in.cancel = true;
            else if (in.screen == Screen::Asking && !pressed.empty())
            {
                if (pressed == "Yes") in.answer(true, false);
                else if (pressed == "No") in.answer(false, false);
                else if (pressed == "Yes to all") in.answer(true, true);
                else if (pressed == "No to all") in.answer(false, true);
                // Cancel: nothing written. From the right-click menu the window closes, as when
                // a cancel comes while unpacking; otherwise it is back to choose again.
                else if (pressed == "Cancel") in.screen = quick ? Screen::Cancelled : Screen::Ready;
            }
            else if (pressed == "Back" && in.screen == Screen::Failed)
            {
                in.screen = Screen::Ready; // to tick Replace, or choose another folder, and Install again
                in.error.clear();
            }
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
        if (in.screen == Screen::Reading)
        {
            text(r, 24, 90, "Reading " + tail(u8(in.source.filename()), 40) + "...", 2, kInk);
            text(r, 24, 130, "Every file is checked before anything is written.", 1.5f, kDim);
            buttons.push_back({{W - 164, H - 60, 140, 40}, "Close"});
        }
        else if (in.screen == Screen::NoInstaller)
        {
            text(r, 24, 90, "Nothing to install yet.", 2, kInk);
            text(r, 24, 130, tail(in.error, 80), 1, kRed);
            buttons.push_back({{W - 164, H - 60, 140, 40}, "Close"});
        }
        else
        {
            if (in.any)
            {
                row(80, "Unpacks", u8(in.source.filename()) + " (" + in.any->format + ")");
                row(104, "Contents", files_n(in.any->files) + ", " + human_bytes(in.any->bytes) +
                                         (in.any->links_left_out ? ", " + std::to_string(in.any->links_left_out) + " link(s) left out" : ""));
            }
            else if (in.archive)
            {
                const Manifest& m = *in.manifest;
                row(80, "Unpacks", m.root + " (7z, " + human_bytes(m.bytes) + ")");
                row(104, "Contents", files_n(in.archive->files) + ", " + human_bytes(in.archive->bytes) +
                                         (in.archive->top.empty() ? "" : ", from " + in.archive->top));
            }
            else
            {
                const Manifest& m = *in.manifest;
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
                // What fits of the address, scrolled so the cursor is in sight; the selection behind
                // it, and the cursor, while it is being edited.
                in.cur = std::min(in.cur, in.dest.size());
                in.anchor = std::min(in.anchor, in.dest.size());
                const size_t fits = size_t(std::max(1.0f, (field.w - 16) / 12)), n = utf8_chars(in.dest, 0, in.dest.size());
                const size_t at = utf8_chars(in.dest, 0, in.cur);
                if (!in.editing) in.view = n > fits ? n - fits : 0; // (not being edited: its end shown, as before)
                if (at < in.view) in.view = at;
                if (at > in.view + fits) in.view = at - fits;
                if (in.view + fits > n) in.view = n > fits ? n - fits : 0;
                const size_t from = utf8_at(in.dest, in.view), to = utf8_at(in.dest, in.view + fits);
                if (in.editing && in.cur != in.anchor)
                {
                    const size_t lo = std::max(utf8_chars(in.dest, 0, std::min(in.cur, in.anchor)), in.view);
                    const size_t hi = std::min(utf8_chars(in.dest, 0, std::max(in.cur, in.anchor)), in.view + fits);
                    if (hi > lo)
                    {
                        SDL_SetRenderDrawColor(r, 40, 80, 150, 255);
                        const SDL_FRect band{field.x + 8 + float(lo - in.view) * 12, field.y + 6, float(hi - lo) * 12, field.h - 12};
                        SDL_RenderFillRect(r, &band);
                    }
                }
                text(r, field.x + 8, field.y + 9, in.dest.substr(from, to - from), 1.5f, kInk);
                if (in.editing && (SDL_GetTicks() / 500) % 2 == 0)
                {
                    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
                    const SDL_FRect caret{field.x + 8 + float(at - in.view) * 12 - 1, field.y + 6, 2, field.h - 12};
                    SDL_RenderFillRect(r, &caret);
                }
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
                if (in.any)
                {
                    text(r, 24, 290, "Each file is read from the archive (checked against its CRC, where it keeps one)", 1, kDim);
                    text(r, 24, 304, "and written as it is. Cancel removes whatever was written.", 1, kDim);
                }
                else if (in.archive)
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
                row(152, "To", tail(in.dest, 48)); // (48 at this size reach the window's edge)
                const size_t d = in.done, t = std::max<size_t>(1, in.total);
                if (in.screen == Screen::Asking)
                {
                    const std::string& p = in.clashes[in.asking];
                    text(r, 24, 196, "Already there (" + std::to_string(in.asking + 1) + " of " + std::to_string(in.clashes.size()) + "):", 2, kInk);
                    text(r, 24, 228, tail(p, 60), 1.5f, kAccent);
                    std::error_code ec;
                    const fs::path here = from_u8(in.dest) / from_u8(p);
                    const uint64_t now = fs::is_regular_file(here, ec) ? uint64_t(fs::file_size(here, ec)) : 0;
                    const auto given = in.given_size(p);
                    text(r, 24, 256, "There now: " + (fs::is_regular_file(here, ec) ? human_bytes(now) : std::string("a folder")) +
                                         (given ? ".   From the installer: " + human_bytes(*given) + "." : "."),
                         1.5f, kDim);
                    text(r, 24, 290, "Replace it? No leaves it as it is and writes the rest; Cancel writes nothing.", 1, kDim);
                    text(r, 24, 306, "Keys: Y yes, N no, A yes to all, Shift+N no to all, Escape cancel.", 1, kDim);
                    buttons.push_back({{24, H - 60, 96, 40}, "Yes"});
                    buttons.push_back({{132, H - 60, 96, 40}, "No"});
                    buttons.push_back({{240, H - 60, 176, 40}, "Yes to all"});
                    buttons.push_back({{428, H - 60, 160, 40}, "No to all"});
                    buttons.push_back({{W - 160, H - 60, 136, 40}, "Cancel"});
                }
                else if (in.screen == Screen::Installing)
                {
                    const bool writing = in.phase == 1;
                    text(r, 24, 196, in.archive || in.any ? "Unpacking" : writing ? "Writing" : "Checking", 2, kInk);
                    // Checking is the first half of the bar, writing the second (unpacking, the whole).
                    const float f = in.archive || in.any ? float(d) / float(t) : (float(d) / float(t) + (writing ? 1.0f : 0.0f)) / 2.0f;
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
                    if (in.any)
                        text(r, 24, 228, files_n(in.any->files) + " unpacked from " + u8(in.source.filename()) + ".", 1.5f, kInk);
                    else if (in.archive)
                        text(r, 24, 228, files_n(in.archive->files) + " unpacked from " + in.manifest->root + ", every one checked.", 1.5f, kInk);
                    else
                        text(r, 24, 228, files_n(in.manifest->files) + (in.manifest->files == 1 ? ", checked against its SHA-256." : ", every one checked against its SHA-256."), 1.5f, kInk);
                    if (!in.keep.empty())
                        text(r, 24, 246, files_n(in.keep.size()) + " already there left as " + (in.keep.size() == 1 ? "it was." : "they were."), 1, kDim);
                    float y = 262;
                    for (const InstallNote& n : in.notes)
                        for (size_t at = 0; at < n.text.size() && y < H - 80; at += 88, y += 14)
                            text(r, 24, y, n.text.substr(at, 88), 1, n.warning ? SDL_Color{255, 190, 60, 255} : kDim);
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
                    buttons.push_back({{W - 320, H - 60, 140, 40}, "Back"});
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
    if (yes && code != 0 && !in.error.empty()) std::fprintf(stderr, "sieve-install: %s\n", in.error.c_str()); // unattended: say why
    SDL_DestroyRenderer(r);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return code;
}
