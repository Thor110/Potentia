# Changes

One commit on top of 824f089 ("sieve N").

## Filters (REVIEW-ROUND list N5; IDEAS 19.253)
- **Typed settings:** in the filters window, digits type a whole-number setting (shown with a `_`), Backspace takes
  the last digit off, Left/Right still step it. Nothing new on the main or pause menus.
- **degree-window-v2:** a window wider than what is left of the loop goes on from 0 (`from` 350, `width` 20 keeps
  350 to 10). v1, which stopped at 360, is retired (still loads). Two windows where one crosses 0 judge only and say
  so.
- **Sound that compacts:** `sound-envelope-v1` and `sound-syllables-v1` now count every survivor at any length. A
  default second of 8-bit sound compacts with them (10^-19.05 and 10^-466.83 kept), and their numbers are the
  automaton's wherever both fit (checked).
- **quotes-v1:** quotation marks where prose puts them. 2,256 of Don Quixote's 2,318 pages of 1,000 pass; random
  ascii96 pages kept at 10^-3.97.
- **Video on a real film:** the Roundhay Garden Scene cut into 20 clips; at the line's default size most clips do
  not move at all (still pictures, set aside by design); every clip that moves in black and white passes
  video-smooth-v2.
- The `.obj` page filter was written and removed: `not-other-line-v1` already sets such pages aside.
- **Windows over more stacks:** under words-v2 real pages sit where survivors are 2.9 to 39 times an even spread,
  so every window width loses (1.3 to 5.0 bits); the stronger the filters, the more a window loses.
- Set aside with reasons (REVIEW-ROUND N5-7, N5-9): row-runs and rows-alike counted together on large palettes,
  and smooth as an automaton.

## J and the File Locator
- **J on a model:** a file that reads as a model (.obj, .gltf, .glb) opens on the models line, fitted to its shape;
  the window says what was left out and the scale (the teapot: 1,857 vertices and 3,552 faces fitted to 8 and 12).
- **File Locator:** Choose a file... opens a file without sieving it (its address at once); Sieve a file... reads
  and packs it, as Choose a file... did; Go to it walks to the .sieve on the binary line where the file has been
  sieved, and to the file as it is where it has not. The window says which.
- For scripts: `--locate-open PATH` opens a file as Choose a file... does; `--locate PATH` still sieves it, as Sieve a
  file... does (CI step 10 checks both walks).

## Checked
CI steps 01, 03, 04, 07 and 10 on the filters; 01 and 10 again with J and the File Locator; help and library
checks.

## Files to delete by hand
None.
