# Changes

One commit on top of 86f5768 (review 4, batch D): batch E, the last of review 4. Tools, RAID, the CI, and a fix to
a regression batch B brought in.

## Fixes you might notice
- **Fractal rulesets refused (from batch B):** the zip for batches A and B bounded the drawn and moving rankers by
  the filter memory setting, which refused rulesets that used to count. They are bounded by their node count again,
  as before; the CI's core-fractals counts (23830 and 91206) pass.
- **RAID:** a piece of a very large file could be given a size of 0 and then fail to join. A tally saved with CR LF
  line ends, and leaving with a keeper not in the tally, are now refused with a message naming the problem.
- **RAID and file paths:** paths with characters outside ASCII are read as UTF-8 on Windows.

## Faster
- `sieve raid place` and `raid leave` no longer make every piece to find one piece's size; `raid pieces` hashes each
  piece once.
- Plugins with the same bytes in two folders are read once.
- `tools/grammars.py --check` took 1m48s here.
- CI step 04 runs OPTIMISE FILTERS once and lists the fractal chart once (363 s here); step 10 checks one charted
  ruleset of each version through `fractal index` and `at` (1,771 s here).

## Smaller
- `sieve fractal check` takes several files, a line for each.
- `sieve help window` names degree-window-v2.
- check_formats.py also checks the documents with a CR it used to skip.
- A test that hid a failure behind `&&` in step 11 is two checks now.
- REVIEW-ROUND.md: review 4 recorded in section 4 (6, 6a to 6d); items to test by hand in section 1 (52 to 58).

## Files to delete by hand
None.
