# Tracks and movies, and the doors in their new order

Against `fa96f24` (dimensions menu).

## What changed

### The doors
The corridor now runs, as decided:

    binary | image  pages  books  audio  tracks  video  movies  models | binary

The setup menu's settings rows follow the same order. Their row numbers are now computed from the
doors, so a later reorder needs no other change.

### Tracks and movies
- **What they are:** a track is a cover (an image unit), a title (the titled lines' title) and N
  units of audio. A movie is the same with N units of video. N is "units per track" / "units per
  movie" in the setup menu (4 by default; `--track-units`, `--movie-units`).
- **Colours:** audio's green and video's red, each with black edges, as books have.
- **Real Graphics:** they use audio's and video's models.
- **Addresses: `composition-v1`** (new, SPECIFICATIONS §11):
  - one mixed-radix number, cover first, then title, then each unit; neighbours differ in the last
    unit;
  - scrambled with shuffle-sha256-v1, keyed, the id as the domain.
  - Books keep bookspace-v1, so **no existing address changes**.
- **In the hallway** an item is one long unit of its line, its units joined. Joining goes voice by
  voice (or channel by channel), so a track of two-voice units is still two voices:
  - the panel shows the notes or frames;
  - P plays a track, and a movie shows all its frames;
  - F saves (a track as MIDI or WAV, a movie as frames, or through ffmpeg);
  - T warps in, splitting into units with a blank cover and title;
  - X goes to an address, and M changes ordering keeping the item's cover and title.
- **Filters:** three stacks each, as books have: cover, title and units, with each unit judged on
  its own. Counts are exact (cover × title × units^N), and compact works in both orderings
  (`composition-compact-v1`). They're saved as `[tracks]`, `[tracks.cover]`, `[tracks.title]`,
  `[tracks.units]`, and the same for movies.
- **FIND MY LIMITS** treats audio and tracks (and video and movies) as it treats pages and books:
  the line grows only while a one-unit track still fits, then the track takes the units left.

### Found on the way
Door numbers written as literals that only worked in the old order: `find_limits()` (0, 1, 2, 3,
4, 5, 6) and the books' count (`resolve(4, ...)`). They're now named doors.

### CI
- **Door order:** the checks that walk through doors or pin the menu's rows now follow the new
  order. Pages' left door leads to BOOKS, and the corridor end to end is seven doors from pages.
- **New checks:** tracks and movies in both orderings; a warp onto tracks; saving a track (MIDI)
  and a movie (PNG); compact movies in both orderings; the composition vectors diffed against the
  oracle.

### Docs
- **SPECIFICATIONS:** §11 composition-v1; the door order in §12.1.
- **README:** a tracks and movies section, and the new order.
- **IDEAS §13:** steps 2 to 4 done, with what is still open.
- **HANDOFF:** an entry and file-map row.

## Not done (in IDEAS §13)
- **Seam filters:** filters that judge the joins between units.
- **The `sieve` tool:** it has no `--line tracks` / `--line movies` yet.
- **Records:** a record format for tracks and movies, and J opening one onto its shelf.

## Checked
- **Build:** no warnings (Linux, GCC 13).
- **Unit tests:** 36,410 checks, 0 failures. That includes the 48 composition vectors, and the
  composition sieve checked against brute force with and without filters on each part.
- **Every CI step passes,** run here: same addresses, filters, models, bytes256, sound, other
  formats, books, the whole hallway step (with the new checks) and the reference oracle (with the
  new vectors).
- **Screenshots:** `setup.png`, `hall-tracks.png`, `hall-movies.png`, `hand-tracks.png`,
  `hand-movies.png`.
