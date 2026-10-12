# The chat window, with figures

Against your main ("diffusion"). Copy `Sieve/` over yours; there is one new file,
`Sieve/client/chat.cpp`, which is already in CMakeLists.txt.

## The window (IDEAS §19.13)
- **Opening it:** take a model of the AI line in hand and press Enter. It replaces the old `SAY > `
  prompt bar.
- **Layout:**
  - the conversation scrolls above;
  - the box to type in is below (Enter sends, Shift+Enter starts a new line);
  - the title names the model in hand and its shape.
- **Moving about:** Page Up, Page Down or the wheel scroll the conversation; Esc closes the window.
- **The model is unchanged:** it answers as before, and its answers are seeded the same way. The
  hand panel still shows the conversation.

## Files (the + button, or Ctrl+O)
- **Choosing:** the system's file dialog, with several files at once.
- **Each file's card shows:**
  - its kind and size;
  - the ways to name it, weighed as the File Locator weighs them: its address on the binary line,
    and its place on another line where it is exactly an item of one, with the shortest way marked.
- **What the model reads:** the file's first bytes, up to half its 512 positions. When there isn't
  room, it starts again and says so.
- **The card's buttons:**
  - GO TO IT walks to the file on the binary line;
  - J opens it on the line of its kind, as J does for a file in hand;
  - SAVE writes it out.
- **The vault** refuses as it does everywhere else.

## Figures (DRAW, or Ctrl+D): the multimodal part
- **Drawing:** DRAW makes a figure with a picture model: any trained diffusion run in the harness's
  folders, with figures-2x32 first. Ctrl+P, or clicking the line under the title, chooses another
  when there are several.
- **The card shows:**
  - the picture;
  - its size and palette;
  - its address on the image line.
- **What the model reads:** the figure as its rows, a character a cell, so it reads a picture as it
  reads a file.
- **The card's buttons:**
  - GO TO IT holds the figure on the image line. If your image line has another size or palette, a
    hallway is built with one that fits, as the Media Player does for melodies.
  - SAVE writes a PNG.
- **Naming:** a figure is named by its model and its number ("figure 3 by figures-2x32"). That is
  always the same picture: the one the harness shows third, and the one `sieve diffuse --make`
  makes third. One function now makes them for all three (`nth_picture` in diffusion_run).
- **Keys for the newest card:** Ctrl+G, Ctrl+J and Ctrl+S press its buttons.

## Notes
- **IDEAS §19.13:** marked as built, with what remains: the model naming places in its replies,
  and published models in the window.
- **IDEAS §19.20 (new):** your text game that makes itself and grows to 3D, as a record of
  parameters with the spectrum as one of them. GAME.md §4a points to it.
- **README:** the AI line's paragraph describes the window.
- **HANDOFF:** a new entry for the window.

## Scripted runs and CI
- **New options:** `--chat`, `--chat-attach FILE`, `--chat-draw N` and `--chat-press KEYS`, listed
  in `hallway --help`.
- **New CI checks:**
  - a file given, two figures drawn (pictures 1 and 2, by address) and a message sent;
  - Ctrl+G on a figure holds it on IMAGE;
  - Ctrl+J on a text file opens it on PAGES.

## Checked here
- **Unit tests:** 76,335 checks, 0 failures. This machine has no hardware SHA-256, so the suite ran
  once rather than twice. Your machine should show about twice that.
- **CI lines run here:**
  - the new chat lines;
  - the AI line's `--talk`;
  - the harness reproducing its shipped run;
  - the six figures-2x32 addresses from `sieve diffuse --make`.
