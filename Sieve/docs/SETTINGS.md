# Settings

Every setting in `sieve-hallway.ini`, its default and what it is. **Written from the code** by
`tools/settings_doc.py` (from `hallway --write-settings`, so from the settings' own table in
`client/app_settings.cpp`); CI step 10 fails if this file differs from what the code writes. Do not
edit it by hand: change the code, then run `python tools/settings_doc.py build/hallway`.

Most are also on the main menu's Settings and the setup menu; the hallway's options win over the
file for the run they are given in. An empty default is written empty.

## [graphics]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `resolution` | (empty) | the window's size, WxH; empty: chosen from the display at start |
| `fullscreen` | `off` | on: fullscreen at the resolution; off: a window |
| `vsync` | `on` | on: wait for the display; off: frames as fast as they can be drawn (may tear) |
| `edge_glow` | `off` | Geometry Edge Glow (only one of it and Real Graphics is on) |
| `real_graphics` | `off` | Real Graphics (only one of it and Geometry Edge Glow is on) |
| `door_portals` | `off` | Door Portals: procedural data noise in the doorways |
| `graphics_memory_gb` | `4` | the graphics card's memory, for the setup menu's budget, 1 to 128 |
| `view_rooms` | `7` | View Distance: rooms drawn and kept either side of yours, 2 to 64 |
| `picture_rooms` | `1` | Picture Distance: rooms either side of yours with item pictures of their own, 0 to 8 |
| `fps_counter` | `off` | frames per second, top right of the hallway |

## [world]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `model_cache_mb` | `64` | the display cache: memory for the pictures on items, 8 MB up |
| `angle_decimals` | `1` | decimal places on the compass's degree readout, 0 to 20 |
| `filter_memory_mb` | `512` | the most one count of the filters' survivors may take for its tables, 64 MB up |
| `counting_memory_pct` | `50` | percent of installed memory the setup menu's counts may take at once, 5 to 100 |
| `merge_cache_pct` | `50` | percent of the filter memory kept for merged filters between counts, 0 to 100 |
| `unit_time_ms` | `50` | the time budget: the longest one unit may take to open, 5 to 60000 ms |
| `item_memory_pct` | `25` | percent of installed memory the items around you may take, 5 to 90 |

## [controls]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `mouse_sensitivity` | `100` | percent, 10 to 400 |
| `invert_mouse_y` | `off` | on: moving the mouse up looks down |
| `read_aloud` | `off` | on: the item chosen in a menu is read aloud, in Sieve's own voice (or Windows' own without it) |
| `text_to_voice` | `off` | on: the AI's replies in the CHAT tab are spoken aloud, in Sieve's own voice (or Windows' own without it) |

## [models]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `text_model` | (empty) | a model folder CHAT talks with; empty: the model in hand |
| `picture_model` | (empty) | a picture model's run folder DRAW uses; empty: the first trained one |
| `widen_weights` | `off` | on: a model file's BF16 and F16 weights widened as they load (twice the memory); off: as stored; auto: widened where the widened model fits in ITEM MEMORY's share of installed memory; the same numbers either way |

## [locator]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `search_seconds` | `0` | the File Locator's search for shorter addresses: seconds, 0 for no limit |
| `search_for` | `file` | file: those seconds for each file; run: for the whole run, shared among its files |
| `optimise_archives` | `0` | 1: either half of Archive and compressed formats ticked (as Archive and Compressed Formats, the one box before v31, read it: both); the next two say which; 0: off |
| `compressed_temporary` | `0` | 1: Archive and compressed formats: temporary ticked (an archive or a compressed file made again exactly on installing, where Sieve can); 0: off |
| `compressed_permanent` | `0` | 1: Archive and compressed formats: permanent ticked (an archive or a compressed file carried as what it holds, installed so); 0: off |
| `sound` | `temporary` | Sound as FLAC (v34). temporary (the default): only what Sieve makes again byte for byte on installing (a WAV, AIFF, RF64, Wave64, .au or CAF as its samples coded by FLAC; an MP3 as packMP3 codes it), an archive's sound included; any other sound carried as its bytes. permanent: also installed as other bytes of the same samples where smaller (those, WavPack, Monkey's Audio, TTA and ALAC as name.flac, through ffmpeg for the last four; a FLAC re-coded). off: sound left to the ways every file has |
| `optimise_search` | `1` | 1: Optimise Search ticked (the default): Try every way chosen for each file by its kind, from what was measured (off for every file with Priority: Decoding speed; else on for a JPEG, a file over 16 MiB or under 4 KiB, off between), its own box unused; 0: Try every way as its box says |
| `search_every_wave` | `0` | 1: Try every way ticked: the search's shortcuts off (the rope through every wave, even on text and on bytes already packed; mixing tried on every file, whole); 0: off |
| `priority` | `balanced` | size: the smallest; balanced: mixing only where it saves 4% of what it carries (the default); speed: no file mixed (quickest to install, largest) |
| `compare` | `1` | 1: Compare with zip and 7z ticked (the default): what zip (deflate 9) and 7z (LZMA2 9e) make of it and of the instructions, beside Sieve's; 0: Sieve's figures alone, quicker on a large folder |
| `more_formats` | `0` | 1: More formats ticked: the comparison with gzip, bzip2, zstd, brotli and lz4 too, each the whole as one stream (slower: brotli above all); 0: zip and 7z only |
| `v35_draft` | `0` | 1: sieve-manifest-v35 in the making, for testing only (docs/NEXT-VERSION.md): what it makes is a draft, not to be published; 0: off, the newest frozen version made |
| `keep_log` | `0` | 1: Keep a log ticked: each reading and saving writes what it reported as it went (every best so far, with its time), how long it took and its phases' times to a text file in the logs folder beside these settings, and each install its decoding and writing times; 0: off |
| `search_share_packing` | `-1` | learned: the share of the search's time packing has used here, 0 to 1, for the time estimate; -1: not measured yet |
| `search_share_weighing` | `-1` | learned: the share of the search's time weighing has used here, 0 to 1, for the time estimate; -1: not measured yet |

## [character]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `line` | (empty) | your character (IDEAS 19.148): a seed-made one as the first line of its .character file; one holding models, or of given spheres (an imported skeleton), as its whole text with ';' between lines; empty: the default |
| `speaks` | (empty) | the languages your character speaks (IDEAS 19.183): ids of the language catalogue (data/languages), separated by commas; empty: none chosen |
| `actions` | (empty) | your character's actions (IDEAS 19.186): the SHA-256 of its map of each action (idle, walk, run, ...) to the animation that plays it, read from the actions folder beside this file; empty: none, and its one animation plays whatever it does |
| `animation` | (empty) | the animation your character plays (IDEAS 19.170): its SHA-256, read from the animations folder beside this file; empty: none |
| `drawing` | `3` | how the CHARACTER page draws it: 0 pixels, 1 wireframe, 2 voxels, 3 shaded |
| `beside_you` | `on` | on: your character is you in the hallway, not drawn, its size setting the hallway's scale and its actions played (the default); off: left out there |
| `max_hall_scale` | `16` | how many times the hallway may grow so that your character fits its doors, 1 to 64 (a larger character does not fit) |

## [raid]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `drives` | (empty) | the infinite RAID (IDEAS 19.176): folders, separated by commas (a spare drive, a second disk), where each character saved with Ctrl+S is also kept as a new version, every piece on two of them; empty: not kept there |
| `share` | `off` | OPT IN ONLY (Settings > RAID): on: each Ctrl+S also sends your save, sealed under your key so only you can read it, to the machines in peers; off (the default): it never leaves this machine |
| `copies` | `3` | with share on: how many of the machines keep each piece of your save, 1 to 16 (3 by default; with fewer machines, each of them); Check my shares puts back any piece kept on fewer |
| `peers` | (empty) | the machines your save is sent to with share on: HOST:PORT, separated by commas (each running sieve raid serve, or Sieve with serve on); empty: none |
| `key` | (empty) | your key's file (sieve-raid-key-v1); keep it safe and keep a copy, as without it what others keep for you cannot be read by anyone; empty: raid.key beside these settings, made when share is first turned on |
| `serve` | `off` | OPT IN ONLY (Settings > RAID): on: while Sieve runs, this machine keeps others' sealed saves for whoever connects to listen, up to the limits below (it cannot read them); off (the default): it keeps none |
| `listen` | `0.0.0.0:47100` | with serve on: the address answered on, HOST:PORT (0.0.0.0: every network this machine is on; 127.0.0.1: this machine alone) |
| `held` | (empty) | with serve on: the folder others' saves are kept in; empty: raid-held beside these settings |
| `space_mb` | `1024` | with serve on: the most kept for others in all, in MiB (1024 by default) |
| `per_peer_mb` | `64` | with serve on: the most kept for any one address, in MiB (64 by default) |

## [replays]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `max_minutes` | `30` | the longest a recording runs (F12 in the hallway starts and stops one), in minutes; at it, the recording stops and is kept (30 by default) |
| `min_free_mb` | `256` | the least free space, in MB, kept where recordings are written: checked every few seconds while one runs, and below it (or below twice the recording's size, if more) the recording stops and is kept (256 by default) |
| `cached` | (empty) | recordings kept as .sieve that are decoded once into the cache (beside the filters') rather than each time they are played, useful for long ones: their files' SHA-256s, separated by commas (the replay viewer's checkbox, one a recording); empty: none |

## [online]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `enabled` | `off` | off (the default): nothing in Sieve reaches another machine, whatever else is set (Sieve needs no network); on: the online systems may run where their own settings are on too (the infinite RAID's share and serve) |

## [personal]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `map` | `on` | your personal dimension's map, top right: the hill from above, you and what is placed (IDEAS 19.197); on by default |
| `compass` | `on` | your personal dimension's compass, along the top: which way you face, north being +z (IDEAS 19.197); on by default |
| `hallway_tools` | `off` | the hallway's own tools (Restart Sieve, the node graph, the navigator) in your dimension's pause menu too; off by default |

## [game]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `ai_explorers` | `off` | AI Explorers in the state space (IDEAS 19.189): off (the default) or on; not built yet, so nothing reads it until they are |

## [manual]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `keep_log` | `1` | 1: Manual Navigation's Keep a log ticked (the default): every step written to a CSV in the logs folder beside these settings as it lands, its head naming the file (its path, size and SHA-256), so Load Log can replay it; 0: off |
| `undo_depth` | `0` | Manual Navigation's undo depth: 0 for all the steps (the default), or 10, 100, 1000 |
| `fresh_random` | `1` | 1: FRESH RANDOM BYTES ticked (the default): with no file loaded, a new example of random bytes each time the width changes; 0: the example keeps its bytes, random ones added or the end cut |
| `width` | `2` | Manual Navigation's width: bytes a row, and the example's side (2 to 16; 2 by default, an example of 4 bytes) |

## [setup]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `line` | `text` | the line the hallway starts on |
| `mode` | `positional` | ordering: positional, scrambled or guided |
| `key` | `sieve` | the key the scrambled ordering's shuffle is made from |
| `items_per_wall` | `128` | items on one room's two walls, a power of two from 2 to 4096 |
| `title_length` | `32` | letters in every line's titles, but the books line's; 0: no titles |
| `item_letters` | `8` | the letter size items are drawn for, in pixels; smaller letters are dashes |
| `close_up` | `screen` | the widest close-up display: screen, 0 for none, or a power of two in pixels |
| `limits_focus` | `all` | the line FIND MY LIMITS grows (all: every line) |
| `length` | `32` | pages: characters a page |
| `alphabet` | `lower27` | pages: the alphabet (lower27, babel29, ascii95, or one sieve alphabets lists) |
| `canon` | `v2` | pages: the warp rules, v2 or v1 |
| `model` | `default` | pages: the guided ordering's model, default or none |
| `image_width` | `10` | image: pixels across |
| `image_height` | `10` | image: pixels down |
| `image_palette` | `mono` | image: the palette, mono, ega16, rgb332 or rgb24 |
| `image_fit` | `levels` | image: how a picture from a file is fitted, levels (canon-image-v2) or average (v1) |
| `note_set` | `notes104` | audio: the set, notes104, notes2, notes3 or pcm |
| `notes` | `16` | audio: notes a unit (each voice's, on notes2 and notes3) |
| `note_low` | `C3` | audio, notes2: the lowest note |
| `note_high` | `C6` | audio, notes2: the highest note |
| `note_durations` | `seEqQhHw` | audio, notes2: the lengths, from s e E q Q h H w |
| `voices` | `1` | audio, notes2: voices |
| `notes3_low` | `C-1` | audio, notes3: the lowest note |
| `notes3_high` | `G9` | audio, notes3: the highest note |
| `notes3_tpq` | `4` | audio, notes3: ticks a quarter note |
| `notes3_longest` | `16` | audio, notes3: the longest length, in ticks |
| `notes3_levels` | `8` | audio, notes3: loudness levels, 1 to 127 |
| `notes3_voices` | `1` | audio, notes3: voices |
| `notes3_tempo` | `120` | audio, notes3: quarter notes a minute, 1 to 1000 |
| `notes3_instruments` | `0` | audio, notes3: a General MIDI program (0 to 127) a voice, comma-separated |
| `samples` | `8000` | audio, pcm: samples a channel in a unit |
| `pcm_rate` | `8000` | audio, pcm: samples a second |
| `pcm_bits` | `8` | audio, pcm: bits a sample |
| `pcm_channels` | `1` | audio, pcm: channels |
| `video_width` | `5` | video: pixels across |
| `video_height` | `5` | video: pixels down |
| `video_frames` | `8` | video: frames a unit |
| `video_palette` | `mono` | video: the palette, mono, ega16, rgb332 or rgb24 |
| `video_fit` | `levels` | video: how a video from a file is fitted, levels or average |
| `video_fps` | `8` | video: frames a second, for a video saved as GIF, MP4 or WebM |
| `book_pages` | `4` | books: pages after the cover and the title page |
| `track_units` | `4` | tracks: units of audio after the cover and the title |
| `movie_units` | `4` | movies: units of video after the cover and the title |
| `vertices` | `8` | models: vertices |
| `faces` | `12` | models: triangles |
| `coords` | `16` | models: the steps across [-1, 1] a coordinate takes |
| `model_tile` | `64` | models: the picture on a crate's face, pixels square, a power of two |
| `world_models` | `4` | worlds: models after the cover and the title |
| `world_grid` | `8` | worlds: cells along each side of the grid, up to 1024 |
| `ai_layers` | `1` | AI: layers, 1 to 64 |
| `ai_width` | `16` | AI: the width, 2 to 4096, which the heads divide into even parts |
| `ai_heads` | `2` | AI: attention heads |
| `ai_bits` | `4` | AI: bits a weight, 1 to 8 |
| `binary_length` | `32` | binary: every file of 0 to this many bytes |

## [paths]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `ffmpeg` | (empty) | the ffmpeg program; empty: SIEVE_FFMPEG, then one beside Sieve, then the PATH |

## [language]

| Setting | Default | What it is |
| :--- | :--- | :--- |
| `language` | `en` | the language file's name in data/lang (en: English) |
