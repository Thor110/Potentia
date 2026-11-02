# The night's work (2 November 2026)

Unzip this over the repository root, on top of your commit 2390b34 ("sieve bench all"). It holds four
local commits, none pushed: 4bccd0c, 49e5adc, 31a4094 and 6d2eb14. v32 stays frozen, and every new
format piece is still behind `--v33-draft`. The unit suite passes (77,118 checks, 0 failures).

## 1. mozilla is mixed, not LZMA2 (49e5adc)

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

## 2. Fixes from the night's code review (31a4094)

- Optimise Search's choice for a file now reaches every thread its work runs on. Without that,
  two runs of the same folder could differ.
- Installing now budgets C2's second match table (16 MiB) for each file mixed with it.
- The oracle reads a v33 head given as raw bytes, and v33 in `graph`.
- `ci/run.sh`: Windows' `rmdir` now gets `//s //q`, which Git Bash would otherwise turn into paths.
- `bench_power.py` reads the processor's energy from its package zones alone (their sub-zones had
  been counted twice). It also no longer stops when Windows can't report a process's times.
- `bench_files.py`: when the oracle refuses a file, the "Ways in full" cell is left empty and the
  bench carries on.

## 3. The roof (4bccd0c, 6d2eb14)

`tools/bench/bench_roof.py` (or `tools\bench_roof.bat`, which asks for the file) measures time and
memory for a large file's first 1, 4, 16, 64, 256 and 1,024 MiB. `results/roof-mozilla-2026-11-02.md`
has the results for mozilla, from this container (4 processors). Making took about 44 times the
file in memory at 49 MiB (2,152 MiB), and about 7.7 s per MiB. Installing took about 3 times the
file. On enwik9, your machine would show where the curve bends: `tools\bench_roof.bat` and give it
enwik9.

## 4. CI step 11: about 800 s shorter (6d2eb14)

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

## 5. Ideas for the next pass (IDEAS §19.112)

In the order they would pay: programs and the 3% test; a bar for mixing over 16 MiB that grows with
install time; the stream weighing's cost on large folders; a bitwise matched literal for the shortcut
seed; counting the cards as a transform; Optimise Search's rules for other kinds (waiting on your
runs); gliders and Life; and the hallway (the picture viewer and the power page).
