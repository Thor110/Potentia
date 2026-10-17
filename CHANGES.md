# Changes: `sieve-hallway.ini` holds every setting (17 October 2026)

The next task from the handoff. Copy `files/` over the repository root.

- **What was checked first:** the setup menu's settings were saved nowhere (`sieve-filters.ini`
  holds only filters), so they now live in `sieve-hallway.ini`, their one home.
- **`[setup]`:** all 54 of the setup menu's settings (every line's shape and the GLOBAL ones),
  under the hallway's option names with `_` for `-`: `items_per_wall`, `title_length`,
  `binary_length`, `ai_layers`, `video_fps` and the rest. Saved when the setup menu changes them,
  so the next start begins where the last one left off. Options on the command line still win.
- **`[paths]`:** `ffmpeg = C:/ffmpeg/bin/ffmpeg.exe` now works (after `--ffmpeg`, before
  `SIEVE_FFMPEG`).
- **Every entry is written** with its value and a one-line comment, an empty one written empty.
- **An older file is rewritten at start** when it lacks an entry, keeping its own values.
- **A bad hand edit** (an unknown alphabet, a word where a number goes) leaves that setting as it
  was instead of stopping the hallway.
- **`hallway --write-settings OUT`** writes the file without a window. A new CI block checks
  that the defaults, written, loaded and written again, are the same byte for byte; that every
  entry has a comment; that an older file keeps its values and gains new entries; and that the
  setup menu saves to `[setup]`. One older CI line (`be.ini`, which failed on the word "binary")
  now looks only for a binary-edge setting.
- **Files:** `client/setup.hpp` (new: the `Settings` struct, moved out of `menu.hpp`),
  `client/app_settings.{hpp,cpp}`, `client/menu.{hpp,cpp}`, `client/app_main.cpp`,
  `.github/workflows/build.yml`, `README.md`, `docs/SPECIFICATIONS.md` §5.3.1, `docs/IDEAS.md`
  §19.79, `docs/HANDOFF.md` (a new entry).
- **Checked here:** every changed client file compiles with no warnings. A scratch program built
  on the real settings code passed all ten of its checks, the byte-for-byte round trip among them.
- **Not run here:** the full hallway build, the unit suite and the new CI lines. Worth trying
  Debug and Release, then starting once with your existing `sieve-hallway.ini`: it should gain
  `[setup]` and `[paths]` and keep everything else.
