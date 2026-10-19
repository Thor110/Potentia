// Sieve hallway -- the File Locator (file_locator.hpp): `sieve locate` with a window round it, from
// the main menu or the hallway's pause menu.

#include "file_locator.hpp"

#include "dialogs.hpp"
#include "font.hpp"
#include "strings.hpp"

#include "cli/compare.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "cli/pack.hpp"
#include "cli/timings.hpp"

#include <stdexcept>

namespace hallway {

using namespace sieve;
using namespace sieve::cli;

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

// A single file's Sieve instructions, from the bytes already read: as a folder holding just it,
// packed, its items of Sieve's lines carried as their places where that makes it smaller, and the
// file pulled tight by the rope (searched for `rope_seconds`, 0: no limit) where that does.
cli::Manifest file_instructions(const fs::path& p, const std::vector<uint8_t>& bytes, const cli::Args& items, double rope_seconds)
{
    cli::Manifest m = cli::manifest_of_file(p);
    m.contents = bytes;
    m.with_contents = true;
    cli::add_packed_items(m, p.parent_path(), items, nullptr, 8, false, rope_seconds);
    return m;
}

std::string head_tail(const std::string& h, size_t each)
{
    return h.size() <= 2 * each + 3 ? h : h.substr(0, each) + "..." + h.substr(h.size() - each);
}

void SDLCALL on_picked(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return; // cancelled, or the dialog failed
    static_cast<FileLocator*>(user)->picked(files[0]);
}

void SDLCALL on_save_to(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<FileLocator*>(user)->save_to(files[0]);
}

void SDLCALL on_install_from(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<FileLocator*>(user)->install_picked(0, files[0]);
}

void SDLCALL on_install_to(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    static_cast<FileLocator*>(user)->install_picked(1, files[0]);
}

} // namespace

FileLocator::FileLocator(SDL_Window* window, SDL_Renderer* renderer, Host host) : window_(window), r_(renderer), host_(std::move(host)) {}

void FileLocator::open()
{
    open_ = true;
    SDL_SetWindowRelativeMouseMode(window_, false);
}

void FileLocator::close()
{
    open_ = false;
    if (host_.closed) host_.closed();
}

void FileLocator::say(const std::string& status)
{
    std::lock_guard<std::mutex> lock(mx_);
    status_ = status;
}

// From the dialogs' callbacks, which may run on another thread: handed to the frame.
void FileLocator::picked(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mx_);
    pending_pick_ = path;
}

void FileLocator::save_to(const std::string& path)
{
    std::lock_guard<std::mutex> lock(mx_);
    pending_save_ = path;
}

void FileLocator::install_picked(int step, const std::string& path)
{
    std::lock_guard<std::mutex> lock(mx_);
    pending_install_ = std::make_pair(step, path);
}

// Install from an installer (a .sieve, an installer program, or an installer's manifest) into a
// folder, on the worker, as sieve install does: every file checked before any is written, and
// nothing replaced that is already there.
void FileLocator::install(const std::string& from, const std::string& to, bool sync)
{
    if (busy_) return;
    if (worker_.joinable()) worker_.join();
    busy_ = true;
    status_ = trf("loc.installing", {to});
    auto job = [this, from, to] {
        std::string done;
        try
        {
            const cli::Manifest m = cli::installable_manifest(from_u8(from), false);
            cli::install_tree(m, from_u8(to), false, [this](int phase, size_t d, size_t t, const std::string& path) {
                std::lock_guard<std::mutex> lock(mx_);
                status_ = trf(phase == 0 ? "loc.install.checking" : "loc.install.writing", {std::to_string(d), std::to_string(t), path});
            });
            done = m.files == 1 ? trf("loc.installed.one", {to}) : trf("loc.installed", {std::to_string(m.files), to});
        }
        catch (const std::exception& e)
        {
            done = trf("loc.install_failed", {e.what()});
        }
        std::lock_guard<std::mutex> lock(mx_);
        status_ = done;
        busy_ = false;
    };
    if (sync) job();
    else worker_ = std::thread(job);
}

// Reads and measures a file or folder on the worker. `sync`: here and now (scripted runs).
void FileLocator::analyse(const std::string& path, bool sync, bool tailor)
{
    if (busy_) return;
    go_armed_ = false;
    if (worker_.joinable()) worker_.join();
    busy_ = true;
    {
        std::lock_guard<std::mutex> lock(mx_);
        status_ = trf(tailor ? "loc.tailoring" : "loc.reading", {path});
    }
    // The search's time as last set (a file's or the run's); the other is worked out from the files
    // counted, once they are.
    const double per_file = search_per_file_, total = search_total_;
    const bool by_total = search_by_total_;
    auto job = [this, path, tailor, per_file, total, by_total] {
        Result r;
        r.path = path;
        try
        {
            const fs::path p = from_u8(path);
            cli::Comparison c;
            if (fs::is_regular_file(p))
            {
                r.kind = Result::File;
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
                const cli::Manifest fi = file_instructions(p, r.bytes, host_.item_args ? host_.item_args() : cli::Args{}, by_total ? total : per_file);
                const std::vector<uint8_t> mb = fi.file();
                c.packed = fi.packed;
                c.version = fi.version();
                const BigUint ma = cli::binary_address(mb);
                c.manifest = mb.size();
                c.manifest_deflate = cli::deflate_size(mb);
                c.manifest_lzma2 = cli::lzma2_size(mb);
                c.installer_hex = ma.is_zero() ? 1 : ma.to_hex().size();
                c.installer_raw = (ma.bit_length() + 7) / 8;
            }
            else if (fs::is_directory(p))
            {
                r.kind = Result::Folder;
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
                // The installer's manifest (v4: every file packed after it), compressed, and the installer.
                cli::Manifest with = r.manifest;
                {
                    // The rope's time for each file: the file's, or the run's shared among the files.
                    size_t n = 0;
                    for (const auto& e : r.manifest.entries) n += e.dir ? 0 : 1;
                    cli::add_packed_items(with, p, host_.item_args ? host_.item_args() : cli::Args{}, nullptr, 8, false,
                                          by_total ? cli::budget_per_file(total, n) : per_file);
                }
                c.packed = with.packed;
                c.version = with.version();
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
            // Every file weighed against its own address, under this hallway's lines and filters
            // (the pages, image, audio and video lines' units, and the binary line's filters).
            {
                const cli::WeighLines wl = host_.lines();
                std::vector<std::string> files;
                fs::path root;
                if (r.kind == Result::File) files.push_back(path);
                else
                {
                    root = p;
                    for (const auto& e : r.manifest.entries)
                        if (!e.dir) files.push_back(e.path);
                }
                cli::WeighBudget budget;
                if (by_total) budget = {cli::budget_per_file(total, files.size()), total};
                else budget = {per_file, cli::budget_total(per_file, files.size())};
                const cli::Weighing w = cli::weigh_files(root, files, wl, tailor, nullptr, budget);
                r.weighing = cli::weighing_table(w, 12);
                r.tailored = tailor;
                if (tailor)
                {
                    FilterConfig next = wl.filters;
                    bool any = false;
                    for (const cli::LineWeight& l : w.lines)
                        if (l.tailored && l.files > 0)
                        {
                            const LineKind k = l.name == "pages" ? LineKind::Text : cli::line_from_string(l.name);
                            const bool full = next.of(k).mode == FilterMode::Full; // a line kept full stays full
                            next.of(k) = l.tailored->filters;
                            if (full) next.of(k).mode = FilterMode::Full;
                            any = true;
                        }
                    if (any) r.tailored_filters = std::move(next);
                }
            }
        }
        catch (const std::exception& e)
        {
            r.kind = Result::None;
            r.error = e.what();
        }
        std::lock_guard<std::mutex> lock(mx_);
        result_ = std::move(r);
        relink_search(); // the files counted may have changed
        status_.clear();
        busy_ = false;
    };
    if (sync) job();
    else worker_ = std::thread(job);
}

// What a save dialog chose, done on the worker: a file's address, a folder's manifest, or its
// installer (which reads every file again for its address).
void FileLocator::save(const std::string& to)
{
    if (busy_ || result_.kind == Result::None) return;
    if (worker_.joinable()) worker_.join();
    busy_ = true;
    const int what = save_what_;
    const Result r = result_;
    const double rope_seconds = search_per_file_; // the rope's time for each file, as the search row sets it
    worker_ = std::thread([this, what, r, to, rope_seconds] {
        std::string done;
        try
        {
            const fs::path out = from_u8(to);
            {
                // A file's instructions are a folder's holding just it (file_instructions).
                const cli::Args items = host_.item_args ? host_.item_args() : cli::Args{};
                cli::Manifest with = r.kind == Result::File ? file_instructions(from_u8(r.path), r.bytes, items, rope_seconds) : r.manifest;
                if (r.kind != Result::File) cli::add_packed_items(with, from_u8(r.path), items, nullptr, 8, false, rope_seconds);
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
        std::lock_guard<std::mutex> lock(mx_);
        status_ = done;
        busy_ = false;
    });
}

// Walk to the file: the binary line, in front of it. A file longer than the BINARY length is
// still reached, after a second press: the line is made exactly long enough for it, the hallway
// goes thin (only your room kept), and the ordering positional, the one in which the address is
// the file's own hex dump plus 0101...01 and costs no shuffle to find.
void FileLocator::go()
{
    if (result_.kind != Result::File) return;
    if (!in_world()) return;
    const bool past = host_.binary_bytes && result_.bytes.size() > host_.binary_bytes();
    if (past && !go_armed_)
    {
        go_armed_ = true;
        return;
    }
    go_armed_ = false;
    const auto bytes = result_.bytes;
    const std::string name = u8(from_u8(result_.path).filename());
    close();
    host_.go(bytes, name, past);
}

void FileLocator::event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_DROP_FILE && e.drop.data) analyse(e.drop.data);
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        mouse_x_ = e.motion.x;
        mouse_y_ = e.motion.y;
        SDL_RenderCoordinatesFromWindow(r_, e.motion.x, e.motion.y, &mouse_x_, &mouse_y_);
    }
    std::string pressed;
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        float x = e.button.x, y = e.button.y;
        SDL_RenderCoordinatesFromWindow(r_, e.button.x, e.button.y, &x, &y);
        for (const auto& [rect, id] : buttons_)
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
        // The search's time: [ and ] a file, with Shift the run.
        case SDLK_LEFTBRACKET: pressed = (e.key.mod & SDL_KMOD_SHIFT) ? "search_run_less" : "search_file_less"; break;
        case SDLK_RIGHTBRACKET: pressed = (e.key.mod & SDL_KMOD_SHIFT) ? "search_run_more" : "search_file_more"; break;
        case SDLK_RETURN:
        case SDLK_KP_ENTER: pressed = result_.kind == Result::File && in_world() ? "go" : ""; break;
        default: break;
        }
    }
    if (pressed.empty() || (busy_ && pressed != "close")) return;
    if (pressed != "go") go_armed_ = false; // "again" means the very next press
    if (pressed == "close") close();
    else if (pressed == "file") open_file_dialog(on_picked, this, window_, nullptr, 0, nullptr, false);
    else if (pressed == "folder") open_folder_dialog(on_picked, this, window_, nullptr, false);
    else if (pressed == "go") go();
    else if (pressed == "tailor") analyse(result_.path, false, true);
    else if (pressed == "search_file_less") step_search(false, -1);
    else if (pressed == "search_file_more") step_search(false, 1);
    else if (pressed == "search_run_less") step_search(true, -1);
    else if (pressed == "search_run_more") step_search(true, 1);
    else if (pressed == "use_filters" && result_.tailored_filters)
    {
        // As Return on COST does: the settings saved, and a hallway built with them (in the world)
        // or kept for the next one built (from the main menu).
        if (host_.use_filters) host_.use_filters(*result_.tailored_filters);
    }
    else if (pressed == "install")
    {
        // First the installer, then (on_install_from, below in draw) the folder to put it in.
        static const SDL_DialogFileFilter kFrom[] = {{"Sieve instructions", "sieve"}, {"Installer program", "exe"}};
        open_file_dialog(on_install_from, this, window_, kFrom, 2, nullptr, false);
    }
    else
    {
        // A save, for a file as for a folder: 3 an installer program (sieve-install with the
        // instructions attached), 4 the Sieve instructions on their own (.sieve: for anyone who
        // has Sieve).
        save_what_ = pressed == "save_program" ? 3 : 4;
        const fs::path from = from_u8(result_.path);
#ifdef _WIN32
        const char* program = " installer.exe";
#else
        const char* program = "-installer";
#endif
        const std::string name = u8(from.filename()) + (save_what_ == 3 ? program : ".sieve");
        const std::string start = u8(from.parent_path() / from_u8(name));
        static const SDL_DialogFileFilter kProgram[] = {{"Installer program", "exe"}};
        static const SDL_DialogFileFilter kSieve[] = {{"Sieve instructions", "sieve"}};
        const SDL_DialogFileFilter* filter = save_what_ == 3 ? kProgram : save_what_ == 4 ? kSieve : nullptr;
        save_file_dialog(on_save_to, this, window_, filter, filter ? 1 : 0, start.c_str());
    }
}

void FileLocator::draw(float W, float H)
{
    // Hand over whatever the dialogs chose since the last frame.
    {
        std::optional<std::string> pick, to;
        {
            std::lock_guard<std::mutex> lock(mx_);
            pick.swap(pending_pick_);
            to.swap(pending_save_);
        }
        if (pick) analyse(*pick);
        if (to) save(*to);
        std::optional<std::pair<int, std::string>> inst;
        {
            std::lock_guard<std::mutex> lock(mx_);
            inst.swap(pending_install_);
        }
        if (inst && inst->first == 0)
        {
            install_src_ = inst->second;
            status_ = trf("loc.install.where", {inst->second});
            open_folder_dialog(on_install_to, this, window_, nullptr, false);
        }
        else if (inst && inst->first == 1 && !install_src_.empty()) install(install_src_, inst->second);
    }
    const SDL_Color ink = host_.ink ? host_.ink() : SDL_Color{0, 255, 255, 255}, grey{150, 150, 150, 255}, white{255, 255, 255, 255}, red{255, 80, 80, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);
    draw_text(r_, 20, 16, tr("loc.title"), 3, ink);
    draw_text(r_, 20, 48, fit_text(tr("loc.subtitle"), W - 40, 1), 1, grey);
    buttons_.clear();
    float bx = 20;
    auto button = [&](const std::string& id, const std::string& label, float y, bool enabled = true) {
        const float w = text_width(label, 2) + 24;
        const SDL_FRect r{bx, y, w, 30};
        const bool hot = mouse_x_ >= r.x && mouse_x_ < r.x + r.w && mouse_y_ >= r.y && mouse_y_ < r.y + r.h;
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, hot && enabled ? 60 : 20);
        SDL_RenderFillRect(r_, &r);
        SDL_SetRenderDrawColor(r_, enabled ? ink.r : 90, enabled ? ink.g : 90, enabled ? ink.b : 90, 255);
        SDL_RenderRect(r_, &r);
        draw_text(r_, r.x + 12, r.y + 7, label, 2, enabled ? white : grey);
        if (enabled) buttons_.emplace_back(r, id);
        bx += w + 12;
    };
    const bool busy = busy_;
    button("file", tr("loc.choose_file"), 76, !busy);
    button("folder", tr("loc.choose_folder"), 76, !busy);
    button("install", tr("loc.install"), 76, !busy);
    float y = 126;
    std::string status;
    {
        std::lock_guard<std::mutex> lock(mx_);
        status = status_;
    }
    const Result& r = result_;
    if (!busy && r.kind == Result::None && r.error.empty())
        draw_text(r_, 20, y, tr("loc.empty"), 2, grey);
    else if (!r.error.empty() && !busy) draw_text(r_, 20, y, fit_text(trf("loc.failed", {r.error}), W - 40, 1), 1, red);
    else if (r.kind != Result::None && !busy)
    {
        auto line = [&](const std::string& label, const std::string& value) {
            draw_text(r_, 20, y, label, 1, grey);
            draw_text(r_, 150, y, fit_text(value, W - 170, 1), 1, white);
            y += 16;
        };
        if (r.kind == Result::File)
        {
            line(tr("loc.file"), r.path);
            line(tr("loc.bytes"), std::to_string(r.bytes.size()));
            line(tr("loc.sha256"), r.sha256);
            line(tr("loc.address"), trf("loc.address.value", {std::to_string(r.hex.size()), head_tail(r.hex, 24)}));
            // Where it can be walked to: the length it needs against the BINARY length now.
            if (in_world() && host_.binary_bytes)
                line(tr("loc.line"), trf("loc.line.value", {std::to_string(r.bytes.size()), std::to_string(host_.binary_bytes())}));
            if (go_armed_)
            {
                draw_text(r_, 20, y, fit_text(trf("loc.go_anyway", {std::to_string(r.bytes.size())}), W - 40, 1), 1, SDL_Color{255, 200, 80, 255});
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
            draw_text(r_, 20, y, fit_text(row, W - 40, 1), 1, row.rfind("comparison", 0) == 0 ? grey : ink);
            y += 13;
            if (nl == std::string::npos) break;
            at = nl + 1;
        }
        // The weighing: each file's best way to be named against its own address, and the totals.
        y += 6;
        at = 0;
        while (at < r.weighing.size() && y < H - 150)
        {
            const size_t nl = r.weighing.find('\n', at);
            const std::string row = r.weighing.substr(at, nl == std::string::npos ? std::string::npos : nl - at);
            draw_text(r_, 20, y, fit_text(row, W - 40, 1), 1, row.rfind("weighing", 0) == 0 || row.rfind("  shared", 0) == 0 ? grey : ink);
            y += 13;
            if (nl == std::string::npos) break;
            at = nl + 1;
        }
        y += 14;
        bx = 20;
        // A file can be walked to, in the world. A file or a folder can be saved as Sieve instructions (the
        // .sieve: its listing and its files, as one number) for anyone who has Sieve, or as an
        // installer program for anyone; a file exactly as a folder holding just it. The listing on
        // its own (the manifest) is for the tools and the command line (sieve locate --manifest),
        // and a file's bare address for the command line (sieve locate FILE --out).
        if (r.kind == Result::File && in_world()) button("go", tr("loc.go"), y);
        button("save_sieve", tr("loc.save_sieve"), y);
        button("save_program", tr("loc.save_program"), y);
        // The filters tailored to these files, and then used: the hallway built again with them,
        // or kept for the next one from the main menu (on a row of their own, under the saves).
        bx = 20;
        if (!r.tailored)
        {
            // How long the search may take: a file, and the run, each moving the other.
            auto secs = [](double v) {
                char b[32];
                std::snprintf(b, sizeof b, v < 1 ? "%.2f" : "%.1f", v);
                return v > 0 ? trf("loc.search.seconds", {b}) : tr("loc.search.none");
            };
            draw_text(r_, 20, y + 48, tr("loc.search"), 1, grey);
            bx = 20 + text_width(tr("loc.search"), 1) + 12;
            button("search_file_less", "-", y + 40);
            const std::string per = trf("loc.search.file", {secs(search_per_file_)});
            draw_text(r_, bx, y + 48, per, 1, white);
            bx += text_width(per, 1) + 12;
            button("search_file_more", "+", y + 40);
            bx += 12;
            button("search_run_less", "-", y + 40);
            const std::string run = trf("loc.search.run", {secs(search_total_), std::to_string(files_counted())});
            draw_text(r_, bx, y + 48, run, 1, white);
            bx += text_width(run, 1) + 12;
            button("search_run_more", "+", y + 40);
            bx = 20;
            button("tailor", tr("loc.tailor"), y + 80);
        }
        else if (r.tailored_filters) button("use_filters", tr("loc.use_filters"), y + 40);
    }
    if (!status.empty()) draw_text(r_, 20, H - 58, fit_text(status, W - 40, 1), 1, busy ? white : ink);
    draw_text(r_, 20, H - 26, fit_text(tr(in_world() ? "loc.keys" : "loc.keys.menu"), W - 40, 1), 1, grey);
}

size_t FileLocator::files_counted() const
{
    if (result_.kind == Result::Folder) return std::max<size_t>(1, size_t(result_.manifest.files));
    return 1;
}

// One step of the search's time, a file or the run: up or down a ladder of seconds (the first step
// down from the bottom is no limit), the other worked out from it.
void FileLocator::step_search(bool total, int dir)
{
    static const double kFile[] = {0, 0.05, 0.1, 0.25, 0.5, 1, 2, 5, 10, 30, 60, 120, 300, 600};
    static const double kRun[] = {0, 1, 2, 5, 10, 30, 60, 120, 300, 600, 1800, 3600, 7200, 14400};
    const double* ladder = total ? kRun : kFile;
    const size_t n = 14;
    double& v = total ? search_total_ : search_per_file_;
    // The next rung above or below the value as it stands (which the other's move may have put off it).
    double next = v;
    if (dir > 0)
    {
        for (size_t i = 0; i < n; ++i)
            if (ladder[i] > v + 1e-9)
            {
                next = ladder[i];
                break;
            }
    }
    else
    {
        next = 0;
        for (size_t i = n; i-- > 0;)
            if (ladder[i] < v - 1e-9)
            {
                next = ladder[i];
                break;
            }
    }
    v = next;
    search_by_total_ = total;
    relink_search();
}

// The one not last set, worked out again from the one that was and the files counted.
void FileLocator::relink_search()
{
    const size_t files = files_counted();
    if (search_by_total_) search_per_file_ = cli::budget_per_file(search_total_, files);
    else search_total_ = cli::budget_total(search_per_file_, files);
}

void FileLocator::set_search(double seconds, bool total)
{
    (total ? search_total_ : search_per_file_) = seconds;
    search_by_total_ = total;
    relink_search();
}

// For scripted runs (--locate PATH): open the locator on it, measured before the next frame.
void FileLocator::locate_now(const std::string& path, bool tailor)
{
    open();
    analyse(path, true, tailor);
}

void FileLocator::install_now(const std::string& from, const std::string& to)
{
    open();
    install(from, to, true);
}

void FileLocator::stop()
{
    if (worker_.joinable()) worker_.join();
}

} // namespace hallway
