# Ideas

A triage of the 546-turn Gemini conversation, plus anything else that has come up and is not yet
built. It is not a plan and not a promise. Each item says what it is, where it came from, and
whether it is a defect, a feature, an open question or a piece of framing.

Nothing here is a commitment. The ordering inside each section is roughly by how much it is worth.

---

## 1. Defects and corrections (do these first)

**1.1 Entropy is measured against the wrong space.** `symbol-entropy-v1` and friends score a unit
against a fixed symbol count, but the alphabet is now chosen by the user and can be anything from
27 symbols to the whole of Unicode. A page drawn from `lower27` judged against a 21-bit code point
space gives a meaningless reading. Every alphabet needs to carry its own entropy baseline, and the
filter needs to use it. This is a correctness bug, not a feature.

**1.2 Padding is counted as content.** `model-information` and `symbol-entropy` count a unit's
trailing padding spaces as real characters, so a short text padded out to the unit length scores
as *more* text-like than the same text alone. Known, unfixed.

**1.3 The cased dictionary lets short gibberish through.** Folding one cased list to lower case at
load also folds SCOWL's abbreviations and chemical symbols, so the names list has 130 two-letter
tokens where the plain list had 69, and "zn" and "tb" now pass. Fix: a dictionary version that
keeps proper names and drops short all-caps tokens.

**1.4 Colour is carrying semantic load with no accessibility pass.** Six lines, the Edge, filter
verdicts and the map all encode meaning in hue. Nothing has been checked against protanopia,
deuteranopia, tritanopia or achromatopsia, and no indicator has a non-colour fallback.

**1.5 Layouts are built for 8-pixel text.** Unifont and every other 16-pixel font draws at half
size, so CJK and much of Cyrillic are unreadable in the menus. Supporting them means reworking the
layouts for taller text first, not adding more fonts.

---

## 2. Filters that do not exist yet

**2.1 An `.obj` structural filter.** A zero-allocation token scanner, three checks: every line
starts with a recognised prefix (`v vt vn f # o g usemtl`) followed by whitespace; vector lines
parse as numbers; face indices are positive and no larger than the vertex count declared so far.
It rejects almost everything on the first character. This is the filter that makes the models
anchor mean something — it is what lets the `pages` line be *searched* for meshes rather than only
checked against one.

**2.2 A dedicated `.obj` alphabet.** Digits, `.`, `-`, `/`, whitespace and the handful of keyword
letters. Cutting the space in the mapping layer before the filter layer ever runs is much cheaper
than filtering, and it is a few lines of alphabet spec.

**2.3 Punctuation filters for `babel29` and `ascii95`.** Punctuation must follow a word and be
followed by a space; for `ascii95`, a capital after a full stop. Both are countable by the same
word-trie machinery `words-v2` already uses. This matters more than it sounds: `lower27` throws
away case and punctuation, so the books line currently holds *word streams, not sentences*.

**2.4 Model-line filters.** Three tiers are already specified in §12 and none are built: local
per-face constraints (rank exactly), small-V constraints such as "every vertex is used" (rank as a
state machine, 256 states at V=8), and whole-mesh properties — watertight, manifold, convex —
which judge but cannot rank, so they run in `mark`/`hide` like any other unrankable filter.

**2.5 Ranking for more than two colours.** Image compact mode only ranks two-colour pictures,
because entropy there depends only on the popcount. Everything with a real palette is unrankable,
which is why compact image mode keeps sparse pictures rather than structured ones.

---

## 3. Open questions (maths, not code)

**3.1 Exact counting under a model.** Counting every page whose information under the order-5
model stays inside a bit budget needs a DP over (context × bits spent × position) and is
infeasible at page length. Two honest routes: lean on guided ordering and do not count, or build a
filter over a *deliberately tiny* model — letter pairs, costs rounded to whole eighth-bits — that
is exactly countable for short pages. Worth doing the small one, if only to see the shape.

**3.2 The two-metric view.** Plot candidates on two independent axes — entropy against structure
density, say — rather than thresholding either alone. Uniform noise collapses into a dense,
predictable cluster; isolated structure sits off the curve. This is the buildable version of
"one dimension has no angles, so cross it over itself", and it is a scoring mode over rankers that
already exist.

**3.3 Calibrate by shrinking the space.** Tune the stack against a large space, then shrink it
until every survivor can be read by hand. The survivors that are merely well-organised gibberish
are exactly where the heuristics leak. A test protocol, not a metaphor: it maps straight onto the
existing `--length` and alphabet controls.

**3.4 Mapping barren ranges.** Once filters have done their work, exhaustively search the surviving
fraction for ranges that are provably empty and store them as a skip list. The safety rule is the
important half and is not negotiable: never exclude a range without having exhaustively searched
it, because a false exclusion erases a real work permanently.

---

## 4. Features worth building

**4.1 Ingest through the game-format parsers.** There are working decoders for several games'
formats already. Piping their output — level geometry, textures, PCM, strings — through
canonicalisation and onto the right line produces a large corpus of *real, verified anchors*. That
is precisely what every filter in §2 and §3 needs in order to be calibrated at all, and it is the
only idea in the whole transcript that produces one out of work already done.

**4.2 File ingest at a content-derived address.** Drop a real file in; it lands at the address
derived from its own bytes, with a small record beside it (name, size, type, time) and a pointer
to the payload. Dropping it twice lands on the same slot, so deduplication is structural rather
than a check. This is what turns the hallway from a viewer into an archive. Corollary worth
keeping: spawn the player's own files on the nearest shelf the first time they walk in — it proves
the addressing is real better than any amount of README.

**4.3 Ambient audio sampled from where you stand.** Warp the current tile's position into the
audio line and pass it through `key-v1` so the result is in a valid key. Continuous in-key ambient,
generated from your actual coordinate, out of three things that already exist. Cheapest
atmosphere-per-line-of-code in the list.

**4.4 Generalise the preview bake.** The crate-face queue (reticle priority, flood fill outward,
per-frame budget, LRU cache) is written for models. One interface where each slot type supplies a
cheap 2D draw would give the same thing to every line: first lines of text for `pages`/`books`,
downsampled picture for `image`, waveform for `audio`, wireframe for `models`.

**4.5 A memory prober.** Watch RAM and VRAM, drop cached previews at the thresholds, without
disturbing position tracking. Needed the moment 4.4 exists.

**4.6 A shareable coordinate string and a `LOCATE` command.** One string naming line, shape and
address, typed in to sweep there. Trivial, and it is how anything found gets passed to anyone else.

**4.7 Telescope view.** Interpolate the camera from first person out to an orbit of the whole
structure; coarse movement moves far, fine movement tweaks the low digits; at range the shelves
collapse into density bands. A navigation UI for addresses that already exist.

**4.8 Wearables and props from the model space.** Model a hat, warp it to get its address, and
equipping it is instantiating the mesh at that address. Items shared as addresses rather than
files. The same mechanism covers any prop.

**4.9 LOD as nested spaces.** Low-poly variants live in shorter digit spaces, high-poly in longer
ones, so detail is a property of which space is addressed rather than a separate asset pipeline.
Open question inside it: whether an object's LOD chain is derivable or has to be bound like a book.

**4.10 Reels — video as a composition, not a line.** F frames of W×H is the same set of
possibilities as F image units in a row, so the video line adds bundling and no content. A reel is
frame rate, image shape, a list of frame addresses, audio shape and a list of audio addresses.
A still scene stores one address and a count; filters and compact mode work per frame on the image
line; and one mechanism covers book, album, comic and reel so no further category is ever needed.
The honest cost: frame-to-frame coherence moves up a level — a given reel can be judged, but every
possible reel cannot be counted or shelved. Nothing achievable is lost, because exact temporal
counting needs the whole previous frame as state, which is the wall the video line already hit.

**4.11 Branching end-caps.** Each line's corridor ends divide into categories and sub-categories,
indefinitely, each staying its own 1D line with the same forward/back traversal. Doors remain the
only cross-domain link. Later, where two deep sub-corridors are strongly related, they could
interlock in a shared room without either losing its identity — but that needs a concrete
relatedness metric before it is anything but a picture.

**4.12 Rings as physical hyperlinks.** A circular cross-section interrupting a corridor, holding
the "see also" doors. The distance walked stays behind you, so following a link gives spatial
memory instead of fifty tabs.

**4.13 A category definition file.** A small declarative file naming which lines are active, how
addresses map and what is locked, which the engine instantiates. It is really a generalisation of
the filter-stack config, and it would let people publish a sieve as one artefact.

**4.14 Agents as scouts, downstream of the filters.** Headless sweeps over the *filtered* space
logging where noise collapses into valid syntax; a vision model watching the preview queue and
stopping the camera when geometry happens to look like something. The honest limit: at these sizes
a blind sweep finds nothing, so an agent is only useful after filters have cut the space, or as a
labeller producing training data for new filters.

**4.15 Small physical touches.** A short wall that morphs into a visible energy grid up close, so
what stops you stepping off the edge is a thing rather than an invisible collider. The Sieve as a
held object that sifts a shelf radius. A hat that toggles the rain overlay.

---

## 5. Things to be careful of

These came up in the transcript stated confidently and are wrong or overstated. Recorded so they
do not come back around.

- **Addressing is not compression.** The address of an ingested N-byte file is about N bytes, so
  "if the drive dies the address regenerates the file" is true for generated units at a given
  shape and false for anything ingested. Same error under "all file sizes shrink dramatically".
  The one real exception is the guided ordering, where a book record genuinely is smaller than the
  text it encodes (~1.8 bits/char) — worth stating next to the rule, because the two look
  contradictory and are not.
- **The π scheme does not work.** Collapsing a file's integer value through coupled modular
  equations to two small residuals plus a step counter, then inverting it, is ruled out by
  counting: 2^N inputs cannot map losslessly onto a bounded pair. The simpler θ = 2π·F mapping *is*
  a bijection but buys nothing — the angular precision needed is exactly the file's bit length, so
  it is the address respelled in radians. Keep the telescope (4.7) and the locate string (4.6);
  drop the maths under them. Likewise "every file is in π": normality is unproven, and even
  granting it, the index of a substring is no shorter than the substring.
- **Similar things are not nearby.** Under positional or scrambled ordering a one-byte difference
  lands arbitrarily far away; prefix ordering puts "This is" next to "Thin ice", not "That is", at
  any unit length. Locality needs an explicit locality-preserving structure — LSH, SimHash, Morton
  coding — which would be a *separate index alongside* the address, never fields stapled into the
  address itself, because the address is already a bijection and cannot carry metadata without
  ceasing to be one. Also: "invert the LCG to solve for the seed" does not apply here; the mapping
  is Feistel/SHA and is built to resist exactly that.
- **The six lines are not orthogonal axes.** They are alternative readers of one shared position
  counter along one straight line. Anything phrased as ℤ⁶ lattice slicing is a misreading.
- **A retrieval boundary does not fix generation.** Querying an addressed space grounds an
  agent's *citations*; it does nothing to any model's weights. The defensible claim is narrower
  and better: making a trajectory addressable makes the divergence point locatable after the fact.
- **The licence wording.** "AGPL with a non-commercial restriction" is not AGPL and is not an
  open-source licence by any standard definition — a field-of-use restriction makes it
  source-available. That may be exactly the right choice, but it should be named accurately and
  the AGPL name should not be on it. No licence at all is worse: it means all rights reserved by
  default *and* no permission for anyone to fork it. Sieve still has no licence file. Separately,
  whether user-ingested content can be redistributed is a real design question with a real answer
  needed before any ingest ships.
- **Mods as vectors is elegant and overstated.** Treating a mod as `base + v` rather than a file
  overwrite is a genuinely interesting reframe, but "different axes can never conflict" ignores
  semantic coupling: two mods touching disjoint variables routinely break each other through a
  shared invariant.
- **The multiverse cluster is out of scope.** Roughly forty turns of hypervisor, emulators,
  cross-game randomisers and n-dimensional run categories. Two things are worth extracting — the
  category definition file (4.13) and the stated position that it deserves to be recorded as an
  idea rather than built. It belongs in a separate speculative document, not in this repo.

---

## 6. Framing worth writing down

**6.1 The epistemic fence.** Meaning is not a property of a bitstream; it is a relation between a
bitstream and a decoder. Without a known grammar, protocol or anchor, high-entropy ciphertext is
indistinguishable from noise — so the engine must classify it as noise *by default*. The
alternative is that everything is potentially meaningful, nothing is filtered, and the system
degenerates into exhaustive enumeration, which is a novelty rather than a tool. This is the
load-bearing axiom under the whole filtration stack and it should be stated as one.

**6.2 The inversion.** The question is not how much is in the library. It is how much of it is
noise we can ignore, how we identify it, and how we define meaningful content. The system is a
lens, not a catalogue.

**6.3 Selection is the creative act.** In a space this size nobody invents anything; they navigate,
filter and curate. Cartographers, not authors. This is the honest description of what the
filtration stack *is*, and it pairs with 6.1.

**6.4 Why a library and not a generator.** A generator that happens to emit something valid loses
it the moment you press the button again, unless you write it to disk — which defeats the premise.
An addressed library gives you a place you can leave, return to, and hand to someone else. This is
the cleanest one-paragraph justification of the project and belongs in the README.

**6.5 The scale anchor.** ~10^1,834,097 books; ~10^394,951 meaningful at one bit per character;
filtering removes about 1.44 million orders of magnitude, and what is left is still ~394,800 orders
beyond Lloyd's limit. Which is exactly why the design is navigate-and-verify and never enumerate.

**6.6 The lineage, and the distinction.** Llull's rotating discs, Swift's Lagado engine, Lasswitz's
*The Universal Library* (1901, which Borges cited), then Borges in 1941. Verify the dates
independently before publishing any of it. The distinction to draw is dimensional scope: the same
combinatorial argument applied to text, audio, images, video, geometry and books at once, with
exact addressing and a terminal you can actually type into.

**6.7 Disclosure.** Anyone happy to consume AI-assisted work will not mind being told; anyone who
prefers human-made work deserves the right to choose. Which means saying so, here, about this.
