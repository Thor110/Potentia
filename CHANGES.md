# Changes: F16 kept as stored, widening a setting; the AI figure with its parameters (17 October 2026)

Copy `Sieve/` over the repository's `Sieve/`. From now on packages are laid out like this: `Sieve/`
at the top, `CHANGES.md` beside it, no `files/` folder.

- **F16 weights are no longer widened as they load** (`core/src/llm.cpp`, `core/include/sieve/llm.hpp`).
  - They are kept as stored, two bytes a value, as BF16 already was. Each is turned into a float
    as it is used, through a table of all 65,536 F16 values, which is exact.
  - **Widening is now a setting** for both BF16 and F16: `[models] widen_weights` in
    `sieve-hallway.ini` (off by default), `--widen-weights` on the hallway, and
    `sieve chat --widen-weights`. On: twice the memory, faster to read. The logits are the same
    to the bit either way.
  - Measured here on one thread, a model of 27.4 million weights:
    - stored: 54.9 MB; F16 64 tokens a second, BF16 48;
    - widened: 109.7 MB; F16 78 to 94, BF16 65 to 85 (two runs on a shared machine).
  - New unit tests (`tests/test_core.cpp`): BF16 and an F16 copy of the stand-in model, stored
    against widened, the same logits to the bit and exactly twice the bytes widened.
- **The AI figure in the metrics and on the map** (`client/menu.cpp`) now shows its parameters:
  `AI     : (2^B)^(256H+16LH^2), B=4 L=1 H=16: 16^8192 = ~10^9864.2`.
- **Docs:** README (`widen_weights`), SPECIFICATIONS §12.0c, IDEAS §19.86 (the F16 question
  answered, with the measurements), HANDOFF (a new entry, and the new package layout).
- **Checked here:** the unit suite, 153,106 checks, 0 failures; every changed client and tool
  file compiles with no warnings. Not run here: the hallway build, CI.
