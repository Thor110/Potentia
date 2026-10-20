# Changes: gzip and zip for Optimise Archive Formats; CI at the repository root (21 October 2026)

Copy `Sieve/` over the repository's `Sieve/`, and `.github/` over the repository root's `.github/`.

**One thing to do by hand:** delete `Sieve/.github/workflows/build.yml` (and the then-empty
`Sieve/.github/`). The workflow has moved to the root, and copying a zip over the folder doesn't
remove the old copy.

Steps 4 and 5 of the five; steps 1 to 3 came in the earlier zips.

## 4. Optimise Archive Formats: gzip and zip (`sieve-manifest-v17`)

- **`.gz` remade exactly** where zlib made it. The recipe holds the gzip's own header, and
  installing rebuilds the identical file. Python's gzip always matches. Your GNU gzip -9 of pi6.txt
  matched too, though only one of three smaller files did, so many `.gz` files will fall back to
  the next point.
- **Otherwise a `.gz` is carried as what it holds:** `x.gz` becomes `x`, and `x.tgz` becomes
  `x.tar`.
- **`.zip` carried as its contents,** in a folder named after it.
  - Stored and deflated entries are read, each checked by its CRC.
  - A zip with encryption, another compression method, zip64, or a name that would escape its
    folder is left as it is.
- **Tar needs no unpacking.** Measured: the pieces already split a tar into its headers (LZMA2)
  and its files (the rope). A tar of pi6.txt and a text comes to 425,701 bytes, against 425,597 for
  the two files alone.
- **A fix:** an `.xz` or `.gz` located on its own and carried as what it holds now gets its new
  name as the root. Before, installing it would have made a folder named after the archive.
- **Format:** `sieve-manifest-v17` is needed only for a remade `.gz`. `--v16` makes v16.

| pi6.txt as | its size | instructions | how |
|---|---|---|---|
| a gzip from Python | 546,741 | **417,128** | remade exactly |
| a gzip from GNU gzip -9 | 546,777 | **417,128** | remade exactly |
| a zip | 546,827 | **417,107** | its contents |
| a `.tar.gz` from GNU tar (with a text) | 557,241 | **425,767** | its `.tar`, cut into pieces |

All of these install byte for byte with `sieve install` and through the oracle.

**Your aim of supporting every archive format** is in VOICES, and IDEAS §19.88 has the plan in
tiers:
1. Contents for anything Sieve can read: bzip2 and zstd next, then cab and others, each where its
   licence allows. RAR's licence has conditions to check first.
2. Remade exactly wherever Sieve has the very compressor.
3. Deflate not made by zlib remade exactly, by recording its differences, the way preflate does.
   This is the largest piece.

## 5. CI at the repository root

- `.github/workflows/build.yml` now sits beside `Sieve/`, so GitHub runs it on every push.
- It already ran every step inside `Sieve/` (`PROJECT_DIR: Sieve`), so no path in it changed. It
  parses as valid YAML.
- **What to expect:** its first runs on GitHub will be the first ever for Windows and macOS, and may
  turn up problems the Linux runs here didn't show. They cost Actions minutes on every push.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,230 checks, 0 failures.
- The hallway CI step, start to end. New in it:
  - a Python gzip remade byte for byte;
  - a zip installed as its contents with the note;
  - v17 called v16, refused by Sieve and the oracle.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.88, README, HANDOFF, VOICES, help.
