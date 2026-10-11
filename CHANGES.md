# The harness: a folder dialog, and models as adjustable as the state spaces

Against the curriculum batch (which you have committed).

## A corpus is a folder, chosen in a folder window
- **N** in the harness opens your system's folder window, the same kind the File Locator uses.
  - The folder's files become the new run's corpus.
  - A path can still be typed in its place, which is how CI drives it.
- **Single files are no longer a corpus:** the harness and `sieve ai-train --record` both refuse
  them.
  - Runs already recorded from a single file are untouched; a run is its files.
  - Recording the shipped text from a folder trains to the very same model as before
    (`b3e5b752...`).

## The new run's settings, each showing what it will take
The new run's page has two parts; **Tab** moves between them.
- **Settings**, each changed with **Left/Right** (**Shift**: bigger steps):
  - layers;
  - width, in steps of two heads, so heads stay an even size;
  - heads, only those that divide the width evenly;
  - bits a weight;
  - learning rate;
  - windows a step;
  - window size;
  - times through;
  - reading.
  
  They start from the AI line's shape as the setup menu has it.
- **Files** in reading order, as before: **Shift+Up/Down** moves one.
- **Live estimates beside the settings:**
  - weights, steps and threads;
  - memory, in red past half this computer's;
  - time, from a training step actually measured on your computer as you change the settings;
  - the result's size.
  
  The measurement runs on a thread of its own, so even a large shape's minute never holds the
  window up.

## The learning rate follows the shape
- **The sweep:** I ran it on the shipped text. The best rate halves each time the width doubles:
  - 0.0005 at width 16;
  - 0.0002 at width 32;
  - 0.0001 at width 64;
  - 0.00005 at width 128.
  
  Layers made little difference. With Micro's rate, width 128 got worse than guessing (9.2 bits a
  byte).
- **The suggestion:** the harness and `sieve ai-train` now suggest 0.008 ÷ width, scaled to the
  bytes a step reads, unless you set a rate.
- **Micro is unchanged:** its suggestion is still 0.0005, so the shipped run is unchanged.
- **At width 128 with the suggested rate,** the model learns: it writes German-looking bytes
  rather than noise.

## What larger models take: measured here
At 8 layers of width 1024:
- 134 million weights;
- a step measured at 41.8 s, so about 29 hours for the shipped run's 2,475 steps;
- 7.7 GB of memory;
- a 129 MB result.

The suggested learning rate there is near the smallest step `training-v1` can hold (1/65536), and
the page says so. That is where `training-v2` comes in. IDEAS §15 lists what's left for larger
models:
- threads within a window;
- resuming a run;
- more precision;
- the result's size;
- the AI line's own limits;
- the shelves' cost.

## Checked here (Linux)
- **CI's training steps, all run here:**
  - a single file refused as a corpus;
  - the folder runs as before;
  - in the harness, the width changed by keys (16 to 20), with the learning rate following to
    0.0002.
- **The earlier hallway steps,** run again.
- **Screenshots** of the settings:
  - at width 28;
  - at 8 layers of width 1024, with the memory, the time, the result and the floor note.
- **Builds:** the tool and the hallway, with no warnings.
- **Not checked:** I have not built on Windows. The folder window is the system's own, so its look
  there is Windows'.

## Docs
- **SPECIFICATIONS §12.0e:** recording takes a folder; the suggested rate is only a default.
- **README:** the harness's new-run page; `ai-train` takes a folder; the learning-rate default.
- **IDEAS §15:** larger models, with the measurements and what they still need.
- **HANDOFF:** a new entry.
