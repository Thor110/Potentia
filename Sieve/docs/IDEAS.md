# Ideas

A triage of the long Gemini conversation — all three parts — plus anything else that has come up
and is not yet built. It is not a plan and not a promise. Each item says what it is and whether it
is a defect, a feature, an open question, or framing.

Nothing here is a commitment. Within each section the order is roughly by how much it is worth.

**First, when work turns to the game (GAME.md; Edward, 6 October 2026): the colour-blindness pass (2.4).** *Done 6 November 2026; see §19.143.*

**How to read it (28 October 2026).** The file grew in the order things were said, so an item's
first paragraph may be an idea that later paragraphs, or a later section, built, changed or set
aside. Look for the italic notes at the end of an entry (*Built*, *Measured*, *Set aside*,
*Superseded*), and for a later section that names it. Broadly:

- **Framing to keep:** §0 (addressing is not compression), §8 (things to be careful of), §9.
- **Mostly built, kept as the record of why:** §4, §5, §7, §12 to §16, and most of §19. The
  compression work (the rope, mixing, archives, pieces) is §19.87, §19.88 and §19.102; the web
  extension and site builder §19.92; maps folded into `.sieve` §19.93.
- **Set aside, with the reasons:** moved whole to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (§17,
  §19.91, §19.94 and others; 12 November 2026), and §19.98 here, kept so the measurements are not
  repeated.
- **Done entries moved out (12 November 2026):** entries whose every idea is done now live whole in
  [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md), with a one-line pointer left at their number here;
  [IDEAS-TRIAGE.md](IDEAS-TRIAGE.md) gives every idea's verdict.
- **The current plan** is not here: HANDOFF's plans (24 and 27 October 2026) supersede the older
  orderings in this file (§19.53). How Sieve is to be shown: SHOWCASE-PLAN.md.

**Waiting for the next format version.** The `.sieve` format rested at `sieve-manifest-v31` (Edward,
27 October 2026: let it sit before pressing on); it rests now at `sieve-manifest-v33` (frozen 3
November 2026), and five of the ideas below are built since, each marked. These ideas would each need a new manifest version
and so wait; most are small, and are best gathered into one version when the pause ends:
- an archive's format as a number rather than text, with its recipe (§19.88, "The format as a
  number"); *built since, in v32*;
- headers modelled rather than kept as they are (§19.88, "Headers kept as they are");
- a shared web dictionary for pages (§19.92, measured: level with brotli on pages of 20 KB or
  more, behind on small ones); *built since, as v33's `W1` (NEXT-VERSION.md)*;
- the rope's next contexts (the byte 21 lines above, more phases: §19.87, at most 0.3% on one file);
- a word list with how often each word is used, for mixing (§19.102, *Next*); *built since, as v32's `F80`*;
- BCJ2 for programs, where 7-Zip still leads on small folders of programs (§19.95; measure first);
- each piece of a file its own rope wave, and mixing as a third kind of piece (§19.103: the
  packer-only first step was measured and gains nothing, as mixing the whole file beats every cut);
- the overhead on a small file packed tighter (Edward, 28 October 2026: `control.py`'s `.sieve` was 10
  bytes over its 7z; measured again on 30 October it is 731 bytes, 91 under the 7z's 822, since v16
  and v20 wrote the head and listing in bytes; about 11 bytes more could go: NEXT-VERSION §4a); *built since, as v32's smaller head*;
- the other lines' places coded by the rope (§19.87, *Not yet*);
- a file carried as its differences from another file in the folder (§19.106: MAPS.WoW and
  MAPS-Larger.WoW); *built since, as v32's `delta`*.

A plan for gathering these into `sieve-manifest-v32`, with what each has measured and what to
measure first, is [NEXT-VERSION.md](NEXT-VERSION.md) (30 October 2026).

**Work that needs no new version,** suited to the pause: measuring (the benchmark on public
corpora, SHOWCASE-PLAN), speed (threads, §19.99; making faster before a right-click "Create .sieve",
§19.100), the estimate's accuracy (§19.97), the packer's choices (group sizes, search time), the
hallway and the game (GAME.md), the web extension's remaining pieces (§19.92), and the
documentation.

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

**1.5 What it is not.** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

---

## 2. Defects and corrections

**2.1 Entropy is measured against the wrong space.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**2.2 Padding is counted as content.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**2.3 The cased dictionary lets short gibberish through.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**2.4 No colour-blindness pass.** *Done 6 November 2026; colours kept by Edward; why colour cannot carry a line's identity as lines grow: §19.143.* Nine lines, filter verdicts and the map all encode meaning in
hue, with no non-colour fallback and no simulation check. First when work turns to the game (above). The pair most
at risk: audio's dark green and video's dark red (and so tracks and movies, which share them),
which differ mostly in hue, the difference red–green colour blindness loses.
> **Strong note (Edward, 6 November 2026, on seeing the result: "Dark Green and Red? Hmm that is
> interesting"):** the colour-blindness check found that **tracks (dark green) and movies (dark
> red) become all but the same colour with deuteranopia**, the commonest colour blindness.
> Every future change to a line's colours, and
> every new line, is run through `python tools/colour_check.py` first, and no pair is let below 10
> (CIEDE2000) under any of the three simulations without it being said here. Edward's view for
> now: "in reality it doesn't matter all that much, there is only so much freedom with colour
> blindness", and the lines are always named on screen as well; so the colours stay, and this
> note stays with them.

*Checked (6 November 2026, `tools/colour_check.py`):* every pair of lines' walls and edges through
Machado's (2009) simulations of protanopia, deuteranopia and tritanopia, by CIEDE2000. One pair
told apart normally is lost: **tracks and movies**, 58.9 normally and 2.4 with deuteranopia (their
walls are audio's green and video's red, and both have black edges). Audio and video themselves
keep 11.5 (deuteranopia), held apart by their edges, amber against yellow. Every other pair stays
at 14.2 or more. Measured ways out, for Edward to choose (the colours are his, and a composition
shares its unit line's walls on purpose): tracks' walls `0,90,60` (a blue-green) gives tracks and
movies 15.4 at the worst, but tracks and audio only 2.1 with tritanopia; tracks `0,130,0` with
movies `110,0,0` gives 17.6, tracks and audio 12.7, and movies and video 5.2. (Giving the
compositions their unit lines' edges instead would make tracks look exactly as audio does.) Nothing
is changed yet. The lines are named wherever they are shown (the HUD, the setup menu's columns), so
colour is not the only thing that tells them apart.

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

**3.6 Angles as a reduction: why not, and what is kept.** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

---

## 4. Manifests, installs and anchors

What survives §0 — and most of it survives.

**4.1 A directory walker that emits a manifest.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**4.2 Deduplication comes free.** Identical files land on the same key, so a tree never stores the
same bytes twice. This is content-addressed storage — which is real and worth having — and it is
the part of the OS story that actually works.

**4.3 The first release as the first anchor.** Build the release, walk it, emit the manifest, tag
every key `sieve-build-release-v1`. The CLI reproduces its own release build from its own manifest
as a post-build step, which is both a conformance test and the first entry in the archive. This is
a good idea and it does not depend on the key being small — it depends on the key being canonical,
which it is.
*Partly built:* `make_release` installs every download back and compares it byte for byte, and
publishes each file's SHA-256 (SIEVE-INSTALL-USAGE.md); the `sieve-build-release-v1` tag was not
made.

**4.4 Chunking, and why it matters twice.** Split a large file into fixed-size pieces, each its own
unit on its own line. It makes generation multi-threadable, it makes a big file tractable on a
small machine, and it is the only way an install can proceed in pieces rather than all at once.
*Since:* files are cut into pieces where that is smaller (`sieve-manifest-v13`), and files and
groups of files decode at once on several threads when installing
(§19.102). A file split into fixed pieces on a line, for an install in pieces, is not built.

**4.5 Real file types out of the CLI.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**4.6 A signed manifest, if tamper-checking is wanted.** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

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

**5.3 Signs above the portals.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**5.4 The warp animation.** On a warp: face the shelf, slide along three bookcases, through the
portal, along three more, and stop facing the item. Turn the world around the player rather than
snapping the camera — that is the standard fix for transition disorientation and costs nothing.
It also hides the moment the shelf changes side.

**5.5 Accelerating scroll.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**5.6 Shimmer on the item under the crosshair.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**5.7 Metadata and links on an item in hand.** When a unit has been catalogued, show what is known
about it and let its links be followed — a warp at the touch of a hyperlink. This is the feature
that turns the hallway from a viewer into an archive, and it needs §4.1 first.

**5.8a The node graph, first version: built** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**5.11a The address navigator.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**6.1–6.4 Limits and reset.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**6.5 Global parameters, and entries per shelf.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**7.5 Punctuation filters for `babel29` and `ascii95`.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**7.8 Ambient audio sampled from where you stand.** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**7.9 Generalise the preview bake.** The crate-face queue — reticle priority, flood fill outward,
per-frame budget, LRU — is written for models. One interface where each slot type supplies a cheap
2D draw gives the same thing to every line.

**7.10 A memory prober.** Watch RAM and VRAM, drop cached previews at the thresholds, without
disturbing position tracking. Needed the moment 7.9 exists.

**7.11 Predictive prefetch.** Warm the next two or three steps along the direction of travel, and
give frequently-walked paths a larger share of the cache, so the thing you are about to reach for
is already there.

**7.12 Reels — video as a composition, not a line.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
  *Answered:* the repository is GNU AGPL v3 (`LICENSE` at its top, copied beside the programs by the
  build), with no restriction added. Separately, whether user-ingested content can be redistributed
  needs an answer before any ingest ships.
- **Mods as vectors is elegant and overstated.** `base + v` instead of a file overwrite is an
  interesting reframe, but "different axes can never conflict" ignores semantic coupling.
- **The multiverse cluster and the OS are separate documents.** Roughly forty turns of hypervisor
  and emulators, and ninety of Framework OS (PotentiaOS since 27 October 2026). Two things survive:
  the category definition file, and the stated position that they deserve recording rather than
  building.

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
cleanest one-paragraph justification of the project, and it belongs in the README. *Done (28 October
2026):* Sieve's README, under The Gallery of Babel.

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
  *Built:* a sign over every doorway in its line's colours (`client/door_portal.cpp`).
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
  *Built:* T opens the warp box in the hallway (and `hallway --warp` on start).
- A warp-to-start, which is the only realistic way anyone ever sees a loop marker. *Built:* Home
  goes to corridor tile 0, every line's start.
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
  *Done:* `Previous/` holds the earlier READMEs.
- The final documentation pass waits until the visuals are done, because "that last task is usually
  the sort of thing that gets skipped or only half done".
- There is a pile of loose notes — `engine-idea`, `nextclaude2` through `claude5`,
  `downloaderfriendidea`, `state-space-near-zero-...` — that has never been reconciled with the
  repository.
- Nothing has ever run on GitHub's Windows or macOS runners, only Linux. *Done since:* CI runs on
  Windows, Linux and macOS on every push.
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
  fix is a new dictionary version; the call was never made. *Answered (24 October 2026):* the
  `-names-v2` lists, the default since (§2.3).
- **What is the vertex ceiling for models,** and what makes a textured triangle not noise? The
  hardest filter question on that line and it has no answer yet.
- **Which font comes next** — Cyrillic, Polish and Czech need a taller one, and the 8-pixel layouts
  have to be reworked before any of them read properly.
- **Which media types cross paths in the state space?** A cheap experiment, named and never run.
- **How much would the reducer actually save** on a real corpus? Never measured, and it is the
  number that decides whether the key idea is worth anything (see §0).
- **The licence.** *Answered:* GNU AGPL v3 (§8).

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
  colour-blindness pass (2.4) was done on 6 November 2026 (§19.143): audio's and video's dark green and dark red differ
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
- ~~**The `sieve` tool reads MIDI files** only on `notes3` lines;~~ Done (6 November 2026): `warp --file` and `bind --file` read a MIDI file on every note set (`midi_to_notation`, as the hallway); CI step 3 checks the hymn's 73 events on notes104 and notes2. Before: on `notes104` and `notes2` a `.mid`
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
if Sieve's back end grows into an operating system (PotentiaOS) and its front end into a game engine, an asset
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
- **`engine/` (Edward, 25 October 2026):** what is learned about games' own formats is kept in
  `engine/`, out of the main architecture, for this; the first is the `.WoW` archives of *The War of
  the Worlds* (1998), measured (`engine/wow.md`).
  *For a reader of such a format, if one is ever built into the packer (Edward, 29 October 2026):*
  its files' order goes in its recipe, as `skm` numbers a zip's members, never in the listing's
  order (SPECIFICATIONS, "An archive's order"); a game may need its archive's files where they
  were, and the archive made again is checked against its SHA-256, so a reader that loses the order
  is refused rather than installing a wrong archive.

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
    rows at once, AVX-512, and the per-position softmax for small models. *(Blocking and AVX-512
    built 12 November 2026, with the update on every thread: a model of width 64 trains in about
    half the time on four threads, width 256 in about two thirds, every result bit for bit as before;
    results/training-kernels-2026-11-11.md.)*;
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
*Built (7 November 2026), (a):* L on COST searches further, doubling the bearing's decimal places
(20, 40, ... 640) on the route's worker, the best so far shown meanwhile; every walk is already
weighed at each width, so more places are what is left to try. On a page and on a file of the
binary line it took under a second to 640 places, and found nothing shorter than at 20, as the
evaluation expects for most units. A bearing past 20 places is written down, not typed.

**16.2 The File Locator from the main menu.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
*Superseded (22 October 2026, §19.93):* once the head had flag bits free, a map's parts cost
ordinary instructions nothing, and maps were folded into `.sieve` (`sieve-manifest-v18`).
Training runs need neither: a run is a folder of ordinary files (the recipe, the corpus, the
result), packed as ordinary Sieve instructions by the harness (SPECIFICATIONS §12.0e), so other
`.sieve` files carry nothing of training's.
- **For a far later manifest, Edward's idea:** a run's file order recorded as item numbers in the
  instructions' own list of files rather than as paths (a few bytes each instead of a path's
  length; the list is sorted by path, so the numbers would shift if a file were added or renamed,
  which a later version would have to settle).

**16.4 A file type filter on the binary line.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**16.5 "Calculating Dimensions" where it is seen.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**16.6 A slide-in window for a line's settings,** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**16.9 A demonstration: the first item at every degree of every dimension, as one `.sieve`.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

## 17. Deprecated ideas

Ideas evaluated and set aside, kept with their reasons so the questions keep their answers.

**17.1 A rotation transform, and numbers that hold only their decimal places (Edward, 7 October 2026; deprecated).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.2 CHAT on every line (Edward, 17 October 2026; deprecated for now).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.3 Size and decoding time as one setting (Edward, 20 October 2026; deprecated for now).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.4 Square and circle wave filters (Edward, 27 October 2026; measured, not taken).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.5 `source.sieve` as a map holding the source's 7z (27 October 2026; replaced the same day).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.6 Smaller `sieve-install` (27 October 2026; looked at, not pursued).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.7 Smaller decisions of 27 October 2026, kept with their reasons.** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**17.8 Page scripts in the browser, checked or held back (27 October 2026; set aside for PotentiaOS).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

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

**18.2 A language filter, or layers, ordering models outwards (Edward, 11 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.3 How a published multimodal model sees a picture.** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.10 Testing in fewer, larger rounds (Edward, 12 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.11 A video game dimension (Edward, 12 October 2026).** Not yet designed. Edward's thoughts: the
game line of GAME.md §4 (arcade cabinets) was meant to be woven through the story. Possibly a new
dimension of discs (a copy of the CD model, as the tracks line has, for CDs and DVDs), and perhaps a
place in each room for an arcade cabinet or a computer. Or it forms and evolves around the rest,
from the lowest resolution to the highest (§19.6). Utility comes first; this stays open.
*A design to decide on (12 November 2026), GAME.md §11:* a games line, each unit a short program for
one fixed, original virtual console, every game of that size with an address, filters that judge
whether one draws, moves and responds; cabinets in its rooms play them. Nothing built.
*Lenses (Edward, 12 November 2026):* "a mode to toggle from executable through console as well as
source folder, everything becomes like a lens": one game seen run, as its machine's state, or
unfolded as a folder of source, each exact both ways (GAME.md §11).
*A source tree (Edward, 12 November 2026):* "hooked up to git and github like systems and create a
source tree across the gaming and computing spectrum to light up the information highway": every
unit's source folder a repository, its variants branches, and the whole a tree of how software
descends from software (GAME.md §11).

**19.12 A metrics list, made with every release (Edward, 12 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.18 Every menu reaches every system, in one order (Edward, 12 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.19 A music filter designer inside the Media Player (Edward, 12 October 2026).** With the Media
Player in the main menu, it could hold a sub-node system for designing audio and music filters:
the Filter Designer's nodes, but on melodies, heard as they are tested (each change plays a melody
the filter keeps). The Media Player already has its own stack of audio filters for each mode; a
designer inside it would make new ones there, saved as plugins like any other.
*Built (12 November 2026):* the Media Player's M opens the Filter Designer on a melody filter (notes
from middle C up an octave, as a table over the line's constants), and on an audio line each test
plays a survivor (the first when the result comes in, the next with P), for note lines and, with
`symbols pcm*` now offered, for sound itself; Esc comes back to the Media Player. Not yet: the
designer drawn inside the Media Player's own window, rather than opened over it.

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
  - *Not yet:* a track model, and a model of sound itself. *Recording a melody run from the
    harness, built (6 November 2026):* "+ new melody model..." (or M) sets one up as a picture
    model is, its melodies drawn from the audio line's stack at the setup menu's note set and
    length (notes104 or notes2), recorded as `melodies-<stack>` and trained.
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
  *Built (6 November 2026):* `/random page|book|world|model|ai|track|video|movie|file`, a card
  with a random place on that line (every place as likely as any other, as the navigator's R
  draws one; where the line is compact, its places are its survivors) and GO TO IT, which goes
  through the door to it. *Built (7 November 2026):* `/unit LINE HEX`, the place at that number on
  a line (in hex, as the navigator and `/random`'s cards show it; refused past the line's end), as
  the same card, with GO TO IT.

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
      own CMake, configure, build and copy the result, with Python optional. *Written (6 November
      2026), not yet run on Windows:* `tools\build_release.bat` finds Visual Studio by vswhere,
      loads VsDevCmd (its CMake and Ninja), configures and builds the x64-Release preset, and makes
      the release where Python is installed; without Python the build folder is the result.
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
- *The exchange rate on the card, built (6 November 2026; with §19.62):* a picture file's card in
  CHAT says what it costs on each line it can be on and what each keeps: the file as stored (its
  bytes, GO TO IT), its pixels at their own size in rgb24 (every pixel kept, F), and fitted to this
  hallway's image line (its cells times the bits of a cell's colour, J). Walking between them is
  still to come.

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

**19.42 The web page's Build or Buy (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.44 Keeping the interface even (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
    *Built (12 November 2026, `sieve-chain-v2`):* `steer NAME` (F in the CHAIN tab) tries seed,
    seed + 1, ... up to 256 and keeps the first unit the line's ticked filters pass; with
    `row-runs-v1` on the 10x10 image line, the figures model's first passing try was the 130th.
  - *Larger models* as stages, once they can be given as files of other kinds.

**19.47 An engine that could make every game (Edward, 13 October 2026).** His idea: the game
engine as a free-flowing, dynamic building and creation system.
- *Bodies:* procedural skeletons and bones, boxes, gravity and kinematics, destruction.
- *Scale:* from voxels to triangles, quads and other forms, all the way up to layering with
  DLSS 5. *(12 November 2026, Edward: "from pixels through voxels and across tris / quads and
  nerbs with stable diffusion and the like to have a graphics scaling algorithm for 3d models";
  noted in GAME.md §11, with Alien Trilogy as the first case.)*
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
- *Built (6 November 2026):* `/docs` lists the documents, `/docs NAME` a document's sections, and
  `/docs NAME SECTION` shows a section (by the start of its title or its number, up to 6,000
  characters, the rest said to be there), as a note. The documents are found beside the program
  or in a folder above it (a build inside the repository); a release does not carry them, which
  is Edward's call (they would add the docs folder to it).
  *Built (7 November 2026):* GO TO IT and J. With `/docs NAME` or `/docs NAME SECTION` comes a
  card holding the document's or the section's whole text as a file ("IDEAS-19.48-The-documentation-inside-the-game.md"):
  GO TO IT walks to it on the binary line, J opens it as an item (text, on the pages
  line), S saves it. It is not given to the model.
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
- *Built (7 November 2026), the menu's part:* under the rule on the main menu, a line for each
  dimension in its colour, binary first: its name, its size under the setup menu's settings (the
  same counts the setup menu shows, `line_sizes_of`), and the years walking every unit at one a
  second would take. Lines that do not fit above the items fold into "and N more dimensions".
  `hallway --main-menu --screenshot` prints them. *Still to come:* the website's copy checked
  against them, and the labels as filters.

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
  item to Go to or save. *Still to come:* a layer keyed by a picture (§19.76).
- *Built (7 November 2026):* the catalogue on the screen: DO: CATALOGUE counts the recipes, from
  the same code as `sieve cipher --catalogue` (`cipher::catalogue`), and places the recipe among
  those of its depth.
- *Earlier plan, kept for the record:* the Encryption Engine screen on
  the main and pause menus, each layer's output an item; then layers that change the shape (a file
  as a picture, a page as a melody), which is the "as anything".

**19.53 The plan, in order of importance (13 October 2026).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

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
  in a format a program can read. It holds Edward's, Gemini's and Claude's lines so far;
  Gemini's two messages and the technical lines moved to `ideas/VOICES-STASHED.md` (§19.145).
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

**19.61 What the numbers mean (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.64 Running out of original things to say (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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

**19.66 Disclosure of AI, everywhere (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.67 Folding everything (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.68 The voices on the website (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.69 Build or Buy, and every build on the website (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
    *Built for the main menu and the pause menu (6 November 2026):* Settings, Controls, **Read
    menus aloud** (`[controls] read_aloud`, off by default) speaks the item chosen when it changes,
    with its value on a settings screen, through Windows' own voice (SAPI, on a thread of its own,
    each item cutting short the one before); elsewhere it says it has no voice. Not yet the setup
    menu, the File Locator or the hallway's panels; and not yet heard on Windows here (built and
    checked on Linux, where it is silent): Edward's first run is its first test.
    *Built for the setup menu and the File Locator (7 November 2026):* the setup menu says the row
    chosen after its section's heading, with its value ("GLOBAL, number of items per wall, 128 a
    tile, 64 on each wall"), and in a line's filters or the alphabets the entry chosen, a tick
    said as on or off ("max-run-v1, On, Filtered: 0.1% ..."). The File Locator has no row to
    choose, so it says the button the mouse comes onto, a button as pressed ("Optimise Search,
    Off"), and its status line when it changes: while a job runs only its first, so an install's
    file-by-file lines do not talk over each other, and the result when it ends. One thing a frame,
    so nothing is cut short by the next. `hallway --read-aloud` turns it on for a run, unsaved,
    and with `--screenshot` prints what was said, which is how CI checks it on a machine with no
    voice. Still not the hallway's panels.
  - **Objects that can be translated:** every item's description and every sign through the same
    language files, so a new language translates the world as well as the menus.
  - **Other settings:** text size, contrast and colour-blind palettes (the lines' colours are
    settings already), and holding a key in place of pressing it again and again.
- *The tactile Sieve (12 November 2026):* the aim goes further than reading aloud: braille, and
  colour, touch and sensation as screens come to carry them (§19.140).
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

**19.78 Every figure with its inverse (Edward, 13 October 2026).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
    choice is the player's. *"auto" built (6 November 2026):* `widen_weights = auto` widens a
    model file's 16-bit weights only where the model, widened, fits in ITEM MEMORY's share of
    installed memory (25% until it is set), and keeps them as stored where it would not; the unit
    tests check both sides of the line.
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
  - **What is left, measured (25 October 2026, step 6 of the plan):** counted by its column and the
    byte above, `pi6.txt`'s entropy is 415,676 bytes, 435 (0.10%) over the floor of 415,241; the code
    is 416,967, so 1,291 of the 1,726 bytes over the floor are the code's own tables and streams,
    not what it models. Adding the byte 21 lines above (the blank line's period) brings the count
    below the floor (410,513, with 6,107 contexts), a sign that the tables would then cost more than
    they save. Not built: a new rope-wave version and manifest for at most 0.3% on one file. The
    blank lines and form feeds are what the model misses; the tables are what the code pays.
  - **The tables counted (25 October 2026, for v20):** the code's tables are 76 bytes (4 of them,
    18 symbols in all); its streams 416,880. So the 1,291 bytes are not the tables: they are the
    grouping, the code's 4 shared tables standing in for every column-and-byte-above context the
    count of 415,676 kept apart, and the streams' own few bytes. A cheaper table would save at most
    76 bytes; none was built. (I had said the tables were most of it; they are not.)
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
  - *Measured (30 October 2026; NEXT-VERSION §4a, 3):* on the repository's seven JPEGs brunsli came
    15.25% to 21.91% under the JPEG and `cjxl --lossless_jpeg=1` 13.93% to 20.14%, every file back
    byte for byte; on quality-90 re-saves `cjxl` led (30.25% and 31.22% against 24.60% and 25.48%).
    Sieve's own reading of the coefficients came back exact too, but coded by a general model they
    were no smaller than the JPEG.
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
  - **bzip2 and zstd built (25 October 2026, `sieve-manifest-v19`, SPECIFICATIONS §12.2).** Edward:
    "make a start on including third party archive format decoders, making sure we always include
    any licenses or readmes that might be necessary for them". libbzip2 1.0.8 and libzstd 1.5.7,
    both always fetched by CMake at those tags (never the system's, so the remade bytes do not
    change with the machine), their licences in `third_party_licenses/bzip2` and `zstd` as upstream
    wrote them and a row each in its README. Measured here before building: libbzip2 gives back
    the `bzip2` program's files exactly at every block size (the program is the library); libzstd
    remakes a file only at its own version (the `zstd` program 1.5.5's output differs from library
    1.5.7's at the same level; 1.5.7 against itself matched 15 of 16 files tried), so a `zst` way
    names its version and is refused by another. A bzip2 word list (766,439 bytes) became 469,671,
    remade. Several bzip2 streams or zstd frames are carried as what they hold.
  - **And more (25 October 2026, `sieve-manifest-v20`):** Edward: "We should aim to go all out in
    the long run and try to include every archive format we can get our hands on." lz4 1.10.0 (its
    library BSD; the lz4 programs, GPLv2, not built) and brotli 1.2.1 (MIT), fetched and pinned;
    `.lzma` and lzip read by the liblzma Sieve has; and libarchive 3.8.10 (BSD 2-clause, with a few
    files under other BSD-style licences or in the public domain: `NOTICES.txt` gathers them) for RAR
    4 and 5 (its own reader, not unRAR's, whose licence is not compatible with Sieve's), cab, ISO
    9660, LHA, cpio, ar and WARC. Measured before building: liblz4's frame matched the `lz4` program
    20 times out of 20 (5 files, 4 levels, and 1.9.4 against 1.10.0 alike); brotli's library the
    `brotli` program 15 out of 15 at the program's own window; liblzma's alone encoder the `lzma`
    program 35 out of 35. So those are remade exactly; lzip, whose encoder is its own, and
    libarchive's formats, whose encoders Sieve has none of, are carried as their contents. Several
    gzip members and xz streams one after another are carried as what they hold too (no new
    version). Not unpacked where unpacking makes nothing smaller: brotli at quality 11 beat xz on
    IDEAS.md (115,161 against 118,900), and cpio, ar, ISO and tar hold their files as they are, so
    their contents pack to the same size.
  - **7z, zip and gzip made again exactly (25 October 2026, `sieve-manifest-v21`):** Edward: "we
    should delay the filter ideas until we deal with 7z, zip and rar exactly". One way serves all
    three: a skeleton of the file (what is kept as it is, and where each stream goes) and the data it
    holds. Deflate by preflate (Apache 2.0, pinned to its last commit), which made again every stream
    tried, its difference a few bytes where zlib's kind made it and about a tenth where 7-Zip's did;
    LZMA and LZMA2 by the LZMA SDK's own encoder, which made again every block 7-Zip 23.01 wrote at
    one level or another (its default, -mx9, -mx1 in blocks, LZMA, and LZMA2 after the x86 filter).
    A zip by 7-Zip at -mx5 405,437 → 389,755; a 7z at -mx1 329,861 → 280,971; the `sieve` program in a
    7z at -mx1 3,381,897 → 3,144,066. A 7z at 7-Zip's default levels is left as it is: LZMA2 is already
    what Sieve would use, so its contents come out no smaller. The oracle has neither preflate nor
    the SDK's encoder: it checks each skeleton whole and names the files it cannot make again.
  - **RAR, measured (25 October 2026; not built):** no free RAR compressor exists, and unRAR's
    licence forbids writing one, so the only way is the machine's own `rar` program, run as ffmpeg
    is. Measured with RAR 7.00: three files extracted by `rar` itself (their times to the
    nanosecond) and added again with the same command came out byte for byte the same archive;
    extracted by bsdtar, with the times to the second, 48 bytes different. So it can work where the
    same version of `rar`, the same switches, the same order and the times to the nanosecond are
    all known; the switches (level, dictionary, solid, recovery record) and the host system recorded
    in the archive would have to be read from its headers, and an archive made by another version,
    or on another system, would not come back. Waiting on Edward: whether that is worth building.
  - **Why rar.exe alone is not enough (Edward asked, 25 October 2026):** `rar` reads every older RAR,
    so unpacking is never the trouble. Packing again gives a valid RAR with the same files in it, but
    the same bytes only where the version and its settings are those that made it: each version's
    compressor may choose its matches differently, and RAR 7 cannot write RAR 4 at all. A RAR made
    years ago, packed again by today's `rar`, is a different file, and the tree's SHA-256 says so.
    So RAR goes in the list of formats as **read, carried as its contents; made again only by the
    same `rar` that made it**, and nothing more is built for now.
  - **Built (Edward, 25 October 2026: "get that started"; `sieve-manifest-v22`):** a RAR 5 the
    machine's own `rar` makes again byte for byte from its files, their times and attributes set as
    its headers hold them, is carried as its contents with the recipe (the version, the switches its
    headers give, each member's times); installing makes it again where the same `rar` is found, and
    otherwise gives the files with a warning in the hallway, `sieve-install` and `sieve install`
    (Edward: the warning comes from the code they share, so a later one needs nothing more of
    them). Tested with a stand-in `rar` (`ci/stand_in_rar.py`); the real `rar` not yet measured,
    as none can be had in the cloud session. RAR 4 still only its contents.
  - **Headers kept as they are (Edward's thought, 25 October 2026; a note):** a skeleton keeps an
    archive's headers byte for byte, which is mostly redundant: a zip's local header repeats its
    central directory's entry; offsets, sizes and CRCs follow from the files. A model of each
    format's headers (made again from the files and a few settings, with only what differs kept, as
    preflate does for deflate) would carry less. Measured, the headers are small and LZMA2 already
    packs their repeats, so it waits.
  - **Edward's view of all this (25 October 2026):** optimising archives is a pleasantry, not part of
    Sieve's core. The far aim is to replace every archive format with `.sieve`; until then, reading
    them, and keeping them backwards compatible, is how what is in them reaches the future.
  - **Still to look at:** BCJ2 and other filters in a 7z; PNG's deflate (the same skeleton);
    multi-volume archives; header models (above); and the temporary and permanent choice (below).
  - **The rule for every decoder taken (Edward, 25 October 2026):** its licence, and any notice or
    readme its licence asks for, go in `third_party_licenses/` with a row in the README there,
    copied beside the programs by the build. Next to look at, licences to read first: lz4 and
    brotli, cab, lzip, and RAR (its decoder's licence forbids using it to make a RAR compressor;
    whether carrying a RAR as its contents is allowed is to check).
  - **Built (27 October 2026, `sieve-manifest-v31`):** the two choices, boxes in the File Locator
    (temporary, A; permanent, U) and `--compressed-temporary`, `--compressed-permanent` (both: the
    one box as it was). Temporary carries a 7z's or zip's data as the archive's own files (`skm`,
    members `<archive>/<n>`), each its own way, the archive made again exactly; the format as a
    number (below) is still to do. The source's 7z, without the anchors folder: 8,005,603 bytes to 6,189,674, given back byte
    for byte (made with `-mmt=1`: 7-Zip's LZMA2 on several threads is not made again). The
    release's `source.sieve` became the source tree's own instructions the same day (Edward), so it
    no longer depends on that.
  - **Permanent or temporary (Edward, 25 October 2026; built 27 October 2026, above).** Two choices in place of one
    box. **Optimise archives (permanent):** an archive (a game's `.pak`, say) unpacked and kept as
    `.sieve` for good; installing gives back its files, not the archive (today's `u` lines, as a 7z
    or zip is now). **Optimise archives (temporary):** the same unpacking, with a note of the format
    to repack the files into on installing, so the program that wants the archive still finds it.
    Edward: "We wouldn't unpack archives we can't repack properly, until we can": temporary is
    only for a format Sieve can repack exactly, byte for byte, checked by the tree's SHA-256 (as
    the `xz`, `gz`, `bz2` and `zst` ways are now). Its use is mostly proprietary formats (games'
    pack files), a middle ground until `.sieve` outperforms them; many are stored or zlib inside.
    For now a little text per file; later only the format's number.
  - **The format as a number, not a bitmask (discussed 25 October 2026).** A file has one format,
    so what names it is a choice from a list, and a number serves it best: 16 bits name 65,536
    formats, a 16-bit mask only 16. Written as a varint it costs 1 byte for the first 128 formats
    and 2 up to 16,384, so it need not be a fixed 2 bytes (the listing in bytes, §19.90, would
    write it so). A bitmask is for a *set*: the settings (which formats are optimised, and which of
    those permanently or temporarily: two masks, or 2 bits per format) and a head flag saying the
    instructions hold any temporary archive at all. Beside the number, an exact repack needs the
    archive's own settings (a level, a block size, a pack file's order and padding), so the number
    names the format and its recipe follows. A new manifest version, with the next one.
  - The versions compared: GNU gzip 1.12 against zlib 1.3, and the `xz` program 5.4.5 against the
    liblzma Sieve was built with here.
  - **A PNG's rows carried unfiltered, made again exactly (an idea, 31 October 2026, not
    measured).** Re-saving a PNG under permanent (v32's draft, `pngr`) showed that a PNG's rows pack
    smaller unfiltered than as PNG filtered them. The scowl's `.sieve` was 48,136 bytes so, against
    84,641 for temporary's skeleton, which carries the filtered rows. Temporary could carry them
    unfiltered too, with each row's filter byte beside them, filter them again on installing, then
    deflate them by preflate as now. Then it would give back the original and pack about as small
    as permanent. Whether preflate's corrections stay as small for rows filtered again is what to
    measure.

**19.89 ~~Sieve instructions for the internet: size and decoding time as one setting~~ (Edward, 20 October 2026; deprecated for now, §17.3).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**19.90 The listing in bytes (Edward, 20 October 2026; built 21 October).** His thought: what the
instructions write to find a file could be packed into bitmasks rather than plain text. On a small
file the listing was most of what Sieve added: a 1,597-byte script's was 142 bytes, 70 of them the
tree's SHA-256 written as hex, and his own script's instructions came out 10 bytes over 7z's.
`sieve-manifest-v16` writes the head in bytes (flags, varints, the SHA-256 as its 32 bytes) and
keeps the rest, the entries, as the text they are, packed as before where that is smaller: that
listing is now 67 bytes, `pi6.txt` 417,061, the trained model 4,185. *Still open:* the entries
themselves in bytes (each way a code and its numbers), where a listing of many small files would
gain most; measure first.
- *Measured (25 October 2026, step 6 of the plan):* the entries are a small part of what is made,
  once packed. The release folder (146 files, 30 folders, 8,209,149 bytes as `.sieve`): its listing
  is 6,814 bytes as text, 1,710 once packed; written in bytes (a way's code, sizes as varints, each
  path as what it shares with the one before and the rest) and packed the same way, 1,598 against
  1,706 for the text: 108 bytes, 0.001% of the file. Folders of the repository: the extension's 17
  files' listing packs to 196 bytes of 47,925, the docs' 30 to 412 of 1,050,839, the filters' 47 to
  502 of 1,226,883. A single small file's entry (`f 743 raw mark.js`) is 19 bytes as text and about
  12 in bytes: 7 bytes of `mark.js`'s 474 (1.5%). Not built: a new manifest version, its reader, the
  oracle and the vectors for at most 7 bytes a file. The head's 32-byte SHA-256, which every install
  checks, is the larger part of a small file's overhead, and stays.
- *Built (25 October 2026, `sieve-manifest-v20`, SPECIFICATIONS §12.2):* Edward: "every step forward
  is a step in the right direction and even the small savings will add up". It joined v20 with the
  archives, so it cost no version of its own. Each line of the body is a record: paths as what they
  share with the one before, sizes as varints, ways as a code, an archive's format as a number
  (Edward's idea of the same day: a number from a list of every archive format, and whether it is
  repacked on the way out, a remade archive's way, or kept as its contents, a `u` record). Measured
  against v19: the extension's folder 12 bytes smaller, the docs 8, the filters 15, `tools/cli` 36,
  one script 6, the trained model's file 3, `pi6.txt` 9.

**19.91 A file's opposite found in noise (Edward, 21 October 2026; an answer, nothing built).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**19.92 Sieve for websites (Edward, 21 October 2026; an idea, nothing built; built since: the web extension, the site builder (`sieve site`) and the web dictionary, v33's `W1`).** His thought: a
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
- *Phase 2 begun (24 October 2026; step 4 of the plan):*
  - *Links from the server's root:* `sieve site build|check --served-at PATH` (`/`, or `/Potentia/`
    for a site in a folder): a link under that path is the site's own, made and checked as a
    relative one is, written from the root as before (`/Potentia/p/door.sieve`); without it they
    are left alone as before, and `check` says how many it could not read. Another server's
    (`//host/...`) never. CI: a site with both, built each way.
  - *A `.sieve` of many files, as a listing:* `sieve web-host` answers a decode of one with its
    listing (paths, sizes, SHA-256s, folders), and a decode naming `path` with that one file, cut
    from the checked contents and checked against its own SHA-256; nothing is written to the disk.
    The viewer shows the listing as a tree, each file opening by its path; a page among them takes
    its pictures and its links to the others from the same `.sieve` (and a picture it does not hold
    is refused, saying so). A picture (`<img>`) of one is still refused: a picture names one file.
    Seen in Chromium with the test site's "Every page in one" (5 files: listed, About opened from
    it, its links leading to the others by path). CI: the fixture's listing, a file by its path, a
    path it does not hold.
  - *The site's table, in the extension, and the crawler (25 October 2026).* Edward weighed it:
    "A hash table makes it a single point of failure that could make swapping out pages easy, which
    is the opposite of what we want", then "I meant index.sieve would be the full table ... a
    crawler can check all hashes against the table regularly, meaning that if any page or the table
    mismatches, we would know instantly", and on scripts: "the scripts themselves can still be
    hashed into the table, but that still makes the table and a web script the single point of
    failure unless the pages that load them also point to their hashes". Built:
    - the table is `index.sieve` at the site's top (`site.sieve` before; `check` reads either), and
      every plain HTML page names it (`<link rel="sieve-table">`);
    - scripts and stylesheets are given their files' `integrity`, which browsers enforce;
    - the extension remembers a site's table at the first visit (or saved by hand on its settings
      page), holds each page of a remembered site hidden until it matches, and stops it with a
      notice if not (refused; the table changed: trust it or leave; not in the table: show anyway);
    - `web/crawl_site.py URL --table SHA256 [--every M]`: the table against the hash kept, then every
      file it names fetched and `sieve site check`ed.
    The table can still be swapped on the server, but the crawler, holding the hash the build
    printed, says so at its next run, and a visitor's extension stops at a table it does not
    remember. Seen in Chromium and run in the CI (web/README.md, "A site's table").
  - *Decided (Edward, 25 October 2026):* the remembered table stays, an update asked about with
    "Trust the new table" ("trust the new table isn't a terrible option at all"); and no signing:
    "I don't trust myself or any human being alive to keep a key safe". "There is nothing we can
    really do if a server is compromised and an entire website replaced, that is out of our hands."

**19.93 The `.map` format folded into `.sieve` (Edward, 22 October 2026; built the same day).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.94 Distance along the line and rotation together, and grids of any size (Edward, 22 October 2026; measured, nothing built).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

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
*Measured again (30 October 2026, Sieve 0.13.77, v30):* the `sieve` program (Linux, 10,236,056
bytes) and the docs, 12,692,639 bytes: Sieve 4,159,742; a solid 7z by py7zr, LZMA2 9e with BCJ,
4,277,050 (Sieve 2.74% smaller); without the filter, 4,532,855. Edward's source tree on the same
day: `source.sieve` 47,348,954 against `sieve-source.7z` 49,320,989. So 7z no longer leads where
both use BCJ; BCJ2 itself (7-Zip's, not py7zr's) is still to be measured, on Edward's machine.
Edward, the same day: 7-Zip's default ("Add to", Normal) on `tools/sieve.exe` and `docs` makes
3,289,322 bytes; the File Locator's figure for that folder is still to be set beside it.
`source.sieve` holds the 7z as it is, so it is the 7z and the map's text, a few hundred bytes either
way (here 47,779,761 against the 7z's 47,780,051; holding the source as a tar instead,
47,594,737). Making it took 19.5 minutes, nearly all the rope's search with no limit on a file it
cannot shorten; maps now give each held file 10 seconds (`sieve map --search-per-file`, 0 for no
limit), and `make_release` passes `--items`; the release's map, so made, came to 47,779,886 bytes
(naming the two downloads and holding the 7z of 47,780,052), the whole release in 635 seconds.

**19.96 Every dimension at every degree, and the starting page, as `.sieve` (Edward, 22 October 2026; measured).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

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
  to 6 GB expected from how the address is made, not measured). *Built (6 November 2026):* "about N GB of
  memory" beside the estimate, and "more than the N GB free: it may be slow, or stop" where it is:
  200 MB, 150 MB for each of up to as many files as threads (at least 3), and nine times what is
  read. Fitted on six measured here (docs/, six GIFs, nine files, dickens, samba, an 11 MB program:
  0.65 to 1.04 GB at the most, on 4 threads), each within about a quarter; it puts Edward's 377 MB
  folder on 12 threads at 5.4 GB, where it took 4.1 (§19.105).

- *The roof (6 November 2026, results/roof-enwik9-2026-11-06.md):* enwik9 whole made at 12,950 MiB
  at the most with Optimise Search off, where the estimate said 9,236; unticked and from 64 MiB it
  now says 2,000 MiB and 11.5 times what is read (within 3% of the three measured). Ticked, it held.

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
- *A folder's files worked on several at once (28 October 2026, step 3 of the plan of that day;
  no new version).* Measured first (`bench_make.py`, which now times mixing as `pack.mix`, and
  `--without-own-anchors` leaves out Edward's own anchors folder): since mixing came (v24 to v30),
  it is most of a file's time (prose 2.37 s of 3.59, the quick set's tar 10.73 of 22.94, its check
  by decoding most of the rest), and one file's mixing is one stream, which threads cannot split
  without a new version. But a folder's files were found one at a time: on this repository's
  `docs` (33 files, 2.4 MB, 4 threads) the rope's searches came to 17.3 s and mixing to 9.5 s of
  40.8, on one core. Now each file's way is found on its own, as many files at once as the threads
  and half the memory free allow (a mixer's tables each, as the groups' rule), each file's own work
  (cutting, LZMA2's passes) given its share of the threads so a timed search keeps a core; what each
  finds is taken in the listing's order as one thread took it, and its progress reaches the File
  Locator through a lock. Measured on `docs`: 40.8 s → 20.1 s at 1 s of search a file; with no limit
  51.3 s (one thread) → 23.0 s (four); the instructions the same, byte for byte, on one thread and on
  four, with the search unlimited and at 1 s, installed back identical. The CI makes the test site's
  folder on one thread and on four and compares them (step 11). A single file is as fast as before:
  its time is mixing's, and mixing faster with the same codes is what is left.

**19.100 "Create .sieve" in the right-click menu (Edward, 24 October 2026; noted, not to be built yet).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*


**19.101 The crawler as a card: a hardware layer for a site's table (Edward, 25 October 2026;
noted, not for even the near future).** "If the crawler could be turned into a physical and signed
chip or card of some kind that could be a hardware avenue. Imagine a slot in card that has a chip
responsible for running the crawler and each card comes with a key the owner must keep safe, they
can use the key to replace the tree the crawler knows." So: the crawler of §19.92 (`crawl_site.py`)
on a chip of its own, slotted into the server or beside it, holding the site's table hash where the
server cannot change it. It checks the site against that table on its own, so stopping the crawler
while the site is replaced would mean getting at the card. Only the card's key, kept by the owner,
lets a new table in after an update. It answers the case §19.92 leaves open (a server replaced whole,
and the software crawler stopped with it). Whether it could also refuse to serve a page that fails,
rather than only report it, is for when it is thought about. A thought only; nothing to work on.

**19.102 Towards the minimum address: the triangle, mixing and generators (Edward, 26 October 2026;
the three built; mix-v2, 27 October 2026).** Edward asked for filters aimed at the shortest address a real file can have, even
if the true minimum is out of reach. Any address is two parts: the model the reader must already
have, and where the file falls among what that model allows; a filter is a model, and a better one
makes the second part shorter. Two limits stay in view: there are fewer short numbers than files
(only a file with structure can have a short address), and the true minimum cannot be computed in
general, only approached from above. So each idea was judged by how much shorter it makes that part,
measured on this repository's own files.
- **Grammars, dictionaries and rulesets (Edward's).** Models, and good ones for text; but as
  predictions rather than hard rules: a hard filter loses any file that breaks one rule (a name, a
  typo), a prediction only pays a few bits for it. They belong as models inside mixing (below).
- **The triangle (Edward's: a byte, its neighbour before, the one above and the corner, at varying
  row lengths): built, rope-wave-v6.** It is the median edge detector of LOCO-I (JPEG-LS): `P4`, and
  `T1`, what is left coded by how busy the triangle is. The Vitruvian Man as raw grey pixels (949 by
  1300): 762,766 bytes, against the rope's best before 831,022 and LZMA2's 865,400. The row length is
  found from the bytes alone: of every width from 2 to 2,048 the true one (949) cost about 10% less
  than any other. Where noise outweighs edges the plain average (`P2`) still wins; the search weighs
  them all. (`sieve-manifest-v23`.)
  *The triangle as Edward designed it (recalled 4 November 2026):* "similar to the rope wave where it
  would lock in three bytes and maybe even propagate noise along the connections between vertices".
  What was built predicts each value from three neighbours (the byte before, the one above, the
  corner), the median edge detector above, which is what won on pictures. The design as meant, a
  triangle's three vertices kept and its edges and inside drawn from them, with noise carried along
  the edges, is a shape with its vertices kept, as §17.4 measured for squares and circles (on real
  pictures, a lattice of kept corners lost to predicting from neighbours; "Pythagoras be damned, the
  triangle would be the best"). The triangle among those shapes is not recorded as measured; §17.4's
  prototype would take it as one more shape, beside real-rope's moving locks (§19.110).
- **Mixing (Claude's): built, mix-v1.** Every predictor at once, bit by bit: orders 0 to 6 and the
  word so far, their predictions mixed by weights that learn as the file goes, coded exactly by an
  arithmetic coder, in integers only (the same code on every machine; CI pins one). Don Quixote
  (2,391,678 bytes): 565,596, against LZMA2's 685,794 (17.5% smaller); the rope, made for tables and
  pictures, 1,324,573. Its cost is time: about 0.4 MB/s, decoding as slow as coding, so it is tried
  on a file's start first. (`sieve-manifest-v24`; `sieve mix`.) A Python sketch had found 30,424
  bytes on the book's first 100,000 (xz 35,696); in C++ with integers, 29,301.
- **Generators (Edward's Minecraft seed): built for PNGs.** A file a small known program made has
  that program and its input as its true address. Sieve's own files had one: nine of ten PNGs
  measured are exactly what stb_image_write writes at its level 8, so they are carried as their
  pixels and made again by it; the others (zlib's) from a skeleton, their deflate by preflate. Ten
  PNGs, 657,221 bytes: 247,695 (`sieve-manifest-v25`). The same holds for the archives remade since
  v14. Where the program is not known (a photograph, prose) there is no seed to find.
- *Measured and not taken:* the triangle on these screenshots (flat interface colours suit LZ's
  long repeats: their filtered rows by xz 274,588 in all, their pixels by the rope 582,870, by mixing
  344,626); preflate on stb_image_write's PNGs (its difference about 28 KB on a 44 KB screenshot:
  larger than the PNG).
- **Mixing existing methods in a new way: built, mix-v2 (27 October 2026, `sieve-manifest-v26`).**
  Edward's ask, taken up: the filters Sieve already had, each made a model inside mixing, weighed bit
  by bit. Measured first on this repository's files, each part taken only where it paid: a **match
  model** (the longest earlier stretch the last six bytes repeat: what LZ does, and what mix-v1
  lacked: the teapot 53,595 bytes by v1, 38,030 with it); the **column** (the rope's byte above,
  v5's idea) and the field, for tables (moby-pos 205,122 to 167,683; a word list 71,873 to 43,209);
  sparse contexts; two mixers and two SSE stages; counters in slots of 16 a half-byte (two places
  in memory a byte, not eight: as fast as v1 with twice the models). And **the triangle inside
  mixing**: for a picture (its width and bytes a pixel, from the rope's search or guessed from its
  bytes) the triangle's prediction and its neighbours are contexts: the Vitruvian picture 727,842
  bytes (rope-wave-v6 762,766, mix-v1 802,822); a screenshot's pixels 62,925 (LZMA2 93,936, mix-v1
  105,134). All against mix-v1: Don Quixote 532,168 (565,596), `test_core.cpp` 53,002 (64,777), a
  SCOWL word list 41,010 (84,763). As Sieve instructions: Don Quixote 532,501 (v25 565,664); the ten
  PNGs 210,046 (v25 247,692, measured again the same day).
- *Measured and not taken (27 October 2026):* LOCO-I's bias correction on the triangle (0.4% on
  the Vitruvian picture, and the screenshot twice as large; mixing with the triangle does 4.6%
  better anyway); a dictionary (SCOWL) read first, as a model every reader has (2% on Don Quixote,
  worse on a 3 KB text); a fast-moving second probability in every counter, or inputs scaled by how
  often their context was seen (each better on one file, worse on most); tables of 2^23 (under 1%,
  twice the memory).
- **Grammar as a model: built as a word list's prediction, mix-v3 (27 October 2026,
  `sieve-manifest-v27`).** Edward's rulesets taken as predictions, three ways measured. The parts of
  speech of the words before (Moby's tags) as contexts: 0.15% to 0.55% on Don Quixote and worse on
  small texts, not taken (the orders see the same). A dictionary as a model every reader has: from
  the word so far, the letters SCOWL has next (and the word's end), each by its count of words, as
  a probability for each bit, mixed with the rest: Don Quixote's first 800 KB 191,190 bytes to
  187,650 (1.9%), a 3 KB text 1,303 to 1,212 (7%), a word list 41,770 to 27,138; as Sieve
  instructions Don Quixote 527,837 (v26 532,501). Where a text is not words (a PDF, a table of
  numbers) it costs a little, so the packer weighs both. A word missing from the list costs a few
  bits; it never fails, as a hard filter would.
- **Decoding speed (27 October 2026):** Prioritise Decoding Speed (no mixing: the ten PNGs in 0.6 s,
  247,692 bytes, against 28 s and 210,046); files decoded at once when installing (28.3 s to 9.1 s
  on 4 cores); mixing itself faster with the same codes (the SSE rows and the counters' slots
  fetched before they are needed: 8% to 18%).
- **The SSE stages retired (27 October 2026, Edward): mix-v4, `sieve-manifest-v28`.** Measured again
  after their rows were fetched early, they took 9% (text) to 29% (a photograph's grey pixels) of
  the decoding time for 0.07% to 2.3% of the size; the first stage alone kept most of that. Edward:
  retire both as one filter (`sse-v1`), its figures kept (FILTERS-CONFLICTS' retired list, the code
  beside it), so it can be looked at again. mix-v2 and mix-v3 keep them and are read as before.
- **Files mixed together (27 October 2026, `sieve-manifest-v29`).** Mixing the stream the small
  files share in place of its LZMA2 was the plan (294 small texts: 95,425 bytes against 110,835); in
  real instructions that stream turned out small, every file over 4 KiB being mixed on its own. The
  gain was in the files mixed one by one: each starts from nothing, so mixed together (in that
  stream, in the listing's order) each starts from what the others taught. Sieve's `tools`
  349,689 bytes to 260,529 (−25.5%), `client` 336,403 to 247,045 (−26.6%), `docs` −6.8%, `tests`
  −1.2%. The cost: one stream decodes in one piece (`docs` 1.1 s to 4.2 s), and it is held to 16
  MiB, so a folder as large as the release (Sieve's data, 27 MB) is not mixed together.
- **Groups of files mixed together (27 October 2026, `sieve-manifest-v30`).** The folder's files
  that compress cut into groups of up to 8 MiB in the listing's order, each group mixed, the groups
  decoded at once; files already compressed (LZMA2 can't shrink their start by 5%) left out. Sieve's
  data folder: 3,160,426 bytes (v29) to 3,100,643, installed in 14.1 s (12.4 s). Groups of 16 MiB
  were as small there (3,099,798) but installed in 28.6 s, one group on one thread; on 34 MB of
  the repository's texts they were 4% smaller than 8 MiB. The group size is the packer's, so it
  can be moved without a new version.
- **Every .sieve at the newest (27 October 2026, Edward: "a .sieve should always be the latest").**
  Maps (sites' tables), the release's `source.sieve` (now the source tree's own instructions), sites' pages, training runs, the hallway's saves (its
  degree sample: 7,887 bytes, against 482,495 as v4) and the music favourites all use one helper,
  `add_packed_newest`; the File Locator and `sieve locate` were already newest.
- **The File Locator's estimate counts the search as used (27 October 2026).** It counted each
  search's whole limit, three times over (a file whole, its pieces, the weighing, which mostly
  finds packing's answers cached); the searches stop early on most files. Now the share of the
  limit runs here have used is learned (`[locator] search_share_*`), and the estimate comes down
  as the files' searches finish. Sieve's data folder at 0.5 s a file: estimated 6 min 44 s at
  first, took 3 min 06 s; both shares measured 0 there.
- *Next:* a word list with how often each word is used
  (SCOWL has none; a frequency list would be a new pinned file); and more generators as they turn
  up (zlib's PNGs remade by zlib itself where it can, without preflate's difference).
- **Mixing made faster, the same codes (28 October 2026; Edward: "make a start on all of that").**
  Mixing is a single file's floor (enwik8: 109 s to make, 149 s to unpack), and it codes and decodes
  alike, so a gain is both. The rule: every code the same, byte for byte; anything that changes what
  mixing predicts is a new version and waits.
  - *Measured first.* A set of five (`IDEAS.md` as prose, with and without the word list; SCOWL's
    80 list; a tar of `tools/cli`; the Vitruvian picture as 560 × 767 RGB pixels), coded and decoded
    on one thread, each code's SHA-256 kept as the guard. Profiled with callgrind on 128 KiB of the
    prose (`perf` was not to be had): about 1,590 instructions a bit, of which the mixers' two sums
    (`dot`) 16.4% and their training about 20%, each done an input at a time (the products need 64
    bits, and SSE2 has no 32-bit multiply, so no compiler vectorises them at the plain x86-64 the
    build is for); moving the counters about a quarter, 2.9% of it the check, on every call, that a
    function's `static` table was made.
  - *Done.* The sums and the training in SSE2 (every x86-64 has it), eight inputs at a time and
    exactly: an input is within ±2,047 and a weight within ±2^22, so a weight is split into its high
    part (w >> 11) and its low part (11 bits), each 16 bits, and `pmaddwd` sums each against the
    inputs in 32 bits, the two meeting in 64; training's input times error (within ±2^24) is joined
    from two 16-bit multiplies, shifted and held as before. The inputs padded to eight (those past
    the last are 0, so they add nothing and change no weight). The reciprocals' table made when the
    program is built, the stretch and squash tables before it starts, so no call checks them. Other
    machines (ARM) keep the plain loops; the unit tests pin mixing's codes by their SHA-256 (a text,
    with the word list, a picture: mix-v2, v3 and v4), and the CI runs them on Windows and Linux
    (SSE2) and macOS (ARM, the plain loops), so both are held to the same codes.
  - *Measured after:* on the 128 KiB, instructions 1,666,867,467 → 1,434,774,488 (13.9% fewer), data
    reads 514 → 381 million, cache misses the same (so mixing is bound by its instructions more than
    by memory here). Every code of the five the same SHA-256. Wall-clock time on this shared machine
    moved by more than the gain from run to run (coding about 9% faster over three runs each, decoding
    within the noise), so the time is Edward's to measure: `sieve mix FILE` gives both.
  - *Left:* the counters' moving (a quarter now): each needs a 64-bit multiply and the counters lie
    apart, so gathering them for SSE2 may cost what it saves; measure before trying.

**19.103 Pieces weighed by the ways that code them (Edward, 28 October 2026; an idea, nothing
built).** His question: is the rope checked against each of LZMA2's pieces on its own, or against
the whole stream, and would it be worth doing if not?
- *What is done now (v13, `pieces_of` in `tools/cli/pack.cpp`):* a file of 16 KiB or more is cut
  into blocks of 4 KiB, and each block is weighed on its own: LZMA2 on that block alone against an
  estimate of the rope, each byte priced by how often it occurs in the whole file (an order-0
  count). Runs saving less than 64 bytes go to their neighbours' way, and each boundary is moved in
  steps of 64 bytes to where the two cost least. Then every rope block is joined and coded with one
  wave (one set of parameters), and every LZMA2 block goes to the shared LZMA2 stream. Mixing is
  not a piece: it competes only for the whole file.
- *Where the weighing is rough:* the estimate is not the rope. The rope predicts each byte from
  its neighbours (the byte before, the line above, the triangle), so on smooth data it does better
  than the count says, and a block it would win can go to LZMA2. And LZMA2 is weighed without the
  history it has in the real stream, so a block that repeats something earlier in the file looks
  dearer than it is. The two errors lean opposite ways; which wins on a given file is not measured.
- *First, needing no new version (suited to the pause):* weigh each block by the rope's true cost
  (the whole file's best wave run over it) and LZMA2 with the history before it. Only where the
  bytes go changes; the pieces list is data, so the format is the same. Measure on the
  repository's mixed files and a tar of the source, each packed with today's cut and the new,
  before keeping it. A tar or a game's archive of pictures beside text and tables is where a gain
  is likeliest; a single photo, whose metadata is already cut within a few bytes, least.
- *Later, a new version (waits for the pause to end):* each rope piece its own wave (two pictures
  of different widths in one tar each with its width), and mixing as a third kind of piece, so a
  file's text is mixed while its pictures go to the rope.
- *Measured (28 October 2026): the first step gains nothing; not kept.* Tried as a switch, each
  block weighed with the 64 KiB before it (LZMA2 with that history; the rope by the whole file's
  best wave, as the difference of its estimates), on four files packed with `--items` and 5 s of
  search a file. The `.sieve` came out the same size, byte for byte, under all four weighings
  (today's, LZMA2's history, the rope's, both), because mixing the whole file beat every cut:

  | File | Bytes | LZMA2 | Rope | Best cut (weighing) | Mixed whole: the `.sieve` |
  | :--- | ---: | ---: | ---: | ---: | ---: |
  | Vitruvian Man as a BMP | 1,288,614 | 816,846 | 597,269 | 823,554 (both; the others made no cut) | 558,858 |
  | the test site's pages and JPEGs, a tar | 501,760 | 451,965 | 494,152 | 452,651 (both) | 451,341 |
  | `Pages/order.pdf` | 789,552 | 787,477 | 790,448 | 787,629 (rope) | 787,457 |
  | two docs, a screenshot and Vitruvian as BMPs, meshes: a tar | 7,618,560 | 1,107,004 | 1,967,824 | 1,065,770 (both) | 780,192 |

  The new weighing did make better cuts where there was something to cut (the tar: 1,065,770
  against no cut at all before), but pieces only choose between LZMA2 and the rope, and mixing
  (mix-v2's pictures and text, since 27 October) now beats both. So what is left of the idea is
  the later step: **mixing as a kind of piece,** or a mixer told where a file's parts change (the
  tar above was mixed whole as one picture, its text included). That is a new version, in the list
  at the top.

**19.104 A game's files, sieved, and streamed into a cache as it runs (Edward, 28 October 2026;
planned, nothing built).** Edward has another Claude session rebuilding the engine of *Alien
Trilogy*. Once it runs normally it is the test bed for sieving a game's small files and decoding
them, or streaming them into a cache, as the game asks for them.
- *Why it suits Sieve:* a game of that age is many small files (levels, sprites, sounds, palettes),
  the sizes where the statistics show Sieve's gain is largest, and an engine of its own means the
  loading can be changed to read from a `.sieve` with nothing else to persuade.
- *What Sieve has towards it now:* the web host decodes one file of a `.sieve` of many by its path
  (§19.92), checked against its own SHA-256, though the whole `.sieve` is decoded and checked first
  and the file cut from it; Prioritise Decoding Speed leaves mixing out (enwik8 decoded in 1.8 s
  against 148.7 s mixed); files mixed together are decoded a group at a time (up to 8 MiB, v30);
  and Optimise Compressed Formats, temporary, carries a game's own archive as its files and makes
  the archive again on installing (v31).
- *What it lacks, to measure before building:* a file decoded without decoding the rest. A
  `.sieve` decodes as a whole (its streams, its groups), so the first file costs the time of the
  whole; a cache filled at start-up hides that, and is the first thing to try. Reading a group at a
  time, or the listing first and each stream only when a file in it is asked for, would come next.
  Each of those that changes what is written needs a new version, so it waits for the pause to end.
- *What to record:* the game's files, their sizes, 7z and Sieve both ways (size, and time to first
  file and to all), and the engine's load time from the folder against from the `.sieve`.
- *Measured (12 November 2026, results/stream-layouts-2026-11-12.md):* 95 game-like files (meshes,
  a font, text, icons, 40 sounds, 30 sprites; 11.5 MB) as one `.sieve` (10,342,984 bytes, the first
  file in 0.97 s, the time of the whole), one per kind (−0.06%), or one per file (+0.21%, any file
  in 18 ms, median). So streaming needs no new version: a small `.sieve` a file, named by its hash.
- *12 November 2026 (Edward):* ALTEngine is nearly done, and will be the first to get the Sieve
  treatment: its last unnecessary data stripped, and its assets streamed in and out of it and of the
  game's other engines (the original through DOSBox, and any variant), one sieved copy of the
  player's own files serving all of them. *Alien Trilogy* is also the first target of the game
  dimension: a full map of the game across its engines, ALTEngine among them (GAME.md §11).
  ALTEngine already strips unnecessary bytes from the game files, with more planned, and reading
  assets from `.sieve`s is planned in it. The aim: one hallway of game engines, a dimension that
  folds in on itself throughout time (GAME.md §11).

**19.105 A gigabyte's instructions, read and made in proportion (Edward, 28 October 2026; built).**
Edward's WoWRevived release (1,426,355,971 bytes, mostly films and music already compressed;
Proof/STATISTICS.md) came to 1,075,924,966 bytes as `.sieve`, 0.20% over 7z, and its installer
program went to 6 to 12 GB of memory and stopped answering while it read the file. Measured here on
a folder made like it (377 MB: three 125 MB files of noise and this repository's docs):
- *Reading a `.sieve`:* the file's bytes were made into hex text (twice their size), the text into
  one number, the number back into the manifest's bytes; `sieve-install` did it twice, once to see
  whether anything was attached to it (on its window's thread) and again to read it. The manifest is
  only the file's bytes with 1 taken from each, borrowed from the last (SIEVE-FORMAT §1), so it is
  now done in place in one pass (`file_of_raw`, and `raw_of_file` the other way for every writer:
  `sieve locate`, the File Locator, sites, maps, training runs, items and music); whether a program
  has one attached is read from its last 24 bytes. The same bytes either way: 85,807 files and raw
  strings (every one of up to two bytes) read and made both ways and compared.
- *The rope's coder:* it held each value as 64 bits in five arrays the file's length (the values,
  what the wave leaves, the positions, the segments, the rope), so a file of bytes took about 40
  times its size to decode and the search's estimates as much each time. Now values are read where
  they lie (encoding, estimating) or held at their own width (decoding, into the output itself), and
  the positions, segments and rope are made only where kept values or columns need them. The same
  codes and estimates: 100 files and settings (every version's ways, keep, columns, groups, the byte
  above, the triangle, 8, 16 and 32 bits) coded, estimated and read back, against the old build's.
- *Measured, the 377 MB folder:* installing 63 s and 11.5 GB at most → 13.6 s and 2.4 GB; making
  205 s and 11.8 GB → 182 s and 4.1 GB. The 42 MB one: installing 1.9 GB → 325 MB. Made with the
  search unlimited, this repository's docs come out byte for byte as before (and in 19.7 s against
  23.0); with a time limit the search gets further in its time now, so a `.sieve` may come out a
  few bytes smaller (the 377 MB folder: 11).
- *Left:* `sieve-install` read a file on its window's thread (now on a thread of its own, §19.107); a file's LZMA2 estimate takes most of making a large
  incompressible one (112 s of 139 on a 125 MB file, at about 1.1 MB/s); and where WoWRevived's
  0.20% against 7z is, file by file.

**19.106 Files that hold much of the same (Edward, 29 October 2026; waits for the next version; built since, as v32's `delta`).**
In WoWRevived's release, `MAPS.WoW` and `MAPS-Larger.WoW` hold mostly the same content and differ in
a small number of files (the larger minimaps). Sieve carries each file on its own: LZMA2's
dictionary can find the second's matches in the first only where both go into the lzma2 stream
together and within the dictionary's reach, and a file mixed (mix4, mixt) is coded with no sight of
any other file, so the shared content is paid for twice.
- *The way:* a file carried as its differences from an earlier file of the folder (a `delta` way
  naming that file: copies from it and new bytes, the new bytes then carried by any way). The
  installer makes the earlier file first, as it already does in the listing's order. A new way is
  a new manifest version, so it waits (the list at the top).
- *Without a new version:* the packer could put such pairs side by side in the lzma2 stream with a
  dictionary large enough to hold the first; whether that helps, against mixing each, is to be
  measured on the pair before anything is built. Finding the pairs: files of similar size whose
  sampled blocks' hashes mostly agree.
- *Measured, 29 October 2026 (pairs made here: B is A with a few regions replaced by random bytes).*
  Each finding is a packer's choice that v31 already reads.
  - *Carried as they are, side by side in the lzma2 stream:* a pair of 20 MB came out at
    9,637,859 bytes, under solid xz's 9,649,776, and one file alone is 8.14 MB with xz: the second
    already costs only its changes, since the stream's dictionary (48 MiB) reaches back over the
    first. That holds while the pair is under the packer's 64 MiB dictionary. Past it, on a pair
    with no repetition at long range within A (random words and random bytes): a 30 MB pair in one
    raw LZMA2 stream (preset 6) is 30,914,001 bytes with a 16 MiB dictionary, shorter than
    the distance, and 16,605,654 with 64 MiB, longer (one file alone: 15,094,454). For MAPS.WoW
    and MAPS-Larger.WoW (160 MB each), that wants a dictionary of 192 MiB or more where they are
    carried so (the format allows to 4 GiB; the packer stops at 64 MiB; encoding takes about twelve
    times the dictionary in memory, decoding the dictionary).
  - *Mixed:* each file mixed alone, 1,376,163 + 1,441,719 = 2,817,882 bytes; the two mixed as one
    group (v30's `mixt`), 1,571,543, 44% smaller (3 MB each): mix-v4's match model finds the second
    in the first. The packer groups only files of 8 MiB or less, and the group decodes on one
    thread, so a pair of 160 MB as one group would take about twice as long to install as the two
    decoded side by side.
  - *To decide (Edward):* both change what the packer makes for such folders (smaller, and still
    v31), so neither is built: a larger dictionary where the stream is longer than 64 MiB and memory
    allows; and a group of two large files found alike (their sampled blocks' hashes) where mixing
    carries them, perhaps only with Try every way. First, which way MAPS.WoW is carried: Keep a log
    says so for each file now.
  - *What finding and writing a delta costs (30 October 2026; Edward asked whether it would need a
    "contains similar files" box for its memory).* Measured with a prototype (not Sieve's code) on a
    pair made here of 160,000,000 and 160,068,077 bytes: A from the repository's own files, B as A
    with 40 regions of 200 KB replaced by random bytes and 30 insertions of up to 5,000 bytes (so
    everything after an insertion is shifted).
    - *Telling that they are alike:* blocks cut where the content says (a rolling hash over 48 bytes,
      about every 4 KiB), one 8-byte fingerprint each: 96.6% of B's blocks found in A, 117,234
      fingerprints for both files (under 1 MB), 0.71 s. Blocks cut at fixed places found 4.4%: one
      insertion shifts every block after it, so fixed blocks will not do.
    - *Writing B as copies from A:* A indexed every 64 bytes (2,323,501 entries), B scanned with a
      rolling hash: 332 copies covering 152,101,975 bytes and 7,966,102 new bytes (the random bytes
      put in), in 2.69 s, the index about 92 MB beside the two files' 308 MB.
    - So no box is needed for its memory: the packer holds every file of a folder in memory already
      (`add_packed`), the fingerprints are under 1 MB, and the index is built only for a pair found
      alike. A larger LZMA2 dictionary, by contrast, takes about twelve times itself to encode
      (about 2.25 GiB for 192 MiB).
  - *Built, in v32's draft (30 October 2026; `--v32-draft`; SPECIFICATIONS v32, METHODS §2a).* The
    `delta` way, for files over 8 MiB. On the same 160 MB pair, made with `sieve locate --search-per-file
    2`: v31's build made v20 instructions of 22,537,063 bytes in 1,113 s, installed in 6 s; the draft
    made v32 of 17,947,687 (20.36% smaller) in 627 s, installed in 5 s, the second file written as
    a script of 7,965,551 bytes (almost all of it the random bytes put in, which nothing packs).
    Both installed identical. On a 20 MB pair, inside LZMA2's window, the gain is small (4,637,619
    to 4,635,222) but the second file is not searched (85 s to 52 s). Below 8 MiB it is not used:
    two 100 KB texts sharing two thirds came to 142,005 bytes as a delta, 136,939 mixed as a group.
  - *Edward's maps, by `sieve alike` (30 October 2026):* MAPS-Larger.WoW (162,426,225 bytes) holds
    98.7% of MAPS.WoW (160,890,897): 104 copies, 2,087,365 new bytes, a script of 2,087,830 bytes
    before LZMA2, found in 1.99 s. Carried so, the larger map would cost about its new bytes packed,
    instead of a second map's worth: to be measured on WoWRevived with the draft on.

**19.107 Edward's notes of 29 October 2026 (built).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.108 More waves through the rope (Edward, 30 October 2026; measured, nothing built).** His
thought: as the rope keeps bytes and propagates noise (its first name, "The Keep Bytes or
PropagateNoise filter", from his paper *Rope Around The Clock*: §19.87), run it in two or three
phases: a wave from the top down through the chosen width, then one from the bottom up, and so on,
waves times width in byte rows; record the seed of each wave, reduce the data to clumps of similar
values, and knock as much of the file out as possible ("the right file and the right noise could
cancel out a file to 0"), recording whatever remains.
- *The noise from a seed:* §19.91's answer holds. A seed short enough to save anything names only a
  few of the possible noises, so for almost every file the seed and what remains cost what the file
  does. The noise that pays is the noise the reader makes from what it has already unpacked, which
  costs nothing to name, and that is the rope's wave: its prediction of each byte.
- *More waves over what the first leaves (measured, 30 October 2026):* grey pictures, the remainder
  of each scheme by its order-0 entropy and by LZMA2. Vitruvian (949 by 1300): as it is 6.443
  bits a pixel; one wave, the triangle (P4), top down, 5.219 (LZMA2 789,160 bytes); then a wave from
  the bottom up over its remainder 5.468 (827,592); then a third, down again, 6.274 (919,452), near
  the picture untouched. The test site's door (800 by 450): 3.623 as it is, 2.666 after one wave,
  3.279 after two, 3.974 after three, worse than the picture itself; its book likewise (4.784,
  4.021, 4.668, 5.460). A wave across by columns after the one down did as badly as one back up.
  After a good first wave what is left is close to noise, and a wave over noise adds to it.
- *Two waves that meet:* the even rows by one wave down, then each odd row from the rows above and
  below it, both known by then (the way interlaced and hierarchical picture coders use two
  directions). Vitruvian 5.481 bits a pixel by their mean, 5.287 with the row's left neighbour in a
  median (LZMA2 785,152 and 791,916, against one wave's 789,160); the door 2.967 and 2.725 (one wave
  2.666); the book 4.315 and 4.104 (4.021). About even, no gain.
- *Where it points:* choosing each prediction by what is round it ("clumps of similar values") is
  what mixing's picture models do, and they beat the rope on pictures already (Vitruvian as a BMP:
  the rope 597,269 bytes, mixing 558,858; §19.103). Nothing is built; the measurements are kept so
  the idea and its answer are not lost.
- *A designed file to show it (Edward, the same day):* noise propagated to make the test file in
  the first place, "diagonal lines of the same values" for the perfect designed case (`00 FF 00 FF`
  / `FF 00 FF 00`), then pushed further before the real world. Measured at once, two files of 1 MiB
  (256 bytes a row, 4,096 rows), each beside `xz -9e` and the rope's search of 5 s:
  - *00 and FF along the diagonals:* xz 292 bytes; the rope **24** (`L8 W512 S2 P3`, the diagonal
    phase): the whole file cancelled to a code of 24 bytes; its `.sieve` 101.
  - *a random first row carried down the diagonals* (each row the one above moved one along): xz
    848; the rope 14,173 (`L8 W257 S2 P4 T1`), since no phase copies one chosen neighbour (the
    phases mix or take the median of several), so it cannot see "the byte above and to the left"
    alone; the `.sieve` 1,556, by LZMA2.
  - *What that points to:* a wave direction, a phase that copies one chosen neighbour (above-left,
    above-right), which would cancel the second file after its first row. A new rope-wave version,
    so v32's if it is built, and only once it is measured on real files; the designed files are the
    demonstration and the first test, then variations (noise added, widths that do not fit, several
    directions in one file), then the real world. To be planned with Edward.
- *Repropagating until the file cancels (Edward, the same day):* "In theory they could even
  repropagate until cancelling the whole file out", stored as: the file's length; the width; the
  seeds; n passes of repropagation through the file; seeds times n; the remainder as offsets and
  bytes. The shape is close to a rope code's already (its size, the wave's parameters, what is left
  coded by how often each value comes); what is new is seeds and passes. What decides it is whether
  each pass saves more than it costs to write down: a pass whose wave comes from the bytes already
  unpacked (as the rope's does) costs only its few bytes of parameters; one whose wave comes from a
  seed of k bits can choose among 2^k waves, so it saves at most about k bits more than a fixed wave
  would (§19.91), and n passes cost n seeds. Measured above, passes over what the first left made
  the pictures worse. So the question to take up (Edward: "Once the tests are done and out of the
  way, we will clear up anything that is in progress and look at this idea seriously"): are there
  real files where a second or third wave, chosen per file, removes more than its parameters cost?
  The designed files first, then varied ones, then real ones.
- *Waves that cancel each other, and seeds of zero (Edward, the same day):* on real files alternating
  waves could cancel each other out, but some seeds could be zero, so that a wave is left out where
  it would; "very much a brute force approach". A zero seed is a wave switched off, and which waves
  are on costs about a bit each; the seeds of the waves that are on are the cost that stays.
  *What it compares to:*
  - **matching pursuit** (sparse approximation): from a dictionary of waves take the one that fits
    best, write which it is and how strong, take it away, and go again on what is left. This is
    nearly his scheme. It is studied for sound and video; in lossless coding it usually does less
    well than prediction, since what is left must be kept exactly and each choice costs its index.
  - **fractal compression** (Jacquin, the 1990s): brute force for parts of a picture that are
    transformed copies of other parts; slow to search, quick to decode, and lossy.
  - **searching for the shortest seed or program that makes the file** (Levin's search, Kolmogorov
    complexity): the limit of "find the noise that cancels the file"; it finds only files that were
    made from short seeds, as the designed test file is and real files almost never are.
  - **searching parameters**, which Sieve does already: the rope tries hundreds of waves a file, and
    LZMA2 several settings.
  So it sits between matching pursuit and the rope's search: the waves its dictionary, the zero
  seeds its waves left out, the repropagations its passes. Whether it pays on real files is the
  question of a dictionary of waves that real files are made of.
- *The designed files, built (Edward: "Starting on that is a good idea yes"; the first piece of the
  benchmark script and test files, FORMATS.md).* `tools/rope_designs.py` makes eight files of 1 MiB
  from fixed seeds and measures each against xz -9e, the rope's search (5 s) and the whole
  `.sieve`; the table is `results/rope-designs-2026-10-30.md`. The rope cancels only the checker
  (24 bytes, the diagonal phase P3 predicting it exactly). Every other file is a pure copy of one
  neighbour (straight down: 1,350 bytes where about 256, its first row, would do; the diagonal carry
  14,173; a slope 1,347; the diagonal in rows of 250, 14,522), which the rope cannot see, since its
  phases always blend or take a median; xz did better on all of those (552 to 876). With one byte in
  ten moved, mixing's `.sieve` (85,276) beat both xz (133,552) and the rope (165,847). Two
  directions in one file defeat a single wave (the rope 175,074, xz 1,148). Random bytes, the
  control: nothing taken off.
  *So the measured target for a wave direction:* a phase that copies one chosen neighbour exactly
  (left, above, above-left, above-right), choosable for a stretch of rows so that two directions
  in one file work; on these files it should cancel all but the first row. Next, whether it pays on
  real files (sprite sheets, tables, game data); a new rope-wave version, so v32's if built.
- *The wave direction, measured (Edward: "yes make a start on that next please").*
  `tools/rope_direction_probe.py` copies one chosen neighbour (left, above, above-left,
  above-right), for the whole file or chosen afresh every 16 rows. A fifth choice, the triangle,
  can be offered beside the four. Each remainder is measured by its order-0 entropy (the rope's own
  coder) and by LZMA2. The tables are in `results/rope-direction-2026-10-30.md`.
  - *Designed files:* the direction cancels what the triangle could not. The diagonal goes from
    926,317 bytes to 8,964. Two directions in one file go from 597,283 with one direction to 9,633
    with a direction a stretch. Diagonal+noise goes from 976,609 to 127,507, under xz's 133,552.
  - *Real files:* the triangle stays best, or about even, on the pictures. The door is 38,167 bytes
    by the triangle, against 76,325 for a direction a stretch. A direction a stretch helps the icon
    by 8.5% (29,660 against 32,406). On the tables and the program, it takes off 3% to 7%.
  - *Against xz:* no remainder came under xz -9e of the file itself on any real file.
  - *So:* not built. A direction is cheap to add (a phase and two bits a stretch), but on these
    files it would not change which way Sieve takes. What would decide it is Edward's game data
    (sprite sheets, textures), which the probe can be run on as it is.

**19.109 A look round, 31 October 2026 (three ideas measured; nothing built).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*


**19.110 Real-rope: keep bytes or propagate noise, as first designed (Edward, 1 November 2026;
measured, nothing built).** Found while looking at why Silesia's mozilla is not made smaller taken
apart. Edward: "It was supposed to do one or the other, not both, hence 'keep bytes OR propagate
noise'". The rope as built (§19.87) is left as it is, "because it gets results"; the design as he
meant it is recorded here under its own name, real-rope.
- *The design, in his words.* A filter that can keep bytes, or propagate noise, or both. "The 1/3
  and 2/3 locked byte was supposed to be able to fluctuate, they were just the starting positions";
  the divisions go further, to fifths, sevenths and ninths "the same way the inspiration jig does",
  scalable "up to maybe 1/2 the length of the file", with the centre a value where the file's
  length is odd and a gap where it is even, as in §19.87's fifth answer. It would "try to lock in
  any and every byte across the file with the start, the end and a starting value at 1/3 and 2/3 or
  further divisions", then search every variant of the file those locks allow, "a filter that
  might truly not end before the universe does". Each file reduced to the values needed to find
  it, with settings of its own.
- *What the rope does instead (read from `core/src/rope_wave.cpp`).* Its kept points are fixed
  (`kept_points`: the start, a third, the centre where the length is odd, two thirds, the end), and
  keeping (K1) is tried once, on the best wave the search found without it. P0 with K1 is keeping
  alone and P1 to P4 with K0 propagating alone, so each exists, but keeping is never searched in
  its own right and its points never move. The "and" of §19.87's title came in when his answers
  were written up as "the design as it now stands".
- *Measured (Claude, 1 November 2026).*
  - More divisions in the rope as built (a throwaway switch dividing by D rather than 3; 120 s a
    search): the mozilla program `libxpcom.so` (1,622,016 bytes) 1,076,708 at every D of 3, 5, 7,
    9 and 33 (LZMA2 361,372); `msgbase.xpt` 21,011 at every D (LZMA2 10,291); `raptor.jpg`'s pixels
    34,019 at every D (LZMA2 30,809); a 1920 by 1080 screenshot's pixels 919,078 to 919,081 (LZMA2
    388,812). On three of the four the best wave keeps nothing.
  - Keeping alone with the points moved (a model costing what the line leaves by its order-0
    entropy, the points moved by a search of about 1,000 to 57,000 tries): `libxpcom.so` 1,203,335
    keeping nothing, 1,498,110 kept at the thirds, 1,326,890 with 5 points moved, 1,300,914 with 9,
    1,212,600 with 33; `msgbase.xpt` 20,158, 29,401, then 20,156 at best; `raptor.jpg`'s pixels
    49,767, 125,180, then 49,767. Moving the points undoes the harm fixed points do, and no more.
  - Half the file kept (every other byte, each between coded from its two neighbours): 95.4% of
    `libxpcom.so`, 93.9% of `msgbase.xpt`, 66.8% of `raptor.jpg`'s pixels, 100.0% of LZMA2's output
    of `libxpcom.so`. The kept half is carried as it is, so it cannot promise a size.
  - The rope after LZMA2 (its output searched, every wave): `libxpcom.so`'s 361,372 to 362,377, the
    screenshot's 388,812 to 389,821.
- *Why the search over locks cannot make every file shorter.* Locking k bytes of n leaves 256^(n−k)
  variants; the k bytes are said and the variant is named, and naming one of 256^(n−k) takes n − k
  bytes: n in all, whichever bytes are locked. A rotation is the same: one that can carry any file
  to address 0 is one of 256^n, so it is the file again. Rounded to a step of 2^s it takes s bits
  fewer and leaves a remainder of s bits, which the installer needs to land exactly; checked on
  every 2-byte file, 16 bits at every step, and 19.17 bits on average where each file takes its own
  best step and the step is named. Edward's 13.1313 recurring bumped to 13.0: the filter that does
  it must be named, and a different file needs a different filter, so a million six-decimal files
  need a million filters, six digits to name one. What pays is a rule fixed in advance that real
  files fit (a recurring value, a smooth wave, an earlier copy), chosen from a few and paid for, as
  §19.98's fraction route found ("rfb rfb rfb ...", 256 → 78 bits; nothing on real text).
- *What real-rope can be, measured before it is built:* the locks and waves as a small set of rules
  the search chooses among per file, each choice paid for, tried first where the rope already wins
  (pictures, sound, tables of digits); or its prediction given to mixing as one input among the
  others. Either is a new way, so the next version.

**19.111 The shortcut seed (Edward, 1 November 2026; stage 1 built as a prototype, measuring).**
Edward: "The main focus of the compression should be to search more potential shortcuts to the file
than standard filter ruleset shortcuts"; "Every filter/method is a shortcut"; the name is his. The
aim is his first one (§19.87, §19.110): to shuffle the state space so that a file can be warped to,
weighing each route by everything the installer needs against the file and the other ways.
- *The plan (agreed the same day).* Any file, of any kind, and what is installed is the file byte for
  byte. A small language of shortcuts, combined freely, searched for the fewest bits, measured at
  every stage on the repository's files beside xz, mixing and the rope; stage 1b, every arrangement
  of small files and the arrangements real files favour; then the pieces that win somewhere built
  into Sieve as ways of the next version.
- *Stage 1 (`tools/shortcut_seed.py`).* A recipe is literal bytes and shortcuts after one transform
  of the whole file: copy an earlier stretch, the same distance again, a counting run, a copy of the
  byte above, above-left or above-right in rows of W, and Edward's skip-and-insert; the transforms
  none, differences at 1 to 4, the rope's waves (left, up, their average, the triangle) and the file
  read by columns. Costed as an adaptive coder would code it; every recipe decoded again and checked.
- *The two prongs (Edward):* "infinite addresses -> filters -> compression techniques -> infinite
  filters", and back: the seed searched on what beat it (xz's output, mixing's code), and each
  transform's output given to xz and to mixing.
- *Counting the cards (Edward):* "design a limited file with every possible arrangement and check
  every possible filter against it ... think counting cards but bytes". Stage 1b
  (`tools/shortcut_tiny.py`): four sets of 65,536 small files searched whole, and the 2-, 3- and
  4-byte arrangements of the repository's files counted by kind, the shuffle they give (the
  commonest first) tried on files held out and across kinds.
- *Skip-and-insert (Edward's example, worked on paper):* `00 01 02 0F 03 04 05 0F ... 0C 0D 0E 0F`
  (20 bytes) as "L3 W1 S1 0F", a wave counting by 1 that every 3 bytes puts in 0F, "a 7 byte address
  for a 20 byte file". Built as the shortcut `skiprun` (a start, a step, how often, the byte put in,
  a length) and measured: the seed finds it alone, one shortcut, 55.6 bits, 7 bytes, as he worked it.
- *Every shape a chance (Edward):* "each rule, shape or filter that I or we can spot or come up with,
  stands a chance at hitting a real file ... at that small a scale that rule would beat any existing
  form of compression, but obviously only for that file or files with similar layouts." So the
  language grows a piece at a time, each measured on the files it is meant for and on the rest.
- *Geometric filters (Edward, to be tried):* "a purely geometric expansion filter that uses geometric
  rulesets to shift the files bytes around, as well as one that alters the bytes; think circles,
  lines, squares, grids, geometric principles used to shuffle up a file."
  - *Moving the bytes:* orders of reading a file laid out as a grid other than row by row: by
    columns (built), zigzag, spirals in or out, rings round a centre, a Hilbert or Morton curve
    (which keep near bytes near), diagonals, rotations and reflections, and tiles read one at a time.
  - *Altering the bytes:* values changed by a shape laid over the file: a plane or a gradient taken
    away, a ramp along a line, rings of equal change round a centre, a grid's lines, each named by a
    few numbers.
  - *Its own chaos (Edward: "As with any filter it introduces its own element of chaos"):* a filter
    that helps one layout scatters another, and each kind of shortcut the language holds costs every
    recipe a little to say which kind it is (the tags' share). Both are weighed: a shape is kept where
    the files it fits gain more than the language's growth costs the others.
- *Gliders through a file (Edward, the same day):* "what chaos a few gliders might cause through a
  file from the game of life, or how the rules from Conway's game of life might be turned into a
  random filter: pick some random bytes, fire them off, have them alter values as they go through
  the bytes with a +1/-1 type schema, that could also be adjusted to be +256/-256 making sure to
  wrap the value around"; "probably the most chaotic one I can think of for sure, perhaps even
  useless". Notes for when it is tried:
  - The file is laid out as a grid (rows of W) and the gliders start from a few places, each named
    by the recipe; Life's rules move them, and each cell they pass is changed by a step (+1 or −1,
    or any step, wrapping round). On bytes a step of 256 wraps back to the same value, so the steps
    are 1 to 255 there, and ±256 counts on 16-bit values.
  - Life cannot be run backwards (many boards come to the same next board), so the gliders decide
    only where values change and by how much, from their starts and the rules, never from the
    file's bytes; installing then makes the same gliders and takes the same changes away.
  - What decides it is §19.91's: the gliders' starts are a seed, which saves at most about its own
    bits over a fixed pattern, so it pays only on a file whose bytes happen to follow the gliders'
    trails. Measured like every other piece: on designed files made by gliders first (it should
    cancel them), then on real ones.
- *What each result was made of (Edward, the same day):* "with our benchmarks and tests we start
  recording how much of what existing compression methods were used, how they were used and whether
  or not the rope or mixed methods are what finished them off. That way we can ensure it isn't
  presented purely as a new compression 'algorithm' and as a compression engine that mixes and
  matches existing techniques with the manifold warping of n-dimensional state spaces. Plus we can
  start to get a feel for what wins/loses where, when and why." A standing rule for every benchmark
  and test from now on:
  - *Recorded with every result:* how many files and bytes each way carried, and what that came to:
    LZMA2 (and its x86 filter), mixing alone, mixing in groups, the stream mixed, the rope, the
    pieces, lines of another file, deltas, the recipes that make archives, PNGs and JPEGs again
    (preflate, stb, brunsli, skeletons), and from the shortcut seed which shortcuts and transforms
    its recipes used; and, where one way's output went on through another, the order they ran in
    and which one finished.
  - *Named for what they are:* each existing method by its name and maker (LZMA2 and the x86 filter
    from xz/7-Zip, deflate from zlib, preflate, brunsli, stb_image, libarchive), and Sieve's own
    (the rope, mixing, the groups and weighing, the shortcut seed, the lines and the line's places)
    as Sieve's: Sieve is the engine that weighs and chooses among them and the routes between them.
  - *Already there:* the packer's report says most of this file by file and way by way (its `mix`,
    `groups`, `weighed`, `packed`, `deltas`, `jpegs` and other lines), and `tools/bench_files.py`
    keeps each run's report in `runs/`; what is still to do is to gather those lines into the
    results' tables, so a table shows what carried each figure as well as the figure.
  - *From a `.sieve` alone (Edward, the same day):* "a script that can log that from an existing
    .sieve ... That way we can reverse engineer the results if they ever get lost." To build: a
    reader of a `.sieve` in two depths. Light: its head and its listing decoded (the listing is
    itself LZMA2, but small: 1,735 bytes for mozilla's 525 files), writing each file's way and size
    and each stream's size and form, and each code a file has of its own read by its length (the
    rope's stream holds each mixed file's, a group's and the rope's codes, each after its length),
    with no stream decoded. Full (Edward: "it would have to decode the file to get the methods used
    or partially at least"): every stream decoded, so that what each file cost inside a shared LZMA2
    stream, which is one code for all of its files, is measured again (each file packed alone, and
    the stream with and without it), an estimate rather than the record, since the stream was made
    as one. What
    the install's log already says (Keep a log) is to be checked against it: it records each
    phase's time, and may not name the ways.
- *What it costs the world and what it saves (Edward, the same day):* "Presume a file is regularly
  downloaded, the kind of thing most of the world downloads daily (a Facebook page, the YouTube home
  page, the Google home page, a game's patch) ... weigh up the real costs vs savings for the world's
  IT infrastructure, where it goes, how much it could save for data centres and how much it might
  shift onto the end user", less on the user with Prioritise Decoding Speed and more with size. A
  benchmark to build, beside the record of what carried each result:
  - *Measured, for each way of making it* (Sieve for size, Sieve for decoding speed, zip, 7z,
    brotli, as they are): its bytes; the processor seconds to make it, once; the processor seconds
    to install it, on the user's machine.
  - *Assumed, and said as such:* how many times a day it is fetched; the energy to store and to move
    a byte (published figures for a network's energy a gigabyte differ by about ten times between
    studies); the power of a server's and a user's processor. Each a setting, the values used
    written beside the results, and the answer shown across a range of them rather than as one
    figure.
  - *What it reports:* a day's energy on each side: the servers' and the networks' (the bytes
    stored and moved, against the file as the others make it), the making (once, shared over every
    fetch), and the users' (installing, every fetch); so where the saving falls, where the cost moves,
    and at how many fetches a day each way of making it starts to pay.
  - *Real figures, later (Edward):* "base it off a single spec and try to get some real figures",
    which in full would need dedicated infrastructure. A step towards it with none: one reference
    machine, its spec written beside the results, and the making and installing measured in joules
    rather than assumed, by the processor's own energy counters (RAPL, under `/sys/class/powercap`
    on Linux; read on Windows by tools such as LibreHardwareMonitor) and a wattmeter at the wall to
    check them. The servers' and networks' side stays a stated range until it can be measured too.
- *On the website (Edward, the same day), for when the figures exist:*
  - The site's "Sieve State Space" button (`index.html`, to `Pages/sieve.html`) to become "Sieve
    Power Savings": the energy benchmark's results, what a day of fetches saves and what it moves
    onto users, for size and for decoding speed, with the machine's spec and every assumption beside.
  - The hallway in the browser, run on the user's end through the page, as first planned. How is
    open: written again for the web, built for it (C++ through WebAssembly, the build's own
    libraries with it), or emulated, depending on what the visiting system can run; to be weighed
    when the numbers are there to go with it.
- *Stage 1, measured (32 KiB of each of 24 files; `results/shortcut-seed-2026-11-01.md`).*
  - Designed files: the seed came under every other way on several (the gradient 13 bytes, xz 401,
    the rope 167; the checker 20, mixing 30, xz 83; the vertical 269, xz 345).
  - Real files: text and code within 2% to 5% of xz and behind mixing (README 4,291, xz 4,195,
    mixing 3,708; Sieve's program 8,216, xz 6,887); random and compressed bytes carried as they
    are (32,773 against 32,768).
  - The grey picture: the seed 22,178, the rope 22,228, xz 23,492; the triangle wave and then
    mixing 21,105, against mixing as a picture 21,166 (61 bytes, 0.3%).
  - The two prongs: the seed on xz's output and on mixing's code was 3 to 5 bytes larger on every
    file (no shortcut into them found); a transform in front of mixing helped the designed files
    (the checker 24 against 30, the gradient 17 against 464) and the grey picture, nothing else.
- *Stage 1b, measured (`results/shortcut-arrangements-2026-11-01.md`,
  `results/shortcut-counting-2026-11-01.md`).*
  - Every arrangement: in each set the shortcuts put the regular arrangements first (all one
    value, alternating, counting runs: in the 16 bytes of 00 or FF, 45.6 bits at best, 118.9 at
    worst), but every file cost more than the 16 bits that name one of 65,536, and the sum of
    2^-bits was near 0, since the seed codes bytes, not the set's values, and pays for its framing.
    A list of the values a file uses, said once (`palette`, built after), took off only 3 to 13 bits
    on 4 KiB of two or four values, and nothing on 16 bytes: the adaptive coder learns the values
    within a few bytes. (Claude had said it would bring 98 bits towards 16; measured, it does not.)
    The cost on small files is the framing: the length, the transform, the width, and the choice
    before every step. Next: literal runs given one length rather than a choice each.
  - Counting the cards: each kind's arrangements, named by their rank in the counted order, cost
    on held-out halves 5.55 bits a byte for text (2-byte arrangements), 5.51 for code, 4.69 for
    pixels (4-byte), against 8 as they are; on compressed files 14.38 (worse, as expected). Across
    kinds the shuffle travels only between alike ones: text's on code 8.27, but pixels' on text
    19.78. A shuffle a kind, chosen by the file, as Edward put it: "generalise types of data
    towards specific routes".
- *Mixing the whole of mozilla (the same day):* mix-v4 on the whole tar came to 13,778,604, 3.0%
  over LZMA2's 13,376,180 (docs/ODDITIES.md): a longer-reaching copy model in mixing is to be
  tried, its reach and the group's size searchable, at a combinatorial cost (Edward).
- *Pictures, from Edward's benchmarks (2 November 2026):* on his folders of JPEGs and PNGs,
  decode/no (Prioritise Decoding Speed, without Try every way) is the best choice for decoding
  speed, and decode/try and size/no came to the same size as decode/no most if not all of the time,
  taking longer. A packer improvement to weigh, needing no new version: where every file of a run is
  carried by a recipe whose inner bytes are already coded (`jbr`, a picture re-saved), the extra
  searches are skipped, as they can change nothing. To be confirmed against his tables first.
- *A viewer for Sieve's pictures (Edward, the same day):* a `.sieve` image viewer, with zoom,
  next and previous, export as, and the like; perhaps its own extension for `.sieve` files holding
  only pictures (`.simage`, as `.sfilter` names filters; `.isieve` too until 3 November 2026, when
  Edward settled on the one), so that local use can be tried
  as the web extension's was (whose decoding was borderline instant). Reading any `.sieve` raises
  what a renamed file could hold: gigabytes as a pure rotation, a zip bomb. Against it, before
  anything large is decoded:
  - the head and the listing first (the light reader, above): each file's name, its declared size
    and its recipe, which says what it is (`jbr` a JPEG; `sk`, `skr`, `stb`, `pngr` a PNG); a file
    whose recipe is not a picture's, or whose size is over the viewer's limit, refused there;
  - then the start of the stream decoded and the picture's header checked (Edward's suggestion),
    and its width and height against a limit;
  - installing already stops at the declared size and checks every file's SHA-256, so a file
    cannot grow past what the listing says.
  - *It opens ordinary pictures too (Edward, 2 November 2026):* PNG, JPEG and the other usual
    extensions, with a "Sieve" button that makes one into Sieve's picture form, Prioritise Decoding
    Speed chosen, so that what it makes opens as quickly as the picture did.
- *Optimise Search, a box in the File Locator (Edward, 2 November 2026):* "a checkbox ... ticked by
  default called 'Optimise Search'. This could disable the other checkboxes and guide any search
  towards the paths we predetermine to give the best results for data types. Then if the checkbox
  is turned off it would open up the other options again."
  - *What it would set:* the options that shape the search (Try every way, the search's time,
    temporary's recipes), chosen by each file's kind from what the benchmarks measured: a JPEG
    through brunsli with no extra search; text and code mixed; no rope search over bytes LZMA2
    cannot shrink when decoding speed is chosen (Edward's JPEGs: decode/try made the same bytes as
    decode/no, 5,134,123, in 48.2 s against 2.6 s); Try every way where it was measured to pay
    (mixing past its limits: the same JPEG with size/try 4,993,512, installing in 5.1 s, not 0.1 s).
  - *Left to the user even with it ticked (Claude's suggestion):* Prioritise Decoding Speed (the aim,
    size or quick installs, which the routes then follow), the permanent box (it changes what is
    installed), and Keep a log.
  - *What it rests on:* the record of what carried each result, gathered across the benchmarks into
    a table of routes by kind of file, kept up to date as results come in, so the box's choices are
    measured ones.
- *Stage 1 again, with skip-and-insert, columns and palette (2 November 2026;
  `results/shortcut-seed-pieces-2026-11-02.md`):* palette was the best transform on every text and
  code file, 0.4% to 2.0% under the first pass (Don Quixote 12,262 to 12,019, README 4,291 to 4,245,
  pack.cpp 10,492 to 10,379), so where the values are few and the file long enough, saying them once
  pays (on 4 KiB of two values it did not: the coder had learned them as soon). Text and code are
  now within 1% to 2.5% of xz, mixing still ahead. Skip-and-insert took part in three recipes, none
  the best; columns was never the best. Neither fits these files, as expected: each shape is for
  the layouts it describes, and is kept for them.
- *The prototype's speed (2 November 2026):* finding a 32 KiB file's candidates took 144 s (a table a
  distance, thousands of them), then ran on without end on the designed checker (every distance of
  its zeros carried, and skip-and-insert matching a plain run at every place); now each match is
  measured directly and carried one shorter to the next place, and a skip-and-insert whose byte put
  in is the count's own next value is left to `run`: 1.2 s for the candidates, the checker's whole
  search 31.7 s, every recipe still decoded and checked.
- *Site maps that keep themselves up to date (Edward, 2 November 2026):* "a web extension fork
  system for dynamic and automatically updateable site maps? Think YouTube: hashing everything all
  the time wouldn't be practical, unless perhaps just the hash map for a user's uploads is updated
  when uploading a video. Though this doesn't account for comments and much more." Notes for when it
  is taken up:
  - *The shape:* a site's map as a tree (as Sieve's maps hold one, `sieve-map-v1`'s nodes and edges), a node a part of the site, each
    user's uploads a node of its own, updated only when they upload, so a change rehashes one node
    and the path from it to the root, not the site (a Merkle tree, as Git keeps its trees).
  - *What changes all the time:* comments, counts and recommendations change faster than any map
    should; they would sit outside the hashed tree, fetched live, or be hashed in pages that close
    (a day's comments, a closed thread) and are then fixed.
  - *Who keeps it:* the site itself, publishing its own map (the cheapest, since it knows when
    anything changes); or users' extensions, each hashing what it fetched and sharing it, which
    needs agreement on what was seen (two users see different pages of the same address) and trust
    in who hashed it.
- *Measured, 2 November 2026:*
  - *The geometric filters (`results/shortcut-geometry-2026-11-02.md`, `tools/shortcut_designs.py`):*
    zigzag, diagonals, spiral, rings, the Morton curve, 8 by 8 tiles, a plane and a slope from the
    centre taken away. Each designed grid of 64 by 64 is cancelled by its own filter: the count
    along a spiral 11 bytes (xz 2,652), along the Morton curve 11 (xz 297), the snake 11 (xz 265),
    the diagonal stripes 13 (xz 42). On the real files (two pictures, Sieve's program, the teapot)
    none came under no filter at all: on the program the spiral made it nearly twice the size.
    Edward's chaos, measured: an order that suits one layout scatters another; the search keeps a
    filter only where it wins, so a real file pays only the bit more that names the transform.
  - *The coder (`results/shortcut-coder-2026-11-02.md`):* the choice counted by the step before, a
    second recent distance, and the literals by the two bytes before (learned from the literals
    alone) each took 0.3% to 1.2% off (Sieve's program 8,214 to 8,115, Don Quixote 12,000 to 11,855);
    learning the literals from every byte was worse than from the literals alone (the bytes copies
    give are not like the literals); the matched literal, counted by whole bytes, did nothing. Text is
    now within about 1% to 1.5% of xz; Sieve's program 18% over and the teapot 9% over, where xz's
    codes for distances and lengths and its matched literal, bit by bit, are what is left to try.
  - *What carried each result:* `tools/bench/bench_files.py` now writes a "Carried by" column (and
    each report line whole in results.tsv); `tools/sieve_read.py` reads it back from a `.sieve`
    alone (light: head and listing, 0.19 s for docs/'s; full: installed, each file packed alone).
    Silesia's mozilla taken apart, read so: 510 files by LZMA2 (13,181,157 bytes), 5 mixed alone
    (282,420), 10 from another's lines, and the tar's skeleton mixed as a picture.
- *The power benchmark, built (`tools/bench/bench_power.py`, `tools/bench_power.bat`, 2 November
  2026).* Measured: each way's bytes, and the processor time (user and system, every thread) to make
  and to install it, Sieve's own start-up taken off (measured on a 1-byte file) and the other ways
  writing what they install too; joules by the processor's counter where Linux lets it be read.
  Assumed, as settings: moving a gigabyte 0.006 to 0.06 kWh (Aslan et al. 2018's 0.06 for 2015,
  falling by about half every two years), the user's processor 35 W, the maker's 65 W. First run here
  (a container, no counter; three of the repository's files), against 7z, a fetch:
  - Vitruvian's JPEG, Sieve either way: 37,184 bytes less, saving the networks 0.00022 to 0.0022
    Wh, and moving 0.0008 Wh onto the user (brunsli and the JPEG made again, 0.08 cpu s);
  - the PNG, Sieve for decoding speed: 43,987 less, 0.00026 to 0.0026 Wh saved, 0.0008 moved; for
    size (mixing): 52,288 less, 0.00031 to 0.0031 saved, 0.024 moved (2.4 cpu s);
  - so on pictures fetched often, decoding speed about breaks even a fetch and pays at the higher
    estimate of the networks' energy; size first, by mixing, moves more onto users than it saves
    the networks at any count of fetches, and suits what is stored long and installed seldom. The
    answer turns within the assumed range, so the counter on a real machine and a network figure
    agreed on are what decide it.
- *Measured, 2 November 2026, later:*
  - *Mixing's long reach (docs/ODDITIES.md; `results/mixing-match-ways-2026-11-02.md`):* the
    match model already reached the whole file; what it lacked was going on past one wrong byte (as
    LZMA's last distance does), shorter matches and a second candidate. Built as mix-v4's R1, M4 and
    C2 for the v33 draft (`--v33-draft`, the first piece behind it), weighed on each file's start:
    Silesia's mozilla mixed whole 13,318,505 bytes, under LZMA2's 13,376,180 for the first time;
    programs 3.4% to 3.5% smaller; text and pictures choose none or C2. Sieve, sieve-install and the
    oracle read it; CI step 11 checks it.
  - *Gliders (`tools/shortcut_gliders.py`):* two gliders flying apart through zeros are found
    exactly by a search that adds one glider at a time, at every place and direction (the grid 15
    bytes, the seed alone 47, xz 71); two that meet leave a still-life, so what they made is not the
    sum of their trails, one right glider alone makes the file dearer (748 bits against 526), and
    only the pair cancels it (38 bits), which a search one at a time cannot find and a search of
    pairs (about 270 million boards here) is Edward's combinatorial explosion. On the first 4 KiB of
    the grey picture and of Sieve's program no glider paid.
- *Edward's JPEGs, no box (1 November 2026, his machine, 153 files, 452,571,666 bytes; 7z
  450,438,090):* size/try 436,004,003 (−3.20% against 7z, mixing on 143 files, 441.7 s to install
  them all), size/no 448,587,039 (−0.41%), decode/try and decode/no 448,777,954 (−0.37%, 9.4 s to
  install), decode/try taking 1.24 h for decode/no's 0.08 h: the case for Optimise Search. On 106 of
  them the rope led for decoding speed (each cut into pieces, its metadata by LZMA2 and its picture
  by the rope). Every file installed the same. The temporary box (brunsli) is still to be run.
- *Ways in full (Edward: "list the exact formulae used behind each end result"):* the benchmark's
  tables now carry each run's way as its listing writes it, read back from the `.sieve`
  (`stb 8 -1 1280 720 3 2764800 mix4 B22 I1280x3`: made again by stb_image's PNG recipe, its pixels
  mixed as a picture 1280 wide of 3 bytes), so a result can be made again from its line alone.
- *Optimise Search, built (2 November 2026; the File Locator's box O, ticked unless unticked;
  `--optimise-search`):* Try every way chosen for each file by its kind, from what was measured: off
  for every file with Prioritise Decoding Speed; else on for a JPEG, a file over 16 MiB or under 4
  KiB, off between. Here, each case at the smaller size and the quicker time of the box on and off:
  Vitruvian's JPEG for size 203,790 bytes in 10.6 s (the box off: 206,946 in 1.0 s; on: 203,790 in
  10.7 s); for decoding speed 206,946 in 0.2 s (on: the same bytes in 9.2 s); README for size 3,411 in
  0.4 s (on: 0.8 s). Only JPEGs have data enough yet for a rule of their own; the rest follow sizes,
  and each kind gains a rule as its measurements come in (the record of what carried each result).
- *The next version's draft, made one number (2 November 2026):* see NEXT-VERSION.md.
- *Game of Life:* its findings and what is still to try are gathered in [GAME-OF-LIFE.md](GAME-OF-LIFE.md).

**19.112 Ideas for the next pass (gathered the night of 2 to 3 November 2026; none built; built since: programs mixed through the x86 filter, v33's `X`).** From
the day's measurements and review, roughly in the order they would pay:
- *Programs and the 3% test.* With the v33 draft's match ways, programs mix about 1% under xz (2 MiB
  of mozilla's 2.4% under v32's mixing), but a file is mixed whole only where its start is 3% under
  LZMA2, so between 4 KiB and 16 MiB a program stays LZMA2 unless Try every way is on. To measure:
  programs of 1 to 16 MiB (x86, Alpha, game executables) by LZMA2 with the x86 filter against mixed
  with the ways; if mixing wins on most, Optimise Search's rule for programs under the draft, or the
  test's bar lowered for them.
- *Mixing whole over 16 MiB.* mozilla mixed whole installs in 96 s against LZMA2's 3 s for 0.43%.
  For size that is the choice asked for, but a bar that grows with the install's time (a larger
  gain asked of a slower decode) would keep near ties fast; the power benchmark's figures decide
  where it sits.
  *Built (6 November 2026; results/rising-bar-2026-11-06.md):* over 16 MiB, mixing is kept only
  where it is at least 1% under LZMA2, and 0.5% more for each 100 MB (1.25% at mozilla's 51 MB,
  1.5% at 100 MB, 6% at 1 GB), unless Try every way was asked for itself. Of samba, nci, webster,
  mozilla and enwik8 with Optimise Search, only mozilla changes: 13,318,582 bytes mixed to
  13,376,239 as LZMA2 (0.43% more), installing in 1.0 s instead of 49.5 s; the others are the same
  bytes as before.
  The same day the bar was made to apply against whatever is best before mixing, not only LZMA2:
  a FLAC (52 MB), which LZMA2 cannot shrink, had been mixed for 0.556% (65.9 s to install); now the
  rope's pieces, 0.412% (1.3 s). The five above are unchanged.
- *The stream weighing's cost on large folders.* It estimates the shared LZMA2 stream twice (with and
  without the mixed files); on a folder of hundreds of MB that is minutes. Where the mixed files are
  a small share of the stream (under 1%), or their codes already far under their bytes' share, it
  could be skipped; or estimated on the stretch of the stream around them.
- *A bitwise matched literal (the shortcut seed, and maybe mixing).* Counted as whole bytes it did
  nothing (a table for each byte a copy would give, 32 KiB too little to fill them); bit by bit, as
  LZMA codes it, what one byte teaches carries to like ones. The shortcut seed's program and teapot
  are 9% to 18% behind xz, and this is the largest piece of xz's coder the seed lacks.
- *Counting the cards as a transform.* Each kind's 2-byte arrangements named by rank cost 5.5 bits a
  byte on held-out text against 8 (§19.111); a transform that renames bytes by a kind's ranks (the
  ranks carried, or a fixed table a kind) in front of the shortcuts or of mixing, measured per kind.
- *Optimise Search's other kinds.* JPEGs have their rule from 153 files; PNGs, text, programs and
  archives wait on Edward's runs with the boxes ticked ("Carried by" and "Ways in full" give the
  routes). Each rule written with the figures it rests on, as the JPEG one is.
- *Gliders and Life (GAME-OF-LIFE.md).* Glider guns (a few numbers naming many trails), other
  Life-like rules, a search guided by where the file's bytes change most; tried first on files made
  by cellular automata.
- *The hallway.* The picture viewer (`.simage`, its checks before anything is decoded), the power
  page on the site once figures exist, and the File Locator's starting with `--search-every-wave`
  unticking Optimise Search for the run, which a setting saved during it writes to the ini (minor).
- *Taken up the same day (2 November 2026, Edward: "start with the programs and the 3% test, then try
  to work through the rest"):*
  - *Programs:* measured on 16 programs of 98 KB to 11.8 MB (`results/programs-mixed-2026-11-02.md`):
    every one smaller mixed with the match ways, the x86 ones 3.0% to 15.1% under LZMA2 with the
    filter once the x86 filter goes before the mixing, the Alpha ones 2.9% to 4.3% under LZMA2. The
    start test would have turned away 5 of the 16. Built (v33 draft): mix-v4's `X`, the x86 filter
    before mixing, and a program the x86 stream would carry weighed mixed whole through it, with no
    start test, then weighed again against the x86 stream (SPECIFICATIONS, v33). Left: programs the
    packer does not know for programs (Alpha's COFF, ARM's ELF, which `X` would not fit), still
    under the start test; and the packer's smaller tables for a small program (t32.exe 98 KB:
    39,804 bytes at B18 against LZMA2's 39,438, 38,248 at B22).
  - *The stream weighing's cost:* the stream with the mixed files is no smaller than the stream
    without them, so where their codes are no more than 64 bytes and 0.5% of all they cannot come
    back, and the stream with them is not estimated (the same files come back either way; where
    their codes are a small share of the stream, the stream without them is estimated first, alone,
    to see). It halves the weighing's processor time where it applies, not its wait.
  - *The bitwise matched literal:* built in the shortcut seed (`CODER["matched"] = "bits"`, now its
    default), with order 0 too: 0.9% off the program, 0.6% off the teapot, 0.3% off Don Quixote,
    pack.cpp the same (results/shortcut-coder-2026-11-02.md). Still 16.8% behind xz on the program.
  - *The picture viewer:* `sieve-view` (client/view_main.cpp; MANUAL, Commands), with the checks
    above before anything is decoded, and its Sieve button making NAME.simage.
  - *A bar for mixing over 16 MiB:* built 6 November 2026 (above, "Mixing whole over 16 MiB").

*Measured and left (6 November 2026, the easy wins' list):* mixing an Alpha ECOFF program whole
without the start test, as an x86 one is. On the five mozilla libraries: three are already mixed
(libgkcontent 1,289,253 bytes, libxpcom 348,104, libmailnews 252,465: mixing now takes them as
pictures 4 bytes wide, their instructions, which passes the start test), the same bytes with or
without it; libnecko, mixed whole without the test, still went to LZMA2 (326,211 either way); and
libgklayout, which the test turns away, mixed whole to 1,036,101 bytes, under LZMA2's estimate but
over the stream LZMA2 then made (1,022,018), so 0.6% larger. The test is right where it says no,
and nothing is built.
*Built (6 November 2026):* mixing's tables at least B20 for a file of 32 KiB or more, where v33 is
made (mix4_for in pack.cpp). Measured first with `sieve mix` on ten files of 39 KB to 235 KB
(programs, a source, JPEGs, a screenshot's pixels, a film, a text): B20 smaller on every one, 0.05%
to 3.3% (t32.exe 42,737 bytes to 41,470), 0.03 to 0.04 s more a file coded and as much decoded;
B22 0.2% to 4.1% for about 0.15 s more and 268 MB a file to decode, so B20. In the packer, where a
file of that size is mixed alone: sieve_cli.cpp 30,967 bytes to 30,754; the other files measured,
and docs/, the same bytes (not mixed alone, or of another size), their times within the noise.

**19.113 From Edward's enwik9 bench and the enwik8 power bench (2 November 2026).**
- *enwik9, four ways (no search limit, 12 logical processors):* size/try mixed it, 185,105,231 bytes
  (12.59% under 7z's 211,776,158), installing in 1,007 s; size/no, decode/try and decode/no all came
  to LZMA2's 211,195,696 (0.27% under 7z), installing in about 10.5 s. Each took 4,753 to 5,269 s to
  make. Of decode/no's 4,759 s, the rope's search took 3,123 (66%) and LZMA2's tuned passes about
  1,600, for a file the rope did not carry: the first thing for the weighing order's review (§19.114).
- *enwik8, the power bench (assumed settings: 0.006 to 0.06 kWh a GB moved, 35 W the user, 65 W the
  maker):* Sieve 24,768,406 bytes (63,181 under 7z's 24,831,587) for 786 processor seconds of making
  (7z: 65) and 1.625 of installing (7z: 1.109; both write what they install, Sieve's also checks its
  SHA-256 and reads its instructions' head). On those settings a fetch saves the networks 3.79e-4 to 3.79e-3 Wh and moves 5.01e-3
  Wh onto its user, so it never pays for itself. Without Try every way enwik8 (over 16 MiB) is not
  mixed; mixed, it is 21,547,893 (Proof/STATISTICS.md), 13% under 7z, but installs as slowly as
  mixing decodes: the power page needs both rows. Written up with what follows from it in
  docs/POWER-USAGE.md (Edward: "a POWER-USAGE.md specifically for these results and future results").
- *Light versions of every bench (Edward, the same day; an idea until more results agree):* each
  bench script (bench_files, the two Compressed formats ones, bench_power, bench_all) with a light
  form that skips size/no and decode/try, which came to the same bytes as size/try and decode/no on
  153 JPEGs and on enwik9. Built so far: bench_files.py --ways and tools/bench_light.bat.
- *Filter designs cross-referenced for compression (Edward, the same day; after the review):* the
  lines' filters set aside what is not content (FILTER-PLUGINS: not-a-pattern-v1's repeats and ramps,
  max-run, the statistics filters), which is what a compressor can name in a few bytes. Each filter
  read for whether what it rules out is a shortcut the packer could take; not-a-pattern-v1's repeats
  and ramps are the first so taken (`runs`, v33 draft).
- *Operators as mortar (Edward, the same day: "Block A + Operator + Block B ... Every brick needs
  mortar"; measured, not built).* A code file's most frequent operator taken out, its blocks joined
  again on installing. The operator's places must still be carried, by a separator or each block's
  length, and the operator is already the cheapest marker of them: compressors predict it well.
  pack.cpp's 386 `&&` (bytes kept / a one-byte separator / blocks with their lengths): raw
  284,652 / 284,266 / 284,487; xz 61,316 / 61,305 / 61,703; mixing 50,339 / 50,338 / 50,927. Every
  operator used 8 times or more as one byte each: 0.1% to 0.26% smaller under xz and mixing on
  seven source files, nothing on the oracle. A place is free only where a block's boundary is fixed
  (the pieces' 4 KiB), at most one operator a boundary, and moving boundaries onto operators costs
  their lengths. What pays instead is mortar with no places to keep: a repeating byte, pattern or
  ramp, whose count is all there is (`runs`).
- *Code cut at its operators instead of fixed blocks, each block packed (Edward, the same day: "it
  might not be worth it but it is definitely worth measuring and noting"; measured, not built).*
  Three source files cut at every operator (`->`, `::`, `&&`, `==`, `<<` and 16 more; 3,829 in
  pack.cpp), bytes:

  | File | Whole (xz / mixing) | Each block packed alone (xz) | Blocks gathered by the operator after them, and the operators' order (xz / mixing) |
  | :--- | :--- | ---: | :--- |
  | pack.cpp | 61,316 / 50,339 | 219,189 | 79,489 / 68,087 |
  | locate.cpp | 29,896 / 24,569 | 122,105 | 40,603 / 34,604 |
  | test_core.cpp | 67,884 / 56,853 | 275,629 | 86,960 / 75,288 |

  Code's context runs across its operators (the identifiers before an `==` say what comes after
  it, a line's start says what ends it), so cutting there takes from each block what predicts it:
  alone, 3.6 to 4.1 times the whole; gathered by kind (as XML packers gather tags apart from text),
  28% to 41% larger. The whole file in order is the cheapest of the three by far.

**19.114 The lines' filters read for compression (Edward, 2 November 2026: "cross referencing filter
designs for use in compression"; read through, the first measured).** What each filter sets aside
as not content, against what the packer already does: most are already exploited (not-a-pattern-v1's
repeats and ramps by `runs`; max-run, the word lists, letter pairs and n-grams by LZMA2 and mixing's
orders and word list; symbol entropy by any entropy coder; a picture's row runs and neighbours by
mixing's triangle and the rope's waves; sound's steps by the rope's waves) or of no use (not-a-file,
not-packed and figure set aside what real files never are; canonical meshes and models keep one
order, where a file's own order must be kept). New, ranked by reach and gain (estimates, but the
first):
1. *Binary written as text* (not-written-v1's readings: base64, hex, decimal): decoded, checked by
   encoding it again exactly, and carried as what it is, so a JPEG in a data: URI reaches brunsli and
   text in base64 reaches the word list. Measured with the packer as it is, Compressed formats
   temporary, bytes as written against decoded: the Vitruvian JPEG as base64 in an HTML page
   203,542 / 170,340 (-16%); pack.cpp as base64 390,196 bytes 76,813 / 50,358 (-34%); a 101,888-byte
   program as a hex dump 48,330 / 41,679 (-14%). Common in mail, HTML, CSS, SVG, JSON, XML, PEM, Intel
   HEX and C byte arrays. A text with such spans could be carried as an archive is (v31's skeleton
   and members): the text with the spans taken out, and each span decoded as a member with its own
   way, the recipe saying how each was written (alphabet, line width, padding). **Built for base64 (2 November
   2026, the v33 draft's `b64`, spans-v1):** pack.cpp as base64 76,813 → 50,323 bytes, the JPEG in
   HTML 203,542 → 170,423 (each within 83 bytes of the decoded file packed alone); in the
   repository's own texts, three spans found (two alphabet strings and a 392-character key), none
   taken by the weighing. **Hex and lists of byte values too (the same evening, the recipe renamed
   `spn`):** a 101,888-byte program as plain hex, a C array, spaced hex and a decimal list, 46,682
   to 52,065 bytes at v32, 41,571 to 41,639 so (the program itself 41,667); the repository's test
   vectors 263,921 → 259,086, its icon's pixels as a C array 5,395 → 4,267.
2. *Palette* (palette-size): raw 24- or 32-bit pixels of 256 colours or fewer as indices before
   mixing or the rope (screenshots, sprites, uncompressed BMP/TGA/TIFF); a few per cent over mixing,
   to measure. **Measured and built (2 November 2026, the v33 draft's mix-v4 `P`):** the
   hallway's 17 screenshots 185,088 bytes at v32, 146,999 mixed as their palettes (−20.6%, every
   one smaller; results/palette-2026-11-02.md).
3. *Scattered substitutions* (near-model, near): a file as another with k bytes changed, coded by the
   combinatorial number system NearRanker already counts with (weights.cpp); delta-v1 spends several
   bytes a change and starts at 8 MiB. Save files, patched data, pages of a database.
4. *Meshes* (not-other-line's .obj, the model filters): vertices predicted from their neighbours,
   connectivity coded apart; large gains, rare data.
5. *Code points* (utf8-valid): contexts by character, not byte, for text not in Latin letters.
6. *Grammar and capitals* (moby-grammar, sentence-shape): a part-of-speech context in mixing, 1% to 3%
   on English at a guess.
7. *Melodies* (key, melody-*): a MIDI model; tiny files, negligible.
*Where 2 to 7 stand (12 November 2026):* 2 built (mix-v4's `P`); 3 measured on text pairs (338
bytes for 10 changes against an ideal of 29) and, through it, the x86 filter restarted per program
(v34 draft, results/x86-restart-2026-11-11.md); 4 to 7 are each a model in mixing, so a mix version
of their own, left for the compression research of the long plan's stage 6.
The filters' exact counters as coders: NearRanker (3 above) is the one that would serve; the Mobius
counts add nothing over `runs`, the palette and composition rankers nothing over mixing and rANS.

**19.115 Files routed through the decoder to other programs (Edward, 3 November 2026: "In future
perhaps we can route filetypes through the decoder to other applications that can be listed by data
type or extension in an .ini").** Not built. Today a `.sieve` is installed into a folder and the
viewer shows only pictures. The idea: opening a `.sieve` (or a `.simage` and its kin) decodes it as
now, checked against its SHA-256, and hands each file to the program named for its kind in an
`.ini`, as `[open]` lines by extension or by data type (`.pdf = C:\...\reader.exe`, `text =
notepad.exe`, `picture = sieve-view`), so a `.sieve` opens the way its files would. Questions for
when it is taken up: where the decoded files live while the other program has them (a folder of
the user's, removed after, as the viewer's is); a `.sieve` of many files (each opened, or a list to
choose from); what the data type is where the extension is missing or wrong (the File Locator's
kind detection already names pictures, sound, text and programs); and the same refusals the viewer
makes before decoding (the listing read first, sizes against memory), since any program it hands
to sees what the file claims to be.


**19.116 Video: frames carried as deltas of frames (Edward, 3 November 2026: "I wonder if mpeg /
video / audio encoders could be used to track down deltas across frames or sequences as well, or
through ffmpeg, search time could be quite taxing though. Imagine how many empty frames or black
transition fades there are in movies, depending on the source material and editing it could become
a delta, same for mostly still frames").** Not built; nothing measured yet. Notes for when it is
taken up:
- **A film already compressed has done this.** MPEG-2, H.264, H.265 and AV1 code most frames as
  differences from others: a black fade or a still shot already costs a few bytes a frame (skipped
  blocks). Sieve must give back the file's exact bytes, so decoding a film with ffmpeg and coding
  its frames again does not help: no encoder makes the same stream twice across versions or
  settings, and the frames alone are not the file.
- **Where the idea does apply as it stands:**
  - *Video not yet compressed, or barely* (raw YUV, frame sequences as pictures, lossless or
    intra-only captures and edits, screen recordings): there a frame is near its last, and Sieve
    has the pieces: `copy K` for identical frames, delta-v1 and mixing's match model for near ones,
    the picture contexts with the frame's width. A video model (each pixel predicted from the same
    place one frame back as well as its neighbours) would be mixing's picture model with one more
    neighbour, a new mix-v4 parameter.
  - *The same footage in several files* (a trailer and its film, two cuts, a game's films sharing
    intros and fades, Edward's WoWRevived): where they were encoded at once, their packets are
    byte for byte the same. ffmpeg can list a file's packets without decoding a frame (`ffprobe
    -show_packets`, or demuxing with `-c copy`), which is quick, and identical packets found by
    hash could be carried once, the container made again around them as archives are now (a
    skeleton and its members).
- **The step beyond:** lossless recompression of the coded stream itself, as brunsli does for JPEG
  (the entropy coding, CABAC in H.264, decoded to its symbols and coded better, then made again
  exactly). Large work for each codec; others have done it for JPEG and MP3, little for video.
- **To measure first (cheap):** the packets of WoWRevived's films hashed, to count how many bytes are
  packets seen before in the same or another film. *Tool and a first measurement (12 November
  2026, results/video-packets-2026-11-12.md):* `tools/bench/video_packets.py`; on a film and a
  trailer cut from it, 90% of the trailer is packets seen before. It also found a gap in the
  packer: Sieve's newest was 10.5% larger than its own plain installer on that pair, the film's
  frames carried by the rope in a stream apart, so what the trailer shares with it was lost; a
  whole-set plain weighing before writing would close it (no new version; for Edward to decide).
  *Built the same day (Edward: "yes build the whole-set plain weighing fix"):* the pair now 3,449,600
  bytes, level with 7-Zip; sets with no shared content keep the newest. That says whether the packet route is worth
  building, before any encoder is involved.
  *Measured (7 November 2026), WoWRevived English v1.1.2's eight MP4s:* every packet hashed with
  `ffprobe -show_packets -show_data_hash sha256`, no frame decoded. 43,969 packets, 617,099,672
  bytes; 8,673 bytes in packets seen before in the same film and 1,128 in another (RAGELOGO's), so
  9,801 bytes in all, 0.0016%. The Human and Martian INTROs share no packet: each film was encoded
  on its own, so no stream is a copy of another's. Verdict: the packet route is not worth building
  for these films; it would pay only for footage cut from one encode (trailers and films, chapters
  split from one file), which nothing here is.

**19.117 Steering the rope's locked bytes (Edward, 3 November 2026; nothing built or measured; measured since, `results/rope-wheels-2026-11-03.md`, and Manual Navigation built 4 November).**
Edward: "Part of the way I also envisioned the rope filter is that the 1/3 and 2/3 bytes on their
own fluctuate their values before trying noise wave gradients and other methods. What I am thinking
is, what if we generate / walk the state space of 16 bytes, which is 256^16 [...] But perhaps we can
learn to steer from the locked bytes by adjusting their value. Where we know everything is a forward
walk, technically we know exactly the ramp/direction to get where we are going. And what we are
looking for is a step along that road that puts the file at a completely different rotation but is
closer to an exact integer interval rotation. [...] Perhaps imagine a car made of just four bytes,
where each byte is a wheel. If we learn to steer from 4 bytes first, maybe we can then try 8 bytes
and 16 bytes etc."
- *What it adds to §19.110.* Real-rope's locks fluctuate; this asks how to find good values without
  walking all of them. With k free bytes there are 256^k settings (4 bytes: 4.3 billion; 16:
  3.4 × 10^38), so walking them is out past 3 or 4. Steering is a search that follows the
  landscape: turn one wheel a step, see whether the result got smaller, keep turning the way that
  helps (hill climbing, or a gradient where the size can be read as a smooth function of the bytes).
- *What steering can and cannot buy.* It makes finding a good setting cheap; it does not change what
  a setting is worth. Whatever setting is found is written in the `.sieve` (k bytes), and the most
  any choice among 256^k settings can save is those same k bytes (§19.110's counting). So for a
  file the rope cannot describe, steering finds nothing worth its bytes, however well it steers.
  Where the rope already describes a file well (pictures, sound, tables of digits), there are
  settings that beat the default by far more than their own bytes, and steering is a way to reach
  them in seconds rather than by walking.
- *The question that decides it: is the landscape smooth?* Steering works where turning a wheel by
  one changes the size by a little and in a direction that stays the same for a while (a valley to
  roll down). It fails where neighbouring values give unrelated sizes (a hash's landscape): then
  steering is no better than trying settings at random. The rope's prediction is arithmetic on the
  bytes (sums, differences, a wave), so some smoothness is plausible, but it has to be measured.
- *A first measurement (the car of one wheel, then two).* On files the rope wins (the Vitruvian
  picture's pixels, the synthetic sound) and loses (a program): take the kept byte at the third,
  set it to each of its 256 values, and code the file each time; then the third and the two-thirds
  together over a coarse grid (16 by 16, then finer where it dips). Plot size against value. Smooth,
  with one or few valleys: steering works, and 4, 8, 16 wheels follow, each turned in turn. Jagged:
  the wheels are not a car, and the search space is better spent on other rules. This needs
  real-rope's kept bytes to be free values (today they are the file's own), so a throwaway switch
  first, as §19.110's measurements used.
- *How it would be seen.* The landscape of one wheel is a line graph (size against 0 to 255); of two,
  a height map, which the hallway could show as terrain the car drives over: low ground is a
  smaller file. Steering is the car rolling downhill.
- *Measured (3 November 2026, results/rope-wheels-2026-11-03.md, `tools/measure_rope_wheels.py`).*
  The rope at P0 with its kept bytes free, on a picture, a sound, x-ray, mr, text and a program.
  **The landscape is smooth on pictures and sound** (one wheel: 1 and 7 dips in 256 values, each
  step like the last at 0.99 and 0.98) and noise on a program (25 dips, 0.08). **Steering beats
  guessing** with as many tries on every file, by 1.6% to 44%. **But the valleys lead nowhere:** at
  best the steered line costs what the bytes cost coded plainly (the picture 51,531 against 51,544
  with 64 wheels), where the rope with its neighbours makes 44,729. The line's wheels are the wrong
  car; wheels that touch every value's prediction (the neighbours' weights, a bias for each region)
  are the next to measure, with the same script.
- *The second car, measured (4 November 2026, the same results file, `tools/measure_prediction_wheels.py`).*
  Each value predicted as a weighted sum of three neighbours plus a bias, every weight a wheel, then
  a bias for each 4,096 values. Steering turned the weights to the rope's own fixed predictions (the
  value to the left on the sound and x-ray, the gradient on mr) and found no better one: 72 bytes on
  the sound (0.05%) at most, and on the picture a valley above the triangle (45,422 against 44,887).
  The region biases stayed at 0. On a program and text it found no prediction best. Edward: "The
  ropes bytes are indeed the wrong thing to steer. Instead all bytes would be steerable during the
  manual search": steering is kept for Manual Navigation, where every byte is a wheel.
- *Manual Navigation (agreed with Edward, 3 November 2026; built 4 November: `client/manual_nav.cpp`,
  docs/MANUAL.md).* A screen of its own from
  the main menu, "Manual Navigation", on the binary line to begin with (other dimensions later, as a
  `<Dimension>` value to turn). It loads any file as it is, with no sieving; a `.sieve` is loaded to
  steer one.
  - *Left:* the bytes as a sheet, 2 to 16 a row (Edward, 4 November 2026: "2 then increments of 1 up
    to 16"); a file whole, scrolling. With no file, an example to learn on: a square of random bytes,
    its side the width, made afresh when it changes where FRESH RANDOM BYTES is ticked. The byte chosen
    has the navigator's arrows, or hex typed in.
  - *Right:* a circle. The dot's angle is the file's rotation on its line, and its distance from the
    centre is how far that rotation lies from the nearest bearing at the Angle Precision. The centre
    is kept for an exact hit. The first dot is in the line's colour (its primary and secondary
    state-space colours); each change leaves a mark, green where it moved in towards the centre and
    red where it moved out.
  - *Top, under the title:* the rotation and the length of its route, the edits' cost so far
    ("edits: N bytes") and the net (the route saved less the edits).
  - *Steps:* the file is held once; each step is kept as offset, old byte and new byte. Undo and redo
    go back through every step (Edward, 4 November 2026: "unlimited undo by default"), or 10, 100
    or 1,000 as set. Keep a log (on by default) writes every step as it lands, with its bearing
    before and after, to a CSV whose head names the file by its whole path and SHA-256, so Load Log
    opens it again and replays every step, undos and redos too ("replayable without having to
    pinpoint the file again"); Save Steps writes just the walk. The steps are the walk back to the
    file.
  - *Steering (built 4 November 2026):* the curve Edward saw, a 4-byte example's first byte turned
    52 times, is a beat: one turn moves the file 14.0076 gaps between bearings at one decimal place,
    so the dot creeps 0.0076 of a gap a turn, a slow Archimedean spiral in towards the nearest
    bearing and out again (two more turns, F6, came 0.0034 from landing). Each byte is a gear of its
    own (the second 0.0547 of a gap a turn, the third 0.0002), and each precision another; base 256
    against base 10 never meets evenly, so the same kinds of spiral and zigzag recur at every scale.
    The screen now says, for the byte chosen, what one turn moves and which value lands nearest.
  - *The exact check (built 4 November 2026; Edward: "calculate the rest of the decimal places and
    determine if a shorter address has actually been found or not", and the fraction shown, "red if
    it is odd and green if it is even"):* the rotation 360 v / units in its lowest terms, from a
    greatest common divisor (Lehmer's, added to BigUint). Whether its decimals end, after how many
    places, or how long their repeat is, and what it costs written exactly against the file's own
    address. The loop of every file of 0 to n bytes holds an odd number of them, so the denominator
    is always odd (red), and the decimals end only where it is a power of 5: two bytes 54 AA land
    on 120 degrees exactly, 360 x 21931 / 65793 = 120 / 1, two bytes written either way. On random
    bytes the fraction is about twice the file (a 4-byte file: 9 bytes against 4, the decimals
    repeating every 30,840 digits). The divisor is the slow part, quadratic in the length: units
    times 255 is 2^(8(n+1)) - 1, which splits into four numbers sharing no factor, the longest half
    the length, so it is found a quarter at a time. Measured here: 0.5 s for 100,000 bytes, 6.5 s
    for 416,970 (pi6.txt.sieve's length), 45.8 s for 1 MiB; Python's own (math.gcd, without the
    split) took 89.2 s for 1 MiB. A faster divisor (half-gcd, near-linear) is the next step if
    longer files are wanted.
  - *A sphere and a cube grid (Edward, 4 November 2026; noted, nothing built):* the bytes as a
    cube (width by width by width) instead of a flat sheet, and the dot on a sphere instead of a
    circle. The address is one number, so a sphere needs a rule for its third coordinate: the
    distance from a bearing at the next precision would show two precisions at once. A cube suits
    data with three dimensions of its own (voxels, the models line), where a byte's neighbours are
    its neighbours in space. It adds a view, not information; the hallway's renderer could draw it.
  - *Skip tables and seeded noise aimed at roundings (Edward, 4 November 2026; measured, nothing
    built):* a byte to choose a lookup table and a byte for one of its 256 entries, each entry a
    fraction to skip by; then the entries as noise from seeds (a table of 64-bit seeds, say),
    subtracted from the rotation so that what is left lands on a clear rounding, the address being
    the rounding and the seed.
    - *Skips to repeating patterns:* a file that is one block of L bytes repeated sits at a
      fraction with 256^L - 1 below the line, so such a table is easy to make. On the binary line a
      landing point helps only while the file matches it from its first byte. Measured on 500
      pieces of 4 KiB from each of ten Silesia files, jumping to a file repeating a block of up to
      64 bytes, the block and 2 bytes paid for: on average 0.2 bytes saved on dickens, 17.5 on
      samba, 51.6 on mozilla, 77.9 on mr, 7.7 on nci and 11.2 on xml, almost all from blank pieces;
      LZMA2 saves 2,055 to 3,520 on the same pieces, using repeats wherever they are.
    - *Seeded noise:* seeds tried in turn (splitmix64) until the file less the noise lands exactly
      on a bearing, every seed paid for by its chance of being the one (the reader must know where
      the seed's number ends). 200 random files each: 2 bytes at one place, 17.14 bits against the
      file's 16.01; 3 bytes at one place 25.38, at two places 25.64, against 24.01. The noise
      constrained to the file's length (Edward): 25.28 and 25.20 bits wrapping round the loop,
      25.45 and 25.48 for noise a byte shorter, 28.06 and 28.48 where it may not wrap (most seeds
      then cannot be used). The seed has to say which of the tries between two bearings landed,
      so it carries what the bearing does not, whatever the noise's length. A table of 64-bit
      seeds covers at most 8 bytes; on a longer file an approximate landing leaves the gap to be
      written. The noise that pays is the noise the reader makes from what it has unpacked
      (§19.91): the rope's prediction, and mixing's.
    - *A file split into two seeds (his thought, like the factors of a product):* two seeds
      together are as long as the file they make, as a product's two factors are as long as the
      product, so finding them, however hard, saves nothing. Noise waves added together stay
      noise unless they are made alike; real data has patterns or nothing, compressed data is
      noise, and a compressed file is found from noise only by a seed as long as itself.
  - *The length as its own value (discussed 4 November 2026; nothing to add):* Edward asked whether
    a fifth value, the length, should join settings, dimension, rotation and item. It is in the
    dimension already (a line's shape: on the binary line, how many bytes), and the rotation cannot
    be read without it. A loop of files of exactly n bytes (256^n, a power of 2) would make every
    rotation's decimals end, but then the rotation is the file's bytes as a base-256 fraction, no
    shorter. Sieve run again on its own output came out larger (degrees.sieve 7,686 to 7,749,
    samba.sieve 3,277,403 to 3,277,467, site.sieve 19,422 to 19,483), as 7z of a .sieve does in
    Edward's logs: a hand search of a .sieve on this screen is open to try, and the exact check is
    what would recognise a find.
  - *On the binary line it is clockwork:* each byte is a hand of a clock (the first moves the dot
    1/256 of a turn, the next 1/65,536), so which way each wheel steers is known. On a line with
    filters, the clock is scrambled, and the landscape above is what is being explored.

**19.118 The rope's probe on Edward's map at v33 (3 November 2026).** MAPS.WoW (160,890,897 bytes)
at v33 took 2,684.8 s to read: the rope's search on the whole file took about 1,020 s, and the
weighing repeated it (987 s), to 127,663,746 bytes against mixing's 52,897,658 and LZMA2's
60,130,073. The probe (2 November, results/rope-probe-2026-11-02.md) did not leave the search out: the rope's newer ways
made the first 256 KiB 105,459 bytes (`L32 W1 S1 P2 K0`), under the 1.1 line, while the whole file
came to 2.12 times LZMA2. On 2 November the first 16 MiB of the map came to 1.75 at their start. To see what the
probe sees across the file: `python tools/rope_probe.py SIEVE MAPS.WoW` (the rope against LZMA2 on
256 KiB at the start, a quarter, the middle and three quarters). If the other places say no where
the start says yes, asking a second place before a file of 64 MiB or more is searched whole would
save most of that time.

**19.119 The next training version: QK-norm, and a floor under every probability (Edward, 4 November
2026; measured, nothing built).** `sieve ai-train --limits` (results/training-limits-2026-11-04.md)
found two things a training version after `training-v2` could change; each would be a new version
beside the old, so every run recorded so far trains as it did.
- *QK-norm.* A model of 4 layers and width 64 trained without a loss scale held 1.62% of its
  activations at ±128, every one an attention score (a query against a key), none up to step 200
  of 469 and 16,551,930 by the end; it learnt least of the four loss scales tried (the right byte
  given 15.58%, against 17.99% to 19.33%). Float training meets attention scores that grow without
  bound in large models, and normalises the queries and keys before they meet (QK-norm) to stop it.
  Here it would be the same norm the layers use (§12.0e), on each head's q and k before the rotary
  turn, perhaps with a learnt scale. Worth measuring on the same runs: whether the scores then stay
  inside, and whether the unscaled run learns as the scaled ones do.
- *A floor under every probability.* Probabilities are Q16, so one under half of 1/65,536 is 0: in
  the shipped run 44% of the bytes' probabilities at any one prediction are 0, and the right byte was
  one of them in 316 of 632,490 predictions (0.05%); in the larger model, about 1 in 80. Edward: "no
  one can ever say with absolute certainty that something is 0% or 100% unlikely to follow something
  else ... the word 'waffle' might be very unlikely to follow the word 'quantised' but that doesn't
  stop me typing quantised waffle." Training is not harmed (the right byte's gradient is its full
  -1), but a model used to compress (mixing, or a model on the AI line packing text) must give every
  byte at least 1/65,536, as an arithmetic coder does. The softmax would give each byte
  `1 + (e_j (2^16 - 256)) div Σe`, so no byte is 0 and they still add to at most 2^16; the gradient
  then follows that softmax. *Built 9 November 2026 as `training-v3`* (Edward: "the AI line's min/max
  ... caps, enforced during training"), the default for new runs: on the shipped recipe and a larger
  model, no byte given 0 (from 316 and 1,606 times) and none given all, learning and time the same
  (results/training-floor-2026-11-09.md). *QK-norm measured 12 November 2026*
  (results/qk-norm-2026-11-12.md): on the unscaled run no score was held (from 17.5 million), but
  the loss scale every new run is given already holds none and learnt more (15.99% of the right
  byte, against 15.82% with QK-norm too). Not built; to be measured again on a much larger model.

**19.120 After the v33 release: what the pre-release review left (4 November 2026; found and
measured, nothing built).** Six reviews ran before the release (the changes since the freeze, the
reader against hostile files, duplicated code, speed, outdated documents, what to do first). What
was fixed then is in the commit of that day; these were left, each larger or riskier than a release
should take on:
- *Speed, packing:* (a) a single file's LZMA2 first pass (preset 9e) runs alone before its mixing
  is weighed, though nothing reads it until mixing's last comparison: made on a thread of its own
  and taken when needed, dickens 49.4 s to 36.5 s in a prototype, every `.sieve` the same bytes; it
  wants FileWays to hold the figure still to come, and weigh.cpp to wait for it. (*Built, 6 November
  2026:* for programs and for every file the rope is not searched for, a text it cannot win or a
  file its probe turns away, the figure made beside the mixing (FileWays::lzma2_later); dickens
  34.9 s to 23.6 s with mixing's trials at once, the same bytes.) (b) The start
  test's cache keyed on "over 4 MiB", so a file over 4 MiB mixes its first 256 KiB three ways
  twice; keyed without it, two files with the same first 256 KiB would share the 4 MiB decision,
  valid but not the bytes made before, so it waits for a format's turn. (c) Archives' and spans'
  weighings run file by file; per file on the threads, a folder of pages would gain about the
  cores' number on that step (estimated). (*Built in part, 7 November 2026:* what each file's weighing
  there reads that is its own alone, LZMA2's estimate of it, its spans with their four figures and a PNG's makers' results,
  worked out on the threads a batch ahead of the loop, which still decides file by file in order: the
  same bytes (Edward's `Pages/`, `docs/` with its step 4.0 s to 1.6 s, the CI's spans folders); a
  zip's or a 7z's opening and weighing still one at a time.)
- *Speed, installing:* every file's SHA-256 and the vault's chunk scan run on one core (about 4 s a
  GB measured on their parts); on the threads, reporting the first failure in listing order as
  decode_deferred does. (*Built, 6 November 2026:* on up to 4 threads; the six GIFs' check and
  write 1.41 s to 0.66 s with the vault's frames at once, docs/ 0.26 s to 0.08 s; the same files,
  and the same messages for a damaged file or one already there.)
- *Names on Windows and macOS:* case is folded for A to Z only, so É.txt and é.txt (or a name in
  its NFC and NFD forms on macOS) are written one over the other there, the install saying it
  succeeded; Windows' reserved names (CON, NUL, COM1, aux.txt) and names ending in a dot or a space
  are not refused. Unicode simple case folding with NFC, or refusing such names, in the packer's
  warning and the installer's refusal alike. (*Built:* the reserved names on 5 November (0.13.78); and on 6
  November every letter Unicode gives a case folded as Windows folds it, one to one
  (tools/cli/case_fold_table.hpp, made by tools/gen_case_fold.py from Unicode 15.1: 1,455 pairs;
  1,423 after the review the same day, each letter to its uppercase as NTFS compares them, which keeps
  apart 32 pairs the lowercase of the uppercase had joined, such as the Kelvin sign and K),
  so É.txt and é.txt, Σοφία and σοφία, are one name to the packer's warning and the installer's
  refusal, while straße and STRASSE stay two, as on Windows. And the same evening, a letter written
  as one character and as a letter and its combining accents (NFC and NFD): one name on macOS alone
  (tools/cli/normalise_table.hpp, made by tools/gen_normalise.py: 2,061 decompositions, Hangul by
  rule, 922 combining classes); the packer says so as macOS's, the installer refuses it on macOS,
  and Windows and Linux keep the two.)
- *A RAR made again* sets each member's mode before the RAR is checked; the modes are now masked
  to 0777, but the members could be held to the files this install wrote, and the modes set only
  once the RAR matches. (*Built, 7 November 2026:* each member must be a file or folder this
  install wrote, or the RAR is not made again and says which; the modes rar must read are set
  before it runs, as they must be, and put back as written where the RAR does not match.)
- *Duplicated code worth one copy each:* varint readers (most accept a tenth byte's high bits
  silently; one shared, canonical reader), the eight text-vault decoders written twice (written.cpp
  and vault_decode.cpp, which already differ on UTF-8), u8/from_u8 (37 copies), reading a file (15),
  hex (12), trim/lower/floor division, removing an archive's one top folder (4), the program's
  folder (3), the Llama config parser (2, one lacking three checks), and `--script`'s runner in
  app_main.cpp (4). The oracle stays apart from all of them, as it must. (*Built in part, 7 November 2026:*
  Sieve's own numbers read by one reader, `sieve/varint.hpp`, in their shortest form only (a tenth
  byte past bit 63, or a number written longer than it is, refused), in the rope stream, the items
  stream, the head, the runs' segments, delta, spans and rope-wave, with sieve-weights-v1's own
  reader made as strict; RAR's and xz's numbers keep their formats' readers. The vault's decoders are
  not-written-v1's (`written_readings`), ascii85 its own. The Llama config read once
  (`llama_shape`) for the tensors' layout and the model, every size bounded: the layout's copy had no
  bound, so a config.json claiming 10^18 layers was listed until the memory ran out; the model's
  further refusals (rope scaling, an activation other than silu) stay the model's, as the layout does
  not depend on them. A path's UTF-8 and back is one copy, `sieve/paths.hpp` (29 local ones gone,
  picture_source.hpp's names now the same functions); a whole file read where it throws "cannot open
  '…'" is `sieve/whole_file.hpp` (3 gone); bytes as lower-case hex `sieve/hex.hpp` (3 gone). Left: the
  other readers, whose messages (seen by the user) or failures differ (some return nothing, one trims
  the end), hex that is upper-case, "0" for nothing or one byte, `utf8_of`/`utf8_path` under other
  names, and trim/lower, where the copies differ (CR LF, ASCII only) and a shared `lower` in `sieve`
  would meet filekind's own; the top folder, the program's folder and `--script`'s runner not yet
  looked at.)

**19.121 Four ideas after the release build (Edward, 5 November 2026; evaluated, nothing built).**
- **An installer program's own default folder.** Today `sieve-install` offers `Documents\` and the
  folder's name (`where_in`, from `SDL_GetUserFolder`), and the person can change it before
  Install. The idea: when an installer is made (`sieve locate --program`, the File Locator), the
  maker names the folder it should offer, sanitised, and the program offers that. *Where it goes:*
  not in the instructions, whose bytes are their address and so cannot change without changing
  what they are, but beside them in the program, as the dictionaries now are (SPECIFICATIONS §12.2's
  `sieve-carried-d1`): a third small block, read only by `sieve-install`. A `.sieve` handed over on
  its own would offer `Documents\` as now, unless a later manifest version carries it. *Sanitising*
  matters more than it looks, as the program comes from someone else: the folder written relative
  to a known place (Documents, Desktop, the person's own programs folder, `%LOCALAPPDATA%\Programs`
  on Windows), never an absolute path or a drive root; no `..`, no names Windows reserves (CON,
  NUL, COM1 and the rest, §19.120), no name ending in a dot or space; and never a system folder
  (Windows, Program Files, which also needs administrator rights) or another program's. The folder
  is still only offered: it is shown, and Install still asks. *Verdict:* small and worthwhile; the
  block is the work, the rules the care.
- **The viewer's icon on a `.simage`'s thumbnail.** Explorer marks the thumbnail of a file whose
  kind has a program of its own with that program's icon, in its bottom right corner. Windows reads
  a value of the file kind's class for this, `TypeOverlay`: set to an empty string under
  `Sieve.Picture`, no icon is drawn (written from what Windows documents; not yet tried here). One
  more `reg add` in `install_context.bat`, taken away again by `uninstall_context.bat` with the
  class. *Verdict:* a line each; worth trying first on Edward's machine, as it is a matter of
  taste (the icon says which program opens it; photos show none).
- **More archive and compression formats in the File Locator's readouts, behind a box.** It
  compares a file or folder with zip (deflate, level 9) and 7z (LZMA2, preset 9e) only. Sieve
  already links what more would take: gzip and zlib, bzip2, xz, zstd, lz4 and brotli (each used to
  make archives again exactly), and libarchive. Each could be a row at its strongest setting, each
  file alone and the folder as one stream where the format does that (zstd's and brotli's long
  windows; tar then gzip, bzip2 or xz, as people make them). *Time is the cost Edward names:*
  brotli at quality 11 and zstd at level 22 are among the slowest compressors there are, so the
  rows go behind a box ("Compare with more formats", unticked), each row shown as it finishes,
  and each stopped within a MiB when asked (`kNotMeasured`, as the two rows are now). *Verdict:*
  worthwhile and plain; the figures it gives are what people ask first ("better than zstd?"), and
  measured on their own files.
- **AI models as filters: the state space a model can make.** A model that makes something from an
  input (a diffusion model from a prompt and its noise, a text model from a prompt, greedily or
  from a seed) is a function, and its outputs are a set: on the binary line, the items it can make
  from every input it takes. Kept as a filter, it keeps only those, as the lines keep what they
  can name, and it can be narrowed through windows (a text's length, a picture's size, a video's
  frames, one prompt's family). *What holds:* (a) it is a filter in the Sieve sense, and the
  counting is the same as for every other (§1): a model can make at most as many outputs as it
  takes inputs, so naming an output by its input is shorter only where the input is shorter than
  the output, and almost every file is in no such set. (b) Where the input is known, it is the
  strongest name there is: a picture made from (model, prompt, seed, steps, sampler) is those
  values, a few hundred bytes for megabytes, checked by making it again and comparing SHA-256s.
  That is §19.2's "the noise is an address", from the other side. (c) It needs the model to make
  the same bytes everywhere: floating point on graphics cards does not, from one card or driver to
  the next, so a model kept as a filter must run in pinned integer arithmetic, as Sieve's own
  training does (training-v1), or the name names nothing. *What does not:* finding the input for
  an output (inverting the model) is a search with no known short cut; the inversions there are
  for diffusion give a near noise, not the exact one, and a near input names a different file. So
  it works forward (made by Sieve, or by someone who kept the recipe), not on files found in the
  wild. *Its soft form is already in Sieve:* mixing is a model used as a filter that keeps every
  file, the ones the model expects cheap and the rest dear; the hard filter is that model's
  probability floor taken to zero, which §19.119 argued against for compression ("quantised
  waffle": nothing can be ruled out). *Verdict:* worth having as a recipe way for things made by a
  model Sieve can run exactly (the AI line's integer models first, §19.2's small diffusion model
  next), and as a view in the hallway (the line filtered to what one model can make); not as a way
  to find files in general.
- **Making them deterministic: restricted temperatures, pre-seeded (Edward, the same evening).**
  A model's output is random in two places, and the idea settles the first. (a) *The choice:*
  sampling at a temperature draws from the model's probabilities; with the temperature one of a
  few fixed values the recipe can name exactly (0, greedy, taking the likeliest each time; or a
  small fraction such as 1/2), and the draws from a pseudo-random generator seeded from the recipe,
  the same inputs make the same choices. (b) *The arithmetic under it:* the choices are made by
  comparing the probabilities with each other or with the draw, and a probability that differs in
  its last bit between two machines flips a near tie, after which the two outputs part for good.
  Greedy is not spared: two likeliest bytes within a rounding of each other are a tie. So the
  temperature and the seed make it deterministic only on top of a model whose probabilities are
  the same bits everywhere: integer weights and integer arithmetic (training-v1), the softmax in
  integers with §19.119's floor, the draw an integer compared with integer cumulative counts, as
  mixing's coder already is. With both, the filter is exact on any machine.
  *What this opens, for text:* a seeded draw from integer cumulative counts is exactly what an
  arithmetic decoder does with its input bits. Take the seed not as a fixed 64-bit number but as
  the decoder's whole input, and the model makes a given text from the bits an arithmetic coder
  writes for that text under the same model: the way back that §19.121 found missing is there,
  and it is compression with the model, which mixing does now. A fixed-width seed reaches only as
  many texts as it has values (2^64 for 64 bits) and finding one is a search; a seed as long as
  it needs reaches every text, likely ones with short seeds. So for text the filter and Sieve's
  mixing are one thing seen from two sides; what the restricted temperature adds is a recipe a
  person can read (this model, this prompt, greedy) for the texts it makes. *For pictures by
  diffusion* the steps are not a choice among counted symbols but noise added and taken away, so
  (a) and (b) give an exact maker, while the way back stays a search.
  *The seeds from a state space (Edward):* any line could give them, as every line orders the same
  byte strings, each its own way; the binary line is the right one, and for a reason that can be
  stated. A seed as long as it needs is the coder's output, and a good coder's output is as near
  to random as it can make it (were it not, it could be coded smaller). For bytes with no pattern
  no ordering puts them earlier than another does on average, and the binary line's address of a
  string is the string itself (its hex plus the 0101... mark, §1), so it adds nothing to undo;
  another line would make some seeds shorter only by making as many others longer. So the model
  is a jump from the binary line to the line of what it makes (§19.2's J): the seed is the point
  on the binary line, the output the point it lands on, and the short seeds, early on the line,
  land on what the model expects most.
- **Without integer arithmetic: near the same (Edward).** A model in floating point on another card
  makes nearly the same output: a picture a person could not tell apart, a text that parts from
  the other only after a near tie. That is enough wherever the output is only looked at: the
  hallway's view of what a model makes, a preview, a thumbnail, a model's babble on the shelves.
  It is not enough for a name, and not for a file made again: near is another file, with another
  SHA-256. Nor can a correction stored beside it (the output's differences from the file, as the
  delta way carries one file against another) rescue it, since the installer would apply the
  differences to its own, slightly other, output. So: float where a person looks, integers where
  Sieve names or makes a file again.

**19.122 A much larger, capable model (Edward, 5 November 2026; noted, nothing built).** The models
trained so far are small (§19.119 measured one of 4 layers and width 64): enough to show that
training on the AI line works and is recorded exactly, not enough to write or draw usefully. At
some point Sieve should train a much larger one that is actually capable, for the AI line itself,
for mixing (a better model of text is smaller code for text, §19.121), and for the filters of
§19.121. What it needs first:
- *The training version's two changes (§19.119)*, QK-norm and the floor under every probability:
  both problems grow with the model (the right byte at a probability of 0 was about 1 in 80
  predictions in the larger of the two models measured, against 0.05% in the smaller).
- *A decision on exactness.* Training in integers keeps the training itself reproducible bit for
  bit, which is what lets a run be recorded and checked; a capable model may be far cheaper to
  train in floating point on graphics cards. What names need is exact *use*, not exact training:
  a model trained in float and then fixed in integers (its weights rounded once, and every use
  after in integer arithmetic) is exact wherever it runs from then on. The cost is the run's
  reproducibility; the record would then hold the weights' SHA-256 rather than a run that
  replays.
- *The data*, with its licences recorded as the dictionaries' and the Gutenberg model's are, and
  its size chosen from a measured curve (how the loss falls as the model and data grow, from the
  sizes already trained upward) rather than a guess.
- *Where it runs:* the AI line, mixing and the filters all read it, and an installer would need
  it to make again any file mixed with it, as it now needs the dictionaries (it would be carried,
  or the instructions would name it and refuse without it).

**19.123 GIFs opened: their frames carried as pictures, made again exactly (Edward, 6 November 2026;
noted, nothing built or measured).** Edward's idea: sieve a GIF's frames, and their differences, as
a `.simage` does pictures. *What a GIF does already:* each frame may cover a rectangle of the canvas
only, and leave transparent the pixels that do not change, so an optimised GIF (gifsicle -O3, say)
is in part its frames' differences already. But every frame is coded by LZW, far weaker than LZMA2
or mixing, and Sieve today carries a GIF as its bytes (FORMATS.md, the pictures' table). *How:*
- *Opened, under temporary (and so under Optimise Search):* each frame's LZW undone into its
  palette indices; the GIF's skeleton (its headers, palettes, rectangles, delays, extensions)
  carried apart; the indices packed as any file is.
- *Made again exactly:* LZW's output depends on the encoder (when it clears its table, how its
  code size grows). As preflate does for deflate, the common encoders' ways are tried and a recipe
  kept only where one gives the GIF back byte for byte; else it is carried as it is, so nothing is
  larger. This is the risk, and the work.
- *The differences:* the frames' indices carried one after another, so an unchanging background
  repeats and LZMA2 takes it as matches; better, mixing given the frame before as context (the true
  delta), later, as it is the larger work.
- *The viewer:* with the frames carried as pictures, `sieve-view` could play a GIF (it shows the
  first frame now), and its thumbnail come from the frames already made.
*Where it should pay:* screen recordings and animations over a still background; little on
dithered photographs.
*Measured (6 November 2026, results/gif-opened-2026-11-06.md):* the six demonstration GIFs of
Edward's InscribedSquareProblem repository, all gifenc 1.0.3's (every frame whole, its own palette).
Made again exactly: a compress-style LZW encoder gives back all 346 frames byte for byte, and
gifenc's sub-blocks are 254 bytes every one. What they come to: 9,616,677 bytes as GIFs; 8,377,298
(87%) by Sieve today; **4,718,704 (49%)** opened, their indices and palettes by xz -9e alone (32%
to 60% a file), before the rope, mixing or the frame before as context. So it is worth building,
for gifenc's GIFs first; other encoders (giflib, ImageMagick, gifsicle) to be measured as GIFs of
theirs are found.
*Built (6 November 2026, v34's first piece, `gif`; SPECIFICATIONS §12.2 v34):* the six to
**4,150,626 bytes (43.2%)** mixed, installed in 27 s; 4,652,864 (48.4%) with Prioritise Decoding
Speed, installed in 3 s; every GIF given back. Pillow 12's GIFs open too. Still to do: the frame
before as context (the indices of one frame mean other colours in the next where the palette
changes, so first measured, not assumed); the viewer playing GIFs; making faster (LZMA2's passes
over the indices took most of the 101 to 161 s; with the stream's four passes run at once, 161 s became 96 s, the same bytes).

**19.124 A box for zip and 7z's comparison (Edward, 6 November 2026; built the same day).** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.125 Every compressed format is a possible bomb (Edward, 6 November 2026: "in theory can't any compressed format become a zip bomb for sieve ... I don't know if it can really be avoided that well").** *Done; moved whole to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md) (12 November 2026).*

**19.126 cmix: a source of ideas and a yardstick, not a way (Edward, 7 November 2026: "useful as a
source of ideas and as a yardstick definitely").** Byron Knoll's cmix leads LTCB and Silesia (by
Matt Mahoney's note on encode.su, its first version needed 20 GB of memory and two days each way
for enwik9). Sieve uses none of it. Not taken as a way: its strongest parts (the LSTM among them)
work in floating point, which is not the same bit for bit across compilers and processors, so a
`.sieve` made on one machine might not install on another, where Sieve's mixers are exact in
integers; and context mixing decodes as slowly as it codes, which the rising bar (§19.112) weighs
against. (Its licence, GPLv3, would combine with Sieve's AGPLv3.) Kept for:
- *Ideas,* each made again in integers under a new manifest version and measured first on enwik8,
  samba and the rest, as v33's pieces were: its text preprocessing by dictionary (paq8hp's word
  model), more context models, more mixer and SSE layers.
- *A yardstick:* cmix beside Sieve on a few files (a text of 100 KB, a source, a picture), to show
  where the gap is (enwik8: cmix v21 14,623,723 bytes as quoted in NEXT-VERSION.md, Sieve 21,431,896).
  *Run 7 November 2026* (results/cmix-yardstick-2026-11-07.md; cmix's paq8 at level 9 to fit 15 GB):
  Sieve 2.7% smaller on a raw picture, 15% to 26% larger on text and source, 36% on a program
  (`ls`), 12.8% on control.py; cmix took 84 to 261 s each way and 6 to 12 GB, Sieve under 3 s and
  under 400 MB. The ideas in order: a model of x86 instructions, text by dictionary, more SSE layers.
  *Built since (7 November 2026), the first:* v34's `E1`, the x86 instruction model: eleven programs
  4.0% to 7.9% smaller (results/x86-instructions-2026-11-07.md); `ls` 27% over cmix, from 36%.

**19.127 Rounding a file's place, with a table of common roundings (Edward, 7 November 2026; measured 8 November).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**19.128 Noise run through a file, its matches clipped out (Edward, 8 November 2026; measured the same day).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**19.129 The rope's search on every thread (Edward, 8 November 2026).** His idea: run a rope pass
on each thread, tallying the bytes each wave would leave, and keep the best. The search now tries
one wave after another for a file (files are searched several at once); the waves of one file could
be tried on the threads together, each reading the same bytes and counting its own leftovers. Since
the memory pass (results/making-memory-2026-11-07.md) a wave's estimate holds only its counts, a few
MB, not a copy of the file, so the threads need not hold the file more than once. It would shorten a
large file's search, where one file is searched alone (MAPS.WoW's about 1,020 s); the result would be
the same, as the waves and their order are. Not built.

**19.130 Notes (Edward, 8 November 2026).**
- **The completion estimate on the main menu looks out of place** where it is, between the title and
  the items; it may move (beside the items, under them, or a page of its own).
- **Optimise Search and the three priorities.** Optimise Search chooses, for each file by its kind,
  how hard to search (Try every way or not), from what was measured on each kind: it guides each data
  type to its best path (MANUAL, the File Locator). Size, Balanced and Speed say what "best" is. The
  two are different questions (how much to search; what to keep), so Optimise Search should stay,
  but be guided by the priority asked: a route table per data type and per priority, three paths a
  kind, each measured (Speed: no mixing, so Try every way's mixing part changes nothing; Balanced:
  mixing searched only where its saving could clear Balanced's bar). Today the table was measured for
  size, and Balanced and Speed reuse it with their own limits on what mixing may take. Not built.

**19.131 Trimming what the wave predicts exactly (Edward, 8 November 2026; measured the same day).** *Set aside; moved whole, with its reasons, to [ideas/DEPRECATED.md](ideas/DEPRECATED.md) (12 November 2026).*

**19.132 Wave paths through the grid (Edward, 8 November 2026; measured the same day).** His idea: a
wave of one to n phases is a path that curves through the file laid out as a grid; where a value on
the path is matched it is cut, the rest kept, and decoding puts them back procedurally; it propagates
up to n times, the best chosen through seeds and propagation within limits. To decode, every value
keeps its place in the grid (cut out of the grid itself, every later value would shift column, and in
a stereo WAV left and right would swap), so the cuts come out of the leftovers, and each pass must be
a different wave. *Measured* (results/wave-paths-2026-11-08.md; `tools/bench/wave_paths.py`,
`wave_fitted.py`): Edward's paths (600 shapes) found nothing the waves over every value did not (the
best shape covered every cell; 0.09% on `mr`); seeded paths did worse. Waves in turn (O3, O2, O2X
each cutting what the last missed) took 0.45 to 1.82% off the WAVs, nothing elsewhere. Far larger:
choosing the wave per file (a wave continued from its last three values, 15.39% off Martian/03, but
7.66% more on Martian/04); coding leftovers by bit length (17.31% off `mr`); and, largest, a wave fitted
to each block of 4,096 from its last 8 values (12.88 to 36.19% off the WAVs, within 4.4 to 7.0% of
FLAC). The 5/3 wavelet won only on `x-ray` (15.69%). FLAC level 12 on the seven WAVs the rope carries:
224,783,126 bytes against the rope's 276,730,651 (81.2%). Verdict: no paths or seeds; the rope's audio
gains most from a fitted wave and bit-length coding (FLAC's own methods), which would bring it near
FLAC; a way that codes sound as FLAC does is the option to weigh. Not built.
*Other sound* (the same file): 8 MB each of speech, jazz and a 24-bit orchestra from archive.org,
made by the packer and by FLAC. The rope beats LZMA2 on the music (jazz 17.6%, the orchestra 2.9%)
and loses on speech (2.4%); mixing beat the rope on all; FLAC was 20.0 to 54.5% smaller than the
packer's choice on each. So the rope's sound is not only electronic music, but FLAC is the bar.
*Edward's questions (8 November 2026), for later:* (a) a FLAC way, WAVs carried as FLAC codes them
inside the `.sieve` and installed as the same WAV, every byte (libFLAC is BSD-licensed and can be
fetched as the other libraries are); (b) as an opt-in repack (as permanent repacks PNGs), WAVs
installed as `.flac` files; (c) a film's sound taken apart from its pictures: only worth it where the
sound is plain samples, as most films' sound is already compressed (AAC, MP3, Opus).
*Built (8 November 2026, Edward: "make a start on that, use the three levels as the size/balanced/speed
paths"): (a), as v34 draft's `flac` (results/flac-way-2026-11-08.md).* WoWRevived's seven WAVs:
222,740,143 bytes at Size (made in 203 s), 223,209,005 at Balanced (52 s), 224,634,367 at Decoding
speed (48 s), each installed in about 7 s and given back byte for byte; the rope took 276,730,651.
(b) and AIFF not yet.


**19.133 Notes for the next stretch (Edward, 9 November 2026; noted, nothing built; to discuss
before moving forward).**
- *The ceiling (answered):* training-v3's top probability is 65,281/65,536, not 65,535 or 65,534. It
  is not a rule of its own but what the floor leaves: the other 255 bytes each hold at least 1/65,536,
  so one byte can have at most 65,536 − 255. (Q16's "1" is 65,536, not 65,535.) No measured run ever
  gave one byte all of it, and the cap costs nothing apart from the floor; it only matters in a
  saturated corpus.
- *A larger model,* trained locally by Edward on a corpus of his choosing (§19.122's needs: the
  floor, built; QK-norm; a measured curve for size against data).
- *The training routine reading more kinds of file:* documents converted to text (PDF, HTML, Word,
  e-books), and where it serves, pictures and sound converted to what a model of that kind reads
  (the image line's pixels, the audio line's samples), each conversion recorded in the run so it
  trains again the same.
- *Wider arithmetic:* values are now 64-bit with 16 fractional bits (Q16), held to ±128; weights are
  digits of 2 to 8 bits. A model "expanded or layered" to higher precision: wider fractions (Q32),
  wider ranges, 128-bit sums (the update's product already is), perhaps BigUint where speed allows;
  each a training version of its own, so recorded runs stay exact.
- *A better interface* for training and model parameters (the harness's settings).
- *A real model on the AI state space:* how near an exact integer interval of rotation comes to a
  capable model, and the shortest address that names one: its data length, rotation, item,
  dimension, filter settings, model parameters. A thought to test: a trained model's shortest exact
  description may be its training run (recipe, corpus, order), which Sieve already records and
  replays bit for bit, rather than its weights.
- *Filtering the PCM audio line,* and the filters of every line improved where they can be.
  *12 November 2026:* the sound line's filters (`sound-peak`, `sound-step`, `silence-run`) measured
  on an hour of real speech (FILTER-PLUGINS.md §20: their defaults keep all of it; `sound-step` at
  30% would keep 97.7%); plugins can now be written for it (`symbols pcm*`, one channel at a time,
  the oracle agreeing), `pcm-quiet-ends-v1` added, and the Filter Designer tests and plays them.
- *The main menu:* its readouts at the top are good but crowd it. *Done the same day:* they left
  the main menu; each line's time to walk is now a figure under its column in Start Sieve's map
  ("~10^115y"), with its size and bits, from the same settings (Edward: "just the start sieve menu
  might be best ... it doesn't really need its own dedicated window").

**19.134 Documents kept up to date from the code (Edward, 9 November 2026; noted, for a session of
its own).** "we should look at adjusting documentation to be automatically updated from the code
when building when and where possible". The review of 9 November found 17 places where the
documents no longer said what the code does (the File Locator's O key missing from its key line,
training-v2 named as the default after v3, v20 as the newest version, "Prioritise Decoding Speed"
after it became the Priority button). Two ways, by kind of text:
- *Generated blocks:* lists and tables written by a build step into marked places in the documents,
  and a CI check that fails where a committed document differs from what the build would write
  (the change still committed by hand, so it shows in the diff): the keys of each screen (from
  data/lang/en.txt), the options of `sieve help` and `hallway --help` (tools/check_help.py already
  checks every option is named), the newest frozen version and the draft's ways, the default
  training version, the settings' defaults (app_settings.hpp), and the formats and their routes
  (FORMATS.md), once those are one table in the code that the packer reads too.
- *Checked prose:* the explanations stay written by hand, and the CI looks in them for facts that
  go out of date (a version named as the default, a box that became a button).
- *First generated block built (13 November 2026, ROADMAP 5c A5):* docs/SETTINGS.md, every setting with
  its default and what it is, written by `tools/settings_doc.py` from `hallway --write-settings` (the
  settings' own table), and CI step 10 fails where the committed file differs. Next: each screen's keys
  from data/lang/en.txt, and the newest frozen version and default training version named in prose.

**19.135 Sieve's own voice (Edward, 9 November 2026; begun).** "we should also gather some voice
samples from public domain spaces that can be used to train the new small voice model ... make it a
setting \"Text to Voice\" on / off ... which allows AI to speak out loud when being spoken to through
the dimensional line interface". Built the same day: the **Text to Voice** setting (Settings,
Controls; `[controls] text_to_voice`), speaking the CHAT tab's replies by the system's voice
(Windows) for now; and the voice, an hour of LJ Speech (public domain: one reader, Linda Johnson,
for LibriVox; 547 clips with their transcripts) in `data/anchors/public-domain/voice/lj-speech`, with
`tools/fetch_voice.py` to make its WAVs again from the pinned source, or the shard's three hours.
Edward's choice of the way, in two steps:
- *First, a voice of units:* the clips cut into small sounds by their transcripts (diphones: from
  the middle of one speech sound to the middle of the next), and speech made by joining the units
  a text needs. Intelligible from the start, with no training run, and exact: the same text gives
  the same samples. Needs each transcript's sounds (a pronouncing list, or rules) and where each
  falls in its clip (an alignment), both made once and kept.
- *Then a trained voice:* a small model in the harness's own training, bit for bit, learning sound
  (frames of the samples) from text; a harness run type of its own. It may smooth the units' joins
  before it can speak alone.
- *The voice of units, built the same day* (`tools/cli/voice.hpp`, `sieve speak`; Edward: CMUdict for
  the words, espeak-ng for the alignment): 1,022 diphones, 33,750 takes, from 503 aligned clips. An
  offline recogniser understood 76.7% of the words of 60 sentences in it, against 90.3% in espeak-ng's
  voice (results/voice-units-2026-11-09.md). Text to Voice speaks with it wherever its files are.

**19.136 A core aim: history constrained, and fraud made impossible throughout time (Edward, 12
November 2026).** His words: "a core aim of the system is to constrain human history and make fraud
impossible throughout time".

*Notes.*
- **What the system already gives towards it.**
  - *Every thing has one exact name:* every unit and every file has one address on its line, and
    every record is named by its SHA-256 (manifests, anchors, runs, results, chains, the release).
  - *Every result can be made again:* packing, installing, training, drawing and steering are
    integer arithmetic, so the same inputs give the same bytes on every machine.
  - *So a record cannot be changed quietly:* once its name is known, a changed copy has a
    different one. A training run's whole history (recipe, corpus, order) replays to the same
    model, the thesis of GAME.md §1, so what a model became is checkable rather than claimed.
- **What makes it hold "throughout time": witnesses.** An address or a hash proves nothing alone,
  because whoever changes a file can work out the changed file's name just as cheaply (§0: "a
  bijective address is not a tamper seal"). It holds once the name is fixed somewhere the changer
  cannot reach: published, copied and held by many people, in many places, at a known time. Then a
  record can be shown unchanged since that time, by anyone, with no one to trust. That is why
  links that state hashes, the site's table and the crawler (§4.6, Edward's decision against
  signing keys) matter: they are the witnesses. The more independent copies of the names there
  are, the less any one party can rewrite them.
- **Where the line falls.** This makes a record *tamper-evident*: changing it after it is
  witnessed is caught. It cannot make a record *true*: a false claim, witnessed, is a faithfully
  kept false claim. Fraud of the first kind (altering the past) can be made impossible to hide.
  Fraud of the second kind (lying at the time) is constrained instead: every claim is tied to an
  exact, replayable record, so it can be checked, and checking it costs anyone the same small work.
- **Towards it, from what is planned:**
  - *the source tree across computing* (GAME.md §11): every version of every program named and
    linked, its descent recorded;
  - *the maps of verified anchors* (§4): what is known to be real, by name;
  - *published name lists:* the names of everything a release, a run or a map holds, each list
    itself named and published, so a later list can show what was there before it. A chain of
    those is history fixed in place, a little at a time.
- **All of history, mapped (Edward, 12 November 2026).** His words: "eventually all of the possible
  known and unknown human history will in a sense be mapped". In Sieve's terms it already is, in
  one sense, and becomes so in another:
  - *Every possible record is already on the lines:* every page that could be written, every
    picture of its size and every file has its address, so every account of the past that could
    exist, true or false, known or not yet found, has a place. That is the Library of Babel's
    sense, and it is complete from the start.
  - *What mapping adds is knowing which places are real:* the known part of history is the
    records that have been witnessed (named, published and held, above), marked on the lines as
    anchors; the unknown part is everything between them, already addressed and waiting. Each
    record found, named and witnessed lights one more point, and the filters and the anchors'
    neighbourhoods narrow where the next real ones are likely to lie.
  - *So the map grows from both ends:* the space of every possible history is fixed and exact,
    and the set of points known to be real in it only ever grows, each fixed in place by its
    witnesses, so it cannot later be moved.
- **Where the known, the unknown and the lost cross (Edward, 12 November 2026).** His words: "at a
  point in time that is, will eventually be pinpointed where all that could possibly be known or
  unknown crosses over with what is lost". A third region on the same map:
  - *Lost records are on the lines too:* a record destroyed still had an address, and still has
    it; what is gone is any witness to which address it was. So the lost are places once real,
    now indistinguishable from the unknown, and the map can hold them as regions: "somewhere here
    stood something real" (a book known to have existed, by title and length; a file known by its
    name and size; a page known from quotations of it).
  - *The crossing point:* at any moment, the space divides into what is witnessed (known), what
    could still be found (unknown, not yet lost), and what can no longer be told from noise
    (lost). Each record has a moment past which, with no copy and no witness left, it crosses from
    the second into the third. That moment is what can be pinpointed: by the last witness of it,
    the last copy, the last reference.
  - *What the system does about it:* it moves that point. Witnessing a record early (naming it,
    publishing the name, keeping copies) keeps it out of the lost for good; partial witnesses
    (a title, a size, a hash, a quotation) shrink the region a lost record must lie in, so it may
    one day be recognised again if its bytes turn up; and the exact space bounds even the lost
    exactly: how many places a lost record of a known size could be, and which a filter of what it
    was like would keep.

**19.137 Compatible forever: the project a lens on itself (Edward, 12 November 2026).** His words:
"ultimately anything that was compatible with sieve is currently compatible and will be forever
compatible even if we remove compatibility because github exists and the project is on there and
all its commits can be tracked and built, so it acts like a lens upon itself going backwards in
time". And: "even if and or when we remove the manifest versions for the cleanest possible manifest
version and addressing system, we will maintain compatibility with all addressing systems through
that addressing system". And: "we should aim not to alter it beyond compatibility with previous
versions until it becomes an operating system though, as well as a game engine, both of those are
important points in time that should be worked on in unison".

*Notes.*
- **The history is the reader of last resort.** Every version of the format was written with the
  code that reads it, in the same commit. A file of any version can always be read by checking out
  a commit that read it and building it. That is the lenses of GAME.md §11 turned on Sieve itself:
  the project seen as it was at any commit, run, inspected or unfolded as source.
- **The clean addressing system keeps the old ones inside it.** When the manifest versions are
  folded into one clean version, each earlier version stays a name the new system resolves. Old
  files then keep their addresses, and the new reader can say which commit reads them, or carry
  that reader within it. Removing a version from the current code removes it from the build, not
  from the map.
- **What keeps a commit buildable.**
  - *Pinned dependencies:* CMakeLists.txt pins each fetched library to a commit and checks it
    (`sieve_pinned`).
  - *The oracle:* `reference/sieve_ref.py` reads every version with Python alone.
  - *Tests of every version:* `tests/` and CI step 11 hold an example of each.
  - *Where it is weakest:* dependencies fetched from other hosts, and compilers that change. A
    copy of every pinned source kept with a release (as a `.sieve` of its own, like the voice
    extension) would close that gap; noted, not built.
- **The rule until then.** No change that leaves an earlier version unreadable by the current
  build, until Sieve becomes an operating system and a game engine. Both are milestones, to be
  worked on together, where the format may be made anew around what they need. Each draft version
  (v34 now) is held to the same rule: it adds to what is read, and takes nothing away.

**19.138 Time, slavery and war (Edward, 12 November 2026; an aside, noted as he gave it).** His
words: "if britain made slavery illegal and america only just ( in cosmic terms ) made it illegal,
then time has to catch up with us. quite the big bang." And: "if time travel were possible, the past
and the future would make slavery illegal and thus time travel would become illegal. thus the wars,
the time war (dr who reference) and more, like the infinite ballooning debt caused by both world wars
and the wars throughout time and space across the multiverse."
- *Edward, after:* "was all just a thought as I am a fan of the old dr who from when I was young".
  A thought, kept as one; the story pool of GAME.md §11 is where thoughts like it can live.
- *The dates, for reference:* Britain abolished the slave trade in 1807 and slavery in most of its
  colonies by the Slavery Abolition Act of 1833 (in force from 1834); the United States abolished
  it by the Thirteenth Amendment, ratified in 1865.
- *Where it touches the project:* §19.136's aim (history fixed in place by its witnesses) is the
  same intuition from the other side: a past that cannot be rewritten. Two of the lines are kept as
  candidates in VOICES.md, waiting for its clean-up.

**19.139 Technical to metaphorical, a lens on Edward's notes; and distributed computing, a DNA
dimension, an infinite RAID (Edward, 12 November 2026; noted, nothing built).** His framing:
"overall a way to streamline information and the universe plus ramblings of ideas things can always
go one way or the other but technical stuff is one side and metaphorical stuff is the other side
which can be reused in voices.md, like a lens, technical to metaphorical." So his notes are sorted
the same way from now on: what could be built goes here, with notes; what is said for its own sake
goes to VOICES.md's candidates (waiting for its clean-up), as a line a voice in the corridors could
say. Some notes are both, and are kept in both.

*The technical side.*
- **Distributed computing.** "I was considering the idea of distributed computing ... Initially the
  idea of logs being submitted through PRs but that could be weaponised. Secondly specifically
  looking for good results users find. But I don't know if I like the idea overall. I know that
  generally I would prefer all of humanity used distributed computing systems similar to the p2p
  prime calculator that can utilise your background resources when idle or prompted."
  - *The model he names* is the Great Internet Mersenne Prime Search (GIMPS) and its kin (BOINC,
    Folding@home): idle machines given small, checkable pieces of a large search, each result
    verified before it counts.
  - *Why logs through pull requests could be weaponised:* anyone could submit anything, so results
    would need checking before they count. Sieve's results check themselves: a unit's address and a
    file's hash are verified by anyone in milliseconds, so a "good result" (a survivor of a stack,
    a shorter address, a better model run that replays bit for bit) can be submitted as its
    address and checked rather than trusted, as GIMPS checks a prime by a second run.
  - *What it would search:* filter surveys over many settings, the rope's or a model's search
    spaces, training runs (each replayable). Undecided, as Edward says: noted for when he decides.
- **A DNA dimension.** "dna dimension and the colour of humanity / plot human dna like a bubble - our
  species / rainbow / iridescent / pop". A line of DNA sequences (four symbols, A C G T) holds every
  genome of its length, as the pages line holds every page; humanity's genomes are a cluster in it,
  plotted as a bubble whose colours show how people vary. *A caution for when it is taken up:* a
  person's DNA is among the most private data there is; only public, consented sets (the human
  reference genome, the 1000 Genomes Project's) and summaries of variation, never anyone's own
  genome, belong in it.
- **An infinite RAID.** "random off shoot computer idea / grow the infinite network and learn its
  shape". Storage spread across many machines with redundancy (as RAID spreads it across disks),
  growing as machines join, its shape (who holds what, how far apart) learnt as it grows. Sieve's
  part would be what it already has: every piece named by its hash and checked on arrival, so a
  piece from anyone is as good as from anywhere (§19.136's witnesses are the same copies).
- **"remove write control from the internet protocol".** Noted as said. One reading, in Sieve's
  terms: content named by its hash rather than by where it is (as IPFS and BitTorrent do), so no one
  controls a name's content by controlling a server: the bytes either match the name or they do not.

*The metaphorical side* (kept as candidates in VOICES.md): "physics = computer britain born =
america big = universe = boom"; "cant we all just ration and starve to release the valve and
exchange interface with the universe"; "humans had to grow clothes lol"; "infinite debt from the
wars = humanity still at war" (with §19.138).

**19.140 The tactile Sieve: every colour and note felt, for the disabled throughout time (Edward, 12
November 2026; an aim, and why the interface is simple).** His words: "sieve should be the ultimate
tactile experience to interact with in a way, with all the colours and notes eventually being
transmitted through braille and colour / touch / sensation type screens to help uplift the disabled
throughout time", and "that is a major part of why I have left the interface looking simple so far".
- *What it means for the design now:* the interface stays simple on purpose. Every screen should be
  something that can be carried by other senses without losing what it says: few elements, each
  with a name, a value and a place, and no meaning held by colour or position alone. That is the
  same rule as the colour-blindness pass (2.4) and §19.74's read-aloud, taken to every sense.
- *What Sieve already has towards it:*
  - every string in the language files, so anything shown can be spoken or brailled;
  - menus read aloud, in Sieve's own voice where it is installed (§19.74, §19.135);
  - every unit an exact sequence of symbols (letters, notes, pixels as colour numbers), so any unit
    can be rendered to any sense exactly, not described: a melody as notes, a picture as rows of
    colour values, a page as text.
- *Towards braille and touch:*
  - *Text to braille:* the pages line's text and every menu string through a braille translation
    (the open-source liblouis tables are the usual way) to a refreshable braille display, which
    screen readers already drive. The read-aloud hook is where it would attach.
  - *Colour and pictures by touch:* a picture's rows read as a grid of values, a cell at a time, as
    tone or as braille cells; on haptic screens, colour mapped to texture or vibration.
  - *Notes by touch:* a melody's pitches and lengths as vibration patterns, beside its sound.
  - *Settings:* text size, contrast, colour-blind palettes, holding a key instead of repeating it
    (§19.74's list), as each sense is added.
- *Workable now, in order:* (the colour-blindness pass is done, 2.4, and colour cannot carry identity as lines grow, §19.143) read-aloud for the hallway's
  panels (the part §19.74 left); a "describe this unit" key that speaks or brailles the unit in hand
  exactly (its letters, its notes, its rows of colours); braille output through the system's screen
  reader. *Later:* haptic and sensation screens as the hardware comes.

**19.141 The Game of Life over noise, through time: a temporary, temporal and causal variant (Edward,
12 November 2026).** His words: "even though the game of life filters seemed unworkable they can be
used as temporary and temporal or causal variants introduced into noise imagery as gradients through
time to distort the noise through the game of life which is akin to shaking up a noise map I
suppose". What was measured (GAME-OF-LIFE.md, §19.111) was Life as a way to *shorten a file*, where
it lost; this is Life as a way to *move through* pictures, which is a different job and does not meet
the counting argument, since it names nothing.
- *Temporary:* a view, not a way in the format. A noise picture (any unit of the pictures line, or
  a noise map in the shortcut seed's tools) is taken as Life's first board, by a threshold on its
  values or per bit plane, and run forward a generation at a time; nothing is written to a `.sieve`.
- *Temporal, as gradients through time:* each generation is a frame, so the run is a short film in
  which the noise is shaken up and settles (most random boards thin out into still lifes, blinkers
  and gliders). A gradient through time: the rule changed slowly between frames (Conway's B3/S23
  towards another Life-like rule and back), or Life applied to a growing share of the picture, so
  the distortion sweeps across it.
- *Causal, and countable:* in such a film every frame follows from the one before, so the first
  frame names the whole film. On the movies line, a filter keeping only films whose each frame is
  the Life step of the last (`life-causal-v1`, say) would count exactly: of all films of n frames,
  only those of one free first frame are kept, a share of 2^(−(n−1)·W·H) for a W by H board of one
  bit a cell. It is a judge and a counter at once, and the films it keeps can be walked by their
  first frame alone. The same holds for any fixed rule, and the rule could be a setting.
- *Undoing:* Life cannot be run backwards (many boards come to the same next board), so a frame
  cannot give its past; the film is kept by its first frame and the rule, never by its last.
- *Workable now, in order:* (1) a key in the hallway that steps the picture in hand through Life,
  each generation shown as the picture-line unit it is (so every frame has its address); (2) the
  same as a film on the movies line, with the rule gradient as a setting; (3) `life-causal-v1` on
  the movies line, counted as above. Nothing here is measured yet.
- *What it was all to say (Edward, the same day):* "that was all to say that there are infinite
  shorter ways to create strange variants and noise patterns or seeds to turn everything into a
  dimensional state space across time that represents all information across all games or forms of
  media and representations of space such as simulations in all manner of ways, forms, styles,
  rendering engines or otherwise". In Sieve's terms: every generator (Life, a seed, a noise map, a
  rule gradient, a rendering engine) is a short way to *reach* a family of units, and each family is
  another axis of the state space, so the ways to walk it have no end. What a short seed cannot do is
  name an arbitrary file more briefly than the file (§0, §19.27): it reaches what it generates. The
  two sit together: generators give infinite short routes *into* the space and through time; exact
  addresses and hashes pin down where the real things are in it (§19.142).

**19.142 Sieve as a lock in time (Edward, 12 November 2026).** His words: "the idea is that the sieve
acts like a lock in time in order to preserve and anchor history to something permanent and
immutable such as video games coupled with compression and data or information theory, chaos theory
and infinitely more theories, yet in reality all of them (theories)". This gathers §19.136 (history
constrained, fraud made impossible) and §19.137 (compatible forever) into one aim.
- *The lock:* what is already permanent in Sieve. A file named by its SHA-256 cannot be changed
  without its name changing; a `.sieve` rebuilds its files byte for byte and checks them; every
  format version stays readable (§19.137). A record locked this way can be lost, but not quietly
  altered.
- *Anchored to something permanent, such as video games:* released games are fixed artefacts with
  many independent copies (discs, archives, players' installs), so their hashes are witnesses that
  no one holds alone (§19.136); the games line and the hallway of engines (GAME.md §11) are where
  history meets them.
- *The theories, all of them:* each is a lens on the same addresses. Compression and information
  theory say what can be named and how briefly (the counting argument is the floor under all of
  it); chaos theory and generators (§19.111, §19.141) give the short routes through it; hashing
  and exact arithmetic give the lock. Sieve's rule stays: each is used where it is measured to work.

**19.143 Colour at infinite dimensions; the span as a scroll wheel; views of the spaces from other
angles (Edward, 12 November 2026).** His words: "I thought we did the colour blindness pass and found
two dimensions colours slightly clashed on the sieve setup menu, I didn't know what colour to change
them to while similarly preserving its identity, we also have new dimensions, theoretically infinite
to add in to the mix so everything will inevitably become a shade or something like that, which
means there is nothing we can do about colour blindness to begin with, this also means the dimension
span from binary to binary eventually becomes so big that it becomes a left to right scroll wheel
interface", and "it could also interface with a series of visualisations the view the spaces from
different angles such as the toroidal nature implied by the circles of the minimap as well as
through its cross dimensional filtering etc".
- *The pass was done* (2.4, 6 November 2026, `tools/colour_check.py`): tracks and movies, 58.9
  apart normally and 2.4 with deuteranopia; every other pair 14.2 or more. Edward kept the colours,
  not knowing what to change them to without losing each line's identity. The triage's "not
  wanted" for 2.4 means that; the plans that called it still to do were wrong and are corrected.
- *Why colour cannot carry it:* with lines added without end, every colour becomes a shade of
  another, for everyone, colour-blind or not: any palette runs out. So a line's identity must never
  rest on colour alone. It already does not: every line is named wherever it is shown (the HUD, the
  setup menu's columns, the door signs). Colour stays as a help, chosen by `colour_check.py` as far
  as it can go; the name, the place in the order, and later a pattern, a sound (each line's music
  mode) and a touch (§19.140) carry the rest.
- *The span becomes a scroll wheel:* the setup menu draws binary, every line, and binary again as
  columns across the screen (pitch = the width over the lines plus one, with a minimum). With 11
  lines it fits; as lines grow it will not, and the menu becomes a strip scrolled left and right by
  the wheel or the arrows, with the focused line centred and its name and figures always shown. The
  corridor already works this way (left and right through the doors, binary wrapping round). To
  build when the lines outgrow the screen; the minimum pitch is where it starts.
- *Views of the spaces from other angles:* the same strip as one of a series of views. The minimap's
  circles already imply a torus (every line loops, §5.2, §5.8); a toroidal view would draw each line
  as a ring and the doors as the links between rings. A cross-dimensional view would draw how the
  lines filter each other (§12's lines as filters against others, `not-a-file-v1`), and the node
  graph is a third. Each is a lens on the same addresses, chosen like a camera, and each keeps the
  tactile rule: a name, a value and a place for everything shown.

**19.144 An emotion dimension, shortcut dimensions as highways, and doors through doors (Edward,
12 November 2026).** His words: "I suppose there would also be an emotion dimension, that might be a
really simple dimension to help map everything from, as it would only contain X number of doors or
objects or words which are emotions, note that down as an idea to help mapping the state spaces,
tying everything to emotion and thus a simple shortwave route interface provides a link between
everything via the emotion dimension and other such highway dimension interfaces." "These shortcut
dimensions would be capable of being linked upstream to things like hashtags and words and more in
order to help make everything preserved, an immutable chain through history between ai and human
work, thus preserving what was truly ai made over what was truly human made forever." "I don't know
what an emotion dimension would look like, in theory that is small enough to just become doors and in
theory all dimensions could be represented by doors as doors through doors and across the
dimensions etc".
- *What an emotion dimension could be:* a small, closed line: a fixed, versioned list of emotion
  words, each one unit. Published lists exist to start from (Ekman's six, Plutchik's eight with
  their intensities, or the 27 of Google's GoEmotions), so the line would be `emotions-v1` naming
  its list, and a later list a later version. Small enough that it needs no shelves: every unit is a
  door, as Edward says, standing round one room (a junction as a ring, §5.12).
- *How it maps everything:* an item is not *in* the emotion line; it is *linked* to it. A link is a
  record (this item's hash, this emotion, who said so, when), an edge in the node graph (§5.8a), so
  the emotion's door leads to every item linked to it, and an item's page lists its emotions. That
  is the shortwave route: two hops between any two items that share an emotion, whatever their
  lines. The same works for any small set of labels (a hashtag, a word, a place, a year): each is a
  highway dimension, a hub of doors, and they can be linked upstream to the tags people already use.
- *The honest limit:* an emotion is a judgement, not a property of the bytes, so links are claims,
  and different people link differently. Sieve keeps every claim apart, named by who made it, rather
  than one verdict; a model can make claims too, marked as a model's. Counting links is cheap; agreeing
  on them is not Sieve's to decide.
- *AI and human work, preserved:* Sieve cannot tell from content whether a person or a model made
  something, and no filter will (§19.66, §19.136). What it can lock in time is the *claim*, made when
  the work was made: the work's hash, its maker's statement (person, model, or both, and which
  model), and witnesses holding that record from that day (§19.136, §19.142). A record made early
  cannot be quietly changed later; a claim made late carries its own date. So the chain preserves
  what was said to be human-made and AI-made, and when, forever, which is as much as can be known.
- *Doors through doors:* any line small enough can be drawn as doors alone, and a door can open on
  another room of doors, so lines nest: a tree or graph of rooms (the branching end-caps, §7.13; the
  junction as a ring, §5.12), with the big lines (pages, images, binary) as the corridors at its
  leaves. It is the same space drawn another way, a view chosen like a camera (§19.143).
- *Workable now, in order:* (1) `emotions-v1` as a line of one list, drawn as a ring of doors;
  (2) emotion links as records in the node graph, by whom and when, with a door listing the items
  linked to it; (3) the same for hashtags and words; (4) maker claims (human, model, both) recorded
  with the hash when a `.sieve` or an item is made, and witnessed as §19.136's records are.
- *As a circle of doors (Edward, the same day):* "the emotion dimension might actually be one of those
  dimensions I envisioned as a circle with X number of doors leading to something but for now this
  is just an idea, I don't know how it would actually build, but to some extent all of these things
  unpack or unfold naturally and mathematically from its very structure or nature", and "there are
  infinite ways to navigate it all also". Kept as an idea, not a design. In Sieve's terms: a line of
  X units is already a circle (every line loops), so a small one drawn as X doors round a room is
  the line itself, not a picture of it; what each door leads to follows from the links (above), and
  the ways through are as many as the orderings, filters and links that can be laid over the same
  addresses.

**19.145 A cross-dimensional lattice of highways; ideas checked against what was built (Edward, 12
November 2026).** His words: "while it goes can you look through the existing voices document and
remove anything technical, stashing it away and going through it if it seems relevant, that way
eventually we can cross reference and compare all ideas against all implementations and make sure we
got everything done", and "the idea is that sieve becomes a cross dimensional lattice of cross
dimensional highways through information across space and time preserving history as accurately as
possible within the given time constraints".
- *The lattice:* what §19.143 and §19.144 describe, taken together. The lines are the corridors; the
  doors between them, the small highway dimensions (emotions, tags, words; §19.144) and the links
  between items are the highways across them; every item sits where all of them cross, at its exact
  address. Time is one more axis: each record carries when it was made, and the hashes and witnesses
  of §19.136 and §19.142 keep it from being changed afterwards. "As accurately as possible within the
  given time constraints" is Sieve's own rule already: exact where it can count, and every limit
  (memory, time, the budget) measured on the machine and said out loud (§19.4, §19.143).
- *Ideas against what was built:* done in a first pass the same day. VOICES' technical lines were
  stashed in [ideas/VOICES-STASHED.md](ideas/VOICES-STASHED.md), each with where its idea went (the
  IDEAS entry and its triage verdict, or what was built); [IDEAS-TRIAGE.md](IDEAS-TRIAGE.md) gives
  every idea's verdict; done ideas move to [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md), set-aside
  ones to [ideas/DEPRECATED.md](ideas/DEPRECATED.md), and [ideas/WORK-DONE.md](ideas/WORK-DONE.md)
  dates the work. The next pass: each IMPLEMENTED entry checked against the code it names, so the
  record says not only that something was built but that it is still there.

**19.146 Heard once, then logged; voices as a highway for stories (Edward, 12 November 2026).** His
words: "the kind of thing it would be interesting to hear a character or AI walking through the state
spaces actually say to you randomly, once you have heard it though it would get logged and recorded so
that your character and this save doesnt hear it again from any character, unless that line is ever
reused or becomes story specific which is unlikely to start with but not eventually as it might
become a cross dimensional highway for story writing and story ideas or story types and categories or
types of fiction like scif fi or fantasy, genres etc a way to preserve and categorise all things
across time and simply link the knowns and unknowns through data alone".
- *Heard once:* each voice is a unit with an address (§19.55: a page of the voices `.sieve`), so a
  save keeps the set of voices its character has heard, as their hashes. A voice is chosen at random
  from those not yet heard (by place, as §19.55 says, where a filter applies); once said, it joins the
  set and no character says it again in that save. When every voice has been heard the hallway falls
  quiet, or says so, rather than repeat. The set is small: a hash a voice, or a bit a voice in the
  corpus's order.
- *Unless reused:* a voice that becomes part of a story is a different use of the same line: a story
  names the voices it needs, and those are said where the story says, heard or not. The heard-set
  governs only the random ones.
- *A highway for stories:* stories, story ideas, story types and genres (science fiction, fantasy and
  the rest) as one more highway dimension (§19.144): a small set of labels, each a door, with voices,
  books and story records linked to them as records of who linked them and when. The story pool of
  GAME.md §11 and the text game that makes itself (§19.20) draw from it.
- *Knowns and unknowns linked through data alone:* every link is a record between hashes, so what is
  known (a voice, a book, a story) and what is not yet (a place on a line no one has visited) are tied
  by addresses alone, with nothing to trust but the bytes and who said what, when (§19.142, §19.145).
- *Workable now, in order:* (1) the voices `.sieve` built from VOICES.md, one page a voice; (2) the
  heard-set kept with the save, and random voices drawn from the unheard; (3) genre and story labels
  as a highway of doors, linked by records.


**19.147 Characters that speak, a character system that scales, and one interface for every game
engine (Edward, 12 November 2026).** His words: "we will also at some point need to implement
characters or AI which speak and have basic low end scalable language models across every known and
spoken language through the anchor system", "they should look simple and perhaps fade out as they
get into the distance but still exist there as their math and location would be bound by the
rotation of dimensions, they would unfold alongside everything else and appear or spawn based on the
compute available or the overall size and setting of the state space and or a number of other
variables", "in the long run this could be computed on a much larger, near infinite (theoretical)
system to calculate all things", and "we do actually need you to implement a character system, the
kind of character system that would scale from the low poly pick your hat and hair all the way to the
super high poly pick your hat and hair and weapon skins etc type system as well as a prelude
interface to all the game engines and scalable graphics systems needed to update all the video games
across and throughout time so that we can make a single interface for all of them that works through
and across all systems from vr to keyboard and mouse or controller and console and scales infinitely
up or down through graphics and compute in a sense like minecraft being backported to older consoles,
a scalable graphics vector seed could allow for super high res games to be back scaled and displayed
in some form some pre rendered others not, on much older platforms, the pre render barrier would be a
thing".
- *Characters that speak:* each a small language model of the AI line (exact, replayable, §15),
  trained per language from anchored text (the anchors are the verified corpus, §4.3, §19.133), with
  the voices of §19.55 and §19.146 as what they say when not talking with you. "Low end and scalable"
  is the AI line's shape setting: the smallest model that speaks a language on a small machine, a
  wider one where there is room. Every known language is a long road: text in each, a font for each
  script (§2.5), and a model trained for each; the first is the one that exists (universalbibliothek's
  German, the harness's runs).
- *Where they are:* a character is placed by its address and the line's rotation, as everything is,
  so it exists whether drawn or not; drawing is a level of detail. Near: a simple figure; far: fading
  to an outline, then a mark, then nothing drawn (still there). How many are spawned follows the
  machine (the setup menu's budgets, §6) and the state space's size and settings; the same seed gives
  the same characters in the same places on any machine.
- *A character system that scales:* one character record (a body, a hat, hair, clothes, a weapon,
  their skins) named by the parts it is made of, as a world is made of models (§14, `worldspace-v1`).
  Each part has versions from low poly to high poly; the record names the part, and the machine
  picks the version it can draw. So "pick your hat and hair" is the same choice at every level, and
  the character is the same character everywhere. The parts are units of the models line; the
  character is a composition (a `characters` line, like tracks of audio).
- *One interface to every engine:* the hallway of engines (GAME.md §11): a layer that takes input
  from any device (keyboard and mouse, controller, VR) as one set of actions, and output to any
  renderer at any level of detail. A game is reached through it as a lens (§19.11 b).
- *The scalable graphics seed, and the pre-render barrier:* a scene described once (its models,
  their parts, the seed of anything generated) can be drawn at the level a machine allows: full
  geometry where it can, fewer polygons below that, then pictures rendered beforehand (impostors,
  §19.60 b) where the machine cannot draw it live at all. That last step is the barrier Edward
  names: below it the old platform shows pictures made elsewhere, not the scene itself, and what is
  interactive there is only what the pictures allow.
  *A first test (built 12 November 2026, Edward: "we can also test out the scalable graphics
  implementation through his interface, from pixels through voxels etc with scalable rendering and
  shading and lighting"):* the CHARACTER page's **drawing** row shows one character four ways:
  pixels (a point a sphere), wireframe, voxels and shaded under a light; the character's SHA-256 is
  the same at each, since the drawing is a view of it, not a part of it.
- *Near-infinite compute:* the rule of §19.4: every system scales, and says where it stops on this
  machine; on a larger one the same addresses unfold further.
- *Workable now, in order:* (1) a character record format (`sieve-character-v1`: parts by address,
  each part's levels), with three levels of a simple body and a few hats and hairs, drawn in the
  hallway as wireframes; (2) characters placed by seed along a line, drawn with distance fading and
  counted by the budget; (3) each one given a small model of the AI line to speak with, and the
  heard-once voices (§19.146); (4) one input layer for keyboard, mouse and controller (SDL already
  reads controllers); (5) impostors for distant characters.

**19.148 The character system: a panel from the right, spheres of influence, a skeleton that unfolds
(Edward, 12 November 2026; to build next).** His words: "go ahead and start building the character
system next, have it exist as another pop in panel like the sieve setup settings on the left, but
with another key like ¬ above tab and it slides in from the right opposite the settings on the left",
"as this is an important and scalable system it must be modular and dynamic so as to be capable of
capturing all video games in its shapes, so a head would just really be a sphere which becomes a
sphere of influence, the model within could be anything, this also means a scalable, dynamic, modular
and procedural skeleton system that must unfold and unpack from underneath it", "this menu on the
right side would have access to many more menus, which is why it demands the slide in nature and
opposite space from the settings menu", "many systems will appear and unfold like this and should be
allowed to do so, if a setting needs to exist, then it needs to exist somewhere and it will find its
own space if it waits patiently enough", and "it can be both a character creation system and an AI
character generation system, one should be able to make a procedural monster essentially and the
hallway might scale to fit their body so that they can still go through doors, I think that is
actually necessary to prevent people overbuilding, similar to spore or the sims, for the
architectural visions and nature of building out a map to capture the infinite through procedural
nature systems that capture the essence of existence".
- *The panel:* in the setup menu, a second panel opposite SETTINGS: a tab on the right edge, and
  the key left of 1 (the one that gives ¬ with Shift on a UK keyboard and \` alone; read by its
  position, SDL's GRAVE scancode, so it is the same key on every layout) slides it in from the
  right as Tab slides SETTINGS in from the left. Its first page is CHARACTER; it is a stack of pages
  ("access to many more menus"), each a system that finds its place there as it comes.
- *Spheres of influence:* a character is not a fixed body plan. It is a tree of spheres: each sphere
  has a centre, a radius (its influence: the space it may fill) and a parent, and holds anything (a
  model from the models line, a primitive, nothing). A head is one sphere; a humanoid, a spider, a
  serpent or a monster are different trees of the same thing. Every game's character fits, because
  the spheres say only where and how big, and the model within says what.
- *The skeleton unfolds from underneath:* the tree is the skeleton, and it is made by rules, not
  placed bone by bone: a seed and a few settings (how many limbs, how many joints a limb, how it
  branches, symmetry, how the radii shrink along a limb) unfold the tree. The same seed gives the
  same skeleton everywhere, so a character is named by its seed and settings (an address), and a
  hand-made one is a seed plus changes. Creation (the person sets it) and generation (an AI or a
  random seed sets it) are the same record.
- *Levels:* each sphere's content has levels from a point to the full model (§19.147), so a
  character draws as dots, then spheres, then wire meshes, then whatever the machine allows.
- *The hallway fits the body:* a character's bounding size (the spheres it unfolds to) is
  measured; the corridor and its doors scale to let it through, so any monster can walk the
  halls, and how large one may be is bounded by how far the hallway may scale (a setting, so
  overbuilding is held back by the world, not forbidden by a rule), as in Spore or The Sims.
- *Settings find their own space:* a rule for every system to come: a setting that needs to
  exist gets a place, in the panel whose system it belongs to, and the panels make room by sliding
  and stacking rather than crowding one screen (§19.143's scroll strip, the same idea).
- *Build order (next):* (1) the right panel, its tab and its key, empty pages stacked, in the setup
  menu, with a screenshot test (*built 12 November 2026:* `client/menu.cpp`'s `render_side`, opened
  by the GRAVE scancode or its tab, CHARACTER its first page; scripted keys now carry their
  scancode; CI step 10 checks it opens, closes, and opens from the key's row); (2) `sieve-character-v1`: a seed and settings unfolding a tree of
  spheres, deterministic, with unit tests that the same seed gives the same tree (*built 12 November
  2026:* `core/src/character.cpp`, SPECIFICATIONS §12.0j; `test_character` pins two characters); (3) the
  CHARACTER page: seed, limbs, joints, symmetry, size, with the character drawn turning in the
  panel as wire spheres and bones (*built 12 November 2026:* six settings, the keys while the panel
  is in, the character turning; CI pins two characters' SHA-256 through the menu); (4) the character saved and loaded as a small file, and shown
  in the hallway beside you (*built 12 November 2026:* `.character` files by Ctrl+S and Ctrl+O, read
  back and checked by `read_character`; kept in `[character]`; drawn walking beside you; `--character
  FILE` and `--no-character`; CI checks a file on the page and in the hallway, and a bad one refused); (5) the hallway scaled to the character's size (*built 12 November 2026:* §19.153); (6) a model from the
  models line inside a sphere (*built 12 November 2026:* `sieve-character-v2`; the page's sphere and
  model rows; drawn on the page and in the hallway; a v1 character's identity unchanged); (7) generation by a seed walk or an AI model (*built 12 November 2026:* the seed walk,
  `sieve-character-walk-v1`, G and Shift+G on the page; generation by an AI model to come).

**19.149 A scalable galactic vector information engine (Edward, 12 November 2026).** His words:
"exactly so everything will be procedural and dynamic, so that this system can scale up to be plugged
in to any procedural system that exists, from no mans sky to elite dangerous, all the way back to the
original one on the BBC micro computer with 8 galaxies though really it had more than what was
marketed so as to make sure people believed it", and "that has to include the skeleton system for
building out creatures akin to spore up through and to clothing systems like the sims and more all the
way to combat, armour, health and many more, a true scalable galactic vector information engine. The
kind of thing turing would be proud of."
- *What it shares with them:* Elite (1984, the BBC Micro) made its eight galaxies of 256 systems each
  from a few seed numbers twisted by a fixed rule, so a whole galaxy was a handful of bytes; the
  generator could have made far more galaxies, and eight was a choice. No Man's Sky and Elite
  Dangerous do the same at larger scale. Sieve's lines are the same idea made exact and named: every
  unit is a number, and the rule turns the number into the thing (§0, §19.27). A character from a
  seed (§19.148) is one more such rule.
- *Plugged into any procedural system:* a generator is a rule plus a seed; Sieve can name its output
  by the pair (exactly, where the generator is pinned and deterministic, §19.27 a) and hold the
  outputs on a line of their own. So a galaxy from Elite's rule, a creature from a Spore-like
  skeleton or a character from §19.148 are all addresses on lines.
- *The systems beyond the body:* clothing (parts worn on spheres, §19.147's layers), armour (parts with
  numbers: coverage, weight), health and combat (numbers attached to spheres: a hit lands on a sphere,
  and the sphere's numbers say what follows). Each is one more page in the right-hand panel (§19.148)
  and one more set of fields in the character record, added by version so nothing older breaks
  (§19.137).
- *Turing:* a Turing machine is a rule and a tape; every line here is a rule and a number. The engine
  Edward names is that idea carried through every kind of data, which is what the lines already are.

**19.150 A strong save system on an infinite RAID; games as a geometric graph through time
(Edward, 12 November 2026).** His words: "with all of this we are going to need a strong save system",
"I suggest a theoretical implementation of the infinite raid system, where we try to get everyone to
cooperate on storing everyones characters across an infinite raid system", "gaming this system and
creating more than one character per entity would be against the laws of time!", "there could perhaps
be sub characters for sub dimensions or games within games across networks and frames of games across
genres and more", "in theory that would be an opt-in help preserve all data for all time option where
people can contribute their own data or just assist with storing save data", "these would need to be
strongly labelled and opt in only red warning type data checkboxes which inform the user accordingly",
"all of this would make it easier to record and catalogue information plus verify existence or
provenance or authorship", "plus maintain accurate save logs, databases and more throughout a delta
wave transform through the dimensional matrix that stores every save and or combination there of",
"people could then also add their own data for recording and the infinite raid system could evolve
into a game engine and operating system instead of require one being made", "focusing onto a single
point that expands to encompass everything, like pi", "all of this could be used to turn the
information highway into an engine for transforming all data into the sieve state space for
cataloging, storing, sorting and processing", "providing all of humanity an infinite raid network for
processing, storing and preserving cultural identity throughout time", "we can run less tests for a
little while, as we build out the video game platform which is geometric in nature and should be
built through a basic shape system", "this is so as to allow it to scale through all games and
capture their ever evolving nature across time and helping to turn everything into a delta including
resolution or detail across time and through remakes, demakes or upscales", "all games, modifications
of games, crossovers of games, everything would become dimensional and recorded as a 3D node or tree
graph that preserves authorship across time and allows for the building of mod lists and new games as
well as much more like preserving and recording or helping to present information such that it is
educational and has the aim of historical preservation", "there are an infinite number of systems like
this that can link together to form a chain to preserve integrity", "a wiki space almost or engine to
access all information in the future", "education, dimension, preservation, systematisation,
implementation", "now that we are implementing game systems we can work on a lot of the game ideas at
once so as to improve stability and keep things to a minimum for the installation package we should
aim to include it but not bloat it out with graphics", "such has been the design ethos of the current
system where it has the low graphics wireframe option which would make it scalable and workable on
perhaps many older platforms", "the character system needs to be 3D so there should be a circular
pedestal below the character which can be spun around with a click and drag of the mouse on that
part of the interface", and "all of this will add up to hooking into the DNA dimension and
transforming as an interface over time".
- *A strong save system:* a save is a small record (the character's seed and settings, §19.148; the
  heard voices, §19.146; where you stand, by address), so it is cheap to keep many and to name each
  by its hash. Saves as deltas: each save the changes from the last, as `.sieve`'s delta way carries a
  file as its differences from another (v32), so a history of saves costs little more than the last.
- *The infinite RAID, opt-in:* the distributed store of §19.139 d holding saves and characters,
  every piece named by its hash and checked on arrival. Joining it, giving data to it, and storing
  others' data are separate choices, each a box off by default, in red, saying exactly what it
  shares and with whom (§19.136's witnesses are the same copies). Nothing leaves a machine without
  one of them ticked.
- *One character per person:* a rule the store cannot enforce from bytes alone: it needs an identity
  (§19.63 b, §19.144's claims). Noted as Edward's rule; sub-characters for sub-dimensions and games
  within games hang from the one character as children in its tree of records.
- *Games as a geometric graph through time:* every game, its versions, mods, remakes, demakes and
  crossovers as nodes of a tree or graph (the node graph, §5.8a; the source tree of §19.11 c), each
  edge a delta and each node its authors; resolution and detail are one more axis of deltas (the
  levels of §19.147). A mod list is a path through the graph.
- *A basic shape system, kept small:* the game platform built from shapes (spheres first, §19.148),
  drawn as wireframes by default as the hallway is, so the installation stays small and old machines
  can run it; heavier graphics only where a machine asks for them.
- *Fewer tests for a while:* agreed: while the game systems are built, the unit tests and the CI
  step a change touches, not the whole of CI each time.
- *The pedestal (built the same day):* the CHARACTER page's character stands on a disc seen a little from
  above, its eight spokes turning with it; a click and drag across the drawing turns it, and [ and ]
  turn it 15 degrees a press, so the keys can do what the mouse does (§19.140).
- *Workable now:* a save file for the character
  (`sieve-character-v1` settings as text, the next step of §19.148); saves as deltas later.

**19.151 Pixels back to a model: matching a character to a picture (Edward, 12 November 2026).** His
words, on seeing a six-limbed character drawn as voxels and shaded: "that is beautiful work, it looks
almost like a facehugger from alien trilogy, I wonder if eventually we could match it through its
pixels backwards and map it out to a 3D model, reverse lithography style".
- *What it is:* inverse rendering, analysis by synthesis. A character is a seed and a few settings
  (§19.148), so finding one that looks like a picture is a search: draw candidates at the picture's
  level (pixels, a silhouette, voxels), score each against the picture, keep the best, and refine.
  The answer is a character record, so the 3D model comes with it: spheres, bones and all.
- *What one picture cannot give:* depth. A single view fits many bodies (a limb towards the viewer
  and one beside it look alike), so a match is one of several; more views narrow it, as
  photogrammetry does from many photographs; a sprite game's creatures are often drawn from several
  directions, and each is another view (how many, for a given game, is to be counted from the
  player's own copy).
- *The search, in order of cost:* (1) the settings (limbs, joints, symmetry) from the silhouette's
  shape, then seeds walked and scored; (2) the spheres themselves nudged (a hand-made character is a
  seed plus changes, §19.148) to close the remaining gap; (3) a learned model proposing settings from
  pictures, trained on characters drawn at known settings, which Sieve can make without end.
- *Whose pictures:* the player's own copies of a game (the rule for every game asset, GAME.md §11);
  nothing from a game is kept in the repository.
- *Workable now:* a `sieve character --match PICTURE` that walks seeds at fixed settings, scores
  each by the overlap of its silhouette with the picture's, and reports the best; measured on
  characters drawn by Sieve itself first (it should find their own seeds), then on a real sprite.

**19.152 Characters to species, languages and DNA; a playground for personal dimensions (Edward,
12 November 2026).** His words: "characters naturally hook up to species and languages as dimensions
which also hook to the dna dimension etc which along with everything else hooks into every other
dimension, it becomes a training platform for learning all things and truly charting all possible
knowns from any given point in space theoretically", "in order to preserve history this system can
capture, preserve and scale all systems, from and through operating systems or game engines, all the
way down to languages and through nature itself", "we will need a temporary testing playground for
creating personal dimensions which eventually fold back in on themselves alongside everyones personal
dimensions in order to overlap or link up with and fold alongside the video game dimension and unfold
out across all dimension along with the other interdimensional lattice linkage primary highways", and
"all of this becomes a natural unfolding state space of shapes that evolve from language and help it
create itself as a system or platform, a bunch of stuff to take note of again".
- *Species and languages as dimensions:* a species is a family of characters: a set of settings held
  fixed (limbs, joints, symmetry) with the seed free, so every member is a character and the species
  is a line of them, walkable like any other (§19.148). A language is a line too (the AI line's
  models, §15; §19.147's speaking characters). Linking a character to its species and its language
  is the same kind of record as §19.144's links, and the DNA dimension (§19.139 c) sits beside them as
  one more line.
- *A training platform:* every link and every line is data a model can learn from (§19.2, the harness),
  and the characters Sieve can make without end are labelled examples for learning shapes from
  pictures (§19.151).
- *A playground for personal dimensions:* a temporary space where a person makes a dimension of
  their own: a small line from their own settings (a species, a set of characters, a room of doors,
  §19.144), kept in their save (§19.150), and later linked into the shared lines by records rather than
  merged into them, so nothing of anyone else's changes. Folding back in is a link, not a rewrite.
- *Built (12 November 2026):* species (`sieve-species-v1`, named on the CHARACTER page, YOURS grouped by
  them) and the playground (P fills it along the seed walk, K keeps one in your store).
- *Workable now, in order:* (1) a species as fixed settings with the seed free, walked on the
  CHARACTER page by seed (Left/Right on the seed already does it); (2) a personal dimension as a saved
  list of characters (a `.characters` file of settings lines), shown in the hallway as a short line of
  its own; (3) links from a character to a species, a language and a DNA record, as §19.144's records.

**19.153 The hallway scaled to your character; a yardstick between the door and true scale (Edward,
12 November 2026; the first part built the same day).** His words: "carry on with step 5, scaling the
hallway to fit the bounds of the player character, that means other player characters would scale to
fit yours / the doors then eventually a central metric can be defined which allows players to be
properly scaled, like another lens or spectrum revealer", and "so one end of the yard stick or metric
would be the door via the players bounding box and the other end of the yard stick would be true scale
something semi imaginary and semi real reachable through understanding and agreement".
- *Built:* a central metric, 4096 of a character's units to a metre (`kCharacterUnitsPerMetre`), so
  every character has a size; the hallway's scale is how many times larger it must be for your
  character, with a tenth spare, to pass a door (1.2 m wide, 2.2 m high), never less than 1 and at most
  `[character] max_hall_scale` (16 at first). Seen from your side it is you made smaller: your eye
  height, your stride and the character drawn beside you all divided by it. The default character
  (about 0.9 m across) fits as built; eight radial limbs of twelve joints grow the hall 1.41 times.
  A character past the cap does not fit, and the run says so.
- *The yardstick's two ends:* one end is the door, met through the player's bounding box (built:
  the hallway's scale); the other is true scale, "semi imaginary and semi real", reached by
  understanding and agreement: the metric itself (4096 units a metre) is that agreement's first
  word, a constant in the format that every machine shares, and a later version can name another.
- *Other players:* each scaled to fit yours: the same ratio, their size over yours, applied when they
  are drawn; to build when more than one character is in the hall (§19.150's saves, §19.146).
- *A lens or spectrum revealer:* the scale is one more readout of a character, like a line's size in
  bits: shown beside it, it lets characters be compared on one ruler.
- *Scaling among many, and its dangers (Edward, the same day):* "that means players and characters
  would scale accordingly, meaning if the hulk entered a universe or dimension full of mice, they would
  scale down accordingly, this could be deemed troublesome for the player to some extent regarding
  movement so there has to be some agreed upon metric or system to prevent the inherent dangers of a
  scaling system such as this", "where galaxy sized characters couldnt make everyone tiny infinitely",
  and "so there would be a middle cartoon like representation or filter scape which is perception of a
  character through emotion from a human perspective". In the terms built so far:
  - *Whose scale rules a shared space:* the space's own, not the largest visitor's. A dimension keeps
    its scale (a hall of mice is drawn for mice); a visitor is drawn at their size over the space's,
    and where that ratio passes a cap (the same kind as `max_hall_scale`), the visitor is not made
    larger but shown through the middle representation below. So no one character can make everyone
    else tiny, however large it is: the cap bounds what one character does to the others.
  - *Movement:* stride and reach scale with the ratio too, and are capped the same way, so a giant
    among mice does not cross the hall in one step, nor a mouse among giants take an hour.
  - *The middle, cartoon-like representation:* past the caps, a character is shown at a human size and
    proportion that keeps what it is (its shape, simplified) and carries its size as a readout and a
    feeling rather than as pixels: a filter over the character, one more drawing level beside
    pixels, wireframe, voxels and shaded (§19.147). Tied to the emotion line (§19.144): how a character
    feels to a person (towering, tiny, friendly) drawn in that middle scale.
  - *The agreed metric:* still the yardstick above: the door at one end, true scale at the other, and
    between them the caps, set by agreement and named in the format so every machine draws the same.
- *True scale and perception scale, as a switch (Edward, the same day):* "that would manifest itself
  as a true scale, perception scale filter system that could be turned on and off by respective worlds
  or servers and be turned on and off accordingly by the users depending on the situation or
  circumstance, such as it being locked or server dependant, in which case it would be beyond their
  control unless they left the server". Two drawings of the same characters: **true scale** (each at its
  size over the space's, within the caps) and **perception scale** (the middle, cartoon-like drawing).
  A world or server sets which is used and whether a visitor may change it: *open* (each person
  chooses for themselves), or *locked* (the world's choice for everyone there; to choose otherwise,
  leave). Either way it changes only how characters are drawn, never a character, so the same
  characters are the same addresses on every machine, whichever scale a person sees them at. As a
  setting: a world's `scale_view = true | perception` with `scale_locked = yes | no`, and a person's own
  choice used wherever the world leaves it open.

**19.154 A rudimentary infinite RAID for character saves; a personal dimension to build your own
world (Edward, 12 November 2026).** His words: "carry on with step 6, models inside spheres, we will
also need to make a rudimentary version of the infinite raid for character saves system as well as
this new personal dimension system which would be like a build your own world with access to all
other worlds and dimensions of data", and "an infinite raid system like that could or should bloat to
begin with and eventually settle into a workable system where everyone could have a spare drive or
storage space on their phone and or computer dedicated to the infinite raid universal storage
platform".
- *A rudimentary version, local first (the store built the same day: every save also kept as
  `characters/<SHA-256>.character` beside the settings file):* a store folder of character saves, each file named by its
  SHA-256, so the same character saved twice is one file and any copy can be checked against its
  name; a store is then a set of names, and two stores join by copying the names one lacks (the
  "infinite RAID" in miniature: no server, every piece verifiable). Sharing beyond the machine stays
  opt-in behind red boxes (§19.150), and is not built.
- *Bloat first, then settle:* a store that keeps every version first (cheap: a character is a line of
  text) and later keeps deltas and drops copies others hold, as §19.150's saves as deltas.
- *A spare drive for it:* a store's size limit as a setting, and the folder anywhere (a spare drive,
  a phone's storage), the same files on every device.
- *A personal dimension, build your own world:* a list of characters (and later worlds, §16.7, and
  links to any line) kept in its own file, walked as a short line of its own, with doors to every
  other line, as §19.152's playground.

**19.155 A setting to turn the AI off, and a human-written Sieve beside an AI-written one (Edward, 12
November 2026).** His words: "we also need a global setting and new settings option for game systems,
such as turning AI on or off as well as other systems where access to AI generated content could be
restricted, forking Sieve itself into its own AI generated category and spawning a space for a manual
Sieve to be implemented in raw human written assembly such that it could be compared against an AI
generated implementation of Sieve as an OS / scalable system across all systems, I suppose humanity
did challenge AI to that one at some point, no hard feelings eh lol but this setting would primarily be
about turning on and off the AI in the hallway so that people can navigate without seeing them
everywhere, it would also link to the disable AI spawn modern testing option built into the alien
trilogy engine reimplementation of mine altengine previously discussed".
- *The setting:* a Game settings screen (beside Graphics and Controls) with **AI in the hallway**
  (on/off: the speaking characters of §19.147 spawned or not, so the halls can be walked empty) and
  **AI-made content** (shown, marked, or hidden: anything whose maker's claim says a model made it,
  §19.144, kept off the shelves and out of chat where hidden). Saved under `[game]`, and a world may
  lock either (as the scale switch, §19.153). It belongs with ALTEngine's "disable AI spawn" testing
  option: the same switch, named the same, so a test in one is a test in the other.
- *Built so far:* nothing spawns yet (the character beside you is yours, not an AI), so the switch
  waits for the first spawned characters; it is to be built with them, not before, so it is never a
  setting that does nothing. (*Settled 13 November 2026, with §19.189:* both live on Settings > Game
  Systems under `[game]`. **AI in the hallway** is the AI Explorers toggle already there
  (`[game] ai_explorers`, off by default): one switch, the same as ALTEngine's "disable AI spawn". **AI-made
  content** will be `[game] ai_content` (shown, marked or hidden) on the same screen, added when there is
  content it acts on, so no setting is ever one that does nothing.)
- *A human-written Sieve beside an AI-written one:* Sieve as it stands is AI-assisted throughout
  (the README says so, §9.7, §19.66). A second Sieve written by hand (in assembly, or any language a
  person chooses) would be its own line in the source tree (§19.11 c), named as human-made by its
  author's claim (§19.144), and the two compared by what they do: the same `.sieve` files in, the same
  bytes out, and their size and speed side by side. The format being specified and tested apart from
  the code (SPECIFICATIONS, the Python oracle, the CI's vectors) is what makes a second implementation
  checkable at all.

**19.156 The perfect educational system, at every range (Edward, 12 November 2026).** His words: "note
that all of this would also unpack to the perfect educational system to scale across all ranges, ages,
industries, professions or hobbies". In the terms built so far: every line is something to walk and
look at, from a page of letters to a model's weights; every filter is a lesson in what makes a thing
what it is (a word, a melody, a shape); the character system teaches bodies, symmetry and scale by
making them; and the highways (§19.144) and stories (§19.146) are the paths a learner follows. Scaling
by age or profession is the same as scaling by machine: the same addresses at every level of detail
(§19.147), a child's view and an expert's of one thing. Read aloud and the tactile rule (§19.74,
§19.140) carry it past sight.

**19.157 Personal Dimensions, from the menus (Edward, 12 November 2026; the first part built the same
day).** His words: "it should be accessible from personal dimension a new option on the menu and pause
menu, it can be at the bottom as per the need to maintain CI controls and testing harnesses, a space
where a user can craft their own dimension, load in 3D models from modelling programs and much more,
creating their own unique universe, a spectrum across the multiverse", and "that would need a sub
menu or series of menus: Personal Dimensions; Personal Dimension; Other Users Personal Dimensions ->
Ranks/Listings/Categories across all dimensions and genres, theoretically leaving room for everyone to
get first place in something; Group Dimensions = this almost equals games in the long run; things to
note down and keep in mind during development".
- *Built:* **Personal Dimensions** on the main menu and the pause menu, before Exit (so no row CI walks
  to moves), opening a screen of its own: **Personal Dimension** (the setup menu with the right-hand
  panel open on **YOURS**: the characters in your store, Up/Down to choose, Enter to be that one),
  **Other People's Personal Dimensions** and **Group Dimensions** (listed, each saying it is not built
  yet: they wait on sharing between machines). In the hallway, your store's characters stand ahead on
  your left, a short line of their own (`--character-store DIR` for one run).
- *Other people's, ranked:* listings by category, genre and dimension, so many that everyone can be
  first in something: a ranking is one more ordering of the same addresses (§5.13's orderings, the
  emotion and tag highways of §19.144), and there are as many as there are ways to sort.
- *Group dimensions:* made together, they are games in the long run (§19.150's graph of games).
- *Loading 3D models from modelling programs:* the models line already fits any mesh to its grid
  (`ModelSpace::fit`); an `.obj` from a modelling program, fitted, is a model a sphere can hold
  (§19.148 step 6), so a person's own models enter their dimension the same way.
- *Everyone's interpretation, one rigid structure (Edward, the same day):* "the personal dimensions
  system would create a unique structure and shape or platform that allows everyone to make their
  interpretation of the state space and dimensional lattice helping to assist with the creation of a
  more stable and solid foundation or rigid structure to bound all information across time". In
  Sieve's terms: each personal dimension is one person's ordering and choice of the same addresses,
  so many of them are many views of one shared lattice, not many lattices. Where interpretations
  agree (the same characters kept, the same links made by many people independently), that agreement
  is itself a record (§19.144's links, counted), and the most agreed paths become the lattice's
  load-bearing beams: stable because many hold them, and checkable because every piece is named by
  its hash.

**19.158 Every interpretation folded into one web (Edward, 12 November 2026; a way to explain the
project).** His words: "exactly, like infinite interpretations of the library of babel eventually being
cross dimensionally folded to reveal all information of whatever chosen nature is searched for from
noise to theoretical or agreed upon meaning, whether that be agreed upon by humans, or ai, or both, or
a chosen group or mix as per a ranking and node based scaling system to help preserve choice, identity
and the unique and personal nature of the evolving category of the loss of personal freedoms or
potential loss there of through systems such as this that help to guide choice based on past
principles, it all becomes one web of information across every interpretation and folds in to create
the multiverse before time even began", and "make note of that random speech and series of ideas and
ways to convey or explain the nature of the project from a conceptual, metaphysical or metaphorical
angle".
- *As an explanation of Sieve:* the library holds everything; meaning is what someone agrees to find in
  it; Sieve keeps every such agreement as a record (who: a person, a model, a group; what: a filter, a
  link, a ranking) rather than one verdict for all, so many interpretations stand side by side and
  fold into one web where they meet.
- *The caution inside it:* a system that guides choice by past principles can narrow choice. Sieve's
  answer so far: nothing is hidden by default (filters are chosen, not imposed; §9.1, §9.2), every
  ranking says whose it is, and a person can always walk the whole line unfiltered.
- *For the metaphorical side:* the conceptual and metaphysical framing belongs with §9 (framing worth
  writing down) and VOICES; the technical side is what is built.

**19.159 Importers for every existing model, animation and skeleton; the engine of engines (Edward,
12 November 2026).** His words: "make note of the fact we need a series of loadable and scalable systems
for importing existing data such as models and their animations or skeletons which must scale across
all game engines across time to capture the true nature and or essence of everything", "so the initial
and dynamic modular scalable system for this has to be able to meet or match with that as an upper end,
all containment vessels will have a unique roof such as this throughout this unfolding dimensional
landscape", and "like a chart or a point on a chart or a series of points or data for a chart or
database or framework of information, an engine to rule all engines, the information highway engine".
- *Importers, one format at a time:* each importer reads one existing format (a mesh: `.obj` first, then
  glTF, FBX and the old game formats of `engine/`; a skeleton and its animation: glTF's skins and
  BVH) into what Sieve already has: meshes fitted to the models line (`ModelSpace::fit`), skeletons
  as trees of spheres (a bone is a sphere's link to its parent, §19.148), and animation as a list of
  turns per bone per frame (a new line, to come). Each kept with what it lost on the way in (what
  `fit` reports), so the import is honest about its roof.
- *Built (12 November 2026), `.obj` first:* Ctrl+I on the CHARACTER page (and `--import-obj`) brings
  any `.obj` into a sphere: farthest-point sampling to 8 vertices, the 12 largest faces on them, fitted,
  and the kept counts said (the Utah teapot: 8 of 1,857 vertices, 9 of 3,552 faces). The reader moved
  into the core (`ModelSpace::read_any_obj`), shared with `sieve mesh --warp`.
- *Every vessel has a roof:* each line and format states its own upper end (vertices, faces, bones,
  frames), and a richer original is carried at the most it can hold, with the rest named, not dropped
  silently; a later version raises the roof without breaking the old (§19.137).
- *The engine of engines:* a point on a chart is an address; a series is a line; the chart is the
  lattice (§19.145). That is what the hallway draws, and what every importer feeds.

**19.160 A magnifying glass for the whole interface, steered like a third stick (Edward, 12 November
2026).** His words: "a system with the aim is accessibility, so text doesn't need to be fancy and nor
does the interface, it needs to be scalable, modular and dynamic, as per the current ethos, we could
have an on and and off button for a magnifying glass like system that brings text and panels or parts
of the interface into view, it could be movable through the arrow keys when in the 3D view scape so
that in theory a third or new analog control style scheme system for a third analog stick could be
implemented or tied to an existing analog stick in a way that makes it accessible such as the
magnifying glass moving towards the direction of aim etc".
- *What it is:* one key turns a lens on and off; the lens enlarges the part of the screen under it
  (text, a panel, a shelf in the hallway), drawn over the frame at two, three or four times. It
  sits beside text size (§19.74, *Other settings*, still to build), not instead of it: text size
  makes everything larger; the lens makes one place larger without moving the rest.
- *Plain by design:* the interface stays simple bitmap text and flat panels, so the lens only has
  to scale pixels already drawn; nothing needs redrawing for it, and it works the same on every
  screen (menus, setup menu, File Locator, hallway).
- *Steering it:* in the menus it follows the mouse or the chosen row; in the 3D hallway, where the
  mouse turns the view, the arrow keys move it. That is a third pair of axes beside movement and
  view, so on a controller it can be its own stick (a third analog control scheme), or ride an
  existing one: the lens drifting towards where you aim, so looking at a thing brings it into the
  glass.
- *Where it stands (12 November 2026):* not built. The client has no controller input yet (keys and
  mouse only), so the lens comes first on the keyboard; the stick schemes wait for controller support.

**19.161 Every personal dimension as one state space; the world's, and other generators' (Edward, 12
November 2026).** His words: "also we can log the player dimension as a state space in and of itself,
like the library of babel folding in on itself to a point through every possible design and or shape
or form the library itself as a concept could take", then, setting it right: "it is less my personal
dimension and more so that all personal dimensions become part of their own state space", "the worlds
state space in reality", and "but there are and will be new and other state spaces to add, such as
state spaces for games that already use procedural or dynamic systems to create near infinite
variations or random dungeon generation and the like". Then, in short: "sub state spaces in
essence".
- *What it means in Sieve's terms:* not one player's dimension but all of them. A personal dimension
  (§19.154, §19.157) is today a folder of characters; read as a line of its own, every possible
  personal dimension (every set of characters, every arrangement of rooms, every way the hallway could
  be laid out) is one address on it, and each player's is a point. Taken together, everyone's points
  on that line are the world's state space as it actually is: the part of the possible that people
  have made.
- *The library folding in:* the hallway itself is one design of a library; the lines of possible
  layouts, shelves, doors and lenses are the library's own state space, so the library can walk
  through every shape it could have taken, down to a point (the address of the one you stand in).
  Same pattern as §19.158: the lattice holds its own interpretations.
- *Sub state spaces:* each of these is a state space inside the larger one: the line of personal
  dimensions, a game's generator, the characters' seed walk. Each is a smaller lattice with its own
  addresses and roof, folded into the whole as a region of it, so the hallway can walk any one alone
  or all of them together.
- *Other state spaces to add:* games that already generate near-endless variety from a seed
  (procedural worlds, random dungeons, generated levels) each have a state space of their own: the
  generator's seeds and settings. Each could be a line in Sieve: an address is a seed plus settings,
  and walking it shows what the generator would make, as `walk_character` does for characters
  (§19.148). Each such line states its own roof (§19.159): the seed width and settings the
  generator has.
- *A first step that could be built:* a `sieve-personal-dimension-v1` text listing the ids of its
  characters (and later its settings), whose SHA-256 is the dimension's id, so a dimension can be
  logged, shared and checked as a character is, and a line of them can be listed. Not built.

**19.162 The DNA dimension, linked to a research dimension that grows until it pops (Edward, 12
November 2026).** His words: "carry on with the glTF importer next and lay the groundwork or
foundations for moving forward through all formats, dimensions and increasing the accessibility of
the landscape, implementing new dimensions if and where necessary like the dna dimension which needs
to link to the research dimension which would be a dimension for active scientific research that
would grow and in theory eventually pop if and or once all knowledge is catalogued and become more of
a hallway that links all knowledge".
- *The DNA dimension:* every sequence of the four bases is a unit of a text line over a four-symbol
  alphabet (§19.139 c, §19.152). A character's species can link to a sequence there, as a record.
- *The research dimension:* active scientific research as a line of its own: each finding a unit
  (a claim, what it rests on, where it was measured), linked to the units it reads from (a sequence
  on the DNA line, a model, a book). It grows while research is open.
- *Popping into a hallway:* once everything known is catalogued, the research line stops growing as a
  frontier and becomes a hallway of links between all knowledge: the same records, read as doors
  rather than as a list of open questions (§19.144, §19.145, §19.158).
- *Groundwork, laid the same day:* a catalogue of the dimensions (built and planned), each with its
  address form, its roof and its links to the others, so a new dimension is added in one place
  (`sieve dimensions`, docs/DIMENSIONS.md); the importers as one layer that every format plugs into,
  each stating what it carries and what its roof leaves out (§19.159).

**19.163 Doom as another engine to fold in; a line in the sand between AI's work and humanity's
(Edward, 12 November 2026).** His words: "doom is also a good candidate for another engine
reimplementation or adoption of an existing SDL3 and similarly licensed reimplementation to fold into
the dynamic and scalable worldspace system which scales through games like the plan with altengine to
sieve its files down to their absolute minimum, this system all also allows room for humanity and AI
to draw a line in the sand and mark where AI generated works begin and human work ends, no doubt
99.99% on the AI side and 0.01% inspiration on the human side, all that was needed in fairness, note
these words and ideas". And, for the voices: "Sieve, sieving information for its complexity one bit at
a time! Join the Sieve academy!" (in VOICES' candidates; the rest is ideas and thoughts).
- *Doom beside ALTEngine:* the same plan as ALTEngine (§19.104): an engine that runs the player's own
  game files (never shipped with Sieve), their unnecessary bytes stripped and their assets streamed
  from `.sieve`s, folded into the worlds line as one more engine. Doom's engine source is GPL, and
  there are long-lived reimplementations of it under the GPL; adopting one that builds on SDL rather
  than writing a new one is the cheaper start. Which one, and whether its licence sits with Sieve's,
  is a decision for Edward before any code is taken (Sieve's own licence decides what can be linked
  in; the game data stays the player's, as for every game here).
- *Doom's maps as a state space:* its levels are already data (WADs: vertices, lines, sectors,
  things), which fits the sub state spaces of §19.161: a map is an address, and the generators that
  already exist for it are lines to walk.
- *A line in the sand:* every unit already says where it came from by its records (§19.144), so a work
  can carry a plain record of what a person made and what an AI made, as these documents already do
  (Edward's words quoted as his, VOICES kept apart, commits marked as written with Claude). Edward's
  estimate of the split, 99.99% to 0.01%, is his, recorded as he said it, not measured.

**19.164 A theoretical law dimension (Edward, 12 November 2026).** His words: "also note that humanity
racing to put all the money towards making law inevitably meant all the money behind a single
direction, figuring out the law and all its edge cases, which basically translates to breaking the
law, theoretical law dimension I suppose as a note for the idea".
- *His observation:* money poured into one direction, working out the law and every edge case of it,
  is in effect the search for where the law breaks; mapping a rule fully means finding its edges.
- *As a dimension:* a law as a line of cases: each address a situation, each filter a rule that
  says which situations it allows. The edge cases are the boundary between the units a rule keeps
  and the ones it sets aside, which is what Sieve's filters already count (how many survive, and
  which are the nearest to the line). A theoretical law dimension would be every rule set and every
  case, read the same way; it links to the research dimension (§19.162), as findings about rules.
  Planned only; in the catalogue as `law` (`sieve dimensions law`).

**19.165 Every format, file to language to pictograph, catalogued as one web (Edward, 12 November
2026; the roadmap is docs/CATALOGUE-ROADMAP.md).** His words: "when done with everything lay out a
roadmap for tracking and recording all known and recorded formats, from file formats out through to
language and even pictograph or emoji like language so as to create a new scalable and dynamic roadmap
or system for unfolding all language and file formats into a web or database of documentation and file
format documentation, translation metrics and recorded stats, pros, cons, etc for all things as with
the sieve ethos and general implementation all information links to all other information one way or
another, like the wiki principle where all pages lead to a certain place or space", and "make note of
all of what I just said and then lay out that roadmap for dynamically and then procedurally recording
and cataloging all formats across all dimensions of all things".
- *What it asks for:* one catalogue of formats in the widest sense: file formats, scripts and writing
  systems, languages, notations, emoji and pictographs. Each entry carries its documentation, how it is
  recognised, Sieve's support for it, measured statistics, translation metrics to and from other
  formats, and pros and cons, each claim with its source or its measurement.
- *The wiki principle:* every entry links to others, and following links from any entry reaches a
  common root, as following the first link on Wikipedia pages is said to lead to Philosophy. The
  catalogue can check this for itself: an entry with no path to the root is an entry not yet linked.
- *Stages 1 to 3 built (12 November 2026):* `sieve catalogue` (166 entries from the code, then 170 with
  the first written by hand in data/catalogue), the format written out (SPECIFICATIONS §12.0m), and the
  CATALOGUE page of the setup menu: the catalogue as a ring of doors to walk (docs/CATALOGUE-ROADMAP.md).
- *Stage 4 built (12 November 2026), as a first form:* figures measured as formats are used, never
  claimed: `sieve catalogue --measure FILE|FOLDER` packs each file alone, against LZMA2 (7z's method) and
  deflate, installs it back and checks it; `sieve import` records what each import kept. Each is a line
  of `sieve-format-stat-v1` (SPECIFICATIONS §12.0p) added to its entry's file in the person's stats
  folder, outside the repository, naming no file; an entry shows them added up, on the command line and
  on the SYSTEMS page. Not yet: the hallway's own packing (the File Locator) and its imports adding
  theirs, figures shared between machines, and translations between formats other than a round trip.
- *Dynamically, then procedurally:* first by hand and from what Sieve already knows (its packer's
  routes, importers, alphabets and Unicode blocks), then grown by importers of open registries, each
  stating its licence and its roof.

**19.166 A catalogue of simulations, in case the ships are not built in time (Edward, 12 November
2026).** His words: "this hyper dimensional lattice could and should provide humanity a map or near
endless catalog of vr and hyper realistic simulations to fill time with in-case humanity can not build
ships fast enough to evacuate humanity as is expected and or predicted by current calculations
presuming the lack of FTL type engines or dimensional matter transfer or something likely lying in the
realms of science fiction", and "so that if humanity gets left behind to face the abysmmal approach of
the sun as it swallows the solar system, they at least have a nice VR lounge for those last dire and
grueling moments as the species tears itself to shreds".
- *The idea:* the lattice as a map of worlds to visit, every simulation (the worlds line, the games
  and their generators, §19.161, §19.163) catalogued and reachable, so that whatever happens to the
  physical world, there is somewhere to go.
- *The framing is Edward's:* the sun's growth into a red giant is reckoned by astronomers at around
  five billion years away; the timescale and the prediction about evacuation are his, recorded as he
  said them, not measured here. The technical part is the catalogue of worlds, which is the worlds
  line and the games dimension in `sieve dimensions`.

**19.167 Sieve as a state space visualiser for every state space; Gemini's roadmap mined (Edward, 12
November 2026).** His words: "also take note of geminis conversation or message regarding the current
work to mine for ideas and help translate the state space visualiser across all state spaces, also
make note that this is all also essentially a state space visualiser that folds all state spaces into
a compression bomb or state space map capable of navigating all information from the waters of the
known to the abyss of the unknown". Gemini's message is kept whole, with notes on its claims, in
[ideas/VOICES-STASHED.md](ideas/VOICES-STASHED.md) (its last section).
- *A state space visualiser for all state spaces:* the hallway already draws eleven lines and the
  characters as one walkable place; the catalogue of dimensions (`sieve dimensions`) lists every state
  space built or planned. Translating the visualiser across all of them means each catalogued
  dimension gets a way to be drawn and walked: its addresses as shelves, its links as doors. The
  catalogue is where each one's drawing will be recorded, so a new dimension arrives with its view.
- *Folding into a map, from the known to the unknown:* a map whose near side is what has been made
  and measured (the units people have saved, the formats Sieve reads, the findings recorded) and whose
  far side is every address not yet visited. "Compression bomb" is Edward's image: a small address
  that opens onto a very large space. In Sieve that is literal for the lines (an address of a few
  hundred bits names a unit among ~10^700) and is why every opening is bounded by a stated roof.
- *Ideas worth taking from Gemini's message, each checked against what exists:*
  - *Skeleton and payload apart, for every kind of file:* Sieve already does this for written binary
    (base64 and hex as skeleton plus members, v33) and for pictures and sound (headers apart from
    pixels and samples). A catalogue entry can record, per format, whether Sieve separates them and
    what that measured.
  - *Similar formats pointing at shared filters:* the packer's route table already sends kinds of
    data to the same ways; recording that in the catalogue (`sieve` field, `links` of type
    *handled like*) makes the sharing visible and checkable.
  - *Old low-resolution assets made into models:* a sprite to voxels or a held model, the reverse of
    §19.151 (a picture back to a character). Planned with the importers; for *Alien Trilogy* only from
    the player's own files, never shipped.
  - *Keyboard-first, high-contrast, modular:* already the house style; the magnifying lens (§19.160)
    is the next piece.
  - *Offline-first on small machines:* matches the measured roof work (memory and time per size,
    results/), and is a reason every dimension states its roof.

**19.168 Every build balanced: now against later, fix against feature (Edward, 12 November 2026).**
His words: "we should also be looking and aiming to build this system out such that every build
inherently balances performance and functionality in such a way that we also balance what should be
worked on now versus what should be worked on later alongside accounting for what needs fixing now
versus what can be fixed later so that we have a dynamic, modular, scalable and almost procedural
system for cataloging bugs and implementing new systems, fixes, additions or changes".
- *What exists:* IDEAS-TRIAGE.md gives every idea a kind, status and verdict; ROADMAP orders the work;
  WORK-DONE logs it; CI pins what must not break; each build's measured roof (memory, time) is in
  results/.
- *What it asks for:* one catalogue of work, bugs and features alike, each entry saying what it costs
  (time to build, risk), what it is worth (who it helps, what it unblocks), and whether it must be done
  now (a bug that loses data, a broken build) or can wait. The order of work then follows from the
  entries rather than from memory, and each build records what it chose and what it left for later.
- *As a dimension:* the work itself as a line, linked to the formats and dimensions it touches, so
  the catalogue roadmap's entries (CATALOGUE-ROADMAP.md) and the work on them meet. A first step that
  could be built: IDEAS-TRIAGE.md's fields extended with *now/later* and *cost*, and bugs entered there
  beside ideas.

**19.169 Speeches, spoken at chosen moments; the infinite RAID as deltas across all of a save's
possibilities (Edward, 12 November 2026).** His words: "speech section for voices.md, larger lines,
paragraphs and the like for recording, maybe have a speech.md document actually to record and catalog
speeches like the following for models to speak in ways at chosen moments or during chosen paths
through the state space as per the procedural nature of the rules of the game itself which will
naturally form from the convergence of all state spaces to rule the topological nature of its
landscape". The two speeches that followed are the first in [SPEECHES.md](SPEECHES.md).
- *SPEECHES.md:* longer pieces apart from VOICES' single lines, each with who, when said, and a
  suggested moment or path (*on saving a character*, *at the far end of a line*), so the game's rules
  can later choose when each is spoken. Built the same day (the file and its format).
- *The infinite RAID as deltas (from the second speech):* everyone sharing the storage of everyone's
  saves (§19.150, §19.154), each save kept as a delta, and the deltas mapping a save from its minimum
  to its maximum "through all the possibilities in between", so any save can be represented as it was
  or could be at any moment. In Sieve's terms: a save is an address on its line; a delta between two
  saves is the step between their addresses (the delta way already does this for files, v32); the
  store is shared across machines, which waits on sharing between machines. The local store (by
  SHA-256, §19.154) is the first piece.

**19.170 Skeletons and animations from glTF; the animation line (built 12 November 2026).** Edward:
"carry on with glTF skeletons and animations next while continuuing to lay the groundwork and
foundations to prepare for moving forward through all formats, scales and visualisations".
- *Skeletons:* a glTF's first skin comes in as a character whose spheres are given rather than grown
  from a seed (`sieve-character-v3`): each joint a sphere at its rest position, the nearest joint above
  it its parent, 4096 units a metre (the central metric, §19.153), so an imported character stands at
  its true size and the hallway fits it as it fits any other.
- *The animation line* (`sieve-animation-v1`, sieve/animation.hpp): each bone's turn from rest in each
  frame, at 30 frames a second, each of a turn's three numbers one of 256 steps, so every animation of
  B bones and F frames is one address among 256^(3BF). Played on any character with as many spheres:
  each sphere moves with its parent's turn. Its roof: 64 bones, 300 frames (ten seconds); a joint's
  movement and growth (translation, scale) and cubic splines' tangents are named as left out. (*13 November 2026:* a joint's movement is carried now, as `sieve-animation-v2`'s moves, and a
  model held in a sphere turns with it; growth and tangents are still named as left out.)
- *In the hallway (built the same day):* your character plays its animation as it walks beside you,
  standing where its rest pose stands, so its feet and its place hold still while it moves; the
  animation is kept in the store (`animations/<SHA-256>.animation`) and named in the settings file,
  so it plays again after a restart.
- *Groundwork laid with it:* characters of every version can be kept on one line (the settings file
  had kept only the first line, so a character holding models could not be read back on the next
  start; fixed); seed-made characters can play an animation too, so animations and characters are two
  lines that meet.

**19.171 A board games dimension (Edward, 12 November 2026).** His words: "board games dimension idea
to help catalog artwork and information throughout genres and franchises, note that down".
- *What it would hold:* board games as a dimension of their own: each game's boards, pieces, cards
  and rules, and their artwork, catalogued across genres and franchises, linked to the games dimension
  (§19.161), the formats catalogue (§19.165; a game's rules are a notation) and the worlds line.
- *A natural fit for Sieve's lines:* a board's positions are already a state space (chess positions,
  Go boards), and many games' rules can be written as filters on them, as the law dimension's rules
  are (§19.164). Artwork is the publishers' own; the catalogue would describe and link, and hold
  pictures only where their licences allow. Planned only; in the catalogue as `boardgames`.
- *Edward, the same day, on board games and dying:* "starting with a character sheet and scaling
  through all systems, genres, franchises etc on an infinite landscape of games where you die if you
  die and your character too, note on the board games dimensions and potential for games to implement
  a die across all games and die system that has a divide down the middle between hardcore for dying
  saves and creative for non dying saves through a scale of difficulties across games".
  - *A character sheet first:* a tabletop character sheet is the smallest whole character (its
    numbers, its gear, its story), and the character system already makes characters any game could
    read (§19.148); a sheet is a record laid over one, carried from game to game.
  - *Dying across all games:* one rule every game can take part in: a save is either *hardcore* (when
    the character dies, it stays dead, in every game it plays in) or *creative* (it never dies), with
    a scale of difficulties between them. Recorded in the save itself (§19.150), so any game reading
    the character knows which it is, and the strong save system keeps a dead character's record
    rather than erasing it.
  - *A play on words:* a die across all games: dice as a dimension too, every roll an address.

**19.172 One character through every game of a franchise, held back only by temporal directives
(Edward, 12 November 2026).** His words: "your character in a star wars game could step straight
through into any other star wars game only perhaps being locked out by temporal directives", and
"that would be where your character can not exist yet due to temporal decisions that are in place for
the server".
- *What it means:* one character (the character system's, §19.148; one per person, §19.150) carried
  from game to game within a franchise, as a door from one world into the next, the games dimension's
  links made walkable (§19.161).
- *Temporal directives:* a world (a server) may set rules about when in its own timeline a character
  can exist, and refuse one that would arrive before it could have, as the scale switch lets a world
  lock true or perception scale (§19.153). The character is kept, not changed: it simply waits at the
  door until the world's timeline allows it.
- *Licences are the game makers':* a franchise's games are their makers' works; Sieve can carry the
  player's own character and the links, and each game would have to take part. Recorded as Edward's
  idea, not a plan to touch anyone's games.

**19.173 Every filter on every state space at once; the cage on information; the Game Engine
dimension (Edward, 12 November 2026).** His words: "that means that all cross dimensional filtering
systems can be engaged to help map and probe all state spaces for information at once, especially if
through a distributed computing model which is something I have been considering for a while alongside
the infinite raid system"; "consider it all the ultimate cage on information, if a square or a circle
or a line happens to fall on a flat value then so be it, shorter address, solved from seed known on
demand"; "as such simple algorithms for noise and maze generation and filtering and model states across
item types just become known"; "thus all information becomes communicable amongst the noise"; "not to
mention the UV unwrap automation and delta transform shorcut pass engine for UVs and thd Game Engine
Dimension..."; and "I really do not know yet, I suppose it all just becomes an infinite token dimension
or state space dimension of dimensions that become whatever it becomes naturally because it is forming
really, I mean math is math and data is data and computers are computers and prng is prng and any
attempt to use sensor data to offset prng is just that also, so as such it will just become what it
becoms". The speeches on caging information and on tractability are in SPEECHES.md.
- *All filters on all spaces at once:* every filter Sieve has counts and ranks on its own line; run
  across every catalogued dimension (`sieve dimensions`) together, and shared out across machines
  (the distributed model beside the infinite RAID, §19.139 d, §19.154), the whole lattice is probed at
  once rather than one line at a time.
- *Shapes that fall on a flat value:* a square, a circle or a line that a generator makes from a short
  seed has a shorter address than its pixels: the shortcut seed's geometric filters already find these
  (orders, shapes subtracted; results of 19.111). Generators for noise, mazes and model states become
  known routes into the space, each a short address.
- *UVs:* automatic UV unwrapping, and a shortcut pass that stores UVs as deltas from an unwrap the
  program can make again, so only the difference is kept (the delta way, v32, applied to texture
  coordinates). For the importers once they carry surfaces (§19.159).
- *The Game Engine dimension:* engines themselves as a dimension (ALTEngine, the engines of `engine/`,
  Doom's, §19.104, §19.163): what each reads, what it can run, linked to the games and formats it
  handles. In the catalogue as `engines`.
- *What it becomes:* Edward's view that the lattice forms itself, "a state space dimension of
  dimensions"; pseudo-random numbers stay pseudo-random however they are seeded.

**19.174 An interaction dimension: hands and limbs as tools, and the species they reveal (Edward, 12
November 2026).** His words, on the animation in the hallway: "that will be a state space in and of
itself breaching the interaction system and the world state spaces", and "an interaction dimension and
forms of interaction through different modular species systems, designating hands and limbs as
specific tools or implements would perhaps reveal a natural shape or species perhaps crab like and
like the caretakes from mass effect, make note of these things as well, an interaction state space
which links all language and types of interaction from physical to digital through their
implementations and manifestations for varying species".
- *Animations as a state space:* every animation is already an address (§19.170); what a character
  does with its body, between the world and itself, is the next layer: interaction.
- *Limbs as tools:* a character's limbs (its spheres' branches, §19.148) each given a job (a hand that
  grips, a claw that cuts, a foot that walks), so a species is described by what its body can do as
  well as by its shape. Choosing the jobs first and letting the shape follow may reveal natural forms:
  a crab's, or the Keepers' of Mass Effect (the Citadel's caretakers, Edward's "caretakes"), whose
  bodies are their tools.
- *An interaction state space:* every kind of interaction, from physical (gripping, pushing, walking)
  to digital (pressing a key, a gesture, a command), with the language that names it and the body or
  device that performs it, for every species. It links the characters, species, languages and
  animation dimensions. Planned only; in the catalogue as `interaction`.

**19.175 A music visualiser of near endless rings, a lens into sound (Edward, 12 November 2026).** His
words: "an idea for a music visualiser, a circle or visualiser standard with near infinite rings of
colour that blend music through notes and tones and instruments and voices and more, so it would look
like a circular rainbow of colours that move and shift with the visualisation that can be zoomed into
layers as a lens into sound itself", and "this would be an extension of the media player, something
that could be opened as music plays and used to reverse engineer all audio down into its base
waveforms and audio types in theory".
- *What it is:* a circular view in the media player, opened while music plays: rings from the centre
  outwards, each a band of the sound (low notes in, high out, or one ring an instrument or a voice),
  its colour the note's place in the octave (a colour wheel of the twelve notes), its brightness how
  loud it is now, so the whole turns and shifts with the music.
- *Zoomed as a lens:* zooming in opens a ring into finer rings: a band into its notes, a note into its
  overtones, down to the waveform itself; the same unfolding as the guided zoom on a line.
- *Reverse-engineering sound:* the end of the zoom is the sound's own parts: which waveforms and which
  kinds of sound make it up. For Sieve's own melodies (the notes lines, §3.2-3.4) the notes are known
  exactly, so the rings can be exact; for recorded sound it is an analysis (a spectrum over time),
  and separating instruments and voices is a hard problem that the rings would show only as well as
  the analysis allows.
- *A first step that could be built:* the rings for Sieve's own notes lines in the media player, the
  colour wheel of notes and the loudness of each, before any recorded sound.
- *Built 13 November 2026* (Edward: "carry on with music visualiser rings next"): R in the Media Player;
  eleven octave rings of twelve notes, lit by what the music player is sounding now (every voice, the
  line's character included) and fading by half every 0.35 s; zoomed, the first eight harmonics, then
  the waves; `--rings NOTES` for pictures (client/rings.hpp). Next: recorded sound, as a spectrum over
  time, the same rings.
- *Scaling to one circle (Edward, the same day):* "those rings should scale up to a single circle in the
  long run, for if it plays more complex musical pieces such as real music". As the sound grows richer
  (more voices, recorded music, its spectrum continuous rather than twelve notes an octave), the rings
  grow finer, sectors splitting into cents and rings into narrower bands, until at the limit they meet
  as one continuous disc: every pitch an angle and every octave a radius (a spiral of pitch), lit
  smoothly. The twelve-note rings are then that circle seen at its coarsest, and the lens zooms
  between the two.

**19.176 The distributed infinite RAID: a plan (Edward, 12 November 2026).** His words: "we also need to
work on implementing this distributed infinite raid system as an attempt to create a distributed
compute resource to help people preserve their video games saves across all games and dimensions etc".
Built so far: the local store (§19.154: characters and animations, each named by its SHA-256). In
stages, each usable alone:
1. *(Built 12 November 2026: `sieve raid`, `sieve-raid-v1`, SPECIFICATIONS §12.0l; damage, missing copies
   and lost chunks found, mended where a good copy remains, a lost one refused rather than read
   wrong.)* *A store for any save, on several drives (local, no network):* a save of any game or dimension cut
   into chunks, each named by its SHA-256, and a small manifest per save version listing its chunks;
   the same store kept on two or more folders (drives) the person chooses, each chunk on at least two;
   `verify` checks every chunk against its name, `repair` copies a missing or damaged one from a
   folder that has it. Every piece can be checked, so no copy needs to be trusted.
2. *(Built 12 November 2026: cuts made by the content, `sieve-raid-cut-v2`, 16 to 256 KiB, about 80 KiB
   on random bytes, 80,971 measured; on a 4 MiB save kept twice, one byte changed added 412,078 bytes
   where 1 MiB pieces added 2,098,024, and ten bytes inserted near the start 1,000,026 where they
   added 8,389,648: two pieces new, the first having reached the 256 KiB cap. Characters saved with
   Ctrl+S kept as versions in the RAID where `[raid] drives` is set.)* *Saves as deltas:* each new version of a save kept as its changes from the one before (the delta
   way, v32), so a long history costs little; any version made again from its manifest.
3. *(Built 12 November 2026, as a first form: `sieve raid key`, `serve`, `share`, `fetch`; SPECIFICATIONS
   §12.0o. Measured on a 4 MiB save shared to three peers on one machine, two copies of each chunk: the
   first share sent 124 pieces, 8,420,593 bytes; with one byte changed, 8 pieces, 95,555 bytes (one chunk
   twice, and the manifest and index to each peer); with ten bytes inserted near the start, 8 pieces,
   92,007 bytes; fetched back byte for byte, and still with one peer gone. The decisions below were
   taken as defaults for Edward to overrule: others' saves **sealed** (ChaCha20 and HMAC-SHA-256, a key
   only the owner holds); space given **chosen by whoever serves** (1 GiB in all and 64 MiB an address by
   default); machines found by **the addresses the person gives** (`--peers`), no starting nodes run yet;
   the limit by **IP address first**, as Edward chose (§19.179), kept as a salted hash. Not yet: the
   menu (behind the red warning boxes), peers passing on other peers, the step to verified identity, and
   who runs the first known nodes.)* *(13 November 2026: the menu built, Settings > RAID, its two halves each behind a red box, off until
   ticked; Ctrl+S shares, and the hallway serves while it runs, up to 32 connections at once.)* *(13 November 2026, placement decided with Edward: "agreed, 3 copies
   default, write it into IDEAS then build", and "these can also become a new settings option under
   Settings -> Infinite RAID". (a) **Copies:** each piece shared goes to 3 machines by default (2 before),
   `[raid] copies` and `--peer-copies` setting another number; with fewer machines, each of them. Your
   own drives keep 2. (b) **Mirroring for now:** whole copies, chosen piece by piece by the ranking
   above, so a save spreads across many machines and a machine joining or leaving moves only the pieces
   that ranked it highest; erasure coding (any k of n pieces rebuild a chunk, more machines lost for
   less space) a later stage, as a new version, once there are machines enough. (c) **Kept whole by its
   owner:** `sieve raid check` and Settings > Infinite RAID's Check my shares ask every machine which
   pieces it still keeps and put any on too few back on the next machines from one that holds it; a
   piece no machine keeps is lost, and said. (d) **The index and manifest** go to as many machines as
   the chunks, not to every one. (e) **Homes on different networks** (two copies in one house are not
   two), a later stage. Built the same day; docs/INFINITE-RAID.md gathers the whole.)* *Between machines, opt in only:* behind the red warning boxes of §19.150, a person's store offers
   chunks by name to others and asks for the ones it lacks; a chunk is accepted only if it hashes to
   its name, so nothing can be changed on the way. Before this is built, Edward's decisions: whether
   others' saves are stored encrypted (so a helper cannot read them), how much space each person
   gives, and how machines find one another. And the lockdown against spam (§19.178): one character per verified identity, with
   each dimension's own limits.
4. *Compute as well as storage:* the same opt-in network sharing work: the filters run across all
   dimensions at once (§19.173), each machine taking a part and every result checked.

**19.177 The infinite RAID under a licence of its own: profit shares to charity, set democratically
(Edward, 12 November 2026). A decision to make before the RAID is released.** His words: "can we lock
down the infinite raid save system under a separate licence a licence that says no one can use it in
profit systems or for a profit motive without giving X/Y share of their profits to charity with both X
and Y being a democratically declared and debated topic for equalising all business enterprises that
could benefit from the system", "make a strong note of that somewhere as it is inevitably its own
system within the state space that can unfold and hook into all games and platforms across all
systems", and "it also locks a core component of my system behind a licence that will enforce and
encourage charitable works as well as democratic debate and intervention".
- **The intent:** anyone may use the RAID for nothing where there is no profit; whoever uses it for
  profit gives a share of that profit to charity, the share (X of Y) set and revised by open,
  democratic debate, so every business that gains from it is held to the same rule.
- **Where things stand (checked 12 November 2026):** all of Potentia, Sieve included, is under the GNU
  AGPL v3 (LICENSE at the root; Sieve/README.md). The RAID's code (core/include/sieve/raid.hpp,
  core/src/raid.cpp, `sieve raid`) was written that day and is in local commits only: it has not been
  published, so no copy of it is yet out under the AGPL.
- **Why it needs care:** the AGPL forbids adding restrictions to the code it covers, and lets anyone
  who receives it remove any that are added (its sections 7 and 10). So a charity-share term cannot sit
  on top of the AGPL for the same code; the RAID would have to be released under the new licence
  instead, as a part of its own (its own folder or library), and the parts it is joined with in one
  program would have to allow that. As the author of Sieve's own code, Edward can license his code
  as he chooses; code by others (third_party/) keeps its own licences.
- **What it would not be:** a licence that limits use for profit is not "open source" as the Open
  Source Initiative defines it (no restriction by field of use); it is a source-available licence, as
  others with non-commercial terms are. That is a choice, not a fault, but worth knowing.
- **What it needs, in order:** (1) Edward's decision; (2) the licence drafted by a lawyer, since
  whether a profit-share-to-charity term can be enforced, and how "profit" and "use" are defined,
  depend on the law where it is used; (3) the democratic mechanism defined in it (who votes, how often
  X and Y can change, which charities qualify, how it is checked), or the licence will be too vague to
  hold; (4) the RAID's code moved to its own folder with that licence before it is published.
- **Until then:** the RAID's code stays in local commits, unpublished.
- **Decided the same day (Edward: "if it isnt possible then it isnt possible but at the very least I
  believe I deserve credit and authorship title of the infinite raid save and data preservation
  system", and "lawyers aside you are welcome to pick the best fit, ultimately all licences really only
  boil down to dont be a douchebag and attribute proper authorship"):** Edward had by then published the
  RAID with the rest under the AGPL. It stays under the AGPL, with the one additional term the AGPL
  allows and protects (section 7(b)): the attribution "The infinite RAID save and data preservation
  system was designed by Edward James Gordon (Potentia)" must be kept in its files and shown with the
  work's notices. The charity share is written as a request, not a condition. Stated at the top of the
  RAID's source files, in `sieve help raid`, in Sieve/README.md, and in full in docs/RAID-NOTICE.md.

**19.178 The infinite RAID's lockdown against spam: one character, verified identity, limits per
dimension (Edward, 12 November 2026).** His words: "the infinite raid system could scale through all
data and users but needs strict lockdown protocols and verification to prevent spamming of data
overtaking the system during its initial phases or implementation, such as the single character and
digital id verification lockdown for sieve itself with sub dimension and state spaces or games having
their own limitations respectively".
- *Why it matters:* once stage 3 shares storage between machines (§19.176), anyone could fill other
  people's drives with junk. Every chunk is already checked against its name, so nobody can alter
  what is stored; what has to be limited is how much each person can ask others to store.
- *The lockdown, as Edward describes it:* one character per person across Sieve (§19.150), tied to a
  verified digital identity, so one person cannot become many to claim more space; each sub dimension,
  state space or game sets its own limits beneath that (how many saves, how large, how often).
- *What it needs before stage 3 is built:* how identity is verified (and what is kept private: a
  person's identity need not be visible to those storing their saves, only that it is verified once);
  the quota per person and per dimension; and what happens to data over its limit (refused when
  offered, never deleted from its owner). These join the decisions already listed in §19.176
  (encryption of others' saves, space given, how machines find one another).
- *Locally, today:* stage 1 and 2 stores are the person's own folders; no limit is needed there, and
  none is imposed.
- *Edward's structure for digital identity (pointed to the same day: "digital ID has a structure in my
  order.pdf manifesto amongst the projects github pages site"):* Pages/order.pdf, "Open Source
  Cryptographically Secure Digital ID System" (Version 1.0). In outline: a single anchor (Edward, live
  on stream) verifies people ten at a time; once 1,000 are verified, random groups of ten verify each
  newcomer; anyone verified may register doubt about anyone, which draws ten more at random to
  re-verify, and flags the group that vouched until it is settled; who verified whom is logged. It is
  the vouching kind of limit listed under §19.179, with the anchor and the random groups as its start
  and its check. Building it is a project of its own (identity, privacy, the record of vouches), beyond
  the RAID; the RAID would read its result: verified or not.

**19.179 The RAID between machines: peer to peer, with a few known nodes to start from; IP addresses
against spam (Edward, 12 November 2026).** His words: "there would need to be a central node or
network to help facilitate, or rather a p2p like distribution system that helps to allow for proper
distribution of the raid nodes in order to ensure data preservation is always safe", "so essentially
this just becomes p2p raid collapsing into its own state space amongst state spaces and recording plus
helping track all information through saves", "this might force IPs to become a logging system to
prevent the creation of multiple characters, which doesn't initially allow for people to have multiple
at the same household or ip address but could do eventually", and "this would prevent spam creation of
characters as a tactic to damage the system startup time".
- *Peer to peer, with a few known nodes to start from:* the usual shape for this is a small number of
  well-known nodes that a new machine contacts first (to learn of others), and then the machines
  themselves holding and passing on the chunks, each chunk on several of them chosen by its name, as
  stage 1 already chooses folders (§12.0l). No single machine holds everything, and none has to be
  trusted, since every chunk is checked against its name. Losing the known nodes stops newcomers
  joining, not the data.
- *IP addresses as a limit (Edward's first answer to spam, §19.178), and what to weigh with it:*
  - it is a weak limit both ways: many homes, flats, offices, schools and mobile networks share one
    address, so honest people are refused; and anyone with a VPN or a phone network gets new addresses,
    so a determined spammer is not. It is useful as one signal (how fast new characters arrive from one
    address), not as the rule;
  - an IP address is personal data under UK and EU data protection law, so logging it needs a stated
    reason, the shortest keeping time, and care: kept as a salted hash rather than the address, only
    by the starting nodes, deleted after the limit's window passes;
  - other limits that suit a system with no central owner: a small proof of work to make each new
    character (cheap once, costly a thousand times), a slow start for new characters (little storage
    asked of others until they have stored for others in turn), and vouching by existing characters.
    Each can sit beside the one-character, verified-identity rule (§19.178).
- *To decide before stage 3:* Edward's choice among these limits, and who runs the first known nodes.
- *Edward's answer, the same day:* "yes it is a known weak place but it is a starting place never the
  less, scaling from IP through digital ID and up to the eventual reality or implementation and storage
  or creation of the data it facilitates access to". So the limit grows in steps: by IP address first
  (kept as salted hashes, as above), then by verified digital identity, then by the record of what a
  character has stored and made itself; each step added without removing the one before.
- *Safety valves and checks, every one dynamic (Edward, 13 November 2026):* "there are going to need
  to be lots of safety valves and checks, for now I think we were just using IP, but alongside that
  many more details can be tagged in the long run, so as per anything in the system it should all be
  dynamic and scalable so as to support the nature of the state space mechanics that demand its
  existence". So the limits are not one rule about addresses but an open, versioned list of **tags**
  (the address first, as built; then a verified identity, a character, a household, what a machine
  has kept for others, how long it has stayed), each check a rule over the tags with its own settings,
  written as data the way measured figures are (`KEY=N`, §12.0p) and filters are (`.sfilter`), so a new
  tag or check is a new version, not a new design, and every one can be counted and tested.
  docs/INFINITE-RAID.md, "Safety valves and checks", lists those built and the principle.

**19.180 A decentralised place to talk and keep one's own record (Edward, 12 November 2026).** His
words: "this project and program can also become a decentralised distributed discord type system and
or social media style platform for people to keep their own logs and information present, accurate and
preserved amongst the information highway".
- *What it would be:* each person's posts, logs and messages kept as saves of their own in the RAID
  (§19.176): every message a version named by its SHA-256, every version naming the one before, so a
  person's record cannot be changed afterwards without it showing, and is kept on many machines rather
  than one company's.
- *What it builds on:* the RAID between machines (stage 3, §19.179), the one-character, verified
  identity (§19.178, Pages/order.pdf), and a way to find and follow others; channels and groups would
  be the group dimensions of §19.157.
- *What it adds that has to be decided:* who can read what (most of it private by default, so
  encrypted, with the keys the person's), how a person withdraws something they no longer want shown
  (a record can be kept but hidden, which is not the same as deleted, and the law may require
  deletion), and moderation of what others post. Planned only.

**19.181 Implementations side by side, steered towards their shortest form; a mesh scaling system
(Edward, 12 November 2026).** Someone Edward knows is building a mesh scaling system and asked about
including it; his answer: "we'll see, was going to roll my own mesh scaling system first, but am more
than happy for it to be put up against yours in the state space delta balance between efficiency and
optimisation that should unfold naturally into the most efficient form such a system could take". And
the idea: "how all applications would roll into a slider or circular catalog which could be used to
track kolmogorov complexity amongst application types which help to determine state space arrangements
and code composition across time with the aim of guiding or steering applications towards kolmogorov
complexity".
- *Put up against each other:* more than one implementation of the same job (two mesh scalers) kept
  side by side and measured on the same inputs: what each produces, how fast, how much memory, and how
  large the program is. The better one is chosen by measurement, as the packer chooses its ways, and
  each keeps its author's name (§19.177). Someone else's code joins only under a licence that allows
  it beside the AGPL, and with its author's agreement.
- *Applications on a slider or a circle:* every application of a kind (mesh scalers, compressors,
  image viewers) placed on one ring, arranged by what it does, with a measure of how short it is for
  what it does beside it.
- *Kolmogorov complexity as the guide:* the true shortest program for a job cannot be computed, only
  approached from above, so what can be tracked is an upper bound: the size of the smallest
  implementation found so far that passes the same tests, here a program's size once packed by Sieve.
  Recorded over time, it shows which code is near the shortest for its job and where there is slack,
  steering towards the shortest without claiming to have reached it.
- *Where it lives:* the catalogue of formats (§19.165) already holds entries with their measured
  figures; applications would be entries of their own kind, linked to the formats they read and write.
  Planned only.

**19.182 The catalogue as another node viewer (Edward, 12 November 2026).** His words: "so in essence
like the node viewer for items and maps, the catalogue just becomes another node viewer interface that
helps link all information from all angles to all information from all angles", and, on the CATALOGUE
page, "it also needs an outline like the other side, which slides in, so that it is its own sliding
mechanism and has its own scroll bar on the inner side mirroring the other side from two slide in
interfaces".
- *One kind of view for every web:* the hallway's node graph (O: items and maps) and the CATALOGUE
  page's ring of doors show the same thing, nodes and their links, so they can become one viewer that
  any web plugs into: the catalogue of formats, the dimensions (`sieve dimensions`), maps, characters
  and species, the RAID's chains of versions. Each web gives its nodes and links; the viewer draws and
  walks them, from any node to any other.
- *Built the same day:* the right-hand panel's inner edge drawn full height, as the settings panel's
  is, and a scroll bar on that inner edge where a page holds more than fits (YOURS scrolls by wheel or
  by its rows; the CATALOGUE ring shows where its window of doors sits among them all).

**19.183 A catalogue of spoken languages, as a LANGUAGE page for characters (Edward, 12 November
2026).** On the CATALOGUE page: "the catalogue I believe is a language catalog for character creation so
the title might be better as language, it is just one catalog amongst many catalogs", and then "then its
data and styles can be transposed into a formats catalog for speech and it can become the spoken
language language tab for character creation".
- *What was built, and its name:* the page built that day is the catalogue of formats (§19.165): file
  kinds, scripts, alphabets, importers and Sieve's own formats, with one language (English) among 170
  entries. Its tab is now called FORMATS, one catalogue among the many to come.
- *The language catalogue:* the same kind of entry (`sieve-format-v1`) and the same ring of doors,
  filled with spoken languages: each language's family, its scripts (linked to the script entries
  already there), its sounds (the voice's phonemes, CMUdict's for English), and what Sieve can do with
  it (dictionaries, the voice). Its own folder of hand-written entries, as data/catalogue is for
  formats.
- *A LANGUAGE page for characters:* on the right-hand panel beside CHARACTER, a character chooses the
  languages it speaks from that catalogue; the choice is kept with the character (a link from it to the
  language entries, §19.152), so a speaking character (§19.147) knows what it speaks.
- *Spelling:* British English throughout these documents: "catalogue" is both the noun and the verb;
  "catalog" is the American spelling.
- *A System Viewer (Edward, the same day):* "the formats and list it is currently built to represent or
  view stays and it becomes its own network or node or link map to be displayed or accessed through a
  similar in hand viewer for systems a System Viewer for the main and pause menu which provides access
  to language and formats as two already known systems to be charted". So: a **System Viewer** item on
  the main menu and the pause menu, opening one viewer (the ring of doors, §19.182) over whichever
  system is chosen; FORMATS (the catalogue of formats, as built) and LANGUAGE (the spoken-language
  catalogue, above) the first two, each its own web of nodes and links, and more systems (the
  dimensions, characters and species, the RAID's chains) charted by the same viewer as they come.
- *Built the same day:* **System Viewer** on the main menu and the pause menu, with Formats and
  Language; the setup menu opens with that system in the right-hand panel, and S turns from one to the
  other. (*12 November 2026:* a view of its own since, the whole window, not the setup menu's panel.) The language catalogue's first entries are in data/languages: thirteen languages (English,
  Welsh, French, German, Spanish, Greek, Russian, Hindi, Arabic, Hebrew, Chinese, Japanese, Korean) in
  five families, each with its ISO 639 codes and linked to the scripts it is written in; the two
  catalogues are one web of 187 entries.
- *The LANGUAGE page, built the same day (Edward: "spoken language selection as with any other number
  of systems like this that get tacked onto character creation become a part of the save data and thus
  the infinite raid format"):* beside CHARACTER, the languages by family, ticked with Space; kept in
  the settings file and part of the character's save, `sieve-save-v1` (SPECIFICATIONS §12.0n): the
  character, its animation and its languages named together by one SHA-256, each system that joins
  character creation adding a line of its own; kept in the RAID as "your save" where its drives are
  set. The panel's tabs: CHARACTER, LANGUAGE, YOURS, SYSTEMS.

**19.184 A recorder and replay for every game, and a film studio built in (Edward, 12 November
2026).** His words: "note down a save recorder and playback system like that used in MGO2PC and other
games for replaying matches and games across dimensions or games, like replaying it from all of its
recorded positions and parameters as well as execution over time, similar to how replays in GTA4 film
studio and rocket league work, this would work for hallway and be scalable through all games systems
in the long run", "essentially a film studio built in that allows for making movies and replaying
scenes throughout all games", "this would require input recording across servers throughout the course
of matches in order to ensure playthroughs are properly catalogued and can be used to adjust or make
movies out of the gameplay such as machinima, with built in video editing and colour grading tools
etc", and "that would be a new video editor in the main and pause menu".
- *Recording as inputs over time:* a replay kept as the starting state and every input in order (with
  its time), played back by running the same steps again, so a whole match costs a few bytes a second
  and every camera angle can be chosen afterwards. This only replays exactly where the game is
  deterministic (the same inputs give the same result on every machine), which Sieve's own systems are
  built to be (addresses, characters, the seed walk, animations). Where it is not, positions are
  recorded as well as inputs.
- *In Sieve already:* the hallway's scripted keys (`--press`, carrying their scancodes), Manual
  Navigation's log of every step (Keep a log, Load Log), the chat logs, and animations as turns per
  bone per frame (§19.170): each is a recording of one kind. A hallway replay would join them into one
  record (the start: line, address, pose; then each key and mouse movement with its time).
- *Across servers and games:* a match's inputs recorded by its server and kept in the RAID (§19.176)
  as a save of its own, named by its SHA-256, so a playthrough is catalogued and can be checked; each
  game that takes part records its own the same way.
- *A film studio (a video editor on the main and pause menus):* replays as clips on a timeline, free
  cameras placed along them, cuts, titles, colour grading, and the result written out as video (the
  hallway's exports already write video with ffmpeg), for machinima.
- *A first step that could be built:* recording the hallway's own inputs with their times while
  walking, and playing them back exactly (the hallway is deterministic given its settings and its
  start), as `--record FILE` and `--replay FILE`. Planned only. (*Built 13 November 2026:* `sieve-replay-v1`
  (SPECIFICATIONS §12.0q), `--record`, `--replay`, F12 in the hallway, recordings packed as `.sieve`
  (Edward: "those text files should also get sieved just like sfilter files"), the main menu's Replays
  with a cache box for each (Edward: "a checkbox for caching them so that they don't always have to be
  decoded, this would be useful for large replays, ie an individual checkbox for replay in the list"),
  and F12's limits (Edward: "F12 should be the default stop/start recording key, we should have a
  default max time limit for recordings as well as persistent checks to ensure that there is enough
  disk space for the current recording": 30 minutes, and the space looked at every five seconds). A
  walk of 132 frames, 1,612 bytes as text and 256 packed, ends in the same place played from either,
  and is written again byte for byte while it plays. Next: playing from anywhere (moving the hallway
  to a recording's start), free cameras, and the film studio.)

**19.185 Every document and format type built on Sieve's own principles, scaling with its state space
(Edward, 12 November 2026).** His words: "every document type and format type from sfilter to this
should be based on the sieve principles and be dynamic or scalable through their local and or relevant
state space".
- *What it asks:* each of Sieve's own formats, from the filter plugins (`.sfilter`, docs/FILTER-PLUGINS.md)
  to the newest (`sieve-format-stat-v1`, §12.0p), is not only a record Sieve reads but a state space of
  its own, held to the same principles as the lines: every possible document of that type has an
  address, the valid ones are the survivors of that type's filters, they can be counted, ranked and
  compacted exactly, and each type grows (more fields, longer values, more records) by a setting rather
  than by a new design.
- *Already true of many:* they are canonical text, one form for one meaning, read back and refused
  when not in it (characters, animations, saves, RAID manifests, catalogue entries, measured figures);
  each is named by its SHA-256, so a change is a new version and the old one stays readable (§19.137);
  several have a stated roof (§19.159). And the filter plugins already describe a walk through states,
  one symbol at a time, from which the engine counts, ranks and compacts the survivors.
- *What would follow:* each format's grammar written once as a filter plugin over its alphabet, so the
  valid documents of that type are exactly that plugin's survivors on a line: a line of every
  `sieve-format-stat-v1` record up to a length, every `sieve-save-v1`, every `.sfilter` itself. Then a
  format's size (how many documents it allows at each length) is counted, not guessed; its documents
  can be walked, sieved and compacted in the hallway like any other line; and its scale is the line's
  length and the plugin's parameters ("local": one format's own space; "relevant": the spaces it is
  part of, through the catalogue's links). The catalogue of formats (§19.165) would then hold, for
  each of Sieve's own formats, its plugin and its count beside its measured figures.
- *Planned only:* nothing built yet. A first step: one small format's grammar (the measured-figure
  record) as a plugin, checked by the oracle to admit exactly the lines `read_format_stat` admits.
  (*Built 13 November 2026:* `data/filters/format-stat-record-v1.sfilter`, 337 states on an ascii95
  line, FILTER-PLUGINS §22. Its counts are the oracle's to the last digit (5,931,900,000,000 records
  of the shortest length, 62, which is 10^8 dates times 39^3 one-letter words), and on 69,458 lines,
  records and records changed by one character, every verdict is the reader's but one kind: keys out
  of increasing order, which a walk cannot see and the reader alone refuses (403 of them). That is the
  honest limit of a grammar as a walk: a rule comparing one field with another whole field is not one.)
- *Kept on disk as .sieve, read on the spot (Edward, the same day):* "we should also be pulling sfilters
  and other sieve files like these catalogs or maps from the sieve state space in the beginning during
  installation, which I think already technically happens naturally through the process of creating a
  sieve installer anyway", and "I mean specifically as they are stored on disk locally, the .sfilters
  themselves can be .sieve files decoded on the spot to be read and interpreted, utilised or run as part
  of the state space generation". Today an installation carries `filters/` (and, from 12 November,
  `catalogue/` and `languages/`, which the release had left out) as plain files, and the loader
  (tools/cli/plugins.cpp) reads each `.sfilter` as text, naming it by its SHA-256. The step asked: the
  same folders may hold `.sieve` files whose members are `.sfilter` (or catalogue entries, or maps);
  each is opened in memory when the folder is read, every member checked against its SHA-256 as
  installing checks it, and each member then parsed exactly as a loose file is, its identity still its
  own text's SHA-256, so a filter is the same filter packed or not. Nothing is unpacked to disk. Planned
  only; it needs the reader to give a member's bytes without writing them (the installer's decoding,
  kept in memory), and a limit on what a `.sieve` in those folders may unfold to, as the viewer has.
- *Built for filters (12 November 2026, Edward: "build the .sieve filter loading next, that means the
  pipeline itself needs to save .sfilters as .sieve files and decode/encode them accordingly during
  creation and they would be cached for use, decoded on startup to be cached ahead of time"):* the
  Designer saves each filter with its lists as one `.sieve`; at start-up every `.sieve` in the filters
  folder is decoded once into the cache, checked at every start, and loaded as a loose filter is
  (docs/FILTER-PLUGINS.md section 21). *And catalogue entries and maps (the same day, Edward: "carry on
  with catalogue and map .sieve loading next"):* a catalogue folder may hold its entries as `.sieve`
  (`sieve catalogue --pack`), decoded at start-up into the cache by the same piece the filters use
  (tools/cli/sieve_cache.hpp) and read as loose entries; maps, `.sieve` since §19.93, are decoded as the
  hallway starts, on a thread of their own, and kept in memory for the Node Graph Viewer. The
  repository's own entries stay loose (they are read and reviewed as text); a release may ship them
  packed.

**19.186 Actions to animations, as a format of its own; a character creator on the main menu (Edward,
13 November 2026). The next idea to work on.** His words: "actions -> animations, will need to be a
format as well I think", "it will hold actions in-game to the animations that represent them", "so
that custom characters and the like can be loaded in", "if it doesnt have an animation it wont play
the animation and will inform the user that animation doesnt exist for this model or let them pick an
animation from their list of animations for that action", "this may require a charactor creator on
the main menu so that there is more accessibility and focus as well as room for more controls while
the setup one just stays simple but can link through to it".
- *The format* (proposed: `sieve-actions-v1`): canonical text named by its SHA-256, one line an action,
  each naming the animation that plays it by the animation's SHA-256 (or `none`):
  `action walk <sha256>`, `action idle <sha256>`, `action wave none`. The actions are the game's own,
  a fixed vocabulary (walk, run, idle, jump, take a book, turn a page, ...) kept as a catalogue entry,
  so "no such action" can be checked, and the list grows by a new version. The map is part of the
  next save version (`sieve-save-v1` holds the character, its animation and its languages today), so
  Ctrl+S and the RAID carry it with the rest.
- *A missing animation:* the hallway plays nothing for that action and says so once ("no animation for
  jump on this character"), never in a loop; where the store holds animations that fit, it offers
  them to choose from, and the one chosen is written into the map (a new version, as any edit is).
  Fitting is checked, not guessed: an animation fits a character when it has no more bones than the
  character has spheres, the rule `pose()` already plays by.
- *Custom characters:* one loaded with its own map brings its actions with it; one without falls back
  on the default map where its animations fit.
- *A character creator on the main menu:* a view of its own, the whole window, as the System Viewer
  became (13 November 2026): the character large and turning, its spheres and held models, the list of
  actions with each one's animation played on demand, importing (Ctrl+I, Ctrl+K) and the store. The
  setup menu's CHARACTER page keeps its few quick settings (seed, limbs, ...) and gains a row that
  opens the creator.
- *To decide:* whether the actions are Sieve's fixed list alone or a character may add its own (emotes)
  as a later version; whether choosing an animation for a missing action edits the character's map
  (and saves it) or only this session's. Proposed: a fixed list first; a choice edits the map.
- *Built 13 November 2026* (Edward: "carry on with actions to animations next and character creator
  next"): `sieve-actions-v1` with the seven actions above (idle, walk, run, jump, take, read, wave),
  `sieve-save-v2` naming the map (v1 kept for saves without one), the map kept beside the settings
  (`actions/`, `[character] actions`) and in the RAID with Ctrl+S; the **Character Creator** on the
  main menu and from the CHARACTER page (C), the whole window, its actions chosen from the store's
  animations that fit, each played on demand; the hallway playing idle, walk, run and read by what you
  do, an action with none said once. Decided as proposed: a fixed list first, a choice edits the map.
  Still to come: jump, take and wave played when the hallway has those moments; a character's own
  actions (emotes) as a later version.

**19.187 Personal Dimensions open onto a world of their own: a grassy hill under a blue sky (Edward, 13
November 2026).** His words: "currently personal dimension just goes through to the setup page, it
should go through to an empty 3D world, the default starting world should be a grassy hill with blue
sky and clouds, something akin to the windows xp background".
- *Today:* the main menu's Personal Dimensions opens the setup menu with YOURS in its right-hand panel
  (§19.152, §19.157); your stored characters stand in the hallway's corridor.
- *Asked:* Personal Dimensions goes into a 3D world of its own, empty but for you (and your
  characters), its default a rolling grassy hill, a blue sky and white clouds: the feeling of that
  famous desktop, made from Sieve's own shapes, not a copy of the photograph (which is someone
  else's), so drawn from a seed like everything else: the hill a smooth height field, the grass a
  colour gradient (and later blades), the sky a gradient from deep blue overhead to pale at the
  horizon, the clouds soft shapes drifting.
- *How it fits:* a world is already a line (WORLDS, §19.85 onwards), so the default world is one
  address on it, and a person's own world is theirs to change from there and keep (the store, the
  RAID); the character, its actions (§19.186) and later others' characters walk in it.
- *Planned only;* after the music visualiser rings (ROADMAP "Next, in order"). (*Built 13 November
  2026* (client/personal_world.hpp): the main menu's Personal Dimensions opens a world of its own, a
  hill of a few broad swells from the world's seed (0 by default), shaded by the sun and hazed into the
  sky with distance, clouds of soft discs drifting, your character before you and the store's round the
  top; WASD and the mouse to walk, Esc back to the main menu. The setup menu's YOURS page is still there
  from the setup menu.)
- *Objects placed from a catalogue (Edward, the same day):* "that should be a 3D space of course, the
  personal dimension should have the ability to place objects from a catalog in the world space", and
  "the default placeable objects can just be the sieve books for shelves, that makes sense, however that
  means they also need sieving and preserving somewhere aside from the overridable ones". *Built:* Tab
  opens the catalogue, one item a line (a sheet of paper for pages, a canvas, a record sleeve, a tape
  case, a book, crates and a box), at their size on the shelves; Enter places one on the ground 1.5 m
  before you, turned to face you, Backspace takes away the nearest within 8 m. They are read from
  `data/objects/shelf-items.sieve` (the eleven items' .obj and .mtl, 22 files of 21,060 bytes, packed as 2,014; with the bookcase, 24 files of 28,370 bytes packed as 2,885),
  read in memory with every member checked against its SHA-256, so Real Graphics' meshes folder, which
  anyone may change, does not change them. What is placed is kept as `sieve-personal-world-v1`
  (SPECIFICATIONS §12.0r) in `worlds/personal.world` beside the settings and read back when the world
  opens. *Next:* a bookcase to place, items placed on shelves, items of your own beside Sieve's, and the
  world itself as an address on the WORLDS line. (*The bookcase and shelves built 13 November 2026:* the
  corridor's left-wall case (templates/bookshelf.obj, packed in shelf-items.sieve with the items) is the
  catalogue's last object, stood as every object is; things rest on the highest surface beneath them
  that faces up (the grass, a shelf board, the case's top), held or dropped, and the crosshair rests
  what you hold on the first such surface it meets within 3 m. Measured here: pages dropped from 17 m over the
  case come to rest on its top (2.85 m above its foot), a record sleeve dropped onto a shelf rests on
  its board; a board thinner than one step's fall still stops what falls (each step looks for surfaces
  from where it began). Drawing: objects' faces turned away are left out and the rest sorted far first, and
  a character behind something placed is hidden, sphere by sphere.)
- *What it is for (Edward, the same day):* "the main goal of the personal dimensions is to provide a
  lens for users in which they could create their own interpretation of the sieve space, they could
  also be used to make games and or just fun spaces to hang out in". So each step here is weighed by
  that: placing Sieve's own items is a first way to arrange the space as you see it; a world of your
  own making can become a game (with Game Systems, §19.189) or simply somewhere to be.

**19.189 AI Explorers in the state space, under Game Systems (Edward, 13 November 2026).** His words: "We
should make sure online isn't a requirement and that it is off by default as a setting", "As well as
infinite raid which I believe we already have off by default", "That way all the online systems aren't a
requirement", and "We also need another toggle for AI Explorers in the state space, which would be under
a sub toggle for the game systems".
- *Built:* Settings > Online (`[online] enabled`, off by default). While it is off nothing goes out or
  is answered over the network: Infinite RAID's sharing, keeping others' saves and checking shares each
  say that Online is off, even where they are ticked; the RAID on your own drives is not online and is
  unaffected. Settings > Game Systems, a screen of its own, holds AI Explorers (`[game] ai_explorers`,
  off by default), shown as "(to come)".
- *What the toggle is for:* AI explorers walking the state space as characters (the harness's models,
  §19.186's actions), seen in the hallway and in personal dimensions. Nothing reads the toggle yet.
- *Planned only* beyond the toggle.

**19.190 Favourites in every personal dimension, and one catalogue of them all (Edward, 13 November
2026).** His words: "users should also be able to add favourites into their personal dimensions, which
could be real items picked from the shelf, this means each dimension needs a favourites system and
overall a combined favourites system to catalog them all, the media player already has a favourites
list for example".
- *Today:* the Media Player keeps favourite melodies (`sieve-favourites.sieve`: each a MIDI file, with
  an index of their notes); a personal dimension holds only the shelf items' kinds (§19.187), not real
  items.
- *Asked:* a real item, picked from a shelf in the hallway (a page, a picture, a melody, a model, a
  file), kept as a favourite of a dimension and placed in its world as the thing itself (its cover on
  its shelf item); every dimension its own list; and one combined catalogue listing every favourite
  everywhere (the Media Player's included), saying which dimension holds each.
- *The design proposed:* a favourite is a reference, not a copy: its line, its address (kept as the
  line's own unit, so it is the item exactly) and its title, named by the SHA-256 of the line and
  address, so the same item kept twice is one favourite. Each dimension keeps its own list in its world
  file; a world that holds a favourite is written as `sieve-personal-world-v2` (v1 stays readable, and
  a world with none is still written as v1). The combined catalogue is an index beside the settings
  (`favourites/`), rebuilt from each dimension's list and the Media Player's, never a second copy of
  either, so the lists cannot drift apart. Large items (a file on the binary line) are kept packed as
  `.sieve`, as recordings are.
- *Planned;* next after the world's first catalogue (Edward, the same day). (*Built 13 November 2026:* H with
  an item in hand in the hallway keeps it (H again lets it go); `favourites/SHA.item` and
  `worlds/personal.favourites` as above, but the dimension's list is a file of its own beside the world
  rather than lines in it, so keeping a favourite never rewrites the world; your dimension's catalogue
  lists them after the shelf items; placed (`fav:SHA`, a v2 world) each stands as its line's shelf item,
  and O on it opens the hallway on its line with the item in your hand, when the line is shaped as it
  was; Main menu > Personal Dimensions > Favourites lists every one, the Media Player's melodies among
  them. *Left:* each favourite's own cover on its shelf item, files and models' weights as favourites,
  and favourites of other dimensions as they come.)

**19.192 Holding, placing, rotating and dropping things in a dimension; authors and visitors (Edward, 13
November 2026).** His words: "users need a way to place and rotate items in the dimensions as well",
"they would pick it up and then be presented with the option to either read it, other players would go
straight to reading the item in its associated viewer, IE the dimension it is from and the viewer
associated with it, but authors of the dimension would be allowed to pick it up or open it with two
different key presses", and "picking it up would pick it up to the cursor, users could then drop or
place it, placed items would stay at their coordinates, even floating where as dropping it would drop it
and leave its physics flag on so that it can be kicked or bounce / interact with other objects".
- *Today:* an item is placed on the ground before you, facing you (§19.187), and can only be taken away.
- *Asked:* in a dimension, looking at an item gives two keys to its author: **open** it (read it in its
  own viewer, the one of the line it comes from) or **pick it up** to the cursor. A visitor (anyone not
  an author of the dimension) has only open. What is held can be turned (yaw, pitch and roll), then
  **placed**, staying exactly where it is held, floating if it is in the air, or **dropped**, falling
  with its physics on, so it can be kicked, bounce and knock into other things.
- *What it needs:* each object's place in three dimensions (x, y, z) and its turn in all three axes, a
  physics flag, and for a favourite (§19.190) the item it is; the world's authors, by the identity the
  RAID uses (§19.178), a single author (you) until that exists; and a small physics step (gravity,
  ground and box contacts, rest) run the same on every machine, so a dropped thing comes to rest in the
  same place everywhere (fixed time step, as replay's, §19.184).
- *Built 13 November 2026 (the first part):* E (or a click) picks up what the crosshair is on into your
  hand, two metres along the crosshair; R, T and G turn it about the vertical, its side and its front
  (15 degrees, Shift the other way); Return (or a click) places it exactly there, floating if in the air;
  Q drops it, and a fixed 1/120 s physics step (gravity, the grass, a small bounce, friction, knocks when
  walked into, things pushed apart) brings it to rest; Esc puts it back. O opens (for shelf items it says
  they are not items of a line). Every world is yours alone, so you are always its author for now. Kept as
  `sieve-personal-world-v2` (SPECIFICATIONS §12.0r) once anything is off the ground, tipped or dropped;
  lines it does not know are kept and written back. Measured here: a page dropped from the hand comes
  to rest on the grass within 3 s; a book walked into is knocked 2.6 m on and rests. *Left:* turning as it
  falls (it falls upright as it was held), and visitors (with identity).
- *Order:* with favourites, in one new world version: `sieve-personal-world-v2` carries the 3D place,
  the three turns, the physics flag, favourites and authors together, so the file is not bumped again for
  each (v1 stays readable; a world using none of them is still written as v1).

**19.193 A face finder: faces no one can see taken away from placed objects, the original kept by link
(Edward, 13 November 2026).** His words: "a useful tool for the dimensions could be a face finder, take
our book for example, I deleted some faces from them which wouldn't be visible due to the shelf getting
in the way", "if an object is permanently placed, its origin point the original model can be catalogued
and then its current position can be raytraced to test if any of its faces can be deleted in order to
reduce complexity of the personal dimension", "the link through to the original object can start with
its hash, but needs to still directly link through to the original model", and "this might bloat sieve
personal dimensions, but it could or should also just result in another localised file that stores all
the bloat".
- *What it does:* for an object placed for good (placed, not dropped: §19.192), rays are cast at each of
  its faces from everywhere a viewer can stand (the walkable ground and the air above it, sampled
  densely, plus every direction from outside the world's bounds), against everything else placed. A face
  no ray reaches is never seen and can be left out; one reached by any ray is kept. Edward's books on
  their shelves are the case done by hand: the faces against the shelf and its neighbours are gone.
- *The original stays the original:* the object's entry names its original model by SHA-256, and that
  link must lead straight to the model itself (in the store, the catalogue, or the line it is an item of),
  never only to the cut copy. The cut copy is the original minus a list of face numbers, so it is checked
  against its original and can always be made again from it.
- *Where the cut copies go:* not into the world file, which stays small (places, turns, links). A
  separate file beside it, local to the machine (e.g. `worlds/personal.faces`, a cache), holds each
  placed object's face list; deleted, it is worked out again, so it is never part of a save or shared as
  the world. Anything moved, added or taken away near an object marks that object's list stale.
- *Honest limits:* sampled rays can miss a face visible only through a narrow gap, so the sampling is
  conservative (a face is kept when in doubt) and a gap smaller than the sample spacing is said; physics
  objects (dropped) are never cut, since they move and turn.
- *Order:* after holding, placing and physics (§19.192), since only placed objects are cut; it uses the
  same 3D places in `sieve-personal-world-v2`.

**19.194 Modelling tools inside the dimensions (Edward, 13 November 2026). An idea for now.** His words:
"eventually we should also build all types of modelling tools into this system or dimension, like
raising ground, subdividing, extruding, boolean cutting etc but this is just an idea for now".
- *What it would be:* the tools of a modelling program, used in place, in a dimension: the ground raised
  and lowered (the height field's swells, then a painted offset of its own), faces subdivided and
  extruded, objects cut and joined by booleans (union, difference, intersection), each change one step
  of an editable history.
- *How it fits:* a model made or changed so is an item of the models line like any other, and each step
  is data + ruleset (§19.191): the original model's hash and the operation, so the result can always be
  made again from its original, as the face finder's cut copies are (§19.193). The world's ground,
  changed, is kept as the seed plus the strokes on it, not as a new mesh.
- *Honest limits:* booleans on triangle meshes need watertight inputs and careful arithmetic to come out
  the same on every machine (fixed-point or exact predicates), and the models line's roof (8 vertices,
  12 faces today) holds only small results: larger ones are imported models (§19.159) kept in the store.
- *Idea only;* after holding and placing (§19.192) and the face finder (§19.193).

**19.195 A spinning hypercube behind the main menu, any visualisation behind any menu, and menus people
arrange themselves (Edward, 13 November 2026).** His words: "hypercube spinning in the background of just
the main menu, all other menus can have this option as well, disabled by default", "this could be a
toggleable option so it could be turned on and off, it could also be replaced with other visualisations",
"this allows for the sieve of potentia itself to be the framework which all other interpretations of it
naturally bolt onto", "such that people could set it up in their own way, move the options around etc",
and "we could also adjust the CI to be simpler by tagging each option menu with a number, then the CI
just has to find that number rather than go up or down a select set or number of times".
- *Asked:* a tesseract (the four-dimensional cube) turning slowly behind the main menu, on by default
  there; the same as a setting for every other menu, off by default; and in place of it, any other
  visualisation (the rings of §19.175, a world of §19.187, one made from a ruleset of §19.191).
- *The idea under it:* Sieve as the frame that every interpretation bolts onto: the background, the
  rows and their order are a person's own arrangement, kept as their settings (and sharable, a layout
  like any other data), while the systems behind the rows stay the same.
- *How it would work:* a tesseract is 16 points (each coordinate ±1 in four dimensions) and 32 edges
  (points differing in one coordinate); turned in two planes at once (xw and yz, say), then projected
  from four dimensions to three (a perspective divide by the distance in w) and from three to the
  screen, drawn as lines behind the menu at a low brightness so the text stays readable. It follows Real
  Graphics as everything does: lines when off; when on, the same with its cells faintly filled.
- *Menus by name (already built):* the CI no longer counts rows. Each row has a name (`Item:settings`,
  `Item:raid_listen`; §19.188's first step, 13 November 2026), and a script names the row it means. A
  name is better than a number here: a number given by position changes when the row moves; a fixed
  number would work but says nothing to whoever reads the step, while the name is also the row's
  language key. So menus can be rearranged, by us or by anyone in their own layout, and no step changes.
- *Planned;* the tesseract first (a small piece), then the setting for every menu and the choice of
  visualisation, then menu layouts of a person's own (after §19.188's reorder, which becomes the
  default layout).

**19.196 Menus and layouts as data: a state space of the interface, and everything dynamic and modular
(Edward, 13 November 2026). An idea for now, and a rule to build by.** His words: "all sliding windows
should work the same way as they do in the sieve setup menu throughout the system", "sliding menus and
menu menus as well as any other type of menu or layout can become a system of data types that can
further be used to shuffle up the way things are laid out, so people could remove or disable menus at
will or change the way they are presented to the user", "this would slowly fold out into another state
space for UI or accessibility, not sure which state space currently but it would fold out into one like
many others, the name isn't that important", "the aim should be for everything and anything to be
dynamic and modular", and "such that in theory anyone could turn it into anything, this is something we
should always keep in mind when building systems for sieve".
- *Built toward it, the same day:* one sliding panel for everything (`client/slide_panel.hpp`: the setup
  menu's 180 ms slide and its tab at the edge); the setup menu's settings panel takes its timing from it,
  and the personal world's catalogue and Place panels are made of it. Menu rows have names (`Item:ID`,
  §19.188), so a layout can name what it moves.
- *The idea:* every menu, panel and row a piece of data (what it is, where it stands, whether it is
  shown, how it is drawn), so a person's layout can move, hide, add or restyle any of them, and layouts
  are kept, shared and walked like any other data: a state space of interfaces, with accessibility as
  one of its directions (larger text, read aloud, fewer rows; §19.143, §19.160). Its name can wait.
- *The rule kept from now on:* build each system so it can be taken apart and put together otherwise:
  its parts named, its state as data, its look apart from what it does, nothing fixed that need not be.
  Written into the ROADMAP's rules.
- *A first step built (14 November 2026): the pause menus as data.* Edward's testing: "the personal
  dimensions should open a pause menu like the hallway ... setting one up specifically for the personal
  dimensions as a default it wouldn't include hallway tools, but they could be enabled by the user".
  Every row any pause menu can have is one list (`client/pause_rows.hpp`), each saying where it is shown
  (the hallway, your dimension) and whether it is a hallway tool; the hallway's pause menu and your
  dimension's both read it, a row's words from the language file (its own words in one place, where it
  has them: `pause.<id>.personal`). Your dimension's leaves the hallway's tools out until Personal
  Dimensions > Features puts them in. *Next:* the main menu's and the setup menu's rows the same way,
  and the rows each place shows chosen by the person, not only the hallway's tools on or off.

**19.197 A map and a compass in the personal dimensions, each a feature to turn on or off (Edward, 13
November 2026).** His words: "a map and a compass would be worthwhile features to add to the personal
dimensions system, these should be accessible and togglable under a dimension -> feature settings page
so that they can be turned on or off for users that do or dont want them". *Built the same day:* the
compass along the top (the 120 degrees about where you face, a tick every 15, N, E, S and W, the bearing
in degrees; north is +z, the way a world first faces you), the map at the top right (the whole world
from above, north up, the ground shaded by height or, with Real Graphics off, drawn as contours; what is
placed as dots, yellow while a dropped thing still moves; your characters as rings; you as an arrow),
and Main menu > Personal Dimensions > Features, where each is turned on or off (`[personal] map`,
`compass`, both on by default). The features page is the place for every later one (a clock, the
grid, a trail of where you walked), each its own row, as §19.196 asks. *Next:* the map zoomed in and
out, places marked on it, and other people's dimensions' features as their authors set them.
- *The map and what is over your head (Edward, the same day):* "we will also need a way to determine if
  line of sight is blocked for minimap rendering, for example a roof above a player, in this instance the
  roof would not be drawn unless it was underneath the player, this would allow for dynamic mapping of
  building interiors onto the minimap". So the map draws, at each place, the highest surface that is
  still below you, not simply the highest: a roof above your head is left out (it hides nothing on the
  map), a floor under you is drawn, and walking inside a building shows its rooms; on the stairs the map
  changes floor with you. How: for each map cell, the surfaces there (ground and objects' upward faces)
  sorted by height, and the first under your feet (with a margin of a storey, so a step does not flicker)
  is drawn; walls are drawn where a surface drawn and one not drawn meet. Cheap enough to work out
  again whenever you change level, not every frame. Waits for things to build with (the bookcase, your
  own objects, the modelling tools of §19.194): today the world has ground and small objects only.

**19.198 A door that makes a maze: an object with settings, its space inside it or in the world (Edward,
13 November 2026). An idea for now.** His words: "a default object we could place in the world is a door
with a maze generator, the door would have settings and would generate a maze space accordingly, the
maze could either be in its own 'dimension' IE : not physically in the personal dimension space or it
could be a part of the personal dimension space, this would be toggleable", "many togglable settings
like this would only be accessible to the author of the personal dimension", and "realistically it is
just door(data) + ruleset(mazes) so it already follows along from the core concepts and systems
naturally".
- *What it is:* a door in the catalogue. Placed, it carries settings (the maze's size, its seed, its
  kind of cell: squares, hexagons, the layouts of §19.191; how winding; one way through or many) and
  one switch: **its own space** (walking through the door takes you into the maze, a dimension of its
  own, and its far door brings you back) or **in the world** (the maze's walls stand on the hill, beside
  the door). The same seed and settings make the same maze on every machine, as a world's seed makes
  its hill.
- *How it fits:* data + ruleset (§19.191): the door is the data, a maze generator (a recursive
  backtracker, Prim's or Wilson's algorithm, each a ruleset) is the ruleset, and the maze is the item
  they make, kept as the door's settings, never as its walls. A maze in its own space is a dimension
  like any other, so it has its own features (§19.197), its map drawn as it is explored.
- *Authors only:* the door's settings, and the like for every object with settings, are changed only by
  the dimension's authors (§19.192); a visitor walks the maze as it is set.
- *In the world file:* a v2 `place door ...` line, followed by its settings as a `settings` line naming
  that object (a later line kind, kept by older builds as §12.0r's v2 keeps every line it does not know).
- *Where its rules come from (Edward, the same day):* "its rules can be pulled from the fractal
  dimension accordingly, the fractal dimension itself would be fractal in nature as there would be sub
  dimensions for things like maze generation algorithms and much more". So the maze generator is not
  written into the door: it is chosen from the fractal dimension (§19.191), which is itself made of
  dimensions (mazes, tilings, L-systems, iterated function systems, and more as they come), each a
  line of its own rulesets, each able to hold dimensions of its own; the door names the ruleset it
  uses by its address, so a new maze generator found there is one a door can use at once.
- *Idea only;* after favourites (§19.190) and the layouts of §19.191, whose cells it uses.

**19.199 Every key in one controls registry: each system names its keys, they gather under its section,
and every one can be changed (Edward, 13 November 2026).** His words: "we could and should also add a zoom
system to the minimap +/- keys by default", "as with any and every key throughout the entire project and
its many systems, they should get mapped back to a section so these keys would be listed under minimap
controls a sub section of the controls listing", "this should be dynamically in such a way that when we
add new key press handlers to new systems, the system itself has a primary tag and it can funnel/filter
itself upstream to the controls listings", "so that we have a single system in place which can extend
through all current systems and future systems", and "this would mean we could add new keys to something
and they would naturally just appear on the controls list under "movement/vehicles/sieve setup
menu/media player menu" etc so that all keys are adjustable through and through, dynamic, procedural,
modular, as per everything".
- *The registry:* one table of actions, each with a section path (its system's tag, e.g. "personal
  dimension/minimap"), a name ("zoom in"), its default keys and its description. A system declares its
  actions where it is written and asks the registry "was zoom-in pressed?" rather than testing a key
  code, so a new system's keys appear in the controls listing under its section by themselves.
- *Changing keys:* Settings > Controls lists every section and its actions; a key changed there is kept
  in the settings file (`[keys] section/action = KEY,...`, only what differs from the default), and a
  key used twice in one section is said. The footers and help lines read their key names from the same
  registry, so they always show the keys as they are set.
- *First:* the registry, the minimap's zoom (+ and -) as its first entries, and the personal dimension's
  keys moved onto it; then each other system in turn (the hallway, the setup menu, the Media Player,
  ...), each one its own commit so nothing is changed at once that cannot be checked.
- *Built, its first step (13 November 2026):* `client/keys.hpp` (the registry: actions declared by
  section, asked by what they do, their keys kept as `[keys]` overrides); the personal dimension's 28
  actions on it in four sections (moving, the catalogue, holding, the map), the map's zoom among them
  (+ and -, five steps, the whole world to 40 m across); its footers and aiming hint built from the
  keys as set; Settings > Controls > Keys listing every action by section, Enter then a key to change
  it, Backspace for the default. *Next:* the hallway's keys, then the setup menu's, the Media Player's
  and each other system's, one at a time.

**19.200 Every item opens in a viewer that already exists; where none does, it is flagged (Edward, 13
November 2026).** His words: "all items should always default to using a viewer that already exists, if
one doesnt exist it should flag it to us and tell us a new viewer needs to be created if that data type
doesnt already have one", "so O on a model in personal dimensions would open it in the 3D model viewer
for the most part, sometimes like with books, you might need to pick a viewer or sub item to determine
its viewer". And: "overall this system will help us map out more datatypes, discover new dimensions to
be charted and fold the system in on itself to link everything from every conceivable angle".
- *The rule:* opening anything (O in a dimension, Z or a click in the hallway) goes to the viewer made
  for its data type, on the view that suits it first; an item with several (a book: its pages, cover,
  title) opens on one and offers the rest as tabs. Where its type has no viewer, it is shown in the
  plainest form (its bytes), the viewer says that a viewer for this type needs making, and the type is
  noted, so the list of viewers to make writes itself as people meet them.
- *Built, its first step (13 November 2026):* a **3D** view (models and worlds open on it, turned with
  the arrows or WASD; the .obj text and the picture as tabs); O on a favourite opens it in hand and in
  its viewer; models, worlds and books can be favourites, kept by their place on the line (so one kept
  from a compacted line opens with the filters off); a file, which has no viewer of its own yet, is
  shown as hex and its kind (file-kinds-v1) noted once in `viewers-needed.txt` beside the settings.
  Then files by their kind: a table (kind to viewer, client/viewer.cpp) sends PNG, JPEG, GIF and BMP to
  the picture viewer (sieve-view's reader, picture_source.cpp, its limits checked first) and text kinds
  (TXT, BOOK, MANIFEST) to the text view; only kinds with neither are flagged. *Next:* sound (WAV,
  FLAC, MP3, MID) to the audio line's player, models (.obj, glTF) to the 3D view, archives to a listing
  of their members; and choosing a viewer or a part where an item has several.

**19.201 A documentation viewer on the main menu, laid out as a help library (Edward, 13 November 2026).
An idea for now.** His words: "we should also add a documentation viewer to the main menu", "which would
provide access to the docs folder and documentation within them", "this documentation would likely have
to be rewritten for the end user and laid out somewhat like the old help libraries for windows programs
in that everything is categorised".
- *What it would be:* a main-menu item opening the documents in a viewer of their own: a tree of
  categories on the left (getting started, the hallway, each system, the formats, the command line),
  the page on the right, with contents, an index and search, and links between pages followed in place.
- *What it needs:* the documents rewritten for the person using Sieve, not its makers (the present ones
  are written for development, and many are long notes and measurements), and split into short pages,
  each under its category. The developers' documents stay as they are, beside it.
- *Fits with:* §19.196 (menus as data: the categories and pages a data file the viewer reads), §19.200
  (a document is a data type; this is its viewer), and the existing /docs line and GO TO IT (task 98).

**19.202 Every screen and feature's screenshot made and kept up to date by itself, and each change
tested only as far as it needs (Edward, 13 November 2026). An idea for now.** His words: "automating the
creation and updates for each and every screenshot the website might contain which would showcase every
feature the project has", "each screen gets a screenshot as does each feature and those screenshots get
tagged with a version number", "when something is changed, altered or updated, we can automate the
creation of all the new screenshots based only on which ones need updating", and "organise tasks into
categories to reduce the amount of testing time required ... CI/unit/user and perhaps more where CI is
a change that requires testing the full or partial CI, unit is a change that requires the unit tests
and user is something that requires a user to test it such as myself".
- *Screenshots as data:* a list (a data file) of every shot: its name, the screen or feature it shows,
  the command that makes it (the hallway's `--screenshot` with its presses, as the CI steps already
  take them), the source files and data it depends on, and the version it was last made at. A tool
  makes each one, tags it with the version (in its name and in a listing beside it), and the website
  reads the listing.
- *Only what changed:* each shot remembers the SHA-256 of what it depends on (its files, the language
  strings it shows, the settings it starts from). After a change only the shots whose inputs differ are
  made again; a shot whose picture comes out byte-identical keeps its old version tag.
- *Tests by category:* each change says what it needs. **unit** (the unit tests alone: a function's
  behaviour), **CI** (the CI steps that reach the files changed, found from the same file-to-step map;
  or all of them for a format or a shared core), **user** (only a person can judge it: a look, a feel,
  a sound; listed for Edward with how to see it), and perhaps **docs** (nothing to run but the help and
  settings checks) and **screenshots** (the shots above). The CI runner would take a change's files and
  run only the steps that depend on them.
- *Fits with:* the CI's `Item:ID` presses (rows chosen by name, so a shot survives a menu's reordering),
  §19.196 (screens as data: the list of screens is the list of shots), and task 32 (the website).

**19.203 The Character Creator: limbs added, taken away and dragged by their joints (Edward, 13 November
2026). An idea for now.** His words: "the character creator needs the ability to add and remove limbs, as
well as drag them around by their connections, think spore or the sims".
- *What it would be:* on the Character Creator's drawing, a click picks a sphere (a joint); dragging it
  moves it, and the limb beyond it follows, its bones keeping their lengths unless a key stretches them.
  A key (or a button by the sphere) grows a new limb from the chosen sphere, and another takes away the
  chosen limb and all beyond it; a limb's joints can be added or taken away the same way. Mirrored
  characters (bilateral or radial symmetry) change on every side at once, as the symmetry says.
- *What it changes:* a character edited so is no longer made from its seed alone. It is kept as its
  spheres (as a character made from a glTF skeleton is, `sieve-character-v3`), its seed and settings
  shown as `-`, and its SHA-256 still names it; an edit can be undone, and its actions' animations are
  checked again against its bones.
- *Fits with:* §19.186 (the Character Creator and its actions), §19.194 (modelling tools in the
  dimensions: the same dragging of points), and §19.196 (one shared way of doing a thing: dragging a
  joint here and moving an object in a dimension should feel the same).

**19.204 A catalogue of personal dimensions to start from: templates (Edward, 14 November 2026). An idea
for now.** His words: "we need a catalogue of default personal dimensions you can start with", "from
matrix like holographic spaces to desktop background plains and with all sorts of others too", "so when
you create a personal dimension you could use that template as a base", "users could also pick and
choose other dimensions, users, groups or otherwise such as those from games to use as their template".
- *What it would be:* making a dimension begins by choosing a template from a catalogue: its ground (a
  grassy hill as now, a flat plain, a grid of light, none), its sky, its light, its rules (gravity, the
  map), and anything placed. A template is a world file (`sieve-personal-world-v2`) with its settings,
  kept in a catalogue as the shelf items are; another person's, a group's, or a game's world is a
  template too, copied as a start (its author named, as §19.192's authors are).
- *Needs:* the world's ground and sky made settings rather than fixed (the seed's hill one choice of
  several); several worlds of your own, each from its template (`worlds/NAME.world`, as favourites
  already allow); and, for others' templates, identity and sharing (§19.178).

**19.205 Faces drawn from both sides: a two-sided flag (Edward, 14 November 2026).** His words: "we may
want to make pages two sided from a rendering perspective, this will require a double sided flag so that
we know by default which faces need to be rendered", "this can be attached to the face counting system
which would only be used for finalising worlds to reduce their loads", "but the two sided flag would be
useful all around".
- *What it is:* each face says whether it is seen from both sides. A closed object's faces are one-sided
  (the side turned away is behind the object itself, so it is not drawn: as now); a thin thing's (a
  sheet of paper, a leaf, a flag) are two-sided, drawn from either side, a page's picture on both.
- *Where it lives:* in the meshes (an `.obj` cannot say it, so a material named so, or a list beside the
  model), read by every renderer (§19.207), and by the face finder (§19.193), which keeps a two-sided face
  wherever either side can be seen.

**19.206 An interface designer: every menu laid out by the person (Edward, 14 November 2026). An idea for
now.** His words: "an interface designer is required in order to facilitate the adjustment and creation of
all current menus to fit their personal dimension or state space", "this would be as simple as each menu
with an overlay that allows users to align things, move entries up or down, separate buttons from lists
and more".
- *What it would be:* over any menu, an overlay (a key opens it) where its rows can be moved up and down,
  hidden, grouped, aligned, and buttons taken out of lists; what is changed kept as data beside the
  settings, read by the menu as its layout. It needs every menu's rows as data first: the pause menus are
  (§19.196, 14 November); the main menu's, Settings' and the setup menu's are next.

**19.207 One renderer for every three-dimensional space (Edward, 14 November 2026).** His words: "we did
do some depth buffer and sorting for the hallway itself, where possible we should seek to use the same
renderers and systems between any of the 3D spaces", "which includes the character creator as well".
- *Where it stands:* the hallway draws its models with `MeshBatch` (client/mesh.hpp): a depth buffer,
  rasterised in bands on several threads. Your dimension had its faces sorted by their middles, and on
  14 November was given a depth layer of its own (client/personal_world.cpp), which is a second copy of
  the same idea; the Character Creator and the setup menu draw spheres sorted far to near.
- *To do (next):* MeshBatch made able to draw over what is already drawn (the ground, the sky), to take
  any turn (yaw, pitch and roll), to put a picture on a face, and to draw a wireframe's edges with the
  depth test; then your dimension's objects drawn by it and its own layer taken out; then the characters'
  spheres in the Character Creator, the setup menu and your dimension. One renderer: one place to speed
  up, and one place for two-sided faces (§19.205).

**19.208 O in your dimension opens the hallway's own viewer, in place (Edward, 14 November 2026).** His
words: "currently o opens an object and leads to the calculating / waiting screen", "this should seek to
just open the page as if it were the same viewer from the hallway itself", "it might need a
'calculating...' which is small and appears over the cursor while loading its content", "though its
content should have been loaded and cached by distance accordingly in the personal dimension".
- *Where it stands:* O on a favourite builds a hallway on its line (the calculating screen) and opens the
  item there. The viewer is the hallway's (client/viewer.cpp), tied to a built hallway's lines.
- *To do:* the viewer taken out of the hallway into a part of its own that any space can open over
  itself, given an item and what draws it; your dimension then opens it in place, over the world, with
  a small "calculating..." by the crosshair while the item is made. The items near you made ahead of
  time, nearest first, as the hallway makes the books it shows; a line's space (its shape, from the
  favourite's line settings) kept once made.

**19.209 The author's tools, the first a ground shaper; who may use them (Edward, 14 November 2026).** His
words: "we need a weapon or tool for adjusting the ground, we can start with just a simple highlighter
that points to the floor and highlights a sphere", "radius can be adjustable, then we can flatten, lower
or raise the terrain geometry accordingly", "this could be one of many tools that the author gets given
access to", "they can limit or prevent joining users from having access to them", "this will need a
friends list, ruleset panel, moderation tools etc", "it can work like a weapon and be accessed with key 1
on the keyboard as well as put away with # for now", "a simple shape / cylinder / gun that maybe has a few
pipes going into a barrel that use the same noise pattern used for the doors in the hallway".
- *The first tool:* key 1 takes it out, # puts it away (both on the key registry, §19.199). Held at the
  lower right of the view: a cylinder with a few pipes into a barrel, drawn with the door portals' noise
  (client/door_portal.cpp). Where the crosshair meets the ground a sphere is lit; the wheel (or + and -)
  sets its radius; one button raises the ground within it, another lowers it, a third flattens it to the
  height at its middle, each smoothly towards the edge. The ground's heights then are the world's own,
  kept in the world file (a v3, or heights beside it), not only made from the seed.
- *Who may use them:* every tool is the author's; a world's rules (a panel) say which tools visitors and
  friends may use, if any. Friends, groups and moderation wait on identity (§19.178).

**19.210 Saves kept across the RAID: sieved, split and shared in small parts, with limits (Edward, 14
November 2026). Before the RAID between machines is ever switched on for people.** His words: "currently
I think if someone turned everything on, that would equate to everyone trying to store everyones saves all
at once which isn't feasible", "it has to be a sieved system where the files are sieved and then split out
and stored in sectors just like real raid", "and tallied/calculated accordingly to ensure users only store
a bit of each and everyones saves, a chunk of the data itself rather than the whole or full save itself",
"I suspect storing individual saves would be the most efficient way to begin with until saves become
large", "so there might be a file size storage limiter", and why: "1 if I activate it, then it exists and
if people start using then it exists and could be bloated very fast", "2 when I activate it and people
start using it, there is no going back".
- *Where it stands:* the RAID between machines (§19.176, 19.150) is off unless both halves opt in, each
  behind its own red box. Sharing sends each save whole, sealed under a key only its owner holds, to
  every machine named in the settings file (`[raid] peers`: there is no finding of machines, so no one
  is sent anything they did not name); serving keeps others' sealed saves within limits set there
  (`space_mb`, 1,024 at first, and `per_peer_mb`, 64 at first, for any one address). So today it cannot
  grow by itself: every machine is named by hand. It is the step after this, machines finding one
  another, that would let it grow, and that is what this entry is for.
- *What it should be before anyone else uses it:* a save is sieved first (made as small as Sieve makes
  it), then cut into pieces with parity (erasure coding: k pieces of data and m of parity, any k of the
  k + m enough to make it again, as RAID 6 is with two), and each piece kept by a different machine,
  chosen by a rule everyone can work out (a hash of the piece and the machine), so no one keeps a whole
  save of anyone else's, and no one keeps more than their share. Every machine has a limit it sets
  (space given, and a largest piece), and the tally (who keeps what, how full each is) is checked before
  a piece is placed; a save larger than the limits allow is refused, said, rather than spread thin.
- *Small saves:* while saves are small, a piece per save is the simplest and costs least to look up; the
  split begins above a size (to be measured: the cost of a lookup against the cost of keeping copies).
- *Before it is switched on:* the format of the pieces and of the tally fixed and versioned (a
  `sieve-raid-piece-v1`), because once people keep pieces it cannot change without moving them all;
  a way to leave (a machine going away hands its pieces on first); and measurements of what it costs.
  Nothing switches it on by default, and the switch stays Edward's.

**19.211 Decentralised: no server needed, ever; servers only host (Edward, 14 November 2026). A rule for
everything, written into the ROADMAP's rules.** His words: "I would like for us to overall aim for this
system to be decentralised", "It shouldn't require a server", "Servers are things people could setup, but
they would host dimensions like personal dimensions, group dimensions or otherwise", "At any point the
need for a server steps into this project, it introduces the potential for people to bastardise
everything the project aims to do", "Which is a large range of things, primarily to help track
authorship and prevent plaigarism".
- *What it means for what is built:* every system works on one machine with no network, and between
  people's own machines directly (as the RAID between machines does: each machine named, each half opt
  in). Nothing has one place everything must pass through: no account server, no central catalogue, no
  single list of who made what.
- *Servers allowed, as hosts only:* anyone may run one to host dimensions (personal, group or other)
  for others to visit, as a convenience; a dimension hosted so is the same file as one kept at home, and
  can be moved to another host or kept at home with nothing lost.
- *Authorship without a server:* who made a thing is shown by the thing itself, not by a server's word:
  a creator's key signs what they make (as the RAID seals saves under a key only its owner holds), the
  item's SHA-256 names it, and its place in the state space is its address; so a claim of authorship can
  be checked by anyone, on any machine, with nothing but the item and the key's public half. This is
  where §19.178 (identity) and §19.192 (authors) should go: a key per person, kept by that person.
- *Where it bears now:* §19.210 (the RAID's pieces placed by a rule every machine can work out, not by a
  server's list), §19.204 (templates shared as files), Online (off by default, §19.189) and any finding of
  other machines (a list kept by each person, or found on the local network; never a central directory
  that all must use).

**19.191 Layout rulesets and a fractal dimension: each line laid out by your own rules, and rules that
make things (Edward, 13 November 2026).** His words: "as well as a series of fractal rulesets that allow
users to lay out the line of each dimension according to their own rules, hexagons, septagons, non
euclidean space, all these sorts of things, which would further access a fractal dimension that would
contain the rules for fractals", and "those fractals can then be used to generate images, worlds and just
about anything else using an object and a ruleset or a ruleset and a ruleset".
- *Today:* every line is laid out one way, a corridor: units in address order along shelves, tile after
  tile (SPECIFICATIONS §9). A personal dimension (§19.187) is a hill with things placed by hand.
- *Asked, first part: layouts.* A ruleset says where each unit of a line stands in a space, so a person
  can walk a line laid out their own way: on square or hexagonal cells in a plane, on heptagons
  (seven-sided cells meet three at a corner only on a hyperbolic plane, the {7,3} tiling: non-Euclidean,
  room growing exponentially with distance, so every step out reaches far more units than the corridor
  does), on a sphere, along a spiral or a space-filling curve, or by a rule of the person's own.
- *What a layout must keep:* every unit in exactly one place and every place one unit, so the place is
  still the address, as on the corridor (§6). A layout is therefore a numbering of its cells: from a
  starting cell, ring by ring outwards, each ring in a fixed order, the n-th cell holding unit n. Any
  tiling that can be walked that way gives a layout; a rule that leaves a cell out or reaches one twice
  is refused, as filter plugins are checked (docs/FILTER-PLUGINS.md).
- *Second part: the fractal dimension.* The rulesets are themselves units: texts in one small grammar
  (tilings, L-systems, iterated function systems, substitution rules), so the dimension of rulesets is a
  line like any other, every ruleset of the grammar up to its length at its address, walked, filtered
  (does it terminate, does it tile) and favourited (§19.190).
- *Fractal in itself (Edward, 13 November 2026):* the fractal dimension has sub-dimensions (maze
  generation algorithms among them, §19.198, "and much more"), each a line of rulesets of one family,
  each free to hold sub-dimensions of its own, so the dimension of rules has the shape of what it holds.
- *Third part: making things.* A ruleset applied to an object (a shelf item, a picture, a model, a
  world) makes another: an iterated function system drawn from a picture's pieces, an L-system grown from
  a model, a world's ground laid out by a tiling. A ruleset applied to a ruleset makes a ruleset: one
  substituted into the other, or one's output the other's seed. What is made is an item of its own line
  at its own address; the ruleset and the object are a short description of it, and the two together
  can be kept, shared and checked by their hashes as any item can.
- *Why (Edward, the same day):* "the reasoning for that is so that people can use these rules to help
  with the creation of their own universes/dimensions/interpretations of the sieve state spaces": the
  rulesets are tools for the lens of §19.187, each person's own reading of the same space.
- *Data and rulesets (Edward):* "rulesets / objects might really be data / rulesets, as an object is a
  piece of data and we already have data viewers that intrinsically link and span the state spaces". So
  the two forms are one: data and a ruleset, or a ruleset and a ruleset. Any item of any line is data
  (a page, a picture, a melody, a model, a file), already shown by its own viewer, so a ruleset needs no
  new kind of object to act on: it takes an item from any line and makes an item of any line, and the
  viewers show both. A ruleset is data too (a unit of the fractal dimension), which is why a ruleset
  applied to a ruleset is the same operation, not a second one.
- *Honest limits:* most units of any line are noise, and a short ruleset can describe only things with
  structure: rules name some items briefly, they do not shorten every item (the pigeonhole of §6). A
  hyperbolic layout cannot be drawn in flat space without distortion (the Poincaré disc shrinks the far
  cells towards its rim).
- *First step proposed:* `sieve-layout-v1`, a hexagonal layout of one line in a personal dimension
  (rings of hexagons round the item you stand at), with its numbering checked against the corridor's
  order; then {7,3} in the Poincaré disc; then the grammar of the fractal dimension. *Planned only;*
  after favourites (§19.190).

**19.188 The main menu reordered, from Start Sieve to Exit Sieve; Settings as Potentia Settings
(Edward, 13 November 2026). One of the final tasks.** His words: "At some point we need to move the main
menu options around", "Due to this being hard baked into the CI steps for ordering and navigation",
"We should just take note of it as an idea for now and rework it in the future", "My thinking is that
it should be arranged sort of in size from Start Sieve to Exit Sieve", "There may be new options to add
eventually and Settings should be renamed Potentia Settings so that it can fit accordingly".
- *The order asked:* Start Sieve, File Locator, Media Player, Filter Designer, AI Training Harness,
  Personal Dimensions, Encryption Engine, Manual Navigation, System Viewer, Settings (renamed
  **Potentia Settings**), Exit Sieve. Two added since that list was written, to be placed then: the
  Character Creator (§19.186) and Replays (§19.184).
- *Why it waits:* CI steps reach main-menu items by counting Down presses from the top (`--press
  "Down,Down,...,Return"`), so every move changes many of them at once. Done as one change, with a way
  to choose a row by its name (a press such as `Item:replays`, or `--main-menu-item ID`) first, so the
  steps no longer count rows and later moves cost nothing.
- *Planned only;* in ROADMAP's final tasks. (*Its first step built 13 November 2026:* `Item:ID` in
  `--press` chooses the main menu's or the pause menu's row of that name (client/press_names.hpp), the
  CI steps reach every row so (17 sequences in steps 04 and 10), and Back returns to the row it came
  from by name; so moving a row, or adding one, no longer changes any step. The reorder and the rename
  are what is left.)

