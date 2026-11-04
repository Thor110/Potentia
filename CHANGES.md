# The exact check (Manual Navigation), 4 November 2026

Commit 31e948c (local, not pushed), on top of 5f8ec17 and 76519a3.

## What it does

- Under the steering line, two new lines:
  - `exact: 360 x v / units = num / den`: the rotation as a fraction in its lowest terms. num and den are green where even, red where odd. Numbers over 96 bits show as their length in bits.
  - Whether the decimals end (a whole number of degrees, or after k places) or recur (repeating every P digits, worked out exactly where it can be, else "or more"). Then what the rotation costs written exactly, against the file's own address: "shorter address found" in green, or "no shorter address".
- It runs by itself for files up to 64 KiB. Above that, press **X** (or the new EXACT button); it shows its progress, and a turn lets it go.
- The denominator is always odd: the loop of every file of 0 to n bytes holds an odd number of files. So the decimals can end only where it is a power of 5. Example: `54 AA` lands on exactly 120 degrees (360 x 21931 / 65793 = 120 / 1).

## Timings (whole hallway runs, this machine)

| File | Time |
| :--- | ---: |
| up to 64 KiB | under 0.3 s |
| 100,000 bytes | 0.5 s |
| 416,970 bytes (pi6.txt.sieve's length) | 6.5 s |
| 1 MiB | 45.8 s |

For comparison, Python's math.gcd on 1 MiB, without the split below, took 89.2 s.

## Changed

- `Sieve/core/.../biguint.hpp/.cpp`: `BigUint::gcd`, Lehmer's algorithm L. It can be stopped, and is tested against Euclid's in `tests/test_core.cpp`. The loop's count times 255 is 2^(8(n+1)) - 1, which splits into four factors with nothing in common, so the gcd is worked out a quarter at a time.
- `Sieve/client/manual_nav.*`, `app_main.cpp`, `data/lang/en.txt`:
  - the check itself, and its two lines;
  - scripted runs print it.
- **Key change:** FRESH RANDOM BYTES moves from **F** to **R**. A to F are hex digits, and pressing F both toggled the box and typed an "f" into the byte.
- `Sieve/ci/steps/10-hallway.sh`: the exact lines for C0 25 F7 63 and 54 AA, and the X wait on a 70,000-byte file. The `F,W,Up` script is now `R,W,Up`.
- `Sieve/docs/MANUAL.md`, `docs/IDEAS.md` §19.117: the check, the timings, and the length discussion.

## Tested

- The unit suite: 154,318 checks, 0 failures.
- The Manual Navigation block of CI step 10: all pass.
