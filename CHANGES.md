# Changes

One commit on top of 8cb572a ("sieve O6").

## Seeds, studied: O8
- **`sieve seeds`:** a new command for the study you described. Each file is a unit of the binary line of its own
  length, and a seed is the line's key. n seeds are spread evenly through the 64-bit seeds (`--seeds N`, 16 by
  default), and each file is read to its first 16 KB by default (`--max-bytes B`). For each file it prints the best
  seed's saving net of naming it, the seeds sorted by the bits they save (overall and by kind), and how seeds move
  files: whether a seed good for one file is good for another of its kind, how far a seed's neighbour moves a file,
  and whether the landings are even. It also runs the affine family, (P + b) mod M, and its b solved for a landing at 0.
  Nothing is written; no address changes.
- **The result (IDEAS 19.255):** 256 seeds on 16 files (three each of text, PNG, JPEG, .sieve and programs, and the
  Hurrian hymn MIDI), 17 seconds here. No seed pays: the best of n lands about log2 n bits nearer the start, and naming
  which one costs log2 n bits. One JPEG got a 26-bit landing under one seed, which reads back exactly, but it is chance
  (about 1 in 8,000 over the run; that file's other landings match chance exactly); without it the 16 files net -4 bits.
  No seed carries from one file to another of its kind, the landings are even, and neighbouring seeds land a file a
  quarter of the loop apart. The affine seed solved for a landing at 0 is as long as the place itself: the seed
  carries the file.
- **So:** Seed Search (O10) has nothing to buy on its own, and waits on O9 (orders by a key that means something).
- **CI:** step 02 pins a small run (two short texts, 8 seeds).

## Files to delete by hand
None.
