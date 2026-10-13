# The characters model, every release on the record, and Specs vs Stats (13 October 2026)

This zip also carries the last round's files (Build or Buy, `log_metrics.py`, `run_tests.bat`,
the voices' odds): GitHub's main branch did not have them yet when this round began.

## The characters model: `data/training/type-2x32` (new)
- **What it is:** the fourth entry in the model list, after figures and melodies, with the text
  run beside them. It is called `type-2x32` because the list is alphabetical. A name after
  `melodies` keeps `figures-2x32` the default, and the menu's CI steps stay as they were.
- **Its pictures:** the 94 printable ASCII characters (the space left out), drawn from Sieve's own
  8 × 8 font (font8x8, public domain) at each of the 9 places a glyph fits in 10 × 10: 846
  pictures. They are written by `tools/build_character_pictures.py` (new).
- **Its training:** as `figures-2x32` was trained: 2 layers of width 32, 15,000 steps, 6 min 52 s
  here.
- **CI:** checks its first six pictures by address, and that the font gives its pictures again
  byte for byte.
- **Honestly, it does not draw characters yet.** I measured four runs, 200 pictures each. None
  makes a recognisable character: within 3 cells of a character that has at least 8 inked cells.
  - The runs: this one, twice as wide (2 × 64), centred characters only, and the ten digits only.
  - Its median picture is 16 cells from the nearest character. The figures model, never shown a
    character, scores 20.
  - It learns where characters sit (its bottom rows stay blank).
  - IDEAS §19.73 has the table, a guess at why (each level picks every cell's most likely colour
    at once), and what to try next in order.
  - It ships as a run to bake and build on, as you asked, and the README says plainly what it
    does and doesn't draw yet.

## Every release on the record
- **`make_release` step 8c (new):** runs the build's unit suite and times it. A failure stops
  the release. Then `log_metrics.py` writes a `release` record with `unit_seconds` and the
  hallway's metrics list. Commit `Sieve/metrics` with each release. `--no-record` skips the step.
- **`log_metrics.py` records the machine,** read from the system: the processor's name, logical
  processors, memory, and on Windows the graphics adapters. It never records the machine's name.
  Records also take `--set KEY=VALUE`.
- **Your machine:** `Sieve/metrics/machines/edward-1.txt`, in your own words, listed in
  `list.txt`. A record whose processor matches is tagged `edward-1`. Your next computer becomes
  `edward-2`, so each machine stays its own series.

## Specs vs Stats: `Pages/specs.html` (new)
- **Where:** linked only from Build or Buy's builds section, so it is semi-hidden. The AI
  disclosure is at the top, as on every page.
- **The machines:** each machine's full specification, with its builds.
- **The leaderboard:** ranked by unit checks a second, comparing like with like (the same
  version).
- **All Of Them At Once:** the fastest rate on the record, times however many machines you type
  in.
- **The limits of physics:** at the least energy a computer can spend per bit (Landauer's
  kT ln 2), with the sources named:
  - all the sunlight that reaches Earth, spent at room temperature: 6.0 × 10^37 bits a second.
    That covers the 10 × 10 pictures in 2 × 10^-8 seconds and the pages in about 3 years;
  - **a Dyson sphere, in a category of its own:** the Sun's whole output at the temperature of
    deep space: 1.5 × 10^49 bits a second. Even that takes 3 × 10^9807 years for the AI line's
    models.
- **Add Your Own:** how anyone adds a machine and their records, by pull request.

## Voices
- **Yours:**
  - "We can make it all as free-flowing as possible, as a program and as a state space." (a point
    about everything)
  - "If we all use the commands and filters, won't we all have an angle?" (a point about
    everything)
  - "Does that mean we can fold all video games into one?"
- **The marker:** "a point about everything" is now a marker in VOICES.md's format, explained
  there.
- **Mine:** an answer to the video games question. Counting-wise they are folded already: every
  game is a file on the binary line. A rule that plays any of them is an emulator, and the hallway
  is the start of one.

## Notes (IDEAS)
- §19.71: every release measured; Specs vs Stats.
- §19.72: a library of small baked models, the corpus of a multimodal system, and what to bake
  next.
- §19.73: the characters model and its measurements.
- §19.74: accessibility.
  - The voice in the game, with subtitles; Windows has a speech engine built in.
  - Menus and buttons read aloud.
  - Objects translated through the same language files.
  - Text size, contrast and colour-blind palettes.
  - Your quick-time-event grid idea, noted as not wanted.
- §19.75: points about everything.
- HANDOFF has an entry for this round.

## For you to run
- **`tools\make_release.bat`:** it now ends with "== the build's record (metrics/)". The record
  should name `machine_profile = edward-1`. If it doesn't, the processor name Windows gives
  differs from the file's: tell me what the record's `cpu` line says.
- **The website:** Specs vs Stats fills in once `Sieve/metrics` is on GitHub.
- **The game:** the setup menu's picture model row steps through figures, melodies and type.

## Checked here
- The characters model's CI lines.
- `log_metrics --dry-run`.
- Specs vs Stats in Chromium at phone width, with a sample record: no errors, no sideways scroll.
- **Not run:** `make_release` itself (it needs Windows and 7-Zip).
