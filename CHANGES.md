# A sound model (melodies), CHAT commands, and five notes

Against your main ("model files"). Copy `Sieve/` over yours. There are no new source files. There
is one new folder, `Sieve/data/training/melodies-2x64` (diffusion.ini, pictures.txt, result.txt):
the shipped melody model's run.

## A melody model, made the way the picture model is
- **diffusion-v1 on the audio line** (tools/cli/diffusion_run, tools/cli/diffuse.cpp):
  - A melody is a picture one cell high: `length` cells, each one of the note set's symbols
    (notes104 has 104). The model, the noise, the training and the making are unchanged.
  - The recipe says `line = audio`, `symbols` (the note set) and `length`. A recipe without
    `line` is a picture run, so every existing run reads as before.
  - `pictures.txt` writes a cell as a base-36 digit, as before, or, where there are more than 36
    symbols, as a number, with the numbers apart by spaces.
  - `sieve diffuse --record DIR --line audio` draws its melodies from the `[audio]` stack in the
    filters file and trains. Naming the run trains it again and says "reproduced exactly", and
    the oracle (`sieve_ref.py diffuse-replay`) trains it to the same result.txt.
  - `--make` prints each melody's notation and its address on the audio line.
- **`--judge FILTERS`** (with `--make`): instead of each unit, it counts how many of those made
  each filter keeps, and how many of the noises they were made from: a model measured against
  noise, by the filters themselves.
- **The shipped model, melodies-2x64:**
  - 2 layers of width 64 (163,840 weights), 30,000 steps, on 4,000 melodies of 16 notes104
    events. The melodies were drawn from key, ending, leaps, metre and rests, which together keep
    1.5 × 10^-14 of all melodies.
  - Of 300 melodies it makes (three noise seeds):

    | filter | made | noise |
    |---|---|---|
    | in key | 300 | none |
    | ends on the tonic | 300 | about 35 |
    | leaps | 59 to 85 | none |
    | metre | 14 to 20 | 0 to 2 |
    | rests | 230 to 255 | 276 to 294 |
    | the whole stack | 3 to 5 | none |

  - Rests is the one rule it keeps less often than noise does. The text says so.
  - Trained again here: every checkpoint and the final model match its result.txt, byte for
    byte.

## Melodies in the harness and in CHAT
- **The harness** lists melody runs as it lists picture runs, and shows the melodies made in
  notation.
- **CHAT:** with a melody model chosen for DRAW (Ctrl+P, or the setup menu's row), DRAW gives a
  melody card:
  - a piano roll, the melody's address on the audio line, and its notation for the model to read;
  - **PLAY** plays it;
  - **GO TO IT** holds it on the audio line;
  - **SAVE** writes a MIDI file.

## Commands in CHAT (for you, not yet for the models)
- **A line that starts with `/`** is a command and goes to no model (this works with `--talk` too).
- **`/random picture`** (or `image`) and **`/random melody`** bring a random unit as a card, with
  the same buttons.
  - Where the line's ticked filters can be counted, it is a random survivor of them, drawn by its
    rank.
  - Otherwise it is any unit at all, and the card's title says which.
- **`/help`** lists the commands; an unknown command says so.

## IDEAS
- **§19.29 Reconstruction, and probes that send the means to rebuild.** It is how cosmology
  already works: constrained simulations search for the initial conditions that match the
  snapshots. The trade is compute for bandwidth: the probe sends little, and the receiving end
  spends supercomputer time. Data in distorted forms is a search over the agreed decoders, judged
  by filters. What was never sent cannot be rebuilt.
- **§19.30 Every dimension in flavours, and categories of categories.**
  - *Flavours already exist:* the audio line is notes104, notes2, notes3 or pcm.
  - *A category is a stack,* so categories of categories are stacks of stacks, a tree the maps
    can hold.
  - *The noise category* (encryption, good compression) is what fails every structure filter.
    Recognising it takes a key, a header, or a generator run backwards.
  - *Kinds of model* are named by the line whose units they model.
- **§19.31 A sound model: melodies first.** What was built (above). Avoiding ComfyUI where
  Sieve's own models serve. Tracks come next (melodies joined); sound itself (pcm) needs a model
  of another shape.
- **§19.32 Commands in CHAT.** The person's commands are built; the models' use of them waits for
  models that follow formats, and for a rule of what a model may do without asking.

- **§19.33 A voice made the way a voice is made** (your engine-simulator idea, for the voice).
  - *What it is called:* articulatory synthesis. The vocal folds (the two-mass model) feed a tube
    of changing width (the Kelly–Lochbaum vocal tract), shaped by the tongue, jaw, lips and velum.
  - *Why it suits Sieve:*
    - a voice's address would be a dozen controls every 10 ms, not 44,100 samples a second;
    - run in fixed point, it is exact, so the controls name the sound;
    - "could a body have made this?" is a filter;
    - a model that writes control tracks is the shape §19.31 said sound was waiting for.
  - *The reverse is J for speech:* finding the controls that make a recording.
  - *Built small first:* vowels from a tube, then consonants, then folds, then a voice line, then
    a model.

## Opening Vitruvian.jpg on the image line: why it is white (found, not changed)
- **It is located correctly.** The white picture is the right unit for the rule the image line
  uses (`canon-image-v1`), and ffmpeg would not change it.
- **What the rule does:** it averages each cell's area, then takes the nearest palette colour. On
  a 10x10 mono line each cell is a 95x130 patch of parchment crossed by a few thin lines. Its
  average is light, and light is nearer to white. At 32x32 it is still all white.
- **What would help:** a rule that judges each cell against the picture's own light and dark,
  rather than against pure black and white. Tried here on Vitruvian.jpg:
  - at 64x64, the figure, its limbs, the circle and the square come back;
  - at 10x10, no rule can show a Vitruvian man in 100 cells.
- **Why I haven't built it yet:** it would be a second rule, `canon-image-v2`. As the default, it
  would change the address of every photo warped onto the image line; as an option, the white
  picture stays until it is chosen. That choice is yours (see my message). Picture files the
  hallway saved itself would keep their addresses either way, because they are already exact
  palette colours.

## Other changes
- **SPECIFICATIONS §12.0g:** melodies, the run format, and the measurements.
  - *A correction:* the figures model was said to make 29 of 300 pictures that are one shape clear
    of the edges. `--judge` gives 22, 24 and 27 over three noise seeds, and the text now says so.
- **help:** `sieve help diffuse` documents `--line` and `--judge`.
- **README:** the AI line's paragraph and the `diffuse` section.
- **HANDOFF:** a new entry, including the image finding.
- **data/lang/en.txt:** the melody, PLAY and command strings. DRAW's strings no longer say
  "figure" where the model may draw a melody.

## Checked here
- **Unit tests:** 76,334 checks, 0 failures (once, as this machine has no hardware SHA-256).
- **New CI lines:**
  - a tiny melody run recorded, trained again and replayed by the oracle;
  - `--make` and `--judge` on it;
  - the shipped model's first three melodies, by address;
  - in CHAT, the melody card, `/random melody`, `/random picture`, and GO TO IT landing on AUDIO.
- **A slip of mine, caught before handing over:** the CHAT melody line wrote its transcript to
  `chatmel` while the next two checks read `chatmel.txt`. Here they passed only because an older
  `chatmel.txt` was lying in my folder. On CI they would have failed. It now writes
  `chatmel.txt`, and I ran the three checks again, on their own, against a fresh transcript:
  they pass.
- **One existing CI line changed, because of the new run:** last round's setup-menu line pressed
  Right once on an empty picture model and expected figures-2x32. An empty setting means the
  first trained run (figures-2x32), so with melodies-2x64 beside it, Right now steps to
  melodies-2x64. The menu is right and the line was not. The line now presses Right (and expects
  melodies-2x64), then Left (and expects figures-2x32), which checks both directions. The figures
  stay DRAW's default, so the earlier figure lines are unchanged.
- **The whole hallway CI step,** run again on this build in a clean copy, with real copies of the
  binaries as CI has them: every line passes. Its mesh lines need cube.obj, which an earlier step
  writes; I made it the same way and ran those lines too.
- **Screenshots** (sent with the zip, not in it): a melody card in CHAT, and the harness showing melody runs.
