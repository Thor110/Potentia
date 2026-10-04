# The remaining counting tables (relative to origin/main c8a8653)

Unzip `files/` over the Potentia repository root, or apply `remaining-tables.patch` with
`git apply` from the repository root.

## Every table, looked at
| Table | Grows with | What was done |
| :--- | :--- | :--- |
| Plugins (DfaRanker) | states × length | packed last round, now through the shared class below |
| not-written's binary table (Bn) | 872 states × length | **packed** (a bug fix, below) |
| Word filters (clean, words, window) | length² | **estimate corrected** |
| utf8-valid | 9 states × length² | done last round |
| not-written's joint memo | the walk of two-symbol units | measured: at most 41,000 entries (about 7 MB) in X's counts; left alone |
| Picture tables (palette-size, row-runs) | colours × pixels in scope | small; left alone |
| File kinds and page patterns | a 16-byte head | small; left alone |
| Models line (rules, canonical-mesh's binomials) | vertices × faces | small; left alone |

## PackedRows (`core/include/sieve/packed.hpp`, new)
The packing from last round is now one small reusable class: rows of exact numbers, each row one
block of limbs plus where each number starts. A row is worked out number by number into one
BigUint whose memory is reused, then stored at exactly its size. The plugins' tables (DfaRanker)
and not-written's Bn table both use it.

## not-written's Bn table, packed: a fix
**The problem.** Last round the memory estimate for counting tables (`DfaRanker::table_bytes`)
changed to the packed size. not-written's budget check also uses it for its Bn table, but Bn still
held one BigUint per number, so the check underestimated it. Bn has 872 states, and on a
3,000-character page its table would have been about 150 MB against an estimate of a fraction of
that.

**The fix.** Bn is now packed, so the estimate is true of it again. Its counts are unchanged: the
585 not-written vectors pass.

## The word filters' estimate (`core/src/filters/text_m1.cpp`)
**The problem.** It took every count at full length. Counts grow along the row, so on average
they're half that, and the estimate was twice the real rows.

**The fix.** Corrected, the word filters now rank pages up to about 30,000 characters within
512 MB, where they stopped at about 21,000.

**Still on the safe side.** Measured at 16,000 characters:

| Filter | Estimated | Actually held |
| :--- | :--- | :--- |
| clean-v1 | 146 MB | 87 MB |
| words-v1 | 291 MB | about 48 MB |

Words-only text has far fewer units than 27 per letter, so its numbers are shorter still. A growth
rate measured on a short prefix could make words' estimate exact. That's possible if you want it.

## Checked
- **Tests** (`tests/test_core.cpp`): 56,268 checks over both passes, 0 failures. A new test checks
  PackedRows: numbers of every size come back exactly, zeros are known without reading them, a row
  can be read while the next is worked out, and the estimate is never under what's held.
- **X:** 5.4 s, and the screenshot after it is byte-identical to the last round.
- **CI:** all of these steps pass locally:
  - same addresses;
  - filters judge, count and rank the same;
  - models and the setup menu;
  - the bytes256 line;
  - books;
  - the whole hallway step.
