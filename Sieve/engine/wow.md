# The `.WoW` archives of *The War of the Worlds* (1998)

Jeff Wayne's *The War of the Worlds* (Rage Software, 1998) keeps its files in `.WoW` archives.
Edward decoded them by hand; his WoWViewer (part of his WoWRevived toolkit) reads, plays and writes
them, and decodes and encodes their Huffman compression. These are notes made from his source and two
of the game's archives, which were measured and not kept (25 October 2026). Sieve does nothing with
`.WoW` files; this is for a future engine (`README.md` here).

## Two kinds

Both begin with four letters and a little-endian `int32` count of entries, then the table, then the
data. Everything is little-endian.

**`KAT!`** (data and maps: `Dat.wow`, `MAPS.WoW`). Each entry is 44 bytes:

| Bytes | What |
| ---: | :--- |
| 4 | 0 (in the archive measured) |
| 4 | the data's offset |
| 4 | its length |
| 12 | its name, ASCII, ended by a nul; the rest `0xCD` |
| 20 | `0xCD`, or a nul and 19 `0xCD` |

`0xCD` is the fill of the Microsoft compiler's debug heap: the tool that wrote the archives kept
whatever was in memory there, so the table cannot be made again from the files alone. Before each
entry's data are 32 bytes: its name again, padded with `*`. The data are not in the table's order.
Entries are of the game's own kinds (`SPR`, `IOB`, `SHL`, `SHH`, `SHM`, `WOF`, `INT`, `FNT`, `PAL`,
`RAW`, `DAT`, `HSM`, `HSH` in the archive measured), and many are compressed (FFUH, below).

**`SfxL`** (sound). Each entry is 16 bytes: an 8-byte name (nul-padded), the length, the offset.
The data follow the table in its order with no gaps; each is raw 16-bit mono PCM (22,050 or
44,100 Hz, not recorded: WoWViewer guesses from the length). Every length in the archive measured
is even.

## FFUH: the Huffman coding

A compressed `KAT!` entry is:

| Bytes | What |
| ---: | :--- |
| 4 | `FFUH` |
| 4 | the size unpacked |
| 4 | the code's size in bytes (with the 4 bytes of 0 after it) |
| 4 | the code's length in bits |
| 1,024 | how many times each byte value occurs, 256 `uint32`s (they add up to the size unpacked) |
| the code's size | the code, then 4 bytes of 0 |

The code is a plain static Huffman code built from those counts, its bits read from the lowest bit
of each byte up; its bytes are the bits rounded up to a whole byte, then the 4 bytes of 0. Built
the two least counts joined each time, ties going to the node made first (the leaves are made in
byte order, before any join), the first taken being the 0 branch, it is the archive's code exactly: in the archive measured, all 926 compressed
entries decoded with their stated lengths in bits and encoded again byte for byte.

## Measured (25 October 2026)

Sizes in bytes, packed by Sieve's packer (`sieve locate --items --optimise-archives`):

| | As it is | Its entries as files | FFUH undone, one file | `xz -9e` |
| :--- | ---: | ---: | ---: | ---: |
| `Dat.wow` (`KAT!`, 1,403 entries, 25,283,808) | 17,166,728 | 17,167,704 | 9,590,967 | 17,178,180 |
| `human.wow` (`SfxL`, 153 sounds, 30,768,118) | 24,164,317 | 23,619,652 | (none compressed) | 25,972,304 |

- **`Dat.wow`:** 926 of its entries (23,481,677 bytes) are FFUH; undone, they are 36,605,229
  bytes, which `xz -9e` packs to 9,043,252 (the FFUH bytes as they are: 16,590,608). Undoing the
  Huffman coding nearly halves the archive packed; cutting it into files does nothing (with FFUH
  undone, 9,749,285: worse than one file).
- **`human.wow`:** the sounds are already plain; as files, each gets its own rope table, 2.3% less.

## If an engine ever carries them

- A `KAT!` made again exactly: the table and the 32-byte name gaps kept as they are (the `0xCD`
  fill cannot be guessed), each FFUH entry carried undone and encoded again on installing.
- A `SfxL`: the table kept, the sounds carried as sounds (each its own rope, or the audio line).
