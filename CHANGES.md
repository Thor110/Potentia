# ai-train --limits, part 2: the probabilities, and why the attention grew (4 November 2026)

Commit a6df054 (local, not pushed), on top of the first --limits commit, which is also in this zip. This zip holds both commits' files.

## Added to --limits

- **The probabilities** (in 65,536ths): how often the right byte was given exactly 0, how often one byte was given all of it, and how many bytes' probabilities were 0.
- **Which end** of their range the weights sit at.
- **Running totals at each checkpoint** when checkpoints are shown, so you can see when the limits start being met.

## Measured (Sieve/results/training-limits-2026-11-04.md)

**The shipped run:**
- The right byte was given exactly 0 in 316 of 632,490 predictions (0.05%).
- 44% of bytes' probabilities are 0 at any one prediction.

**The larger model:**
- The right byte was given 0 about 1.27% of the time.
- 36 to 43% of bytes' probabilities are 0.

**Why the unscaled run's activations grew:**
- All 16.55 million held values were attention scores (query against key).
- None were held up to step 200; holding started after that.
- The weights at their range's ends sit on both sides about equally, so it isn't a drift one way.
- This resembles the attention growth that float training meets in large models; the usual cure there is QK-norm.

**Two ideas for a future training version.** Neither is built.
- QK-norm.
- A floor of 1/65,536 on every probability, needed before a model could be used to compress.

## Tested

- Every run reproduces its result.txt byte for byte with --limits.
- Unit suite: 154,318 checks, 0 failures.
- The CI lines (step 4): pass.
