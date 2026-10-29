# A gigabyte's instructions read and made in proportion (29 October 2026)

Unzip over the repository root (after the mixing zip, "mix-faster", if it is not applied yet: they
touch different files). Committed locally (not pushed). Code changed: rebuild. No new manifest
version: every `.sieve` is the same bytes as before (with a time-limited search, it may come out a
few bytes smaller, the search getting further in its time).

From Edward's WoWRevived release (1.43 GB, mostly films and music): 0.20% over 7z, and its installer
went to 6 to 12 GB of memory and stopped answering while it read the file.

- `Sieve/tools/cli/locate.cpp`, `locate.hpp`: a `.sieve` read and made without a number the size of
  the file. Its manifest is its bytes with 1 taken from each, borrowed from the last, and back the
  other way (SIEVE-FORMAT §1): `file_of_raw`, `raw_of_file`, in place, one pass; `raw_summary` (the
  File Locator's display of an address, from one pass and no copy), `write_raw_address_file`,
  `write_installer_program` from raw bytes, `attached_raw`, and `has_attached` (from a program's last
  24 bytes). Checked against the number's way: 85,807 files and raw strings (every one up to two
  bytes) made, read and shown both ways, all the same.
- Every reader and writer moved to them: `sieve locate`, `sieve install`, `sieve-install` (which had
  read and converted the whole file twice, once on its window's thread just to see whether anything
  was attached), the File Locator, the web host, sites, maps, training runs, items and music
  favourites.
- `Sieve/core/src/rope_wave.cpp`: the rope's coder no longer holds every value as 64 bits in five
  arrays the file's length. Values are read where they lie (encoding, estimating) or held at their
  own width (decoding, into the output itself); positions, segments and the rope are made only where
  kept values or columns need them. The same codes, estimates and decodes on 100 files and settings
  (every version's ways, 8, 16 and 32 bits), against the old build's.
- `Sieve/tools/cli/pack.cpp`: one append in `gzip_inner` written as resize and copy, the same bytes:
  GCC 13 warned of it wrongly since the parallel files change altered its inlining (now warning-free).
- `Proof/STATISTICS.md`: WoWRevived (English v1.1.2), zip, 7z, `.sieve` and the installer program;
  and `sieve-install`'s size on Windows, 4,497,408 bytes, also in `SIEVE-INSTALL-USAGE.md`.
- `Sieve/docs/IDEAS.md` §19.105, `Sieve/docs/HANDOFF.md`: measured and noted, with what is left.

Measured here on a folder made like it (377 MB: three 125 MB files of noise and the docs):

| | Before | After |
| :--- | ---: | ---: |
| Installing, time | 63 s | 13.6 s |
| Installing, memory at most | 11.5 GB | 2.4 GB |
| Making, time | 205 s | 182 s |
| Making, memory at most | 11.8 GB | 4.1 GB |

And the docs folder, its search unlimited: byte for byte the `.sieve` the earlier build made, in 19.7
s against 23.0. Checked: the unit suite, 154,218 checks, 0 failures; CI steps 10 (544 s) and 11
(1,416 s) passed, both faster than before (667 s and 1,784 s).

Left (IDEAS §19.105): `sieve-install` still reads on its window's thread (seconds now, but the window
does not answer meanwhile); a large incompressible file's LZMA2 estimate is most of making it; and
where the 0.20% against 7z is, file by file.

## Files
- `Proof/STATISTICS.md`
- `Sieve/client/file_locator.cpp`, `Sieve/client/file_locator.hpp`
- `Sieve/client/installer_main.cpp`, `Sieve/client/item_save.cpp`, `Sieve/client/music.cpp`
- `Sieve/core/src/rope_wave.cpp`
- `Sieve/docs/HANDOFF.md`, `Sieve/docs/IDEAS.md`, `Sieve/docs/SIEVE-INSTALL-USAGE.md`
- `Sieve/tools/cli/locate.cpp`, `Sieve/tools/cli/locate.hpp`, `Sieve/tools/cli/map_sieve.cpp`
- `Sieve/tools/cli/pack.cpp`, `Sieve/tools/cli/site.cpp`, `Sieve/tools/cli/train.cpp`
- `Sieve/tools/cli/web_host.cpp`, `Sieve/tools/sieve_cli.cpp`
