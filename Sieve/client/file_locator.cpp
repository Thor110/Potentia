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
#include "sieve/rope_wave.hpp"
#include "sieve/stop.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <random>
#include <stdexcept>
#include <thread>

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
cli::Manifest file_instructions(const fs::path& p, const std::vector<uint8_t>& bytes, const cli::Args& items, double rope_seconds, bool archives)
{
    cli::Manifest m = cli::manifest_of_file(p);
    m.contents = bytes;
    m.with_contents = true;
    cli::add_packed_items(m, p.parent_path(), items, nullptr, 8, false, rope_seconds, 9, true, true, archives);
    return m;
}

// The time estimate (IDEAS §19.97). Reading a file does much the same work on every byte (its
// address, zip's and 7z's sizes, its instructions packed and compressed again, the weighing), and
// some at every place its kind of bytes changes (where it is cut into pieces), so it is timed here
// on 1 and 2 MiB taken in eight stretches evenly across the file (a folder's largest), with the
// search as near none as the packer allows, and drawn as a straight line through the two to the
// whole size; the most the search may take is added to it. Below 16 MiB the reading takes too
// little to need one.
constexpr size_t kSampleBytes = size_t(1) << 20;
constexpr uint64_t kEstimateFrom = uint64_t(16) << 20;
constexpr size_t kSlices = 8;      // the samples' stretches, evenly across the file
constexpr double kNoSearch = 1e-3; // 0 would be no limit
constexpr double kEstimateMargin = 1.25; // the first estimate's margin (IDEAS §19.97)

using Clock = std::chrono::steady_clock;

double since(Clock::time_point t) { return std::chrono::duration<double>(Clock::now() - t).count(); }

std::string duration_text(double s)
{
    char b[64];
    if (s < 10) std::snprintf(b, sizeof b, "%.1f s", s);
    else if (s < 60) std::snprintf(b, sizeof b, "%.0f s", s);
    else if (s < 3600) std::snprintf(b, sizeof b, "%d min %02d s", int(s) / 60, int(s) % 60);
    else std::snprintf(b, sizeof b, "%d h %02d min", int(s) / 3600, int(s) % 3600 / 60);
    return b;
}

// The reading's work, timed on `sample` saved as a file named `name` in a folder of its own: each
// step's seconds, in the order the whole does them (address and SHA-256, zip, 7z, the instructions,
// compared, the weighing).
using Steps = std::array<double, 6>;

Steps sample_seconds(const std::vector<uint8_t>& sample, const fs::path& name, const cli::Args& items, bool archives, const cli::WeighLines& wl,
                      const std::atomic<int>* stop)
{
    std::random_device rd;
    const fs::path dir = fs::temp_directory_path() / ("sieve-estimate-" + std::to_string(rd()) + std::to_string(rd()));
    struct Cleanup
    {
        fs::path p;
        ~Cleanup()
        {
            std::error_code ec;
            fs::remove_all(p, ec);
        }
    } cleanup{dir};
    const StopScope quiet(stop, {}); // the sample says nothing of itself, and Stop ends it too (no estimate then)
    fs::create_directories(dir);
    const fs::path f = dir / name.filename();
    {
        std::ofstream o(f, std::ios::binary);
        o.write(reinterpret_cast<const char*>(sample.data()), std::streamsize(sample.size()));
        if (!o) throw std::runtime_error("cannot write the sample");
    }
    // Each step timed as the whole's is (--timings: "estimate.*" beside "locator.*"), to see which the
    // samples undercount (IDEAS §19.97).
    using T = cli::timings::Scope;
    Steps out{};
    auto t0 = Clock::now();
    auto next = [&](size_t k) {
        out[k] += since(t0);
        t0 = Clock::now();
    };
    BigUint a;
    {
        const T t("estimate.address");
        a = cli::binary_address(sample);
        (void)a.to_hex();
    }
    {
        const T t("estimate.sha256");
        (void)cli::sha256_hex(sample);
    }
    next(0);
    {
        const T t("estimate.zip");
        (void)cli::deflate_size(sample);
    }
    next(1);
    {
        const T t("estimate.7z");
        (void)cli::lzma2_size(sample);
    }
    next(2);
    cli::Manifest fi;
    {
        const T t("estimate.instructions");
        fi = file_instructions(f, sample, items, kNoSearch, archives);
    }
    next(3);
    {
        const T t("estimate.instructions-compared");
        const std::vector<uint8_t> mb = fi.file();
        const BigUint ma = cli::binary_address(mb);
        (void)cli::deflate_size(mb);
        (void)cli::lzma2_size(mb);
        (void)ma.to_hex();
    }
    next(4);
    {
        const T t("estimate.weigh");
        (void)cli::weigh_files(dir, {u8(f.filename())}, wl, false, nullptr, cli::WeighBudget{kNoSearch, kNoSearch});
    }
    next(5);
    return out;
}

// `slices` stretches of `each` bytes taken evenly across a file of `size` bytes, joined: a sample
// that meets the kinds of bytes the file holds as often as the whole does (where they change is
// what cutting a file into pieces spends its time on).
std::vector<uint8_t> spread_of(const fs::path& file, uint64_t size, size_t slices, size_t each)
{
    std::ifstream in(file, std::ios::binary);
    std::vector<uint8_t> out;
    std::vector<uint8_t> buf(each);
    for (size_t k = 0; k < slices; ++k)
    {
        const uint64_t at = size > each ? (size - each) * k / std::max<size_t>(1, slices - 1) : 0;
        in.clear();
        in.seekg(std::streamoff(at));
        in.read(reinterpret_cast<char*>(buf.data()), std::streamsize(each));
        out.insert(out.end(), buf.begin(), buf.begin() + std::ptrdiff_t(in.gcount()));
    }
    return out;
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

namespace {

// The search's time kept between runs, and what saves it (remember_search).
struct KeptSearch
{
    double seconds = 0;
    bool total = false;
    std::function<void(double, bool)> changed;
};
KeptSearch& kept_search()
{
    static KeptSearch k;
    return k;
}

struct KeptArchives
{
    bool on = false;
    std::function<void(bool)> changed;
};
KeptArchives& kept_archives()
{
    static KeptArchives k;
    return k;
}

} // namespace

void FileLocator::remember_search(double seconds, bool total, std::function<void(double, bool)> changed)
{
    kept_search() = {seconds, total, std::move(changed)};
}

void FileLocator::remember_archives(bool on, std::function<void(bool)> changed) { kept_archives() = {on, std::move(changed)}; }

namespace {
std::function<void(bool)>& kept_every()
{
    static std::function<void(bool)> k;
    return k;
}
} // namespace

void FileLocator::remember_every(std::function<void(bool)> changed) { kept_every() = std::move(changed); }

void FileLocator::set_archives(bool on) { optimise_archives_ = on; }

FileLocator::FileLocator(SDL_Window* window, SDL_Renderer* renderer, Host host) : window_(window), r_(renderer), host_(std::move(host))
{
    (kept_search().total ? search_total_ : search_per_file_) = kept_search().seconds;
    search_by_total_ = kept_search().total;
    optimise_archives_ = kept_archives().on;
    relink_search();
}

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

std::string FileLocator::status_text()
{
    std::lock_guard<std::mutex> lock(mx_);
    return status_;
}

FileLocator::~FileLocator()
{
    stop();
    if (graph_) SDL_DestroyTexture(graph_);
}

void FileLocator::start_job(bool new_graph)
{
    std::lock_guard<std::mutex> lock(mx_);
    if (new_graph)
    {
        graph_reset_ = true;
        graph_pending_.clear();
    }
    job_started_ = Clock::now();
    job_estimate_ = -1;
    job_estimate_text_.clear();
    progress_.clear();
    stop_ = 0;
}

// After the reading's first `steps_done` steps (est_steps_), the estimate again: the time so far,
// and what is left as the samples predict it, times how long the done steps took against their
// prediction (the search's time taken out, as far as it can have been spent), with the searches'
// time still to come. Locks mx_ itself.
void FileLocator::refine_estimate(size_t steps_done)
{
    std::lock_guard<std::mutex> lock(mx_);
    if (!est_on_ || steps_done == 0 || steps_done > est_steps_.size()) return;
    double predicted = 0, left = 0;
    for (size_t k = 0; k < est_steps_.size(); ++k) (k < steps_done ? predicted : left) += est_steps_[k];
    double worked = since(est_work_start_);
    if (steps_done >= 4) worked = std::max(worked - est_pack_search_, 0.5 * worked); // the instructions' search
    if (predicted < 1) return; // too little done to tell anything
    const double ratio = std::clamp(worked / predicted, 0.5, 4.0);
    const double remaining = left * ratio + est_weigh_search_ + (steps_done < 4 ? est_pack_search_ : 0);
    job_estimate_ = since(job_started_) + remaining;
    job_estimate_text_ = trf("loc.estimate.refined", {duration_text(job_estimate_), duration_text(remaining)});
}

// Under mx_.
std::string FileLocator::finish_job()
{
    const double took = since(job_started_);
    std::string t = job_estimate_ < 0 ? trf("loc.took", {duration_text(took)})
                    : est_first_ >= 0 && std::abs(est_first_ - job_estimate_) >= 1
                        ? trf("loc.took.refined", {duration_text(took), duration_text(est_first_), duration_text(job_estimate_)})
                        : trf("loc.took.estimated", {duration_text(took), duration_text(job_estimate_)});
    job_estimate_ = -1;
    est_on_ = false;
    est_first_ = -1;
    job_estimate_text_.clear();
    job_started_ = {};
    progress_.clear();
    if (stop_ >= kForce) t = tr("loc.forced") + "; " + t;
    else if (stop_ >= kStop) t = tr("loc.stopped") + "; " + t;
    return t;
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
            using clock = std::chrono::steady_clock;
            const auto t0 = clock::now();
            const cli::Manifest m = cli::installable_manifest(from_u8(from), false);
            const auto t1 = clock::now();
            std::vector<cli::InstallNote> notes;
            cli::install_tree(
                m, from_u8(to), false,
                [this](int phase, size_t d, size_t t, const std::string& path) {
                    std::lock_guard<std::mutex> lock(mx_);
                    status_ = trf(phase == 0 ? "loc.install.checking" : "loc.install.writing", {std::to_string(d), std::to_string(t), path});
                },
                nullptr, &notes);
            const auto t2 = clock::now();
            done = (m.files == 1 ? trf("loc.installed.one", {to}) : trf("loc.installed", {std::to_string(m.files), to})) + "; " +
                   cli::install_times_text(std::chrono::duration<double>(t1 - t0).count(), std::chrono::duration<double>(t2 - t1).count());
            // What the install had to say, as every program that installs shows it; a warning pops up too.
            std::string warn;
            for (const cli::InstallNote& n : notes)
            {
                done += (n.warning ? "; warning: " : "; ") + n.text;
                if (n.warning) warn += (warn.empty() ? "" : "\n\n") + n.text;
            }
            if (!warn.empty())
            {
                std::lock_guard<std::mutex> lock(mx_);
                pending_warning_ = warn;
            }
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
    const bool archives = optimise_archives_;
    start_job();
    auto job = [this, path, tailor, per_file, total, by_total, archives] {
        // Stop asks every search and every trial not needed for a result to end, the best so far
        // kept; what the work is doing, and its best so far, goes to the line under the status.
        const StopScope scope(&stop_, [this](const std::string& what, double best, double of) { report(what, best, of); });
        // A measure for the table, or kNotMeasured once asked to stop.
        auto measured = [](uint64_t (*f)(std::span<const uint8_t>), std::span<const uint8_t> b) { return stop_requested() ? cli::kNotMeasured : f(b); };
        auto plus = [](uint64_t a, uint64_t b) { return a == cli::kNotMeasured || b == cli::kNotMeasured ? cli::kNotMeasured : a + b; };
        Result r;
        r.path = path;
        try
        {
            const fs::path p = from_u8(path);
            cli::Comparison c;
            // The estimate: the reading timed on the start of the largest file and drawn out to
            // the whole, and the most the search may take (packing, then weighing) on top.
            auto estimate = [&](uint64_t bytes, const fs::path& largest, uint64_t largest_bytes, size_t files, size_t searched) {
                const double each = by_total ? cli::budget_per_file(total, files) : per_file;
                r.pack_search = 2 * each * double(searched); // the file whole, then its pieces (file_ways)
                r.pack_search_unlimited = each <= 0;
                if (bytes < kEstimateFrom || largest_bytes < 2 * kSampleBytes) return;
                {
                    std::lock_guard<std::mutex> lock(mx_);
                    job_estimate_text_ = tr("loc.estimating");
                }
                const cli::Args items = host_.item_args ? host_.item_args() : cli::Args{};
                const cli::WeighLines wl = host_.lines();
                const std::vector<uint8_t> one = spread_of(largest, largest_bytes, kSlices, kSampleBytes / kSlices),
                                           two = spread_of(largest, largest_bytes, kSlices, 2 * kSampleBytes / kSlices);
                const cli::SampleAsWhole as_whole(largest_bytes); // its samples packed with the whole's dictionary
                const Steps t1 = sample_seconds(one, largest, items, archives, wl, &stop_);
                const Steps t2 = stop_requested() ? Steps{} : sample_seconds(two, largest, items, archives, wl, &stop_);
                if (stop_requested()) // stopped while timing it: no estimate
                {
                    std::lock_guard<std::mutex> lock(mx_);
                    job_estimate_text_.clear();
                    return;
                }
                // Each step drawn out as a straight line through its two samples to the whole size.
                Steps steps{};
                double line = 0;
                for (size_t k = 0; k < steps.size(); ++k)
                {
                    const double per_byte = (t2[k] - t1[k]) / double(kSampleBytes);
                    steps[k] = per_byte > 0 ? t1[k] + per_byte * double(bytes - kSampleBytes) : t2[k] * double(bytes) / double(2 * kSampleBytes);
                    line += steps[k];
                }
                // A quarter more: on a large file the work grows faster than in proportion, by an
                // amount two small samples cannot show (the 52 MB recording took 1.4 to 1.75 times
                // the line, step by step; IDEAS §19.97); refined as the steps are done.
                const double reading = line * kEstimateMargin;
                const double weigh_search = by_total ? total : cli::budget_total(per_file, files);
                std::lock_guard<std::mutex> lock(mx_);
                est_steps_ = steps;
                est_on_ = true;
                est_pack_search_ = r.pack_search_unlimited ? 0 : r.pack_search;
                est_weigh_search_ = r.pack_search_unlimited ? 0 : weigh_search;
                est_work_start_ = Clock::now();
                if (r.pack_search_unlimited)
                {
                    job_estimate_ = reading;
                    job_estimate_text_ = trf("loc.estimate.unlimited", {duration_text(reading)});
                }
                else
                {
                    job_estimate_ = reading + r.pack_search + weigh_search;
                    job_estimate_text_ = trf("loc.estimate", {duration_text(job_estimate_), duration_text(reading), duration_text(r.pack_search + weigh_search)});
                }
                job_estimate_ += since(job_started_); // from the job's start, the samples' time in it
                est_first_ = job_estimate_;
            };
            if (fs::is_regular_file(p))
            {
                r.kind = Result::File;
                const uint64_t size = fs::file_size(p);
                estimate(size, p, size, 1, size >= 512 ? 1 : 0);
                progress("reading it, its address and its SHA-256");
                using T = cli::timings::Scope; // each step as the estimate times it ("estimate.*")
                r.bytes = cli::read_file_bytes(p);
                BigUint a;
                {
                    const T t("locator.address");
                    a = cli::binary_address(r.bytes);
                    r.hex = a.is_zero() ? "0" : a.to_hex();
                }
                {
                    const T t("locator.sha256");
                    r.sha256 = cli::sha256_hex(r.bytes);
                }
                refine_estimate(1);
                c.original = r.bytes.size();
                progress("what zip makes of it (deflate, level 9)");
                {
                    const T t("locator.zip");
                    c.deflate = measured(cli::deflate_size, r.bytes);
                }
                refine_estimate(2);
                progress("what 7z makes of it (LZMA2, preset 9 extreme)");
                {
                    const T t("locator.7z");
                    c.lzma2 = measured(cli::lzma2_size, r.bytes);
                }
                refine_estimate(3);
                c.address_bytes = (a.bit_length() + 7) / 8;
                c.address_hex = a.is_zero() ? 0 : r.hex.size();
                // Its Sieve instructions, as a folder's: its name, size and SHA-256, then its bytes.
                r.manifest = cli::manifest_of_file(p);
                progress("packing its Sieve instructions");
                const auto packing = Clock::now();
                cli::Manifest fi = [&] {
                    const T t("locator.instructions");
                    return file_instructions(p, r.bytes, host_.item_args ? host_.item_args() : cli::Args{}, by_total ? total : per_file, archives);
                }();
                r.pack_seconds = since(packing);
                refine_estimate(4);
                const T tc("locator.instructions-compared");
                const std::vector<uint8_t> mb = fi.file();
                c.packed = fi.packed;
                c.version = fi.version();
                progress("the instructions' address");
                BigUint ma = cli::binary_address(mb);
                c.manifest = mb.size();
                progress("Sieve instructions of " + std::to_string((ma.bit_length() + 7) / 8) + " bytes; what zip and 7z make of them");
                c.manifest_deflate = measured(cli::deflate_size, mb);
                c.manifest_lzma2 = measured(cli::lzma2_size, mb);
                c.installer_hex = ma.is_zero() ? 1 : ma.to_hex().size();
                c.installer_raw = (ma.bit_length() + 7) / 8;
                r.instructions = std::move(fi);
                r.instructions_address = std::move(ma);
                r.made_rope_seconds = by_total ? total : per_file;
                r.made_archives = archives;
                refine_estimate(5);
            }
            else if (fs::is_directory(p))
            {
                r.kind = Result::Folder;
                r.manifest = cli::walk_folder(p);
                {
                    fs::path largest;
                    uint64_t most = 0;
                    size_t files = 0, searched = 0;
                    for (const auto& e : r.manifest.entries)
                    {
                        if (e.dir) continue;
                        ++files;
                        searched += e.size >= 512 ? 1 : 0;
                        if (e.size > most) most = e.size, largest = p / from_u8(e.path);
                    }
                    estimate(r.manifest.bytes, largest, most, files, searched);
                }
                std::vector<uint8_t> all;
                progress("reading its files, their addresses, and what zip makes of each");
                for (const auto& e : r.manifest.entries)
                {
                    if (e.dir) continue;
                    const auto bytes = cli::read_file_bytes(p / from_u8(e.path));
                    const BigUint a = cli::binary_address(bytes);
                    c.original += bytes.size();
                    c.deflate = plus(c.deflate, measured(cli::deflate_size, bytes));
                    c.address_bytes += (a.bit_length() + 7) / 8;
                    c.address_hex += a.is_zero() ? 0 : a.to_hex().size();
                    all.insert(all.end(), bytes.begin(), bytes.end());
                }
                refine_estimate(2);
                progress("what 7z makes of them (LZMA2, preset 9 extreme, as one stream)");
                c.lzma2 = measured(cli::lzma2_size, all);
                refine_estimate(3);
                // The installer's manifest (v4: every file packed after it), compressed, and the installer.
                cli::Manifest with = r.manifest;
                {
                    // The rope's time for each file: the file's, or the run's shared among the files.
                    size_t n = 0;
                    for (const auto& e : r.manifest.entries) n += e.dir ? 0 : 1;
                    progress("packing its Sieve instructions");
                    const auto packing = Clock::now();
                    r.made_rope_seconds = by_total ? cli::budget_per_file(total, n) : per_file;
                    cli::add_packed_items(with, p, host_.item_args ? host_.item_args() : cli::Args{}, nullptr, 8, false, r.made_rope_seconds, 9, true,
                                          true, archives);
                    r.pack_seconds = since(packing);
                }
                refine_estimate(4);
                c.packed = with.packed;
                c.version = with.version();
                const std::vector<uint8_t> mb = with.file();
                progress("the instructions' address");
                BigUint ma = cli::binary_address(mb);
                c.manifest = mb.size();
                progress("Sieve instructions of " + std::to_string((ma.bit_length() + 7) / 8) + " bytes; what zip and 7z make of them");
                c.manifest_deflate = measured(cli::deflate_size, mb);
                c.manifest_lzma2 = measured(cli::lzma2_size, mb);
                c.installer_hex = ma.is_zero() ? 1 : ma.to_hex().size();
                c.installer_raw = (ma.bit_length() + 7) / 8;
                r.instructions = std::move(with);
                r.instructions_address = std::move(ma);
                r.made_archives = archives;
                const std::string plain = r.manifest.text(); // the tree's identity is its v1 manifest's
                r.sha256 = cli::sha256_hex(std::vector<uint8_t>(plain.begin(), plain.end()));
                refine_estimate(5);
            }
            else throw std::runtime_error(tr("loc.neither"));
            r.table = cli::comparison_table(c);
            // Every file weighed against its own address, under this hallway's lines and filters
            // (the pages, image, audio and video lines' units, and the binary line's filters);
            // not where it was stopped first.
            if (stop_requested()) r.weighing = tr("loc.weighing.stopped") + "\n";
            else
            {
                progress("weighing it against its own address");
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
                const cli::Weighing w = [&] {
                    const cli::timings::Scope t("locator.weigh");
                    return cli::weigh_files(root, files, wl, tailor, nullptr, budget);
                }();
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
        r.stopped = stop_ != 0;
        std::lock_guard<std::mutex> lock(mx_);
        result_ = std::move(r);
        relink_search(); // the files counted may have changed
        status_ = finish_job();
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
    const Result& r = result_; // not changed while busy (nothing else starts), so read in place
    const double rope_seconds = search_per_file_; // the rope's time for each file, as the search row sets it
    const bool archives = optimise_archives_;
    // Made when it was read, with what is set now: written as it is, nothing packed again (and the
    // graph of how it was made is kept).
    const bool made = r.instructions && r.made_rope_seconds == rope_seconds && r.made_archives == archives;
    start_job(!made);
    if (made)
    {
        std::lock_guard<std::mutex> lock(mx_);
        job_estimate_text_ = tr(r.stopped ? "loc.save.made.stopped" : "loc.save.made");
    }
    else if (r.pack_seconds >= 0)
    {
        // Saving packs it again, as reading it did: that time, measured, and the search's at most.
        std::lock_guard<std::mutex> lock(mx_);
        if (r.pack_search_unlimited) job_estimate_text_ = trf("loc.estimate.save.unlimited", {duration_text(r.pack_seconds)});
        else
        {
            job_estimate_ = r.pack_seconds + r.pack_search;
            job_estimate_text_ = trf("loc.estimate.save", {duration_text(job_estimate_), duration_text(r.pack_seconds), duration_text(r.pack_search)});
        }
    }
    worker_ = std::thread([this, what, to, rope_seconds, archives, made] {
        const Result& r = result_;
        const StopScope scope(&stop_, [this](const std::string& what, double best, double of) { report(what, best, of); });
        std::string done;
        try
        {
            const fs::path out = from_u8(to);
            {
                BigUint address;
                if (made) address = r.instructions_address;
                else
                {
                    // A file's instructions are a folder's holding just it (file_instructions).
                    const cli::Args items = host_.item_args ? host_.item_args() : cli::Args{};
                    cli::Manifest with = r.kind == Result::File ? file_instructions(from_u8(r.path), r.bytes, items, rope_seconds, archives) : r.manifest;
                    if (r.kind != Result::File) cli::add_packed_items(with, from_u8(r.path), items, nullptr, 8, false, rope_seconds, 9, true, true, archives);
                    progress("the instructions' address");
                    address = cli::binary_address(with.file());
                }
                progress("writing them");
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
        status_ = done + "; " + finish_job();
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
        case SDLK_A: pressed = "archives"; break; // Optimise Compressed Formats, on or off
        case SDLK_E: pressed = "every"; break;    // Try every way, on or off
        case SDLK_S: pressed = busy_ ? "stop" : ""; break; // Stop: the best found so far kept
        case SDLK_RETURN:
        case SDLK_KP_ENTER: pressed = result_.kind == Result::File && in_world() ? "go" : ""; break;
        default: break;
        }
    }
    if (pressed.empty() || (busy_ && pressed != "close" && pressed != "stop")) return;
    if (pressed != "go") go_armed_ = false; // "again" means the very next press
    if (pressed == "stop")
    {
        // Once: stop, the pass under way finished. Again: force, that pass abandoned too.
        if (busy_ && stop_ < kForce)
        {
            stop_ = stop_ + 1;
            std::lock_guard<std::mutex> lock(mx_);
            status_ = tr(stop_ >= kForce ? "loc.forcing" : "loc.stopping");
        }
    }
    else if (pressed == "close") close();
    else if (pressed == "file") open_file_dialog(on_picked, this, window_, nullptr, 0, nullptr, false);
    else if (pressed == "folder") open_folder_dialog(on_picked, this, window_, nullptr, false);
    else if (pressed == "go") go();
    else if (pressed == "tailor") analyse(result_.path, false, true);
    else if (pressed == "search_file_less") step_search(false, -1);
    else if (pressed == "search_file_more") step_search(false, 1);
    else if (pressed == "search_run_less") step_search(true, -1);
    else if (pressed == "search_run_more") step_search(true, 1);
    else if (pressed == "every")
    {
        const bool on = !sieve::rope_search_every_wave();
        sieve::set_rope_search_every_wave(on);
        if (kept_every()) kept_every()(on);
    }
    else if (pressed == "archives")
    {
        optimise_archives_ = !optimise_archives_;
        kept_archives().on = optimise_archives_;
        if (kept_archives().changed) kept_archives().changed(optimise_archives_);
    }
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
        open_file_dialog(on_install_from, this, window_, kFrom, 2, nullptr, false, true); // the folder's dialog follows
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
    // An install's warning, in a box of its own (on this thread: SDL's message boxes want the main one).
    {
        std::string warn;
        {
            std::lock_guard<std::mutex> lock(mx_);
            warn.swap(pending_warning_);
        }
        if (!warn.empty()) SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_WARNING, "Sieve", warn.c_str(), nullptr);
    }
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
    if (busy)
    {
        // Stop, on a row of its own under the others (greyed while it works), so it always fits.
        bx = 20;
        button("stop", tr(stop_ == 0 ? "loc.stop" : stop_ == kStop ? "loc.force" : "loc.stop.pressed"), 116, stop_ < kForce); // the best so far kept
    }
    float y = 126;
    std::string status;
    {
        std::lock_guard<std::mutex> lock(mx_);
        status = status_;
    }
    const Result& r = result_;
    // The graph of the best so far: large while it works, beside the result after.
    bool waiting = false; // new bests not drawn yet (a scripted run draws nothing while it works)
    {
        std::lock_guard<std::mutex> lock(mx_);
        waiting = !graph_pending_.empty();
    }
    const bool graph_beside = !busy && r.kind != Result::None && (graph_drawn_ || waiting);
    if (busy) draw_graph(20, y + 50, W - 40, std::max(60.0f, std::min(220.0f, H - y - 190)), ink, grey);
    else if (graph_beside) draw_graph(W * 0.62f, y + 13, W * 0.38f - 20, 60, ink, grey);
    // How long the search for shorter addresses may take: a file, and the run, each moving the
    // other. Shown before anything is chosen too, so the first search can have its time.
    auto search_row = [&](float row_y) {
        auto secs = [](double v) {
            char b[32];
            std::snprintf(b, sizeof b, v < 1 ? "%.2f" : "%.1f", v);
            return v > 0 ? trf("loc.search.seconds", {b}) : tr("loc.search.none");
        };
        draw_text(r_, 20, row_y + 8, tr("loc.search"), 1, grey);
        bx = 20 + text_width(tr("loc.search"), 1) + 12;
        button("search_file_less", "-", row_y);
        const std::string per = trf("loc.search.file", {secs(search_per_file_)});
        draw_text(r_, bx, row_y + 8, per, 1, white);
        bx += text_width(per, 1) + 12;
        button("search_file_more", "+", row_y);
        bx += 12;
        button("search_run_less", "-", row_y);
        const std::string run = trf("loc.search.run", {secs(search_total_), std::to_string(files_counted())});
        draw_text(r_, bx, row_y + 8, run, 1, white);
        bx += text_width(run, 1) + 12;
        button("search_run_more", "+", row_y);
        // Optimise Compressed Formats, under it: a box, off unless ticked (what installing gives back
        // changes for an archive carried as its contents, so it is never on by itself). Beside it,
        // Try every way: the search's shortcuts off (slower; never larger).
        bx = 20;
        button("archives", std::string(optimise_archives_ ? "[x] " : "[ ] ") + tr("loc.archives"), row_y + 40);
        button("every", std::string(sieve::rope_search_every_wave() ? "[x] " : "[ ] ") + tr("loc.every"), row_y + 40);
    };
    if (!busy && r.kind == Result::None && r.error.empty())
    {
        draw_text(r_, 20, y, tr("loc.empty"), 2, grey);
        search_row(y + 40);
    }
    else if (!r.error.empty() && !busy)
    {
        draw_text(r_, 20, y, fit_text(trf("loc.failed", {r.error}), W - 40, 1), 1, red);
        search_row(y + 40);
    }
    else if (r.kind != Result::None && !busy)
    {
        auto line = [&](const std::string& label, const std::string& value) {
            draw_text(r_, 20, y, label, 1, grey);
            draw_text(r_, 150, y, fit_text(value, (graph_beside ? W * 0.62f - 20 : W) - 170, 1), 1, white);
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
            search_row(y + 40);
            bx = 20;
            button("tailor", tr("loc.tailor"), y + 120);
        }
        else if (r.tailored_filters) button("use_filters", tr("loc.use_filters"), y + 40);
    }
    if (!status.empty()) draw_text(r_, 20, H - 58, fit_text(status, W - 40, 1), 1, busy ? white : ink);
    if (busy)
    {
        // The job's estimate, and how long it has run.
        std::string est;
        {
            std::lock_guard<std::mutex> lock(mx_);
            if (job_started_ != Clock::time_point{})
                est = job_estimate_text_.empty() ? trf("loc.elapsed", {duration_text(since(job_started_))})
                                                 : trf("loc.estimate.so_far", {job_estimate_text_, duration_text(since(job_started_))});
        }
        if (!est.empty()) draw_text(r_, 20, H - 42, fit_text(est, W - 40, 1), 1, grey);
        // What it is doing now, and the best found so far.
        std::string now;
        {
            std::lock_guard<std::mutex> lock(mx_);
            now = progress_;
        }
        if (!now.empty()) draw_text(r_, 20, H - 74, fit_text(now, W - 40, 1), 1, ink);
    }
    draw_text(r_, 20, H - 26, fit_text(tr(in_world() ? "loc.keys" : "loc.keys.menu"), W - 40, 1), 1, grey);
}

// What the work says of itself (sieve/stop.hpp), on the worker: the line of what it is doing, and
// a new best of a step for the graph.
void FileLocator::report(const std::string& what, double best, double of)
{
    std::lock_guard<std::mutex> lock(mx_);
    progress_ = what;
    if (best < 0 || of <= 0 || job_started_ == Clock::time_point{}) return;
    // The step's kind: its words before any count in brackets or the colon ("the lzma2 stream").
    const size_t cut = std::min(what.find(" ("), what.find(':'));
    if (graph_pending_.size() < 4096) graph_pending_.push_back({since(job_started_), best / of, of, what.substr(0, cut)});
}

void FileLocator::draw_graph(float x, float y, float w, float h, SDL_Color ink, SDL_Color grey)
{
    static const SDL_Color kColours[] = {{0, 255, 255, 255}, {255, 200, 80, 255}, {255, 110, 200, 255}, {120, 230, 120, 255}, {170, 170, 255, 255}};
    const int tw = std::max(16, int(w)), th = std::max(16, int(h));
    std::vector<GraphPoint> points;
    bool reset = false;
    double estimate = -1;
    {
        std::lock_guard<std::mutex> lock(mx_);
        points.swap(graph_pending_);
        reset = std::exchange(graph_reset_, false);
        estimate = job_estimate_;
    }
    SDL_Texture* const before = SDL_GetRenderTarget(r_);
    auto blank = [&](SDL_Texture* t) {
        SDL_SetRenderTarget(r_, t);
        SDL_SetRenderDrawColor(r_, 0, 0, 0, 0);
        SDL_RenderClear(r_);
    };
    if (!graph_ || reset)
    {
        // A new job (or the first): a clean picture, its time across the estimate's, else 10 s, doubled as needed.
        if (graph_ && (graph_w_ != tw || graph_h_ != th))
        {
            SDL_DestroyTexture(graph_);
            graph_ = nullptr;
        }
        if (!graph_)
        {
            graph_ = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, tw, th);
            if (!graph_) return;
            SDL_SetTextureBlendMode(graph_, SDL_BLENDMODE_BLEND);
            graph_w_ = tw;
            graph_h_ = th;
        }
        blank(graph_);
        graph_span_ = 10;
        graph_depth_ = 0.01;
        graph_x_ = graph_y_ = -1;
        graph_of_ = -1;
        graph_step_.clear();
        graph_steps_.clear();
        graph_drawn_ = false;
    }
    for (const GraphPoint& p : points)
    {
        if (!graph_drawn_ && estimate > 0) graph_span_ = std::max(graph_span_, estimate * 1.1);
        while (p.seconds > graph_span_)
        {
            // Out of room: what is drawn squeezed into the left half, the time across doubled.
            SDL_Texture* half = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, graph_w_, graph_h_);
            if (!half) break;
            SDL_SetTextureBlendMode(half, SDL_BLENDMODE_BLEND);
            blank(half);
            const SDL_FRect left{0, 0, graph_w_ / 2.0f, float(graph_h_)};
            SDL_RenderTexture(r_, graph_, nullptr, &left);
            SDL_DestroyTexture(graph_);
            graph_ = half;
            graph_span_ *= 2;
            if (graph_x_ >= 0) graph_x_ /= 2;
        }
        size_t k = 0;
        while (k < graph_steps_.size() && graph_steps_[k] != p.step) ++k;
        if (k == graph_steps_.size()) graph_steps_.push_back(p.step);
        const SDL_Color c = kColours[k % (sizeof kColours / sizeof kColours[0])];
        // A step of the same kind on the same bytes goes on from its last best; another starts
        // afresh, from the top, its first try.
        const bool goes_on = graph_x_ >= 0 && p.step == graph_step_ && p.of == graph_of_;
        if (!goes_on) graph_first_ = p.share > 0 ? p.share : 1;
        const double smaller = std::max(0.0, 1.0 - p.share / graph_first_); // below its first try
        while (smaller > graph_depth_)
        {
            // Deeper than the bottom: what is drawn squeezed into the top half, the depth doubled.
            SDL_Texture* half = SDL_CreateTexture(r_, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, graph_w_, graph_h_);
            if (!half) break;
            SDL_SetTextureBlendMode(half, SDL_BLENDMODE_BLEND);
            blank(half);
            // (The top margin stays where it is: what is below it is what halves.)
            const SDL_FRect top{0, 0, float(graph_w_), 4 + (graph_h_ - 4) / 2.0f};
            SDL_RenderTexture(r_, graph_, nullptr, &top);
            SDL_DestroyTexture(graph_);
            graph_ = half;
            graph_depth_ *= 2;
            if (graph_y_ >= 0) graph_y_ = 4 + (graph_y_ - 4) / 2;
        }
        // A few pixels in from the top and the bottom, so that a step with nothing smaller yet (a
        // packed file's) shows as a line, not lost under the frame.
        constexpr float kMargin = 4;
        const float px = float(p.seconds / graph_span_) * float(graph_w_ - 1);
        const float py = kMargin + (float(graph_h_ - 1) - 2 * kMargin) * float(smaller / graph_depth_);
        SDL_SetRenderTarget(r_, graph_);
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        if (goes_on)
        {
            SDL_RenderLine(r_, graph_x_, graph_y_, px, graph_y_); // the best held until now
            SDL_RenderLine(r_, px, graph_y_, px, py);
        }
        const SDL_FRect dot{px - 1, py - 1, 3, 3};
        SDL_RenderFillRect(r_, &dot);
        graph_x_ = px;
        graph_y_ = py;
        graph_step_ = p.step;
        graph_of_ = p.of;
        graph_drawn_ = true;
    }
    SDL_SetRenderTarget(r_, before);
    // The frame, its quarters, the scales, and which colour is which step.
    SDL_SetRenderDrawColor(r_, grey.r, grey.g, grey.b, 60);
    for (int q = 1; q < 4; ++q) SDL_RenderLine(r_, x, y + h * float(q) / 4, x + w, y + h * float(q) / 4);
    SDL_SetRenderDrawColor(r_, grey.r, grey.g, grey.b, 255);
    const SDL_FRect frame{x - 1, y - 1, w + 2, h + 2};
    SDL_RenderRect(r_, &frame);
    const SDL_FRect at{x, y, w, h};
    SDL_RenderTexture(r_, graph_, nullptr, &at);
    // Its words outside it, so that nothing covers a line: what it shows above, and under it the
    // depth of its bottom, which colour is which step, and the time across.
    draw_text(r_, x, y - 13, fit_text(tr("loc.graph.top"), w, 1), 1, grey);
    char depth[32];
    std::snprintf(depth, sizeof depth, graph_depth_ < 0.1 ? "%.1f" : "%.0f", graph_depth_ * 100);
    const std::string bottom = trf("loc.graph.bottom", {depth});
    draw_text(r_, x, y + h + 4, bottom, 1, grey);
    const std::string across = duration_text(graph_span_);
    draw_text(r_, x + w - text_width(across, 1), y + h + 4, across, 1, grey);
    float lx = x + text_width(bottom, 1) + 16;
    for (size_t k = 0; k < graph_steps_.size() && lx + text_width(graph_steps_[k], 1) < x + w - text_width(across, 1) - 16; ++k)
    {
        const SDL_Color c = kColours[k % (sizeof kColours / sizeof kColours[0])];
        draw_text(r_, lx, y + h + 4, graph_steps_[k], 1, c);
        lx += text_width(graph_steps_[k], 1) + 16;
    }
    (void)ink;
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
    // Kept for the next run, and for the other File Locator (the main menu's, the world's).
    KeptSearch& k = kept_search();
    k.seconds = v;
    k.total = total;
    if (k.changed) k.changed(v, total);
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

namespace {
struct Scripted
{
    double stop_after = 0;
    std::string save_to;
    double shot_after = 0;  // the picture taken while it works, so many seconds in
    double force_after = 0; // and Stop pressed again, so many seconds in
};
Scripted& scripted()
{
    static Scripted s;
    return s;
}
} // namespace

void FileLocator::script(double stop_after, const std::string& save_to, double shot_after, double force_after)
{
    scripted() = {stop_after, save_to, shot_after, force_after};
}

// For scripted runs (--locate PATH): open the locator on it, measured before the next frame; with
// --stop-after, Stop pressed after so many seconds; with --locate-save, the instructions saved.
void FileLocator::locate_now(const std::string& path, bool tailor)
{
    open();
    const Scripted s = scripted();
    if (s.shot_after > 0)
    {
        // Left working, and the picture taken so many seconds in (the caller draws it, then waits).
        analyse(path, false, tailor);
        const auto t0 = Clock::now();
        while (busy_ && since(t0) < s.shot_after) std::this_thread::sleep_for(std::chrono::milliseconds(20));
        return;
    }
    std::atomic<bool> done{false};
    std::thread timer;
    if (s.stop_after > 0 || s.force_after > 0)
        timer = std::thread([this, &done, s] {
            const auto t0 = Clock::now();
            while (!done && since(t0) < (s.stop_after > 0 ? s.stop_after : s.force_after)) std::this_thread::sleep_for(std::chrono::milliseconds(20));
            if (!done) stop_ = std::max<int>(stop_, kStop);
            while (s.force_after > 0 && !done && since(t0) < s.force_after) std::this_thread::sleep_for(std::chrono::milliseconds(20));
            if (s.force_after > 0 && !done) stop_ = kForce;
        });
    analyse(path, true, tailor);
    done = true;
    if (timer.joinable()) timer.join();
    if (!s.save_to.empty() && result_.kind != Result::None)
    {
        save_what_ = 4;
        save(s.save_to);
        stop();
    }
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
