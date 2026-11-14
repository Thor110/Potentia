// Sieve hallway — the music player (music.hpp): the mixer on SDL's audio thread, the worker that
// chooses melodies by the modes' filter stacks, and the settings, recent melodies and favourites kept
// between runs.
//
// The mixer synthesises as it goes, a sample at a time, rather than rendering a melody before it
// plays: a slow melody of long notes with an echo would be minutes of samples to hold. Each channel
// knows its note, how far into it it is, its phase and its echo's delay line; a music channel also
// has a gain that moves towards its target over a second and a half, which is all the fading
// there is. A music channel whose gain has reached nothing waits where it is.

#include "music.hpp"

#include "font.hpp"
#include "strings.hpp"

#include "cli/lines.hpp"
#include "cli/items.hpp"
#include "cli/locate.hpp"
#include "sieve/audio.hpp"
#include "sieve/composition.hpp"
#include "sieve/space.hpp"
#include "sieve/text.hpp"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <map>
#include <random>
#include <sstream>
#include <string>

namespace hallway {

namespace fs = std::filesystem;
using sieve::BigUint;

namespace {

constexpr int kRate = 44100;
constexpr float kPi = 3.14159265358979f;
constexpr float kFadeSeconds = 1.5f;
constexpr size_t kRecent = 10;

MusicPlayer* g_player = nullptr;
// A track's cover and title shapes (set_track_shape): the setup menu's defaults until a hallway is built.
std::mutex g_shape_mx;
std::optional<sieve::Space> g_cover, g_title; // no cover yet: the defaults (made when first asked for)
std::mutex g_colour_mx; // the hallway may set them while it is built, on a worker
SDL_Color g_bg{0, 0, 0, 255}, g_ink{255, 255, 255, 255}; // the box's colours: where you are

const char* mode_id(MusicMode m) { return m == MusicMode::Menus ? "menus" : m == MusicMode::Personal ? "personal" : "world"; }

// The seven modes, brightest first, as semitones above the tonic, and the lines' names.
struct ModeSteps
{
    const char* name;
    int steps[7];
};
constexpr ModeSteps kModes[kMusicModes] = {
    {"lydian", {0, 2, 4, 6, 7, 9, 11}},  {"ionian", {0, 2, 4, 5, 7, 9, 11}},  {"mixolydian", {0, 2, 4, 5, 7, 9, 10}},
    {"dorian", {0, 2, 3, 5, 7, 9, 10}},  {"aeolian", {0, 2, 3, 5, 7, 8, 10}}, {"phrygian", {0, 1, 3, 5, 7, 8, 10}},
    {"locrian", {0, 1, 3, 5, 6, 8, 10}},
};
// The key each door is in when the key follows the line: a fifth further round the circle at each
// door (C G D A E B F# ...), folded within six semitones of C.
constexpr int fifths_at(int door)
{
    const int up = 7 * door % 12;
    return up > 6 ? up - 12 : up;
}
static_assert(fifths_at(0) == 0 && fifths_at(1) == -5 && fifths_at(2) == 2 && fifths_at(6) == 6);

// "world/binary" and back: where a melody played, in the files.
std::string where_id(const MusicMelody& t)
{
    return std::string(mode_id(t.mode)) + (t.mode == MusicMode::World && t.line >= 0 && t.line < kLines ? std::string("/") + kDimensions[t.line].id : "");
}
void where_from(const std::string& id, MusicMelody& t)
{
    const size_t slash = id.find('/');
    const std::string m = id.substr(0, slash);
    t.mode = m == "menus" ? MusicMode::Menus : m == "personal" ? MusicMode::Personal : MusicMode::World;
    t.line = -1;
    if (slash != std::string::npos) t.line = line_named(id.substr(slash + 1));
}

// A tonic's name ("C", "F#", "Bb") as a pitch class.
int pitch_class(const std::string& name)
{
    static const int letters[7] = {9, 11, 0, 2, 4, 5, 7}; // A B C D E F G
    if (name.empty() || name[0] < 'A' || name[0] > 'G') return 0;
    int pc = letters[name[0] - 'A'];
    for (size_t i = 1; i < name.size(); ++i) pc += name[i] == '#' ? 1 : name[i] == 'b' ? -1 : 0;
    return ((pc % 12) + 12) % 12;
}

std::string clock_now()
{
    const std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[8];
    std::snprintf(buf, sizeof buf, "%02d:%02d", tm.tm_hour, tm.tm_min);
    return buf;
}

// (the line feed too: a value can end one)
std::string trim(const std::string& s) { return sieve::trim(s, " \t\r\n"); }

MusicSettings defaults(MusicMode m)
{
    // MENUS is the serious one: a minor key, lower and slower, a plain sine, a little more
    // movement. WORLD is the relaxing one: the major scale (not the pentatonic, so every line's
    // mode is heard: the pentatonic lacks the steps Lydian and Mixolydian change), small steps,
    // long notes, few rests, a register kept low, ending home on the tonic, soft struck tones and
    // more echo. Both are starting points to be tuned by ear in the Media Player. The melody
    // plugins (data/filters) all count, so the stack counts and a melody is drawn by rank; without
    // them (a filters folder not found) only key-v1, which counts on its own.
    MusicSettings s;
    const bool plugins = sieve::find_filter("key-data-v2") && sieve::find_filter("melody-leap-v1") && sieve::find_filter("melody-lengths-v1") &&
                         sieve::find_filter("melody-rests-v1") && sieve::find_filter("melody-ending-v1") && sieve::find_filter("melody-range-v1");
    const bool menus = m == MusicMode::Menus;
    const std::string tonic = menus ? "A" : "C", scale = menus ? "minor" : "major";
    if (plugins)
    {
        s.filters.enabled = {"key-data-v2", "melody-leap-v1", "melody-lengths-v1", "melody-rests-v1", "melody-ending-v1", "melody-range-v1"};
        s.filters.values["key-data-v2"] = {{"tonic", tonic}, {"scale", scale}};
        s.filters.values["melody-leap-v1"] = {{"leap", menus ? "5" : "4"}, {"rest_resets", "no"}};
        s.filters.values["melody-lengths-v1"] = menus ? sieve::FilterValues{{"shortest", "e"}, {"longest", "h"}, {"eighths", "2"}}
                                                      : sieve::FilterValues{{"shortest", "q"}, {"longest", "w"}, {"eighths", "0"}};
        s.filters.values["melody-rests-v1"] = {{"run", menus ? "2" : "1"}, {"total", menus ? "6" : "4"}, {"leading", "no"}};
        s.filters.values["melody-ending-v1"] = {{"tonic", tonic}, {"hold", menus ? "q" : "h"}};
        s.filters.values["melody-range-v1"] = {{"low", "60"}, {"high", menus ? "76" : "79"}};
    }
    else
    {
        s.filters.enabled = {"key-v1"};
        s.filters.values["key-v1"] = {{"tonic", tonic}, {"scale", scale}};
    }
    if (menus)
    {
        s.volume = 25;
        s.tempo = 66;
        s.voice = Voice::Sine;
        s.echo = 20;
        s.gap = 6;
    }
    else
    {
        s.volume = 30;
        s.tempo = 72;
        s.voice = Voice::Soft;
        s.echo = 35;
        s.gap = 8;
    }
    return s;
}

Voice voice_from(const std::string& s)
{
    for (int v = 0; v < 4; ++v)
        if (s == voice_id(Voice(v))) return Voice(v);
    return Voice::Soft;
}

// A notation read back into its unit (the whole notation is one unit, its length the melody's; on
// several voices, the first voice's events are each voice's).
std::optional<std::vector<uint32_t>> notes_of(const sieve::NoteSet& set, const std::string& notation)
{
    const std::string first = notation.substr(0, notation.find("//"));
    std::istringstream in(first);
    std::string tok;
    uint32_t n = 0;
    while (in >> tok) ++n;
    if (n == 0) return std::nullopt;
    try
    {
        const auto c = set.legacy ? sieve::canonicalise_notes(notation, n) : sieve::canonicalise_notes2(notation, set, n);
        if (c.units.size() != 1) return std::nullopt;
        return c.units.front();
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

sieve::NoteSet set_of(const std::string& id)
{
    try
    {
        return sieve::note_set_of(id);
    }
    catch (const std::exception&)
    {
        return sieve::NoteSet{};
    }
}

} // namespace

sieve::cli::Line audio_line(const sieve::NoteSet& set, uint32_t length)
{
    sieve::cli::Args la;
    la.opts["line"] = "audio";
    la.opts["length"] = std::to_string(length);
    if (!set.legacy)
    {
        la.opts["note-set"] = "notes2";
        la.opts["low"] = sieve::note_name(set.low);
        la.opts["high"] = sieve::note_name(set.high);
        la.opts["durations"] = set.durations;
        la.opts["voices"] = std::to_string(set.voices);
    }
    return sieve::cli::make_line(la);
}

sieve::NoteSet MusicSettings::notes() const
{
    if (note_set != "notes2") return sieve::NoteSet{};
    try
    {
        return sieve::make_note_set(sieve::note_midi_of(low), sieve::note_midi_of(high), durations, voices);
    }
    catch (const std::exception&)
    {
        return sieve::NoteSet{};
    }
}

// ---------------------------------------------------------------- a dimension's music (IDEAS 19.220)

namespace {

// A parameter's value in the line: a space, a line break and % itself percent-encoded, so the line
// stays one line of words.
std::string pct_encode(const std::string& v)
{
    std::string out;
    for (const char c : v)
        if (c == '%' || c == ' ' || c == '\t' || c == '\n' || c == '\r')
        {
            char b[4];
            std::snprintf(b, sizeof b, "%%%02X", unsigned(static_cast<unsigned char>(c)));
            out += b;
        }
        else out += c;
    return out;
}

std::optional<std::string> pct_decode(const std::string& v)
{
    std::string out;
    for (size_t i = 0; i < v.size(); ++i)
    {
        if (v[i] != '%')
        {
            out += v[i];
            continue;
        }
        if (i + 2 >= v.size() || !std::isxdigit(static_cast<unsigned char>(v[i + 1])) || !std::isxdigit(static_cast<unsigned char>(v[i + 2]))) return std::nullopt;
        out += char(std::stoi(v.substr(i + 1, 2), nullptr, 16));
        i += 2;
    }
    return out;
}

// A whole number from first to last, nothing else: nullopt for anything that is not one.
std::optional<long long> whole(const std::string& v)
{
    if (v.empty() || v.size() > 12) return std::nullopt;
    for (size_t i = 0; i < v.size(); ++i)
        if (!std::isdigit(static_cast<unsigned char>(v[i])) && !(i == 0 && v[i] == '-' && v.size() > 1)) return std::nullopt;
    // (as the line is written: no leading zeros, no "-0"; a number written otherwise is not as written)
    const long long n = std::stoll(v);
    if (std::to_string(n) != v) return std::nullopt;
    return n;
}

} // namespace

std::string personal_music_line(const MusicSettings& s)
{
    std::ostringstream o;
    o << "music on=" << (s.on ? "on" : "off") << " volume=" << s.volume << " length=" << s.length << " source=" << (s.tracks ? "tracks" : "audio") << " units=" << s.units
      << " notes=" << (s.note_set == "notes2" ? "notes2/" + s.low + "/" + s.high + "/" + s.durations + "/" + std::to_string(s.voices) : std::string("notes104"))
      << " tempo=" << s.tempo << " voice=" << voice_id(s.voice) << " echo=" << s.echo << " gap=" << s.gap
      << " character=" << (s.character >= 0 && s.character < kMusicModes ? kModes[s.character].name : "none") << " filters=";
    for (size_t i = 0; i < s.filters.enabled.size(); ++i) o << (i ? "," : "") << s.filters.enabled[i];
    for (const auto& [f, vals] : s.filters.values)
        for (const auto& [n, v] : vals) o << " param=" << f << ":" << n << ":" << pct_encode(v);
    if (!s.melody.empty())
    {
        o << " melody=" << s.melody_set << ":" << s.melody_units << ":";
        for (size_t i = 0; i < s.melody.size(); ++i) o << (i ? "," : "") << s.melody[i];
    }
    for (const std::string& u : s.unknown) o << " " << u;
    return o.str();
}

std::optional<MusicSettings> personal_music_of(const std::string& line)
{
    std::istringstream in(line);
    std::string word;
    if (!(in >> word) || word != "music") return std::nullopt;
    // Every key it leaves out as WORLD's are at first (what a dimension played before it had music).
    MusicSettings s = defaults(MusicMode::World);
    s.character = -1;
    bool filters_seen = false;
    const auto ranged = [](const std::string& v, long long lo, long long hi) -> std::optional<long long> {
        const auto n = whole(v);
        if (!n || *n < lo || *n > hi) return std::nullopt;
        return n;
    };
    while (in >> word)
    {
        const size_t eq = word.find('=');
        if (eq == std::string::npos || eq == 0) return std::nullopt;
        const std::string k = word.substr(0, eq), v = word.substr(eq + 1);
        if (k == "on")
        {
            if (v != "on" && v != "off") return std::nullopt;
            s.on = v == "on";
        }
        else if (k == "source")
        {
            if (v != "audio" && v != "tracks") return std::nullopt;
            s.tracks = v == "tracks";
        }
        else if (k == "volume" || k == "length" || k == "units" || k == "tempo" || k == "echo" || k == "gap")
        {
            // (the Media Player's own bounds for each)
            const auto n = k == "volume" ? ranged(v, 0, 100) : k == "length" ? ranged(v, 1, 4096) : k == "units" ? ranged(v, 1, 64) : k == "tempo" ? ranged(v, 20, 300) : k == "echo" ? ranged(v, 0, 80) : ranged(v, 0, 600);
            if (!n) return std::nullopt;
            if (k == "volume") s.volume = int(*n);
            else if (k == "length") s.length = uint32_t(*n);
            else if (k == "units") s.units = uint32_t(*n);
            else if (k == "tempo") s.tempo = int(*n);
            else if (k == "echo") s.echo = int(*n);
            else s.gap = int(*n);
        }
        else if (k == "voice")
        {
            bool found = false;
            for (int i = 0; i < 4; ++i)
                if (v == voice_id(Voice(i))) s.voice = Voice(i), found = true;
            if (!found) return std::nullopt;
        }
        else if (k == "notes")
        {
            if (v == "notes104") s.note_set = "notes104";
            else
            {
                // notes2/LOW/HIGH/DURATIONS/VOICES, a set the Media Player could make
                std::vector<std::string> p;
                std::istringstream w(v);
                std::string part;
                while (std::getline(w, part, '/')) p.push_back(part);
                if (p.size() != 5 || p[0] != "notes2") return std::nullopt;
                const auto voices = ranged(p[4], 1, sieve::kMaxVoices);
                if (!voices) return std::nullopt;
                try
                {
                    (void)sieve::make_note_set(sieve::note_midi_of(p[1]), sieve::note_midi_of(p[2]), p[3], uint32_t(*voices));
                }
                catch (const std::exception&)
                {
                    return std::nullopt;
                }
                s.note_set = "notes2", s.low = p[1], s.high = p[2], s.durations = p[3], s.voices = uint32_t(*voices);
            }
        }
        else if (k == "character")
        {
            s.character = -2;
            if (v == "none") s.character = -1;
            for (int i = 0; i < kMusicModes; ++i)
                if (v == kModes[i].name) s.character = i;
            if (s.character == -2) return std::nullopt;
        }
        else if (k == "filters")
        {
            if (!filters_seen) s.filters = {};
            filters_seen = true;
            s.filters.enabled.clear();
            std::istringstream w(v);
            std::string name;
            while (std::getline(w, name, ','))
                if (!name.empty()) s.filters.enabled.push_back(name);
        }
        else if (k == "param")
        {
            // FILTER:KEY:VALUE, the value percent-encoded
            const size_t a = v.find(':'), b = a == std::string::npos ? a : v.find(':', a + 1);
            if (b == std::string::npos || a == 0 || b == a + 1) return std::nullopt;
            const auto val = pct_decode(v.substr(b + 1));
            if (!val) return std::nullopt;
            if (!filters_seen) s.filters = {};
            filters_seen = true;
            s.filters.values[v.substr(0, a)][v.substr(a + 1, b - a - 1)] = *val;
        }
        else if (k == "melody")
        {
            // SET:UNITS:D,D,... (the set's id, 0 or a track's units, its digits in decimal)
            const size_t a = v.find(':'), b = a == std::string::npos ? a : v.find(':', a + 1);
            if (b == std::string::npos) return std::nullopt;
            const std::string set_id = v.substr(0, a);
            const auto units = ranged(v.substr(a + 1, b - a - 1), 0, 64);
            if (!units) return std::nullopt;
            sieve::NoteSet set;
            try
            {
                set = sieve::note_set_of(set_id);
            }
            catch (const std::exception&)
            {
                return std::nullopt;
            }
            std::vector<uint32_t> digits;
            std::istringstream w(v.substr(b + 1));
            std::string d;
            while (std::getline(w, d, ','))
            {
                const auto n = ranged(d, 0, (long long)set.base() - 1);
                if (!n) return std::nullopt;
                digits.push_back(uint32_t(*n));
            }
            const size_t per = size_t(std::max<uint32_t>(1, uint32_t(*units))) * std::max<uint32_t>(1, set.voices);
            if (digits.empty() || digits.size() > 1u << 20 || digits.size() % per != 0) return std::nullopt;
            s.melody_set = set.id(), s.melody_units = uint32_t(*units), s.melody = std::move(digits);
        }
        else s.unknown.push_back(word); // (a later build's: kept and written back)
    }
    return s;
}

std::string personal_music_summary(const MusicSettings& s)
{
    std::string t = std::string(s.on ? "on" : "off") + ", volume " + std::to_string(s.volume) + "%, tempo " + std::to_string(s.tempo) + " bpm, " + voice_id(s.voice) + ", mode " +
                    (s.character >= 0 && s.character < kMusicModes ? kModes[s.character].name : "none") + ", ";
    t += s.melody.empty() ? std::to_string(s.filters.enabled.size()) + " filters" : "kept melody of " + std::to_string(s.melody.size()) + " notes";
    return t;
}

const char* voice_id(Voice v)
{
    switch (v)
    {
    case Voice::Soft: return "soft";
    case Voice::Sine: return "sine";
    case Voice::Triangle: return "triangle";
    case Voice::Square: return "square";
    }
    return "soft";
}

MusicPlayer* music() { return g_player; }
void set_music(MusicPlayer* p) { g_player = p; }

const char* music_mode_name(int i) { return kModes[std::clamp(i, 0, kMusicModes - 1)].name; }
const char* line_key(int li) { return theme_of(std::clamp(li, 0, kLines - 1)).key; }

void music_colours(SDL_Color bg, SDL_Color ink)
{
    std::lock_guard<std::mutex> lock(g_colour_mx);
    g_bg = bg;
    g_ink = ink;
}

void music_colours_default() { music_colours({0, 0, 0, 255}, {255, 255, 255, 255}); }

void music_mode(MusicMode m)
{
    if (g_player) g_player->set_mode(m);
}

void present(SDL_Renderer* r)
{
    if (g_player)
    {
        int w = 0, h = 0;
        SDL_RendererLogicalPresentation mode = SDL_LOGICAL_PRESENTATION_DISABLED;
        SDL_GetRenderLogicalPresentation(r, &w, &h, &mode);
        if (mode == SDL_LOGICAL_PRESENTATION_DISABLED || w <= 0 || h <= 0) SDL_GetCurrentRenderOutputSize(r, &w, &h);
        SDL_Color bg, ink;
        {
            std::lock_guard<std::mutex> lock(g_colour_mx);
            bg = g_bg;
            ink = g_ink;
        }
        g_player->draw_overlay(r, float(w), float(h), bg, ink);
    }
    SDL_RenderPresent(r);
}

std::string MusicMelody::notation() const { return sieve::notes_to_notation(set, notes); }

std::string MusicMelody::where() const
{
    const std::string m = tr(std::string("music.mode.") + mode_id(mode));
    return mode == MusicMode::World && line >= 0 && line < kLines ? m + " [" + tr(line_key(line)) + "]" : m;
}

void set_track_shape(const sieve::Space& cover, const std::optional<sieve::Space>& title)
{
    std::lock_guard<std::mutex> lock(g_shape_mx);
    g_cover = cover;
    g_title = title;
}

std::string MusicMelody::address() const
{
    if (notes.empty()) return "";
    if (units == 0)
    {
        const sieve::Space sp(set.id(), set.base(), uint32_t(notes.size()));
        return sp.hex_of(sp.address_digits(notes, sieve::AddressMode::Positional));
    }
    // A track: its units split from the joined notes, a blank cover and title, on the tracks line.
    std::optional<sieve::Space> cover, title;
    {
        std::lock_guard<std::mutex> lock(g_shape_mx);
        if (!g_cover)
        {
            g_cover = sieve::Space("image/mono/10x10", 2, 100);
            g_title = sieve::Space(sieve::alphabet_of("lower27"), 32);
        }
        cover = g_cover;
        title = g_title;
    }
    try
    {
        const sieve::Space unit(set.id(), set.base(), uint32_t(notes.size() / units));
        const sieve::CompositionSpace cs("tracks", *cover, title, unit, units);
        sieve::CompositionSpace::Parts p;
        p.cover.assign(cover->unit_length(), 0);
        if (title) p.title.assign(title->unit_length(), 0);
        p.units = sieve::split_units(notes, std::max<uint32_t>(1, set.voices), units);
        return cs.hex_of(cs.index_of(p, sieve::AddressMode::Positional));
    }
    catch (const std::exception&)
    {
        return "?";
    }
}

std::string MusicMelody::short_address(size_t n) const
{
    const std::string a = address();
    if (a.size() <= n || n < 4) return a;
    const size_t half = (n - 2) / 2;
    return a.substr(0, half) + ".." + a.substr(a.size() - half);
}

// ---------------------------------------------------------------- the player

MusicPlayer::MusicPlayer(const fs::path& folder, bool audio)
    : ini_path_(folder / "sieve-music.ini"), fav_path_(folder / "sieve-favourites.sieve")
{
    settings_[0] = defaults(MusicMode::Menus);
    settings_[1] = defaults(MusicMode::World);
    settings_[2] = defaults(MusicMode::World); // (PERSONAL's: unheard until a dimension with music is open)
    load_settings();
    recharacter();
    read_favourites();
    if (audio)
    {
        if (SDL_WasInit(SDL_INIT_AUDIO) || SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            const SDL_AudioSpec spec{SDL_AUDIO_F32, 1, kRate};
            stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &MusicPlayer::callback, this);
        }
        if (stream_) SDL_ResumeAudioStreamDevice(stream_);
        else status_ = std::string("audio unavailable: ") + SDL_GetError();
    }
    worker_ = std::thread([this] { worker(); });
}

MusicPlayer::~MusicPlayer()
{
    if (stream_) SDL_DestroyAudioStream(stream_); // stops the callback first
    stream_ = nullptr;
    {
        std::lock_guard<std::mutex> lock(mx_);
        quit_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    if (g_player == this) g_player = nullptr;
}

void MusicPlayer::set_mode(MusicMode m)
{
    std::lock_guard<std::mutex> lock(mx_);
    if (mode_ == m) return;
    mode_ = m;
    retarget();
    cv_.notify_all();
}

void MusicPlayer::set_line(int li)
{
    std::lock_guard<std::mutex> lock(mx_);
    if (line_ == li) return;
    line_ = std::clamp(li, 0, kLines - 1);
    recharacter();
}

// A space's pitch shifts: each note of the melody's scale (key-v1's, in the space's stack) moved to the
// same step of mode `target_mode` (one of the seven; -1 leaves every step where it is), on the same
// tonic, and the whole key moved `transpose` semitones.
static void shifts_for(const MusicSettings& s, int target_mode, int transpose, int* shift)
{
    // The melody's scale, read as the seven-note scale it comes from.
    const int* parent = nullptr;
    int tonic = 0;
    static const int ionian[7] = {0, 2, 4, 5, 7, 9, 11}, aeolian[7] = {0, 2, 3, 5, 7, 8, 10}, harmonic[7] = {0, 2, 3, 5, 7, 8, 11};
    // key-data-v2 (the plugin) or key-v1 (built in): the same tonics and scales, by the same names.
    const char* key = s.filters.is_enabled("key-data-v2") ? "key-data-v2" : s.filters.is_enabled("key-v1") ? "key-v1" : nullptr;
    if (key)
    {
        const auto it = s.filters.values.find(key);
        const auto value = [&](const char* k, const char* def) {
            if (it == s.filters.values.end()) return std::string(def);
            const auto v = it->second.find(k);
            return v == it->second.end() ? std::string(def) : v->second;
        };
        tonic = pitch_class(value("tonic", "C"));
        const std::string scale = value("scale", "major");
        if (scale == "major" || scale == "major-pentatonic") parent = ionian;
        else if (scale == "minor" || scale == "minor-pentatonic" || scale == "blues") parent = aeolian;
        else if (scale == "harmonic-minor") parent = harmonic;
    }
    if (target_mode < 0) parent = nullptr; // (no mode of its own: only the transposition, if any)
    const int* target = kModes[std::clamp(target_mode, 0, kMusicModes - 1)].steps;
    const sieve::NoteSet set = s.notes();
    for (int pitch = 1; pitch <= int(set.pitches()) && pitch < 62; ++pitch)
    {
        int delta = transpose;
        if (parent)
        {
            const int step = ((int(set.low) + pitch - 1 - tonic) % 12 + 12) % 12; // semitones above the tonic
            for (int d = 0; d < 7; ++d)
                if (parent[d] == step) delta += target[d] - parent[d];
        }
        shift[pitch] = delta;
    }
}

// WORLD's shifts for the line you are on (its mode, and the key round the circle of fifths if that is
// on), and PERSONAL's for the dimension's own mode. Taken up from the next note.
void MusicPlayer::recharacter()
{
    const MusicSettings& w = settings_[int(MusicMode::World)];
    shifts_for(w, w.modes[size_t(kDimensions[line_].media)], w.fifths ? fifths_at(line_) : 0, music_[int(MusicMode::World)].shift);
    const MusicSettings& p = settings_[int(MusicMode::Personal)];
    shifts_for(p, p.character, 0, music_[int(MusicMode::Personal)].shift);
}

void MusicPlayer::set_personal(const fs::path& world, const std::optional<MusicSettings>& s)
{
    std::lock_guard<std::mutex> lock(mx_);
    personal_world_ = world;
    personal_own_ = s.has_value();
    if (s)
    {
        settings_[2] = *s;
        Channel& c = music_[2];
        c.level = float(s->volume) / 100.0f;
        c.voice = s->voice;
        c.bpm = float(std::max(1, s->tempo));
        c.echo = float(s->echo) / 100.0f;
        if (c.ring.empty() && c.echo > 0) c.ring.assign(size_t(0.33f * kRate), 0.0f);
    }
    retry_at_[2] = 0;
    recharacter();
    retarget();
    cv_.notify_all();
}

fs::path MusicPlayer::personal_world() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return personal_world_;
}

std::optional<MusicSettings> MusicPlayer::personal() const
{
    std::lock_guard<std::mutex> lock(mx_);
    if (!personal_own_) return std::nullopt;
    return settings_[2];
}

int MusicPlayer::heard_locked() const { return mode_ == MusicMode::Personal && !personal_own_ ? int(MusicMode::World) : int(mode_); }

MusicMode MusicPlayer::heard() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return MusicMode(heard_locked());
}

MusicMode MusicPlayer::mode() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return mode_;
}

std::string MusicPlayer::play_item(const sieve::NoteSet& set, const std::vector<uint32_t>& notes)
{
    if (!stream_) return status_.empty() ? std::string("audio unavailable") : status_;
    std::lock_guard<std::mutex> lock(mx_);
    sound_.clear();
    load(item_, set, notes, 120, Voice::Square, 0, 1.0f);
    item_.gain = item_.target = 1;
    retarget();
    return "";
}

std::string MusicPlayer::play_sound(const sieve::PcmFormat& f, const std::vector<uint32_t>& samples)
{
    if (!stream_) return status_.empty() ? std::string("audio unavailable") : status_;
    // Each frame's channels added and scaled to -1..1 (divided by the channels, so it cannot clip).
    const size_t frames = samples.size() / f.channels;
    std::vector<float> mono(frames);
    const float full = float(uint64_t(1) << (f.bits - 1)) * float(f.channels);
    for (size_t i = 0; i < frames; ++i)
    {
        float v = 0;
        for (uint32_t ch = 0; ch < f.channels; ++ch) v += float(sieve::pcm_sample(f, samples[ch * frames + i]));
        mono[i] = v / full;
    }
    return play_samples(std::move(mono), f.rate);
}

std::string MusicPlayer::play_samples(std::vector<float> mono, uint32_t rate)
{
    if (!stream_) return status_.empty() ? std::string("audio unavailable") : status_;
    std::lock_guard<std::mutex> lock(mx_);
    item_.active = false;
    item_.done = true;
    sound_ = std::move(mono);
    sound_at_ = 0;
    sound_step_ = double(rate) / kRate;
    retarget();
    return "";
}

void MusicPlayer::stop_item()
{
    std::lock_guard<std::mutex> lock(mx_);
    if (!item_.active && sound_.empty()) return;
    sound_.clear();
    item_.active = false;
    item_.done = true;
    retarget();
    cv_.notify_all();
}

MusicSettings MusicPlayer::settings(MusicMode m) const
{
    std::lock_guard<std::mutex> lock(mx_);
    return settings_[int(m)];
}

void MusicPlayer::set_settings(MusicMode m, const MusicSettings& s)
{
    std::lock_guard<std::mutex> lock(mx_);
    MusicSettings& now = settings_[int(m)];
    // Volume, voice, tempo and echo change the melody playing; length, source and filters the next one.
    now = s;
    Channel& c = music_[int(m)];
    c.level = float(s.volume) / 100.0f;
    c.voice = s.voice;
    c.bpm = float(std::max(1, s.tempo));
    c.echo = float(s.echo) / 100.0f;
    if (c.ring.empty() && c.echo > 0) c.ring.assign(size_t(0.33f * kRate), 0.0f);
    retry_at_[int(m)] = 0;
    if (m == MusicMode::Personal) personal_own_ = true; // (its world file is the caller's to write)
    recharacter();
    retarget();
    if (m != MusicMode::Personal) save();
    cv_.notify_all();
}

void MusicPlayer::next(MusicMode m)
{
    std::lock_guard<std::mutex> lock(mx_);
    skip_[int(m)] = true;
    retry_at_[int(m)] = 0;
    cv_.notify_all();
}

void MusicPlayer::play(const MusicMelody& t)
{
    // In the mode where you are, so it is heard now, with that mode's sound; it keeps its own
    // mode's name.
    std::lock_guard<std::mutex> lock(mx_);
    const int m = heard_locked();
    const MusicSettings& s = settings_[m];
    load(music_[m], t.set, t.notes, float(s.tempo), s.voice, float(s.echo) / 100.0f, float(s.volume) / 100.0f);
    MusicMelody now = t;
    now.when = clock_now();
    playing_[m] = now;
    toast_ = trf(now.units ? "music.now_playing.track" : "music.now_playing", {now.where(), now.short_address(14)});
    toast_until_ = SDL_GetTicks() + 5000;
    retarget();
}

std::vector<MusicMelody> MusicPlayer::recent() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return recent_;
}

std::vector<MusicMelody> MusicPlayer::favourites() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return favourites_;
}

std::optional<MusicMelody> MusicPlayer::now_playing(MusicMode m) const
{
    std::lock_guard<std::mutex> lock(mx_);
    return playing_[int(m)];
}

std::vector<MusicPlayer::Sounding> MusicPlayer::sounding() const
{
    std::lock_guard<std::mutex> lock(mx_);
    std::vector<Sounding> out;
    auto from = [&](const Channel& c) {
        if (!c.active || c.done) return;
        for (const Part& p : c.parts)
        {
            if (p.note >= p.to || p.note >= c.notes.size()) continue;
            const uint32_t d = c.notes[p.note], midi = c.set.midi(d);
            if (!midi) continue; // a rest
            const uint32_t pitch = d / c.set.duration_count();
            out.push_back({float(int(midi) + c.shift[std::min<uint32_t>(pitch, 61)]), std::clamp(c.gain * c.level, 0.0f, 1.0f)});
        }
    };
    for (const Channel& c : music_) from(c);
    from(item_);
    return out;
}

std::string MusicPlayer::status() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return status_;
}

// Each music channel sounds while its mode is where the application is, the mode is on, and no
// melody in hand is playing; the gains then move to their targets in the mixer.
void MusicPlayer::retarget()
{
    const bool item = in_hand();
    const int heard = heard_locked();
    for (int m = 0; m < kMusicSpaces; ++m) music_[m].target = (m == heard && settings_[m].on && !item) ? 1.0f : 0.0f;
}

void MusicPlayer::load(Channel& c, const sieve::NoteSet& set, const std::vector<uint32_t>& notes, float bpm, Voice v, float echo, float level)
{
    c.notes = notes;
    c.set = set;
    c.bpm = std::max(1.0f, bpm);
    c.voice = v;
    c.echo = echo;
    c.level = level;
    c.ring.assign(echo > 0 ? size_t(0.33f * kRate) : 0, 0.0f);
    c.ring_pos = 0;
    c.tail = echo > 0 ? size_t(3 * kRate) : size_t(0.05f * kRate);
    c.gain = 0;
    c.active = true;
    c.done = false;
    // Every voice at once, each through its own stretch of the unit.
    const size_t voices = std::max<uint32_t>(1, set.voices), per = notes.size() / voices;
    c.parts.assign(voices, Part{});
    for (size_t p = 0; p < voices; ++p)
    {
        c.parts[p].from = c.parts[p].note = p * per;
        c.parts[p].to = p + 1 == voices ? notes.size() : (p + 1) * per;
        start_note(c, c.parts[p]);
    }
}

void MusicPlayer::start_note(Channel& c, Part& p)
{
    p.sample = 0;
    p.phase = 0;
    if (p.note >= p.to) return;
    const uint32_t d = c.notes[p.note];
    p.note_samples = std::max<size_t>(1, size_t(float(c.set.sixteenths(d)) / 4.0f * 60.0f / c.bpm * float(kRate)));
    const uint32_t pitch = d / c.set.duration_count(), midi = c.set.midi(d); // pitch 0 a rest
    p.freq = midi ? 440.0f * std::pow(2.0f, (float(int(midi) + c.shift[std::min<uint32_t>(pitch, 61)]) - 69.0f) / 12.0f) : 0.0f;
}

// Every voice summed (softened as they add up), then the echo.
float MusicPlayer::sample_of(Channel& c)
{
    float x = 0;
    bool sounding = false;
    for (Part& p : c.parts)
        if (p.note < p.to)
        {
            sounding = true;
            x += part_sample(c, p);
        }
    if (c.parts.size() > 1) x /= std::sqrt(float(c.parts.size()));
    if (!sounding)
    {
        if (c.tail > 0) --c.tail; // the echo dying away
        else
        {
            c.done = true;
            c.active = false;
            c.done_at = SDL_GetTicks();
            return 0;
        }
    }
    if (c.ring.empty()) return x;
    const float y = x + c.echo * c.ring[c.ring_pos];
    c.ring[c.ring_pos] = y;
    c.ring_pos = (c.ring_pos + 1) % c.ring.size();
    return y;
}

float MusicPlayer::part_sample(Channel& c, Part& p)
{
    float x = 0;
    {
        if (p.freq > 0)
        {
            const float t = float(p.sample) / kRate;
            const float left = float(p.note_samples - p.sample);
            p.phase += p.freq / kRate;
            if (p.phase >= 1) p.phase -= std::floor(p.phase);
            switch (c.voice)
            {
            case Voice::Soft:
            {
                // Struck, like a chime: a quick attack, then dying away towards a quiet sustain.
                const float env = std::min(1.0f, t / 0.01f) * (0.15f + 0.85f * std::exp(-3.0f * t)) * std::min(1.0f, left / (0.06f * kRate));
                x = 0.3f * env * (std::sin(2 * kPi * p.phase) + 0.12f * std::sin(4 * kPi * p.phase));
                break;
            }
            case Voice::Sine:
            {
                const float env = std::min(1.0f, t / 0.03f) * std::min(1.0f, left / (0.08f * kRate));
                x = 0.3f * env * std::sin(2 * kPi * p.phase);
                break;
            }
            case Voice::Triangle:
            {
                const float env = std::min(1.0f, t / 0.03f) * std::min(1.0f, left / (0.08f * kRate));
                x = 0.3f * env * (4.0f * std::fabs(p.phase - 0.5f) - 1.0f);
                break;
            }
            case Voice::Square:
            {
                // As the hallway always played a melody in hand: short attack and release, no clicks.
                const float env = std::fmin(1.0f, std::fmin(float(p.sample) / 200.0f, left / 800.0f));
                x = (p.phase < 0.5f ? 0.12f : -0.12f) * env;
                break;
            }
            }
        }
        if (++p.sample >= p.note_samples)
        {
            ++p.note;
            start_note(c, p);
        }
    }
    return x;
}

void MusicPlayer::mix(float* out, int n)
{
    std::lock_guard<std::mutex> lock(mx_);
    const float step = 1.0f / (kFadeSeconds * kRate);
    const bool item_before = in_hand();
    for (int i = 0; i < n; ++i)
    {
        float v = 0;
        for (Channel& c : music_)
        {
            if (!c.active || c.done) continue;
            if (c.gain <= 0 && c.target <= 0) continue; // faded out: waits where it is
            if (c.gain < c.target) c.gain = std::min(c.target, c.gain + step);
            else if (c.gain > c.target) c.gain = std::max(c.target, c.gain - step);
            v += sample_of(c) * c.gain * c.level;
        }
        if (item_.active && !item_.done) v += sample_of(item_) * item_.level;
        if (!sound_.empty())
        {
            // Between two samples of the sound, the straight line between them.
            const size_t k = size_t(sound_at_);
            const float t = float(sound_at_ - double(k));
            v += sound_[k] + (k + 1 < sound_.size() ? (sound_[k + 1] - sound_[k]) * t : 0.0f);
            sound_at_ += sound_step_;
            if (sound_at_ >= double(sound_.size())) sound_.clear();
        }
        out[i] = std::clamp(v, -1.0f, 1.0f);
    }
    if (item_before && !in_hand()) retarget(); // what was in hand ended: the music comes back
}

void SDLCALL MusicPlayer::callback(void* user, SDL_AudioStream* stream, int additional, int)
{
    auto* self = static_cast<MusicPlayer*>(user);
    float buf[1024];
    int left = additional / int(sizeof(float));
    while (left > 0)
    {
        const int n = std::min(left, 1024);
        self->mix(buf, n);
        SDL_PutAudioStreamData(stream, buf, n * int(sizeof(float)));
        left -= n;
    }
}

// ---------------------------------------------------------------- choosing melodies

std::optional<std::vector<uint32_t>> MusicPlayer::pick(const MusicSettings& s, std::string& why) const
{
    try
    {
        const sieve::NoteSet set = s.notes();
        // A track's units joined are one unit of the audio line N units long, and the filters
        // judge that, as a track's JOINED stack does.
        const sieve::cli::Line line = s.tracks ? sieve::cli::joined_line(audio_line(set, s.length), s.units) : audio_line(set, s.length);
        const sieve::FilterStack st = sieve::cli::build_stack(line, s.filters);
        std::mt19937_64 rng(std::random_device{}() ^ uint64_t(SDL_GetTicksNS()));
        auto any_unit = [&] {
            std::vector<uint32_t> u(line.space.unit_length());
            for (auto& d : u) d = uint32_t(rng() % set.base());
            return u;
        };
        // A withheld unit (the vault) is never played: another is drawn instead.
        for (int tries = 0; tries < 16; ++tries)
        {
            std::optional<std::vector<uint32_t>> u;
            if (st.empty()) u = any_unit();
            else if (const sieve::Ranker* rk = st.ranker())
            {
                const BigUint& count = rk->count();
                if (count.is_zero())
                {
                    why = tr("music.none_pass");
                    return std::nullopt;
                }
                // A rank drawn uniformly below the count: 64 bits more than it needs, reduced,
                // so the bias is below one part in 2^64.
                std::vector<uint32_t> limbs((count.bit_length() + 64 + 31) / 32);
                for (auto& l : limbs) l = uint32_t(rng());
                u = rk->unrank(BigUint::mod(BigUint::from_limbs(limbs), count));
            }
            else
            {
                // A stack that judges only: draw until something passes.
                for (int i = 0; i < 5000 && !u; ++i)
                {
                    auto c = any_unit();
                    if (st.passes(c)) u = std::move(c);
                }
                if (!u)
                {
                    why = tr("music.none_found");
                    return std::nullopt;
                }
            }
            if (!sieve::cli::unit_withheld(line, *u)) return u;
        }
        why = tr("music.none_found");
    }
    catch (const std::exception& e)
    {
        why = e.what();
    }
    return std::nullopt;
}

void MusicPlayer::worker()
{
    std::unique_lock<std::mutex> lk(mx_);
    while (!quit_)
    {
        cv_.wait_for(lk, std::chrono::milliseconds(200));
        if (quit_) break;
        if (!stream_) continue; // no sound: nothing to choose
        const int m = heard_locked();
        const Channel& c = music_[m];
        const MusicSettings& s = settings_[m];
        const bool item = in_hand();
        const Uint64 now = SDL_GetTicks();
        // The first melody straight away; the next after the quiet between melodies.
        const bool due = skip_[m] || (c.done && (c.notes.empty() || now >= c.done_at + Uint64(std::max(0, s.gap)) * 1000));
        if (!s.on || item || !due || now < retry_at_[m]) continue;
        skip_[m] = false;
        const MusicSettings chosen_with = s;
        std::optional<std::vector<uint32_t>> notes;
        std::string why;
        // A melody the dimension keeps (PERSONAL): that one, every time, not one drawn.
        const bool kept = m == int(MusicMode::Personal) && !s.melody.empty();
        if (kept) notes = s.melody;
        else
        {
            lk.unlock();
            notes = pick(chosen_with, why);
            lk.lock();
        }
        if (quit_) break;
        if (!notes)
        {
            status_ = why;
            retry_at_[m] = SDL_GetTicks() + 30000;
            continue;
        }
        status_.clear();
        const MusicSettings& cur = settings_[m];
        const sieve::NoteSet set = kept ? set_of(chosen_with.melody_set) : chosen_with.notes();
        load(music_[m], set, *notes, float(cur.tempo), cur.voice, float(cur.echo) / 100.0f, float(cur.volume) / 100.0f);
        MusicMelody t;
        t.mode = MusicMode(m);
        t.set = set;
        t.line = t.mode == MusicMode::World ? line_ : -1;
        t.when = clock_now();
        t.notes = *notes;
        t.units = kept ? chosen_with.melody_units : chosen_with.tracks ? chosen_with.units : 0;
        playing_[m] = t;
        recent_.insert(recent_.begin(), t);
        if (recent_.size() > kRecent) recent_.resize(kRecent);
        if (m == heard_locked())
        {
            toast_ = trf(t.units ? "music.now_playing.track" : "music.now_playing", {t.where(), t.short_address(14)});
            toast_until_ = SDL_GetTicks() + 5000;
        }
        retarget();
        save();
    }
}

// ---------------------------------------------------------------- the box

void MusicPlayer::draw_overlay(SDL_Renderer* r, float W, float, SDL_Color bg, SDL_Color ink) const
{
    std::string msg;
    {
        std::lock_guard<std::mutex> lock(mx_);
        if (SDL_GetTicks() >= toast_until_) return;
        msg = toast_;
    }
    const std::string head = tr("music.box");
    const float w = std::max(text_width(msg, 1), text_width(head, 2)) + 24, h = 48, x = W - w - 6, y = 73; // below the hallway's two lines of readout, the FPS counter and the setup menu's budget
    SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(r, bg.r, bg.g, bg.b, 235);
    const SDL_FRect box{x, y, w, h};
    SDL_RenderFillRect(r, &box);
    SDL_SetRenderDrawColor(r, ink.r, ink.g, ink.b, 255);
    SDL_RenderRect(r, &box);
    draw_text(r, x + 12, y + 8, head, 2, ink);
    draw_text(r, x + 12, y + 30, msg, 1, ink);
}

// ---------------------------------------------------------------- kept between runs

void MusicPlayer::load_settings()
{
    std::ifstream in(ini_path_);
    if (!in) return;
    std::string line, section;
    bool filters_seen[2] = {false, false};
    while (std::getline(in, line))
    {
        const size_t semi = line.find(';');
        if (semi != std::string::npos) line = line.substr(0, semi);
        line = trim(line);
        if (line.empty()) continue;
        if (line.front() == '[' && line.back() == ']')
        {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = trim(line.substr(0, eq)), v = trim(line.substr(eq + 1));
        try
        {
            if (section == "recent" && (k == "melody" || k == "track")) // ("track" before tracks were a line)
            {
                // mode when [tracks:N] [set] notation... (tracks:N for a track of N units; the set
                // named when it is not notes104)
                std::istringstream w(v);
                std::string mode, when;
                w >> mode >> when;
                std::string rest;
                std::getline(w, rest);
                rest = trim(rest);
                uint32_t units = 0;
                if (rest.rfind("tracks:", 0) == 0)
                {
                    const size_t sp = rest.find(' ');
                    units = uint32_t(std::stoul(rest.substr(7, sp - 7)));
                    rest = sp == std::string::npos ? "" : trim(rest.substr(sp));
                }
                sieve::NoteSet set;
                if (rest.rfind("notes2/", 0) == 0)
                {
                    const size_t sp = rest.find(' ');
                    set = set_of(rest.substr(0, sp));
                    rest = sp == std::string::npos ? "" : trim(rest.substr(sp));
                }
                if (auto notes = notes_of(set, rest))
                    if (recent_.size() < kRecent)
                    {
                        MusicMelody t;
                        t.set = set;
                        where_from(mode, t);
                        t.when = when;
                        t.notes = *notes;
                        if (units && t.notes.size() % (size_t(units) * std::max<uint32_t>(1, set.voices)) == 0) t.units = units;
                        recent_.push_back(t);
                    }
                continue;
            }
            const int m = section == "menus" ? 0 : section == "world" ? 1 : -1;
            if (m < 0) continue;
            MusicSettings& s = settings_[m];
            if (k == "on") s.on = v == "on";
            else if (k == "volume") s.volume = std::clamp(std::stoi(v), 0, 100);
            else if (k == "length") s.length = uint32_t(std::clamp(std::stoi(v), 1, 4096));
            else if (k == "source") s.tracks = v == "tracks";
            else if (k == "units") s.units = uint32_t(std::clamp(std::stoi(v), 1, 64));
            else if (k == "notes")
            {
                // notes = notes104, or notes = notes2 LOW HIGH DURATIONS VOICES
                std::istringstream w(v);
                std::string set, lo, hi, durs;
                uint32_t voices = 1;
                w >> set >> lo >> hi >> durs >> voices;
                s.note_set = set == "notes2" ? "notes2" : "notes104";
                if (s.note_set == "notes2")
                {
                    s.low = lo;
                    s.high = hi;
                    s.durations = durs;
                    s.voices = std::clamp<uint32_t>(voices, 1, sieve::kMaxVoices);
                }
            }
            else if (k == "tempo") s.tempo = std::clamp(std::stoi(v), 20, 300);
            else if (k == "voice") s.voice = voice_from(v);
            else if (k == "echo") s.echo = std::clamp(std::stoi(v), 0, 80);
            else if (k == "gap") s.gap = std::clamp(std::stoi(v), 0, 600);
            else if (k == "fifths") s.fifths = v == "on";
            else if (k == "modes")
            {
                // modes = each medium's mode by name, in Media's order (dimensions.hpp), not the doors'
                std::istringstream w(v);
                std::string name;
                size_t at = 0;
                while (std::getline(w, name, ',') && at < s.modes.size())
                {
                    for (int i = 0; i < kMusicModes; ++i)
                        if (trim(name) == kModes[i].name) s.modes[at] = i;
                    ++at;
                }
            }
            else if (k == "filters")
            {
                if (!filters_seen[m]) s.filters = {};
                filters_seen[m] = true;
                std::istringstream w(v);
                std::string name;
                while (std::getline(w, name, ','))
                    if (!trim(name).empty()) s.filters.enabled.push_back(trim(name));
            }
            else if (k == "param")
            {
                // param = FILTER NAME VALUE
                std::istringstream w(v);
                std::string f, n, val;
                w >> f >> n;
                std::getline(w, val);
                if (!filters_seen[m])
                {
                    s.filters = {};
                    filters_seen[m] = true;
                }
                s.filters.values[f][n] = trim(val);
            }
        }
        catch (const std::exception&)
        {
            // A value that is not a number keeps its default.
        }
    }
}

void MusicPlayer::save() const
{
    std::ostringstream o;
    o << "; Sieve hallway - the music player's settings and its last ten melodies (client/music.hpp).\n"
         "; Written by the Media Player in the pause menu; edit it with the hallway closed.\n";
    for (int m = 0; m < 2; ++m) // (MENUS and WORLD: PERSONAL's are the dimension's, in its world file)
    {
        const MusicSettings& s = settings_[m];
        o << "\n[" << mode_id(MusicMode(m)) << "]\n"
          << "on = " << (s.on ? "on" : "off") << "\n"
          << "volume = " << s.volume << "\n"
          << "length = " << s.length << "\n"
          << "source = " << (s.tracks ? "tracks" : "audio") << "\n"
          << "units = " << s.units << "\n"
          << "notes = " << (s.note_set == "notes2" ? "notes2 " + s.low + " " + s.high + " " + s.durations + " " + std::to_string(s.voices) : std::string("notes104")) << "\n"
          << "tempo = " << s.tempo << "\n"
          << "voice = " << voice_id(s.voice) << "\n"
          << "echo = " << s.echo << "\n"
          << "gap = " << s.gap << "\n";
        if (m == int(MusicMode::World))
        {
            o << "modes = ";
            for (size_t i = 0; i < s.modes.size(); ++i) o << (i ? ", " : "") << kModes[s.modes[i]].name;
            o << "   ;";
            for (size_t i = 0; i < s.modes.size(); ++i) o << (i ? ", " : " ") << kDimensions[line_of(Media(i))].id;
            o << "\n"
              << "fifths = " << (s.fifths ? "on" : "off") << "\n";
        }
        o << "filters = ";
        for (size_t i = 0; i < s.filters.enabled.size(); ++i) o << (i ? ", " : "") << s.filters.enabled[i];
        o << "\n";
        for (const auto& [f, vals] : s.filters.values)
            for (const auto& [n, v] : vals) o << "param = " << f << " " << n << " " << v << "\n";
    }
    o << "\n[recent]\n";
    for (const MusicMelody& t : recent_)
        o << "melody = " << where_id(t) << " " << (t.when.empty() ? "--:--" : t.when) << " " << (t.units ? "tracks:" + std::to_string(t.units) + " " : std::string())
          << (t.set.legacy ? std::string() : t.set.id() + " ") << t.notation() << "\n";
    std::ofstream out(ini_path_, std::ios::binary);
    out << o.str();
}

// Favourites: Sieve instructions (the newest version, as its address) holding favourites.tsv
// (one line a melody: file, mode, when, notes, its note set and, for a track, its units) and
// each melody as a MIDI file.
void MusicPlayer::write_favourites(const std::vector<MusicMelody>& favs) const
{
    std::error_code ec;
    if (favs.empty())
    {
        fs::remove(fav_path_, ec);
        return;
    }
    std::vector<std::pair<std::string, std::vector<uint8_t>>> files;
    std::string index;
    for (size_t i = 0; i < favs.size(); ++i)
    {
        char name[32];
        std::string where = where_id(favs[i]);
        std::replace(where.begin(), where.end(), '/', '-');
        std::snprintf(name, sizeof name, "%03zu-%s.mid", i + 1, where.c_str());
        const std::string midi = sieve::notes_to_midi(favs[i].set, favs[i].notes);
        files.emplace_back(name, std::vector<uint8_t>(midi.begin(), midi.end()));
        // file, where, when, notes, then the note set when it is not notes104 and, for a track,
        // tracks:N (with an empty set field before it on notes104)
        index += std::string(name) + "\t" + where_id(favs[i]) + "\t" + (favs[i].when.empty() ? "--:--" : favs[i].when) + "\t" + favs[i].notation() +
                 (favs[i].set.legacy && !favs[i].units ? std::string() : "\t" + (favs[i].set.legacy ? std::string() : favs[i].set.id())) +
                 (favs[i].units ? "\ttracks:" + std::to_string(favs[i].units) : std::string()) + "\n";
    }
    files.emplace_back("favourites.tsv", std::vector<uint8_t>(index.begin(), index.end()));
    // Laid out as a folder named favourites, then packed as the newest instructions are (a moment's
    // search a file: they are small, and written whenever one is added or taken away).
    std::random_device rd;
    const fs::path work = fs::temp_directory_path() / ("sieve-favourites-" + std::to_string(rd()) + std::to_string(rd()));
    const fs::path dir = work / "favourites";
    struct Cleanup
    {
        fs::path p;
        ~Cleanup()
        {
            std::error_code e;
            fs::remove_all(p, e);
        }
    } cleanup{work};
    try
    {
        fs::create_directories(dir);
        for (const auto& [name, bytes] : files)
        {
            std::ofstream f(dir / name, std::ios::binary);
            f.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
        }
        sieve::cli::Manifest m = sieve::cli::walk_folder(dir);
        sieve::cli::add_packed_newest(m, dir, nullptr, 0.25, false, false);
        sieve::cli::write_raw_address_file(fav_path_, sieve::cli::raw_of_file(m.file()), false);
    }
    catch (const std::exception&)
    {
        // (Not saved this time: the ones before stay as they were.)
    }
}

void MusicPlayer::read_favourites()
{
    std::error_code ec;
    if (!fs::exists(fav_path_, ec)) return;
    try
    {
        // Any version: those written before 27 October 2026 are sieve-manifest-v3, the newer the newest.
        const sieve::cli::Manifest m = sieve::cli::installable_manifest(fav_path_, false);
        size_t at = 0;
        for (const auto& e : m.entries)
        {
            if (e.dir) continue;
            if (e.path == "favourites.tsv" && at + e.size <= m.contents.size())
            {
                std::istringstream in(std::string(m.contents.begin() + long(at), m.contents.begin() + long(at + e.size)));
                std::string l;
                while (std::getline(in, l))
                {
                    std::istringstream w(l);
                    std::string file, mode, when, notation, set_id;
                    if (!std::getline(w, file, '\t') || !std::getline(w, mode, '\t') || !std::getline(w, when, '\t') || !std::getline(w, notation, '\t')) continue;
                    std::getline(w, set_id, '\t');
                    std::string units_field;
                    std::getline(w, units_field);
                    const sieve::NoteSet set = set_id.empty() ? sieve::NoteSet{} : set_of(set_id);
                    if (auto notes = notes_of(set, notation))
                    {
                        MusicMelody t;
                        t.set = set;
                        where_from(mode, t);
                        t.when = when;
                        t.notes = *notes;
                        if (units_field.rfind("tracks:", 0) == 0)
                        {
                            const uint32_t units = uint32_t(std::stoul(units_field.substr(7)));
                            if (units && t.notes.size() % (size_t(units) * std::max<uint32_t>(1, set.voices)) == 0) t.units = units;
                        }
                        favourites_.push_back(t);
                    }
                }
            }
            at += e.size;
        }
    }
    catch (const std::exception& e)
    {
        status_ = trf("music.fav_unreadable", {e.what()});
    }
}

std::string MusicPlayer::add_favourite(const MusicMelody& t)
{
    std::lock_guard<std::mutex> lock(mx_);
    for (const MusicMelody& f : favourites_)
        if (f.notes == t.notes) return tr("music.fav_already");
    if (sieve::cli::unit_withheld(audio_line(t.set, uint32_t(t.notes.size() / std::max<uint32_t>(1, t.set.voices))), t.notes)) return tr("music.withheld");
    auto next = favourites_;
    next.push_back(t);
    try
    {
        write_favourites(next);
    }
    catch (const std::exception& e)
    {
        return e.what();
    }
    favourites_ = std::move(next);
    return "";
}

std::string MusicPlayer::remove_favourite(size_t i)
{
    std::lock_guard<std::mutex> lock(mx_);
    if (i >= favourites_.size()) return "";
    auto next = favourites_;
    next.erase(next.begin() + long(i));
    try
    {
        write_favourites(next);
    }
    catch (const std::exception& e)
    {
        return e.what();
    }
    favourites_ = std::move(next);
    return "";
}

} // namespace hallway
