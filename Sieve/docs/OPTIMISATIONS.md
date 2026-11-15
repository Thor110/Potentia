# Optimisations still to make

Edward, 14 November 2026: "we should aim for every possible optimisation within reason, any gain is a gain
in the potential for the state space and tools itself".

A standing list of known costs that a review found real but left for now, each with what it costs (measured
or estimated, and said which), what a fix would need, and how to show the fix changes no output. The rule
for every entry is the reviews' rule: a fix keeps every picture, address, saved file, `.sieve` byte and report
the same, and is shown to by comparing before and after. An entry leaves this list when it is made (named
in the commit) or found to be gone.

Sorted by what each costs today, largest first. CI steps are those of `ci/steps/`.

Each keeps the number it was given (REVIEW-ROUND's list O names them so). Made and gone from here: 1, 2 and
6, the RAID's reads buffered, its `has` questions asked together, and a held piece written outside the
lock (15 November 2026): the share, check and fetch reports, every held folder and its peers file the same
byte for byte before and after, four sharers at once included; a check of 35 pieces on 3 peers made 485
`recvfrom` calls and now 14, and asks each peer once per 256 pieces rather than once a piece.
Made and gone, the same day: 3 and 5, the Character Creator's picture drawn and uploaded only where its
character reaches, and each edge walked only through its band's rows. The texture is still shown whole:
showing only the rectangle put pixels a pixel off where the menu is scaled to its window. The same pictures
pixel for pixel: a character in its box, across its edge and off the window (a 20-sphere scene, 1920 x 1080:
3.7 ms a frame with the window's clear, now 2.6 ms; the rest is the renderer laying the whole texture
over the window), random edges in 1, 3, 7 and 8 bands (331,776,000 pixels, none different; 1,000 edges up
to 400 px, 1.3 to 1.4 ms a frame on one thread, now 0.87 ms), and the screenshots of CI step 10.

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

## 7. To measure: the trial mixers on a large file
- Review 1 made the trial mixers' history come as untouched zero pages, which should save one block the size
  of the file during the trial of any file over 4 MiB. At 6.7 MB and 10 MB the peak memory moved by 0 and
  10 MB (measured); a file of 100 MB or more would show it. Not a change to make: a measurement to take.
