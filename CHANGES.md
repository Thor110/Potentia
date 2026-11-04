# The prediction wheels measured; Manual Navigation built (4 November 2026)

Copy `Sieve/` over the repository's. `manual-navigation-example.png` beside this file is only to
look at: your four bytes after a few turns. It isn't for the repository.

## The wheels, measured first (`results/rope-wheels-2026-11-03.md`, its second half)

The second car: wheels on every value's prediction. Each value is predicted as a weighted sum of
its neighbours plus a bias (for a picture: left, above and above-left; for a file of one dimension:
the three values before it). Each weight is a byte, plus a bias for each 4,096 values. It was
steered on the same six files and set against the rope's own fixed predictions.

- **Steering finds the right fixed prediction, and no better one.** On the sound, x-ray and mr it
  turned the wheels to "the value to the left" or "the gradient", which the rope already tries.
  The best it found past them is 72 bytes on the sound (0.05%). On the picture it stopped above
  the rope's triangle. On a program and text it found that no prediction is best.
- **The region biases found nothing.**
- **Steering still beats guessing**, by 9% on the picture and 30% on the sound.

So turning a few fixed numbers for a whole file finds what the rope's search already finds. What
does better is a prediction that changes as the file goes, as mixing does (the picture 42,488
against the rope's 44,729). Steering stays useful as a way to search by hand, which is the screen
below. The script is `tools/measure_prediction_wheels.py`.

## Manual Navigation (`client/manual_nav.cpp`, `docs/MANUAL.md`)

On the main menu, the row above Exit. It is there and not beside the File Locator because the CI
reaches other rows by counting Down presses, and an item above them would have moved those rows.
Everything we agreed:

- **Load File** (Ctrl+O): any file up to 1 MiB, as it is; load a `.sieve` to steer one.
- **Left:** the bytes, 4 at a time (W: 8 or 16), each with arrows above and below.
  - Left/Right choose a byte; Up/Down turn it (Shift: 16 at a time); PgUp/PgDn move along the file.
  - Two hex digits typed set a byte as one step: the first waits in the box for the second.
- **Right:** the circle.
  - The dot's angle is the rotation (0 at the top, clockwise), and its distance from the centre is
    how far it is from landing on a bearing. The centre is an exact landing.
  - The first mark is in the binary line's colours, then green marks in and red marks out.
  - `[` and `]` change the precision.
- **Top:** the rotation, the route, the steps and their cost, and the net.
- **Undo and redo:** 10 deep (Ctrl+Z, Ctrl+Y). Every step is kept with its bearing and distance
  before and after, undone ones included. **Save Steps** (Ctrl+S) writes them as CSV.
- **The worker:** every step is worked out in order on a worker, so each step starts where the
  last landed and gets its own mark, however fast the keys are pressed. The route is worked out
  for files up to 64 KiB.

## Checks

- CI step 10 has a new block: your four bytes turned, a byte typed (one step), undo and redo, each
  step starting where the last landed, and the main menu's row. Steps 4 and 10 pass here.
- I can't click here, so the mouse (the arrows and buttons) wants a try in the hallway itself.

## The rewind

Sorry about that. It cost me the memory of having done the measurement and most of the screen, but
the files kept everything. You can report the false flag with the thumbs-down on that turn, or with
`/feedback` in Claude Code.
