# Recording and replay (IDEAS 19.184), and the Replays viewer

## The recording format: `sieve-replay-v1` (SPECIFICATIONS §12.0q)

- It keeps where you began: the line, the tile, and the camera exactly.
- Then, for every frame, it keeps the frame's time, the keys held, and that frame's input events.
- Playback runs the same steps again, so a recording plays back exactly. Each frame's time is rounded to the microsecond as it's recorded so the steps match.

## In the hallway

- **F12** starts and stops a recording.
- Recordings go into `replays/` beside the settings file, packed as `.sieve` (as you asked), and are named by the date and time they began.
- A recording stops by itself and keeps what it has:
  - at **30 minutes** (`[replays] max_minutes`), or
  - when the free space where it's kept, checked every five seconds, falls below **256 MB** (`[replays] min_free_mb`) or twice the recording's size, whichever is more.

## Replays (main menu, after Character Creator)

- Lists every recording in `replays/`.
- **Enter** plays one in the hallway, then hands the hallway back to you.
- **Space** ticks its **cache** box: a packed recording is then decoded once into the cache (beside the filters' cache) instead of every time it's played. The box is kept by the file's SHA-256 in `[replays] cached`.

## From the command line

- `--record FILE`: a `.replay` file is plain text, `.sieve` is packed.
- `--replay FILE`
- `--replay-cache`
- With `--screenshot`, a recording is played through and the picture is taken where it ends.

## Limits

- A recording only plays where the hallway starts the same way it did when recorded (the same settings). Otherwise it's refused, saying where it began.
- Moving the hallway to any recording's start, free cameras and the film studio come next.

## Noted

- IDEAS §19.187 and ROADMAP step 5b: Personal Dimensions opening onto a world of its own, by default a grassy hill under a blue sky with clouds, drawn from a seed rather than copying the XP photograph.

## Checked

- CI 10 passed. A 132-frame walk (1,612 bytes as text, 256 packed) ends in the same place whether played as text, packed or cached.
- Played back while recording, it is written again byte for byte.
- A recording begun elsewhere is refused.
- The viewer's cache box is kept.

## Files

- New: `client/replay.cpp` and `client/replay.hpp`. Re-run CMake.
- No files to delete.
