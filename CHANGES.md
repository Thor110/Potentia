# Changes: `sieve-manifest-v7` and `digit-histogram-v1` (17 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. One new source file, `core/src/digit_histogram.cpp`, is
added to `CMakeLists.txt`, so re-run CMake (Visual Studio usually does this by itself).

## The results

| | v4 | v6 | **v7** | for comparison |
|---|---|---|---|---|
| `sieve-ai-16432c2a3953.safetensors` (34,072 bytes) | 5,818 | 4,486 | **4,257** | your 7z: 5,698 |
| The degree sample (3,960 files, 13,321,756 bytes) | 482,496 | 391,033 | **9,618** | `tar` + `xz -9e`: 28,152 |

All four new files install byte for byte with `sieve install`, and the oracle gives back the same
files from its own reading of the format.

## What v7 does (`--items` now makes it; `--v6` and `--v5` make the older ones)

- **A shorter place for a trained model: `digit-histogram-v1`.** Each tensor's digits are named
  by two numbers: their histogram (how many of each digit), then the model's rank among all
  models with that histogram. A trained model's digits aren't spread quite as evenly as chance
  would spread them, so its number is shorter: **4,020 bytes for the model's 4,096**. This is
  exactly a "learn the counts as you go" code (Laplace's rule), done with whole numbers.
  - A model whose digits are spread as evenly as chance gets a slightly longer number, so v7
    uses the coding for a line only where it comes out smaller.
- **Less text around it.**
  - A single file's name is written once.
  - No `files`/`bytes` lines and no empty stream lines.
  - A line's options are written only where its ID doesn't already fix them: the AI line is now
    just `line=ai`.
  - Items are listed as `i K path`.
  - Each file's own SHA-256 is left out. The tree's SHA-256 still checks every file, as it
    already did for items; a damaged file is still refused, you just aren't told which one.
    `--file-hashes` keeps them.
- **The listing compressed.** Where it's smaller, the whole listing is carried as LZMA2. On the
  degree sample this takes it from 203,395 bytes to 6,110.

## Caveats

- **Against 7z on the degree sample:** 7z's stream of the sample's file contents alone, with
  no names or folders, is 9,226 bytes. A real `.7z` adds the names and folders on top (the
  sample's paths alone come to 4,744 bytes under LZMA2), so v7 (9,618, names and checks
  included) is probably smaller, but there's no 7z here to measure it. Worth a check on your
  side.
- **The model's place is close to its limit.** Its digits average 3.97 bits each out of 4, so
  the most any coding of this kind can save is about 2%, which the histogram coding gets. The
  AI line's existing filters can't shorten it without loss: `canonical-model` would give back an
  equivalent model, not the same file.
- **Tailored addresses won't help the degree sample.** A tailoring run on the sample found no
  filter stack that shortens any line's places, because its items are spread evenly round each
  line. They'd help where items cluster, like the CI's melodies (864 bits as their own address,
  36 tailored).
- **A correction to last round:** I wrote that v6 keeps the sample's 360 AI models as files.
  It carries them as places, because LZMA2 packs their near-identical places to almost nothing.
  The same thing caught v7's first version: it chose the histogram codes, which don't compress,
  and the sample grew to 405,002 bytes. v7 now compares the two after compression.

## Also

- The File Locator makes v7. Its weighing table still doesn't weigh the AI line (a model shows
  as its own address), though the instructions do carry it as a model. Not changed yet.
- `sieve-install` reads v7, compressed listing included, and refuses its items with the usual
  message.
- **Checked here:**
  - the whole build, with no warnings;
  - the unit suite, 153,130 checks and 0 failures (22 more than your last run), with new tests for the coding (every row of 5 digits of 3 values, and random
    even and uneven models);
  - the new CI lines (v5, v6 and v7 on the sample and the model; `--file-hashes`; 64-character
    pages; the oracle on each).
- **Docs:** SPECIFICATIONS §12.2 (v7 and `digit-histogram-v1`, and the correction), README,
  HANDOFF, help, and your two quotes in VOICES.
