# The File Locator's estimate: a quarter more, then refined as it goes (24 October 2026)

## What changed
- **Each step gets its own prediction.** The samples time six steps separately:
  - the address and SHA-256;
  - the zip comparison;
  - the 7z comparison;
  - making the instructions;
  - comparing the instructions;
  - the weighing.

  Each step's time is projected along its own straight line to the whole size.
- **The first estimate is their sum plus 25%,** as you suggested. It still says "estimated", and
  estimates still start only at 16 MiB.
- **It's refined after each step.** The time so far, plus the steps still to come as predicted,
  scaled by how long the finished steps took against their prediction. The search's time is taken
  out, and the scaling is kept between 0.5 and 4. While it works, the screen says "refined as it
  goes: about ... left".
- **The end message gives both:** "took X (estimated Y at first, Z as it went)".

## Measured here
| File | Took | First estimate | Refined |
| :--- | --- | --- | --- |
| 52 MB recording | 8 min 14 s | 7 min 38 s | 8 min 01 s |
| 20 MB repeated text | 4 min 48 s | 1 min 54 s | 5 min 06 s |

Before this, the recording's estimate was a third low. The repeated text is the worst case: its
repetition, 2.9 MB apart, can't show up in any small sample, so only the refined figure comes close.

## Tested here
Unit tests and the full local CI pass.

## To apply
Copy `Sieve/` over and rebuild. `data/lang/en.txt` has two new lines: `loc.estimate.refined` and
`loc.took.refined`.
