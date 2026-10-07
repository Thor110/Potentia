// Sieve hallway — the setup menu's behaviour: the rows, the map, the filters overlay, and the
// arithmetic behind what it shows. See menu.hpp for what the menu is and how it is driven.
//
// Three things in here are easy to mistake for cosmetics and are not. The bar lengths are worked
// out in bits (log2 of a line's size) because the lines differ by factors no screen could draw
// literally. The padding figures come from modular arithmetic on the line's size rather than from
// building the line, since the line may be far too large to build. And find_limits() searches for
// the largest settings this machine could actually walk, growing the lines that share a
// constraint alternately rather than one at a time, because satisfying one line to its own limit
// first can leave another with nothing.
//
// Rows are numbered, and the numbers appear in the row table, the filter mapping and the tests
// alike, so a row inserted in the middle moves everything after it.
#include "menu.hpp"
#include "cli/timings.hpp"
#include "display.hpp"
#include "music.hpp"

#include "font.hpp"
#include "gpu_memory.hpp"
#include "mesh.hpp"
#include "strings.hpp"
#include "theme.hpp"
#include "world.hpp"

#include "cli/dictionaries.hpp"
#include "cli/plugins.hpp"
#include "cli/models.hpp"

#include "sieve/alphabet.hpp"
#include "sieve/audio.hpp"
#include "sieve/notes3.hpp"
#include "sieve/sound.hpp"
#include "sieve/filter.hpp"
#include "sieve/plugin.hpp"
#include "sieve/corridor.hpp"
#include "sieve/binaryspace.hpp"
#include "sieve/biguint.hpp"
#include "sieve/image.hpp"

#include <algorithm>
#include <optional>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <functional>
#include <array>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <vector>

namespace hallway {

namespace {

// The lines to start on, by --line name, in door order (dimensions.hpp).
std::vector<std::string> start_lines()
{
    std::vector<std::string> out;
    for (const Dimension& d : kDimensions) out.push_back(d.line);
    return out;
}
const std::vector<std::string> kStartLines = start_lines();
// What FIND MY LIMITS can be told to spend the budget on: every line, or one of them (binary
// first, as it stands first on the map).
std::vector<std::string> focuses()
{
    std::vector<std::string> out = {"all", kDimensions[kBinaryLine].line};
    for (const Dimension& d : kDimensions)
        if (d.media != Media::Binary) out.push_back(d.line);
    return out;
}
const std::vector<std::string> kFocuses = focuses();
// Where the map starts: the settings' labels and values take the screen's left to here.
constexpr float kMapX = 620;
// The unit line at door li, as its kind's index into the filter settings (cfg_.lines), or -1.
int unit_at(int li) { return li >= 0 && li < kLines && kDimensions[li].unit ? int(*kDimensions[li].unit) : -1; }
const std::vector<std::string> kModes = {"positional", "scrambled", "guided"};
const std::vector<std::string> kAlphabets = {"lower27", "babel29", "ascii95"};
const std::vector<std::string> kCanons = {"v2", "v1"};
const std::vector<std::string> kPalettes = {"mono", "ega16", "rgb332", "rgb24"};
// The audio line's note set, from the settings (a notes2 set the settings cannot make falls back
// to notes104, and the row says so).
sieve::NoteSet note_set_of_settings(const Settings& s)
{
    if (s.note_set != "notes2") return sieve::NoteSet{};
    try
    {
        return sieve::make_note_set(sieve::note_midi_of(s.note_low), sieve::note_midi_of(s.note_high), s.note_durations, s.voices);
    }
    catch (const std::exception&)
    {
        return sieve::NoteSet{};
    }
}
const std::vector<std::string> kNoteDurationPresets = {"seEqQhHw", "seqhw", "eEqQhHw", "eqhw", "qhw", "sq"};
// The audio line's sets, in the order NOTE SET steps through them.
const std::vector<std::string> kNoteSets = {"notes104", "notes2", "notes3", "pcm"};
// notes3's lengths, as LENGTHS steps through them: ticks a quarter note, and the longest length in
// ticks. The settings file takes any.
const std::vector<std::pair<uint32_t, uint32_t>> kNotes3Lengths = {{4, 16}, {4, 32}, {2, 8}, {8, 32}, {12, 48}, {24, 96}, {4, 64}};
bool is_notes3(const Settings& s) { return s.note_set == "notes3"; }
// The notes3 set the settings make, or nothing if they cannot make one (notes104 is used then).
std::optional<sieve::Notes3Set> notes3_of_settings(const Settings& s)
{
    try
    {
        std::vector<uint32_t> ins;
        for (size_t at = 0;;)
        {
            const size_t c = s.n3_instruments.find(',', at);
            ins.push_back(uint32_t(std::stoul(s.n3_instruments.substr(at, c == std::string::npos ? std::string::npos : c - at))));
            if (c == std::string::npos) break;
            at = c + 1;
        }
        return sieve::make_notes3_set(sieve::notes3_midi_of(s.n3_low), sieve::notes3_midi_of(s.n3_high), s.n3_tpq, s.n3_longest, s.n3_levels,
                                      s.n3_voices, s.n3_tempo, ins);
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}
// The sample rates SAMPLE RATE steps through; the settings file takes any rate.
const std::vector<uint32_t> kPcmRates = {8000, 11025, 16000, 22050, 32000, 44100, 48000, 96000, 192000};
bool is_pcm(const Settings& s) { return s.note_set == "pcm"; }
bool pcm_ok(const Settings& s)
{
    try
    {
        sieve::make_pcm_format(s.pcm_rate, s.pcm_bits, s.pcm_channels);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}
// The audio line's shape from the settings: how many symbols a position has, and how many
// positions a unit has (a pcm set the settings cannot make falls back to notes104, as notes2 does).
std::pair<uint32_t, uint64_t> audio_shape(const Settings& s)
{
    if (is_pcm(s) && pcm_ok(s)) return {uint32_t(1) << s.pcm_bits, uint64_t(s.samples) * s.pcm_channels};
    if (is_notes3(s))
        if (const auto n3 = notes3_of_settings(s)) return {n3->base(), uint64_t(s.notes) * n3->voices};
    const sieve::NoteSet set = note_set_of_settings(s);
    return {set.base(), uint64_t(s.notes) * set.voices};
}

std::string cycle(const std::vector<std::string>& v, const std::string& cur, int dir)
{
    auto it = std::find(v.begin(), v.end(), cur);
    const int i = it == v.end() ? 0 : int(it - v.begin());
    const int n = int(v.size());
    return v[size_t(((i + dir) % n + n) % n)];
}

uint32_t palette_size(const std::string& id) { return sieve::palette_by_id(id).size(); }
uint32_t alphabet_size(const std::string& id) { return sieve::alphabet_of(id).size(); }

// a * b * c, stopping at UINT64_MAX instead of wrapping (settings go up to 2^32 - 1 each).
uint64_t positions(uint64_t a, uint64_t b, uint64_t c = 1)
{
    uint64_t r = a;
    for (uint64_t f : {b, c})
    {
        if (f != 0 && r > UINT64_MAX / f) return UINT64_MAX;
        r *= f;
    }
    return r;
}
uint32_t clamp32(uint64_t v) { return uint32_t(std::min<uint64_t>(v, UINT32_MAX)); }

uint32_t parse_u32(const sieve::cli::Args& a, const char* key, uint32_t def) { return a.has(key) ? a.get_positive(key, def) : def; }


// The budget. Three things bound what this machine can open, and a shape has to fit all three:
//
//   memory    the hallway keeps the units around you in a cache -- up to cached_units() of them,
//             each its characters, pixels, notes or coordinates at four bytes a position, and its
//             address twice, as a number and as hex -- and that cache is given ITEM MEMORY's share
//             of installed memory (a quarter at first). Only one line is walked at a time, so the largest line is what
//             has to fit.
//   graphics  the models line's crate faces (the model image cache) and the world itself -- the
//             frames, the portals, the signs, the text (world_graphics_mb) -- have to fit the graphics
//             memory set in Settings > Graphics, since SDL cannot ask the card.
//   time      opening one unit means turning its address into its content, and that grows faster
//             than the address. It is measured on this machine, once, at two lengths (which gives
//             how it grows, Karatsuba's 1.6 or so), and the longest address that opens within the
//             time budget (TIME BUDGET, 50 ms at first) is the limit, so a slow build or a slow
//             machine gets smaller limits rather than a hallway that freezes.
//
// None is a limit of the design. They describe the machine of the day, and a bigger one finds
// bigger numbers with the same arithmetic.
std::atomic<double> g_time_budget{50};   // the longest one unit may take to open (set_time_budget)

void text(SDL_Renderer* r, float x, float y, const std::string& s, float scale, SDL_Color c) { draw_text(r, x, y, s, scale, c); }

// The tallest picture, height over width, of any line's items (faces.ini): what the picture
// cache has to be sized for, since a picture is the chosen size wide and this much taller.
double tallest_face()
{
    double a = 1.0;
    for (const Dimension& d : kDimensions)
        if (d.media != Media::Binary) a = std::max(a, double(load_face_rect(d.id).aspect())); // binary's files draw no picture
    return a;
}

// Installed memory in words ("16.0 GB"), or "?" when SDL cannot tell.
std::string installed_memory_text()
{
    const int mb = SDL_GetSystemRAM();
    return mb > 0 ? sieve::memory_text(double(mb) * 1048576.0) : std::string("?");
}

std::string fixed(double v, int d)
{
    char b[48];
    std::snprintf(b, sizeof b, "%.*f", d, v);
    return b;
}

} // namespace

// What the hallway's cache of units may take: ITEM MEMORY's share of installed memory (a quarter at
// first), or the default when SDL cannot tell (Budget's, which is careful).
std::atomic<int> g_item_memory_pct{25};
static double cache_bytes_here()
{
    const int mb = SDL_GetSystemRAM();
    return mb > 0 ? double(mb) * 1048576.0 * (g_item_memory_pct / 100.0) : Budget{}.cache_bytes;
}

void set_item_memory_share(int percent) { g_item_memory_pct = std::clamp(percent, 5, 90); }

Budget machine_budget()
{
    // The measurement is taken once: a unit of a line of 27 symbols, 40,000 long (about 190,000
    // bits, a long page), turned from its address into its digits. The first turn builds the
    // powers the conversion keeps for that base, so the second is the one that is timed.
    // A quarter of that length too, for how the time grows (the quickest of three each: a
    // scheduler's pause is not the cost).
    struct Measured
    {
        double bits, ms, growth;
    };
    static const Measured measured = [] {
        sieve::cli::timings::Scope timed("menu.budget.measure");
        const uint32_t base = 27;
        auto time_at = [&](uint32_t length) {
            sieve::BigUint v = sieve::BigUint::pow(base, length);
            v -= sieve::BigUint(1);
            (void)v.to_digits(base, length);
            double best = 1e300;
            for (int i = 0; i < 3; ++i)
            {
                const auto t0 = std::chrono::steady_clock::now();
                (void)v.to_digits(base, length);
                best = std::min(best, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
            }
            return std::make_pair(double(v.bit_length()), std::max(best, 0.01));
        };
        const auto [bits, ms] = time_at(40000);
        const auto [qbits, qms] = time_at(10000);
        return Measured{bits, ms, std::clamp(std::log(ms / qms) / std::log(bits / qbits), 1.0, 2.5)};
    }();
    // And the binary line's: a file of 64 KB turned from its place on the line into its bytes,
    // in scrambled order, which is the slower of the two (the keyed shuffle over a number of
    // half a million bits costs several times the conversion; both grow in proportion).
    static const std::pair<double, double> measured_binary = [] {
        sieve::cli::timings::Scope timed("menu.budget.measure_binary");
        const sieve::BinarySpace bs(65536, "sieve");
        sieve::BigUint top = bs.size();
        top -= sieve::BigUint(1);
        const auto t0 = std::chrono::steady_clock::now();
        (void)bs.bytes_at(top, sieve::AddressMode::Scrambled);
        const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        return std::make_pair(double(top.bit_length()), std::max(ms, 0.01));
    }();
    Budget b;
    b.ref_bits = measured.bits;
    b.ref_ms = measured.ms;
    b.growth = measured.growth;
    b.binary_ref_bits = measured_binary.first;
    b.binary_ref_ms = measured_binary.second;
    b.ms_per_unit_at_limit = g_time_budget;
    b.bits = std::min(too_large_bits(), b.ref_bits * std::pow(b.ms_per_unit_at_limit / b.ref_ms, 1.0 / b.growth));
    b.cache_bytes = cache_bytes_here();
    return b;
}

double unit_ms(const Budget& b, double bits) { return b.ref_ms * std::pow(std::max(bits, 1.0) / b.ref_bits, b.growth); }

void set_time_budget(double ms) { g_time_budget = std::max(1.0, ms); }

namespace {
std::atomic<int> g_view_rooms{7}, g_picture_rooms{1};
} // namespace

void set_view(int rooms, int pictures)
{
    g_view_rooms = std::clamp(rooms, 2, 64);
    // No pictures past the rooms drawn: those are not seen.
    g_picture_rooms = std::clamp(pictures, 0, std::min(8, g_view_rooms.load()));
}
int view_rooms() { return g_view_rooms; }
int tiles_kept() { return 2 * view_rooms() + 1; }
int picture_rooms() { return g_picture_rooms; }

size_t cached_units() { return size_t(tiles_kept() + 2) * sieve::books_per_tile(); }

double too_large_bits() { return cache_bytes_here() * 8.0 / 3.0; }

uint64_t large_file_bytes()
{
    const Budget b = machine_budget();
    const double ms_per_byte = b.binary_ref_ms / (b.binary_ref_bits / 8.0);
    return uint64_t(std::max(1.0, b.ms_per_unit_at_limit / 4.0 / std::max(ms_per_byte, 1e-12)));
}

// ---------------------------------------------------------------- settings

Settings Settings::from_args(const sieve::cli::Args& a)
{
    Settings s;
    s.start_line = a.get("line", s.start_line);
    if (s.start_line == "pages") s.start_line = "text"; // another name for the text line
    s.mode = a.get("mode", s.mode);
    s.key = a.get("key", s.key);
    s.length = parse_u32(a, "length", s.length);
    s.alphabet = a.get("alphabet", s.alphabet);
    s.canon = a.get("canon", s.canon);
    s.model = a.get("model") != "none";
    s.image_w = parse_u32(a, "image-width", s.image_w);
    s.image_h = parse_u32(a, "image-height", s.image_h);
    s.image_palette = a.get("image-palette", s.image_palette);
    s.notes = parse_u32(a, "notes", s.notes);
    s.note_set = a.get("note-set", s.note_set);
    s.note_low = a.get("note-low", s.note_low);
    s.note_high = a.get("note-high", s.note_high);
    s.note_durations = a.get("note-durations", s.note_durations);
    s.voices = parse_u32(a, "voices", s.voices);
    s.n3_low = a.get("notes3-low", s.n3_low);
    s.n3_high = a.get("notes3-high", s.n3_high);
    s.n3_instruments = a.get("notes3-instruments", s.n3_instruments);
    s.n3_tpq = parse_u32(a, "notes3-tpq", s.n3_tpq);
    s.n3_longest = parse_u32(a, "notes3-longest", s.n3_longest);
    s.n3_levels = parse_u32(a, "notes3-levels", s.n3_levels);
    s.n3_voices = parse_u32(a, "notes3-voices", s.n3_voices);
    s.n3_tempo = parse_u32(a, "notes3-tempo", s.n3_tempo);
    s.samples = parse_u32(a, "samples", s.samples);
    s.pcm_rate = parse_u32(a, "pcm-rate", s.pcm_rate);
    s.pcm_bits = parse_u32(a, "pcm-bits", s.pcm_bits);
    s.pcm_channels = parse_u32(a, "pcm-channels", s.pcm_channels);
    s.video_w = parse_u32(a, "video-width", s.video_w);
    s.video_h = parse_u32(a, "video-height", s.video_h);
    s.frames = parse_u32(a, "video-frames", s.frames);
    s.video_palette = a.get("video-palette", s.video_palette);
    s.book_pages = parse_u32(a, "book-pages", s.book_pages);
    s.track_units = parse_u32(a, "track-units", s.track_units);
    s.movie_units = parse_u32(a, "movie-units", s.movie_units);
    s.model_vertices = parse_u32(a, "vertices", s.model_vertices);
    s.model_faces = parse_u32(a, "faces", s.model_faces);
    s.model_coords = parse_u32(a, "coords", s.model_coords);
    s.model_tile = parse_u32(a, "model-tile", s.model_tile);
    s.items_per_wall = parse_u32(a, "items-per-wall", s.items_per_wall);
    if (a.has("title-length")) s.title_length = a.get_u32("title-length", s.title_length); // 0 is allowed: no titles
    s.letters_px = parse_u32(a, "item-letters", s.letters_px);
    if (a.has("close-up")) s.closeup_px = closeup_setting(a.get("close-up", ""), s.closeup_px); // 0 is allowed: off
    s.limits_focus = a.get("limits-focus", s.limits_focus);
    if (s.limits_focus == "pages") s.limits_focus = "text";
    s.binary_bytes = parse_u32(a, "binary-length", s.binary_bytes);
    s.ffmpeg = a.get("ffmpeg", s.ffmpeg);
    s.video_fps = parse_u32(a, "video-fps", s.video_fps);
    return s;
}

void Settings::apply(sieve::cli::Args& a) const
{
    a.opts["line"] = start_line;
    a.opts["mode"] = mode;
    a.opts["key"] = key;
    a.opts["length"] = std::to_string(length);
    a.opts["alphabet"] = alphabet;
    a.opts["canon"] = canon;
    if (model) a.opts.erase("model");
    else a.opts["model"] = "none";
    a.opts["image-width"] = std::to_string(image_w);
    a.opts["image-height"] = std::to_string(image_h);
    a.opts["image-palette"] = image_palette;
    a.opts["notes"] = std::to_string(notes);
    a.opts["note-set"] = note_set;
    a.opts["note-low"] = note_low;
    a.opts["note-high"] = note_high;
    a.opts["note-durations"] = note_durations;
    a.opts["voices"] = std::to_string(voices);
    a.opts["notes3-low"] = n3_low;
    a.opts["notes3-high"] = n3_high;
    a.opts["notes3-instruments"] = n3_instruments;
    a.opts["notes3-tpq"] = std::to_string(n3_tpq);
    a.opts["notes3-longest"] = std::to_string(n3_longest);
    a.opts["notes3-levels"] = std::to_string(n3_levels);
    a.opts["notes3-voices"] = std::to_string(n3_voices);
    a.opts["notes3-tempo"] = std::to_string(n3_tempo);
    a.opts["samples"] = std::to_string(samples);
    a.opts["pcm-rate"] = std::to_string(pcm_rate);
    a.opts["pcm-bits"] = std::to_string(pcm_bits);
    a.opts["pcm-channels"] = std::to_string(pcm_channels);
    a.opts["video-width"] = std::to_string(video_w);
    a.opts["video-height"] = std::to_string(video_h);
    a.opts["video-frames"] = std::to_string(frames);
    a.opts["video-palette"] = video_palette;
    a.opts["book-pages"] = std::to_string(book_pages);
    a.opts["track-units"] = std::to_string(track_units);
    a.opts["movie-units"] = std::to_string(movie_units);
    a.opts["vertices"] = std::to_string(model_vertices);
    a.opts["faces"] = std::to_string(model_faces);
    a.opts["coords"] = std::to_string(model_coords);
    a.opts["model-tile"] = std::to_string(model_tile);
    a.opts["items-per-wall"] = std::to_string(items_per_wall);
    a.opts["title-length"] = std::to_string(title_length);
    a.opts["item-letters"] = std::to_string(letters_px);
    a.opts["close-up"] = closeup_text(closeup_px);
    a.opts["binary-length"] = std::to_string(binary_bytes);
    a.opts["limits-focus"] = limits_focus;
    a.opts["video-fps"] = std::to_string(video_fps);
    if (ffmpeg.empty()) a.opts.erase("ffmpeg");
    else a.opts["ffmpeg"] = ffmpeg;
}

// b^e mod the tile (books_per_tile), by squaring: how many slots a count leaves in its last tile.
static uint64_t tile_pow(uint64_t b, uint64_t e)
{
    const uint64_t per = sieve::books_per_tile();
    uint64_t m = 1;
    for (b %= per; e; e >>= 1, b = b * b % per)
        if (e & 1) m = m * b % per;
    return m;
}

LineSize line_size(uint32_t base, uint64_t length)
{
    LineSize z;
    z.bits = double(length) * std::log2(double(base));
    // base^length mod the tile; padding fills the last tile.
    const uint64_t m = tile_pow(base, length);
    const bool tiny = z.bits < 7; // fewer units than one tile
    uint64_t exact = 1;
    if (tiny)
        for (uint64_t i = 0; i < length; ++i) exact *= base;
    z.padding = tiny ? uint32_t(sieve::books_per_tile() - exact) : uint32_t((sieve::books_per_tile() - m) % sieve::books_per_tile());
    const double log10 = z.bits * std::log10(2.0);
    z.units = std::to_string(base) + "^" + std::to_string(length) + (log10 < 15 ? " = " + fixed(std::pow(10.0, log10), 0) : " = ~10^" + fixed(log10, 1));
    return z;
}

// ---------------------------------------------------------------- menu

std::array<LineSize, kLines> Menu::line_sizes() const
{
    const LineSize page = line_size(alphabet_size(s_.alphabet), s_.length);
    const LineSize image = line_size(palette_size(s_.image_palette), uint64_t(s_.image_w) * s_.image_h);
    // Books: a cover, a title and book_pages pages, so |cover| * |page|^(pages + 1) books. The
    // padding follows from both factors mod 128.
    LineSize books;
    const uint64_t parts = uint64_t(s_.book_pages) + 1;
    books.bits = image.bits + double(parts) * page.bits;
    {
        const uint64_t m = tile_pow(palette_size(s_.image_palette), uint64_t(s_.image_w) * s_.image_h) *
                           tile_pow(alphabet_size(s_.alphabet), parts * s_.length) % sieve::books_per_tile();
        books.padding = uint32_t((sieve::books_per_tile() - m) % sieve::books_per_tile()); // books.bits >= 7 always
        const double log10 = books.bits * std::log10(2.0);
        books.units = "cov*pg^" + std::to_string(parts) + " = ~10^" + fixed(log10, 1);
    }
    // Models: C^(3V) coordinates times V^(3F) face indices, so the bits add and the padding is
    // the product of the two mod 128.
    LineSize models;
    {
        const LineSize coords = line_size(s_.model_coords, 3ull * s_.model_vertices);
        const LineSize faces = line_size(s_.model_vertices, 3ull * s_.model_faces);
        models.bits = coords.bits + faces.bits;
        const uint64_t m = (sieve::books_per_tile() - coords.padding) % sieve::books_per_tile() *
                           ((sieve::books_per_tile() - faces.padding) % sieve::books_per_tile()) % sieve::books_per_tile();
        models.padding = uint32_t((sieve::books_per_tile() - m) % sieve::books_per_tile());
        const double log10 = models.bits * std::log10(2.0);
        models.units = std::to_string(s_.model_coords) + "^" + std::to_string(3ull * s_.model_vertices) + "*" +
                       std::to_string(s_.model_vertices) + "^" + std::to_string(3ull * s_.model_faces) +
                       (log10 < 15 ? " = " + fixed(std::pow(10.0, log10), 0) : " = ~10^" + fixed(log10, 1));
    }
    // Every line but books is titled, and audio and video carry a cover from the image line
    // (SPECIFICATIONS §11): a titled line is |cover| * |title| * |content| units, so the bits add
    // and the empty slots follow from the product of the three mod the tile.
    const LineSize title = line_size(alphabet_size(s_.alphabet), s_.title_length);
    auto titled = [&](LineSize z, bool with_cover) {
        const uint64_t per = sieve::books_per_tile();
        const auto rem = [per](const LineSize& x) { return (per - x.padding) % per; }; // the count mod the tile
        uint64_t m = rem(z) * rem(title) % per;
        if (with_cover) m = m * rem(image) % per;
        z.bits += title.bits + (with_cover ? image.bits : 0.0);
        z.padding = uint32_t((per - m) % per);
        const double log10 = z.bits * std::log10(2.0);
        z.units = std::string(with_cover ? "cov*" : "") + "title*" + z.units.substr(0, z.units.find(" = ")) + " = ~10^" + fixed(log10, 1);
        return z;
    };
    // Binary: every file of 0..N bytes, (256^(N+1) - 1) / 255 of them (SPECIFICATIONS §12.1), a
    // hair over 8N bits; titled, without a cover. Its files stand on one wall, half a tile's slots,
    // and since 256 is a multiple of every wall's slot count the count is 1 mod the wall, so the
    // empty slots at the end of a loop follow from the title alone.
    LineSize binary;
    {
        const uint64_t per = sieve::books_per_tile(), wall = per / 2;
        const auto rem = [per](const LineSize& x) { return (per - x.padding) % per; };
        binary.bits = 8.0 * double(s_.binary_bytes) + std::log2(256.0 / 255.0) + title.bits; // a title, no cover (its kind is read from its bytes)
        const uint64_t m = rem(title) % wall;
        binary.padding = uint32_t((wall - m) % wall);
        binary.units = "title*files<=" + std::to_string(s_.binary_bytes) + "B = ~10^" + fixed(binary.bits * std::log10(2.0), 1);
    }
    const auto [audio_base, audio_positions] = audio_shape(s_);
    const LineSize audio = line_size(audio_base, audio_positions);
    const LineSize video = line_size(palette_size(s_.video_palette), positions(s_.video_w, s_.video_h, s_.frames));
    // Tracks and movies (composition-v1): a cover, a title and n units, |cover| * |title| * |unit|^n,
    // so the bits add and the empty slots follow from the product mod the tile.
    auto composed = [&](const LineSize& unit, uint32_t n, const char* name) {
        const uint64_t per = sieve::books_per_tile();
        const auto rem = [per](const LineSize& x) { return (per - x.padding) % per; };
        const uint64_t m = rem(image) * rem(title) % per * tile_pow(rem(unit), n) % per;
        LineSize z;
        z.bits = image.bits + title.bits + double(n) * unit.bits;
        z.padding = uint32_t((per - m) % per);
        z.units = std::string("cov*title*") + name + "^" + std::to_string(n) + " = ~10^" + fixed(z.bits * std::log10(2.0), 1);
        return z;
    };
    std::array<LineSize, kLines> out;
    out[size_t(line_of(Media::Pages))] = titled(page, false);
    out[size_t(line_of(Media::Image))] = titled(image, false);
    out[size_t(line_of(Media::Audio))] = titled(audio, true);
    out[size_t(line_of(Media::Video))] = titled(video, true);
    out[size_t(line_of(Media::Tracks))] = composed(audio, s_.track_units, "au");
    out[size_t(line_of(Media::Movies))] = composed(video, s_.movie_units, "vid");
    out[size_t(kBooksLine)] = books;
    out[size_t(kModelsLine)] = titled(models, false);
    out[size_t(kBinaryLine)] = binary;
    // A unit has at most 2^32 - 1 positions: a larger picture cannot be opened at all.
    const uint64_t kMaxPositions = 0xFFFFFFFFull;
    if (uint64_t(s_.image_w) * s_.image_h > kMaxPositions) out[size_t(line_of(Media::Image))].bits = out[size_t(kBooksLine)].bits = HUGE_VAL;
    if (positions(s_.video_w, s_.video_h, s_.frames) > kMaxPositions) out[size_t(line_of(Media::Video))].bits = HUGE_VAL;
    // A track or movie is shown and played as one unit of its units joined, which has the same limit.
    if (uint64_t(audio_positions) * s_.track_units > kMaxPositions) out[size_t(line_of(Media::Tracks))].bits = HUGE_VAL;
    if (positions(s_.video_w, s_.video_h, s_.frames) * s_.movie_units > kMaxPositions) out[size_t(line_of(Media::Movies))].bits = HUGE_VAL;
    return out;
}

// (menu_ink(), the colour a line's name is written in on black: dimensions.hpp.)

bool Menu::too_large() const
{
    for (const auto& z : line_sizes())
        if (z.bits > too_large_bits()) return true;
    return false;
}

// Which line, if any, is beyond what this machine can open. -1 if none.
int Menu::over_budget() const
{
    const Budget b = machine_budget();
    for (int i = 0; i < kLines; ++i)
        if (!over_budget_line(i, b)) return i;
    return -1;
}

// What line i's cache of units would take: each unit its positions at four bytes and its address
// held twice, as a number (an eighth of a byte a bit) and as hex (a quarter), for as many units
// as the hallway keeps around you (the tiles it keeps: tiles_kept, the view distance).
double Menu::line_cache_bytes(int i) const
{
    const auto sizes = line_sizes();
    // Each unit's positions: its content, its title and, on audio and video, its cover.
    const uint64_t cover = uint64_t(s_.image_w) * s_.image_h, t = s_.title_length;
    // (The binary line's file is bytes, a quarter of a position each.)
    uint64_t pos = 0;
    switch (kDimensions[i].media)
    {
    case Media::Pages: pos = s_.length + t; break;
    case Media::Image: pos = cover + t; break;
    case Media::Audio: pos = audio_shape(s_).second + t + cover; break;
    case Media::Video: pos = positions(s_.video_w, s_.video_h, s_.frames) + t + cover; break;
    case Media::Books: pos = uint64_t(s_.book_pages + 1) * s_.length + cover; break;
    case Media::Models: pos = 3ull * s_.model_vertices + 3ull * s_.model_faces + t; break;
    case Media::Binary: pos = (uint64_t(s_.binary_bytes) + 3) / 4 + t; break;
    case Media::Tracks: pos = audio_shape(s_).second * uint64_t(s_.track_units) + t + cover; break;
    case Media::Movies: pos = positions(s_.video_w, s_.video_h, s_.frames) * uint64_t(s_.movie_units) + t + cover; break;
    }
    const double units = double(tiles_kept()) * double(s_.items_per_wall);
    // A binary file is kept lazily (hallway.hpp, Book::is_file): only its place on the line, a
    // number as long as the file, its title, and its first sixteen bytes; its bytes and hex only for the one
    // looked at. And only half a tile's slots hold files.
    if (i == kBinaryLine) return units / 2 * (double(s_.binary_bytes) + 4.0 * double(s_.title_length) + 16.0 + 256.0) + 3.0 * double(s_.binary_bytes);
    return units * (4.0 * double(pos) + 0.375 * sizes[size_t(i)].bits + 256.0);
}

double Menu::graphics_mb_needed() const { return double(model_cache_mb()) + closeup_mb() + world_graphics_mb(); }

// The world's share of the graphics memory, from what it makes (gpu_memory.hpp counts it): the
// renderer's own frames (the one shown, the one being drawn and one queued, each the window's
// size), Real Graphics' frame (the window's size) and the doorways' portal noise (a kPortalGrain-th
// of it each way) when they are on, a sign over the doors for each line, and the item in hand's
// picture (two, at an image's or a video frame's pixels). Or what is held already, where that is
// more (in the hallway: the letters' atlases and whatever else it has made). It was 512 MB, taken
// for granted: at 1920 x 1080 with everything on it comes to 37 MB.
double Menu::world_graphics_mb() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    const double screen = double(w) * double(h) * 4.0;
    double planned = 0;
    if (app_ && app_->real_graphics) planned += screen;
    if (app_ && app_->door_portals) planned += screen / double(kPortalGrain * kPortalGrain);
    planned += double(kLines) * kSignPxW * kSignPxH * 4.0;
    const uint64_t picture = std::max(uint64_t(s_.image_w) * s_.image_h, uint64_t(s_.video_w) * s_.video_h);
    planned += 2.0 * double(picture) * 3.0;
    return (3.0 * screen + std::max(planned, gpu::bytes(gpu::Use::World))) / 1048576.0;
}

// The width line i's displays are drawn at, as the hallway works it out (display.hpp), taking
// the tallest item's shape so that the figures are never under what it will use. Lines in
// setup order: 0 pages, 1 image, 2 audio, 3 video, 4 books, 5 models.
int Menu::display_px_line(int i, bool cache_held) const
{
    const double title = double(s_.title_length);
    // (Binary carries a title, and its kind drawn large below it, which the letters do not size.)
    const DisplayText t = i == line_of(Media::Pages) ? display_text_pages(double(s_.length), title)
                          : i == kBooksLine ? display_text_books(double(s_.length))
                                   : display_text_titled(title);
    return display_px(int(s_.model_tile), int(s_.letters_px), tallest_face(), t, cache_held ? widest_px() : texture_px_);
}

// Every picture of the rooms with pictures (yours and the picture distance either side) at the
// widest the display cache holds them all, as the hallway works it out (item_faces.cpp).
int Menu::widest_px() const
{
    const double pictures = double(2 * picture_rooms() + 1) * double(s_.items_per_wall);
    return widest_display_px(double(model_cache_mb()) * 1048576.0, pictures, tallest_face(), texture_px_);
}

double Menu::display_mb(bool cache_held) const
{
    int px = 0;
    for (int i = 0; i < kLines; ++i) px = std::max(px, display_px_line(i, cache_held));
    return double(px) * (double(px) * tallest_face()) * 4 / 1048576.0;
}

// The close-ups, as the hallway works them out (item_faces.cpp closeup_px and sharp_max): as wide
// as the setting, or the screen's, and as many as the graphics memory holds beside the world and
// the display cache.
int Menu::closeup_px() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    return closeup_width(s_.closeup_px, w, texture_px_);
}

double Menu::closeup_one_mb() const
{
    const double w = double(closeup_px());
    return w * std::ceil(w * tallest_face()) * 4 / 1048576.0;
}

size_t Menu::closeup_count() const
{
    const double gfx = app_ ? double(app_->graphics_memory_gb) * 1024.0 : 4096.0;
    const double room = (gfx - world_graphics_mb() - double(model_cache_mb())) * 1048576.0;
    return ::hallway::closeup_count(room, closeup_px(), tallest_face(), s_.items_per_wall);
}

// What they take: all of them at their widest, or what the screen can show of them
// (closeup_bytes_most), whichever is less. 63 MB at 1920 x 1080, 253 MB at 4K.
double Menu::closeup_mb() const
{
    int w = 0, h = 0;
    SDL_GetRenderOutputSize(r_, &w, &h);
    return std::min(double(closeup_count()) * closeup_one_mb(), closeup_bytes_most(w, h) / 1048576.0);
}

bool Menu::graphics_over() const { return app_ && graphics_mb_needed() > double(app_->graphics_memory_gb) * 1024.0; }

void Menu::save_app() const
{
    if (app_ && !app_path_.empty()) app_->save(app_path_); // a read-only folder keeps it for this session
}

// Find my limits: set every line to the largest shape this machine can open.
//
// It costs nothing to work out -- a line's size in bits and the length of one of its units are
// both closed-form from its shape -- so this doubles each shape until it is over budget and then
// walks it back, rather than generating anything. The books line is last because it is made of
// the other two: its size follows from the pages and image lines, so it takes whatever pages per
// book is left over, and if even one page will not fit the pages line comes down until it does.
void Menu::find_limits()
{
    const Budget b = machine_budget();
    auto fits = [&](int i) {
        return over_budget_line(i, b);
    };
    const int pages = line_of(Media::Pages), image = line_of(Media::Image), audio = line_of(Media::Audio);
    const int video = line_of(Media::Video), tracks = line_of(Media::Tracks), movies = line_of(Media::Movies);
    // Grows v, doubling then in steps, for as long as `ok` holds.
    auto grow_while = [&](uint32_t& v, const std::function<bool()>& ok) {
        v = 1;
        if (!ok()) return; // even the smallest is too much on this machine
        while (v < (UINT32_MAX >> 1))
        {
            const uint32_t was = v;
            v *= 2;
            if (!ok()) { v = was; break; }
        }
        // Then a few linear steps, so the answer is not always a power of two.
        const uint32_t step = std::max(1u, v / 16);
        while (v < UINT32_MAX - step)
        {
            const uint32_t was = v;
            v += step;
            if (!ok()) { v = was; break; }
        }
    };
    auto grow = [&](uint32_t& v, int line) { grow_while(v, [&] { return fits(line); }); };
    // The focus (GLOBAL): every line, or one of them, the rest left as they are. Each line is
    // judged on its own against the budget -- the memory bar is the largest line's cache, not
    // all of them added, and the time bar the slowest line -- so a line grown on its own reaches
    // the same largest shape it would with every other line at its smallest. The one exception
    // is pages and image, which the books line is made of (below).
    const std::string& focus = s_.limits_focus;
    auto on = [&](const char* line) { return focus == "all" || focus == line; };
    // Growing a pair together, a step each in turn, each kept only while `ok` holds.
    auto grow_pair = [](uint32_t& a, uint32_t& b2, const std::function<bool()>& ok_a, const std::function<bool()>& ok_b,
                        bool same) {
        a = b2 = 1;
        for (bool moved = true; moved;)
        {
            moved = false;
            const uint32_t was_a = a;
            a = was_a + std::max(1u, was_a / 8);
            if (same) b2 = a;
            if (!ok_a()) { a = was_a; if (same) b2 = was_a; }
            else moved = true;
            if (same) continue;
            const uint32_t was_b = b2;
            b2 = was_b + std::max(1u, was_b / 8);
            if (!ok_b()) b2 = was_b;
            else moved = true;
        }
    };
    if (focus == "all" || focus == "books")
    {
        // The pages and image lines are grown together, because the books line is made of both
        // and has to fit as well: a book is a cover from the image line, a title and its pages
        // from the pages line. Growing one to its own limit first would leave the other with
        // nothing, so they take turns, and each step is only kept if its own line and the books
        // line both still fit.
        s_.book_pages = 1;
        s_.length = s_.image_w = s_.image_h = 1;
        for (bool moved = true; moved;)
        {
            moved = false;
            const uint32_t was_len = s_.length;
            s_.length = was_len + std::max(1u, was_len / 8);
            if (!fits(pages) || !fits(kBooksLine)) s_.length = was_len;
            else moved = true;
            const uint32_t was_px = s_.image_w;
            s_.image_w = s_.image_h = was_px + std::max(1u, was_px / 8);
            if (!fits(image) || !fits(kBooksLine)) s_.image_w = s_.image_h = was_px;
            else moved = true;
        }
        // Then as many pages to a book as what is left allows.
        while (s_.book_pages < 0xFFFF)
        {
            const uint32_t was = s_.book_pages;
            ++s_.book_pages;
            if (!fits(kBooksLine)) { s_.book_pages = was; break; }
        }
    }
    // Focused on pages or on image alone. The books line is made of both -- a cover from the
    // image line, a title page and pages from the pages line -- so neither can grow to its own
    // limit without making every book too large to open: grown alone, pages took the books line
    // to ten times the time allowed. So books drops to its smallest, one page, and the line
    // grows for as long as it and a one-page book both still open in time.
    if (focus == "text" || focus == "image") s_.book_pages = 1;
    if (focus == "text") grow_pair(s_.length, s_.length, [&] { return fits(pages) && fits(kBooksLine); }, [&] { return true; }, true);
    if (focus == "image") grow_pair(s_.image_w, s_.image_h, [&] { return fits(image) && fits(kBooksLine); }, [&] { return true; }, true);
    // Audio and video, as pages and books: tracks and movies are made of their units, so the line
    // grows only while a one-unit track (movie) still fits too, and then the track (movie) takes as
    // many units as what is left allows.
    if (on("audio") || on("tracks")) s_.track_units = 1;
    if (on("audio")) grow_while(is_pcm(s_) ? s_.samples : s_.notes, [&] { return fits(audio) && fits(tracks); });
    if (on("tracks")) grow(s_.track_units, tracks);
    if (on("video") || on("movies")) s_.movie_units = 1;
    if (on("video")) grow_while(s_.frames, [&] { return fits(video) && fits(movies); });
    if (on("movies")) grow(s_.movie_units, movies);
    // Vertices and faces grow together, so a mesh gets both rather than all of one.
    if (on("models")) grow_pair(s_.model_vertices, s_.model_faces, [&] { return fits(kModelsLine); }, [&] { return fits(kModelsLine); }, false);
    if (on("binary")) grow(s_.binary_bytes, kBinaryLine);
    // And the model image cache: the rooms with pictures of their own (yours and the picture
    // distance either side) at the chosen size, or as many as the graphics memory has room for
    // beside the world.
    if (app_)
    {
        const double rooms = double(2 * picture_rooms() + 1) * s_.items_per_wall * display_mb(false);
        const double room = double(app_->graphics_memory_gb) * 1024.0 - world_graphics_mb() - closeup_one_mb(); // room for one close-up at least
        const int mb = int(std::ceil(std::min(rooms, room) / 8.0)) * 8;
        app_->model_cache_mb = std::max(8, mb);
        save_app();
    }
}

// Does line i fit inside `b` as the settings stand? Its address has to open in time and its
// cache of units has to fit in memory (the infinities of a shape too large to hold fail both).
bool Menu::over_budget_line(int i, const Budget& b) const
{
    const double bits = line_sizes()[size_t(i)].bits;
    const bool in_time = i == kBinaryLine ? line_ms(i, b) <= b.ms_per_unit_at_limit : bits <= b.bits;
    return in_time && line_cache_bytes(i) <= b.cache_bytes;
}

// How long a unit of line i takes to open: the base conversion's growth for the six, and in
// proportion to the length for binary, whose conversions are hex (binaryspace.hpp).
double Menu::line_ms(int i, const Budget& b) const
{
    const double bits = line_sizes()[size_t(i)].bits;
    if (i == kBinaryLine) return b.binary_ref_ms * std::max(bits, 1.0) / b.binary_ref_bits;
    return unit_ms(b, bits);
}

void Menu::reset_settings()
{
    const std::string key = s_.key; // the key names the shuffle, not the shape: it is kept
    s_ = Settings{};
    s_.key = key;
    // The picture cache is kept in the app settings, but finding the limits sets it, so resetting
    // puts it back too: otherwise reset would undo everything find-my-limits did but that.
    if (app_)
    {
        app_->model_cache_mb = AppSettings{}.model_cache_mb;
        save_app();
    }
}

Menu::Menu(SDL_Window* window, SDL_Renderer* renderer, Settings settings, sieve::cli::FilterConfig filters, std::string filters_path,
           AppSettings* app, std::filesystem::path app_path)
    : window_(window), r_(renderer), s_(std::move(settings)), cfg_(std::move(filters)), cfg_path_(std::move(filters_path)),
      app_(app), app_path_(std::move(app_path))
{
    texture_px_ = gpu::max_texture_px(r_);
}

// The settings rows, then ENTER THE HALLWAY last. adjust() switches on the same numbering, and
// the rows are built in the same order in render(); kEnterRow keeps all three in step.
int Menu::row_count() const { return kEnterRow + 1; }

void Menu::adjust(int dir, int step)
{
    // No upper limit but what a setting can hold: the state spaces are meant to grow without end.
    auto num = [&](uint32_t& v) {
        int64_t n = int64_t(v);
        if (step == 0) n = dir > 0 ? n * 2 : n / 2; // PgUp/PgDn: double or halve (towards powers of two)
        else n += int64_t(dir) * step;
        v = uint32_t(std::clamp<int64_t>(n, 1, int64_t(UINT32_MAX)));
    };
    switch (row_)
    {
    // GLOBAL: what belongs to the corridor rather than to any one line.
    case 0: s_.start_line = cycle(kStartLines, s_.start_line, dir); break;
    case 1: s_.mode = cycle(kModes, s_.mode, dir); break;
    case 2: break; // key: typed
    // Items per wall: the two values that use a whole byte well (see setup.items_per_wall).
    case 3: s_.items_per_wall = s_.items_per_wall == 128 ? 256u : 128u; break;
    // Angle precision: decimal places on the compass, 0 to kMaxAngleDecimals. Kept with the
    // application's settings so that it is saved.
    case kAngleRow:
        if (app_)
        {
            app_->angle_decimals = std::clamp(app_->angle_decimals + dir, 0, kMaxAngleDecimals);
            save_app();
        }
        break;
    // Title length: how long every line's titles are, in characters.
    case kLettersRow: s_.letters_px = uint32_t(std::clamp(int(s_.letters_px) + dir, 1, 64)); break;
    case kFocusRow: s_.limits_focus = cycle(kFocuses, s_.limits_focus, dir); break;
    // The filter memory: what one count's tables may take. 256 MB a step (Shift and Ctrl ten and a
    // hundred times that), PgUp/PgDn double and halve. Kept with the application's settings, and
    // only that: nothing is counted again until X (memory_pending), so it can be stepped freely.
    case kFilterMemoryRow:
        if (app_)
        {
            const int64_t mb = app_->filter_memory_mb;
            const int64_t to = step == 0 ? (dir > 0 ? mb * 2 : mb / 2) : mb + int64_t(dir) * 256 * step;
            app_->filter_memory_mb = int(std::clamp<int64_t>(to, 64, 1048576));
            save_app();
        }
        break;
    // The counting memory and the merge cache: percentages, 5 a step (Shift and Ctrl ten and a
    // hundred times that), PgUp/PgDn 25. They change no count, only how many run at once and what
    // is kept between them, so they take effect at once, with nothing counted again.
    case kItemMemoryRow:
        if (app_)
        {
            app_->item_memory_pct = std::clamp(app_->item_memory_pct + dir * (step == 0 ? 25 : 5 * step), 5, 90);
            set_item_memory_share(app_->item_memory_pct);
            save_app();
        }
        break;
    case kCountingMemoryRow:
    case kMergeCacheRow:
        if (app_)
        {
            const bool counting = row_ == kCountingMemoryRow;
            int& pct = counting ? app_->counting_memory_pct : app_->merge_cache_pct;
            pct = std::clamp(pct + dir * (step == 0 ? 25 : 5 * step), counting ? 5 : 0, 100);
            if (counting) set_counting_share(pct);
            else sieve::set_merge_cache_share(pct / 100.0);
            save_app();
        }
        break;
    // The time budget: 5 ms a step (Shift and Ctrl ten and a hundred times that), PgUp/PgDn double
    // and halve. The budget's bars follow it at once; what can rank follows it on X (time_pending),
    // as the filter memory does.
    case kTimeBudgetRow:
        if (app_)
        {
            const int64_t ms = app_->unit_time_ms;
            const int64_t to = step == 0 ? (dir > 0 ? ms * 2 : ms / 2) : ms + int64_t(dir) * 5 * step;
            app_->unit_time_ms = int(std::clamp<int64_t>(to, 5, 60000));
            set_time_budget(app_->unit_time_ms);
            save_app();
        }
        break;
    // Off, then 256, 512, 1024.
    case kCloseUpRow:
        // Off, then the screen's width, then the powers of two from 256 up to the renderer's widest.
        if (dir > 0)
            s_.closeup_px = s_.closeup_px == 0 ? kCloseUpScreen
                            : s_.closeup_px == kCloseUpScreen ? 256u
                                                              : std::min(uint32_t(texture_px_), s_.closeup_px * 2);
        else
            s_.closeup_px = s_.closeup_px == kCloseUpScreen ? 0u : s_.closeup_px <= 256 ? kCloseUpScreen : s_.closeup_px / 2;
        break;
    case kTitleRow:
    {
        // 0 is allowed here, unlike a line's length: no titles at all.
        const int64_t t = step == 0 ? (dir > 0 ? std::max<int64_t>(1, int64_t(s_.title_length) * 2) : s_.title_length / 2)
                                    : int64_t(s_.title_length) + int64_t(dir) * step;
        s_.title_length = uint32_t(std::clamp<int64_t>(t, 0, int64_t(UINT32_MAX)));
        break;
    }
    case kPagesRows: num(s_.length); break;
    case kPagesRows + 1: s_.alphabet = cycle(kAlphabets, s_.alphabet, dir); break;
    case kPagesRows + 2: s_.canon = cycle(kCanons, s_.canon, dir); break;
    case kPagesRows + 3: s_.model = !s_.model; break;
    case kImageRows: num(s_.image_w); break;
    case kImageRows + 1: num(s_.image_h); break;
    case kImageRows + 2: s_.image_palette = cycle(kPalettes, s_.image_palette, dir); break;
    case kAudioRow: num(is_pcm(s_) ? s_.samples : s_.notes); break;
    case kAudioRow + 1: s_.note_set = cycle(kNoteSets, s_.note_set, dir); break;
    // On the pcm set the four rows below are its sample rate, bits, (none) and channels.
    case kAudioRow + 2:
        if (is_notes3(s_))
        {
            // A semitone at a time (Shift: an octave), within MIDI 0..127 and at least an octave apart.
            const int by = dir * (step >= 10 ? 12 : 1);
            int lo = int(sieve::notes3_midi_of(s_.n3_low)), hi = int(sieve::notes3_midi_of(s_.n3_high));
            lo = std::clamp(lo + by, 0, hi - 11);
            s_.n3_low = sieve::notes3_name(uint32_t(lo));
            break;
        }
        if (is_pcm(s_))
        {
            // The standard rates in turn (PgUp/PgDn: double or halve); from a rate not among them,
            // the nearest in the direction asked.
            if (step == 0) num(s_.pcm_rate);
            else if (dir > 0)
            {
                auto it = std::upper_bound(kPcmRates.begin(), kPcmRates.end(), s_.pcm_rate);
                s_.pcm_rate = it == kPcmRates.end() ? kPcmRates.front() : *it;
            }
            else
            {
                auto it = std::lower_bound(kPcmRates.begin(), kPcmRates.end(), s_.pcm_rate);
                s_.pcm_rate = it == kPcmRates.begin() ? kPcmRates.back() : *(it - 1);
            }
            s_.pcm_rate = std::min<uint32_t>(s_.pcm_rate, 0x7FFFFFFFu);
            break;
        }
        [[fallthrough]];
    case kAudioRow + 3:
    {
        if (is_notes3(s_))
        {
            const int by = dir * (step >= 10 ? 12 : 1);
            int lo = int(sieve::notes3_midi_of(s_.n3_low)), hi = int(sieve::notes3_midi_of(s_.n3_high));
            hi = std::clamp(hi + by, lo + 11, 127);
            s_.n3_high = sieve::notes3_name(uint32_t(hi));
            break;
        }
        if (is_pcm(s_))
        {
            s_.pcm_bits = uint32_t(std::clamp(int(s_.pcm_bits) + dir, 1, int(sieve::kPcmMaxBits)));
            break;
        }
        // A semitone at a time (Shift: an octave), within C2..C7 and at least an octave apart.
        if (s_.note_set != "notes2") break;
        const int by = dir * (step >= 10 ? 12 : 1);
        int lo = int(sieve::note_midi_of(s_.note_low)), hi = int(sieve::note_midi_of(s_.note_high));
        if (row_ == kAudioRow + 2) lo = std::clamp(lo + by, int(sieve::kNoteLowest), hi - 11);
        else hi = std::clamp(hi + by, lo + 11, int(sieve::kNoteHighest));
        s_.note_low = sieve::note_name(uint32_t(lo));
        s_.note_high = sieve::note_name(uint32_t(hi));
        break;
    }
    case kAudioRow + 4:
        if (s_.note_set == "notes2") s_.note_durations = cycle(kNoteDurationPresets, s_.note_durations, dir);
        if (is_notes3(s_))
        {
            // The presets in turn; from lengths not among them, the first.
            auto it = std::find(kNotes3Lengths.begin(), kNotes3Lengths.end(), std::make_pair(s_.n3_tpq, s_.n3_longest));
            const int n = int(kNotes3Lengths.size()), i = it == kNotes3Lengths.end() ? (dir > 0 ? -1 : 0) : int(it - kNotes3Lengths.begin());
            std::tie(s_.n3_tpq, s_.n3_longest) = kNotes3Lengths[size_t(((i + dir) % n + n) % n)];
        }
        break;
    case kAudioRow + 5:
        if (s_.note_set == "notes2") s_.voices = uint32_t(std::clamp(int(s_.voices) + dir, 1, int(sieve::kMaxVoices)));
        if (is_notes3(s_)) s_.n3_voices = uint32_t(std::clamp(int(s_.n3_voices) + dir, 1, int(sieve::kNotes3MaxVoices)));
        if (is_pcm(s_))
        {
            num(s_.pcm_channels);
            s_.pcm_channels = std::min(s_.pcm_channels, sieve::kPcmMaxChannels);
        }
        break;
    case kVideoRows: num(s_.video_w); break;
    case kVideoRows + 1: num(s_.video_h); break;
    case kVideoRows + 2: num(s_.frames); break;
    case kVideoRows + 3: s_.video_palette = cycle(kPalettes, s_.video_palette, dir); break;
    case kBooksRow: num(s_.book_pages); break;
    case kTracksRow: num(s_.track_units); break;
    case kMoviesRow: num(s_.movie_units); break;
    case kModelsRows: num(s_.model_vertices); break;
    case kModelsRows + 1: num(s_.model_faces); break;
    // The coordinate grid must be a power of two, so it doubles and halves.
    case kBinaryRow: num(s_.binary_bytes); break;
    case kModelsRows + 2: s_.model_coords = std::clamp(dir > 0 ? s_.model_coords * 2 : s_.model_coords / 2, 2u, 4096u); break;
    // The display size is a power of two, so it doubles and halves like the grid.
    case kDisplaySizeRow: s_.model_tile = std::clamp(dir > 0 ? s_.model_tile * 2 : s_.model_tile / 2, 16u, uint32_t(texture_px_)); break;
    // The display cache, in steps of 8 MB, as far as the graphics memory allows.
    case kDisplayCacheRow:
        if (app_)
        {
            const int room = int(double(app_->graphics_memory_gb) * 1024.0 - world_graphics_mb() - closeup_one_mb());
            const int top = std::max(8, room);
            app_->model_cache_mb = std::clamp(app_->model_cache_mb + dir * 8, 8, top);
            save_app();
        }
        break;
    default: break;
    }
}

void Menu::press(SDL_Keycode key, SDL_Keymod mod)
{
    SDL_Event e{};
    e.type = SDL_EVENT_KEY_DOWN;
    e.key.key = key;
    e.key.mod = mod;
    bool done = false;
    Result r = Result::Enter;
    handle(e, done, r);
    // A scripted key waits, as a person would, for what it started (X weighing filters that clash).
    if (toggling_)
    {
        finish_filter_warmup();
        poll_toggle();
    }
}

void Menu::handle(const SDL_Event& event, bool& done, Result& result)
{
    // Mouse positions in the menu's own coordinates (it may be drawn scaled: see render).
    SDL_Event e = event;
    SDL_ConvertEventToRenderCoordinates(r_, &e);
    if (e.type == SDL_EVENT_QUIT) { done = true; result = Result::Quit; return; }
    auto inside = [](const SDL_FRect& r, float x, float y) { return x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h; };
    if (e.type == SDL_EVENT_MOUSE_WHEEL && alpha_open_)
    {
        const int last = std::max(0, int(alpha_rows().size()) - 1);
        arow_ = std::clamp(arow_ - int(e.wheel.y), 0, last);
        while (arow_ > 0 && alpha_rows()[size_t(arow_)].empty()) arow_ += e.wheel.y > 0 ? -1 : 1;
        arow_ = std::clamp(arow_, 0, last);
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && alpha_open_)
    {
        const float mx = e.button.x, my = e.button.y;
        if (!inside(box_, mx, my)) { alpha_open_ = false; return; }
        for (const auto& [rect, i] : arow_rects_)
            if (inside(rect, mx, my))
            {
                arow_ = i;
                alpha_choose(i);
                break;
            }
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL && overlay_ >= 0)
    {
        const int last = std::max(0, int(overlay_rows().size()) - 1);
        wheel_ += e.wheel.y; // touchpads send fractions of a notch
        const int notches = int(wheel_);
        wheel_ -= float(notches);
        oscroll_ = std::clamp(oscroll_ - notches, 0, last);
        orow_ = std::clamp(std::max(orow_, oscroll_), 0, last);
        return;
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN)
    {
        const float mx = e.button.x, my = e.button.y;
        if (overlay_ >= 0)
        {
            if (!inside(box_, mx, my)) { save_filters(); overlay_ = -1; return; }
            for (const auto& [r, i] : row_rects_)
                if (inside(r, mx, my))
                {
                    orow_ = i;
                    overlay_change(e.button.button == SDL_BUTTON_RIGHT ? -1 : 1, false);
                }
            return;
        }
        for (int c = 0; c < kLines + 1; ++c)
            if (inside(magnifier_[c], mx, my)) open_filters(overlay_of_column(c));
        return;
    }
    if (alpha_open_)
    {
        if (e.type == SDL_EVENT_KEY_DOWN) alpha_key(e.key.key);
        return;
    }
    if (overlay_ >= 0)
    {
        if (e.type == SDL_EVENT_KEY_DOWN) overlay_key(e.key.key, (e.key.mod & SDL_KMOD_SHIFT) != 0);
        return;
    }
    if (e.type == SDL_EVENT_TEXT_INPUT && row_ == 2)
    {
        for (const char* p = e.text.text; *p; ++p)
            if (s_.key.size() < 40 && (std::isalnum(static_cast<unsigned char>(*p)) || *p == '-' || *p == '_')) s_.key.push_back(*p);
        return;
    }
    if (e.type != SDL_EVENT_KEY_DOWN) return;
    const bool shift = (e.key.mod & SDL_KMOD_SHIFT) != 0, ctrl = (e.key.mod & SDL_KMOD_CTRL) != 0;
    const int step = ctrl ? 100 : shift ? 10 : 1;
    // "Enter again to go in anyway" holds only for the Enter straight after it.
    if (e.key.key != SDLK_RETURN && e.key.key != SDLK_KP_ENTER) go_anyway_armed_ = false;
    switch (e.key.key)
    {
    case SDLK_UP: row_ = (row_ + row_count() - 1) % row_count(); break;
    case SDLK_DOWN: row_ = (row_ + 1) % row_count(); break;
    case SDLK_LEFT: adjust(-1, step); break;
    case SDLK_RIGHT: adjust(1, step); break;
    case SDLK_PAGEUP: adjust(1, 0); break;
    case SDLK_PAGEDOWN: adjust(-1, 0); break;
    case SDLK_BACKSPACE:
        if (row_ == 2 && !s_.key.empty()) s_.key.pop_back();
        break;
    case SDLK_A:
        // The alphabet picker: the built-ins, then every Unicode block, to stack as you like.
        open_alphabets();
        break;
    case SDLK_X:
        // Optimise all dimensions, as X in a filters window does (and, after the filter memory has
        // changed, with it). Not on the key's row, where X is a letter of the key.
        if (row_ != 2) toggle_all_filters(ToggleScope::EveryLine);
        break;
    case SDLK_F:
    {
        // The filters of the line whose settings are selected (its rows: setup_row_of and
        // setup_rows_of, in door order). Anywhere else (the GLOBAL rows, and the three at the
        // foot), F opens the filters of the line you will start on (the "line" row); F again
        // closes them.
        int line = -1;
        for (int li = 0; li < kLines; ++li)
            if (row_ >= setup_row_of(kDimensions[li].media) && row_ < setup_row_of(kDimensions[li].media) + setup_rows_of(kDimensions[li].media))
                line = li;
        if (line < 0)
        {
            const auto at = std::find(kStartLines.begin(), kStartLines.end(), s_.start_line); // in door order
            line = at == kStartLines.end() ? 0 : int(at - kStartLines.begin());
        }
        open_filters(line);
        break;
    }
    case SDLK_RETURN:
    case SDLK_KP_ENTER:
        // The way in is the last row, chosen like any other: Enter anywhere else does nothing,
        // so a stray Return while setting a line up never drops you into the hallway.
        // Find my limits and Reset act where they stand; only the last row is the way in.
        if (row_ == kLimitsRow) { find_limits(); break; }
        if (row_ == kResetRow) { reset_settings(); break; }
        if (row_ != kEnterRow) break;
        if (s_.key.empty()) s_.key = "sieve";
        // Over the budget, the foot of the list says which limit and which line, and the first
        // Enter only says what going in anyway means: slower than this machine's budget, with only
        // the room you stand in kept (thin). A second Enter goes in. Past too_large_bits(), where one
        // address would need a gigabyte, the way stays shut.
        if (too_large()) break;
        if (over_budget() >= 0 || graphics_over())
        {
            if (!go_anyway_armed_)
            {
                go_anyway_armed_ = true;
                break;
            }
            went_in_thin_ = true;
        }
        save_filters();
        done = true;
        result = Result::Enter;
        break;
    case SDLK_ESCAPE:
        done = true;
        result = Result::Back;
        break;
    default: break;
    }
}

Menu::Result Menu::run()
{
    SDL_SetWindowRelativeMouseMode(window_, false);
    SDL_StartTextInput(window_);
    music_mode(MusicMode::Menus);
    music_colours_default();
    bool done = false;
    Result result = Result::Quit;
    while (!done)
    {
        SDL_Event e;
        // Wait for input (up to a frame), so the menu does not spin a core when VSync is off.
        if (SDL_WaitEventTimeout(&e, 16))
        {
            handle(e, done, result);
            while (SDL_PollEvent(&e)) handle(e, done, result);
        }
        poll_toggle(); // X: the ticks, once the filters that clash are weighed
        {
            sieve::cli::timings::Scope timed("menu.setup.frame"); // its slowest: the first, which counts the lines
            render();
        }
        present(r_);
    }
    SDL_StopTextInput(window_);
    SDL_SetRenderLogicalPresentation(r_, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED); // the hallway draws at full size
    return result;
}

// The budget, drawn as four bars under the settings the way a game's graphics menu shows its
// memory: what the largest line's cache of units takes against the memory it is given, what the
// crate faces and the world take against the graphics memory, how long the slowest line's unit
// takes to open against the time allowed, and what the largest count of the filters needs against
// the filter memory. A bar that is over is red, and so is its figure. Under them, one status line
// (a changed filter memory waiting for X, or the dimensions still being worked out). Drawn at the
// menu's top right, out of the settings' way; returns the y below it.
float Menu::draw_budget(float x, float y)
{
    const Budget b = machine_budget();
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255}, red{255, 80, 80, 255};
    double cache = 0, slowest = 0;
    for (int i = 0; i < kLines; ++i)
    {
        const double c = line_cache_bytes(i), ms = line_ms(i, b);
        if (std::isfinite(c)) cache = std::max(cache, c);
        if (std::isfinite(ms)) slowest = std::max(slowest, ms);
    }
    const double gb = 1073741824.0;
    const double gfx_have = app_ ? double(app_->graphics_memory_gb) * 1024.0 : 0.0;
    // The filter memory: the most any line's count needs for its tables (the books' parts are counted
    // one at a time, so their largest), against the setting; past it, that line judges only.
    double filters_need = 0;
    for (int i = 0; i < kLines; ++i) filters_need = std::max(filters_need, stack_info(i).table_bytes); // (worked out on the workers, and kept)
    const double filters_have = filter_memory_setting(); // as set: what X will count with
    struct Bar { std::string label, figure; double used, have; };
    const Bar bars[4] = {
        {tr("setup.budget.memory"), trf("setup.budget.gb", {fixed(cache / gb, 2), fixed(b.cache_bytes / gb, 1)}), cache, b.cache_bytes},
        {tr("setup.budget.graphics"), trf("setup.budget.gb", {fixed(graphics_mb_needed() / 1024.0, 2), fixed(gfx_have / 1024.0, 0)}),
         graphics_mb_needed(), gfx_have},
        {tr("setup.budget.time"), trf("setup.budget.ms", {fixed(slowest, 1), fixed(b.ms_per_unit_at_limit, 0)}),
         slowest, b.ms_per_unit_at_limit},
        {tr("setup.budget.filters"), trf("setup.budget.filters.value", {sieve::memory_text(filters_need), sieve::memory_text(filters_have)}),
         filters_need, filters_have},
    };
    for (const Bar& bar : bars)
    {
        const bool known = bar.have > 0;
        const bool over = known && bar.used > bar.have;
        text(r_, x, y, bar.label, 1, grey);
        const float bx = x + 280, bw = 150, bh = 8;
        SDL_SetRenderDrawColor(r_, 90, 90, 90, 255);
        const SDL_FRect frame{bx, y, bw, bh};
        SDL_RenderRect(r_, &frame);
        const float fill = known ? float(std::min(1.0, bar.used / bar.have)) : 0.0f;
        const SDL_Color c = over ? red : white;
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        const SDL_FRect filled{bx + 1, y + 1, (bw - 2) * fill, bh - 2};
        SDL_RenderFillRect(r_, &filled);
        text(r_, bx + bw + 10, y, known ? bar.figure : tr("value.none"), 1, over ? red : grey);
        y += 14;
    }
    // Under the bars, one line, always kept free so nothing moves: the filter memory changed and
    // not yet counted with (until X), or which lines are still being counted.
    const std::string counting = counting_lines();
    const size_t cells = size_t(kBudgetW / 8);
    auto fit = [cells](const std::string& t) { return text_cells(t) <= cells ? t : fit_cells(t, cells - 2) + ".."; };
    if (memory_pending()) text(r_, x, y, fit(tr("setup.memory_changed")), 1, red);
    else if (time_pending()) text(r_, x, y, fit(tr("setup.time_changed")), 1, red);
    else if (toggling_ || !counting.empty()) text(r_, x, y, fit(trf("setup.calculating", {counting.empty() ? tr("setup.calculating.weighing") : counting})), 1, grey);
    y += 14;
    return y;
}

std::string Menu::counting_lines() const
{
    std::string out;
    for (int i = 0; i < kLines; ++i)
        if (pending_[size_t(i)].job && !pending_[size_t(i)].job->ready) out += (out.empty() ? "" : ", ") + tr(theme_of(i).key);
    return out;
}

void Menu::render()
{
    // The menu needs to be as wide as the title's subtitle and the budget beside it (and at least
    // 1240, for the settings on the left and the map on the right), and as tall as its rows: 16 px
    // each, 22 more for each section heading (GLOBAL and a line's), and the footer. Every setting
    // is on screen at once, so that turning one shows at once what it does to the bars; 1920 x
    // 1080 holds them all. In a smaller window it is drawn at that size and scaled down to fit,
    // instead of running off the edge or the actions at the foot running into the footer; worked
    // out from the rows and the text, so a row added later cannot bring that back.
    const int kMinW = std::max(1240, int(std::ceil(20 + text_width(tr("setup.subtitle"), 1) + 20 + kBudgetW + 20)));
    const int kMinH = 80 + row_count() * 16 + (kLines + 1) * 22 + 12 + 60;
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
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderClear(r_);
    SDL_SetRenderDrawBlendMode(r_, SDL_BLENDMODE_BLEND);

    text(r_, 20, 16, tr("setup.title"), 3, white);
    text(r_, 20, 48, tr("setup.subtitle"), 1, grey);
    const float budget_bottom = draw_budget(W - kBudgetW - 20, 16);

    // Settings.
    struct Row
    {
        int section; // -1: none; -2: the budget; kGlobal: GLOBAL; otherwise the door of the line (its colour)
        std::string label, value;
    };
    constexpr int kGlobal = -3;
    const int text_line = line_of(LineKind::Text), image_line = line_of(LineKind::Image);
    const int audio_line = line_of(LineKind::Audio), video_line = line_of(LineKind::Video);
    auto n = [](uint32_t v) { return std::to_string(v); };
    const std::string start_key = s_.start_line == "text" ? "line.pages" : "line." + s_.start_line;
    // The rooms with pictures of their own: yours and the picture distance either side.
    const int picture_rooms_here = 2 * picture_rooms() + 1;
    // What they would take drawn as wide as their letters ask, which the display cache may hold back.
    const size_t rooms_mb = size_t(std::ceil(double(picture_rooms_here) * s_.items_per_wall * display_mb(false)));
    std::vector<Row> rows = {
        {kGlobal, tr("setup.line"), tr(start_key)},
        {-1, tr("setup.ordering"), tr("ordering." + s_.mode)},
        {-1, tr("setup.key"), s_.key + (row_ == 2 ? "_" : "")},
        {-1, tr("setup.items_per_wall"), trf("setup.items_per_wall.value", {n(s_.items_per_wall), n(s_.items_per_wall / 2)})},
        {-1, tr("setup.angle_decimals"), trf("setup.angle_decimals.value", {std::to_string(app_ ? app_->angle_decimals : 1)})},
        {-1, tr("setup.title_length"), trf("setup.title_length.value", {n(s_.title_length)})},
        {-1, tr("setup.letters"), trf("setup.letters.value", {n(s_.letters_px), n(Settings{}.letters_px)})},
        {-1, tr("setup.model_tile"), trf("setup.model_tile.value", {n(s_.model_tile), std::to_string(display_px_line(0)), std::to_string(display_px_line(4))})},
        {-1, tr("setup.model_cache"), trf("setup.model_cache.value", {std::to_string(model_cache_mb()),
                                                                      std::to_string(size_t(double(model_cache_mb()) / display_mb())),
                                                                      n(s_.model_tile), std::to_string(rooms_mb),
                                                                      std::to_string(picture_rooms_here)})},
        {-1, tr("setup.closeup"), s_.closeup_px == 0 ? tr("setup.closeup.off")
                                                     : trf(s_.closeup_px == kCloseUpScreen ? "setup.closeup.screen" : "setup.closeup.value",
                                                           {std::to_string(closeup_px()), std::to_string(closeup_count()), std::to_string(size_t(std::ceil(closeup_mb())))})},
        {-1, tr("setup.focus"), s_.limits_focus == "all" ? tr("setup.focus.all") : trf("setup.focus.line", {tr(s_.limits_focus == "text" ? "line.pages" : "line." + s_.limits_focus)})},
        {-1, tr("setup.filter_memory"), trf("setup.filter_memory.value", {sieve::memory_text(filter_memory_setting())})},
        {-1, tr("setup.counting_memory"), trf("setup.counting_memory.value", {std::to_string(app_ ? app_->counting_memory_pct : 50),
                                                                             installed_memory_text(), std::to_string(counting_slots(filter_memory_setting()))})},
        {-1, tr("setup.merge_cache"), trf("setup.merge_cache.value", {std::to_string(app_ ? app_->merge_cache_pct : 50),
                                                                     sieve::memory_text(filter_memory_setting() * (app_ ? app_->merge_cache_pct : 50) / 100.0)})},
        {-1, tr("setup.time_budget"), trf("setup.time_budget.value", {std::to_string(app_ ? app_->unit_time_ms : 50)})},
        {-1, tr("setup.item_memory"), trf("setup.item_memory.value", {std::to_string(app_ ? app_->item_memory_pct : 25),
                                                                     sieve::memory_text(machine_budget().cache_bytes)})},
    };
    // Each line's rows (as many as setup_rows_of says), appended in door order.
    auto line_rows = [&](Media m) -> std::vector<Row> {
        switch (m)
        {
        case Media::Pages: return {
            {text_line, tr("setup.length"), trf("setup.length.value", {n(s_.length)})},
            {-1, tr("setup.alphabet"), trf("setup.alphabet.value", {s_.alphabet, n(alphabet_size(s_.alphabet))})},
            {-1, tr("setup.canon"), "canon-text-" + s_.canon},
            {-1, tr("setup.model"), tr(s_.model ? "setup.model.default" : "setup.model.none")},
        };
        case Media::Image: return {
            {image_line, tr("setup.width"), trf("setup.px", {n(s_.image_w)})},
            {-1, tr("setup.height"), trf("setup.px", {n(s_.image_h)})},
            {-1, tr("setup.palette"), trf("setup.palette.value", {s_.image_palette, n(palette_size(s_.image_palette))})},
        };
        case Media::Audio: return {
            is_pcm(s_)      ? Row{audio_line, tr(s_.pcm_channels > 1 ? "setup.samples.per_channel" : "setup.samples"), n(s_.samples)}
            : is_notes3(s_) ? Row{audio_line, tr(s_.n3_voices > 1 ? "setup.notes.per_voice" : "setup.notes"), n(s_.notes)}
                            : Row{audio_line, tr(note_set_of_settings(s_).voices > 1 ? "setup.notes.per_voice" : "setup.notes"), n(s_.notes)},
            {-1, tr("setup.note_set"), is_notes3(s_) ? (notes3_of_settings(s_) ? trf("setup.note_set.notes3", {n(notes3_of_settings(s_)->base())})
                                                                              : tr("setup.note_set.notes3_bad"))
                                       : is_pcm(s_) ? (pcm_ok(s_) ? trf("setup.note_set.pcm", {n(sieve::make_pcm_format(s_.pcm_rate, s_.pcm_bits, s_.pcm_channels).base())})
                                                                : tr("setup.note_set.pcm_bad"))
                                       : note_set_of_settings(s_).legacy && s_.note_set == "notes2" ? tr("setup.note_set.bad")
                                       : s_.note_set == "notes2" ? trf("setup.note_set.value", {n(note_set_of_settings(s_).base())})
                                                                 : tr("setup.note_set.fixed")},
            is_pcm(s_)      ? Row{-1, tr("setup.pcm_rate"), trf("setup.pcm_rate.value", {n(s_.pcm_rate)})}
            : is_notes3(s_) ? Row{-1, tr("setup.note_low"), s_.n3_low}
                            : Row{-1, tr("setup.note_low"), s_.note_set == "notes2" ? s_.note_low : tr("setup.notes2_only")},
            is_pcm(s_)      ? Row{-1, tr("setup.pcm_bits"), n(s_.pcm_bits)}
            : is_notes3(s_) ? Row{-1, tr("setup.note_high"), s_.n3_high}
                            : Row{-1, tr("setup.note_high"), s_.note_set == "notes2" ? s_.note_high : tr("setup.notes2_only")},
            is_notes3(s_) ? Row{-1, tr("setup.notes3_lengths"), trf("setup.notes3_lengths.value", {n(s_.n3_tpq), n(s_.n3_longest)})}
                          : Row{-1, tr("setup.note_durations"), s_.note_set == "notes2" ? s_.note_durations : tr(is_pcm(s_) ? "setup.pcm_unused" : "setup.notes2_only")},
            is_pcm(s_)      ? Row{-1, tr("setup.pcm_channels"), n(s_.pcm_channels)}
            : is_notes3(s_) ? Row{-1, tr("setup.voices"), n(s_.n3_voices)}
                            : Row{-1, tr("setup.voices"), s_.note_set == "notes2" ? n(s_.voices) : tr("setup.notes2_only")},
        };
        case Media::Video: return {
            {video_line, tr("setup.width"), trf("setup.px", {n(s_.video_w)})},
            {-1, tr("setup.height"), trf("setup.px", {n(s_.video_h)})},
            {-1, tr("setup.frames"), n(s_.frames)},
            {-1, tr("setup.palette"), trf("setup.palette.value", {s_.video_palette, n(palette_size(s_.video_palette))})},
        };
        case Media::Books: return {
            {kBooksLine, tr("setup.book_pages"), trf("setup.book_pages.value", {n(s_.book_pages)})},
        };
        case Media::Tracks: return {
            {line_of(Media::Tracks), tr("setup.track_units"), trf("setup.track_units.value", {n(s_.track_units)})},
        };
        case Media::Movies: return {
            {line_of(Media::Movies), tr("setup.movie_units"), trf("setup.movie_units.value", {n(s_.movie_units)})},
        };
        case Media::Models: return {
            {kModelsLine, tr("setup.model_vertices"), n(s_.model_vertices)},
            {-1, tr("setup.model_faces"), n(s_.model_faces)},
            {-1, tr("setup.model_coords"), trf("setup.model_coords.value", {n(s_.model_coords)})},
        };
        case Media::Binary: return {
            {kBinaryLine, tr("setup.binary_length"), trf("setup.binary_length.value", {n(s_.binary_bytes)})},
        };
        }
        return {};
    };
    for (const Dimension& d : kDimensions)
    {
        const std::vector<Row> r = line_rows(d.media);
        rows.insert(rows.end(), r.begin(), r.end());
    }
    rows.insert(rows.end(), {
        {-2, tr("setup.limits"), ""},
        {-1, tr("setup.reset"), ""},
        {-1, tr("setup.enter"), ""},
    });
    SDL_assert(int(rows.size()) == kEnterRow + 1); // setup_rows_of and the rows here agree
    // The values start just past the widest label ("> " and the label, from x 24), not at a fixed
    // place, so the longest values still end short of the map.
    size_t widest = 0;
    for (const Row& r : rows) widest = std::max(widest, text_cells(r.label));
    const float value_x = 24 + float(2 + widest + 2) * 8;
    const size_t value_cells = size_t(std::max(8.0f, kMapX - value_x - 8) / 8);
    float y = 80;
    for (int i = 0; i < int(rows.size()); ++i)
    {
        const Row& r = rows[size_t(i)];
        // The three actions at the foot of the list, clear of the settings above and the footer
        // below, so the list can grow without them ever running into either.
        if (r.section == -2) y = std::max(y + 8, H - 118);
        if (r.section >= 0 || r.section == kGlobal)
        {
            y += 4;
            const Theme* th = r.section >= 0 ? &theme_of(r.section) : nullptr;
            const std::string head = th ? tr(th->key) : tr("setup.start");
            text(r_, 20, y, head, 2, th ? menu_ink(*th) : white);
            y += 18;
        }
        if (i == row_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            const SDL_FRect sel{14, y - 3, kMapX - 20, 16};
            SDL_RenderFillRect(r_, &sel);
        }
        text(r_, 24, y, std::string(i == row_ ? "> " : "  ") + r.label, 1, white);
        // The rows are tight enough that the whole list fits above the three actions at the foot
        // of it. A value too long for its column stops short of the map rather than running into it.
        const std::string value = text_cells(r.value) <= value_cells ? r.value : fit_cells(r.value, value_cells - 2) + "..";
        text(r_, value_x, y, value, 1, i == row_ ? white : grey);
        y += 16;
    }
    text(r_, 20, H - 40, tr("setup.footer1"), 1, grey);
    text(r_, 20, H - 26, tr(in_game_ ? "setup.footer2.in_game" : "setup.footer2"), 1, grey);
    const SDL_Color red{255, 80, 80, 255};
    if (too_large()) text(r_, 20, H - 54, tr("setup.too_large"), 1, white);
    else if (go_anyway_armed_) text(r_, 20, H - 54, tr("setup.go_anyway"), 1, SDL_Color{255, 200, 80, 255}); // in place of why
    else if (const int over = over_budget(); over >= 0)
    {
        // Not a limit of the design: a limit of this machine, named so it can be reduced, and
        // which of the limits it is.
        const Theme& th = theme_of(over);
        const Budget mb = machine_budget();
        const bool slow = line_ms(over, mb) > mb.ms_per_unit_at_limit;
        text(r_, 20, H - 54, trf(slow ? "setup.over_time" : "setup.over_budget", {tr(th.key)}), 1, red);
    }
    else if (graphics_over()) text(r_, 20, H - 54, tr("setup.over_graphics"), 1, red);

    // The map: one bar per line, length proportional to its size in bits.
    const auto sizes = line_sizes();
    // The longest line spans the full height: nothing can leave the screen, however large.
    double scale_bits = 1;
    for (const auto& z : sizes)
        if (std::isfinite(z.bits)) scale_bits = std::max(scale_bits, z.bits);
    // The binary line takes a column at each end, because that is where it is:
    //
    //     binary | pages image audio video books models | binary   (the doors' order: dimensions.hpp)
    //
    // It is one line drawn twice, not two: it wraps around the outside of the others, and
    // which end of the corridor you meet it at decides which side of it the edge is on. It is
    // drawn like any other line, with its own two colours and a bar of the same width, and its
    // size is every file up to the BINARY length (SPECIFICATIONS §12.1).
    const float x0 = kMapX, pitch = std::max(80.0f, (W - x0 - 20) / float(kLines + 1));
    // The map starts below the budget, top right; at 1920 wide it is beside the subtitle instead.
    const float map_y = std::max(80.0f, budget_bottom + 6);
    const float label = map_y + 38, top = map_y + 136, bottom = H - 60, span = bottom - top, min_bar = 12;
    // Line names are drawn at double size where a column is wide enough to hold one.
    const float name_scale = pitch >= 110 ? 2.0f : 1.0f;
    text(r_, x0, map_y, tr("map.title"), 1, white);
    text(r_, x0, map_y + 12, trf("map.scale", {fixed(scale_bits, 0)}), 1, grey);
    for (int c = 0; c < kLines + 1; ++c)
    {
        const bool binary = c == 0 || c == kLines;
        const int i = binary ? kBinaryLine : c - 1; // the door
        Theme th = theme_of(i);
        const SDL_Color ink = menu_ink(th);
        const float x = x0 + float(c) * pitch;
        const LineSize& z = sizes[size_t(i)];
        // Labels above the bar, so a full-length bar never runs into them.
        const size_t cols = size_t(std::max(8.0f, pitch - 8) / 8);
        auto clip = [&](const std::string& t) { return text_cells(t) <= cols ? t : fit_cells(t, cols - 2) + ".."; };
        const SDL_Color edge = th.edge;
        th.edge = ink; // labels in a readable colour; the bar keeps the line's own edges
        // Magnifying glass: opens this line's filters, on every line; both binary columns open the
        // one binary line's filters (by its files' kinds).
        {
            magnifier_[c] = {x - 2, label - 2, 20, 20};
            SDL_SetRenderDrawColor(r_, th.edge.r, th.edge.g, th.edge.b, 255);
            for (int k = 0; k < 16; ++k)
            {
                const float a0 = float(k) / 16 * 6.2831853f, a1 = float(k + 1) / 16 * 6.2831853f;
                SDL_RenderLine(r_, x + 6 + 5 * std::cos(a0), label + 6 + 5 * std::sin(a0), x + 6 + 5 * std::cos(a1),
                               label + 6 + 5 * std::sin(a1));
            }
            SDL_RenderLine(r_, x + 10, label + 10, x + 15, label + 15);
            SDL_RenderLine(r_, x + 11, label + 10, x + 16, label + 15);
        }
        text(r_, x + 22, label, tr(th.key), name_scale, th.edge);
        {
            const size_t caret = z.units.find(" = ");
            text(r_, x, label + 22, clip(trf("map.units", {z.units.substr(0, caret)})), 1, th.edge);
            text(r_, x, label + 34, clip(z.units.substr(caret + 3)), 1, th.edge);
            text(r_, x, label + 46, clip(trf("map.bits", {fixed(z.bits, 0)})), 1, th.edge);
            text(r_, x, label + 58, clip(trf("map.tiles", {fixed(std::max(0.0, z.bits * std::log10(2.0) - std::log10(128.0)), 1)})), 1, th.edge);
            text(r_, x, label + 70, clip(z.padding ? trf("map.empty_slots", {std::to_string(z.padding)}) : tr("map.whole_tiles")), 1, th.edge);
            if (z.bits > too_large_bits()) text(r_, x, label - 14, clip(tr("map.too_large")), 1, white);
            // Slow: a unit of it takes more than a quarter of the time allowed to open, measured here.
            else if (const Budget mb = machine_budget(); line_ms(i, mb) > mb.ms_per_unit_at_limit / 4) text(r_, x, label - 14, clip(tr("map.slow")), 1, grey);
        }
        // The bar: the line's own two colours; never shorter than min_bar, never past the bottom.
        // Its body in the line's colour, its frame and what survives in the edges'. A line whose
        // edges are too dark to see on the black menu (books: grey, with black edges) has the two
        // swapped, so it reads like the others: a dark body in a light frame.
        const bool dark_edges = (edge.r * 3 + edge.g * 6 + edge.b) / 10 < 60;
        const SDL_Color body = dark_edges ? edge : th.bg, frame = dark_edges ? th.bg : edge;
        const float len = std::isfinite(z.bits) ? std::clamp(float(z.bits / scale_bits) * span, min_bar, span) : span;
        const SDL_FRect bar{x + 8, top, 40, len};
        SDL_SetRenderDrawColor(r_, body.r, body.g, body.b, 255);
        SDL_RenderFillRect(r_, &bar);
        SDL_SetRenderDrawColor(r_, frame.r, frame.g, frame.b, 255);
        SDL_RenderRect(r_, &bar);
        const SDL_FRect inner{x + 9, top + 1, 38, len - 2};
        SDL_RenderRect(r_, &inner);
        // What survives the ticked filters, where it can be counted exactly: a filled bar inside.
        const StackInfo& info = stack_info(i);
        if (info.survivor_bits >= 0)
        {
            const float slen = std::clamp(float(info.survivor_bits / scale_bits) * span, 3.0f, len - 4);
            const SDL_FRect sv{x + 14, top + 2, 28, slen};
            SDL_SetRenderDrawColor(r_, frame.r, frame.g, frame.b, 200);
            SDL_RenderFillRect(r_, &sv);
            text(r_, x, label + 82, clip(trf("map.survivors", {fixed(info.survivor_bits, 0)})), 1, th.edge);
        }
        else
        {
            size_t ticked = 0;
            if (i == kBooksLine)
                for (const auto& part : cfg_.books.parts) ticked += part.enabled.size();
            else if (is_composition(i))
                for (const auto& part : comp_cfg(i).parts) ticked += part.enabled.size();
            else if (i == kBinaryLine) ticked = cfg_.binary.enabled.size();
            else if (i == kModelsLine) ticked = cfg_.models.enabled.size();
            else if (unit_at(i) >= 0) ticked = cfg_.lines[unit_at(i)].enabled.size();
            if (ticked) text(r_, x, label + 82, clip(trf("map.ticked", {std::to_string(ticked)})), 1, th.edge);
        }
    }
    if (alpha_open_) render_alphabets(W, H);
    else if (overlay_ >= 0) render_overlay(W, H);
}


// ---------------------------------------------------------------- filters overlay

sieve::FilterLine Menu::filter_line_of(int i) const
{
    sieve::FilterLine f;
    switch (kDimensions[i].media)
    {
    case Media::Pages:
    {
        const sieve::Alphabet& a = sieve::alphabet_of(s_.alphabet);
        f = {"text", a.id(), a.size(), s_.length, &a, 0, 0, 0};
        break;
    }
    case Media::Image:
        f = {"image", "image/" + s_.image_palette + "/" + std::to_string(s_.image_w) + "x" + std::to_string(s_.image_h),
             palette_size(s_.image_palette), clamp32(positions(s_.image_w, s_.image_h)), nullptr, s_.image_w, s_.image_h, 1};
        break;
    case Media::Audio:
    {
        const sieve::NoteSet set = note_set_of_settings(s_);
        f = {"audio", set.id(), set.base(), s_.notes * set.voices, nullptr, 0, 0, 0};
        break;
    }
    case Media::Models: f = sieve::cli::models_filter_line(s_.model_vertices, s_.model_faces, s_.model_coords); break;
    case Media::Binary: f = sieve::cli::binary_filter_line(s_.binary_bytes); break;
    case Media::Tracks:
    case Media::Movies: f = filter_line_of(line_of(*kDimensions[i].composes)); break; // a unit (their parts: part_line)
    default: // video (and books, whose parts are book_part_line's)
        f = {"video", "video/" + s_.video_palette + "/" + std::to_string(s_.video_w) + "x" + std::to_string(s_.video_h) + "x" + std::to_string(s_.frames),
             palette_size(s_.video_palette), clamp32(positions(s_.video_w, s_.video_h, s_.frames)), nullptr, s_.video_w, s_.video_h, s_.frames};
        break;
    }
    return f;
}

sieve::FilterLine Menu::book_part_line(int part) const
{
    if (part == 0) return filter_line_of(line_of(Media::Image));
    sieve::FilterLine f = filter_line_of(line_of(Media::Pages));
    if (part == 2) f.length = uint32_t(std::min<uint64_t>(uint64_t(f.length) * s_.book_pages, UINT32_MAX));
    return f;
}

sieve::FilterLine Menu::part_line(int line, int part) const
{
    if (line == kBooksLine) return book_part_line(part);
    // Tracks and movies: the cover a picture, the title one of the titled lines' titles, each
    // unit a unit of its line, and the units joined that line's unit N long (joined_filter_line).
    if (part == 0) return filter_line_of(line_of(Media::Image));
    if (part == 1)
    {
        sieve::FilterLine f = filter_line_of(line_of(Media::Pages));
        f.length = s_.title_length;
        return f;
    }
    sieve::FilterLine f = filter_line_of(line);
    if (part == kJoinedPart)
    {
        f.length = uint32_t(std::min<uint64_t>(uint64_t(f.length) * comp_units(line), UINT32_MAX));
        f.frames *= comp_units(line);
    }
    return f;
}

sieve::cli::LineFilters& Menu::filters_of(const ORow& row)
{
    if (overlay_ == kModelsLine) return cfg_.models;
    if (overlay_ == kBinaryLine) return cfg_.binary;
    if (is_composition(overlay_)) return comp_cfg(overlay_).parts[std::max(0, row.part)];
    return overlay_ == kBooksLine ? cfg_.books.parts[std::max(0, row.part)] : cfg_.lines[unit_at(overlay_)];
}

const sieve::cli::LineFilters& Menu::filters_of(const ORow& row) const
{
    if (overlay_ == kModelsLine) return cfg_.models;
    if (overlay_ == kBinaryLine) return cfg_.binary;
    if (is_composition(overlay_)) return comp_cfg(overlay_).parts[std::max(0, row.part)];
    return overlay_ == kBooksLine ? cfg_.books.parts[std::max(0, row.part)] : cfg_.lines[unit_at(overlay_)];
}

sieve::cli::FilterMode& Menu::mode_of(int line)
{
    if (line == kModelsLine) return cfg_.models.mode;
    if (line == kBinaryLine) return cfg_.binary.mode;
    if (is_composition(line)) return comp_cfg(line).mode;
    return line == kBooksLine ? cfg_.books.mode : cfg_.lines[unit_at(line)].mode;
}

// How much of a line its filters remove, exactly, as a percentage: truncated (so 100% means every
// unit, and 0% none), with as many decimals as it takes to get past the leading 9s or 0s (up to
// twelve, then "..."), and, where one side is too small to read as a percentage, that side
// as a power of ten.
static std::string filtered_text(const sieve::BigUint& kept, const sieve::BigUint& total)
{
    if (total.is_zero()) return "0%";
    sieve::BigUint removed = total;
    removed -= kept;
    if (removed.is_zero()) return "0%";
    if (kept.is_zero()) return "100%";
    std::string pct;
    bool cut = false, extra = false;
    for (uint32_t d = 1;; ++d)
    {
        sieve::BigUint q, r;
        sieve::BigUint::divmod(sieve::BigUint::mul(removed, sieve::BigUint::pow(10, d + 2)), total, q, r);
        std::string digits = q.to_decimal();
        if (digits.size() < d + 1) digits.insert(0, d + 1 - digits.size(), '0');
        const std::string whole = digits.substr(0, digits.size() - d), frac = digits.substr(digits.size() - d);
        pct = whole + "." + frac;
        const bool all_zero = whole == "0" && frac.find_first_not_of('0') == std::string::npos;
        const bool all_nine = whole == "99" && frac.find_first_not_of('9') == std::string::npos;
        if (!all_zero && !all_nine)
        {
            // Past the 9s or 0s: one digit more, so it reads to two figures (0.0059%, 99.9941%).
            if (d > 1 && !extra)
            {
                extra = true;
                continue;
            }
            break;
        }
        if (d == 12)
        {
            cut = true;
            break;
        }
    }
    std::string out = pct + (cut ? "...%" : "%");
    const double lt = total.log10_approx(), lk = kept.log10_approx() - lt, lr = removed.log10_approx() - lt;
    // The smaller side as a power of ten, always, so every stack reads the same way (92.4% is kept
    // 10^-1.12; 0.0046% is removed 10^-4.34).
    if (lk <= lr) out += "  (" + trf("filters.filtered.kept", {"10^" + fixed(lk, 2)}) + ")";
    else out += "  (" + trf("filters.filtered.removed", {"10^" + fixed(lr, 2)}) + ")";
    return out;
}

// The same from an estimate of the survivors (their log10), where they are too many to count
// exactly (utf8-valid-v1 past its table): marked "~", worked out in doubles, so its last figures
// are not guaranteed, but read the same way.
static std::string filtered_estimate(double kept_log10, const sieve::BigUint& total)
{
    const double lt = total.log10_approx(), lk = std::min(0.0, kept_log10 - lt);
    if (!std::isfinite(kept_log10)) return "~100%";
    const double kept = std::pow(10.0, lk), removed = -std::expm1(lk * std::log(10.0));
    if (removed <= 0) return "~0%";
    std::string pct;
    bool cut = false, extra = false;
    if (kept < 1e-14)
    {
        pct = "99.999999999999";
        cut = true;
    }
    else
        for (uint32_t d = 1;; ++d)
        {
            char buf[64];
            std::snprintf(buf, sizeof buf, "%.0f", std::floor(removed * std::pow(10.0, double(d + 2))));
            std::string digits = buf;
            if (digits.size() < d + 1) digits.insert(0, d + 1 - digits.size(), '0');
            const std::string whole = digits.substr(0, digits.size() - d), frac = digits.substr(digits.size() - d);
            pct = whole + "." + frac;
            const bool all_zero = whole == "0" && frac.find_first_not_of('0') == std::string::npos;
            const bool all_nine = whole == "99" && frac.find_first_not_of('9') == std::string::npos;
            if (!all_zero && !all_nine)
            {
                if (d > 1 && !extra)
                {
                    extra = true;
                    continue;
                }
                break;
            }
            if (d == 12)
            {
                cut = true;
                break;
            }
        }
    std::string out = "~" + pct + (cut ? "...%" : "%");
    const double lr = std::log10(removed);
    if (lk <= lr) out += "  (" + trf("filters.filtered.kept", {"10^" + fixed(lk, 2)}) + ")";
    else out += "  (" + trf("filters.filtered.removed", {"10^" + fixed(lr, 2)}) + ")";
    return out;
}

// Which tab of the filters window lists a filter: 0 built-in, 1 custom (plugins), 2 retired.
int tab_of(const sieve::FilterSpec& f) { return f.retired ? 2 : f.plugin_sha256.empty() ? 0 : 1; }

// A stack's tally from its survivors: the share removed, the survivors and the footer's line.
void Menu::survivors_of(StackInfo& out, const sieve::BigUint& n, const sieve::BigUint& total)
{
    out.filtered = filtered_text(n, total);
    out.survivor_bits = n.is_zero() ? 0 : n.log10_approx() / std::log10(2.0);
    out.kept_log10 = n.is_zero() ? -std::numeric_limits<double>::infinity() : n.log10_approx() - total.log10_approx();
    out.status = trf(n.is_zero() ? "status.survivors_none" : "status.survivors",
                     {n.log10_approx() < 15 ? n.to_decimal() : "~10^" + fixed(n.log10_approx(), 1)});
}

void Menu::none_of(StackInfo& out)
{
    out.status = tr("status.none_units");
    out.filtered = "0%";
    out.kept_log10 = 0;
}

static std::string settings_key(const sieve::cli::LineFilters& lf)
{
    // (With the filter memory and the time budget: what can be counted changes with them.)
    std::string key = std::to_string(uint64_t(sieve::filter_memory())) + "/" + std::to_string(uint64_t(sieve::unit_time_ms())) + "/" +
                      std::string(to_string(lf.mode)) + ":";
    for (const auto& n : lf.enabled) key += n + ",";
    for (const auto& [n, vals] : lf.values)
        for (const auto& [k, v] : vals) key += n + "." + k + "=" + v + ";";
    return key;
}

// The key and the work for one stack's tally: the line at door i (the books' with a part: one part
// of the books) with filters `lf`. The work runs on a worker, so
// it takes copies of everything it needs. Used for each line's own stack (stack_info) and for each
// filter alone (filter_share).
std::pair<std::string, std::function<Menu::StackInfo()>> Menu::stack_job(int i, const sieve::cli::LineFilters& lf, int part) const
{
    // The models line: its own stack on a model's number (sieve/modelsieve.hpp).
    if (i == kModelsLine)
    {
        const uint32_t v = s_.model_vertices, f = s_.model_faces, c = s_.model_coords;
        const std::string key = "models/" + std::to_string(v) + "/" + std::to_string(f) + "/" + std::to_string(c) + "/" + settings_key(lf);
        return {key, [lf, v, f, c]() {
                    StackInfo out;
                    const sieve::ModelSpace space(v, f, c, "sieve");
                    const sieve::ModelSieve ms = sieve::cli::build_model_sieve(space, lf);
                    out.table_bytes = ms.table_bytes();
                    if (!ms.empty() && !ms.can_rank()) out.status = trf("status.not_countable", {ms.blocker()});
                    else if (ms.empty()) none_of(out);
                    else survivors_of(out, ms.count(), space.size());
                    return out;
                }};
    }
    // The binary line: its files' kinds, counted exactly at any length (sieve/filekind.hpp), and
    // the pages as its items (counted exactly; the other forms need the lines built).
    if (i == kBinaryLine)
    {
        const uint32_t bytes = s_.binary_bytes, length = s_.length;
        const std::string alphabet = s_.alphabet;
        const std::string key = "binary/" + std::to_string(bytes) + "/" + alphabet + "/" + std::to_string(length) + "/" + settings_key(lf);
        return {key, [lf, bytes, length, alphabet]() {
                    StackInfo out;
                    const sieve::BinarySpace space(std::max<uint32_t>(1, bytes), "sieve");
                    sieve::BinaryItems items;
                    items.pages = sieve::cli::page_pattern(sieve::alphabet_of(alphabet), length);
                    const sieve::BinarySieve bs = sieve::cli::build_binary_sieve(space, lf, &items);
                    out.table_bytes = bs.table_bytes();
                    if (bs.empty()) none_of(out);
                    else if (bs.can_rank()) survivors_of(out, bs.count(), space.size());
                    else if (!bs.can_count()) out.status = trf("status.not_countable", {bs.blocker()});
                    else if (bs.count_exact())
                    {
                        // Counted, though compact cannot number them (utf8-valid-v1 with the kind filters).
                        survivors_of(out, bs.survivors(), space.size());
                        out.status = trf("status.survivors_no_compact", {out.status.substr(0, out.status.find(" (exact)")), bs.blocker()});
                    }
                    else
                    {
                        // Past utf8-valid-v1's table: estimated.
                        const double l = bs.survivors_log10();
                        out.filtered = filtered_estimate(l, space.size());
                        out.survivor_bits = std::isfinite(l) ? l / std::log10(2.0) : 0;
                        out.kept_log10 = l - space.size().log10_approx();
                        out.status = trf("status.survivors_estimated", {fixed(l, 1), bs.blocker()});
                    }
                    return out;
                }};
    }
    const sieve::FilterLine fl = has_parts(i) ? part_line(i, std::max(0, part)) : filter_line_of(i);
    const std::string key = (has_parts(i) ? "part" + std::to_string(part) + "/" : std::string()) + fl.symbols_id + "/" + std::to_string(fl.length) + "/" + settings_key(lf);
    if (line_sizes()[size_t(i)].bits > too_large_bits())
        return {key, [] { return StackInfo{"too large", tr("status.too_large"), -1, ""}; }};
    return {key, [fl, lf]() {
                StackInfo out;
                sieve::cli::timings::Scope timed("menu.survivors"); // a line's stack built and counted
                const sieve::FilterStack st = sieve::cli::build_stack(fl, lf);
                if (!st.empty()) out.table_bytes = st.table_bytes();
                if (st.empty()) none_of(out);
                else if (st.ranker()) survivors_of(out, st.ranker()->count(), sieve::BigUint::pow(fl.base, fl.length));
                else out.status = trf("status.not_countable", {st.compact_blocker()});
                return out;
            }};
}

const Menu::StackInfo& Menu::stack_info(int i)
{
    if (has_parts(i)) return parts_stack_info(i);
    if (unit_at(i) >= 0 && line_sizes()[size_t(i)].bits > too_large_bits())
    {
        info_[i] = StackInfo{"too large", tr("status.too_large"), -1, ""};
        return info_[i];
    }
    const sieve::cli::LineFilters& lf = i == kModelsLine ? cfg_.models : i == kBinaryLine ? cfg_.binary : cfg_.lines[unit_at(i)];
    auto [key, work] = stack_job(i, lf);
    return resolve(i, key, std::move(work));
}

// One filter alone on its line (with what ticking it ticks too, its prerequisites), with its
// settings as they stand: the share of the line it removes, for comparing filters at a glance and
// for X to keep the stronger of two that clash. Counted on a worker like a line's stack, and kept.
sieve::cli::LineFilters Menu::alone(const sieve::cli::LineFilters& lf, const std::string& filter)
{
    sieve::cli::LineFilters one;
    one.mode = sieve::cli::FilterMode::Mark;
    one.values = lf.values;
    (void)sieve::cli::tick_filter(one, filter, true);
    return one;
}


// The books', tracks' or movies' parts, each counted on its own, on a worker like the other lines;
// the whole exact when every part with filters can rank (a part with none keeps all its units). A
// track's or movie's units are each judged alone, so their survivors count to the power of N, unless
// the joined stack judges them: then the joined unit is the part, counted once, and with the units'
// own filters as well nothing is counted (CompositionSieve).
const Menu::StackInfo& Menu::parts_stack_info(int line)
{
    if (line_sizes()[size_t(line)].bits > too_large_bits())
    {
        info_[line] = StackInfo{"too large", tr("status.too_large"), -1, ""};
        return info_[line];
    }
    const bool books = line == kBooksLine;
    const int nparts = parts_of(line);
    const sieve::cli::LineFilters* settings = books ? cfg_.books.parts : comp_cfg(line).parts;
    std::string key = std::string(kDimensions[line].id) + "/" + std::to_string(uint64_t(sieve::filter_memory())) + "/" + std::to_string(uint64_t(sieve::unit_time_ms())) +
                      "/" + to_string(mode_of(line)) + "/";
    std::vector<std::pair<sieve::FilterLine, sieve::cli::LineFilters>> parts(static_cast<size_t>(nparts));
    for (int part = 0; part < nparts; ++part)
    {
        parts[size_t(part)] = {part_line(line, part), settings[part]};
        const auto& [fl, lf] = parts[size_t(part)];
        key += fl.symbols_id + "/" + std::to_string(fl.length) + ":";
        for (const auto& n : lf.enabled) key += n + ",";
        for (const auto& [n, vals] : lf.values)
            for (const auto& [k, v] : vals) key += n + "." + k + "=" + v + ";";
        key += "|";
    }
    // Books: the pages are one part (book_pages of them read as one text; none at 0). Tracks and
    // movies: the units part is one unit, counted once and raised to the N units.
    const uint32_t book_pages = books ? s_.book_pages : 1, repeat = books ? 1 : comp_units(line);
    const bool has_title = books || s_.title_length > 0;
    key += std::to_string(repeat) + (has_title ? "" : "/notitle");
    return resolve(line, key, [parts, book_pages, repeat, has_title, books]() {
        const char* const names[4] = {"cover", "title", books ? "pages" : "units", "joined"};
        StackInfo out;
        sieve::cli::timings::Scope timed("menu.survivors.books");
        const size_t n = parts.size();
        // Each part's stack, built once (none for a part that is absent or has nothing ticked).
        std::vector<std::optional<sieve::FilterStack>> stacks(n);
        auto absent = [&](size_t part) { return (part == 2 && book_pages == 0) || (part == 1 && !has_title); };
        auto filtered = [&](size_t part) { return stacks[part] && !stacks[part]->empty(); };
        for (size_t part = 0; part < n; ++part)
            if (!absent(part) && !parts[part].second.enabled.empty())
            {
                stacks[part] = sieve::cli::build_stack(parts[part].first, parts[part].second);
                out.table_bytes = std::max(out.table_bytes, stacks[part]->table_bytes()); // the parts are counted one at a time
            }
        const size_t joined = kJoinedPart;
        const bool by_joined = n > joined && filtered(joined);
        double bits = 0;
        bool exact = true, any = false, none_survive = false;
        sieve::BigUint kept(1), total(1); // exact, part by part
        std::string blocker;
        for (size_t part = 0; part < std::min<size_t>(n, 3); ++part)
        {
            const sieve::FilterLine& fl = parts[part].first;
            const uint32_t times = part == 2 ? repeat : 1;
            const sieve::BigUint whole = absent(part) ? sieve::BigUint(1) : sieve::BigUint::pow(sieve::BigUint::pow(fl.base, fl.length), times);
            total = sieve::BigUint::mul(total, whole);
            // The units, judged joined: the joined unit is the part, alone or not counted at all.
            const size_t judge = part == 2 && by_joined ? joined : part;
            if (judge == joined && filtered(2))
            {
                any = true;
                exact = false;
                if (blocker.empty()) blocker = "units and joined: units judged one by one and joined as well cannot be counted"; // as CompositionSieve says
                continue;
            }
            if (!filtered(judge))
            {
                if (!absent(part)) bits += double(fl.length) * std::log2(double(fl.base)) * times;
                kept = sieve::BigUint::mul(kept, whole);
                continue;
            }
            any = true;
            const sieve::FilterStack& st = *stacks[judge];
            if (!st.ranker())
            {
                exact = false;
                if (blocker.empty()) blocker = std::string(names[judge]) + ": " + st.compact_blocker();
                continue;
            }
            const uint32_t power = judge == joined ? 1 : times; // the joined unit is all N units at once
            const sieve::BigUint& c = st.ranker()->count();
            kept = sieve::BigUint::mul(kept, sieve::BigUint::pow(c, power));
            if (c.is_zero()) none_survive = true;
            else bits += power * c.log10_approx() / std::log10(2.0);
        }
        if (exact) out.filtered = filtered_text(kept, total);
        if (!any) out.status = tr(books ? "status.none_books" : "status.none_units");
        else if (!exact) out.status = trf("status.not_countable", {blocker});
        else if (none_survive)
        {
            out.survivor_bits = 0;
            out.status = trf("status.survivors_none", {"0"});
        }
        else
        {
            out.survivor_bits = bits;
            out.status = trf(books ? "status.books" : "status.composed", {fixed(bits * std::log10(2.0), 1)});
        }
        return out;
    });
}

// One stack's rows under the current tab: its filters, the settings of those ticked, and for a
// custom filter who made it and what it requires; notes on prerequisites come first.
void Menu::add_filter_rows(std::vector<ORow>& rows, const sieve::FilterLine& line, const sieve::cli::LineFilters& lf, int part) const
{
    for (const std::string& n : sieve::cli::prerequisite_notes(lf)) rows.push_back({ORow::Kind::Info, "", "", part, trf("filters.note", {n})});
    bool any = false;
    for (const sieve::FilterSpec* f : sieve::filters_for(line))
    {
        const bool custom = !f->plugin_sha256.empty();
        if (tab_of(*f) != otab_) continue;
        any = true;
        rows.push_back({ORow::Kind::Filter, f->name(), "", part});
        if (custom)
        {
            rows.push_back({ORow::Kind::Info, f->name(), "", part, trf("filters.custom.by", {f->author, f->origin, f->plugin_sha256.substr(0, 12)})});
            for (const auto& r : f->prerequisites)
            {
                std::string what = r.name;
                for (const auto& [k, v] : r.values) what += " " + k + "=" + v;
                rows.push_back({ORow::Kind::Info, f->name(), "", part, trf("filters.custom.requires", {what})});
            }
        }
        if (lf.is_enabled(f->name()))
            for (const auto& p : f->params) rows.push_back({ORow::Kind::Param, f->name(), p.key, part});
    }
    if (otab_ == 1 && !any) rows.push_back({ORow::Kind::Info, "", "", part, tr("filters.custom.none")});
    if (otab_ == 2 && !any) rows.push_back({ORow::Kind::Info, "", "", part, tr("filters.retired.none")});
}

std::vector<Menu::ORow> Menu::overlay_rows() const
{
    std::vector<ORow> rows{{ORow::Kind::Mode, "", ""}, {ORow::Kind::Tabs, "", ""}};
    if (has_parts(overlay_))
    {
        for (int part = 0; part < parts_of(overlay_); ++part)
        {
            if (!has_part(overlay_, part)) continue; // no titles
            rows.push_back({ORow::Kind::Header, "", "", part});
            add_filter_rows(rows, part_line(overlay_, part), overlay_ == kBooksLine ? cfg_.books.parts[part] : comp_cfg(overlay_).parts[part], part);
        }
    }
    else if (unit_at(overlay_) >= 0) add_filter_rows(rows, filter_line_of(overlay_), cfg_.lines[unit_at(overlay_)], -1);
    else if (overlay_ == kModelsLine) add_filter_rows(rows, filter_line_of(kModelsLine), cfg_.models, -1);
    else if (overlay_ == kBinaryLine) add_filter_rows(rows, filter_line_of(kBinaryLine), cfg_.binary, -1);
    else return {{ORow::Kind::Mode, "", ""}}; // closed, or a line with no filters yet
    // Custom filter files that did not load, with why: never skipped without a word.
    if (otab_ == 1)
        for (const auto& pf : sieve::cli::load_plugins())
            if (!pf.error.empty()) rows.push_back({ORow::Kind::Info, "", "", -1, trf("filters.custom.refused", {pf.path, pf.error})});
    return rows;
}

void Menu::save_filters()
{
    try
    {
        cfg_.save(cfg_path_);
    }
    catch (const std::exception&)
    {
        // Read-only folder: the settings still apply to this session.
    }
}

// Z (this tab) and C (both tabs): if every filter in reach is ticked, untick them all; otherwise
// tick them all (with their prerequisites). On the books line, every part's list.
namespace {

// The menu's counting workers (Menu::resolve, Menu::filter_share): each line's stack is built
// and counted off the drawing thread. Jobs wait in a queue and are taken by at most
// counting_slots() threads at once, so pressing X (dozens of counts) neither starts dozens of threads nor holds
// dozens of tables. The threads are kept here, not in the menu, so leaving the menu never waits
// for one; building the hallway, and leaving the program, wait for them all first (they use the
// same caches, and statics must outlive them).
struct Workers
{
    std::mutex mx;
    std::condition_variable idle;          // signalled as each thread runs out of work
    std::deque<std::function<void()>> queue; // jobs not yet started, oldest first
    // Every thread started and not yet joined, with whether it has returned.
    std::vector<std::pair<std::thread, std::shared_ptr<std::atomic<bool>>>> threads;
    int alive = 0;   // threads still taking jobs
    int working = 0; // jobs being run now

    static int limit() { return counting_slots(); }
    void add(std::function<void()> job)
    {
        std::lock_guard<std::mutex> lock(mx);
        for (auto it = threads.begin(); it != threads.end();)
            if (*it->second)
            {
                it->first.join();
                it = threads.erase(it);
            }
            else ++it;
        queue.push_back(std::move(job));
        if (alive >= limit()) return; // a thread takes it when one comes free
        ++alive;
        auto done = std::make_shared<std::atomic<bool>>(false);
        threads.emplace_back(std::thread([this, done] { run(*done); }), done);
    }
    // A thread's loop: jobs from the queue until there are none.
    void run(std::atomic<bool>& done)
    {
        for (;;)
        {
            std::function<void()> job;
            {
                std::lock_guard<std::mutex> lock(mx);
                if (queue.empty())
                {
                    --alive;
                    done = true;
                    idle.notify_all();
                    return;
                }
                job = std::move(queue.front());
                queue.pop_front();
                ++working;
            }
            job();
            std::lock_guard<std::mutex> lock(mx);
            --working;
        }
    }
    bool busy()
    {
        std::lock_guard<std::mutex> lock(mx);
        return working > 0 || !queue.empty();
    }
    // Waits for every job queued, then joins the threads.
    void finish()
    {
        std::vector<std::pair<std::thread, std::shared_ptr<std::atomic<bool>>>> all;
        {
            std::unique_lock<std::mutex> lock(mx);
            idle.wait(lock, [this] { return alive == 0; });
            all.swap(threads);
        }
        for (auto& [t, done] : all)
            if (t.joinable()) t.join();
    }
    ~Workers()
    {
        {
            std::lock_guard<std::mutex> lock(mx);
            queue.clear(); // at exit, counts not yet started are not wanted
        }
        finish();
    }
};
Workers& workers()
{
    static Workers w;
    return w;
}

} // namespace

void finish_filter_warmup() { workers().finish(); }

namespace {
std::atomic<int> g_counting_share{50};
} // namespace

void set_counting_share(int percent) { g_counting_share = std::clamp(percent, 5, 100); }

// One per core but the one that draws, and no more than the counting memory holds when each
// count's tables may take the whole filter memory (at least one, however small the share).
int counting_slots(double filter_bytes)
{
    if (filter_bytes <= 0) filter_bytes = sieve::filter_memory();
    const int cores = std::max(1, int(std::thread::hardware_concurrency()) - 1);
    const int mb = SDL_GetSystemRAM(); // 0 if SDL cannot tell: then the cores alone
    if (mb <= 0) return cores;
    const double room = double(mb) * 1048576.0 * (g_counting_share / 100.0) / std::max(filter_bytes, 1.0);
    return std::clamp(int(room), 1, cores);
}

void Menu::start_job(std::shared_ptr<Job> job, std::function<StackInfo()> work)
{
    workers().add([job = std::move(job), work = std::move(work)] {
        try
        {
            if (job->wanted) job->result = work();
        }
        catch (const std::exception& e)
        {
            job->result = StackInfo{"", trf("status.error", {e.what()}), -1, ""};
        }
        job->ready = true;
    });
}

const Menu::StackInfo* Menu::filter_share(int line, int part, const sieve::cli::LineFilters& lf, const std::string& filter)
{
    auto [key, work] = stack_job(line, alone(lf, filter), part);
    key = std::to_string(line) + "|" + key;
    auto it = shares_.find(key);
    if (it == shares_.end())
    {
        if (shares_.size() > 1024) // settings long gone; a page of filters is far fewer
        {
            for (auto& [k, j] : shares_) j->wanted = false;
            shares_.clear();
        }
        auto job = std::make_shared<Job>();
        it = shares_.emplace(key, job).first;
        start_job(job, std::move(work));
    }
    return it->second->ready ? &it->second->result : nullptr;
}

bool filter_workers_busy() { return workers().busy(); }

// A line's stack info for `key`: at once if it has been worked out, else worked out by `work` on a
// worker thread while the menu says it is counting (and asked again each frame).
const Menu::StackInfo& Menu::resolve(int i, const std::string& key, std::function<StackInfo()> work)
{
    StackInfo& info = info_[i];
    if (info.key == key) return info;
    Pending& p = pending_[size_t(i)];
    if (p.key != key || !p.job)
    {
        if (p.job) p.job->wanted = false; // the settings it was for have changed
        p.key = key;
        p.job = std::make_shared<Job>();
        start_job(p.job, std::move(work));
    }
    if (p.job->ready)
    {
        info = p.job->result;
        info.key = key;
        p = Pending{};
        return info;
    }
    waiting_[size_t(i)] = StackInfo{"", tr("status.counting"), -1, ""};
    return waiting_[size_t(i)];
}

// The filters a toggle reaches: this tab of the open line (Z), both main tabs of it (C), or both
// main tabs of every line (X).
std::vector<Menu::Reach> Menu::reach_of(ToggleScope scope, int overlay, int tab)
{
    const bool both_tabs = scope != ToggleScope::ThisTab;
    struct Stack
    {
        sieve::cli::LineFilters* lf;
        sieve::FilterLine line;
        int li, part; // the line's door and, on the books, the part
    };
    std::vector<Stack> stacks;
    if (scope == ToggleScope::EveryLine)
    {
        for (LineKind k : {LineKind::Text, LineKind::Image, LineKind::Audio, LineKind::Video})
            stacks.push_back({&cfg_.lines[size_t(k)], filter_line_of(line_of(k)), line_of(k), -1});
        for (int part = 0; part < 3; ++part) stacks.push_back({&cfg_.books.parts[part], book_part_line(part), kBooksLine, part});
        for (int li = 0; li < kLines; ++li)
            if (is_composition(li))
                for (int part = 0; part < parts_of(li); ++part)
                    if (has_part(li, part) && part != kJoinedPart) stacks.push_back({&comp_cfg(li).parts[part], part_line(li, part), li, part});
        stacks.push_back({&cfg_.models, filter_line_of(kModelsLine), kModelsLine, -1});
        stacks.push_back({&cfg_.binary, filter_line_of(kBinaryLine), kBinaryLine, -1});
    }
    else if (overlay == kBooksLine)
        for (int part = 0; part < 3; ++part) stacks.push_back({&cfg_.books.parts[part], book_part_line(part), kBooksLine, part});
    else if (is_composition(overlay))
    {
        for (int part = 0; part < parts_of(overlay); ++part)
            if (has_part(overlay, part) && part != kJoinedPart) stacks.push_back({&comp_cfg(overlay).parts[part], part_line(overlay, part), overlay, part});
    }
    else if (unit_at(overlay) >= 0) stacks.push_back({&cfg_.lines[unit_at(overlay)], filter_line_of(overlay), overlay, -1});
    else if (overlay == kModelsLine) stacks.push_back({&cfg_.models, filter_line_of(kModelsLine), kModelsLine, -1});
    else if (overlay == kBinaryLine) stacks.push_back({&cfg_.binary, filter_line_of(kBinaryLine), kBinaryLine, -1});
    std::vector<Reach> in_reach;
    for (const Stack& st : stacks)
        for (const sieve::FilterSpec* f : sieve::filters_for(st.line))
            // This tab, or the two main tabs: retired filters are ticked only by hand, or by Z
            // on their own tab.
            // The title filters (title-v1, title-data-v1) are for a book's title page (words, then
            // SPACEs): ticking all ticks them only there, never on pages, where they would leave
            // little but short lines.
            if ((both_tabs ? tab_of(*f) != 2 : tab_of(*f) == tab) && (f->id.rfind("title", 0) != 0 || st.lf == &cfg_.books.parts[1]))
                in_reach.push_back({st.lf, st.li, st.part, f->name()});
    // Which of them clash with another in reach on the same stack: those are weighed.
    for (Reach& r : in_reach)
    {
        const sieve::FilterSpec* a = sieve::find_filter(r.name);
        const sieve::FilterLine line = r.part >= 0 ? part_line(r.li, r.part) : filter_line_of(r.li);
        for (const Reach& o : in_reach)
            if (o.lf == r.lf && o.name != r.name)
                if (const sieve::FilterSpec* b = sieve::find_filter(o.name);
                    a && b && !sieve::filter_conflict(*a, *b, &line, &r.lf->values_of(r.name), &o.lf->values_of(o.name)).empty())
                    r.clashes = true;
    }
    return in_reach;
}

// Whether the filter memory has been changed since the tallies were counted: the setting against the
// limit in use (sieve::filter_memory), which only X (or going into the hallway) brings up to it.
bool Menu::memory_pending() const { return filter_memory_setting() != sieve::filter_memory(); }

// Whether the time budget has been changed since the tallies were counted (what can rank follows
// it: symbol-entropy-v1 on two symbols), likewise brought up to it by X or by going in.
bool Menu::time_pending() const { return app_ && double(app_->unit_time_ms) != sieve::unit_time_ms(); }

double Menu::filter_memory_setting() const { return app_ ? double(app_->filter_memory_mb) * 1024 * 1024 : sieve::filter_memory(); }

void Menu::toggle_all_filters(ToggleScope scope)
{
    // X after the filter memory has changed: re-optimise every line with it, whatever is ticked
    // (everything in reach is unticked, then ticked again, the clashes weighed with the new limit).
    if (scope == ToggleScope::EveryLine && (memory_pending() || time_pending()))
    {
        sieve::set_filter_memory(filter_memory_setting());
        if (app_) sieve::set_unit_time_ms(app_->unit_time_ms);
        for (const Reach& r : reach_of(scope, overlay_, otab_)) (void)sieve::cli::tick_filter(*r.lf, r.name, false);
    }
    std::vector<Reach> in_reach = reach_of(scope, overlay_, otab_);
    // Anything in reach ticked: untick them all, at once. Otherwise tick them all (bar those that
    // clash), once the filters that clash have been weighed. ("All ticked" can never be reached
    // when some clash, so it is not the test.)
    if (std::any_of(in_reach.begin(), in_reach.end(), [](const Reach& e) { return e.lf->is_enabled(e.name); }))
    {
        toggling_.reset();
        for (const Reach& r : in_reach) (void)sieve::cli::tick_filter(*r.lf, r.name, false);
        save_filters();
        return;
    }
    // Where two clash, the one kept is the one that filters more on its own, its settings as they
    // stand. Weighing them can take seconds (a grammar over the books' pages), so it is done on the
    // workers while the window answers, and the ticking is done when they are in (poll_toggle).
    for (const Reach& r : in_reach)
        if (r.clashes) (void)filter_share(r.li, r.part, *r.lf, r.name);
    toggling_ = Toggling{scope, overlay_, otab_};
    poll_toggle();
}

void Menu::poll_toggle()
{
    if (!toggling_) return;
    std::vector<Reach> in_reach = reach_of(toggling_->scope, toggling_->overlay, toggling_->tab);
    for (Reach& r : in_reach)
        if (r.clashes)
        {
            const StackInfo* sh = filter_share(r.li, r.part, *r.lf, r.name);
            if (!sh) return; // still weighing
            r.kept = sh->kept_log10;
        }
    toggling_.reset();
    // The filters counted by arithmetic (a sliver each, and clashing with everything) last; then
    // the stronger alone first, a filter that could not be counted alone after those that could;
    // then newer versions first.
    std::stable_sort(in_reach.begin(), in_reach.end(), [](const Reach& x, const Reach& y) {
        const sieve::FilterSpec* a = sieve::find_filter(x.name);
        const sieve::FilterSpec* b = sieve::find_filter(y.name);
        if (!a || !b) return false;
        const bool arith_a = a->counts_as == "arithmetic", arith_b = b->counts_as == "arithmetic";
        if (arith_a != arith_b) return arith_b;
        const bool known_a = !std::isnan(x.kept), known_b = !std::isnan(y.kept);
        if (known_a != known_b) return known_a;
        if (known_a && x.kept != y.kept) return x.kept < y.kept;
        return a->version > b->version;
    });
    // Ticking all keeps the first of any two that cannot be counted together (the order above),
    // and skips the other: docs/FILTERS-CONFLICTS.md.
    for (const Reach& r : in_reach)
    {
        const sieve::FilterSpec* a = sieve::find_filter(r.name);
        const sieve::FilterLine line = r.part >= 0 ? part_line(r.li, r.part) : filter_line_of(r.li);
        bool clash = false;
        for (const std::string& other : r.lf->enabled)
            if (const sieve::FilterSpec* b = sieve::find_filter(other);
                a && b && !sieve::filter_conflict(*a, *b, &line, &r.lf->values_of(r.name), &r.lf->values_of(other)).empty())
                clash = true;
        if (!clash) (void)sieve::cli::tick_filter(*r.lf, r.name, true);
    }
    save_filters();
}

void Menu::overlay_change(int dir, bool big)
{
    const auto rows = overlay_rows();
    if (orow_ < 0 || orow_ >= int(rows.size())) return;
    const ORow& row = rows[size_t(orow_)];
    sieve::cli::LineFilters& lf = filters_of(row);
    using sieve::cli::FilterMode;
    switch (row.kind)
    {
    case ORow::Kind::Header:
    case ORow::Kind::Info: return;
    case ORow::Kind::Tabs: otab_ = ((otab_ + (dir < 0 ? -1 : 1)) % 3 + 3) % 3; break;
    case ORow::Kind::Mode:
    {
        const FilterMode order[] = {FilterMode::Off, FilterMode::Mark, FilterMode::Hide, FilterMode::Compact, FilterMode::Full, FilterMode::Excluded};
        constexpr int n = int(std::size(order));
        FilterMode& mode = mode_of(overlay_);
        // A mode read back from a settings file someone has edited by hand may not be one of the
        // list, so the search is bounded and anything unrecognised is treated as Off rather than
        // running off the end of it.
        int m = 0;
        while (m < n && order[m] != mode) ++m;
        if (m == n) m = 0;
        mode = order[((m + dir) % n + n) % n];
        break;
    }
    case ORow::Kind::Filter:
    {
        // With its prerequisites, unticking what it cannot be counted with on this line.
        const sieve::FilterLine line = has_parts(overlay_) ? part_line(overlay_, row.part) : filter_line_of(overlay_);
        (void)sieve::cli::tick_filter_by_hand(lf, row.filter, !lf.is_enabled(row.filter), &line);
        break;
    }
    case ORow::Kind::Param:
    {
        const sieve::FilterSpec* spec = sieve::find_filter(row.filter);
        const sieve::FilterParam* p = nullptr;
        for (const auto& q : spec->params)
            if (q.key == row.key) p = &q;
        sieve::FilterValues& vals = lf.values[row.filter];
        if (p->kind == sieve::FilterParam::Kind::Integer)
        {
            int64_t v = 0;
            try { v = sieve::param_int(*spec, vals, p->key); } catch (const std::exception&) { v = std::stoll(p->default_value); }
            v = std::clamp<int64_t>(v + dir * p->step * (big ? 10 : 1), p->min, p->max);
            vals[p->key] = std::to_string(v);
        }
        else
        {
            // A fixed list (key-v1's tonic and scale), or registered data: the registry's ids
            // ("" = the default).
            std::vector<std::string> choices{""};
            if (!p->choices.empty()) choices = p->choices;
            else try
            {
                if (p->key == "dictionary" || p->registry == "dictionary")
                    for (const auto& e : sieve::cli::load_registry().entries) choices.push_back(e.id);
                if (p->key == "model")
                    for (const auto& e : sieve::cli::load_model_registry().entries)
                        if (e.symbols == (has_parts(overlay_) ? part_line(overlay_, row.part) : filter_line_of(overlay_)).symbols_id) choices.push_back(e.id);
            }
            catch (const std::exception&)
            {
            }
            const std::string cur = sieve::param_value(*spec, vals, p->key);
            auto it = std::find(choices.begin(), choices.end(), cur);
            const int i = it == choices.end() ? 0 : int(it - choices.begin());
            vals[p->key] = choices[size_t(((i + dir) % int(choices.size()) + int(choices.size())) % int(choices.size()))];
        }
        break;
    }
    }
    save_filters();
}

void Menu::overlay_key(SDL_Keycode key, bool shift)
{
    const int n = int(overlay_rows().size());
    switch (key)
    {
    case SDLK_UP: orow_ = (orow_ + n - 1) % n; break;
    case SDLK_DOWN: orow_ = (orow_ + 1) % n; break;
    case SDLK_LEFT: overlay_change(-1, shift); break;
    case SDLK_RIGHT:
    case SDLK_SPACE:
    case SDLK_RETURN:
    case SDLK_KP_ENTER: overlay_change(1, shift); break;
    case SDLK_Z: toggle_all_filters(ToggleScope::ThisTab); break;
    case SDLK_X: toggle_all_filters(ToggleScope::EveryLine); break;
    case SDLK_C: toggle_all_filters(ToggleScope::BothTabs); break;
    case SDLK_ESCAPE:
    case SDLK_F:
        save_filters();
        overlay_ = -1;
        orow_ = 0;
        return; // closed: overlay_rows() has no line to read any more, so nothing below applies
    default: break;
    }
    orow_ = std::clamp(orow_, 0, int(overlay_rows().size()) - 1);
}

// ---------------------------------------------------------------- the alphabet picker

void Menu::open_alphabets()
{
    alpha_open_ = true;
    arow_ = 0;
    ascroll_ = 0;
    // Start on whatever is chosen, so the list opens where you left it.
    const std::vector<std::string> rows = alpha_rows();
    const std::vector<std::string> stack = alpha_stack();
    for (size_t i = 0; i < rows.size(); ++i)
        if (!rows[i].empty() && (rows[i] == s_.alphabet || (!stack.empty() && rows[i] == stack.front()))) { arow_ = int(i); break; }
}

// The rows: the built-in alphabets, a blank, then every Unicode block, in the table's order.
std::vector<std::string> Menu::alpha_rows() const
{
    std::vector<std::string> rows = sieve::alphabet_ids();
    rows.emplace_back();
    for (const std::string& id : sieve::block_ids()) rows.push_back(id);
    return rows;
}

// The current setting split on '+', which for a built-in is just its id.
std::vector<std::string> Menu::alpha_stack() const
{
    std::vector<std::string> parts;
    size_t start = 0;
    for (size_t i = 0; i <= s_.alphabet.size(); ++i)
    {
        const bool end = i == s_.alphabet.size();
        const bool sep = !end && s_.alphabet[i] == '+' && !(i > 0 && (s_.alphabet[i - 1] == 'u' || s_.alphabet[i - 1] == 'U'));
        if (!end && !sep) continue;
        if (i > start) parts.push_back(s_.alphabet.substr(start, i - start));
        start = i + 1;
    }
    return parts;
}

// A built-in replaces the whole setting; a block is ticked into the stack or out of it. The last
// block unticked falls back to lower27, since a line must have an alphabet.
void Menu::alpha_choose(int row)
{
    const std::vector<std::string> rows = alpha_rows();
    if (row < 0 || row >= int(rows.size()) || rows[size_t(row)].empty()) return;
    const std::string& id = rows[size_t(row)];
    if (sieve::block_by_id(id) == nullptr)
    {
        s_.alphabet = id;
        return;
    }
    std::vector<std::string> stack = alpha_stack();
    // Ticking a block when a built-in is chosen starts a fresh stack with just that block.
    if (stack.size() == 1 && sieve::block_by_id(stack.front()) == nullptr) stack.clear();
    const auto at = std::find(stack.begin(), stack.end(), id);
    if (at != stack.end()) stack.erase(at);
    else stack.push_back(id);
    if (stack.empty())
    {
        s_.alphabet = "lower27";
        return;
    }
    std::sort(stack.begin(), stack.end());
    stack.erase(std::unique(stack.begin(), stack.end()), stack.end());
    std::string spec;
    for (const std::string& p : stack)
    {
        if (!spec.empty()) spec += '+';
        spec += p;
    }
    s_.alphabet = spec;
}

void Menu::alpha_key(SDL_Keycode key)
{
    const int n = int(alpha_rows().size());
    auto step = [&](int dir) {
        do arow_ = (arow_ + n + dir) % n;
        while (alpha_rows()[size_t(arow_)].empty());
    };
    switch (key)
    {
    case SDLK_UP: step(-1); break;
    case SDLK_DOWN: step(1); break;
    case SDLK_PAGEUP: for (int i = 0; i < 8; ++i) step(-1); break;
    case SDLK_PAGEDOWN: for (int i = 0; i < 8; ++i) step(1); break;
    case SDLK_HOME: arow_ = 0; break;
    case SDLK_END: arow_ = n - 1; break;
    case SDLK_SPACE:
    case SDLK_RIGHT:
    case SDLK_RETURN:
    case SDLK_KP_ENTER: alpha_choose(arow_); break;
    case SDLK_LEFT: alpha_choose(arow_); break;
    case SDLK_A:
    case SDLK_ESCAPE: alpha_open_ = false; break;
    default: break;
    }
    arow_ = std::clamp(arow_, 0, n - 1);
}

void Menu::render_alphabets(float W, float H)
{
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    box_ = {600, 70, W - 614, H - 124};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 250);
    SDL_RenderFillRect(r_, &box_);
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
    SDL_RenderRect(r_, &box_);
    const SDL_FRect inner{box_.x + 2, box_.y + 2, box_.w - 4, box_.h - 4};
    SDL_RenderRect(r_, &inner);
    const float x = box_.x + 14;
    const size_t cols = size_t((box_.w - 60) / 8);
    text(r_, x, box_.y + 10, tr("alphabets.title"), 2, theme_of(Media::Pages).edge);
    text(r_, x, box_.y + 32, tr("alphabets.intro"), 1, grey);

    const std::vector<std::string> rows = alpha_rows();
    const std::vector<std::string> stack = alpha_stack();
    struct Item
    {
        std::vector<std::string> lines;
        float h;
    };
    std::vector<Item> items;
    for (const std::string& id : rows)
    {
        Item it;
        if (id.empty()) it.lines = {"-- " + tr("alphabets.blocks") + " --"};
        else if (const sieve::Block* b = sieve::block_by_id(id))
        {
            const bool on = std::find(stack.begin(), stack.end(), id) != stack.end();
            it.lines = {std::string(on ? "[x] " : "[ ] ") + id + "   " + std::string(b->name) + "   " +
                        trf("alphabets.count", {std::to_string(b->range.count())})};
            std::string d(tr_or("block." + id, std::string(b->description)));
            while (!d.empty())
            {
                size_t cut = d.size() <= cols - 4 ? d.size() : d.rfind(' ', cols - 4);
                if (cut == std::string::npos || cut == 0) cut = std::min(d.size(), cols - 4);
                it.lines.push_back("    " + d.substr(0, cut));
                d = d.substr(std::min(d.size(), cut + 1));
            }
        }
        else
        {
            const sieve::Alphabet& a = sieve::alphabet_by_id(id);
            it.lines = {std::string(s_.alphabet == id ? "(*) " : "( ) ") + id + "   " +
                            trf("alphabets.count", {std::to_string(a.size())}),
                        "    " + tr_or("alphabet." + id, a.description())};
        }
        it.h = float(it.lines.size()) * 12 + 8;
        items.push_back(it);
    }
    const float top = box_.y + 54, bottom = box_.y + box_.h - 44;
    ascroll_ = std::clamp(ascroll_, 0, std::max(0, int(items.size()) - 1));
    if (arow_ < ascroll_) ascroll_ = arow_;
    for (;;)
    {
        float h = 0;
        for (int i = ascroll_; i <= arow_ && i < int(items.size()); ++i) h += items[size_t(i)].h;
        if (h <= bottom - top || ascroll_ >= arow_) break;
        ++ascroll_;
    }
    arow_rects_.clear();
    float y = top;
    for (int i = ascroll_; i < int(items.size()); ++i)
    {
        const Item& it = items[size_t(i)];
        if (y + it.h > bottom) break;
        const SDL_FRect r{box_.x + 6, y - 3, box_.w - 12, it.h};
        arow_rects_.emplace_back(r, i);
        if (i == arow_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            SDL_RenderFillRect(r_, &r);
        }
        for (size_t k = 0; k < it.lines.size(); ++k)
            text(r_, x, y + float(k) * 12, (k == 0 && i == arow_ ? "> " : "  ") + it.lines[k], 1, k == 0 ? white : grey);
        y += it.h;
    }
    if (ascroll_ > 0) text(r_, box_.x + box_.w - 90, top - 12, tr("filters.more_above"), 1, grey);
    // What the setting works out to, which is the thing worth watching while ticking blocks.
    const sieve::Alphabet& chosen = sieve::alphabet_of(s_.alphabet);
    std::string status = trf("alphabets.status", {chosen.id(), std::to_string(chosen.size())});
    if (!chosen.text_is_encodable()) status += "  " + tr("alphabets.surrogates");
    text(r_, x, box_.y + box_.h - 36, status.substr(0, cols), 1, white);
    text(r_, x, box_.y + box_.h - 20, tr("alphabets.footer"), 1, grey);
}

void Menu::render_overlay(float W, float H)
{
    const SDL_Color white{255, 255, 255, 255}, grey{150, 150, 150, 255};
    const Theme& th = theme_of(overlay_);
    const SDL_Color title_ink = menu_ink(th);
    box_ = {600, 70, W - 614, H - 124};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 250);
    SDL_RenderFillRect(r_, &box_);
    SDL_SetRenderDrawColor(r_, 255, 255, 255, 255);
    SDL_RenderRect(r_, &box_);
    const SDL_FRect inner{box_.x + 2, box_.y + 2, box_.w - 4, box_.h - 4};
    SDL_RenderRect(r_, &inner);
    const float x = box_.x + 14;
    const size_t cols = size_t((box_.w - 60) / 8);
    const std::string title = trf("filters.title", {tr(th.key)});
    text(r_, x, box_.y + 10, title, 2, title_ink);
    // The tally: how much of this line the ticked filters remove, live as they change.
    {
        const StackInfo& tally = stack_info(overlay_);
        text(r_, x + text_width(title, 2) + 24, box_.y + 16,
             trf("filters.filtered", {tally.filtered.empty() ? tr("filters.filtered.unknown") : tally.filtered}), 1, white);
    }
    text(r_, x, box_.y + 32, (is_composition(overlay_) ? trf("filters.intro.composition", {tr(theme_of(overlay_).key)})
                                    : tr(overlay_ == kBooksLine ? "filters.intro.books" : overlay_ == kBinaryLine ? "filters.intro.binary" : overlay_ == kModelsLine ? "filters.intro.models" : "filters.intro")), 1, grey);

    const sieve::cli::FilterMode mode = mode_of(overlay_);
    const auto rows = overlay_rows();
    // Layout: each row's height, then scroll so the selected row stays in view.
    struct Item
    {
        std::vector<std::string> lines;
        float h;
        size_t red = SIZE_MAX; // lines from here on are drawn red (a filter's conflicts)
    };
    std::vector<Item> items;
    for (const ORow& row : rows)
    {
        Item it;
        const bool selected = int(items.size()) == orow_;
        const sieve::cli::LineFilters& lf = filters_of(row);
        if (row.kind == ORow::Kind::Mode)
            it.lines = {trf("filters.mode", {tr(std::string("mode.") + to_string(mode))}), "    " + tr("filters.mode.help1"),
                        "    " + tr("filters.mode.help2"), "    " + tr("filters.mode.help4"), "    " + tr("filters.mode.help3")};
        else if (row.kind == ORow::Kind::Header)
        {
            const bool comp = is_composition(overlay_);
            static const char* const heads[4] = {"filters.part.cover", "filters.part.title", "filters.part.pages", "filters.part.joined"};
            const std::string head = comp && row.part == 2 ? "filters.part.units" : heads[row.part];
            const std::string base_name = comp ? tr(theme_of(line_of(*kDimensions[overlay_].composes)).key) : std::string();
            std::string sub = comp && row.part == 1   ? trf("filters.part.title.composition.help", {std::to_string(s_.title_length)})
                              : comp && row.part >= 2 ? trf(head + ".help", {std::to_string(comp_units(overlay_)), base_name})
                              : row.part == 2         ? trf(head + ".help", {std::to_string(uint64_t(s_.book_pages) * s_.length)})
                                                      : tr(head + ".help");
            it.lines = {"-- " + tr(head) + " --"};
            while (!sub.empty())
            {
                size_t cut = sub.size() <= cols - 4 ? sub.size() : sub.rfind(' ', cols - 4);
                if (cut == std::string::npos || cut == 0) cut = std::min(sub.size(), cols - 4);
                it.lines.push_back("    " + sub.substr(0, cut));
                sub = sub.substr(std::min(sub.size(), cut + 1));
            }
        }
        else if (row.kind == ORow::Kind::Tabs)
        {
            // How many of each the line offers, whichever tab is showing.
            size_t n[3] = {0, 0, 0};
            auto count = [&](const sieve::FilterLine& l) {
                for (const sieve::FilterSpec* f : sieve::filters_for(l)) ++n[tab_of(*f)];
            };
            if (has_parts(overlay_))
                for (int part = 0; part < parts_of(overlay_); ++part) count(part_line(overlay_, part));
            else count(filter_line_of(overlay_));
            static const char* const keys[3] = {"filters.tab.builtin", "filters.tab.custom", "filters.tab.retired"};
            std::string tabs;
            for (int t = 0; t < 3; ++t)
            {
                const std::string label = trf(keys[t], {std::to_string(n[t])});
                tabs += (t ? "  -  " : "") + (t == otab_ ? "[ " + label + " ]" : label);
            }
            it.lines = {tabs, "    " + tr("filters.tab.help")};
        }
        else if (row.kind == ORow::Kind::Info)
        {
            std::string d = row.text;
            const std::string indent = row.filter.empty() ? "  " : "      ";
            while (!d.empty())
            {
                size_t cut = d.size() <= cols - 8 ? d.size() : d.rfind(' ', cols - 8);
                if (cut == std::string::npos || cut == 0) cut = std::min(d.size(), cols - 8);
                it.lines.push_back(indent + d.substr(0, cut));
                d = d.substr(std::min(d.size(), cut + 1));
            }
            if (it.lines.empty()) it.lines.push_back("");
        }
        else if (row.kind == ORow::Kind::Filter)
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            // Beside its name, the share of the line it removes on its own, as its settings stand,
            // to compare filters at a glance (counted on a worker; "counting..." until then).
            const StackInfo* share = filter_share(overlay_, row.part, lf, row.filter);
            const std::string tally = !share ? tr("filters.share.counting")
                                             : trf("filters.share", {share->filtered.empty() ? tr("filters.share.unknown") : share->filtered});
            std::string head = std::string(lf.is_enabled(row.filter) ? "[x] " : "[ ] ") + f->name();
            head += std::string(std::max<size_t>(3, 28 > head.size() ? 28 - head.size() : 3), ' ') + tally;
            it.lines = {head};
            std::string d = tr_or("filter." + f->name(), f->description);
            if (!f->replaced_by.empty()) d += " " + trf("filters.replaced_by", {f->replaced_by});
            while (!d.empty())
            {
                size_t cut = d.size() <= cols - 4 ? d.size() : d.rfind(' ', cols - 4);
                if (cut == std::string::npos || cut == 0) cut = std::min(d.size(), cols - 4);
                it.lines.push_back("    " + d.substr(0, cut));
                d = d.substr(std::min(d.size(), cut + 1));
            }
            // Selected: which filters it cannot be counted with here, in red (ticking one unticks
            // the other). "filters need merging" or "conflicting filters": docs/FILTERS-CONFLICTS.md.
            if (selected)
            {
                const sieve::FilterLine line = has_parts(overlay_) ? part_line(overlay_, row.part) : filter_line_of(overlay_);
                std::string merge, hard;
                for (const sieve::FilterSpec* g : sieve::filters_for(line))
                {
                    const std::string why = sieve::filter_conflict(*f, *g, &line, &lf.values_of(f->name()), &lf.values_of(g->name()));
                    if (why.empty()) continue;
                    std::string& list = why == "conflict" ? hard : merge;
                    list += (list.empty() ? "" : ", ") + g->name() + (lf.is_enabled(g->name()) ? " " + tr("filters.conflict.ticked") : "");
                }
                for (const auto& [key, list] : {std::pair{"filters.conflict.merge", merge}, std::pair{"filters.conflict.hard", hard}})
                {
                    if (list.empty()) continue;
                    if (it.red == SIZE_MAX) it.red = it.lines.size();
                    std::string m = trf(key, {list});
                    while (!m.empty())
                    {
                        size_t cut = m.size() <= cols - 4 ? m.size() : m.rfind(' ', cols - 4);
                        if (cut == std::string::npos || cut == 0) cut = std::min(m.size(), cols - 4);
                        it.lines.push_back("    " + m.substr(0, cut));
                        m = m.substr(std::min(m.size(), cut + 1));
                    }
                }
            }
        }
        else
        {
            const sieve::FilterSpec* f = sieve::find_filter(row.filter);
            const auto vit = lf.values.find(row.filter);
            std::string v;
            try { v = sieve::param_value(*f, vit == lf.values.end() ? sieve::FilterValues{} : vit->second, row.key); } catch (...) {}
            std::string desc;
            for (const auto& p : f->params)
                if (p.key == row.key) desc = tr_or("filter." + f->name() + "." + p.key, p.description);
            it.lines = {"      " + row.key + " = " + (v.empty() ? tr("filters.default") : v) + "     " + desc};
        }
        it.h = float(it.lines.size()) * 12 + 8;
        items.push_back(it);
    }
    // The footer: what the stack counts, or why it cannot, on as many as two lines (a reason that
    // names a table's size and the limit it passes is longer than one), the list making room.
    const StackInfo& info = stack_info(overlay_);
    std::vector<std::string> footer;
    for (std::string f = toggling_ ? tr("filters.weighing") : info.status; !f.empty() && footer.size() < 2;)
    {
        size_t cut = f.size() <= cols ? f.size() : footer.empty() ? f.rfind(' ', cols) : cols;
        if (cut == std::string::npos || cut == 0) cut = std::min(f.size(), cols);
        footer.push_back(f.substr(0, cut));
        f = f.substr(std::min(f.size(), cut + (cut < f.size() && f[cut] == ' ' ? 1 : 0)));
    }
    const float top = box_.y + 54, bottom = box_.y + box_.h - 44 - 12.0f * float(footer.size() > 1 ? footer.size() - 1 : 0);
    // Scroll: first row shown.
    oscroll_ = std::clamp(oscroll_, 0, std::max(0, int(items.size()) - 1));
    if (orow_ < oscroll_) oscroll_ = orow_;
    for (;;)
    {
        float h = 0;
        for (int i = oscroll_; i <= orow_ && i < int(items.size()); ++i) h += items[size_t(i)].h;
        if (h <= bottom - top || oscroll_ >= orow_) break;
        ++oscroll_;
    }
    row_rects_.clear();
    float y = top;
    for (int i = oscroll_; i < int(items.size()); ++i)
    {
        const Item& it = items[size_t(i)];
        if (y + it.h > bottom) break;
        const SDL_FRect r{box_.x + 6, y - 3, box_.w - 12, it.h};
        row_rects_.emplace_back(r, i);
        if (i == orow_)
        {
            SDL_SetRenderDrawColor(r_, 255, 255, 255, 40);
            SDL_RenderFillRect(r_, &r);
        }
        for (size_t k = 0; k < it.lines.size(); ++k)
            text(r_, x, y + float(k) * 12, (k == 0 && i == orow_ ? "> " : "  ") + it.lines[k], 1,
                 k >= it.red ? SDL_Color{255, 80, 80, 255} : k == 0 && rows[size_t(i)].kind != ORow::Kind::Info ? white : grey);
        y += it.h;
    }
    if (oscroll_ > 0) text(r_, box_.x + box_.w - 90, top - 12, tr("filters.more_above"), 1, grey);
    if (y < bottom && false) {}
    for (size_t k = 0; k < footer.size(); ++k)
        text(r_, x, box_.y + box_.h - 36 - 12.0f * float(footer.size() - 1 - k), footer[k], 1, white);
    text(r_, x, box_.y + box_.h - 20, tr("filters.footer"), 1, grey);
}

} // namespace hallway
