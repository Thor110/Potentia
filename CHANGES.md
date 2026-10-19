# Changes: the rope by columns, `rope-wave-v2` and `sieve-manifest-v9` (19 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files.

## The per-column idea, built

`rope-wave-v2` adds one parameter, written ` C1` after the others (`L8 W111 S1 P0 K0 C1`). What the
wave leaves is counted **column by column** of the line (the value's position mod W), with a table
for each column. A column that holds one byte value all the way down (a space, a line end) costs
only its table entry, with no stream at all.

Your pi file couldn't be downloaded here: the site is blocked from this sandbox. So I made the
first million decimals with mpmath and laid them out as your file's size suggests: groups of ten,
ten groups a line, Windows line ends, 1,110,004 bytes. Its layout is my estimate, so its figures
won't match yours exactly.

| | bytes |
|---|---|
| the copy | 1,110,004 |
| LZMA2 (7z's stream) | 486,077 |
| the rope before (v1's best) | 491,890 |
| **the rope by columns (v2)** | **420,025** |
| the digits' own floor (1,000,000 × log₂10 ÷ 8) | 415,241 |
| as Sieve instructions (one file) | 420,214 |

That's within about 4.8 KB of the floor, and that 4.8 KB is mostly the column tables. On the same
copy, the earlier rope (rope-wave-v1) came out 491,890, close to your 490,473 on the real file.

It helps elsewhere too: the colour picture went from 494,774 to **468,078** bytes, with one table
per colour.

## The search had to change to find it

The copy's line length (111) wasn't found at first. With random digits the match scores are
noisy, and multiples of 111 score as well as 111 itself, so the candidates filled up with
multiples. Now:

- the length the best widths have in common is tried first (where it matches nearly as well as
  they do);
- each width is taken down to its smallest divisor that matches as well;
- multiples of a width already taken are left out, and four widths are tried instead of three.

So a picture's row width can now be found alongside its pixel width. The noisy CI sine also now
beats LZMA2 (10,213 bytes against 10,295).

## In the instructions: `sieve-manifest-v9`

v8 whose rope ways may use columns. It's made only where some file does; a v8 reader refuses a
column way. `sieve install`, the oracle (its own decoder) and the small `sieve-install` all install
it, and CI checks all three.

## Also

- Yes, the File Locator already shows the wave chosen, in the weighing table's "how" column.
- **Fixed in IDEAS:** last round's note that the weighing table shows the rope hadn't actually
  landed (an edit that only half applied). It's in now.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,172 checks, 0 failures. New checks cover columns in every round trip, a
  digit table, and parsing.
- The rope CI lines and new ones for v2 and v9.
- The degree-sample and model CI block again: unchanged at 9,618 and 4,257.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.87, README, HANDOFF, help.
