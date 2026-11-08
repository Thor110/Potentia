# training-v3: every byte's probability floored and capped (9 November 2026)

Edward: "the AI line's min/max ... caps, enforced during training, as per the reasoning in the post
draft about quantised waffle" (docs/POST-DRAFT.md; IDEAS §19.119; results/training-limits-2026-11-04.md).
Probabilities are Q16 (65,536ths), and one under half of 1/65,536 was 0: a model said a byte could not
come next, and sometimes it came. A model used to compress (an arithmetic coder) must give every byte
a share.

## What changed

`training-v3`, the default for new runs (`sieve ai-train --record`, the harness's New run): v2 with the
softmax over the 256 bytes giving byte j `1 + (e_j (2^16 - 256)) div Σe`. Each byte is at least 1/65,536
and at most 65,281/65,536, and they add to at most 2^16. Training scores and learns from these
probabilities (the gradient `p_t - 2^16·[t is the next byte]`, scaled as v2's). Attention's softmax is
unchanged. Runs recorded before train as they did (`training-v1` and `-v2` are kept; `--training-v2`
and `--training-v1` record them). The oracle trains v3 apart (`tr_softmax_floored`), and
`tests/vectors_training_v3.tsv` (v2's three cases, ten steps) are checked by the core tests with one
thread and three. The picture and melody models (diffusion) stay on v2.

## Measured (`sieve ai-train RUN --limits`, this container, 2 threads)

| Run | Version | The right byte given 0 | Bytes' probabilities that were 0 | Updates with a gradient of 0 | Mean right-byte probability, last 3 checkpoints | Time |
| :--- | :--- | ---: | ---: | ---: | ---: | ---: |
| The shipped run (1 layer, width 16, 4 bits; 2,475 steps) | training-v1 | 316 of 632,490 (0.050%) | 44.19% | 0.019% | 28.03% | 14.5 s |
| the same recipe | training-v3 | **0** | **0%** | 0% | 28.03% | 13.7 s |
| 4 layers, width 64, 8 bits; 60,000 bytes of docs/MANUAL.md, 2 epochs, loss scale 2^6 (469 steps) | training-v2 | 1,606 of 119,998 (1.34%) | 37.13% | 1.09% | 15.18% | 29.8 s |
| the same recipe | training-v3 | **0** | **0%** | 0.000001% | 15.22% | 30.0 s |

No byte was given all of it in any run. Activations held at ±128: none in any. Gradients flowing back
held: 0.0168% (v2) and 0.0169% (v3) on the larger model. So the floor costs nothing measured in
learning or time here, and removes every zero. (The larger model's corpus is today's MANUAL, so its
v2 figures differ a little from those of 4 November.)

## Not done

QK-norm (IDEAS §19.119's other half: queries and keys normalised before they meet, against attention
scores growing to ±128 in unscaled runs) is not part of v3; with a loss scale, as every new run of
that size is given, no attention score was held in these runs.
