# Making's memory, cut (7 November 2026)

Edward asked for it after WoWRevived (1,426,355,971 bytes, 237 files) could not be made on the Linux
machine here (4 logical processors, 16 GB): `sieve locate FOLDER --installer X --optimise-search`
grew to 13.3 GiB and was killed by the machine 61 minutes in. Making had held about 9 to 11 times
what it read (results/roof-enwik9-2026-11-06.md). Every change below makes the same `.sieve`, byte for
byte; each figure is the most the process held at once (its resident memory, sampled every half
second), on this machine.

## What held the memory, and what changed

Found by sampling the memory beside `--progress`'s lines, and by valgrind's massif on a smaller
folder of the same kinds of files (28 MB):

1. **The read-back** (every file decoded and compared, as installing does) ran inside the packer, so
   every buffer of the making (every file's bytes, their ways, pieces, scripts and streams) was still
   held while the instructions and the files read back were. Now it runs after them.
2. **Reading a compact `.sieve`** made it into text by appending, two or three copies of its streams
   at once. Now the text is sized once and the parts' copy let go as soon as it is in; and the
   bytes read are let go once the text is made (the read-back and installing pass their own).
3. **The files held twice at the read-back**: the packer's own copy and the one read back. Now the
   packer's is let go during it, every file read back is checked against the listing's size and
   SHA-256 (as installing checks; an item's "-" by the tree's SHA-256), and the manifest takes the
   read-back's bytes, so it ends as it was.
4. **The ways cache** (each file's ways kept for the File Locator's weighing) held their codes, up
   to 2 GiB or a quarter of the free memory. Now it keeps the waves, cuts and sizes, at most 64 MiB,
   and makes the codes again on a hit, each checked against the size kept.
5. **The pieces cache** kept both halves of every file cut, up to 1 GiB. Now it keeps where the cuts
   fall.
6. **The rope's estimate** counted every pair of a file's column and the byte above it in 64 bits,
   with a 32-bit number held for every byte. Now it counts in one pass, in 32 bits.
7. `locate` held a third copy of the instructions to write them. Now it holds a reference.

## Measured

| Folder | Read | Before | After | The same `.sieve` |
| :--- | ---: | ---: | ---: | :--- |
| WoWRevived's films, two WAVs and 51 small files | 190 MB | 2,878 MiB | 2,183 MiB | yes |
| The same, and an INTRO, MAPS.WoW and a third WAV | 528 MB | 4,750 MiB | 3,931 MiB | yes |
| **WoWRevived, the whole release** | **1,426 MB** | **over 13.3 GiB, killed** | **8,547 MiB** | made and installed: yes |

What grows with the files read came from about 7.5 MiB a MB read to about 5.4. WoWRevived, made for
size with Optimise Search on this build: **1,005,544,163 bytes** (1.42:1, 70.50% of the folder, 6.35%
under 7z's 1,073,748,401), made in 4,224 s; installed in 221.3 s at 5,573 MiB at the most, every one
of its 237 files the same as the folder's.

Checked besides: CI's PNG pair with both Compressed formats boxes (made again through preflate and
the PNG re-save), `docs/`, the test archives, and 3,960 items of Sieve's own lines (7,686 bytes, as
before), each installed back the same; the File Locator's weighing found its ways kept 8 times on a
28 MB folder, each made again to its kept size; the rope's unit tests.

## Not changed yet

- The **File Locator's memory estimate** (650 MiB and 9 times what is read, with Optimise Search) now
  says more than these folders take: about 12.6 GiB for WoWRevived, which took 8.5. It is left as it
  is until it is measured again on enwik9's prefixes, where one large file held more than these
  folders (5,319 MiB at 512 MiB before these changes); then it can be fitted again.
- The **search's own peak** (four large files searched at once, each with its pieces and rope code,
  and mixing's models of about 500 MB each) is sized already by the memory free (half of it), and is
  now the highest point on these folders.
