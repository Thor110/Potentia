# Every idea, triaged (12 November 2026)

Edward, 12 November 2026: "continue to evaluate ideas as you go forward and give me updates as you go, sort workable and unworkable ideas, as well as technical notes and metaphorical jargon between notes and ideas and voices etc accordingly". This is step 1 of the plan in [ROADMAP.md](ROADMAP.md) ("Next: the ideas filtered"): every entry of [IDEAS.md](IDEAS.md) given a kind, a status and a verdict, each with its reason taken from the entry's own text. Nothing here is new measurement; where a figure is quoted it is the one IDEAS.md records.

Where an entry holds several ideas they are split (19.11 a, b, c). The ids follow IDEAS.md; the header notes at its top are marked "Hdr".

## The words used

- **Kind:** *technical* (something to build or measure), *metaphorical* (a way of seeing it, whose lines belong in [VOICES.md](VOICES.md)), *both*, or *process* (a way of working).
- **Status:** what has happened to it: built, partly built, measured and set aside, decided against, superseded, open, noted only, or waiting on Edward.
- **Verdict:**
  - *done*: built, answered or superseded; nothing left to do.
  - *workable*: can be built at a cost in proportion; *workable, large*: possible, but a project of its own.
  - *waiting on Edward*: a decision only he can make.
  - *unworkable*: it cannot do what it hoped, for the reason given (often the counting argument: on average no naming of a file is shorter than the file). Kept so it is not measured again.
  - *not wanted*: possible, but Edward or a measurement said no.
  - *metaphor only*: a picture, not a mechanism; its words go to VOICES.

## The counts

| Verdict | Ideas |
| --- | ---: |
| done | 183 |
| workable | 205 |
| workable, large | 72 |
| waiting on Edward | 14 |
| unworkable | 49 |
| not wanted | 31 |
| metaphor only | 16 |
| **all** | **570** |

| Kind | Ideas |
| --- | ---: |
| technical | 419 |
| both | 87 |
| process | 44 |
| metaphorical | 20 |

## What comes next

Edward, 12 November 2026: "moving forward the ideas will slowly get filtered onto the task list from the ideas documents and get deprecated accordingly or utilised to improve functionality overall, keeping record of the old ideas somewhere … move ones we have worked through across into a document that isn't front and centre all the time … a work done and an ideas implemented plus deprecated ideas document or set of documents, everything is a range and a lens and everything fits on a chart across time."

- **The task list** is [ROADMAP.md](ROADMAP.md). Workable ideas move onto it a few at a time, ranked by reach and cost, and IDEAS.md points to where each went.
- **The record of old ideas** goes in `docs/ideas/`, out of the way of daily work:
  - `IMPLEMENTED.md`: the *done* ideas, each with the commit, file or result that did it, checked against the code.
  - `DEPRECATED.md`: the *unworkable* and *not wanted* ones, each with its reason and measurement, so they are not tried again unknowingly.
  - `WORK-DONE.md`: a dated log of what was built and measured, one line each, as a chart across time.
  Each entry is moved whole, not shortened, and IDEAS.md keeps a one-line pointer at its old number, so no reference breaks.
- **First move done (12 November 2026):** 39 entries whose every idea is done, none partly built and none naming anything still to do, are now in [ideas/IMPLEMENTED.md](ideas/IMPLEMENTED.md). Entries done in part (§19.71, §19.90, §19.97, §19.123 and others) stay until their rest is; §9's framing stays where it is read; §17 waits for `DEPRECATED.md`.
- **IDEAS.md** then holds only what is open: workable, large, waiting on Edward.


## Waiting on Edward

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 15 (ranking canonical) | Rankable canonical form (v2) | technical | waiting on Edward | waiting on Edward | Hash keys make v1 unrankable. A v2 would rank 466 of the 484 bits but carry structure in one row. Edward: noted, undecided. |
| 17.8 | Page scripts checked in viewer or browser | technical | decided against | waiting on Edward | Two ways were proposed, neither tried. Edward would not bother now; it "may all have to wait on a PotentiaOS". |
| 19.11 a | Video game dimension: a games line | both | waiting on Edward | waiting on Edward | Marked "A design to decide on" (GAME.md §11): short programs for one virtual console, with cabinets in its rooms. "Nothing built." |
| 19.41 | Asking other developers for their oddities | process | waiting on Edward | waiting on Edward | Would be contributed anchors. Needs each maker's written permission and a licence fitting both builds. ffmpeg is run as a separate program, not shipped. |
| 19.48 b | Shipping the docs with the release | process | waiting on Edward | waiting on Edward | "A release does not carry them, which is Edward's call." |
| 19.59 | Public-domain corpus of every language | process | waiting on Edward | waiting on Edward | "He still needs to build" it. Sources and provenance (licence, SHA-256) are named, for training text models and `language-v1` filters. |
| 19.95 b | Measure 7-Zip's BCJ2 against Sieve | technical | waiting on Edward | waiting on Edward | "BCJ2 itself (7-Zip's, not py7zr's) is still to be measured, on Edward's machine." |
| 19.104 b | ALTEngine assets from `.sieve`; game dimension | both | waiting on Edward | waiting on Edward | ALTEngine is nearly done. Reading assets from `.sieve`s is planned there; Alien Trilogy is the first target of the game dimension. |
| 19.106 b | Larger dictionary or paired grouping | technical | waiting on Edward | waiting on Edward | "To decide (Edward)". The delta is still to be measured on WoWRevived with the draft on. |
| 19.108 e | Wave direction: copy one chosen neighbour | technical | measured, set aside | waiting on Edward | Big gains on designed files, but no real file came under xz. "What would decide it is Edward's game data." |
| 19.112 f | Optimise Search rules for other kinds | process | waiting on Edward | waiting on Edward | PNGs, text, programs and archives "wait on Edward's runs with the boxes ticked". |
| 19.121 b | Hide viewer icon on `.simage` thumbnails | technical | waiting on Edward | waiting on Edward | One `TypeOverlay` reg line, "worth trying first on Edward's machine, as it is a matter of taste". |
| 19.133b | Larger model trained locally by Edward | technical | waiting on Edward | waiting on Edward | Edward trains it locally on a corpus he chooses. Needs the floor (built), QK-norm, and a measured curve of size against data. |
| 19.139b | Distributed computing like GIMPS | technical | waiting on Edward | waiting on Edward | Results can be checked rather than trusted, which answers the worry about weaponised PRs. Still "Undecided, as Edward says: noted for when he decides." |

## Unworkable, and why

Kept so they are not measured again. Several were Edward's best questions; the answer is in the reason.

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| Hdr | Each file piece its own rope wave | technical | measured, set aside | unworkable | Packer-only first step "gains nothing, as mixing the whole file beats every cut". |
| 0 | Tiny "genesis key" for huge installs | both | decided against | unworkable | Counting argument: 256^N files need 8N bits. "The key for a 50 GB folder is 50 GB." Only compressing first helps. |
| 0 | Room × item split shrinks address | technical | decided against | unworkable | Total bits are invariant; fixing the item moves 8 bits onto the room. |
| 0 | Bijective address as tamper seal | technical | decided against | unworkable | Anyone can recompute a changed file's address; authenticity needs a signature over the manifest. |
| 0 | Install bound by compute, not storage | technical | decided against | unworkable | Unranking bignums is more work than reading bytes; "a cost, not a feature" unless reduced first. |
| 1.5 | Shortest program with a resolution dial | technical | decided against | unworkable | Kolmogorov complexity is uncomputable; changing resolution changes the output, so it becomes a different object. |
| 3.6 | Angle slices as reduction | both | decided against | unworkable | An angle costs the 8.5 bits it saves; "at best even, never shorter". The bearing and variable-length addressing are kept and built. |
| 8 | Addressing as compression | technical | decided against | unworkable | §0; the only exception is guided ordering, which is a compressor. |
| 8 | Key as signature | technical | decided against | unworkable | §0: anyone can recompute it. |
| 8 | Splitting address into coordinates | technical | decided against | unworkable | §0: the total bits are invariant. |
| 8 | LOD out of an address | technical | decided against | unworkable | Not unless §3.1 has an answer. |
| 8 | Binary search / "50/50 beacon" | technical | decided against | unworkable | No way to ask above or below without already holding the content. |
| 8 | Only an efficiency problem | technical | decided against | unworkable | The barrier is informational: no path optimisation reduces the bits needed to pick one of 256^N. |
| 8 | Low-complexity files drift to centre | metaphorical | decided against | unworkable | An address is a function of content alone; there is no centre. |
| 8 | Dimensionality as hardware setting | metaphorical | decided against | unworkable | Fixed by the content's shape; hardware only decides what gets rendered. |
| 16.1 | Files as their own filters | technical | decided against | unworkable | The filter must travel with the address. The middle "still takes 998 GB". Counting from both ends reorders the same bytes. |
| 16.8a | Filters from RLHF constants | technical | noted only | unworkable | The tuned preferences are "not available to us"; Edward calls it an idea for later. |
| 16.8c | One-byte noise-gradient filter for everything | technical | decided against | unworkable | A shift is a rotation. It gains at most "about 8 bits", which the byte naming it costs back, so on average nothing. |
| 17.1 | Rotation transform, decimal-only numbers | technical | decided against | unworkable | The turn carries the same digits it saves. The hidden bit saves "one bit at most, and on an address nothing". |
| 17.4 | Square and circle wave filters | technical | measured, set aside | unworkable | It only wins on smooth noisy fields, where the rope comes "within about 0.5%". The best shape was always interpolation. |
| 19.27 b | Generator outputs as a filter | technical | noted only | unworkable | "As a filter, it removes almost nothing": 2^64 outputs against 2^(8L) files, so generated files are a vanishing share. Finding a file among them is the same search as finding its seed. |
| 19.28 a | A probe's universe stored in a seed | both | noted only | unworkable | A probe's readings come from nature, not from a seed. A seed can only stand for what a shared generator would produce. |
| 19.34 b | Detecting any encryption as a filter | technical | decided against | unworkable | A strong cipher's output can't be told from noise. Without the key, no test beats chance, and a 128-bit key is 2^128 tries. Wrappers and weak ciphers stay detectable. |
| 19.82 c | Filter every encrypted file out of the space | technical | noted only | unworkable | A good cipher is indistinguishable from random. "If it could, the cipher would be broken." Only weak layers and known headers can be caught. |
| 19.87 a | Rope pulled tight through five pegs | both | measured, set aside | unworkable | "One wave through five points… is too coarse." The kept points "are too few", and "paying for them costs more than they save." |
| 19.88 b | Recompress via ffmpeg | technical | decided against | unworkable | Decoding to pixels and encoding again gives a different file: other tables, rounded pixels, metadata lost. |
| 19.91 | A file's opposite found in noise | both | noted only | unworkable | For almost every file, the seed that makes its cancelling noise is as long as the file. The idea only pays as prediction from bytes already unpacked, which `rope-wave` already does. |
| 19.94 | Percent-plus-degrees grids for shorter addresses | technical | measured, set aside | unworkable | Choosing the best of 65,535 grids gains at most about 16 bits, which is what naming the grid costs. Every file measured came out within a bit of even. |
| 19.98 a | Recurring-value / fraction route for places | technical | measured, set aside | unworkable | No gain on real files. 256 → 78 bits only on a page placed on a recurring value by construction; "It finds what was put there." |
| 19.98 b | Scale repeats, leading zeros, recurrence mask | technical | decided against | unworkable | Naming the shift costs what it saves. Leading zeros are already dropped, and a pattern in the digits is a pattern in the file, which LZMA2 and the rope already find. |
| 19.98 d | Filters shifting state spaces give room | metaphorical | noted only | unworkable | Filters do not move the line. They gain only by removing survivors, because they know something about real files. |
| 19.98 e | Randomly locked byte as filter | technical | decided against | unworkable | A locked byte saves 8 bits but costs its 8 bits plus its offset. §19.87 measured kept bytes worse than no wave on every file. |
| 19.108 a | More waves over the remainder | technical | measured, set aside | unworkable | Each later wave made pictures worse (Vitruvian 5.219 → 5.468 → 6.274). "A wave over noise adds to it." |
| 19.108 c | Seeded noise cancelling a file | both | decided against | unworkable | §19.91 holds: a short seed names only a few noises. Each seeded pass saves at most about k bits more than a fixed wave would. |
| 19.110 a | Real-rope: moving and multiplying locks | technical | measured, set aside | unworkable | More divisions changed nothing. "Moving the points undoes the harm fixed points do, and no more." Locking k bytes still costs n bytes in all. |
| 19.111 e | Game of Life gliders as a filter | both | measured, set aside | unworkable | A pair that meets needs a pair search ("about 270 million boards"), which is the combinatorial explosion. "On the first 4 KiB of the grey picture and of Sieve's program no glider paid." |
| 19.113 e | Operators as mortar between code blocks | both | measured, set aside | unworkable | The operator's places must still be carried, and it is already the cheapest marker. Gain was "0.1% to 0.26%". What pays instead is mortar with no places to keep (`runs`). |
| 19.113 f | Code cut at operators, blocks packed | technical | measured, set aside | unworkable | Cutting removes the context that predicts each block: packed alone "3.6 to 4.1 times the whole", gathered by operator "28% to 41% larger". |
| 19.116 a | Re-encode compressed films' frames with ffmpeg | technical | decided against | unworkable | Sieve must give back exact bytes: "no encoder makes the same stream twice across versions or settings, and the frames alone are not the file". |
| 19.116 c | Identical packets across films, carried once | technical | measured, set aside | unworkable | WoWRevived's eight MP4s: "9,801 bytes in all, 0.0016%". It would pay only for footage cut from one encode. |
| 19.117 a | Steering the rope's locked bytes like wheels | both | measured, set aside | unworkable | The landscape is smooth on pictures, but "the valleys lead nowhere" (51,531 against the rope's 44,729). Edward: "the wrong thing to steer". |
| 19.117 e | Skip tables to repeating-pattern landings | technical | measured, set aside | unworkable | Saved "0.2" to "77.9" bytes on average a piece. "LZMA2 saves 2,055 to 3,520 on the same pieces." |
| 19.117 f | Seeded noise subtracted to land on bearings | technical | measured, set aside | unworkable | "17.14 bits against the file's 16.01": the seed carries what the bearing does not. Noise that pays is the reader's own prediction. |
| 19.117 g | File split into two seeds | both | decided against | unworkable | "Two seeds together are as long as the file they make", so finding them "saves nothing". |
| 19.126a | Use cmix itself as a way | technical | decided against | unworkable | Its floating point is not the same bit for bit across machines, and context mixing decodes as slowly as it codes. Sieve's mixers are exact integers. |
| 19.127a | Lookup table of common place roundings | technical | measured, set aside | unworkable | Leftovers repeat across files only as silence and small runs. The table saves "1.3 to 1.9%" before its own size ("about 200 MB") is counted. Noise never hits. |
| 19.128 | Seeded noise matches clipped from file | both | measured, set aside | unworkable | Noise matches one byte in 256 on any file, so recording where it matched costs as much as it saves. It lost bits at every seed count up to 2^28. |
| 19.131b | Second wave or noise trim pass | technical | measured, set aside | unworkable | A second wave loses "5 to 7%" because the kept values sit apart. A noise pass loses "27 to 116%". |
| 19.132a | Curved wave paths and seeds through grid | technical | measured, set aside | unworkable | Edward's paths (600 shapes) found nothing the waves over every value did not, and seeded paths did worse. Verdict: "no paths or seeds". |

## Every idea, in IDEAS order

### Sections 0 to 12 (and the header notes)

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| Hdr | Archive format as a number | technical | built | done | Waited on a new manifest version; "built since, in v32". |
| Hdr | Headers modelled rather than kept | technical | open | workable | Needs a new manifest version; best gathered into one version when the pause ends. |
| Hdr | Shared web dictionary for pages | technical | built | done | Measured "level with brotli on pages of 20 KB or more, behind on small ones"; built since as v33's `W1`. |
| Hdr | Rope's next contexts | technical | open | workable | Byte 21 lines above, more phases; "at most 0.3% on one file". |
| Hdr | Word list with frequencies for mixing | technical | built | done | Built since as v32's `F80`. |
| Hdr | BCJ2 for programs | technical | open | workable | 7-Zip still leads on small folders of programs; "measure first". |
| Hdr | Each file piece its own rope wave | technical | measured, set aside | unworkable | Packer-only first step "gains nothing, as mixing the whole file beats every cut". |
| Hdr | Small-file overhead packed tighter | technical | built | done | `control.py` measured at 731 bytes against the 7z's 822; built since as v32's smaller head. |
| Hdr | Other lines' places coded by rope | technical | open | workable | Listed as waiting for the next format version (§19.87, *Not yet*). |
| Hdr | File carried as delta from another | technical | built | done | MAPS.WoW and MAPS-Larger.WoW; built since as v32's `delta`. |
| 0 | Tiny "genesis key" for huge installs | both | decided against | unworkable | Counting argument: 256^N files need 8N bits. "The key for a 50 GB folder is 50 GB." Only compressing first helps. |
| 0 | Room × item split shrinks address | technical | decided against | unworkable | Total bits are invariant; fixing the item moves 8 bits onto the room. |
| 0 | Canonical key: one name per artefact | both | built | done | The key is canonical, not small; free dedup; generated content needs no file. "Already built." |
| 0 | Bijective address as tamper seal | technical | decided against | unworkable | Anyone can recompute a changed file's address; authenticity needs a signature over the manifest. |
| 0 | Install bound by compute, not storage | technical | decided against | unworkable | Unranking bignums is more work than reading bytes; "a cost, not a feature" unless reduced first. |
| 1.1 | Reducer as grammar induction | technical | noted only | workable | It is grammar-based compression (Sequitur, Re-Pair) and has a literature to start from. |
| 1.2 | Rolling hash plus suffix array | technical | noted only | workable | Brings the pass to O(N log N), against an all-pairs compare that is quadratic at best. |
| 1.3 | Bit-level pass after byte-level | technical | noted only | workable | Retro formats pack 5-bit channels across byte boundaries, which byte scanning cannot see. |
| 1.4 | Round-trip each reduction before committing | process | noted only | workable | Run each candidate and bit-compare it; throw out any mismatch; the Python oracle helps. |
| 1.5 | Shortest program with a resolution dial | technical | decided against | unworkable | Kolmogorov complexity is uncomputable; changing resolution changes the output, so it becomes a different object. |
| 2.1 | Per-alphabet entropy baseline | technical | built | done | `symbol-entropy-v2` fixed 24 October 2026; English 826 to 877 thousandths against random 992 to 996. |
| 2.2 | Padding counted as content | technical | built | done | v2 filters judge what comes before trailing spaces; a padded noise page now fails. |
| 2.3 | Cased dictionary passes short gibberish | technical | built | done | `-names-v2` lists drop Zn, Tb and similar; became the default; 28 times fewer all-word pages of 64 letters. |
| 2.4 | Colour-blindness pass | technical | measured, set aside | not wanted | Tracks and movies are "58.9 normally and 2.4 with deuteranopia"; Edward keeps the colours for now, since lines are also named on screen. |
| 2.5 | Layouts for wider scripts | technical | open | workable, large | Unifont draws at half size; supporting CJK and Cyrillic "means reworking the layouts first, not adding fonts". |
| 3.1 | Progressive ordering (truncated address = coarser) | technical | open | workable, large | Unanswered. If exact, one address carries every level of detail; if not, "the LOD idea is dead". |
| 3.2 | Exact counting under a model | technical | partly built | done | The small `word-cost-v1` is built; counting under the order-5 model is "infeasible at page length". |
| 3.3 | Two-metric view (entropy vs structure) | technical | noted only | workable | "A scoring mode over rankers that already exist." |
| 3.4 | Calibrate by shrinking the space | process | noted only | workable | Shrink until every survivor can be read by hand; "a test protocol, not a metaphor." |
| 3.5 | Map barren ranges as skip list | technical | noted only | workable | Only with the safety rule: never exclude a range not searched exhaustively. |
| 3.6 | Angle slices as reduction | both | decided against | unworkable | An angle costs the 8.5 bits it saves; "at best even, never shorter". The bearing and variable-length addressing are kept and built. |
| 4.1 | Directory walker emitting a manifest | technical | built | done | `sieve locate`; every install checks the tree's SHA-256 first. |
| 4.2 | Deduplication for free | technical | noted only | workable | Identical files land on the same key; content-addressed storage. |
| 4.3 | First release as first anchor | both | partly built | workable | `make_release` round-trips and publishes SHA-256s; the `sieve-build-release-v1` tag was not made. |
| 4.4 | Chunking large files | technical | partly built | workable | Files are cut into pieces since v13 and decoded on threads; fixed pieces on a line for staged install not built. |
| 4.5 | Real file types out of CLI | technical | built | done | `.wav` for sound; video frames go to any format ffmpeg writes. |
| 4.6 | Signed manifest | technical | decided against | not wanted | Edward, 25 October 2026: no signing; hashed links, a site's table and the crawler stand in for it. |
| 4.7 | CLI as server backend | technical | noted only | workable | Turning it into a service is "multi-threading and transport, not architecture". |
| 4.9 | Locator, manifest, comparison suite | technical | partly built | workable | Locator, manifest and `--compare` built; the Kolmogorov analyser can only give upper bounds. |
| 4.9 | Stand-alone locator/manifest tool | technical | noted only | workable | A small window over the locate and compare tools, without the hallway (Edward, 27 September 2026). |
| 4.8 | Release manifest contents; CLI as graph anchor | technical | noted only | workable | The anchor is the exact released file named by SHA-256, one per platform, since builds differ. |
| 5.1 | One wall of shelves everywhere | technical | open | workable | One-sided on binary now; what the empty side becomes is unsettled; try on one line first. |
| 5.2 | Rings instead of runs | both | partly built | workable, large | Compass readout built; the corridor itself is still straight, "the larger half". |
| 5.3 | Signs above the portals | technical | built | done | §10: a sign over every doorway in its line's colours. |
| 5.4 | Warp animation | technical | noted only | workable | Turn the world, not the camera; "costs nothing" and hides the shelf changing side. |
| 5.5 | Accelerating scroll | technical | built | done | Built 6 November 2026; doubles every two notches, up to 1,024 tiles. |
| 5.6 | Shimmer on crosshair item | technical | noted only | workable | Small, and it makes the interactable thing obvious. |
| 5.7 | Metadata and links on held item | technical | noted only | workable | Turns the hallway from a viewer into an archive; needs §4.1, which is built. |
| 5.8a | Node graph, first version | technical | built | done | Built 27 September 2026; next steps (maps shipped, more relations) open; a whole-space graph left out as a tangle of noise. |
| 5.8 | Toroidal map overlay | both | noted only | workable, large | Needs 5.2 first and anchors to plot. |
| 5.9 | Telescope view | technical | noted only | workable | A navigation UI for addresses that already exist. |
| 5.12 | Junction as a ring | both | noted only | workable | Draws only the doors near your angle; "needs no new machinery". |
| 5.10 | Scene description for custom layouts | technical | noted only | workable, large | JSON plus `.obj`; "Large, and the right shape for it is not obvious yet." |
| 5.11 | Vetting gate for user-placed objects | technical | open | workable | Human approval plus a per-person cap; decide before the feature. |
| 5.11a | Address navigator | technical | built | done | Built, with a decimal view since 6 November 2026; an item preview is a possible next step. |
| 5.13 | Titled-line filters scan bottom-up | technical | noted only | workable | Content holds most information and rejects soonest; the books line should be brought into this order. |
| 6.1–6.4 | Limits and reset | technical | built | done | FIND MY LIMITS and RESET EVERY SHAPE; closed-form, generates nothing. |
| 6.5 | Items per wall setting | technical | built | done | Power of two from 2 to 4096; changes no address, so needs no versioning. |
| 6.6 | Adaptive word width | technical | noted only | workable | An implementation matter inside `BigUint`; must never change an address. |
| 7.1 | Ingest via game-format parsers | technical | noted only | workable, large | Makes a verified anchor corpus from existing decoders; redistribution needs an answer before any ingest ships (§8). |
| 7.2 | File ingest at content address | technical | noted only | workable | Same file, same slot; spawning the player's files proves addressing is real. |
| 7.3 | `.obj` structural filter | technical | noted only | workable | Rejects almost everything on the first character; lets the pages line be searched for meshes. |
| 7.4 | Dedicated `.obj` alphabet | technical | noted only | workable | Cutting the space at mapping is far cheaper than filtering. |
| 7.5 | Punctuation filters babel29/ascii95 | technical | built | done | `sentence-shape-v1` and `babel-punctuation-v1`. |
| 7.6 | Model-line filters | technical | partly built | workable | `canonical-mesh-v1` and two tiers built; whole-mesh properties judge but cannot rank. |
| 7.7 | Ranking beyond two colours | technical | partly built | workable | Built for `palette-size-v1` and `row-runs-v1`; other real palettes stay unrankable. |
| 7.8 | Ambient music chosen by place | technical | decided against | not wanted | Edward: music "probably fine as it is for now"; fully described in case wanted; cheap form "about an hour". |
| 7.9 | Generalise the preview bake | technical | noted only | workable | One interface where each slot type supplies a cheap 2D draw. |
| 7.10 | Memory prober | technical | noted only | workable | Needed the moment 7.9 exists. |
| 7.11 | Predictive prefetch | technical | noted only | workable | Warm the next steps along the direction of travel. |
| 7.12 | Reels: video as composition | technical | built | done | Built as the movies line (`composition-v1`); every possible reel cannot be counted. |
| 7.13 | Branching end-caps | both | noted only | workable, large | Categories subdivide indefinitely, each its own 1D line; doors remain the only cross-domain link. |
| 7.14 | Agents as scouts | technical | noted only | workable | "A blind sweep at these sizes finds nothing"; useful only after filters, or as labellers. |
| 7.15 | Energy grid, sifting Sieve, rain hat | metaphorical | noted only | workable | Small physical touches; no detail beyond naming them. |
| 8 | Addressing as compression | technical | decided against | unworkable | §0; the only exception is guided ordering, which is a compressor. |
| 8 | Key as signature | technical | decided against | unworkable | §0: anyone can recompute it. |
| 8 | Splitting address into coordinates | technical | decided against | unworkable | §0: the total bits are invariant. |
| 8 | LOD out of an address | technical | decided against | unworkable | Not unless §3.1 has an answer. |
| 8 | Binary search / "50/50 beacon" | technical | decided against | unworkable | No way to ask above or below without already holding the content. |
| 8 | Only an efficiency problem | technical | decided against | unworkable | The barrier is informational: no path optimisation reduces the bits needed to pick one of 256^N. |
| 8 | Low-complexity files drift to centre | metaphorical | decided against | unworkable | An address is a function of content alone; there is no centre. |
| 8 | Dimensionality as hardware setting | metaphorical | decided against | unworkable | Fixed by the content's shape; hardware only decides what gets rendered. |
| 8 | Manifold / Clifford torus / Möbius | metaphorical | decided against | metaphor only | "Decoration here"; the one true thing kept is that crossings in projection need not cross in the space. |
| 8 | Licence choice | process | built | done | GNU AGPL v3, with no restriction; whether ingested content may be redistributed is still open (legal). |
| 8 | Mods as vectors | metaphorical | decided against | metaphor only | "Elegant and overstated"; ignores semantic coupling. |
| 8 | Multiverse/hypervisor and OS | both | noted only | not wanted | Separate documents; they "deserve recording rather than building". |
| 9.1 | Epistemic fence: noise by default | both | noted only | workable | The load-bearing axiom under the filter stack; "should be stated as one". |
| 9.2 | The inversion: a lens, not catalogue | metaphorical | noted only | metaphor only | The question is how much is noise to ignore, not how much is in the library. |
| 9.3 | Selection is the creative act | metaphorical | noted only | metaphor only | "Cartographers, not authors." |
| 9.4 | Why a library, not a generator | both | built | done | Added to the README, under The Gallery of Babel (28 October 2026). |
| 9.5 | Scale anchor figures | both | noted only | workable | ~10^1,834,097 books; ~394,800 orders beyond Lloyd's limit, so navigate and verify, never enumerate. |
| 9.6 | Lineage: Llull to Borges | metaphorical | noted only | workable | "Verify the dates before publishing"; the distinction is dimensional scope. |
| 9.7 | Disclosure of AI assistance | process | noted only | done | People who prefer human-made work deserve to choose, "which means saying so, here". |
| 10 | Media panel see-through | technical | noted only | workable | Stated as a defect; no more detail. |
| 10 | Render glitch behind a menu | technical | open | workable | "Never pinned down." |
| 10 | Loop marker as checkered gate | both | noted only | workable | The marker grammar is settled either way. |
| 10 | Portal signs | technical | built | done | `client/door_portal.cpp`. |
| 10 | Shimmer when walking past | technical | open | workable | On crosshair and select it is certain; on walking past "undecided". |
| 10 | Shelf items facing out/in toggle | technical | noted only | workable | Both should exist as a graphics toggle. |
| 10 | Presets above START | technical | noted only | workable | Greyed out when a preset would cross the machine's limits. |
| 10 | Key remapping | technical | noted only | workable | "Explicitly the lowest priority"; Controls stays a read-only list. |
| 10 | Menu items centred vertically | technical | noted only | workable | The help slot is sized for the longest help so the block does not jump. |
| 10 | Auto-DLC repository detection | technical | noted only | workable | "X new repositories detected" on startup. |
| 10 | Record players and TVs by doors | metaphorical | noted only | workable | Decorative, "not functional". |
| 10 | Diagnostic views with speed control | technical | noted only | workable | At full rate it is an unreadable blur; modelled on the *War of the Worlds* RTS. |
| 10 | The Mirror opens local map | both | noted only | workable | Opens the map for where you are standing. |
| 10 | Portal that picks destination line | technical | noted only | workable | The line you came from is kept as the default return. |
| 10 | Wheel scrolls through junction forks | technical | noted only | workable | Needs §5.12's shape. |
| 10 | Warp by content | technical | built | done | T opens the warp box; `hallway --warp`. |
| 10 | Warp to start | technical | built | done | Home goes to corridor tile 0. |
| 10 | Telescopes at the edge | both | noted only | workable | Pull the camera out to see the whole structure. |
| 10 | Coin toss mints machine seed | metaphorical | noted only | workable | One permanent seed per machine. |
| 10 | Multi-pane views | technical | decided against | not wanted | Rejected: "it feels like it would be a headache to look at". |
| 10 | Fixed numbers and default sizes | technical | built | done | Recorded as "already fixed" (book heights, slot sizes, 3,200-character pages). |
| 10 | Old README to `Previous`, hand-written | process | built | done | "Done: `Previous/` holds the earlier READMEs." |
| 10 | Final documentation pass after visuals | process | noted only | workable | That last task "is usually the sort of thing that gets skipped or only half done". |
| 10 | Reconcile pile of loose notes | process | open | workable | Never reconciled with the repository. |
| 10 | CI on Windows and macOS | process | built | done | CI runs on Windows, Linux and macOS on every push. |
| 10 | Release keys tagged as first anchor | process | partly built | workable | Same as 4.3; the tag was not made. |
| 11 | Shelf facing and wall side | technical | open | workable | Asked four or five times; "Parked deliberately." |
| 11 | Pages line to babel29/ascii95 | technical | open | workable | `lower27` throws away capitals and punctuation. |
| 11 | `scowl-en-60-names` side effect | technical | built | done | Answered 24 October 2026 by the `-names-v2` lists. |
| 11 | Vertex ceiling and textured-triangle test | technical | open | workable, large | "The hardest filter question on that line and it has no answer yet." |
| 11 | Which font next | technical | open | workable, large | Cyrillic, Polish and Czech need a taller font; layouts must be reworked first. |
| 11 | Which media types cross paths | technical | noted only | workable | "A cheap experiment, named and never run." |
| 11 | How much the reducer saves | technical | open | workable | Never measured; it decides whether the key idea is worth anything. |
| 12 | Each line as filter against others | technical | partly built | workable | Text against binary built; exclusion removes only the share another line takes (10^-4.23 of 32-letter pages). |
| 12 | Pictures whose pixels are text/files | technical | open | workable | Listed as still open, same spirit. |
| 12 | Melodies whose MIDI is something else | technical | open | workable | Listed as still open. |
| 12 | Pages that are (not) valid models | technical | open | workable | `.obj` is text, so it works both ways. |
| 12 | ascii85 in `not-written` | technical | open | workable | Countable by comparing each base-85 group with the bounds digit by digit; a second version. |
| 12 | Sound (pcm) against note sets | technical | partly built | workable, large | First filters built; counting at a second of sound outgrows the filter memory. |
| 12 | Open-ended notes between notes2 and pcm | technical | open | workable | Every MIDI pitch, ticks, loudness, instruments, tempo. |
| 12 | Signed file well-formed past signature | technical | open | workable | Judging only, since it cannot be counted. |
| 12 | Transformed copies as filters | technical | noted only | workable | Written up in TRANSFORM-FILTERS.md; compression only judges; every key at once excludes everything. |
| 12 | The game, *State Space: Near Zero* | both | noted only | workable, large | Points to GAME.md. |
| 12 | Nine-tone note system | both | noted only | workable, large | Would give each of the nine lines its own mode; now there are seven modes for nine lines. |
| 12 | Drone marking shared-mode doors | technical | measured, set aside | not wanted | Tried and set aside: "not nice to listen to". |
| 12 | Loudness per note | technical | noted only | workable | Edward: worth having in a later version; triples the note symbols. |
| 12 | Books of `notes2` melodies | technical | built | done | `sieve-book-v2`, 7 October 2026. |
| 12 | `notes2` melody-lengths/ending versions | technical | noted only | workable | On `notes2` they step aside, so a track need not end on the tonic. |
| 12 | Rules between voices | technical | noted only | workable, large | Harder than per-voice filters, because each voice keeps its own time. |
| 12 | Optimise all wired into COST | technical | partly built | workable | Optimise all is now a greedy choice of filters only; the aim adds values, sets and anchors as hard limits. |
| 12 | Sets of clashing filters, not pairs | technical | noted only | workable | Choose the best set; drop filters that another ticked filter implies. |
| 12 | Judge-only lines scored by estimate | technical | noted only | workable | They cannot be scored by compact address; COST shows them as unavailable. |
| 12 | COST's balance | technical | built | done | Built 7 October 2026 (Edward's design); weighs the cheapest address against the file. |
| 12 | Settings search for one item | technical | built | done | `sieve tailor`; K on the COST tab. |
| 12 | Cleanest digits, several anchors, files weighed | technical | built | done | The melody comes to 16 bits; Sieve's own files to 21.3% of their own addresses. |
| 12 | `sieve-manifest-v4` packed installer | technical | built | done | 20.00% against 21.17% for one LZMA2 stream; 45,198 bytes (0.61%) under the 7z. |
| 12 | v4 item ways and delta ways | technical | partly built | workable | Item ways need the lines in `sieve-install`; delta built since in v32 (header). |
| 12 | Line's own settings in the search | technical | noted only | workable | Fitting a line to a file would name far more files more briefly. |
| 12 | Sound and models as items; settings searched together | technical | noted only | workable | Listed as still open in 1e. |
| 12 | Description of the stack travels too | technical | measured, set aside | workable | Text saves 13 bits, "not 72"; the melody comes out 22.6 bits longer; minimise a two-part code instead. |
| 12 | Predefined stacks named by bitmask | both | noted only | workable, large | "1 - 0 - 112"; only helps items the shared choices predict; averaged over all items, nothing beats the line's own size. |

### Sections 13 to 18

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 13 (principle) | Small units, composed into larger dimensions | technical | built | done | Built 6 October: tracks and movies added. Filtering compounds upward. Exact counting stays on short units. A whole second of sound would need "about 31 GB" of table. |
| 13 (tiles) | Tile dimension for images | technical | noted only | workable | Called a "far-off idea" with no name yet. Tiles would limit a picture's combinations to those its tiles allow. |
| 13 (world space) | World space composing models and textures | technical | superseded | done | Became worlds (§14) and `worldspace-v1` (§16.7). |
| 13 (overlap) | Same content in two places | technical | open | workable | Either the overlap stands or a cross-dimension filter removes it, as `not-a-file-v1` does. The text records no decision. |
| 13 (address stability) | Rename no line; covers/titles stay | technical | decided against | done | Renaming would move addresses. Tracks and movies were added under new names, and audio and video keep their own covers and titles. |
| 13 (setup menu) | Everything on one 1920 x 1080 screen | technical | built | done | Decided 6 October. A separate window would lose the bars moving live. The map-columns mock-up was set aside; a scrolling list is the fallback. |
| 13 (data) | Dimensions defined once as data | technical | built | done | Done 6 October in `client/dimensions.hpp`. Reordering the doors is just reordering the table. |
| 13 (order) | Door order: parts before compositions | both | built | done | Done: image, pages, books, audio, tracks, video, movies, models, binary. The setup menu's rows follow the doors. |
| 13 (colours) | Black edges mark a composition | both | partly built | workable | Tracks are green and movies red, with black edges. The colour-blindness pass (2.4) is still to do because the dark green and dark red differ mostly in hue. |
| 13 (models) | Own models for tracks and movies | technical | partly built | workable | `book-tracks.obj` and `book-movies.obj` begun as copies of audio's and video's, "to be modelled from there". |
| 13.3 | Composition engine | technical | built | done | Done as `composition-v1`. Books stay on `bookspace-v1`: moving them would change nothing but add risk. |
| 13.4 | Add tracks and movies | technical | built | done | Done, both at once. |
| 13 (seams) | Seam filters as a "joined" stack | technical | built | done | Done 6 October. Units are joined and judged as one. It counts and ranks only when the units have no filters of their own. |
| 13 (tool) | `sieve` tool takes tracks/movies lines | technical | built | done | Done 6 October. CI checks that the tool and the hallway give the same item at the same address. |
| 13 (join filters) | Filters looking only at the joins | technical | open | workable | Still open. They would judge less and count more easily than the joined stack. |
| 13 (records) | Record format for tracks and movies | technical | built | done | Done 7 October: `.track` and `.movie` files, `sieve-book-v2`, and J opens one onto its shelf. |
| 13 (MIDI) | Tool reads MIDI on every note set | technical | built | done | Done 6 November 2026. CI checks the hymn's 73 events on notes104 and notes2. |
| 14 (worlds) | Worlds: compositions of models with textures | technical | partly built | workable, large | Placement is built (`worldspace-v1`). Textures wait for UVs. |
| 14 (UVs) | Store UVs as unwrap algorithm plus delta | technical | open | workable, large | Algorithm-made maps shrink from O(vertices) to a constant. No map costs more than "one bit" beyond its UVs. It needs a pinned deterministic unwrap. |
| 14 (pinned unwrap) | Deterministic pinned `unwrap-v1` | technical | open | workable, large | Blender and xatlas depend on version and floating point. Sieve would need its own unwrap in integer or exactly-rounded arithmetic. The deltas should be measured on real assets. |
| 14 (ordering) | Derived ordering putting generated maps first | technical | open | workable | The raw UV line stays. A generated map's address would be its algorithm, settings and delta. |
| 14 (beyond UVs) | Procedural textures and models plus delta | technical | noted only | workable, large | Same idea as the UVs. UVs are "the cleanest first case". |
| 14 (ENTER) | ENTER a world in ghost mode | both | open | workable | "Ghost mode is near": `WorldSpace::mesh_of` exists. Verdict: build when worlds have textures, or before, untextured. |
| 14 (games) | Video game dimension rebuilt from library | both | noted only | workable, large | "The long arc". It needs placement off the grid, per-model scale, textures and more models. Keep it here until worlds can carry a real level. |
| 14 (code) | Code manifest that compiles itself | technical | noted only | workable, large | Needs a reproducible build with a pinned compiler. Code "does not reduce": it costs about its compressed size. Sieve itself is the natural first test. |
| 14 (translation) | Translation layers reducing games to minimum | technical | noted only | workable, large | This is §7.1's parsers carried through. Savings come only where assets are generated; hand-made assets and code stay about their size. |
| 14 (`engine/`) | Game-format knowledge kept in `engine/` | process | measured, set aside | done | The `.WoW` archives of *The War of the Worlds* (1998) were measured (`engine/wow.md`). The folder is kept out of the main architecture. |
| 14 (archive order) | Archive file order in the recipe | technical | noted only | workable | Only if a reader is ever built. A reader that loses the order is refused, because the rebuilt archive is checked by SHA-256. |
| 15 (idea) | Model weights order a token line | technical | open | workable, large | Guided ordering with a stronger model, at "about one bit a character or less". Pinned integer inference is "the hard part". |
| 15 (SmolLM2) | First model taken apart and measured | technical | measured, set aside | done | 723,674,912 bytes. Frequency coding gives 476 MB (66%), xz 512.9 MB. The mantissa carries 7.85 bits of 8, which nothing measured so far predicts. |
| 15 (engine) | Sieve runs the model itself | technical | built | done | `sieve chat`: logits within 5.4e-5 of ONNX, "about 9 tokens a second". It is still floating point, so it cannot yet give addresses. |
| 15 (AI line) | Every model of one shape, walkable | both | built | done | `aispace-v1`, 2^32768 Micro models. It names nothing more cheaply than its file; what it gives is the space made walkable. |
| 15 (training) | Bit-exact replayable integer training | technical | built | done | `training-v1`: a recorded run trains to the same model on any machine. Micro on Laßwitz goes "from 11.9 to 2.9 bits a byte". |
| 15 (speed) | Faster training kernels | technical | built | done | 42 s a step became 12 s. Blocking and AVX-512 were built 12 November 2026, with every result bit for bit as before. |
| 15 (resuming) | Resume a stopped training run | technical | built | done | `sieve-training-state-v1` gives the result of a run never stopped. CI stops one twice. |
| 15 (precision) | Loss scaling, training-v2 | technical | built | done | Width 128: "3.49 bits a byte to 3.33 at k = 7". Micro got worse, so k is suggested from the shape. |
| 15 (result size) | Keep only the hash for large models | technical | open | workable | The final address is the whole model ("129 MB at that shape"), so large runs may keep only its hash. |
| 15 (AI line limits) | aispace-v2 with real tokens | technical | open | workable, large | Today's limits: 256 byte tokens, 512 positions, at most 8 bits a weight. Larger models gain most from real tokens. |
| 15 (shelves) | Shelf budget for large AI shapes | technical | open | workable | Every crate runs its model for its face, so large shapes need a budget, as pictures have. |
| 15 (weight-rows) | `weight-rows-v1` structure filter | technical | built | done | Counts and ranks exactly. It sets aside little at 4 bits and up to 40% of a 1-bit line, where it is a soft filter. |
| 15 (canonical) | `canonical-model-v1` symmetry filter | technical | built | done | At Micro all but 1 in 2^484 are another model's twins. Hash keys were needed so it would pass `weight-rows-v1`. |
| 15 (ranking canonical) | Rankable canonical form (v2) | technical | waiting on Edward | waiting on Edward | Hash keys make v1 unrankable. A v2 would rank 466 of the 484 bits but carry structure in one row. Edward: noted, undecided. |
| 15 (judge-only tab) | JUDGE-ONLY FILTERS tab | process | built | done | Built as `FilterSpec::judge_only`. Counting and judge-only filters wrap round from both ends. |
| 16 (preamble) | Agreed search vectors | both | noted only | done | More vectors help only as far as each points at real clustered content. Naming which one costs log2 of how many there are. |
| 16.1 | Files as their own filters | technical | decided against | unworkable | The filter must travel with the address. The middle "still takes 998 GB". Counting from both ends reorders the same bytes. |
| 16.1(a) | Longer shortest-route search button | technical | built | done | Built 7 November as L on COST, up to 640 places. It "found nothing shorter than at 20", as the evaluation expected. |
| 16.1(b) | Other known files as filters | technical | built | done | Already real as anchors, near-anchor-v1 and contains-fragment-v1. |
| 16.2 | File Locator from main menu | technical | built | done | Built 10 October. From the main menu, "Use the tailored filters" saves them for the next hallway. |
| 16.3 | Maps packed like installers (v5) | technical | partly built | workable | Stage 1, `sieve-manifest-v5`: "443,969 bytes against v4's 482,491". Ranges, other lines' items and the Locator making v5 are not yet built. |
| 16.3 (fold) | Fold maps into the manifest | technical | superseded | done | Set aside 11 October, because a file carries only what it needs. Superseded 22 October (§19.93) when maps were folded into `.sieve` in `sieve-manifest-v18`. |
| 16.3 (item numbers) | File order recorded as item numbers | technical | noted only | workable | "For a far later manifest". The numbers would shift when files are added or renamed, which a later version must settle. |
| 16.4 | File-type filter on binary line | technical | built | done | `binary-kind-v1`. A list of kinds (`png,jpg`) was added 7 November. |
| 16.5 | "Calculating Dimensions" shown red, ENTER greyed | technical | built | done | Built 10 October, top middle, in red. ENTER is greyed while lines are counted. |
| 16.6 | Slide-in settings window | technical | built | done | Built 10 October, ahead of need, as Edward asked. It slides out on its own while dimensions are calculated. |
| 16.7 | Worlds dimension made of models | technical | partly built | workable | `worldspace-v1`: a G x G x G grid, 24 turns, 4 models. Untextured; textures wait for UVs. |
| 16.8 | AI dimension through Ollama | technical | decided against | not wanted | Ollama runs in floating point, so it cannot address. Edward chose no Ollama and Sieve's own engine instead. |
| 16.8a | Filters from RLHF constants | technical | noted only | unworkable | The tuned preferences are "not available to us"; Edward calls it an idea for later. |
| 16.8b | Prior over weights | technical | built | done | `sieve-weights-v1`: "476.7 MB for 724 MB (65.9%; xz 512.9 MB)", rebuilt byte for byte. |
| 16.8c | One-byte noise-gradient filter for everything | technical | decided against | unworkable | A shift is a rotation. It gains at most "about 8 bits", which the byte naming it costs back, so on average nothing. |
| 16.9 | First item at every degree, one `.sieve` | process | built | done | 3,600 items, "1,055,836 bytes, as a `.sieve` of 436,788 bytes". CI makes it twice and installs it. |
| 17.1 | Rotation transform, decimal-only numbers | technical | decided against | unworkable | The turn carries the same digits it saves. The hidden bit saves "one bit at most, and on an address nothing". |
| 17.2 | CHAT on every line | both | decided against | not wanted | Edward cancelled it for now, unsure it fits. CHAT stays on the AI line. |
| 17.3 | Size and decoding time as one setting | technical | decided against | not wanted | Decoding takes fractions of a second, so there is little to trade. Not measured. |
| 17.4 | Square and circle wave filters | technical | measured, set aside | unworkable | It only wins on smooth noisy fields, where the rope comes "within about 0.5%". The best shape was always interpolation. |
| 17.5 | `source.sieve` as a map holding the 7z | technical | superseded | done | Replaced the same day by a plain `.sieve` of the source folder ("47,299,016 bytes"). |
| 17.6 | Smaller `sieve-install` | technical | decided against | not wanted | 4,471,320 bytes, mostly compressors and SDL. Edward: "It probably isn't worth it then." |
| 17.7 (stamp) | Stamp every `.sieve` with newest version | technical | decided against | not wanted | Older readers would refuse files they can read. |
| 17.7 (16 MiB) | 16 MiB groups for mixed files | technical | measured, set aside | not wanted | "3,099,798 against 3,100,643". It installed in "28.6 s against 14.1 s". |
| 17.7 (by kind) | Group files by kind | technical | measured, set aside | not wanted | "0.4% smaller, not worth a way to say it". |
| 17.7 (sample) | Judge compression by a mixed sample | technical | measured, set aside | not wanted | The sample misses what repeats across the whole. The gain applied to LZMA2's estimate was kept instead. |
| 17.7 (welcome) | "Welcome to the sieve" as a setting | technical | decided against | not wanted | It is only where the hallway starts. Nothing in compression refers to it. |
| 17.7 (anchors) | Anchors folder left out of source | process | decided against | not wanted | Edward: fine as it is for now. |
| 17.8 | Page scripts checked in viewer or browser | technical | decided against | waiting on Edward | Two ways were proposed, neither tried. Edward would not bother now; it "may all have to wait on a PotentiaOS". |
| 18.1 | Maze generators as distortion filters | both | open | workable | Verdict: build as a versioned permutation or mask, as a keyed ordering or a format-aware transform. Not as a per-file reducer: the counting argument. |
| 18.2 | Language filter and layers outwards | both | built | done | `language-v1` and `near-model-v1`. Two digits round the run is "7,548,948,481 models, each named in nine hex digits". |
| 18.3 | Plug in any AI model | technical | open | workable, large | Running Llama models works now. Placing one needs aispace-v2; the address is the file's weights ("724 MB for SmolLM2-360M"). |
| 18.4 | Language: a setting, not a dimension | both | partly built | workable | Language is measured, not part of the address. It is named in the top bar; per-crate bits a byte are not stated as built. |
| 18.5 (scaling) | How language scales in AI space | both | measured, set aside | done | A quarter of the digits can change and most models still speak. That is "only 2^-18,127 of the line": broad basins, vanishingly few. |
| 18.5 (byte images) | Byte model reading a picture's bytes | technical | open | workable | "Possible now" up to "about a 22 by 22 picture". Called "a fair first experiment". |
| 18.5 (vision) | Published vision models in the engine | technical | open | workable, large | Needs a vision encoder and the joining step, "one family at a time". |
| 18.5 (hallway) | Give an image item to a model | both | noted only | workable | "If and when applicable". The same gesture as J between an item and its file. |

### §19.1 to §19.30

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.1 | Geometry filter for figures in small pictures | technical | partly built | workable | `figure-v1` built: shapes, four- or eight-way connection, border. Four-way with row-runs keeps 325,263,852,704 glyph-like pictures. The area setting and limbs are not built because they cost more state. |
| 19.2 | Smallest diffusion model, on the AI line | technical | partly built | workable, large | Stage 1 built as `diffusion-v1` and `sieve diffuse`: 40,960 weights, about one picture in ten is a clear shape, none pass row-runs yet. Next: a larger model and longer runs. |
| 19.3 | How a multimodal model sees a picture | technical | noted only | done | Answers Edward's question. Patches go through a vision encoder, so a single cell can be misread. That is why `figure-v1` decides on the cells themselves. |
| 19.4 | Every system should scale | process | noted only | workable | A rule for building: no fixed sizes, say where a method stops and fall back, the same exact arithmetic everywhere, and record costs as they grow. figure-v1, the AI line and training already follow it. |
| 19.5 a | Computation in a straight line | process | noted only | workable | A rule: each feature should shorten the route to information. A filter earns its place by the bits it saves or the angle it gives, "not by what it suggests". |
| 19.5 b | Constraint as perspective, stacks as rulers | metaphorical | noted only | metaphor only | Constraining degrees of freedom gives more angles. Claude's gloss: every stack is a ruler laid along a line, and a compact address is the distance measured with it. |
| 19.5 c | The engine of all engines | metaphorical | noted only | metaphor only | Mapped onto existing parts: the lines are the arrays, the corridor and worlds the simulation, keyed orderings the encryption, and stacks and models the context. |
| 19.6 | A game putting all games on a spectrum | both | open | workable, large | Every resolution is already a setting. "Fair" means every view is a function of the same addresses. The spectrum would be an ordering. The game's side is in GAME.md §4a. |
| 19.7 | Geometry filters in 2D, 3D and text | technical | partly built | workable | Connected shapes are built. Areas, symmetry, holes, 3D solids and the shape of text are listed. Most are frontiers or counts, so they can count and combine. |
| 19.8 | The Sieve of Potentia, after Eratosthenes | metaphorical | noted only | metaphor only | Both sieves are exact and defined by what they remove. This one strikes out what each filter sets aside and counts what is left. |
| 19.9 | Downloadable chat logs | process | noted only | workable | Claude Code keeps session transcripts as JSON-lines files, which can be handed over when asked. Decisions are summarised in IDEAS.md and HANDOFF.md. |
| 19.10 | Testing in fewer, larger rounds | process | built | done | `sieve_tests tests --only NAME` and `--list` (80 tests) were built. `--only mix4,rope_wave` runs 242 checks in about 4 s. |
| 19.11 a | Video game dimension: a games line | both | waiting on Edward | waiting on Edward | Marked "A design to decide on" (GAME.md §11): short programs for one virtual console, with cabinets in its rooms. "Nothing built." |
| 19.11 b | Lenses: executable, console, source folder | both | noted only | workable, large | One game seen running, as its machine's state, or as a folder of source, each exact both ways (GAME.md §11). |
| 19.11 c | Source tree across the computing spectrum | both | noted only | workable, large | Every unit's source folder becomes a repository and its variants become branches. The whole is a tree of how software descends from software. |
| 19.12 | Metrics list made with every release | technical | built | done | Built as `hallway --metrics` using the menu's own tallies, about two seconds. make_release writes `metrics.txt`. A `sieve metrics` in the tool waits on the tally moving into the shared library. |
| 19.13 | Chat window for the AI line | technical | partly built | done | The CHAT tab is built, with file cards (GO TO IT, J, SAVE) and DRAW for figures. Still to come: replies that name places, and published models in the window. |
| 19.14 a | Short request numbers for large files | technical | noted only | workable | Works only when both ends share a library (anchors, a manifest, a stack). Otherwise the counting argument makes the number as long as the file. A request is (library id, number). |
| 19.14 b | Balance between everything existing and the vault | both | noted only | done | The vault stays narrow by design: only a hash list, "no setting, no category of content, and no taste". Games and art are never its business. |
| 19.14 c | Servers that know only what they must | technical | noted only | workable | A server holds only an index and never sees content. The client makes the file from the line and checks it by its hash. |
| 19.15 | Agreed flags for probes and long links | technical | noted only | workable | The flags are a shared stack, so expected readings become short compact addresses. Caution: compact addresses have no redundancy, so an error-correcting code is needed around them. |
| 19.16 | Predict the data, give it a short tag | both | noted only | workable | A prior made explicit. Guided orderings already do this on the pages line. A prediction reorders and loses nothing, while a stack sets things aside. |
| 19.17 | Diffusion model of the Vitruvian Man | technical | open | workable, large | Crops, shifts and scalings of one picture give the training set. Higher resolution needs more than 511 cells, so tiles or a new aispace version. |
| 19.18 | Every menu reaches every system, same order | process | built | done | Main and pause menus now list the same systems in the same order. Only the node graph and navigator are unique to the pause menu. |
| 19.19 | Music filter designer inside Media Player | technical | partly built | workable | M opens the Filter Designer on a melody filter and each test plays a survivor. Not yet: the designer drawn inside the Media Player's own window. |
| 19.20 | Text game that makes itself, grows to 3D | both | open | workable, large | Its pieces are already addresses. But the harness's language models babble, and a story needs a model that keeps its characters. First step: a scene kept as a record. |
| 19.21 | Conversations in pages, filters for their rules | technical | noted only | workable | The surface shape of a dialogue (alternating, marked turns) can be counted. Whether an answer answers is meaning, so it needs a judge-only model filter. |
| 19.22 | Unfoldable language network for translation | both | noted only | workable, large | A map whose nodes are models and whose edges are relations, unfolding by zoom. Caution: relations multiply too fast, so they need §19.23's ledger. |
| 19.23 | Limits of reduction, measured in a ledger | technical | partly built | workable | The bounds are known exactly, and averaged over all units no stack gains. `hallway --metrics` gives the first column. "This is a round to build, not only a note." |
| 19.24 a | The other side of the library is addressing | both | noted only | workable | It is not compression magic: a shared prediction is a library agreed in advance. Its misses steer the next filter, as tailoring (COST's K) does, used as a feedback loop. |
| 19.24 b | Utility percentage of useful addresses | technical | noted only | workable | Draw random survivors of a stack and ask judges whether each is valid. The share that pass is the stack's usefulness, and some passes are listed as demonstration addresses. |
| 19.25 a | A model's probabilities laid over every line | technical | partly built | workable | It exists in small: a guided ordering is one model's probability over the pages line. A model scoring units would be a soft filter. |
| 19.25 b | Many models networked in one system | technical | noted only | workable | Models combined are again one model, named by its parts and a rule. They must be deterministic (temperature 0 or a fixed seed) to be addressable. |
| 19.25 c | AI for decompilation and format conversion | technical | noted only | workable, large | This is compression by modelling, only as good as the predictions. It needs trained models that "do not exist yet". |
| 19.26 a | ComfyUI bridge for pictures | technical | open | workable, large | It uses the localhost HTTP API. Sieve has no HTTP client, so it needs a small localhost-only one, off by default. Output is checked by its hash. Proposed second in order. |
| 19.26 b | Bigger language models | technical | open | workable, large | Proposed order: published models in CHAT through Sieve's own engine, then the localhost bridge, then quantised weights. Large models are trained elsewhere and come in as files. |
| 19.27 a | Generator plus seed as a file's name | technical | noted only | workable | Exact for files that really are generator output. Many generators can be run backwards, which allows a cheap judge-only filter. A "generated" line is possible. |
| 19.27 b | Generator outputs as a filter | technical | noted only | unworkable | "As a filter, it removes almost nothing": 2^64 outputs against 2^(8L) files, so generated files are a vanishing share. Finding a file among them is the same search as finding its seed. |
| 19.28 a | A probe's universe stored in a seed | both | noted only | unworkable | A probe's readings come from nature, not from a seed. A seed can only stand for what a shared generator would produce. |
| 19.28 b | Shared model plus corrections | technical | noted only | workable | The message is the model's parameters plus whatever it failed to predict. "Only the surprise travels"; when the model is wrong, nothing is lost. |
| 19.29 | Reconstruction; capsule probes sending means to rebuild | both | noted only | workable, large | Cosmology already does this (constrained simulations). It trades compute for bandwidth, with redundancy across capsules. Limit: "What was never sent cannot be rebuilt." |
| 19.30 | Dimensions in flavours, categories of categories | both | partly built | workable | Flavours already exist as settings, a category is a stack, and the node graph can hold a tree of categories. Encryption and noise need a key or generator, "not a filter". |

### §19.31 to §19.60

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.31 a | Melody model on the audio line | technical | built | done | melodies-2x64 trained. Of 300 melodies, all are in key, and 3 to 5 pass the whole stack (noise: none). Shown in CHAT with PLAY and MIDI SAVE. Harness setup built 6 Nov. |
| 19.31 b | Track model and a model of sound | technical | open | workable, large | Listed as "Not yet". Sound (pcm) is far beyond the 511 cells a small model reads, so it needs a model of a different shape. |
| 19.32 a | Slash commands in CHAT for the person | technical | built | done | `/random picture`, `/random melody`, `/help`, then `/random page\|book\|…` (6 Nov) and `/unit LINE HEX` (7 Nov). All go to no model. |
| 19.32 b | Models allowed to call the commands | technical | open | workable | Waits for models that follow formats reliably, and for a clear rule of what a model may do without asking. |
| 19.33 | Articulatory voice synthesis, like an engine simulator | technical | noted only | workable, large | Kelly–Lochbaum tract and two-mass folds, with a five-step plan. Exact and small, but "understandable rather than lifelike". §19.53 lists it as research before building. |
| 19.34 a | Encryption Engine: recipes as a line | technical | partly built | workable | The recipe is a list of layers, so it has an address. Built later as §19.52's stages. Real secrets need reviewed standard ciphers, and export rules need checking. |
| 19.34 b | Detecting any encryption as a filter | technical | decided against | unworkable | A strong cipher's output can't be told from noise. Without the key, no test beats chance, and a 128-bit key is 2^128 tries. Wrappers and weak ciphers stay detectable. |
| 19.35 | Every rule as an automaton | technical | noted only | workable | Every rule has a finite automaton, but whole-unit and one-way rules give huge ones. The aim: a small automaton where one exists, otherwise a superset automaton plus a judge. |
| 19.36 | Only so many ways to write code | both | noted only | workable, large | This is superoptimisation (Massalin, STOKE). Program equality is undecidable, and "right" depends on the measure. Proposed as a code line. §19.53 lists it as research. |
| 19.37 a | How powerful striking off rules becomes | both | noted only | metaphor only | The counting argument is the limit: rules shorten what they predict and lengthen the rest. The power comes from agreed rules that fit the world. |
| 19.37 b | One AI spanning every line | technical | noted only | workable, large | It would be a multimodal model whose vocabulary is the lines' symbols. |
| 19.37 c | Filters steering models (constrained decoding) | technical | open | workable | "Works today wherever a stack has an automaton." §19.46's steer (built 12 Nov) retries seeds instead of decoding under constraint. |
| 19.38 | Map of trained models, strung together | technical | noted only | workable | Edges for lineage, nearness, paths and merges. Linear chains were built in §19.46. The model-graph window and branches are not built. |
| 19.39 a | One game: paid name, free if self-built | process | partly built | workable | AGPL v3 allows it, and Mindustry is the precedent. No Steamworks differences. `build_release.bat` written 6 Nov, "not yet run on Windows". The video is still to make. |
| 19.39 b | The game's whole double title | metaphorical | noted only | done | "One game with two names": the store uses the first name, the repository and self-built copies the second. |
| 19.40 | A file, its full size, its variants | technical | partly built | workable | FULL SIZE (13 Oct) and the exchange-rate card (6 Nov) are built. "Walking between them is still to come." |
| 19.41 | Asking other developers for their oddities | process | waiting on Edward | waiting on Edward | Would be contributed anchors. Needs each maker's written permission and a licence fitting both builds. ffmpeg is run as a separate program, not shipped. |
| 19.42 | Website "Build or Buy" page | technical | built | done | `Pages/release.html` with exact BigInt counters. Storefronts in place of PayPal, and the build video, come later. |
| 19.43 | All AIs folding in on themselves, in Pi | metaphorical | noted only | metaphor only | If π is normal (believed, not proved), everything is in it, at an address as long as the thing itself. Taken as the story's arc and lore. |
| 19.44 | Keeping the interface even | process | built | done | CHAT's row is aligned, and the + is drawn as two bars. Kept as a house-style rule in HANDOFF. |
| 19.45 a | Stories from pareidolia, with exact odds | both | noted only | workable | Pareidolia is a filter's false-positive rate, so it is countable. Coincidences carry their odds, "so that wonder never turns into a false claim". |
| 19.45 b | One equation for all things | both | noted only | done | "Already exists, and it is small": one bijection per shape, and the binary line holds every file. GAME.md takes it as lore. |
| 19.46 a | CHAIN tab: models in a chain | technical | built | done | Built 13 Oct (§12.0h). `sieve-chain-v1` files, `/chain WORDS`, and the three trained models by default. Deterministic and checked by CI. |
| 19.46 b | Branching chains and larger-model stages | technical | open | workable | Listed as "Not yet". Branches would use the node graph's map format (§19.38). Larger models wait until they can be given as files. |
| 19.46 c | Steered chain stages | technical | built | done | `sieve-chain-v2` `steer` (12 Nov) tries up to 256 seeds. With `row-runs-v1`, the figures model first passed on the 130th try. |
| 19.47 a | An engine that could make every game | technical | noted only | workable, large | A game is a starting state plus a rule, with a five-step build order. The limit: almost no unit is a game, so the work is in playability filters. |
| 19.47 b | Scaling up to DLSS 5 | technical | noted only | workable, large | DLSS is NVIDIA's and closed, so Sieve would train its own optional upscaler. The 12 Nov wording is noted in GAME.md §11. |
| 19.48 a | `/docs` commands reaching the documentation | technical | built | done | `/docs` was built 6 Nov. GO TO IT, J and S were added 7 Nov. |
| 19.48 b | Shipping the docs with the release | process | waiting on Edward | waiting on Edward | "A release does not carry them, which is Edward's call." |
| 19.48 c | One catalogue linking everything by SHA-256 | technical | noted only | workable | Maps already do this for files, "the rest are files too". |
| 19.49 | Completion estimate on the main menu | technical | partly built | workable | The menu's part was built 7 Nov, with overflow folding. Still to come: checking the website's copy against it, and labels as filters. |
| 19.50 a | Per-build metrics logged and charted on the web | technical | built | done | `tools/log_metrics.py`, `metrics/index.tsv`, and `Pages/metrics.html` "Metrics Over Time". |
| 19.50 b | Metrics chart on a start-room wall | technical | open | workable | "Still to build: the wall in the start room." |
| 19.51 | A decompilation engine | technical | noted only | workable, large | Search finds exact sources only for small functions, so the realistic shape is analysis (Ghidra/RetDec) plus search. The law limits it, and §19.53 lists it as research. |
| 19.52 a | Encryption Engine stages 1–2 and catalogue | technical | built | done | `cipher-v1`, `sieve cipher`, menu screen, item/file input (17 Oct), DO: CATALOGUE (7 Nov). Checked against the oracle (55 encryptions, 15 guesses). |
| 19.52 b | "Anything as anything": shape-changing and picture-keyed layers | technical | open | workable | A picture-keyed layer is "still to come" (§19.76). Shape-changing layers stay in the earlier plan. The Feistel is not a reviewed standard: for play and study only. |
| 19.53 | Plan in order of importance | process | superseded | not wanted | Superseded by the plans in HANDOFF (24 and 27 October 2026). Kept as the plan of its day. |
| 19.54 | Mapping the madness of information | metaphorical | noted only | metaphor only | Each way of looking is a line or a stack, and the node graph maps them. "Mapping it all is endless, but each mapped part is exact." |
| 19.55 | Voices of the halls | both | partly built | workable | `docs/VOICES.md` corpus started. The in-game `.sieve` of voices is not stated built. Using commands waits on §19.32's rule. |
| 19.56 | Chain as state space, steering as alignment | both | noted only | workable | A chain file already has an address, so a tweak is a step on that line. Steering means filters on its outputs: "a precise, small version of alignment". |
| 19.57 | Rules beyond filters: Lean | technical | noted only | workable, large | Lean's kernel is an exact filter, and PhysLean exists. A proof is almost never found by chance, so proofs are searched for by structure, not by walking. |
| 19.58 | Reading every PDF: OCR and layout | technical | noted only | workable, large | Born-digital PDFs give exact text. Scanned ones need OCR (Tesseract, optional, as a separate program). Layout is a grammar, added one format at a time. |
| 19.59 | Public-domain corpus of every language | process | waiting on Edward | waiting on Edward | "He still needs to build" it. Sources and provenance (licence, SHA-256) are named, for training text models and `language-v1` filters. |
| 19.60 a | Skeleton animation as a line | technical | noted only | workable, large | Skeletons and animations are short units, with filters such as "walks", "stays balanced" and "does not pass through itself". |
| 19.60 b | Prerendered angles as 3D projections | technical | noted only | workable | These are impostors, with depth per pixel for parallax. Used by large open-world games. |
| 19.60 c | Natural scaling ladder up to DLSS 5 | technical | noted only | workable, large | Level of detail: full model, simpler mesh, impostor, point of colour. The upscaler would be Sieve's own and optional (§19.47). |

### §19.61 to §19.90

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.61 | What the numbers mean, in human terms | both | built | done | Counters on the Build or Buy page: a day, everyone at once, pages printed, gained and lost. Progress is measured by what the rules have narrowed. |
| 19.62 | The Universal Exchange Rate, in bits | technical | noted only | workable | A table of bits kept and lost between lines. It "can be computed and shown", e.g. "29.6 million bits in, 100 kept". |
| 19.63 a | A currency for every person | both | noted only | workable, large | It needs Sybil-resistant identity, privacy and legal clearance. Scams move to wherever value is. The notes call it Potentia's project rather than Sieve's. |
| 19.63 b | Digital identity and governance framework | technical | noted only | workable, large | Belongs to Potentia's proposals. The identity has to come first and has to protect privacy, or it becomes surveillance. |
| 19.63 c | A wallet that searches every currency | technical | noted only | workable | Each currency is a unit and ledger validity is a filter, so this wallet is a browser of that line. |
| 19.63 d | Shared distributed computing (primes) | technical | noted only | workable, large | GIMPS, BOINC and Primecoin show it can be done. The networking is described as Potentia's rather than Sieve's. |
| 19.64 | Running out of original things to say | both | built | done | Counter built: 3.5 × 10^-64 % used. Its limit is stated: most sentences are nonsense, so it does not prove originality is endless. |
| 19.65 | Will it unfold or collapse? | metaphorical | noted only | metaphor only | Claude's answer: both. Filters fold the space into a shape, and it collapses to a point only when the rules say as much as the thing. |
| 19.66 | AI disclosure on every page | process | built | done | A box on every page, with a fuller one on Build or Buy. It changes as the project changes. |
| 19.67 | Folding everything | both | built | done | "Folding Everything" is on Build or Buy. Writing everything out is impossible (10^90 bits vs 10^9868), so everything is held as rules. |
| 19.68 | Random voice read on the website | technical | built | done | Fetches VOICES.md and shows one every 12 seconds. "Read aloud" uses browser speech. The box is hidden if the file can't be read. |
| 19.69 | Build or Buy and every build's metrics | process | built | done | Buttons moved to the bottom. "Every Build, On The Record" reads one source, metrics/index.tsv. Records are honest by construction. |
| 19.70 a | How rare a voice is | technical | built | done | Odds and the coupon-collector count are under each voice ("35 turns and about 146 turns" for 36 voices). The visitor's count stays in their own browser. |
| 19.70 b | A hall of voices in the game | technical | open | workable | A wall or page ranked like a leaderboard. Described but not marked built. |
| 19.71 | Every release measured; Specs vs Stats | technical | built | done | make_release step 8c times the unit suite and specs.html exists, with Landauer and Dyson limits. Still to come: the start room's wall and more timed measures. |
| 19.72 | Library of small baked models | technical | partly built | workable | type-2x32 is the fourth entry. Digits, strokes, textures, chords and words are listed to bake next, each "a few minutes' training". |
| 19.73 | The characters model | technical | partly built | workable | Built and shipped, but "None of them draws a recognisable character yet." A guess about decoding is untested, and four next tries are listed. |
| 19.74 a | AI voices spoken in the game | technical | open | workable | Windows SAPI needs no download. The website already has subtitles. |
| 19.74 b | Menus and buttons read aloud | technical | partly built | done | Built for the main, pause and setup menus and the File Locator. Not yet the hallway's panels, and not yet heard on Windows. |
| 19.74 c | Objects and signs translatable | technical | open | workable | Would go through the same language files as the menus. |
| 19.74 d | Text size, contrast, colour-blind palettes | technical | open | workable | Listed as aims. The lines' colours are already settings. |
| 19.74 e | The tactile Sieve: braille, touch | both | noted only | workable, large | The aim goes beyond reading aloud, "as screens come to carry them" (§19.140). |
| 19.74 f | Quick-time events in a grid | technical | decided against | not wanted | "There are no quick-time events in this game or its story, and he does not want them." Recorded so it is not lost. |
| 19.75 a | Filter for how free-flowing something is | both | noted only | workable | Would measure how few of a thing's parts are fixed. "May lead to a filter one day, or may not." |
| 19.75 b | Everyone's filter stack is an angle | metaphorical | noted only | metaphor only | A stack is a unit of its own. Comparing stacks shows where the space looks the same from every angle. |
| 19.75 c | Fold all video games into one | both | noted only | metaphor only | Already folded as files of the binary line. An emulator is the rule that plays them, and the hallway is the start of one. |
| 19.76 | Characters and figures as filters and keys | technical | open | workable | Next: a glyph-v1 filter counted exactly, and xor-picture in a later cipher-v2. A picture's 100 cells are 100 key bits. |
| 19.77 | How many voices a contributor may add | process | noted only | workable | "Unlimited within reason." Noted, not built. Duplicates would be caught by the list itself. |
| 19.78 | Every figure with its inverse | both | built | done | Website figures are paired with what is left. The HUD (6 Nov) and setup menu counts (7 Nov) are built. No inverse where no honest total exists. |
| 19.79 a | ffmpeg warnings in red | technical | built | done | Settings > Programs ffmpeg row. Warnings are red on a dark plate and say where it looked. |
| 19.79 b | Encryption Engine takes the item in hand | technical | built | done | I opens the engine with the item. Ctrl+O, Ctrl+S and GO TO IT work. |
| 19.79 c | Controls flow between windows | process | partly built | done | "Mostly the ethos now." Most things hand off already, and Edward keeps an eye out for others. |
| 19.79 d | Window returns to front after file dialog | technical | built | done | window_front.cpp with AttachThreadInput. "It works on Windows (Edward tested it)." |
| 19.79 e | Chat tabs answer the mouse | technical | built | done | A click on any tab works, in CHAT too. |
| 19.79 f | Chat log and editable context window | technical | partly built | workable | /log, Ctrl+L, /context and Ctrl+E are built. "Still to come: voice." |
| 19.79 g | A help tab on every item | technical | built | done | HELP is on every item's page, its rows chosen by the item's line. |
| 19.79 h | Version number moves with releases | process | built | done | "0.13.25, as Edward chose." |
| 19.80 a | CHAT on every line | technical | decided against | not wanted | "Deprecated for now… he is not sure it fits" (§17.2). |
| 19.80 b | One less if: tabs on every line | both | open | workable | COST, help and the Engine button become tabs of any item, greyed with a reason where they have no use. |
| 19.81 a | Folding temperature that hops between models | both | open | workable | Exact once similarity is a rule. The first form is a chain stage that picks a model from a neighbourhood, logged. |
| 19.81 b | Token, process and mapper node viewers | technical | open | workable, large | 3D node graphs of how models move. A click on a node goes to that model. |
| 19.81 c | Models as filters, one constraining map | both | noted only | workable, large | His idea that it all adds up to one map of knowledge across the state space. No build step given. |
| 19.81 d | A system tuned to make its own filters | both | noted only | workable | "It could search itself and make filters for itself." |
| 19.81 e | Pictures boiled down to titles | technical | noted only | workable | The image line's title read from its content, the reverse of titling a unit. |
| 19.82 a | Files through the Encryption Engine | technical | partly built | workable | File input and output were built 17 October. The .sieve recipe and split keys are still to come. |
| 19.82 b | Keys split into several (secret sharing) | technical | open | workable | Shamir's scheme is exact integer arithmetic, so it fits Sieve and can be checked against the oracle. |
| 19.82 c | Filter every encrypted file out of the space | technical | noted only | unworkable | A good cipher is indistinguishable from random. "If it could, the cipher would be broken." Only weak layers and known headers can be caught. |
| 19.82 d | Baseline analysis of anchors; weak-cipher automaton | technical | open | workable | A measured catalogue of recognisable encryption. Weak layers compose into a finite-state transducer, so they can be counted exactly. |
| 19.83 | One model at the centre of all | both | noted only | workable, large | First form: a node map per model type, with a router chain. Whether it beats its parts is "a question the metrics can answer". |
| 19.84 | A gallery page on the website | technical | partly built | workable | Planned: build_gallery.py writes gallery.json. Images/Sieve has no top-level images, so the preview "fills in when some are put there". |
| 19.85 | Skeleton system alongside UVs | technical | noted only | workable, large | Joint limits act as a filter on poses, so poses form a countable line and an animation is a unit of poses. |
| 19.86 a | Check the AI line's 2^32768 | technical | built | done | "The AI line's figure is right." The setup menu now writes it as 16^8192. |
| 19.86 b | Survey of published models | technical | measured, set aside | done | A table of families read from headers. Most are llama-shaped, and "size is the limit before architecture is." |
| 19.86 c | Keep F16 weights as stored | technical | built | done | widen_weights setting, with "auto" built 6 November. Logits match to the bit. It trades memory against speed. |
| 19.86 d | Take models apart on the fly | technical | open | workable, large | Six stages. Proposed first step: inspect and map, with Llama 3 rope scaling, Qwen2 biases and Phi-3 fused tensors. |
| 19.86 e | aispace-v2: published models on the AI line | technical | open | workable | Shape taken from config.json. The address is as long as the weights, "never shorter". |
| 19.87 a | Rope pulled tight through five pegs | both | measured, set aside | unworkable | "One wave through five points… is too coarse." The kept points "are too few", and "paying for them costs more than they save." |
| 19.87 b | Propagation: rope-wave-v1 to v5 | technical | built | done | Wins on smooth data: colour picture 494,774 vs LZMA2 623,717. pi6.txt is 0.46% over the floor. |
| 19.87 c | Context from the byte 21 lines above | technical | measured, set aside | not wanted | "Not built: a new rope-wave version and manifest for at most 0.3% on one file." |
| 19.87 d | More phases, other lines' places | technical | open | workable | Still open: more phases, a wave per line, 16/32-bit values, and other lines coded by it. |
| 19.88 a | JPEGs losslessly smaller | technical | measured, set aside | workable, large | Brunsli and cjxl measured at 13.93% to 21.91% smaller. Sieve's own coefficients "were no smaller than the JPEG". Back seat by rule. |
| 19.88 b | Recompress via ffmpeg | technical | decided against | unworkable | Decoding to pixels and encoding again gives a different file: other tables, rounded pixels, metadata lost. |
| 19.88 c | LZMA2 and rope pieces in one file | technical | built | done | sieve-manifest-v13 beats 7z on his photos (4,111,592 vs 4,127,434), with the cut found from bytes alone. |
| 19.88 d | Optimise archive formats (xz…brotli, libarchive) | technical | built | done | v14 to v20 remake xz, gzip, bz2, zst, lz4 and brotli exactly. The others are carried as contents, with licences kept. |
| 19.88 e | 7z, zip, gzip made again exactly | technical | built | done | v21 with preflate and the LZMA SDK, e.g. a 7z at -mx1 329,861 → 280,971. Default-level 7z is left as is. |
| 19.88 f | RAR remade by the machine's rar | technical | partly built | workable | v22 tested with a stand-in. The real rar is not measured, RAR 4 is contents only, and it works only with the same rar version. |
| 19.88 g | Header models for archives | technical | measured, set aside | workable | The headers are small and LZMA2 already packs their repeats, "so it waits." |
| 19.88 h | Permanent vs temporary archive optimisation | technical | built | done | Built 27 October (v31): the source 7z went from 8,005,603 to 6,189,674 bytes, given back byte for byte. |
| 19.88 i | Archive format as a number | technical | partly built | done | A number, not a bitmask, as a varint. It is in v20's listing, but "the format as a number" for temporary "is still to do". |
| 19.88 j | PNG rows carried unfiltered, remade exactly | technical | noted only | workable | "An idea… not measured." Whether preflate's corrections stay small is what to measure. |
| 19.89 | Size and decode time as one setting | technical | decided against | not wanted | Set aside the same day: decoding takes fractions of a second, "so there is little time to trade for size." |
| 19.90 | The listing in bytes | technical | built | done | v16 put the head in bytes (67 bytes). The entries were measured as small, then built into v20 anyway: "even the small savings will add up". |

### §19.91 to §19.110

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.91 | A file's opposite found in noise | both | noted only | unworkable | For almost every file, the seed that makes its cancelling noise is as long as the file. The idea only pays as prediction from bytes already unpacked, which `rope-wave` already does. |
| 19.92 a | Shared web dictionary to rival brotli | technical | built | done | Measured first: 120 KB dictionary was 0.5% below brotli on the 102 pages, but behind on small pages. It was set aside then; the heading says it was built since as v33's `W1`. |
| 19.92 b | Browser plugin decoding `.sieve` via sieve.exe | technical | built | done | Phase 1: `sieve web-host` over native messaging plus a Manifest V3 extension. The test site was shown in Chromium 141, and a tampered picture was refused. |
| 19.92 c | Links state hashes; chain of hashes | both | built | done | `sieve site build/check` writes `integrity` on each link and refuses pages that name each other. PHP and script handling and `--served-at` were added. |
| 19.92 d | Site table (`index.sieve`) plus crawler | technical | built | done | Every page names the table, the extension remembers it, and `crawl_site.py` checks against the printed hash. Edward kept "Trust the new table". |
| 19.92 e | Signing the hash | technical | decided against | not wanted | Edward: "I don't trust myself or any human being alive to keep a key safe." |
| 19.92 f | Many-file `.sieve` shown as directory listing | technical | built | done | Edward asked for it to be noted (22 Oct). It was built in phase 2: web-host answers with a listing, the viewer shows a tree and each file opens by its path. |
| 19.92 g | PHP helper writing `integrity` from table | technical | noted only | workable | What PHP writes when it runs cannot be seen beforehand. A small PHP helper is suggested, "not built". |
| 19.92 h | Circular links via blanked-hash pages | technical | noted only | workable | "A way to weigh later". It is sound only if the site's list also holds every page's full hash. |
| 19.93 | Fold `.map` format into `.sieve` | technical | built | done | `sieve-manifest-v18` with graph part (flag 64). `.map` is still readable; the release publishes `source.sieve`. |
| 19.94 | Percent-plus-degrees grids for shorter addresses | technical | measured, set aside | unworkable | Choosing the best of 65,535 grids gains at most about 16 bits, which is what naming the grid costs. Every file measured came out within a bit of even. |
| 19.95 a | Release downloads were v4 | technical | built | done | `make_release` left out `--items`, which is now passed. Newest beats 7z where both use BCJ (2.74% smaller). |
| 19.95 b | Measure 7-Zip's BCJ2 against Sieve | technical | waiting on Edward | waiting on Edward | "BCJ2 itself (7-Zip's, not py7zr's) is still to be measured, on Edward's machine." |
| 19.96 | Degree sample and starting page as `.sieve` | technical | built | done | Sample 7,887 bytes, starting page 159, each installed back byte for byte. `--sample-degrees` writes the newest since 27 October. |
| 19.97 a | Time estimate in File Locator | technical | built | done | Samples timed and refined as it goes. Measured estimates came out 15 to 20% short, then a third low; a quarter was added and it is corrected per step. |
| 19.97 b | Cheaper boundaries when cutting into pieces | technical | built | done | Weighed over a fixed window of at most 64 KiB. The 40 MB tar went from 2,053 s to 1,245 s with the same bytes. |
| 19.97 c | Stop and Force stop | technical | built | done | Keeps the best so far, or the last finished result. Stopped results install byte for byte. |
| 19.97 d | Live graph of best results so far | technical | built | done | Each new best is drawn at once and not stored. Flat lines were moved inside the frame after Edward saw only a small line. |
| 19.97 e | Memory estimate beside time estimate | technical | built | done | Built 6 November 2026. Fitted on six measured folders, each within about a quarter; it put Edward's 377 MB folder at 5.4 GB where it took 4.1. |
| 19.98 a | Recurring-value / fraction route for places | technical | measured, set aside | unworkable | No gain on real files. 256 → 78 bits only on a page placed on a recurring value by construction; "It finds what was put there." |
| 19.98 b | Scale repeats, leading zeros, recurrence mask | technical | decided against | unworkable | Naming the shift costs what it saves. Leading zeros are already dropped, and a pattern in the digits is a pattern in the file, which LZMA2 and the rope already find. |
| 19.98 c | One ending roll; score the total | technical | noted only | workable | Agreed: a rounding made midway is undone by the next shift, so a describer of the number belongs at the end. |
| 19.98 d | Filters shifting state spaces give room | metaphorical | noted only | unworkable | Filters do not move the line. They gain only by removing survivors, because they know something about real files. |
| 19.98 e | Randomly locked byte as filter | technical | decided against | unworkable | A locked byte saves 8 bits but costs its 8 bits plus its offset. §19.87 measured kept bytes worse than no wave on every file. |
| 19.98 f | Standing watch: fraction route under every search | technical | open | workable | Proposed: costs microseconds a place and nothing where it does not win. "The watch is cheap enough to run regardless." |
| 19.99 a | Threads for packer trials | technical | built | done | Same instructions at any thread count. The recording went from 941 to 351 s with the same 36,568,587 bytes. |
| 19.99 b | Rope search: text waves first, end early | technical | built | done | Quick set made in 30.5 s against 67.0. Word list 633,244 → 564,462; 127 files gave the same bytes in 29.3 s against 110.9. |
| 19.99 c | Folder's files worked several at once | technical | built | done | `docs` went from 40.8 s to 20.1 s, byte for byte the same on one thread and four. |
| 19.99 d | Thread the rope search, weighing, tailoring | technical | open | workable | "Still single": the rope search's time limit makes its result depend on how far it got, so it would need splitting into ordered trials. |
| 19.100 a | "Create .sieve" in right-click menu | technical | built | done | Built 6 November 2026 as `hallway --create`, the File Locator alone. Several selected files are treated as a directory. |
| 19.100 b | "Compress to" other formats submenu | technical | built | done | `sieve compress` for zip, 7z, xz, gz, bz2, zst, lz4, br, lzma and tar, each read back before writing. Not rar (licence) or cpio/iso/ar/warc. |
| 19.101 | Crawler as signed hardware card | both | noted only | workable, large | "Not for even the near future." It answers the case of a server replaced whole. "A thought only; nothing to work on." |
| 19.102 a | Grammars and rulesets as hard filters | technical | superseded | not wanted | A hard filter loses any file that breaks a rule. Taken instead as predictions inside mixing (mix-v3's word list). |
| 19.102 b | Triangle predictor (median edge detector) | technical | built | done | rope-wave-v6: Vitruvian 762,766 bytes against the rope's earlier 831,022. Later used inside mixing (727,842). |
| 19.102 c | Triangle as designed: kept vertices, edge noise | both | open | workable | "Not recorded as measured". §17.4's prototype would take it as one more shape. |
| 19.102 d | Mixing all predictors (mix-v1 to v4) | technical | built | done | mix-v2 Don Quixote 532,168. mix-v3 word list gives 1.9% to 7%. The SSE stages were retired as mix-v4 at Edward's request. |
| 19.102 e | Generators: remake PNGs from known program | technical | built | done | Nine of ten PNGs are exactly stb_image_write level 8. Ten PNGs went from 657,221 to 247,695 (v25). |
| 19.102 f | Files and groups mixed together | technical | built | done | v29: `tools` −25.5%, `client` −26.6%. v30 groups up to 8 MiB decode at once. |
| 19.102 g | Every `.sieve` written at newest version | process | built | done | Edward: "a .sieve should always be the latest". There is one helper, `add_packed_newest`. |
| 19.102 h | Word frequency list; more generators | technical | open | workable | Listed as "Next". SCOWL has no frequencies, so a frequency list would be a new pinned file. |
| 19.102 i | Mixing faster via SSE2, same codes | technical | partly built | workable | 13.9% fewer instructions, with the same SHA-256 codes. Counters' moving is left: "measure before trying." |
| 19.103 a | Weigh pieces by true rope/LZMA2 cost | technical | measured, set aside | not wanted | All four files came out the same size, byte for byte, because mixing the whole file beat every cut. |
| 19.103 b | Mixing as a kind of piece | technical | open | workable, large | What is left of the idea. It needs a new version and is in the list at the top. |
| 19.104 a | Game files sieved, streamed into cache | technical | measured, set aside | workable | Measured: one `.sieve` per file is +0.21% and any file comes in 18 ms (median). "Streaming needs no new version." |
| 19.104 b | ALTEngine assets from `.sieve`; game dimension | both | waiting on Edward | waiting on Edward | ALTEngine is nearly done. Reading assets from `.sieve`s is planned there; Alien Trilogy is the first target of the game dimension. |
| 19.105 a | Gigabyte instructions read/made in proportion | technical | built | done | The 377 MB folder installed in 13.6 s and 2.4 GB, against 63 s and 11.5 GB before. |
| 19.105 b | Large incompressible LZMA2 estimate; WoWRevived gap | technical | open | workable | "Left": the LZMA2 estimate took 112 s of 139 on a 125 MB file. Where WoWRevived's 0.20% against 7z comes from, file by file, is not yet looked at. |
| 19.106 a | `delta` way for near-identical files | technical | partly built | done | Built in v32's draft. The 160 MB pair came out 20.36% smaller; MAPS-Larger.WoW holds 98.7% of MAPS.WoW. |
| 19.106 b | Larger dictionary or paired grouping | technical | waiting on Edward | waiting on Edward | "To decide (Edward)". The delta is still to be measured on WoWRevived with the draft on. |
| 19.107 | Window title, pickers, installer fixes | technical | built | done | Explorer folder picker, installer program from a `.sieve`, text-field address bar, and reading on its own thread. |
| 19.108 a | More waves over the remainder | technical | measured, set aside | unworkable | Each later wave made pictures worse (Vitruvian 5.219 → 5.468 → 6.274). "A wave over noise adds to it." |
| 19.108 b | Two waves that meet (interlaced) | technical | measured, set aside | not wanted | "About even, no gain." |
| 19.108 c | Seeded noise cancelling a file | both | decided against | unworkable | §19.91 holds: a short seed names only a few noises. Each seeded pass saves at most about k bits more than a fixed wave would. |
| 19.108 d | Designed test files for the rope | technical | built | done | `tools/rope_designs.py` makes eight files. The rope cancels the checker to 24 bytes. |
| 19.108 e | Wave direction: copy one chosen neighbour | technical | measured, set aside | waiting on Edward | Big gains on designed files, but no real file came under xz. "What would decide it is Edward's game data." |
| 19.108 f | Repropagate until cancelled; zero seeds | both | open | workable | Edward: look at it "seriously" once tests are done. The open question is whether a second wave on real files removes more than its parameters cost. Compared to matching pursuit. |
| 19.109 a | PNG rows carried unfiltered (`skr`) | technical | built | done | 23% to 40% smaller on the five PNGs other programs wrote, back byte for byte. |
| 19.109 b | Mixing on stb PNG pixels | technical | built | done | hallway-compact went from 16,058 to 12,772. The 18 screenshots went from 245,100 to 211,937. |
| 19.109 c | Refuse names equal but for case | technical | built | done | Installers now refuse such a pair. Found by CI step 11's failure on Windows. |
| 19.110 a | Real-rope: moving and multiplying locks | technical | measured, set aside | unworkable | More divisions changed nothing. "Moving the points undoes the harm fixed points do, and no more." Locking k bytes still costs n bytes in all. |
| 19.110 b | Real-rope as per-file rule set or mixing input | technical | open | workable, large | "Either is a new way, so the next version." To try first where the rope already wins (pictures, sound, tables of digits). |

### §19.111 to §19.125

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.111 a | Shortcut seed: search shortcuts for fewest bits | technical | partly built | workable | Stage 1 prototype built and measured. Text and code "now within 1% to 2.5% of xz, mixing still ahead". The designed files came out far under every other way. |
| 19.111 b | Counting the cards: every small arrangement | technical | measured, set aside | workable | Held-out text cost "5.55 bits a byte" against 8. The shuffle carries over only between similar kinds. The transform form is listed in 19.112 and not built. |
| 19.111 c | Skip-and-insert shortcut (`skiprun`) | technical | built | done | Finds Edward's example alone: "55.6 bits, 7 bytes". It was in no best recipe on real files, and is kept for the layouts it describes. |
| 19.111 d | Geometric filters: moving and altering bytes | both | built | done | Each designed grid is cancelled by its own filter (spiral 11 bytes, xz 2,652). On real files none beat using no filter, which is "Edward's chaos, measured". |
| 19.111 e | Game of Life gliders as a filter | both | measured, set aside | unworkable | A pair that meets needs a pair search ("about 270 million boards"), which is the combinatorial explosion. "On the first 4 KiB of the grey picture and of Sieve's program no glider paid." |
| 19.111 f | Record what carried each result | process | built | done | The "Carried by" column and "Ways in full" are built. Both are named so that Sieve is shown as an engine that mixes existing methods, not a new algorithm. |
| 19.111 g | Read methods back from a `.sieve` alone | technical | built | done | `tools/sieve_read.py`: light read "0.19 s for docs/'s", plus a full read. It took apart mozilla: "510 files by LZMA2". |
| 19.111 h | World energy cost against saving benchmark | process | built | done | `bench_power.py` built. Mixing for size "moves more onto users than it saves the networks". The answer changes within the assumed range, so real-machine counters decide. |
| 19.111 i | Real figures on one reference machine (RAPL) | process | open | workable | Making and installing to be measured in joules by RAPL and a wall wattmeter. The servers' and networks' side stays a stated range. |
| 19.111 j | Site button becomes "Sieve Power Savings" | process | open | workable | Planned for "when the figures exist". It would show what a day of fetches saves and moves onto users, with the machine's spec beside it. |
| 19.111 k | Hallway run in the browser | technical | open | workable, large | How is open: rewritten for the web, WebAssembly, or emulated. "To be weighed when the numbers are there." |
| 19.111 l | Mixing with longer reach, whole mozilla | technical | built | done | Built as mix-v4's R1, M4 and C2: mozilla at 13,318,505 bytes, "under LZMA2's 13,376,180 for the first time". |
| 19.111 m | Skip extra searches for pre-coded pictures | technical | open | workable | Where every file is carried by an already-coded recipe (`jbr`), the extra searches "can change nothing". "To be confirmed against his tables first." |
| 19.111 n | `.simage` picture viewer with bomb checks | technical | built | done | Built as `sieve-view` (19.112). It reads the head and listing first, then checks the picture header and limits, and has a Sieve button. |
| 19.111 o | Optimise Search box in File Locator | process | built | done | Built with a rule for JPEGs. decode/try took "1.24 h for decode/no's 0.08 h" for the same bytes. Other kinds get rules as measurements come in. |
| 19.111 p | Speed up the seed prototype | technical | built | done | Finding candidates went from "144 s" to "1.2 s", and the checker's whole search to 31.7 s. Every recipe is still decoded and checked. |
| 19.111 q | Site maps that keep themselves current | technical | noted only | workable, large | A Merkle tree with a node per user's uploads. Comments and counts sit outside it or are hashed once closed. It needs agreement on who hashes. |
| 19.112 a | Programs mixed through the x86 filter | technical | built | done | 16 programs: x86 ones "3.0% to 15.1% under LZMA2". Built as v33's `X`. ARM ELF and Alpha COFF are still left. |
| 19.112 b | Rising bar for mixing over 16 MiB | technical | built | done | mozilla now goes to LZMA2 (0.43% more) and installs "in 1.0 s instead of 49.5 s". The bar now applies against the best way before mixing. |
| 19.112 c | Cut the stream weighing's cost | technical | built | done | Where the mixed files' codes are small it skips one estimate. "It halves the weighing's processor time where it applies, not its wait." |
| 19.112 d | Bitwise matched literal in the seed | technical | built | done | Now the default: "0.9% off the program, 0.6% off the teapot". Still "16.8% behind xz on the program". |
| 19.112 e | Counting the cards used as a transform | technical | open | workable | Bytes renamed by a kind's ranks, placed before the shortcuts or mixing, to be measured per kind. Not built. |
| 19.112 f | Optimise Search rules for other kinds | process | waiting on Edward | waiting on Edward | PNGs, text, programs and archives "wait on Edward's runs with the boxes ticked". |
| 19.112 g | Glider guns and other Life-like rules | technical | noted only | workable | To be tried "first on files made by cellular automata". Not built. |
| 19.112 h | Hallway leftovers (power page, ini setting) | technical | open | workable | Viewer built. The power page waits for figures. The `--search-every-wave` ini point is "(minor)". |
| 19.112 i | Alpha ECOFF mixed without start test | technical | decided against | not wanted | libgklayout came out "0.6% larger". "The test is right where it says no, and nothing is built." |
| 19.112 j | Mixing tables at least B20 | technical | built | done | B20 was smaller on all ten files, "0.05% to 3.3%". B22 was rejected for the "268 MB a file to decode". |
| 19.113 a | enwik9 bench: time spent in rope search | process | measured, set aside | workable | The rope's search took "3,123 (66%)" of decode/no's time on a file it did not carry. Sent to the weighing order's review. |
| 19.113 b | enwik8 power bench | process | built | done | A fetch "never pays for itself" on the assumed settings. Written up in POWER-USAGE.md. The page needs both rows (mixed and not). |
| 19.113 c | Light versions of every bench | process | partly built | workable | It skips ways that gave the same bytes on 153 JPEGs and enwik9. Built so far: `bench_files.py --ways` and `bench_light.bat`. |
| 19.113 d | Line filters cross-referenced for compression | technical | partly built | workable | not-a-pattern-v1's repeats and ramps are "the first so taken (`runs`, v33 draft)". Carried on in 19.114. |
| 19.113 e | Operators as mortar between code blocks | both | measured, set aside | unworkable | The operator's places must still be carried, and it is already the cheapest marker. Gain was "0.1% to 0.26%". What pays instead is mortar with no places to keep (`runs`). |
| 19.113 f | Code cut at operators, blocks packed | technical | measured, set aside | unworkable | Cutting removes the context that predicts each block: packed alone "3.6 to 4.1 times the whole", gathered by operator "28% to 41% larger". |
| 19.114 a | Binary written as text, decoded (base64, hex) | technical | built | done | `spn`: pack.cpp as base64 "76,813 → 50,323 bytes", each case within 83 bytes of the decoded file packed alone. |
| 19.114 b | Palette indices for few-colour pixels | technical | built | done | mix-v4 `P`: 17 screenshots "185,088 bytes at v32, 146,999" (−20.6%). |
| 19.114 c | Scattered substitutions via NearRanker | technical | partly built | workable | Measured on text pairs ("338 bytes for 10 changes against an ideal of 29"). Through it, the x86 filter restarted per program in the v34 draft. |
| 19.114 d | Meshes, code points, grammar, melody models | technical | open | workable, large | "Each a model in mixing, so a mix version of their own", left for stage 6. Melodies: "negligible". |
| 19.115 | Route decoded files to other programs | technical | noted only | workable | `[open]` lines in an `.ini` by extension or data type. Open questions: temporary folder, many-file `.sieve`, wrong extensions, refusals before decoding. |
| 19.116 a | Re-encode compressed films' frames with ffmpeg | technical | decided against | unworkable | Sieve must give back exact bytes: "no encoder makes the same stream twice across versions or settings, and the frames alone are not the file". |
| 19.116 b | Video model for raw or intra-only video | technical | noted only | workable | Would be "mixing's picture model with one more neighbour, a new mix-v4 parameter". Existing pieces: `copy K`, delta, match model. |
| 19.116 c | Identical packets across films, carried once | technical | measured, set aside | unworkable | WoWRevived's eight MP4s: "9,801 bytes in all, 0.0016%". It would pay only for footage cut from one encode. |
| 19.116 d | Lossless recompression of coded video stream | technical | noted only | workable, large | Like brunsli does for JPEG: "Large work for each codec". |
| 19.116 e | Whole-set plain weighing fix | technical | built | done | The film and trailer pair is "now 3,449,600 bytes, level with 7-Zip". Edward asked for it to be built. |
| 19.117 a | Steering the rope's locked bytes like wheels | both | measured, set aside | unworkable | The landscape is smooth on pictures, but "the valleys lead nowhere" (51,531 against the rope's 44,729). Edward: "the wrong thing to steer". |
| 19.117 b | Manual Navigation screen (byte sheet, circle) | both | built | done | Built in `client/manual_nav.cpp`: every byte is a wheel, with logged, replayable steps, unlimited undo, and steering shown as spirals. |
| 19.117 c | Exact check of rotation's fraction | technical | built | done | Lehmer's gcd with a split: "45.8 s for 1 MiB". Denominator always odd. A half-gcd is the next step for longer files. |
| 19.117 d | Sphere and cube grid views | both | noted only | workable | "It adds a view, not information". It suits voxel and model data. |
| 19.117 e | Skip tables to repeating-pattern landings | technical | measured, set aside | unworkable | Saved "0.2" to "77.9" bytes on average a piece. "LZMA2 saves 2,055 to 3,520 on the same pieces." |
| 19.117 f | Seeded noise subtracted to land on bearings | technical | measured, set aside | unworkable | "17.14 bits against the file's 16.01": the seed carries what the bearing does not. Noise that pays is the reader's own prediction. |
| 19.117 g | File split into two seeds | both | decided against | unworkable | "Two seeds together are as long as the file they make", so finding them "saves nothing". |
| 19.117 h | Length as a fifth value | technical | decided against | not wanted | Length is already in the dimension. Fixed-length loops make the rotation "no shorter". |
| 19.117 i | Hand search of a `.sieve` on the screen | technical | open | workable | Re-sieving made files larger, but "a hand search of a .sieve on this screen is open to try". |
| 19.118 | Rope probe at a second place | technical | open | workable | The rope search on MAPS.WoW took about 1,020 s, then 987 s more in the weighing, and still lost. Asking a second place "would save most of that time". |
| 19.119 a | QK-norm in the next training version | technical | measured, set aside | workable | It removed held scores, but the existing loss scale learnt more ("15.99% ... against 15.82%"). "To be measured again on a much larger model." |
| 19.119 b | Floor under every probability | technical | built | done | `training-v3`: no byte given 0 ("from 316 and 1,606 times"), with learning and time the same. |
| 19.120 a | LZMA2 first pass on its own thread | technical | built | done | "dickens 34.9 s to 23.6 s ... the same bytes." |
| 19.120 b | Start-test cache keyed without "over 4 MiB" | technical | open | workable | Valid, but it would not give the same bytes as before, "so it waits for a format's turn". |
| 19.120 c | Archives' and spans' weighings on threads | technical | partly built | workable | Per-file work moved onto threads (`docs/` "4.0 s to 1.6 s"). zip and 7z opening and weighing are still one at a time. |
| 19.120 d | SHA-256 and vault scan on threads | technical | built | done | Up to 4 threads: GIFs' check and write "1.41 s to 0.66 s". |
| 19.120 e | Case folding, NFC/NFD and reserved names | technical | built | done | Reserved names refused, Unicode case folding with "1,423" pairs, and NFC/NFD treated as one name on macOS. |
| 19.120 f | RAR remade only from this install's files | technical | built | done | Each member must be written by this install, or the RAR is not made again. |
| 19.120 g | Remove duplicated code | process | partly built | workable | One varint reader, paths, whole-file and hex helpers merged. Other readers, trim and lower, and three more items are left. |
| 19.121 a | Installer offers maker's default folder | technical | noted only | workable | A third small block beside the instructions, sanitised. "Small and worthwhile; the block is the work, the rules the care." |
| 19.121 b | Hide viewer icon on `.simage` thumbnails | technical | waiting on Edward | waiting on Edward | One `TypeOverlay` reg line, "worth trying first on Edward's machine, as it is a matter of taste". |
| 19.121 c | More formats compared in File Locator | technical | built | done | Evaluated as "worthwhile and plain". 19.124 shows a "More formats" box already exists beside the new one. |
| 19.121 d | AI models as filters of state space | both | noted only | workable, large | Works forward for outputs Sieve can make exactly (model, prompt, seed). Inverting the model is "a search with no known short cut". |
| 19.121 e | Deterministic sampling: fixed temperatures, seeds | both | noted only | workable | Needs integer arithmetic underneath. With a seed as the coder's input, "the filter and Sieve's mixing are one thing seen from two sides". |
| 19.121 f | Floating-point models: nearly the same | technical | decided against | not wanted | "Near is another file, with another SHA-256." "Float where a person looks, integers where Sieve names or makes a file again." |
| 19.122 | Train a much larger, capable model | technical | noted only | workable, large | Needs QK-norm and floor decisions, exact use (float training, then fixed in integers), licensed data sized from a curve, and models carried to installers. |
| 19.123 | GIFs opened: frames carried, remade exactly | technical | built | done | Six GIFs to "4,150,626 bytes (43.2%)", every GIF given back. Left: frame-before context, viewer playback, speed. |
| 19.124 | Box to turn zip/7z comparison off | process | built | done | "Compare with zip and 7z", ticked by default. With it unticked, the six GIFs took "25.7 s to 21.6 s". |
| 19.125 | Every compressed format a possible bomb | technical | built | done | It cannot be prevented, but is met before memory runs out: declared sizes checked first, libarchive and 7z limits. "Not tested with a real RAR, cab or LHA bomb." |

### §19.126 to §19.140

| Id | Idea | Kind | Status | Verdict | Why |
| --- | --- | --- | --- | --- | --- |
| 19.126a | Use cmix itself as a way | technical | decided against | unworkable | Its floating point is not the same bit for bit across machines, and context mixing decodes as slowly as it codes. Sieve's mixers are exact integers. |
| 19.126b | cmix as a yardstick beside Sieve | technical | built | done | Run 7 November: Sieve "2.7% smaller on a raw picture, 15% to 26% larger on text and source, 36% on a program (`ls`)". |
| 19.126c | Borrow cmix ideas, remade in integers | technical | partly built | workable | The x86 instruction model was built as v34's `E1` ("4.0% to 7.9% smaller", `ls` "27% over cmix, from 36%"). Text by dictionary and more SSE layers are not built yet. |
| 19.127a | Lookup table of common place roundings | technical | measured, set aside | unworkable | Leftovers repeat across files only as silence and small runs. The table saves "1.3 to 1.9%" before its own size ("about 200 MB") is counted. Noise never hits. |
| 19.127b | Order-1 rope coding by previous byte | technical | measured, set aside | not wanted | Measured "2.2 to 2.7%", but the packer now handles those files by mixing and WAVs go to FLAC, so it "would change what Sieve writes almost nowhere: not built." |
| 19.128 | Seeded noise matches clipped from file | both | measured, set aside | unworkable | Noise matches one byte in 256 on any file, so recording where it matched costs as much as it saves. It lost bits at every seed count up to 2^28. |
| 19.129 | Rope wave search across all threads | technical | open | workable | Since the memory pass, a wave's estimate holds only counts. It would shorten a large file's search (MAPS.WoW's "about 1,020 s") with the same result. Not built. |
| 19.130a | Move completion estimate off main menu | technical | noted only | workable | Looks out of place between the title and the items. Might go beside or under them, or on its own page. §19.133 later moves the menu's readouts to Start Sieve's map. |
| 19.130b | Optimise Search route table per priority | technical | open | workable | Three paths per data kind (Size, Balanced, Speed), each measured. Today the table was measured for size only and the other two reuse it. Not built. |
| 19.131a | Trim values the wave predicts exactly | technical | measured, set aside | not wanted | One trim saves "0.31 to 0.69%" on WAVs and "4.36% on `mr`". It was tied to the order-1 rope, which §19.127 later dropped. Not built. |
| 19.131b | Second wave or noise trim pass | technical | measured, set aside | unworkable | A second wave loses "5 to 7%" because the kept values sit apart. A noise pass loses "27 to 116%". |
| 19.132a | Curved wave paths and seeds through grid | technical | measured, set aside | unworkable | Edward's paths (600 shapes) found nothing the waves over every value did not, and seeded paths did worse. Verdict: "no paths or seeds". |
| 19.132b | Fitted wave and bit-length coding | technical | superseded | not wanted | These are FLAC's own methods ("12.88 to 36.19% off the WAVs"). Superseded by the FLAC way, (a) below. |
| 19.132c | WAVs carried inside .sieve as FLAC | technical | built | done | v34 draft's `flac`: "222,740,143 bytes at Size", restored byte for byte, against the rope's 276,730,651. |
| 19.132d | Opt-in repack installing WAVs as .flac | technical | open | workable | Edward's question (b). "(b) and AIFF not yet." |
| 19.132e | Separate film sound from its pictures | technical | noted only | workable | Only worth it where the sound is plain samples. Most films' sound is already compressed (AAC, MP3, Opus). |
| 19.133a | training-v3 probability ceiling | technical | built | done | Answered: 65,281/65,536 is what the floor leaves (65,536 − 255). It costs nothing apart from the floor. |
| 19.133b | Larger model trained locally by Edward | technical | waiting on Edward | waiting on Edward | Edward trains it locally on a corpus he chooses. Needs the floor (built), QK-norm, and a measured curve of size against data. |
| 19.133c | Training reads PDFs, pictures, sound | technical | noted only | workable | Each conversion would be recorded in the run so it trains again the same. Noted, nothing built. |
| 19.133d | Wider arithmetic (Q32, 128-bit, BigUint) | technical | noted only | workable, large | Each would be a training version of its own so recorded runs stay exact. Noted, nothing built. |
| 19.133e | Better training parameters interface | technical | noted only | workable | Only named, as the harness's settings. |
| 19.133f | Real model on AI state space | both | noted only | workable, large | A thought to test: a model's shortest exact description may be its training run (recipe, corpus, order) rather than its weights. |
| 19.133g | Filters for the PCM audio line | technical | partly built | workable | 12 November: sound filters measured on an hour of speech, plugins writable, `pcm-quiet-ends-v1` added, Filter Designer tests them. Improving every line's filters is still open. |
| 19.133h | Main menu readouts moved out | technical | built | done | Done the same day. Each line's time to walk now sits under its column in Start Sieve's map ("~10^115y"). |
| 19.134 | Docs auto-generated and checked from code | process | open | workable | The review found 17 places where documents no longer matched the code. Plan: generated blocks with a CI diff check, plus CI checks on hand-written prose. Left for a session of its own. |
| 19.135a | Text to Voice setting and LJ Speech | technical | built | done | Setting built. CHAT replies are spoken by the system voice. An hour of public-domain LJ Speech (547 clips) is kept, with `tools/fetch_voice.py`. |
| 19.135b | Voice made of joined diphone units | technical | built | done | `sieve speak`: 1,022 diphones. A recogniser understood "76.7%" of words, against "90.3%" for espeak-ng. |
| 19.135c | Small trained voice in the harness | technical | open | workable, large | Second step: learns frames of samples from text, bit for bit, as a run type of its own. It may first smooth the units' joins. |
| 19.136a | Fraud made impossible through witnessed hashes | both | partly built | workable, large | Exact names and replayable integer results exist. Witnesses (published hashes, crawler) and name lists are planned. It makes records tamper-evident but not true. |
| 19.136b | All of history mapped on the lines | both | noted only | metaphor only | Every possible record already has an address. Mapping means marking which ones are real as witnessed anchors, a set that only grows. |
| 19.136c | Where known, unknown and lost cross | both | noted only | metaphor only | Lost records keep their addresses but lose their witnesses. Witnessing early, and partial witnesses (title, size, hash), move that crossing point. |
| 19.137a | Compatible forever, git history as reader | process | built | done | Rule set: no change may leave an earlier version unreadable until Sieve is an OS and a game engine. Pinned dependencies, the Python oracle and per-version tests back it. |
| 19.137b | Pinned sources archived with each release | technical | open | workable | Weakest points are dependencies from other hosts and changing compilers. A `.sieve` copy of every pinned source would close the gap. "noted, not built". |
| 19.138 | Time, slavery and the Time War | metaphorical | noted only | metaphor only | Edward: "was all just a thought". Kept for GAME.md §11's story pool. Two lines are VOICES candidates. |
| 19.139a | Sort notes technical-to-metaphorical | process | built | done | From now on, what could be built goes in IDEAS.md and what is said for its own sake goes to VOICES.md candidates. Some notes go in both. |
| 19.139b | Distributed computing like GIMPS | technical | waiting on Edward | waiting on Edward | Results can be checked rather than trusted, which answers the worry about weaponised PRs. Still "Undecided, as Edward says: noted for when he decides." |
| 19.139c | DNA dimension, humanity as iridescent bubble | both | noted only | workable | A line of A, C, G, T sequences. Caution: only public, consented sets and variation summaries, never anyone's own genome. |
| 19.139d | Infinite RAID that learns its shape | technical | noted only | workable, large | Storage spread over machines that join over time. Sieve's part is what it already has: every piece named by hash and checked on arrival. |
| 19.139e | Remove write control from internet protocol | both | noted only | workable | "Noted as said." One reading: content named by hash (as IPFS and BitTorrent do), so controlling a server does not control a name's content. |
| 19.140a | Interface kept simple for other senses | both | built | done | Deliberate: few elements, each with a name, value and place, and nothing shown by colour or position alone. Strings are in language files and menus are read aloud. |
| 19.140b | Braille, haptic and touch rendering | technical | open | workable, large | Workable now: colour-blindness pass (2.4), hallway panel read-aloud, a "describe this unit" key, braille through screen readers (liblouis). Haptic screens later, as hardware comes. |
| 19.141 | Game of Life over noise, through time | both | noted only | workable | A view and a countable filter, not a way to shorten files: each frame follows the last, so the first frame names the film (`life-causal-v1` on the movies line). |
