# The plan from here, with new ideas measured

Holds both of today's commits: the step 11 case fix (in sieve-ci-case-fix.zip too) and this.

## The plan: Sieve/docs/ROADMAP.md, made again

It now runs from the work in progress to the best ideas:
- **A. In progress:** step 11 on Windows, the draft ways, WoWRevived, the BCJ2 figure.
- **B. Finishing v32**, in order.
- **C. The largest open gains** from the documents.
- **D. New ideas** from this look round, three measured.
- **E. Speed; F. evidence; G. measured and set aside; H. further off.**
- **What waits on you.**

The short list at the top gives seven things in order.

## The new ideas measured (part D)

1. **A PNG's rows carried unfiltered under temporary.** PNG's filters can be undone and done again
   exactly, so the original comes back byte for byte by construction. On the two PNGs other
   programs wrote: icon128 19,775 → at most about 11,700 (−41%), and the scowl 84,640 → at most
   about 66,000 (−22%); the rows alone are 47% and 31% smaller. It's larger on small pictures, so
   it must be weighed per PNG. It's for v32.
2. **Mixing tried on a hallway screenshot's pixels** (no version): hallway-compact's pixels are
   packed by LZMA2 in 15,957 bytes, where mixing as a picture makes 14,059.
3. **Names equal but for case** (found by step 11's failure): installing checks for files already
   there before writing, so two names in one `.sieve` that differ only in case both pass, and on
   Windows the second would overwrite the first without a word. Read from the code, not tried on
   Windows.

Plus three not measured: undo any reversible filter before modelling, step 11's own time, and word
lists beyond English.

## Files

- `Sieve/docs/ROADMAP.md`: the plan.
- `Sieve/docs/IDEAS.md`: §19.109, pointing to it.
- `Sieve/docs/MANUAL.md`: ROADMAP's row.
- `Sieve/tools/png_unfiltered_probe.py`: the probe (writes a PNG's rows unfiltered, and its filter
  bytes); checked to give the same pixels.
- `Sieve/results/png-unfiltered-2026-10-31.md`: the tables.
- `Sieve/ci/steps/11-oracle.sh`: the case fix, as before.

No program code changed.
