# GIF route, GIF playback, and a faster stream search (6 November 2026)

Three commits on top of 5505251 ("sieve gif plan measurements"):

1. `0a205dd`: **v34 draft: GIFs made again exactly from their frames' colour indices**
   - **What it does:** behind `--v34-draft`, under temporary or Optimise Search, a GIF is carried
     as its frames' colour indices followed by its skeleton. Its LZW is coded again on installing,
     as compress codes it (gifenc's coder; Pillow's too). The recipe is kept only where the GIF
     comes back byte for byte.
   - **Measured on your six demo GIFs** (9,616,677 bytes, 346 frames):

     | Settings | Size | Of the GIFs | Installing |
     | :--- | ---: | ---: | ---: |
     | v33, Optimise Search | 8,377,298 | 87% | |
     | v34 | 4,150,626 | 43.2% | 27 s |
     | v34, Prioritise Decoding Speed | 4,652,864 | 48.4% | 3 s |

   - **Also changed:**
     - SPECIFICATIONS §12.2 v34; the oracle reads it;
     - a new block at the end of CI step 11 (about 33 s here, mostly the oracle's Python);
     - the reader refused 20,000 damaged inputs, with no crash under AddressSanitizer.
2. `156dc3e`: **sieve-view plays animated GIFs**
   - **Playback:** each frame plays for its own delay. **Pause/Play** (P), comma and full stop step a
     frame, and Export writes the frame shown.
   - **Memory:** all the frames are made only where they fit in half the memory free, counted from
     the GIF's blocks first. Otherwise the first frame is shown, and the foot says why.
   - **Thumbnails** stay the first frame. A .simage or .sieve of GIFs plays too.
   - **New options:** `--check` counts the frames, and `--frame K` starts paused at frame K.
3. `ad27940`: **Speed: the lzma2 stream's four LZMA2 passes at once**
   - The settings trial (half a second) now runs first, then all four passes together, instead of
     the preset's pass alone and then the other three.
   - The six GIFs went from 161 s to 96 s to make, and the `.sieve` is byte for byte the same.

Unit suite: 154,318 checks, 0 failures. Tested here: the new CI block, the six GIFs both ways,
and viewer screenshots of frames 1 and 30.

**To try:**
- `sieve locate <folder of GIFs> --installer gifs.sieve --optimise-search --v34-draft`
- `sieve-view some.gif`, or open the .sieve in the viewer.
