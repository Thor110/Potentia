# Releases at the newest version, maps with a search limit (22 October 2026)

## Fixed
- **The release was v4** (`Sieve/tools/make_release.py`). `sieve.sieve` and the installer program
  were made without `--items`, so they came out as `sieve-manifest-v4`, without any of the packing
  since. They are the newest version now. Measured on this release's folder (made here end to end
  with `make_release`, 635 s): v4 7,939,263 bytes, now 7,819,304, 7z 8,414,101. The installer
  program installs the folder byte for byte, and no release file is an item, so `sieve-install`
  can install it.
- **Maps were slow** (`Sieve/tools/cli/map_sieve.*`, `sieve_cli.cpp`, `help.cpp`). The rope's
  search had no time limit, and spent 19.5 minutes on the source's 7z for nothing. Each held file
  now gets 10 seconds; `sieve map --search-per-file S` changes it (0: no limit). The release's
  `source.sieve` came to 47,779,886 bytes, against the 7z's 47,780,052, with the two downloads named.

## Measured, not built
- **IDEAS §19.94, distance along the line and rotation together, and grids of 1 to 65,535.** The
  saving is always eaten by naming the grid point; every file tried came out within a bit or so of
  its own address.
- **IDEAS §19.96, the degrees sample and the starting page as `.sieve`** (in the second zip).

**Docs:** IDEAS §19.94 to §19.96, README (map), HANDOFF, VOICES. **CI:** a check of
`--search-per-file` for maps.

To apply: copy `.github/` and `Sieve/` over the repository's top folder.
