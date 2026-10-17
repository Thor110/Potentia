// Sieve hallway — the setup menu's settings: every line's shape, and the GLOBAL settings the lines
// share. They come from sieve-hallway.ini's [setup] section (app_settings.hpp), then from the
// hallway's own options, which win; the setup menu edits them and they are saved back to [setup].
// They live in a header of their own because both the setup menu and the settings file need them.
#pragma once

#include "display.hpp"     // kCloseUpScreen
#include "cli/args.hpp"
#include "cli/lines.hpp"   // kDefaultExportFps

#include <cstdint>
#include <string>
#include <vector>

namespace hallway {

struct Settings
{
    std::string start_line = "text";
    std::string mode = "positional"; // positional | scrambled | guided
    std::string key = "sieve";
    uint32_t length = 32;
    std::string alphabet = "lower27";
    std::string canon = "v2";
    bool model = true; // use the alphabet's default model (guided ordering)
    uint32_t image_w = 10, image_h = 10;
    std::string image_palette = "mono";
    std::string image_fit = "levels"; // how pictures from files are fitted: levels (canon-image-v2) or average (v1)
    uint32_t notes = 16; // events per book (per voice, on notes2)
    // The audio line's note set: notes104 (fixed: C4-C6, e q h w, one voice), or notes2 with its
    // range, durations (codes of s e E q Q h H w) and voices (sieve/audio.hpp).
    std::string note_set = "notes104", note_low = "C3", note_high = "C6", note_durations = "seEqQhHw";
    uint32_t voices = 1;
    // The notes3 set (sieve/notes3.hpp): open-ended notes. Levels, tempo and instruments are set
    // in the settings file only.
    std::string n3_low = "C-1", n3_high = "G9", n3_instruments = "0";
    uint32_t n3_tpq = 4, n3_longest = 16, n3_levels = 8, n3_voices = 1, n3_tempo = 120;
    // The pcm set (sieve/sound.hpp): sound itself, `samples` a channel per book.
    uint32_t samples = 8000, pcm_rate = 8000, pcm_bits = 8, pcm_channels = 1;
    uint32_t video_w = 5, video_h = 5, frames = 8;
    std::string video_palette = "mono";
    std::string video_fit = "levels";
    // The ffmpeg that reads picture and video formats stb_image does not (cli/media_decode.hpp);
    // empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH.
    std::string ffmpeg;
    uint32_t video_fps = sieve::cli::kDefaultExportFps; // a video saved as a video through ffmpeg: frames a second
    uint32_t book_pages = 4; // books: a cover (image line), a title and this many pages (pages line)
    // Tracks and movies (sieve/composition.hpp): a cover, a title and this many units of audio, of video.
    uint32_t track_units = 4, movie_units = 4;
    // models: V vertices and F triangles, each coordinate one of C steps across [-1, 1]
    uint32_t model_vertices = 8, model_faces = 12, model_coords = 16;
    // Worlds (sieve/worldspace.hpp): a cover, a title and this many models, each in a cell of a
    // grid this many cells along each axis, with one of 24 turns.
    uint32_t world_models = 4, world_grid = 8;
    // AI (sieve/aispace.hpp): every language model of this shape, numbered by its weights: layers,
    // width, heads (dividing the width into heads of an even size) and bits a weight.
    uint32_t ai_layers = 1, ai_width = 16, ai_heads = 2, ai_bits = 4;
    // How big the picture on the front of a crate is drawn, pixels square (a power of two).
    uint32_t model_tile = 64;
    // Global, because the corridor is shared: how many units stand on one tile's two walls.
    // A power of two, so a power-of-two line fills whole tiles (sieve/corridor.hpp).
    uint32_t items_per_wall = 128;
    // Global too: how long every line's titles are, in the pages line's alphabet (SPECIFICATIONS
    // §11, "Titled lines"). The books line keeps a page as its title.
    uint32_t title_length = 32;
    uint32_t letters_px = 8;   // the letter size item displays are drawn for; smaller letters are dashes
    uint32_t closeup_px = kCloseUpScreen; // the widest close-up display (0: none; display.hpp)
    uint32_t binary_bytes = 32; // the binary line: every file up to this many bytes
    // Which line FIND MY LIMITS grows: "all", or one line's name, the others left as they are.
    std::string limits_focus = "all";

    bool operator==(const Settings&) const = default;

    // The settings the hallway's options give (--line, --length, ...), and `base` for the rest:
    // the defaults, or what sieve-hallway.ini's [setup] holds.
    static Settings from_args(const sieve::cli::Args& a, const Settings& base);
    static Settings from_args(const sieve::cli::Args& a); // over the defaults
    // Writes these settings into `a` (as the hallway's own options), keeping everything else.
    void apply(sieve::cli::Args& a) const;

    // sieve-hallway.ini's [setup] section: every setting above but ffmpeg (which is [paths]'s), in
    // the setup menu's order, each under its option's name with '_' for '-', its value and a line
    // saying what it is. The values are as apply() writes them, but for the model ("default" or
    // "none").
    struct Entry
    {
        std::string key, value, note;
    };
    std::vector<Entry> entries() const;
    // One [setup] entry read from the file, over these settings: false, and nothing changed, if
    // the key is not one of entries()' or the value is not one the setting can have, so that a
    // mistyped line leaves the setting as it was rather than stopping the hallway from starting.
    bool set_entry(const std::string& key, const std::string& value);
};

} // namespace hallway
