# The figures picture model, shipped

Against your main as it is now. It adds the run in your screenshot as a shipped run, so it's in
the AI Training Harness by default, beside universalbibliothek.

## `Sieve/data/training/figures-2x32/` (new)
- **`diffusion.ini`:** the recipe.
  - Model: 2 layers, width 32, 2 heads, 8 bits a weight (40,960 weights).
  - Noise: 16 levels.
  - Training: 15,000 steps of 8 pictures, learning rate 0.0001, loss scale 4.
  - Seeds 1 (training) and 7 (start).
- **`pictures.txt` (404 KB):** the 4,000 training pictures, drawn from the image line under
  figure-v1 (connect = four) and row-runs-v1 (changes = 2), seed 42.
- **`result.txt`:** each checkpoint's hash and the final model's address.

The build copies the training folder beside the programs, and the release ships it.

## Checked here (Linux)
- **`sieve diffuse data/training/figures-2x32`:** "reproduced exactly", byte for byte. It takes 7
  minutes here; on your machine it's a check you can run in the harness too (Enter on the run).
- **Its final model makes the six pictures in your screenshot,** by address.
- **New CI line:** checks those six addresses (fast). Training it again is left out of CI for
  time.

## Docs
- README (Picture models: the shipped run).
- HANDOFF.
