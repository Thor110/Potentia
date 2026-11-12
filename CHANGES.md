# Mixing's speed pass: the same bytes, made sooner (12 November 2026)

On top of f8d72da ("sieve v34 frozen"). Unzip at the repository root; nothing to delete; no new source
files (CMake need not be re-run). No format change: every .sieve is made the same, byte for byte.

## What changed

1. **A trial carried on to the file's end.** A file over 4 MiB whose start chose all three match ways
   had its first 4 MiB mixed twice (the trials) and then the whole file mixed again from byte one. Where
   the trial is the whole's own start (same tables, same bytes; a program's through the x86 filter
   checked), the winning trial is now carried on over the rest: same model, same code, 4 MiB less work.
   New: `Mix4Run` in core/include/sieve/mix.hpp (a run fed in parts; its code is mix4_code's).
2. **Balanced's two codes of a program side by side**: with E1 and without, on two threads at once.

## Measured here (4-core container, Release; wall seconds)

| File | Priority | Before | After |
| :--- | :--- | ---: | ---: |
| program, 5.3 MB | Size | 22.0 | 15.9 |
| program, 5.3 MB | Balanced | 27.0 | 13.5 |
| text, 6 MB | Size | 7.5 | 6.7 |
| text, 6 MB | Balanced | 6.9 | 6.7 |

Same bytes on all four, and on docs/, tests/ and a folder of each v34 kind under both priorities.
The read-back (the decode that checks every made file installs) stays as it is.
Full notes: results/mixing-speed-2026-11-12.md. Your Windows benchmark (program.exe, folder.tar)
should show the program gain when you next run it.

## Files

core/include/sieve/mix.hpp, core/src/mix.cpp, tools/cli/pack.cpp, tests/test_core.cpp (test_mix4_run),
results/mixing-speed-2026-11-12.md, docs/ROADMAP.md.
