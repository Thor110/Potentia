# Mixing's speed pass: the same bytes, made sooner (12 November 2026)

Edward: "carry on with the mixing speed pass next" (ROADMAP, "Next, in order", 1: after the v34
freeze). The rule: every `.sieve` made the same, byte for byte; only the time to make it changes.

## Where the time went

`sieve locate prog.bin --installer` on the first 5,337,607 bytes of a program (ctest, ELF, x86-64:
the size of the benchmark's program.exe), `--timings`, before: 21.1 s in all; mixing 11.8 s, the
read-back (every file decoded and checked, as installing will) 6.7 s. callgrind on a 256 KiB slice: 87%
of the instructions in mixing's model, of which each bit's prediction and update about 87%, building
the model's tables 11%. The model itself was already tuned (SSE2 mixers, prefetching, reciprocals);
asking for the next nibble's two possible lines a bit ahead was measured and gave nothing (4.30 s
against 4.20 s for 4 MiB, noise), so it is not kept. The time was in mixing the same bytes again.

## What changed (no format change)

1. **A trial carried on to the file's end** (`Mix4Run`, core/include/sieve/mix.hpp). For a file over
   4 MiB whose start chose all three match ways, its first 4 MiB are mixed twice (C2 alone, and all
   three) and the whole file then mixed again from its first byte with the winner. Where the trial's
   tables are the whole's and its bytes are the whole's own start (a program's through the x86 filter:
   the first 4 MiB filtered alone must be the whole's first 4 MiB filtered), the winner's run is
   carried on over the rest instead: the same model and coder, so the same code, and 4 MiB not mixed
   again. Otherwise, as before.
2. **Balanced's two codes of a program side by side.** Under Balanced a program is mixed whole with
   the x86 instruction model (E1) and without it, and the cheaper kept; the two are now mixed on two
   threads at once.

## Measured (this machine: a 4-core x86-64 container, Release build; seconds of wall time)

| File | Priority | Before | After | Bytes |
| :--- | :--- | ---: | ---: | :--- |
| program, 5,337,607 bytes | Size (the command line's default) | 22.0 | 15.9 | the same |
| program, 5,337,607 bytes | Balanced (the File Locator's default) | 27.0 | 13.5 | the same |
| text, 6,000,000 bytes (Sieve's docs joined) | Size | 7.5 | 6.7 | the same |
| text, 6,000,000 bytes | Balanced | 6.9 | 6.7 | the same |

`--timings` on the program, Size, after: 15.6 s in all; mixing 6.4 s (was 11.8), the read-back 6.4 s.
"Before" is the build from before this pass with `--v34-draft` (the same bytes as today's default, shown
when v34 was frozen). The same bytes were also checked on Sieve's docs/ and tests/ folders and a folder
of one of each of v34's kinds, under Size and Balanced (six of six). Installing is unchanged, as the
bytes are. A unit test (test_mix4_run) checks a run fed in parts against mix4_code, cut anywhere, a
program through the x86 filter with E1 too.

## Not done

The read-back's time is a decode of everything made, and stays: it is the check that what was made
installs. Each bit's model work was not made faster: no change found there gave the same code in
measurably less time.
