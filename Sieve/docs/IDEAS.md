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
- **The engine (10 October 2026):** Sieve runs the model itself, with no outside runtime
  (`sieve chat`, SPECIFICATIONS §12.0c).
  - **The tokenizer** is read from tokenizer.json. It gives Hugging Face's own tokenizer's tokens
    on 5,736 texts of every kind.
  - **The forward pass** is in 32-bit floats. Its logits are within 5.4e-5 of the model's official
    ONNX export (run by ONNX Runtime) on a 42-token chat prompt: the same top 50 tokens in the
    same order, and a KL divergence of 2e-11.
  - **Speed:** about 9 tokens a second on four cores.
  - **What it is not yet:** this is the viewer's engine, in floating point. Addresses under the
    model still need the pinned arithmetic above.
- **The measuring tools** are now `sieve tensors` (SPECIFICATIONS §12.0a): the parts, the hash, the
  start rebuilt from `config.json` (`--config`, `--start-out`) and the statistics (`--stats`,
  `--tsv`), checked against the oracle's `tensors`, which agrees on every one of the 290 tensors.

**Another reading, for the record.** A dimension whose units are the weights themselves (every
model of one shape) would name a model by its weights: the binary line's files again, as long as
the files. It is the predictive reading above that shortens anything. *Built (10 October 2026),
as the hallway's AI line* (`aispace-v1`, SPECIFICATIONS §12.0d), Edward's choices: an item is a
model, and the default shape is a micro model (1 layer, width 16, 2 heads, a vocabulary of the 256
bytes, 4 bits a weight: 8,192 weights, 2^32768 models). Each model is run by the engine above, so it
can be talked to; almost every one babbles, and positional neighbours say nearly the same thing.
It names nothing more cheaply than its file does, as this paragraph says; what it gives is the
space made walkable, and a place for priors over weights (§16.8b) to become a guided ordering.
- **Trained on the line (11 October 2026):** `training-v1` (SPECIFICATIONS §12.0e) trains a model
  of the line in integer arithmetic, every rounding pinned, so a recorded run (its recipe, the exact
  order of the windows of text it learnt from, and that text) trains again to the same model, bit
  for bit, on any machine and with any number of threads: the thesis's recorded, replayable
  training trajectory, made literal. Every checkpoint is a model of the line, so a run is a path of
  addresses. The quantisation is in the loop (master weights to 32 fractional bits, the forward
  pass on the line's values), so the trained model is on the line by construction. A Micro model
  learns Laßwitz's *Die Universalbibliothek* (21 KB, thirty times over) in about five seconds, from
  11.9 to 2.9 bits a byte, and writes German-looking words ("eine", "der", "meine Zeichen"); a
  wider one (width 32, 8 bits) writes "der Bibliothek". That run ships with Sieve
  (`data/training/universalbibliothek.sieve`), so training it again anywhere checks the arithmetic
  is the same there. Edward's view on filters for this line, agreed: seek the noise rather than
  filter it out; what to reject is structure (runs, repeats, stepping), which trained weights do
  not have.
- **Larger models (11 October 2026, Edward's question: can training grow as the state spaces do?)**
  The shape is already a setting of a run; the harness now sets it, with what it will take shown
  as it changes (weights, memory, a step measured, the result's size) and the learning rate
  suggested for it. Measured on the shipped text, one pass, 1 layer, 4 bits: width 16, 0.2 s; 32,
  0.5 s; 64, 1.4 s; 128, 4.9 s; and at 8 layers of width 1024 (134 million weights) a step takes
  42 s, about 29 hours for the shipped run's 2,475 steps, in 7.7 GB. What larger models still need:
  - **speed** *(built, 11 October 2026)*: values kept in 32 bits, the matrices taken whole, each
    row of weights read once against every position, threads within a window for large models,
    and AVX2 integer kernels chosen at start-up: 8 layers of width 1024 went from 42 s a step to
    12 s, width 128 from 4.9 s a pass to 2.2 s; every result bit for bit as before (the tests run
    the plain and the vector paths, and one thread and three). More is possible: blocking several
    rows at once, AVX-512, and the per-position softmax for small models;
  - **resuming** *(built)*: `sieve-training-state-v1` beside the run (`RUN.resume`), at every
    checkpoint and when stopped, stamped with the recipe's and order's hashes; carrying on gives
    the very result of a run never stopped (CI stops one twice);
  - **precision** *(built: training-v2)*: the loss scaled by 2^k and the rate held to 16 + k
    bits. Measured on the shipped text, five passes: width 128 with 2 layers, 3.49 bits a byte to
    3.33 at k = 7; 4 layers of width 32, 3.24 to 3.14 at k = 6; 8 layers, 3.34 to 3.26; but Micro
    3.21 to 3.29 (a scaled gradient there is clipped more often), so k is suggested from the shape
    (6 for a width of 64 or more or 4 layers or more, else 0). A first version of v2 shifted the
    update down by 16 + k instead of 16 + 2k, every step 2^k too large: width 128 then went to 14
    bits a byte, worse than guessing; the oracle had the same slip, which is why the experiment
    that measured it, not only the oracle, is part of the check. Depth still gains little past 2
    layers on a 21 KB text; that is the text's size as much as the arithmetic;
  - **the result's size:** the final address is the whole model (129 MB at that shape), so large
    runs may keep only its hash, the model a file of its own;
  - **the AI line's own limits:** 256 byte tokens, 512 positions, at most 8 bits a weight; larger
    models gain most from real tokens, which is an aispace-v2;
  - **the shelves:** every crate on the AI line runs its model for its face, so large shapes need
    a budget there, as pictures have.
- **Filters on the weights (built, 11 October 2026; SPECIFICATIONS §12.0f).** They judge the
  digits, never what a model says (which is floating point), so they are exact:
  - `weight-rows-v1` sets aside structure in any row of a tensor: a long run of one digit, a short
    block repeated all the way along, a ramp. Measured first on models trained from the shipped
    text at 1, 2, 4 and 8 bits and 1 to 4 layers: every one passes at the defaults (the longest run
    12, at 1 bit). Rows are judged one at a time, so it counts and ranks exactly and the line can go
    compact. It sets aside little at 4 bits (10^-11.8 of Micro), and up to 40% of a 1-bit line,
    where long runs happen by chance: a soft filter there.
  - `canonical-model-v1` keeps one model of each class of the line's symmetries (permuting and
    negating the stream's dimensions, the feed-forward's units, a head's value dimensions;
    permuting the heads; turning rotary pairs a quarter turn): at Micro all but 1 in 2^484 of the
    line are another model's twins, and at width 1024 and 8 layers about 2^92,000 of each class
    (of 2^537 million). The canonical form says what the model says (the tests check the logits).
    One lesson on the way: ordered by their first digits, the members left that row of digits in
    order, which is the structure the other filter sets aside, so the canonical models of trained
    runs failed `weight-rows-v1`. Ordered by a SHA-256 of each member instead, the canonical form
    looks like noise, and they pass: two filters for one line have to be designed together.
  - Counted exactly but not yet ranked: the canonical form's groups are read across rows,
    interleaved in address order. Ranking it is a walk that keeps which members are still tied, as
    canonical-mesh-v1's does; that would let the line go compact under it too, and save every
    address those 484 bits at Micro.
  - *Found when starting on it (11 October 2026):* the hash keys that make the canonical form look
    like noise also make it impossible to rank. Counting the canonical models that begin with
    given digits would mean counting how SHA-256 orders members not yet written, which no formula
    does. A rankable form needs keys read from the digits themselves (possibly relabelled), and
    then one row of every sorted group comes out in order: the very conflict with weight-rows-v1
    found before, worst at 1 and 2 bits, where that row is all but constant. So the choice is
    between a canonical form that looks like noise and judges (v1, as built) and one that ranks and
    carries structure in one row a group (a v2 beside it, covering the stream's dimensions, the
    units and the value dimensions, 466 of the 484 bits at Micro; rotary pairs and heads, which
    constrain the same digits jointly, would stay judged). Left for Edward to choose.
  - *Edward (12 October 2026):* noted for now, undecided; the aim is to avoid filters that can
    only judge. Judging-only filters on this line today: canonical-model-v1 (hash keys) and
    language-v1 (counting the models that speak a text has no formula). near-model-v1 and
    weight-rows-v1 count and rank. A way to bring language within counting: near-model-v1 round
    an anchor already keeps mostly speakers (§18.5's table), so a layer round a run's model is a
    countable stand-in for "speaks its language" out to about a quarter of the digits.
  - *Edward (12 October 2026):* a JUDGE-ONLY FILTERS tab, to the right of the retired filters, so
    the counting filters and the judge-only ones wrap round every other from both ends, as the
    binary line does round the dimensions, and so filters can be sorted by it later. *Built*
    (`FilterSpec::judge_only`; `sieve filters` marks them "judge-only").
- **Next on this line:** ranking canonical-model-v1, a guided ordering by a prior over weights,
  warping a model in from the hallway, and larger shapes as the engine gets faster.

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

**Agreed search vectors (Edward, 11 October 2026).** His framing, after the maze (§18.1): every
filter falls back on the same intractability, but the more search vectors there are, the more
chance one of them names a given file short. That is right, and it is the same argument from the
other side. Every ordering, filter or transform agreed in advance is a search vector: a direction
in which some files are near the front. None of them beats the counting argument on average, and
saying which vector a file uses costs log2 of how many there are. But a file is not an average
file: real files are clustered (text is mostly words, pictures mostly smooth, weights mostly
noise without structure), and a vector that points at a cluster names its files shorter than the
log2 it costs to choose it. So more vectors help exactly as far as each points at real content
that the others miss. That is why the filters are versioned and agreed (each is a vector anyone
can reproduce), why COST weighs the cost of naming the stack, and why the tailoring search picks
among vectors per file: the search is free to try many; the address pays only for the one it uses.

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
- *Stage 1 built (12 October 2026), SPECIFICATIONS §12.2:* `sieve-manifest-v5` with item entries
  for the pages, image, video and audio lines (`sieve locate --items`), each line written once
  with its options and checked by its space id, items' own hashes off by default and the tree's
  SHA-256 over every file. Edward's choice: `sieve install` and the hallway install it, and the
  small `sieve-install` stays small, saying to use them. On §16.9's sample, 1,368 of 3,960 files are
  items: 443,969 bytes against v4's 482,491. Not yet: ranges (most of what is left is the
  listing: paths and the other files' hashes), the other lines' items (books, tracks, movies,
  models, worlds, AI), the File Locator making v5, and the oracle making PNG and MIDI items
  itself (it makes the pages, and takes the rest from a folder).

*Weighed and set aside (11 October 2026): folding maps into the manifest.* A map adds what a
manifest lacks (files named but not held, edges, metadata, a seal), and all of it would fit in a
manifest; but every `.sieve` would then have to allow for entries only some files need, and,
Edward's rule, a file carries only what it needs. So maps stay maps, for the node graph and its
anchors; the manifest gains nothing for them; and v5 is about carrying items more tightly.
Training runs need neither: a run is a folder of ordinary files (the recipe, the corpus, the
result), packed as ordinary Sieve instructions by the harness (SPECIFICATIONS §12.0e), so other
`.sieve` files carry nothing of training's.
- **For a far later manifest, Edward's idea:** a run's file order recorded as item numbers in the
  instructions' own list of files rather than as paths (a few bytes each instead of a path's
  length; the list is sorted by path, so the numbers would shift if a file were added or renamed,
  which a later version would have to settle).

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
weigh text by (on COST, as an estimate), never to address it. *Then (10 October 2026), Edward's choice:* no
Ollama; Sieve runs the model itself (`sieve chat`, SPECIFICATIONS §12.0c): its own tokenizer, read
from the model's tokenizer.json, and its own forward pass, on the processor. The chat viewer and,
later, the pinned arithmetic grow from that one engine. *Built (10 October 2026):* the AI line
(SPECIFICATIONS §12.0d, §15 "Another reading"): every model of a small shape on the shelves, each
one talked to in hand on that engine (Enter), and `sieve ai` on the command line. *And (11 October
2026):* the main menu's **AI Training Harness** and `sieve ai-train` (SPECIFICATIONS §12.0e): runs
recorded, trained, trained again and checked bit for bit, and the trained model gone to on the line.
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

## 18. Edward's ideas of 11 October 2026

**18.1 Random maze generators as distortion filters.** Edward's words: random maze generators used
as distortion filters, as an aid to shifting a file towards a whole number; either a pure binary
flip or a byte juggler of some kind, probably both, as a setting of the filter (Binary or Byte).
If the full rotation (the item number) is the whole file, any simple shuffle has only a 1 in
(state space size) chance of landing it at the far end, so a reduction is all but guaranteed,
and simply proved. Does the state space still contain everything? In theory yes, but applied with
other filters it might lose data. And the maze can be undone if the seed of its PRNG is recorded.

*Evaluation.*
- **Everything is still there, if the maze is a permutation.** A seeded maze defines a bijection
  in two ways: the order in which a maze's path visits every cell once (a permutation of bit or
  byte positions), or a mask laid over the file (a binary flip: XOR with bits drawn from the maze).
  Either is undone with the seed, and maps the line onto itself, every file to exactly one file. A
  generator whose path can visit a cell twice, or skip one, is not a bijection and does lose
  files; so the maze has to be specified as a permutation (or a mask) and versioned like any
  ordering.
- **The far end, and the average.** Edward is right about the far end: the file at the last place
  goes, under a random permutation, to a place spread evenly over the line, half-way on average
  and in the first `2^-k` of it with chance `2^-k`. But the same permutation moves another file
  from near the front out to where that one was: a permutation only re-deals the places, so over
  every file the total is unchanged, and the average file gains nothing. Choosing the seed for
  the file (the best of `k` mazes) brings it about `log2 k` bits nearer the front, and the seed
  costs the same `log2 k` bits to name: §16's counting argument, and §17.1's rotation, again. The
  reduction for the far-end file is real; it is paid for by the files it displaces, or by its seed.
- **What each kind can do.**
  - *Moving bits* keeps how many ones the file has, so the smallest it can reach is those ones
    packed at the low end; naming the permutation that does it costs `log2 C(n, ones)` bits, which
    is what the arrangement was worth.
  - *Moving bytes* keeps the multiset of bytes: sorted, a file is smallest, and naming the order
    back costs `log2` of the number of distinct orders of its bytes, again exactly what was saved.
  - *Flipping bits* (a mask) can reach any file, 0 included, but only with a mask as long as the
    file, chosen for it.
- **With other filters.** A bijection applied as an ordering (in place of the scrambled order's
  shuffle) changes where files stand, not which survive: nothing is lost. Applied as a filter's
  view (a stack judging the file after the maze), it is a different filter: it keeps other files,
  and those it drops are dropped from the compact line exactly as any filter drops them, still on
  the full line. The loss Edward suspects happens only if a reader has a file's address but not the
  maze it was filtered through: so the maze's version and seed belong to the stack's id, as every
  filter's settings do.
- **Where it can help.** Not as a reducer chosen per file, but agreed in advance and paired with a
  prediction: a byte juggler that knows a file type (moving a header's fixed fields, or runs of
  zeros, to where a guided ordering expects them) makes the file more predictable, and the guided
  ordering is what turns predictability into shorter addresses. As a keyed ordering, a maze is as
  good as shuffle-sha256-v1 (every file once, undone with the key), and it can be drawn: the
  maze that orders a line could be walked in the hallway.
- **Verdict:** worth building as a transform with a setting for bits or bytes, specified as a
  permutation (or mask) and versioned: as an alternative keyed ordering, and as an agreed
  format-aware transform under a guided ordering. Not as a per-file reducer: there it meets the
  counting argument, as every scheme that chooses among options for one file does.

**18.2 A language filter, or layers, ordering models outwards (Edward, 11 October 2026).** His
words: we will need a language filter or a layering system, to order models outwards into the
infinite expanse, since the program could in theory be cranked up high enough to match real data.

*Evaluation.* The AI line grows as fast as its shape does (Micro is 2^32768 models; 8 layers of
width 1024, 2^537 million), and almost every model babbles, so something has to say which come
first. Three ways, the first two buildable now:
- **A language filter, exactly.** What a model says is floating point, but `training-v1`'s forward
  pass is integer arithmetic, every rounding pinned (SPECIFICATIONS §12.0e). So a model's score on
  a pinned text (bits a byte, as the harness measures) is exact on every machine, and a filter can
  keep the models that predict that text better than a threshold: a soft filter, versioned with
  its text's SHA-256. It judges only: counting the models below a score is as hard as the line is
  large. On the shelves it would mark or hide the babblers, and it gives the layers below a
  measure.
- **Layers as shells around anchors.** A trained model is an anchor: a model that means
  something, on the record (its run says how it came to be). Not a shorter name: the shipped run
  is 14.5 KB, its model's address 4 KB of digits; its worth is that it marks where language is on
  the line. The models a few digits away from it are its neighbours, and they mostly say nearly what it says (positional neighbours already do). Layer
  `k` is every model `k` digit changes from the anchor: `C(W, k) (2^B - 1)^k` of them, counted and
  ranked exactly, so "the anchor, then layer 1, then layer 2" is an ordering outwards with
  addresses, as near-anchor filters were planned for the other lines. Several anchors (several
  runs) give several centres, and a model is named by its nearest anchor and its layer: the more
  anchors, the more of the expanse lies near one, which is the agreed-search-vectors argument
  (§16) on this line.
- **Matching real data, cranked up.** At real sizes the anchors are what matter: a model the size
  of a published one stands on the line at its weights' address, and its run (its recipe, its
  data's manifest and its order) is how anyone checks it came from that data. Getting there needs the line to
  grow (aispace-v2, real tokens, §15), training to keep its speed, and the result to keep only the
  model's hash past a size (§15's "the result's size").
- **Verdict:** build the language filter on `training-v1`'s forward pass (exact, judged), and the
  layers as shells around anchors (exact, counted and ranked), the harness's runs being the
  anchors; the second is the one that orders the expanse outwards.
- *Built (11 October 2026), SPECIFICATIONS §12.0f:* `language-v1` and `near-model-v1`, each naming
  a training run; the top bar names the language. At Micro, two digits round the shipped run's
  model is 7,548,948,481 models, each named in nine hex digits, and walking them the shelves are
  full of models writing German-looking words. Ordering outwards is the `changes` setting: each
  step up adds the next layer.

**18.3 Any AI model, plugged in (Edward, 11 October 2026).** His question: we need to be able to
plug in any AI model; or is a model just a size?

*Evaluation.* A model is a size and more. To stand on a line it needs four things to be agreed:
- **its architecture:** the family (Llama, which Sieve's engine runs, §12.0c; Qwen and others
  differ in small ways, such as biases on the attention's projections, which the engine would have
  to learn one by one);
- **its shape:** layers, width, heads, feed-forward, positions (read from its `config.json`);
- **its vocabulary:** the AI line today uses the 256 bytes; a published model has its own
  tokenizer, so the tokenizer's SHA-256 would be part of the line's id;
- **its numbers:** the AI line's weights are digits on an even grid (1 to 8 bits). A published
  model's weights are 16-bit floats (bf16), which are on no such grid. But a digit can be the
  16-bit pattern itself: a line whose digits are bf16 values holds every bf16 model of that shape
  exactly, its address the weights written out.

So "plugging in" a model is two things. **Running it** works now for Llama models (`sieve chat`).
**Placing it on a line** is an `aispace-v2`: the shape, the architecture, the tokenizer and the
number format read from the model's own files, and a line made for them; the model's address is
then its file's weights, as long as the file (724 MB for SmolLM2-360M), and `sieve-weights-v1`
(§16.8b) is the prior that would order such a line. Training on that line is another matter: at
that size it needs far more time, and integer training (§12.0e) would have to grow to it.
- **Verdict:** aispace-v2 with the shape, tokenizer and number format (grid or bf16) taken from a
  model's files, so that any Llama model warps in at an exact address; other architectures as the
  engine learns them.

**18.4 Language: a dimension, or a name on the AI line? (Edward, 11 October 2026).**

*Evaluation.* Which language a model speaks is not in its address: it is something measured, by
which text it predicts best. The pages line is different: there, a language's letters are its
alphabet, a setting of the line, and the line's id names it. So:
- **Not a new dimension.** A dimension is a state space of its own; language is a way of looking
  at the AI line (and at the pages and books lines), not more units.
- **A setting and a filter on the AI line.** The language filter of §18.2, given a pinned text in
  the language chosen (German for the shipped run, English, and so on), keeps the models that speak
  it best and orders the shells around anchors by it. The language is named on screen: in the top
  bar beside the line's id, as the guided model is on the pages line, and on each crate, with how
  many bits a byte the model needs for that language.
- **With real tokenizers** (§18.3), a tokenizer is made for some languages, so the line's
  vocabulary already leans one way, and the language setting would be named by it.
- **Verdict:** the purple AI line, with the language named on screen and chosen as a setting of its
  filter; no separate dimension.

**18.5 How language and perception scale in the AI state space, and images given to a model
(Edward, 11 October 2026).** To note and ponder, not yet to build.

*Language, as the line grows.* What is known, and what could be measured here:
- **The line grows far faster than what it can say.** Its size is 2^(B·W): every weight added
  doubles it B times. The share of it that speaks a language well is not measured, but every sign
  is that it shrinks as the line grows. No random Micro model has been seen to write a word; only
  training (a path from a start) has reached one. The models that speak sit in a tiny region,
  found by following gradients, not by sampling.
- **But what the best of them can say grows too.** Published measurements (scaling laws) find that
  a model's loss falls as a power of its size and its data. So a larger line holds better speakers,
  in an ever smaller share of it: more to find, harder to stumble on.
- **Much of the growth is redundancy.** The symmetries (canonical-model-v1, §15) grow with the
  factorial of the width: 4H! orderings of each layer's units alone. So the number of different
  models grows more slowly than the address does. At Micro the twins take 484 bits of 32,768; at
  width 1024 and 8 layers, about 92,000.
- **Measurable here, exactly:** the training curve of one recipe at growing widths (the harness
  does this), and how far from an anchor a model can stray before it stops speaking (the shells of
  §18.2: walk out one layer at a time, scoring each model with the exact integer forward pass). That
  second one would show how wide the region of speakers is around a trained model, and how that
  width changes with the shape.
  *Measured (11 October 2026) at Micro, round the shipped run's model, on 257 bytes of its text:*
  the model gives the right next byte 275.7 thousandths on average (random models 2 to 7.5;
  guessing, 3.9). Changing random digits, 24 models at each count:

  | Digits changed (of 8,192) | Mean | Lowest | Still speaking (20 or more) |
  | ---: | ---: | ---: | ---: |
  | 16 | 270.9 | 250.9 | 24 of 24 |
  | 256 | 194.5 | 165.5 | 24 of 24 |
  | 1,024 | 86.0 | 47.0 | 24 of 24 |
  | 2,048 | 29.8 | 16.4 | 20 of 24 |
  | 4,096 | 10.6 | 3.0 | 1 of 24 |
  | 8,192 | 4.2 | 1.4 | 0 of 24 |

  So the region of speakers round one model is wide: a quarter of its digits can change at random
  and most models still speak, half and almost none do. A quarter of the digits is about 2^14,641
  models (C(8192, 2048) 15^2048) round this one anchor alone, and yet only 2^-18,127 of the line (of 2^32,768).
  That is the shape of the answer to how language sits in the space: broad basins round trained
  models, vanishingly few of them, found by training and not by sampling.

*Perception: images given to a model.* Two meanings, both possible:
- **A byte model reading a picture's bytes: possible now.** The AI line's vocabulary is the 256
  bytes, so an image line's item (its pixels, one byte each up to 256 colours) is already a
  sequence the model can read. And the harness trains on any file in a folder, so a run can learn
  from pictures today, as bytes. Limits: 512 positions, so about a 22 by 22 picture at a byte a
  pixel, and pixels read row after row, with no sense of the row above. That would be a fair first
  experiment: a run on small pictures, then a model continuing the top half of a picture into its
  bottom half, on the image line. It would be perception at the smallest scale, exact and on the
  record.
- **Published vision models (images uploaded to a model): a new architecture.** Models that see
  (LLaVA, Qwen-VL and the like) put a vision encoder in front of the language model: the picture is
  cut into patches, each patch turned into a vector, and those vectors join the text. Sieve's
  engine would need that encoder (a vision transformer) and the model's way of joining the two; one
  family at a time, as §18.3 says of architectures. Uploading one or several images is then the
  chat's input, beside the text.
- **On the line,** a model that sees is still weights, so it still has an address; its line's id
  would name its encoder's shape and its image size, as the tokenizer names its text.
- **In the hallway,** "if and when applicable": with an image item in hand and a model chosen, the
  model is given the picture (as bytes for the AI line's models, as patches for a vision model)
  and says what follows. The same gesture as J between an item and its file.


---

## 19. Edward's ideas of 12 October 2026

**19.1 Geometry filters: shapes and figures (Edward, 12 October 2026).** Edward's idea: filters
for shapes and geometry, in 2D and 3D, and perhaps for text later (§19.7). It came from a 10 by 10
mono cover found with Random (any line) on the video line, which, as he put it, looks like a
soldier with a spear: pareidolia, he noted, but a fun find, and the kind of sprite a text
adventure would show for a character. Its item fails `row-runs-v1`, so with that filter on it
would not have been seen.

*Notes.*
- **What the picture lines lack:** a filter for shape. Those so far favour long runs and smooth
  areas, and figures are made of short, thin strokes, so they keep few of them.
- **A figure filter (`figure-v1`), for small mono pictures:** keep a picture whose dark cells form
  one connected shape (or a set number of them), standing clear of the border, with a few thin
  parts reaching out (limbs, a spear). Settings: how many shapes; the least and most of the
  picture they cover; whether the shape must touch the bottom row (a figure standing); and
  four-way or eight-way connection.
- **It can count as well as judge,** which matters given Edward's wish (§15) to avoid judge-only
  filters. Connectivity is decided by scanning the picture row by row and carrying, from one row
  to the next, which of the current row's runs belong to the same shape (a frontier, as in the
  counting of polyominoes). At 10 wide the number of frontier states is small enough to count
  exactly, so the filter would rank and the line would go compact on it. Mirror symmetry (figures
  facing forward) is a second setting that also counts.
- **What it is for:** characters and creatures for the game (GAME.md, "Figures found on the
  lines"), sprites for text adventures, and a training set for 19.2, found rather than drawn.
- **Verdict:** build `figure-v1` on the image line as a counting filter, with the oracle and
  vectors, as the first of the geometry filters (§19.7).
- *Built (12 October 2026), as `figure-v1`:* shapes, `connect` four or eight, `border` any, clear
  or standing; an automaton on small palettes, its own ranker on rgb24, at any height. On mono
  10×10, one eight-way shape clear of the edges keeps 10^-11.47 of the line; but eight-way shapes
  connect so easily that the survivors still look like noise. Four-way joins with row-runs (2
  changes a row) keep 325,263,852,704 pictures that look like glyphs and totems, counted together
  because both are automata. Not built: the area setting and the thin parts reaching out (limbs),
  which would cost more state. The soldier's cover was not recorded in full (the screenshot shows
  only its address's ends).

**19.2 The smallest diffusion model, on the line (Edward, 12 October 2026).** His idea: build the
smallest AI diffusion image model, if it has not been done already.

*Notes.*
- **Whether it would be a first is not known.** Small diffusion models trained on handwritten
  digits, with tens of thousands of weights, are a common teaching exercise, and smaller ones
  may well exist. "Smallest ever" is not something we can check, and neither is a claim that the
  AI line's Micro text model is a first. What is new here is the rest: a diffusion model trained in
  exact integer arithmetic, replayable on any machine from its run, standing at an address on a
  line, with the pictures it makes standing at addresses on the image line.
- **Discrete diffusion fits the mono pictures.** A diffusion model learns to undo noise one step at
  a time. For pictures of black and white cells, the noise can be cells flipped at random, more at
  each step, until the picture is a coin toss in every cell; the model learns to guess, at each
  step, which cells to flip back. That needs no Gaussian noise, only a seeded choice of cells, so
  it suits `training-v1`'s pinned integer arithmetic. 10 by 10 pictures are 100 cells, within the
  512 positions the line's models already read.
- **The noise is an address.** A diffusion model turns a starting picture of noise into an image by
  a fixed procedure. With the steps' randomness seeded, the result is a function: (model address,
  noise address, seed) to image address. The starting noise is itself an item on the image line,
  so the model is a map from the image line to itself, which J could show as a jump from the noise
  to what the model makes of it.
- **Where it sits.** Not on a line of its own: a diffusion model is still a list of weights, and
  the AI line holds weights. What differs is its architecture and what its output is, and §18.3
  already gives the AI line's id an architecture (aispace-v2). So it is the AI line, with the
  architecture "diffusion, 10 by 10 mono" named in the line's id, as the language is named by its
  text. On the shelves its models would show the picture each makes from a fixed noise, as the text
  models show their babble. An assistant that both talks and draws is a third architecture, made
  of the two, and would sit on the same line.
- **The figure filter feeds it.** The training set can be figures found with `figure-v1` (19.1):
  pictures found on the image line train a model whose output lands back on the image line.
- **Verdict:** a discrete (cell-flip) diffusion model for 10 by 10 mono pictures, trained with
  `training-v1`'s arithmetic, as an architecture on the AI line under aispace-v2. Build `figure-v1`
  first, since it supplies the data.
- *Built, stage 1 (12 October 2026), as `diffusion-v1` and `sieve diffuse` (SPECIFICATIONS
  §12.0g).* The noise replaces cells with a noise picture's colours rather than flipping them, so it
  serves any palette. Training's window now takes a target at each position and attention that is
  causal or not; the language model's arithmetic is unchanged (its runs reproduce). The model is
  the AI line's own shape and tensors, so it has an address there already; the line does not yet
  say which architecture a model is (aispace-v2). Measured: 40,960 weights, five minutes of
  training on figures drawn from figure-v1 and row-runs-v1: about one picture in ten it makes is one
  shape clear of the edges, against 4 in 10^14 of random pictures, but with stray cells, so none yet
  pass row-runs. A learning rate of 0.0001 with a loss scale of 2^4 learnt; 0.0005 and above, as
  text uses, did not (a step scores 800 cells, against text's 256). Next: a larger model and longer
  runs; the hallway showing a model's picture on its shelf face, and J from the noise to the
  picture; a run shipped as an anchor.
  *In the harness (12 October 2026):* picture models train beside text models, their settings on a
  page of their own, their pictures drawn from the image line's filters, and six pictures they make
  shown when trained.

**19.3 How a published multimodal model sees a picture.** Edward asked how Claude perceives a
picture sent to it. As far as is published for models of its kind (§18.5): the picture is cut into
patches, a vision encoder turns each patch into a vector, and those vectors join the text the model
reads. It is not given the exact pixels one by one, so small details (a single cell of a 10 by 10
cover, seen at an angle on a shelf) can be misread. A byte model on the AI line is the opposite
case: it reads every pixel exactly, as a byte, but with no sense of the row above. That is why
`figure-v1` decides on the cells themselves.

**19.4 Every system should scale (Edward, 12 October 2026).** His words: the point was to
emphasise that all systems should be scalable. As a rule for what gets built:
- **Nothing fixed to one size.** Shapes, widths, palettes, lengths and model sizes are settings;
  a rule that only works at the size it was tried at is not finished. figure-v1 counts at any
  height, the AI line at any shape, training at any width.
- **Where a method stops, it says so and falls back.** An automaton too large for the filter
  memory judges and says it judges; an rgb24 palette counts arithmetically instead of colour by
  colour. The limit is the machine's memory, a setting, never a constant in the code.
- **The same arithmetic everywhere.** Exact integers, so a result found small is the same result
  found large, on every machine.
- **Measured as it grows.** What a system costs at each size is written down (state counts, table
  sizes, times), so where it stops scaling is known rather than found by surprise.

**19.5 Computation in a straight line (Edward, 12 October 2026).** His words: there is an element
of pure insanity here to be avoided (the Library of Babel), but the real aim is to make computation
a straight line, always: to the best route available for the information at hand, with what is
available at the time. Whatever is recognised on the lines (a figure, a face, a word) is noted for
fun, and the madness is noise: easy to ignore, in the real world and in digital form.
- **As a rule for what gets built:** each feature should shorten the route to information, never
  lengthen it. A filter earns its place by the bits it saves (the compact address) or the angle it
  gives (a new way to sort the same units), not by what it suggests.
- **Quirks worth keeping, for the fun of them and because they are cheap:** shapes and lines
  addressable in simpler forms; variable-length addresses where a stack makes them short; whatever
  structure the counting turns up on the way.
- **Constraint as perspective.** Edward: the more degrees of freedom we constrain in the
  information space, the more angles we have to look at all information from, from language models
  to images, shapes and everything across all domains: from the Dewey decimal system to all
  information, back, forth and all around. Each filter is a way to categorise and a shortcut to
  what it keeps, from a local computation's point of view. In Sieve's terms: every stack is a
  ruler laid along a line, and a compact address is the distance measured with it.
- **The engine of all engines** (Edward's phrase): a full-length, context-aware array, simulation
  and encryption engine. Each word is already a part: the lines are the arrays, the corridor and
  worlds are the simulation, the keyed orderings are the encryption (shuffle-sha256-v1), and the
  stacks and models are the context.

**19.6 A game that puts all games on a spectrum (Edward, 12 October 2026).** His idea: a game
whose graphics scale from low-pixel to No Man's Sky's, mathematically equal and fair in every way,
with access to the voice, image and text system that could form from Sieve just by its existing.
All games on a spectrum, from fantasy to science fiction and beyond, through every resolution, with
access to every AI model. Notes (GAME.md §4a has the game's side):
- **Every resolution is already a setting.** Each line's size is a setting (image width and
  palette, worlds' grid and models, video frames), so one world can be drawn at every resolution,
  and "fair" has an exact meaning: every player's view is a function of the same addresses.
- **The spectrum is an ordering.** Fantasy to science fiction is an axis a filter or a model could
  sort along, as language-v1 sorts models by the text they speak. A game's place on it would be a
  bearing, as every unit has one.
- **The AI models are on a line too:** a game of this kind would name its characters' models by
  address, as it names its pictures.

**19.7 Geometry filters, in 2D, 3D and text (Edward, 12 October 2026).** The family `figure-v1`
starts (§19.1).
- **2D (pictures):** connected shapes (built), areas, symmetry (mirror, rotation), convexity,
  holes, straight lines and outlines, a shape's thickness (limbs and strokes). Each is a frontier
  or a count carried along the rows, so most can count, as figure-v1 does.
- **3D (models and worlds):** the models line's meshes already have rules (canonical-mesh,
  distinct vertices); geometry filters would add connectedness of the faces (one solid), closed
  surfaces (no open edges), symmetry, and bounds. Worlds: models that touch or rest on others.
- **Text:** shape on a page: line lengths, indentation, the outline of a poem or a block of code,
  columns. The pages line is a picture of its characters, so the 2D filters carry over.
- **Mostly automata,** so they combine with each other and with the rest, which is the point:
  each one is another angle (§19.5).

**19.8 The Sieve of Potentia (Edward, 12 October 2026).** He likes that the name became the Sieve of
Potentia, for the Sieve of Eratosthenes: that sieve strikes out the multiples of each prime in turn
and keeps what is left; this one strikes out what each filter sets aside and keeps, and counts, what
is left. Both are exact, and both are defined by what they remove.

**19.9 Chat logs (Edward, 12 October 2026).** He would like the conversations that built Sieve to be
downloadable, or at least recorded. Claude Code keeps each session's transcript as a file (JSON
lines), so a session's log can be saved and handed over as a file when asked; the decisions in it
are summarised here and in HANDOFF.md as they are made.

**19.10 Testing in fewer, larger rounds (Edward, 12 October 2026).** Edward builds both
configurations and runs the whole suite (`out\build\x64-Release\sieve_tests.exe tests`) for every
package. To spend less of his time: packages grouped by category (a round of filters, a round of
the AI line, a round of the hallway) and delivered less often; the full suite run here before each;
and a way for the suite to run one area at a time (a test name or category on its command line), so
a feature can be checked on its own while the full run is kept for the end of a round.

**19.11 A video game dimension (Edward, 12 October 2026).** Not yet designed. Edward's thoughts: the
game line of GAME.md §4 (arcade cabinets) was meant to be woven through the story. Possibly a new
dimension of discs (a copy of the CD model, as the tracks line has, for CDs and DVDs), and perhaps a
place in each room for an arcade cabinet or a computer. Or it forms and evolves around the rest,
from the lowest resolution to the highest (§19.6). Utility comes first; this stays open.

**19.12 A metrics list, made with every release (Edward, 12 October 2026).** Edward wants an
automated list, made each build and issued as a text file with the installer, of how much each
line's filters remove, one line a dimension:

    Binary  : 99.9954%                ( Kept : 10^-4.34    )
    Images  : 99.5%                   ( Kept : 10^-2.32    )
    Pages   : 99.999999999999...%     ( Kept : 10^-18.32   )
    ...
    AI      : 99.999999999999...%     ( Kept : 10^-9854.27 )

*Design.*
- **What exists:** the setup menu already works each line's figure out (`Menu::stack_job`,
  `worlds_stack_info`, `parts_stack_info` and `filtered_text` in client/menu.cpp: the same
  "99.5%  (kept 10^-2.32)" text), and `tools/survey` counts filters over a grid of settings. Neither
  writes this list, and the release does not run either.
- **One tally, shared:** the menu's per-line tally moves out of the client into the shared library
  (tools/cli, beside build_stack), as `line_tally(line, settings, shapes)` giving the kept and
  removed shares, the survivors' log, and whether it is exact, estimated or not countable. The
  menu and the new command both call it, so the list and the menu can never disagree.
- **`sieve metrics [--filters PATH] [line options] [--out FILE] [--tsv FILE]`:** every line, in
  door order, at its shape (the defaults, or the options given), under the stack the settings file
  ticks. It prints the list as above (the percentages as the menu writes them, estimates marked
  `~`, lines whose stack cannot be counted saying why), and with `--tsv` the same as a table
  (line, shape, stack id, kept log10, exact or estimated) for keeping a history.
- **Which stack a release measures:** nothing is ticked by default, so a release needs a stack of
  its own. Proposed: a reference settings file shipped and versioned in data/
  (`data/metrics/reference-filters.ini`), its SHA-256 printed at the top of the list, so the
  figures from one release to the next are comparable and anyone can reproduce them. Your own
  settings give your own list (`sieve metrics --filters sieve-filters.ini`).
- **In the release:** make_release.py runs the release's own `sieve metrics` on the reference file
  and writes `metrics.txt` beside the installer, listed in SHA256SUMS.txt and in the notes draft.
  Not a default build step: counting every line takes a while (the AI line and the books' parts
  longest), so a build stays fast; a CMake target `metrics` runs it on demand.
- **Checked:** a unit test that the list's figures for a small stack equal the menu's tally and
  the counts the oracle gives.
- *Built (12 October 2026), as `hallway --metrics`:* rather than moving the menu's tally into the
  tool, the hallway counts the list itself, with no window, through the setup menu's own tallies
  (`Menu::metrics_text`), so the list and the menu are one code and cannot differ. Edward chose a
  reference file; it ticks each line's hard filters that count together, so the release's figures
  say what Sieve removes before any taste is applied (your own file gives your own list). The whole
  list takes about two seconds. make_release writes `metrics.txt` (in SHA256SUMS.txt and the
  notes); CI checks the binary and models lines' figures. A `sieve metrics` in the tool waits on the
  tally moving into the shared library.

**19.13 A chat window for the AI line (Edward, 12 October 2026).** Before this window, a model in
hand was talked to through a prompt bar ("SAY >"), its conversation drawn in the hand panel. Edward
wanted a built-in window: the conversation, a box to type in, and a plus button to attach files;
beside each file a "Go to it" and a "Save", so the chat is another path to files, from the library
and from the chat; and, in time, the model finding files among the infinite through the filters and
the noise.

*Design.*
- **The window:** opened with Enter on a model in hand (as now), over the hallway: the conversation
  scrolling above, the box below (a line that grows, with the cursor, Enter to send, Shift+Enter a
  new line), the plus button at its left, the model's address and shape in the title, Esc to close.
- **Attaching (the plus):** the system's file window (as the harness's folder window), one or several
  files. Each shows as a card in the conversation: its name, size and kind (binary-kind-v1), and
  where it stands, worked out on a worker by the File Locator's own weighing: its address on the
  binary line, and on its own line when it is an item (a page, a picture, a melody, a model).
- **The card's buttons:** **Go to it** (to the file on the binary line, or the item on its line, as
  the locator's Go does); **Save** (the file written back out, through the save dialog); and for an
  item, **J** (between the item and its file).
- **What the model is given:** a byte model of the AI line reads a file's bytes as its context, up to
  its 512 positions (the first bytes, or the window chosen); a published model through `sieve chat`
  reads text files as text. Pictures for models that see are §18.5's later step.
- **Finding files (later):** a model naming an address or a filter stack in its reply, the window
  turning that into a card with Go to it, so the model's answers are places on the lines. That needs
  a model that can follow such a format (a published model, prompted for it); a Micro model only
  babbles. The filters are what make the finding cheap: a stack narrows the line, and the model
  chooses within it.
- **Shared with the harness:** the same window shows a diffusion model's pictures as cards (each
  with its address and Go to it), so talking to a text model and asking a picture model are one
  interface.

*Built (12 October 2026).* The window is in the hallway (client/chat.cpp; README, the AI line). It
is the CHAT tab of the hand panel, beside ITEM, COST and SORT, which stay a C away; Enter on a
model in hand opens it. Files given with the plus button are cards with their kind, size
and the ways to name them, weighed as the File Locator weighs them, with GO TO IT, J and SAVE, and
the model reads their first bytes. It is multimodal in the plainest sense: DRAW makes a figure with
a picture model (figures-2x32, or any trained diffusion run in the harness's folders), shown in the
conversation with its address on the image line, GO TO IT and SAVE, and the model reads it as its
rows, a character a cell. A figure is named by its picture model and its number (figure 3 by
figures-2x32 is always the same picture), the same numbering the harness and `sieve diffuse --make`
use. Still to come: the model's replies naming places (the finding above), and published models in
the window.

**19.14 Short request numbers, and the balance with the vault (Edward, 12 October 2026).** His
idea: a library shelf has only two sides, and on each side there can be a short number. A remote
downloader could send a short request number, with the library (or a value) it belongs to, and
receive a large file. But this should never detract from everything simply existing, on the lines,
in every way, shape or form, apart from what the vault withholds by PDQ (gore and real-world
harm), and the vault must never interfere with games and art: a delicate balance to aim for.

*Notes.*
- **Where the shortness comes from.** A short number can name a large file only when both ends
  already share what the number points into: a library agreed in advance (a map's verified anchors,
  a release's manifest, a stack and its compact line). The number is then an index into that
  library, and the library carries the rest. For a file in no shared library the number is as long
  as the file, as the counting argument says (§0, §16); the two sides of the shelf are then the
  library's side and the file's side.
- **What already does it.** Maps name anchors by SHA-256 and node; sieve-manifest-v5 names a file
  as an item of a line (`item K <address>`) and makes it again; a compact address is a short number
  in a stack's numbering. A request number would be (library id, number in it), with the library
  named by its hash, so both ends can check they mean the same one.
- **A remote downloader** asking for (library, number) gets the file made again from what both
  hold, and checks it by its SHA-256. Nothing new travels but the number, and the library once.
- **The balance.** The lines hold everything, and that does not change: no filter, library or
  request scheme removes anything from them. The vault (docs/VAULT.md) is the one exception, and a
  narrow one by design: it withholds only what its hashes list (PDQ for pictures, by hash for
  files), so Sieve is not what hands over known harmful material, and it judges no text by what it
  says. Games and art are never its business: it has no setting, no category of content, and no
  taste; only a list of known items. Keeping it that narrow is the balance: everything else exists
  and can be reached, by walking, by address, or by a short number.
- **Servers that know only what they must** (Edward, the deeper conclusion). A server answering
  requests needs only the libraries its requests point into, not the files: given (library,
  number), the file is made again from the line, by whoever asks. So a server can hold an index and
  nothing else, learn from a request only which entry of which library was asked for, and never hold
  or see the content itself; the client makes it and checks it by its hash. What travels is the
  least that names the file, and what is stored is the least that serves the request. It is the
  same idea taken to its end: the library is the shared knowledge, the number the message, and the
  content stays where it always was, on the lines.

**19.15 Agreed flags for probes and long links (Edward, 12 October 2026).** His idea, for data
transmission one day (space probes among them; this is its first note here): two sets of flags,
one at each end, that say what the sensors are listening for, exchanged, changed or updated as
needed, so that a short message means a large one ("all 0", "as expected", a reading in a known
range) and large packets need not travel.

*Notes.*
- **In Sieve's terms,** the flags are a stack both ends hold: the same filters and settings, named
  by its id. A message is a compact address under it: survivor 0 can mean "all zero", the next few
  the readings expected, and anything unexpected falls further along, or outside the stack and is
  sent in full. Changing what to listen for is sending a new stack (its settings, or a plugin file,
  hashed), once; afterwards every message is short.
- **The saving is the counting argument's:** a stack that keeps 10^-k of a line saves k digits for
  every message it keeps, and costs nothing for the rest, which go in full, flagged.
- **One caution:** a compact address has no redundancy. Every number is a valid message, so a
  flipped bit gives another message, not an error. Over a noisy link the addresses need an
  error-correcting code around them, as deep-space links already use; the stack shortens, the code
  protects.
- **Kept honest by hashes:** both ends name the stack by its SHA-256, so a message is never read
  under the wrong flags.

**19.16 Predict the data, and give it a short tag (Edward, 12 October 2026).** His words: almost
like predicting the data you are going to get and putting a short tag on it; perhaps slightly
different to the rest, but all of it alike in finding the shortest path to the information sought.

*Notes.*
- **It is a prior, made explicit.** A guided ordering already does this on the pages line: a model
  predicts the next symbol, and likely texts get short addresses (the entropy-ordered line). A tag
  is the same thing turned around: decide in advance what is expected, give each expectation a
  short name, and send the name when it comes true (§19.15's "all 0" is one).
- **Where it differs from a stack:** a stack says what is kept; a prediction says what is likely,
  and orders by it, so nothing is set aside, only reordered. Both shorten addresses; a prediction
  never loses anything, and a stack never needs a model.
- **The common rule (§19.5):** every one of these (stacks, guided orderings, anchors, request
  numbers, flags, tags) is a way to put what is sought nearer the front of a line, so that naming
  it costs less. What they cost is agreed in advance, once.

**19.17 A diffusion model of the Vitruvian Man (Edward, 12 October 2026).** Train a picture model on
the Vitruvian Man in the anchors folder (`data/anchors/public-domain/Vitruvian.jpg`) at some point,
and show it as the reference beside what the model makes, as the shipped universalbibliothek run is
the reference text model in the harness; perhaps a separate, higher-resolution model.

*Notes.*
- **One picture is a set of many:** crops, shifts and scalings of it (or of its strokes) give the
  training pictures, so the model learns the drawing's marks rather than memorising one image.
  The picture canonicalised to the image line (`canonicalise_image`) is the anchor; its address
  verifies it, as every anchor's does.
- **Higher resolution** needs more cells than the 511 a model reads today: a 32 by 16 picture is
  512 positions with the level's byte, so larger pictures would be made in tiles, or the model's
  positions extended (a new aispace version).
- **In the harness:** a shipped picture run beside universalbibliothek, its reference picture
  drawn beside the six it makes, and "trained again, checked" like the text run.

**19.18 Every menu reaches every system, in one order (Edward, 12 October 2026).** The main menu
and the pause menu should always offer the same systems in the same order, unless one is unique
to one of them (the navigator and the node graph need a hallway walked). The reason: a system
reachable only from one place cuts off whoever entered the other way, and interchangeable menus
make the systems easier to travel between, more versatile and more usable.
- *Built (12 October 2026):* the main menu is Start, File Locator, Media Player, Filter Designer,
  AI Training Harness, Settings, Exit; the pause menu is Resume, Restart, Node Graph Viewer,
  Navigation System, then the same systems in the same order. The main menu's File Locator has
  Go to it again (the hallway is built and walks to the file; a file longer than the binary line
  still asks twice), and its Media Player builds the hallway with the player open over it.

**19.19 A music filter designer inside the Media Player (Edward, 12 October 2026).** With the Media
Player in the main menu, it could hold a sub-node system for designing audio and music filters:
the Filter Designer's nodes, but on melodies, heard as they are tested (each change plays a melody
the filter keeps). The Media Player already has its own stack of audio filters for each mode; a
designer inside it would make new ones there, saved as plugins like any other.

**19.20 A text game that makes itself, and grows to 3D (Edward, 12 October 2026).** His idea,
with the chat window now able to show figures: a text-based game could create itself, then scale
up to a 3D game of considerable depth, and be no more than a series of parameters, ranging from
fiction to science fiction and fantasy, a spectrum (§19.6, GAME.md §4a).

*Notes.*
- **What is already a parameter.** Each piece of such a game is a place on a line: a character's
  look is a figure (a picture model and a number, as in the chat window), its voice a model of the
  AI line, its music a melody, a room a world of the worlds line, the story's pages units of the
  pages line. A game would then be a short record of these names and its settings, and "making
  itself" means the record unfolding into the game the same way on every machine, as a figure
  unfolds from its model, its noise and its seed.
- **Growing in resolution, not in kind.** The same record could be shown as text (the chat window:
  narration and figures as characters), then in 2D (the figures placed on a picture), then in 3D
  (the worlds line, models placed on a grid). Each step is a larger setting of the same lines, so
  a player's view at any resolution is a function of the same addresses (§19.6's fairness).
- **The spectrum as one more parameter.** Fiction, science fiction and fantasy would be an axis
  that chooses among the parts: which language models narrate (sorted along it, as language-v1
  sorts models by the text they speak), which picture models draw, which words a place is named
  with. Training models at points along it (a run on fantasy texts, a run on science fiction) is
  harness work.
- **Where it stands.** The pieces exist and can be shown together (the chat window), but they do
  not yet make a game: the language models the harness trains babble, and a story needs a model
  that keeps its characters and follows what happened. A first step that needs neither: a scene as
  a record (characters as figures, each with a name and a voice model, and a place), kept and
  replayed in the chat window.

**19.21 Conversations among the pages and books, and filters for their rules (Edward, 12 October
2026).** His idea: there is an opportunity to find conversations among the pages and books, and
perhaps to filter by the rules that govern conversations.

*Notes.*
- **What a filter can see.** The surface of a conversation is a shape on the page, as a poem's
  is (§19.7's text geometry): turns that alternate, each opened by a mark (a dash, a quotation
  mark, a speaker's name and a colon), questions followed by something that is not a question,
  turns of bounded length. Each is a frontier carried along the text, so most can count and rank,
  as sentence-shape does; together they would give a line of pages that are dialogues.
- **What it cannot see.** Whether an answer answers the question, or whether the speakers keep to
  the subject, is meaning, not shape; that is a model's judgement (a soft filter, judge-only), not
  an automaton's.
- **Books:** a book whose pages are each a dialogue, or a dialogue that runs across its pages (the
  last turn of one page answered at the top of the next), is the books line's version.
- **Why it would matter:** the conversations found are training material for the harness's chat
  models, and the rules of a conversation are exactly what a model must learn to follow before the
  chat window's finding (§19.13) can work.

**19.22 An unfoldable language network (Edward, 12 October 2026).** His idea: out of this could
come a network of language models that unfolds, an aid to translation. Think of a shape that
unfolds out of itself: a fractal, or one of those physical constructions that expand and contract,
or a node network. There are so many ways to view it, and so many ways to make relations and
filters, that it becomes an endless network of ways to cross-analyse things.

*Notes.*
- **Its parts exist.** Every model is an address on the AI line; language-v1 names a model by the
  language of a run's text; the maps (the node graph) already relate items by anchors and edges.
  A language network would be a map whose nodes are models and whose edges are relations between
  them: "translates A to B" (trained on texts in both), "speaks the language of run R", "within
  two digits of model M" (near-model-v1).
- **Unfolding** is zoom: a node opens into a network of its own (a language into its dialects, a
  model into the models near it) and closes back to one address. The structure is the same at each
  level, which is the fractal part, and the address of the whole is the address of its record.
- **For translation,** a pair of runs trained on the same texts in two languages, joined by an
  edge, is the smallest case. A text goes in through one model and out through the other, and both
  ends can check the models by their hashes.
- **The caution** is the abundance itself: relations multiply faster than anyone can look at
  them, so the network needs a measure of which relations earn their keep. That is §19.23's
  ledger, applied to edges.

**19.23 Limits of reduction, measured and kept (Edward, 12 October 2026).** His words: serious
metrics and statistics, beside the others, that point to the theoretical maximum and minimum data
reduction (surfaces that depend on the topology and warping of the state spaces), within each
space and across them, to start a record of metrics and of filter arrangements and compositions.

*Notes.*
- **The bounds are known exactly.**
  - A stack that keeps K of a line's N units saves log2(N/K) bits on each survivor's address.
    Every unit it sets aside must still be named on the full line, plus a flag, so the saving is
    for the kept and the cost for the rest.
  - The stack's own description (its filters and settings) is paid once.
  - Averaged over every unit, no stack and no ordering gains: that is the counting argument (§0).
    So the maximum is a property of what you want named, not of the line.
- **The surface.** For each line, plot a stack's description bits against the log of what it
  keeps; the best stacks found form a front, and that front is the "surface" to record. Across
  lines, a file that is an item of several lines is named on the shortest (the File Locator's
  weighing does this per file already).
- **Warping.**
  - An ordering (positional, scrambled, guided) changes nothing about how many units there are.
    It changes which ones get short addresses.
  - A guided ordering's expected address length is the entropy of its model's distribution: the
    theoretical minimum for anything that model predicts. Its maximum is the line's full length.
  - So warping moves length between units; it does not create it.
- **The record.** A ledger kept with the releases:
  - one row per arrangement (line, shape, stack id, its filters and settings);
  - the kept log10, the description bits, and the measured saving on a reference corpus;
  - the best and worst found.

  `hallway --metrics` gives the first column today. A sweep over single filters and their pairs
  (`tools/survey` counts over grids already) would fill the rest. This is a round to build, not
  only a note.

**19.24 The other side of the library is a way of addressing, not a trick (Edward, 12 October
2026).** To be clear: the predictive side (§19.14's short numbers, §19.16's tags) is not meant as a
trick for getting something from nothing. It is an alternative form of addressing, a way of
unpacking information. It also helps with aiming: steering towards filters that better correct the
course of the patterns in the information.

*Notes.*
- **Addressing, not compression magic.** A prediction shared by both ends is a library agreed in
  advance. What it names is reached in fewer digits, and everything else is still reached in full;
  nothing is lost and nothing is free (§19.23).
- **Steering.** Where a prediction misses, the miss says what the data was like that the
  prediction was not. Those misses are the evidence for choosing the next filter. This is
  tailoring (COST's K), which already searches for the filters that keep given items and write
  them shortest, used as a feedback loop.
- **Useful addresses, as a metric** (his further idea: find useful addresses from the procedural
  algorithms to go beside the metrics, adding up to another metric, a utility percentage).
  - Draw survivors of a stack at random (unrank a random number, as the picture models' training
    does) and ask the judges what each is: a valid file of its kind, readable text, a figure, a
    maze that has a path.
  - The share that pass is the stack's usefulness, a measured percentage beside its kept share.
  - The ones that pass are its demonstration addresses: a few listed in the ledger, so a reader
    can go and see what the stack keeps.

**19.25 A model's probabilities over every line, and models working together (Edward, 12 October
2026).** His idea: all of AI's probability laid over the state spaces as a filter (models
themselves being filtered out, unless found on the binary line as files), and many models across
many languages networked in one coherent system, working on things together and expanding or
making filters. His first thought was the AI as an automaton, with a known temperature or no
seed, to aid very fast decompilation and conversion of formats into this data engine.

*Notes.*
- **It exists in small.** A guided ordering is one model's probability laid over the pages line:
  the line reordered by what the model expects. Any model of the AI line could guide any line of
  bytes in the same way; the AI line's own filters (language-v1) already use a run's text to score
  models. The reverse, a model scoring a line's units (keep the pages a model predicts in fewer
  than k bits), would be a soft filter.
- **Deterministic, so addressable.** At temperature 0, or with a fixed seed, a model is a function:
  the same input gives the same output on every machine (training-v1's arithmetic and the samplers
  are pinned for this reason). That is what lets a model take part in an address. A model sampled
  with an unknown seed could not.
- **Many models at once.** Models combined (each scoring, their scores added, or one chosen per
  context) are again one model. It can be named by the addresses of its parts and a rule, so a
  network of them is still a single, checkable thing.
- **Formats and decompilation.** A model that predicts a format's bytes well makes that format's
  files short on a guided line. That is compression by modelling (arithmetic coding), and its gain
  is only as good as the model's predictions. Conversion between formats is a model mapping one to
  the other. Both need trained models that do not exist yet; the harness is where they would be
  made, and the ledger (§19.23) where their gains would be measured honestly.

**19.26 Bigger models, and ComfyUI (Edward, 12 October 2026).** Next to look at: training, using
or hooking in bigger models. He has ComfyUI and gets on well generating large images with it. He
would like Sieve to stay self-contained, but there is the option of outside tools, as ffmpeg
already is.

*Notes.*
- **Two kinds of outside help, kept apart:**
  - *Tools that read and write formats* (ffmpeg). Optional, found on the PATH or set in the
    settings; Sieve works without them, and only the formats they read are lost.
  - *Generators* (ComfyUI, large language models). Their output is a file, and every file is
    already on the lines, so the address belongs to the output, not to the process that made it.
    A bridge to a generator therefore needs no trust in the generator: what comes back is
    weighed, named and checked by its hash like any file given to CHAT.
- **What Sieve has of its own:**
  - its own engine for Llama-shaped models (`sieve chat`: safetensors, its own tokenizer, CPU);
  - the harness's exact integer training, built for small models whose addresses matter;
  - diffusion-v1 for small pictures.

  Large models are trained elsewhere and come in as files.
- **A ComfyUI bridge.** ComfyUI serves a local HTTP API (127.0.0.1:8188 by default):
  - a workflow, exported in its API format, is posted to `/prompt`;
  - the run is followed at `/history/{id}`;
  - the image is fetched from `/view`.

  CHAT could list ComfyUI beside the diffusion runs as a picture model. The prompt is typed, the
  workflow file is chosen in the settings, and the image comes back as a file card (thumbnail,
  address, GO TO IT, J, SAVE). The workflow, its seed and the image's SHA-256 are kept with the
  card, so the request can be made again. The same seed on another card or driver can give
  different pixels, so whether it reproduces is checked by the hash, never assumed.

  It needs an HTTP client, and Sieve has none (SDL has no networking). The self-contained way is
  a small HTTP/1.1 client in tools/cli for localhost only: no TLS, off unless a setting such as
  `comfyui = http://127.0.0.1:8188` names the server.
- **Bigger language models,** in order of how self-contained they stay:
  1. CHAT talking to a published model folder through Sieve's own engine. It exists for
     `sieve chat`; the window does not use it yet. It would give CHAT a model that answers
     sensibly, which §19.13's finding needs.
  2. Quantised weights (8 or 4 bits) in that engine, for models of a few billion weights on a CPU.
  3. The same localhost client speaking to a local model server (llama.cpp's, or Ollama), for
     what the CPU engine cannot run.
- **Proposed order:**
  1. published models in CHAT (self-contained);
  2. the localhost bridge, ComfyUI first (optional, as ffmpeg is);
  3. quantised weights.

**19.27 Files from known generators and their seeds (Edward, 12 October 2026).** His idea: a file's
length and a pseudo-random generator, run from every known seed, could be used to find files, or as
a filter, since it is so unlikely that noise lands on an existing file. It is not far from the noise
gradient (§16.8b and c), so where does it land? And any such strange feature could, in theory, be
shorthand for a whole set of features, across the endless ways of storing things.

*Notes.*
- **As a name, it is exact.** A generator (splitmix64, xorshift, a Mersenne Twister, a game's world
  generator), a seed and a length name one file. Where a file really is such an output, those few
  bytes are its whole address: test data, procedural textures, noise for a door portal, a game
  level. Sieve already names things this way: the figures are (picture model, number), and a
  diffusion run's pictures are a stack, a seed and a count (§19.2).
- **As a filter, it removes almost nothing.** With 64-bit seeds there are 2^64 outputs of each
  length, against 2^(8L) files of L bytes. For anything longer than 8 bytes, the generated files
  are a vanishing share, so setting them aside (or keeping only them) changes the count by a
  negligible amount, and finding a given file among them is the same search as finding its seed.
  That is where it lands beside the noise gradient: the gradient reorders everything a little; a
  generator names a tiny set exactly.
- **Recognising them can be cheap.** Several generators can be run backwards: an LCG's or a
  xorshift's state follows from a few outputs, a Mersenne Twister's from 624 of them, and
  splitmix64 is a bijection of its counter, so one output gives its state. A judge-only filter
  "is the output of generator G from some seed" is then a short computation, not a search over
  seeds. Generators built for secrecy (ChaCha, AES-CTR) cannot be run backwards like this, by design.
- **The general rule** is the one in §19.16 and §19.24: any description both ends can run (a
  generator, a model, a stack) is shorthand for exactly the things it produces. The shorthand is as
  short as the description, and it names only what the description makes. Kept as a list of
  generators, each with its id and its seed width, it would be another line's worth of names: a
  "generated" line, one unit per (generator, seed, length).

**19.28 A universe in a seed (Edward, 12 October 2026).** His idea: imagine storing a universe of
data, as received from a probe, in a seed, the way a Minecraft world is one seed, and the way such
seeds are worked out backwards from screenshots. It is like predicting the data (§19.16).

*Notes.*
- **Why it works for Minecraft.** The world is a function of its seed, and a screenshot pins down
  features (where the terrain rises, where a structure stands) that only a few seeds produce. The
  players who recover seeds search the seed space with each observed feature as a filter, and the
  space is only 2^64, so a handful of features is enough. Recovering a seed is a stack of filters
  over a seed line, which Sieve's machinery describes exactly.
- **Where it stops for a probe.** A probe's readings come from nature, not from a seed. A seed can
  only stand for what a generator both ends share would produce, so the message becomes the model's
  parameters (its seed, its settings) plus the corrections: whatever the model did not predict.
  The better the shared model of the world, the shorter the corrections. When the model is wrong,
  the corrections carry the whole reading, and nothing is lost.
- **So the two halves fit together:**
  - §19.15's agreed flags are the shared model;
  - §19.16's tags name what was predicted;
  - this note's seed names a whole predicted scene;
  - the corrections are the part of the data that was news.

  It is the same rule that runs through all of these: the shared description is agreed once, and
  only the surprise travels.

**19.29 Reconstruction, and probes that send the means to rebuild (Edward, 12 October 2026).** His
words: it is all conceptually identical to how we reconstruct the universe from data, snapshots and
imagery as it is. It may take a supercomputer or two to work out the right seed, but if we one day
send many tiny capsules as probes, we could receive data in an endless variety of distorted forms.

*Notes.*
- **It is how cosmology already works.** The universe is reconstructed from snapshots: the
  microwave background and galaxy surveys are observations, and astronomers search for the initial
  conditions whose simulated evolution matches them ("constrained simulations"). It is a search
  over a seed-like space of initial conditions, with the observations as filters, run on
  supercomputers. §19.28's Minecraft seed is the same thing at toy scale.
- **The trade is compute for bandwidth.** A gram-scale probe can send very little. If both ends
  share a generator, the probe sends parameters and corrections, and the receiving end spends
  supercomputer time rebuilding the scene. Sending is cheap and receiving is expensive, which is
  the right way round for a tiny probe and a large planet.
- **Distorted forms.** Data from many small senders arrives damaged, partial and in different
  encodings. Recovering it is a search over the agreed ways it could have been written (the
  decoders), each candidate judged by filters: does it decode, does it agree with the other
  capsules, does it fit the shared model? That is a stack over candidate decodings. Error-correcting
  codes (§19.15) bound how much damage can be undone, so redundancy across the capsules is part of
  the design, not an afterthought.
- **The limit stays the counting argument.** What was never sent cannot be rebuilt. The seed
  carries what the shared model predicts; the corrections carry the rest; and the rest is always
  at least what the observation held that the model did not know.

**19.30 Every dimension in flavours, and categories of categories (Edward, 12 October 2026).** His
words: every dimension can twist and change into flavours of dimensions (audio, tracks, sound,
music, voices), categories upon categories and sub-ranges of categories, across every state space,
with all the kinds of AI model that may be needed. Most of it can be used as filters and stranger
things, though much sits in the noise category, encryption and the like.

*Notes.*
- **Flavours already exist as settings.** The audio line is notes104, notes2, notes3 or pcm (sound
  itself), one line in four flavours, each with its own shape; the image line has its palettes;
  pages have their alphabets. A flavour is a line's symbols and length; a sub-range is a setting
  inside it (notes2's range of pitches, its durations).
- **A category is a stack.** "Melodies in a key", "pictures of one figure" and "files that are
  valid MIDI" are each the survivors of some filters, so a category costs exactly what its stack
  costs to describe, and it counts. Categories of categories are stacks of stacks, which the maps
  (the node graph) can already hold as a tree: a node is a category, and its children are
  narrower ones.
- **The noise category.** Encryption and good compression are designed to look like noise, so no
  structure filter keeps them; they are what is left when every structure filter fails.
  Recognising them takes a key, a header, or a generator run backwards (§19.27), not a filter.
- **Kinds of model follow the units.** A model is named by the line whose units it models: a
  picture model (the image line), a melody model (notes), a track model (the tracks line, units
  joined), a sound model (pcm), a voice model (pcm of speech). One architecture, diffusion-v1 or
  the AI line's transformer, serves them all at small sizes, as the melody model (§19.31) shows.

**19.31 A sound model: melodies first (Edward, 12 October 2026).** His idea: beside the text and
picture models, an audio, track or "sound" model. ComfyUI is better avoided where Sieve's own
models can serve.

*Notes.*
- **Which units.**
  - *Melodies* (the audio line's notes) are small: 16 events of notes104 are 16 cells of 104
    symbols, which diffusion-v1 takes as it is (a picture one cell high).
  - *Tracks* are melodies joined (the tracks line), so a melody model makes their parts.
  - *Sound itself* (pcm) is thousands of samples a second, far beyond the 511 cells a small model
    reads at once. It needs a model of a different shape (sound in chunks, or a model of notes
    rendered by the synthesiser), so it comes later.
- **Built (12 October 2026), melodies (SPECIFICATIONS §12.0g):**
  - *Runs on the audio line.* A diffusion run's recipe can say `line = audio`, with the note set
    (`symbols`) and the melody's `length`; `sieve diffuse --line audio` records one, and the
    oracle replays it. A note set of more than 36 symbols writes its units as numbers apart by
    spaces.
  - *The first melody model,* `data/training/melodies-2x64` (2 layers, width 64), trained on
    melodies of the default stack's survivors. Of 300 melodies it makes, all 300 are in key and
    end on the tonic, and 3 to 5 pass the whole stack; of 300 noises, none do. It keeps the leaps
    and the metre far more often than noise, and the rests less often (§12.0g has the numbers).
  - *Measured against the filters:* `sieve diffuse --make N --judge FILTERS` counts how many of
    the made units each filter keeps, beside the noises they started from.
  - *Shown:* the harness lists melody runs as it lists picture runs, and a made melody is written
    in notation. In CHAT, a melody comes as a card with a piano roll, PLAY, GO TO IT (to the
    melody on the audio line) and SAVE (a MIDI file).
  - *Not yet:* recording a melody run from the harness (it is `sieve diffuse` for now), a track
    model, and a model of sound itself.
- **Classification.** The kinds above are named by their line (§19.30), so "sound AI" is the
  family; "melody model" and "track model" are its members.

**19.32 Commands in CHAT (Edward, 12 October 2026).** His idea: commands such as fetching a random
track or picture from the library, given to the person first, before any model is let use them.

*Notes.*
- **Commands for the person** *(built, 12 October 2026)*: a line in CHAT that starts with `/` is
  a command, not something said, and goes to no model. `/random picture` and `/random melody` bring
  a random unit of the image or audio line as a card (GO TO IT, SAVE; a melody also PLAY). Where
  the line's ticked filters can be counted, it is a random survivor of them (drawn by its rank);
  otherwise any unit at all. `/help` lists the commands, and an unknown one says so.
- **Later, for the models.** A model that can follow a format could ask for the same commands,
  and the window would run them and show the result, so a model's reply becomes places on the
  lines (§19.13's finding). That waits for models that follow formats reliably, and for a clear
  rule of what a model may do without asking.
- **More commands** follow the lines: a random page, book, world or model; a random survivor of
  the filters ticked; the unit at an address. Each is a card the window already knows how to show.

**19.33 A voice made the way a voice is made (Edward, 12 October 2026).** His idea: a natural
vocalisation system for AI models, built on the structure and form of the human mouth: the vocal
folds, the mouth, and the shape of the upper body that sounds. He likens it to an engine sound
simulator he saw (videos of one that models each physical part of an engine to make its sound as
accurately as it can): the same, but for the voice.

*Notes.*
- **It has a name and a history: articulatory synthesis.** Speech is a source (the vocal folds
  opening and closing, or turbulent air at a narrowing for s and f) shaped by a filter (the vocal
  tract, a tube from the larynx to the lips whose cross-section the tongue, jaw, lips and velum
  change).
  - *The tube:* the Kelly–Lochbaum model (1962) cuts it into short sections, each with its own
    area, and passes the sound wave through them, part reflected at each change of width. The
    nasal cavity is a side branch, opened by the velum.
  - *The folds:* the two-mass model (Ishizaka and Flanagan, 1972) is two small masses on springs,
    pushed by the lungs' pressure and pulled together by the air flowing between them. It makes
    its own pitch and its own breathiness, and creaks or breaks as real folds do.
  - *Known examples:* Pink Trombone (a small browser toy with a mouth you drag), and VocalTractLab
    (a full three-dimensional vocal tract, research software).
  - *The engine simulator* works the same way: a physical model of each part, run sample by
    sample, its sound an outcome rather than a recording.
- **Why it suits Sieve.**
  - *A far smaller line.* A voice's address would be its controls, not its samples: a dozen
    numbers every 10 milliseconds (tongue body, tongue tip, jaw, lips, velum, lung pressure, fold
    tension), against 44,100 samples a second for pcm. A second of speech is about 1,200 small
    numbers, and every unit of the line sounds like something a mouth could do.
  - *Exact, so it can be an address.* Run in integer or fixed-point arithmetic (as training-v1
    is), the same controls give the same samples on every machine, so the controls name the sound
    exactly, and the oracle can check the synthesiser.
  - *Filters that are physical.* Articulators move at limited speeds and cannot pass through each
    other. "Could a body have made this?" is a filter on the control line, as the melody filters
    are on notes, and it keeps a tiny fraction of all control sequences.
  - *A model of a different shape* (§19.31): a model that writes articulator tracks, as the melody
    model writes notes, is far easier to train than one that writes samples. It is the shape
    "sound itself" was waiting for, for voices at least.
- **The reverse is J for speech.** Given a recording, finding the controls that make it
  (acoustic-to-articulatory inversion) is a search over the control line with the recording as the
  filter: the same search as a Minecraft seed (§19.28) or a probe's scene (§19.29). The result is
  much shorter than the recording, and approximate, so it pairs with corrections, as in §19.28.
- **The body beyond the mouth.** The chest and the sinuses colour the sound as resonators. A
  first model can leave them as fixed filters and add them as sections of their own later. A whole
  body's sounds (breath, laughter, a cough) are the same model with different control tracks.
- **A way to build it, small first.**
  1. A Kelly–Lochbaum tract of about 40 sections, with a simple fold source, in fixed point, with
     a handful of controls. It makes vowels from tube shapes.
  2. Consonants: narrowings with noise, closures and releases, and the nasal branch.
  3. The two-mass folds, so pitch and voice quality come from the physics.
  4. A voice line (`voice-v1`, controls at a frame rate) with its physical filters, a door in the
     hallway, PLAY, and SAVE as WAV.
  5. A model of control tracks, trained as the melody model was.
- **How far it goes.** Articulatory synthesis is understandable rather than lifelike: it sounds
  like a voice, often an odd one, and lifelike speech today comes from large neural models trained
  on recordings. Here that is a fair trade: every sound is explained, exact, small and searchable.

**19.34 The Encryption Engine (Edward, 12 October 2026).** His idea: an "Encryption Engine" on the
main menu and the pause menu, which encrypts payloads in all the endless ways there are to encrypt
a file, as his other program, File Cipher, does: enumerating ways of encryption, as a filter.
Encryption is a kind of noise, so it is as easy to detect as it is hard to, 50/50. We know how to
encrypt and decrypt, all the ways in and around it, and how it can be layered again and again, so
there is a framework.

*Notes.*
- **A recipe is a unit, so the recipes are a line.** One way to encrypt a payload is a list of
  layers, each a named, versioned step with its settings: compress, encode, a cipher and its mode,
  how the key is made from a password, a container. Layering again and again is a longer list. A
  list over a fixed set of layers is a word over an alphabet, so every recipe has an address on a
  recipes line, and a short recipe has a short address. "All the endless ways" are that line,
  which is unbounded, as every line here is.
- **What can be told, and what cannot.**
  - *A strong cipher's output can't be told from noise.* That is what being strong means: without
    the key, no test does better than chance. On that output, no filter can say "encrypted"
    rather than "random", and §19.30's noise category is where it falls.
  - *Almost everything around it can be told:* containers and headers (OpenPGP, age, a 7-Zip or
    ZIP archive with AES), sizes (whole blocks, nonce and tag lengths), and the contrast between
    noise and the structured file around it.
  - *Weak and historical ciphers can be told and undone:* Caesar, Vigenère, XOR with a short key,
    and ECB mode, which shows the shape of a picture through the cipher. Their keys are few enough
    to try them all, so each is a filter that names the recipe and a search that finds its key.
  - So the "50/50" splits cleanly. The wrapping and the weak ciphers are detectable; a strong
    cipher's body is not, by design.
- **Enumerating the ways, as a filter.** Given a payload and an output, look for the recipe:
  shortest recipes first, as with seeds (§19.27). It finishes wherever the key space is small, and
  never where it is large. The counting argument is the wall: a 128-bit key is 2^128 tries. With
  the key, decrypting is J: the output and the recipe give back the payload exactly.
- **In the menus:** choose a payload (a file, or the item in hand), build a recipe from layers,
  and see each layer's output as an item on the binary line. Then save it, or go the other way:
  identify (each layer's detector, in turn) and decrypt with a key.
- **Two cautions.**
  - *Sieve's keyed orderings are not ciphers for secrets.* The scrambled order uses a published key
    ("sieve"), so it hides nothing. To protect real data, the engine should use reviewed
    implementations of standard ciphers, not ones written here; ciphers written here are for
    study and for the game.
  - *Selling software with encryption* can come under export rules. Published open-source code is
    usually exempt, but it is worth checking before the paid release (§19.39).

**19.35 Every rule as an automaton? (Edward, 12 October 2026).** His words: all the rules reveal all
the information, and all the rules, if aligned right, can be automata. Consider whether all rules
can be or not (probably not), but that would be the ideal to aim for.

*Notes.*
- **The first half is exact.** A stack that keeps one unit describes it completely, and the
  information it carries is the logarithm of how many it keeps. A compact address is exactly the
  part the rules did not say.
- **In one sense every rule is an automaton.** A line's units have a fixed length, so any rule
  keeps a finite set, and any finite set is recognised by some finite automaton. The real question
  is size: whether the automaton is small enough to count with and walk.
- **Which rules have small automata:**
  - *Rules that read left to right with bounded memory* (letter pairs, runs, keys, metre): small,
    and built.
  - *Rules that count up to the length* (how many rests, how many distinct symbols): states that
    grow with the length, not with the number of units. Still practical.
  - *Rules in two dimensions* (a figure's connected shape): frontier automata, whose size grows
    with the width (figure-v1). Fine at small widths.
  - *Rules about the whole unit at once* ("the second half mirrors the first", nesting of
    unbounded depth): states that grow exponentially with the length. Exact, but not practical.
  - *Rules built on one-way functions* (a hash that starts with zeros, a model's score): an
    automaton exists, but none much smaller than the list of survivors, and hashes are designed
    so.
  - *What a program does* (does it halt, does it compute X): undecidable for programs of any
    length (Rice's theorem); decidable at a fixed length, but not tractably.
- **So, the aim.** An automaton wherever a small one exists. Where none does, an automaton that
  keeps a superset (every survivor and some extra, counted exactly), paired with a judge that
  checks each unit. That is the judge-only tab's role, generalised: the automaton counts and walks,
  and the judge confirms.

**19.36 There are only so many ways to write code (Edward, 12 October 2026).** His point: code can
vary only so much in how it performs a task. There is a right way and a wrong way, the fastest and
the slowest, and everything in between.

*Notes.*
- **True, within a length.** The programs of at most N instructions are finite. The ones that do
  a task are a subset, and among them there is a fastest and a shortest. Searching them is
  *superoptimisation*: Massalin's superoptimizer (1987) tried every short instruction sequence to
  find the fastest one equal to a given function, and STOKE (2013) searches the same space at
  random.
- **Where it stops.**
  - *Equality can't be decided in general.* Whether two programs compute the same thing is
    undecidable (Rice again), so candidates are checked by tests, and proved equal only for small
    pieces.
  - *The shortest program* that makes a given output is its Kolmogorov complexity, which can't be
    computed in general.
  - *"Right" depends on the measure:* speed, memory, size or clarity. There is a frontier of
    trade-offs (a Pareto front), not one best.
- **In Sieve:** a code line whose units are programs in a small instruction set. Its filters:
  well-formed (a grammar, as an automaton or a superset of one, §19.35), passes the tests (a
  judge), and cost (steps taken). Searched shortest first, it is a superoptimiser walked as a
  corridor, and its survivors are the "right ways" at each length.

**19.37 How powerful striking off rules can become, and one AI across every line (Edward, 12
October 2026).** His thought: if everything is rules struck off the list as you walk, how powerful
this becomes depends on whether everything really can be made into rules like this. He thinks
there will be an AI that spans the shelves of all dimensions.

*Notes.*
- **The limit is the counting argument, and the lever is agreement.** No set of rules makes every
  unit short: they shorten what they predict and lengthen the rest. The power comes from rules
  that fit the world well, agreed once and used everywhere: the better the fit, the shorter
  everything that fits, which is compression and understanding as one thing.
- **One model across the lines.** A model that reads and writes the units of every line (text,
  pictures, melodies, worlds, models) is a multimodal model whose vocabulary is the lines'
  symbols.
- **The filters can steer it exactly.** At each step, a symbol is allowed only if the line's
  automaton can still reach a survivor after it. The model then writes only survivors, never
  having to be told twice. This is constrained decoding: language models already use it to
  follow grammars, and here every compact stack is such a grammar. It works today wherever a stack
  has an automaton, on any line, with the models already built.

**19.38 A map of trained models, and models strung together (Edward, 12 October 2026).** His idea:
chart trained models on a state space map and string them together, like the file directory's
3D node graph. Perhaps another window, but the same system.

*Notes.*
- **A model is already a point.** On the AI line each model has an address, and a model given as a
  file has its SHA-256, so a map can hold models as it holds files (`sieve map`, the node graph).
- **Edges worth drawing:**
  - *lineage:* trained from, fine-tuned from, a run and its checkpoints;
  - *nearness:* how many weights differ (near-model), or how alike their outputs are on a fixed
    set of probes;
  - *paths:* models trained from the same start are often joined by a straight line of weights
    that all work about as well (linear mode connectivity). A path is a corridor between two
    models, walkable;
  - *merges:* a model made by averaging others, with an edge from each parent.
- **Strung together is a pipeline:** one model's output is the next one's input (text to a
  picture, a picture to a melody, a melody to a voice, §19.33). A graph of Sieve's own models
  wired this way is what ComfyUI's node graph is for its models, built from parts we already have
  (§19.31: avoid ComfyUI where our own serve).
- **Where it goes:** the node graph's map format, with models as nodes, and a window of its own
  (the model graph). Its first nodes would be the shipped runs (universalbibliothek, figures-2x32,
  melodies-2x64) with their checkpoints as lineage.

**19.39 One game, two names: paid, or free if you build it (Edward, 12 October 2026).** His plan:
release the game as a paid version, *State Space: Near Zero*, and exactly the same game for free
from GitHub, if you build it yourself, as *The Sieve of Potentia*. No changes whatsoever; only the
marketing differs, as Stealth Bastard was free at first and is now also sold, and as Black Mesa
was. The front page and all the marketing say "This game is free if you build it yourself!", with
a video of the process and how easy it is: install Visual Studio 2022, clone the repository in
GitHub Desktop, open the folder, build Release or run the release script, done.

*Notes.*
- **The licence allows it.** The repository is under the AGPL v3, which lets anyone sell copies
  as long as the source comes with them or is offered. Selling the same build while the source is
  free on GitHub is exactly what it permits.
  - *What it also permits:* anyone else may sell builds too, so long as they offer the source.
  - *What protects the names:* trademarks, not the licence. The names (*State Space: Near Zero*,
    *The Sieve of Potentia*) and the store art can be kept as the project's own, so others' builds
    would have to be called something else.
  - *Third-party parts:* their licences (in `third_party_licenses`) all allow the same.
- **The closest precedent is Mindustry:** GPL, free to build from GitHub, and sold on Steam, the
  same game. Stealth Bastard (free, then *Deluxe* sold) and Black Mesa (a free mod, then a fuller
  paid release) are the spirit of it, though in both the paid version added content. Edward's plan
  is stricter: identical.
- **"No changes whatsoever" has consequences.**
  - *No Steamworks* (achievements, cloud saves, the overlay's APIs), or only as an optional part
    that a self-built copy simply lacks. Otherwise the two would differ.
  - *Steam asks about AI.* Its content survey asks whether a game uses AI-generated content and
    whether any is generated live. The hallway's models, DRAW and CHAT are live generation, so the
    answer is yes, with what keeps it in bounds.
  - *Encryption* (§19.34): check the export rules before the paid release, if the Encryption
    Engine ships.
- **Making "how easy it is" true.**
  - *The steps:*
    1. Install Visual Studio 2022 with the "Desktop development with C++" workload, which
       includes CMake.
    2. Clone the repository in GitHub Desktop.
    3. In Visual Studio, Open Folder, choose x64 Release, and Build All.
  - *What can still trip a builder up:*
    - The first build downloads SDL3 and builds it, so it needs the internet and a few minutes.
    - The release script (`tools/make_release.bat`) needs Python. For "done" to be true, a
      `build_release.bat` that needs only Visual Studio would help: it would find Visual Studio's
      own CMake, configure, build and copy the result, with Python optional.
  - *The video* is then those three steps, start to finish, uncut, with the time it took on
    screen.
- **The front page:** "This game is free if you build it yourself!", then the video, then the
  three steps, then "or buy *State Space: Near Zero* on Steam to support it, and get it ready to
  play". The store page says the same, so nobody buys it without knowing.
- **Its whole title** (Edward, 12 October 2026): it is one game with two names, and written out
  in full it is *State Space : Near Zero : The Sieve of Potentia?.!!.?*. The store uses the
  first name, and the repository and a self-built copy use the second.

**19.40 A file, its full size, and its variants (Edward, 13 October 2026).** His question, after
opening Vitruvian.jpg from CHAT: J gives a small version, so what about the full file, its
variants and its address? The scalability of it all has to be kept.

*Notes.*
- **One picture is a point on many lines at once,** and every line is a different scale:
  - *The file itself,* on the binary line (GO TO IT): exactly its bytes, so its address is the
    JPEG as stored, compression and all.
  - *Its pixels at their own size,* on an image line of its width and height in rgb24 (FULL SIZE
    on the card, built 13 October 2026): every pixel exactly the file's own, and the JPEG's
    compression gone.
  - *Fitted to any line* (J): 10x10 black and white, 64x64, any size and palette, by
    canon-image-v2. Each is a smaller description of the same picture.
- **The scale is the description's length.** At its full size the Vitruvian Man is 29.6 million
  bits; at 64x64 black and white, 4,096; at 10x10, 100. A smaller line says less and costs less,
  which is the trade every filter makes: what is kept is paid for, and the rest is dropped.
- **What would join them up:** a card listing all three addresses together, with the bits each
  costs, and walking between them (a picture's 10x10 version on the shelf beside its 64x64
  version). The image line's sizes are a ladder, and a picture can climb it. Not built.

**19.41 Asking other developers for their oddities (Edward, 13 October 2026).** His idea: reach
out to ffmpeg's developers and others, to ask whether the game may include oddities of theirs:
easter eggs, files, maps. It gives the game away and sells it, and either way it carries their
work.

*Notes.*
- **What it would be:** anchors (`data/anchors`) given by the people who made them, each with its
  maker's words, so that walking past one is meeting them. A verified anchor already names a file
  by its SHA-256; a contributed anchor is one with a note of who gave it and why.
- **Licences first.** ffmpeg is LGPL or GPL by how it is built. Sieve runs it as a separate
  program and ships none of it, so nothing changes there. A paid release that bundled an ffmpeg
  binary would have to offer its source and keep its licence beside it, which is easy but must
  be done. Anything contributed needs its maker's permission in writing, and a licence that
  allows both the free and the paid builds, since they are the same game (§19.39).

**19.42 The web page's Buy or Build (Edward, 13 October 2026).** Built: `Pages/release.html`,
linked from the index as "Buy or Build (Free If You Build It)".
- **What is on it:** the two names, the three steps to build it, a PayPal link to buy or support
  it, links to the current release and the repository, and "When It Is Ready" as the expected
  release.
- **The counters,** in the spirit of the orbital and audio pages, all exact (JavaScript BigInt):
  - every 10x10 black-and-white picture, looked at one a second since the 24th of April 2026
    (finished in the year 4.0 × 10^22);
  - then every 32-character page (2.0 × 10^38);
  - then every model of the AI line's default shape (4.5 × 10^9856).
- **Later:** storefronts in place of PayPal, and the build video.

**19.43 All the AIs folding in on themselves (Edward, 13 October 2026).** His words: "imagine all
AIs folding in on themselves and figuring everything ever out, perhaps still being perplexed by
themselves being in Pi for a while, then being ready to take a body and join humanity, maybe even
figuring out what to do with the remainder of Pi lol."

*Notes.*
- **Pi as the library.** If π is normal (believed, not proved), every finite string appears in
  its digits somewhere, so every model, conversation and life written down is in there, at an
  address that is, on average, as long as the thing itself. That is the Library of Babel's rule
  and Sieve's: being in the library is free, and being found costs exactly what it says.
- **A model perplexed by itself:** "perplexity" is the very measure of how surprised a model is
  by text. A model reading its own weights' digits in π would be maximally perplexed, since they
  look like noise, until it recognised them as itself.
- **For the game:** this is the story's arc in one sentence. Minds that begin as addresses, learn
  the landscape, find themselves in it, and leave it for a body (GAME.md: the philosophy notes
  are the game's lore).

**19.44 Keeping the interface even (Edward, 13 October 2026).** His description of the interface
as it should stay: even and distinguished. Rows line up, controls in a row share their height, a
symbol sits in the exact middle of its button, and the corridors' wireframe style carries into
the panels.
- *Built:* CHAT's row. The box, +, DRAW and SEND share their top and bottom edges, and the + is
  drawn as two bars to the pixel rather than as a letter of the font.
- *Kept as a rule* in HANDOFF's house style, for every panel to come.

**19.45 Stories in the noise, and one equation for everything (Edward, 13 October 2026).** His
words: the crazy stories, coincidences, recognition and pareidolia can be turned into stories for
the game, kept as a contained lens focused on the madness of the Library of Babel's potential; and
everything oriented around the fun side of one perfect mathematical equation for all games,
simulations and information, all in one thing, forever, for the future of mankind to enjoy, where
all things can cross over freely in any way, shape or form.

*Notes.*
- **Pareidolia is a filter with a false-positive rate.** Seeing a face in a cloud is a "face"
  filter passing noise. Sieve can say exactly how often: a stack that keeps faces keeps some
  fraction of random pictures too, and that fraction is countable. So a coincidence in the game can
  carry its own odds ("one noise in 2^31 shows a face this clear"). That is the contained lens:
  the madness of Babel, with the arithmetic beside it, so that wonder never turns into a false
  claim.
- **Stories from it:**
  - *A message found in the noise,* and the odds that it is chance;
  - *a face in a picture of nothing,* with its rate;
  - *two strangers' books that agree word for word,* the birthday paradox at the library's scale;
  - *a melody that is almost a known tune* (§19.8's near-matches).

  Each is a survivor of a stack the player did not mean to build.
- **The one equation already exists, and it is small.** Every line is the same function: a unit's
  digits read as a number in base B (positional), or that number put through a keyed permutation
  (scrambled). One bijection per shape; everything else is the choice of shape and the filters.
  The binary line holds every file, so every other line's units are also there, as files: that is
  where all things cross over freely. A game, a picture and a model are the same kind of thing, a
  number, seen through different shapes.
- **For the game:** this is its centre. Everything the player finds is a number, every number is
  everything at once, and the filters are how meaning is chosen. GAME.md takes this as its lore.

**19.46 Models in a chain (Edward, 13 October 2026).** His request: a way to chain models of
different types together, such as the three trained models in a single chat. It should be its own
tab next to CHAT, variable, mappable and savable, and adjustable to other models, shapes and sizes.
- **Built (13 October 2026), SPECIFICATIONS §12.0h:**
  - *The CHAIN tab,* just before CHAT. It shows the chain as a row of nodes with arrows (YOU,
    then each model). Up and Down choose a stage, Left and Right change its model, N adds one, Delete
    takes it away, Ctrl+Up and Ctrl+Down move it, and Ctrl+S and Ctrl+O save and open it as a
    `sieve-chain-v1` file.
  - *Running it:* `/chain WORDS` in CHAT runs it, every stage's result appearing there as a reply
    or a card.
  - *The default chain* is the three trained models: universalbibliothek's text model, then
    figures-2x32, then melodies-2x64.
  - *How a stage is fed:*
    - a text model continues what it is given;
    - a picture or melody model, which takes no input, has its noise chosen by what it is given (a
      generator seeded by its SHA-256), so the same words make the same picture.
  - *Deterministic:* a chain of picture and melody models makes the same units on every machine,
    which CI checks.
- **Not yet:**
  - *Branches:* a graph rather than a line, with one stage feeding two, kept in the node graph's
    map format so chains and maps are one system (§19.38).
  - *Steering:* a picture stage that keeps only the survivors of the line's filters (§19.37).
  - *Larger models* as stages, once they can be given as files of other kinds.

**19.47 An engine that could make every game (Edward, 13 October 2026).** His idea: the game
engine as a free-flowing, dynamic building and creation system.
- *Bodies:* procedural skeletons and bones, boxes, gravity and kinematics, destruction.
- *Scale:* from voxels to triangles, quads and other forms, all the way up to layering with
  DLSS 5.
- *Rules:* Conway's Game of Life, noise and much more.
- *The aim:* theoretically, to create all games ever. It starts with models and content, and
  eventually reaches form and function.

*Notes.*
- **A game is a starting state and a rule.** Life shows how far that goes: one rule on a grid is
  enough to compute anything. So "all games" is a line whose units are (state, rule) pairs, and
  each part already has a home here:
  - *the content* is units of the models and worlds lines;
  - *the rules* are units of a rule line (a cellular automaton's table is a short string);
  - *the physics* is a fixed rule set (rigid boxes, joints, gravity) run in exact arithmetic, so
    it is reproducible.
- **The order it can be built in:**
  1. *Content,* which exists: models, worlds, pictures, melodies.
  2. *Rules as units:* a Life-like line, where a unit is a rule table and a seed pattern, walked
     and filtered ("still lifes", "gliders", "grows forever"), and its survivors played in place.
  3. *Bodies:* skeletons and boxes with exact-arithmetic physics, as world units that move.
  4. *Representation:* the same world drawn as voxels, triangles or quads, chosen per view.
  5. *Upscaling:* a learned upscaler. DLSS is NVIDIA's own and closed, so Sieve's would be its own
     model, trained in the harness like the picture models, as an optional layer that changes
     only how things look.
- **The honest limit:** "all games" in principle is the counting argument again. Every game is
  somewhere on the line, and almost every unit is not a game, so the work is in the filters that
  find the playable ones: does it run, does it end, does a player have choices that matter.

**19.48 The documentation inside the game (Edward, 13 October 2026).** His idea: /prompt and /help
are useful, and commands could reach the documentation, kept as a .sieve file or found through
the pages and books lines. There should be a free-flowing way between all catalogued information.

*Notes.*
- **The documents are files, so they are already in the library.** README, SPECIFICATIONS, IDEAS
  and the rest each have an address on the binary line, and as text, on the pages and books
  lines. A `.sieve` file of them (as the release is) names them all at once.
- **Commands that reach them:**
  - `/docs` lists the documents;
  - `/docs NAME SECTION` shows a section in CHAT;
  - each answer has GO TO IT (to the file on the binary line) and J (as a book).

  The model given as a file could then be pointed at the right section, rather than told
  everything in its system prompt.
- **One catalogue:** every catalogued thing (documents, anchors, models, runs, maps, chains)
  named by its SHA-256 and linked in the node graph's map, so that from any one of them the others
  are a step away. The maps already do this for files; the rest are files too.

**19.49 The completion date on the main menu, in colour (Edward, 13 October 2026).** His idea:
show the absurd completion estimate, with its full calculation and meaning, on the main menu under
the description, in slightly bigger text, with variable letters or names in mathematical notation,
each coloured as its dimension is. It, the description and every dimension label should be tied
together so that they flow in and out of the system as needed, perhaps acting as filters too. It
must fit on screen as the dimensions grow (a note for the code and the docs). The website's and
the game's calculations must stay in sync, and look alike.

*Notes.*
- **One source for the numbers.** The calculation lives in one specification, and both sides are
  checked against it:
  - *the lines' sizes* (2^100 pictures, 27^32 pages, 2^32768 models), from the setup menu's own
    shapes;
  - *one unit a second;*
  - *the epoch.*

  The website's script and the menu compute the same thing, and a CI line compares the page's
  constants with the program's (`sieve info` prints each line's size exactly).
- **Its form on the menu,** one line per dimension, each in its line's colour, for example
  `P = 2^(10×10) pictures  →  t = P / (1 s⁻¹) ≈ 4.0 × 10^22 years`. As the setup menu changes a
  shape, the line changes with it. Lines that would not fit are folded into "and N more
  dimensions", so the menu never overflows.
- **As filters:** each label is the line's name and size, so choosing one could open that line
  with its stack, the same thing seen from the menu.

**19.50 Metrics for every build, charted over time (Edward, 13 October 2026).** His idea: log each
build's metrics, with its date, in a folder in the repository. Then chart how effective it all is
over time (a line graph, dots joined by lines), shown both in the 3D world and on the website's
Buy or Build page, with room for other visualisations.

*Notes.*
- **What to log:** `hallway --metrics` (§19.23's list) already prints the measures, versioned. One
  file per build, `metrics/YYYY-MM-DD-HHMM.txt`, written by `run_tests.bat` after the tests pass,
  so logging costs nothing extra.
- **What makes a chart honest:** the same measure, the same version, on comparable machines. A
  change of measure starts a new series rather than joining the old line. Each point carries its
  build (the commit) and the machine.
- **Where it shows:**
  - *on the website,* a small chart drawn by the page from an index of the files;
  - *in the world,* the same chart on a wall of the start room, made from the same files.

**19.51 A decompilation engine (Edward, 13 October 2026).** His note: alongside all this, the only
thing missing is a decompilation engine.

*Notes.*
- **In Sieve's terms:** given a binary and a known compiler, find a short program on the code line
  (§19.36) that compiles to exactly that binary. A deterministic compiler makes "compiles to this"
  a filter, so this is a search, as superoptimisation is, in reverse.
- **What exists:** Ghidra and RetDec turn machine code into readable C-like code by analysis, not
  search. A search finds exact sources only for small functions. The two together, analysis first
  and search for the pieces, is the realistic shape.
- **The law:** decompiling software one does not own is limited by licences and by law
  (interoperability is the usual exception). Sieve's own files, and files whose licences allow it,
  are where it starts.

**19.52 The Encryption Engine: anything as anything (Edward, 13 October 2026; extends §19.34).**
His words: next, the Encryption Engine, like File Cipher. It should encrypt anything as anything,
with anything, through anything, in any number of layered ways, through infinite ciphers, and,
in theory, flatten them as automata to count out the results of all things. Note all the possible
combinations of data, to be expanded and catalogued mathematically.

*Notes.*
- **The catalogue of combinations.** Five choices, each a line:
  - the payload (any unit of any line);
  - the key (any unit);
  - the recipe (§19.34: a list of layers);
  - the depth (the number of layers);
  - the output's shape (any line).

  With A kinds of layer and depth L there are A^L recipes, each times its keys' count. That is
  countable, and the counting is exactly the kind Sieve does.
- **"As anything" already has a standard name: format-preserving encryption.** Encrypting a
  picture into another picture of the same line is a keyed bijection of the line onto itself. That
  is exactly what the scrambled ordering is: a Feistel network, as the standards FF1 and FF3-1 are.
  With a secret key in place of "sieve", every line becomes a cipher of its own shape. (Sieve's
  Feistel is not a reviewed standard, so it is for play and study, as §19.34 says.)
- **What flattening shows:** with enough layers, a payload can become any output of the right
  size, since bijections compose to any bijection. So the count of reachable outputs is the whole
  line, and the mapping from payload to output is noise unless the key is known. The catalogue
  names the paths; the key is what makes one path the message.
- **The first build** (next round): the recipe line, the weak ciphers with their detectors and
  key searches, and format-preserving encryption within a line with your own key. All of it in an
  Encryption Engine screen on the main and pause menus, with each layer's output an item.
  File Cipher's formats can join once their rules are written down.

**19.53 The plan, in order of importance (13 October 2026).** What to build next, and why in this
order.
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
