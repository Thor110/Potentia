# Potentia — Gallery of Babel Specifications

*Version 2.0. Concrete design for the search-space engine and its navigable view. Supersedes the search-space sections of the README and Specifications v1.0.*

**Concept and architecture by Edward James Gordon.**

---

## 1. Purpose

The Gallery of Babel is **not** an attempt to enumerate every possible work. It **orders** the space of possible works so that meaningful content is reachable, classifiable and verifiable, and noise is pushed to the margins.

| The system does | The system does not |
| :--- | :--- |
| Address every unit uniquely and reversibly | Enumerate the meaningful set at realistic scales |
| Locate any known work instantly (warp) | Recover lost works from content alone |
| Classify any unit deterministically | Treat fluency or apparent meaning as evidence of reality |
| Guide navigation towards meaningful regions | Delete anything: every unit remains addressable |
| Anchor units to verified real-world works | Use hashes as navigable addresses |

**Locating is instant; discovering is the hard part.** A known work is found by encoding it, never by searching. The rest of the system exists to make discovery of unrecorded works as well-guided as possible.

---

## 2. Core Quantities

Text estimates assume ~1 bit/char of real information in meaningful English (Shannon's range: 0.6–1.3).

| Space | All possibilities | Meaningful (est.) |
| :--- | :--- | :--- |
| Babel page (25 symbols × 3,200 chars) | ~10^4,473 | ~10^963 |
| Babel book (25 symbols × 1,312,000 chars) | ~10^1,834,097 | ~10^394,951 |
| Paragraph (27 symbols × 1,000 chars) | ~10^1,431 | ~10^301 |

- Meaningful fraction of Babel books: ~**10^-1,439,146**.
- Physical ceiling on computation (Lloyd): ~**10^120** operations for the observable universe.
- Conclusion: filtering removes almost all of the space, but the remainder is unenumerable beyond small unit lengths. The design prioritises **navigation, location and verification** over enumeration.

---

## 3. Lines (State Spaces)

The Gallery consists of **four lines**, one per modality. Each line is a one-dimensional address space defined by a parameter set, so every line can be dialled from trivially small to full scale.

| Parameter | Description |
| :--- | :--- |
| `MODALITY` | `text`, `image`, `audio`, `video` |
| `ALPHABET` | Ordered, finite symbol set (characters, pixel values, note events) |
| `UNIT_LENGTH` | Symbols per unit (characters, pixels, events, frames × pixels) |
| `MODEL` | Pinned scorer for entropy ordering and classification (by hash) |
| `SPEC_VERSION` | Version of this spec and all pinned data |

Rules:

- Unicode alphabets are **pinned to a specific Unicode version** and contain only assigned, printable codepoints. Surrogates, noncharacters and unassigned codepoints are excluded.
- Every quantity (space size, meaningful estimate, filter survival rate) is computed and reported **per parameter set**.
- Line lengths **do not need to match**. Doors map between lines by fractional position (§7).

### 3.1 Starting Sizes

| Line | Starting configuration | Possibilities | Status |
| :--- | :--- | :--- | :--- |
| Text | 27 symbols × 4–8 chars | 5.3 × 10^5 – 2.8 × 10^11 | Exhaustive (M1) |
| Text | 27 symbols × 1,000 chars | ~10^1,431 | Guided only |
| Image | 5×5, 1-bit | ~3.4 × 10^7 | Exhaustive |
| Image | 6×6, 1-bit | ~6.9 × 10^10 | Exhaustive with patience |
| Image | 10×10, 1-bit | ~10^30 | Guided only |
| Image | 10×10, 16 colours | ~10^120 | Guided only (equals Lloyd's limit) |
| Image | 10×10, 24-bit | ~10^722 | Guided only |
| Audio | Symbolic: 104 note events (26 pitches/rest × 4 durations) × 6 events | ~1.3 × 10^12 | Exhaustive with patience |
| Audio | Symbolic: 104 note events × 16 events | ~10^32 | Guided only |
| Video | 5×5, 1-bit × 8 frames | ~10^60 | Guided only (deferred) |

Audio begins as a **symbolic** line (note sequences), not raw samples. Raw audio at even one second of 8 kHz 8-bit is 256^8,000 possibilities and carries almost no structure per sample; symbolic audio reuses all of the text machinery unchanged.

---

## 4. Address Map

Every unit on a line has exactly one address, and every address decodes to exactly one unit (a bijection). Each line is a **loop**: the address interval `[0, 1)` wraps around.

### 4.1 Raw Mode

- **Positional:** address = unit read as a base-`|ALPHABET|` number. Neighbours share a prefix.
- **Scrambled:** positional address passed through a fixed invertible permutation (Borges-style distribution).
- Address length = `UNIT_LENGTH × log₂|ALPHABET|` bits. No compression is possible.

### 4.2 Entropy-Ordered Mode

- Addresses are produced by **arithmetic coding driven by the line's pinned model**.
- The interval `[0, 1)` is divided so that probable (meaningful) units occupy wide arcs and noise occupies vanishingly thin ones.
- A meaningful unit's address length ≈ its information content (~1 bit/char for English, vs ~4.75 in raw mode).

**Warning:** entropy-ordered decoding is equivalent to sampling from the model. A random address decodes to fluent output that **carries no evidence of being real**.

### 4.3 Determinism

Encoding and decoding must be **bit-for-bit identical** on every machine.

- All address arithmetic uses **exact integers** (arbitrary precision). Floating point is prohibited in the address path: a double holds 53 bits of precision, while a single paragraph address needs ~4,755.
- The arithmetic coder uses integer or fixed-point arithmetic with fully specified rounding.
- Model outputs are **quantised to integer frequency tables** before entering the coder.
- Models (weights, tokenizer, quantisation) are pinned by **cryptographic hash**. Changing a model changes every entropy-ordered address on that line, so models are versioned, never silently replaced.

---

## 5. The Hallway

Each line is presented as a **single hallway**: an endless corridor with a shelf on each side. Position along the hallway *is* the address.

### 5.1 Layout

- **Shelf, door, repeat.** Shelving runs continuously; doors (§7) appear at fixed intervals.
- **Sides:** left and right shelves hold consecutive halves of the local range.
- **Views:**
  - **Guided view** (default, entropy-ordered): shelf length ∝ probability. Meaningful units fill long runs; noise is too thin to occupy floor space. Only shelved units (§9) are shown.
  - **Raw view** (diagnostic): fixed-width units, every unit shown, noise everywhere. This is Borges' library, and the contrast with the guided view is the thesis made visible.

### 5.2 Zoom (Depth)

The hallway is navigable at any scale through **zoom depth** `d`:

- At depth `d`, each shelf segment represents all units sharing a `d`-symbol prefix, labelled with that prefix.
- Walking into a segment expands it into its next symbol. At `d = UNIT_LENGTH`, segments are individual units (books).
- In the guided view, segment length ∝ the prefix's total probability, so at every depth the routes towards meaning are wide and routes into noise are hairlines. Structurally this is a trie; physically it remains one hallway.

### 5.3 Readout

The client always displays: line, view, zoom depth, current address (truncated, expandable) and fractional position.

### 5.4 Appearance

The hallway is a wireframe. Each line has exactly two colours, a solid background and the colour of every edge, and doors are solid black:

| Line | Background | Edges |
| :--- | :--- | :--- |
| Text | black | white |
| Image | blue | cyan |
| Audio | green | amber |
| Video | red | yellow |

One tile of geometry (a bookcase and a door on each wall) is built once and repeated along the corridor. Edges fade towards the background with distance. Each tile holds 200 consecutive units: the left wall, then the right, shelf by shelf from the top, in walking order along each shelf.

---

## 6. Warp

A **warp** moves the reader directly to any unit.

### 6.1 Inputs

- **Paste:** a page, paragraph, image or melody is dropped into the warp box and encoded.
- **Address:** a raw or entropy-ordered address is entered directly.

### 6.2 Canonicalisation

Pasted input is fitted to the line's parameter set by **fixed, versioned rules**, so the same input always lands in the same place:

- **Text (`canon-text-v2`, default):** whitespace becomes SPACE; typographic punctuation becomes ASCII (curly quotes, dashes, ellipsis); Latin accented letters U+00C0–U+017F fold to ASCII (`é`→`e`, `ß`→`ss`); ASCII case folds where the alphabet has no capitals. Any character still outside the alphabet is then handled one of two ways: apostrophes are removed (`didn't`→`didnt`), and everything else becomes SPACE (`well-known`→`well known`). Finally, SPACE runs collapse, the text is trimmed, split into units and padded to `UNIT_LENGTH`. `canon-text-v1` removed every out-of-alphabet character, which glued words together; it remains selectable so that earlier results stay reproducible.
- **Image (`canon-image-v1`):** composite onto black; stretch to the line's dimensions by exact integer area-averaging, rounded half up; quantise each pixel to the nearest palette colour by squared RGB distance, with ties to the lowest index. Video applies this per frame, drops frames beyond `FRAMES`, and pads short input with black frames.
- **Audio (`canon-notes-v1`):** note notation (`C4q F#5e Bb4h Rq`) converts to the 104-symbol event alphabet (a rest or C4–C6, each with four durations). Flats are written as sharps, out-of-range notes move by whole octaves into C4–C6, and the last unit is padded with eighth rests.

Palettes are pinned by the implementation: `mono` (2), `ega16` (16), `rgb332` (256) and `rgb24` (16,777,216). Index 0 is black in every palette.

The warp reports every transformation applied, so the reader knows exactly what was located.

### 6.3 Multi-Unit Input

Input longer than one unit becomes a **sequence of addresses**. The warp lands on the first and displays the rest as a trail.

### 6.4 Arrival

- **Shelved unit:** the reader arrives at its shelf position.
- **Unshelved unit:** the unit is presented **in hand**, as a book the reader is holding, with its address and classification, but with no shelf position and nowhere to be returned to. Units never physically leave the shelves; the in-hand view is a separate presentation.

---

## 7. Doors

Doors connect the four lines in a cycle: **Text → Image → Audio → Video → Text**.

### 7.1 Mapping

A door maps by **fractional position**. For a unit at address `a` on a line of size `N`, the door leads to address

`b = ⌊a × M / N⌋` on the target line of size `M`,

computed with exact integer arithmetic. In entropy-ordered mode, the fraction is the lower bound of the unit's coded interval, decoded on the target line.

- When source and target lines have the **same bit length**, the door is a bijection: the same bits, read by a different decoder.
- When lengths differ, the mapping is many-to-one in one direction. The client therefore records the **return path**: stepping back through the door the reader just used always returns them to their exact origin. Every door passed is kept on a stack, so a whole chain of doors can be retraced exactly; a warp starts a fresh walk and clears it.
- Doors sit in the walls: the left wall's doors lead to the next line in the cycle, the right wall's to the previous one. The reader comes in through the opposite wall's door.

### 7.2 What Doors Demonstrate

Meaning does not survive a change of decoder. Stepping from a meaningful paragraph through a door almost always lands in visual or musical noise. What a string of bits means depends on what reads it, and the doors show this without explanation.

---

## 8. Classification

Classification uses **two independent axes**. Every result records the parameter set, scorer versions and scores that produced it.

### 8.1 Structure (Intrinsic)

Decided from content alone, relative to the pinned models.

| Level | Name | Method |
| :--- | :--- | :--- |
| S0 | Noise | Encoding validity, high Shannon entropy, low model likelihood |
| S1 | Non-semantic | Low entropy, repetition, dictionary density, n-gram/Markov scores, format validators |
| S2 | Coherent | Model likelihood above threshold, internal consistency checks |

"Noise" means **no pinned model finds structure**. It is relative to the models in use and may change when models improve; classifications are therefore versioned.

### 8.2 Anchoring (Extrinsic)

Decided by reference to the Anchor Registry (§10), never by content alone.

| Level | Name | Meaning |
| :--- | :--- | :--- |
| A0 | Unanchored | No link to any verified work |
| A1 | Plausible | Partial overlap with anchored works (names, dates, style) but no match |
| A2 | Anchored | Verified Registry entry |
| A3 | Lost-candidate | Independent evidence that a lost work existed, plus a proposed candidate; never promoted to A2 without external confirmation |

### 8.3 Mapping From the v1 Layers

| v1 Layer | v2 Classification |
| :--- | :--- |
| 1. Symbolic noise | S0 / A0 |
| 2. Non-semantic structure | S1 / A0 |
| 3. Coherent fiction | S2 / A0 |
| 4. Plausible alternate history | S2 / A1 |
| 5. Real human works | any S / A2 |
| 6. Lost human works | S2 / A3 |
| 7. Adjacent realities | S2 / A0 (indistinguishable from Layer 3 by content) |

The two axes express cases the single ladder could not: content that is noise but matters by reference.

### 8.4 Worked Examples

| Unit | Classification | Notes |
| :--- | :--- | :--- |
| `蹪ꡎ孺徨` drawn uniformly from full Unicode | S0 / A0 | A translator and an LLM fluently linked it to an opera melody system and a prophecy book. The input is noise; the interpretation is fabricated meaning. |
| Decorative pseudo-hieroglyph wall texture from a game, registered | S0 / A2 | Noise as text, but a verified human creation. Shelved by virtue of its anchor. |
| A game's published world seed, registered | S0 / A2 | Indistinguishable from noise; meaningful entirely by reference. |
| A pasted paragraph of Dickens | S2 / A2 | Located instantly by warp. |
| A fluent biography of a person who never existed | S2 / A1 or A0 | Coherence is not evidence. |

---

## 9. Shelving

The address space is complete; the shelves are curated.

- A unit is **shelved** if it is **S2**, or if it is **A2/A3** at any structure level.
- All other units (S0, S1 unanchored) are **unshelved**. They remain fully addressable and can be reached by warp or address, where they are presented in hand (§6.4).
- In the guided view, unshelved units already occupy negligible length; omitting them removes the remaining gaps. In the raw view, every unit is shown regardless.
- Shelving thresholds are part of the pinned parameter set.

---

## 10. Anchor Registry (The Museum)

The Registry is the only way to add verified works to the navigable space. It must be closely guarded against abuse.

### 10.1 Entries

Each entry stores:

- **Content hash** (SHA-256 of the canonical form) for identity.
- **Addresses** on the relevant line (raw and entropy-ordered, under pinned models).
- **Metadata:** title, creator, date, type.
- **Provenance:** source, evidence, chain of custody.
- **Status:** anchored (A2), lost-candidate (A3), or retracted.
- **Signatures** of the approving curators.

Hashes prove identity but cannot be reversed into content and cannot address the possibility space. They are never used for navigation.

### 10.2 Safeguards

- **No anonymous writes.** Submitters are identified; submissions are rate-limited.
- **Independent review.** An entry requires sign-off from at least two independent curators, each signing with their own key.
- **Append-only log.** Entries are recorded in a hash-chained, publicly auditable log. Nothing is deleted; a retraction is itself a logged entry with a stated reason.
- **Pinned snapshots.** Clients and classifications reference a specific Registry snapshot by hash, so results remain reproducible.
- **Content policy.**
  - No secret or personal material (keys, passwords, private data).
  - Copyrighted works may be registered by hash and metadata alone, with content stored only where licensing permits.
- **Lost-candidate discipline.** A3 entries are always displayed as candidates and require independent external evidence for promotion.

Curator governance (appointment, removal, disputes) is defined separately from this specification.

---

## 11. Composition

- A **book** (or any larger work) is an ordered sequence of unit addresses plus a structure record (unit count, formatting, type).
- Composition does not reduce the number of possible books. Searching units reduces the work of *examining* content; choosing the correct ordering remains a separate search.

---

## 12. Architecture and Implementation

- **Core** (headless): addressing, canonicalisation, classification, door mapping and Registry access, exposed through a narrow interface and a command-line tool.
- **Client** (the hallway): rendering and input only. It asks the core what is at a position and **never decides content itself**, so it can be replaced without touching the core.

Implementation (as built):

1. **Core** in C++20 with no dependencies: exact integer arithmetic throughout the address path, SHA-256, the scramble, canonicalisation for every line, and the sieve. The same code serves the command-line tool and, later, the hallway client.
2. **Independent reference oracle** in Python. It shares no code with the core and generates the conformance vectors; the core must reproduce them bit for bit.
3. **Conformance suite:** fixed inputs with expected addresses, canonical forms and image quantisations. It runs on Windows, Linux and macOS on every push, which also confirms that every platform produces identical addresses.
4. **Pinned data:** dictionaries are registered by id and SHA-256 (`data/dictionaries/dictionaries.tsv`) and are refused if their hash no longer matches.
5. **Models as pluggable scorers** (M3 onwards), run through existing libraries, with quantised integer outputs passed to the core.
6. **Hand-tuned code** (SIMD, assembly) only where profiling proves it worthwhile, and only once it passes the full conformance suite.

---

## 13. Milestones

| # | Milestone | Deliverable | Status |
| :--- | :--- | :--- | :--- |
| M1 | Exhaustive sieve | Enumerator + S0/S1 filters over **every** unit: text at lengths 4–8, images at 5×5 and 6×6 1-bit; plot of surviving fraction vs. size | **Text done.** Exact counts to length 1,000, cross-checked by brute force and pruned walk. Images not started: needs image S0/S1 filters. |
| M2 | Raw addressing and warp | Positional and scrambled bijections, canonicalisation rules, CLI warp, round-trip tests | **Done**, for all four lines. Includes neighbour stepping. |
| M3 | Entropy-ordered addressing | Integer arithmetic coder with a small pinned text model; measured bits/char on real text | Not started |
| M4 | Hallway prototype | 2D side view: shelves, zoom depth, readout, warp box, in-hand view, guided/raw toggle | **Built directly in 3D** (see M8). The raw view is done: shelves, readout, warp and go-to, in-hand view, ordering toggle. Zoom depth and the guided view are still to come; the guided view needs M3. |
| M5 | Doors | Image and symbolic audio lines; fractional door mapping with return paths | **Done.** Exact door mapping in the core, with a return-path stack in the hallway. |
| M6 | Anchor Registry | Local, signed, append-only Registry with review workflow; anchored units shelved and marked | Not started |
| M7 | Paragraph-scale sampling | Classification and sampling at `UNIT_LENGTH` ≈ 1,000; extrapolation checked against M1 | Not started |
| M8 | 3D hallway | 3D client over the same core interface | **Raw view done.** Wireframe SDL3 client over the core: books, doors, the four line colours, warp, playback of audio books. |

### 13.1 Deferred

- **Similarity map and hexagonal gallery.** A many-dimensional "what is like this?" view, for which the hex tiles (main rooms, stair hubs, three-exit layout) remain the intended geometry.
- **Raw audio**, **video** beyond minimal sizes, and **arbitrary binary** lines.

---

## 14. Known Limits

- Exhaustive enumeration is only possible at small unit sizes (see §3.1).
- Faster code gives constant-factor gains, not orders of magnitude.
- Guided navigation cannot distinguish real, lost and invented works without external evidence.
- "Noise" is model-relative; better models can reclassify units.
- Humans and models both perceive patterns in noise; the system must not.
