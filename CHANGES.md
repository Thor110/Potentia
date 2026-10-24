# A text's rope search ends after its own waves (24 October 2026)

## Measured first
On 127 files, searching every wave with no time limit:
- **The set:**
  - this repository's docs, code, tables, word lists, meshes, filter plugins and test files;
  - pi's digits, a log;
  - two I made to test the risk: 200,000 digits in lines of 50, and a fixed-width CSV.
- **The result:** of the 118 that count as text, 117 were won by a wave the search now tries first.
  That's the plain byte counts, the value before, or a line wave.
- **The one exception** was the digits in lines of 50. There, the columns at that width won by
  5 bytes (83,153 against 83,158).

## What changed
- **Fixed-width lines:** where every line of a text is the same length, the early waves now also
  include the columns at that width. That's one or two quick waves, and it closes the 5-byte gap.
- **Then a text's search ends.** `--search-every-wave` still searches everything, and the time is
  still yours to lengthen.

## Results
- **The 127 files, no limit:** exactly the same bytes as searching every wave, file for file
  (3,026,562 in total), in 29.3 s instead of 110.9 s.
- **The quick set at 1 s:** the texts come out identical to searching every wave, in about half
  the time:

| File | Every wave (s) | Now (s) |
| :--- | ---: | ---: |
| prose | 1.82 | 0.84 |
| code | 1.39 | 0.42 |
| oracle | 2.00 | 1.06 |
| mesh | 1.96 | 0.95 |
| rope text | 0.46 | 0.06 |

The tar doesn't count as text (its lines run to 4,096 bytes), so it's unchanged. Full table:
`results/bench-make-quick-2026-10-24-text.md`.

## Tested here
Unit tests and the full local CI pass.

## To apply
Copy `Sieve/` over, rebuild, and run the tests. `bench_make.bat` will show it on your machine.
