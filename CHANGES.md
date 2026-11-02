# Programs, the picture viewer, and the morning's work (2 November 2026)

Unzip this over the repository root, on top of your commit 2390b34 ("sieve bench all"). It holds
everything since then: the morning zip's four commits and today's one (5989bc4), none pushed. If you
already applied the morning zip, this one simply writes the same files again plus today's. v32 stays
frozen; every new format piece is behind `--v33-draft`. The unit suite passes (77,128 checks, 0 failures).

## Today

### 1. Programs are mixed through the x86 filter (v33 draft)

Measured first, on 16 programs of 98 KB to 11.8 MB (`results/programs-mixed-2026-11-02.md`): Linux's
own (bash, cmake, gdb, git, perl, python3, libc, libcrypto, libstdc++), Python's two Windows
launchers, and five of mozilla's Alpha libraries. Every one came out smaller mixed with the match
ways than under LZMA2's best:

- the 11 x86 programs, 3.0% to 15.1% smaller, once the x86 filter goes before the mixing (git
  1,549,874 → 1,315,846; cmake 2,921,639 → 2,611,053);
- the 5 Alpha ones, 2.9% to 4.3% smaller (the x86 filter doesn't fit their instructions).

The test on a file's first 256 KiB would have turned away 5 of the 16, all smaller whole (cmake was
8.5% larger on its start). And x86 programs never reached mixing at all: they went straight to the
x86 stream.

Built, behind `--v33-draft`:
- **`X`**, a new last parameter for mixing (`mix4 B22 R1 M4 C2 X`): the x86 filter before mixing,
  undone after. The filter is ported into Sieve and into the oracle, and both match the LZMA SDK's
  byte for byte on 18 files, both ways.
- **The packer** weighs each program (ELF or EXE) mixed whole through the filter against LZMA2 with
  the filter, without the start test. The programs it mixes are then weighed again against the x86
  stream they'd otherwise share. They stay out of groups and out of the stream's mixing, which have
  no filter.
- Three programs and the manual: **595,308 bytes with the draft against 651,739 without (8.7%
  smaller)**. Sieve installs it back byte for byte in 3.6 s; the oracle reads it back exactly.
- CI step 11 has a block for it: a made-up 48 KB program, 9,627 bytes against 13,948 (about 20 s).

Still open: t32.exe (98 KB) stays in the x86 stream, because the packer uses smaller tables (B18)
for a small file. There it came to 39,804 bytes against LZMA2's 39,438; at B22 it was 38,248.

### 2. sieve-view, the picture viewer

A new program beside sieve-install (`client/view_main.cpp`; MANUAL, under Commands). It opens PNG,
JPEG, BMP, GIF, TGA, and `.simage`, `.isieve` or a `.sieve` holding only pictures.

- **Moving around:** Right and Left go through the folder, and through a `.sieve`'s pictures. The
  wheel zooms about the pointer; 0 fits and 1 is one to one; drag to move.
- **Buttons:** Export (as PNG); Save file (a `.sieve`'s picture as it was carried); and **Sieve**,
  which makes `NAME.simage` beside the picture using the `sieve` beside the viewer, with Prioritise
  Decoding Speed.
- **Against bombs, before anything is decoded:**
  - Sieve now has a way to read a `.sieve`'s listing alone (`listing_of_installer`). The viewer
    refuses the whole file there if any file isn't a picture by its recipe or name, or if there are
    more than 4,096 files, a file over 256 MiB, or over 1 GiB in all. 300 MB of zeros named `.png`
    packs to 94 bytes; the viewer refuses it in 4 ms, with nothing decoded.
  - Then installing checks each file's size and SHA-256.
  - Then each picture's header is read, and one over 32,768 pixels a side or 268 million in all is
    refused before its pixels are made.
- CI step 10 checks it: a PNG, a `.simage` made and read back, a `.sieve` of texts refused, a
  100,000 × 100,000 header refused, and the window drawn.

### 3. The weighing does half the work where it can't change anything

Adding files to a stream can't make it smaller. So where the mixed files' codes are too small
to come back (64 bytes and 0.5% of the total), the second estimate is skipped. On mozilla's tar plus
two small texts, that was one 51 MB estimate instead of two (46 s of processor time). The results
are the same; the time saved is processor time, not waiting time.

### 4. The shortcut seed's matched literal, bit by bit

Coded as LZMA does, it is now the seed's default: the program 0.9% smaller, the teapot 0.6%, Don
Quixote 0.3%, pack.cpp the same. The seed is still 16.8% behind xz on the program.

### Not done

The rising bar for mixing over 16 MiB waits on the power benchmark's figures.

## The morning's work (already in the morning zip)


### 1. mozilla is mixed, not LZMA2 (49e5adc)

With `--optimise-search --v33-draft`, Silesia's mozilla is now mixed as bytes with the match ways
(`mix4 B22 R1 M4 C2`): 13,318,582 bytes against LZMA2's 13,376,239 (−0.43%). It installs back byte
for byte, in 96 s against LZMA2's 3 s. Two packer settings had kept it under LZMA2:

- **Mixing's margin.** Mixing replaced LZMA2 only when 0.5% smaller. That bar was set the day before
  for your five JPEGs. It is now 64 bytes and 0.25%, which still keeps the five JPEGs under LZMA2
  and lets mozilla be mixed.
- **A guessed picture shape.** A shape guessed from mozilla's start (48 wide) won the start by
  0.56% and lost the whole file. A guessed shape now has to win its start by 2%. Where the start is
  the whole file, the 2% bar doesn't apply and the smaller result is taken. With the bar,
  Vitruvian.jpg would have grown by 387 bytes.

Also new: `sieve locate --progress` prints the packer's progress as it goes (each file's ways weighed
and the one taken), as the hallway's Keep a log does.

### 2. Fixes from the night's code review (31a4094)

- Optimise Search's choice for a file now reaches every thread its work runs on. Without that,
  two runs of the same folder could differ.
- Installing now budgets C2's second match table (16 MiB) for each file mixed with it.
- The oracle reads a v33 head given as raw bytes, and v33 in `graph`.
- `ci/run.sh`: Windows' `rmdir` now gets `//s //q`, which Git Bash would otherwise turn into paths.
- `bench_power.py` reads the processor's energy from its package zones alone (their sub-zones had
  been counted twice). It also no longer stops when Windows can't report a process's times.
- `bench_files.py`: when the oracle refuses a file, the "Ways in full" cell is left empty and the
  bench carries on.

### 3. The roof (4bccd0c, 6d2eb14)

`tools/bench/bench_roof.py` (or `tools\bench_roof.bat`, which asks for the file) measures time and
memory for a large file's first 1, 4, 16, 64, 256 and 1,024 MiB. `results/roof-mozilla-2026-11-02.md`
has the results for mozilla, from this container (4 processors). Making took about 44 times the
file in memory at 49 MiB (2,152 MiB), and about 7.7 s per MiB. Installing took about 3 times the
file. On enwik9, your machine would show where the curve bends: `tools\bench_roof.bat` and give it
enwik9.

### 4. CI step 11: about 800 s shorter (6d2eb14)

Step 11 took 2,860 s here. Timed line by line, two of the oracle's reads took 800 s of that. Both
read files that have been mixed since v32, and the oracle mixes in Python:

| Block | Oracle's read before | After |
| :--- | ---: | ---: |
| v25 PNGs (the screenshot's 2.76 MB of pixels) | about 605 s | 0.3 s |
| v32 delta (its 330 KB base file) | about 195 s | 0.3 s |

Neither block is about mixing, so each now uses `--prioritise-decoding`, which keeps those bytes
under LZMA2. Each block still checks what it was written for: the PNGs are made again exactly
(25,221 bytes, under the block's 50,000), and the delta is made, installed and read back by the
oracle. Blocks that test mixing on purpose are left as they are; the next slowest reads take 27 to
58 s each.

### 5. Ideas for the next pass (IDEAS §19.112)

In the order they would pay: programs and the 3% test; a bar for mixing over 16 MiB that grows with
install time; the stream weighing's cost on large folders; a bitwise matched literal for the shortcut
seed; counting the cards as a transform; Optimise Search's rules for other kinds (waiting on your
runs); gliders and Life; and the hallway (the picture viewer and the power page).
