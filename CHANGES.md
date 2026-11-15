# List O: O3 and O5, the renderer

The two renderer costs the reviews left, made. The pictures are the same pixel for pixel.

## What changed (`client/mesh.cpp`, `mesh.hpp`)

- **O3, the Character Creator's picture.**
  - A character drawn square-on (the Creator, and the setup menu's side panel) was drawn into a picture
    the size of the whole window, every pixel cleared, drawn and uploaded.
  - Now only the rectangle its spheres, bones and faces reach is drawn and uploaded. The rest is clear,
    as it always was.
  - The texture is still shown whole. Showing only the rectangle put some pixels one pixel off where the
    menu is scaled down to fit its window, so that was not kept.
  - Measured on a 20-sphere scene at 1920 x 1080: 3.7 ms a frame before, 2.6 ms now, the window's clear
    included. Most of what is left is the renderer laying the whole texture over the window.
- **O5, edges in their band.**
  - Each band of rows walked every edge along its whole length, throwing away the points outside it.
  - Now it walks only the steps that can land in its rows. Each point is worked out and tested as before.
  - Measured with 1,000 edges up to 400 px: 1.3 to 1.4 ms a frame on one thread before, 0.87 ms now.

## How it was shown the same

- **O3:** the old and new renderer, built into one small test program, drew the same scene three ways:
  the character in its box, crossing the box's edge, and running off the window. The three pictures were
  the same byte for byte.
- **O5:** random edges (flat, upright, on whole rows, off the screen) in 1, 3, 7 and 8 bands, the old
  loop against the new: 331,776,000 pixels compared, none different.
- **The program itself:** CI step 10's 528 screenshots, from two runs of the old build and one of the new.
  - 424 were the same in both old runs.
  - 22 of those 424 differ on the new build. Each is in a place that varies by itself: the memory bars, the
    character at the hallway's centre, the edge glow, the side panel's arm, and recording and audio timing.
  - The arm, run again: three old runs gave three different pictures, and the new build's matched two of
    them.
  - The Creator's pictures, which never vary, are the same.
- **CI:** steps 04 and 10 pass.

## Also

- `OPTIMISATIONS.md`: entries 3 and 5 removed, with their figures. 4 (recordings) and 7 (a measurement)
  remain.
- `REVIEW-ROUND.md`: O3 and O5 marked made.

## Files to delete by hand

None.
