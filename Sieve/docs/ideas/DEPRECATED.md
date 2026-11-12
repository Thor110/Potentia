# Ideas deprecated

Entries of [IDEAS.md](../IDEAS.md) that were set aside: unworkable (they cannot do what they hoped, for the reason given, most often the counting argument of IDEAS §0) or not wanted (possible, but Edward or a measurement said no). They are moved here whole, with their measurements and reasons, so the questions keep their answers and nothing is measured again unknowingly. IDEAS.md keeps a one-line pointer at each old number.

Nothing here is forbidden. An idea comes back when something changes what it rests on: a new measurement, a new version, or Edward's word; it then returns to IDEAS.md with a note of why. Ideas that were unworkable only in one of their parts stay in IDEAS.md with the rest; [IDEAS-TRIAGE.md](../IDEAS-TRIAGE.md) lists every unworkable idea, moved or not.

The first move, 12 November 2026: 19 entries, among them all of IDEAS §17 ("Deprecated ideas"). The colour-blindness pass (2.4) was kept in IDEAS.md although once set aside, since the tactile Sieve (§19.140) makes it a first step again. The section headings are IDEAS.md's own.

### 1. The reducer

**1.5 What it is not.** Kolmogorov complexity itself is uncomputable — there is no procedure that
finds the shortest program, only ones that find short programs. And a minimal program reproduces
*one* bitstring; there is no free resolution dial attached to it, because changing a resolution
parameter changes the output, which changes the address, which makes it a different object. See
§3.1 for the version of that idea that might actually be true.

### 3. Open questions

**3.6 Angles as a reduction: why not, and what is kept.** Edward, 2 October 2026. The proposal:
limit the line to a slice of the circle (say 330 to 331 degrees), map the slice to a full 360
degrees, and repeat, each time cutting the line to 1/360 of its size, so that a file's address
would come out 1/360 as long; with a byte for the angle (0 to 180, the values 181 to 255 kept for
other meanings). The arithmetic says no. An angle is the leading part of the address: a slice of
1/360 of the line takes log2(360), about 8.5 bits, off what is left to say, and the angle costs
those 8.5 bits to say (a byte of 181 values says 7.5 for 8). A 32-byte file is 256 bits whole,
8 + 247.5 with one angle, and 80 + 171 after ten; at best even, never shorter. Dividing the rooms
by 360 shortens an address by 2.56 decimal digits, because an address's length is the number's
order of magnitude. Repeated, the angles are the address written in base 360: one to one and
reversible, so it no longer looks like the data, but it is the data in another form (no smaller,
and no secret). The filters are different in kind, not degree: a stack is one rule everyone holds,
so what it removes is never sent, while the slice a file is in depends on the file and must go
with every address. The two meet in compact mode, where the circle is made of survivors only.
What is kept: the bearing as a way to choose where to look (built: the navigator's bearing field,
Tab in X, at the Angle Precision); a range view over one arc of a line; angle, room and item as a
readable notation of an address, with a leading tag for the notation's version (the 181..255
idea, as a format tag); and progressive orderings (§3.1), the version of this in which narrowing
the angle would mean refining the content. And, measured rather than assumed (Edward, the same
day: a file could land exactly on 90.10 degrees): COST's **variable length addressing**
(corridor.hpp `shortest_path`) finds, for the unit in hand, the shortest route there is: the
position without leading zeros, or a bearing of up to 20 places then a walk forward or back. It
is a minimum over the routes, not a scheme for every unit: averaged over a line it cannot beat
the address (pigeonhole), but it names the lucky units (a bearing's own landing, the line's
start and end) at what they really cost.

### 4. Manifests, installs and anchors

**4.6 A signed manifest, if tamper-checking is wanted.** Ordinary Merkle tree plus a signature.
Says nothing about the generative layer, works perfectly well, and is the only thing that would
actually stop a modified file passing (see §0).
*Decided (Edward, 25 October 2026): no signing* (§19.92: "I don't trust myself or any human being
alive to keep a key safe"). Links that state hashes, a site's table and the crawler stand in for it.

### 7. Features from earlier in the conversation, still standing

**7.8 Ambient audio sampled from where you stand.** Warp the current tile's position into the audio
line and pass it through `key-v1` so the result is in a valid key. Continuous in-key ambient from
your actual coordinate, out of three things that already exist. The cheapest real win in the list.

- *Edward's answer (6 November 2026): "I think the music is probably fine as it is for now. It
  already has scales per dimension." So nothing below is built; it stays here, described, in case
  it is wanted later.*
- *Described in full (6 November 2026, for Edward to decide on before it is built).* What exists
  already (client/music.hpp): WORLD's music is drawn from the survivors of WORLD's filter stack (the
  key filter among them), at random, a rank drawn below their count and unranked; each line moves
  the notes into its own mode (Lydian on the brightest, Locrian on the darkest); a melody plays to
  its end, then the gap's quiet, then the next. What this adds is only *which* survivor: the one
  belonging to the place you stand in, instead of a random one.
  - *What you hear.* Every stretch of corridor has a melody of its own. Stand in a tile and its
    melody plays; walk on and the next one is the new tile's; come back tomorrow, with the same
    settings, and that tile plays the same melody again. The music becomes part of the place, like
    the books on its shelves: a tile can be remembered by its tune, and a tune found again by
    walking back to it. Nothing about the sound changes (tempo, voice, echo, modes, key and filters
    are as they are now); only the choosing.
  - *A place.* The line you are on and your tile in its loop: `(t mod loop tiles)`, the same number
    that puts unit `(t mod loop) * 128 + k` in slot k (hallway.hpp). By tile and not by book, so a
    melody belongs to a stretch about as long as it takes to walk past one at a stroll, and does not
    change at every step. Each line's loop repeats, so its tunes repeat with it, exactly where its
    books do.
  - *From a place to a melody (the cheap form).* SHA-256 of a fixed label (`sieve-music-place-v1`),
    the line's id and the tile's number, as its 256 bits (and as many more of the same chain as the
    count needs, 64 above its length, so the rank is as even as the random draw's): that number
    modulo the count of survivors is the rank, and the rank unranked is the melody. It is the code
    there now with the random generator swapped for the place's bits; a stack that only judges
    draws its tries from the same chain, in order, so it is just as fixed. A withheld melody (the
    vault) moves on to the next number of the chain, as a random draw moves on now. The same tile
    on another line plays something else, and then in that line's mode as well.
  - *Neighbouring places alike (a second form, to be heard before it is offered).* Instead of a
    hash, the line's own seed plus the tile number as the rank, so tile t+1 plays the survivor
    after tile t's: a walk along the corridor is a walk along the stack's own order of melodies.
    What consecutive survivors have in common depends on how the stack orders them (a key filter's
    count, a length's, a range's), so whether this sounds like variations on a theme or like
    nothing at all has to be listened to first. The cheap form scatters, and is the one to build.
  - *When it changes.* Never mid-melody: when one ends, after the gap, the next is the tile you are
    in *then*. Running past twenty tiles during one melody plays only the tile you stop in;
    standing still repeats your tile's melody after each gap (or, a setting, plays it once and then
    falls back to the random draw until you move). A jump with the navigator is the same: the
    melody in hand finishes, the next one is where you landed.
  - *Settings.* One more choice in WORLD's music settings, "Chosen by: chance / where you stand",
    saved in sieve-music.ini as the others are. MENUS has no place and keeps chance. The tracks
    source works the same way, the place choosing a track of N units instead of one unit.
  - *Showing it.* The overlay says where a melody came from ("tile 4,096 of BINARY"); the recent
    list keeps the place with each melody, so a melody you liked can be walked back to (a jump to
    the tile it names), and its address, MIDI and favourites stay as they are.
  - *What changes the tune of a place.* Anything that changes the count: a filter ticked or
    unticked, the melody length, the note set, the tracks source. That is the honest consequence of
    choosing by rank among survivors: the place keeps its number, and the number lands on a
    different melody when the shelf of melodies changes. Worth saying in the setting's help ("the
    same place plays the same melody while the music settings stay the same").
  - *What it is not.* Not the book's contents turned into sound (sonifying a file's bytes, which
    would be a new filter or a new line, not this); not a soundtrack written for the hallway. Each
    melody is still one unit of the audio line with an address, played because of where you are.
  - *Cost and checks.* One SHA-256 and the unrank done now, per melody; nothing heavier. Checked
    by a harness test that one place gives one melody twice, that two tiles and two lines give
    different ones, and that a withheld melody is passed over the same way each time. About an
    hour for the cheap form, as the list of easy wins has it; the second form, half as much again
    plus the listening.

### 17. Deprecated ideas

**17.1 A rotation transform, and numbers that hold only their decimal places (Edward, 7 October
2026; deprecated).** The proposal: a file at 31.55591717178 degrees is named as "the rules, the line turned by -0.55591717178
degrees, and 31": a global setting that turns the circle so that the file lands on a whole number.
The turn has to travel with the address, and it is the same digits: 0.55591717178 is what the
decimals carried before, so "31 after a turn of -0.55591717178" is as long as "31.55591717178",
plus the cost of saying a turn follows. A turn agreed once for every file (a setting of the line,
not of the file) costs nothing to send, but it is a fixed shuffle of the circle, like the
scrambled ordering's key: it moves every file the same way, so for every file it brings onto a
whole degree it takes another off (pigeonhole), and on average names nothing shorter. It is §3.6
again from the other side: what an angle saves in the address it spends in the angle.
The variant raised with it, **a number that stores only its decimal places**, has the same
answer. Leaving out the whole degrees (31 of 360, log2(360) = 8.5 bits) works only where the
reader already knows them; otherwise they are part of the address. "Pay only for the precision you
need" is already how bearings work here: COST's variable length addressing writes a bearing to as
few places as land near the unit (up to 20) and the walk carries the rest, leading zeros dropped.
(IEEE 754's "hidden bit", the leading 1 of a normalised number left unstored, saves exactly one
bit, because it is always 1.) Edward had hoped for a byte at least; it is one bit at most, and on
an address nothing, so it is not worth a format of its own.

**17.2 CHAT on every line (Edward, 17 October 2026; deprecated for now).** The proposal (§19.80):
the CHAT tab on the item page of every line, the item in hand given to whichever model is loaded,
as a file is given now with the plus button. Edward has cancelled it for now: he is not sure it
fits. CHAT stays where it is, on the AI line, where the item in hand is the model itself; a file
from any line can still be given to it there (the plus button, Ctrl+O). The rest of §19.80 stands:
a tab with a use belongs on every line, as HELP now is.

**17.3 Size and decoding time as one setting (Edward, 20 October 2026; deprecated for now).** The
proposal (§19.89): a File Locator box that optimises a set of `.sieve` files for size and decoding
time together, as one parameter, for use as a format for the internet; the ways differ in the time
they take to undo (an item made again from its line, the rope's columns, LZMA2, a file carried as
it is), so it would choose, file by file, the smallest way whose decoding stays within a time.
Edward set it aside: as he recalls, decoding takes fractions of a second, while what takes seconds
is loading and working in the hallway itself, which this would not touch; so there is little to
trade. In the far future, he says, the deprecated ideas can be looked back over, to see whether any
still stand out as worthwhile. (Not measured here: what each way takes to decode.)

**17.4 Square and circle wave filters (Edward, 27 October 2026; measured, not taken).** The
proposal, beside the rope and the triangle: lay a file's bytes out W wide and H high, draw a
square or a circle through them, and record the bytes the shape passes through to rebuild the rest
from (the shape and the grid's width and height reconstructing that part of the data), searched as
the rope's ways are. Two modes, both tried: only the shape's vertices kept (a square's four corners,
a circle's 4 or 8 compass points, the outline drawn between them, as the rope draws straight
between its kept values), and every byte on the outline kept. Inside, each row (and a square's
columns) filled between where it meets the outline; outside a circle, the nearest outline value
along the row. Tried over the whole grid and over tiles (squares of 8 to 64, and rectangles of 4 to
64 by 4 to 64), the best widths of each file, the rope's phases on top or not, the kept bytes costed
a byte each and the rest as the rope costs a plane (prototype in Python, the rope's own cost model;
the real rope's code beside it, 20 s of search). Bytes:

| File | The phases alone | Best shape | The rope today |
| :--- | ---: | ---: | ---: |
| radial (made up: smooth, plus a little noise) | 21,165 | 18,989 (square, vertices, 16 by 16 tiles, no phases) | 19,085 |
| gradient (made up: smooth) | 7,235 | 7,117 (square, vertices, 32 high by 16 wide) | 5,539 |
| the Vitruvian picture (`tests/rope`) | 44,422 | 46,079 (circle, vertices) | 44,729 |
| the README, first 64 KB | 38,857 | 50,390 (square, vertices) | 39,028 |
| `data/filters/moby-pos-v1.tsv`, first 128 KB | 73,730 | 99,639 (circle, vertices) | 48,571 |
| the Utah teapot (public domain) | 124,260 | 145,275 (circle, vertices) | 75,237 |

Why not: the shapes win only on a smooth field with noise in it (the made-up radial, where a
baseline from corners 16 apart ignores the noise that each neighbour's prediction carries), and
there the rope's contexts already come within about 0.5%. On the real picture the neighbours (the
triangle) predict better; on text and models a drawn shape has nothing to follow. Every byte of
the outline kept was worse than its vertices alone everywhere, and a circle never beat a square:
the best was always a lattice of kept corners with straight lines between, which is interpolation,
not a new kind of wave. Edward: "Pythagoras be damned, the triangle would be the best". Worth
looking at again only for files that are smooth and noisy (scans, sensor grids, some scientific
data), and then as one more way in the rope's search. The prototype (`shapes.py`) was not kept in
the repository; it is short, and this paragraph says what it did.

**17.5 `source.sieve` as a map holding the source's 7z (27 October 2026; replaced the same day).**
The release's `source.sieve` was a sealed map naming `sieve.exe` and `sieve.sieve` and holding
`sieve-source.7z`, so it was never smaller than the 7z (Edward's: 49,240,740 bytes at v29 and v30).
v31 tried to keep it so and carry the 7z's files inside (the 7z made again exactly on installing),
but that holds only where Sieve's LZMA SDK makes the 7z again byte for byte, which it did not on
Edward's machine (49,244,484 bytes). Edward had meant a plain `.sieve` of the source folder, and
that is what the release makes now (47,299,016 bytes on his machine; without the anchors folder,
6,185,486 against the 7z's 8,006,568); the 7z is published beside it as the baseline. Not done
instead: a map whose held files may have paths (a map holding a folder), a format change for
something the plain `.sieve` already gives.

**17.6 Smaller `sieve-install` (27 October 2026; looked at, not pursued).** It grew to 4,471,320
bytes when it became the full installer (24 October, d6e4d85, linking `sieve_lines` and
`sieve_compare`, so it installs everything Sieve can make): the remake compressors (brotli, zstd's encoder and the rest) about 2.07 MB, SDL
1.19 MB, the reader about 0.72 MB, items about 0.56 MB, libarchive about 0.5 MB (Linux, measured
by function sections). Edward: "It probably isn't worth it then."

**17.7 Smaller decisions of 27 October 2026, kept with their reasons.**
- *Every `.sieve` stamped with the newest version:* not taken. A file names the oldest version
  that holds what it uses (the degree sample is v20 whatever made it), so any Sieve from that
  version on reads it; stamping the newest would only make older readers refuse files they can
  read.
- *Groups of 16 MiB for files mixed together:* not taken for 8 MiB (v30). On Sieve's data folder
  16 MiB was no smaller (3,099,798 against 3,100,643) and installed in 28.6 s against 14.1 s, a
  group decoding on one thread; on 34 MB of the repository's texts 16 MiB was 4% smaller. The size
  is the packer's alone, so it can move without a new version.
- *Groups made by kind of file rather than the listing's order:* 0.4% smaller, not worth a way to
  say it.
- *Whether a compressed format is worth unpacking, judged by a sample mixed alone:* 1 MiB in 8
  stretches, mixed, put the source at 10,990,184 bytes against LZMA2's 7,992,234, since a sample
  misses what repeats across the whole. Kept instead: mixing's gain over LZMA2 on the sample,
  applied to LZMA2's estimate of the whole (6,702,006; it came to 6,189,674).
- *"Welcome to the sieve" as a setting:* not needed for compression. It is only where the hallway
  starts (0% along the pages line because its own address is small); nothing a search or the
  packer does refers to it.
- *The anchors folder left out of the release's source:* Edward: "Not a bother to be honest, I
  think it is fine as it is for now."

**17.8 Page scripts in the browser, checked or held back (27 October 2026; set aside for
PotentiaOS).** What is true now: a script is a file like any other, so its bytes are checked
already (`sieve site build` gives every `<script src>` its `integrity`, which the browser enforces,
and the site's table holds each script's hash; an inline script is part of its page's hash). What
cannot be done is to run them inside the checked viewer, which runs no scripts (so a page with one,
the test site's gallery among them, is kept plain), or to keep a guarded site's page from starting
its scripts before the extension's check ends. Two ways were proposed, neither tried:
- *Checked scripts in the viewer.* The decoded page in a sandboxed frame (no origin of its own, no
  reach into the extension or the site's cookies), with a Content-Security-Policy whose
  `script-src` names only the SHA-256 of each script the `.sieve` holds, and every script it loads
  taken from the same `.sieve`, as its pictures are. A self-contained interactive page could then be
  a `.sieve` and still run. Its limits: no logging in, no cookies, no calls to its server. Chrome
  would need trying first, since an extension page's own policy may refuse inline scripts in a
  frame it makes (a sandboxed extension page with its own policy is the likely way round).
- *The page held back until checked, in Firefox.* Firefox still lets an extension hold a page's
  response as it arrives (`webRequest.filterResponseData`): hash the whole of it against the table
  and pass it on only if it matches, so a changed page's scripts never start. Chrome took that
  from ordinary extensions in Manifest V3, so there the gap stays. It would need a Firefox build of
  the extension and a host registration Firefox finds (the install scripts register Chrome,
  Chromium and Edge only).
What neither reaches, and so waits for PotentiaOS: pages a server writes as it serves them, scripts
that fetch whatever they like as they run, and holding everything until it is checked in every
browser. (Chrome's Isolated Web Apps package a whole site with every file checked, but they rest
on signing keys, which the web part of Sieve does without.) Edward: "Honestly I think we shouldnt
bother with either at the moment. I still think we should as a society be looking to compile web
pages down into real code. So perhaps it may all have to wait on a PotentiaOS."

### 19. Edward's ideas of 12 October 2026

**19.53 The plan, in order of importance (13 October 2026).** What to build next, and why in this
order. *Superseded:* the plans in HANDOFF (24 and 27 October 2026) are the current ones; this is
kept as the plan of its day.
1. **The Encryption Engine, stage 1** (§19.52): asked for next. It also teaches the
   format-preserving view of every line, which the rest builds on.
2. **Metrics for every build, charted** (§19.50): cheap, and it makes progress visible on the
   website and in the world from now on, so it is worth starting early.
3. **Models steered by the filters** (§19.37): DRAW and CHAIN making survivors only, the clearest
   demonstration that rules and models are one system.
4. **The documentation inside the game** (§19.48): /docs, with the documents on the lines, so the
   game explains itself, and models can be pointed at the right section.
5. **The completion line on the main menu, from one source with the website** (§19.49): the
   numbers specified once, checked by CI.
6. **Chains as graphs in the node graph** (§19.46, §19.38): branches, and chains, maps and models
   in one catalogue.
7. **Rules as units:** the Life-like line, the first step of the engine (§19.47).
8. **Research before building:** decompilation (§19.51), the voice (§19.33), the code line
   (§19.36).

Throughout: exact arithmetic for anything addressed; versioned rules that are never edited; the
oracle written from the specification; measurements before claims; and the counting argument kept
in view.

**19.89 ~~Sieve instructions for the internet: size and decoding time as one setting~~ (Edward, 20
October 2026; deprecated for now, §17.3).** Another box: optimise for file size and decoding time
together, as one parameter, to make a set of `.sieve` files fit to serve as a format for the
internet. Set aside the same day: Edward recalls decoding as taking fractions of a second, so
there is little time to trade for size.

**19.91 A file's opposite found in noise (Edward, 21 October 2026; an answer, nothing built).** His
thought: there might be an inverse of a file, found amongst noise, that cancels it to nothing, so
that the noise could be generated to pull the file back. Filters tailored to a file, he adds,
should stay dynamic.
- *The opposite always exists:* for any file, the noise that cancels it under XOR is the file
  itself (under subtraction, its negative). So the question is never whether it exists, only what
  it costs to name it.
- *Naming it costs what the file costs:* noise from a generator is named by its seed. There are
  256^n files of n bytes and fewer than 256^n seeds shorter than n bytes, so for almost every file
  the seed that makes its opposite is as long as the file. This is the same counting that holds
  every compressor to its floor.
- *Cancelling part of it:* noise that cancels most of a file leaves a remainder, and the seed plus
  the remainder is never less than the file's entropy. Where the noise is chosen well the remainder
  is small, and that is what a prediction is.
- *Where the idea pays, and stays dynamic:* noise generated from what the reader has already
  unpacked costs nothing to name, because the reader can make it again. That is how `rope-wave`
  works: its prediction of each byte, from the bytes before and above it, is the "noise", and only
  the remainder is written. A filter tailored this way adapts to each file without being fitted to
  any one file, so better generators of that kind are the route the idea points to.
- *A side note:* XOR with noise that only one side can make is a one-time pad, so the same idea is
  the Encryption Engine's rather than compression's when the seed is kept secret.

**19.94 Distance along the line and rotation together, and grids of any size (Edward, 22 October
2026; measured, nothing built).** His thought: "we already track percentage along the line, but
we are using rotation as the primary search space for the shorter addresses. We could be
analysing the distance along the line 0-100 and the rotation in degrees 0-359 in order to
increase our chances of landing on a whole integer or higher rounding for a shorter address.
Still unlikely we will ever find a whole integer for a file of any real meaning, but it sort of
doubles our chances. In theory we could use a variable measuring system as well, perhaps
line/1-65535."
- *What it is:* a file's place written as the nearest point of a grid laid along the line (N
  equal steps), which point (k, below N), and the offset from it to the file.
- *Percent and degrees together:* the two grids share every point at a multiple of 5% (18°), 20
  of them, so together they have 440 points, not 720: about 1.2 times the degrees' 360, and saying
  which grid costs a bit.
- *Measured (Claude):* each file's place among the files of its length, every grid from N = 1 to
  65,535 tried and the best kept, paying 16 bits for N, k's bits and the offset's with its sign;
  against the file's own address (8 bits a byte):

  | File | Bytes | Best grid | Percent or degrees |
  |---|---|---|---|
  | "hi" | 2 | +10 bits | 0 |
  | "Sieve" | 5 | +1 | +1 |
  | the README's first 64 bytes | 64 | 0 | +1 |
  | IDEAS.md's first 1,024 bytes | 1,024 | +1 | +1 |
  | 2,048 bytes of noise | 2,048 | +1 | 0 |
  | the test fixture's `a.txt` | 34 | −1 | +1 |

- *Why (the counting argument, §16):* a grid of N points lands within about 1/N of the line of a
  file, so the offset is log2 N bits shorter, and naming the point costs log2 N bits. Choosing the
  best of 65,535 grids gains at most about 16 bits, which is what naming the grid costs. Every
  file above is within a bit or so of even, as that predicts.
- *Where it can win:* a grid taken from the file itself, as the rope's wave is (a prediction
  from its own values, §19.87), not a ruler laid over the line. *Verdict: not built;* noted so the
  measurement is not repeated.

**19.127 Rounding a file's place, with a table of common roundings (Edward, 7 November 2026; measured
8 November).** His idea: round a place to something short and keep what the rounding loses in a
lookup table, or tables of tables, of common roundings shipped as a setting; or round with
high-entropy noise. *Measured* (results/rope-roundings-2026-11-08.md): what a rounding would have to
restore is the rope's leftovers, and across files they repeat only as silence and runs of small
values. A table of 16-byte chunks saves 1.3 to 1.9% of three WAVs before its own size (about 200 MB)
is charged; shorter chunks lose; noise never hits. The same structure taken without a table, each
leftover byte coded given the one before, saves 2.2 to 2.7%. Verdict: no table of roundings; a rope
version coding by the byte before is worth building (a new format version).
*Looked at again (12 November 2026), before building it:* the files it was measured on are no longer
the rope's. The packer now carries `mr` (9,970,564 bytes, 1,967,657 packed), `x-ray` (8,474,240 to
3,614,957) and `sao` (7,251,944 to 4,149,437) by mixing as pictures of their own shape (`mix4 ...
I512x2`, `I1901x2`, `I12264x1`), and v34's draft carries WAVs as FLAC. So an order-1 rope (and the
trim of §19.131) would change what Sieve writes almost nowhere: not built.

**19.128 Noise run through a file, its matches clipped out (Edward, 8 November 2026; measured the
same day).** His idea: noise from a seed propagated through the file, the bytes it matches counted,
every seed tried (and the noise propagated again), the most matches kept; the matching bytes clipped
out and put back when decoding. *Measured* (`tools/bench/noise_match.py`): on 65,536 bytes of a WAV
(WoWRevived's Music-Martian/03), a seed matches 256 bytes on average; the best of 65,536 seeds
matched 341. Clipping them saves 2,728 bits; saying where they were costs 3,078 bits (the matched
places as a bitmap, at its entropy) and 16 for the seed: 366 bits lost. On random bytes the same:
the best seed matched 336, and 368 bits were lost. Why: noise matches a byte one time in 256 whatever
the file is, so which bytes it matched is itself as much to write down as the bytes; more seeds find
a few more matches, about as many as the seed's own bits pay for. Verdict: it cannot gain on any
file; what the rope does instead is predict the bytes from the file's own neighbours, where a match
is likely, not one in 256.
*More seeds* (Edward, 8 November 2026; `tools/bench/noise_seeds.c`, the best seed at every power of
two, its saving under the block's own byte table, its places as a bitmap at its entropy, and its own
bits). On 4,096 bytes of Music-Martian/03, up to 2^28 (268 million) seeds: the best matched 30 bytes
at 2^8 seeds and 42 at 2^28, and lost 33 to 79 bits at every count. On 4,096 random bytes, 26 to 44
matches, 25 to 32 bits lost. On 65,536 bytes up to 2^22 seeds: the WAV lost 891 to 1,055 bits,
random bytes 370 to 374. Each doubling of the seeds finds about as many extra matches as its extra
bit of seed pays for, so the loss stays where it is however many seeds are tried. (The rope itself
runs no noise: its `P` chooses the predictor.)

**19.131 Trimming what the wave predicts exactly (Edward, 8 November 2026; measured the same day).**
His idea: the values the wave predicts exactly are trimmed out and the file shrinks; decoding runs
the wave again in order and puts each back where it came from; passes may repeat over what is left,
by a second wave or by noise from a seed. To put them back the decoder needs a yes/no for every
value, so a trim costs those plus the kept leftovers. *Measured* (results/rope-trim-2026-11-08.md,
`tools/bench/trim_passes.py`, every variant costed by one adaptive coder): against the bands the WAVs'
recipe uses (`T1`), one trim, its yes/nos coded given the eight before and the band, saves 0.31 to
0.69% on three WAVs, 0.15% on x-ray and 4.36% on `mr` (a third of whose values the wave predicts
exactly, in runs). It adds to order-1 coding of the planes (against order 0: trim + order 1, 1.52 to
3.70% off the WAVs; order 1 alone 0.88 to 2.60%). A second wave over the kept values loses 5 to 7%
(they sit apart in the file), and a noise pass loses 27 to 116% (noise matches one 16-bit value in
65,536 and turns the kept leftovers into noise). The control gains nothing. By its cost the trim is
a flag per value coded in context, as JPEG-LS's run mode and the significance flags of H.264 and
HEVC are, so it is not new as a method. Verdict: one pass, with the order-1 rope if that is built;
no second or noise pass. Not built.
