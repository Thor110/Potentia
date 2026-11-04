// Sieve hallway — Manual Navigation's screen (manual_nav.hpp).

#include "manual_nav.hpp"

#include "dialogs.hpp"
#include "dimensions.hpp"
#include "font.hpp"
#include "music.hpp"
#include "strings.hpp"

#include "cli/locate.hpp" // binary_address, read_file_bytes
#include "cli/vault.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/corridor.hpp" // bearing_of, unit_at_bearing, shortest_path
#include "sieve/sha256.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <deque>
#include <limits>
#include <sstream>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace hallway {

namespace {

const SDL_Color kWhite{255, 255, 255, 255}, kGrey{150, 150, 150, 255}, kDim{90, 90, 90, 255}, kGood{120, 230, 120, 255},
    kBad{255, 110, 110, 255}, kAccent{120, 200, 255, 255};
constexpr int kMinW = 1024, kMinH = 640;
constexpr size_t kMaxBytes = size_t(1) << 20;  // files taken up to 1 MiB
constexpr size_t kRouteBytes = size_t(64) << 10; // the route worked out up to 64 KiB
constexpr double kPi = 3.14159265358979323846;

std::filesystem::path from_u8(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }

void txt(SDL_Renderer* r, float x, float y, const std::string& s, SDL_Color c, float scale = 1) { draw_text(r, x, y, s, scale, c); }

bool inside(const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; }

std::string hex2(uint8_t b)
{
    static const char k[] = "0123456789ABCDEF";
    return {k[b >> 4], k[b & 15]};
}

int hex_digit(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

std::string fixed(double v, int places)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "%.*f", places, v);
    return buf;
}

// a / b as a double, for drawing only (both may be millions of bits).
double ratio(const sieve::BigUint& a, const sieve::BigUint& b)
{
    if (b.is_zero()) return 0;
    const size_t k = b.bit_length();
    return a.ratio_to_power_of_two(k) / b.ratio_to_power_of_two(k);
}

// The bytes it takes to name one of n offsets (at least 1).
size_t offset_bytes(size_t n)
{
    size_t bits = 0;
    for (size_t v = n > 0 ? n - 1 : 0; v; v >>= 1) ++bits;
    return std::max<size_t>(1, (bits + 7) / 8);
}

void SDLCALL load_chosen(void* user, const char* const* files, int)
{
    if (files && files[0]) static_cast<ManualNav*>(user)->file_chosen(files[0], 0);
}

void SDLCALL steps_chosen(void* user, const char* const* files, int)
{
    if (files && files[0]) static_cast<ManualNav*>(user)->file_chosen(files[0], 1);
}

void SDLCALL log_chosen(void* user, const char* const* files, int)
{
    if (files && files[0]) static_cast<ManualNav*>(user)->file_chosen(files[0], 2);
}

// Keep a log and the Undo depth as the settings have them, the logs folder, and who to tell.
struct Remembered
{
    bool keep_log = true;
    int undo_depth = 0;
    std::filesystem::path folder;
    std::function<void(bool, int)> changed;
};
Remembered& remembered()
{
    static Remembered r;
    return r;
}

// The time now, as the local clock has it (localtime_s and localtime_r: std::localtime is not
// thread-safe).
std::string local_time(const char* format)
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char out[64] = "";
    std::strftime(out, sizeof out, format, &tm);
    return out;
}

std::string u8(const std::filesystem::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

// The head every log and every walk saved begins with: what it was made on, so Load Log can open the
// file again and know it is the same bytes.
void write_head(std::ostream& o, const char* what, const std::string& path, size_t bytes, const std::string& sha, int decimals)
{
    o << "# Sieve " SIEVE_VERSION " Manual Navigation " << what << ", " << local_time("%Y-%m-%d %H:%M:%S") << "\n"
      << "# file," << path << "\n"
      << "# bytes," << bytes << "\n"
      << "# sha256," << sha << "\n"
      << "# precision," << decimals << "\n"
      << "step,kind,offset,from,to,bearing_before,bearing_after,off_before,off_after\n";
}

void circle(SDL_Renderer* r, float cx, float cy, float rad, SDL_Color c)
{
    SDL_FPoint pts[181];
    for (int i = 0; i <= 180; ++i)
    {
        const double t = 2 * kPi * i / 180;
        pts[i] = {cx + float(std::sin(t) * rad), cy - float(std::cos(t) * rad)};
    }
    SDL_SetRenderDrawColor(r, c.r, c.g, c.b, c.a);
    SDL_RenderLines(r, pts, 181);
}

void box(SDL_Renderer* r, const SDL_FRect& b, SDL_Color fill, SDL_Color line)
{
    SDL_SetRenderDrawColor(r, fill.r, fill.g, fill.b, fill.a);
    SDL_RenderFillRect(r, &b);
    SDL_SetRenderDrawColor(r, line.r, line.g, line.b, line.a);
    SDL_RenderRect(r, &b);
}

} // namespace

ManualNav::ManualNav(SDL_Window* window, SDL_Renderer* renderer, int angle_decimals)
    : window_(window), r_(renderer), decimals_(std::clamp(angle_decimals, 0, 20))
{
    status_ = tr("mnav.status.empty");
    keep_log_ = remembered().keep_log;
    undo_depth_ = remembered().undo_depth;
}

void ManualNav::remember(bool keep_log, int undo_depth, std::filesystem::path folder, std::function<void(bool, int)> changed)
{
    remembered() = {keep_log, undo_depth, std::move(folder), std::move(changed)};
}

int ManualNav::depth() const { return replaying_ || undo_depth_ <= 0 ? std::numeric_limits<int>::max() : undo_depth_; }

void ManualNav::open_log(bool append)
{
    if (log_out_.is_open()) log_out_.close();
    if (!append) log_path_.clear();
    const std::filesystem::path folder = remembered().folder;
    if (!keep_log_ || bytes_.empty() || (folder.empty() && log_path_.empty())) return;
    if (append)
    {
        log_out_.open(from_u8(log_path_), std::ios::binary | std::ios::app);
        return;
    }
    std::error_code ec;
    std::filesystem::create_directories(folder, ec);
    std::string stem = name_;
    for (char& c : stem)
        if (c == ' ' || c == '/' || c == '\\' || c == ':') c = '_';
    log_path_ = u8(folder / from_u8("mnav-" + local_time("%Y%m%d-%H%M%S") + "-" + stem + ".csv"));
    log_out_.open(from_u8(log_path_), std::ios::binary);
    if (!log_out_)
    {
        log_path_.clear();
        return;
    }
    // (begun part-way, Keep a log ticked after some turns: the walk so far first, so the log still
    // replays to the bytes now)
    write_head(log_out_, "log", path_, bytes_.size(), sha_, decimals_);
    if (!active_.empty()) log_out_ << "# the walk so far, before this log began\n";
    for (const Step& s : active_)
        log_out_ << s.number << ",turn," << s.offset << ',' << hex2(s.from) << ',' << hex2(s.to) << ',' << s.bearing_before << ','
                 << s.bearing_after << ',' << fixed(s.off_before, 6) << ',' << fixed(s.off_after, 6) << '\n';
    log_out_.flush();
}

void ManualNav::write_landed(const Step& s)
{
    if (!log_out_.is_open() || s.number == 0) return; // (a step replayed from the log is in it already)
    log_out_ << s.number << ',' << s.kind << ',' << s.offset << ',' << hex2(s.from) << ',' << hex2(s.to) << ',' << s.bearing_before << ','
             << s.bearing_after << ',' << fixed(s.off_before, 6) << ',' << fixed(s.off_after, 6) << '\n';
    log_out_.flush(); // (each as it lands: a crash keeps every step before it)
}

ManualNav::~ManualNav()
{
    if (worker_.joinable()) worker_.join();
    collect_reading(); // (the steps still in hand written to the log: Esc straight after a turn loses none)
}

ManualNav::Reading ManualNav::read(const std::vector<uint8_t>& bytes, int decimals, uint64_t gen)
{
    using sieve::BigUint;
    Reading out;
    out.gen = gen;
    // The loop of every file of 0 to n bytes, and this file's place in it (binary-v1, positional).
    const BigUint units = sieve::BinarySpace::count_shorter(uint64_t(bytes.size()) + 1);
    const BigUint v = sieve::cli::binary_address(bytes);
    out.address_bytes = (v.bit_length() + 7) / 8;
    out.bearing = sieve::bearing_of(v, units, decimals);
    out.turn = ratio(v, units);
    // The bearing just below (floor(v * 360 * 10^d / units)) and the one just above: where each
    // lands, and how far the file lies from the nearer of the two, as a share of the gap between.
    BigUint k = BigUint::pow(10, uint64_t(decimals));
    k.mul_small(360);
    BigUint below, rest;
    BigUint::divmod(BigUint::mul(v, k), units, below, rest);
    const BigUint lo = sieve::unit_at_bearing(below, units, decimals);
    BigUint next = below;
    next.add_small(1);
    const BigUint hi = next < k ? sieve::unit_at_bearing(next, units, decimals) : units;
    BigUint down = v, up = hi, gap = hi;
    down -= lo;
    up -= v;
    gap -= lo;
    out.exact = down.is_zero();
    out.off = out.exact || gap.is_zero() ? 0 : std::clamp(2 * ratio(down < up ? down : up, gap), 0.0, 1.0);
    if (bytes.size() <= kRouteBytes)
    {
        const sieve::ShortestPath sp = sieve::shortest_path(v, units, decimals);
        out.routed = true;
        out.route = sp.written;
        out.route_bits = sp.bits;
    }
    return out;
}

bool ManualNav::load(const std::string& path)
{
    try
    {
        std::vector<uint8_t> b = sieve::cli::read_file_bytes(from_u8(path));
        if (sieve::cli::vault::withheld_bytes(b))
        {
            status_ = tr("vault.withheld");
            return false;
        }
        if (b.empty())
        {
            status_ = tr("mnav.status.empty_file");
            return false;
        }
        if (b.size() > kMaxBytes)
        {
            status_ = trf("mnav.status.too_large", {std::to_string(b.size())});
            return false;
        }
        if (worker_.joinable()) worker_.join();
        collect_reading(); // (the last file's steps written, before its log is let go)
        if (log_out_.is_open()) log_out_.close();
        bytes_ = std::move(b);
        const std::u8string fn = from_u8(path).filename().u8string();
        name_ = std::string(fn.begin(), fn.end());
        std::error_code ec;
        const std::filesystem::path whole = std::filesystem::absolute(from_u8(path), ec);
        path_ = ec ? path : u8(whole);
        sha_ = sieve::Sha256::hex(sieve::Sha256::hash(std::string_view(reinterpret_cast<const char*>(bytes_.data()), bytes_.size())));
        steps_made_ = 0;
        unlogged_.clear();
        at_ = 0;
        chosen_ = 0;
        pending_hi_ = -1;
        active_.clear();
        redo_.clear();
        undoable_ = 0;
        marks_.clear();
        have_reading_ = false;
        {
            std::lock_guard<std::mutex> lock(mx_);
            answers_.clear();
        }
        original_address_bytes_ = 0;
        ++gen_;
        status_ = trf("mnav.status.loaded", {name_, std::to_string(bytes_.size())});
        start_reading();
        if (waiting_log_)
        {
            // A log waiting for its file: replayed where this is the same bytes.
            if (waiting_log_->first != sha_)
            {
                // The path the log names, holding other bytes (changed since): the log waits for the
                // right file. A file chosen by hand that is not it lets the log go.
                status_ = trf(from_log_ ? "mnav.status.log_changed" : "mnav.status.log_not_same", {path_});
                if (!from_log_) waiting_log_.reset();
                open_log(false);
                return true;
            }
            auto w = std::move(*waiting_log_);
            waiting_log_.reset();
            log_path_ = waiting_log_path_;
            replay(w.second);
            open_log(true);
            // (found somewhere else by hand: where it is now, for the next Load Log; the last such
            // line is the one read)
            if (!from_log_ && log_out_.is_open()) log_out_ << "# file," << path_ << "\n", log_out_.flush();
            return true;
        }
        if (!replaying_) open_log(false);
        return true;
    }
    catch (const std::exception& e)
    {
        status_ = trf("mnav.status.load_failed", {e.what()});
        return false;
    }
}

void ManualNav::start_reading()
{
    if (bytes_.empty()) return;
    Job j;
    j.gen = gen_;
    j.decimals = decimals_;
    j.whole = true;
    j.bytes = bytes_;
    queue(std::move(j));
}

void ManualNav::queue(Job j)
{
    bool start = false;
    {
        std::lock_guard<std::mutex> lock(mx_);
        jobs_.push_back(std::move(j));
        start = !busy_;
        if (start) busy_ = true;
    }
    if (!start) return; // (the worker takes it after the ones before it)
    if (worker_.joinable()) worker_.join();
    worker_ = std::thread([this] {
        for (;;)
        {
            Job j;
            {
                std::lock_guard<std::mutex> lock(mx_);
                if (jobs_.empty())
                {
                    busy_ = false;
                    return;
                }
                j = std::move(jobs_.front());
                jobs_.pop_front();
            }
            if (j.whole) wbytes_ = std::move(j.bytes);
            else if (j.at < wbytes_.size()) wbytes_[j.at] = j.value;
            Reading r;
            try
            {
                r = read(wbytes_, j.decimals, j.gen);
            }
            catch (const std::exception& e)
            {
                r.gen = j.gen;
                r.bearing = e.what();
            }
            r.decimals = j.decimals;
            std::lock_guard<std::mutex> lock(mx_);
            answers_.push_back(std::move(r));
        }
    });
}

void ManualNav::collect_reading()
{
    std::deque<Reading> got;
    {
        std::lock_guard<std::mutex> lock(mx_);
        got.swap(answers_);
    }
    for (Reading& r : got)
    {
        const bool first = marks_.empty();
        const Reading before = now_;
        const bool had = have_reading_;
        now_ = std::move(r);
        have_reading_ = true;
        if (original_address_bytes_ == 0) original_address_bytes_ = now_.address_bytes;
        // The step it answers for (each step its own generation): where it started, the reading
        // before this one; where it landed, this one.
        auto land = [&](Step& s) {
            if (!s.pending || s.gen != now_.gen) return;
            s.bearing_before = had ? before.bearing : std::string();
            s.off_before = had ? before.off : 1;
            s.bearing_after = now_.bearing;
            s.off_after = now_.off;
            s.pending = false;
        };
        for (Step& s : active_) land(s);
        for (Step& s : unlogged_) land(s);
        while (!unlogged_.empty() && !unlogged_.front().pending)
        {
            write_landed(unlogged_.front());
            unlogged_.pop_front();
        }
        if (now_.decimals != decimals_) continue; // (worked out at a precision since changed: no mark)
        Mark m;
        m.turn = now_.turn;
        m.off = now_.off;
        m.kind = first ? 0 : now_.exact ? 3 : now_.off < before.off ? 1 : 2;
        marks_.push_back(m);
    }
}

void ManualNav::wait()
{
    // (The worker ends once the queue is empty; a step queued meanwhile starts it again.)
    if (worker_.joinable()) worker_.join();
    collect_reading();
}

void ManualNav::set_byte(uint64_t at, uint8_t value, const char* kind)
{
    if (at >= bytes_.size() || bytes_[at] == value) return;
    Step s;
    s.kind = kind;
    s.offset = at;
    s.from = bytes_[at];
    s.to = value;
    bytes_[at] = value;
    s.gen = ++gen_;
    s.number = replaying_ ? 0 : ++steps_made_; // (0: replayed from the log, in it already)
    unlogged_.push_back(s);
    if (std::string(kind) == "turn")
    {
        active_.push_back(s);
        redo_.clear();
        undoable_ = std::min(undoable_ + 1, depth());
    }
    Job j;
    j.gen = s.gen;
    j.decimals = decimals_;
    j.at = at;
    j.value = value;
    queue(std::move(j));
}

void ManualNav::undo()
{
    if (undoable_ == 0 || active_.empty())
    {
        status_ = tr("mnav.status.no_undo");
        return;
    }
    Step s = active_.back();
    active_.pop_back();
    --undoable_;
    set_byte(s.offset, s.from, "undo");
    redo_.push_back(s);
    status_ = trf("mnav.status.undone", {std::to_string(undoable_)});
}

void ManualNav::redo()
{
    if (redo_.empty())
    {
        status_ = tr("mnav.status.no_redo");
        return;
    }
    Step s = redo_.back();
    redo_.pop_back();
    set_byte(s.offset, s.to, "redo");
    s.pending = true;
    s.gen = gen_; // (its reading is the redo's)
    active_.push_back(s);
    undoable_ = std::min(undoable_ + 1, depth());
    status_ = tr("mnav.status.redone");
}

bool ManualNav::save_steps(const std::string& path)
{
    wait();
    std::ofstream o(from_u8(path), std::ios::binary);
    if (!o)
    {
        status_ = trf("mnav.status.save_failed", {path});
        return false;
    }
    // (the same head as a log's, so Load Log replays a walk too)
    write_head(o, "walk", path_, bytes_.size(), sha_, decimals_);
    for (size_t i = 0; i < active_.size(); ++i)
    {
        const Step& s = active_[i];
        o << i + 1 << ",turn," << s.offset << ',' << hex2(s.from) << ',' << hex2(s.to) << ',' << s.bearing_before << ','
          << s.bearing_after << ',' << fixed(s.off_before, 6) << ',' << fixed(s.off_after, 6) << '\n';
    }
    status_ = trf("mnav.status.saved", {std::to_string(active_.size()), path});
    return bool(o);
}

std::string ManualNav::readout() const
{
    if (bytes_.empty()) return tr("mnav.status.empty");
    if (!have_reading_) return tr("mnav.working");
    std::string t = trf("mnav.rotation", {now_.bearing, std::to_string(decimals_)});
    t += ", " + (now_.exact ? tr("mnav.exact") : trf("mnav.off", {fixed(now_.off, 4)}));
    if (!now_.routed) return t + "; " + tr("mnav.no_route");
    const double steps = double(active_.size() * (offset_bytes(bytes_.size()) + 1));
    const double net = double(original_address_bytes_) - now_.route_bits / 8 - steps;
    return t + "; " +
           trf("mnav.route", {now_.route, fixed(now_.route_bits / 8, 1), std::to_string(original_address_bytes_), std::to_string(active_.size()),
                              fixed(steps, 0), (net >= 0 ? "+" : "") + fixed(net, 1)});
}

void ManualNav::file_chosen(const std::string& path, int what)
{
    std::lock_guard<std::mutex> lock(mx_);
    chosen_file_ = std::make_pair(path, what);
}

bool ManualNav::load_log(const std::string& path)
{
    std::ifstream in(from_u8(path), std::ios::binary);
    if (!in)
    {
        status_ = trf("mnav.status.log_bad", {path});
        return false;
    }
    std::string file, sha, line;
    size_t bytes = 0;
    bool head = false;
    std::vector<Logged> lines;
    while (std::getline(in, line))
    {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        auto field = [&](const char* key) -> std::optional<std::string> {
            const std::string k = std::string("# ") + key + ",";
            if (line.compare(0, k.size(), k) != 0) return std::nullopt;
            return line.substr(k.size());
        };
        if (auto v = field("file")) file = *v, head = true; // (a path may hold commas: the rest of the line; the last such line wins, where the file was last found)
        else if (auto v = field("bytes")) bytes = size_t(std::strtoull(v->c_str(), nullptr, 10));
        else if (auto v = field("sha256")) sha = *v;
        else if (auto v = field("precision"))
        {
            // (the head's, then any change: each a line, replayed in its place)
            const int d = std::clamp(std::atoi(v->c_str()), 0, 20);
            Logged l;
            l.kind = "precision";
            l.decimals = d;
            lines.push_back(l);
        }
        else if (line.empty() || line[0] == '#' || line.compare(0, 5, "step,") == 0) continue;
        else
        {
            // step,kind,offset,from,to,...
            std::istringstream row(line);
            std::string number, kind, offset, from, to;
            if (!std::getline(row, number, ',') || !std::getline(row, kind, ',') || !std::getline(row, offset, ',') || !std::getline(row, from, ',') ||
                !std::getline(row, to, ','))
                continue;
            if (kind != "turn" && kind != "undo" && kind != "redo") continue;
            Logged l;
            l.kind = kind;
            l.offset = std::strtoull(offset.c_str(), nullptr, 10);
            l.from = uint8_t(std::strtoul(from.c_str(), nullptr, 16));
            l.to = uint8_t(std::strtoul(to.c_str(), nullptr, 16));
            lines.push_back(l);
        }
    }
    if (!head || sha.size() != 64)
    {
        status_ = trf("mnav.status.log_bad", {path});
        return false;
    }
    (void)bytes; // (the SHA-256 says it; the size is for whoever reads the log)
    // The file it was made on, from where it was: replayed where it is the same bytes; asked for
    // where it is not there, or not the same.
    waiting_log_ = std::make_pair(sha, std::move(lines));
    waiting_log_path_ = path;
    from_log_ = true;
    const bool loaded = load(file);
    from_log_ = false;
    if (!loaded)
    {
        status_ = trf("mnav.status.log_file_missing", {file});
        return false;
    }
    return !waiting_log_; // (still waiting: the file there has changed, its status says so)
}

void ManualNav::replay(const std::vector<Logged>& lines)
{
    // The log's steps again, in order, undos and redos included (the undo depth no limit meanwhile);
    // none written again, the log already holding them.
    replaying_ = true;
    uint64_t done = 0, number = 0;
    bool fits = true;
    for (const Logged& l : lines)
    {
        if (l.kind == "precision")
        {
            if (l.decimals != decimals_)
            {
                wait();
                decimals_ = l.decimals;
                marks_.clear();
                have_reading_ = false;
                ++gen_;
                start_reading();
            }
            continue;
        }
        ++number;
        if (l.kind == "turn")
        {
            if (l.offset >= bytes_.size() || bytes_[l.offset] != l.from)
            {
                fits = false;
                break;
            }
            set_byte(l.offset, l.to, "turn");
        }
        else if (l.kind == "undo") undo();
        else redo();
        ++done;
    }
    replaying_ = false;
    steps_made_ = number;
    status_ = fits ? trf("mnav.status.replayed", {name_, std::to_string(done)}) : trf("mnav.status.log_mismatch", {std::to_string(number)});
}

void ManualNav::poll_chosen()
{
    std::optional<std::pair<std::string, int>> c;
    {
        std::lock_guard<std::mutex> lock(mx_);
        c.swap(chosen_file_);
    }
    if (!c) return;
    if (c->second == 0) load(c->first);
    else if (c->second == 1) save_steps(c->first);
    else load_log(c->first);
}

ManualNav::Result ManualNav::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
    music_mode(MusicMode::Menus);
    music_colours_default();
    while (!done_)
    {
        SDL_Event e;
        if (SDL_WaitEventTimeout(&e, 16))
        {
            handle(e);
            while (SDL_PollEvent(&e)) handle(e);
        }
        poll_chosen();
        collect_reading();
        render();
        present(r_);
    }
    SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    return result_;
}

void ManualNav::press(SDL_Keycode k, SDL_Keymod mod) { key(k, mod); }

void ManualNav::type(const std::string& utf8)
{
    // Hex typed into the byte chosen: the high digit, then the low, then on to the next byte.
    for (const char c : utf8)
    {
        const int d = hex_digit(c);
        if (d < 0 || bytes_.empty()) continue;
        const uint64_t at = at_ + uint64_t(chosen_);
        if (at >= bytes_.size()) continue;
        // The first digit waits for the second (shown in the box), so a byte typed is one step.
        if (pending_hi_ < 0)
        {
            pending_hi_ = d;
            continue;
        }
        set_byte(at, uint8_t((pending_hi_ << 4) | d), "turn");
        pending_hi_ = -1;
        if (chosen_ + 1 < span_ && at + 1 < bytes_.size()) ++chosen_;
    }
}

void ManualNav::key(SDL_Keycode k, SDL_Keymod mod)
{
    const bool ctrl = (mod & SDL_KMOD_CTRL) != 0, shift = (mod & SDL_KMOD_SHIFT) != 0;
    const uint64_t n = bytes_.size();
    auto clamp_window = [&] {
        if (n == 0) return;
        if (at_ >= n) at_ = (n - 1) / uint64_t(span_) * uint64_t(span_);
        chosen_ = std::clamp(chosen_, 0, int(std::min<uint64_t>(uint64_t(span_), n - at_)) - 1);
    };
    if (k == SDLK_ESCAPE)
    {
        done_ = true;
        return;
    }
    if (ctrl && k == SDLK_O)
    {
        open_file_dialog(load_chosen, this, window_, nullptr, 0, nullptr, false, false, this);
        return;
    }
    if (ctrl && k == SDLK_L)
    {
        static const SDL_DialogFileFilter kLogs[] = {{"Manual Navigation logs", "csv"}};
        open_file_dialog(log_chosen, this, window_, kLogs, 1, remembered().folder.empty() ? nullptr : u8(remembered().folder).c_str(), false, false, this);
        return;
    }
    if (k == SDLK_L && !ctrl)
    {
        // Keep a log, on or off (remembered): on, a log begun with the walk so far.
        keep_log_ = !keep_log_;
        if (remembered().changed) remembered().changed(keep_log_, undo_depth_);
        if (keep_log_)
        {
            wait();
            open_log(false);
            status_ = log_path_.empty() ? tr("mnav.status.log_on_later") : trf("mnav.status.log_on", {log_path_});
        }
        else
        {
            if (log_out_.is_open()) log_out_.close();
            log_path_.clear();
            status_ = tr("mnav.status.log_off");
        }
        return;
    }
    if (k == SDLK_U && !ctrl)
    {
        // The undo depth: all, 10, 100, 1,000 (remembered).
        undo_depth_ = undo_depth_ == 0 ? 10 : undo_depth_ >= 1000 ? 0 : undo_depth_ * 10;
        undoable_ = std::min(undoable_, depth());
        if (remembered().changed) remembered().changed(keep_log_, undo_depth_);
        status_ = trf("mnav.status.depth", {undo_depth_ == 0 ? tr("mnav.depth.all") : std::to_string(undo_depth_)});
        return;
    }
    if (ctrl && k == SDLK_S)
    {
        if (active_.empty()) status_ = tr("mnav.status.no_steps");
        else save_file_dialog(steps_chosen, this, window_, nullptr, 0, nullptr, this);
        return;
    }
    if (ctrl && k == SDLK_Z) return undo();
    if (ctrl && k == SDLK_Y) return redo();
    if (k == SDLK_LEFTBRACKET || k == SDLK_RIGHTBRACKET)
    {
        // The Angle Precision here: the marks so far were measured at the old one, so they start again.
        // (Its line in the log after every step before it, so a replay changes it at the same place.)
        wait();
        decimals_ = std::clamp(decimals_ + (k == SDLK_RIGHTBRACKET ? 1 : -1), 0, 20);
        if (log_out_.is_open()) log_out_ << "# precision," << decimals_ << "\n", log_out_.flush();
        marks_.clear();
        have_reading_ = false;
        ++gen_;
        start_reading();
        return;
    }
    if (n == 0) return;
    pending_hi_ = -1; // (a first digit typed and then anything else: let go)
    switch (k)
    {
    case SDLK_LEFT:
        if (chosen_ > 0) --chosen_;
        else if (at_ > 0) at_ -= uint64_t(span_), chosen_ = span_ - 1;
        break;
    case SDLK_RIGHT:
        if (chosen_ + 1 < span_ && at_ + uint64_t(chosen_) + 1 < n) ++chosen_;
        else if (at_ + uint64_t(span_) < n) at_ += uint64_t(span_), chosen_ = 0;
        break;
    case SDLK_UP:
    case SDLK_DOWN:
    {
        const uint64_t at = at_ + uint64_t(chosen_);
        if (at < n) set_byte(at, uint8_t(bytes_[at] + (k == SDLK_UP ? 1 : -1) * (shift ? 16 : 1)), "turn");
        break;
    }
    case SDLK_PAGEUP: at_ = at_ >= uint64_t(span_) ? at_ - uint64_t(span_) : 0; break;
    case SDLK_PAGEDOWN:
        if (at_ + uint64_t(span_) < n) at_ += uint64_t(span_);
        break;
    case SDLK_HOME: at_ = 0, chosen_ = 0; break;
    case SDLK_END: at_ = (n - 1) / uint64_t(span_) * uint64_t(span_); break;
    case SDLK_W:
        span_ = span_ == 4 ? 8 : span_ == 8 ? 16 : 4;
        at_ = at_ / uint64_t(span_) * uint64_t(span_);
        break;
    default: break;
    }
    clamp_window();
}

void ManualNav::handle(const SDL_Event& event)
{
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT)
    {
        result_ = Result::Quit;
        done_ = true;
    }
    else if (e.type == SDL_EVENT_TEXT_INPUT) type(e.text.text);
    else if (e.type == SDL_EVENT_KEY_DOWN) key(e.key.key, e.key.mod);
    else if (e.type == SDL_EVENT_MOUSE_WHEEL && !bytes_.empty())
        key(e.wheel.y > 0 ? SDLK_UP : SDLK_DOWN, SDL_KMOD_NONE);
    else if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (const auto& [rect, id] : buttons_)
            if (inside(rect, e.button.x, e.button.y))
            {
                const SDL_Keycode keys[] = {SDLK_O, SDLK_Z, SDLK_Y, SDLK_S, SDLK_W, SDLK_L, SDLK_L, SDLK_U};
                const bool with_ctrl[] = {true, true, true, true, false, true, false, false};
                key(keys[id], with_ctrl[id] ? SDL_KMOD_CTRL : SDL_KMOD_NONE);
                return;
            }
        for (const auto& [rect, id] : arrows_)
            if (inside(rect, e.button.x, e.button.y))
            {
                chosen_ = id / 2;
                pending_hi_ = -1;
                key(id % 2 ? SDLK_DOWN : SDLK_UP, SDL_KMOD_NONE);
                return;
            }
        for (const auto& [rect, id] : cells_)
            if (inside(rect, e.button.x, e.button.y))
            {
                chosen_ = id;
                pending_hi_ = -1;
                return;
            }
    }
}

void ManualNav::render()
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    if (w < kMinW || h < kMinH)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        SDL_SetRenderLogicalPresentation(r_, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    else SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
    const float W = float(w), H = float(h);
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);
    const Theme& line = theme_of(Media::Binary);
    const size_t cols = size_t(std::max(20.0f, (W - 48) / 8));

    txt(r_, 24, 16, tr("mnav.title"), kWhite, 2.5f);
    // Under the title: the rotation, then the route and what the steps cost.
    if (!bytes_.empty())
    {
        txt(r_, 24, 46, fit_cells(trf("mnav.file", {name_, std::to_string(bytes_.size()), std::to_string(at_), std::to_string(std::min<uint64_t>(bytes_.size(), at_ + uint64_t(span_)) - 1)}), cols), kGrey);
        const std::string ro = readout();
        const size_t semi = ro.find("; ");
        txt(r_, 24, 62, fit_cells(ro.substr(0, semi), cols), now_.exact && have_reading_ ? kGood : kWhite);
        if (semi != std::string::npos) txt(r_, 24, 78, fit_cells(ro.substr(semi + 2), cols), kAccent);
        if (busy_) txt(r_, W - 24 - 8 * float(tr("mnav.working").size()), 46, tr("mnav.working"), kDim);
    }
    else txt(r_, 24, 46, tr("mnav.subtitle"), kGrey);

    // The bytes, at the left: 2 a row for 4, 4 a row for 8 and 16.
    cells_.clear();
    arrows_.clear();
    const int per_row = span_ == 4 ? 2 : 4;
    const float cw = 96, ch = 132, left = 40, top = 116;
    for (int i = 0; i < span_; ++i)
    {
        const uint64_t at = at_ + uint64_t(i);
        const float x = left + cw * float(i % per_row), y = top + ch * float(i / per_row);
        const SDL_FRect cell{x, y + 24, cw - 16, 56};
        const bool real = at < bytes_.size();
        const bool chosen = real && i == chosen_;
        box(r_, cell, chosen ? SDL_Color{40, 60, 90, 255} : SDL_Color{15, 15, 15, 255}, chosen ? kAccent : kDim);
        if (!real) continue;
        cells_.emplace_back(cell, i);
        const SDL_FRect up{x + 20, y, cw - 56, 20}, down{x + 20, y + 84, cw - 56, 20};
        arrows_.emplace_back(up, i * 2);
        arrows_.emplace_back(down, i * 2 + 1);
        txt(r_, up.x + up.w / 2 - 8, up.y + 2, "/\\", chosen ? kWhite : kGrey, 1);
        txt(r_, down.x + down.w / 2 - 8, down.y + 2, "\\/", chosen ? kWhite : kGrey, 1);
        bool changed = false;
        for (const Step& s : active_)
            if (s.offset == at) changed = true;
        if (chosen && pending_hi_ >= 0) // (a first digit typed, the second awaited)
            txt(r_, cell.x + (cell.w - 64) / 2, cell.y + 12, std::string(1, "0123456789ABCDEF"[pending_hi_]) + "_", kAccent, 4);
        else txt(r_, cell.x + (cell.w - 64) / 2, cell.y + 12, hex2(bytes_[at]), changed ? kAccent : kWhite, 4);
        txt(r_, x, y + 108, "+" + std::to_string(at), kDim);
    }

    // The circle, at the right.
    const float cx = W * 0.72f, cy = 116 + (H - 116 - 90) / 2;
    const float rad = std::max(60.0f, std::min(W * 0.22f, (H - 116 - 110) / 2));
    circle(r_, cx, cy, rad, kDim);
    circle(r_, cx, cy, 3, kGrey); // the centre: a bearing landed on exactly
    txt(r_, cx - 4, cy - rad - 16, "0", kGrey);
    auto at_of = [&](const Mark& m) {
        const double t = 2 * kPi * m.turn;
        return SDL_FPoint{cx + float(std::sin(t) * m.off * rad), cy - float(std::cos(t) * m.off * rad)};
    };
    for (size_t i = 0; i < marks_.size(); ++i)
    {
        const Mark& m = marks_[i];
        const SDL_FPoint p = at_of(m);
        const bool last = i + 1 == marks_.size();
        const float s = last ? 7 : 3;
        const SDL_FRect b{p.x - s, p.y - s, 2 * s, 2 * s};
        if (m.kind == 0) box(r_, b, line.bg, line.edge);
        else box(r_, b, m.kind == 1 ? kGood : m.kind == 2 ? kBad : kWhite, last ? kWhite : (m.kind == 1 ? kGood : m.kind == 2 ? kBad : kWhite));
    }
    {
        // (under the circle, from its left edge to the window's: on two lines where it needs them)
        const size_t room = size_t(std::max(20.0f, (W - (cx - rad) - 24) / 8));
        const std::string c = tr("mnav.circle");
        const size_t cut = c.size() <= room ? c.size() : c.rfind(' ', room);
        txt(r_, cx - rad, cy + rad + 10, c.substr(0, cut), kDim);
        if (cut < c.size()) txt(r_, cx - rad, cy + rad + 24, fit_cells(c.substr(cut + 1), room), kDim);
    }

    // The buttons, and what the keys do.
    buttons_.clear();
    const char* labels[] = {"mnav.button.load", "mnav.button.undo", "mnav.button.redo", "mnav.button.save", "mnav.button.window",
                            "mnav.button.load_log", "mnav.button.keep_log", "mnav.button.depth"};
    // (in rows that stop short of the circle, the last row lowest)
    std::vector<std::string> names;
    for (int i = 0; i < 8; ++i)
        names.push_back(i == 4   ? trf(labels[i], {std::to_string(span_)})
                        : i == 6 ? trf(labels[i], {keep_log_ ? "X" : " "})
                        : i == 7 ? trf(labels[i], {undo_depth_ == 0 ? tr("mnav.depth.all") : std::to_string(undo_depth_)})
                                 : tr(labels[i]));
    const float room = std::max(300.0f, cx - rad - 24 - 16);
    std::vector<int> row_of(names.size());
    {
        float bx = 0;
        int row = 0;
        for (size_t i = 0; i < names.size(); ++i)
        {
            const float w = 8 * float(names[i].size()) + 24;
            if (bx > 0 && bx + w > room) ++row, bx = 0;
            row_of[i] = row;
            bx += w + 10;
        }
    }
    const int rows = row_of.back() + 1;
    float bx = 24;
    for (size_t i = 0; i < names.size(); ++i)
    {
        if (i > 0 && row_of[i] != row_of[i - 1]) bx = 24;
        const SDL_FRect b{bx, H - 92 - 34 * float(rows - 1 - row_of[i]), 8 * float(names[i].size()) + 24, 26};
        box(r_, b, SDL_Color{20, 20, 20, 255}, kGrey);
        txt(r_, b.x + 12, b.y + 9, names[i], kWhite);
        buttons_.emplace_back(b, int(i));
        bx += b.w + 10;
    }
    txt(r_, 24, H - 52, fit_cells(status_, cols), kGood);
    txt(r_, 24, H - 32, fit_cells(tr("mnav.footer"), cols), kGrey);
}

} // namespace hallway
