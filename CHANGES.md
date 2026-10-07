# Records for tracks and movies

Against `d0d0e03` with `fixes.zip` applied first: apply `fixes.patch`, then `records.patch`. The
files in `files/` are the final versions, with both applied.

## What changed

### The record
Tracks and movies keep records of the same kind books do (`sieve-book-v1`):
- **Three sections:** `cover`, `title` and `units`. A missing part is blank, and a blank cover or
  title is left out, so one item has one record and one id however it was made.
- **Which line:** the sections say where a record belongs. `pages` means books; `units` of audio
  means tracks; `units` of video means movies.

### `sieve-book-v2`, for other note sets
Until now a record's audio section could only describe `notes104`. Version 2 adds one field after
an audio section's line, `notes <symbols id>`, which names a `notes2`, `notes3` or `pcm` set
whole. Its `length` is then per voice or channel.
- **Written only when needed:** a record without the field is still v1, readable by every earlier
  Sieve.
- **The id is unchanged:** it is computed as before.
- **Also closes** the IDEAS item "books of notes2 melodies".

### In the hallway
- **F** on a track or movie offers **Sieve record** (`.track`, `.movie`).
- **J** on a record standing on the binary line opens it onto its own shelf (books, tracks or
  movies, from its sections).
- **A record that doesn't fit** the hallway's settings is refused with what it needs, e.g. "its
  units are notes2/C3-C6/seEqQhHw/V2 (8 symbols a unit), not notes104 (16 symbols a unit) (set
  IMAGE, the title length and TRACKS in the menu to the record's shape)".

### In the `sieve` tool
- **`sieve bind --line tracks|movies --file F [--title T] [--cover P] --out R`** makes a record
  from a melody, sound or video. It also prints the item's address.
- **`sieve read --line tracks|movies ... ADDRESS --record R`** writes the item at an address as a
  record. It is byte for byte the record `bind` makes for the same item.
- **`sieve unbind R --units F`** saves the units joined, as one melody, sound or video.

## Checked
- **In the hallway:** a track and a movie saved as records, then opened with J from the binary line
  and saved again, give byte-identical files. The same holds for a two-voice notes2 track (a v2
  record).
- **In the tool:** a v2 track bound from a melody, read back by its address, and unbound to MIDI.
- **CI:** these checks are now in the books and hallway steps, and every step was run here.

## Not done
The `sieve` tool reads MIDI files only on notes3 lines, so `bind --file tune.mid` on a notes2
track is refused. This was already the case before this change. The hallway's J reads MIDI on
every note set. It is noted in IDEAS §13.

## Docs
- **SPECIFICATIONS §11:** v2 records, and records for tracks and movies.
- **README:** a Records bullet under Tracks and movies.
- **`sieve help`:** `bind`, `read` and `unbind`.
- **IDEAS:** §13's records step done, and §12's notes2 books done.
- **HANDOFF:** a new entry.
