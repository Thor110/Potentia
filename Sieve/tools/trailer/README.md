# The trailer

A film of Potentia : Sieve made from the programs themselves: about a quarter of an hour, in eight
chapters, each opening on a title card.

```
python tools/trailer/trailer.py --hallway BUILD/hallway --sieve BUILD/sieve --out FOLDER
```

It writes `FOLDER/potentia-sieve-trailer.mp4` (1920×1080, 30 frames a second), and
`FOLDER/music.txt` (which melodies play, by their rank among their stack's survivors). Its work, every
segment and still, is kept in `FOLDER/work`. Run it again with `--only NAME,...` to remake only those
segments, and `--skip-music` for a silent film. It needs ffmpeg on the PATH, Python 3, Pillow and numpy.

## How each part is made

- **Moving shots** are recordings of the hallway (`sieve-replay-v1`, the format F12 records), written
  by hand by `walker.py`: keys held, mouse moves and keys pressed, frame by frame, at 30 a second. The
  walker keeps the camera where the hallway would (3 m/s walking along `(sin yaw, 0, cos yaw)`, doors at
  z 6.4–7.6 of each 8 m tile), so a walk goes through a door where one is. The hallway plays each one
  back with `--replay FILE --replay-video OUT.mp4`, drawing every frame and handing it to ffmpeg.
- **Stills** are pictures the hallway takes of its screens (`stills.py`, the options CI gives them),
  held a few seconds each with a slow push in.
- **Cards and captions** (`cards.py`) are drawn in Sieve's own font, `data/fonts/sieve8x8.hex`, in
  each line's colours.
- **The music** (`music.py`) is melodies of the audio line, chosen as the hallway's music player
  chooses them: a rank drawn at random below the count of its default stack's survivors (WORLD under
  the walks, MENUS under the cards and tools), each read with `sieve read --survivor`, and played with
  the player's voices, tempo and echo.

Every run gets its own settings and filters files, so nothing of the person running it (their
character, their dimension, their filters) is drawn, and the film is the same for everyone.

## The figures it quotes

Chapter 5's shares come from `docs/IDEAS.md` and CI step 04; chapter 6's table from
`Proof/STATISTICS.md`; the line sizes in chapter 3 from `metrics/builds`. The help pages packed,
installed and refused in chapter 6 are measured by the run itself.
