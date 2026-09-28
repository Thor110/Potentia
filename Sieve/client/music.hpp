// Sieve hallway — the music player: melodies from the audio line, played quietly behind
// everything, chosen by filters.
//
// There are two modes, each with its own settings and its own stack of audio filters: MENUS (every
// menu and screen that is not the world: the main menu, the setup menu, the designer, the pause
// menu and the tools opened from it) and WORLD (walking the hallway). A track is one unit of the
// audio line (notes104) at the mode's own length, picked at random from the survivors of the
// mode's stack: the stack counts them, a rank is drawn uniformly below the count, and the unit at
// that rank is played. So every track is on a shelf, with an address, and nothing is searched for.
// With no filter ticked a track is any unit at all; with a stack that cannot count (a judge-only
// filter) units are drawn at random until one passes, a few thousand tries at most.
//
// How a track sounds is the player's, not the track's: its tempo, its voice (the shape of the
// tone: soft, sine, triangle or square) and its echo. The unit, its address and a saved track
// (MIDI, as the hallway saves any melody) are the same whatever they are set to.
//
// So is each line's character, in WORLD: every line plays in a mode of its own, brightest to
// darkest Lydian, Ionian, Mixolydian, Dorian, Aeolian, Phrygian, Locrian (by default image,
// pages, audio, video, books, models, binary, in that order of brightness). A note is moved from
// its step of the track's scale to the same step of the line's mode, on the same tonic; the scale
// and tonic are those of the key filter in WORLD's stack (key-data-v2, or key-v1), so without one the modes do nothing. Scales
// of fewer than seven notes are read as the seven-note scale they come from (the major
// pentatonic as the major scale's steps 1 2 3 5 6), and then lack the steps some modes change:
// on the major pentatonic, Lydian, Ionian and Mixolydian sound the same. Optionally the key
// follows the line round the circle of fifths too (pages C, image G, audio D, video A, books E,
// models B, binary F#, each folded within six semitones). A change of line takes effect from the
// next note. MENUS belongs to no line and is never moved.
//
// One audio stream carries everything, mixed on SDL's audio thread: a channel per mode and one for
// the melody in hand (P in the hallway, played as before, square and at 120 bpm). Moving between
// the menus and the world crossfades from one mode's channel to the other's, which waits where it
// was and picks up there when its mode comes back; playing a melody in hand fades the music out,
// and it fades back in when the melody ends or is put down. Tracks are chosen on a worker thread
// (a large plugin can take seconds to compile), with a few seconds of quiet between them.
//
// The last ten tracks the player chose are listed (recent), with the mode they played in and when;
// tracks you play yourself are not. Favourites are kept as Sieve instructions, a .sieve holding
// each favourite as a MIDI file and an index (favourites.tsv) of their notes, so the file installs
// with `sieve install` like any other. Settings and the recent list are kept in sieve-music.ini
// beside the hallway's settings.
#pragma once

#include "cli/filter_config.hpp"

#include <SDL3/SDL.h>

#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>

namespace hallway {

enum class MusicMode { Menus = 0, World = 1 };
enum class Voice { Soft = 0, Sine, Triangle, Square };
const char* voice_id(Voice v); // "soft", "sine", "triangle", "square"

struct MusicSettings
{
    bool on = true;
    int volume = 30;       // percent
    uint32_t length = 32;  // notes in a track
    int tempo = 80;        // quarter notes a minute
    Voice voice = Voice::Soft;
    int echo = 25;         // percent fed back
    int gap = 5;           // seconds of quiet between tracks
    sieve::cli::LineFilters filters;
    // WORLD only: each line's mode (an index into music_modes(), brightest first), and whether the
    // key follows the line round the circle of fifths.
    int modes[7] = {1, 0, 2, 3, 4, 5, 6};
    bool fifths = false;
};

// The seven modes, brightest first ("lydian" ... "locrian"), and the lines' names ("line.pages"
// ... "line.binary": language keys), in the hallway's order of lines.
const char* music_mode_name(int i);
inline constexpr int kMusicModes = 7;
const char* line_key(int li);

struct MusicTrack
{
    MusicMode mode = MusicMode::World;
    int line = -1;                // WORLD: the line it began on (the hallway's numbering), else -1
    std::string when;             // "14:05", when it began
    std::vector<uint32_t> notes;  // the unit (notes104), its length the track's
    std::string notation() const;
    std::string address() const;  // positional, in hex, on the audio line of its length
    std::string where() const;    // "WORLD [BINARY]", "MENUS"
};

class MusicPlayer
{
public:
    // `folder`: where sieve-music.ini and sieve-favourites.sieve are kept. `audio`: false for
    // scripted runs, which draw the player but make no sound.
    MusicPlayer(const std::filesystem::path& folder, bool audio);
    ~MusicPlayer();
    MusicPlayer(const MusicPlayer&) = delete;
    MusicPlayer& operator=(const MusicPlayer&) = delete;

    // Where the application is: the menus or the world. Crossfades when it changes.
    void set_mode(MusicMode m);
    MusicMode mode() const;
    // The line you are on (the hallway, on entering it and at every door): WORLD's character.
    void set_line(int li);

    // The melody in hand: square tones at 120 bpm, over the music, which fades out under it.
    std::string play_item(const std::vector<uint32_t>& notes);
    void stop_item();

    MusicSettings settings(MusicMode m) const;
    void set_settings(MusicMode m, const MusicSettings& s); // applied at once, saved
    void next(MusicMode m);                    // a new track now
    void play(const MusicTrack& t);            // a listed track, now, where you are (not listed again)

    std::vector<MusicTrack> recent() const;    // newest first
    std::vector<MusicTrack> favourites() const;
    std::string add_favourite(const MusicTrack& t); // "" or why not
    std::string remove_favourite(size_t i);
    std::filesystem::path favourites_path() const { return fav_path_; }

    std::optional<MusicTrack> now_playing(MusicMode m) const;
    std::string status() const;                // what went wrong last, if anything
    bool audio() const { return stream_ != nullptr; }

    // The "Now playing" box, for a few seconds after a track begins: every screen draws it last.
    // In the colours of where you are: the line's background and edges in the hallway, white on
    // black in the menus.
    void draw_overlay(SDL_Renderer* r, float W, float H, SDL_Color bg, SDL_Color ink) const;

private:
    struct Channel
    {
        std::vector<uint32_t> notes;
        float bpm = 120, echo = 0, gain = 0, target = 0, level = 1;
        Voice voice = Voice::Square;
        size_t note = 0, sample = 0, note_samples = 0;
        float freq = 0, phase = 0;
        int shift[26] = {}; // semitones each pitch is moved by (the line's character)
        std::vector<float> ring; // the echo's delay line
        size_t ring_pos = 0, tail = 0;
        bool active = false, done = true;
        Uint64 done_at = 0;
    };
    static void SDLCALL callback(void* user, SDL_AudioStream* stream, int additional, int total);
    void mix(float* out, int n);
    float sample_of(Channel& c);
    void start_note(Channel& c);
    void load(Channel& c, const std::vector<uint32_t>& notes, float bpm, Voice v, float echo, float level);
    void retarget();                              // under mx_
    void recharacter();                           // under mx_: WORLD's shifts, for the line and settings
    void worker();
    std::optional<std::vector<uint32_t>> pick(const MusicSettings& s, std::string& why) const;
    void save() const;                            // under mx_
    void load_settings();
    void write_favourites(const std::vector<MusicTrack>& favs) const;
    void read_favourites();

    std::filesystem::path ini_path_, fav_path_;
    SDL_AudioStream* stream_ = nullptr;
    mutable std::mutex mx_;
    std::condition_variable cv_;
    std::thread worker_;
    bool quit_ = false;
    MusicMode mode_ = MusicMode::Menus;
    int line_ = 0;
    MusicSettings settings_[2];
    Channel music_[2], item_;
    bool skip_[2] = {false, false};
    std::optional<MusicTrack> playing_[2];
    std::vector<MusicTrack> recent_, favourites_;
    std::string status_, toast_;
    Uint64 toast_until_ = 0, retry_at_[2] = {0, 0};
};

// The one player, for every screen (null when there is none: tests and scripted pictures of
// screens other than the player's own).
MusicPlayer* music();
void set_music(MusicPlayer* p);

// SDL_RenderPresent, with the "Now playing" box drawn over the frame first (at the renderer's
// logical size when a screen draws at one). Every screen presents through this.
void present(SDL_Renderer* r);
// Where the application is, for the player (nothing when there is no player).
void music_mode(MusicMode m);
// The colours the box is drawn in: the hallway sets its line's every frame; the menus set white
// on black (the default).
void music_colours(SDL_Color bg, SDL_Color ink);
void music_colours_default();

} // namespace hallway
