# Optimise Search routes the compressed formats (6 November 2026)

Two commits on top of your 98872fc, both local and not pushed:
- **1861a7f:** the picture formats list in FORMATS.md, and the ODDITIES entry on the zip.
- **90d5330:** this change.

## What changed
- **Optimise Search on (the File Locator's default):**
  - temporary is taken for every compressed file Sieve can make again exactly, whether or not its box is ticked;
  - each file is opened only where that is clearly smaller (a JPEG where its brunsli form is), so nothing comes out larger, and everything installs byte for byte;
  - the temporary box shows ticked and greyed, "(chosen file by file)", and A does nothing while Optimise Search is on.
- **Prioritise Decoding Speed forks it.** With it, an opened file's contents go to LZMA2 or the rope, never to mixing, so installs stay fast.
- **Never automatic:**
  - permanent, because it changes what is installed;
  - RAR, because remaking one needs the installing machine's own `rar` at the right version.

  A box you tick yourself still takes both.

## Measured here (4 threads; every .sieve installed back byte for byte)

**The repository's `Images` folder** (67 PNGs, 25 WebP, 10 MB):

| Settings | Size | Making | Installing |
|---|---:|---:|---:|
| Before | 8,884,242 bytes | 60 s | 2 s |
| Optimise Search | 5,574,710 (−37.3%) | 349 s | 30 s |
| Optimise Search + Prioritise Decoding Speed | 6,367,562 (−28.3%) | 309 s | 4 s |

**Other files:**

| File | Before | After | Change |
|---|---:|---:|---:|
| The web test site | 451,120 bytes | 382,940 bytes | −15.1%; making went from 13 s to 1 s |
| oddity-click.zip | 4,984 bytes | 3,880 bytes | |
| icon16.png | 1,317 bytes | 994 bytes | |

**Why making is slower:** an opened 1920×1080 screenshot is about 6 MB of pixels to pack. That is paid once, when making.

## Two slow steps fixed on the way
Neither changes any output:
- **The rope's search in the opening check:** the whole search on the opened pixels was used only to decide whether opening paid. It now runs only when LZMA2's estimate doesn't already decide it. One screenshot went from 30 s to 9.5 s, with an identical .sieve.
- **The stb check on PNGs:** a PNG not laid out as stb_image_write lays them out is no longer decoded and encoded ten times to find that out.

## Tests
- **CI step 10:** passes in full (874 s).
- **Unit suite:** unchanged since the last run.

## Docs
- FORMATS.md has the table.
- MANUAL and `sieve help locate` describe the routing.
