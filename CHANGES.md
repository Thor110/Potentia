# Catalogue entries and maps kept as .sieve, decoded at start-up and cached (12 November 2026)

On top of 2ed4757 ("sieve filters update"). Unzip at the repository root; nothing to delete by hand.
**Re-run CMake**: one new source file, tools/cli/sieve_cache.cpp.

## What it does (IDEAS 19.185)

- **One piece for all of Sieve's own .sieve files** (tools/cli/sieve_cache.hpp): a .sieve decoded once
  into the cache, `<plugin cache>/<kind>/<its SHA-256>/`, with an index of every member's SHA-256 and
  size checked at every start. Filters (kind `sfilters`) now use it, and so do catalogue entries.
- **Catalogue entries as .sieve.** A catalogue folder (`catalogue`, `languages`, or `--folder DIR`)
  may hold `.sieve` files of `.format` entries; each is decoded into the cache (kind `catalogue`) and
  its entries read exactly as loose ones, with the same versions. `sieve catalogue --pack FOLDER
  --out FILE.sieve` packs one, every entry read and checked first. One that does not decode is left out
  with a warning saying why. The command line and the hallway share one lookup (catalogue_folders).
- **Maps decoded ahead of time.** Maps are already .sieve (IDEAS 19.93). The hallway now decodes every
  map in its `maps` folder as it starts, on a thread of its own, and keeps them in memory (by path,
  size and time), so the Node Graph Viewer opens on them without waiting; writing a map drops its copy.
- **At hallway start-up:** filters (as before), the catalogue's .sieve files and the maps are all
  decoded and cached before first use; the map thread starts only once decoding's word lists are set.
- Measured: data/catalogue and data/languages, 7,912 bytes as loose files (with their READMEs), are 840 and 1,163 bytes
  as one .sieve each, and give all 194 entries from a release-shaped folder with no loose entries.

## Files

- New: tools/cli/sieve_cache.hpp, tools/cli/sieve_cache.cpp.
- Changed: CMakeLists.txt, .gitignore, tools/cli/plugins.cpp (uses sieve_cache), tools/cli/map_sieve.hpp
  and .cpp (read_map_file_cached, prefetch_maps), client/app_main.cpp (start-up), client/menu.cpp
  (catalogue_folders), client/node_graph.cpp (cached maps), tools/sieve_cli.cpp (catalogue --pack,
  --folder with .sieve), tools/cli/help.cpp, ci/steps/04-models.sh (a packed catalogue read with the
  same versions); docs: SPECIFICATIONS 12.0m, MANUAL, FILTER-PLUGINS 21, IDEAS 19.185.

## Not yet

The repository's own entries and filters stay loose (read and reviewed as text); a release could ship
them packed.
