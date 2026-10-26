# The roadmap: the triangle (v23), mixing (v24), PNGs (v25) (26 October 2026)

Unzip over the repository root. Commit 866ba4e locally (not pushed), on top of your bd1df0d. Each item is a new instructions version, and the older ones read as before. Details are in IDEAS §19.102 and SPECIFICATIONS §12.2.

## 1. Your triangle: rope-wave-v6, `sieve-manifest-v23`
- **`P4`:** predicts each value from the one before, the one above and the corner between them (the median edge detector of LOCO-I / JPEG-LS).
- **` T1`:** codes what's left in nine tables, by how busy each value's triangle is.
- **The search:** tries both at the widths it already finds, so the row length is still found from the bytes alone.
- **Measured** (Vitruvian Man as raw grey pixels, 949 × 1300):

  | Way | Bytes |
  | :--- | ---: |
  | `L8 W949 S1 P4 K1 T1` | 762,766 |
  | The rope's best before | 831,022 |
  | LZMA2 | 865,400 |

  It installs in 0.09 s. Where noise outweighs edges, the plain average (`P2`) can still win, and the search weighs them all.
- **New fixture:** `tests/rope/vitruvian-centre-256.gray`, a 64 KB crop of the public-domain picture stored as raw bytes, so no JPEG decoder version can change it. On it, the search picks `P4` by itself.

## 2. Mixing: mix-v1, `sieve-manifest-v24`, `sieve mix`
- **How it works:** every bit is predicted by the bytes before it (0 to 6) and by the word so far. The predictions are mixed by weights that learn, then coded exactly by an arithmetic coder, in integers only.
- **Measured** (Don Quixote, 2,391,678 bytes):

  | Way | Bytes |
  | :--- | ---: |
  | Mixing | 565,596 |
  | LZMA2 | 685,794 |
  | The rope | 1,324,573 |

  As Sieve instructions it comes to 565,664, against v23's 683,195. Packing took 22 s and installing 9.6 s: about 0.4 MB/s, decoding as slow as coding.
- **When it's tried:** files of 4 KiB to 16 MiB whose first 256 KiB, mixed, beat LZMA2 by 3%. `--v23` turns it off.
- **Same bytes everywhere:** a unit test pins one code's SHA-256. **If step 01 fails on Windows or macOS, that's the check telling us those machines make different bytes**, so please send me the line.

## 3. PNGs made again exactly: `sieve-manifest-v25` (Optimise Archive Formats)
- **Your Minecraft seed, found in Sieve's own files:** 9 of the 10 PNGs measured (the hallway screenshots) are exactly what stb_image_write writes at level 8. Those are carried as their pixels, and stb_image_write makes them again.
- **Other PNGs (zlib's):** go by a skeleton with a new segment, a run of IDAT chunks. Their deflate goes through preflate, and the chunks' CRCs are worked out again.
- **Measured:**
  - 10 repository PNGs: 657,221 bytes become 247,695 (519,859 without the box).
  - One screenshot: 43,886 bytes become 11,382.
- **When it's tried:** with Optimise Archive Formats. `--v24` turns it off.

## Also
- **An oracle fix:** it read any v22-or-later manifest file, whose head is in bytes, as a `.sieve` address. It now knows heads 16 to 25. The `.sieve` checks never hit this; only reading a manifest file directly did.
- **The oracle now has:**
  - its own rope-wave-v6;
  - its own mix-v1 (`mix-unpack`);
  - PNG reading: it checks both kinds and names them, as it does for preflate, since it has neither stb_image_write nor preflate.
- **CI:**
  - step 11 gains v23, v24 and v25 blocks;
  - step 10's rope checks now allow v6;
  - three expectations of an exact version now take a range.
- **Ran here (Linux):** the unit tests (154,116 checks) and steps 03, 10 and 11, all passing.
- **Not run here:** Windows and macOS, which matters most for the pinned mixing code.
- **Measured and not taken:**
  - the triangle and mixing on the screenshots' pixels: LZ's long repeats suit flat interface colours better;
  - preflate on stb_image_write's PNGs: its difference is bigger than the gain.

## Next, when you want them
- The triangle as one of mixing's models, so pictures can be mixed too.
- Mixing's speed, then more models (a dictionary's words, a grammar, the line above).
- LOCO-I's bias correction for the triangle.
- More generators as they turn up.
