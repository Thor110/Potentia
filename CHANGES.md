# Groups of files mixed together: `sieve-manifest-v30` (27 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your e132132. One new instructions version (v30); every older one reads as before.

## What it does
- **Files mixed together in groups.** v29 mixed files together in one stream, held to 16 MiB, decoded in one piece. v30 cuts the folder's files into groups of up to 8 MiB, in listing order. Each group is mixed (mix-v4), and the groups decode at once, alongside the files still mixed alone.
- **No size limit:** a folder of any size gets as many groups as it needs, so the release can be mixed together now.
- **Already-compressed files are left out:** a file whose start LZMA2 can't shrink by 5% stays as it was. The docs folder's PNG screenshots still go in, because they do compress by more than 5%.
- **A group is kept only where it's smaller** than its files packed apart.

## Measured
Sieve's data folder (27 MB, your own anchors left out), here on four threads:

| | Bytes | Install | Pack |
| :--- | ---: | ---: | ---: |
| v29 (here the same as v28: too big for v29) | 3,160,426 | 12.4 s | 94 s |
| v30, groups of 4 MiB | 3,154,427 | 9.9 s | 100 s |
| **v30, groups of 8 MiB (chosen)** | **3,100,643** | **14.1 s** | **108 s** |
| v30, groups of 16 MiB | 3,099,798 | 28.6 s | 140 s |

- **Why 8 MiB:** each group decodes on one thread, at about 0.6 MB a second, so the largest group sets the install time. Here 8 MiB keeps almost all of 16 MiB's gain at about half its install time.
- **The trade-off elsewhere:** on 34 MB of the repository's files that compress, 16 MiB groups were 4% smaller than 8 MiB ones.
- **The group size is the packer's choice, not the format's.** A reader takes a group of any size, so it can be moved later without a new version.
- **Small folders** (one group) come out about as v29: tools 262,273 (v29 260,529), docs 974,804 (973,631), tests 396,246 (396,337).
- **Every folder above installed back exactly**, by Sieve; the small CI folder also by the oracle and sieve-install.
- I haven't measured the full `sieve.sieve`. That's yours to see.

## The format
- **A group's first file:** `mixt <parameters> T<total>`, e.g. `mixt B22 D60 T7851713`. Its code is next in the rope stream (a varint, then the code).
- **The group's other files:** `mixn`. Each takes the next `size` bytes of the group, in listing order.
- **Refused:** a group whose files don't use up its total exactly, a `mixn` with no group open, and a file of another way inside a group.
- **Switches:**
  - `--v29` makes v29 or older;
  - Prioritise Decoding Speed turns groups off, as it does all mixing;
  - the File Locator makes v30.
- **The report line:** `groups    N file(s) mixed together in G group(s) by mix-v4 (B bytes, carried in C; apart A; sieve-manifest-v30)`.
- **The oracle** reads v30.

## Checks run here
- The build passes, as do the unit tests (154,214 checks, 0 failures) and `check_help` (606 options, none missing).
- CI steps 10 and 11 pass (10 in 762 s, 11 in 2,111 s here).
- **CI changes (step 11):**
  - the v29 block now names `--v29`;
  - a new v30 block mixes the same five files in one group and reads them back with Sieve, the oracle and sieve-install;
  - `--v29` and Prioritise Decoding Speed make no v30;
  - version ranges widened to 30.

## Files
- `Sieve/tools/`: cli/pack.cpp, pack.hpp, locate.cpp, locate.hpp, items.hpp, help.cpp, args.hpp; sieve_cli.cpp
- `Sieve/client/file_locator.cpp`
- `Sieve/reference/sieve_ref.py`
- `Sieve/ci/steps/11-oracle.sh`
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md
