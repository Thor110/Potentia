# Where training's time goes, and one way tried (11 November 2026)

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

## Next

What may pay: several windows' positions taken together through each matrix (a step's 4 windows as
256 positions), so each row of weights is read once a step rather than once a window, and the
threads get work worth sharing. Same integers, same result; to be measured.
