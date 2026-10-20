# Changes: the weighing weighs every way; the byte above, `rope-wave-v5` (21 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. No new files. Steps 1 and 2 of the five; steps 3 to 5
follow in later zips.

## 1. The weighing weighs every way the instructions use

- **One shared function** now gives a file's three ways and their sizes: LZMA2 alone, the rope,
  and the pieces. The packer carries the smallest; the weighing weighs all three. They can't
  disagree again.
- **The File Locator's table** shows `lzma2` and `pieces z… r… rope …` as ways, beside the rope.
- **Your first photo:** it now weighs 4,111,435 bytes, against its instructions' 4,111,599. Before
  this change it weighed 4,122,311.
- **Not yet:** archives weighed as they'd be carried with the box ticked. Carried as its contents,
  one archive becomes many files, which the table's one-row-per-file layout doesn't fit yet.

## 2. The byte above: `rope-wave-v5`

- **Measured first.** Counted by its column and the byte above it, pi6.txt's entropy is 415,679
  bytes, 438 over the floor. By its column alone it's 418,604.
- **How it's coded.** Writing down all 566 combinations of column and byte above would cost more
  than they save. Instead, each combination takes the next group number the first time it appears,
  and those numbers are coded as a small stream of their own. Only 3 groups form. It works on any
  text, with nothing tuned to digits.
- **Results:**

| file | before | now |
|---|---|---|
| pi6.txt, rope code | 418,927 | **416,967** |
| **pi6.txt as Sieve instructions** | 419,096 | **417,139** |
| the CI's pi-like table (100,000 digits) | 42,083 | 41,790 |

- pi6.txt is now **0.46% over the floor** of 415,241. What's left is mostly the blank lines, one
  after every twenty lines, which the byte above can't count.
- All three readers rebuild it byte for byte: `sieve install`, `sieve-install` and the oracle.
- **Format:** `sieve-manifest-v15`, made only where a file uses v5. `--v14` and `--v13` make the
  older forms.

## Also

Two CI checks read `sieve locate`'s report from standard output without `--manifest`. Without it
the report goes to standard error, so the checks couldn't see it. Both now pass `--manifest`.

## Checked here

- The full build, with no warnings.
- The unit suite: 153,230 checks, 0 failures. New checks cover A1 parsing, round trips, the search
  choosing v5 (and v4 when limited), and refusals.
- The hallway CI step, start to end. New in it:
  - the weighing choosing pieces;
  - v5 against v4 on the pi-like table, with the oracle decoding both;
  - v15 installed by all three readers;
  - `--v14` making the older form;
  - v15 relabelled v14, refused.

I've sent pi6.txt as v15 instructions beside this zip.

**Docs:** SPECIFICATIONS §12.2, IDEAS §19.87, README, HANDOFF, help.
