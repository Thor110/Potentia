# Ideas

A triage of the long Gemini conversation — all three parts — plus anything else that has come up
and is not yet built. It is not a plan and not a promise. Each item says what it is and whether it
is a defect, a feature, an open question, or framing.

Nothing here is a commitment. Within each section the order is roughly by how much it is worth.

**First, when work turns to the game (GAME.md; Edward, 6 October 2026): the colour-blindness pass (2.4).**

---

## 0. One correction first, because a lot depends on it

A large part of the third conversation is built on the idea that an address is much smaller than
the thing it addresses — a "genesis key" for a 50 GB install, archive.org backed up to one key.
That part is not true, and it is worth being exact about why, because what is left after the
correction is still good.

**In a bijection, the address is the content.** There are 256^N files of N bytes. Any scheme that
gives each one its own address needs at least 8N bits of address. In Sieve's positional ordering
this is not even an inequality: the address *is* the file's bytes read as one integer. Scrambled
ordering permutes those bytes and changes nothing. So the key for a 50 GB folder is 50 GB.

**Splitting the address into room and item saves nothing.** `index = room × 256 + item` costs
`log2(rooms) + 8` bits. Fixing the item to one byte takes 8 bits off the item and puts 8 bits onto
the room. The total is invariant. All the size is in the room number and always was.

**But the idea works if you reduce first.** The saving does not come from addressing; it comes
from *compression*, and then addressing costs nothing extra on top. Address the short program that
generates a file rather than the file, and the key is as small as that program is. This is exactly
what the guided ordering already does on the pages line: a guided book record is about 1.8 bits per
character, so it really is smaller than the text it encodes, while the same book positionally
addressed is not. The honest statement is:

> A key is as small as the content is compressible, and no smaller. Sieve's contribution is not
> that the key is small — it is that the key is *canonical*: one name per artefact, derived from
> the artefact, identical for everyone who has it, with no registry to consult.

That is worth having on its own. It gives deduplication for free (§4.2), it makes an install
reproducible and checkable, and for content Sieve *generates* — a mesh, a page, a melody — the
address genuinely is the whole thing and there is no file to ship at all. That part is already
built and is the real version of the idea.

Two smaller corrections in the same family:

- **A bijective address is not a tamper seal.** Anyone who changes a file can compute the changed
  file's address just as cheaply and rewrite the manifest. Authenticity needs a *signature over*
  the manifest, and then it is an ordinary signed Merkle tree — which is a fine thing to build, but
  it gets nothing from the generative layer.
- **Generating is slower than reading.** Unranking N bytes of bignum is strictly more work than
  reading N bytes off an NVMe drive. "Installation becomes compute-bound rather than a storage
  wait" is a cost, not a feature — unless the reduction in §1 has made the thing being transported
  much smaller, which is the whole point again.

---

## 1. The reducer

The strongest new thread in the third conversation, and it stands on its own whether or not
anything else is built. Take a file and find the shortest program that reproduces it exactly.

**1.1 It is grammar induction, and it has a literature.** What is described — find repeating byte
strings, replace them with rules, then rescan the remainder — is *grammar-based compression*
(Sequitur, Re-Pair). It infers a context-free grammar whose start symbol expands to the file, so
repeats inside repeats become recursive rules rather than a flat dictionary. Start there rather
than from scratch.

**1.2 Use a rolling hash and a suffix array.** Comparing every block against every block is not
O(1); it is quadratic at best. Rabin–Karp rolling hashes make the sliding window O(1) per byte, and
a suffix array or suffix automaton gives *all* maximal repeats in one pass instead of a descending
max-to-min loop. That puts the whole pass at O(N log N).

**1.3 A bit-level pass after the byte-level one.** Retro formats pack 5-bit colour channels and
3-bit flags across byte boundaries, so byte-aligned scanning structurally cannot see those
patterns. Worth having for the very files this would be pointed at first.

**1.4 Round-trip before committing.** Every candidate reduction is executed and bit-compared
against the original before the original is discarded; any mismatch throws the rule away. This is
where the Python oracle earns its keep a second time.

**1.5 What it is not.** Kolmogorov complexity itself is uncomputable — there is no procedure that
finds the shortest program, only ones that find short programs. And a minimal program reproduces
*one* bitstring; there is no free resolution dial attached to it, because changing a resolution
parameter changes the output, which changes the address, which makes it a different object. See
§3.1 for the version of that idea that might actually be true.

---

## 2. Defects and corrections

**2.1 Entropy is measured against the wrong space.** The filters score a unit against a fixed
symbol count, but the alphabet is user-chosen and runs from 27 symbols to the whole of Unicode. A
page drawn from `lower27` judged against a 21-bit code point space gives a meaningless reading.
Every alphabet needs its own entropy baseline. A correctness bug, not a feature.

**2.2 Padding is counted as content.** `model-information` and `symbol-entropy` count trailing
padding spaces as real characters, so a padded page scores as *more* text-like than its text alone.

**2.3 The cased dictionary lets short gibberish through.** Folding one cased list to lower case
also folds SCOWL's abbreviations and chemical symbols, so "zn" and "tb" now pass. Fix: a version
that keeps proper names and drops short all-caps tokens.

**2.4 No colour-blindness pass.** Nine lines, filter verdicts and the map all encode meaning in
hue, with no non-colour fallback and no simulation check. First when work turns to the game (above). The pair most
at risk: audio's dark green and video's dark red (and so tracks and movies, which share them),
which differ mostly in hue, the difference red–green colour blindness loses.

**2.5 Layouts are built for 8-pixel text.** Unifont draws at half size, so CJK and much of Cyrillic
are unreadable in the menus. Supporting them means reworking the layouts first, not adding fonts.

---

## 3. Open questions

**3.1 Progressive ordering — the best unanswered question in the whole conversation.** Can a line
be ordered so that *truncating* an address gives a legitimate coarser version of the same item? For
images that is what progressive JPEG and SPIHT do; for meshes it is what subdivision surfaces do.
If it can be done exactly, then one address carries every level of detail, "LOD from a key" becomes
true rather than wishful, and a slow machine can stop reading early and still get something real.
If it cannot, then a low-poly variant is simply different content with a different address and the
LOD idea is dead. Nobody asked this question in 700 turns and it is the one with teeth.

**3.2 Exact counting under a model.** *The small one is built: `word-cost-v1` (FILTER-PLUGINS
§19), a letter-pair model in quarter-bits, budgeted per stretch of a word, not per page.* Counting every page whose information under the order-5 model
stays inside a bit budget needs a DP over (context × bits spent × position) and is infeasible at
page length. Two honest routes: lean on guided ordering and do not count, or build a filter over a
*deliberately tiny* model — letter pairs, costs rounded to whole eighth-bits — that is exactly
countable for short pages. Worth doing the small one to see the shape.

**3.3 The two-metric view.** Plot candidates on two independent axes — entropy against structure
density — rather than thresholding either alone. Uniform noise collapses into a dense predictable
cluster; isolated structure sits off the curve. This is the buildable version of "one dimension has
no angles, so cross it over itself", and it is a scoring mode over rankers that already exist.

**3.4 Calibrate by shrinking the space.** Tune the stack against a large space, then shrink it until
every survivor can be read by hand. The survivors that are merely well-organised gibberish are
exactly where the heuristics leak. A test protocol, not a metaphor.

**3.5 Mapping barren ranges.** Exhaustively search the surviving fraction for ranges that are
provably empty and store them as a skip list. The safety rule is the important half: never exclude
a range without having exhaustively searched it, because a false exclusion erases a real work
permanently.

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

---

## 4. Manifests, installs and anchors

What survives §0 — and most of it survives.

**4.1 A directory walker that emits a manifest.** Point the CLI at a folder; it walks the tree,
records the structure, filenames and sizes, and gets each file's key. Then it takes the key of the
manifest itself. Buildable now, useful now, and it is the honest core of the genesis-key idea:
one canonical, self-verifying description of a tree. Add total size to the manifest so an install
can refuse before it starts.

**4.2 Deduplication comes free.** Identical files land on the same key, so a tree never stores the
same bytes twice. This is content-addressed storage — which is real and worth having — and it is
the part of the OS story that actually works.

**4.3 The first release as the first anchor.** Build the release, walk it, emit the manifest, tag
every key `sieve-build-release-v1`. The CLI reproduces its own release build from its own manifest
as a post-build step, which is both a conformance test and the first entry in the archive. This is
a good idea and it does not depend on the key being small — it depends on the key being canonical,
which it is.

**4.4 Chunking, and why it matters twice.** Split a large file into fixed-size pieces, each its own
unit on its own line. It makes generation multi-threadable, it makes a big file tractable on a
small machine, and it is the only way an install can proceed in pieces rather than all at once.

**4.5 Real file types out of the CLI.** Already partly there: `read --out` writes `.png` for image
and video, `.mid` for audio, `.txt` for text, and `mesh --out` writes `.obj`. Worth finishing —
`.wav`, and a frame sequence rather than frames side by side — so any address can be handed to any
other program.

**4.6 A signed manifest, if tamper-checking is wanted.** Ordinary Merkle tree plus a signature.
Says nothing about the generative layer, works perfectly well, and is the only thing that would
actually stop a modified file passing (see §0).

**4.7 The CLI already is a server backend.** The 3D client only asks the core what is at a position;
nothing about that is graphical. Turning it into a service is multi-threading and transport, not
architecture. The wireframe mode means it stays usable on a terminal in a server room.

**4.9 The locator, the manifest, and a comparison suite.** *The first two are built* (`sieve
locate`, SPECIFICATIONS §12.2). Edward's plan, 27 September 2026: the manifest installer maker gets
a COST-like page, run after locating a file or making a manifest, that sets the original size
beside zip's and 7z's and the address's, as shares of the original (`--compare` does this now).
The address will almost always be 100%: that is the point of showing it. The suite is meant to
grow into many views of the same information: a **Kolmogorov complexity analyser** among them,
which strips repeated byte runs and then slides a window over every length, largest first, to find
repeats. That is LZ77-style matching plus grammar-based compression (Re-Pair, Sequitur), which is
§1's reducer. It gives an upper bound on the complexity, as every compressor does. The exact
limit is not computable by any program, so the analyser's honest output is a set of upper bounds
from different methods side by side, and the smallest of them.

A later release could ship the locator and manifest creator as a stand-alone tool as well, as
sieve-install already is: a small window over `tools/cli/locate.*` and `compare.*`, without the
hallway (Edward, 27 September 2026).

**4.8 What the first release manifest holds, and the node graph's first anchor.** Edward, 27
September 2026. The release manifest lists the executables and the Real Graphics model templates
(`data/meshes/templates/*.obj`, the item models), since they ship in the first installer. The CLI
executable is the anchor the node graph is first viewed from: its bytes are one unit of the binary
line, placed on the binary shelf. An executable differs between platforms and usually between two
builds of the same source, so the anchor is the exact released file, named by its SHA-256 in the
manifest, one per platform release (or the builds are made reproducible). It is where the graph
starts, not a claim that the bytes are universal.

---

## 5. The hallway's shape

**5.1 One wall of shelves, everywhere.** The binary line is one-sided now, and the argument for
doing the same to the others is good: there is only one address space, and a double-sided
corridor draws it twice. Open question to settle: what the empty side becomes — blank wall, the
portals, or something else. Worth trying on one line before committing.

**5.2 Rings instead of runs.** *The readout half is built.* Every line already loops, so where you
stand in one is a bearing: the compass in the corner draws the corridor as concentric circles —
binary outermost, the six it bounds, binary again innermost — with a needle at your angle on the
line you are on, a mark on every other ring at its own angle, and the bearing written out to
whatever precision **Angle Precision** in the setup menu's GLOBAL section asks for. Zero is at the top, where
every loop starts and finishes.

What is *not* done is the world itself: the corridor is still a straight run. Turning it into an
actual circle would drop the double start line (a circle needs no telling where it began) and is
the larger half of this.

**5.3 Signs above the portals.** Neon-style lettering in the destination line's colour above each
doorway. Small, and it fixes real navigation confusion.

**5.4 The warp animation.** On a warp: face the shelf, slide along three bookcases, through the
portal, along three more, and stop facing the item. Turn the world around the player rather than
snapping the camera — that is the standard fix for transition disorientation and costs nothing.
It also hides the moment the shelf changes side.

**5.5 Accelerating scroll.** The wheel already moves by tiles; give it an acceleration curve so a
fast spin compresses distance, which is the only way browsing stays possible once the fork count is
unbounded.

**5.6 Shimmer on the item under the crosshair.** Small, and it makes the interactable thing obvious.

**5.7 Metadata and links on an item in hand.** When a unit has been catalogued, show what is known
about it and let its links be followed — a warp at the touch of a hyperlink. This is the feature
that turns the hallway from a viewer into an archive, and it needs §4.1 first.

**5.8a The node graph, first version: built** (27 September 2026, SPECIFICATIONS §12.3). Maps
of verified anchors, one at a time, in 3D, and the item page's SORT tab. Edward's notes on where
it goes next: maps shipped with a release or offered as optional downloads ("if you want map X,
you get it"); relations beyond `contains` (versions, patches); nodes for items on the other lines;
and anchors feeding the filters, so verified real content helps build heuristics that find
meaningful content. A graph of the whole state space was considered and left out: it would be a
tangle of noise, at the cost of a new limit in the setup menu. It can be added later if wanted.
On directions like "anchor X, 1,000 rooms right, item 128": they cost as many bits as the
distance, so they only reach near an anchor (in positional order, files that differ from it only
in their last few bytes). Edges that are patches between versions are where a map saves real
data.

**5.8 The toroidal map.** A rotatable 3D overlay of the whole library: each line a ring, shaded
with its own two colours, anchors as points, links as edges, the binary line as the inner and outer
boundary. Needs 5.2 first, and needs there to be anchors to plot.

**5.9 Telescope view.** Interpolate from first person out to an orbit of the structure; coarse
movement covers distance, fine movement tweaks the low digits; at range the shelves collapse into
density bands. A navigation UI for addresses that already exist.

**5.12 A junction is a line, not a room.** The unsolved navigation problem is a hallway that forks
a million ways: you cannot draw a million doors, and a list is not a place. But an ordered set of
categories is exactly what every line already is, so a junction should be a *ring* — a round room
whose wall carries the doors, which rotates past you as you scroll, with the compass showing where
you are in it and how far round it goes. A million forks then costs nothing to stand in, because
only the dozen doors near your angle are ever drawn, and finding one is the same act as finding a
book: warp to its number, or filter the ring down and walk what survives. It needs no new
machinery — a loop, a position and an angle are all built.

**5.10 A scene description, so people can lay the gallery out themselves.** JSON plus `.obj`:
which lines exist, where their rings go, what furniture stands between them. Everything the backend
knows, rendered wherever someone wants it. Large, and the right shape for it is not obvious yet.

**5.11 A vetting gate for anything user-placed.** If people can leave objects in the space, a human
has to approve them, and there needs to be a hard cap per person. Decide this before the feature,
not after.

**5.12 The address navigator.** *Built (X in the hallway, `client/navigator.cpp`).* Walking and
the jump keys only ever reach the right-hand end of an address. The navigator shows the whole of
it, a hex digit at a time, full screen and scrolling; any digit can be chosen with the keys or the
pointer and turned with Up/Down, the wheel or its arrows, carrying as counting does and wrapping
round the line. Possible next steps: a preview of the item at the address being turned, and a
decimal view beside the hex.

**5.13 Filters on titled lines scan bottom-up.** Edward, 27 September 2026. A titled unit reads
top-down as cover, title, content, and that is the order its address is packed in (the content
varies fastest, so neighbours share a cover and a title). Filtering goes the other way: bottom-up,
content first, then the title, then the cover. The content is where most of the information is
and where most units fail, so judging it first rejects them soonest; and it is the part that
changes between neighbours, so a compact line over titled units will be built from the content
outwards, the title and cover stacks applied to what the content stack lets through. The books
line already has three stacks (cover, title, pages); when titled lines get filters, they follow
this order, and the books line should be brought into it too.

---

## 6. Limits and settings

**6.1–6.4 Limits and reset.** *Built.* `FIND MY LIMITS` in the setup menu sets every line to the
largest shape this machine can open; `RESET EVERY SHAPE` puts them all back. Two things bound a
shape and both are checked: the length of one address, which is a single number held in memory and
whose arithmetic grows with the square of its length (a 256th of installed memory), and the length
of one *unit*, because the hallway caches a few thousand whole units and that is usually what runs
a machine out of memory first (a quarter of installed memory across four thousand of them). Both
are closed-form from the shape, so the search generates nothing. A red line names any line that is
over, rather than a general refusal. Pages and image grow in step, because the books line is made
of both and has to fit too.

Neither number is a limit of the design. They describe the machine of the day, and a bigger one
finds bigger numbers with the same arithmetic.

**6.5 Global parameters, and entries per shelf.** *Built.* Items per wall is a setting (a power of
two from 2 to 4096, default 128), under GLOBAL in the setup menu. It turned out not to need
versioning: it changes no address, only the (tile, slot) coordinate an address is shown at, so
"tile N" is a display coordinate tied to the setting, and the address is what gets written down.

**6.6 Adaptive word width.** Let the machinery move between 8/16/32/64-bit words per line as the
shape demands. This is an implementation matter inside `BigUint` rather than a user setting, and it
should never change an address.

---

## 7. Features from earlier in the conversation, still standing

**7.1 Ingest through the game-format parsers.** Working decoders for several games' formats already
exist. Piping their output — level geometry, textures, PCM, strings — through canonicalisation and
onto the right line produces a corpus of *real, verified anchors*, which is exactly what every
filter in §2 and §3 needs in order to be calibrated at all. The best unbuilt item in the whole
conversation, because it manufactures that corpus out of work already done.

**7.2 File ingest at a content-derived address.** Drop a file in; it lands at the address derived
from its bytes, with a small record beside it. Dropping it twice lands on the same slot. Corollary:
spawn the player's own files on the nearest shelf the first time they walk in — it proves the
addressing is real better than any amount of README.

**7.3 An `.obj` structural filter.** A zero-allocation token scanner: every line starts with a
recognised prefix, vector lines parse as numbers, face indices are positive and within the vertex
count declared so far. It rejects almost everything on the first character, and it is what lets the
pages line be *searched* for meshes rather than only checked against one.

**7.4 A dedicated `.obj` alphabet.** Digits, `.`, `-`, `/`, whitespace and the keyword letters.
Cutting the space in the mapping layer before the filter layer runs is far cheaper than filtering.

**7.5 Punctuation filters for `babel29` and `ascii95`.** *Built: `sentence-shape-v1` and
`babel-punctuation-v1` (FILTER-PLUGINS §19).* Punctuation follows a word and is followed
by a space; for `ascii95`, a capital after a full stop. Countable with the word trie that already
exists. This matters: `lower27` throws away case and punctuation, so the books line currently holds
word streams, not sentences.

**7.6 Model-line filters.** *`canonical-mesh-v1` built (one encoding of each mesh, 10^-19 of the
default line; FILTER-PLUGINS §19).* The models line has a stack (`ModelSieve`) with `not-a-file-v1` and the
first two tiers built (distinct vertices, distinct indices, every vertex used; FILTER-PLUGINS §16).
The third tier is still to come. The three tiers of §12: local per-face
constraints (rank exactly), small-V constraints such as "every vertex is used" (rank as a state
machine, 256 states at V=8), and whole-mesh properties — watertight, manifold, convex — which judge
but cannot rank, so they run in `mark`/`hide`.

**7.7 Ranking beyond two colours.** *Built for two rules that count on any palette:
`palette-size-v1` and `row-runs-v1` (FILTER-PLUGINS §19).* Image compact mode only ranks two-colour pictures, because
entropy there depends only on the popcount. Everything with a real palette is unrankable, which is
why compact image mode keeps sparse pictures rather than structured ones.

**7.8 Ambient audio sampled from where you stand.** Warp the current tile's position into the audio
line and pass it through `key-v1` so the result is in a valid key. Continuous in-key ambient from
your actual coordinate, out of three things that already exist. The cheapest real win in the list.

**7.9 Generalise the preview bake.** The crate-face queue — reticle priority, flood fill outward,
per-frame budget, LRU — is written for models. One interface where each slot type supplies a cheap
2D draw gives the same thing to every line.

**7.10 A memory prober.** Watch RAM and VRAM, drop cached previews at the thresholds, without
disturbing position tracking. Needed the moment 7.9 exists.

**7.11 Predictive prefetch.** Warm the next two or three steps along the direction of travel, and
give frequently-walked paths a larger share of the cache, so the thing you are about to reach for
is already there.

**7.12 Reels — video as a composition, not a line.** *Built as the movies line (§13): a cover, a
title and N units of video, `composition-v1`; the video line stays, as the units.* F frames of W×H is the same set of
possibilities as F image units in a row, so the video line adds bundling and no content. A reel is
frame rate, image shape, a list of frame addresses, audio shape, a list of audio addresses. A still
scene stores one address and a count; filters work per frame on the image line; and one mechanism
covers book, album, comic and reel so no further category is ever needed. The honest cost:
frame-to-frame coherence moves up a level — a given reel can be judged, but every possible reel
cannot be counted, because exact temporal counting needs the whole previous frame as state.

**7.13 Branching end-caps.** Each line's ends divide into categories and sub-categories,
indefinitely, each staying its own 1D line. Doors remain the only cross-domain link.

**7.14 Agents as scouts, downstream of the filters.** Headless sweeps over the *filtered* space
logging where noise collapses into valid syntax; a vision model watching the preview queue and
stopping when the geometry looks like something. The honest limit: a blind sweep at these sizes
finds nothing, so an agent is only useful after filters have cut the space, or as a labeller
producing training data for new filters.

**7.15 Small physical touches.** A short wall that becomes a visible energy grid up close. The
Sieve as a held object that sifts a shelf radius. A hat that toggles the rain.

---

## 8. Things to be careful of

Recorded so they do not come back around.

- **Addressing is not compression.** §0. The one real exception is guided ordering, where the
  record genuinely is smaller than the text — because that ordering *is* a compressor.
- **A key is not a signature.** §0.
- **Splitting an address into coordinates saves nothing.** §0.
- **LOD does not come out of an address** unless §3.1 has an answer.
- **There is nothing to bisect.** Binary search needs a way to ask "is my target above or below
  here" without holding the target. In this space the only handle on the target is its content, and
  if you have the content you already have the file. A "50/50 beacon" has nothing to aim at.
- **The barrier is informational, not computational.** "All configurations contain all things, so
  this is only an efficiency problem" — no path optimisation reduces the number of bits needed to
  say which of 256^N items you meant.
- **Reducing a file does not move it.** An address in a fixed ordering is a function of content
  alone; there is no centre for low-complexity things to drift toward.
- **Dimensionality is not a hardware setting.** How many dimensions a space has is fixed by the
  shape of the content; hardware decides how much gets rendered, nothing more.
- **"Manifold", "Clifford torus" and "Möbius" are decoration here.** Worth keeping from that
  stretch is exactly one true thing: curves that appear to cross in a 3D projection need not cross
  in the space itself. The rest has no chart, metric or dimension attached.
- **The licence.** "AGPL with a non-commercial restriction" is not AGPL and is not an open-source
  licence — a field-of-use restriction makes it source-available. That may be the right choice, but
  it should be named accurately. No licence is worse: all rights reserved *and* nobody may fork it.
  Sieve still has no licence file. Separately, whether user-ingested content can be redistributed
  needs an answer before any ingest ships.
- **Mods as vectors is elegant and overstated.** `base + v` instead of a file overwrite is an
  interesting reframe, but "different axes can never conflict" ignores semantic coupling.
- **The multiverse cluster and the OS are separate documents.** Roughly forty turns of hypervisor
  and emulators, and ninety of Framework OS. Two things survive: the category definition file, and
  the stated position that they deserve recording rather than building.

---

## 9. Framing worth writing down

**9.1 The epistemic fence.** Meaning is not a property of a bitstream; it is a relation between a
bitstream and a decoder. Without a known grammar, protocol or anchor, high-entropy ciphertext is
indistinguishable from noise — so the engine must classify it as noise *by default*. The
alternative is that everything is potentially meaningful, nothing is filtered, and the system
degenerates into exhaustive enumeration, which is a novelty rather than a tool. This is the
load-bearing axiom under the whole filtration stack and it should be stated as one.

**9.2 The inversion.** The question is not how much is in the library. It is how much of it is
noise we can ignore, how we identify it, and how we define meaningful content. A lens, not a
catalogue.

**9.3 Selection is the creative act.** In a space this size nobody invents anything; they navigate,
filter and curate. Writing is excavation. Cartographers, not authors.

**9.4 Why a library and not a generator.** A generator that happens to emit something valid loses
it the moment you press the button again, unless you write it to disk — which defeats the premise.
An addressed library gives you a place you can leave, return to, and hand to someone else. The
cleanest one-paragraph justification of the project, and it belongs in the README.

**9.5 The scale anchor.** ~10^1,834,097 books; ~10^394,951 meaningful at one bit per character;
filtering removes about 1.44 million orders of magnitude, and what is left is still ~394,800 orders
beyond Lloyd's limit. Which is exactly why the design is navigate-and-verify, never enumerate.

**9.6 The lineage, and the distinction.** Llull's rotating discs, Swift's Lagado engine, Lasswitz's
*The Universal Library* (1901, which Borges cited), then Borges in 1941. Verify the dates before
publishing any of it. The distinction to draw is dimensional scope: the same combinatorial argument
applied to text, audio, images, video, geometry and books at once, with exact addressing and a
terminal you can type into.

**9.7 Disclosure.** Anyone happy to consume AI-assisted work will not mind being told; anyone who
prefers human-made work deserves the right to choose. Which means saying so, here, about this.

---

## 10. Loose ends

Small, concrete things said once in the Gemini conversation and not followed up. Kept because they
are cheap and because they are the sort of thing that is lost otherwise.

**Interface**

- The media panel is see-through and should not be.
- Something renders behind one of the menus during a transition. Never pinned down.
- The loop marker could be a floating checkered gate you walk through rather than a floor strip.
  The grammar is settled either way: one strip is a line's start, two a metre apart is where every
  line starts together.
- Portal signs: a plain rectangle above each doorway naming the line it leads to, either neon in
  that line's colour or bubble letters carrying the portal's own shimmer. Try the second first.
- Shimmer on an item: on the crosshair and on select for certain; on walking past, undecided.
- A shelf's items could face outward on the outside of the ring or inward. Both should exist as a
  graphics toggle; which way you face only reorders how the lines are listed in the readout.
- Presets above START, greyed out when a preset would cross the machine's limits.
- Key remapping, explicitly the lowest priority: Controls stays a read-only list read from the
  language file.
- Menu items centred vertically as well as horizontally, with the help slot sized for the longest
  help on that screen so the block does not jump as you move down it.
- Auto-DLC toggle: "X new repositories detected" on startup, pulling newly published repos.
- A record player on a table beside every AUDIO doorway and a television beside every VIDEO one.
  Not functional. "It is a lot of TVs but why not."
- The diagnostic views, with a slot deliberately left empty: Code, Keys, Mixed, Future. The system
  key alone toggles in and out; bare function keys slide between them; Tab cycles. It needs a speed
  control, because at full rate it is an unreadable blur — modelled on the 1-0 speed keys and
  slider in the 1998 *War of the Worlds* RTS.

**Navigation**

- The Mirror: a mirror in a room that opens the map for where you are standing, "like opening a
  Wikipedia article on the room you are in".
- An interactive portal where you pick the line to travel to, with the one you came from kept as
  the default return.
- Wheel scrolling already moves through tiles, and shift scrolls faster. The same mechanism should
  move through the forks at a junction — see §5.12 for the shape that makes that work.
- Warp by content: paste a paragraph and be taken to its address. Built for text; worth a button.
- A warp-to-start, which is the only realistic way anyone ever sees a loop marker.
- Telescopes at the edge that really do pull the camera out to see the whole structure.
- A coin toss into the drop that mints one permanent seed per machine.
- Multi-pane views were considered and rejected: "it feels like it would be a headache to look at".

**Numbers and defaults already fixed**

- Book heights vary 0.34–0.46 m on pages, image and books; audio and video are fixed at 0.40 m,
  because records and cassettes come in one size. A slot is 0.375 m along a shelf, 0.50 m between
  boards, 0.35 m deep; a corridor tile is 8 m; the marker is a 16 x 2 checkered strip.
- Four models are needed — hallway with door frame, bookcase, book, marker — plus the per-line
  variants: record, canvas, sheet of paper, cassette, book, crate.
- A page is cut at 3,200 characters by default. A book's id is the SHA-256 of its content alone.
- The noise fraction, written down because it was asked for twice: 10^-1,439,146, about 1.44
  million nines.
- The dial to test against: 6 characters (~10^8.6), 8 (~10^11.5), 12 (~10^17), 100 (~10^143),
  1,000 (~10^1,431). Exhaustive checking starts at 6-8, where everything can be counted.

**Process**

- The old README moves to a `Previous` folder rather than being versioned, and it is written by
  hand, because an outsider's view of the project is still possible to fake but not for long.
- The final documentation pass waits until the visuals are done, because "that last task is usually
  the sort of thing that gets skipped or only half done".
- There is a pile of loose notes — `engine-idea`, `nextclaude2` through `claude5`,
  `downloaderfriendidea`, `state-space-near-zero-...` — that has never been reconciled with the
  repository.
- Nothing has ever run on GitHub's Windows or macOS runners, only Linux.
- The first release is to be the archive's first verified anchor, every key tagged
  `sieve-build-release-v1`.

---

## 11. Questions that never got an answer

- **Which way do the shelves face, and where does the wall change sides?** Asked four or five
  times across the conversation and never settled. Currently: both walls everywhere except the
  binary line. Parked deliberately.
- **Should the pages line move to `babel29` or `ascii95`,** so books hold sentences rather than
  word streams? `lower27` throws away every capital and every mark of punctuation.
- **Does the `scowl-en-60-names` side effect matter?** It has 130 two-letter tokens where the plain
  list had 69, so `zn`, `tb` and `pt` now pass and stand among real words on a compact shelf. The
  fix is a new dictionary version; the call was never made.
- **What is the vertex ceiling for models,** and what makes a textured triangle not noise? The
  hardest filter question on that line and it has no answer yet.
- **Which font comes next** — Cyrillic, Polish and Czech need a taller one, and the 8-pixel layouts
  have to be reworked before any of them read properly.
- **Which media types cross paths in the state space?** A cheap experiment, named and never run.
- **How much would the reducer actually save** on a real corpus? Never measured, and it is the
  number that decides whether the key idea is worth anything (see §0).
- **The licence.** Still nothing in the repository.

---

## 12. Noted along the way (September 2026)

- **Each line as a filter against the others** (Edward). Built for text against the binary line:
  `not-written-v1`, `binary-kind-v1` and J (FILTER-PLUGINS.md §15), with the arithmetic that
  exclusion removes only the share another line takes up (10^-4.23 of 32-letter pages) while
  requirement multiplies shares. Still open, in the same spirit:
  - pictures whose pixels are text or a file (an image line reading of its bits);
  - melodies whose MIDI is also valid as something else;
  - models (`.obj` is text, so a models filter could ask that a page is not a valid model, and
    the reverse);
  - ascii85 in `not-written` (its groups of five are base 85: countable by comparing each group
    with the bounds digit by digit, a second version);
  - sound itself (the `pcm` set, SPECIFICATIONS §3.3) against the note sets: a recording that is exactly a melody played by the synth belongs on the note line. Its first filters are built (`sound-peak-v1`, `sound-step-v1`, `silence-run-v1`); next, what tells sound from noise beyond a step: pitch (a period that repeats), timbre, loudness over time, and counting them at a second of sound, where the step automaton's table outgrows the filter memory;
  - open-ended notes between `notes2` and `pcm`: every MIDI pitch, lengths in ticks, loudness, an instrument a voice, tempo;
  - a check that a signed file is well formed past its signature (a PNG's chunks and CRCs): judging
    only, since it cannot be counted.
  The binary line as the ground the other lines stand on is already true in one direction: every
  item's file is on it, and J goes there and back.
- **Transformed copies** (Edward): compression and encryption of other lines' content as filters.
  Written up in TRANSFORM-FILTERS.md: keyless and small-keyed transforms can count exactly,
  compression can only judge, and every key at once excludes everything.
- **The game**, *State Space: Near Zero*: GAME.md.
- **A nine-tone note system** (Edward, 6 October 2026: "perhaps in the future we can implement the
  Klingon music space, which has a 9 tone scale or something"). An entirely different note system:
  a scale of nine steps rather than seven, with nine modes, which would give each of the nine lines
  a mode of its own. Now there are seven modes for nine lines, so tracks and movies share audio's
  and video's by default and their doors are silent unless a mode is set in the Media Player. A
  drone under the lines made of another's was tried to mark those doors and set aside (not nice to
  listen to); books keeps Aeolian.
- **Loudness per note** (Edward: worth having, in a later version). Each note event quiet, medium
  or loud, which triples the note symbols; for the `notes2` family, after its first version.
- ~~**Books of `notes2` melodies.**~~ Done as `sieve-book-v2` (7 October 2026): an audio section
  on another set names it (`notes <symbols id>`), for `notes2`, `notes3` and `pcm` alike.
- **`notes2` versions of `melody-lengths` and `melody-ending`,** whose duration choices name
  `notes104`'s e q h w: on `notes2` they step aside, so a `notes2` track need not end on the tonic.
- **Rules between voices** of a `notes2` unit (whether notes sounding together agree): harder than
  per-voice filters, because each voice keeps its own time.
- **Optimise all with the COST pages** (Edward, 2 October 2026: "the optimise all eventually needs
  wiring into the COST system, so that we can use both the filter pages and the cost pages to work
  out the best possible values for everything"). X (optimise all) now ticks every filter in reach
  and, of two that cannot be counted together, keeps the one that removes more on its own, its
  settings as they stand (each filter's own share is in the list: Filtered: X%). That is a greedy
  choice of which filters, not of their values, and it measures only the shelves. The aim:
  - **Both pages as one measure.** The filter pages say how much of the line a stack removes (its
    survivors); the COST page says what naming one survivor costs (its address in bits, chars and
    bytes, against the unit itself, and the guided model's shorter form). The best settings are
    those that make known content cheapest to name while still holding it: survivors fewer, and
    the content we care about still among them. So the score needs anchors, real text, melodies and
    pictures that must survive, and reads their compact addresses off the COST page.
  - **Values, not only ticks.** Each parameter (max-run's `max`, key's tonic and scale, row-runs'
    `changes`, palette-size's colours, the dictionaries) searched for its best value under that
    score, a filter at a time and then together, with the anchors' verdicts as the hard limit
    (a setting that drops an anchor is out).
  - **Sets, not pairs.** Of filters that clash, the best set rather than the stronger of each pair;
    and a filter that another ticked one implies is dropped (X now ticks `clean-data-v1` and
    `clean-data-v2` together: harmless, as the stricter decides, but it costs a product state).
  - **Lines that judge only** (utf8-valid past its table, not-an-item asking for melodies) can be
    scored by estimate (`~` in the tally) but not by compact address, so COST shows them as
    unavailable rather than wrong.
  - *Built, 7 October 2026:* **COST's balance** (Edward's design). The anchor is the item in hand.
    The tab weighs the cheapest address found for it, under the filters as they stand, against it
    as a file (`Neutral : 0%`, `Positive : X%`, `Negative : X%`), with a bar from the centre, red
    for longer and green for shorter.
  - *Built, 7 October 2026:* **the search, for one item** (`cli/tailor.*`; K on the COST tab,
    Return to apply; `sieve tailor`). Each filter's settings searched one at a time for the item
    (integers coarse to fine, texts over their choices and the registered dictionaries), then a
    stack grown from each filter kept, the best of them chosen.
  - *Built, 7 October 2026:* **the cleanest digits, several anchors, and files weighed.**
    - The search's target is each anchor's shortest route among the survivors (its number with
      leading zeros dropped, or a bearing and a walk), not the count: Edward's "we shouldn't always
      push to 0 in case the filters can't land there; we would be searching for the cleanest
      digits". Ranks move as a stack changes, so a search on routes alone falls into corners;
      stacks are grown both by shrinking the count (which bounds every number) and by shortening
      the routes, and the shortest wins. On the melody: 16 bits (`b4f7`), as by the count.
    - Several anchors under one stack (`--unit all`), and the description priced and, where the
      stack travels, paid for (`--describe`).
    - **Files weighed** (`cli/weigh.*`; `sieve locate --weigh [--tailored]`, the File Locator):
      every file against its own address, by its number among the binary line's survivors, and,
      where it is exactly an item of another line (`item_of`, the test not-an-item-v1 makes), its
      place and number there; each line's shape and stack paid once, and dropped where they cost
      more than they save; with tailoring, each line's filters tailored to its items. Sieve's own
      files (two melodies, a picture, a note) come to 21.3% of their own addresses.
  - **Still open in 1e:**
    - *Built, 7 October 2026:* **`sieve-manifest-v4`, the installer packed** (Edward: "a new
      manifest and update to the installer is a given"; compression built in, as he chose). Each
      file's way: `x86` (programs, the x86 filter, then LZMA2), `raw`, or `lines K` (a text made
      of some of file K's lines, as a mask: each SCOWL list but the largest); two LZMA2 streams;
      unpacked by the LZMA SDK in every program that installs, and independently by the oracle.
      A release-shaped folder: 20.00% of its size, against 21.17% for one LZMA2 stream. Edward's
      Windows release, the installer program: 7,403,749 bytes packed, against 7,448,947 carrying a
      7z of the folder (`--seven-zip`), 45,198 bytes (0.61%) smaller, although 7-Zip uses BCJ2 on
      the programs. A release's
      files are no line's items, so the weighing's item ways are not among v4's yet: they need the
      lines in `sieve-install` (pages and melodies are core; pictures need the PNG writer). Still
      open for v4: item ways, and "made from another file" ways beyond lines (a few bytes changed:
      a delta).
    - **The line's own settings as part of the search** (alphabet, length, palette, picture size):
      a text file is a page only if the page is its length and its alphabet holds its characters,
      so most files are no line's item as the lines stand. Fitting a line to a file (a page of the
      file's length in the smallest alphabet that holds it) would name far more files shorter,
      the shape paid for as it is now.
    - Sound (pcm) and models as items; the books, models, binary, tracks and movies lines in the
      search; settings searched together rather than one at a time.
  - **The description travels too (Edward's example, 7 October 2026).** A stack tailored to one
    item is part of that item's name unless the reader has it already: which filters are ticked,
    and each setting, are bits like the address's. Priced at one bit for each filter the line
    offers (a mask) and log2 of the values each setting allows, measured on two items:
    - "welcome to the sieve", 32 characters: 152.2 bits unfiltered; tailored, 80.4 bits of
      compact address plus 58.4 of stack (19 of mask; dictionary 2.6, word-cost's rate and slack
      13.6, letter-triples 3.3, max-run's max_run 19.9), 138.8 in all: a saving of 13 bits, not 72.
    - the melody `A4q C5q D5q E5q G5q A4q C5q E5q`: 53.6 bits unfiltered; tailored, 16.3 plus 59.9
      of stack (11 of mask, 48.9 of settings), 76.2 in all: 22.6 bits *longer* than no filters.

    So the search should minimise the address plus the stack's description (a two-part code, the
    minimum description length), and COST should show both; a setting left at its default could
    be priced at a bit or so rather than at its whole range. And the stacks worth most are those
    agreed in advance, which is the next idea.
  - **Predefined stacks, named by a bitmask (Edward, 7 October 2026; future development).**
    Stacks of filters predefined for specific use cases, each with its settings fixed, pinned and
    versioned as the canonicalisation rules are, and agreed by every reader. An address is then
    (mask, rotation, item): the mask chooses the stack, the rotation says where on the line's loop
    of survivors (which shelf), and the item is its number on that shelf. Edward's reference
    example is the hallway's starting page, "welcome to the sieve", written as "1 - 0 - 112": stack
    1, rotation 0, item 112 on the shelf. That is the idealised form: the filters are not needed to
    find the page at all, but under a stack made for its use case it would stand on the first
    shelf. What each part must carry, measured today:
    - without filters the page is item 112 on the shelf of tile 427158...2915, a number 44 digits
      long, so the rotation carries 152 bits;
    - under the stack the search tailors to it, it is survivor 1577089591533231767573498 of
      1626339824884908067816628 (80.4 bits), so the rotation still carries 73 of them;
    - rotation 0 is reached when the stack leaves the page among its first shelf's 128 survivors.
      A predefined stack does that only for the items its use case predicts; a stack made for this
      page alone would hold the page's information in its settings (above), so the bits would
      move into the mask's meaning rather than vanish.

    Short of needing no mask at all, that is the limit: the bits that choose the stack, plus what
    the stack leaves to say about the item. Filters are not the only way down: the guided ordering
    under the same stack names this page in 52 bits, because the model predicts English. Averaged
    over every item, no scheme beats the line's own size; the reduction is for the items the shared
    choices predict, which is what a use case is.

## 13. Composition dimensions (discussed 5–6 October 2026; built 6 October, but for what is still open below)

**The principle.** Keep every dimension's unit as small as it can be, and get length and arrangement
from a dimension of compositions above it, as books are made of pages. A composition of N units
keeps its base's filtered share to the power of N (before any filter at the seams), so filtering
compounds upward, and exact counting stays where it fits: the short units. The sound filters show
why: they count a tenth of a second exactly and only judge a whole second, whose table would need
about 31 GB.

**The shape Edward chose.** Each composition dimension holds one kind of thing, so none is vaster
than before:

| Base (short units) | Composition |
| :--- | :--- |
| pages | books (as now) |
| audio (a stretch of sound, or a bar of notes) | tracks |
| video (as now) | movies |

No existing line is renamed: audio and video keep their names and ids, and the two new dimensions
take new names, so every address made so far stands. A track or movie holds base units whose length
is a setting, as a book holds pages of any length.

- **Images and models** have no lower dimension of this kind. Two far-off ideas belong here,
  though:
  - **A tile dimension for images:** pictures made of small tiles, as the vault's PDQ check
    reads a picture in blocks. Tiles reduce the combinations a picture can have to those its
    tiles allow. It has no name yet.
  - **A world space for models:** a higher dimension that composes models and textures (§14:
    what it needs for UVs).
- **Seam filters:** each composition gets filters at its joins: sound that doesn't jump between
  seconds, a key kept across bars, frames that carry on from one video unit to the next.
- **The same content in two places:** a base line set long enough to hold a whole composition
  overlaps it, as the pages and books lines already overlap. Either it stands, or a
  cross-dimension filter removes one from the other, as `not-a-file-v1` does with the binary line.

**The problems to solve first.**
- **Address stability:**
  - Line names are in the symbol ids that scrambled addresses are keyed on (`video/mono/5x5x8`),
    in settings files, in `--line`, in book records' section shapes and in CI's pinned outputs.
    Renaming a line would move its addresses, so none is renamed: the names above add tracks and
    movies and leave every existing line as it is.
  - **Covers and titles stay where they are.** A second of sound or a short video can have a
    title and a cover as well as the composition holding it, so tracks and movies get their own
    and audio and video keep theirs. Moving them up would reshape (and readdress) those lines;
    nothing calls for it.
- **The setup menu** was already crowded. A separate settings window would cost the bars moving as
  a setting is turned, the reason the menu reads as it does, so that is ruled out. Decided (6
  October): everything on one screen, laid out for 1920 x 1080. The budget's bars moved to the top
  right and "the largest that fit the budget" went, which leaves room for tracks and movies with
  no scrolling. A mock-up of settings in the map's columns was tried and set aside (values cut
  short, the bars pushed down); a scrolling list stays the fallback if the rows outgrow the screen.
- **Done (6 October): the dimensions are data.** `client/dimensions.hpp` defines each once, in door
  order: its identity (`Media`), its names, its two colours, the unit line it is, its music mode.
  Doors, themes, the map's columns, the HUD's rings, the menu's lists, J's routing and the music
  all read it, and anything saved per dimension is keyed by identity, never by door, so
  reordering the doors is reordering the table.

**Decided since.**
- **The order,** each part before what it composes, and image, a part of nearly all of them, first:

      binary | image | pages books | audio tracks | video movies | models | binary

- **The colours:** tracks audio's green, movies video's red, each with black edges, as books
  (grey) has. Black edges then mean a composition, whose background says what of. The
  colour-blindness pass (2.4) is still to do: audio's and video's dark green and dark red differ
  mostly in hue, which red–green colour blindness loses.
- **Their models:** their own (`book-tracks.obj`, `book-movies.obj`), begun as copies of audio's and
  video's, to be modelled from there.

**The order suggested.**
1. ~~Make the dimensions data, with no change in behaviour.~~ Done.
2. ~~Reorder the doors as decided.~~ Done: image, pages, books, audio, tracks, video, movies,
   models, binary; the setup menu's rows follow the doors.
3. ~~Build a composition engine.~~ Done as `composition-v1` (`sieve/composition.hpp`,
   SPECIFICATIONS §11): a cover, a title and N units, one mixed-radix number, with exact filters
   part by part, an oracle and vectors. Books were left on `bookspace-v1` rather than moved: their
   title is a whole page and their pages are filtered as one text, so moving them would change
   neither their addresses nor their behaviour, only add risk.
4. ~~Add tracks and movies.~~ Done, both at once.

- ~~**Seam filters.**~~ Done (6 October) as a fourth stack, **joined**: the units joined into one
  unit of their line and judged as one, as a book's pages are read as one text, so any filter of
  the line judges across the seams. It counts and ranks when the units have no filters of their own
  (the joined unit is then the part); with both, items are judged but not counted.
- ~~**The `sieve` tool.**~~ Done (6 October): `info`, `warp`, `read` and `filters` take `--line
  tracks` and `--line movies`, with the hallway's option names, and CI checks that the tool and the
  hallway give the same item at the same address, plainly and compact.

**Still open.**
- **Filters made for seams:** the joined stack uses the line's own filters over the whole joined
  unit. Filters that look only at the joins (a note held across the boundary, a cut between frames)
  would judge less and count more easily.
- ~~**A record format for them** and J opening one onto its shelf.~~ Done (7 October): records of
  the books' kind (`cover`, `title`, `units` sections), `sieve-book-v2` where an audio section names
  a set other than `notes104`; F saves one (`.track`, `.movie`), J opens one onto its shelf, and
  `sieve bind --line tracks|movies`, `read --record` and `unbind --units` make and read them.
- **The `sieve` tool reads MIDI files** only on `notes3` lines; on `notes104` and `notes2` a `.mid`
  given to `warp --file` (or `bind --file`) is read as notation and refused. The hallway's J reads
  MIDI on every note set (`midi_to_notation`); the tool should too.

---

## 14. Worlds, and UVs described by how they were made (Edward, 7 October 2026)

**Worlds.** A future dimension of compositions of models, as books are of pages and tracks of audio:
a world places models and puts textures on them (§13, "a world space for models").

**It needs UVs.** A UV map says where each point of a model's surface falls on its texture: a pair
of coordinates for each vertex of each face. Without them a world cannot texture a model. A line of
every possible UV map is easy to define and vast, and UVs already exist in Sieve in another form:
the `vt` lines of an `.obj` file, a page of the `ascii96` text line.

**Edward's insight.** Most UV maps made now are not drawn by hand. An algorithm unwraps the mesh
(automatic seams, projection, packing into the square) and the result is used as it is. Such a map
is fully described by what made it:

- the mesh (an address on the models line), the texture it is packed for (an address on the
  image line, which can matter for packing: its size and aspect), the unwrapping algorithm (a
  pinned, versioned id, as the canonicalisation rules are: `unwrap-v1`) and its settings;
- for a map someone then adjusted, that and a list of the adjustments: which UVs moved, and by how
  much (a delta);
- for a map made wholly by hand, the same test: it is compared against the unwrap algorithms, and
  if one of them plus a delta is provably smaller than its UVs, it is stored that way; if none is,
  it is stored as its UVs, as now.

So a world's UVs need not be stored where an algorithm can make them: generate the bulk, then apply
the delta over the top. That is "throwing more memory at the problem" put to use: computation and
memory spent to rebuild what can be rebuilt, so that only what cannot is written down.

**Where it pays: the storage footprint.** The aim is not a shorter UV map in use but less to keep:
if Sieve's back end grows into an operating system and its front end into a game engine, an asset
is stored as model + texture + UV algorithm (+ delta where needed), and the engine rebuilds the
UVs when it loads it. Once rebuilt (and cached) they take as much memory as the original UVs did,
and rebuilding costs some computation and working memory; what is saved is what sits on disk and
goes over the wire. The library is mixed: each asset keeps whichever form is smaller, the
generated description where an algorithm and its delta come out smaller, however the UVs were
made, and its UVs in their original form where none does.

**What the reduction is, exactly.** It is the stored size, and it is not a reduction over every UV
map that could exist: by counting, no description can make every one of them shorter (there are as
many maps as descriptions of their length). It shortens the maps an algorithm makes, from
O(vertices) coordinates to a constant (an id and its settings), and those close to one, by the size
of their delta. How a map was made does not decide its form: a hand-made map that happens to lie
close to what some algorithm gives is stored as that algorithm and a delta too. The comparison is
exact, not estimated: the encoder builds both forms, measures them and keeps the smaller, so a map
is stored raw only where every algorithm and setting it tried came out larger, and no map costs more
than one bit beyond its UVs (a flag saying "raw UVs follow"). The price is computation when a map
is stored: trying the algorithms, and their settings, against it. "Provably smaller" means smaller
than the forms tried; a search that tries more settings can only find more maps it shortens. The
maps people actually use are overwhelmingly made by an algorithm or close to one, which is the
point. It is the same trade the guided ordering makes for text (SPECIFICATIONS §4.2): a model
predicts, and the address pays only for what it did not predict.

**What it would take.**
- **A deterministic, pinned unwrap.** The same description must give the same UVs on every machine,
  to the bit, or addresses mean different things in different places. Blender's Smart UV Project or
  xatlas cannot be borrowed as they are (their output depends on their version and on floating
  point); Sieve would pin its own, specified exactly (integer or exactly-rounded arithmetic), as it
  pins `canon-text-v2`. Maps made by other tools' unwrappers then come in as "ours plus a delta",
  and how large that delta is decides how much is saved: worth measuring on real assets (§7.1's
  game-format parsers would supply them).
- **An ordering for it.** The raw UV line stays (every map has its place); a derived ordering puts
  the generated maps first, as the guided line puts likely text first, so a generated map's address
  is its algorithm, settings and delta.
- **The same pattern beyond UVs.** Textures that are procedural (noise, gradients, tiles) plus a
  delta, and models from generators plus a delta, are the same idea; UVs are the cleanest first case,
  because unwrapping is so often left as the algorithm made it.

**Entering a world, and a games dimension (Edward, 10 October 2026).** In Edward's words, as near
as written down:
- **ENTER:** eventually the worlds dimension should have an ENTER button, which takes you into the
  world in hand, to fly around it in "ghost" mode.
- **A video game dimension, in the far future:** games reconstructed from the library. A game
  would pull its worlds from the worlds dimension, its models from the models dimension, and its
  UVs as this section's spec has them (the unwrap algorithm and its delta).
- **Code:** each would carry an attached manifest of code that compiles itself, or is already
  compiled. The first test: Sieve itself.
- **Translation layers, in the far future:** layers that reduce every game to its smallest
  footprint and store it in the library.

*Evaluation:*
- **Ghost mode** is near: a world's mesh is built already (`WorldSpace::mesh_of`). Entering it
  means a camera inside that mesh, flying with no collision and no gravity, at a scale where a cell
  is a room, with Esc back to the hallway. Its parts are this section's (textures, once worlds have
  UVs) and §16.7's (the placement).
- **What a game needs from worlds** is more than `worldspace-v1` gives: placement off the grid, a
  scale per model, textures, and many more models than four. Each of those is a later version of
  the world space, numbered as this one is, and each makes a world's address longer. So
  reconstructing a real game's level wants an ordering that puts likely worlds first (a world
  built by a generator, plus a delta), as §14's UVs are.
- **The code is the part that does not reduce.** Code is stored as files (the binary line, or
  manifest entries, §4 and §16.3); the library names it, but it costs about its compressed size.
- **"Compiles itself"** needs a reproducible build: a pinned compiler and flags, so the same source
  gives the same bytes and its SHA-256 checks out. "Already compiled" is per platform. Sieve's own
  release (its source as a `sieve-manifest-v4`, built by CI from that source) is the natural first
  test, as Edward says.
- **Translation layers** are §7.1's game-format parsers carried through: assets read out of a
  game's formats into Sieve's forms (models, worlds, textures, UVs as algorithm plus delta). The
  footprint falls where the assets are generated or near it. Hand-made assets and code stay about
  their size, as the counting argument says (§16).
- **Verdict:** ENTER and ghost mode, build when worlds have textures (or before, untextured). The
  games dimension and the translation layers are the long arc this section and §7.1 describe;
  keep them here until worlds can carry a real level.

## 15. An AI model's state space (Edward, 7 October 2026)

**The idea.** A dimension made from a model file: given a language model's file, its tokens are the
line's symbols, a unit is a run of tokens, and the model's weights order the line. At every place,
the weights give a probability for every token that could come next; a run of tokens the model
expects is near the front of the line and named in few bits, and one it does not expect is far
out and named in many. Choose another model file and the same texts stand in another order.

**It is the guided ordering with a far stronger model.** That ordering exists (SPECIFICATIONS
§4.2): a text's address is its arithmetic code under a model, short where the model predicts it.
Today's model is small and pinned, a character model of order 5 trained on Project Gutenberg
(`gutenberg-lower27-o5`), and it names English at about 1.8 bits a character against the 4.75 of
the plain line (the README's examples). A large language model predicts English far better:
published experiments that drive an arithmetic coder with one report about one bit a character or
less, roughly half of what the character model does. So the same idea, with a model file in place
of the character model, would name likely text in about half the bits again, and the COST tab's
balance and the tailoring search (§12) would weigh it like any other way.

**What it would take.**
- **Reading the file:** the formats models are published in (GGUF, safetensors), the tokenizer
  from the file itself, and the model's id being the file's SHA-256, so an address says which model
  it is taken under, as `canon-text-v2` and the pinned character model do now.
- **The same probabilities everywhere, to the bit.** An arithmetic code decodes only if the reader
  computes exactly the probabilities the writer did. Inference in floating point differs from one
  processor, library or thread count to another, so a model line needs inference pinned as the
  canonicalisation rules are: integer or fixed-point arithmetic on a quantised model, specified
  exactly, with an independent implementation in the oracle to check it. That is the hard part, and
  the part that makes it Sieve's rather than another compressor's.
- **Text to tokens as the canon step.** One text can be split into tokens in more than one way;
  the line takes the tokenizer's own split as canonical, so a text has one unit, and the
  other splits are units of their own (or set aside by a filter).
- **Time.** Every token costs one pass through the model, writing and reading alike, so reading an
  address of a page means running the model over the page. A small model (tens of megabytes) on a
  processor is quick enough to hold an item; walking the shelves, where every item in view is
  decoded, wants the units short or the model smaller.
- **The model is the shared agreement.** A reader must have the same file, so the model is paid for
  once, as the dictionaries are, and pays back over every text named under it (§12's two-part
  code: a model file of gigabytes is worth it only to whoever names a great deal of text).
- **Filters as before:** token-level filters and compact mode over the token line, ranked under the
  model as the guided ordering's survivors are now.

**The first model, taken apart (10 October 2026).** Edward chose SmolLM2-360M-Instruct for the
first test. It is pinned as `HuggingFaceTB/SmolLM2-360M-Instruct` at commit
`a10cc1512eabd3dde888204e902eca88bddb4951`, under the Apache 2.0 licence, from Hugging Face.
Its `model.safetensors` is 723,674,912 bytes, SHA-256
`e6bffe7435d7ddc10fd3b9a9efd429dafbacb1cb17015fb5562664e7532bf86e`. The model files are not in the
repository.
- **The shape:** a Llama design (`config.json`, 846 bytes).
  - **Sizes:** 32 layers, hidden size 960, feed-forward size 2,560, 15 attention heads sharing 5
    key/value heads of 64.
  - **The vocabulary:** 49,152 tokens, with the embedding shared by the output layer.
  - **The total:** 361,821,120 values, all bf16.
- **The file in three parts:**
  - **The start:** 32,672 bytes, an 8-byte length and a JSON table of 290 tensors (2,424 bytes under
    xz). It is rebuilt byte for byte from `config.json` alone: the tensor names in alphabetical
    order, their shapes from the config, offsets end to end, `{"format":"pt"}`, padded with
    spaces to a multiple of 8. So the start needs no address of its own.
  - **The weights:** 723,642,240 bytes. By kind:
    - the feed-forward layers, 472 MB (gate, up and down, 157 MB each);
    - attention, 157 MB (q and o, 59 MB each; k and v, 20 MB each);
    - the embedding, 94 MB;
    - the norms, 0.1 MB.
  - **The tokenizer:** a file of its own, `tokenizer.json`, 2,104,556 bytes (436,260 under xz):
    49,152 tokens, 48,900 merges and 17 special tokens. The chat template is in
    `tokenizer_config.json`.
- **What the weights are made of:**
  - **The high byte** of a bf16 value (the sign and the exponent's top seven bits) carries 2.72 bits. The values cluster near
    zero, with a spread of 0.12 to 0.18 by tensor (the norms' scales sit near 1).
  - **The low byte** (the exponent's lowest bit and the mantissa's seven) carries 7.85 bits of 8.
  - **Neighbours:** the previous value in a row tells almost nothing (2.716 bits against 2.714).
  - **Distinct values:** 6,486 of the 65,536 a bf16 can take.
- **What that comes to, lossless:**
  - **Coded by frequency:** each tensor's values coded by how often they occur come to 10.52 bits a
    value, 476 MB, 66% of the file.
  - **xz:** xz -6 on the whole file gives 512.9 MB, 71%.
  - **So** a prior over weights (§16.8b, the noise gradient as a prior) starts from about 476 MB on
    this file. What it can still win is in the 2.7 exponent bits, by modelling more of the
    structure (each tensor's own scale, rows and columns, layers alike, low rank). The 7.85 bits a
    value of mantissa noise are not predicted by anything measured so far.
  - **Smaller still is lossy:** 8-bit is about 362 MB and 4-bit about 181 MB of weights. A
    quantised model is a different model, with an address of its own.
- **Where the AI dimension shortens things:** the model is a shared agreement, paid for once (the
  config, the tokenizer and the weights under the prior). The shortening comes in the text named
  under it, at about half the bits of the guided ordering (above).
- **Edward's view:** the gains will grow as the system is built out. The measurements fit that in two
  places: the exponent bits, where better priors can win more, and the text named under ever better
  models. The mantissa noise is the part that stays, unless a model is quantised.
- **The measuring tools** are now `sieve tensors` (SPECIFICATIONS §12.0a): the parts, the hash, the
  start rebuilt from `config.json` (`--config`, `--start-out`) and the statistics (`--stats`,
  `--tsv`), checked against the oracle's `tensors`, which agrees on every one of the 290 tensors.

**Another reading, for the record.** A dimension whose units are the weights themselves (every
model of one shape) would name a model by its weights: the binary line's files again, as long as
the files. It is the predictive reading above that shortens anything.

## 16. Edward's list of 10 October 2026, evaluated

Nine ideas, each in Edward's terms and then weighed. Two of them (16.1, and the noise gradient
in 16.8c) meet the counting argument head on, so it is set out once here. Any scheme that picks, for
a file, the best of `k` options (rotations, shuffles, keys, filters made for it) can bring the file
`log2 k` bits nearer the front, at best: the best of `k` evenly spread places is about `k` times
nearer 0. Naming which of the `k` was chosen costs the same `log2 k` bits. On average the two
cancel exactly; for a lucky file the search wins a few bits, for an unlucky one it loses them. That
is §3.6, §12's two-part code and §17.1's rotation transform again. A scheme gains only
where the options are agreed in advance and the files are not evenly spread among them: where a
prediction (a model, a use case's filters) puts likely files first.

**16.1 Files as their own filters, counted from both ends (a button on COST).** Edward's words: a
counting system with sliding windows that uses files themselves as filters, counting from the first
and last bytes towards the middle and watching the bearing (degrees) the file lands at as the count
goes; a custom filter for one file. In the idealised example, a gigabyte at each end would find a
terabyte file by pushing it to an exact interval. It could take a long time for large files, and it
may be the best path to the shortest route.
*Evaluation.* A filter made from a file is a description of that file, and it must travel with the
address; however it is counted, the address and the filter together are at least as long as the
file's information. In the example: the two ends are 2 GB, and the terabyte files that share those
ends differ in the 998 GB between them, so naming one of them still takes 998 GB unless something
already known predicts the middle. Counting inwards from both ends reads the same bytes in another
order, and the bearing a file lands on is its leading digits, so neither shortens it. What does
stand: (a) a **longer shortest-route search** on COST, run on a worker with a time limit and the
best route so far kept: variable length addressing (§3.6) already finds the shortest route by
bearings of up to 20 places, and a button could search further (more places, longer walks) for as
long as allowed. Built today, it would find routes for the lucky units, as now, never for most;
(b) **other files as filters**: a file known beforehand helps name a new one (an anchor's fragment,
near-anchor-v1 and contains-fragment-v1; v4's `lines K`, which names a word list by another). That is
real, and it is what the idea becomes when the filter is not the file itself. Verdict: (a) worth a
button; the self-filter, no.

**16.2 The File Locator from the main menu.** Show "Go to it" only when the locator is open in
the world, and add "File Locator" to the main menu, under Settings and above the Filter Designer.
*Evaluation.* Straightforward. From the main menu there is no hallway to walk in, so Go to it is
hidden there and everything else (choose, weigh, tailor, save, install) works as it does now. One
point to decide: "Use the tailored filters" from the main menu would save the filters for the next
hallway built rather than build one at once. Verdict: build. *Built (10 October 2026):* the
locator is its own class, opened from either place; from the main menu, Use the tailored filters
saves them for the next hallway.

**16.3 Maps as the installers are.** Update the `.map` system to work the same way as the `.sieve`
and installer system.
*Evaluation.* A map that holds files (`sieve-map-v2`) carries their bytes as they are, after its
text, as v3 installers did. Packing those bytes as v4 does (the x86 stream, `lines K`, LZMA2,
unpacked and compared before use, kept as they are where packing is no smaller) is the same code
(`pack.*`, `locate.cpp`'s unpacking), so a `sieve-map-v3` follows directly. A map that holds a file
already compressed (the release map holds `sieve-source.7z`) gains nothing; a map holding many
anchors of its own (texts, programs) gains what an installer does. Verdict: build, small.
*Agreed since (10 October 2026): the next manifest and map schema together (v5 and its map),
carrying items rather than their bytes, and as little as reproduces them.*
- **An item entry:** a file that is an item of a line is named by that line (its shape, filter
  stack, ordering and key, written once in the header for all its items) and its number on it, or
  its bearing where a bearing names it exactly (the first unit at or past it). Its name (its title),
  its folder (its dimension) and its bytes all follow from those; nothing of them is stored. Its
  size is implied by the number (a fixed shape, or a binary file's length in its number).
- **Ranges:** many items in a regular pattern are one entry ("bearings 0 to 359, step 1"), so a
  sample such as §16.9's is a few hundred bytes, almost all of it the lines' settings, against the
  436,788 bytes v4 makes of it.
- **Files that are not items** are carried as v4 carries them, or by a shorter way where weighing
  finds one (a survivor number under a stack, `lines K`).
- **Hashes:** one SHA-256 over the whole tree, always; one per line, as a pointer to where a fault
  is (32 bytes each); per-file hashes off by default for items (they would be 32 bytes a file,
  104 KB of §16.9's sample, more than everything else) and kept for files carried as bytes. While
  the program that regenerates the items is unchanged, the arithmetic fixes every byte and so
  every hash; the hashes guard against a changed program (another ordering, a fixed encoder, a
  compiler). Where the tree hash fails, a command regenerates the items and lists each file's
  SHA-256, on the machine that made the instructions and on the one installing them, so the two
  lists show which file differs and how.
- **The cost moves to the installer:** it must carry the code that makes every line's items
  (text, PNG, MIDI, OBJ, books, tracks, movies), which `sieve` and the hallway already have; the
  instructions shrink, the program grows once.

**16.4 A file type filter on the binary line.** Filter the binary dimension by file types, read
from their headers.
*Evaluation.* Built: `binary-kind-v1` keeps (or excludes) the files of chosen kinds, read from their
own first bytes by the `file-kinds-v1` table (PNG, ZIP, MID, EXE, ...), counted and ranked exactly,
so the line compacts under it. Its `kinds` setting takes one kind or a group (signed, text,
signed-or-text, unknown, empty, any). What it lacks is a **set** of kinds (PNG and JPG together,
say). Verdict: extend the setting to a list.

**16.5 "Calculating Dimensions" where it is seen.** Move it to the top middle of the setup menu, in
red, and grey out ENTER THE HALLWAY while it is calculating.
*Evaluation.* Straightforward: the status line under the budget bars moves and turns red, and ENTER
is shown greyed (and refuses) while any line is being counted, as Go to it is greyed in the
locator when it cannot go. Verdict: build. *Built (10 October 2026):* "Calculating
Dimensions..." in red, top middle, always the same words, size and place; which lines are still being
counted stays under the bars.

**16.6 A slide-in window for a line's settings,** to free the background for new dimensions.
*Evaluation.* The setup menu shows every setting at once (the budget model needs to see them
change together). A slide-in panel, one dimension's settings at a time, frees the screen for more
dimensions and suits a growing list. Verdict: build when the dimensions outgrow the screen, which
worlds and an AI dimension would bring about. *Built (10 October 2026), ahead of those dimensions, as Edward
asked:* the whole list, GLOBAL down to ENTER THE HALLWAY, slides in from the left over where it
stood (Tab, or the SETTINGS tab at the left edge); while it is out the map takes the width; and
while the dimensions are being calculated it slides out by itself so the bars can be watched,
coming back when that is done. *Then (10 October 2026):* the settings above the three actions
scroll, so more of them, and more dimensions, fit.

**16.7 The worlds dimension, made of models.** *Evaluation.* Already the plan (§13, §14): a
composition of models, as tracks are of melodies, but a world also places each model (a position
and a turn) and textures it, so it needs a placement in each slot and UVs (§14's generated
description). The models line is not a unit line (its index is mixed radix), so composition-v1 does
not take it as it is. Verdict: build, after the models line can be composed. *Built (10 October
2026), first version:* `worldspace-v1` (SPECIFICATIONS §12.0) numbers the models itself, as
composition-v1 numbers units: a cover, a title and N slots, each a model of the models line in a
cell of a G x G x G grid with one of the 24 turns of a cube (Edward's choices: model and placement,
grid and 24 turns, 4 models and a cover and title). Untextured: textures wait for UVs (§14), as a
version of their own. In the hallway, WORLDS stands after MODELS; in the tool, `sieve world`.

**16.8 An AI dimension through Ollama, with a chat window.** Ollama does the heavy lifting; the
dimension's item is a chat with the model rather than a view of its file or its weights.
*Evaluation.* As an interface it fits: a room whose viewer is a conversation, sending requests to a
local Ollama and showing the replies. It is not a state space Ollama can address, though: Ollama
runs in floating point, so what it predicts can differ from one machine or version to another, and
an address must decode the same everywhere (§15's hard part). So the chat is a viewer, and §15's
pinned model line is what would name text under a model. Ollama could still supply probabilities to
weigh text by (on COST, as an estimate), never to address it.
- **(a) Filters from RLHF constants:** the preferences a model was tuned to; not available to us,
  so an idea for later, as Edward says.
- **(b) A noise gradient as the primary filter for model weights:** weights made procedurally follow
  smooth, noise-like gradients, so ordering the weight space so that such weights come first would put
  coherent models nearer the front. The useful core of this is a **prior over weights**: trained
  weights are not evenly spread (they are near zero, roughly normal, low rank, repeated), and an
  ordering that predicts that names real models in fewer bits, as quantisation and low-rank
  factoring do in practice. Verdict: a guided ordering for the model line, with that prior. *Measured
  (10 October 2026)* on SmolLM2-360M-Instruct: §15, "The first model, taken apart". *Built
  (10 October 2026), first version:* `sieve-weights-v1` (SPECIFICATIONS §12.0b), one table of value
  frequencies for each kind of tensor and an exact integer coder (rANS). SmolLM2-360M-Instruct:
  476.7 MB for 724 MB (65.9%; xz 512.9 MB), rebuilt byte for byte. Measured on the way: a table for
  each tensor costs 4.6 MB to store and saves 0.7 MB, so the tables are by kind; the exponent given
  its row's typical scale saves only 0.3 MB, so v1 leaves rows and columns out. Later versions are
  measured against this one.
- **(c) The noise gradient as the filter for every dimension, boiled down to one byte that shifts
  everything forward by one;** content aware (shifting floating-point values rather than bytes for
  weights); searched over its range with the best value and shortest path so far kept, and a time
  limit per file. *Evaluation:* a shift by one, or by any agreed amount, is a rotation of the loop:
  every unit moves the same way, so it is §17.1's rotation transform. One byte chooses one of 256
  orderings, so the search can bring a file at most about 8 bits nearer the front, and the byte that
  says which costs those 8 bits (the counting argument above): on average nothing, for every
  dimension alike. Kept: the search harness (best so far, a time limit per file) serves 16.1(a) and
  the tailoring search. Verdict: the one-byte universal filter, no; the prior of (b), yes.

**16.9 A demonstration: the first item at every degree of every dimension, as one `.sieve`.** For
each dimension and each whole degree from 0 to 359, the item a bearing of that many degrees lands on
(the unit at `floor(N * d / 360)` of the line's `N`), saved as the file F saves, all in one folder
and made into Sieve instructions (v4). Even if the files are noise, it proves the system end to end:
the installer installs them, each checked against its SHA-256, and each one, opened with J or reached
by its bearing (X), is that item again.
*Evaluation.* Straightforward with what exists (bearings, F's files, `--installer`), and a good CI
check besides: the bearings and the files agree on every machine. The files are about the size of the
lines' units (a page 32 characters, a picture a small PNG, a file on the binary line up to its
length), so the instructions are small. Verdict: build, as `sieve sample --degrees` or a script.
*Built (10 October 2026)* in the hallway, so every dimension's file is the hallway's own:
`hallway --sample-degrees DIR`. The bearing names the first unit at or past it (the navigator's
rule). Each item is a file named by its title (Edward: the reconstructed item is a file with its
title as its name; the pictures are for looks), pictures one pixel a pixel, as J reads them. At the
default shapes: 3,600 items of ten dimensions (worlds included), 1,055,836 bytes, as a `.sieve` of
436,788 bytes. CI makes it twice and
installs it, and checks a bearing typed in the navigator against it. What it showed: at whole
degrees the units repeat (every 40 or 72 degrees); on the lines without a cover the titles tell the
items apart, and on those with one (books, audio, tracks, video, movies, worlds) the cover leads
the address, so the titles repeat as well (45 pairs a line) and " (2)" tells the files apart. Each file
is named, under its line, by three values (the line with its stack, its ordering, its number),
which is what the next manifest and map schema should carry instead of its bytes (§16.3).

## 17. Deprecated ideas

Ideas evaluated and set aside, kept with their reasons so the questions keep their answers.

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
