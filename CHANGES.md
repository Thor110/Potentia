# Changes: the x86 instruction model, mix-v4's E1 (v34 draft, 7 November 2026)

One commit after 9976c0c. Full write-up: Sieve/results/x86-instructions-2026-11-07.md.

## What it is
A program's mixing given contexts from where each byte falls in its x86 instruction (a small decoder
of x86-64's encodings follows the bytes), its mixers' weights chosen by that too, and one stage after
the mixers. Written `E1` (`mix4 B20 R1 M4 C2 E1 X`). Only with `--v34-draft` (the File Locator's v34
DRAFT); without it, v33 exactly as before.

## Measured (v33 against v34, same bytes back every time)
| Program | v33 | v34 | Change |
| :--- | ---: | ---: | ---: |
| ls | 48,480 | 45,201 | -6.8% |
| t32.exe (32-bit Windows) | 38,701 | 36,413 | -5.9% |
| bash | 477,770 | 443,583 | -7.2% |
| git | 1,315,846 | 1,211,490 | -7.9% |
| cmake | 2,611,053 | 2,414,180 | -7.5% |
| Eleven programs together | 11,701,204 | 10,965,018 | -6.3% |

Decoding about 35% to 55% slower (git 5.2 s to 7.0 s). ls against cmix: 27% over, from 36%.

## Files
- core/src/mix.cpp, core/include/sieve/mix.hpp: X86Reader and E1.
- tools/cli/pack.cpp: programs take E1 under the v34 draft; v34 named where it is used.
- tools/cli/locate.cpp: E1 refused before v34.
- reference/sieve_ref.py: the oracle's X86Reader and E1 (reads Sieve's code byte for byte).
- tests/test_core.cpp: E1's parameters and a made-up program given back.
- ci/steps/11-oracle.sh: a program at v34 by Sieve, sieve-install and the oracle.
- docs/SPECIFICATIONS.md (v34), docs/NEXT-VERSION.md, docs/IDEAS.md (19.126).

## Tests
Unit suite: 154,370 checks, 0 failures. CI step 11's program blocks (v33 and the new v34) pass here.
