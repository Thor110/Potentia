# Training faster, the same result (11 and 12 November 2026)

Stage 4 of the plan: training faster, with the same result byte for byte.

## Where it goes

`sieve ai-train` on 60,000 bytes of docs/MANUAL.md (2 layers, width 64, 4 heads, windows of 65
bytes, 4 a step), one thread, 10 steps under callgrind (instructions, not time):

| Function | Share of instructions |
| --- | ---: |
| `axpy_avx2` (the backward pass and the gradient: a row times a number, added into 64-bit sums in memory) | 32.0% |
| `dot_avx2` (the forward pass) | 15.3% |
| Everything else (attention, softmax, the update, reading the corpus) | each under 6% |

About 33 ms a step on one thread at that size; 27 ms on four (the four windows of a step go to four
threads, but the work is small). At 4 layers, width 256, 8 heads, 10 steps: 8.3 s on one thread,
6.9 s on four (this machine: 4 threads of an Intel Xeon at 2.1 GHz).

## Tried: the matrices turned on their side

The backward pass and the gradient rewritten as dot products over transposed copies, so each sum is
held in registers rather than added into memory row by row. The same integers summed in another
order, so the same result: the resume files after 80 steps (width 64) and 10 steps (width 256) were
the same, byte for byte. But slower:

| | Width 64, 80 steps, 1 thread | Width 256, 10 steps, 1 thread | 4 threads |
| --- | ---: | ---: | ---: |
| As it is | 2.74 s | 8.30 s | 6.87 s |
| Turned on their side | 2.87 s | 9.06 s | 7.47 s |

The copies cost more than the sums in registers saved: at a window of 64 positions each dot product
is short, and its last step (adding the four lanes) is paid once for every weight. Not taken.

## Tried: a step's windows through each matrix together (12 November 2026)

The four windows of a step as 256 positions one after another, each matrix read once a step,
attention kept within each window. The same result, byte for byte, but no faster: the matrices
already stay in the cache from one window to the next.

| | Width 64, 80 steps, 1 thread | 4 threads | Width 256, 10 steps, 1 thread | 4 threads | Width 512, 4 steps, 4 threads |
| --- | ---: | ---: | ---: | ---: | ---: |
| A window at a time | 3.01 s | 1.42 s | 9.47 s | 5.18 s, 5.36 s | 8.08 s, 8.05 s |
| Together | 3.47 s | 2.27 s | 9.88 s | 4.78 s, 4.74 s | 7.94 s, 8.45 s |

Not taken.

## Taken: the sums held in registers, and the update on every thread (12 November 2026)

The cost is in the arithmetic, so the arithmetic was changed, never its result (every sum is of
integers, so the order they are added in cannot change it):

- **Tiles.** The backward pass, the gradient and the forward pass are each one sum of the same
  shape, C[i][c] += the sum over k of A[k][i] B[k][c]. They are now worked four rows by eight
  columns at a time (sixteen with AVX-512), the 32 sums held in registers over every k and added
  into memory once, rather than a row added into memory at every k. The forward pass reads each
  matrix turned on its side, turned once for all of a step's windows (turned once a window, it was
  slower on four threads: 4.71 s against 4.06 s at width 256).
- **AVX-512** where the processor has it (chosen when the program starts, as AVX2 is): eight 64-bit
  products an instruction rather than four.
- **The update** (each weight's velocity, master value and digit) shared out among the trainer's
  threads; each weight's update is its own.

All on this machine (4 threads of an Intel Xeon at 2.1 GHz, which has AVX-512), whole runs of
`sieve ai-train --record ... --stop-after N` on the same 60,000 bytes; two runs each where two are
given:

| | Width 64, 80 steps, 1 thread | 4 threads | Width 256, 10 steps, 1 thread | 4 threads |
| --- | ---: | ---: | ---: | ---: |
| As it was (AVX2) | 3.01 s | 1.42 s | 9.47 s | 5.18 s |
| Plain C++ (no vector instructions), for comparison | 6.04 s, 6.00 s | | 16.80 s, 16.50 s | 7.26 s, 7.41 s |
| AVX2, tiles for the backward pass and gradient | 2.20 s, 2.54 s | | 7.50 s, 7.43 s | 4.56 s, 4.54 s |
| AVX-512, the same | 2.20 s, 2.00 s | | 6.44 s, 6.41 s | 4.31 s, 4.04 s |
| AVX-512, the forward pass in tiles too | 1.66 s, 1.83 s | 1.08 s, 0.95 s | 6.06 s, 6.04 s | 4.17 s, 4.17 s |
| **And the update on every thread** | **1.81 s, 1.82 s** | **0.74 s, 0.71 s** | **5.76 s, 5.49 s** | **3.34 s, 3.40 s** |

So a run of the shape the hallway's harness trains (width 64) takes about half the time on four
threads, and a wider one about two thirds. After every change, the resume files of all four runs were
the same as before it, byte for byte, and the unit tests' training vectors give the same steps on
the plain, AVX2 and AVX-512 paths (each tried in turn where the processor has it).

## Next

What is left is mostly serial within a window: the attention's loops over positions, the norms and
the head's softmax. Four threads now give 1.6 to 2.5 times one.
