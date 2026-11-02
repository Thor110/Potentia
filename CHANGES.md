# docs/ settled, and the palette (2 November 2026)

Unzip this over the repository root, on top of your commit 6471ef0 ("sieve rope probe"). Two local
commits, not pushed: a06486b and d06c887. The unit suite passes (77,147 checks, 0 failures; 14 new
for the palette). The new CI block in step 11 passes here; the full steps 10 and 11 haven't been
run on it.

## 1. docs/ packs the same every run (a06486b)

The run-to-run difference (594,466 or 598,620 bytes) was my test setup, not Sieve. The runs giving
598,620 used copies of the binary I'd moved out of its build folder, away from the dictionaries
Sieve finds beside it, so mixing had no word list (each file's log says "the word list not to be
had here"). The same binary beside its data gives 594,466. Four runs of the current build, alone
and two at once, all gave the same bytes. The note in `results/weighing-order-2026-11-02.md` now
says so. My other comparisons hold: the JPEG and PNG ones don't use the word list, and I reran the
rope-probe pictures with the data in place (same sizes).

Worth knowing: a Sieve binary moved without its data files still works, but packs text a little
larger (0.7% on docs/), and only the per-file log mentions it. A warning in the summary could be
added if you want one.

## 2. A picture of few colours mixed as its palette (v33 draft, d06c887)

**Measured first.** The repository's PNGs with 256 colours or fewer are the hallway's 17
screenshots (3 to 21 colours each). As raw pixels against one-byte indices plus the palette, the
indices won 16 of 17. Against what Sieve does with the PNGs today (they're stb_image_write's, so
already carried as their pixels and mixed), the indices came to 14% less overall.

**Built** as a mixing parameter, like the x86 filter's `X`: `mix4 B22 I1280x3 P7`. Mixing codes
the picture's one-byte indices (a picture of the same width), then the palette; installing turns
them back into pixels. The packer weighs it on the picture's start and keeps it where it's smaller.

| The hallway's 17 screenshots (PNGs, Compressed formats temporary) | Bytes |
| :--- | ---: |
| the PNGs | 896,976 |
| v32 | 185,088 |
| the draft | **146,999** (−20.6%) |

Every one smaller (−11% to −33%), every one given back exactly. Sieve, sieve-install and the oracle
read it. The table per screenshot is in `results/palette-2026-11-02.md`.

**A packer change at every version, found on the way.** Where the rope's best wave reads a file as
a picture under 16 wide (in effect "each pixel from the one before", with no rows), the shape
guessed from the bytes is now weighed too, kept only if 2% better. On the 13 decoded pictures from
the rope-probe work it changed nothing. On six-colour drawings of 320 × 240 that the rope read as
3 wide: 10,944 → 9,133 bytes and 8,005 → 7,516.

**New CI block (step 11):** a 320 × 240 six-colour drawing as raw pixels. With the draft it's mixed
as a palette of 6, smaller than v32 (8,843 against 9,135 bytes here), and given back by Sieve,
sieve-install and the oracle.

**Not yet measured:** other kinds of few-colour pictures (sprites, diagrams, pixel art, BMP/TGA),
and PNGs with their own palette (PLTE).
