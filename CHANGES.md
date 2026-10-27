# Proof files made again, and the source.sieve figures corrected (27 October 2026)

Unzip over the repository root, after the v31 zip. Committed locally (not pushed), on top of v31 (cfe3fc3). No format change.

## Proof/
Both files made again with today's Sieve. Each installs back byte for byte, and the oracle gives the degree sample back too.

| | Before | Now |
| :--- | ---: | ---: |
| `degrees.sieve` (3,960 files, 13,321,756 bytes) | 8,784 | **7,887** |
| `welcome to the sieve.sieve` (32 bytes) | 162 | **159** |

- **Same version:** both are still v20. Items of Sieve's lines plus LZMA2 is all they use, so v21–v31 add nothing to them. The saving comes from today's packer; I haven't traced which change gave it.
- **The hallway gives the same file:** `hallway --sample-degrees` now writes exactly the same 7,887-byte `degrees.sieve` as `sieve locate --items`.
- **PROOF.md:** the two sizes are updated. Your 7z figure (22,961) and your install timing are your own measurements, so I left them. The 7z here (p7zip 16.02, one thread) came to 22,567.

## source.sieve: what my figures measured
- **What they were:** my "about 8.0 MB before, 6,189,841 after" was a stand-in. It was the source from git with your anchors folder left out (38.4 MB of files, a 7z of 8,005,603 bytes), because I keep that folder out of measurements.
- **The release's own** holds the anchors folder too: about 49 MB at v29 and v30, by your figures. Its maps were still v18 then, as you thought. I can't say what v31 makes of it without measuring that folder.
- **Corrected:** SPECIFICATIONS, IDEAS, HANDOFF and the comment in `make_release.py` now say which source the figures are for.

## Files
- `Proof/`: PROOF.md, degrees.sieve, welcome to the sieve.sieve
- `Sieve/docs/`: SPECIFICATIONS.md, IDEAS.md, HANDOFF.md
- `Sieve/tools/make_release.py` (a comment only)
