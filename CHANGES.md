# Changes: the tightened rope, built: `rope-wave-v1` and `sieve-manifest-v8` (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. **Re-run CMake:** there are four new source files
(`core/src/rope_wave.cpp`, `core/src/weights_rans.cpp`, `tools/cli/rope.cpp`, and a header). The
rANS coder moved out of `core/src/weights.cpp` into `weights_rans.cpp`, unchanged.

## What it is

Your filter, as an ordering any file can go through (so it works across every dimension: it takes
a file's bytes, which is the binary line). It runs a wave through the file's values, predicts each
value from the ones before it, and codes what the wave leaves by how often each byte occurs.
Parameters, written `L8 W949 S1 P3 K0`:

- **L:** value length, 8, 16 or 32 bits.
- **W:** the values in a line.
- **S:** how far back "the value before" is.
- **P:** your phases. 0 is the rope alone. 1 is the value before. 2 adds the line before. 3 adds
  the diagonal too.
- **K:** keep the values at the start, ⅓, the centre (odd counts only, as you said), ⅔ and the end.
  The rope is drawn through them.

**Pulling tight** searches every combination within the time given (the File Locator's time "a
file" now governs it too) and keeps the shortest.

## The results (code size against LZMA2, the 7z stream)

| File | Bytes | Rope | LZMA2 |
|---|---|---|---|
| Vitruvian, grey pixels | 1,233,700 | **832,146** | 865,400 |
| Vitruvian, colour pixels (475 × 650) | 926,250 | **494,774** | 623,717 |
| A noisy sine, 16-bit (made here) | 6,003 | **4,583** | 5,324 |
| The shipped text | 21,084 | 12,947 | **8,490** |

On the colour picture the search found a better wave than the one I'd designed: each colour
predicted from the colour beside it plus the change since the pixel before. On a cleaner sine it
found the sine's own period by itself, but LZMA2 still won there, so that file stays LZMA2.

## In the instructions: `sieve-manifest-v8`

- `--items` now makes v8 wherever the rope beats LZMA2 for a file. The file is listed as
  `rope L8 W3 S1 P3 K0`, and its code goes in a fourth stream.
- Files under 512 bytes are skipped, and so are files with a copy in the folder (LZMA2 stores a
  copy almost free).
- Otherwise the instructions stay v7. `--v7` turns the rope off.
- A mixed folder (2,190,959 bytes): **v8 1,341,514**, v7 1,504,516, and the File Locator's 7z row
  1,504,404 (68.66%), against v8's 61.23%.
- v8 needs no lines, so the small `sieve-install` installs it, unless it also carries items. Moving
  the coder into its own file brought `sieve-install` back down from 2.31 to 2.14 MB.
- v7 and v8 are now also made for folders with no items. The dropped hashes and the packed listing
  help any folder.

## Also

- `sieve rope FILE`: the wave the file pulls tight to, its code checked by reading it back, and
  LZMA2 beside it. `--params` sets the parameters; `--decode` gives the file back
  (`sieve help rope`).
- The oracle decodes `rope-wave-v1` with its own rANS (`rope-unpack`) and reads v8.

## Caveats

- **One wave through the five kept points never helped on its own.** On every file I measured,
  it came out a little worse than no wave at all. Your five points (K) remain as a parameter, but
  the gains come from propagation.
- **Text, meshes, MIDI and models aren't smooth, so the rope doesn't help them.** LZMA2 or the
  items keep those, which is why each file is checked both ways.
- **The search only tries the three line widths where the file best matches itself.** For a
  colour picture that's the pixel, not the row, so a wider search could find more.
- **Not done yet:**
  - other lines' places coded by the rope;
  - the File Locator's weighing table showing it.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,158 checks, 0 failures. That includes 28 new checks for the rope: every
  parameter on six files, refusals, and the search.
- The new CI lines: `sieve rope`, and v8 through `sieve install`, the oracle and `sieve-install`.
- Every earlier v5 to v7 line again, since v8 is now the default: the degree sample stays v7 at
  9,618 bytes and your model at 4,257.

**Docs:** SPECIFICATIONS §12.2 (`rope-wave-v1` and v8), IDEAS §19.87 (built, with what changed
from the plan and why), README, HANDOFF, help.
