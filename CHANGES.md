# The plan worked through

Holds everything since your last update of the repository: the step 11 case fix and the plan
(both sent before, in sieve-ci-case-fix.zip and sieve-plan.zip), and the work below. Commits
4060ad2, c6b59d6, a5d120a, 0e859d9, 9d64180 and aab7b4b. Each item's state is in
`Sieve/docs/ROADMAP.md`.

## Built

1. **Names that are one where case is ignored (D.3, no version).**
   - A `.sieve` or an archive made on Linux can hold `A.txt` and `a.txt`. Installing it on Windows
     or macOS would have written one over the other without a word.
   - All three installers now refuse such a pair before writing anything, and name it:
     `install_tree`, `unpack_archive` and `unpack_7z`.
   - Whether the destination ignores case is looked at, not assumed, so WSL's `/mnt/c` counts.
   - `sieve locate` warns of such a pair.
   - New: `tools/cli/case_names.{hpp,cpp}`, and `tests/case_pair.sieve` (80 bytes).
   - CI step 11 installs that file and a zip of the pair. On Windows it checks the refusal for
     real; here, with the check forced on, all three refused and wrote nothing.

2. **A PNG's rows carried unfiltered, `skr` (D.1, v32 draft, archive format 23).**
   - Under temporary, a PNG that preflate makes again carries its rows unfiltered, then their
     filter bytes, then the skeleton. Installing filters the rows again, exactly, and makes the PNG
     as `sk` does.
   - Specification, packer, reader, oracle and CI.
   - The five PNGs here that other programs wrote, as a `.sieve`, each back byte for byte:

     | PNG | before | with `skr` | |
     | :--- | ---: | ---: | ---: |
     | icon16 | 1,321 | 1,014 | −23.2% |
     | icon32 | 2,757 | 1,911 | −30.7% |
     | icon48 | 4,446 | 3,215 | −27.7% |
     | icon128 | 19,775 | 11,863 | −40.0% |
     | m1_sieve_scowl60 | 84,640 | 59,895 | −29.2% |

3. **Pixels mixed as their own shape (D.2, v32 draft).**
   - Mixing now also weighs the shape a PNG's recipe gives, beside the shapes it guessed before.
   - Before, a screenshot's pixels were tested as "3 wide", lost to LZMA2, and were never mixed.
   - The 18 hallway screenshots as one `.sieve`: 245,100 → 211,937 bytes (−13.5%), installed the
     same.
   - It's behind the draft only because the packer must make the same bytes without it. It writes
     only v26's picture parameters.

4. **Step 11's time by block (D.5).**
   - Each traced command now carries its start time.
   - `python ci/trace_times.py LOGS/11-oracle.trace ci/steps/11-oracle.sh` adds a step's time up by
     block.
   - Its first run found 27.6% of step 11 in one command: the oracle decoding, in Python, 1.4 MB
     of mixed archive contents. That block now packs with Prioritise Decoding Speed: 726 s → 3 s,
     the same checks.

## Measured, not built

- **C.2, progressive JPEGs.** libjpeg already optimises a progressive JPEG's tables, so there's
  nothing to gain there (0.0% on twelve). Installing a baseline JPEG as progressive is about 6%
  smaller on large pictures (the door 156,746 → 147,587 bytes at quality 85), and 0.2% larger on
  icon128. It would need a progressive JPEG writer of Sieve's own.

## Left for you

- **B.3 and B.4** (a small file's overhead; the archive format as a number). They take bytes out
  of v32's head, which is the same decision as B.5, the second flags byte.
- **B.2**, the group question, (a) or (b).

## Corrected

- My plan had listed the book cover among other programs' PNGs; it is Sieve's own (stb).
- It had also said hallway-door's 7,882 bytes were LZMA2's; they were mixing's already.

## Tested here

- The unit suite: 77,110 checks, 0 failures.
- Full CI step 11 passed (2,646 s) with every new block in it, on the build before the last two
  small changes. Those were the `skr` check made cheaper and the a21 speed-up; both blocks were
  run again after them and passed.
- Without the draft, the `.sieve`s of five PNGs are byte for byte as before.
