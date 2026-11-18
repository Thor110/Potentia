# Sieve: the fractals' structural filters counted on v3 (list F)

One commit on top of 7e9e25c (the same filters counted on v1 and v2).

## The fractals line now has a real figure

The four structural filters (fractal-draws, fractal-grows, fractal-no-idle and fractal-canonical) are now
counted exactly on v3, the version that is nearly all of the fractals line. So the line's share is known:

**FRACTALS: 99.999999999999...% (kept 10^-59.27)** with all four ticked.

v4 to v6 are still judged only, but at the default 256 characters they are about 10^-200 of the line, far
too few to change any figure shown, even at 20 decimal places.

How much each filter keeps of v3 on its own:

| Filter | Kept |
| :--- | :--- |
| fractal-canonical | 10^-59.27 (nearly all of the effect) |
| fractal-grows | 10^-0.17 |
| fractal-no-idle | 10^-0.17 |
| fractal-draws | 10^-0.05 |

## What v3 needed

The counter is the same as for v1 and v2: it works by the set of symbols the axiom reaches. v3 rulesets
differ in three ways that matter:

- **Several rules per symbol.** A symbol can have any number of rules, in the order written.
- **Contexts.** Each rule can have a context letter on either side (52 choices each), or none.
- **A different alphabet.** Words use 74 symbols, of which 52 letters can have rules.

The header lines (angle, scale, seed, ignore, digit definitions) reach nothing, so they are counted
separately, from the space's own parts.

## Other changes

- **Judge-only tab:** the four are off it, since they count now. So X and OPTIMISE FILTERS tick them on
  the fractals line, and OPTIMISE sets the line to compact. The hallway shows it as hide until compact
  exists. fractal-dimension, fractal-strange and fractal-lasts have to draw or run each ruleset, so they
  stay judge-only.
- **The metrics list:** for the fractals line it now says "compact: not on this line yet (its survivors are
  counted, not yet numbered)". Before, it claimed a compact saving that doesn't exist yet.
- **The setup page:** names any judge-only fractal filters that are ticked but not in the count.
- **`sieve fractal count --filters FILE`:** prints v3's survivors as well.

## Checks

- **Unit tests (step 01):**
  - Every v3 body of 17 characters, judged by the real filters, for all 16 combinations of the four.
  - The Python prototype's v3 figures at 30 and 40 characters. The prototype matched brute force in all
    336 checks on small grammars with several rules per symbol and with contexts.
  - The unfiltered total equals the space's own v3 count exactly.
  - 3,000 random v3 rulesets up to 90 characters, so several rules per symbol, contexts and header lines
    are all exercised.
- **CI step 04:** OPTIMISE FILTERS ticks the four on the fractals line.
- **CI step 10:** v3's 10^-59.27, the metrics line, and the no-compact-yet line.
- **CI steps 01, 04 and 10** pass, as do the help, help-library and settings checks.

## Next

Compact on the fractals line (F-B): finding the *k*-th survivor and a survivor's number.

## Files to delete by hand

None.
