// The File Locator, in the hallway (pause menu): `sieve locate` with a window round it.
//
// Choose a file or a folder (the system's picker, or drop one on the window). A file is named by
// its place on the binary line and its SHA-256, set beside what zip and 7z make of it, and can be
// walked to: the locator puts you on the binary line in front of it, if the line is long enough to
// hold it. A folder is walked and listed as a manifest, compared the same way (with its installer's
// manifest compressed too), and can be saved as a manifest or made into an installer that
// sieve-install opens. The reading, hashing and compressing run on a worker thread, so the hallway
// goes on drawing; the code is the tool's own (tools/cli/locate.*, compare.*), so the two agree.

#include "hallway.hpp"

#include "cli/compare.hpp"
#include "cli/locate.hpp"
#include "cli/timings.hpp"
#include "cli/vault.hpp"

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

// A single file's Sieve instructions, from the bytes already read: as a folder holding just it.
cli::Manifest file_instructions(const fs::path& p, const std::vector<uint8_t>& bytes)
{
    cli::Manifest m = cli::manifest_of_file(p);
    m.contents = bytes;
    m.with_contents = true;
    return m;
}

std::string head_tail(const std::string& h, size_t each)
{
    return h.size() <= 2 * each + 3 ? h : h.substr(0, each) + "..." + h.substr(h.size() - each);
}

void SDLCALL picked(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    static_cast<Hallway*>(user)->locator_picked(files[0]);
}

void SDLCALL save_to(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<Hallway*>(user)->locator_save_to(files[0]);
}

void SDLCALL install_from(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<Hallway*>(user)->locator_install_picked(0, files[0]);
}

void SDLCALL install_to(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<Hallway*>(user)->locator_install_picked(1, files[0]);
}

} // namespace

void Hallway::open_locator()
{
    loc_open_ = true;
    SDL_SetWindowRelativeMouseMode(window_, false);
}

void Hallway::close_locator()
{
    loc_open_ = false;
    if (!pause_open_) SDL_SetWindowRelativeMouseMode(window_, true);
}

// From the dialogs' callbacks, which may run on another thread: handed to the frame.
void Hallway::locator_picked(const std::string& path)
{
    std::lock_guard<std::mutex> lock(loc_mx_);
    loc_pending_pick_ = path;
}

void Hallway::locator_save_to(const std::string& path)
{
    std::lock_guard<std::mutex> lock(loc_mx_);
    loc_pending_save_ = path;
}

void Hallway::locator_install_picked(int step, const std::string& path)
{
    std::lock_guard<std::mutex> lock(loc_mx_);
    loc_pending_install_ = std::make_pair(step, path);
}

// Install from an installer (a .sieve, an installer program, or an installer's manifest) into a
// folder, on the worker, as sieve install does: every file checked before any is written, and
// nothing replaced that is already there.
void Hallway::locator_install(const std::string& from, const std::string& to, bool sync)
{
    if (loc_busy_) return;
    if (loc_worker_.joinable()) loc_worker_.join();
    loc_busy_ = true;
    loc_status_ = trf("loc.installing", {to});
    auto job = [this, from, to] {
        std::string done;
        try
        {
            const cli::Manifest m = cli::installable_manifest(from_u8(from), false);
            cli::install_tree(m, from_u8(to), false, [this](int phase, size_t d, size_t t, const std::string& path) {
                std::lock_guard<std::mutex> lock(loc_mx_);
                loc_status_ = trf(phase == 0 ? "loc.install.checking" : "loc.install.writing", {std::to_string(d), std::to_string(t), path});
            });
            done = m.files == 1 ? trf("loc.installed.one", {to}) : trf("loc.installed", {std::to_string(m.files), to});
        }
        catch (const std::exception& e)
        {
            done = trf("loc.install_failed", {e.what()});
        }
        std::lock_guard<std::mutex> lock(loc_mx_);
        loc_status_ = done;
        loc_busy_ = false;
    };
    if (sync) job();
    else loc_worker_ = std::thread(job);
}

// Reads and measures a file or folder on the worker. `sync`: here and now (scripted runs).
void Hallway::locator_analyse(const std::string& path, bool sync)
{
    if (loc_busy_) return;
    loc_go_armed_ = false;
    if (loc_worker_.joinable()) loc_worker_.join();
    loc_busy_ = true;
    {
        std::lock_guard<std::mutex> lock(loc_mx_);
        loc_status_ = trf("loc.reading", {path});
    }
    auto job = [this, path] {
        LocatorResult r;
        r.path = path;
        try
        {
            const fs::path p = from_u8(path);
            cli::Comparison c;
            if (fs::is_regular_file(p))
            {
                r.kind = LocatorResult::File;
                r.bytes = cli::read_file_bytes(p);
                const BigUint a = cli::binary_address(r.bytes);
                r.hex = a.is_zero() ? "0" : a.to_hex();
                r.sha256 = cli::sha256_hex(r.bytes);
                c.original = r.bytes.size();
                c.deflate = cli::deflate_size(r.bytes);
                c.lzma2 = cli::lzma2_size(r.bytes);
                c.address_bytes = (a.bit_length() + 7) / 8;
                c.address_hex = a.is_zero() ? 0 : r.hex.size();
                // Its Sieve instructions, as a folder's: its name, size and SHA-256, then its bytes.
                r.manifest = cli::manifest_of_file(p);
                const std::vector<uint8_t> mb = file_instructions(p, r.bytes).file();
                const BigUint ma = cli::binary_address(mb);
                c.manifest = mb.size();
                c.manifest_deflate = cli::deflate_size(mb);
                c.manifest_lzma2 = cli::lzma2_size(mb);
                c.installer_hex = ma.is_zero() ? 1 : ma.to_hex().size();
                c.installer_raw = (ma.bit_length() + 7) / 8;
            }
            else if (fs::is_directory(p))
            {
                r.kind = LocatorResult::Folder;
                r.manifest = cli::walk_folder(p);
                std::vector<uint8_t> all;
                for (const auto& e : r.manifest.entries)
                {
                    if (e.dir) continue;
                    const auto bytes = cli::read_file_bytes(p / from_u8(e.path));
                    const BigUint a = cli::binary_address(bytes);
                    c.original += bytes.size();
                    c.deflate += cli::deflate_size(bytes);
                    c.address_bytes += (a.bit_length() + 7) / 8;
                    c.address_hex += a.is_zero() ? 0 : a.to_hex().size();
                    all.insert(all.end(), bytes.begin(), bytes.end());
                }
                c.lzma2 = cli::lzma2_size(all);
                // The installer's manifest (v3: every file's bytes after it), compressed, and the installer.
                cli::Manifest with = r.manifest;
                cli::add_contents(with, p);
                const std::vector<uint8_t> mb = with.file();
                const BigUint ma = cli::binary_address(mb);
                c.manifest = mb.size();
                c.manifest_deflate = cli::deflate_size(mb);
                c.manifest_lzma2 = cli::lzma2_size(mb);
                c.installer_hex = ma.is_zero() ? 1 : ma.to_hex().size();
                c.installer_raw = (ma.bit_length() + 7) / 8;
                const std::string plain = r.manifest.text(); // the tree's identity is its v1 manifest's
                r.sha256 = cli::sha256_hex(std::vector<uint8_t>(plain.begin(), plain.end()));
            }
            else throw std::runtime_error(tr("loc.neither"));
            r.table = cli::comparison_table(c);
        }
        catch (const std::exception& e)
        {
            r.kind = LocatorResult::None;
            r.error = e.what();
        }
        std::lock_guard<std::mutex> lock(loc_mx_);
        loc_result_ = std::move(r);
        loc_status_.clear();
        loc_busy_ = false;
    };
    if (sync) job();
    else loc_worker_ = std::thread(job);
}

// What a save dialog chose, done on the worker: a file's address, a folder's manifest, or its
// installer (which reads every file again for its address).
void Hallway::locator_save(const std::string& to)
{
    if (loc_busy_ || loc_result_.kind == LocatorResult::None) return;
    if (loc_worker_.joinable()) loc_worker_.join();
    loc_busy_ = true;
    const int what = loc_save_what_;
    const LocatorResult r = loc_result_;
    loc_worker_ = std::thread([this, what, r, to] {
        std::string done;
        try
        {
            const fs::path out = from_u8(to);
            {
                // A file's instructions are a folder's holding just it (file_instructions).
                cli::Manifest with = r.kind == LocatorResult::File ? file_instructions(from_u8(r.path), r.bytes) : r.manifest;
                if (r.kind != LocatorResult::File) cli::add_contents(with, from_u8(r.path));
                const BigUint address = cli::binary_address(with.file());
                // The installer on its own (for anyone with Sieve: sieve install, or sieve-install),
                // or an installer program, sieve-install with it attached, one file for anyone.
                if (what == 4)
                {
                    cli::write_address_file(out, address, false);
                    done = trf("loc.saved.installer", {to});
                }
                else
                {
                    const char* base = SDL_GetBasePath();
                    const auto stub = cli::installer_program_beside(base ? from_u8(base) : fs::current_path());
                    if (!stub) throw std::runtime_error(tr("loc.no_installer_program"));
                    cli::write_installer_program(*stub, address, out);
                    done = trf("loc.saved.program", {to});
                }
            }
        }
        catch (const std::exception& e)
        {
            done = trf("loc.save_failed", {e.what()});
        }
        std::lock_guard<std::mutex> lock(loc_mx_);
        loc_status_ = done;
        loc_busy_ = false;
    });
}

// Walk to the file: the binary line, in front of it. A file longer than the BINARY length is
// still reached, after a second press: the line is made exactly long enough for it, the hallway
// goes thin (only your room kept), and the ordering positional, the one in which the address is
// the file's own hex dump plus 0101...01 and costs no shuffle to find.
void Hallway::locator_go()
{
    if (loc_result_.kind != LocatorResult::File) return;
    const bool past = loc_result_.bytes.size() > binary_space_->max_bytes();
    if (past && !loc_go_armed_)
    {
        loc_go_armed_ = true;
        return;
    }
    loc_go_armed_ = false;
    const auto bytes = loc_result_.bytes;
    close_locator();
    if (pause_open_) close_pause();
    walk_to_file(bytes, u8(from_u8(loc_result_.path).filename()), past);
}

void Hallway::walk_to_file(const std::vector<uint8_t>& bytes, const std::string& name, bool past)
{
    sieve::cli::timings::Scope timed("hallway.walk"); // to a file: the line made long enough, and the place
    if (cli::vault::withheld_bytes(bytes)) // the vault: nowhere to walk to
    {
        message(tr("vault.withheld"));
        return;
    }
    // What touches the renderer (the pictures let go, the line's colours), here; then the rest,
    // which for a file of megabytes takes seconds, on a worker while the window says LOCATING.
    if (past)
    {
        set_thin(true, kBinaryLine);
        mode_ = AddressMode::Positional;
    }
    clear_faces();
    if (!on_binary())
    {
        binary_from_ = 0; // its door leads to pages, as when you start on binary
        drop_in_hand();
        set_line(kBinaryLine);
    }
    busy(tr("locating"), [&] {
        if (past)
        {
            sieve::cli::timings::Scope t1("hallway.walk.length"); // the line made long enough
            set_binary_length(bytes.size());
        }
        const Space::Digits title = title_for_name(name); // its name is its title
        walked_names_[cli::sha256_hex(bytes)] = name;
        {
            sieve::cli::timings::Scope t2("hallway.walk.place"); // the file's place, and you there
            go_to_file(bytes, true, &title);
        }
        sieve::cli::timings::Scope t3("hallway.walk.room"); // the room's items, ahead of drawing them
        warm_room();
    });
    message(trf(past ? "loc.went_past" : "msg.warped.file", {std::to_string(bytes.size())}));
}

void Hallway::locator_event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_DROP_FILE && e.drop.data) locator_analyse(e.drop.data);
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        loc_mx_pos_ = e.motion.x;
        loc_my_pos_ = e.motion.y;
        SDL_RenderCoordinatesFromWindow(r_, e.motion.x, e.motion.y, &loc_mx_pos_, &loc_my_pos_);
    }
    std::string pressed;
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        float x = e.button.x, y = e.button.y;
        SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &x, &y);
        for (const auto& [rect, id] : loc_buttons_)
            if (x >= rect.x && x < rect.x + rect.w && y >= rect.y && y < rect.y + rect.h) pressed = id;
    }
    if (e.type == SDL_EVENT_KEY_DOWN)
    {
        switch (e.key.key)
        {
        case SDLK_ESCAPE: pressed = "close"; break;
        case SDLK_F: pressed = "file"; break;
        case SDLK_D: pressed = "folder"; break;
        case SDLK_I: pressed = "install"; break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: pressed = loc_result_.kind == LocatorResult::File ? "go" : ""; break;
        default: break;
        }
    }
    if (pressed.empty() || (loc_busy_ && pressed != "close")) return;
    if (pressed != "go") loc_go_armed_ = false; // "again" means the very next press
    if (pressed == "close") close_locator();
    else if (pressed == "file") SDL_ShowOpenFileDialog(picked, this, window_, nullptr, 0, nullptr, false);
    else if (pressed == "folder") SDL_ShowOpenFolderDialog(picked, this, window_, nullptr, false);
    else if (pressed == "go") locator_go();
    else if (pressed == "install")
    {
        // First the installer, then (install_from, below in draw_locator) the folder to put it in.
        static const SDL_DialogFileFilter kFrom[] = {{"Sieve instructions", "sieve"}, {"Installer program", "exe"}};
        SDL_ShowOpenFileDialog(install_from, this, window_, kFrom, 2, nullptr, false);
    }
    else
    {
        // A save, for a file as for a folder: 3 an installer program (sieve-install with the
        // instructions attached), 4 the Sieve instructions on their own (.sieve: for anyone who
        // has Sieve).
        loc_save_what_ = pressed == "save_program" ? 3 : 4;
        const fs::path from = from_u8(loc_result_.path);
#ifdef _WIN32
        const char* program = " installer.exe";
#else
        const char* program = "-installer";
#endif
        const std::string name = u8(from.filename()) + (loc_save_what_ == 3 ? program : ".sieve");
        const std::string start = u8(from.parent_path() / from_u8(name));
        static const SDL_DialogFileFilter kProgram[] = {{"Installer program", "exe"}};
        static const SDL_DialogFileFilter kSieve[] = {{"Sieve instructions", "sieve"}};
        const SDL_DialogFileFilter* filter = loc_save_what_ == 3 ? kProgram : loc_save_what_ == 4 ? kSieve : nullptr;
        SDL_ShowSaveFileDialog(save_to, this, window_, filter, filter ? 1 : 0, start.c_str());
    }
}

void Hallway::draw_locator(float W, float H)
{
    // Hand over whatever the dialogs chose since the last frame.
    {
        std::optional<std::string> pick, save;
        {
            std::lock_guard<std::mutex> lock(loc_mx_);
            pick.swap(loc_pending_pick_);
            save.swap(loc_pending_save_);
        }
        if (pick) locator_analyse(*pick);
        if (save) locator_save(*save);
        std::optional<std::pair<int, std::string>> inst;
        {
            std::lock_guard<std::mutex> lock(loc_mx_);
            inst.swap(loc_pending_install_);
        }
        if (inst && inst->first == 0)
        {
            loc_install_src_ = inst->second;
            loc_status_ = trf("loc.install.where", {inst->second});
            SDL_ShowOpenFolderDialog(install_to, this, window_, nullptr, false);
        }
        else if (inst && inst->first == 1 && !loc_install_src_.empty()) locator_install(loc_install_src_, inst->second);
    }
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{255, 255, 255, 255}, red{255, 80, 80, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);
    text(20, 16, tr("loc.title"), 3, ink);
    text(20, 48, fit(tr("loc.subtitle"), W - 40, 1), 1, grey);
    loc_buttons_.clear();
    float bx = 20;
    auto button = [&](const std::string& id, const std::string& label, float y, bool enabled = true) {
        const float w = text_width(label, 2) + 24;
        const SDL_FRect r{bx, y, w, 30};
        const bool hot = loc_mx_pos_ >= r.x && loc_mx_pos_ < r.x + r.w && loc_my_pos_ >= r.y && loc_my_pos_ < r.y + r.h;
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, hot && enabled ? 60 : 20);
        SDL_RenderFillRect(r_, &r);
        SDL_SetRenderDrawColor(r_, enabled ? ink.r : 90, enabled ? ink.g : 90, enabled ? ink.b : 90, 255);
        SDL_RenderRect(r_, &r);
        text(r.x + 12, r.y + 7, label, 2, enabled ? white : grey);
        if (enabled) loc_buttons_.emplace_back(r, id);
        bx += w + 12;
    };
    const bool busy = loc_busy_;
    button("file", tr("loc.choose_file"), 76, !busy);
    button("folder", tr("loc.choose_folder"), 76, !busy);
    button("install", tr("loc.install"), 76, !busy);
    float y = 126;
    std::string status;
    {
        std::lock_guard<std::mutex> lock(loc_mx_);
        status = loc_status_;
    }
    const LocatorResult& r = loc_result_;
    if (!busy && r.kind == LocatorResult::None && r.error.empty())
        text(20, y, tr("loc.empty"), 2, grey);
    else if (!r.error.empty() && !busy) text(20, y, fit(trf("loc.failed", {r.error}), W - 40, 1), 1, red);
    else if (r.kind != LocatorResult::None && !busy)
    {
        auto line = [&](const std::string& label, const std::string& value) {
            text(20, y, label, 1, grey);
            text(150, y, fit(value, W - 170, 1), 1, white);
            y += 16;
        };
        if (r.kind == LocatorResult::File)
        {
            line(tr("loc.file"), r.path);
            line(tr("loc.bytes"), std::to_string(r.bytes.size()));
            line(tr("loc.sha256"), r.sha256);
            line(tr("loc.address"), trf("loc.address.value", {std::to_string(r.hex.size()), head_tail(r.hex, 24)}));
            line(tr("loc.line"), trf("loc.line.value", {std::to_string(r.bytes.size()), std::to_string(binary_space_->max_bytes())}));
            if (loc_go_armed_)
            {
                text(20, y, fit(trf("loc.go_anyway", {std::to_string(r.bytes.size())}), W - 40, 1), 1, SDL_Color{255, 200, 80, 255});
                y += 16;
            }
        }
        else
        {
            size_t dirs = 0;
            for (const auto& e : r.manifest.entries) dirs += e.dir ? 1 : 0;
            line(tr("loc.folder"), r.path);
            line(tr("loc.contents"), trf("loc.contents.value", {std::to_string(r.manifest.files), std::to_string(dirs),
                                                                 std::to_string(r.manifest.bytes)}));
            line(tr("loc.identity"), r.sha256);
            if (r.manifest.skipped) line(tr("loc.skipped"), std::to_string(r.manifest.skipped));
        }
        y += 10;
        size_t at = 0;
        while (at < r.table.size())
        {
            const size_t nl = r.table.find('\n', at);
            const std::string row = r.table.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
            text(20, y, fit(row, W - 40, 1), 1, row.rfind("comparison", 0) == 0 ? grey : ink);
            y += 13;
            if (nl == std::string::npos) break;
            at = nl + 1;
        }
        y += 14;
        bx = 20;
        // A file can be walked to. A file or a folder can be saved as Sieve instructions (the
        // .sieve: its listing and its files, as one number) for anyone who has Sieve, or as an
        // installer program for anyone; a file exactly as a folder holding just it. The listing on
        // its own (the manifest) is for the tools and the command line (sieve locate --manifest),
        // and a file's bare address for the command line (sieve locate FILE --out).
        if (r.kind == LocatorResult::File) button("go", tr("loc.go"), y);
        button("save_sieve", tr("loc.save_sieve"), y);
        button("save_program", tr("loc.save_program"), y);
    }
    if (!status.empty()) text(20, H - 58, fit(status, W - 40, 1), 1, busy ? white : ink);
    text(20, H - 26, fit(tr("loc.keys"), W - 40, 1), 1, grey);
}

// For scripted runs (--locate PATH): open the locator on it, measured before the next frame.
void Hallway::locate_now(const std::string& path)
{
    open_locator();
    locator_analyse(path, true);
}

void Hallway::stop_locator()
{
    if (loc_worker_.joinable()) loc_worker_.join();
}

} // namespace hallway::hall
