# The metrics list

Against your main as it is now (all ten files checked against it, one by one).
`metrics-example.txt` is the list as made here, for you; it isn't for the repository.

## `hallway --metrics FILE [--filters PATH]`
- **What it writes,** with no window and no display (it runs before SDL starts): every line's
  share removed, in your format.
  - Binary comes first, then the lines in door order.
  - The share is the percentage the setup menu shows, followed by "( Kept : 10^x )".
  - A short section after it gives each line's size.
- **It counts through the setup menu's own tallies,** on the menu's own workers, so the list and
  the menu can't differ.
- **It takes about two seconds** for all eleven lines.
- **The lines' settings** come from the hallway's own options, or their defaults without them.
- **The header** names the settings file and its SHA-256, the filter memory, and the version.
- **The menu's books, tracks and movies tallies now record their kept share** as the others do, so
  those lines show "Kept" too.

## The reference file: `data/metrics/reference-filters.ini` (new)
- **It ticks, on every line, the hard filters that count together:** they set aside only noise,
  with no dictionary, model or taste. So a release says what Sieve removes before any choice is
  made, and releases can be compared.
- **Measured here:**
  - binary 10^-4.34, models 10^-21.47, worlds 10^-85.90, AI 10^-145.65;
  - pages, pictures and sound barely move under hard filters alone (10^-0.00062 for pages);
  - the full list is in `metrics-example.txt`.
- **Your own list:** `hallway --metrics my.txt --filters sieve-filters.ini`. If you'd rather the
  reference held your soft filters too, send me your `sieve-filters.ini`, or replace the file's
  contents with yours. Its hash in the list records the change.

## The release (`tools/make_release.py`)
- **New step 8b:** the build's hallway writes `metrics.txt` beside the installer. It uses the
  build's hallway because the staged folder is already packed and checked, and is left as it is.
- **`metrics.txt` is published:** it's listed in `SHA256SUMS.txt`, and `RELEASE-NOTES.md` carries
  the list.
- **"Publish these four".**

## Checked here (Linux)
- The client builds without warnings.
- The list is right for every line.
- `make_release.py` parses (I can't run a release here: it needs your Windows build and 7-Zip).
- The new CI lines pass: the binary and models lines' figures, and eleven lines in all.

## Docs
- IDEAS §19.12 (built) and §19.13 (the chat window's design, from designs.zip).
- README ("How much the filters remove, line by line").
- SIEVE-INSTALL-USAGE (the release).
- HANDOFF.
- `hallway --help`.

## Next
- **The chat window** (IDEAS §19.13), as its own round.
- **Later:** moving the tally into the shared library, for a `sieve metrics` in the tool and a unit
  test of the list.

## New files
- `Sieve/data/metrics/reference-filters.ini`
