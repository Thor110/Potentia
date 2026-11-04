# Manual Navigation: the steering readout (4 November 2026)

Copy `Sieve/` over the repository's. This zip also holds the undo/redo marks fix from the last one,
so it's fine whichever you apply. `manual-navigation-steering.png` beside this file is only to look
at: your walk's start, C0 25 F7 63, with the steering line and the hollow target mark.

## What it shows

A fourth line under the readout, for the byte you've chosen, always there (not a hover):

> steering byte 0: one turn moves 14.0076 gaps; nearest the centre 54 turn(s) up, at F6 (about 0.0034 from landing)

- **One turn moves N gaps:** how far a turn of this byte moves the file, counted in the gaps between
  bearings. The whole gaps leave the dot where it was; only what's left over (here 0.0076) moves
  it. That's your curve: a beat, so the dot spirals slowly in towards a bearing and then out again.
- **Nearest the centre:** which of this byte's 256 values puts the dot nearest the centre, and how
  many turns up or down from where it is.
- **A hollow mark on the circle** shows where that value would put the dot, so you can drive to it.
- When the byte is already at its best, the line says so.

Each byte is a gear of its own. On your file the second byte moves 0.0547 of a gap a turn, and the
third 0.0002; far into a big file a turn moves too little to show, and the line says "under 0.0001".

## Large files

How far a turn moves depends only on which byte it is, the file's length and the precision, not on
the bytes. So it's worked out once each time you choose another byte, not on every turn, and the
best value is then picked from 256 at once. It runs on the worker, so the screen never waits; moving
quickly along the bytes works out only the last one you stop on. On a 1 MiB file, a whole run (load
it, choose a byte, steer, draw) took 0.13 s here.

## Checks

- CI step 10 replays your walk: from C0 it says 54 turns up, at F6, 0.0034. After driving those 54
  turns, the readout shows 0.0034 and the line says the byte is at its best. The Manual Navigation
  block passes here.
- As before, the mouse wants a try in the hallway itself.
