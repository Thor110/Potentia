# An archive's order (29 October 2026)

Unzip over the repository root. It goes on top of your "sieve planned work" commit. Committed locally
(not pushed). Documentation and the oracle only, so nothing to rebuild. CI step 11 (the oracle) was
run here and passed (1,615 s).

## What was decided

No checkbox. Every format Sieve makes again exactly keeps the order of what it holds in its own
recipe, never in the listing's order:

- `skm` numbers a zip's members (`m.zip/1`, `/2`, `/3` in the zip's own order);
- a RAR's recipe lists its members;
- `sk` keeps the data whole.

A format added later, such as a game's `.pak` or the `.WoW` reader if it is ever revived, does
the same. If one ever didn't, nothing would be installed wrong: the archive made again is checked
against its SHA-256 before anything is written, so the packer never keeps a way that loses it.

An archive carried as its contents (permanent) is a folder once installed, and a folder has no order.

## What changed

- **`Sieve/docs/SPECIFICATIONS.md`:**
  - The listing's rule, "sorted by their paths' bytes", now names its two exceptions: v31's
    members, and the contents of an archive carried as its contents. The latter come together
    where the archive was, in the archive's order.
  - A paragraph "An archive's order" beside the skeleton way states the rule above.
- **`Sieve/docs/SIEVE-FORMAT.md`:** the same exceptions, with an example, and that a reader
  shouldn't refuse them.
- **`Sieve/docs/IDEAS.md`:** a note on the `.WoW` entry (`engine/`) for whoever builds a game
  format's reader.
- **`Sieve/reference/sieve_ref.py`:** the oracle checks the order of every entry but those two.
  Before, it skipped any `.sieve` that held an archive's contents.
  - A zip carried as its contents within a folder is read.
  - The reviewer's unsorted file is still refused.
- **`Sieve/docs/REVIEW.md`:** the decision, in place of the open question.

## Files
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/REVIEW.md`
- `Sieve/docs/SIEVE-FORMAT.md`
- `Sieve/docs/SPECIFICATIONS.md`
- `Sieve/reference/sieve_ref.py`
