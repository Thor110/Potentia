# The pieces measured, and a script for where Sieve starts to win (28 October 2026)

Unzip over the repository root. Committed locally (not pushed). No code of Sieve's changed; one
new script.

- `Sieve/tools/bench/ladder.py` (new): each file given, cut to its first 1 KB, 4 KB, 16 KB, 64 KB,
  256 KB and 1 MB (or `--sizes`), each cut packed by Sieve (installed back and checked) and by 7z
  (7-Zip's program, else py7zr); `--decoding` for Prioritise Decoding Speed; `--out` writes the
  table. For the plan's step 2: run it on a text, a picture and a program with 7-Zip installed.
  Tried here on `docs/GAME.md` with py7zr: Sieve 676 bytes against 777 at 1 KB (−13.00%), 1,692
  against 2,183 at 4 KB (−22.49%).
- `Sieve/docs/IDEAS.md` §19.103 measured: each block weighed with what comes before it (LZMA2's
  history, the rope's true wave), tried as a switch on four files. The `.sieve` came out the same
  size under every weighing, since mixing the whole file beats every cut (a mixed tar: best cut
  1,065,770 bytes, mixed whole 780,192). Not kept; the switch is gone. What remains, mixing as a
  kind of piece, waits for a new version.
- `Sieve/docs/HANDOFF.md`, `Sieve/docs/SHOWCASE-PLAN.md`: both noted in the plan.

## Files
- `Sieve/docs/HANDOFF.md`
- `Sieve/docs/IDEAS.md`
- `Sieve/docs/SHOWCASE-PLAN.md`
- `Sieve/tools/bench/ladder.py` (new)
