# Exact word-filter estimate, the time budget, and fixed numbers made dynamic (relative to origin/main 54fd218)

Unzip `files/` over the Potentia repository root, or apply `budgets-round.patch` with `git apply`
from the repository root.

## 1. The word filters' estimate, exact (`core/src/filters/text_m1.cpp`)
**One row, not two.** Each word ranker (clean, words, window, title) keeps one row of counts, and
the estimate assumed two. Each count is now trimmed to exactly its size as it's stored (numbers
built by repeated adds over-allocate).

**The formula.** The row is (L + 1) × 56 + g·L·(L + 1) / 16 bytes, where g is the bits a count gains
per character.
- **The upper bound:** g starts at log2(27) = 4.75.
- **Measured when needed:** if the bound doesn't fit the filter memory, the real g is measured on a
  512-character table, once per filter, padding and dictionary, then cached.

**Measured against the allocator's own heap figures:**

| Filter | Length | Estimated | Actually held |
| :--- | :--- | :--- | :--- |
| clean-v1 | 30,000 | 257 MB | 256 MB |
| words-v1 | 30,000 | 156 MB | 153 MB |
| window-v1 | 30,000 | 156 MB | 152 MB |

**Longest pages ranked in 512 MB:**

| Filter | At the start of the review | Last round | Now |
| :--- | :--- | :--- | :--- |
| clean | about 21,000 characters | about 30,000 | about 42,000 |
| words and window (default dictionary) | about 21,000 | about 30,000 | about 54,000 |

**Test:** a words filter whose row wouldn't fit at 27 symbols per character, but does at its
dictionary's real growth. It ranks, with the same count as with memory to spare.

## 2. The time budget (H3 and H11)
**A new GLOBAL row, TIME BUDGET.** The longest one unit may take to open.
- 50 ms at first, 5 ms a step, PgUp/PgDn double or halve.
- Saved as `unit_time_ms`; `--unit-time MS` on the hallway and the `sieve` tool.
- The line rows moved down one (`kFirstLineRow` is 15), and CI moved with them, with a new check
  that the row saves.

**The budget bars and FIND MY LIMITS** follow it at once.

**The growth figure (H3).** How opening time grows with the address used to be assumed (1.6). It's
now measured at start-up, from conversions at 10,000 and 40,000 characters.

**symbol-entropy on black-and-white pictures (H11).** Its fixed 2,048 limit followed neither memory
nor time, and one rank there took about a second, against 50 ms.
- **How it's set:** the rank is timed at 256 and 512 symbols. The length that fits the budget is
  then timed itself, brought in until it fits, and kept per budget. The filter memory caps it first.
- **Measured here:**

| Budget | Ranks up to | One unrank there takes |
| :--- | :--- | :--- |
| 50 ms | 960 symbols | 50 ms |
| 200 ms | 1,498 symbols | 217 ms |
| 1,000 ms | 2,588 symbols | 1,107 ms |

**Applied like the filter memory.** What can rank depends on it, so the row only saves it. X, or
going into the hallway, applies it. Meanwhile a red line says *Time Budget Change Detected : Press X
to re-optimise all dimensions*, and the counts' cache keys carry it.

## 3. Fixed numbers now taken from the machine (H1, H4, H5, H6, H7)
| What | Was | Now |
| :--- | :--- | :--- |
| H1. Item cache | 4,096 items | the tiles kept plus one either side, × items per tile (2,048 at 128 a wall, 4,096 at 256) |
| H4. Largest address | 8×10⁹ bits | one that alone would fill the item cache's memory (a quarter of installed memory): about 11×10⁹ bits on a 16 GB machine |
| H5. Face time per frame | 4 ms | a quarter of a frame at the display's refresh rate (4 ms at 60 Hz, 1.7 ms at 144 Hz) |
| H6. Face worker threads | at most 6 | one per core, less the one that draws |
| H7. "Large file" | 65,536 bytes, written twice | one threshold: a file whose bytes take a quarter of the time budget to work out, measured (about 10 KB here at 50 ms) |

The hallway bench runs at about 16.6 ms per frame on the video, image and pages lines, where the
video and image lines measured 23–26 ms earlier in the review. Item pictures draw fully.

## Left for you to decide
- **The world's graphics allowance (512 MB):** the graphics bar counts it for everything but the
  item pictures. It could be measured from the textures the world actually makes.
- **The rooms drawn and kept (6 back, 7 ahead) and the rooms with pictures (1 either side):**
  these could become a graphics "view distance" setting.
- **The widest item picture (1,024 px):** raised to the renderer's own limit, a long page's picture
  could be 16,384 px wide, which is 1 GB each. It would need to follow the display cache instead.
- **The item cache's share of memory (a quarter of installed memory):** it could become a setting,
  as the counting memory is.

## Checked
- tests/test_core.cpp: 56,274 checks over both passes, 0 failures.
- Run locally, all these CI steps pass:
  - same addresses;
  - filters judge, count and rank the same;
  - models and the setup menu (with the new row check);
  - the bytes256 line;
  - books;
  - the whole hallway step.
- X takes 5.4 s.
