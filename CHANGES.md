# Changes (8 November 2026)

Replace these files in the repository (paths from its root):

- Sieve/tools/bench/noise_seeds.c (new): noise from up to 2^28 seeds against a block of bytes.
- Sieve/docs/IDEAS.md: the more-seeds measurement added to section 19.128.

More seeds do not help: up to 268 million seeds on 4,096 bytes of a WAV, every count lost 33 to 79 bits.
