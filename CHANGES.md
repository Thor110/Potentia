# The filter survey, and open-ended notes (notes3) (relative to origin/main f8ba3d0)

To apply it, do one of these from the Potentia repository root:
- unzip `files/` over the repository;
- or run `git apply survey-notes3.patch`.

**New files:**
- the survey: `Sieve/tools/survey/survey.py`, `grid.tsv` and `profiles.tsv`;
- the first results: `Sieve/results/survey-average-2026-10-05.tsv` and `.md`;
- notes3: `Sieve/core/include/sieve/notes3.hpp` and `Sieve/core/src/notes3.cpp`;
- notes3's vectors: `Sieve/tests/vectors_notes3_v1.tsv`.

## 1. The filter survey
`tools/survey/survey.py` counts what each filter keeps on its own, and what stacks of them keep together, over a grid of settings on every line.
- **The engine's own figures:** it runs the `sieve` tool itself.
- **Each row records:**
  - the share kept, as a power of ten;
  - the exact percentage removed, with as many decimals as it takes to get past the leading 9s or 0s;
  - whether the count is exact or the stack only judges (with the reason);
  - the time and peak memory it took.

**The grid** (`tools/survey/grid.tsv`) is data:
- **Settings per line:** lists `a,b,c`; ranges `lo..hi`, `lo..hi:step` or `lo..hi*k`; and `height==width` for paired settings, so 5×5 to 10×10 is one row.
- **Stacks to count:**
  - `each` filter on its own;
  - `all` of them together;
  - named filters together (`a-v1+b-v1`);
  - a filter at stepped settings (`row-runs-v1[changes=0..4]`).

**Hardware profiles** (`tools/survey/profiles.tsv`): low (8 GB, 2 cores), average (16 GB, 4 cores) and high (64 GB, 8 cores).
- **What each sets:**
  - the filter memory one count may take (the setup menu's FILTER MEMORY);
  - a time limit for each count;
  - how many counts run at once, no more than three quarters of the RAM allows.
- **Your own machine:** `--ram-gb`, `--cores`, `--filter-memory` and `--time-limit` override any of them.
- **Past the bounds:** a count is recorded as "over time" or "judge only", with what it would have needed.

**Output:** `results/survey-<profile>-<date>.tsv` (every row) and `.md` (a table per line).

**First run included:** the average profile on this machine (4 cores, 15 GB), as `results/survey-average-2026-10-05.*`.
- **Result:** 507 counts; 426 exact and 81 judge-only, none with an error and none over time.
- **Time:** 130 s of counting in all, four at once.

## 2. Open-ended notes: `notes3`
A third note family on the audio line, beside `notes104` and `notes2`, with nothing capped but MIDI:

| Part | Range |
| :--- | :--- |
| Pitches | any MIDI pitches, C-1 to G9 (at least an octave) |
| Lengths | whole ticks: 1 to 960 ticks a quarter, longest 1 to 65,535 |
| Loudness | 1 to 127 levels; level k plays at velocity round(127k / levels) |
| Voices | 1 to 15, each on its own MIDI channel (the drums' channel 10 left out) |
| Tempo, instruments | a tempo, and an instrument a voice: how it plays and saves, not its addresses |

- **The id** names everything: `notes3/C-1..G9/q4/d16/v8/V1/t120/i0`.
- **Size:** at full range, 8 levels and lengths up to 16 ticks, each event is one of 16,400 symbols.
- **Notation:**
  - `C#4:3!5` is C#4 for 3 ticks at level 5;
  - `R:2` is a rest of 2 ticks;
  - notes2's codes (`E4q`, `Bb3e.`) work too;
  - ` // ` goes between voices.
- **Fitting (`canon-notes-v3`):**
  - flats become sharps;
  - pitches move by octaves into range;
  - a length beyond the longest becomes the event then rests;
  - a level above the most is clamped;
  - a missing level is the one nearest velocity 96, what the other sets play at.

  Every change is reported.
- **MIDI:**
  - **Saving:** a unit saves as MIDI with its tempo, instruments and loudness.
  - **Reading:** a MIDI file warps onto the line, and J opens one there. It comes back as the same music. A run of rests may come back as fewer events, because MIDI has no rest events (the same as notes2).
- **Hallway:**
  - **Setup menu:** AUDIO SET steps notes104 → notes2 → notes3 → pcm. On notes3 the six existing audio rows become notes, set, lowest note, highest note, lengths (ticks a quarter and the longest, from presets) and voices, so no rows were added.
  - **Settings file only:** levels, tempo and instruments (`notes3-levels`, `notes3-tempo`, `notes3-instruments`).
  - **P** plays it as square tones at its levels and tempo. The instruments are what the MIDI file asks for; the synth plays every voice the same way.
- **Filters:** each voice is judged on its own, and the general filters apply.
  - **The melody plugins don't apply yet:** they're written for notes2's digits.
  - **A fix along the way:** plugins written for `symbols notes*` were matched by prefix, so the survey found them offered on notes3 lines, where they failed. `notes*` now means notes104 and notes2 only, and FILTER-PLUGINS.md says so.
- **The oracle has its own `canon-notes-v3`:** MIDI writing in exact fractions, 8 cases covering:
  - every part of the notation;
  - code lengths that round;
  - splitting;
  - clamping;
  - octave moves;
  - all 15 voices;
  - the extremes (960 ticks, 127 levels, 1000 a minute).

  The engine matched all 8 on its first run.

## Also
`docs/IDEAS.md` §13 records the composition plan:
- **The pairs:** pages → books, audio → tracks, clips → video.
- **The far-off ideas:** a tile dimension for images, and a world space for models.
- **The problems to solve first:** address stability on renaming, the crowded setup menu, and the hallway's fixed count of seven dimensions.
- **The suggested order.**

## Checked
- **Unit tests:** 33,918 checks, 0 failures.
- **CI steps run here, all passing:**
  - same addresses, filters, models, the bytes256 line, books;
  - the sound and media steps, which now also warp and read back a notes3 MIDI file;
  - the whole hallway step;
  - the whole reference-oracle step, which now also diffs the notes3 vectors.
- **The hallway:** the setup menu on notes3, and a two-voice melody in hand with P playing.
- **By hand:** a notes3 MIDI file saved and warped back to the same address.
