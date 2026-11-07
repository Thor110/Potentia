# Changes: Size, Balanced and Decoding speed (7 November 2026)

Commits 225226c and 7ecb559 after e1cfb6b. Full write-up: Sieve/results/balanced-2026-11-07.md.
No new format: each setting chooses among the ways v33 has.

## Measured first
23 files and folders (Silesia, enwik8, the FLAC, programs, the GIFs, Pages/, docs/), each at Size and
Decoding speed, packed with Optimise Search and installed. Mixing installs at about 1.2 s a MB, so its
worth for a file is what it saves as a share of the file. The bend is between 3% and 4%: Balanced
mixes a file only where mixing saves at least 4% of it over the quickest other way.

| Setting | Bytes together | Installed in |
| :--- | ---: | ---: |
| Size | 128,683,728 | 335.2 s |
| Balanced | 133,597,417 (+3.8%) | 116.9 s (35%) |
| Decoding speed | 138,537,077 (+7.7%) | 10.8 s |

Mixed under Balanced: mr, x-ray, docs/, dickens, git, webster, osdb, bash. Not: enwik8 (3.3% saved for
134 s more), samba, nci, xml, the small programs. Every item installed as it was.

## Where it is
- File Locator: Prioritise Decoding Speed's box is now one button going round Size, Balanced and
  Decoding speed (P). Balanced unless changed; [locator] priority (an older prioritise_decoding = 1
  is read as Decoding speed). The right-click Create .sieve takes the hallway's setting.
- Hallway: --priority size|balanced|speed for a run.
- Command line: --priority size|balanced|speed; its own default stays Size, so benchmark figures and
  the CI's expectations are unchanged; --prioritise-decoding is speed.

## Files
core/include/sieve/mix.hpp, core/src/mix.cpp (Priority); tools/cli/pack.cpp (the 4% rule for a file,
the mixed stream and groups); tools/sieve_cli.cpp, tools/cli/help.cpp (--priority); client/
app_settings, app_main, file_locator (the button, the setting); data/lang/en.txt; docs (SPECIFICATIONS,
MANUAL, FEATURES); results/balanced-2026-11-07.md.

## Tests
Unit suite: 154,370 checks, 0 failures. Nothing in the CI changes (its command line stays at Size).
