# Changes: the AI Training Harness's new run settings (9 November 2026)

Task 4 on the list: the settings of a new run, in the AI Training Harness.

## What changed

- **The settings have the keys first.** A new run's page opens with its settings chosen; Tab
  still moves to the files' order and back.
- **Six more rows** (sixteen in all), each changed with Left and Right (Shift: bigger steps):
  - **preset**: the AI line's shape (as the setup menu has it), or tiny (1 layer, width 16,
    2 heads), small (2, 32, 2), medium (4, 64, 4) or large (8, 128, 8); the bits a weight kept
    as set. A shape that is none of these shows as custom.
  - **training**: training-v1, v2 or v3 (v3 unless another is chosen). v1 has no loss scale, so
    its row then says so.
  - **momentum**: 0, 0.5, 0.8, 0.9, 0.95 or 0.99 (0.9 as before).
  - **shuffle seed**: any number from 0 (Shift: by 100).
  - **checkpoint every**: 10 to 10,000 steps (100 as before).
  - **threads**: as many as the batch can use (as before), or 1 up to this computer's. The
    result is the same on any number; the estimate and the step measured follow the number set.
- **R** puts the settings back as they start: the AI line's shape, with the loss scale and
  learning rate suggested for it, and the recipe's defaults for the rest (the files' order kept).
- The keys at the foot sit lower, so their two lines no longer cover the status line; "seed"'s
  label is short enough not to run into its value.

## Checked

- A run recorded with training-v1, momentum 0.95, seed 2, a checkpoint every 50 steps, on one
  thread: its training.ini says so, and training it again gives "REPRODUCED EXACTLY".
- CI step 10: the two harness walks changed for the settings having the keys first (a Tab added
  to reach the files; one Down more for the width, the preset now the top row), and the run above
  added as a check.

## Documents

- MANUAL: the harness's settings, the presets, R, and the settings having the keys first.
