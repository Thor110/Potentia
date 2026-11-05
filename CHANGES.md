# sieve-view: export as .simage on Windows (5 November 2026)

Commit 02a152c (local, not pushed), on top of your 3ae909c.

## The error

"not made: could not run sieve: DuplicateHandle(): The handle is invalid."

## The cause

- sieve-view is a windowed program, so on Windows it has no console and no error stream.
- To make the .simage it starts sieve, and SDL tried to hand sieve the viewer's own error stream. Windows had none to hand on.

## The fix (Sieve/client/view_main.cpp, make_simage)

- sieve now runs with its error output sent into the same pipe as its normal output, and no input.
- It runs in the background, so no console window flashes up for it either.
- In the background SDL does not keep the exit code. So the export counts as made when all three hold:
  - sieve printed its "installer ..." line;
  - it printed no error;
  - the file is there.

## Tested

- On Linux: CI step 10's viewer lines make view_pic.simage and read it back as one 1280 by 720 picture.
- The Windows case itself needs your machine: export a PNG as .simage from the viewer.
