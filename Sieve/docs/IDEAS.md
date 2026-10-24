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
- *Fixed, 24 October 2026 (step 3 of the plan):* `symbol-entropy-v2` takes its bounds in
  thousandths of the most a unit of the line can have, log2 min(symbols, length), so one setting
  means the same on every alphabet. Measured on Don Quixote (`pg996.txt`) in lower27 pages of
  1,000: English 826 to 877 thousandths (median 851), random letters 992 to 996; pages of 100,
  700 to 885 and 920 to 960. The default top, 925, lies between them at both lengths. v1 stays,
  retired as before, and says it is replaced by v2.

**2.2 Padding is counted as content.** `model-information` and `symbol-entropy` count trailing
padding spaces as real characters, so a padded page scores as *more* text-like than its text alone.
- *Fixed, 24 October 2026:* `symbol-entropy-v2` and `model-information-v2` judge a unit that ends
  in two or more SPACEs on what comes before them, as `words-v2` does; a unit of padding alone
  fails. A page of six letters of noise padded to 32 with SPACEs passed `model-information-v1`
  and fails v2 (unit tests). Both are in the oracle, with 288 verdict vectors and 24 ranks (v2 in
  black and white).

**2.3 The cased dictionary lets short gibberish through.** Folding one cased list to lower case
also folds SCOWL's abbreviations and chemical symbols, so "zn" and "tb" now pass. Fix: a version
that keeps proper names and drops short all-caps tokens.
- *Fixed, 24 October 2026:* SCOWL writes the symbols in mixed case ("Zn", "Tb", "pH"), so dropping
  all-caps tokens alone would miss them. The `-names-v2` lists (`scowl-en-35-names-v2`, `-60-`,
  `-80-`) leave out, from each pinned `-names` file, every entry of two or more letters written all
  in capitals (OK, IBM, NASCAR: 45 in the 60 list) and every entry of one or two letters whose
  lower case is not in the plain list of its size (Zn, Tb, Hz, pH, Ed, Jo: 59), but "a" and "I";
  names of three letters or more stay. Made by `tools/build_dictionary.py --names-v2`, registered
  beside the old lists, which are unchanged. With scowl-en-60-names-v2, `sieve sift` counts 28
  times fewer pages of 64 letters that are all words (10^-38.49 of the space against 10^-37.03),
  and at 2 letters the plain list's 73 (134 before). In Don Quixote, no whole word was lost (only
  pieces of accented words split by the counting: "Señor" as "se"). Made the default the same
  day (Edward: "We should probably just make a function-words-v2 so as to maintain the backwards
  compatibility we have built in so far"): `scowl-en-60-names` stays registered, so a stack made
  with it reproduces by naming it, and `function-words-v1` keeps its own copy of that list;
  `function-words-v2` is the same rules from the new list, so it implies `words-v2` with the
  default again. Measured: 10^-19.08 of the pages of 32 letters against v1's 10^-18.33, and the
  same verdict on each of the 100 held-out sentences and their 100 shuffles.

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

**5.11a The address navigator.** *Built (X in the hallway, `client/navigator.cpp`).* Walking and
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

**19.42 The web page's Build or Buy (Edward, 13 October 2026).** Built: `Pages/release.html`,
linked from the index as "Build or Buy (Free If You Build It)".
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
Build or Buy page, with room for other visualisations.

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
- *Built (13 October 2026; §19.69):*
  - `tools/log_metrics.py`, run by `run_tests.bat` once everything it ran has passed;
  - a record a build (`sieve-metrics-v1`) in `metrics/builds`, with `metrics/index.tsv` written
    again from all of them;
  - the website's "Every Build, On The Record", which charts every measure.

  *Still to build:* the wall in the start room.
- *Built (13 October 2026):* `Pages/metrics.html`, **Metrics Over Time**. Its button is on Build or
  Buy below every metric and above Build It Or Buy It, since it is a metric of its own.
  - **What it shows:** one line chart per measure, read from `metrics/index.tsv`: a dot a build,
    joined in order. Each chart has its own scale, since the measures are in different units, and
    one axis can't honestly carry two scales.
  - **Choosing:** the machine (Edward's, `edward-1`, by default) and releases or every build.
  - **The measures:**
    - the unit checks, the seconds the suite took, and the checks a second worked out from the two;
    - the vector files, specification sections, commands, source files and lines, ideas and
      voices;
    - what the reference filters keep on each line.
  - **Reading it:** hovering over a chart gives the nearest build's value, date, version and
    commit, and every chart can be read as a table. A measure that has not changed says so, and no
    range is drawn around it.

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
- *Built (stage 1, 13 October 2026):* `cipher-v1` (SPECIFICATIONS §12.0i) and `sieve cipher`:
  - recipes of reverse, caesar, xor, Vigenère and feistel layers, there and back;
  - breaking the weak ones, by caesar shift, one-byte XOR, and repeating keys fitted column by
    column, ranked by an English score less a cost for each key byte. Caesar, XOR and Vigenère
    keys come out first; a feistel layer's output gives nothing English;
  - the catalogue counted exactly: 130 bits name one layer with keys of up to 16 bytes, and a
    recipe of depth d is d times that.

  Checked against the oracle (55 encryptions, 15 guesses).
- *Built (stage 2, its first screen, 13 October 2026):* **Encryption Engine** on the main menu and
  the pause menu (`client/cipher_screen.cpp`):
  - the input, as text or as hex;
  - the recipe;
  - encrypt, decrypt or break;
  - the output in hex and, when it is printable, as text;
  - Tab puts the output in as the input, so a result can be decrypted, broken or encrypted again.

  It sits after Settings, so the menus' existing rows keep their places. CI: `hallway --cipher
  --script ...`.
- *Stage 2, built (17 October 2026):* the item in hand or a file as the input, the output as an
  item to Go to or save. *Still to come:* a layer keyed by a picture (§19.76), and the catalogue
  on the screen.
- *Earlier plan, kept for the record:* the Encryption Engine screen on
  the main and pause menus, each layer's output an item; then layers that change the shape (a file
  as a picture, a page as a melody), which is the "as anything".

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

**19.54 Mapping the madness (Edward, 13 October 2026).** His words: "There are just so many ways
to look at information that it maps out to absolute madness, but if no one maps it, no one finally
maps it all or bothers trying to." File types, grammar, logic, rules: it all adds up.
- *Note:* each way of looking is a line, or a stack on a line, and Sieve's work is to make each
  one countable. The map of the ways is the node graph's job (§19.38): every line, filter, format
  and grammar a node, and every translation between them an edge. Mapping it all is endless, but
  each mapped part is exact and stays mapped.

**19.55 The voices of the halls (Edward, 13 October 2026).** His idea: messages, notes, ideas,
rules, filters and quotes (anything he or Claude says here) can be things the AIs say as they walk
the halls, in a big `.sieve` of pages, from the documentation to the philosophy. The odd metaphor
or quote goes a long way, and they can be attributed to him and to Claude directly. Ideas and
filters could lead from one to another, or simply be discussed in plain sight by the AIs, as could
the commands, and whether they may use them.
- *Started:* `docs/VOICES.md`, a corpus with one attributed voice a line (who, when, what kind),
  in a format a program can read. It holds Edward's, Gemini's and Claude's lines so far, and
  Gemini's two messages whole.
- *In the game:* the corpus packed as a `.sieve` of pages, each voice a page with its
  attribution, and the AIs on the shelves saying them. Choosing a voice by where the player is
  (near a filter's door, a voice about that filter) is a filter on the voices line.
- *Commands:* AIs talking about commands is free; using them waits for the rule of what a model
  may do without asking (§19.32). Until then they say which command they would use, and the
  player can type it.
- *Short lines:* one thought a voice, a sentence or two, so that they fit a speech bubble or a
  page.

**19.56 A chain is a state space, and steering it is alignment (Edward, 13 October 2026).** His
words: the model chain is its own state space; tweaking it slightly at will would be ultimate
control and an aim for alignment. It is recorded as a file for now, but it may become its own state
space, or the overarching one, or both. What other models can be chained together?

*Notes.*
- **It is already a line.** A `sieve-chain-v1` file is a short string, so every chain has an
  address on the pages or binary line, and on a line of chains of its own: stages over an alphabet
  of models. A tweak is a step along that line, and the chains near a good one are its
  neighbourhood. That is exactly the near-model idea (§12.0f), one level up.
- **Steering is filters on the chain's outputs.** The control Edward means is a stack applied to
  what the chain makes: keep the chains whose outputs pass these rules. With steered stages
  (§19.37), the outputs are survivors by construction. That is a precise, small version of
  alignment: the rules are stated, checked and countable.
- **What else chains:** any model with an input and an output in some line's units:
  - text to picture (here, by noise seed);
  - picture to text (a captioner);
  - melody to sound (the synthesiser);
  - sound to text (a transcriber);
  - text to voice controls (§19.33);
  - text to a world (a layout model);
  - a model to a model (a trainer: a run is a chain stage whose output is a model).

**19.57 Rules beyond the filters: Lean (Edward, 13 October 2026).** His idea: constrain things by
mathematical rule systems beyond the filters, like Lean for physics, so that further rules can be
applied in strange ways, perhaps even the rules of Lean itself.

*Notes.*
- **A proof checker is a filter.** Lean's kernel decides whether a proof term proves its
  statement. So "has a proof that Lean accepts" is an exact, checkable rule on a line of Lean
  files, and theorems are the survivors.
- **Physics in Lean exists:** PhysLean (formerly HepLean) formalises parts of physics, so a world's
  rules could be stated in Lean and its states checked against them.
- **The rules of Lean itself:** its type theory is a small set of rules, and its expressions are
  strings, so Lean is one more line with a filter. A stack of "parses", "type-checks" and "proves
  X" is the most exacting filter there could be. It is almost never satisfied by chance, which is
  why proofs are searched for by structure rather than by walking.

**19.58 Reading every document: PDF, OCR and layout (Edward, 13 October 2026).** His idea: a tool
for PDF files that transcribes their text and images for the models, with OCR, laying out the
syntax, grammar and rules for all layouts, formats and languages.

*Notes.*
- **PDF has two kinds:** born-digital, where the text is in the file and can be read out exactly,
  and scanned, where it is pictures and needs OCR. Tesseract (Apache 2.0) is the established
  open OCR engine, used as ffmpeg is, as a separate program and optional.
- **Layout is a grammar:** columns, headings, tables, footnotes. A page's structure can be
  written down as rules and judged as a filter ("is this a two-column page?").
- **All formats, all languages:** each format is a grammar and each language an alphabet with
  rules. The catalogue of them is a node graph (§19.54), filled one format at a time.

**19.59 A corpus of every language (Edward, 13 October 2026).** He still needs to build a proper
corpus of public-domain material across all known languages, speakable and not, and train models
on it.

*Notes.*
- **The sources:** Project Gutenberg, Wikisource and the Internet Archive's public-domain texts,
  and for languages with little text, translated scripture and government texts in the public
  domain.
- **The provenance:** every file's licence and SHA-256 kept, as `fetch_corpus.py` already does
  for the shipped text.
- **"Not speakable":** written-only systems (mathematics, music notation, programming languages,
  sign-language notations) and undeciphered scripts. Each is a line with its own alphabet.
- **Its use:** text models trained on it in the harness, and filters of "speaks language X"
  (`language-v1`) for every one.

**19.60 Bodies, light and scale (Edward, 13 October 2026).** His ideas:
- a skeleton-based animation system for the models and the game, or an animation space, that
  evolves and flows naturally in many ways, alongside light, texture and layers of textures;
- prerendering things from three angles and making a 3D projection from the flat images, to cut
  rendering costs for large scenes;
- a natural scaling system for everything, between models, projections and parallax, all the way
  to DLSS 5 hooked on top.

*Notes.*
- **The technique's name:** pictures prerendered from several angles and shown in place of the
  model are called impostors, and with depth per pixel they give parallax. Large open-world games
  use them for distant objects.
- **A ladder of representations** (§19.40's ladder for pictures, applied to scenes):
  1. a full model near;
  2. a simpler mesh further;
  3. an impostor beyond;
  4. a point of colour at the horizon.

  Choosing the step by distance is level of detail, and it is how a scene of everything stays
  drawable.
- **Skeletons:** a skeleton is a tree of bones, and an animation is each bone's turn over time.
  Both are short units, so animations are a line, and "walks", "stays balanced" and "does not
  pass through itself" are filters on it.
- **Upscaling on top:** as §19.47 says, our own learned upscaler, optional, changing only how
  things look.

**19.61 What the numbers mean (Edward, 13 October 2026).** His questions: how would these numbers
translate to anyone in the real world? What do they mean for the information, how long will it
take to get somewhere, and where is it all going? It will never finish, but how much is learned or
lost per day? He wants lists of the strange ways a human can quantify these things, kept as
statistics, ideas and filters for the AIs to talk about from every perspective.
- *Built, on the Build or Buy page,* computed live and exactly:
  - *a day:* 86,400 pictures, 6.8 × 10^-24 % of the 10x10 ones;
  - *everyone at once:* all 8.1 billion people would take 5.0 × 10^12 years, 359 times the age of
    the universe;
  - *the pages printed:* a stack 7.2 × 10^14 times the width of the observable universe;
  - *gained and lost:* the library loses nothing, and what can be lost is attention;
  - *the audio page's tracks,* from the same epoch, so the two pages agree.
- **Where it is going:** not to the end of the library, which is unreachable by walking (that is
  what the numbers say). It goes to better filters. Each rule found makes the meaningful part of
  a line findable, and that part can be small enough to finish: the survivors of a good stack can
  be few enough to walk in a lifetime. Progress is measured in what the rules have narrowed, not in
  units visited.
- **What is learned per day:** the bits that the day's new rules say (a filter that keeps one unit
  in 2^k has said k bits). That can be counted and charted (§19.50).
- **The list grows:** each new measure is a line in VOICES.md and, where exact, a counter on the
  page.

**19.62 The Universal Exchange Rate (Gemini's phrase; Edward likes it, 13 October 2026).**
*Notes.*
- **A real quantity, in bits:** what it costs to carry a thing from one line to another, and what
  is lost on the way. A translation that is a bijection loses nothing. A fitting (a photo to a
  10x10 picture) loses all but what the smaller unit holds. A chain's picture stage keeps only a
  64-bit seed of what it was given, so it loses almost everything.
- **So the rate is a table:** for each pair of lines and each way between them, the bits kept and
  the bits lost. It can be computed and shown: "this photo to this line: 29.6 million bits in, 100
  kept".

**19.63 A currency for every person, an identity, a wallet and shared computing (Edward, 13 October
2026).** His ideas:
- **A currency for everyone:** every human being could eventually have a cryptocurrency of their
  own, all generated through natural inflation. That might ease currency problems, give people
  less reason to scam cryptocurrencies by devaluing them, and focus them on the ideas in each
  currency's whitepaper: the science or technology it represents or discusses.
- **Identity and governance:** alongside the cryptographically secure digital identity and the
  governance proposals (Potentia's), as a framework for betterness in all things.
- **A wallet for everything:** a digital wallet that can hold and search for every currency when
  pointed at it, for the fun of it.
- **Shared computing:** distributed problem-solving, such as counting and checking primes (he
  remembers a program that did it across many computers), with networking.

*Notes.*
- **The prime program** is likely GIMPS, the Great Internet Mersenne Prime Search, which has found
  every record prime since 1996 on volunteers' computers. BOINC runs many such projects.
  Primecoin made finding prime chains its proof of work, so useful computation as a currency's
  work has been tried.
- **Hard parts, honestly:**
  - *One person, one currency* needs identity that cannot be faked many times over (Sybil
    resistance), which is why the digital ID comes first. It must then protect privacy, or it
    becomes surveillance.
  - *Issuing by inflation* dilutes everyone's holdings evenly, which is fair. Value still comes
    from what people will exchange for it, and scams move to wherever value is.
  - *Law:* issuing currencies and handling identities is regulated almost everywhere.
- **What fits Sieve:** a currency's whitepaper, its rules and its ledger are files and rules, so
  each currency is a unit, and "is this ledger valid under these rules" is a filter. A wallet that
  searches all currencies is a browser of that line. The networking and distributed computing are
  the larger project, Potentia's rather than Sieve's.

**19.64 Running out of original things to say (Edward, 13 October 2026).** Some people fear that
if everything is put into grammar, there will be no room left for anyone to say anything original.
He doubts it, and wants a counter.
- *Built, on the Build or Buy page:*
  - *the space:* sentences of 20 words from a 20,000-word vocabulary, 20,000^20 of them;
  - *the speakers:* every person who has ever lived (about 117 billion), each saying a new one
    every second for a hundred years;
  - *the share used:* 3.5 × 10^-64 %.
- *Its limits:* most of those sentences are nonsense, so the count of meaningful ones is far
  smaller. It is still astronomically larger than anything that can be used up, and longer
  sentences multiply it again. The counter measures how small the share is; it does not prove
  originality endless. That rests on the fact that a sentence can always be longer.

**19.65 Will it unfold, or collapse as a shape? (Edward, 13 October 2026).** His question, and what
it touches: quote, question, filter, shapes and nodes, all of them equating to something, with the
filters applied.
- **Claude's answer:** both, in order. Each filter folds the space: it keeps a subset, so the
  space narrows, nested like a telescope. It collapses to a single point only when the rules
  together say as much as the thing itself (the counting argument). Until then it is a shape, the
  set of survivors, and every new rule unfolds a new distinction inside it. The shape it collapses
  towards is whatever the rules describe; choosing the rules is choosing the shape.
- **As a node graph:** each question or quote is a node, each filter an edge that narrows, and
  each shape a node's survivors. The arrows in his message (quote, question, filter, shapes and
  nodes) are such a path.

**19.66 Disclosure of AI, everywhere (Edward, 13 October 2026).** His request: an AI honesty
disclosure across the whole website, as the audio page has, and fullest on the Build or Buy page.
It should say that Gemini helped to theorise and conceptualise (the website, many concepts and
ideas), that Claude built with him, and that he has been building the theory and ideas himself
over the past few years.
- *Built:* a disclosure box on every page, and a fuller one on Build or Buy. It says what the
  game's own models are trained on: a public-domain text, and Sieve's own lines.
- *Kept true:* as the project changes, the disclosure changes with it. A new contributor, human or
  model, is added when they contribute.

**19.67 Folding everything (Edward, 13 October 2026).** His words: they (the filters, rules and
ideas) are pretty much all intended to be automata, and many are or can be turned into them. So
it could fold in on all information that could ever exist, through all the light humanity will be
able to capture in the time the universe has, against the fading horizon of the observable
universe and the unobservable universe beyond it. That makes another set of numbers to weigh on
the website. The aim is to fold everything. "There are so many ways to map the data. We'll get
there." (VOICES.md).

*Notes.*
- **Why folding is the only way:**
  - *Writing everything out is impossible:* the universe can hold about 10^90 bits (Lloyd, 2002),
    and the AI line's models alone, written out, would take 10^9868 bits.
  - *Folding works:* a rule that can be run (an automaton, a line's definition, a stack) holds a
    whole set in a few bits. The AI line's definition is one line of SPECIFICATIONS.
  - *So "fold everything" means:* hold everything as rules, and unfold only the part looked at.
    That is what Sieve already does, line by line.
- **The light and the horizon:**
  - *Every photon that reaches us is finite in number:* the afterglow of the Big Bang alone is
    about 1.5 × 10^89 photons in the observable universe.
  - *The reachable part shrinks:* only about 4.6% of what we can see could ever be reached by
    anything sent today, and that share falls every second (the Orbital Simulation page's
    figures).

  So what can ever be captured is bounded, while the lines are not. The rules are what reach past
  that bound: a rule written here describes what no telescope will ever see.
- **Built, on the Build or Buy page:** "Folding Everything".
  - *Its counter:* bits written out against one line folded.
  - *Its list:* what the universe can hold, the light there is, the receding horizon, the stars
    crossing out of reach this year (the Orbital Simulation page's constants and formula, so the
    pages agree), and what fits and what folds.
- **Also on the website:** the AI disclosure is now at the top of every page, and on Build or Buy
  it comes before the buttons to buy or download, which are now at the bottom (Edward: it always
  has to be front and centre).

**19.68 The voices on the website (Edward, 13 October 2026).** His idea: the site can read out a
random voice from VOICES.md, at the very bottom of Build or Buy, under the donate button, as
another quirk of the site.
- *Built:* the page fetches `docs/VOICES.md` and shows one voice at random, with who said it and
  when, changing every 12 seconds. A "Read aloud" button speaks each voice with the browser's own
  speech synthesis, once a visitor turns it on (browsers do not let a page speak unasked). The page
  reads the file itself, so a voice added there is on the site with no change to the page. Where
  the file cannot be read (a page opened from disk), the box stays hidden.

**19.69 Build or Buy, and every build on the website (Edward, 13 October 2026).** His words: the
website should only ever read as build or buy, and not try to sell, which is why the buy button
went to the bottom, as in the honesty disclosure he drafted with the audio page. All the metrics
and statistics recorded by builds can go on the site too, some as counters and some fixed until
new figures come in. All of it has to be kept in sync, and eventually it may all translate across
every programming language, interface, address and taxonomy.
- *Built:*
  - *The name and the order:* the page is "Build or Buy" everywhere. Its buttons run Build, then
    Download, then Buy, at the bottom. The line at the top says the game is the same either way and
    that nothing on the page will try to talk anyone into buying it.
  - *"Every Build, On The Record":*
    - a counter of the time since the last recorded build;
    - the latest build's figures, fixed until the next build: unit checks, oracle vector files,
      specification sections, commands, source files and lines, ideas and voices;
    - what the reference filters keep on each line, when a hallway was built;
    - a chart for each measure, a dot a build, joined by lines.
- *Kept in sync:* there is one source, `metrics/index.tsv`, written by `tools/log_metrics.py` from
  the records. The page reads it, and the start room's wall will read the same file. Each measure is
  a named column, so a change of meaning is a new column, never a new meaning for an old one.
- *Honest by construction:*
  - a record names its commit, with `+changes` when it was made before committing;
  - it names the kind of machine (system, release and processor family), not which machine;
  - nothing is logged from a run with a failure.
- *Translating across everything:* every figure is a named, counted quantity in a plain table. A
  program in any language can read it, and an interface in any language can label it. That is the
  first step towards every address and taxonomy being one map (§19.54).

**19.70 How rare a voice is, and a hall of voices (Edward, 13 October 2026).** His idea: note and
count how rare it becomes to come across a given quote as the list grows, and perhaps a page in
the game with all of them on it, almost like a leaderboard. Anything can become a statistic
somewhere on the page.
- *Built, on the website:*
  - under the voice, its odds. With n voices, each turn shows one of the other n − 1, so a given
    voice comes back once every n − 1 turns on average. Hearing them all takes
    1 + (n − 1)(1 + 1/2 + … + 1/(n − 1)) turns on average (the coupon collector's problem, with
    the voice on show left out of each draw). With 36 voices that is 35 turns and about 146 turns;
  - and how many of them this visitor has met, kept in their own browser only.
- *In the game:* a hall of voices, a page or a wall listing every voice with who said it, when, and
  how often it has been heard. It ranks like a leaderboard: by how rarely each one comes up, by who
  has said the most, or by which the players stopped to read.
- *Anything a statistic:* any count the project can make honestly (a voice's odds, a filter's
  share, a build's checks) is a figure the page can show. The rule is the same as for the build
  metrics: named, with its source, and the same in the game as on the site.

**19.71 Every release measured, on the machine that made it, and Specs vs Stats (Edward, 13
October 2026).** His words: every time he makes a release with make_release.bat, record a full
suite of metrics, so they can be followed over time on his hardware, which will not change until
he chooses to fire up his next computer. The specifications can sit semi-hidden on the site in a
category of their own, specs against stats: a leaderboard anyone could add to, which changes the
game at its root by letting everyone contribute to the statistics. Each person's figures can then
be weighed against their own hardware. From there, theorise about all the computers ever working
together, and everything that might one day run or contribute at once. Dyson spheres go in a
category of their own. There are infinite ways to look at it, from many angles.
- *Built:*
  - *Every release on the record:* `make_release` step 8c runs the build's unit suite and times
    it. A failure stops the release. `tools/log_metrics.py` then writes a record (kind
    `release`, with `unit_seconds`) and the hallway's metrics list under the reference filters.
  - *Records say what the machine is:* the processor, the logical processors, the memory, and on
    Windows the graphics adapters, read from the system; never the machine's name.
  - *His machine on the record:* `metrics/machines/edward-1.txt` (`sieve-machine-v1`), in his
    words, listed in `metrics/machines/list.txt`. A record whose processor matches a machine file
    names it (`machine_profile`). When he changes machine, the new one gets `edward-2`, so each
    machine's figures stay their own series.
  - *`Pages/specs.html`, Specs vs Stats,* linked only from Build or Buy's builds section:
    - every machine's specification with its builds;
    - the leaderboard, by unit checks a second, comparing like with like (the same version);
    - "All Of Them At Once": the fastest rate times however many machines the reader gives;
    - the limits of physics: Landauer's kT ln 2 a bit, with all the sunlight that reaches Earth
      spent at room temperature: 6.0 × 10^37 bits a second;
    - a Dyson sphere in a category of its own: the Sun's whole output at the temperature of deep
      space, 1.5 × 10^49 bits a second.

    Both limits are set against the lines: the 10 × 10 pictures, the pages, and the AI line's
    models (10^9807 years even for the sphere).
  - *How to add a machine:* the page says how: a machine file, its name in the list, and the
    records, by pull request.
- *Still to come:*
  - the start room's wall (§19.50);
  - more timed measures (training steps a second, the filter counts' time) as the record grows.

**19.72 A library of small baked models, the corpus of a multimodal system (Edward, 13 October
2026).** His words: a characters image run at low resolution, a fourth entry in the model list.
Aim to ship with various pre-baked small, low-resolution models, of shapes, figures, characters
and more simple things, that add up to the corpus of a multimodal system. Across all model types,
have numerous pretrainable shapes, models and variations, and ship some baked. People can then
bake their own and share them.
- *Built:* `data/training/type-2x32`, the characters model (§19.73's run). It sorts after the
  others, so it is the fourth entry: figures, melodies, type, with the text run beside them.
- *The library's shape:* each model is a run folder (a recipe, its pictures or melodies, and its
  result), so baking is `sieve diffuse RUN`, reproduced exactly. Sharing one is sharing its
  folder: anyone can check it trains again to the same SHA-256.
- *What to bake next, smallest first:*
  - digits alone;
  - single strokes and arcs;
  - the figure families of `figure-v1` (four-way, eight-way);
  - textures (row-runs);
  - chords and rhythms on the audio line;
  - short words on the pages line (the text runs).

  Each is a few minutes' training, and together they are the corpus a multimodal chain draws on
  (§19.46).
- *Variations:* the same pictures at other widths and depths (2 × 16, 2 × 64, 3 × 32), so the
  library also measures what size a shape needs.

**19.73 The characters model (13 October 2026).** `tools/build_character_pictures.py` draws every
printable ASCII character but the space (94) from Sieve's own 8 × 8 font (font8x8, public domain)
onto a 10 × 10 picture, at each of the 9 places a glyph fits: 846 pictures. `type-2x32` is trained
on them as `figures-2x32` was on figures: 2 layers of width 32, 15,000 steps, seed 1, in 6 minutes
52 seconds here. CI checks its first six pictures by address, and that the font gives its
pictures again byte for byte.

*What it draws, measured* (200 pictures from each model, against its own training pictures):
- **The plain measure,** the Hamming distance to the nearest training character, flatters
  near-blank pictures, since '.', ',' and '-' are almost blank.
- **The stricter measure:** a picture counts when it is within 3 cells of a character with 8 or
  more inked cells.

| Run | Pictures | Median distance | Within 8 cells | Strict | Median ink (theirs) |
|---|---|---|---|---|---|
| `type-2x32` (shipped) | 846 | 16 | 1 | 0 | 17 (22) |
| `type-2x64` | 846 | 17 | 0 | 0 | 20 (22) |
| centred only, 2 × 32 | 94 | 6 | 133 | 0 | 6 (22) |
| digits only, centred, 2 × 32 | 10 | 13 | 7 | 0 | 30 (25) |
| `figures-2x32`, never shown a character | - | 20 | 0 | 0 | - |

- **None of them draws a recognisable character yet.** Twice the width did not help, nor did 94
  or even 10 pictures in place of 846.
- **The centred model looks best on the plain measure,** but only by drawing almost nothing.
- **So the limit is not the model's size or the number of shapes.** It is something in how
  `diffusion-v1` learns or draws.
- **A guess, not yet tested:** when making a picture (SPECIFICATIONS §12.0g), each level decides
  all its cells at once, and each takes its own most likely colour. A stroke needs neighbouring
  cells decided together, which a figure (one blob) forgives and a letter does not. A cell's most
  likely colour on its own is usually the background, which fits the centred model drawing almost
  nothing.
- **What to try next, in order:**
  - drawing a colour by its probability rather than the most likely one;
  - deciding a few cells a level, in an order, so later cells see earlier ones;
  - more levels and steps on the digits;
  - a model told which character to draw.

  Each is a measured run, compared in this table.

**19.74 Accessibility: the voice in the game, and everything readable (Edward, 13 October 2026).**
His words, after hearing the website's voice: push towards that voice being in the game too, with
buttons that can be read aloud and objects that can be translated, and much more. The website
already shows each voice as text while it speaks, so it has subtitles.
- *The aims:*
  - **The voices spoken in the game:** the AIs' lines (§19.55) read aloud, with subtitles, as the
    website does. Windows has a speech engine built in (SAPI), so the voice costs no download;
    elsewhere, the system's own.
  - **Menus and buttons that can be read aloud:** the item under the cursor spoken when it
    changes, as a screen reader would. Every string is already in the language files
    (`data/lang`), so the speech reads what the screen shows.
  - **Objects that can be translated:** every item's description and every sign through the same
    language files, so a new language translates the world as well as the menus.
  - **Other settings:** text size, contrast and colour-blind palettes (the lines' colours are
    settings already), and holding a key in place of pressing it again and again.
- *Quick-time events, noted and not wanted:* Edward's old idea, recorded so it is not lost. A
  prompt could repeat across the screen in a grid, many at once, with how many on screen as a
  setting, so no player could miss it. There are no quick-time events in this game or its story,
  and he does not want them. If anything ever must be noticed at once, the grid is how.

**19.75 Points about everything (Edward, 13 October 2026).** Some of his quotes are points about
the whole of it, and VOICES.md marks them "a point about everything". The thought: any of them may
lead to a filter one day, or may not; there is lots of time and ground to cover. Two so far:
- *"We can make it all as free-flowing as possible, as a program and as a state space."* A filter
  would measure how free-flowing a thing is: how few of its parts are fixed.
- *"If we all use the commands and filters, won't we all have an angle?"* Yes: a person's filter
  stack is their angle on the space, and it is a unit of its own. Many people's stacks, compared,
  are where the space looks the same from every angle and where it does not.

His question, "Does that mean we can fold all video games into one?", has Claude's answer in
VOICES.md. Counting-wise they are folded already: every game is a file of the binary line. A rule
that plays any of them is an emulator, and the hallway is the start of one.

**19.76 Characters and figures as filters and keys (Edward, 13 October 2026; with §19.52 and the
filters).** His thought: characters and figures could surely be used as filters or as keys, in
strange ways. Why not? His question with it: "This, models across all dimensions?"
- *As filters:*
  - **A picture that is a character** (within a few cells of one of the 94 glyphs) is a filter
    on the image line, an exact one, since the glyphs are a fixed list. Its survivors can be
    counted: 846 placed glyphs, each with every picture a few cells from it.
  - **A page whose characters are drawn,** not written, crosses lines: a row of pictures on the
    image line that read as a word of the pages line. That is one line filtered by another
    (SPECIFICATIONS, `not-written-v1` and its kin), with pictures as its letters.
  - **A figure's shape** (`figure-v1`) can filter a line where shape means something else: a
    melody whose plotted notes form one connected figure, or a model whose weights do.
- *As keys:*
  - **A picture as a cipher key:** its 100 cells are 100 bits, so a figure or a drawn character is
    a key for `xor`, `vigenere` or `feistel` (`cipher-v1`). Anyone holding the picture holds the
    key; a key that is a drawing can be remembered as one.
  - **The model as the key:** a picture model and a noise address make a picture (§12.0g), so
    the pair (model, noise) is a key, and every model of the AI line is a family of keys. A
    message can be locked with a picture that only a given model draws.
  - **Characters as an alphabet of keys:** a key spelt in glyphs, each glyph a picture, so a
    recipe's key and its drawing are one thing seen on two lines.
- *Models across all dimensions:* every line can have its baked models (§19.72). With those, a
  character, a figure or a melody is at once a unit, a filter, a key, and something a model makes.
  The same thing seen along every dimension.
- *Next:* a `glyph-v1` filter (a picture within K cells of a glyph, counted exactly), and
  `xor-picture`, a cipher layer keyed by a picture's address, in a later `cipher-v2`.

**19.77 How many voices a contributor may add (Edward, 13 October 2026).** His words: let
contributors add some number of quotes. He does not like numbers or money, so: unlimited within
reason. "Within reason" is whether a quote exists, and whether he can store it, because infinite
is infinite. So a respectful amount is asked for, and more may be considered, evaluated,
enumerated and stored later. Nothing is guaranteed. Noted for now, not built. The same words are
in VOICES.md beside the format.
- *How it could work, when built:*
  - each voice carries its contributor, as every voice already does;
  - the website's odds (§19.70) count every voice, so the list's length is the measure of what
    is stored;
  - a voice already on the list (the same text) is not stored twice, so "whether it exists" is
    checked by the list itself.

**19.78 Every figure with its inverse (Edward, 13 October 2026).** His words: "Running Out Of
Original Things To Say" says how many are used up, but has no counter beside it saying how many
are left. Always try to give the inverse of a metric, where and when it can be given, to put it in
perspective.
- *The rule:* a count of what is done comes with what is left, and a share with its complement.
  Where the inverse cannot be known (no honest total to take from), the figure says so, and no
  total is made up.
- *Built on the website:*
  - **Expected Completion:** the pictures looked at, and the pictures still to go, both as counts
    and as shares (1.267 × 10^30 left, 99.99999999999999999999882...%).
  - **And Then The Pages / The Language Models:** none read yet, so all of them still to read,
    100%.
  - **Running Out Of Original Things To Say:** the share used up, and the sentences still unsaid
    (1.048 × 10^86, 100% less the share used).
  - **Folding Everything:** the bits written out, and the rule that holds them: under 10^4 bits,
    less than 1 part in 10^9864 of them.
  - **The Orbital Simulation:** 95.4% unreachable, and 4.6% still reachable.
- *Not given an inverse, and why:*
  - **The audio page's tracks:** there is no total of tracks that could be made to take them from.
    The honest inverse would be the tracks not made, which needs the number that were made.
  - **The time since the last build:** a time has no "left", until there is a plan to measure
    against.
- *Next:* the game's figures (the setup menu's counts, the HUD's position along a line) get the
  same pairing: how far along, and how far still to go.

**19.79 Things that get in the way, from playing it (Edward, 17 October 2026).** Found while
building Debug and Release, running the tests and playing it. Each is small; together they are
how the game feels to use.
- **ffmpeg's warnings in red, always.** A video that cannot be opened (the anchor video: "an MP4:
  no line holds that item") should say so in red, as every warning that stops something should,
  so it is seen.
  - *Where Sieve looks for ffmpeg:* `--ffmpeg PATH` on the command line (the program itself, e.g.
    `C:/ffmpeg/bin/ffmpeg.exe`), then the `SIEVE_FFMPEG` environment variable, then beside Sieve's
    own programs, then `PATH`. Since 17 October 2026 `sieve-hallway.ini` holds it too, as
    `ffmpeg =` under `[paths]`, after `--ffmpeg` and before `SIEVE_FFMPEG` (an earlier note here
    said it did before it did). A folder such as `C:/ffmpeg/bin` is not looked in unless it is on
    `PATH`.
  - *Built (17 October 2026):* Settings > Programs, an ffmpeg row: the version found, or "not
    found" in red with where it looked; Enter chooses it with the system's file window, Left goes
    back to looking. The hallway's messages that say something stopped (`Hallway::warn`) are red on
    a dark plate, and a video with no ffmpeg says so and where it looked (`ffmpeg_looked`), instead
    of "no line holds that kind".
- **The Encryption Engine with the item in hand:** a button in the engine's window that takes the
  item in hand (its bytes, as F would save them) as the input. The output becomes an item that
  can be gone to. *Built (17 October 2026):* I on the item page opens the engine with it (Esc
  would put the item down before the pause menu opened); Ctrl+O takes a file; Ctrl+S saves the
  output; GO TO IT puts you on the binary line in front of it; buttons do the same.
- **Controls that flow between windows:** a window opened from another, or from an item, should
  take the item and the keys along with it, so nothing has to be put down to be used elsewhere.
  *Mostly the ethos now (Edward, 17 October 2026):* most things can or do hand off to most things
  already, the Encryption Engine being the exception (the item in hand, below). He will keep an
  eye out for others.
- **The window comes back to the front after a file is opened.** It still does not, from any of
  the ways a file is opened (the plus button, Ctrl+O, the File Locator, the system's file
  window).
  - *Tried already:* `client/dialogs.hpp` raises the window when each dialog closes (HANDOFF, the
    entry on dialogs), and on Windows that is not enough.
  - *A likely cause, not yet confirmed:* Windows only lets the program the user is already in
    take the foreground, and otherwise flashes the taskbar button instead. The fix to try first:
    restore and raise the window, then call `SetForegroundWindow` from within the dialog's own
    turn, while Windows still counts the click as the user's.
  - *Built (17 October 2026), and it works on Windows (Edward tested it):*
    `client/window_front.cpp` does exactly that from the dialog's callback, with
    `AttachThreadInput` to the foreground's thread for the moment it takes; SDL's raise on the
    main thread follows as before.
- **The chat's tabs:** C switches tabs everywhere else, but in CHAT the keys go to the message, so
  Esc must come first. The simplest fix: the tabs answer the mouse too, always, chat or not.
  *Built (17 October 2026):* a click on any tab goes to it, in CHAT too (`click_hand_tab`).
- **Chat that grows:** save the conversation as a log, and edit the context window (what the model
  is given) from a tab, with voice and speech to come, as in the other systems.
  *Built (17 October 2026):* the log, `/log [PATH]` or Ctrl+L; and the context window, `/context`
  to see it and Ctrl+E to edit it (the model in hand's every byte, kept in step with its position;
  a model file's system prompt). Still to come: voice.
- **A help tab:** in CHAT, and on every item's page, saying what can be done there and with which
  keys. It is part of the architecture, not an afterthought.
  *Built (17 October 2026):* HELP on every item's page, after SORT and META and before CHAIN and
  CHAT, its rows chosen by the item's line; CHAT's own keys are listed under its box as before.
- **The version number:** still 0.13.1 after many rounds. It should move with the next release,
  and the metrics (§19.71) name a version, so each step is on the record. *Done (17 October
  2026):* 0.13.25, as Edward chose.

**19.80 One less if: a tab with a use belongs on every line (Edward, 17 October 2026).** His words:
"If a tab has utility, it should exist in all dimensions," and "one less if statement" (both in
VOICES.md). Apply that rule always, if and where possible.
- ~~**CHAT on every line:**~~ *Deprecated for now (Edward, 17 October 2026; §17.2): he is not sure
  it fits.* On the AI line the item in hand is the model itself. On every other line
  the item is a file or unit, and the model is whichever is loaded (or a chosen chain, §19.46).
  CHAT then means talking about the item with that model: its bytes given as a file is given now
  (the plus button). Equating a model with a file "by some ruleset or length along the line" is
  the same act: the file is the context, the model is the reader.
- **Why it matters:** each special case removed is one less branch to keep right, and a system
  that works the same on every line can be learned once. The rule is the code's as much as the
  player's.
- **What follows:** COST, the help tab and the Encryption Engine's button become tabs of any item
  (CHAT left out for now, §17.2). A line without a use for one shows it greyed with the reason, rather than hiding it.

**19.81 Folding as a second temperature, and the models' own maps (Edward, 17 October 2026).** His
ideas, in his order:
- **n-dimensional folding as a variable, like a second temperature.** Temperature chooses among a
  model's next tokens. A folding temperature would choose among models: at each step it could flip
  to a random, similarly weighted model on the line. "Similarly weighted" could mean a near
  address, a near weight vector, or the same filters' verdicts. Which one is chosen depends on the
  context, on a token set up for it, or on how the user constrains the node-shifting state space.
- **Seeing how models differ:** watch how models fluctuate through node maps, and give each a
  temperature, to see how they interact at the large scale.
- **The tools it needs:**
  - a model token viewer (what each token weighed);
  - a process viewer (what ran, and in what order);
  - mappers and loggers that keep the statistics.

  Each is a 3D node graph like the node viewer, showing how models move through the state space.
  Every one is also a way to warp: click a node, and go to that model.
- **Models as filters:** lock a model's state space down and refilter every variant from every
  other state space with it, for a moment or as a standing filter. Dimensions already filter one
  another out. The point is that all of it adds up to one constraining map (another node graph),
  a record of knowledge across the state space. Files stack up as anchors, by address or whole,
  and fold out a map of the infinite among the state spaces.
- **Tuned across languages, through models and filters,** it could search itself and make filters
  for itself.
- **His picture of it:** "a living web of ranges and states and fluctuations, bound by an
  addressable fluid topology that warps and weaves itself through the filters' rulesets, amongst
  the n-dimensional manifold filter that strikes content from varying dimensions" (VOICES.md).
- **Pictures boiled down to titles:** a filter, or filters, that reduce a picture to its title
  according to the topology's curves at the time. This is the image line's title read from its
  content, the reverse of titling a unit.
- *Notes:*
  - **What is exact:** a step to "a similar model" is exact once the similarity is a rule (a
    distance on addresses or weights, with a fixed tie-break). The folding temperature then chooses
    among a finite, counted set, and every walk is reproducible from its seed, like everything
    else here.
  - **Where to start:** the chain (§19.46) is already a list of models run in order. The first
    folding temperature is a chain stage that picks its model from a neighbourhood instead of by
    name, logged so the walk can be replayed and drawn as a graph.

**19.82 The Encryption Engine as an automaton over every file (Edward, 17 October 2026; with
§19.52).** His words: files should be encrypted and decrypted through the engine, not only typed
text: an automated matrix, an infinite stacking layer of potential encryption, in a `.sieve`, with
keys and layer values, and keys that can be split into several keys. Then weigh the infinite noise
of encryption and its layering, to make a filtered automaton that filters every possible encrypted
(and many-times encrypted) file out of the state space. It would be an automated engine that takes
every verified anchor, now and later, and runs a baseline analysis until all files reach the same
baseline, then studies further as new files are added: pinning down the rules of encryption on
all known files, and filtering them out as an automaton where possible.
- **Files through the engine:** the input from a file, or the item in hand (§19.79), and the
  output to a file or an item. *Built (17 October 2026); the `.sieve` recipe and split keys are
  still to come.* The recipe and its keys are saved as a `.sieve`, so an encrypted
  file and how to open it can travel together or apart.
- **Keys split into several:** this has a standard name, *secret sharing*. Shamir's scheme
  (1979) splits a key into n shares so that any k of them rebuild it and fewer reveal nothing. It
  is exact integer arithmetic, so it fits Sieve's rules, and it can be checked against the oracle
  like the rest.
- **Filtering encrypted files out, honestly:** a good cipher's output is built to be
  indistinguishable from random bytes. A filter can therefore set aside what looks random (high
  entropy, no structure): that is most of every line already, and the noise filters do it. It
  cannot tell a ciphertext from noise; if it could, the cipher would be broken. What it *can*
  catch is everything weak: the caesar, xor and Vigenère layers `--break` already finds, file
  formats with known headers (an encrypted ZIP says so in its header), and any recipe whose output
  keeps structure.
- **The baseline analysis:** run every verified anchor through the known weak layers and the
  format detectors, and record what each reveals: a table of which files carry which
  recognisable encryption. As anchors are added, the table grows; a layer never seen before is a
  study of its own. It is a measured catalogue, and the honest version of "pinning down the
  rules".
- **The automaton:** the weak layers compose to a finite-state transducer (each is a per-byte
  map with a period). The set of files that one weak recipe of English makes is therefore a
  language a filter can count, which makes "filtering out what a weak cipher made" exact for
  those ciphers.

**19.83 One model at the centre of every model (Edward, 17 October 2026; with §19.46, §19.56).**
His words: chains were meant for models of every type, one type to a node map or network, so that
every model becomes a fluid, accessible map that can be changed when needed. The chaining could
also be repurposed as another layer of the system above. But the aim is to build a single
multimodal model from the infinite state space: something that might just exist as a fluctuating
wave among all model variants and sizes. A single point with perfect access to every model, and
the perfect vantage point to start recording metrics and node maps of the state space: the token
weights, and how everything lines up across all state spaces as each filter and dimension filters
the others out. If all data is boiled down into this engine and turned into a mass filter, it
could help reduce the state space of what could exist.
- **The first concrete form:**
  - one node map per model type (text, pictures, melodies, characters), each node a model on the
    line with its metrics;
  - chains drawn across the maps;
  - the single model as the chain that reaches every map, a router choosing which model answers.
- **Measured, not assumed:** each map's node carries the measures the harness already records
  (bits a cell, what its filters keep). Whether a single model "at the centre" does better than
  its parts is a question the metrics can answer, run by run.

**19.84 A gallery on the website (Edward, 17 October 2026).** His request: a Gallery button and page
from Build or Buy, previewing the development screenshots in `Images/Sieve`.
- **On Build or Buy:** a small preview of the images in `Images/Sieve` itself (not its subfolders),
  beside the button.
- **On the gallery page:** those, and every subfolder (Check, Designer, GraphicsTests, ItemViewer,
  Latest, Next, Node, Old, Other, Shapes, Text), each previewed and opening full size.
- **How the page knows the files:** a website cannot list a folder. So `tools/build_gallery.py`
  would write `Pages/gallery.json` (each folder, its files, their sizes) as part of the build, and
  the page reads it, as the metrics page reads `index.tsv`.
- **As of 17 October 2026:** `Images/Sieve` holds no images at its top level, since all are in the
  subfolders after the move. The Build or Buy preview fills in when some are put there.

**19.85 A skeleton system, alongside the UV system (Edward, 17 October 2026; with §19.60).** For
animation and more: bones with names and parents, a pose as one rotation per bone, and meshes
skinned to them. Joint limits are a filter on poses (§19.60), so a skeleton line is a state space
of poses that can be counted and sifted like any other, and an animation is a unit of poses over
time, as a film is of pictures.

**19.86 Larger models, plugged in: the models evaluated, and how to take them apart as they load
(Edward, 17 October 2026).** His request: the AI line's figure (2^32768 at the defaults) should
show where its parameters come in, and larger models should simply plug in; so evaluate the models
and model types there are, then work out the best way to dissect them on the fly.
- **The AI line's figure is right.** At the defaults (1 layer, width 16, 4 bits a weight) a model
  has 8,192 weights: the embedding 256 × 16 = 4,096, and a layer's q, k, v and o (16 × 16 each)
  and gate, up and down (64 × 16 each) another 4,096. Each weight is one of 2^4 = 16 values, so
  the line holds 16^8192 = 2^32768 models. The heads do not change the count; they divide the
  width. The setup menu now writes it `16^8192`, values^positions as the other lines are written,
  so both parameters show. The line has no titles (SPECIFICATIONS §12.0d).
- **The models looked at,** read from their own `config.json` and safetensors headers (a few KB
  each by range request, no weights downloaded; 17 October 2026). Bytes are the values at their
  stored width (BF16 two bytes, F32 four), worked out, not measured on a machine:

  | Model | Family | Values | Stored | Files | What Sieve's engine lacks for it |
  |---|---|---|---|---|---|
  | SmolLM2-135M / 360M / 1.7B | llama | 134.5 M / 361.8 M / 1,711.4 M | BF16, 0.27 / 0.72 / 3.42 GB | 1 | nothing: runs today |
  | TinyLlama-1.1B-Chat | llama | 1,100.0 M | BF16, 2.20 GB | 1 | nothing in the config (an untied `lm_head`, which is covered) |
  | Llama-3.2-1B (unsloth's copy; Meta's is gated) | llama | 1,235.8 M | BF16, 2.47 GB | 1 | `rope_scaling` of type llama3, which `Config::from_json` refuses |
  | Qwen2.5-0.5B / 1.5B | qwen2 | 494.0 M / 1,543.7 M | BF16, 0.99 / 3.09 GB | 1 | biases on q, k and v |
  | Phi-3-mini-4k | phi3 | 3,821.1 M | BF16, 7.64 GB | 2 (sharded) | `qkv_proj` and `gate_up_proj` fused (one tensor each, to split), shards |
  | Mistral-7B-v0.3 | mistral | 7,248.0 M | BF16, 14.50 GB | 3 (sharded) | shards; its shape is llama's, but its size needs quantised weights |
  | Gemma-2-2B (unsloth's copy) | gemma2 | 2,614.3 M | BF16, 5.23 GB | 1 | GELU (tanh), two more norms a layer, soft-capped logits (30) and scores (50), a head size of 256 that is not width / heads, alternating sliding windows (4096), its own tokenizer |
  | GPT-2 | gpt2 | 137.0 M | F32, 0.55 GB | 1 | another family: LayerNorm with biases, learned positions, GELU, fused attention, weights stored transposed |
  | Mamba-130M | mamba | 129.1 M | F32, 0.52 GB | 1 | not a transformer: a state-space model, an engine of its own |

  Quantised copies of the same models are published as single GGUF files, for example
  SmolLM2-1.7B-Instruct: 993,874,912 bytes at Q4_0 (bartowski's), and Qwen2.5-1.5B-Instruct:
  1,066,227,232 bytes at Q4_0, 1,894,532,128 at Q8_0 and 3,560,416,288 at F16 (Qwen's own).
- **What it shows:**
  - Most small chat models are *llama-shaped*: llama, qwen2, phi3 and mistral differ by a few
    switches (biases, fused tensors, rope scaling, sliding windows), not by architecture. One
    engine with switches covers all four; that is §19.80's "one less if" applied to models.
  - Gemma 2 is llama-like with several more switches; GPT-2 and Mamba are other families.
  - Size is the limit before architecture is. The engine kept BF16 as stored but widened F16 to
    F32 (twice the memory), and copies every tensor into memory before the first token.
  - *Since 17 October 2026 (Edward: "do we have to widen the F16 weights?"):* no. F16 is kept as
    stored, as BF16 is, and turned into a float as it is used through a table of all 65,536 F16
    values (each is a float exactly). Widening is now a setting for both, `widen_weights` (off by
    default; `sieve chat --widen-weights`). The logits are the same to the bit either way, which
    the unit tests check for both types. Measured here on one thread, a model of 27.4 million
    weights: as stored 54.9 MB, F16 64 tokens a second and BF16 48; widened 109.7 MB, F16 78 to
    94 and BF16 65 to 85 (two runs on a shared machine). So it is memory against speed, and the
    choice is the player's. *Still to come:* "auto", widening only while the model fits a share
    of installed memory, as ITEM MEMORY is a share.
- **The way to take them apart on the fly,** in stages, each one measured before the next:
  1. **Inspect without loading.** From `config.json` and the header (or a GGUF file's header)
     alone: the family, every tensor's role, the memory it needs at its stored width, the shards,
     and what the engine lacks for it, said before anything is read. `sieve tensors --inspect`,
     and CHAT's model row says the same before it loads. Cost: the headers, a few KB.
  2. **Map, do not copy.** Memory-map the files (and every shard named by
     `model.safetensors.index.json`) and read each row of weights where it lies, BF16 and F16
     both as stored. Loading becomes reading a header; the system pages weights in as they are
     used, and two runs of the same model share them.
  3. **Families as data.** One versioned table per family (`arch-llama-v1`, `arch-qwen2-v1`,
     `arch-phi3-v1`, ...): the tensor names and their roles, which tensors are fused and how they
     split, and the switches (biases, rope scaling, sliding window, norm offset, activation,
     soft caps). The oracle checks each against a published model's logits, as SmolLM2's were
     checked against ONNX Runtime. Llama 3's rope scaling, Qwen2's biases and Phi-3's fused
     tensors first; then Gemma 2.
  4. **Quantised weights, read in place.** A GGUF reader, its block formats dequantised a row at
     a time inside the dot product: Q8_0 (32 values, int8, one F16 scale) and Q4_0 (32 values,
     4 bits, one F16 scale) first, the K-quants after. This is what puts Mistral-7B, about 4 GB at
     Q4, on a machine without 15 GB to spare.
  5. **Tokenizers.** `tokenizer.json`'s byte-level BPE is covered with GPT-2's pattern. Qwen's
     and Llama 3's patterns differ and need checking against Hugging Face's `tokenizers`, as the
     5,736 texts were; Gemma's (and other SentencePiece tokenizers) are a second kind; a GGUF file
     carries its vocabulary in its header.
  6. **Other families,** each an engine of its own, only when one is wanted: GPT-2 (simple, and
     the start of many), Mamba. Picture models (a UNet or a DiT, its VAE and text encoder) stay
     behind the localhost bridge of §19.26 for now.
- **Plugging a model into the AI line, honestly.** `aispace-v1` is one Llama shape with a
  256-byte vocabulary, a feed-forward of 4H, its norms all 1, and digits evenly spaced over a
  range. A published model fits none of that, so today it sits exactly on the binary line (its
  file) and not on the AI line. A later `aispace-v2` could take its shape from a `config.json`
  (the vocabulary, the feed-forward, the key and value heads, the norms as weights) and its
  digits as the stored values, BF16 being 16 bits a digit. Then a published model *is* a unit of
  that line, exactly, and the address of SmolLM2-360M is its 723,642,240 bytes of weights
  written out: as long as the weights, never shorter (§0). What the line adds is the neighbours:
  every model one digit away, the filters, and the walk.
- **Proposed first step:** stage 1 and stage 2 together (inspect, then map), with Llama 3's rope
  scaling, Qwen2's biases and Phi-3's fused tensors, since all three are small switches and they
  bring in the three most used families of small chat models.

**19.87 The tightened rope: one filter that keeps bytes and propagates noise (Edward, 19 October
2026; with §16.8c, §17.1 and his paper *Rope Around The Clock*).** Recorded from his notes, his
diagram and the paper; his own words are quoted, and his notes ask that the AI conversation around
them be given less weight than his own messages, so it is left out here.

- **The inspiration, his paper *Rope Around The Clock*.** A rope-and-peg jig divides any length of
  rope into exact thirds without arithmetic: the rope is secured at peg A, woven round peg B, then
  peg C, and secured at peg D; pulling the jig's two pegboards apart sideways makes the rope take
  three equal lengths between the pegs, which are then marked. Laid back along an arc, the marks
  trisect the angle. With more pegs it divides by 5, 7 or 9. The principle: the pegs are fixed
  constraints, and pulling the rope taut settles it into the one position they allow, with no
  calculation. His words: "I wonder if in the end the filters might be able to be pulled tight
  just like what I designed in this paper. Imagine a scale of bytes going through a file,
  generated as a waveform of sorts and propagating through the file. That wave would then tweak
  itself left and right like a set of pixelly steps adjusting to fit the state space
  (bytes/floats/etc). Eventually you would have two ends of a line weaving itself through the file
  that can be pulled tight until the file itself lands at the best position you can find." He
  also pointed to a video of the Biham–Middleton–Levine traffic model
  (https://www.youtube.com/watch?v=LUOLdcLbGnQ), where simple local rules sort a grid into lanes,
  as how he imagines the values and the wave weaving through a file if drawn out in rows and
  columns.
- **The diagram** (his drawing, transcribed). Across the top, a bar labelled FILE with four marks
  along it: at its start, about a third of the way, about three quarters of the way, and at its
  end, coloured to match the points below. Below it, point **1, Start** (a circle, top left); a
  line runs down to point **2**, inside a tall bar on the left; on through an oval in the middle,
  labelled **State Space** and **Rotational Alignment**, holding two small circles on the line; up
  to point **3**, near the top of a tall bar on the right; and down to point **4, End** (a circle,
  bottom right). Written across the bottom: "The Keep Bytes or PropagateNoise filter". So the
  line through the file is the rope, the two bars are the jig's two pegboards, points 1 to 4 are
  its four pegs (start secured, woven round two, end secured), and the oval is the circle that
  keeps count of the rotation.
- **The filter, in his words.**
  - *Notes 1:* "Tighten byte rope filter: a filter that works similarly to the noise filters that
    propagate through the file as a wave, but instead of the wave propagating it would first find
    the best wave to propagate through with a single line. Then it might still propagate if
    necessary to get further reductions, so this might classify as a noise or wave propagation
    filter in the long run. The first parameter would be bit length 8/16/32/64, with potential for
    larger and more fine-tuned adjustments if necessary. The second parameter would be bits, bytes
    or floats. Where the waveform repeats as if each sequence of bytes and or bits is a track":

    ```
    XXXXXXX
    V-V-V-V
    XXXXXXX
    ```

    each `V` and `-` the waveform and each `X` above or below it the values themselves.
  - *Notes 2:* "This sounds like it might result in the absolute perfect alignment given enough
    iterations, vs file length of course. Because if byte/bit/float length are parameters, as is
    length, then all it takes is a basic waveform to weave its way through the file once to
    fundamentally shift the file alignment in the state space by a serious amount. I suspect
    ranging from 0-100% or 0-360 degrees, because of the way I have designed it. The wave could be
    set to propagate like the noise filters, so it could weave itself through once, then repeat
    the waveform going forwards or backwards. So three parameters, I think: value type, value
    length, propagation radius. That literally adds up to infinite ways to shuffle the file
    around."
  - *The two functions:* "The circle in the centre keeps track of the rotation, while a wave of
    noise propagates through the file to shift it around the state space. Then, once it finds the
    shortest possible address, it logs how it found it, records the noise function and parameters
    or the locked-in bytes of the file so that it can unscramble whatever was broken in the file by
    the noise filter. This could be set to byte tracking mode or noise wave propagation mode. That
    way one filter can track two potentials at once."
  - *Four points:* "It has 4 constraining points, and with a minimum viable address of 4 values I
    think this might be able to point directly to them with relative ease, depending upon the
    file size of course. That 4 for 4 match might be coincidental, but it might also just be the
    constraint required to reach the minimum viable addresses with relative ease." And: "it could
    find a neighbour a mile away at an exact interval, only with a scrambled mess of bytes inside
    it found through the noise wave; or while searching for it, it can track the bytes through the
    file as well, keeping track of two rotations at once and picking the best at the end. It
    wouldn't make any real difference, as through the noise wave it has to track the byte wave
    through the file anyway, so it would just utilise the same recorded values in another
    function."
  - *Both at once:* "I think I would just have it search both at the same time by default. So it
    wouldn't be a toggle, just the strongest, most hard-edge filter that tracks a wave through the
    file, records four specific bytes at equal intervals throughout the file as it checks the
    adjustment of those bytes to determine where it lands rotation-wise in the state space relative
    to where it lands in the state space via the noise. I think it would still be quite
    computationally heavy, but I have a feeling it forces the data line into a rigid structure with
    an exact method for pulling it to the shortest possible address. That is the hope at least."
    A version tracking more bytes across the file is noted too, as likely impractical for its
    search time.
- **So the proposal is one filter with two halves, searched together:**
  1. **Propagate noise.** A waveform of a chosen value type (bits, bytes or floats) and value
     length (8, 16, 32 or 64 bits, perhaps finer), run along the file's values like a track under
     them, repeating forwards or backwards to a propagation radius; the search finds the wave that
     moves the file to its shortest address, and the wave's parameters are recorded so the change
     can be undone exactly.
  2. **Keep bytes.** Four values recorded at equal intervals through the file (start, two inside,
     end: the jig's four pegs), followed as the wave moves the file, to measure the turn (0 to 360
     degrees, 0 to 100%) it lands at, against the turn the noise gives.
  The better of the two is kept, with what is needed to undo it.
- **Evaluation (Claude), so far.**
  - **Reversible: yes.** A wave added to the values (modulo their width) or XORed with them, with
    its parameters recorded, is undone exactly. So the filter can be lossless.
  - **The counting argument still applies to the search part (§16).** A family of waves with k
    settings, searched for the best one for a file, brings the file about log2 k bits nearer the
    front at best, and recording which setting was chosen costs the same log2 k bits. "Infinite
    ways to shuffle the file around" is exactly what makes naming the chosen way cost as much as
    it saves, on average. This is §16.8c (one shifting byte) and §17.1 (the rotation transform)
    again. A wave that shifts the file *as a whole* round its line is one of those rotations.
  - **Where it can win: the wave as a prediction, not a shuffle.** A wave gains where it
    *predicts* the values, so that taking it away leaves small, uneven residuals: that is how FLAC
    (a fitted curve under the sound) and PNG (each row predicted from the last) win. The residuals
    are still as many bytes, so the address does not shorten by itself; it shortens when they are
    then numbered with small and repeated values first, which is what `digit-histogram-v1`
    (SPECIFICATIONS §12.2) already does for a model's digits. So the testable form is: **find the
    wave that best predicts the file's values, subtract it, and code the residuals by their
    histogram**; the wave's few parameters are the "pegs", recorded, and pulling tight is the
    search that minimises the residuals' cost. It should help smooth data (sound, pictures,
    gradients, sorted tables), and not data that is already noise to every simple wave (packed
    files, encrypted files, `sieve.exe`'s compressed parts, a trained model's digits).
  - **The keep bytes, as anchors.** Four values recorded cost their width each and leave the rest
    of the file to be named, so on their own they do not shorten the address. They help if they
    are what the wave is fitted through (a wave pinned at four points, as the rope is pinned at
    four pegs, is described by those four values and nothing more). That reading makes the four
    pegs the wave's parameters, which fits "a minimum viable address of 4 values".
- **Questions for Edward (what is not yet clear).**
  1. How does the wave act on the values: added (wrapping at the value's width), XORed, or does it
     move values to other positions (a shuffle)? "Shift the file around the state space" could
     mean either.
  2. Are the four kept bytes the points the wave is pinned through (so the wave is fixed by them),
     or bytes left untouched by the wave, or a check on where the file lands?
  3. The rotation the circle counts: the turn of the whole file round its line (as a bearing), or
     the phase of the wave?
  4. Propagation radius: how far one pass of the wave reaches before it repeats, or how many
     values each step of the wave affects?
  5. The equal intervals: the diagram's marks sit near the start, a third, three quarters and the
     end; the notes say equal intervals (which with four points is the start, a third, two thirds
     and the end: the jig's thirds). Which is meant?
- **Edward's answers (19 October 2026).**
  1. *The wave:* "the same as the previous noise filter idea, it could change values, shift values;
     there is an infinite expanse of potential for different noise rules to take control there."
     So the rule the wave applies is open: changing values and moving them are both in.
  2. *The kept bytes:* this "could also become a parameter, like a sliding window around the
     bytes": how many values are kept round each point.
  3. *The circle:* "just a representation of its functionality": it is the centre of the file,
     a single byte or the gap between two bytes, and stands for keeping count of the degrees of
     rotation that part of the filter shifts the file to.
  4. *Propagation:* whether "the wave propagates to the next line or phase of bytes; think
     three-phase electricity, but we would have 0/1/2/3 etc, where 0 means it doesn't propagate
     and just weaves a single wave through the file".
  5. *The points:* the paper is a similar concept, mechanically the same; the intervals are equal
     intervals through the file:

     ```
     0x00 = start
     1/3
     1/2  = centre
     2/3
     end
     ```

     "When bytes don't split evenly, the value or byte in the centre becomes the centre, rather
     than the gap, allowing for even distribution."
- **The design as it now stands.** Five points at equal intervals: the start, a third, the centre,
  two thirds and the end (the jig's four pegs, with the circle at the centre between them). The
  values round each point, within a window (a parameter), are kept. A wave of a chosen value type
  and length, under a noise rule chosen from an open family (changing values or moving them),
  weaves through the file; with propagation 0 it passes once, and with 1, 2, 3 and more it carries
  on to the next line or phase of values, as the phases of a supply follow one another. The circle
  at the centre counts the degrees the file is turned to. The search tries waves and keeps the one
  that turns the file to its shortest address, recording the rule, its parameters and the kept
  values, so the change is undone exactly.
- **Edward's second answers (19 October 2026).** The aim is to shift the file towards an exact
  degree along the line, as near a whole degree (0 to 359) as possible, 0 being ideal. The rules
  will likely need a bitmask ruleset of their own later; for now a few parameters, written in the
  instructions as other filters' are. The kept values fix the wave, and that could be a parameter
  too, as could what a line of values is (its width) for propagation. Byte or bit tracking mode is
  the wave drawn through the kept values. The points round down, the centre a value for an odd
  length and the gap between two for an even one.
- **What landing on a degree costs (Claude).** A whole degree names one of 360 points, 8.5 bits.
  What else the instructions must hold is the walk from that degree to the file, and that is as
  long as the file is far from it: the instructions are the rule and its parameters, the kept
  values, the degree, and what is left. So a degree saves only as the wave leaves less behind,
  and 0 degrees is the ideal because there the walk is the whole address: the file after the wave
  is nearly all zeros. That is the reading the measurement below uses.
- **Measured (19 October 2026), tracking mode, value length 8 unless said.** The wave is drawn
  straight between the kept values at the start, 1/3, centre, 2/3 and end, taken away (wrapping
  at 256), and what is left numbered by its histogram (`digit-histogram-v1`'s size, exactly); the
  kept values are paid at 8 bits each. Bytes, after each:

  | File | Bytes | Histogram, no wave | Rope, one wave | Value by value | LZMA2 |
  |---|---|---|---|---|---|
  | Vitruvian, as grey pixels (949 × 1300) | 1,233,700 | 993,816 | 1,045,592 | 877,954 | 865,400 |
  | Vitruvian.jpg (already compressed) | 207,500 | 206,408 | 207,564 | 207,498 | 207,513 |
  | universalbibliothek.txt | 21,084 | 12,833 | 18,912 | 17,308 | 8,490 |
  | utah_teapot.obj | 248,968 | 124,493 | 189,202 | 158,037 | 34,380 |
  | hurrian-hymn-6.mid | 1,377 | 718 | 1,381 | 912 | 172 |
  | tones, 16-bit sound (made here, value length 16) | 110,240 | 104,848 | 108,932 | 88,526 | 58,173 |
  | the trained model's .safetensors | 34,072 | 23,924 | 34,059 | 25,773 | 5,576 |

  One wave through five points across a whole file is too coarse: on every file it leaves more
  than the histogram does with no wave at all. On the pixels, with the file read as lines of the
  picture's width:

  | Way | Bytes |
  |---|---|
  | the rope drawn through each line's own five points | 1,056,133 |
  | propagation 1: each line's wave is the line before | 894,073 |
  | the line before, corrected by a rope through the difference | 967,990 |
  | two phases: the line before and the value before, averaged | **843,855** |
  | LZMA2 | 865,400 |

  So the part of the idea that wins is **propagation**: a wave taken from the values next to each
  value (the line before, the value before). Two phases name the pixels in 2.5% less than LZMA2.
  The five kept points do not, at any scale tried: they are too few to follow a picture, a sound
  or a text, and paying for them costs more than they save. Text, meshes, MIDI and models are not
  smooth, so no wave helps them; LZMA2's repeats are what work there.
- **Edward (19 October 2026):** make it a filter that works across all dimensions, for the edge
  cases, even where it is expected to win only on some kinds of file; note everything down first,
  then make a start.
- **`rope-wave-v1`, as it is to be built (Claude, 19 October 2026).** A filter here means an
  ordering: a way to number a line's units, so that the units the wave predicts well come first and
  have short numbers. On the binary line a unit is a file's bytes, so it serves every file; on the
  other lines a unit's digits, so every dimension. Its parameters, all written with what it numbers:
  - `bits` L, the value length: 8, 16 or 32 (a file's bytes read as values of L bits, little-endian;
    bytes past the last whole value are kept as they are). On a line other than the binary line,
    the line's own digit.
  - `width` W, the values in a line, for propagation (1 for a stream with no lines).
  - `step` S, how far back "the value before" is (1 for neighbours; 3 for pixels of three colours).
  - `phases` P, Edward's 0/1/2/3: 0, no propagation, the rope alone; 1, the value before (S back,
    in the same line); 2, the value before and the line before (W back), averaged; 3, the value
    before plus the line before less the one diagonal between them, held within the values (as
    three phases follow one another). A value with no neighbour in a phase falls back to the
    phases below it, and in the end to the rope.
  - `keep` (0 or 1): the kept values, at the start, a third, the centre (only for an odd count: an
    even count's centre is a gap), two thirds and the end; the rope is drawn straight between
    them, rounded down, and is the wave where no neighbour is. Without `keep` the rope is 0.
  - What is left, each value less its prediction wrapping at 2^L, is cut into L/8 planes of bytes
    (the low byte of every value, then the next), each plane a segment, and numbered by
    `digit-histogram-v1`; the kept values and the trailing bytes follow, as raw bytes.
  - **Pulling tight** is the search over the parameters for the shortest number, with the time
    the File Locator's search times set (a file's and the run's): every value length, phase, step and keep,
    and widths found by where the file best matches itself shifted.
- **Measured (19 October 2026), the definitions above, before they were built:**

  | File | Bytes | LZMA2 | Best rope-wave | Its parameters |
  |---|---|---|---|---|
  | Vitruvian, grey pixels | 1,233,700 | 865,400 | **831,533** | L8 W949 S1 P3 |
  | Vitruvian, colour pixels at 475 × 650 | 926,250 | 623,717 | **511,659** | L8 W1425 S1 P3 |
  | tones, 16-bit sound (made here) | 110,240 | 58,173 | 75,540 | L16 W1 P2 |
  | universalbibliothek.txt | 21,084 | 8,490 | 12,833 | no wave |

  `keep` never helped by more than a few bytes. In colour, a step of 1 beats a step of 3: the third
  phase then predicts each colour from the one beside it and the pixel above. The made tones repeat
  exactly, which LZMA2 finds and no wave does; a recording would differ.
- **Built (19 October 2026):** `rope-wave-v1` (SPECIFICATIONS §12.2), with these changes from
  the plan above, each for a measured reason:
  - What the wave leaves is coded by `sieve-weights-v1`'s tables and rANS, not numbered by
    `digit-histogram-v1`: that one's exact numbering takes time growing with the square of the
    length (0.03 s at 8,192 digits, too long at a megabyte). The sizes are within a few bytes of
    each other.
  - The search found waves better than the hand-made ones: on the colour pixels a line of 3
    values (each colour from the one beside it and the same colour a pixel before), 494,774
    bytes against LZMA2's 623,717; on a made sine, the sine's own period (a width of 2,639
    values, 21 cycles).
  - `sieve rope FILE` searches, codes and checks; `sieve-manifest-v8` carries a file so where it
    is smaller than LZMA2 makes it; the oracle, written apart from Sieve's code, decodes both.
  - Across all dimensions, as Edward asked: it takes any file's bytes, so it serves the binary
    line, and through it every file. *Not yet:* the other lines' places coded by it (a line's
    units are its digits, its width the line's own).
  - *Since 19 October 2026* the weighing (`sieve locate --weigh`, the File Locator's table) weighs
    every file's rope code as a way to name it.
  - **Columns (`rope-wave-v2`, 19 October 2026).** From Edward's stress test: pi to a million
    digits as text (1,111,103 bytes: 7z 512,745, `.sieve` 490,473, rebuilt byte for byte). The
    digits' own floor is 415,241 bytes (log2(10) bits a digit); what stood between was the layout,
    spaces and line ends priced as if they could be anywhere. Counting each column of the line
    apart, a column of one byte value costing its table alone, a copy laid out in groups of ten
    came to 420,025 bytes (LZMA2 486,077), and the colour picture to 468,078 (was 494,774). Finding
    a noisy file's line length needed the common divisor of its best lags (its multiples score as
    well as it does); and the widths are no longer only the three best lags, so a picture of three
    colours finds its rows as well as its pixel. Instructions with it are `sieve-manifest-v9`.
  - **Columns by line (`rope-wave-v3`, 19 October 2026).** On Edward's own `pi6.txt` v2 gained
    nothing: its lines are 55 and 54 bytes, with a blank line after every twenty and a form feed
    in every tenth, so a fixed width drifts a byte at each form feed. Counting a byte's column from
    the start of its line instead gives 421,306 bytes (v1 490,293; LZMA2 505,978; floor 415,241);
    his file as Sieve instructions, 421,491 against 7z's 512,745. Instructions with it are
    `sieve-manifest-v10`.
  - **Shared tables (`rope-wave-v4`, 19 October 2026).** v3 paid a table and a stream for each
    of `pi6.txt`'s 56 columns, though fifty of them hold the same thing, digits spread evenly.
    Columns that hold alike now share one (` G1`), each column's group number written first:
    his file comes to 418,927 bytes (as instructions 419,115, `sieve-manifest-v11`), a table of
    200,000 digits to 83,225 against its floor of 83,048 (v2 87,967), and the grey picture to
    831,022 (v1 832,146), the first time columns have won on it. What is left of `pi6.txt`'s gap
    to the floor is its layout, not its tables: summed column by column its entropy is 418,604
    bytes, 323 under the code. Of the rest, 2,500 bytes are its last column, a space on one line
    and a line feed on the next, which counting by column cannot see; about 900 its blank lines
    and form feeds. *Proposed next:* a byte's context taken from the byte above it (the same
    column on the line before), which would see the alternation.
  - **The byte above (`rope-wave-v5`, 21 October 2026).** Measured first: counted by its column
    and the byte above, `pi6.txt`'s entropy is 415,679 bytes, 438 over the floor (by its column
    alone, 418,604). Naming each of the 566 pairs would cost more than it saves, so each pair
    takes the next group number the first time it is met, and the numbers are coded as a stream
    of their own (3 groups). His file: 416,967 bytes, as Sieve instructions 417,139 (were 419,096),
    0.46% over the floor of 415,241. What is left is mostly the blank lines (one after every
    twenty lines, which the byte above cannot count) and the tables. Classing the byte above by
    hand (digit, space) did as well on pi but would be tuned to it; the pairs serve any text.
- **As first proposed (done since):** a measurement before a filter: on a handful of files of
  different kinds, fit waves of a few simple families by value type and length, subtract, and set
  the residuals' histogram-coded size, plus the wave's parameters, against the file's own size and
  against LZMA2; if a family won on some kind of file, it would become a versioned filter and a way
  to carry a file in the instructions. The measurements above are that step, and `rope-wave-v1` to
  `v4` and `sieve-manifest-v8` to `v11` the filter and the way. *Still open:* more phases, a wave
  fitted per line, value lengths of 16 and 32 for sound and floats, the byte above as a byte's
  context (above), and the other lines' places coded by it.

**19.88 Other compression schemes layered in, last (Edward, 19 October 2026).** His rule: the
layering of other compression schemes and ways of saving takes a back seat, to be among the last
things looked at, so that the basic utility of the tool is proven first. Kept here so it is not
lost:
- **JPEGs, losslessly smaller.** A JPEG is already compressed, so the instructions gain little on
  it (Edward's photo, 4,047,272 bytes: 7z 4,034,819, its `.sieve` 4,031,460, rebuilt byte for
  byte). The gain there is a step below the pixels: read the JPEG's stored frequency values out
  of its entropy coding without making pixels, code them better, and write back exactly the same
  file with its own tables (as Lepton, packJPG and JPEG XL's lossless JPEG transcoding do, about
  20% smaller).
- **ffmpeg cannot do it.** Decoding to pixels and encoding again gives a different file: another
  encoder's tables, pixels already rounded, metadata and layout not kept.
- **Two ways, for when it is taken up:**
  - an optional outside tool, as ffmpeg is (`cjxl` and `djxl` from libjxl), which whoever
    installs the `.sieve` would need too;
  - or Sieve's own reader and writer of baseline JPEG's coding (a few hundred lines; progressive
    JPEGs more), the values then coded by the rope or a coder of their own, checked by the oracle,
    with nothing from outside. The second fits the aim of proving it within Sieve; `cjxl` is the
    measure to set it against.
- The same holds for other formats that are compressed already (PNG's deflate, MP3, MP4): each is
  a scheme to undo exactly and redo, and each is for later.
- **PNGs (Edward, 20 October 2026):** he found a PNG's `.sieve` larger than the PNG. A PNG saved
  at zlib's strongest leaves nothing for LZMA2 or the rope (`xz -9e` grows it too), so what the
  instructions add is all that changes: 184 bytes on 270,448 of noise, 205 on a 715,596-byte
  photo. `sieve-manifest-v12` cuts that to 145 (the file carried as it is, no empty items line);
  the rest is its name, size and the SHA-256 that checks it, which no format of this kind can
  leave out. What would make a PNG smaller is its pixels, under the deflate: undone and redone
  exactly, as the JPEG above.
- **Two photos (Edward, 20 October 2026), 3096 by 4128:** 4,139,682 bytes, 7z 4,127,434, `.sieve`
  4,122,488, the 7z's `.sieve` 4,112,687; and 4,587,473, 7z 4,575,201, `.sieve` 4,569,484, the 7z's
  `.sieve` 4,559,259. The 7z's instructions are the smallest: LZMA2 finds what repeats (its
  headers and tables) and the rope how unevenly the bytes are spread, and 7z leaves what it cannot
  pack as it is, so the rope still finds that in the 7z. The two gains nearly add up (12,248 and
  17,194 against 26,995). A file's instructions could take both, LZMA2's matches and the rope's
  coding together: done the next day, below.
- **Done, in Sieve's own ways (`sieve-manifest-v13`, 20 October 2026):** with his originals
  (uploaded in a zip) the split was plain: the first photo's metadata, 17,733 bytes, packs by
  LZMA2 to 4,678 and its picture, 4,121,949 bytes, not at all, while the rope's counts take the
  picture to about 4,105,653. A file is now cut into pieces, LZMA2's and the rope's, found from
  its bytes alone (no reading of JPEG): his photos come to 4,111,592 and 4,558,132, below 7z
  (4,127,434 and 4,575,201) and below his `.7z.sieve` (4,112,669 and 4,559,241), the cut within
  5 bytes of where the metadata ends. Nothing from outside is used: LZMA2 was Sieve's already.
- **Archives (Edward, 20 October 2026):** one box in the File Locator, **"Optimise Archive
  Formats"**, off by default (a command-line flag to match): an archive remade exactly where
  Sieve can, else carried as its contents, said plainly in the instructions and on installing.
  Off, every file comes back byte for byte as now. Testing against 7z and others stays a
  measurement, never a part of Sieve. **Started 20 October 2026 (`sieve-manifest-v14`):**
  - **xz, remade exactly:** what it holds carried (pulled tight as any file), and the file remade
    on installing by liblzma's streaming encoder with the preset and check found (the first
    block's dictionary narrows the presets to try). Measured: every file `xz` 5.4.5 made here at
    -1, -6 and -9 came back byte for byte. `pi6.txt.xz` (507,092 bytes): 419,117.
  - **7z, as its contents:** Sieve has 7-Zip's decoder (the installer's) but not its encoder, so a
    7z is never remade; its contents go in a folder named after it, as `sieve-install` unpacks
    one, a `u` line saying so and `sieve install` saying so too. A 7z of `pi6.txt` made here
    (507,182 bytes): 419,128.
  - **Only where clearly smaller** (64 bytes and 0.5% under the archive as it is, its contents
    estimated by LZMA2 and, for one file, the rope): a small 7z, or an xz of text LZMA2 packs as
    well, stays as it is.
  - **gzip and zip (`sieve-manifest-v17`, 21 October 2026):** a gzip is remade exactly where
    zlib at some level gives its deflate back (Python's always; GNU gzip's -9 of `pi6.txt` too, at
    level 6 and memory level 9, though of three smaller files only one matched), else carried as
    what it holds; a zip as its contents. `pi6.txt` as a gzip (546,741) 417,128; as a zip
    (546,827) 417,107; a `.tar.gz` (557,241) as its `.tar`, 425,767.
  - **Tar needs nothing:** the pieces already cut a tar into its headers (LZMA2) and its files (the
    rope): a tar of `pi6.txt` and a text, 425,701 as instructions, the two files alone 425,597.
  - **The aim (Edward, 20 October 2026): every archive format there is,** in the long run. In
    tiers: contents for whatever Sieve can read (next: bzip2, zstd, then cab and others, each
    decoder taken where its licence allows; RAR's has conditions, to check first); remade exactly
    where Sieve has the very compressor; and, for deflate made by other than zlib (zip, gzip and
    PNG alike), the small differences from Sieve's own deflate recorded, so any deflate is remade
    exactly (as preflate does), the largest piece.
  - **Not yet:** an xz written in several blocks (`xz -T`); a gzip of several members.
  - The versions compared: GNU gzip 1.12 against zlib 1.3, and the `xz` program 5.4.5 against the
    liblzma Sieve was built with here.

**19.89 ~~Sieve instructions for the internet: size and decoding time as one setting~~ (Edward, 20
October 2026; deprecated for now, §17.3).** Another box: optimise for file size and decoding time
together, as one parameter, to make a set of `.sieve` files fit to serve as a format for the
internet. Set aside the same day: Edward recalls decoding as taking fractions of a second, so
there is little time to trade for size.

**19.90 The listing in bytes (Edward, 20 October 2026; built 21 October).** His thought: what the
instructions write to find a file could be packed into bitmasks rather than plain text. On a small
file the listing was most of what Sieve added: a 1,597-byte script's was 142 bytes, 70 of them the
tree's SHA-256 written as hex, and his own script's instructions came out 10 bytes over 7z's.
`sieve-manifest-v16` writes the head in bytes (flags, varints, the SHA-256 as its 32 bytes) and
keeps the rest, the entries, as the text they are, packed as before where that is smaller: that
listing is now 67 bytes, `pi6.txt` 417,061, the trained model 4,185. *Still open:* the entries
themselves in bytes (each way a code and its numbers), where a listing of many small files would
gain most; measure first.

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

**19.92 Sieve for websites (Edward, 21 October 2026; an idea, nothing built).** His thought: a
quick browser plugin that hands the `.sieve` files it finds online to `sieve.exe` (through an input
folder) and shows the HTML they unpack to, as a first test where the result is known to be HTML.
- *The bar is brotli, not the raw page:* web servers already send pages compressed (gzip or
  brotli), and every browser unpacks them with no plugin. Measured on this repository's own pages,
  in bytes (the page, gzip -9, brotli at quality 11, Sieve's instructions as one file):
  index.html 4,773 / 1,894 / 1,434 / 1,909; sieve.html 3,878 / 1,497 / 1,121 / 1,525; audio.html
  6,345 / 2,451 / 1,840 / 2,452; quest.html 6,924 / 2,555 / 1,863 / 2,533; space.html 12,173 /
  3,989 / 3,215 / 3,897; specs.html 15,512 / 5,503 / 4,581 / 5,359; metrics.html 19,084 / 6,703 /
  5,577 / 6,461; release.html 29,841 / 9,707 / 8,147 / 9,273. Sieve is level with gzip and about
  15 to 25% behind brotli.
- *Why brotli is ahead:* it ships with a built-in dictionary of common web text, so even a small
  page starts out knowing HTML; Sieve starts each file from nothing, and on a page this small its
  name and SHA-256 (about 70 to 90 bytes) count.
- *The route that could close it:* a shared web dictionary, as brotli has. LZMA2 takes a preset
  dictionary, so Sieve could ship one made from HTML, CSS and JavaScript (a new version, refused
  by older readers). Measure it first on pages from outside this site, so it is not fitted to
  Edward's own HTML; a plugin is worth building only once it beats brotli.
- *The plugin, when it is worth it:* an extension cannot start `sieve.exe` itself. An input folder
  watched for downloads works for a test on one machine; native messaging is the browser's own way
  to reach a local program, but each user installs a helper; the decoder (LZMA2, the rope, the
  listing's reader) built as WebAssembly runs in the page or the extension, installs nothing and
  works everywhere, and is the route beyond a test. Any of them decodes files from strangers, so
  the decoder must treat every `.sieve` as hostile and never run what it unpacks.
- *A verifiable layer, from the SHA-256 (Edward, 21 October 2026):* every set of instructions
  carries the SHA-256 of what it unpacks to, and the reader checks it, so a plugin could show
  whether a page arrived exactly as made. What that proves depends on where the hash comes from:
  - *From the same file:* it proves the page was not damaged on the way, but not who made it.
    Whoever can change the file can change its hash with it, and HTTPS already guards the
    connection itself.
  - *From somewhere else:* this is where it becomes security. A link that carries the hash of the
    page it points to (as a `.sieve` address already names a file by its own bytes) lets the
    reader refuse any page that is not exactly the one the linking page meant: a mirror, a cache
    or a changed server cannot swap it unnoticed. The web has this for scripts and stylesheets
    (Subresource Integrity, an `integrity="sha256-…"` attribute), and content-addressed networks
    (IPFS) name everything this way; Sieve would bring it to whole pages and folders.
  - *Who made it:* a signature over the hash (the maker's key, published once) would add that,
    and would be a new version; nothing in Sieve signs today.
- *Measured (22 October 2026): a web dictionary.* Dictionaries of 32, 64 and 120 KB were learnt
  (zstd's trainer, its output used as LZMA2's preset dictionary) from 155 saved real-world pages of
  half of 90 sites (the npm package `htmlparser-benchmark`), and tried on 102 pages of the other
  half, which it never saw, and on 7 of Edward's pages; every stream decoded back. In bytes, the
  102 pages (10,144,997): gzip -9 2,095,915; brotli 1,654,224; LZMA2 1,832,710; with 32 KB
  1,728,489, 64 KB 1,684,831, 120 KB 1,646,408 (10.2% below LZMA2 alone, 0.5% below brotli,
  smaller than brotli on 66 of the 102). Edward's 7 pages (92,185): brotli 25,938; LZMA2 30,556;
  with 120 KB 27,638 (6.6% over brotli, on none smaller), and Sieve's listing adds 70 to 90 bytes
  a file besides. So a dictionary of brotli's size draws level on pages of 20 KB and more and
  stays behind on small ones, where brotli's own modelling of its dictionary (words and their
  transforms) is the stronger. *Verdict: not yet a new version;* the dictionary is the right
  route, and small pages want more than a dictionary (brotli's context modelling, or the rope on
  text). The websites were not reachable from the session, so the pages are a 2014 snapshot;
  measure again on today's pages before building.
- *The decoder plugin, planned (Edward, 22 October 2026).* His notes, kept in his words where they
  are quoted:
  - *A first plugin for Chrome, tested locally,* pointing at `sieve.exe`: "from there it is pretty
    much just check hash and hand over the file to the browser, that way we don't have to worry
    about the type of file, image, page, etc." It decodes a `.sieve` "to its original format and
    pass it to the browser for now like it would a normal file."
  - *Test pages* that link to one another, one with a slideshow of the `.sieve` files in an images
    folder beside it, "refusing them if the hash doesn't match within the display itself."
  - *Links that name hashes:* "with the use of hashes and the potential for pages to point to the
    hashes of any links in the page we could create an automated pass through mechanism to
    routinely check websites for mismatches among the hashes. If a hash mismatch occurs the plugin
    should just refuse the .sieve file altogether." This needs "a custom plugin or utilising an
    existing html system to have links point to hashes."
  - *Why the link, not the file:* "files carrying their own hashes is a vulnerability in and of
    itself; an attacker would just create their own sieve file with their own hash. But, if we
    utilise an automated harness for updating html pages or entire websites, so that links also
    point to hashes for the .sieves themselves, each and every page becomes an immutable link in a
    chain of hashes."
  - *Single files only?* "I think that the web plugin decoder should only accept single file
    .sieve's and not multi file .sieve files though I am not sure."
- *What follows from them (Claude, 22 October 2026).*
  - *Reaching `sieve.exe`:* an extension cannot start a program itself; Chrome's own way is native
    messaging. The program is registered once (a small JSON file naming it and the extension
    allowed to call it, and on Windows a registry key under
    `HKCU\Software\Google\Chrome\NativeMessagingHosts`), and Chrome starts it with the
    extension's origin as its argument and talks to it over standard input and output, each
    message a 4-byte length and JSON. A message from the program to the browser is at most 1 MB,
    so a decoded file comes back in pieces. An unpacked extension's id depends on its folder
    unless its manifest carries a key, so the test extension carries one, to be named in the
    program's JSON. WebAssembly (the decoder built into the extension) remains the route beyond a
    test: nothing to install, and every browser.
  - *Which hash a link names:* the `.sieve` file's own SHA-256, checked before a byte of it is
    decoded, so a file that is not the one the page meant never reaches the decoder. The page can
    say so with the attribute the web already has for this, Subresource Integrity
    (`integrity="sha256-<base64>"`, which browsers enforce on scripts and stylesheets and leave
    alone on images), so the tools that write it already exist.
  - *A chain of hashes cannot go round in a circle:* if page A holds B's hash and B holds A's,
    changing either changes both, without end. Hashes held in links make a tree (a page holds its
    images' and its children's), not a web of pages that link back. For a whole site, one file
    holds every page's hash, and every page is checked against it: that file is a map
    (`sieve-manifest-v18`, §19.93, which already names files by SHA-256), and its own hash is the
    one root to trust, pinned in the plugin or published apart from the site, and later signed.
    The harness that rewrites a site writes the links' hashes and the map together; the checker
    that routinely walks a site compares what it fetches with the map.
  - *Single files:* agreed, for showing. A link names one thing, so a single-file `.sieve` stands
    for it in an `img`, a link or a page. A `.sieve` of many files has no one thing to show; it is
    offered as a download, or (later) served file by file to the page that holds it, never
    unpacked onto the disk by the plugin.
  - *Hostile files:* whatever the plugin decodes comes from strangers. Limits on the decoded size
    and the time, no items of Sieve's lines (they ask for lines and models) and no remade archives
    in the browser for now, and a decoded page shown in a sandboxed frame, so its scripts get none
    of the extension's powers.
- *Edward's answer (22 October 2026):* "A global list of hashes for the .sieve's themselves is a
  good idea. I still think each link should state the hash of the sieve page it points to though.
  Eventually a crawler could be created to routinely check hashes across links for servers. In
  theory a multi-sieve file could unpack to a basic directory tree listing, that might be
  beneficial in the future, make note of it for now." So: every link to a `.sieve` states its hash,
  the global list (the site's map) comes with the site harness, the crawler after it, and a `.sieve`
  of many files is shown one day as a listing of its folders and files (noted, not built).
- *Links that go round, stated anyway (Claude, a way to weigh later):* a page can state the hash of
  a page that states its own only if a page's hash is taken with the hashes in its links left out
  (blanked before hashing). Then each link's hash checks the page it names, but nothing checks the
  hashes a page states, so the site's list must hold every page's full hash as well; that list is
  what makes it sound.
- *Built, phase 1 (23 October 2026):* `web/README.md`.
  - `sieve web-host` (`tools/cli/web_host.*`): native messaging over standard input and output,
    started by Chrome with the extension's origin as its argument; `hello`, and `decode` of a
    `.sieve` given as base64 with the hash its link states. It checks that hash before decoding,
    accepts one file only (no items, remade archives, archives carried as contents or maps; at
    most 256 MiB), checks the file against its listing (its own SHA-256, or v7's tree), and answers
    with its name, media type, size and SHA-256, then the bytes in parts of 699,999 (base64 under
    native messaging's 1 MB).
  - The extension (`web/extension`, Manifest V3, its id fixed by the public key in its manifest,
    `bnigebolckfoapimidlcmbilcbhjbmjg`): its background fetches a `.sieve`, checks its SHA-256
    against the link's `integrity` before anything else, and asks Sieve to decode it; the content
    script puts decoded pictures in place, or a red refusal; links to `.sieve` files open the
    viewer, which shows a page in a frame sandboxed with no scripts (its `.sieve` pictures put in
    as data, its `.sieve` links back through the viewer), and a picture, sound, film or text as
    itself.
  - Registering the host: `web/host/install_host.bat` (Chrome and Edge, this user's registry) and
    `install_host.sh` (Linux and Mac); `uninstall_host.bat`.
  - The test site (`web/test-site`, built by `make_site.py` with the `sieve` given): two plain
    pages, two `.sieve` pages, a slideshow of seven `.sieve` pictures with one byte changed in one,
    and links with a wrong hash, no hash, and a `.sieve` of all six pictures; the pictures are the
    hallway's screenshots and the public-domain Vitruvian Man, as JPEGs.
  - Tested here in Chromium 141 (headless, the extension loaded, the host registered): six pictures
    of the slideshow decoded and shown (800 by 450, and 800 by 1,096), the changed one refused in
    the slideshow ("its SHA-256 is not the one its link states"); About shown in its frame with its
    own `.sieve` picture; the wrong hash, no hash and the six files refused with their reasons. The
    CI checks the host as Chrome talks to it (`web/host/check_host.py`), builds the site, and checks
    the extension's scripts parse.
  - *On the website (Edward, 23 October 2026: "add a page to the github pages site ... secure.html
    which will explain how to set the test website up from the repository in as few steps as
    possible, from there it can link to a built version for people to test live online"):*
    `Pages/secure.html`, with the test site built into `Pages/secure` and served by GitHub Pages
    like any other file; the extension needs nothing of the server beyond the files.
- *The site harness, `sieve site` (Edward, 23 October 2026: "I presume that script and src folder
  could be repurposed in the future for any website? I ask as I notice vitruvian man reference
  hardcoded in the py script that makes it"; built the same day, `tools/cli/site.*`):*
  - `sieve site build SRC --out OUT [--sieve-pages] [--plain LIST] [--sieve LIST]` takes any site's
    folder as it is, with plain links (`<img src="pictures/door.jpg">`, `<a href="notes.html">`),
    and writes it with its pictures (and, with `--sieve-pages`, its pages but for every `index.html` and
    those kept plain) made into `.sieve` files, each link and picture that names one changed to
    the `.sieve` and given that file's `integrity`. Nothing else in a page changes: only those
    attributes are edited, in the page's own text.
  - A page is made after the pages it names, so the hash it states is final; pages that name one
    another are refused, naming them, with the answer (keep one plain, so the others name it by
    name only). Last it writes `site.sieve`, the global list: a map (`sieve-manifest-v18`) of every
    file by its SHA-256, and prints that map's own SHA-256, the one hash to trust for the site.
  - `sieve site check DIR` reads a built site as the extension would (the `.sieve` pages decoded
    as the host decodes them, against their own hash) and lists every stated hash and every map
    entry that does not match, and every file the map does not name; it is the crawler's first
    half, on a folder instead of a server.
  - A tag with `data-sieve-keep` is left exactly as written and not checked: the test site's links
    meant to be wrong. The test site's own `make_site.py` now only adds what is the test's (the link
    home, the many-file `pages.sieve`, the tampered picture and its hash) and calls `sieve site`.
  - Measured here (23 October 2026): the test site, 13 files, 8 made into `.sieve` files (2 of them
    pages), 11 links given their hashes, built in 14.6 s with the default second of search a file;
    `site.sieve` 863 bytes; checked 24, none wrong, 3 kept as written. Built twice, every file came
    out the same. Shown again in Chromium 141: the same six pictures shown and the tampered one,
    the wrong hash, the missing hash and the many files refused.
  - *Tried on the Potentia website, and PHP and scripts (Edward, 23 October 2026: "I am also
    wondering how this will perform with php, javascript and other web languages as they have
    their own syntax and can open links as well"):* the first build made 90 pictures into
    `.sieve` files that no page links to, so anything else naming them (a script, CSS) would have
    named files no longer there; it found the pages' loop only after 3.5 minutes of making
    pictures, and left that half-made folder behind, which then blocked the next build. Now:
    - only a file a tag the extension reads names is made; a file named anywhere else (CSS, a
      script, PHP's code, `srcset`, `style`, any other attribute; found by its name, as a whole
      name) is kept as it is too, beside its `.sieve`, and `--keep-originals` keeps every one, for
      scripts that put names together;
    - a page with a script stays plain (the viewer runs none), and a `.php` page is read for its
      tags, its HTML rewritten and its `<?php ... ?>` left untouched (`php -l` passes);
    - loops are found before anything is made, shown (`a.html -> b.html -> a.html`), with a
      `--plain` list that breaks every one (from each loop, the page most others name); a failed
      build leaves nothing; it says which file it is making;
    - the extension takes away a picture's `srcset` and its `<picture>`'s `<source>`s before
      showing the `.sieve`: those win over `src`, and showed the unchecked file (seen in Chromium,
      fixed and shown: five pictures, all from checked files).
    The website with `--sieve-pages`: 101 files, its 3 pages with no script made in 2.9 s, its 5
    with scripts kept plain, its 90 pictures copied as they are (no page shows one yet).
  - *What PHP writes when it runs* cannot be seen beforehand: build the pages it serves (saved
    from the running site), or have it write each link's `integrity` from `site.sieve` (a small
    PHP helper, not built). Links from the server's root (`/a.jpg`) are not read yet; the
    extension does not read `site.sieve` yet.

**19.93 The `.map` format folded into `.sieve` (Edward, 22 October 2026; built the same day).**
His thought: update the `.map` format, or replace it with `.sieve` and deprecate it, the
differences once needed for the 3D node graph perhaps being redundant now.
- *What a map has that instructions lack* (SPECIFICATIONS §12.3): files **named but not held**
  (a size and SHA-256, a claim that the bytes exist somewhere), **edges** with a relation
  (`contains`, `anchor`, any word), **metadata** on a node, a **seal**, and **held** files whose
  bytes it carries as they are.
- *What the node graph needs from it:* none of that is 3D. A map stores no position; the graph
  lays it out afresh each time (force-directed, starting from each node's id), so nothing of the
  3D view is in the file, and that difference is already gone.
- *What instructions have that a map lacks:* every way of packing (LZMA2 as now chosen, the
  x86 stream, the rope, pieces, items, archives remade), the binary head, and installing.
- *Why it was set aside before (11 October 2026, §16.3):* every `.sieve` would have had to allow
  for entries only maps need. Since v16 that no longer costs anything: the head's flags byte has
  two bits free (64, 128), so a map's parts would be present only where a flag says so, and
  ordinary instructions would be the same bytes as now.
- *Recommended:* one format. A `.sieve` whose head may flag a **graph part** (named-only entries,
  edges, metadata, a seal), its held files carried and packed as any instructions' files are; a
  new manifest version, refused by older readers. The node graph opens `.sieve` files as maps;
  `sieve install` puts back the held files and lists the named-only ones (it cannot make them);
  `sieve map` writes `.sieve`. The `.map` formats (v1, v2) stay readable, with a conversion
  (`sieve map OLD.map --to NEW.sieve`), and nothing writes them any more. The DOT and GraphML
  exports stay. Held anchors gain what installers gain (§16.3: a map of texts or programs).
- *To weigh first:* the release map is sealed and its SHA-256 published, so it would be
  republished as a `.sieve` with a new hash; "This installation" is made fresh each time and
  would simply be made as a `.sieve`. Edward to decide before it is built.
- *Edward's decision (22 October 2026):* build it; the release's map becomes `source.sieve`
  rather than `sieve.map`, and any `.sieve` opens as a node graph, perhaps without notes.
- *Built (22 October 2026):* `sieve-manifest-v18` (SPECIFICATIONS §12.2, §12.3). The graph part
  is the map's own text, every line behind `g<TAB>`, after the entries; head byte 18, flag 64.
  The held files are the instructions' files, packed as any are; the named-only nodes are in the
  text alone, and `sieve install` says how many it does not carry. `sieve map` writes `.sieve`
  and refuses `--out NAME.map`; older `.map` files are read, and `--out NAME.sieve` converts one
  (the conversion is `--out`, not the `--to` first suggested above). The hallway lists `.sieve`
  and `.map` in `maps/`, saves every change as `.sieve`, and shows instructions without a graph
  part as the map of their tree, read-only. `make_release` publishes `source.sieve`, checked by
  installing the source back from it. The oracle reads v18 (`unpack`, `graph`). Measured: a map of
  a 6-byte and a 3,000-byte file, named only, 271 bytes; the same two named and a 200,000-byte file
  of noise held, sealed, 200,353 bytes.

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

**19.95 The release's own downloads were v4 (22 October 2026; fixed).** Edward found
`source.sieve` larger than the source's 7z, and asked whether `make_release` used an old version.
It did, for the instructions and the installer program: `sieve locate --installer` without
`--items` makes `sieve-manifest-v4`, and the script left `--items` out. `source.sieve`, a map, was
always the newest. Measured on this release's folder (Sieve 0.13.25, 127 files, 39,172,603 bytes, made
by `make_release` here on Linux): v4 7,939,263 bytes, the newest (v16) 7,819,304, 7z 8,414,101; the
installer program installs it byte for byte, and no file of the release is an item, so
`sieve-install` needs nothing it lacks. On a smaller folder (the `sieve` program and the docs, 8.4
MB) 7z still wins (2,836,214 against 2,949,958), its x86 filter being BCJ2 (calls and jumps in
streams of their own) against Sieve's BCJ: a lead to measure next.
`source.sieve` holds the 7z as it is, so it is the 7z and the map's text, a few hundred bytes either
way (here 47,779,761 against the 7z's 47,780,051; holding the source as a tar instead,
47,594,737). Making it took 19.5 minutes, nearly all the rope's search with no limit on a file it
cannot shorten; maps now give each held file 10 seconds (`sieve map --search-per-file`, 0 for no
limit), and `make_release` passes `--items`; the release's map, so made, came to 47,779,886 bytes
(naming the two downloads and holding the 7z of 47,780,052), the whole release in 635 seconds.

**19.96 Every dimension at every degree, and the starting page, as `.sieve` (Edward, 22 October
2026; measured).** His ask: the sample of every dimension at each whole degree (§16.9,
`hallway --sample-degrees`), "which should be tiny", and "welcome to the sieve" ("1 - 0 - 112",
§9) as `.sieve` files. Made here with the newest version (`sieve locate … --items`), each
installed back byte for byte:
- *The sample* (3,960 files, 13,321,756 bytes): 8,784 bytes (the hallway's own, v4, 482,495; 7z
  22,961). 1,368 files are items, their places 1,492,272 bytes carried in 1,293: a whole degree's
  place is a fraction of the line with a short repeating expansion, so the places repeat, and
  LZMA2 takes the repeats. Each dimension alone, its `.sieve` against its 7z: AI 1,531 / 6,756;
  audio 755 / 1,326; binary 1,044 / 1,548; books 993 / 1,649; image 811 / 3,144; models 1,123 /
  1,591; movies 1,216 / 2,758; pages 893 / 1,378; tracks 801 / 1,400; video 661 / 2,314; worlds
  910 / 3,366.
- *The starting page* (32 bytes as the file the hallway saves): 162 bytes (v4 196), the page an
  item of the pages line, its place 20 bytes; the rest is the instructions' head, the tree's
  SHA-256 (32 bytes) and the file's name. "1 - 0 - 112" is the idealised form (§9) and is not a
  format.
- The hallway's `--sample-degrees` still writes v4; the CI checks that the newest made from the
  sample is smaller than it, so changing it waits for a check of its own.

**19.97 A time estimate in the File Locator, and cutting into pieces made cheaper (Edward, 22
October 2026; built).** His ask: "We will want to show a time estimate for compressing files in
the file locator", after a 700 MB file that "even with a limit set it never finished in the
allotted time".
- *What reading a file does:* its address, zip's and 7z's sizes, its instructions packed (and
  compressed again for the table), and the weighing, which looks at its pieces and searches the
  rope again; saving packs it once more. The search's limit covers the rope's search only.
- *The estimate, as built:* for 16 MiB and more, the reading's work is timed here on 1 MiB and
  then 2 MiB, each taken in eight stretches evenly across the file (a folder's largest), with the
  search as near none as the packer allows; a straight line through the two is drawn out to the
  whole size, and the most the search may take is added (packing: twice the time for a file, the
  file whole and then its pieces, for each file of 512 bytes or more; the weighing: the run's
  time). With no limit, it says so. While it works, the line under the status gives the estimate
  and the time so far; when done, "took X (estimated Y)". Saving's estimate is the packing's time
  as it was measured when the file was read, and the search's.
- *What it found:* timed first on the file's first 2 MiB, a 40 MB tar of the source was estimated
  at 4 min 33 s and took 1 h 08 min. The time went to cutting into pieces: each boundary between a
  piece for LZMA2 and one for the rope was moved in steps of 64 bytes, up to a block either way,
  and at every step the whole LZMA2 run beside it (up to 1 MiB) was packed again, so a file of
  many runs spent minutes on each boundary, and the file's start, with few, said nothing of it.
  Now each step is weighed over a window of at most 64 KiB beyond the steps, fixed while the
  boundary moves (SPECIFICATIONS §12.2). Measured: the varied set (16 files, one of them cut into
  pieces) byte for byte the same instructions, two JPEGs faster (12.8 s → 7.8 s, 10.2 s → 6.6 s);
  the 40 MB tar packed in 1,245 s (2,053 s before), the same bytes.
- *Measured, estimate against time taken, the search 5 s a file:* 40 MB of a 7z (already packed),
  3 min 03 s against 3 min 46 s; the 40 MB tar, 38 min 34 s against 45 min 11 s (68 min before the
  window). Both 15 to 20% short; why is not yet measured (the likely parts: the whole file's
  address worked out in memory as hex, and LZMA2's dictionary, which grows with the file to 64
  MiB, against the sample's 2 MiB).
- *Stop, and the best so far (Edward, 22 October 2026: "lets add a stop button so that we can
  choose what result to use", "ensuring the current timing message traces the best result found
  so far"; built).* While it works, the File Locator has **Stop (keep the best so far)**, and S.
  Every trial not needed for a result ends when asked (`sieve/stop.hpp`, a flag for the thread
  doing the work): LZMA2's settings and its further passes, 7-Zip's encoder, cutting into pieces
  (the boundaries stay where the blocks put them, or the file is not cut), the rope's search (its
  best kept, and not cached as a finished one), the comparison's zip and 7z (in pieces of a MiB,
  shown as not measured), tailoring and the weighing. One LZMA2 pass of each stream is always made,
  so what is stopped installs. Above the estimate, a line says what it is doing and the best so far
  ("the lzma2 stream (S bytes): best so far B bytes; packing it with 7-Zip's encoder"). Saving writes the instructions made when it was read where the settings are the same,
  with nothing packed again, so a stopped result is the one saved. Measured, the 40 MB tar stopped
  after 90 s: finished at 180 s (the pass under way, and the address), saved in 0.3 s, 27,868,382
  bytes against 27,832,122 unstopped, installed byte for byte. Scripted: `--stop-after S`,
  `--locate-save FILE`, and a CI check that a stopped result installs.
- *Force stop (Edward, 22 October 2026: "change the stop button over to a force stop to take the
  previous result rather than finish the current pass after pressing stop once"; built).* Pressed
  again, Stop becomes **Force stop (the last finished result)**: the pass under way is abandoned
  too. LZMA2 is fed a MiB at a time so that it can be (liblzma's streaming encoder, the same
  bytes), 7-Zip's encoder is broken off through its progress callback, and the file being pulled
  tight or cut into pieces is left as it is. Where a stream's first pass was abandoned, the lzma2
  stream is carried as it is (v12's way) and the x86 stream, which never is, packed at LZMA2's
  quickest preset; the listing and the items stream, small, are always finished. The estimate's
  timing of its sample stops too (then there is no estimate): it had been the longest wait, over a
  minute on a tar, taken across the file where its kinds of bytes change often. Measured, Stop at
  20 s and Force at 25 s: the 40 MB tar done at 27 s (stored as it is, 40,000,074 bytes); the
  source's 7z (47.8 MB) at 27 s; both installed byte for byte. Scripted: `--force-after S`.
- *A graph of the best so far (Edward, 22 October 2026: "a line graph display which showcases the
  best found results along the way, instead of storing each value we can just draw the line graph
  as we go"; built).* Each new best a step reports (LZMA2's passes, a stream at a time; the rope's
  search, a file at a time) is drawn at once onto a picture the File Locator keeps, and let go:
  each step's line starts at the top, its first try, and falls as it finds smaller, the bottom 1%
  below at first and doubled whenever a best goes deeper (what is drawn squeezed into the top
  half), the time across the estimate's, else 10 s, doubled the same way (squeezed into the left
  half). Only each step's first try is kept, to measure from. Large while it works; beside the
  result after. Seen on `pi6.txt`: the rope's line falls 16% within its first second. On a file
  already packed (Edward's 60.8 MB 7z, and 20 MB of the source's 7z here) nothing beats its first
  try, so every line runs flat along the top: drawn a few pixels inside the frame, with the words
  outside it, since at first such a line lay under the frame's edge and the title and looked
  missing (Edward, 22 October 2026: "I am only seeing a small line at the top during a search").
  While it works, Stop has a row of its own. `--shot-after S` takes a scripted picture while it
  works.
- *Still open:* the time the boundaries take grows with how many there are, and threads (§19.99)
  are the next step; the memory a very large file needs is not estimated yet (for 700 MB, about 4
  to 6 GB expected from how the address is made, not measured).

**19.98 Recurring values, scale repeats and a randomly locked byte (Edward, 22 October 2026;
measured or answered, nothing built).**
- *Recurring values:* "a filter that keeps an eye out for when recurring values might be able to be
  utilised to round up the rotation, ie: 0.3..., 0.0000004...", stored as "number of decimal places
  + the recurring digit", with a fallback recording in case another filter "shoots past it on the
  next pass". A value with some places and then a recurring block is a fraction whose denominator
  is 10^k (10^p − 1), so the strongest form writes a file's place as the nearest fraction with a
  small denominator and the offset from it. *Measured (Claude):* every denominator up to 2^64
  tried (the continued fraction's), paying for its size, the fraction and the offset: no gain on
  "hi", "Sieve", the README's first 64 bytes, IDEAS.md's first 1,024 bytes, `a.txt`, the starting
  page, a MIDI and a mesh; 2 bits on 2,048 bytes of noise (chance); and a page of the degrees sample
  ("rfb rfb rfb ...", §19.96), placed on a recurring value by construction, 256 → 78 bits. It finds
  what was put there, as the grids did (§19.94).
- *Scale repeats* (0.123456789 against 0.000000123456789): the same digits shifted, naming the
  shift costing what it saves. *Leading zeros and a mask of recurrence types* ("10 0's, then maybe
  bitmasking the type of recurrence"): leading zeros are dropped already (the shortest route,
  SPECIFICATIONS §3.6), and a mask of T types costs log2 T bits, within what the test charged. On
  the binary line a file's place is its own bytes read as a number, so a pattern in the digits is a
  pattern in the file, and describing it is compressing the file, which LZMA2 and the rope do
  already, more generally.
- *One ending roll:* Edward's own conclusion, agreed: the total is what counts (the place and every
  step recorded to reach it), a rounding made midway is undone by the next shift, so a describer of
  the number belongs at the end, as one more way of writing the address. Keeping the greatest
  reduction so far is enough where it is scored on that total; trying orders back and forth spends
  time, not bits.
- *"With filters already shifting the state spaces around, isn't there more room?"* Filters do
  not move the line: all 32 built pass or fail a unit, and the ones that count number their
  survivors in the line's own order. The line is reordered only by its ordering (positional,
  scrambled, guided), and only the guided one, built on a model of text, shortens what it moves.
  A filter gains by removing (fewer survivors), and only for files it keeps, because it knows
  something about real files ("welcome to the sieve": 152 bits, 138.8 under its tailored stack
  with the stack paid for).
- *A random byte locked in* (his note): "try picking a random byte in the file, locking it in as a
  requirement for the file in the state space, then try another/all bytes randomly ... the filter
  would store the offset and the byte recorded." §16 and §19.87 again: a locked byte keeps 1 file
  in 256 (8 bits saved) and costs its 8 bits and its offset (log2 of the file's size), whichever
  byte it is, so no byte is a better pick than another and the order they are tried in cannot
  matter; offsets from a generator agreed in advance (a seed) leave it break-even. §19.87 measured
  kept bytes with a wave drawn through them: worse than no wave on every file. What won was each
  byte predicted from its neighbours, the tightened rope. "Isn't every real file rare?" Yes, among
  all files of its length, and that is what compression uses; but it is rare in a structured way,
  which a model can learn (words, smooth pictures, repeats), not rare among its own bytes.
- *Keep watching for them (Edward, 22 October 2026):* "Through multi threading and parallelisation
  as well as possibly distributed processing in the future it could still be worth keeping an eye
  out for those reductions, presuming the current system wouldn't already find them." Recorded as
  a standing watch:
  - *What finds them now:* a file whose bytes recur (a recurring value on the binary line is its
    bytes repeating) is found by LZMA2 and the rope, which is why the degrees sample is 8,784 bytes.
    The weighing's shortest route drops leading zeros and tries a bearing and a walk; it does not
    try a fraction or a grid. So a place that is a short fraction on a line other than the binary
    line, or under a filter's survivor numbering, would go unseen today.
  - *The watch:* a fraction route (the continued fraction of the place, every denominator to
    2^64, as measured above) added to the shortest route, run on every place the search produces.
    It takes microseconds a place, costs nothing in the instructions where it does not win (the
    route is chosen, and paid for, only where it is shorter), and so can sit under every search
    as more trials become affordable.
  - *What more trials buy:* a candidate lands n bits better than even with odds of about 1 in 2^n,
    so 2^k candidates tried bring the best of them about k bits nearer, and naming which of 2^k
    equally likely candidates it was costs k bits, however they are agreed. Threads and
    distributed search (§19.99) make k larger, so they find the lucky few bits sooner; they gain
    on average only where the candidates are not equally likely (a list agreed in advance whose
    first entries suit real files, §16's search vectors). The watch is cheap enough to run
    regardless, so a lucky file is never missed.

**19.99 Threads for the filters' search and the packer (Edward, 22 October 2026; an idea, nothing
built).** "We could look at multi-threading the filter analysis to improve performance." Threads
are used today by training (`training.cpp`), the language model (`llm.cpp`), the exhaustive counts
of `sieve sift` (`sieve.cpp`), the hallway's faces, items and vault, and the menu's survivor counts.
Single-threaded, and each made of separate trials whose results are compared:
- the tailoring search (COST's K, the File Locator's Tailor): each filter and setting tried is a
  count of survivors on its own;
- the weighing, file by file (`weigh.cpp`);
- the packer: LZMA2's settings tried on a stream's first MiB (seven), the full passes (liblzma at
  two match lengths, 7-Zip's), and cutting into pieces, where each 4 KiB block's LZMA2 and each
  step of a boundary are separate (§19.97: the boundaries were where a 40 MB tar spent its time);
- the rope's search over its ways.
The rule for all of them: the result must not depend on the number of threads, so each trial is
scored as now and the best taken by the same order, ties going to the earlier, exactly as one
thread takes them; the trials run at once, not in a different way. A search with a time limit is
the exception already (it depends on the machine's speed), and with threads it gets further in the
same time. Memory is the limit to watch: each LZMA2 pass at preset 9 takes its dictionary's worth
several times over (64 MiB at the most, about ten times that to encode), so a thread count from the
cores and from the memory free, as the menu's filter memory is given. First to measure: the pieces'
blocks and boundaries, and LZMA2's three full passes, on the 40 MB tar and the release folder.
- *Measured, then built (24 October 2026; Edward: "make a start on step 1"):* `tools/bench/bench_make.py
  SIEVE` builds a fixed set from this repository's own files (prose, code, a table, a word list, a
  PNG, JPEGs, a mesh, a MIDI, an MP4, a WAV made by the script, a Windows program, a tar of them;
  `--set full` adds the 52 MB screen recording) and times `sieve locate FILE --installer` by the
  packer's stages, which `--timings` now names (`pack.rope.search`, `pack.pieces.cut`,
  `pack.lzma2.first-pass`, `.tuned-pass`, `.7zip-pass`, ...); results/bench-make-*.md. On this
  machine (Xeon at 2.80 GHz, 4 threads), before threads:
  - the rope's search took its whole second on almost every file, and 10 s found nothing smaller
    on any of the fifteen (every file the same size at 1 s and at 10 s);
  - on the large files LZMA2's four full passes were most of the time (the program: 9.5 of 12.9 s);
  - the 52 MB recording took 941 s, 676 of them cutting into pieces (each boundary weighed at up
    to 129 steps, LZMA2 at each).
- *Threads, the same instructions at any count:* the blocks' costs and each boundary's steps when
  cutting into pieces, LZMA2's settings tried on the first MiB, its first pass beside that trial,
  and the tuned passes beside 7-Zip's, each scored as one thread scores it and the best taken in
  the same order; as many passes at once as half the memory free holds (about twelve times the
  dictionary each). `SIEVE_PACK_THREADS` sets the count (1: one). Shown: every file of the quick
  set the same, byte for byte, before and after; the CI makes Vitruvian Man (cut into pieces, its
  search without a limit) on one thread and on four and compares them. Before → after, seconds:
  prose 2.80 → 1.75, the word list 12.23 → 6.39, the program 11.36 → 5.29, the tar 17.94 → 9.90,
  the mesh 3.16 → 1.91, the PNG 3.70 → 2.64; the recording 941 → 351 (cutting 676 → 176), the same
  36,568,587 bytes.
- *On Edward's machine (Windows, 24 October 2026, `bench_make.bat`), seconds:* prose 1.34, code
  1.14, the oracle 1.46, the table 0.48, the word list 3.66, the rope text 0.12, the PNG 1.76,
  Vitruvian Man 2.16, the door 1.07, the mesh 1.41, the MIDI 0.01, the film 2.26, the WAV 1.60,
  the program 2.92, the tar 5.46. Every file the same size as here, but the two whose source
  differed (his IDEAS.md was newer: 345,569 bytes, and the tar holds it), so the instructions are the
  same across machines and systems as well as thread counts. The large files two to three times
  faster than here (the word list 3.66 against 6.74, the program 2.92 against 5.66); the rope's
  search, a second a file (two for a file cut into pieces), is now the floor on most files.
- *When the rope's search finds its best (measured 24 October 2026; Edward: "make sure that people
  can search longer if they choose to", and "apply this globally / dynamically in a way that will
  apply to all files"):* `sieve rope FILE --seconds 0` now says which wave was the best and when it
  was found. With no limit, every file of the quick set:
  - *text* (prose, code, the oracle, the word list, the mesh): the best was the last wave of all
    (the 247th of 247 for prose), always the line wave `C2 G1 A1` (rope-wave-v5), which the order
    tried last, so a short search never reached it; on the word list it codes 564,366 bytes, found
    after 15.5 s, against the 633,244 a 1-second search made (LZMA2's). (So "10 s found nothing" was
    the search not reaching it.)
  - *bytes already packed* (the JPEGs, the PNG, the MP4, the table, the short text): the best was
    the first wave, the bytes' own counts; the 160 to 230 waves after it found nothing.
  - *the rest* (the WAV, the MIDI, the program, the tar) found theirs part way.
- *So, for every file, in the search itself (`core/src/rope_wave.cpp`, so every caller has it: the
  File Locator, `sieve locate`, `site`, `map`):*
  - a text (95% of its bytes printable, white space or UTF-8, in lines of at most 1,024 bytes) has
    its line waves tried straight after the first quick waves; any other file, last, as before (a
    tar of texts and pictures counts as text but has lines of 4,096, where one of these waves took
    17 s and is not cut short by the time);
  - bytes that the first waves make no smaller by 2% are searched no further: no later wave has
    helped such a file;
  - `--search-every-wave` (every command; the File Locator's `[locator] search_every_wave`, and the
    hallway's option) searches every wave up to the time given, as before; the time itself is the
    user's to lengthen as ever.
  With no limit and every wave, the best is the same as before on every file (only the order
  changed). At 1 s a file, against the build before this: every file the same size or smaller (the
  word list 633,244 → 564,462, 10.86%), and the quick set made in 30.5 s against 67.0 s (the JPEGs
  0.15 to 0.31 s against 1.2 to 2.3, the PNG 0.56 against 3.60, the film 0.42 against 2.73, the word
  list 3.07 against 12.45). results/bench-make-quick-2026-10-24-rope.md.
- *Still single:* the rope's search (its time limit makes the result depend on how far it got, so
  threads would need it split into trials taken in order); the weighing; the tailoring search.
- *On Edward's machine after it (24 October 2026; results/bench-make-quick-2026-10-24.md, 12
  threads, Windows 10), seconds, before → after:* the word list 3.66 → 2.16 (633,244 → 564,462
  bytes), the PNG 1.76 → 0.29, Vitruvian Man 2.16 → 0.15, the door 1.07 → 0.06, the film 2.26 →
  0.19; the rest within a few hundredths; the fifteen 26.9 → 18.9. Left: the rope's search takes
  its whole second on prose, code, the oracle and the mesh, though on every text measured the line
  waves found the best within 0.2 s and nothing after them won; ending a text's search after them
  would bring those to about 0.4 s, once checked on texts where a table's fixed-width wave (C1)
  might win (digits laid out in lines).
- *A text's search ended after its own waves (24 October 2026; Edward: "yes make a start on that"):*
  measured first on 127 files (this repository's docs, code, tables, word lists, meshes, filter
  plugins and test files; pi's digits; a log; and two made: 200,000 digits in lines of 50 and a
  fixed-width CSV), every wave, no limit: of the 118 that are text by the rule, 117 were won by a
  wave the early group already tries (the bytes' counts, the value before, a line wave), and the
  one other, the digits in lines of 50, by the columns at that width (`C1 G1`, 83,153 bytes) by 5
  bytes over the best line wave (83,158). So a text's early group now also tries, where every line
  is the same length, the columns at that width (one or two waves), and then its search ends
  (`--search-every-wave` goes on). Every wave against the default, no limit, the 127: the same
  3,026,562 bytes in all, file for file, in 29.3 s against 110.9. At 1 s a file, the quick set's
  texts the same bytes as every wave, in about half the time: prose 1.82 → 0.84 s, code 1.39 →
  0.42, the oracle 2.00 → 1.06, the mesh 1.96 → 0.95, the rope text 0.46 → 0.06
  (results/bench-make-quick-2026-10-24-text.md). The tar is not text by the rule (lines of 4,096),
  so it is unchanged.
- *The File Locator's estimate, measured (24 October 2026):* the 52 MB recording took 11 min 30 s
  against 8 min 01 s estimated (30% low); a 20 MB text (the word list seven times over) took 5 min
  36 s against 1 min 35 s (its repetition, 2.9 MB apart, slows LZMA2's matching on the whole, and no
  sample of 1 or 2 MiB holds it). Packing the samples with the whole's dictionary was tried and
  changed nothing measurable (1 min 32 s → 1 min 35 s), so it was left out. The locator does more
  than `sieve locate` on the whole (zip's and 7z's sizes, the weighing: the recording takes about 6
  minutes there, 11.5 here); which part the samples undercount is the next thing to measure, phase
  by phase (`--timings` with the hallway's `--locate`), before the estimate is changed.
- *Measured step by step (24 October 2026; Edward: "continue with the list"):* the File Locator now
  times each step of its samples and of the whole (`--timings`: `estimate.*` beside `locator.*`).
  On the 52 MB recording (`hallway --no-menu --locate FILE --search-total 1 --timings`), of 703 s:
  - the cut into pieces was made twice, by the instructions and again by the weighing, 170 s each
    (the rope's search already kept its answer for the second; the cut did not). It is kept now, by
    the file's SHA-256 (at most four, 1 GiB in all, none a stop ended): the weighing 257 s → 79 s,
    the run 11 min 43 s → 8 min 28 s, every file of the quick set the same bytes;
  - against the line the samples draw: the instructions took 1.4 times it, the weighing 1.75, the
    7z comparison 1.7 (LZMA2 over 50 MB with its 64 MiB dictionary). Packing the samples with the
    whole's dictionary brings it nearer (on the recording 7 min 15 s → 8 min 01 s estimated, against
    11.5 min taken; on the repeated text it changed nothing), so it is back in.
  The estimate is still a third low (5 min 37 s against 8 min 28 s): the time grows faster than in
  proportion on a large file, by an amount two small samples cannot show.
- *A quarter more, and refined as it goes (24 October 2026; Edward: "we could just add 25% to the
  initial estimate and clearly state that it is an estimate ... correcting it as it goes is also
  practical"):* the samples now give each step its own line (address and SHA-256, zip, 7z, the
  instructions, compared, the weighing), the first estimate is their sum and a quarter more, and
  after each step is done the estimate is made again: the time so far, and what is left as the
  samples predict it, times how long the done steps took against their prediction (the search's
  time taken out; the ratio kept between 0.5 and 4). The screen says so ("refined as it goes:
  about ... left"), and at the end gives both: "took 8 min 14 s (estimated 7 min 38 s at first,
  8 min 01 s as it went)" on the 52 MB recording, where it had been a third low; on the 20 MB
  repeated text "took 4 min 48 s (estimated 1 min 54 s at first, 5 min 06 s as it went)", where
  no sample can see the repetition. Estimates still begin at 16 MiB.
- *The boundaries made cheaper (24 October 2026; Edward: "continue"):* timed apart
  (`pack.pieces.blocks`, `pack.pieces.boundaries`), the 52 MB recording's cut was almost all
  boundaries: 584 of them, 68,825 steps, each LZMA2 over up to 64 KiB, 164 s, against 2 s for its
  12,815 blocks. Tried: steps of 512 bytes and then of 64 within 448 of the best (14,598 steps), and
  a window of 16 KiB in place of 64, each alone and both. Every one made the same instructions,
  byte for byte, on the recording, on all fifteen files of the quick set, and on 10.6 MB of the
  repository's pictures, films and sounds joined end to end (38 boundaries). Both together are kept:
  the recording's boundaries 164 s → 13 s, `sieve locate` on it 318 s → 170 s; the joined media's
  6.8 s → 0.64 s. Not moving the boundaries at all was tried too: Vitruvian Man 41 bytes larger, so
  they stay. (A tar of the source, and the same files joined with text, were not cut at all: LZMA2
  wins every block.) Most of the recording's time is now LZMA2's full passes and the rope's search.

**19.100 "Create .sieve" in the right-click menu (Edward, 24 October 2026; noted, not to be built
yet).** "I was thinking about adding create .sieve to the context menu, but that brings with it a
barrel of issues, such as needing the full file locator and the potential amount of time it might
take to sieve a file." Extracting is quick and its time is known (decoding is measured on every
install); making is neither: the rope's search, LZMA2's passes and cutting into pieces take from
seconds to many minutes (a 40 MB tar: §19.97), and the File Locator is what shows that time, its
estimate, the best so far, and Stop and Force stop. So it waits until making is understood and
faster:
- *Measured first:* how long making takes, per stage and per kind of file, on a fixed set of files
  (the varied set, the 40 MB tar, the release folder, Edward's JPEGs), and how far the estimate is
  from the time taken (15 to 20% low when last measured: §19.97).
- *Made faster:* threads (§19.99), with results that do not depend on how many.
- *Then the entry itself:* likely a small window of the File Locator's (progress, estimate, best
  so far, Stop) rather than the command line, with a time budget chosen beforehand, as the
  locator's search time is; one file or a selection, made beside them as `NAME.sieve`.

