# Menus in one order, Go to it from the main menu, and notes

## First: the last commit went in the wrong place
Your "harness" commit added the zip's `files/` folder and `harness-pictures.zip` at the top of the
repository (`files/Sieve/...`), not over `Sieve/`. So that round's changes aren't in `Sieve/`
yet.

This package carries every file that differs from your main, the harness round included.
1. Copy the contents of this zip's `files/` (its `Sieve` folder) over the repository's `Sieve`.
2. Delete the top-level `files/` folder and `harness-pictures.zip` from the repository.

`main-menu.png` and `pause-menu.png` are screenshots for you, not for the repository.

## Menus: the same systems in the same order (IDEAS §19.18)
- **Main menu:** Start Sieve, File Locator, Media Player (new), Filter Designer, AI Training
  Harness, Settings, Exit Sieve.
- **Pause menu:** Resume, Restart, Node Graph Viewer, Navigation System, then File Locator, Media
  Player, Filter Designer (new), AI Training Harness (new), Settings, Exit Sieve.
  - The node graph and the navigator stay pause-only: they need a hallway walked.
  - The panel is as tall as its rows.
- **Filter Designer and AI Training Harness from the pause menu** open over the hallway and come
  back to it, still paused. The harness's GO TO IT goes to its model on the AI line, as from the
  main menu.
- **Media Player from the main menu:**
  - It's the hallway's own (its Go to it walks to a melody), so the hallway is built with the
    player open over it.
  - Esc there goes back to the main menu.
  - Going to a melody leaves you in the world.

## The main menu's File Locator has Go to it again
- **Go to it** builds the hallway with the setup menu's settings and walks to the file.
- **A file longer than the binary line** still asks a second time, as in the world, because the
  line is then made long enough for it.
- **CI** now expects "open, Go to it offered".

## The harness round (from harness-pictures.zip)
Picture models in the AI Training Harness, `tools/cli/diffusion_run.{hpp,cpp}`, and IDEAS §19.14
and §19.15. See that zip's notes.

## Notes (IDEAS, new)
- **§19.16 Predict the data and give it a short tag.** A prior made explicit (the guided ordering
  is one). Unlike a stack it reorders and never removes. All of these are ways to put what's
  sought nearer the front.
- **§19.17 A diffusion model of the Vitruvian Man.**
  - The anchor as the reference, shown beside what the model makes, like universalbibliothek.
  - Crops and shifts of it as the training set.
  - Higher resolution needs tiles or more positions.
- **§19.18 Every menu reaches every system, in one order.** Your reasoning, and what's built.
- **§19.19 A music filter designer inside the Media Player:** the designer's nodes on melodies,
  heard as they're tested.

## CI
- The five main-menu scripts that open Settings now press Down five times: Settings is sixth.

## Checked here (Linux)
- The client builds without warnings.
- Both menus checked by screenshot.
- The scripted Settings path still lands on the same screens.
- The main menu's File Locator offers Go to it.
- Unit tests: 152,668 checks, 0 failures (no core changes since the last run).
- Not checked: the interactive paths (main menu to the Media Player, Go to it from the locator,
  the pause menu's designer and harness). They can't be driven by the screenshot scripts, so
  those want a try in your build.

## New files (if not already in Sieve/)
- `Sieve/tools/cli/diffusion_run.hpp`
- `Sieve/tools/cli/diffusion_run.cpp`
