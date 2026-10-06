# Joined filters, tracks and movies in the `sieve` tool, and notes3's melody filters

Against `2fd86ce` (code review).

## What changed

### A held model no longer redraws the crates
Each crate's picture was drawn at the held model's current turn, so any face painted while you
turned a model came out at that angle. Faces are now always drawn at the resting turn
(`Hallway::kModelSpin` / `kModelTilt`, which **R** also returns to). Turning a model in hand
redraws nothing else.

### Joined filters for tracks and movies (seam filtering)
- **What it is:** a fourth stack, **JOINED**, beside cover, title and units. It judges a track's or
  movie's units joined into one unit of their line, N units long, in the same way that a book's
  pages are read as one text. Every filter of the line can then judge across the seams: a melody
  running from one unit into the next, or a picture moving on from frame to frame.
- **Counting:**
  - When the units have no filters of their own, the joined unit is the part that is counted:
    cover × title × joined survivors. Compact works in both orderings. Survivors follow the joined
    unit's order. That is the positional order when a unit is one strand (video, or one voice);
    with more strands, it goes strand by strand.
  - With both the units' and the joined filters ticked, items are judged but not counted, so
    compact falls back to hide and says why.
- **Addresses:** the compact domain gains `/<joined id>` only when the joined stack has filters,
  so no existing address changes.
- **Settings:** saved as `[tracks.joined]` and `[movies.joined]`.
- **Setup menu:** the filters window has a JOINED part, with each filter's share worked out on the
  joined line. The line's survivor count takes it into account.
- **Z, C and X leave the joined part alone.** If they ticked it along with the units' filters,
  nothing would count, so it is ticked by hand.

### `sieve` takes `--line tracks` and `--line movies`
- **Commands:** `info`, `warp`, `read` and `filters`.
- **Options:** the hallway's own names: `--track-units`, `--movie-units`, `--title-length`,
  `--image-width`, `--image-height` and `--image-palette`. So the same options give the same
  addresses in both.
- **warp** reads its input as the units joined and gives it a blank cover and title, as T does in
  the hallway.
- **read** prints the title, the cover and the units joined. `--out` saves the units joined;
  `--cover-out` saves the cover.
- **`--compact`** works through the composition filters, the joined stack included.
- **Shared code:** `strands_of()` and `joined_line()` moved from the hallway into `cli/lines`, so
  the tool and the hallway build compositions with the same code.

### Melody filters for notes3
- **A family of its own:** notes3's symbols carry lengths in ticks and loudness levels, so it
  cannot share the `notes*` plugins. Plugins for it declare `symbols notes3*`.
- **What its plugins can use:**
  - the constants `PITCHES`, `LOW`, `LEVELS`, `LONGEST` and `TPQ`;
  - two functions that name its symbols: `NOTE(p, level, ticks)` and `REST(ticks)`.

  These are added to both the engine and the reference oracle.
- **Ten plugins:**
  - `key-notes3-v1`;
  - `melody-leap-`, `-range-`, `-ambitus-`, `-rests-`, `-gapfill-`, `-metre-`, `-ending-` and
    `-lengths-notes3-v1`, each judging as its notes2 namesake does. Metre is counted in ticks and
    adds 6/8. Ending and lengths take notes2's codes, a quarter being TPQ ticks.
  - `melody-loudness-notes3-v1`, which only notes3 can have: notes between the softest and loudest
    levels, and no jump of more than `step` levels between neighbours.
- **Speed:** the notes of one pitch, at every level and length, are one run of symbols. The
  plugins that judge pitch alone therefore name each pitch as a single range. On the default set
  (128 pitches, 8 levels, 16 lengths), ambitus builds in 2.4 s rather than 16, and gap-fill in 1 s
  rather than 9. The rest take under 0.2 s.

## Checked
- **Build:** no warnings (Linux, GCC 13), for the client and the tools.
- **Unit tests:** 46,872 checks, 0 failures. The joined stack is checked against brute force with
  one and two strands. The tests also cover both unit stacks at once, and confirm that an empty
  joined stack leaves the domain unchanged.
- **Tool and hallway agree:** I saved the same track by its address in both and compared the MIDI
  files byte for byte. This is checked plainly in both orderings, and compactly through a joined
  `key-v1`.
- **Oracle and engine agree** on every notes3 plugin: 13 settings, two notes3 sets, one and two
  voices, three lengths each.
- **Compact round trip:** a stack of notes3 plugins on a two-voice line round-trips through compact
  in both orderings, and a melody with a fifth in its second voice is rejected by the leap filter.
- **Every CI step was run here:** same addresses, filters, models, bytes256, sound, other formats,
  books, the whole hallway step and the reference oracle. New checks were added to each step that
  these changes touch.
- **Screenshot:** the tracks filters window with its JOINED part (`filters-joined.png`).

## Docs
- **SPECIFICATIONS:** §11 covers the joined stack, its counting and its domain. §3.4 covers
  notes3's plugin family.
- **FILTER-PLUGINS §14:** the `notes3*` family, `NOTE()` and `REST()`, and the plugins.
- **README:**
  - tracks and movies in the `sieve` tool;
  - the joined filters, and the Z, C and X exception;
  - notes3's melody filters;
  - the crate faces' fixed angle.
- **`sieve help`:** `--line tracks|movies` and their options, `--cover-out`, and a TRACKS AND
  MOVIES section under `sieve help lines`.
- **IDEAS §13:** seam filters and the `sieve` tool done. Filters made only for the joins are left
  open.
- **HANDOFF:** an entry, and the file-map row for `composition.cpp`.

## Still open
- **Filters made only for the joins** (a note held across a boundary, a cut between frames). These
  would judge less than the joined stack, and count more easily.
- **A record format for tracks and movies,** and J opening one onto its shelf.
- **1e:** wire optimise-all into the COST pages. **1f:** cross-line filters.
- **Later:** the colour-blindness pass.
