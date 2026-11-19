# Changes

One commit on top of 8a52a3c ("sieve optimise fix").

## Pages of a range of lengths, stage O1: the address space
- The text space can now hold every page of a minimum to a maximum length, the shorter first, as the binary line holds
  its files. Nothing in the hallway, CLI or menus uses it yet (stages O2 to O6 follow), so the program behaves as before.
- Positional: a page's place is the pages shorter than it plus its own letters read as a number. Scrambled: the
  binary line's shuffle over the whole range. With minimum = maximum it is exactly today's space, every address unchanged.
- Checked: every page of 1 to 3 letters (20,439 of them) both ways in both orders; the Python oracle has the same
  space, and `tests/vectors_range_v1.tsv` (84 vectors, "welcome to the sieve" among them) agrees with the C++ (CI step 11).
- SPECIFICATIONS §4.1 describes it.

## Plans and ideas
- REVIEW-ROUND list O: the rest of the pages work (O2 to O6), the seed study (O8), orders by sort key (O9) and Seed
  Search for the File Locator (O10, with its time estimate and cap).
- IDEAS 19.254: runs of spaces named in the header, recorded and set aside.

## Files to delete by hand
None.
