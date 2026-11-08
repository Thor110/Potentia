# The rounding experiment (8 November 2026)

One commit on top of 7e5854d (the memory zip): 54e10f0. It adds results/rope-roundings-2026-11-08.md,
IDEAS §19.127 and three measuring scripts in tools/bench (residual_repeats.py, hits_detail.py,
orders.py).

**The question:** do the rope's leftovers repeat across files, so that a table of common roundings
could pay?

**The answer:** not as roundings.
- With a table of 16-byte chunks, built from four WAVs and a scan and looked up on three other WAVs,
  1.3 to 1.9% is saved before the table's own size is counted. That size is about 200 MB.
- Shorter chunks lose.
- Random noise never hits the table.
- On a medical scan (`x-ray`), the table loses at every chunk size.

**What the hits are:** silence, runs of zeros where the wave predicts exactly; and runs of small
values, whose high bytes are 00 or FF. That's each file's own neighbourhood, not shared roundings.

**What to take forward:** coding each leftover byte given the byte before it captures the same
structure with no table, 2.2 to 2.7% on the WAVs. The rope doesn't do this now. It would be a new
rope version, so a new format version, and decoding stays a table lookup a byte.
