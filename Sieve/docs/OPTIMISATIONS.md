# Optimisations still to make

Edward, 14 November 2026: "we should aim for every possible optimisation within reason, any gain is a gain
in the potential for the state space and tools itself".

A standing list of known costs that a review found real but left for now, each with what it costs (measured
or estimated, and said which), what a fix would need, and how to show the fix changes no output. The rule
for every entry is the reviews' rule: a fix keeps every picture, address, saved file, `.sieve` byte and report
the same, and is shown to by comparing before and after. An entry leaves this list when it is made (named
in the commit) or found to be gone.

Sorted by what each costs today, largest first. CI steps are those of `ci/steps/`.

## 1. A request on the RAID is read a byte at a time
- **Where:** `read_line` in `core/src/raid_net.cpp`, over `SocketStream::read` in `tools/cli/raid_socket.cpp`:
  one `recv` system call for each byte of every request and answer line.
- **Costs:** 17.1 µs a 74-byte line read so, against 0.6 µs with one `recv` (measured, a socket pair). A
  check of a shared save makes about 390 requests: about 6.5 ms of it on one machine (estimated from those
  figures), 1% or less across a network, where each request waits a round trip anyway.
- **Needs:** a read buffer of 4 to 64 KiB in the stream that client and server share: `read(n)` takes from
  the buffer first, large reads go straight to the caller, small ones refill it with one `recv`.
- **Shown the same:** share, check and fetch against local `raid serve` peers, before and after: the
  reports and the fetched file byte for byte; `strace -e recvfrom` counts the calls saved.
- **CI:** 04, 10.

## 2. Sharing asks each peer in turn
- **Where:** `place()` in `core/src/raid_net.cpp`: for each piece, `has` then `put` on each home peer,
  one after another.
- **Costs:** since the one-write fix (review 1), an 8 MiB share takes 0.14 s on one machine (measured); a
  share of 119 pieces that were all held, 0.083 s. Across a network each piece waits about two round trips
  per peer (not measured).
- **Needs:** send `has` for every piece to each peer in one write and read the answers in order (the server
  answers in turn), then run the placement on those answers, so refusals and the fall to the next peer
  happen in the same order as now. Threads per peer would change the order of the report's notes, so
  pipelining is the way that keeps the output.
- **Shown the same:** the share report (sent, had, bytes sent, short of, remote id, notes) and each peer's
  held store before and after, with a peer that refuses (`tests/test_core.cpp`, the RAID share tests).
- **CI:** 01, 04, 10.

## 3. The Character Creator draws a picture the size of the window
- **Where:** `render_char_drawing` in `client/menu.cpp`: `begin_ortho` with the whole window, so `MeshBatch`
  clears, draws and uploads every pixel for a character in a box.
- **Costs:** 2.3 ms a frame at 1920x1080 against 0.35 ms for a 432x500 box, 1.0 ms at 1280x720 (measured,
  20 shaded spheres, software renderer, 4 cores); paid only while the CHARACTER page or the Creator is open.
- **Needs:** `MeshBatch` to draw into an integer sub-rectangle of the window, every coordinate still in
  window space so each float comparison is unchanged, and the texture drawn to that rectangle. (Moving the
  centre into the box's own coordinates would round differently and move pixels at spheres' edges.)
- **Shown the same:** pixel for pixel, with an animated character crossing the box's edge.
- **CI:** 10, 04.

## 4. A recording keeps every input event whole
- **Where:** the recorder in `client/app_main.cpp` (`frame.events.push_back(e)`) and `ReplayFrame` in
  `client/replay.hpp`: each event as a 128-byte `SDL_Event`, for up to 30 minutes, and the whole recording
  packed again each time the hallway is left while recording.
- **Costs:** up to about 220 MB with a 1,000 Hz mouse for the full 30 minutes (estimated, not measured);
  only while you record (F12).
- **Needs:** a redesign of the replay type: frames kept as their text lines (already built every frame to
  count bytes) or a compact struct, the text appended as it goes. Playback reads the same type, so it moves
  with it. Packing only when the recording stops would leave the file on disk behind after a menu visit or a
  crash, so that part stays.
- **Shown the same:** a recording made, saved and played back, before and after: the saved text and the
  `.sieve` byte for byte, and the replay ending where it did.
- **CI:** 10.

## 5. Edges are walked once for each band
- **Where:** `raster_band` in `client/mesh.cpp`: each edge touching a band is walked along its whole length
  and the points outside the band thrown away; there are as many bands as threads.
- **Costs:** 0.1 ms of CPU a frame for 300 edges up to 300 px, 0.45 ms for 1,000 edges up to 400 px (measured,
  one thread, all bands); spread over the bands, under 0.1 ms a frame in any scene seen so far.
- **Needs:** each edge's first and last step inside the band worked out from its slope, one step of margin
  either side, with the same per-step formula and the same in-band test, so the pixels written are the same.
- **Shown the same:** per-band pixel buffers against the current loop over random edges (the review's bench
  did so), then the hallway's and your dimension's pictures.
- **CI:** 10, 04.

## 6. A piece kept for another machine is written under one lock
- **Where:** `RaidHeld::put` in `core/src/raid_net.cpp`: the lock is held across checking the space, writing
  the piece and rewriting the peers file.
- **Costs:** under a millisecond a put at 1 Gbit/s (worked out, not measured); only several sharers putting
  to the same folder at once would wait on one another.
- **Needs:** more than moving the write out of the lock: every write goes to the piece's name plus `.part`
  and is renamed over it, so two writers of the same piece would spoil each other's file. It needs a temporary
  name of each writer's own, the space reserved under the lock and given back if the write fails, and a
  decision about how often the peers file is written (what may be lost on a crash).
- **Shown the same:** a held folder after many puts from several sharers at once, against one sharer after
  another: the same pieces and counts.
- **CI:** 04, 10.

## 7. To measure: the trial mixers on a large file
- Review 1 made the trial mixers' history come as untouched zero pages, which should save one block the size
  of the file during the trial of any file over 4 MiB. At 6.7 MB and 10 MB the peak memory moved by 0 and
  10 MB (measured); a file of 100 MB or more would show it. Not a change to make: a measurement to take.
