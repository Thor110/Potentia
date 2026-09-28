// Sieve hallway — the music player (music.hpp): the mixer on SDL's audio thread, the worker that
// chooses tracks by the modes' filter stacks, and the settings, recent tracks and favourites kept
// between runs.
//
// The mixer synthesises as it goes, a sample at a time, rather than rendering a track before it
// plays: a slow track of long notes with an echo would be minutes of samples to hold. Each channel
// knows its note, how far into it it is, its phase and its echo's delay line; a music channel also
// has a gain that moves towards its target over a second and a half, which is all the fading
// there is. A music channel whose gain has reached nothing waits where it is.

#include "music.hpp"

#include "font.hpp"
#include "strings.hpp"

#include "cli/lines.hpp"
#include "cli/locate.hpp"
#include "sieve/audio.hpp"
#include "sieve/space.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <fstream>
#include <map>
#include <random>
#include <sstream>

namespace hallway {

namespace fs = std::filesystem;
using sieve::BigUint;

namespace {

constexpr int kRate = 44100;
constexpr float kPi = 3.14159265358979f;
constexpr float kFadeSeconds = 1.5f;
constexpr size_t kRecent = 10;
constexpr float kQuarters[4] = {0.5f, 1.0f, 2.0f, 4.0f}; // e q h w

MusicPlayer* g_player = nullptr;
std::mutex g_colour_mx; // the hallway may set them while it is built, on a worker
SDL_Color g_bg{0, 0, 0, 255}, g_ink{255, 255, 255, 255}; // the box's colours: where you are

const char* mode_id(MusicMode m) { return m == MusicMode::Menus ? "menus" : "world"; }

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
constexpr const char* kLineKeys[7] = {"line.pages", "line.image", "line.audio", "line.video", "line.books", "line.models", "line.binary"};
constexpr const char* kLineIds[7] = {"pages", "image", "audio", "video", "books", "models", "binary"};
// The key each line is in when the key follows the line: a fifth further round the circle at each
// door (C G D A E B F#), folded within six semitones of C.
constexpr int kFifths[7] = {0, -5, 2, -3, 4, -1, 6};

// "world/binary" and back: where a track played, in the files.
std::string where_id(const MusicTrack& t)
{
    return std::string(mode_id(t.mode)) + (t.mode == MusicMode::World && t.line >= 0 && t.line < 7 ? std::string("/") + kLineIds[t.line] : "");
}
void where_from(const std::string& id, MusicTrack& t)
{
    const size_t slash = id.find('/');
    t.mode = id.substr(0, slash) == "menus" ? MusicMode::Menus : MusicMode::World;
    t.line = -1;
    if (slash != std::string::npos)
        for (int i = 0; i < 7; ++i)
            if (id.substr(slash + 1) == kLineIds[i]) t.line = i;
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

std::string trim(const std::string& s)
{
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    return s.substr(a, s.find_last_not_of(" \t\r\n") - a + 1);
}

MusicSettings defaults(MusicMode m)
{
    // MENUS is the serious one: a minor key, lower and slower, a plain sine. WORLD is the relaxing
    // one: the major pentatonic, soft struck tones and more echo. Both are starting points to be
    // tuned by ear in the Media Player.
    MusicSettings s;
    s.filters.enabled = {"key-v1"};
    if (m == MusicMode::Menus)
    {
        s.volume = 25;
        s.tempo = 66;
        s.voice = Voice::Sine;
        s.echo = 20;
        s.gap = 6;
        s.filters.values["key-v1"] = {{"tonic", "A"}, {"scale", "minor"}};
    }
    else
    {
        s.volume = 30;
        s.tempo = 72;
        s.voice = Voice::Soft;
        s.echo = 35;
        s.gap = 8;
        // The major scale, not the pentatonic, so every line's mode is heard (the pentatonic lacks
        // the steps Lydian and Mixolydian change).
        s.filters.values["key-v1"] = {{"tonic", "C"}, {"scale", "major"}};
    }
    return s;
}

Voice voice_from(const std::string& s)
{
    for (int v = 0; v < 4; ++v)
        if (s == voice_id(Voice(v))) return Voice(v);
    return Voice::Soft;
}

// A notation read back into its unit (the whole notation is one unit, its length the track's).
std::optional<std::vector<uint32_t>> notes_of(const std::string& notation)
{
    std::istringstream in(notation);
    std::string tok;
    uint32_t n = 0;
    while (in >> tok) ++n;
    if (n == 0) return std::nullopt;
    try
    {
        const auto c = sieve::canonicalise_notes(notation, n);
        if (c.units.size() != 1) return std::nullopt;
        return c.units.front();
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}

sieve::cli::Line audio_line(uint32_t length)
{
    sieve::cli::Args la;
    la.opts["line"] = "audio";
    la.opts["length"] = std::to_string(length);
    return sieve::cli::make_line(la);
}

} // namespace

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
const char* line_key(int li) { return kLineKeys[std::clamp(li, 0, 6)]; }

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

std::string MusicTrack::notation() const { return sieve::notes_to_notation(notes); }

std::string MusicTrack::where() const
{
    const std::string m = tr(std::string("music.mode.") + mode_id(mode));
    return mode == MusicMode::World && line >= 0 && line < 7 ? m + " [" + tr(kLineKeys[line]) + "]" : m;
}

std::string MusicTrack::address() const
{
    if (notes.empty()) return "";
    const sieve::Space sp(sieve::kNotesSymbolsId, sieve::kNoteSymbols, uint32_t(notes.size()));
    return sp.hex_of(sp.address_digits(notes, sieve::AddressMode::Positional));
}

// ---------------------------------------------------------------- the player

MusicPlayer::MusicPlayer(const fs::path& folder, bool audio)
    : ini_path_(folder / "sieve-music.ini"), fav_path_(folder / "sieve-favourites.sieve")
{
    settings_[0] = defaults(MusicMode::Menus);
    settings_[1] = defaults(MusicMode::World);
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
    line_ = std::clamp(li, 0, 6);
    recharacter();
}

// WORLD's pitch shifts for the line you are on: each note of the track's scale (key-v1's, in
// WORLD's stack) moved to the same step of the line's mode, on the same tonic, and the whole key
// round the circle of fifths if that is on. Taken up from the next note.
void MusicPlayer::recharacter()
{
    const MusicSettings& s = settings_[int(MusicMode::World)];
    int* shift = music_[int(MusicMode::World)].shift;
    const int transpose = s.fifths ? kFifths[line_] : 0;
    // The track's scale, read as the seven-note scale it comes from.
    const int* parent = nullptr;
    int tonic = 0;
    static const int ionian[7] = {0, 2, 4, 5, 7, 9, 11}, aeolian[7] = {0, 2, 3, 5, 7, 8, 10}, harmonic[7] = {0, 2, 3, 5, 7, 8, 11};
    if (s.filters.is_enabled("key-v1"))
    {
        const auto it = s.filters.values.find("key-v1");
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
    const int* target = kModes[std::clamp(s.modes[line_], 0, kMusicModes - 1)].steps;
    for (int pitch = 1; pitch <= 25; ++pitch)
    {
        int delta = transpose;
        if (parent)
        {
            const int step = ((60 + pitch - 1 - tonic) % 12 + 12) % 12; // semitones above the tonic
            for (int d = 0; d < 7; ++d)
                if (parent[d] == step) delta += target[d] - parent[d];
        }
        shift[pitch] = delta;
    }
}

MusicMode MusicPlayer::mode() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return mode_;
}

std::string MusicPlayer::play_item(const std::vector<uint32_t>& notes)
{
    if (!stream_) return status_.empty() ? std::string("audio unavailable") : status_;
    std::lock_guard<std::mutex> lock(mx_);
    load(item_, notes, 120, Voice::Square, 0, 1.0f);
    item_.gain = item_.target = 1;
    retarget();
    return "";
}

void MusicPlayer::stop_item()
{
    std::lock_guard<std::mutex> lock(mx_);
    if (!item_.active) return;
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
    // Volume, voice, tempo and echo change the track playing; length and filters the next one.
    now = s;
    Channel& c = music_[int(m)];
    c.level = float(s.volume) / 100.0f;
    c.voice = s.voice;
    c.bpm = float(std::max(1, s.tempo));
    c.echo = float(s.echo) / 100.0f;
    if (c.ring.empty() && c.echo > 0) c.ring.assign(size_t(0.33f * kRate), 0.0f);
    retry_at_[int(m)] = 0;
    if (m == MusicMode::World) recharacter();
    retarget();
    save();
    cv_.notify_all();
}

void MusicPlayer::next(MusicMode m)
{
    std::lock_guard<std::mutex> lock(mx_);
    skip_[int(m)] = true;
    retry_at_[int(m)] = 0;
    cv_.notify_all();
}

void MusicPlayer::play(const MusicTrack& t)
{
    // In the mode where you are, so it is heard now, with that mode's sound; it keeps its own
    // mode's name.
    std::lock_guard<std::mutex> lock(mx_);
    const int m = int(mode_);
    const MusicSettings& s = settings_[m];
    load(music_[m], t.notes, float(s.tempo), s.voice, float(s.echo) / 100.0f, float(s.volume) / 100.0f);
    MusicTrack now = t;
    now.when = clock_now();
    playing_[m] = now;
    toast_ = trf("music.now_playing", {now.where(), now.address().substr(0, 12)});
    toast_until_ = SDL_GetTicks() + 5000;
    retarget();
}

std::vector<MusicTrack> MusicPlayer::recent() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return recent_;
}

std::vector<MusicTrack> MusicPlayer::favourites() const
{
    std::lock_guard<std::mutex> lock(mx_);
    return favourites_;
}

std::optional<MusicTrack> MusicPlayer::now_playing(MusicMode m) const
{
    std::lock_guard<std::mutex> lock(mx_);
    return playing_[int(m)];
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
    const bool item = item_.active && !item_.done;
    for (int m = 0; m < 2; ++m) music_[m].target = (m == int(mode_) && settings_[m].on && !item) ? 1.0f : 0.0f;
}

void MusicPlayer::load(Channel& c, const std::vector<uint32_t>& notes, float bpm, Voice v, float echo, float level)
{
    c.notes = notes;
    c.bpm = std::max(1.0f, bpm);
    c.voice = v;
    c.echo = echo;
    c.level = level;
    c.note = 0;
    c.ring.assign(echo > 0 ? size_t(0.33f * kRate) : 0, 0.0f);
    c.ring_pos = 0;
    c.tail = echo > 0 ? size_t(3 * kRate) : size_t(0.05f * kRate);
    c.gain = 0;
    c.active = true;
    c.done = false;
    start_note(c);
}

void MusicPlayer::start_note(Channel& c)
{
    c.sample = 0;
    c.phase = 0;
    if (c.note >= c.notes.size()) return;
    const uint32_t d = c.notes[c.note];
    c.note_samples = std::max<size_t>(1, size_t(kQuarters[d % 4] * 60.0f / c.bpm * float(kRate)));
    const uint32_t pitch = d / 4; // 0 a rest, 1 C4 ... 25 C6
    c.freq = pitch ? 440.0f * std::pow(2.0f, (float(60 + int(pitch) - 1 + c.shift[pitch]) - 69.0f) / 12.0f) : 0.0f;
}

float MusicPlayer::sample_of(Channel& c)
{
    float x = 0;
    if (c.note < c.notes.size())
    {
        if (c.freq > 0)
        {
            const float t = float(c.sample) / kRate;
            const float left = float(c.note_samples - c.sample);
            c.phase += c.freq / kRate;
            if (c.phase >= 1) c.phase -= std::floor(c.phase);
            switch (c.voice)
            {
            case Voice::Soft:
            {
                // Struck, like a chime: a quick attack, then dying away towards a quiet sustain.
                const float env = std::min(1.0f, t / 0.01f) * (0.15f + 0.85f * std::exp(-3.0f * t)) * std::min(1.0f, left / (0.06f * kRate));
                x = 0.3f * env * (std::sin(2 * kPi * c.phase) + 0.12f * std::sin(4 * kPi * c.phase));
                break;
            }
            case Voice::Sine:
            {
                const float env = std::min(1.0f, t / 0.03f) * std::min(1.0f, left / (0.08f * kRate));
                x = 0.3f * env * std::sin(2 * kPi * c.phase);
                break;
            }
            case Voice::Triangle:
            {
                const float env = std::min(1.0f, t / 0.03f) * std::min(1.0f, left / (0.08f * kRate));
                x = 0.3f * env * (4.0f * std::fabs(c.phase - 0.5f) - 1.0f);
                break;
            }
            case Voice::Square:
            {
                // As the hallway always played a melody in hand: short attack and release, no clicks.
                const float env = std::fmin(1.0f, std::fmin(float(c.sample) / 200.0f, left / 800.0f));
                x = (c.phase < 0.5f ? 0.12f : -0.12f) * env;
                break;
            }
            }
        }
        if (++c.sample >= c.note_samples)
        {
            ++c.note;
            start_note(c);
        }
    }
    else if (c.tail > 0) --c.tail; // the echo dying away
    else
    {
        c.done = true;
        c.active = false;
        c.done_at = SDL_GetTicks();
        return 0;
    }
    if (c.ring.empty()) return x;
    const float y = x + c.echo * c.ring[c.ring_pos];
    c.ring[c.ring_pos] = y;
    c.ring_pos = (c.ring_pos + 1) % c.ring.size();
    return y;
}

void MusicPlayer::mix(float* out, int n)
{
    std::lock_guard<std::mutex> lock(mx_);
    const float step = 1.0f / (kFadeSeconds * kRate);
    const bool item_before = item_.active && !item_.done;
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
        out[i] = std::clamp(v, -1.0f, 1.0f);
    }
    if (item_before && item_.done) retarget(); // the melody in hand ended: the music comes back
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

// ---------------------------------------------------------------- choosing tracks

std::optional<std::vector<uint32_t>> MusicPlayer::pick(const MusicSettings& s, std::string& why) const
{
    try
    {
        const sieve::cli::Line line = audio_line(s.length);
        const sieve::FilterStack st = sieve::cli::build_stack(line, s.filters);
        std::mt19937_64 rng(std::random_device{}() ^ uint64_t(SDL_GetTicksNS()));
        auto any_unit = [&] {
            std::vector<uint32_t> u(s.length);
            for (auto& d : u) d = uint32_t(rng() % sieve::kNoteSymbols);
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
        const int m = int(mode_);
        const Channel& c = music_[m];
        const MusicSettings& s = settings_[m];
        const bool item = item_.active && !item_.done;
        const Uint64 now = SDL_GetTicks();
        // The first track straight away; the next after the quiet between tracks.
        const bool due = skip_[m] || (c.done && (c.notes.empty() || now >= c.done_at + Uint64(std::max(0, s.gap)) * 1000));
        if (!s.on || item || !due || now < retry_at_[m]) continue;
        skip_[m] = false;
        const MusicSettings chosen_with = s;
        lk.unlock();
        std::string why;
        const auto notes = pick(chosen_with, why);
        lk.lock();
        if (quit_) break;
        if (!notes)
        {
            status_ = why;
            retry_at_[m] = SDL_GetTicks() + 30000;
            continue;
        }
        status_.clear();
        const MusicSettings& cur = settings_[m];
        load(music_[m], *notes, float(cur.tempo), cur.voice, float(cur.echo) / 100.0f, float(cur.volume) / 100.0f);
        MusicTrack t;
        t.mode = MusicMode(m);
        t.line = t.mode == MusicMode::World ? line_ : -1;
        t.when = clock_now();
        t.notes = *notes;
        playing_[m] = t;
        recent_.insert(recent_.begin(), t);
        if (recent_.size() > kRecent) recent_.resize(kRecent);
        if (m == int(mode_))
        {
            toast_ = trf("music.now_playing", {t.where(), t.address().substr(0, 12)});
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
    const float w = std::max(text_width(msg, 1), text_width(head, 2)) + 24, h = 48, x = W - w - 12, y = 52; // below the hallway's two lines of readout
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
            if (section == "recent" && k == "track")
            {
                // mode when notation...
                std::istringstream w(v);
                std::string mode, when;
                w >> mode >> when;
                std::string rest;
                std::getline(w, rest);
                if (auto notes = notes_of(trim(rest)))
                    if (recent_.size() < kRecent)
                    {
                        MusicTrack t;
                        where_from(mode, t);
                        t.when = when;
                        t.notes = *notes;
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
            else if (k == "tempo") s.tempo = std::clamp(std::stoi(v), 20, 300);
            else if (k == "voice") s.voice = voice_from(v);
            else if (k == "echo") s.echo = std::clamp(std::stoi(v), 0, 80);
            else if (k == "gap") s.gap = std::clamp(std::stoi(v), 0, 600);
            else if (k == "fifths") s.fifths = v == "on";
            else if (k == "modes")
            {
                // modes = the seven lines' modes by name, in the lines' order
                std::istringstream w(v);
                std::string name;
                int li = 0;
                while (std::getline(w, name, ',') && li < 7)
                {
                    for (int i = 0; i < kMusicModes; ++i)
                        if (trim(name) == kModes[i].name) s.modes[li] = i;
                    ++li;
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
    o << "; Sieve hallway - the music player's settings and its last ten tracks (client/music.hpp).\n"
         "; Written by the Media Player in the pause menu; edit it with the hallway closed.\n";
    for (int m = 0; m < 2; ++m)
    {
        const MusicSettings& s = settings_[m];
        o << "\n[" << mode_id(MusicMode(m)) << "]\n"
          << "on = " << (s.on ? "on" : "off") << "\n"
          << "volume = " << s.volume << "\n"
          << "length = " << s.length << "\n"
          << "tempo = " << s.tempo << "\n"
          << "voice = " << voice_id(s.voice) << "\n"
          << "echo = " << s.echo << "\n"
          << "gap = " << s.gap << "\n";
        if (m == int(MusicMode::World))
        {
            o << "modes = ";
            for (int li = 0; li < 7; ++li) o << (li ? ", " : "") << kModes[s.modes[li]].name;
            o << "   ; pages, image, audio, video, books, models, binary\n"
              << "fifths = " << (s.fifths ? "on" : "off") << "\n";
        }
        o << "filters = ";
        for (size_t i = 0; i < s.filters.enabled.size(); ++i) o << (i ? ", " : "") << s.filters.enabled[i];
        o << "\n";
        for (const auto& [f, vals] : s.filters.values)
            for (const auto& [n, v] : vals) o << "param = " << f << " " << n << " " << v << "\n";
    }
    o << "\n[recent]\n";
    for (const MusicTrack& t : recent_) o << "track = " << where_id(t) << " " << (t.when.empty() ? "--:--" : t.when) << " " << t.notation() << "\n";
    std::ofstream out(ini_path_, std::ios::binary);
    out << o.str();
}

// Favourites: Sieve instructions (a sieve-manifest-v3, as its address) holding favourites.tsv
// (one line a track: file, mode, when, notes) and each track as a MIDI file.
void MusicPlayer::write_favourites(const std::vector<MusicTrack>& favs) const
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
        const std::string midi = sieve::notes_to_midi(favs[i].notes);
        files.emplace_back(name, std::vector<uint8_t>(midi.begin(), midi.end()));
        index += std::string(name) + "\t" + where_id(favs[i]) + "\t" + (favs[i].when.empty() ? "--:--" : favs[i].when) + "\t" + favs[i].notation() + "\n";
    }
    files.emplace_back("favourites.tsv", std::vector<uint8_t>(index.begin(), index.end()));
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    sieve::cli::Manifest m;
    m.root = "favourites";
    m.with_contents = true;
    for (const auto& [name, bytes] : files)
    {
        sieve::cli::ManifestEntry e;
        e.path = name;
        e.size = bytes.size();
        e.sha256 = sieve::cli::sha256_hex(bytes);
        m.entries.push_back(e);
        m.files += 1;
        m.bytes += bytes.size();
        m.contents.insert(m.contents.end(), bytes.begin(), bytes.end());
    }
    sieve::cli::write_address_file(fav_path_, sieve::cli::binary_address(m.file()), false);
}

void MusicPlayer::read_favourites()
{
    std::error_code ec;
    if (!fs::exists(fav_path_, ec)) return;
    try
    {
        const std::vector<uint8_t> whole = sieve::cli::file_at(sieve::cli::read_address_file(fav_path_, false));
        const sieve::cli::Manifest m = sieve::cli::Manifest::parse(std::string_view(reinterpret_cast<const char*>(whole.data()), whole.size()));
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
                    std::string file, mode, when, notation;
                    if (!std::getline(w, file, '\t') || !std::getline(w, mode, '\t') || !std::getline(w, when, '\t') || !std::getline(w, notation)) continue;
                    if (auto notes = notes_of(notation))
                    {
                        MusicTrack t;
                        where_from(mode, t);
                        t.when = when;
                        t.notes = *notes;
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

std::string MusicPlayer::add_favourite(const MusicTrack& t)
{
    std::lock_guard<std::mutex> lock(mx_);
    for (const MusicTrack& f : favourites_)
        if (f.notes == t.notes) return tr("music.fav_already");
    if (sieve::cli::unit_withheld(audio_line(uint32_t(t.notes.size())), t.notes)) return tr("music.withheld");
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
