# The system prompt, FULL SIZE, an even CHAT row, Buy or Build, and the notes

Against your main ("image levels"). Copy `Sieve/` over yours. This time there are two files at
the repository's root as well, `index.html` and `Pages/release.html` (new). Other new files:
- `Sieve/data/prompts/sieve-system-v1.txt`
- `Sieve/docs/SYSTEM-PROMPT.md`
- `Sieve/tools/check_vectors.py`
- `Sieve/tools/run_tests.bat`

## The full file and its variants: FULL SIZE
- **What you saw:** J on a picture card gives the small version, because it fits the picture to
  the image line's size. GO TO IT already gives the whole file: its exact bytes on the binary
  line, and its full address.
- **New: FULL SIZE** (or F) on a picture card (PNG, JPG, GIF, BMP). It opens the picture on an
  image line of its own width and height, in full colour (rgb24), so every pixel is the file's
  own. The hallway is built again with that line, as DRAW's GO TO IT does for another size.
- **Vitruvian.jpg:** 949 x 1300, 1,233,700 pixels, exactly as it looks (screenshot sent with the
  zip). A room full of pictures that size takes a while to fill: about a minute here, on worker
  threads while you walk.
- **The three, side by side:**
  - the file (GO TO IT);
  - its pixels at their own size (FULL SIZE);
  - fitted to any line (J), at any size and palette.

  IDEAS §19.40 sets out how they could be joined into one ladder of scales.

## CHAT's row, even
- **Heights:** the text box, +, DRAW and SEND now share their top and bottom edges. A box of more
  lines grows upwards from them.
- **The +** is drawn as two bars to the pixel, so it sits exactly in the middle of its square.
- **The rule:** HANDOFF's house style now keeps your description, "even and distinguished", for
  every panel to come.

## The system prompt: sieve-system-v1 (docs/SYSTEM-PROMPT.md)
- **What it is:** what a model given as a file is told about where it is and what it is, before
  you say anything. It is written as a specification, like the manifest:
  - pinned by its SHA-256, which the unit tests check;
  - never edited, so a change is v2, and v1 stays;
  - honest about what a model can and cannot do here.
- **What it says:**
  - *Its opening line,* made from your description: a near-infinite, procedurally generated
    landscape that in principle contains everything.
  - *Where they are:* lines, units, addresses, filters, stacks and survivors.
  - *Who they are and what they can do:* they cannot run commands yet, only suggest them.
  - *How to answer:* exactly, briefly, in the program's words.
  - *Then the place, now:* the line and its space, the ordering, the filters, the model in hand,
    and which model it is.
- **Where your words are:** quoted in full in SYSTEM-PROMPT.md and in GAME.md.
- **Seeing it:**
  - `/prompt` in CHAT shows exactly what the model is told;
  - `hallway --print-system-prompt` prints it;
  - CI checks the printed text against the file, byte for byte.
- **Small models:** a model with too few positions for it is told the old one-liner, and CHAT
  says so.
  - *Found here:* the tiny test model broke silently with the full prompt, which is why.
- **Shipped:** the build copies `data/prompts` next to the hallway, and `make_release.py`
  includes it.

## Testing on your side: tools\run_tests.bat
- **What it runs:**
  - the unit tests of every build in `out\build` (Debug, RelWithDebInfo, Release);
  - then, if Python is there, every conformance vector through the oracle
    (`tools\check_vectors.py`, the same 36 checks CI runs).
- **How it reports:** a summary line for each part.
- **Speed:** the oracle part takes about a quarter of an hour; `run_tests.bat quick` skips it.

## The web page: Buy or Build
- **`Pages/release.html`,** linked from the index as "Buy or Build (Free If You Build It)", in the
  same style as the other pages.
- **What is on it:**
  - the two names;
  - the three steps to build it;
  - your PayPal link to buy or support it;
  - the current release and the repository;
  - "When It Is Ready" as the release date.
- **The counters,** exact (BigInt), and absurd as asked:
  - every 10x10 black-and-white picture, one a second since the 24th of April 2026 (your audio
    page's epoch): finished in the year 4.0 × 10^22;
  - then every 32-character page: 2.0 × 10^38;
  - then every model of the AI line's default shape: 4.5 × 10^9856.
- **Change the epoch** in one line at the top of its script, if you'd rather count from another
  date.

## Notes (IDEAS §19.40–19.44, GAME.md, HANDOFF)
- **§19.40** A file, its full size, and its variants: one picture is a point on many lines, each a
  different scale.
- **§19.41** Asking ffmpeg's developers and others for their oddities: easter eggs, files and maps
  as anchors from the people who made them. Licences first, since the free and paid builds are
  the same game.
- **§19.42** The Buy or Build page.
- **§19.43** Your note on all the AIs folding in on themselves, in your words, with what it means
  here: π as the library, and "perplexity" being literally the measure of a model's surprise.
- **§19.44** Keeping the interface even.
- **GAME.md:** the one-sentence description, and "the philosophy is the lore".
- **HANDOFF:**
  - more of the testing on your side;
  - this conversation has not been handed off, and whoever continues keeps the same principles;
  - a new entry.

## Checked here
- **Unit tests:** 76,407 checks, 0 failures (once; no hardware SHA-256 here), including the
  prompt's pinned hash.
- **Every oracle vector:** 36 of 36 reproduced, through the new `check_vectors.py`.
- **CI lines:**
  - the CHAT lines, including the model file and the new prompt lines, on a clean copy of the
    rebuilt hallway;
  - FULL SIZE on Vitruvian.jpg, with a 949x1300 rgb24 line.
- **The web page:** rendered in headless Chromium at 1280 and 520 pixels wide. Chromium won't lay
  out narrower than 500, so a phone itself wasn't tried.
- **Not tried here:**
  - *`run_tests.bat`,* since there is no Windows here.
  - *FULL SIZE's rebuild of the hallway,* since scripted runs don't follow requests. It is the
    same path DRAW's GO TO IT already uses.
  - *A large model given the full prompt,* since there is none here. The tiny model shows the
    fallback.
