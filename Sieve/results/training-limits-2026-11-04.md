# How often training meets its arithmetic's limits (4 November 2026)

Edward's friend asked of Sieve's integer training (SPECIFICATIONS §12.0e): "What size int. Is that
not inherently floored by max/min value?" Every number is a 64-bit integer; activations, gradients
and weights' values have 16 fractional bits and are held to ±2^23 (±128); a weight's master value
stays within its tensor's range, [-S, S]; gradients and momenta have limits of their own. The bounds
are part of the definition, the same on every machine, so they cost nothing in reproducibility; what
they could cost is learning, wherever a value would have gone past them. Edward: "realistically a
value should never hit the floor."

`sieve ai-train RUN --limits` now counts them (counting only: every run below gave its result.txt
byte for byte, as without it; counting is off by default, as it made the shipped run 4.6 s to 5.5 s).

## The shipped run (universalbibliothek: 1 layer, width 16, 4 bits a weight; 2,475 steps)

- Activations held at ±128: **0 of 537,556,560**. Gradients flowing back held: **0 of 710,798,880**.
- Of 20,275,200 weight updates, 3,840 (0.019%) had a gradient of exactly 0; no gradient or momentum
  reached its limit.
- 1,247,824 updates stopped a weight at an end of its range, and after the last step 538 of the 8,192
  weights (6.57%) sat at an end. At 4 bits a weight has 16 values, and its range is the tensor's
  scale, not ±128: these are weights that would have gone further than the 16 values reach.
- `tests/training_run_v1` (3,072 weights): none held either way; 325 weights (10.58%) at an end.

## A larger model (4 layers, width 64, 4 heads, 8 bits; 278,528 weights; 60,000 bytes of the MANUAL, 2 epochs, 469 steps)

The same run recorded at four loss scales (training-v2's `--loss-scale-bits`; 6 is what Sieve
suggests for this shape):

| Loss scale | Activations held | Gradients held | Gradients of 0 | Weights at an end after | Right byte given (mean, last 100 steps) |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 2^0 | 16,551,930 of 1,023,310,208 (1.62%) | 0 | 1.27% | 7,591 (2.73%) | 15.58% |
| 2^3 | 0 | 1,001 of 1,355,431,936 (0.00007%) | 1.16% | 1,898 (0.68%) | 17.99% |
| 2^6 | 0 | 259,597 (0.019%) | 1.14% | 1,921 (0.69%) | 18.49% |
| 2^7 | 0 | 2,202,236 (0.16%) | 1.32% | 2,058 (0.74%) | 19.33% |

No gradient or momentum reached its own limit in any run.

## What it says

- **On the shipped model nothing ever reaches ±128**, either way: Edward's expectation holds there.
- **On the larger model, the limit is met, and where depends on the loss scale.** Without it, 1.6% of
  activations were held, and that run learnt least. With it, activations stayed inside, and gradients
  flowing back were held more often the larger the scale (the scale carries them larger, which is its
  purpose: fewer fall below what Q16 holds). The runs that held more gradients still learnt more
  (2^7: 19.33% against 2^3's 17.99%), so here a rare held gradient costs less than a lost small one.
- **Why the unscaled run's activations grew past ±128:** see below.
- **Weights at an end of their range** (0.7% to 10.6%) are the quantisation's bound, not ±128: a
  weight of B bits takes 2^B values over its tensor's range.
- Only small models have been measured. Larger ones would want these counts again, and perhaps a
  wider Q format for activations, or 128-bit sums past 131,072 products a row.

## Why the unscaled run's activations grew (looked into the same day)

- **Where:** every one of its 16,551,930 held values was an **attention score** (a query against a
  key, before the softmax), counted by a temporary probe at each place a value is held: none in the
  residual stream, none in the final logits. The scaled runs held no attention score at all.
- **When:** none up to step 200 of 469; then 1,813,113 by step 300, 9,761,725 by 400 and 16,551,930
  at the end, as the weights at an end of their range went from 3,507 to 7,591.
- **Not a drift one way:** the weights at an end sit at both ends about equally (3,680 at the low end,
  3,911 at the high), so it is not the rounding down of every shift pushing them one way.
- **What it is like:** the queries and keys grow until attention picks one position outright. Float
  training meets the same thing in large models, attention scores that grow without bound, and its
  usual cure is to normalise the queries and keys before they meet (QK-norm), which would be a new
  training version here. Why it appears without the loss scale and not with it has not been shown:
  the scaled runs keep the gradients that a softmax this sure gives back, which are small, and the
  unscaled run's Q16 may round them to 0 (its updates with a gradient of 0: 1.27%, against 1.14% at
  2^6); that is a likely reading, not a measured one.

## The probabilities: a byte given exactly 0

Edward: "no one can say that something is 0% or 100% unlikely to follow something else ... the word
'waffle' might be very unlikely to follow the word 'quantised' but that doesn't stop me typing
quantised waffle." The probabilities are Q16 too (65,536ths), and one that comes to less than half
of 1/65,536 is 0. `--limits` counts them:

| Run | Predictions | The right byte given 0 | One byte given all of it | Bytes' probabilities that were 0 |
| :--- | ---: | ---: | ---: | ---: |
| The shipped run | 632,490 | 316 (0.050%) | 0 | 44.19% |
| The larger model, 2^0 | 119,998 | 1,523 (1.27%) | 0 | 36.43% |
| 2^3 | 119,998 | 1,515 (1.26%) | 0 | 39.97% |
| 2^6 | 119,998 | 1,516 (1.26%) | 0 | 40.41% |
| 2^7 | 119,998 | 1,557 (1.30%) | 0 | 43.13% |

So the models do say "0%", for about 4 in 10 bytes at any moment, and about 1 time in 80 (the larger
model) the byte that came next was one of them. Training is not harmed by it (the gradient for the
right byte is still its full -1), but a model used to compress would have to be given a floor: every
byte at least 1/65,536, as an arithmetic coder must. That, like QK-norm, would be a new training
version; nothing here is changed.
