# Ideas

A triage of the long Gemini conversation — all three parts — plus anything else that has come up
and is not yet built. It is not a plan and not a promise. Each item says what it is and whether it
is a defect, a feature, an open question, or framing.

Nothing here is a commitment. Within each section the order is roughly by how much it is worth.

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

**2.4 No colour-blindness pass.** Seven lines, filter verdicts and the map all encode meaning in
hue, with no non-colour fallback and no simulation check.

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

**3.2 Exact counting under a model.** Counting every page whose information under the order-5 model
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

---

## 5. The hallway's shape

**5.1 One wall of shelves, everywhere.** The binary line is one-sided now, and the argument for
doing the same to the other six is good: there is only one address space, and a double-sided
corridor draws it twice. Open question to settle: what the empty side becomes — blank wall, the
portals, or something else. Worth trying on one line before committing.

**5.2 Rings instead of runs.** *The readout half is built.* Every line already loops, so where you
stand in one is a bearing: the compass in the corner draws the corridor as concentric circles —
binary outermost, the six it bounds, binary again innermost — with a needle at your angle on the
line you are on, a mark on every other ring at its own angle, and the bearing written out to
whatever precision **Angle Precision** in Settings > Graphics asks for. Zero is at the top, where
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

**6.5 Global parameters, and entries per shelf.** A shelf holds 128 units because 128 is a power of
two and the code asserts it. Making it a setting (128, 256, …) is possible, and 256 makes an item
number exactly one byte. The cost is that it changes what "tile N" means, so every address written
down under one setting is misread under another — which makes it a *versioned* parameter, not a
free one. Worth doing only with that clearly stated, and probably worth doing.

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

**7.5 Punctuation filters for `babel29` and `ascii95`.** Punctuation follows a word and is followed
by a space; for `ascii95`, a capital after a full stop. Countable with the word trie that already
exists. This matters: `lower27` throws away case and punctuation, so the books line currently holds
word streams, not sentences.

**7.6 Model-line filters.** Three tiers are specified in §12 and none are built: local per-face
constraints (rank exactly), small-V constraints such as "every vertex is used" (rank as a state
machine, 256 states at V=8), and whole-mesh properties — watertight, manifold, convex — which judge
but cannot rank, so they run in `mark`/`hide`.

**7.7 Ranking beyond two colours.** Image compact mode only ranks two-colour pictures, because
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

**7.12 Reels — video as a composition, not a line.** F frames of W×H is the same set of
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
