# source.sieve: a plain .sieve of the source folder (27 October 2026)

Unzip over the repository root. Committed locally (not pushed), on top of your 3e7fecb. No format change; nothing to rebuild (only the release script and docs changed).

## What was wrong
- **`source.sieve` was still a sealed map holding the 7z.** So it was never smaller than the 7z. v31 could only help where Sieve made that 7z again byte for byte, and on your machine it didn't, hence 49,244,484 bytes.
- **On a stand-in here** (58.5 MB of random bytes in place of your anchors folder), the 7z was made again and the result went from 66,614,143 to 64,742,836. That's little, because the stand-in can't be compressed.

## What the release script does now
- **Step 7:** `source.sieve` is the committed source tree's own Sieve instructions, newest version (`sieve locate <tree> --installer source.sieve --items`). It's made from the same tree as `sieve-source.7z` and installs as the Sieve folder.
- **Step 8:** checks that `source.sieve` installs back to that tree, file for file.
- **Publishing:** five files: `sieve.exe`, `sieve.sieve`, `source.sieve`, `sieve-source.7z` (the baseline, now published) and `metrics.txt`. SHA256SUMS.txt and the release notes list all five.
- **`--map-with` is gone:** there's no map to make again.
- **`-mmt=1` is gone:** it was only there so the held 7z could be made again.
- **`source.sieve` no longer names `sieve.exe` and `sieve.sieve`:** the SHA-256 list and the notes do.

## Measured here
On the source without your anchors folder (749 files, 38,415,528 bytes):

| | Bytes |
| :--- | ---: |
| `sieve-source.7z` | 8,006,568 |
| `source.sieve` (v30) | 6,185,486 |

- **Time:** step 7, with its install check, took 4 min 26 s here.
- **Your release:** it holds the anchors folder too (58.5 MB in 6 files), so your figure will be larger. How much depends on how those files pack.

## Files
- `Sieve/tools/make_release.py`
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md, SIEVE-INSTALL-USAGE.md
