# Changes: I opens the engine, the AI line's figure, larger models evaluated (17 October 2026)

Copy `files/` over the repository root, on top of the earlier zips.

- **Fixed: I with an item in hand now opens the Encryption Engine** (`client/hallway.cpp`). The
  key set the request but not `quit`, and the hallway only hands over to another screen when both
  are set, as the pause menu's rows do.
- **The AI line's figure** (`client/menu.cpp`): 2^32768 was right, and the setup menu and
  `--metrics` now write it `16^8192 = ~10^9864.2`. That is 16 values a weight (4 bits) to the
  power of 8,192 weights: the embedding 256 × 16 = 4,096, plus one layer's q, k, v, o, gate, up
  and down = 4,096. The heads split the width and do not change the count. `sieve ai`'s own line
  is unchanged.
- **Larger models, evaluated** (`Sieve/docs/IDEAS.md` §19.86): twelve published models, from
  their own config and safetensors headers, a few KB each, no weights downloaded. The table says
  what each needs beyond Sieve's engine, then gives a six-stage plan for taking models apart as
  they load. `HANDOFF.md` has the entry and the open list.
- **Checked here:** the changed files compile with no warnings. Not run here: the build, unit
  suite or CI.
