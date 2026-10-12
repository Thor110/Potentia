# diffusion-v1: a model that makes pictures

Against your main plus the earlier zips, figure-v1 included (several files carry both).
`diffusion-samples.txt` holds three pictures from the model measured below; it is for you, not
for the repository.

## What it is
- **A denoiser of the AI line's own shape and tensors,** so a trained model is an address there
  already.
- **How it works:** it reads a picture of the image line with some cells replaced by noise, and
  gives every cell's own colour back. To make a picture, it starts from noise (any picture of the
  line, so an address) and works down the levels.
- **A picture is a function** of the model's address, the noise's address and a seed.
- **The training pictures are drawn from the image line's stack** in the filters file, figures for
  instance, so the data is found on the line, not drawn by hand.
- **Spec:** SPECIFICATIONS §12.0g.

## The trainer, generalised (training.cpp)
- **A window takes an Example:** the inputs, a target at each position (or none), and whether
  attention looks only back.
- **Windows of text are made into those examples,** so the language model's arithmetic is
  unchanged. `tests/training_run_v1` and the shipped universalbibliothek run both reproduce
  exactly, through the tool and through the oracle.

## `sieve diffuse`
- **`--record DIR`:** draws pictures from the stack (or reads `--pictures`), writes
  `diffusion.ini` and `pictures.txt`, trains, and writes `result.txt`.
- **`sieve diffuse DIR`:** trains it again and says "reproduced exactly", as `ai-train` does.
- **`--make DIR`:** makes pictures, each with its address and the noise's; `--show-levels` shows
  every level.

## Measured
- **Learning rate:** 0.0001 with a loss scale of 2^4 learns. Text's 0.0005 and above don't: a step
  here scores 800 cells, against text's 256.
- **2 layers of width 32, 8 bits a weight (40,960 weights):** 15,000 steps on 4,000 figures (from
  `figure-v1` four-way plus `row-runs-v1` 2), 5 minutes on 4 threads. It ends at 0.46 bits a cell.
- **Of 300 pictures made from random noise:**
  - 29 are one shape clear of the edges (`figure-v1`), against about 4 in 10^14 of random
    pictures;
  - none yet have rows as smooth as the training pictures: there are stray cells.
- **Tried, and measured no better:**
  - 64 levels instead of 16: 32 and 1 of 300;
  - keeping each cell once made: 7 of 300.

  The sampler stays as specified.
- **1 layer of width 16 (8,192 weights)** learned the clear border but not the figures.

## Not built yet
- **The hallway:** a model's picture on its shelf face, and J from the noise to the picture.
- **aispace-v2** naming which architecture a model is.
- **Larger and longer runs,** and a shipped run as an anchor.

Whether 40,960 weights is the smallest diffusion model can't be checked (IDEAS §19.2). What is
new here is that it is exact, replayable, and on the line.

## Checked here (Linux)
- **Unit tests:** 152,668 checks, 0 failures. New:
  - the diffusion vectors;
  - a window of text scored the same as its causal example;
  - the same example scoring differently when not causal.
- **New oracle vectors** (`tests/vectors_diffusion_v1.tsv`): each training step's score and digits'
  SHA-256, and a picture made, on two small shapes. The engine agrees with every row.
- **The oracle's `diffuse-replay`** trains a CLI run again to its `result.txt`, byte for byte. CI
  runs it, and `sieve diffuse RUN`'s own check.
- **The hallway builds** (it links the changed trainer).

## Docs
- SPECIFICATIONS §12.0g.
- README (`diffuse`).
- IDEAS §19.2: marked built, with the measurements.
- HANDOFF.
- `sieve help diffuse`.

## New files
- `Sieve/core/include/sieve/diffusion.hpp`
- `Sieve/core/src/diffusion.cpp`
- `Sieve/tools/cli/diffuse.cpp`
- `Sieve/tests/vectors_diffusion_v1.tsv`
