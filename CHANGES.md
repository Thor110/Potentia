# The catalogue of formats, stage 4: figures measured as formats are used (12 November 2026)

On top of f6f56c9 ("sieve raid system"). Unzip at the repository root; nothing to delete by hand.
**Re-run CMake**: one new source file, core/src/format_stats.cpp.

## What it does

- `sieve catalogue --measure FILE|FOLDER [--machine NAME]` packs each file alone as Sieve does, against
  LZMA2 (7z's method) and deflate, with the time to make and to install, installs it back and checks
  it, and adds the figures to its kind's entry (file-png, file-txt, ...). Exit 1 if a round trip differs;
  a withheld or unreadable file is skipped and said so.
- Every `sieve import` adds what it kept (vertices, faces, joints, animations, things left out) to its
  importer's entry (model-obj, model-gltf, model-glb). `--no-stats` keeps nothing.
- `sieve catalogue ID` shows the figures added up beneath the lineage; `--measured` lists every entry
  measured; the SYSTEMS page shows them in short beneath the entry.
- Kept in your stats folder, never in the repository: `--stats-folder DIR`, else SIEVE_STATS, else
  %LOCALAPPDATA%\Sieve\stats (Windows) or ~/.local/share/sieve/stats. Each record is one line of
  `sieve-format-stat-v1` (SPECIFICATIONS 12.0p) and names no file: only the kind, sizes, times, date
  and the machine's name (--machine, or SIEVE_MACHINE; "unnamed" otherwise).

## Files

- New: core/include/sieve/format_stats.hpp, core/src/format_stats.cpp.
- Changed: CMakeLists.txt, .gitignore (/sieve-stats/, /measure.txt), core/include/sieve/catalogue.hpp
  and core/src/catalogue.cpp (format_id_of_file_kind; the record's own entry, 194 in all),
  tools/sieve_cli.cpp, tools/cli/args.hpp, tools/cli/help.cpp, client/menu.cpp and menu.hpp, client/node_graph.cpp, tools/make_release.py,
  tests/test_core.cpp (test_format_stats), ci/lib.sh (SIEVE_STATS kept in the step's folder),
  ci/steps/04-models.sh, ci/steps/10-hallway.sh (document now has 6 doors); docs: SPECIFICATIONS 12.0p, CATALOGUE-ROADMAP, IDEAS 19.165, MANUAL, ROADMAP.

## Also: releases now carry catalogue/ and languages/

tools/make_release.py and the node graph's "This installation" list now include the two folders (a
release built before this had neither: the LANGUAGE page would be empty), and `sieve` in a release's
tools/ finds them in the folder above. **In your build folder, delete
`catalogue/language-english.format`** if it is there: the build copies new data files but never removes
old ones, and that stale copy makes the catalogue refuse to load (an id twice).

## Ideas noted

IDEAS 19.185: every document and format type built on Sieve's principles, scaling with its state
space; and .sfilter (and catalogue, map) files kept on disk as .sieve and read on the spot. Planned only.

## Not yet

The hallway's own packing (File Locator) and its imports adding their figures; figures gathered from
many machines (the RAID could carry them); translations between formats beyond a round trip.
