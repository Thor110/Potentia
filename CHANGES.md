# IDEAS §19.121: restricted, pre-seeded temperatures (5 November 2026)

Commits b22a620 and the one after it (local, not pushed), on top of your c8ce9c7.

**b22a620** is §19.121 (the four ideas) again: your last push didn't include it, so I re-applied it on top of yours.

**The new bullet at the end of §19.121** is your temperature idea:

- **It settles the choice.** With a temperature from a few fixed values (0 for greedy, or a fraction such as 1/2) and draws seeded from the recipe, the same inputs make the same choices.
- **The arithmetic under it must match too.** If a probability differs in its last bit between two machines, a near tie flips and the two outputs part for good, greedy included. So it also needs integer weights and arithmetic (training-v1), an integer softmax with §19.119's floor, and the draw compared with integer counts, as mixing's coder already does.
- **What it opens for text.** A seeded draw from integer counts is exactly what an arithmetic decoder does with its input bits.
  - If the "seed" is as long as it needs, every text has one, and finding it is compression with the model, which is what mixing does now.
  - A fixed 64-bit seed reaches only 2^64 texts, and finding one is a search.
- **For diffusion,** it gives an exact maker, but the way back stays a search.
