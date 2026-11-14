// Sieve hallway -- the program: its options, the menus, the screenshot and scripting paths used by
// the checks, and the event loop. The hallway itself is in hallway.hpp and the files beside it.

#include "raid_service.hpp"
#include "personal_world.hpp"
#include "keys.hpp"
#include "press_names.hpp"
#include "replay.hpp"
#include "sieve/paths.hpp"
#include "sieve/sha256.hpp"
#include "window_icon.hpp"
#include "win_console.hpp"
#include "cli/media_decode.hpp"
#include "cli/plugins.hpp"
#include "cli/map_sieve.hpp"
#include "cli/sieve_cache.hpp"
#include "cipher_screen.hpp"
#include "manual_nav.hpp"
#include "designer.hpp"
#include "harness.hpp"
#include "cli/items.hpp"
#include "cli/gather.hpp"
#include "speech.hpp"
#include "cli/draft.hpp"
#include "cli/pack.hpp"
#include "cli/dictionaries.hpp" // executable_dir (--metrics-text)
#include "sieve/mix.hpp"       // Prioritise Decoding Speed
#include "sieve/rope_wave.hpp" // the rope's search through every wave
#include "cli/locate.hpp" // read_file_bytes (--cipher-in)
#include "sieve/llm.hpp"
#include "cli/timings.hpp"
#include "hallway.hpp"
#include "sieve/plugin.hpp"
#include "sieve/sound.hpp"

#include <SDL3/SDL_main.h>

#include <cstdlib>
#include <fstream>
#include "sieve/character.hpp"
#include "character_store.hpp"
#include <sstream>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <functional>
#include <iterator>
#include <map>
#include <optional>
#include <random>
#include <set>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

using namespace hallway;
using namespace hallway::hall;

namespace {

bool save_render(SDL_Renderer* r, const std::string& path)
{
    SDL_Surface* s = SDL_RenderReadPixels(r, nullptr);
    if (!s) return false;
    SDL_Surface* rgb = SDL_ConvertSurface(s, SDL_PIXELFORMAT_RGB24);
    SDL_DestroySurface(s);
    if (!rgb) return false;
    std::vector<Rgb> px(size_t(rgb->w) * rgb->h);
    for (int y = 0; y < rgb->h; ++y)
    {
        const auto* row = static_cast<const Uint8*>(rgb->pixels) + size_t(y) * rgb->pitch;
        for (int x = 0; x < rgb->w; ++x) px[size_t(y) * rgb->w + x] = {row[x * 3], row[x * 3 + 1], row[x * 3 + 2]};
    }
    write_png(path, uint32_t(rgb->w), uint32_t(rgb->h), px, 1);
    SDL_DestroySurface(rgb);
    return true;
}

// ---------------------------------------------------------------- options

const char* kUsage =
    "hallway - walk the Sieve's lines in 3D\n\n"
    "Usage: hallway [options]\n\n"
    "Lines (the same meaning as in the sieve tool):\n"
    "  --line LINE         the line to start in: image, pages (default), books, audio, tracks,\n"
    "                      video, movies, models, worlds, ai or binary\n"
    "  --length L          text: characters per book (default 32)\n"
    "  --alphabet ID       text: lower27 (default), babel29, ascii95\n"
    "  --canon v2|v1       text: warp rules (default v2)\n"
    "  --image-width W  --image-height H  --image-palette ID    image line (10, 10, mono)\n"
    "  --image-fit levels|average  --video-fit levels|average   how pictures from files are fitted:\n"
    "                      canon-image-v2's levels (default) or v1's plain average\n"
    "  --notes N           audio: notes per book (default 16; per voice on notes2)\n"
    "  --note-set notes104|notes2|notes3|pcm  --note-low C3  --note-high C6  --note-durations seEqQhHw  --voices 1..4\n"
    "                      audio: the note set (default notes104; low to voices are notes2's)\n"
    "  --notes3-low C-1  --notes3-high G9  --notes3-tpq 4  --notes3-longest 16  --notes3-levels 8\n"
    "  --notes3-voices 1..15  --notes3-tempo 120  --notes3-instruments 0[,0...]\n"
    "                      audio, --note-set notes3: open-ended notes (lengths in ticks, loudness levels)\n"
    "  --samples N  --pcm-rate HZ  --pcm-bits B  --pcm-channels C\n"
    "                      audio, --note-set pcm: sound itself, N samples a channel per book\n"
    "                      (default 8000), at 8000 a second, 8 bits (1-31), 1 channel\n"
    "  --video-width W  --video-height H  --video-frames F  --video-palette ID   video (5, 5, 8, mono)\n"
    "  --title-length T    every line's titles: characters (default 32; 0: no titles; books keep a page)\n"
    "  --binary-length N   binary: every file of up to N bytes (default 32)\n"
    "  --book-pages N      books: pages per book (default 4); a book is a cover (an image of the\n"
    "                      image line), a title and N pages (pages of the pages line)\n"
    "  --track-units N     tracks: units of audio per track (default 4); a track is a cover, a title\n"
    "                      and N units of the audio line (composition-v1)\n"
    "  --movie-units N     movies: units of video per movie (default 4), as tracks are of audio\n"
    "  --world-models N    worlds: models per world (default 4); a world is a cover, a title and N\n"
    "                      models of the models line, each in a cell of the grid with one of 24 turns\n"
    "  --world-grid G      worlds: cells along each axis of a world (default 8; worldspace-v1)\n"
    "  --vertices V  --faces F  --coords C\n"
    "                      models: the shape of every mesh of the models line (default 8 vertices,\n"
    "                      12 triangles, a grid of 16 steps a coordinate; modelspace-v1), as sieve mesh\n"
    "                      takes them\n"
    "  --ai-layers L  --ai-width H  --ai-heads A  --ai-bits B\n"
    "                      AI: every language model of this shape (default 1 layer, width 16, 2 heads, 4 bits a\n"
    "                      weight; aispace-v1), numbered by its weights: take one and talk to it (Enter)\n"
    "  --ffmpeg PATH       the ffmpeg that reads and writes picture, video and sound formats beyond\n"
    "                      PNG, JPEG, BMP, GIF, TGA, WAV and MIDI (default: ffmpeg in the settings' [paths],\n"
    "                      then SIEVE_FFMPEG, then beside the hallway, then the PATH)\n"
    "  --video-fps N       a video saved as GIF, MP4 or WebM (with ffmpeg): frames a second (default 8)\n"
    "  --key K             scramble key (default sieve)\n"
    "  --mode positional|scrambled|guided   starting ordering (default positional)\n"
    "  --model ID|PATH|none  text: model for the guided ordering (default: the alphabet's default)\n"
    "  --zoom D            guided: books 2^-D of the line apart (after --warp/--goto set it)\n\n"
    "Start:\n"
    "  --warp INPUT        warp on start (text, notes, a picture file for image/video, a .book file for books)\n"
    "  --goto ADDR|P%|@T   go to an address, a percentage, or corridor tile T on start\n\n"
    "Menu:\n"
    "  The main menu opens first: Start Sieve, the File Locator (as in the pause menu; Go to it builds\n"
    "  the hallway and walks to the file), the Media Player, the Filter Designer (make filter plugins\n"
    "  as nodes), the AI Training Harness (train a model of the AI line from a recorded run, and check\n"
    "  it comes out the same), Settings (graphics, controls, language; saved to sieve-hallway.ini), the\n"
    "  Encryption Engine, Manual Navigation (a file's bytes turned by hand) and Exit Sieve. Start Sieve opens the setup menu\n"
    "  (Esc: back to the main menu): adjust every line's state space and see the lines as a map.\n"
    "  The three dots after a line's name in the map (or I) open its details: its size, address\n"
    "  length, walk time, tiles, filters and cost here.\n"
    "  The magnifying glass beside a line's title (or F) opens its filters: tick filters, set\n"
    "  their parameters, and choose the mode: off, mark (failures faint), hide (failures left\n"
    "  out), compact (only survivors, packed together, in every ordering) or excluded (only\n"
    "  failures, in their places, to check what the filters set aside). Saved to the\n"
    "  --filters file. In compact mode, G takes a compact address, as the books show it.\n"
    "  --no-menu           go straight into the hallway (F1 opens the setup menu from the hallway)\n"
    "  --settings PATH     application settings (default: sieve-hallway.ini next to the executable):\n"
    "                      every setting there is, the setup menu's among them; the options win over it\n"
    "  --write-settings OUT  write the settings (--settings PATH's, else the defaults) to OUT, and stop\n"
    "  --language CODE     menu language for this run (a file in the lang folder, e.g. en)\n"
    "  --limits-focus LINE the line the setup menu's FIND MY LIMITS grows (default all: every line)\n"
    "  --optimise-search   the File Locator's Optimise Search ticked (as it is unless unticked): Try every\n"
    "                      way chosen for each file by its kind ([locator] optimise_search)\n"
    "  --search-every-wave the File Locator's Try every way ticked, for this run: the search's\n"
    "                      shortcuts off, up to its time ([locator] search_every_wave)\n"
    "  --sound-temporary   the File Locator's Sound as FLAC: Temporary, for this run (v34; the default,\n"
    "                      [locator] sound): only sound Sieve makes again byte for byte (WAV and the like as\n"
    "                      FLAC, MP3 as packMP3 codes it), an archive's too\n"
    "  --sound-permanent   Sound as FLAC: Permanent, for this run: also installed as .flac where smaller (WAV\n"
    "                      and the like; WavPack, Monkey's Audio, TTA and ALAC through ffmpeg); a FLAC re-coded\n"
    "                      (--sound-to-flac is the same)\n"
    "  --sound-off         Sound as FLAC: Off, for this run: sound left to the ways every file has\n"
    "  --more-formats      the File Locator's More formats ticked, for this run: the comparison with\n"
    "                      gzip, bzip2, zstd, brotli and lz4 beside zip and 7z (slower)\n"
    "  --priority WHAT     the File Locator's Size, Balanced or Decoding speed, for this run: size, balanced\n"
    "                      (mixing only where it saves 4% of what it carries; the File Locator's default)\n"
    "                      or speed (sieve's command line itself defaults to size)\n"
    "                      ([locator] priority)\n"
    "  --prioritise-decoding  as --priority speed: no file mixed, so its instructions install quickly\n"
    "  --v35-draft         the next version's draft in the File Locator, for this run (testing only: its\n"
    "                      name follows the version in the making, docs/NEXT-VERSION.md; [locator] v35_draft;\n"
    "                      --v34-draft, from before v34 was frozen, is still accepted and changes nothing)\n"
    "  --locate-log        the File Locator's Keep a log ticked, for this run: each reading and saving\n"
    "                      writes its reports and times to logs/ beside the settings ([locator] keep_log)\n"
    "  --menu              with --screenshot: a picture of the setup menu (--press keys go to it)\n"
    "  --character FILE    your character from a .character file (any version): beside you in the\n"
    "                      hallway, and on the setup menu's CHARACTER page\n"
    "  --no-character      no character beside you in the hallway for this run\n"
    "  --import-model FILE with --menu: a model (.obj, .gltf, .glb) into your character's first sphere, as\n"
    "                      the CHARACTER page's Ctrl+I does (brought under the held models' roof of 8\n"
    "                      vertices, 12 faces; what it leaves out is said). --import-obj FILE: the same\n"
    "  --import-skeleton FILE  with --menu: a glTF's skeleton as your character (sieve-character-v3), its\n"
    "                      first animation playing on it, as the CHARACTER page's Ctrl+K does\n"
    "  --catalogue-entry ID  with --menu: the right-hand panel open on CATALOGUE, standing at entry ID (IDEAS\n"
    "                      19.165): the catalogue of formats as a ring of doors\n"
    "  --rings NOTES       with --screenshot: the Media Player's rings (R there), those MIDI notes (by\n"
    "                      commas, 60 the middle C) held sounding; --rings-zoom N steps in (0 to 2)\n"
    "  --personal-world    with --screenshot: your personal dimension's world (a grassy hill, your characters),\n"
    "                      --pworld-walk S walked first, --press to its catalogue (Tab, Up, Down, Return,\n"
    "                      Backspace; E picks up what the crosshair is on, R/T/G turn it, Return places it, Q\n"
    "                      drops it; Tab while holding opens Place: exact numbers), --pworld-look DEG looks up or\n"
    "                      down first, --pworld-wait S lets what was dropped fall, --no-map and --no-compass\n"
    "                      leave those out (as Personal Dimensions > Features can), --pworld-back S walked\n"
    "                      back after; --world-file FILE what is placed\n"
    "  --open-favourite SHA  a favourite (kept beside --settings) opened as your dimension's O opens it: gone\n"
    "                      to on its line and taken into your hand (give --line its line)\n"
    "  --record FILE       record what you do in the hallway (sieve-replay-v1: where you began, then every\n"
    "                      input with its time), written each time you leave it; FILE.sieve: packed\n"
    "  --replay FILE       play a recording back in the hallway (started as it was recorded), then hand it\n"
    "                      back to you; with --screenshot, played through and pictured where it ends\n"
    "  --replay-cache      decode a packed recording once into the cache, not each time ([replays] cached)\n"
    "  --action NAME       hold your character at one action for a picture (idle, walk, run, jump, take,\n"
    "                      read, wave): it plays that action's animation from its map, or none\n"
    "  --creator           with --menu: the Character Creator instead (the whole window, as the main menu's\n"
    "                      Character Creator opens it): your character and its actions\n"
    "  --system-viewer ID  with --menu: the System Viewer instead (the whole window, as the main menu's\n"
    "                      System Viewer opens it), standing at entry ID\n"
    "  --save-character FILE  with --menu: save your character to FILE as the CHARACTER page's Ctrl+S does\n"
    "                      (also into the store, and into the infinite RAID where [raid] drives is set)\n"
    "  --animation FILE    the animation your character plays, from a .animation file (sieve-animation-v1),\n"
    "                      for this run: in the hallway as it walks beside you (with --character: only so)\n"
    "  --animation-frame N hold the animation at frame N (for a picture), on the CHARACTER page and in the hallway\n"
    "  --character-store DIR  your personal dimension from DIR (the store: <SHA-256>.character files) for\n"
    "                      this run, in place of the characters folder beside the settings file\n"
    "                      (--busy: while the dimensions are still being calculated, if that is caught)\n"
    "  --main-menu         with --screenshot: a picture of the main menu (--press keys go to it)\n"
    "  --read-aloud        the menus read aloud for this run (not saved); with --screenshot, prints what was said\n"
    "  --text-to-voice     the CHAT tab's replies spoken for this run (not saved); with --screenshot, prints the last\n"
    "  --cipher            with --screenshot: a picture of the Encryption Engine; --script \"Down,=text,Return\"\n"
    "  --manual-nav FILE   with --screenshot: a picture of Manual Navigation with FILE loaded; --script \"Up,Right,=4f\",\n"
    "                      --angle-decimals N its precision, --manual-steps OUT.csv saves the walk; --manual-logs DIR\n"
    "                      keeps a log there (none otherwise); --manual-log LOG (in place of FILE) replays a log;\n"
    "                      --manual-pick FILE (in place of both) gives FILE as Load File's window would;\n"
    "                      --manual-example (none of those) the example, --manual-seed N its bytes\n"
    "                      runs keys and typed text (=text types into the row chosen), as --designer's does;\n"
    "                      --cipher-in FILE takes a file's bytes as the input, --cipher-out FILE saves the output\n"
    "  --designer          with --screenshot: a picture of the filter designer; --design FILE opens a\n"
    "                      plugin, --script \"Down,Right,=text,Return\" runs keys and typed text, and\n"
    "                      --design-out FILE writes the filter as the designer would save it;\n"
    "                      --design-melody starts it on a melody filter, as the Media Player's M does\n"
    "  --no-music          no background music this run (the Media Player is still there, silent)\n"
    "  --media-player      with --screenshot: a silent music player, so --press can open the Media\n"
    "                      Player from the pause menu (Escape, then Down to it, Return)\n"
    "  --filters PATH      filter settings (default: sieve-filters.ini next to the executable)\n"
    "  --timings           time each phase (building, items, filters, vault, faces, frames, menus);\n"
    "                      written at exit to sieve-timings.txt beside the settings, and to stderr\n"
    "  --metrics PATH      write the metrics list and exit, with no window: how much of every line the\n"
    "                      filters (--filters) remove, as the setup menu counts it, at the lines' settings\n"
    "                      (the options above; their defaults without them). make_release writes one with\n"
    "                      data/metrics/reference-filters.ini beside the installer\n\n"
    "Screenshots (for documentation and testing):\n"
    "  --screenshot PATH   render one frame to a PNG and exit\n"
    "  --size WxH          window size (default 1280x720)\n"
    "  --pose X,Z,YAW,PITCH  camera position and angles in degrees\n"
    "  --tile N            then move N tiles along the corridor\n"
    "  --take              take the book you are looking at off the shelf\n"    "  --talk TEXT         with --take on the AI line: say TEXT to the model in hand; the conversation is printed\n"
    "  --chat              with --take on the AI line: open the CHAT tab on the model in hand\n"
    "  --chat-attach FILES with --take on the AI line: give the files (apart by ';') to the model, as CHAT's + does\n"
    "  --chat-draw N       with --take on the AI line: draw N figures with CHAT's picture model\n"
    "  --text-model DIR    CHAT talks with this model folder (config.json, model.safetensors, tokenizer.json)\n"
    "  --metrics-text FILE with --metrics: the text the guided ordering's saving is measured on\n"
    "                      (default: data/metrics/reference-text.txt, Dickens, which the model was not trained on)\n"
    "  --widen-weights     a model file's BF16 and F16 weights widened to 32-bit floats as they load (twice the\n"
    "                      memory; the same numbers): [models] widen_weights in the settings (off, on or auto)\n"
    "  --picture-model DIR DRAW uses this picture model's run folder\n"
    "  --chat-press KEYS   keys pressed in CHAT after those (e.g. Ctrl+G: the newest card's GO TO IT)\n"
    "  --print-system-prompt  print the system prompt a model given as a file is told here\n"
    "  --chain             show the CHAIN tab (with --take on the AI line); --chain-file PATH opens a\n"
    "                      chain, --chain-press KEYS presses keys there, --chain-save PATH saves it;\n"
    "                      --talk \"/chain WORDS\" runs it\n"
    "  --route-places N    with --take: COST's shortest route searched to N places of bearing (L), waited for and printed\n"
    "  --tailor PATH       with --screenshot, the item in hand on COST: K, waited for, and what Return\n"
    "                      would apply saved to PATH as a settings file\n"
    "  --save-item PATH    then save it as a file (F on the item page), to PATH\n"
    "  --save-view PATH    with the viewer open (--press E,Z,Tab...): save what it shows (F), to PATH\n"
    "  --walk DX,DZ;...    walk these distances in metres first (doors work as when walking)\n"
    "  --press K,K,...     then press these keys (e.g. M,M,-,Shift+=; Click: a left click where the\n"
    "                      crosshair is; Item:ID: the main or pause menu's row named ID, as Up/Down would\n"
    "                      reach it), printing where you are\n"
    "  --edge-glow         draw with Geometry Edge Glow (or --settings a file that has it on)\n"
    "  --real-graphics     draw with Real Graphics: the models in the meshes folder\n"
    "  --door-portals      fill the doorways with procedural data noise (Real Graphics turns this on)\n"
    "  --model-cache MB    display cache: memory for the pictures on items (8 up, default 64)\n"
    "  --model-tile PX     display size: how wide each item's picture is drawn (16 up to the\n"
    "                      renderer's widest texture, default 64)\n"
    "  --item-letters PX   letter size on item displays: pages and titles are drawn wide enough\n"
    "                      for it, and smaller letters are dashes (default 8)\n"
    "  --close-up PX       the displays nearest you are drawn again up to this wide (0: off;\n"
    "                      \"screen\", the default, for the screen's width rounded up to a power of\n"
    "                      two; or a power of two from 256 to the renderer's widest texture)\n"
    "  --items-per-wall N  units on one tile of the corridor (a power of two, 2 to 4096; 128 unless\n"
    "                      given; changes no address,\n"
    "                      only the tile and slot that name a unit's place in the corridor)\n"
    "  --fps-counter       show the FPS counter\n"
    "  --bench N           before the screenshot, time N frames and print the frame rate\n"
    "  --settle N          before the screenshot, draw N frames standing still, so the item pictures arrive\n"
    "  --sample-degrees DIR   every dimension's item at each whole degree 0-359, saved into DIR as its\n"
    "                      file named by its title (pictures one pixel a pixel, as J reads them), and DIR\n"
    "                      made into Sieve instructions (--sample-out FILE, else DIR.sieve)\n"
    "  --stop-after S      with --locate: press Stop after S seconds (the best found by then kept)\n"
    "  --locate-save FILE  with --locate: save its Sieve instructions to FILE once it is read\n"
    "  --create PATH       Create .sieve: the File Locator alone on a file or folder, NAME.sieve\n"
    "                      saved beside it once it is read (--out FILE: there), then closed (0);\n"
    "                      stopped or failed, it stays open saying why (1 once closed)\n"
    "  --shot-after S      with --locate and --screenshot: the picture taken S seconds in, while it works\n"
    "  --force-after S     with --locate: press Stop again (force) S seconds in: the pass under way abandoned\n"
    "  --locate PATH       open the File Locator on a file or folder (--tailored: and tailor the filters to its files;\n"
    "                      with --main-menu: the main menu's, with no world to walk to)\n"
    "  --search-per-file S, --search-total S   the File Locator's search for shorter addresses: seconds a\n"
    "                      file, or for the run (one of them; the other follows, the files counted)\n"
    "  --optimise-compressed the File Locator's Archive and compressed formats boxes both ticked, for this run\n"
    "                      (--optimise-archives, its old name, works too)\n"
    "  --compressed-temporary its Archive and compressed formats: temporary ticked (made again exactly), for this run\n"
    "  --compressed-permanent its Archive and compressed formats: permanent ticked (carried as what it holds), for this run\n"
    "  --install FILE --install-to DIR   the File Locator's install, at once\n"
    "  --harness           a picture of the AI Training Harness (--training-dir DIR: its runs; --script KEYS:\n"
    "                      keys and =typed text, each step waiting for a training to finish)\n"
    "  --map PATH          choose a map for the node graph: Sieve instructions (.sieve), an older .map, or a folder to map\n"
    "  --graph             open the node graph (on --map, or on this installation)\n"
    "  --new-map PATH      the node graph's New map..., at once\n"
    "  --thin              keep only the room you stand in (what going in past the budget does)\n"
    "  --filter-memory MB  the most one count of the filters' survivors may take for its tables (default\n"
    "                      the setup menu's, 512 MB at first)\n"
    "  --counting-memory PCT  the share of installed memory the setup menu's counts may take at once\n"
    "                      (5 to 100; the setup menu's, 50 at first)\n"
    "  --merge-cache PCT   the share of the filter memory kept for merged filters between counts (0 to\n"
    "                      100; the setup menu's, 50 at first)\n"
    "  --unit-time MS      the time budget: the longest one unit may take to open (5 to 60000; the\n"
    "                      setup menu's, 50 at first)\n"
    "  --item-memory PCT   the share of installed memory the items around you may take (5 to 90; the\n"
    "                      setup menu's, 25 at first)\n"
    "  --view-rooms N      rooms drawn and kept either side of you (2 to 64; 7 at first)\n"
    "  --picture-rooms N   rooms either side of yours with item pictures of their own (0 to 8; 1)\n\n"
    "Controls: WASD move, mouse look, Shift run, E or click take a book, T warp, G go to,\n"
    "M switch ordering (positional, scrambled, guided), - and = zoom out/in (guided; Shift: 8x),\n"
    "wheel/PgUp/PgDn/[ ] jump 1 (more spun fast)/1000/1000000 tiles, Home to corridor tile 0 (every line's start line),\n"
    "N/B next/previous unit of a warped trail (or page of a book in hand), P play an audio book, F save the item in hand as a file, F1 the setup menu, Tab free the mouse,\n"
    "Esc close or free the mouse, Ctrl+Q quit.\n\n"
    "Every line shares one corridor, 128 books a tile unless --items-per-wall. Each line repeats along it; a\n"
    "checkered start line marks where each repeat begins. Black doors lead to the next line\n"
    "(left wall) or the previous line (right wall) at the same corridor position.\n";

// Whether the settings make a pcm set (a set they cannot make falls back to notes104, as the setup
// menu says).
bool pcm_settings_ok(const Args& a)
{
    try
    {
        sieve::make_pcm_format(a.get_positive("pcm-rate", 8000), a.get_positive("pcm-bits", 8), a.get_positive("pcm-channels", 1));
        return uint64_t(a.get_positive("samples", 8000)) * a.get_positive("pcm-channels", 1) <= 0xFFFFFFFFull;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// Whether the settings make a notes3 set (one they cannot make falls back to notes104).
bool notes3_settings_ok(const Args& a)
{
    try
    {
        Args la;
        la.opts["line"] = "audio";
        la.opts["note-set"] = "notes3";
        for (const char* k : {"low", "high", "tpq", "longest", "levels", "voices", "tempo", "instruments"})
            if (a.has(std::string("notes3-") + k)) la.opts[k] = a.get(std::string("notes3-") + k);
        (void)make_line(la);
        return true;
    }
    catch (const std::exception&)
    {
        return false;
    }
}

// Where the AI Training Harness's new picture and melody models draw their units from: the image
// line and the audio line as the setup menu has them, and the filters file.
PictureSource picture_source(const Settings& s, const std::filesystem::path& filters)
{
    PictureSource p{s.image_w, s.image_h, s.image_palette, filters, {}};
    p.melody_line["length"] = std::to_string(s.notes);
    if (s.note_set == "notes2")
    {
        p.melody_line["note-set"] = "notes2";
        p.melody_line["low"] = s.note_low;
        p.melody_line["high"] = s.note_high;
        p.melody_line["durations"] = s.note_durations;
        p.melody_line["voices"] = std::to_string(s.voices);
    }
    else if (s.note_set != "notes104") p.melody_line["note-set"] = s.note_set; // (refused: not notes)
    return p;
}

std::vector<Line> make_lines(const Args& a)
{
    std::vector<Line> lines;
    for (LineKind k : kLineOrder)
    {
        Args la;
        la.opts["line"] = to_string(k);
        la.opts["key"] = a.get("key", "sieve");
        switch (k)
        {
        case LineKind::Text:
            la.opts["length"] = a.get("length", "32");
            la.opts["alphabet"] = a.get("alphabet", "lower27");
            la.opts["canon"] = a.get("canon", "v2");
            if (a.has("model")) la.opts["model"] = a.get("model");
            break;
        case LineKind::Image:
            la.opts["width"] = a.get("image-width", "10");
            la.opts["height"] = a.get("image-height", "10");
            la.opts["palette"] = a.get("image-palette", "mono");
            la.opts["fit"] = a.get("image-fit", "levels"); // canon-image-v2; "average" for v1
            break;
        case LineKind::Audio:
            la.opts["length"] = a.get("notes", "16");
            if (a.get("note-set", "notes104") == "pcm" && pcm_settings_ok(a))
            {
                la.opts["note-set"] = "pcm";
                la.opts["length"] = a.get("samples", "8000");
                la.opts["rate"] = a.get("pcm-rate", "8000");
                la.opts["bits"] = a.get("pcm-bits", "8");
                la.opts["channels"] = a.get("pcm-channels", "1");
            }
            else if (a.get("note-set", "notes104") == "notes3" && notes3_settings_ok(a))
            {
                la.opts["note-set"] = "notes3";
                la.opts["low"] = a.get("notes3-low", "C-1");
                la.opts["high"] = a.get("notes3-high", "G9");
                la.opts["tpq"] = a.get("notes3-tpq", "4");
                la.opts["longest"] = a.get("notes3-longest", "16");
                la.opts["levels"] = a.get("notes3-levels", "8");
                la.opts["voices"] = a.get("notes3-voices", "1");
                la.opts["tempo"] = a.get("notes3-tempo", "120");
                la.opts["instruments"] = a.get("notes3-instruments", "0");
            }
            else if (a.get("note-set", "notes104") == "notes2")
            {
                la.opts["note-set"] = "notes2";
                la.opts["low"] = a.get("note-low", "C3");
                la.opts["high"] = a.get("note-high", "C6");
                la.opts["durations"] = a.get("note-durations", "seEqQhHw");
                la.opts["voices"] = a.get("voices", "1");
            }
            break;
        case LineKind::Video:
            la.opts["width"] = a.get("video-width", "5");
            la.opts["height"] = a.get("video-height", "5");
            la.opts["frames"] = a.get("video-frames", "8");
            la.opts["palette"] = a.get("video-palette", "mono");
            la.opts["fit"] = a.get("video-fit", "levels"); // canon-image-v2; "average" for v1
            break;
        }
        lines.push_back(make_line(la));
    }
    return lines;
}

// The BINARY length (bytes): the binary line's, as the setup menu sets it.
uint32_t binary_length(const Args& a) { return a.has("binary-length") ? a.get_positive("binary-length", 32) : 32u; }

// The AI line's shape as the setup menu has it: a new training run starts from it.
sieve::AiShape ai_shape_of(const Settings& s)
{
    sieve::AiShape shape;
    shape.layers = s.ai_layers, shape.width = s.ai_width, shape.heads = s.ai_heads, shape.bits = s.ai_bits;
    return shape;
}

// The AI Training Harness's runs: the training folder beside the programs.
std::filesystem::path training_dir()
{
    const char* base = SDL_GetBasePath();
    const std::string b = base ? base : "";
    return (b.empty() ? std::filesystem::current_path() : std::filesystem::path(std::u8string(b.begin(), b.end()))) / "training";
}

// A path and its UTF-8 text, both ways (as the File Locator has them).
std::filesystem::path path_of_utf8(const std::string& s) { return std::filesystem::path(std::u8string(s.begin(), s.end())); }
std::string utf8_of(const std::filesystem::path& p)
{
    const std::u8string u = p.u8string();
    return std::string(u.begin(), u.end());
}

// The File Locator from the main menu: files weighed under the lines the setup menu's settings
// would build (made when the first weighing starts), and nowhere to walk to, so no Go to it.
// Tailored filters, when used, are saved for the next hallway built. Esc comes back to the main
// menu; `path` (scripted runs, --main-menu --locate PATH) opens it on a file or folder at once.
// Returns false if the window was closed.
std::vector<std::pair<SDL_Keycode, SDL_Keymod>> parse_presses(const std::string& spec); // below

struct MenuLocator
{
    Args la;
    std::vector<Line> lines;
    std::once_flag made;
    FilterConfig filters;
};

// A file the main menu's File Locator was asked to go to: the hallway is built, then walks to it.
struct LocatedGo
{
    std::vector<uint8_t> bytes;
    std::string name;
    bool past = false; // longer than the binary line: the line is made long enough (asked twice)
};

// The search's time for a scripted File Locator: --search-per-file or --search-total, in seconds
// (one of them: each sets the other); the seconds, and whether they are the run's.
std::optional<std::pair<double, bool>> locator_search_of(const sieve::cli::Args& a)
{
    const bool file = a.has("search-per-file"), run = a.has("search-total");
    if (!file && !run) return std::nullopt;
    if (file && run) throw std::invalid_argument("--search-per-file and --search-total: give one, the File Locator works out the other");
    const char* key = run ? "search-total" : "search-per-file";
    const std::string v = a.get(key);
    size_t used = 0;
    double s = -1;
    try
    {
        s = std::stod(v, &used);
    }
    catch (const std::exception&)
    {
    }
    if (used != v.size() || !(s >= 0)) throw std::invalid_argument(std::string("--") + key + ": seconds, 0 or more (0: no limit): " + v);
    return std::make_pair(s, run);
}

bool run_locator(SDL_Window* window, SDL_Renderer* renderer, const Settings& settings, FilterConfig& filters, const std::string& filters_path,
                 const std::string& path = "", bool tailor = false, const std::string& screenshot = "", const std::string& presses = "",
                 std::optional<LocatedGo>* go = nullptr, std::optional<std::pair<double, bool>> search = std::nullopt,
                 const std::string& create_out = "", int* created = nullptr, std::vector<std::string> only = {})
{
    MenuLocator m;
    settings.apply(m.la);
    m.filters = filters;
    FileLocator::Host host;
    host.lines = [&m] {
        std::call_once(m.made, [&m] { m.lines = make_lines(m.la); });
        sieve::cli::WeighLines wl;
        wl.pages = &m.lines[size_t(LineKind::Text)];
        wl.image = &m.lines[size_t(LineKind::Image)];
        wl.audio = &m.lines[size_t(LineKind::Audio)];
        wl.video = &m.lines[size_t(LineKind::Video)];
        wl.filters = m.filters;
        wl.binary_bytes = binary_length(m.la);
        return wl;
    };
    // Go to it, from the main menu: the locator closes, and the hallway is built and walks to the
    // file (app_main), measured against the binary line as the setup menu has it.
    if (go)
    {
        host.binary_bytes = [&settings] { return uint64_t(settings.binary_bytes); };
        host.go = [go](const std::vector<uint8_t>& bytes, const std::string& name, bool past) { *go = LocatedGo{bytes, name, past}; };
    }
    // An installer's items, on the lines of the setup menu's shapes: its pages and its AI line.
    host.item_args = [&settings] {
        Args a;
        a.opts["alphabet"] = settings.alphabet;
        a.opts["page-length"] = std::to_string(settings.length);
        a.opts["ai-layers"] = std::to_string(settings.ai_layers);
        a.opts["ai-width"] = std::to_string(settings.ai_width);
        a.opts["ai-heads"] = std::to_string(settings.ai_heads);
        a.opts["ai-bits"] = std::to_string(settings.ai_bits);
        return a;
    };
    FileLocator* self = nullptr;
    host.use_filters = [&](const FilterConfig& f) {
        filters = m.filters = f;
        try
        {
            f.save(filters_path);
            self->say(tr("loc.filters_kept"));
        }
        catch (const std::exception& e)
        {
            self->say(trf("loc.filters_not_saved", {e.what()}));
        }
    };
    FileLocator loc(window, renderer, host);
    self = &loc;
    if (search) loc.set_search(search->first, search->second);
    if (!create_out.empty()) loc.create(path, create_out, std::move(only)); // (Create .sieve: the window live while it works)
    else if (!path.empty()) loc.locate_now(path, tailor);
    else loc.open();
    auto frame = [&] {
        int ww = 0, wh = 0;
        SDL_GetCurrentRenderOutputSize(renderer, &ww, &wh);
        SDL_SetRenderLogicalPresentation(renderer, 0, 0, SDL_LOGICAL_PRESENTATION_DISABLED);
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        loc.draw(float(ww), float(wh));
    };
    if (!screenshot.empty())
    {
        for (const auto& [key, mod] : parse_presses(presses))
        {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = key;
            e.key.scancode = SDL_GetScancodeFromKey(key, nullptr); // as a real key brings it (the right-hand panel reads it)
            e.key.mod = mod;
            loc.event(e);
        }
        std::cout << "file locator (main menu): " << (loc.is_open() ? "open" : "closed") << ", "
                  << (loc.offers_go() ? "Go to it offered" : "no Go to it") << "\n";
        if (const std::string st = loc.status_text(); !st.empty()) std::cout << "file locator: " << st << "\n";
        frame();
        if (!save_render(renderer, screenshot)) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        loc.stop();
        return true;
    }
    bool quit = false;
    while (loc.is_open() && !quit)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_EVENT_QUIT) quit = true;
            else loc.event(e);
        }
        music_mode(MusicMode::Menus);
        frame();
        present(renderer);
        // (Create .sieve: saved and said, so closed; stopped or failed, left open saying why)
        if (!create_out.empty() && loc.create_finished() == 1)
        {
            if (created) *created = 1;
            break;
        }
    }
    loc.stop();
    return !quit;
}

// "M,M,-,Shift+=" -> key presses.
// "Click" in --press: a left click in the middle of the window, where the crosshair is (the hallway
// only; a menu takes it as a key it does not know).
constexpr SDL_Keycode kClickPress = SDLK_SCANCODE_MASK | 0xFFFF;

std::vector<std::pair<SDL_Keycode, SDL_Keymod>> parse_presses(const std::string& spec)
{
    std::vector<std::pair<SDL_Keycode, SDL_Keymod>> out;
    for (size_t start = 0; start < spec.size();)
    {
        const size_t end = std::min(spec.find(',', start), spec.size());
        std::string name = spec.substr(start, end - start);
        SDL_Keymod mod = SDL_KMOD_NONE;
        if (name.rfind("Shift+", 0) == 0) { mod = SDL_KMOD_LSHIFT; name = name.substr(6); }
        if (name.rfind("Ctrl+", 0) == 0) { mod = SDL_KMOD_LCTRL; name = name.substr(5); }
        if (name.rfind("Item:", 0) == 0 && name.size() > 5)
        {
            out.emplace_back(hallway::item_press(name.substr(5)), SDL_KMOD_NONE); // a menu's row by its name (press_names.hpp)
            start = end + 1;
            continue;
        }
        const SDL_Keycode key = name == "-" ? SDLK_MINUS : name == "=" ? SDLK_EQUALS : name == "Click" ? kClickPress : SDL_GetKeyFromName(name.c_str());
        if (key == SDLK_UNKNOWN) throw std::invalid_argument("--press: unknown key '" + name + "'");
        out.emplace_back(key, mod);
        start = end + 1;
    }
    return out;
}

// Your character (IDEAS 19.148): from --character FILE, or the settings file's [character] where it
// walks beside you (the default character when it has none), or none with --no-character or
// beside_you off. A file or setting that is refused is said on stderr and leaves the default.
// Your personal dimension's world (sieve-personal-world-v1): worlds/personal.world beside the settings.
std::filesystem::path personal_world_file(const std::filesystem::path& app_path) { return app_path.parent_path() / "worlds" / "personal.world"; }

std::optional<sieve::Character> character_of(const AppSettings& app, const Args& a)
{
    if (a.has("no-character")) return std::nullopt;
    if (a.has("character"))
    {
        std::ifstream in(std::filesystem::path(std::u8string(a.get("character").begin(), a.get("character").end())), std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return sieve::read_character(text); // a bad file stops the run: it was asked for by name
    }
    if (!app.character_beside) return std::nullopt;
    if (!app.character.empty())
        try
        {
            return sieve::read_character(app.character);
        }
        catch (const std::exception& e)
        {
            std::cerr << "the settings file's character was refused (" << e.what() << "): the default instead\n";
        }
    return sieve::unfold_character(sieve::CharacterSettings{});
}

// The animation your character plays (IDEAS 19.170): from --animation FILE, or the settings file's
// [character] animation (its SHA-256, read from the store); none with --no-character. A file asked
// for by name that is refused stops the run; a stored one that is missing or changed is left out.
std::optional<sieve::Animation> animation_of(const AppSettings& app, const std::filesystem::path& app_path, const Args& a)
{
    if (a.has("no-character")) return std::nullopt;
    if (a.has("animation"))
    {
        std::ifstream in(std::filesystem::path(std::u8string(a.get("animation").begin(), a.get("animation").end())), std::ios::binary);
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        return sieve::read_animation(text);
    }
    if (a.has("character") || app.character_animation.empty()) return std::nullopt; // (a character by name plays nothing unless asked)
    return stored_animation(animation_store_dir(app_path), app.character_animation);
}

// Recordings (IDEAS 19.184, sieve-replay-v1, client/replay.hpp): kept as text, or, named .sieve, packed
// as Sieve packs its own files (Edward, 13 November 2026: "those text files should also get sieved just
// like sfilter files"), the recording one member of it, read back and checked on the spot.
constexpr uint64_t kReplaySieveRoof = uint64_t(256) << 20;

// A packed recording is decoded in memory each time it is played, or, where it is marked cached
// ([replays] cached, by the .sieve's SHA-256; --replay-cache for one run), decoded once into the cache
// (kind "replays", beside the filters') and read from there, every member checked against the index
// at each later start as the filters' are (Edward: "a checkbox for caching them so that they don't
// always have to be decoded, this would be useful for large replays").
Replay load_replay_file(const std::string& path, bool cached = false)
{
    const std::filesystem::path p = sieve::from_u8(path);
    std::string text;
    if (p.extension() == ".sieve" && cached)
    {
        const std::filesystem::path dir = sieve::cli::decoded_sieve(p, "replays", kReplaySieveRoof);
        std::error_code ec;
        for (const auto& e : std::filesystem::recursive_directory_iterator(dir, ec))
            if (e.is_regular_file() && e.path().extension() == ".replay")
            {
                std::ifstream in(e.path(), std::ios::binary);
                text.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
                break;
            }
        if (text.empty()) throw std::invalid_argument(path + " holds no .replay");
    }
    else if (p.extension() == ".sieve")
    {
        for (auto& [name, bytes] : sieve::cli::files_in_sieve(p, kReplaySieveRoof))
            if (name.size() > 7 && name.substr(name.size() - 7) == ".replay")
            {
                text.assign(bytes.begin(), bytes.end());
                break;
            }
        if (text.empty()) throw std::invalid_argument(path + " holds no .replay");
    }
    else
    {
        std::ifstream in(p, std::ios::binary);
        if (!in) throw std::invalid_argument("cannot read " + path);
        text.assign((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    }
    try
    {
        return read_replay(text);
    }
    catch (const std::invalid_argument& e)
    {
        throw std::invalid_argument(path + ": " + e.what());
    }
}

void save_replay_file(const std::string& path, const Replay& r)
{
    const std::filesystem::path p = sieve::from_u8(path);
    if (p.extension() != ".sieve")
    {
        std::ofstream out(p, std::ios::binary);
        out << r.text();
        if (!out) throw std::runtime_error("cannot write " + path);
        return;
    }
    // Packed: the recording as one member of a folder of its own, packed and read back, then removed.
    std::error_code ec;
    const std::filesystem::path dir = std::filesystem::temp_directory_path(ec) / ("sieve-replay-" + std::to_string(SDL_GetTicksNS()));
    std::filesystem::create_directories(dir, ec);
    {
        std::ofstream out(dir / (p.stem().string() + ".replay"), std::ios::binary);
        out << r.text();
    }
    try
    {
        sieve::cli::pack_folder_sieve(dir, p, kReplaySieveRoof);
    }
    catch (...)
    {
        std::filesystem::remove_all(dir, ec);
        throw;
    }
    std::filesystem::remove_all(dir, ec);
}

// Whether a recording is marked to be decoded once into the cache: --replay-cache, or its file's
// SHA-256 in [replays] cached.
bool replay_cached(const AppSettings& app, const Args& a, const std::string& path)
{
    if (a.has("replay-cache")) return true;
    if (app.replays_cached.empty() || sieve::from_u8(path).extension() != ".sieve") return false;
    const std::string sha = sieve::cli::sha256_hex(sieve::cli::read_file_bytes(sieve::from_u8(path)));
    return app.replays_cached.find(sha) != std::string::npos;
}

// A recording's start: where the hallway stands now (its line, its tile, the camera exactly).
Replay replay_start_of(Hallway& hall)
{
    Replay r;
    r.line = hall.line_name();
    r.tile = hall.tile_decimal();
    const Camera& c = hall.camera();
    r.x = c.pos.x, r.y = c.pos.y, r.z = c.pos.z, r.yaw = c.yaw, r.pitch = c.pitch;
    return r;
}

// Put the hallway where a recording began; it must be the same line and tile (the same settings and
// options), else the replay is refused, saying where each is.
void replay_begin(Hallway& hall, const Replay& r)
{
    if (r.line != hall.line_name() || r.tile != hall.tile_decimal())
        throw std::invalid_argument("the recording began on the " + r.line + " line, tile " + r.tile + ", and this hallway starts on the " +
                                    hall.line_name() + " line, tile " + hall.tile_decimal() + ": start it as the recording was started");
    Camera& c = hall.camera();
    c.pos = {r.x, r.y, r.z};
    c.yaw = r.yaw;
    c.pitch = r.pitch;
}

// One frame of a recording played: its events, then its step with its keys held.
void replay_frame(Hallway& hall, const ReplayFrame& f, bool& quit)
{
    for (const SDL_Event& e : f.events) hall.handle(e, quit);
    const auto keys = keyboard_of(f.held);
    hall.update(float(f.dt_us) / 1e6f, keys.data());
}

// Your character's actions (IDEAS 19.186): the settings file's [character] actions (its map's SHA-256,
// read from the store), and every animation the map names that the store holds; none with
// --no-character, or for a character asked for by name.
std::pair<std::optional<sieve::ActionMap>, std::vector<sieve::Animation>> actions_of(const AppSettings& app, const std::filesystem::path& app_path, const Args& a)
{
    if (a.has("no-character") || a.has("character") || app.character_actions.empty()) return {};
    auto m = stored_actions(actions_store_dir(app_path), app.character_actions);
    if (!m) return {};
    std::vector<sieve::Animation> anims;
    for (const auto& id : m->animation)
        if (!id.empty() && std::none_of(anims.begin(), anims.end(), [&](const sieve::Animation& x) { return x.id() == id; }))
            if (auto an = stored_animation(animation_store_dir(app_path), id)) anims.push_back(std::move(*an));
    return {std::move(m), std::move(anims)};
}

// Builds the hallway from options, and applies the start-up and scripting options.
std::unique_ptr<Hallway> make_hallway(SDL_Window* window, SDL_Renderer* renderer, const Args& a, bool scripted,
                                      const FilterConfig& filters, int angle_decimals)
{
    // The corridor's tile size, before any line's loop is worked out from it. It changes no
    // address: only the tile and slot that name a unit's place in the corridor (corridor.hpp).
    if (a.has("items-per-wall")) sieve::set_books_per_tile(a.get_u32("items-per-wall", 128));
    std::vector<Line> lines = make_lines(a);
    // The door to start at, by name: an id ("pages") or a --line name ("text"). Any other name is
    // refused, by line_from_string(), with the names it takes.
    int start_line = line_named(a.get("line", "text"));
    if (start_line < 0) start_line = line_of(line_from_string(a.get("line", "text")));
    const bool text_has_model = lines[size_t(LineKind::Text)].guided != nullptr;
    // Books and models are not made of one alphabet, so neither starts with the text greeting.
    const LineKind start_kind = kDimensions[start_line].unit.value_or(LineKind::Image);

    const Hallway::ModelShape shape{a.get_positive("vertices", 8), a.get_positive("faces", 12), a.get_positive("coords", 16),
                                    a.has("title-length") ? a.get_u32("title-length", 32) : 32u,
                                    binary_length(a),
                                    a.has("track-units") ? a.get_positive("track-units", 4) : 4u,
                                    a.has("movie-units") ? a.get_positive("movie-units", 4) : 4u,
                                    a.has("world-models") ? a.get_positive("world-models", 4) : 4u,
                                    a.has("world-grid") ? a.get_positive("world-grid", 8) : 8u,
                                    a.has("ai-layers") ? a.get_positive("ai-layers", 1) : 1u,
                                    a.has("ai-width") ? a.get_positive("ai-width", 16) : 16u,
                                    a.has("ai-heads") ? a.get_positive("ai-heads", 2) : 2u,
                                    a.has("ai-bits") ? a.get_positive("ai-bits", 4) : 4u};
    sieve::AiShape{shape.ai_layers, shape.ai_width, shape.ai_heads, shape.ai_bits}.check(); // a shape the AI line can have, or say why not
    auto hall = std::make_unique<Hallway>(window, renderer, std::move(lines), filters,
                                          a.has("book-pages") ? a.get_u32("book-pages", 4) : 4, shape);
    hall->set_line(start_line);
    hall->set_angle_decimals(angle_decimals); // before any scripted keys, which may type a bearing
    if (a.has("mode"))
    {
        if (a.get("mode") == "guided")
        {
            if (text_has_model) hall->enable_guided();
            else
            {
                std::cerr << "guided ordering needs a model for the text line (see: sieve models); using positional\n";
                hall->message("guided ordering needs a text model: using positional");
            }
        }
        else hall->set_mode(address_mode_from_string(a.get("mode")));
    }
    if (a.has("warp")) hall->warp(a.get("warp"));
    else if (a.has("goto")) hall->go_to(a.get("goto"));
    else if (start_kind == LineKind::Text)
    {
        // The greeting is Latin letters, so on an alphabet that cannot hold them (Greek, kana,
        // hieroglyphs) nothing survives canonicalisation: start halfway along instead.
        if (!hall->warp("welcome to the sieve")) hall->go_to("50%");
        hall->put_back();
    }
    else
    {
        // Halfway along, and on the shelf: going to an address hands you the unit, and holding
        // one holds you still, so starting a line holding a record would start it unable to move.
        hall->go_to("50%");
        hall->put_back();
    }
    if (!scripted) return hall;

    if (a.has("zoom")) hall->zoom_to(a.get_positive("zoom", 20));
    if (const auto search = locator_search_of(a)) hall->locator_search(search->first, search->second);
    if (a.has("locate"))
    {
        FileLocator::script(a.has("stop-after") ? std::stod(a.get("stop-after")) : 0, a.get("locate-save", ""),
                            a.has("shot-after") ? std::stod(a.get("shot-after")) : 0, a.has("force-after") ? std::stod(a.get("force-after")) : 0);
        hall->locate_now(a.get("locate"), a.has("tailored"));
    }
    if (a.has("install") && a.has("install-to")) hall->install_now(a.get("install"), a.get("install-to"));
    if (a.has("new-map")) hall->graph_picked(4, a.get("new-map")); // the viewer's New map..., at once
    if (a.has("map") || a.has("graph")) hall->graph_map_now(a.has("map") ? a.get("map") : "", a.has("graph"));
    if (a.has("tile"))
    {
        const std::string t = a.get("tile");
        const bool neg = !t.empty() && t[0] == '-';
        const int64_t n = int64_t(parse_whole(neg ? t.substr(1) : t, "--tile", uint64_t(1) << 62));
        hall->move_tiles(neg ? -n : n);
    }
    if (a.has("pose"))
    {
        float v[4] = {};
        if (!hallway::parse_floats(a.get("pose"), v, 4)) throw std::invalid_argument("--pose expects X,Z,YAW,PITCH");
        const float x = v[0], z = v[1], yaw = v[2], pitch = v[3];
        hall->camera().pos = {x, 1.6f, z};
        hall->camera().yaw = yaw * kPi / 180;
        hall->camera().pitch = pitch * kPi / 180;
    }
    if (a.has("walk"))
    {
        // "DX,DZ;DX,DZ;..." in metres, applied in 5 cm steps so walls and doors behave as when walking.
        // They are in the walker's own frame: a door that turns you round (into or out of the
        // binary line from models) turns the rest of the walk round with you, as it turns a
        // person walking forward.
        std::string spec = a.get("walk");
        float frame = 1.0f;
        for (size_t start = 0; start < spec.size();)
        {
            const size_t end = std::min(spec.find(';', start), spec.size());
            float v[2] = {};
            if (!hallway::parse_floats(spec.substr(start, end - start), v, 2)) throw std::invalid_argument("--walk expects DX,DZ;DX,DZ;...");
            const float dx = v[0], dz = v[1];
            const int steps = std::max(1, int(std::ceil(std::sqrt(dx * dx + dz * dz) / 0.05f)));
            for (int i = 0; i < steps; ++i)
            {
                const float yaw = hall->camera().yaw;
                hall->move_by({frame * dx / steps, 0, frame * dz / steps});
                if (std::abs(std::remainder(hall->camera().yaw - yaw, 2 * kPi)) > 3.0f) frame = -frame;
            }
            std::cout << hall->status() << "\n";
            start = end + 1;
        }
    }
    // (favourites kept by H in --press go beside a --settings file only, never beside the program: IDEAS 19.190)
    if (a.has("settings")) hall->set_favourites_base(std::filesystem::path(sieve::from_u8(a.get("settings"))).parent_path());
    if (a.has("open-favourite"))
    {
        // A favourite opened as your dimension's O opens it (IDEAS 19.190): read from beside --settings.
        const auto base = std::filesystem::path(sieve::from_u8(a.get("settings", "."))).parent_path();
        const auto f = hallway::read_favourite(base, a.get("open-favourite"));
        if (!f) throw std::invalid_argument("--open-favourite: no favourite " + a.get("open-favourite") + " beside --settings");
        hall->open_favourite(*f);
        std::cout << "favourite: " << (hall->holding() ? "in hand" : "not opened") << "\n";
        if (!hall->viewer_tab().empty()) std::cout << "viewer: " << hall->viewer_tab() << "\n";
    }
    if (a.has("press"))
        for (const auto& [key, mod] : parse_presses(a.get("press")))
        {
            SDL_Event e{};
            e.type = SDL_EVENT_KEY_DOWN;
            e.key.key = key;
            e.key.scancode = SDL_GetScancodeFromKey(key, nullptr); // as a real key brings it (the right-hand panel reads it)
            e.key.mod = mod;
            if (const std::string* id = hallway::item_press_id(key))
            {
                // Item:ID: the pause menu's row of that name (press_names.hpp).
                if (!hall->pause_select(*id)) throw std::invalid_argument("--press: no row '" + *id + "' in the pause menu (is it open?)");
                std::cout << hall->status() << "\n";
                continue;
            }
            if (key == kClickPress)
            {
                int w = 0, h = 0;
                SDL_GetWindowSize(window, &w, &h);
                e = SDL_Event{};
                e.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
                e.button.button = SDL_BUTTON_LEFT;
                e.button.x = float(w) * 0.5f;
                e.button.y = float(h) * 0.5f;
            }
            bool quit = false;
            hall->render(); // so the key sees what you are looking at
            hall->handle(e, quit);
            std::cout << hall->status() << "\n";
            // Where a key asked to go (a screen opened over the hallway), said, as a picture is taken
            // here rather than going there.
            if (hall->request() == Hallway::Request::Personal) std::cout << "goes to: Personal Dimensions\n";
            if (hall->request() == Hallway::Request::Settings) std::cout << "goes to: Settings\n";
            if (hall->menu_requested()) std::cout << "goes to: the setup menu\n";
        }
    return hall;
}

int run(const Args& a)
{
    int w = 1280, h = 720;
    if (a.has("size") && !hallway::parse_size(a.get("size"), w, h))
        throw std::invalid_argument("--size expects WxH, e.g. 1280x720");
    const bool shot = a.has("screenshot");

    // --write-settings OUT: the settings file as save() writes it, from --settings PATH or from the
    // defaults, without a window. Written twice, once from the other, it comes out the same.
    if (a.has("write-settings"))
    {
        const AppSettings s = a.has("settings") ? AppSettings::load(a.get("settings")) : AppSettings{};
        if (!s.save(a.get("write-settings"))) throw std::runtime_error("cannot write " + a.get("write-settings"));
        std::cout << "wrote " << a.get("write-settings") << "\n";
        return 0;
    }
    // Create .sieve on several files chosen at once: gathered before anything starts, so the
    // programs Explorer starts for the others hand their paths over and close without a window, a
    // renderer or a sound player each (the review of 6 November 2026), and the first is listening
    // at once. (Below, at --create, the rest.)
    std::optional<std::vector<std::string>> create_gathered;
    if (a.has("create") && !a.has("out"))
    {
        std::string what = a.get("create");
        while (what.size() > 1 && (what.back() == '/' || what.back() == '\\')) what.pop_back();
        create_gathered = sieve::cli::gather_chosen(what, "create");
        if (!create_gathered) return 0; // (handed to the first, which makes the .sieve)
    }
    // --metrics FILE: the metrics list (IDEAS §19.12), counted without a window or a display.
    const bool metrics = a.has("metrics");
    if (!metrics && !SDL_Init(SDL_INIT_VIDEO)) throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
    // The application's settings (sieve-hallway.ini, or --settings PATH). On the first start the
    // resolution is chosen from the display. Screenshots keep --size (default 1280x720) and the
    // default settings, so they come out the same on every machine.
    const std::filesystem::path app_path = a.has("settings") ? std::filesystem::path(a.get("settings")) : AppSettings::default_path();
    // --timings: how long each phase takes, written at exit to standard error and to
    // sieve-timings.txt beside the settings (tools/cli/timings.hpp).
    if (a.has("timings")) sieve::cli::timings::enable(app_path.parent_path() / "sieve-timings.txt");
    sieve::cli::load_plugins(); // the filter plugins in the filters folder, beside the built-in filters
    // The training runs the AI line's filters can name (language-v1, near-model-v1): the harness's
    // folder, then data/training (running from the repository).
    sieve::cli::use_training_runs({a.has("training-dir") ? std::filesystem::path(a.get("training-dir")) : training_dir(),
                                   std::filesystem::path("data") / "training"});
    // Sieve instructions that carry items of Sieve's lines (sieve-manifest-v5) are made again here.
    sieve::cli::use_item_maker();
    // Sieve's own files kept as .sieve, decoded now and cached ahead of their first use (IDEAS 19.185):
    // the catalogue's entries (in the cache, as the filters are), and the maps (in memory, on a thread
    // of their own, so the window opens without waiting for them; started once what decoding reads is set).
    (void)sieve::cli::catalogue_folders();
    sieve::cli::prefetch_maps(sieve::cli::executable_dir() / "maps");
    AppSettings app = shot ? AppSettings{} : AppSettings::load(app_path);
    if (shot && a.has("settings")) app = AppSettings::load(app_path);
    for (const auto& [id, k] : app.keys) hallway::Keys::get().set(id, k); // (keys changed on Settings > Controls > Keys)
    const AppSettings loaded = app; // (as the file has them: what the options below are for this run only)
    // The filter memory (the setup menu's FILTER MEMORY): what one count's tables may take.
    if (a.has("filter-memory")) app.filter_memory_mb = std::clamp(int(a.get_positive("filter-memory", 512)), 64, 1048576);
    sieve::set_filter_memory(double(app.filter_memory_mb) * 1024 * 1024);
    // The rope's search through every wave, even where its first make the bytes no smaller
    // ([locator] search_every_wave; --search-every-wave for this run).
    if (a.has("search-every-wave")) app.locator_every_wave = true;
    sieve::set_rope_search_every_wave(app.locator_every_wave);
    // Optimise Search ([locator] optimise_search, ticked unless unticked; --optimise-search for this run):
    // Try every way chosen for each file by its kind. Asking for Try every way by name unticks it.
    if (a.has("optimise-search")) app.locator_optimise = true;
    if (a.has("search-every-wave") && !a.has("optimise-search")) app.locator_optimise = false;
    sieve::cli::set_optimise_search(app.locator_optimise);
    // Size, Balanced or Decoding speed ([locator] priority; --priority, or --prioritise-decoding for
    // speed, for this run).
    if (a.has("prioritise-decoding")) app.locator_priority = "speed";
    if (a.has("priority"))
    {
        sieve::Priority given = sieve::Priority::balanced;
        if (!sieve::priority_from_name(a.get("priority"), given)) throw std::invalid_argument("--priority: size, balanced or speed, not " + a.get("priority"));
        app.locator_priority = sieve::priority_name(given);
    }
    {
        sieve::Priority p = sieve::Priority::balanced;
        sieve::priority_from_name(app.locator_priority, p);
        sieve::set_priority(p);
    }
    // The next version's draft ([locator] v35_draft; --v35-draft for this run: cli/draft.hpp; the names follow kDraftVersion).
    if (a.has(sieve::cli::draft_flag())) app.locator_draft = true;
    sieve::cli::set_draft(app.locator_draft);
    // How a model given as a file keeps its weights ([models] widen_weights; --widen-weights for this run).
    if (a.has("widen-weights")) app.widen_weights = 1;
    set_read_aloud(app.read_aloud); // the menus read aloud ([controls] read_aloud; speech.hpp)
    if (a.has("read-aloud")) set_read_aloud(true); // for this run only, not saved; scripted pictures print what is said
    set_text_to_voice(app.text_to_voice); // the CHAT tab's replies spoken ([controls] text_to_voice)
    if (a.has("text-to-voice")) set_text_to_voice(true); // for this run only, not saved; scripted pictures print the reply spoken
    // How much of installed memory the setup menu's counts may take at once (COUNTING MEMORY), and
    // how much of the filter memory keeps merged automata between counts (MERGE CACHE).
    if (a.has("counting-memory")) app.counting_memory_pct = std::clamp(int(a.get_positive("counting-memory", 50)), 5, 100);
    if (a.has("merge-cache")) app.merge_cache_pct = std::clamp(int(a.get_u32("merge-cache", 50)), 0, 100);
    set_counting_share(app.counting_memory_pct);
    sieve::set_merge_cache_share(app.merge_cache_pct / 100.0);
    // The time budget (TIME BUDGET): the longest one unit may take to open.
    if (a.has("unit-time")) app.unit_time_ms = std::clamp(int(a.get_positive("unit-time", 50)), 5, 60000);
    sieve::set_unit_time_ms(app.unit_time_ms);
    set_time_budget(app.unit_time_ms);
    // The items around you (ITEM MEMORY): what the hallway's cache of them may take.
    if (a.has("item-memory")) app.item_memory_pct = std::clamp(int(a.get_positive("item-memory", 25)), 5, 90);
    apply_widen_weights(app); // (after --item-memory: auto weighs against its share)
    set_item_memory_share(app.item_memory_pct);
    // The view distance (Settings > Graphics): the rooms drawn and kept, and those with pictures.
    if (a.has("view-rooms")) app.view_rooms = std::clamp(int(a.get_positive("view-rooms", 7)), 2, 64);
    if (a.has("picture-rooms")) app.picture_rooms = std::clamp(int(a.get_u32("picture-rooms", 1)), 0, 8);
    {
        // The options above are for this run: saving a setting writes each back as the file had it,
        // unless it was changed since (in the setup menu, or a box in the File Locator).
        // --locate-log, below, is one too.
        AppSettings run = app;
        run.locator_log = true;
        const bool every = a.has("search-every-wave"), optimise = a.has("optimise-search") || every, decoding = a.has("prioritise-decoding") || a.has("priority"),
                   draft = a.has(sieve::cli::draft_flag()), widen = a.has("widen-weights"), filter = a.has("filter-memory"),
                   counting = a.has("counting-memory"), merge = a.has("merge-cache"), unit = a.has("unit-time"), item = a.has("item-memory"),
                   rooms = a.has("view-rooms"), pictures = a.has("picture-rooms"), log = a.has("locate-log");
        app.run_only = std::make_shared<const std::function<void(AppSettings&)>>([=](AppSettings& s) {
            auto back = [](bool given, auto& now, const auto& for_run, const auto& on_disk) {
                if (given && now == for_run) now = on_disk;
            };
            back(every, s.locator_every_wave, run.locator_every_wave, loaded.locator_every_wave);
            back(optimise, s.locator_optimise, run.locator_optimise, loaded.locator_optimise);
            back(decoding, s.locator_priority, run.locator_priority, loaded.locator_priority);
            back(draft, s.locator_draft, run.locator_draft, loaded.locator_draft);
            back(widen, s.widen_weights, run.widen_weights, loaded.widen_weights);
            back(filter, s.filter_memory_mb, run.filter_memory_mb, loaded.filter_memory_mb);
            back(counting, s.counting_memory_pct, run.counting_memory_pct, loaded.counting_memory_pct);
            back(merge, s.merge_cache_pct, run.merge_cache_pct, loaded.merge_cache_pct);
            back(unit, s.unit_time_ms, run.unit_time_ms, loaded.unit_time_ms);
            back(item, s.item_memory_pct, run.item_memory_pct, loaded.item_memory_pct);
            back(rooms, s.view_rooms, run.view_rooms, loaded.view_rooms);
            back(pictures, s.picture_rooms, run.picture_rooms, loaded.picture_rooms);
            back(log, s.locator_log, run.locator_log, loaded.locator_log);
        });
    }
    set_view(app.view_rooms, app.picture_rooms);
    if (metrics)
    {
        // Every line at the settings the options give (the defaults without them), under the
        // filter settings named (--filters), counted as the setup menu counts them.
        set_language(a.has("language") ? a.get("language") : app.language);
        Settings settings = Settings::from_args(a);
        const std::string filters_path = a.has("filters") ? a.get("filters") : FilterConfig::default_path().string();
        if (!std::filesystem::exists(filters_path)) throw std::invalid_argument("no filter settings file at " + filters_path);
        Menu menu(nullptr, nullptr, settings, FilterConfig::load(filters_path), filters_path);
        // The text the guided ordering is measured on (--metrics-text, else data/metrics/reference-text.txt
        // from here, beside the program, or in the source tree above it).
        std::string reference = a.get("metrics-text", "");
        if (reference.empty())
        {
            std::vector<std::filesystem::path> where = {std::filesystem::path("data") / "metrics"};
            if (const std::filesystem::path exe = sieve::cli::executable_dir(); !exe.empty())
            {
                where.push_back(exe / "metrics");
                where.push_back(exe / ".." / "data" / "metrics");
            }
            for (const auto& w : where)
                if (std::error_code ec; std::filesystem::is_regular_file(w / "reference-text.txt", ec))
                {
                    const std::u8string u = (w / "reference-text.txt").u8string();
                    reference.assign(u.begin(), u.end());
                    break;
                }
        }
        std::string text = menu.metrics_text(reference);
        std::ifstream fin(filters_path, std::ios::binary);
        std::ostringstream body;
        body << fin.rdbuf();
        text = "Sieve " SIEVE_VERSION ": how much of each line its filters remove\nFilters: " +
               std::filesystem::path(filters_path).filename().string() + " (SHA-256 " +
               sieve::Sha256::hex(sieve::Sha256::hash(body.str())) + ")\nFilter memory: " +
               std::to_string(app.filter_memory_mb) + " MB\n\n" + text;
        const std::string out_path = a.get("metrics");
        std::ofstream out(out_path, std::ios::binary);
        if (!out || !out.write(text.data(), std::streamsize(text.size()))) throw std::runtime_error("cannot write " + out_path);
        std::cout << text;
        return 0;
    }
    const DisplayInfo display = detect_display();
    if (!shot && app.resolution.w <= 0)
    {
        app.resolution = default_resolution(display);
        app.save(app_path);
    }
    else if (!shot && !app.complete) app.save(app_path); // an older file gains the entries added since, with its own values
    // The File Locator's search time, kept between runs (not by scripted runs, which save nothing).
    FileLocator::remember_search(app.locator_search, app.locator_search_run, shot ? std::function<void(double, bool)>() : [&app, app_path](double s, bool run) {
        app.locator_search = s;
        app.locator_search_run = run;
        app.save(app_path);
    });
    FileLocator::remember_every(shot ? std::function<void(bool)>() : [&app, app_path](bool on) {
        app.locator_every_wave = on;
        app.save(app_path);
    });
    // Sound as FLAC ([locator] sound; --sound-temporary, --sound-permanent (--sound-to-flac) or
    // --sound-off for this run): v34's.
    {
        sieve::cli::SoundMode mode = sieve::cli::sound_mode_named(app.locator_sound).value_or(sieve::cli::SoundMode::temporary);
        if (a.has("sound-off")) mode = sieve::cli::SoundMode::off;
        else if (a.has("sound-permanent") || a.has("sound-to-flac")) mode = sieve::cli::SoundMode::permanent;
        else if (a.has("sound-temporary")) mode = sieve::cli::SoundMode::temporary;
        sieve::cli::set_sound_mode(mode);
    }
    FileLocator::remember_sound_flac(shot ? std::function<void(sieve::cli::SoundMode)>() : [&app, app_path](sieve::cli::SoundMode mode) {
        app.locator_sound = sieve::cli::sound_mode_name(mode);
        app.save(app_path);
    });
    FileLocator::remember_optimise(shot ? std::function<void(bool)>() : [&app, app_path](bool on) {
        app.locator_optimise = on;
        app.save(app_path);
    });
    FileLocator::remember_priority(shot ? std::function<void(sieve::Priority)>() : [&app, app_path](sieve::Priority p) {
        app.locator_priority = sieve::priority_name(p);
        app.save(app_path);
    });
    // Compare with zip and 7z ([locator] compare, on unless turned off).
    FileLocator::remember_compare(app.locator_compare, shot ? std::function<void(bool)>() : [&app, app_path](bool on) {
        app.locator_compare = on;
        app.save(app_path);
    });
    // More formats in the comparison ([locator] more_formats; --more-formats for this run).
    FileLocator::remember_more(app.locator_more || a.has("more-formats"), shot ? std::function<void(bool)>() : [&app, app_path](bool on) {
        app.locator_more = on;
        app.save(app_path);
    });
    // Keep a log ([locator] keep_log; --locate-log for this run): each run's reports, in the logs
    // folder beside the settings.
    if (a.has("locate-log")) app.locator_log = true;
    FileLocator::remember_log(app.locator_log, app_path.parent_path() / "logs", shot && !a.has("locate-log") ? std::function<void(bool)>() : [&app, app_path, shot](bool on) {
        app.locator_log = on;
        if (!shot) app.save(app_path);
    });
    FileLocator::remember_shares(app.locator_share_packing, app.locator_share_weighing, shot ? std::function<void(double, double)>() : [&app, app_path](double p, double w) {
        app.locator_share_packing = p;
        app.locator_share_weighing = w;
        app.save(app_path);
    });
    {
        // The Archive and compressed formats boxes (v31): --optimise-compressed ticks both, as the one box was.
        const bool both = a.has("optimise-archives") || a.has("optimise-compressed");
        FileLocator::remember_archives(app.locator_temporary || both || a.has("compressed-temporary"), app.locator_permanent || both || a.has("compressed-permanent"),
                                       shot ? std::function<void(bool, bool)>() : [&app, app_path](bool temporary, bool permanent) {
                                           app.locator_temporary = temporary;
                                           app.locator_permanent = permanent;
                                           app.save(app_path);
                                       });
    }
    if (!shot && !a.has("size")) { w = app.resolution.w; h = app.resolution.h; }
    if (shot && app.resolution.w <= 0) app.resolution = {w, h}; // the screenshot's own size
    set_language(a.has("language") ? a.get("language") : app.language);
    // A fullscreen window is not minimised when it loses the focus: a system dialog (a file or a
    // folder to choose) takes it every time, and minimising at each one hid the hallway behind it.
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");
    SDL_Window* window = SDL_CreateWindow("The Sieve of Potentia", w, h, SDL_WINDOW_RESIZABLE | (shot ? SDL_WINDOW_HIDDEN : 0));
    set_window_icon(window);
    if (!window) throw std::runtime_error(std::string("cannot open a window: ") + SDL_GetError());
    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    if (!renderer) throw std::runtime_error(std::string("cannot create a renderer: ") + SDL_GetError());
    SDL_SetRenderVSync(renderer, app.vsync ? 1 : 0);
    if (!shot && !a.has("size") && app.fullscreen) apply_video(window, app);
    // The music player (music.hpp): melodies from the audio line behind every screen. Scripted
    // pictures have none, unless --media-player asks for one (silent) to draw the Media Player;
    // --no-music leaves it silent in the app too.
    std::unique_ptr<MusicPlayer> player;
    if (!shot || a.has("media-player"))
    {
        const std::filesystem::path folder = app_path.parent_path().empty() ? std::filesystem::path(".") : app_path.parent_path();
        player = std::make_unique<MusicPlayer>(folder, !shot && !a.has("no-music"));
        set_music(player.get());
    }
    // The RAID between machines, where its serving is ticked (Settings > RAID, opt in only): this machine
    // keeps others' sealed saves while Sieve runs (never for a picture).
    if (!shot) raid_service::apply(app, app_path);
    auto finish = [&] {
        raid_service::stop(); // no more accepted; those answering finished
        player.reset(); // its audio stream goes before SDL does
        stop_speech();  // and the voice's
        release_fonts(); // glyph textures belong to the renderer
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    };

    // The setup menu's settings: sieve-hallway.ini's [setup], and the options over it. The ffmpeg
    // is --ffmpeg's, else [paths]'s.
    Settings settings = Settings::from_args(a, app.setup);
    if (!a.has("ffmpeg")) settings.ffmpeg = app.ffmpeg;
    sieve::cli::set_ffmpeg_path(settings.ffmpeg);
    // When the setup menu has changed them, they are saved to [setup] (a scripted picture saves
    // only into a --settings file, never into the one beside the program). They are saved as the
    // menu had them, options and all, so that the next start begins where this one left off.
    auto keep_setup = [&](const Settings& before, const Settings& now) {
        if (now == before || (shot && !a.has("settings"))) return;
        app.setup = now;
        app.setup.ffmpeg.clear(); // [paths] keeps it, not [setup]
        app.save(app_path);
    };
    // Filter settings: --filters PATH, or sieve-filters.ini next to the executable.
    const std::string filters_path = a.has("filters") ? a.get("filters") : FilterConfig::default_path().string();
    if (a.has("filters") && !std::filesystem::exists(filters_path))
        std::cerr << "note: " << filters_path << " does not exist yet: no filters ticked (the menu saves your choices there)\n";
    FilterConfig filters = FilterConfig::load(filters_path);
    if (shot && a.has("cipher"))
    {
        // A picture of the Encryption Engine (for documentation and testing), after --script:
        // keys, and "=text" typed into the row chosen.
        CipherScreen c(window, renderer);
        c.allow_go(false);
        // --cipher-in FILE: the input a file's bytes, as the item in hand or Ctrl+O gives them.
        if (a.has("cipher-in"))
        {
            const std::string given = a.get("cipher-in");
            const std::filesystem::path in(std::u8string(given.begin(), given.end()));
            const std::u8string name = in.filename().u8string();
            c.set_input_file(sieve::cli::read_file_bytes(in), std::string(name.begin(), name.end()));
        }
        if (a.has("script"))
        {
            const std::string script = a.get("script");
            size_t at = 0;
            while (at <= script.size())
            {
                const size_t comma = script.find(',', at);
                const std::string item = script.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                if (!item.empty() && item[0] == '=') c.type(item.substr(1));
                else if (!item.empty())
                    for (const auto& [key, mod] : parse_presses(item)) c.press(key, mod);
                if (comma == std::string::npos) break;
                at = comma + 1;
            }
        }
        // --cipher-out FILE: the output saved, as Ctrl+S saves it.
        if (a.has("cipher-out") && !c.save_output(a.get("cipher-out"))) throw std::runtime_error(c.status());
        c.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "cipher: " << c.status() << "\n";
        for (const std::string& l : c.output_lines()) std::cout << "  " << l << "\n";
        if (!c.output_hex().empty()) std::cout << "output: " << c.output_hex() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && (a.has("manual-nav") || a.has("manual-log") || a.has("manual-pick") || a.has("manual-example")))
    {
        // A picture of Manual Navigation (for documentation and testing): --manual-nav FILE loads
        // it; --script runs keys and hex typed into the byte chosen ("Up,Up,Right,=4f"); each
        // step's landing is worked out before the next is taken, so every mark is drawn;
        // --manual-steps FILE saves the steps as CSV, as Ctrl+S does.
        // (a log kept only where --manual-logs names a folder: a scripted run leaves nothing behind otherwise)
        ManualNav::Kept kept;
        kept.keep_log = a.has("manual-logs");
        ManualNav::remember(kept, a.has("manual-logs") ? std::filesystem::path(a.get("manual-logs")) : std::filesystem::path(), {});
        ManualNav nav(window, renderer, a.has("angle-decimals") ? std::stoi(a.get("angle-decimals")) : app.angle_decimals);
        if (a.has("manual-seed")) nav.seed(uint32_t(std::stoul(a.get("manual-seed"))));
        // (--manual-log and --manual-nav both: the log first, and where its file was not found, or has
        // changed, FILE loaded as Ctrl+O would load it, the log replayed if it is the same bytes)
        bool loaded = a.has("manual-log") && nav.load_log(a.get("manual-log"));
        if (!loaded && a.has("manual-nav")) loaded = nav.load(a.get("manual-nav"));
        if (!loaded && a.has("manual-pick"))
        {
            // (as Load File's window answers: through the same owner check, which once dropped it)
            nav.answer_as_window(a.get("manual-pick"), 0);
            loaded = nav.loaded();
        }
        if (!loaded && a.has("manual-example") && !a.has("manual-nav") && !a.has("manual-log") && !a.has("manual-pick")) loaded = true; // (the example it opens on)
        if (!loaded) throw std::runtime_error(nav.status());
        nav.wait();
        if (a.has("script"))
        {
            const std::string script = a.get("script");
            size_t at = 0;
            while (at <= script.size())
            {
                const size_t comma = script.find(',', at);
                const std::string item = script.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                if (!item.empty() && item[0] == '=') nav.type(item.substr(1));
                else if (!item.empty())
                    for (const auto& [key, mod] : parse_presses(item)) nav.press(key, mod);
                nav.wait();
                if (comma == std::string::npos) break;
                at = comma + 1;
            }
        }
        if (a.has("manual-steps") && !nav.save_steps(a.get("manual-steps"))) throw std::runtime_error(nav.status());
        nav.wait_exact(); // (the exact check, where it was asked for or is made by itself, done before the picture)
        nav.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "manual navigation: " << nav.status() << "\n";
        std::cout << "  " << nav.readout() << "\n";
        if (!nav.log_path().empty()) std::cout << "  log: " << nav.log_path() << "\n";
        std::cout << "  marks: " << nav.marks_text() << "\n";
        std::cout << "  " << nav.steering() << "\n";
        std::cout << "  " << nav.exact_text() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && a.has("designer"))
    {
        // A picture of the filter designer (for documentation and testing): --design FILE opens a
        // plugin in it; --script runs keys and typed text ("Down,Right,=some words,Return").
        Designer d(window, renderer, a.get("design"), a.has("design-melody"));
        // --busy: the picture while the first test is still running (the progress window).
        const bool busy = a.has("busy");
        if (!busy) d.settle();
        if (a.has("script"))
        {
            std::string script = a.get("script");
            size_t at = 0;
            while (at <= script.size())
            {
                const size_t comma = script.find(',', at);
                const std::string item = script.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                if (!item.empty() && item[0] == '=') d.type(item.substr(1));
                else if (!item.empty())
                    for (const auto& [key, mod] : parse_presses(item)) d.press(key, mod);
                d.settle();
                if (comma == std::string::npos) break;
                at = comma + 1;
            }
        }
        if (busy)
        {
            d.press(SDLK_F5, SDL_KMOD_NONE); // starts the test on the worker
            SDL_Delay(400);
        }
        else d.settle();
        d.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        if (busy) d.settle(); // never leave the worker running as the program ends
        std::cout << "designer: " << d.doc().name() << "\n";
        if (d.played()) std::cout << "designer played: " << d.played() << " (" << d.status() << ")\n";
        if (!d.status().empty()) std::cout << "status: " << d.status() << "\n";
        if (a.has("design-out"))
        {
            std::ofstream out(std::filesystem::path(a.get("design-out")), std::ios::binary);
            out << d.doc().to_text();
        }
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && a.has("harness"))
    {
        // A picture of the AI Training Harness (for documentation and testing): --training-dir DIR
        // for its runs (else the training folder beside the programs); --script runs keys and typed
        // text as the designer's does, each step waiting for a training to finish.
        Harness harness(window, renderer, a.has("training-dir") ? std::filesystem::path(a.get("training-dir")) : training_dir(), ai_shape_of(settings),
                        picture_source(settings, filters_path));
        harness.set_scripted(true);
        if (a.has("script"))
        {
            std::string script = a.get("script");
            size_t at = 0;
            while (at <= script.size())
            {
                const size_t comma = script.find(',', at);
                const std::string item = script.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                if (!item.empty() && item[0] == '=') harness.type(item.substr(1));
                else if (!item.empty())
                    for (const auto& [key, mod] : parse_presses(item)) harness.press(key, mod);
                harness.settle();
                if (comma == std::string::npos) break;
                at = comma + 1;
            }
        }
        if (harness.going())
        {
            // GO TO IT: the hallway on the AI line, in the run's shape, the model in hand.
            Args ga = a;
            for (const char* k : {"harness", "script", "training-dir"}) ga.opts.erase(k);
            ga.opts["line"] = "ai";
            ga.opts["ai-layers"] = std::to_string(harness.go_shape.layers);
            ga.opts["ai-width"] = std::to_string(harness.go_shape.width);
            ga.opts["ai-heads"] = std::to_string(harness.go_shape.heads);
            ga.opts["ai-bits"] = std::to_string(harness.go_shape.bits);
            auto hall = make_hallway(window, renderer, ga, true, filters, app.angle_decimals);
            hall->go_to_ai(harness.go_digits);
            hall->render();
            if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
            std::cout << "went to the model on the AI line: " << hall->status() << "\n";
            std::cout << "saved " << a.get("screenshot") << "\n";
            if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
            return finish();
        }
        harness.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "harness: " << (harness.verdict().empty() ? "-" : harness.verdict()) << "\n";
        if (!harness.status().empty()) std::cout << "status: " << harness.status() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && a.has("main-menu") && a.has("locate"))
    {
        FileLocator::script(a.has("stop-after") ? std::stod(a.get("stop-after")) : 0, a.get("locate-save", ""),
                            a.has("shot-after") ? std::stod(a.get("shot-after")) : 0, a.has("force-after") ? std::stod(a.get("force-after")) : 0);
        // A picture of the File Locator opened from the main menu, on a file or folder (--tailored:
        // and its filters tailored): no world, so no Go to it.
        std::optional<LocatedGo> go; // offered as from the main menu (the screenshot does not go)
        run_locator(window, renderer, settings, filters, filters_path, a.get("locate"), a.has("tailored"), a.get("screenshot"), a.get("press", ""), &go, locator_search_of(a));
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && a.has("main-menu"))
    {
        // A picture of the main menu; --press keys go to it (e.g. Down,Enter for Settings).
        MainMenu mm(window, renderer, app, a.has("settings") ? app_path : std::filesystem::path(), display);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) mm.press(key, mod);
        mm.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "main menu: language " << language_code() << ", font " << font_name() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot && a.has("personal-world"))
    {
        // A picture of your personal dimension's world (IDEAS 19.187): your character and the store's
        // on the hill, --pworld-walk SECONDS walked forward first, --press keys to the catalogue
        // (Tab, Up, Down, Return, Backspace), --pworld-back SECONDS walked back after them, --world-file FILE the placements read and kept (else
        // beside --settings, else none).
        std::vector<sieve::Character> chars;
        if (auto ch = character_of(app, a)) chars.push_back(*ch);
        if (!a.has("no-character"))
            for (auto& c : read_personal_dimension(a.has("character-store") ? std::filesystem::path(a.get("character-store")) : character_store_dir(app_path)).characters) chars.push_back(std::move(c));
        const std::filesystem::path wf = a.has("world-file") ? sieve::from_u8(a.get("world-file"))
                                         : a.has("settings")  ? personal_world_file(app_path)
                                                              : std::filesystem::path();
        hallway::PersonalWorld world(window, renderer, std::move(chars), wf);
        world.set_wireframe(!(app.real_graphics || a.has("real-graphics")));
        world.set_features(app.personal_map && !a.has("no-map"), app.personal_compass && !a.has("no-compass"));
        world.set_hallway_tools(app.personal_hallway_tools);
        if (a.has("pworld-walk")) world.hold(SDL_SCANCODE_W, std::stof(a.get("pworld-walk")));
        if (a.has("pworld-look")) world.look(std::stof(a.get("pworld-look")));
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press")))
            {
                if (const std::string* id = hallway::item_press_id(key))
                {
                    // Item:ID: the pause menu's row of that name (press_names.hpp).
                    if (!world.pause_select(*id)) throw std::invalid_argument("--press: no row '" + *id + "' in your dimension's pause menu (is it open?)");
                    continue;
                }
                world.press(key, mod);
                world.hold(SDL_SCANCODE_UNKNOWN, 1.0f / 60); // (a frame between keys: what is held follows the crosshair)
            }
        if (a.has("pworld-wait")) world.hold(SDL_SCANCODE_UNKNOWN, std::stof(a.get("pworld-wait"))); // (what was dropped falls and rests)
        world.settle_panels(); // (a picture shows the panels where they are going, as the setup menu's do)
        if (a.has("pworld-back")) world.hold(SDL_SCANCODE_S, std::stof(a.get("pworld-back"))); // (to see what was placed)
        world.render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "personal world: " << world.report() << "\n";
        if (world.opened()) std::cout << "opened: " << world.opened()->line << " " << world.opened()->sha << "\n";
        if (world.pause_open())
        {
            std::string rows;
            for (const auto& r : world.pause_list()) rows += (rows.empty() ? "" : ", ") + r;
            std::cout << "pause menu: " << rows << "\n";
        }
        if (!world.system().empty()) std::cout << "goes to: " << world.system() << "\n";
        std::cout << "features: map " << (app.personal_map && !a.has("no-map") ? "on" : "off") << ", compass " << (app.personal_compass && !a.has("no-compass") ? "on" : "off") << "\n";
        std::cout << "map: " << world.map_across() << " m across\n";
        if (!hallway::PersonalWorld::catalogue_note().empty()) std::cout << "catalogue: not read: " << hallway::PersonalWorld::catalogue_note() << "\n";
        std::cout << "saved " << a.get("screenshot") << "\n";
        return finish();
    }
    if (shot && a.has("menu"))
    {
        // A picture of the setup menu (for documentation and testing); --press keys go to the menu.
        if (a.has("real-graphics")) app.real_graphics = true; // (characters drawn as the drawing row says)
        Menu menu(window, renderer, settings, filters, filters_path, &app, a.has("settings") ? app_path : std::filesystem::path());
        if (a.has("character")) menu.load_character(a.get("character"));
        // Into sphere 0, as Ctrl+I does into the chosen one (--import-obj is the older name, kept).
        if (a.has("import-model")) menu.import_model(a.get("import-model"));
        else if (a.has("import-obj")) menu.import_model(a.get("import-obj"));
        if (a.has("import-skeleton")) menu.import_skeleton(a.get("import-skeleton"));
        if (a.has("animation-frame")) menu.set_animation_frame(int(a.get_u32("animation-frame", 0)));
        if (a.has("save-character")) menu.character_chosen(a.get("save-character"), true); // as Ctrl+S does, saved as the menu is drawn
        if (a.has("catalogue-entry")) menu.open_catalogue(a.get("catalogue-entry"));
        if (a.has("system-viewer")) menu.open_viewer(a.get("system-viewer"));
        if (a.has("creator")) menu.open_creator(true);
        if (a.has("press"))
            for (const auto& [key, mod] : parse_presses(a.get("press"))) menu.press(key, mod);
        keep_setup(settings, menu.settings());
        menu.render();              // starts the counting workers
        // A picture shows the counts, not "counting..." (--busy: while they are still being
        // counted, if they take long enough to be caught).
        if (!a.has("busy"))
        {
            finish_filter_warmup();
            menu.render();
        }
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        // Only one of the two panels is in at a time (and neither in the System Viewer).
        if (menu.viewer()) std::cout << "system viewer: open\ncatalogue: " << menu.catalogue_line() << "\n";
        else if (menu.creator())
            std::cout << "character creator: open\ncharacter: " << menu.character_id() << "\nactions: " << menu.actions_line() << "\nsave: " << menu.save_line() << "\n"
                      << (menu.character_note().empty() ? std::string() : "note: " + menu.character_note() + "\n");
        else std::cout << "settings panel: " << (menu.panel_open() ? "in" : "out") << "\n";
        if (menu.side_open() && menu.side_page() == 3)
        {
            std::string sp;
            for (const auto& id : menu.speaks()) sp += (sp.empty() ? "" : ",") + id;
            std::cout << "right-hand panel: open (LANGUAGE)\nspeaks: " << (sp.empty() ? "none" : sp) << "\nsave: " << menu.save_line() << "\n";
        }
        else if (menu.side_open() && menu.side_page() == 2) std::cout << "right-hand panel: open (CATALOGUE)\ncatalogue: " << menu.catalogue_line() << "\n";
        else if (menu.side_open() && menu.side_page() == 1) std::cout << "right-hand panel: open (YOURS, " << menu.yours_count() << " characters)\n";
        else if (menu.side_open())
        {
            std::cout << "right-hand panel: open (CHARACTER)\n" << "character: " << menu.character_id() << "\n";
            if (!menu.animation_line().empty()) std::cout << "animation: " << menu.animation_line() << "\n";
            std::string sp;
            for (const auto& id : menu.speaks()) sp += (sp.empty() ? "" : ",") + id;
            std::cout << "speaks: " << (sp.empty() ? "none" : sp) << "\nsave: " << menu.save_line() << "\n";
            if (!menu.character_note().empty()) std::cout << "note: " << menu.character_note() << "\n";
        }
        if (a.has("busy")) finish_filter_warmup(); // never leave the workers counting as the program ends
        for (const auto& [line, text] : walk_estimate_lines(menu.settings())) std::cout << "estimate: " << text << "\n";
        for (const auto& [kind, text] : menu.details_lines()) std::cout << "details: " << text << "\n"; // (I, or the dots: a line's details)
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        return finish();
    }
    if (shot)
    {
        auto hall = [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, a, true, filters, app.angle_decimals);
        }();
        {
            const bool glow = app.edge_glow || a.has("edge-glow"), real = app.real_graphics || a.has("real-graphics");
            // --real-graphics brings Door Portals with it, as turning it on in the menu does. The
            // saved setting is taken as it stands: a settings file that turns portals off with
            // Real Graphics on means exactly that, since nothing may turn them off but you.
            const bool portals = app.door_portals || a.has("real-graphics") || a.has("door-portals");
            hall->set_graphics(glow && !real, real, portals);
            hall->set_model_files(a.get("text-model", app.text_model), a.get("picture-model", app.picture_model));
            hall->set_model_cache(a.has("model-cache") ? int(a.get_u32("model-cache", 64)) : app.model_cache_mb);
            hall->set_angle_decimals(app.angle_decimals);
            hall->set_face_px(a.get_u32("model-tile", 64));
            hall->set_letters_px(a.has("item-letters") ? a.get_u32("item-letters", 8) : 8);
            hall->set_export_fps(a.has("video-fps") ? a.get_u32("video-fps", sieve::cli::kDefaultExportFps) : sieve::cli::kDefaultExportFps);
            hall->set_closeup_px(closeup_setting(a.get("close-up", "screen"), kCloseUpScreen));
            hall->set_graphics_memory(app.graphics_memory_gb);
            hall->set_fps_counter(app.fps_counter || a.has("fps-counter"));
            {
                auto ch = character_of(app, a);
                // (said only when asked for by name: other scripts read the readout's last lines)
                const bool named = ch && a.has("character");
                const std::string id = named ? ch->id() : std::string();
                hall->set_max_scale(float(app.character_max_scale));
                hall->set_character(std::move(ch));
                hall->set_animation(animation_of(app, app_path, a), a.has("animation-frame") ? int(a.get_u32("animation-frame", 0)) : -1);
                {
                    auto [map, anims] = actions_of(app, app_path, a);
                    hall->set_actions(std::move(map), std::move(anims));
                    if (a.has("action")) hall->set_action(int(sieve::action_from_name(a.get("action")))); // (a picture of one action)
                }
                // Your personal dimension: the store beside the settings file (or --character-store DIR).
                if (!a.has("no-character"))
                {
                    const auto pd = read_personal_dimension(a.has("character-store") ? std::filesystem::path(a.get("character-store")) : character_store_dir(app_path));
                    hall->set_personal(pd.characters);
                    if (a.has("character-store")) std::cout << "personal dimension: " << pd.characters.size() << " characters, " << pd.left_out << " left out\n";
                }
                if (named)
                {
                    std::cout << "character beside you: " << id << "\n";
                    std::cout << "hallway scale: " << hallway::hall::fixed(hall->hall_scale(), 2) << (hall->character_fits() ? "" : " (the most it may grow: the character does not fit)") << "\n";
                }
                if (a.has("animation")) std::cout << "animation beside you: " << animation_of(app, app_path, a)->id() << "\n";
            }
            if (a.has("thin")) hall->set_thin(true); // (a walk to a long file in --press may have made it thin already)
            // On stderr: stdout is where the readout goes, which scripts read line by line.
            std::cerr << "graphics: edge glow " << (glow && !real ? "on" : "off") << ", real graphics " << (real ? "on" : "off")
                      << ", door portals " << (portals ? "on" : "off")
                      << ", fps counter " << (app.fps_counter || a.has("fps-counter") ? "on" : "off") << "\n";
        }
        if (a.has("bench"))
        {
            // Frame timing: render N frames (turning slowly, so nothing is cached between them).
            const uint32_t n = a.get_positive("bench", 60);
            hall->render();
            const Uint64 t0 = SDL_GetTicksNS();
            for (uint32_t i = 0; i < n; ++i)
            {
                hall->camera().yaw += 0.002f;
                hall->render();
                SDL_RenderPresent(renderer); // includes drawing the batched geometry
            }
            const double ms = double(SDL_GetTicksNS() - t0) / 1e6 / n;
            std::cout << "bench: " << n << " frames, " << ms << " ms per frame (" << 1000.0 / ms << " fps), renderer "
                      << SDL_GetRendererName(renderer) << "\n";
        }
        if (a.has("settle"))
        {
            // Draw N frames standing still, a few milliseconds apart, so that what is drawn in the
            // background (the pictures on the items) has time to arrive before the picture is taken.
            const uint32_t n = a.get_positive("settle", 30);
            for (uint32_t i = 0; i < n; ++i)
            {
                hall->render();
                SDL_RenderPresent(renderer);
                SDL_Delay(10);
            }
        }
        hall->render(); // computes what you are looking at
        hall->settle_vault(); // the vault's verdicts on the pictures in view, so the picture shows them
        if (a.has("take")) hall->take_hovered();
        // The AI line: the chat window open on the model in hand (--chat), a file given to it
        // (--chat-attach FILE), figures drawn with its picture model (--chat-draw N), something
        // said to it (--talk TEXT), and the conversation printed.
        if (a.has("chat")) hall->open_chat();
        if (a.has("chat-attach"))
        {
            // Several files apart by ';', given in turn.
            const std::string all = a.get("chat-attach");
            for (size_t at = 0; at <= all.size();)
            {
                const size_t semi = std::min(all.find(';', at), all.size());
                if (semi > at) hall->chat_attach(all.substr(at, semi - at));
                at = semi + 1;
            }
        }
        if (a.has("chat-draw"))
            for (uint32_t i = a.get_positive("chat-draw", 1); i > 0; --i) hall->chat_draw();
        // CHAIN: a chain opened (--chain-file), the tab shown (--chain), keys pressed there
        // (--chain-press), and the chain saved (--chain-save); /chain in --talk runs it.
        if (a.has("chain-file")) hall->chain_load(a.get("chain-file"));
        if (a.has("chain") || a.has("chain-press")) hall->open_chain();
        if (a.has("chain-press"))
            for (const auto& [key, mod] : parse_presses(a.get("chain-press")))
            {
                SDL_Event e{};
                e.type = SDL_EVENT_KEY_DOWN;
                e.key.key = key;
                e.key.scancode = SDL_GetScancodeFromKey(key, nullptr); // as a real key brings it (the right-hand panel reads it)
                e.key.mod = mod;
                bool quit = false;
                hall->handle(e, quit);
            }
        if (a.has("chain-save")) hall->chain_save(a.get("chain-save"));
        if (a.has("talk")) hall->talk(a.get("talk"));
        if (a.has("chat-attach") || a.has("chat-draw") || a.has("talk"))
        {
            std::string line;
            for (const char c : hall->conversation() + "\n")
                if (c == '\n')
                {
                    if (!line.empty()) std::cout << Hallway::printable(line) << "\n";
                    line.clear();
                }
                else line.push_back(c);
        }
        // The system prompt a model file would be given here, word for word (docs/SYSTEM-PROMPT.md).
        if (a.has("print-system-prompt")) std::cout << hall->system_prompt();
        // Keys pressed in the chat window after that (Ctrl+G: the newest card's GO TO IT, ...),
        // with where you are after each.
        if (a.has("chat-press"))
            for (const auto& [key, mod] : parse_presses(a.get("chat-press")))
            {
                SDL_Event e{};
                e.type = SDL_EVENT_KEY_DOWN;
                e.key.key = key;
                e.key.scancode = SDL_GetScancodeFromKey(key, nullptr); // as a real key brings it (the right-hand panel reads it)
                e.key.mod = mod;
                bool quit = false;
                hall->handle(e, quit);
                hall->render();
                std::cout << hall->status() << "\n";
            }
        // COST's shortest route for the item in hand, searched to --route-places N (L), waited for.
        if (a.has("route-places")) std::cout << "route: " << hall->route_now(int(a.get_positive("route-places", 20))) << "\n";
        // K on the item in hand, waited for, and what Return would apply saved as a settings file.
        if (a.has("tailor") && !hall->tailor_now(a.get("tailor")))
            std::cerr << "--tailor: nothing tailored (no item in hand on the pages, image, audio or video line)\n";
        if (a.has("save-item")) hall->save_in_hand_to(a.get("save-item")); // F, without the dialog
        if (a.has("save-view")) hall->save_view_to(a.get("save-view"));   // F in the viewer, without the dialog
        if (a.has("sample-degrees"))
            std::cout << hall->sample_degrees(a.get("sample-degrees"), a.get("sample-out", a.get("sample-degrees") + ".sieve"));
        // The music visualiser's rings (IDEAS 19.175) with those notes held sounding, --rings-zoom steps in.
        if (a.has("rings"))
        {
            std::vector<float> notes;
            const std::string list = a.get("rings");
            for (size_t i = 0; i <= list.size();)
            {
                const size_t c = std::min(list.find(',', i), list.size());
                if (c > i) notes.push_back(float(std::stod(list.substr(i, c - i))));
                i = c + 1;
            }
            hall->show_rings(notes, int(a.get_u32("rings-zoom", 0)));
        }
        // A recording played through (IDEAS 19.184), every frame as it was, then the picture of where it
        // ends; --record FILE with it writes what was played (the same, frame for frame).
        if (a.has("replay"))
        {
            const Replay r = load_replay_file(a.get("replay"), replay_cached(app, a, a.get("replay")));
            replay_begin(*hall, r);
            Replay again = replay_start_of(*hall);
            bool q = false;
            for (const auto& f : r.frames)
            {
                replay_frame(*hall, f, q);
                again.frames.push_back(f);
            }
            std::cout << "replay: " << r.frames.size() << " frames, " << hallway::hall::fixed(r.seconds(), 3) << " s\n" << hall->status() << "\n";
            if (a.has("record")) save_replay_file(a.get("record"), again);
        }
        else if (a.has("record")) save_replay_file(a.get("record"), replay_start_of(*hall)); // (where it begins, no frames: a start to build on)
        hall->render();
        if (!save_render(renderer, a.get("screenshot"))) throw std::runtime_error(std::string("screenshot failed: ") + SDL_GetError());
        std::cout << "saved " << a.get("screenshot") << "\n";
        if (a.has("rings") || hall->rings_open()) std::cout << "rings: " << hall->rings_report() << "\n";
        if (a.has("action") || hall->has_actions()) std::cout << "action: " << hall->action_line() << "\n"; // (IDEAS 19.186)
        if (read_aloud() && !last_announced().empty()) std::cout << "read aloud: " << last_announced() << "\n";
        if (!last_voiced().empty()) std::cout << "voice: " << last_voiced() << "\n"; // (Text to Voice's reply, or CHAT's /say)
        hall->release_textures();
        hall.reset(); // before SDL goes (its audio stream, its textures)
        return finish();
    }

    // Create .sieve (Explorer's right-click menu, IDEAS 19.100, as Edward decided on 29 October
    // 2026): the File Locator alone, no hallway behind it, on PATH; NAME.sieve saved beside it
    // (--out: there instead), said for a moment, and closed, with 0 for a script to read; stopped
    // or failed, it stays open saying why, and closing it gives 1.
    if (a.has("create"))
    {
        std::string what = a.get("create");
        while (what.size() > 1 && (what.back() == '/' || what.back() == '\\')) what.pop_back();
        // Several files or folders chosen (Edward, 6 November 2026: "treat the multiple selected
        // files as a directory"): Explorer starts one program for each, so the first gathers the
        // others' paths and makes one .sieve of them, the folder they are in holding just them,
        // named after it; the others hand over their path and close. (Not with --out: a script
        // names one thing.)
        std::vector<std::string> only;
        std::string folder = what;
        if (create_gathered) // (gathered at the start, above)
        {
            const auto& gathered = create_gathered;
            if (gathered->size() > 1)
            {
                folder = utf8_of(std::filesystem::path(path_of_utf8(what)).parent_path());
                for (const std::string& p : *gathered) only.push_back(utf8_of(std::filesystem::path(path_of_utf8(p)).filename()));
            }
        }
        const std::string out = a.has("out") ? a.get("out")
                                : only.empty() ? what + ".sieve"
                                               : utf8_of(std::filesystem::path(path_of_utf8(folder)) /
                                                           (std::filesystem::path(path_of_utf8(folder)).filename().u8string() + u8".sieve"));
        int created = 0;
        run_locator(window, renderer, settings, filters, filters_path, folder, false, "", "", nullptr, locator_search_of(a), out, &created, only);
        finish();
        return created ? 0 : 1;
    }
    // Interactive: the main menu, then the setup menu, then the hallway (--no-menu goes straight
    // in). F1 in the hallway returns to the setup menu, and Esc there to the main menu.
    bool show_menu = !a.has("no-menu");
    bool settings_chosen = false; // settings came from a setup menu (F1 in game), even with --no-menu
    bool went_in_thin = false;    // the setup menu was told to go in past the budget
    bool show_main = show_menu;
    bool open_yours = false; // Personal Dimensions chosen: the setup menu opens with YOURS on the right
    std::string open_system; // the System Viewer chosen: it opens on that system, the whole window (Esc: the main menu)
    bool open_creator = false; // the Character Creator chosen: the whole window, Esc back to the main menu
    bool first = true;
    bool first_hallway_of_run = true; // (--record and --replay: the run's first hallway)
    std::string pending_replay;       // a recording chosen in the replay viewer, played in the next hallway
    std::optional<MusicMelody> pending_melody; // a melody of the music to go to in the next hallway built
    std::optional<Space::Digits> pending_unit; // an item to hold on its COST tab there (COST's tailoring)
    std::optional<hallway::Favourite> pending_favourite; // a favourite opened in your dimension, to hold there
    // Your personal dimension: its own world (IDEAS 19.187), your character and the store's on the
    // hill, what you place there kept beside the settings. The same from the main menu and from the
    // hallway's pause menu; a favourite opened there with O is given back in `opened`.
    // Its pause menu (pause_rows.hpp): Settings, its Personal Dimensions screen and the Filter Designer
    // open over it and come back to it; any other system's row is given back in `system`, for the
    // caller to open (the main menu's way, or the paused hallway's).
    auto run_personal_world = [&](std::optional<hallway::Favourite>& opened, std::string& system) {
        std::vector<sieve::Character> chars;
        if (auto ch = character_of(app, a)) chars.push_back(*ch);
        for (auto& c : read_personal_dimension(character_store_dir(app_path)).characters) chars.push_back(std::move(c));
        hallway::PersonalWorld world(window, renderer, std::move(chars), personal_world_file(app_path));
        auto apply = [&] {
            world.set_wireframe(!app.real_graphics);
            world.set_features(app.personal_map, app.personal_compass);
            world.set_hallway_tools(app.personal_hallway_tools);
        };
        apply();
        for (;;)
        {
            const auto res = world.run();
            if (world.opened()) opened = *world.opened();
            if (res != hallway::PersonalWorld::Result::System) return res;
            const std::string id = world.system();
            if (id == "settings" || id == "personal")
            {
                // (Personal Dimension chosen there: this one, already open)
                MainMenu mm(window, renderer, app, app_path, display);
                if ((id == "settings" ? mm.run_settings() : mm.run_personal()) == MainMenu::Result::Quit) return hallway::PersonalWorld::Result::Quit;
                apply();
                continue;
            }
            if (id == "designer")
            {
                Designer designer(window, renderer);
                if (designer.run() == Designer::Result::Quit) return hallway::PersonalWorld::Result::Quit;
                continue;
            }
            system = id;
            return res;
        }
    };
    // A system chosen in your dimension's pause menu (from the main menu): opened as the main menu opens
    // it, and back to your dimension after (back_to_world), unless it goes into the hallway.
    std::optional<MainMenu::Result> forced_r;
    bool back_to_world = false;
    std::string pending_tool; // a hallway tool to open once the hallway is built (graph, navigator)
    std::optional<sieve::AiSpace::Digits> pending_ai; // a model to go to on the AI line there (the harness's GO TO IT)
    std::optional<LocatedGo> pending_file;            // a file to walk to there (the File Locator's Go to it)
    std::optional<std::vector<uint32_t>> pending_picture; // a picture to go to there (the chat window's Go to it)
    bool pending_media = false;                       // the Media Player opened there (from the main menu)
    bool skip_setup = false;                         // straight into the hallway, once
    while (true)
    {
        if (show_main)
        {
            MainMenu mm(window, renderer, app, app_path, display);
            MainMenu::Result r;
            if (forced_r) r = *forced_r, forced_r.reset();
            else if (back_to_world) r = MainMenu::Result::Personal, back_to_world = false;
            else r = mm.run();
            if (r == MainMenu::Result::Quit) break;
            if (r == MainMenu::Result::Locator)
            {
                // The File Locator, and back to the main menu from it.
                std::optional<LocatedGo> go;
                if (!run_locator(window, renderer, settings, filters, filters_path, "", false, "", "", &go)) break;
                if (!go) continue;
                // Go to it: into the hallway as the settings stand, then to the file.
                pending_file = std::move(go);
                settings_chosen = true;
                skip_setup = true;
            }
            if (r == MainMenu::Result::Harness)
            {
                // The AI Training Harness; GO TO IT goes into the hallway on the AI line, at the
                // model, in the run's shape (and without the setup menu: the shape is the run's).
                Harness harness(window, renderer, training_dir(), ai_shape_of(settings),
                                picture_source(settings, filters_path));
                const Harness::Result hr = harness.run();
                if (hr == Harness::Result::Quit) break;
                if (hr == Harness::Result::Back) continue;
                settings.ai_layers = harness.go_shape.layers;
                settings.ai_width = harness.go_shape.width;
                settings.ai_heads = harness.go_shape.heads;
                settings.ai_bits = harness.go_shape.bits;
                settings.start_line = "ai";
                settings_chosen = true;
                skip_setup = true;
                pending_ai = harness.go_digits;
            }
            if (r == MainMenu::Result::Media)
            {
                // The Media Player is the hallway's (its Go to it walks to a melody): the hallway
                // is built as the settings stand, with the player open over it; Esc there comes
                // back here.
                pending_media = true;
                settings_chosen = true;
                skip_setup = true;
            }
            if (r == MainMenu::Result::Cipher)
            {
                // The Encryption Engine, and back to the main menu from it; GO TO IT goes into the
                // hallway as the settings stand, in front of the output on the binary line (made
                // long enough for it, as the File Locator's Go to it does).
                CipherScreen engine(window, renderer);
                const CipherScreen::Result er = engine.run();
                if (er == CipherScreen::Result::Quit) break;
                if (er != CipherScreen::Result::GoTo) continue;
                pending_file = LocatedGo{engine.go_bytes(), engine.go_name(), engine.go_bytes().size() > settings.binary_bytes};
                settings_chosen = true;
                skip_setup = true;
            }
            if (r == MainMenu::Result::Personal)
            {
                // Your personal dimension (IDEAS 19.187); Esc back to the main menu.
                std::optional<hallway::Favourite> opened;
                std::string sys;
                const auto res = run_personal_world(opened, sys);
                if (res == hallway::PersonalWorld::Result::Quit) break;
                if (res == hallway::PersonalWorld::Result::System)
                {
                    const std::map<std::string, MainMenu::Result> as_main = {{"locator", MainMenu::Result::Locator}, {"media", MainMenu::Result::Media},
                                                                            {"harness", MainMenu::Result::Harness}, {"cipher", MainMenu::Result::Cipher}};
                    if (const auto it = as_main.find(sys); it != as_main.end())
                    {
                        forced_r = it->second;
                        back_to_world = true;
                        continue;
                    }
                    if (sys == "graph" || sys == "navigator")
                    {
                        // A hallway tool: into the hallway as the settings stand, and the tool open there.
                        pending_tool = sys;
                        settings_chosen = true;
                        skip_setup = true;
                    }
                    else if (sys == "systems") open_system = "information";
                    // ("restart": the setup menu, as Start Sieve)
                    show_main = false;
                    back_to_world = false;
                    continue;
                }
                if (res != hallway::PersonalWorld::Result::Open || !opened) continue;
                // A favourite opened there (O): into the hallway on its line, as the settings stand, and
                // into your hand (IDEAS 19.190).
                pending_favourite = *opened;
                for (const auto& d : hallway::kDimensions)
                    if (pending_favourite->line == d.id) settings.start_line = d.line;
                settings_chosen = true;
                skip_setup = true;
            }
            if (r == MainMenu::Result::Systems) open_system = mm.system_root(); // the System Viewer on the system
            if (r == MainMenu::Result::Creator) open_creator = true;
            if (r == MainMenu::Result::Replay)
            {
                // A recording chosen in the replay viewer: into the hallway as the settings stand, then
                // played from where it began.
                pending_replay = sieve::u8(mm.replay_chosen());
                settings_chosen = true;
                skip_setup = true;
            }
            if (r == MainMenu::Result::ManualNav)
            {
                // Manual Navigation, and back to the main menu from it. Keep a log and the undo depth
                // as the settings have them ([manual]), saved when changed there.
                ManualNav::remember({app.manual_log, app.manual_undo, app.manual_fresh, app.manual_width}, app_path.parent_path() / "logs",
                                    [&app, app_path](const ManualNav::Kept& k) {
                                        app.manual_log = k.keep_log;
                                        app.manual_undo = k.undo_depth;
                                        app.manual_fresh = k.fresh;
                                        app.manual_width = k.width;
                                        app.save(app_path);
                                    });
                ManualNav nav(window, renderer, app.angle_decimals);
                if (nav.run() == ManualNav::Result::Quit) break;
                continue;
            }
            if (r == MainMenu::Result::Designer)
            {
                // The filter designer, and back to the main menu from it.
                Designer designer(window, renderer);
                if (designer.run() == Designer::Result::Quit) break;
                continue;
            }
            show_main = false;
        }
        if (show_menu && !skip_setup)
        {
            Menu menu(window, renderer, settings, filters, filters_path, &app, app_path);
            if (open_yours) menu.open_yours();
            open_yours = false;
            if (!open_system.empty()) menu.open_viewer(open_system); // the System Viewer: the window its own, Esc back here
            open_system.clear();
            if (open_creator) menu.open_creator(true); // the Character Creator: the same, Esc back here
            open_creator = false;
            const Menu::Result r = menu.run();
            keep_setup(settings, menu.settings());
            if (r == Menu::Result::Quit) break;
            went_in_thin = menu.went_in_thin();
            settings = menu.settings();
            filters = menu.filters();
            if (r == Menu::Result::Back)
            {
                show_main = true;
                continue;
            }
        }
        Args ha = a;
        // The setup as the menu left it, or as the settings file has it (when it says anything
        // the defaults do not, so that a run with neither is built from its options alone).
        if (show_menu || settings_chosen || !(app.setup == Settings{})) settings.apply(ha);
        if (!first)
            for (const char* k : {"warp", "goto", "zoom", "tile", "pose", "walk", "press"}) ha.opts.erase(k);
        // The hallway is built on a worker thread -- a large shape takes a while, and none of
        // building it touches the renderer -- while this thread keeps the window alive and says
        // it is still working: a red CALCULATING, its dots counting up and starting again. It
        // only appears if the building takes long enough to be noticed.
        bool quit_while_building = false;
        music_mode(MusicMode::Menus); // building is still the menus
        music_colours_default();
        // The hallway counts with the filter memory as set, pressed X or not (and the setup menu,
        // built again when it next opens, counts with it too).
        sieve::set_filter_memory(double(app.filter_memory_mb) * 1024 * 1024);
        sieve::set_unit_time_ms(app.unit_time_ms); // (and the time budget, as set)
        set_view(app.view_rooms, app.picture_rooms); // (and the view distance, as Settings > Graphics has it)
        auto building = std::async(std::launch::async, [&] {
            sieve::cli::timings::Scope timed("hallway.build");
            return make_hallway(window, renderer, ha, first, filters, app.angle_decimals);
        });
        const Uint64 building_since = SDL_GetTicks();
        while (building.wait_for(std::chrono::milliseconds(16)) != std::future_status::ready)
        {
            SDL_Event e;
            while (SDL_PollEvent(&e))
                if (e.type == SDL_EVENT_QUIT) quit_while_building = true;
            const Uint64 waited = SDL_GetTicks() - building_since;
            if (waited < 200) continue;
            int ww = 0, wh = 0;
            SDL_GetCurrentRenderOutputSize(renderer, &ww, &wh);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            const float scale = 4.0f;
            const std::string word = tr("calculating");
            // Centred on the word alone, so the dots grow to the right and it does not jump.
            const float x = (float(ww) - float(word.size()) * 8.0f * scale) * 0.5f, y = (float(wh) - 8.0f * scale) * 0.5f;
            draw_text(renderer, x, y, word + std::string(size_t((waited / 400) % 4), '.'), scale, SDL_Color{255, 60, 60, 255});
            present(renderer);
        }
        auto hall = building.get(); // rethrows here anything the building threw
        if (quit_while_building) break;
        hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
        hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
        hall->set_model_files(app.text_model, app.picture_model);
        hall->set_model_cache(app.model_cache_mb);
        hall->set_angle_decimals(app.angle_decimals);
        hall->set_face_px(ha.get_u32("model-tile", 64));
        hall->set_letters_px(ha.has("item-letters") ? ha.get_u32("item-letters", 8) : 8);
        hall->set_export_fps(ha.has("video-fps") ? ha.get_u32("video-fps", sieve::cli::kDefaultExportFps) : sieve::cli::kDefaultExportFps);
        hall->set_closeup_px(closeup_setting(ha.get("close-up", "screen"), kCloseUpScreen));
        hall->set_graphics_memory(app.graphics_memory_gb);
        hall->set_fps_counter(app.fps_counter);
        hall->set_max_scale(float(app.character_max_scale));
        hall->set_character(character_of(app, ha));
        hall->set_animation(animation_of(app, app_path, ha));
        {
            auto [map, anims] = actions_of(app, app_path, ha);
            hall->set_actions(std::move(map), std::move(anims));
        }
        if (!ha.has("no-character")) hall->set_personal(read_personal_dimension(ha.has("character-store") ? std::filesystem::path(ha.get("character-store")) : character_store_dir(app_path)).characters);
        hall->set_favourites_base(app_path.parent_path()); // (favourites/ and worlds/ beside the settings: IDEAS 19.190)
        hall->set_thin(went_in_thin || ha.has("thin"));
        first = false;
        if (pending_melody)
        {
            hall->go_to_melody(*pending_melody);
            pending_melody.reset();
        }
        if (pending_unit)
        {
            hall->hold_on_cost(*pending_unit);
            pending_unit.reset();
        }
        if (pending_favourite)
        {
            hall->open_favourite(*pending_favourite);
            pending_favourite.reset();
        }
        if (pending_ai)
        {
            hall->go_to_ai(*pending_ai);
            pending_ai.reset();
        }
        if (pending_file)
        {
            hall->go_to_located(pending_file->bytes, pending_file->name, pending_file->past);
            pending_file.reset();
        }
        if (pending_picture)
        {
            hall->go_to_picture(*pending_picture);
            pending_picture.reset();
        }
        if (pending_media)
        {
            hall->open_media_from_main();
            pending_media = false;
        }
        if (!pending_tool.empty())
        {
            hall->open_tool(pending_tool);
            pending_tool.clear();
        }
        skip_setup = false;
        SDL_SetWindowRelativeMouseMode(window, true);
        // Recording (--record FILE) and replay (--replay FILE), the first hallway of the run only
        // (IDEAS 19.184): a replay plays its frames in place of your inputs (closing the window still
        // closes it) and then hands the hallway back to you; a recording keeps every frame from here,
        // written each time the hallway is left.
        std::optional<Replay> playing, recording;
        size_t played = 0;
        std::string record_path;           // where the recording goes
        uint64_t record_bytes = 0;         // its text's length so far
        double record_seconds = 0;
        Uint64 record_checked = 0;         // when the free space was last looked at
        if (first_hallway_of_run && a.has("replay"))
        {
            playing = load_replay_file(a.get("replay"), replay_cached(app, a, a.get("replay")));
            replay_begin(*hall, *playing);
        }
        if (!pending_replay.empty())
        {
            try
            {
                playing = load_replay_file(pending_replay, replay_cached(app, a, pending_replay));
                replay_begin(*hall, *playing);
            }
            catch (const std::exception& ex)
            {
                playing.reset();
                hall->warn(trf("msg.replay_refused", {ex.what()}));
            }
            pending_replay.clear();
        }
        if (first_hallway_of_run && a.has("record"))
        {
            recording = replay_start_of(*hall);
            record_path = a.get("record");
        }
        first_hallway_of_run = false;
        // Recording stopped (F12, its limit, or too little space): written, and said.
        auto stop_recording = [&](const std::string& why_key, const std::string& why_arg) {
            if (!recording) return;
            try
            {
                save_replay_file(record_path, *recording);
                if (why_key.empty()) hall->message(trf("msg.record_off", {std::to_string(recording->frames.size()), hallway::hall::fixed(record_seconds, 1), record_path}));
                else hall->warn(trf(why_key.c_str(), {why_arg, record_path}));
                std::cout << "recorded  " << recording->frames.size() << " frames, " << hallway::hall::fixed(record_seconds, 1) << " s, to " << record_path << "\n";
            }
            catch (const std::exception& ex)
            {
                hall->warn(trf("msg.record_failed", {ex.what()}));
            }
            recording.reset();
            record_bytes = 0;
            record_seconds = 0;
        };
        // F12: a recording started, into `replays` beside the settings, named by when it began, packed.
        auto start_recording = [&] {
            std::error_code ec;
            const std::filesystem::path dir = app_path.parent_path() / "replays";
            std::filesystem::create_directories(dir, ec);
            const std::time_t t = std::time(nullptr);
            std::tm u{};
#ifdef _WIN32
            localtime_s(&u, &t);
#else
            localtime_r(&t, &u);
#endif
            char name[64];
            std::snprintf(name, sizeof name, "%04d-%02d-%02d-%02d%02d%02d.sieve", u.tm_year + 1900, u.tm_mon + 1, u.tm_mday, u.tm_hour, u.tm_min, u.tm_sec);
            record_path = sieve::u8(dir / name);
            recording = replay_start_of(*hall);
            record_bytes = recording->text().size();
            record_seconds = 0;
            record_checked = 0;
            hall->message(trf("msg.record_on", {std::to_string(app.replays_max_minutes), record_path}));
        };
        Menu::Result in_game_menu = Menu::Result::Back;
        bool harness_went = false; // the pause menu's harness went to its model: a hallway on the AI line
        std::optional<hallway::Favourite> favourite_elsewhere; // opened in your world from the pause menu, of another line
        bool requests_waiting = false; // a request made outside the hallway's loop (your dimension's pause menu)
        for (;;)
        {
            hall->music_line(); // back from a menu (or in for the first time): the line's colours and character
            bool quit = requests_waiting; // (a request already made: handled below before the hallway runs again)
            requests_waiting = false;
            Uint64 last = SDL_GetTicksNS();
            while (!quit)
            {
                SDL_Event e;
                ReplayFrame frame;
                while (SDL_PollEvent(&e))
                {
                    if (e.type == SDL_EVENT_KEY_DOWN && e.key.scancode == SDL_SCANCODE_F12 && !e.key.repeat && !playing)
                    {
                        // F12 starts and stops a recording (itself never recorded).
                        if (recording) stop_recording("", "");
                        else start_recording();
                        continue;
                    }
                    if (playing)
                    {
                        if (e.type == SDL_EVENT_QUIT) hall->handle(e, quit); // (while a replay plays, only closing is yours)
                        continue;
                    }
                    hall->handle(e, quit);
                    if (recording && !replay_token(e).empty()) frame.events.push_back(e);
                }
                pump_speech(); // (a reply Sieve's voice has made, played)
                const Uint64 now = SDL_GetTicksNS();
                float dt = std::min(0.1f, float(now - last) / 1e9f);
                last = now;
                if (playing && played < playing->frames.size())
                {
                    // The recording's frame in place of yours, kept to its own time.
                    const ReplayFrame& f = playing->frames[played++];
                    sieve::cli::timings::Scope timed("hallway.frame.update");
                    replay_frame(*hall, f, quit);
                    if (recording)
                    {
                        recording->frames.push_back(f);
                        record_bytes += replay_frame_line(f).size();
                        record_seconds += double(f.dt_us) / 1e6;
                    }
                    if (played == playing->frames.size())
                    {
                        hall->message(trf("msg.replay_done", {std::to_string(played)}));
                        playing.reset();
                    }
                }
                else
                {
                    if (recording)
                    {
                        // Rounded to the microsecond, so the replay takes the same steps.
                        frame.dt_us = std::clamp<uint32_t>(uint32_t(std::lround(double(dt) * 1e6)), 1, kReplayMaxDtUs);
                        dt = float(frame.dt_us) / 1e6f;
                        frame.held = held_keys(SDL_GetKeyboardState(nullptr));
                        record_bytes += replay_frame_line(frame).size();
                        record_seconds += double(frame.dt_us) / 1e6;
                        recording->frames.push_back(std::move(frame));
                    }
                    sieve::cli::timings::Scope timed("hallway.frame.update");
                    hall->update(dt, SDL_GetKeyboardState(nullptr));
                }
                // A recording's safety valves: its longest time, and the free space where it is kept,
                // looked at every five seconds (it needs at least [replays] min_free_mb, and twice
                // its own size to be written and packed). Either stops it, keeping what it has.
                if (recording)
                {
                    if (record_seconds >= double(app.replays_max_minutes) * 60) stop_recording("msg.record_limit", std::to_string(app.replays_max_minutes));
                    else if (now - record_checked >= 5000000000ull)
                    {
                        record_checked = now;
                        std::error_code ec;
                        const std::filesystem::path where = sieve::from_u8(record_path).parent_path();
                        const auto sp = std::filesystem::space(where.empty() ? std::filesystem::path(".") : where, ec);
                        const uint64_t need = std::max<uint64_t>(uint64_t(app.replays_min_free_mb) << 20, 2 * record_bytes);
                        if (!ec && sp.available < need) stop_recording("msg.record_disk", std::to_string(sp.available >> 20));
                    }
                }
                {
                    sieve::cli::timings::Scope timed("hallway.frame.render");
                    hall->render();
                }
                music_mode(hall->in_menu() ? MusicMode::Menus : MusicMode::World);
                {
                    sieve::cli::timings::Scope timed("hallway.frame.present"); // waits for vsync when it is on
                    present(renderer);
                }
            }
            // The recording so far, written each time the hallway is left (a menu, or closing); it
            // carries on if the same hallway is come back to.
            if (recording)
                try
                {
                    save_replay_file(record_path, *recording);
                    std::cout << "recorded  " << recording->frames.size() << " frames, " << hallway::hall::fixed(record_seconds, 1) << " s, to " << record_path << "\n";
                }
                catch (const std::exception& ex)
                {
                    std::cerr << "warning: the recording was not written: " << ex.what() << "\n";
                }
            // F1: the setup menu over the hallway, which is kept. Esc there comes back to it just
            // as it was; ENTER THE HALLWAY builds a new one from the new settings.
            if (hall->menu_requested())
            {
                Menu menu(window, renderer, settings, filters, filters_path, &app, app_path);
                menu.set_in_game(true);
                if (hall->take_systems_request()) menu.open_viewer("information"); // the System Viewer; Esc back to the hallway
                in_game_menu = menu.run();
                if (in_game_menu == Menu::Result::Back)
                {
                    hall->set_model_files(app.text_model, app.picture_model); // the AI rows' model files
                    hall->set_model_cache(app.model_cache_mb);
                    hall->set_graphics_memory(app.graphics_memory_gb);
                    SDL_SetWindowRelativeMouseMode(window, !hall->back_from_menu());
                    continue;
                }
                if (in_game_menu == Menu::Result::Enter)
                {
                    went_in_thin = menu.went_in_thin();
                    keep_setup(settings, menu.settings());
                    settings = menu.settings();
                    filters = menu.filters();
                }
                break;
            }
            // The Filter Designer and the AI Training Harness from the pause menu, as from the main
            // menu: over the hallway, and then the same hallway again, still paused. The harness's
            // GO TO IT goes to its model instead, in a hallway on the AI line in the run's shape.
            if (hall->request() == Hallway::Request::Cipher)
            {
                // From the pause menu, or from the item page's I with the item in hand as the input.
                hall->clear_request();
                CipherScreen engine(window, renderer);
                if (auto item = hall->take_cipher_item()) engine.give_item(std::move(item->first), std::move(item->second));
                const CipherScreen::Result er = engine.run();
                if (er == CipherScreen::Result::Quit) break;
                if (er == CipherScreen::Result::GoTo) hall->go_to_bytes(engine.go_bytes(), engine.go_name());
                SDL_SetWindowRelativeMouseMode(window, false);
                continue;
            }
            if (hall->request() == Hallway::Request::Designer)
            {
                hall->clear_request();
                Designer designer(window, renderer, {}, hall->take_designer_melody());
                if (designer.run() == Designer::Result::Quit) break;
                SDL_SetWindowRelativeMouseMode(window, false);
                continue;
            }
            if (hall->request() == Hallway::Request::Harness)
            {
                hall->clear_request();
                Harness harness(window, renderer, training_dir(), ai_shape_of(settings),
                                picture_source(settings, filters_path));
                const Harness::Result hr = harness.run();
                if (hr == Harness::Result::Quit) break;
                if (hr == Harness::Result::GoTo)
                {
                    settings.ai_layers = harness.go_shape.layers;
                    settings.ai_width = harness.go_shape.width;
                    settings.ai_heads = harness.go_shape.heads;
                    settings.ai_bits = harness.go_shape.bits;
                    settings.start_line = "ai";
                    pending_ai = harness.go_digits;
                    harness_went = true;
                    break;
                }
                SDL_SetWindowRelativeMouseMode(window, false);
                continue;
            }
            // Personal Dimensions from the pause menu: the main menu's Personal Dimensions screen over the
            // hallway; your world from there, and then the same hallway again, still paused. A favourite
            // opened in the world (O) is taken into this hallway's hand where it is of this line, else
            // the hallway is built again on its line, as from the main menu.
            if (hall->request() == Hallway::Request::Personal)
            {
                hall->clear_request();
                MainMenu mm(window, renderer, app, app_path, display);
                const MainMenu::Result pr = mm.run_personal();
                if (pr == MainMenu::Result::Quit) break;
                if (pr == MainMenu::Result::Personal)
                {
                    std::optional<hallway::Favourite> opened;
                    std::string sys;
                    const auto res = run_personal_world(opened, sys);
                    if (res == hallway::PersonalWorld::Result::Quit) break;
                    // (Settings may have been changed there: applied to this hallway as its own Settings are)
                    hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
                    hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
                    hall->settings_changed();
                    if (res == hallway::PersonalWorld::Result::System)
                    {
                        // Another system's row: chosen on this hallway's pause menu, as if chosen there.
                        bool q = false;
                        hall->pause_choose_named(sys, q);
                        if (q) requests_waiting = true; // (the hallway's loop is skipped once: its request is handled next)
                        SDL_SetWindowRelativeMouseMode(window, false);
                        continue;
                    }
                    if (res == hallway::PersonalWorld::Result::Open && opened)
                    {
                        if (opened->line == hall->line_id())
                        {
                            hall->resume(); // (the pause menu closed: the favourite in hand, in its viewer)
                            hall->open_favourite(*opened);
                        }
                        else
                        {
                            favourite_elsewhere = *opened; // (after this loop: a hallway on its line)
                            break;
                        }
                    }
                }
                SDL_SetWindowRelativeMouseMode(window, false);
                continue;
            }
            // Settings from the pause menu: the main menu's screens over the hallway, and then
            // the same hallway again, still paused, with what was changed applied to it.
            if (hall->request() != Hallway::Request::Settings) break;
            hall->clear_request();
            MainMenu mm(window, renderer, app, app_path, display);
            if (mm.run_settings() == MainMenu::Result::Quit) break;
            hall->set_controls(app.mouse_sensitivity, app.invert_mouse_y);
            hall->set_graphics(app.edge_glow, app.real_graphics, app.door_portals);
            hall->set_fps_counter(app.fps_counter);
            // (and the view distance and graphics memory, as Settings > Graphics now has them)
            set_view(app.view_rooms, app.picture_rooms);
            hall->set_graphics_memory(app.graphics_memory_gb);
            hall->settings_changed();
            SDL_SetWindowRelativeMouseMode(window, false);
        }
        if (favourite_elsewhere)
        {
            // A favourite of another line opened in your world from the pause menu: the hallway built
            // again on its line, as the settings stand, and it in your hand (as from the main menu).
            pending_favourite = *favourite_elsewhere;
            for (const auto& d : hallway::kDimensions)
                if (pending_favourite->line == d.id) settings.start_line = d.line;
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::GoToMelody && hall->melody_to_go())
        {
            // A melody of the music on an audio line of another length or note set (or a track of
            // another number of units): the hallway again with that audio line (and tracks), on
            // the audio or tracks line, and there the melody.
            const MusicMelody melody = *hall->melody_to_go();
            const sieve::NoteSet& set = melody.set;
            const uint32_t units = std::max<uint32_t>(1, melody.units);
            settings.notes = uint32_t(melody.notes.size() / std::max<uint32_t>(1, set.voices) / units);
            if (melody.units) settings.track_units = melody.units;
            settings.note_set = set.legacy ? "notes104" : "notes2";
            if (!set.legacy)
            {
                settings.note_low = sieve::note_name(set.low);
                settings.note_high = sieve::note_name(set.high);
                settings.note_durations = set.durations;
                settings.voices = set.voices;
            }
            settings.start_line = melody.units ? "tracks" : "audio";
            pending_melody = melody;
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::GoToPicture && hall->picture_to_go())
        {
            // A figure of the chat window on an image line of another size or palette: the hallway
            // again with that image line, on it, and there the figure.
            const Hallway::PictureToGo p = *hall->picture_to_go();
            settings.image_w = p.width, settings.image_h = p.height, settings.image_palette = p.palette;
            settings.start_line = "image";
            pending_picture = p.cells;
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::Tailored && hall->tailored_filters())
        {
            // Tailored filters (Return on COST, or the File Locator's "use"): kept, as the setup
            // menu keeps what it ticks, and a hallway built with them on the same line, with the
            // item from COST in hand again on its COST tab.
            filters = *hall->tailored_filters();
            try
            {
                filters.save(filters_path);
            }
            catch (const std::exception&)
            {
                // Read-only folder: the filters still apply to this session.
            }
            settings.start_line = hall->line_name();
            pending_unit = hall->tailored_unit();
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (harness_went)
        {
            show_menu = false;
            settings_chosen = true;
            continue;
        }
        if (hall->request() == Hallway::Request::MainMenu)
        {
            // Exit Sieve, then Y: back to the main menu, and from there the setup menu as ever.
            show_main = show_menu = true;
            continue;
        }
        if (!hall->menu_requested()) break;
        if (in_game_menu == Menu::Result::Quit) break;
        // ENTER THE HALLWAY from the F1 menu: a new hallway from its settings, without the menu again.
        show_menu = false;
        settings_chosen = true;
    }
    return finish();
}


} // namespace

int main(int argc, char** argv)
{
    hallway::attach_parent_console();
    try
    {
        // Reuse the sieve argument parser: it expects a command first.
        std::vector<char*> args(argv, argv + argc);
        std::string command = "hallway";
        args.insert(args.begin() + 1, command.data());
        const Args a = parse_args(int(args.size()), args.data());
        if (a.help)
        {
            std::cout << kUsage;
            return 0;
        }
        const int status = run(a);
        // A menu count still running at exit (a large stack in a Debug build can take minutes)
        // would use statics that are being destroyed, and waiting for it would hold the program
        // open: so leave at once. Everything is saved as it changes, and the disk cache is written
        // by renaming a finished file, so nothing is left half done.
        if (filter_workers_busy())
        {
            std::cout.flush();
            std::cerr.flush();
            std::_Exit(status);
        }
        finish_filter_warmup();
        return status;
    }
    catch (const std::exception& e)
    {
        std::cerr << "error: " << e.what() << "\n\n" << kUsage;
        std::cerr.flush();
        if (filter_workers_busy()) std::_Exit(1);
        finish_filter_warmup();
        return 1;
    }
}
