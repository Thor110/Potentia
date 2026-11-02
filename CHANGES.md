# The rope not searched on a file it cannot win (2 November 2026)

Unzip this over the repository root, on top of your commit 5eaaabe ("sieve hexdec"). One local
commit, not pushed: 30d1fe4. The unit suite passes (77,133 checks, 0 failures). The new CI block
in step 11 passes here; the full steps 10 and 11 haven't been run on it.

## What changed (every version, not only the draft)

Texts already had a test on their start that skips the rope's search where it can't win. Other
files had none, and the search has no time limit by default, so on your 160 MB maps it spent 768 s
for nothing.

Now a file over 512 KiB that isn't text gets the rope's own search on its first 256 KiB first. Where
that comes to 1.1 times LZMA2's estimate of the same or more, neither the whole search nor the
pieces are tried; LZMA2 and mixing are weighed as before. Try every way ticked by hand still
searches everything.

**Calibrated on 22 files** (pictures as raw pixels, a synthetic sound, programs, the map, mozilla's
tar, random bytes):

| Whole search | On the first 256 KiB |
| :--- | ---: |
| rope won (0.73 to 0.94 of LZMA2) | 0.69 to 0.94 |
| rope lost (1.15 to 5.71) | 1.23 to 3.71 |

The test took 0.2 to 7 s, where the whole search took 6 to 168 s.

**Packed whole, before and after** (same bytes unless marked):

| File | Before | After |
| :--- | ---: | ---: |
| the map, first 16 MiB | 165.9 s | 42.2 s |
| mozilla's tar, first 16 MiB | 97.5 s | 26.2 s |
| a hallway screenshot | 10,475 bytes, 16.5 s | **8,939 bytes**, 9.6 s |
| the word-list picture | 55,611 bytes, 20.6 s | **49,221 bytes**, 10.5 s |
| the Vitruvian picture (rope wins) | 24.4 s | 26.8 s |
| the synthetic sound (rope wins) | 15.2 s | 20.1 s |

Programs and random bytes took the same time either way. The two pictures came out smaller because
mixing got a better shape: the rope's wave read the screenshot as 3,840 one-byte pixels a row; with
no search, the shape was guessed from the bytes as 1,280 pixels of 3 bytes, the true one. Where the
rope wins, the test adds a few seconds to the search (2.4 s and 4.9 s here).

All of this is in `results/rope-probe-2026-11-02.md`; METHODS.md describes the step.

## STATISTICS.md (pi6)

The table already had 417,049, and your 12 bytes are its last two steps (417,061 → 417,052 →
417,049: 9 from the rope wave's update, 3 from v32's head). I added a paragraph for your run today:
the same 417,049 bytes in 2.8 s with the File Locator's defaults (it was 11.4 s with Try every
way), installed in 0.045 s.

## A correction

Last zip I guessed `docs/` packs differently from run to run because of the rope search's time
limit. There is no time limit by default, so that guess was wrong; the cause isn't found yet. The
note in `results/weighing-order-2026-11-02.md` now says so.

## New CI block (step 11)

1 MiB of random blocks repeated in a shuffled order (LZMA2 wins) logs "not searched"; a smooth
800 KB 16-bit sound (the rope wins) is searched. Both installed back exactly.
