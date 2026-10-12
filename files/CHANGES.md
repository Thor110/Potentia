# Picture models in the AI Training Harness, and notes

Against your main as it is now (twelve files, each checked against it). The three PNGs are
screenshots for you; they aren't for the repository.

## The harness trains picture models (diffusion-v1) beside text models
- **D, or "+ new picture model..." in the list,** opens a settings page:
  - the model's shape (layers, width, heads, bits a weight);
  - levels of noise, steps, pictures a step;
  - learning rate, loss scale, and how many training pictures to draw.
- **What the page says:**
  - the pictures' size and palette: the image line's, as the setup menu has it;
  - which filters they're drawn from, and how many pictures those keep. With figure-v1 and
    row-runs-v1, that's 325,263,852,704.
- **Enter:**
  - draws the pictures;
  - records the run beside the text runs (`pictures-<stack>`, with `diffusion.ini` and
    `pictures.txt`);
  - trains it, with a progress bar and a bits-a-cell curve.
- **A trained picture model shows six pictures it made from noise:** on choosing it (small models)
  or after training.
- **Enter again** trains it again and says REPRODUCED EXACTLY, or that it differs.
- **G** goes to it on the AI line: it's a model of the line's shape like any other.
- **Not yet:**
  - resuming a stopped picture run (it starts again);
  - packing one as .sieve;
  - the hallway showing a picture model's pictures on its shelf face.

## Shared code (`tools/cli/diffusion_run.{hpp,cpp}`, new, in sieve_compare beside train.cpp)
- **What it holds:** the run folder, recording, drawing pictures from a stack, and training with
  progress and stopping.
- **`sieve diffuse` now uses it.**

## Notes (IDEAS)
- **§19.14:** servers that know only what they need. Given (library, number) the file is made
  again by whoever asks, so a server can hold an index and nothing else.
- **§19.15 (new):** agreed flags for probes and long links. There was no earlier note about
  probes, so this is the first.
  - The flags are a stack both ends hold.
  - A message is a compact address under it ("all 0" is survivor 0).
  - Changing what to listen for is sending a new stack once.
  - Caution: a compact address has no redundancy, so it needs an error-correcting code around it
    on a noisy link.

## Checked here (Linux)
- **Unit tests:** 152,668 checks, 0 failures.
- **`sieve diffuse` after the refactor:** gives the earlier run's result.txt byte for byte, and
  the oracle replays it.
- **The harness, by screenshot:**
  - the trained 2×32 model's six pictures;
  - the new-model page;
  - a 125-step model recorded and trained in the harness itself (0.9 s, still noise at that
    length).

  The tool and the oracle reproduce that run exactly.
- **The client builds without warnings.**

## Docs
- README (Picture models, under the harness).
- SPECIFICATIONS §12.0g (In the harness).
- IDEAS §19.2, §19.14, §19.15.
- HANDOFF.

## New files
- `Sieve/tools/cli/diffusion_run.hpp`
- `Sieve/tools/cli/diffusion_run.cpp`
