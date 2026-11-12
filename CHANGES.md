# Filters kept as .sieve: saved packed, decoded at start-up, cached (12 November 2026)

On top of 9d300ae ("sieve catalog"). Unzip at the repository root; nothing to delete by hand. No new
source files, so CMake need not be re-run.

## What it does (IDEAS 19.185; docs/FILTER-PLUGINS.md section 21)

- **The Filter Designer saves each filter as `<id>-vN.sieve`** in the filters folder: the .sfilter and
  the word and tagged lists it reads, packed together as Sieve packs a folder, read back and every
  member checked before it is written. (Its loose lists are still written beside it, for other
  filters.) A loose .sfilter or a .sieve of a name, either, counts as that version taken.
- **At start-up every .sieve in a filters folder is decoded once into the cache**
  (`<plugin cache>/sfilters/<the .sieve's SHA-256>/`, with an index of each member's SHA-256 and size),
  checked again at every start and decoded again where anything differs; its filters load exactly as
  loose ones, with the same SHA-256. The hallway loads plugins as it starts, so they are decoded and
  cached ahead of use.
- A damaged .sieve (or one unfolding past 64 MiB, or holding no .sfilter) is listed by
  `sieve filters --plugins` with why, never skipped. `--plugins` marks packed ones `(packed)`.
- `sieve filters --pack FOLDER --out FILE.sieve` packs any filter and its lists the same way;
  `sieve filters --plugin FILE.sieve` and the Designer's `--design FILE.sieve` open packed filters.
- Measured: function-words-v2 and its tagged list, 1,024,539 bytes as two files, 99,974 as one .sieve.
- Loose .sfilter files load as before; the repository's reference filters stay loose (the oracle reads them).

## Files

- tools/cli/locate.hpp, locate.cpp: `files_of_manifest` (a .sieve's files in memory, checked as
  installing checks them, nothing written).
- tools/cli/plugins.hpp, plugins.cpp: the loader reads .sieve; decoded_filter_sieve, pack_filter_sieve,
  sfilters_in_sieve, sfilter_file.
- client/designer_model.cpp (saving), client/designer.cpp (opening), tools/sieve_cli.cpp (--pack,
  --plugin, --plugins), tools/cli/help.cpp.
- ci/steps/10-hallway.sh: the Designer's save is a .sieve; a filter packed, loaded with its SHA-256,
  run, then one bit changed and refused.
- docs: FILTER-PLUGINS section 21, MANUAL, IDEAS 19.185.

## Not yet

Catalogue entries and maps kept the same way; a release shipping the reference filters packed.
