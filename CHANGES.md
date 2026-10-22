# A graph of the best so far in the File Locator (22 October 2026)

## Built
- **A line graph of the best found along the way** (`Sieve/client/file_locator.*`). Each time a
  step finds a new best, it's drawn at once onto a picture the File Locator keeps, and the value is
  let go: nothing is stored but each step's first try. The steps graphed are LZMA2's passes (a
  stream at a time) and the rope's search (a file at a time).
  - Each step's line starts at the top, its first try, and falls as it finds something smaller.
    The bottom starts 1% below, and doubles whenever a result goes deeper (what's drawn is
    squeezed upwards).
  - Time runs across to the estimate, or 10 s where there isn't one, and doubles the same way
    (squeezed to the left).
  - Each step has its own colour, named under the graph.
  - It's large while the File Locator works, and stays beside the result afterwards.
  - `graph-example.png`, beside this file, is `pi6.txt` read with the search at 20 s: the rope's
    line drops about 15% in its first seconds, and LZMA2's stays at the top (its later passes gained
    a fraction of a percent).
- `sieve::progress(what, best, of)` in `sieve/stop.hpp` now carries the numbers with the words.

**Checked:** unit tests, and every workflow step (6 to 16) locally.

**Docs:** IDEAS §19.97, README (File Locator), HANDOFF, VOICES.

To apply: copy `Sieve/` over the repository's `Sieve/` folder (the workflow is unchanged since the
last zip). `graph-example.png` is only for you to look at; it isn't part of the repository.
