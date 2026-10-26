# Optimise Compressed Formats, and Try every way (26 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your 1ad181a. No format changes: the manifest is still v25, and every older version reads as before. (Your note about this morning's commit name is gone with this file.)

## 1. The rename: "Optimise Compressed Formats"
- The box, its hint, the packer's report lines, the install note and the docs now say **Optimise Compressed Formats**, since a PNG isn't an archive.
- **Command lines:** `--optimise-compressed` is the new name, in `sieve` and in the hallway. `--optimise-archives` still works, so your scripts don't break.
- **The ini:** the key is still `optimise_archives`, so saved settings carry over.
- SPECIFICATIONS notes the old name once. VOICES.md and older HANDOFF entries are left as they were.

## 2. A new box in the File Locator: "Try every way" (key E)
- It sits beside the compressed-formats box (key A). The hints line now reads: `F a file   D a folder   I install   Enter go to it   [ ] search, Shift the run   A compressed formats   E every way   S stop   Esc back`.
- When ticked, the search skips none of its shortcuts. The same switch as `--search-every-wave`, saved as `search_every_wave` in the ini.
- **What it adds:** it also covers mixing. Without the box, mixing is tried only on files from 4 KiB to 16 MiB, and only when a 3% sample says it might win. With the box, it is tried on every file.
- **Measured:** a 3,000-byte text (under the 4 KiB limit) came out at 1,563 bytes with the box against 1,688 without, and installed back exactly. So the limit can skip a winner on small files.

## 3. On your question: should no time limit turn the box on by itself?
My view: no, keep it a box only, off by default.
- **A search with no time limit already finishes.** The shortcuts don't cut it short. They skip trials that never won on the files where they were measured, which is what lets an unlimited search end in reasonable time.
- **Turning them all off can cost a lot of time.** That's worth choosing deliberately, not getting as a side effect of a time setting.
- **The box gives you both:** no limit with shortcuts (finishes, nearly always as good), or no limit with every way (slowest, never misses).
- The small-file case above shows the box is worth having. It also suggests lowering the mixing limit later, once more small files are measured.

## Checks run here
- The build passes, as does `check_help` (596 options, none missing).
- CI steps 10 (hallway) and 11 (oracle) pass, with their expected outputs updated to the new name.
- Screenshots show both boxes, and the hints line fits.
- I didn't run the other CI steps; they don't touch this change.

## Files
- `Sieve/client/`: file_locator.cpp/.hpp, app_main.cpp, app_settings.cpp/.hpp, unpack_7z.hpp
- `Sieve/data/lang/en.txt`
- `Sieve/tools/`: sieve_cli.cpp, cli/args.hpp, cli/help.cpp, cli/pack.cpp, cli/locate.cpp/.hpp, cli/rar_remake.hpp
- `Sieve/ci/steps/`: 10-hallway.sh, 11-oracle.sh
- `Sieve/docs/`: HANDOFF.md, SPECIFICATIONS.md, SIEVE-INSTALL-USAGE.md; `Sieve/README.md`; `Sieve/tests/archives/README.md`
