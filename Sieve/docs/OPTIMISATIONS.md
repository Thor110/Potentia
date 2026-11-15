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

Made and gone, the same day: 4, a recording kept as its text, each frame's line appended as it is taken
(a `Recording`, client/replay.hpp), where every event was kept whole. Thirty minutes at 60 frames a second
with a 1,000 Hz mouse (17 moves a frame) held 451.9 MB that way and 49.8 MB now (30.2 MB of text), the
same text either way; a recording made live in the hallway reads back, replays and is written again the
same, byte for byte, as CI step 10's recordings are. Packing each time the hallway is left stays.

Measured and gone, the same day: 7, the trial mixers on a large file. enwik8 (100,000,000 bytes) packed by
the build as it is and by one whose trial history touches every page as the vector did before review 1:
peak 3,020 MiB and 3,018 MiB, 317 s and 336 s, the same `.sieve` (24,768,406 bytes). The untouched pages
save nothing at the peak there: the peak is set later, by the whole file's mixing, not by the trials.
Nothing to change; the calloc stays (it costs nothing and helps where the trials are the peak).

Nothing is left on this list. A cost found by a later review goes here as before.
