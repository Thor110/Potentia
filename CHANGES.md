# Sieve: the fractals' structural filters counted on v1 and v2 (list F)

One commit on top of 02fa9f3 (OPTIMISE FILTERS sets lines that are off to compact).

## What is counted now

Four of the fractal filters are now counted exactly on every v1 and v2 ruleset: fractal-draws,
fractal-grows, fractal-no-idle and fractal-canonical. With all four ticked:

| Version | Rulesets (to 256 characters) | Kept |
| :--- | :--- | :--- |
| v1 | about 10^357.6 | 10^-62.09 |
| v2 | about 10^355.1 | 10^-62.02 |

- **On the command line:** `sieve fractal count --filters FILE` prints both counts.
- **On the setup page:** "Fractals: v1 and v2 counted: kept 10^-62.09 of them; v3 to v6 judged only, as
  yet".

## Why the line's percentage doesn't move yet

Nearly all of the fractals line is v3. The whole line is about 10^405.1 rulesets, and v1 and v2 together
are about 10^-47 of it. So the line's percentage can only be worked out once v3 is counted the same way.
That's the next step (F-A3 in REVIEW-ROUND).

## How it counts

These filters ask what a ruleset's axiom reaches, rule after rule. Reading the text from left to right
doesn't show that, so the counter works by the set of symbols reached instead:

1. Every ruleset reaches exactly one set of the symbols that can have rules.
2. The rulesets that reach all of a set are all the rulesets on that set, less those that reach only part
   of it.
3. Which symbols the set holds matters only by their kind: F and G; f; the letters whose names change
   nothing drawn; and, for a substitution, every symbol.

All four filters, at every length to 256, take 2.4 seconds.

A first version walked the reach symbol by symbol instead. It agreed with brute force too, but took more
than five minutes at only 80 characters, so I replaced it.

## Checks

- **Unit tests (step 01):**
  - Every ruleset of the smallest lengths, judged by the real filters, for all 16 combinations of the
    four: the 168,948 forms of the 44-character L-systems (the same under each of the nine one-digit
    angles), and the substitutions of 47 and 48 characters, about 3.4 million.
  - The Python prototype's figures at 40 and 60 characters. The prototype was itself checked against
    every ruleset of three small alphabets.
  - The space's own counts, with no filter ticked.
  - 3,000 rulesets drawn at random up to 80 characters.
- **CI step 10:** the two kept figures and the setup page's line.
- **CI steps 01 and 10** pass. The fractals test part now takes about 3 minutes 45 seconds, mostly judging
  the 3.4 million substitutions.

## Not done yet

- **v3:** needs the same counting, with more kinds of rule part.
- **Compact on the fractals line (B):** this needs a way to find the *k*-th survivor and a survivor's
  number. Counting by sets subtracts counts, so it doesn't give that directly. The symbol-by-symbol walk
  does, but slowly. My suggestion is to design compact once, for v3's grammar, rather than twice.
- **The other three fractal filters** (fractal-dimension, fractal-strange, fractal-lasts) have to draw or
  run each ruleset, so they stay judge-only.

## Files to delete by hand

None.
