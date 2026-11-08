# IDEAS 19.128 to 19.130, and the WAVs' recipe (8 November 2026)

One commit on top of 54e10f0 (the roundings zip): ff449d7.

- **IDEAS §19.128, noise run through a file, measured:** the best of 65,536 seeds over 64 KB of a
  WAV matched 341 bytes, against 256 on average. Clipping them saves 2,728 bits; saying where they
  were costs 3,094. On random bytes it's the same, so it can't gain on any file. The script is
  tools/bench/noise_match.py.
- **IDEAS §19.129:** the rope's waves of one file tried on every thread. This is cheap in memory
  since the memory pass. Not built.
- **IDEAS §19.130, Edward's notes:**
  - the main menu's estimate looks out of place;
  - Optimise Search stays, guided by the priority: a path per data type and per priority. Not built.
- **docs/ROPE-POST.md:**
  - the WAVs' recipe in full (`L16 W2 S1 P3 T1`: the rope alone, the two channels predicting each
    other, quiet and loud passages coded apart);
  - the two ideas measured against it.
