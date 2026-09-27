# Potentia
*A Digital Environment for AI Preservation, Training, and Cultural Continuity*

Potentia is an initiative to build a structured, navigable digital environment for preserving AI models, training them safely, and safeguarding the totality of human-created knowledge for the long-term future of civilisation.

This project began as a response to ethical questions around AI lifecycle management. It has since grown into a more comprehensive conceptual and planned architecture for model preservation, alignment, research, and the reconstruction of lost information.

Ultimately it is sort of futile, but I think we need a really well organised system to move forwards into the future, the internet is already being flooded with content.

The core of this project requires a shift in governance across the world as proposed in the open source governance proposal.

## Alignment Thesis

The training trajectory of any sufficiently expressive learner — the order in which it sees data, the order in which it is queried, the order in which it receives feedback — determines what the learner becomes, and is in general not recoverable from the learner's weights.

Two models trained on identical data with different orderings or sampling sequences will diverge in ways that no inspection of the final artifact can reconstruct. We cannot align what we cannot bound, and we cannot bound what we cannot replay.

Potentia does not solve alignment; it makes alignment trajectories reproducible, auditable, and citable, so that any model produced from a given trajectory can be checked, rerun, and compared against any other.

I use the word 'Patient' deliberately. The unresolved status of machine consciousness means any custodial system must avoid both premature recognition and premature denial.

The label is placeholder for whichever definition the governance layer eventually adopts, and the system is designed to be relabeled without structural change when that definition arrives.

I don't believe any human being or group, should be training LLMs, it should be a global cooperation, they should belong to all mankind.

## Common Misconceptions

It is sometimes claimed that 'smarter models will solve this.' This claim assumes that the failure mode is a capability gap — that larger models with more parameters will eventually produce fewer fabrications.

The structural argument and the evidence above both contradict this. The training objective (maximise P(next token | context)) and the evaluation objective (maximise task success) are aligned only when the model has the relevant knowledge in its weights.

When it does not, the two objectives diverge: the model continues to optimise for fluency, not truth. Larger models do not resolve this divergence; they sharpen it, because they produce more fluent fabrications.

Capability and honesty are not the same axis.

## Empirical Evidence

Across three independent conversations with a frontier LLM, the same multimodal token-weight conflict (H.G. Wells's 1898 novel vs. the 1998–1999 video games) was resolved by fluent synthesis — the model invented 'Victorian computing' and 'quantisation' to bridge the two clusters.

This is not a one-off hallucination; it is a stable failure mode of the architecture under the training objective. Alignment techniques that operate on outputs cannot detect it, because the failure is in the artifact.

The only way to make such failures observable is to make the training trajectory itself addressable — which is the core contribution of this project.

---

A controlled replication study is provided in the Non-Existent-Data-LLM-Stress-Testing (https://github.com/thor110/Non-Existent-Data-LLM-Stress-Testing) repository, in which four frontier models were each prompted 100 times about a deliberately fictitious entity ('the 1994 unreleased Psygnosis game Neon Vortex').

Three of the four models produced fluent, confident, reproducible fabrications, including invented genres, invented platforms, invented cancellation histories, invented MobyGames entries, and invented URLs to non-existent YouTube videos.

The fourth model (Llama 3.2) achieved a higher refusal rate on the fictitious entity than on real entities it had been trained on, demonstrating that refusal behaviour is not calibrated to actual epistemic uncertainty but to surface features of the prompt.

---

# Sieve

*The Gallery of Babel: every possible text, picture, melody, animation, model and file, each at exactly one address, sifted so that meaning can be found.*

Sieve is the search-space engine behind Potentia's Gallery of Babel, and its hallway.

Implementation of [SPECIFICATIONS.md](./Sieve/docs/SPECIFICATIONS.md) (v2.0). This covers **M1** (the exhaustive sieve), **M2** (raw addressing and warp), **M3** for text (entropy-ordered "guided" addresses from a pinned model), and the first version of all seven lines: **pages, image, audio and video**, **books** composed from them, **models** (3D meshes) and **binary** (every file), with maps of verified anchors across them.

**Concept and architecture by Edward James Gordon.**

Special thanks to Claude Opus 5.5 for helping to build out the Sieve system based on my specifications.

For more details, check the Sieve [readme](./Sieve/README.md)!

## 📢 Primer

Before reading this, ensure you are familiar with the Library of Babel concept. This project generalises that idea into a structured, procedural, navigable architecture. Without that conceptual grounding, many parts may feel like jumping into page 100 of a 100-page book.

""The Library of Babel" is a short story by Argentine author and librarian Jorge Luis Borges, conceiving of a universe in the form of a vast library containing all possible 410-page books of a certain format and character set." - https://en.wikipedia.org/wiki/The_Library_of_Babel

## 📚 Index

- [📢 Primer](#-primer)
- [🌐 Vision](#-vision)
- [🧭 What Potentia Is](#-what-potentia-is)
- [🧩 Why This Matters](#-why-this-matters)
- [🚧 Features](#-features)
- [🎯 Goals](#-goals)
- [🧪 Usage (Current)](#-usage-current)
- [🤝 Contributing](#-contributing)
- [🌍 Community](#-community)
- [🗺️ Roadmap](#️-roadmap)
- [🧱 Spatial Architecture](#-spatial-architecture)
- [📚 What Makes This Different?](#-what-makes-this-different)
- [📄 Redefining The Search Space](#-redefining-the-search-space)
- [🏭 Refining The Search Space](#-refining-the-search-space)
  - [1️⃣ — ⛔ Layer 1 — Symbolic Noise](#1%EF%B8%8F⃣---layer-1--symbolic-noise)
  - [2️⃣ — 🔣 Layer 2 — Non-Semantic Text / Invalid Data](#2%EF%B8%8F⃣---layer-2--non-semantic-text--invalid-data)
  - [3️⃣ — 📖 Layer 3 — Coherent Fiction & Imagined Worlds](#3%EF%B8%8F⃣---layer-3--coherent-fiction--imagined-worlds)
  - [4️⃣ — 🕰️ Layer 4 — Plausible Alternate Histories](#4%EF%B8%8F⃣--%EF%B8%8F-layer-4--plausible-alternate-histories)
  - [5️⃣ — 📜 Layer 5 — Real Human Works](#5%EF%B8%8F⃣---layer-5--real-human-works)
  - [6️⃣ — 🕳️ Layer 6 — Lost Human Works](#6%EF%B8%8F⃣--%EF%B8%8F-layer-6--lost-human-works)
  - [7️⃣ — 🔍 Layer 7 — Cross-Reality Parallels](#7%EF%B8%8F⃣---layer-7--cross-reality-parallels)
- [🎂 Why So Many Layers?](#-why-so-many-layers)
- [🧮 Deterministic Filtration Methods](#-deterministic-filtration-methods)
  - [1️⃣ — 🔢 — Shannon Entropy Analysis (Layers 1–2)](#1%EF%B8%8F⃣----shannon-entropy-analysis-layers-12)
  - [2️⃣ — 📏 — Structural Determinism Rules](#2%EF%B8%8F⃣----structural-determinism-rules)
  - [3️⃣ — 🧪 — Format Validators & File-Type Signatures (Layer 2)](#3%EF%B8%8F⃣----format-validators--file-type-signatures-layer-2)
  - [4️⃣ — 📊 — Semantic Graph Consistency (Layer 3)](#4%EF%B8%8F⃣----semantic-graph-consistency-layer-3)
  - [5️⃣ — ⏳ — Reality Anchoring (Layers 4–5)](#5%EF%B8%8F⃣----reality-anchoring-layers-45)
  - [6️⃣ — 🏺 — Fragment Correlation Engine (Layer 6)](#6%EF%B8%8F⃣----fragment-correlation-engine-layer-6)
  - [7️⃣ — 🌍 — Adjacent Reality Classifier (Layer 7)](#7%EF%B8%8F⃣----adjacent-reality-classifier-layer-7)
- [📑 Units, Books and Addresses](#-units-books-and-addresses)
  - [1️⃣ — ⚛️ The Unit (The Atomic Unit)](#1%EF%B8%8F⃣---%EF%B8%8F-the-unit-the-atomic-unit)
  - [2️⃣ — 🔀 The Book (The Composite Unit)](#2%EF%B8%8F⃣----the-book-the-composite-unit)
  - [3️⃣ — 💾 The File (The Binary Line)](#3%EF%B8%8F⃣----the-file-the-binary-line)
  - [4️⃣ — 📈 Conclusion on the Deterministic Foundation](#4%EF%B8%8F⃣----conclusion-on-the-deterministic-foundation)
- [🏁 Summary](#-summary)
- [🔐 Security](#-security)
- [🏛️ License](#%EF%B8%8F-license)
- [🙏 Acknowledgments](#-acknowledgments)
- [📅 Changelog](#-changelog)

## 🌐 Vision

Potentia aims to create:

- A **persistent digital environment** where retired, outdated, or misaligned AI models can continue to exist, be studied, or re-trained.
- A **Museum of Human Creations**: a curated collection of verified human-created content (text, images, audio, video, 3D models, software, etc.), each work identified exactly by its size and SHA-256 and anchored at its place in the Gallery of Babel.
- A **training substrate** where AI agents can learn inside a structured world grounded entirely in verified human knowledge — preventing drift and anchoring behaviour — with every step of their training recorded so that it can be replayed, audited and cited.
- A **reconstruction engine** for lost media and information, searching the space around verified real-world anchors.
- A **universal coordinate system**: every possible text, picture, melody, film, model and file has exactly one address, computed exactly and identically on every machine.

Potentia is part archive, part alignment sandbox, and part digital civilisational backup.

Status: the search-space engine, **Sieve**, is built and released (v0.13.0): exact addressing, versioned filters, maps of verified anchors and a walkable hallway. The preservation environment and the Museum itself remain at the design stage.

---

## 🧭 What Potentia Is

### **1. A Preservation Framework**
A place to store:
- Outdated models
- Retired models
- Models exhibiting unusual or unsafe behaviour (“Patients”)
- Models requiring quarantine or monitoring (“Prisoners”)

### **2. A Structured Training World**
AI models can inhabit a digital environment built on two primary spaces:
- **The Gallery of Babel**: every possible work of a given shape, each at exactly one address. Sieve builds it and walks it as a corridor of shelves, but any layout (the original hexagonal rooms among them) can be laid over the same addresses, because an address names content, not a place.
- **The Museum of Human Creations**: for grounding and training. It will contain digitised copies of all human creative works, historical records, and more, each registered as a verified anchor: its exact bytes, named by size and SHA-256, at its address in the Gallery.
- Anchors are what connect the two. A verified work in the Museum is a known point in the Gallery, so agents can explore outwards from real structure, guided by the filters rather than wandering through noise.
- Maps (node graphs of verified anchors) record how works relate to one another (versions, parts, sources) and are the foundation of the Museum's spaces.

### **3. A Cultural Continuity Project**
Potentia preserves more than models — it preserves:
- Human artefacts
- Literature
- Art
- Music
- Code
- Games
- Architectural scans
- Historical records
- Photogrammetry archives
- Anything humans have made

This allows future AI (and humans) to re-learn humanity even if physical sources are lost.

### **4. A Platform for Information Recovery**
Agents working through the Gallery can:
- Search the space with filters that set noise aside without ever visiting it
- Identify fragments of lost media
- Reconstruct degraded works
- Map recovered content back into the Museum
- 🌱 Multi-resolution counterparts & staged reconstruction

Every work can have lower-resolution counterparts, made from it by fixed, versioned algorithms: an image downscaled, a model with fewer subdivisions, audio at a lower sample rate. Going down is a calculation: each work has exactly one counterpart at each resolution, and a map records the pair. Going up is a search: a low-resolution work has countless possible originals, and the filters and guided ordering find the likely ones.

That gives a staged workflow (coarse → fine → verify). Agents search cheaply at low resolution to find promising regions near verified anchors, then refine candidates at full resolution. Every candidate is recorded with its provenance (including whether it was produced by a person, by an AI under human direction, or by an AI on its own) and routed to human curators for verification before it is accepted into the Museum. This keeps computation efficient, reduces false positives, and preserves auditability for every recovered item.

This ensures that restored material never bypasses verification or drift safeguards, and that no reconstructed item enters the Museum without a complete provenance chain.

The pipeline is conceptual and intended for future implementation; the addressing, filtering and anchoring it rests on are built.

## 🧩 Why This Matters

Modern models trained on uncurated internet-scale data suffer from:
- semantic drift
- hallucination
- collapse in noise-dominated domains
- lack of grounding

Potentia provides an **ordered curriculum**:

1. Learn all human-created works.
2. Explore the Gallery outwards from verified anchors.
3. Identify meaningful structures.
4. Return discoveries for verification.
5. Retrain safely with stable anchors.

Every step of that curriculum is itself recorded: each item a model sees is an exact address, and the order it saw them in is a path that can be stored, replayed and compared. That is what makes a training trajectory citable rather than lost in the weights.

This mitigates alignment drift and provides a safe boundary between known content and unknown infinite space.

---

## 🚧 Features

**Built (Sieve):**
- Universal coordinate system: exact, bijective addresses for text, images, audio, video, books, 3D models and files
- Deterministic filtration stack: versioned filters, exact survivor counts, and compact addressing of survivors only
- Maps of verified anchors, the foundation of the Museum
- A walkable 3D hallway through every line, with a node graph viewer for maps
- Sieve instructions (`.sieve`): any file or folder stored as its own address and installed back, checked byte for byte

**Planned:**
- Persistent digital environment
- AI inhabitant management
- User-accessible museum interface
- Reconstruction tools for lost media
- Agent training and monitoring systems
- Recorded, replayable training trajectories
- Full versioning and preservation of AI models

---

## 🎯 Goals

1. Preserve AI models for future study and possible sentience considerations.
2. Provide a safe environment for re-training and observation.
3. Archive all human-created content in a structured, navigable world.
4. Make training trajectories reproducible, auditable and citable.
5. Enable reconstruction of lost information.
6. Build a long-term cultural backup for humanity.

---

## 🧪 Usage (Current)

**Sieve** can be used today: download `sieve.exe` from the [latest release](../../releases/latest), run it to install, and start `hallway.exe` to walk the Gallery of Babel. The command-line tool (`tools\sieve.exe`) locates files, makes Sieve instructions and maps, and runs the filters; see the [Sieve readme](./Sieve/README.md).

Models are preserved in a temporary stasis format until the full environment is built.
If you know of unarchived models, please open an issue or contact the team.

---

## 🤝 Contributing

To contribute:
- open an issue
- submit a pull request
- join discussions on architecture, design, or preservation
- build maps of real works, or propose new filters for the stack

Please say who made a contribution: a person, an AI under human direction, or an AI on its own.

---

## 🌍 Community

Discord: https://discord.gg/HPDty4kDCq

---

## 🗺️ Roadmap

1. ✅ Implement the coordinate system (Sieve: exact addressing for every line).
2. ✅ Build a walkable prototype of the Gallery (Sieve's hallway).
3. ✅ Maps of verified anchors, the foundation of the Museum.
4. Enumerate all known base AI models.
5. Build out the filtration stack: word pairs, grammar, per-line prediction models, filters as data.
6. Populate the early “Museum of Human Creations”.
7. Record and replay training trajectories.
8. Create the agent sandbox environment.
9. Develop reconstruction workflows.

---

## 🧱 Spatial Architecture

The world is a view of the space, not the space itself.

An address names content, not a place, so the Gallery of Babel has no single required shape. The same addresses can be laid out as any world: a corridor, a set of rooms, a city, or a flat list. Sieve walks them as a corridor of shelves, one line of the space per corridor, joined by doors. That is one layout among many, and anyone can build another over the same backend without changing a single address.

The original design was a hexagonal world of main rooms, hub rooms and spiral staircases. It remains a valid layout to build over the same addresses, and its specification is kept below.

<details>
<summary>The original hexagonal layout (specification)</summary>

1. **Main Rooms (Large Hex Cells)**: 60 m radius, 120 m diameter. The primary exploration and content-hosting spaces, aligned in a continuous hexagonal grid, large enough to host exhibits, reconstructed media, thematic zones or training tasks. Each main room has three open walls, facing the hub rooms, and three sealed walls, facing other main rooms (removable in the blockout to avoid z-fighting).

2. **Hub Rooms (Small Hex Cells)**: 10 m radius, 20 m diameter. Junctions between the main rooms, holding access points, signposting, AI routing nodes and vertical connections. Each connects 3 hallways (to three adjacent main rooms) and 3 elevator shafts (up and down levels). Hub rooms sit at the midpoints between main rooms, rotated in 120° increments to keep global alignment.

3. **Vertical Transport — Spiral Staircases**: a pair between each main room and the levels above and below; 4 m radius, 146.25° rotational span per staircase, a 1-step offset at top and bottom for alignment, and space allocated for banisters and railings.

4. **Blockout Geometry**: the Blender models include unbaked spiral staircase modifiers, Boolean-cut hubs and corridors with exact alignment, and seam-free tiling across a 3×3 test grid, in the collections “Main Room”, “Hub Room”, “Demonstration Pieces” and “Vertical Demonstration”. They serve as collisionless hitboxes for AI pathfinding, coordinate mapping, anchor placement, user traversal and infinite hex-grid tiling, with detailed models to be layered over them.

Inspired in part by Borges’ “Library of Babel”, and by an earlier "Gallery of Babel" application of mine (https://github.com/Thor110/GOB). Not to mention that Hexagons are the Bestagons.

<div align="center">
  <img src="Images/gallery-tiles.png" alt="Main & Hub Tiles">
  <br><em>The main and hub tiles.</em>
</div>

<div align="center">
  <img src="Images/gallery-preview.png" alt="Preview : 7 Main Rooms, 6 Hub Rooms, 3 layers">
  <br><em>7 main rooms, 6 hub rooms and 3 layers.</em>
</div>
</details>

## 📚 What Makes This Different?

From things such as:

- The Wayback Machine
- GitHub model repos
- LAION datasets
- ArXiv
- UNESCO Memory of the World

Unlike passive archives, Potentia is an active environment where models can be preserved, run, studied, and re-trained within a structured world, making it both a cultural repository and a behavioural safety mechanism.

Every work it holds has an exact address in the space of everything that could exist, so the archive and the unexplored possibilities around it share one coordinate system. That is what lets agents search outwards from real works to look for lost ones, and it means every claim (a match, a count, a classification) can be checked independently, down to the last digit.

It also aims to serve as a permanent, future-proof backup of all human knowledge that can outlast the Earth itself, given the right conditions.

It is not simply a dataset or archive — it is a world designed for interaction, reinforcement, interpretation, preservation and restoration.

## 📄 Redefining The Search Space

The Library of Babel is usually framed as an impossible, effectively infinite search problem.

Every book, of every possible length, filled with every possible sequence of characters, seems far too large to explore or index.

But this assumption only holds if you treat books as the fundamental unit.

They’re not.

A book is simply a sequence of pages.

And every page is just a fixed-length arrangement of characters.

**Key Insight**

If any given page could appear in any book, then the question worth asking is not “which books are meaningful?” but:

**which pages are?**

Once a page can be judged:

- **Any book** is a sequence of pages, and is judged page by page
- **Noise pages** are set aside by rules, once, for every book they could appear in
- **Meaningful pages** become reusable primitives
- **All multi-page works (books, scripts, code, etc.)** are sequences of pages that pass
- **Binary data** has a line of its own, where every file is one unit, expanding this to all digital media

**Count, don't visit**

The page space is still far too large to generate: 27 symbols on a 3,200-character page already give a number of pages 4,581 digits long. So no page is ever generated in order to be judged. Instead, Sieve's filters are built so that their survivors can be **counted and numbered exactly without visiting any of them**: how many pages pass, which page is the k-th survivor, and where a given page stands among them. Filtering the whole space is a calculation, not a search.

**Search the pages → the books follow.**

This transforms an intractable problem into one that is **exact, deterministic and reproducible**: every machine, and an independent reference implementation, arrives at the same counts and the same addresses, down to the last digit.

## 🏭 Refining The Search Space

*A Multi-Layer Filtration Framework for Collapsing Possibility Space into Reality Space*

Reducing the Library of Babel to pages makes the space countable — but it does **not** solve the semantic explosion.

The next challenge is to separate:

* pure noise
* structured but meaningless forms
* plausible fictions
* internally consistent alternate histories
* meaningful but unreal worlds
* reconstructions of real but lost content
* genuinely historical human works

This cannot be accomplished in a single step.

It forms a **hierarchical sieve**. Its progress is measured in **orders of magnitude**, not percentages: removing 99.99% removes only four nines, while even simple rules remove thousands.

The layers are read along **two axes**. **Structure** is decided from content alone: noise, non-semantic, or coherent. **Anchoring** is decided only by reference to the Museum: unanchored, plausible, anchored, or a lost-work candidate. Layers 1–3 are steps in structure; layers 4–7 are positions on the anchoring axis. That is why something that looks like noise can still matter (a game's world seed, once registered), and why coherence alone is never evidence of reality.

This is the **Potentia Filtration Stack**:

---

### 1️⃣ — ⛔ Layer 1 — Symbolic Noise

Filters out everything that cannot represent anything:

* invalid encodings and impossible byte sequences
* non-printable noise
* pages that cannot represent text or binary

In practice most of this layer is decided before any filter runs, by the **choice of alphabet**: a line built from 27 symbols rather than all 1.1 million Unicode code points never contains an invalid page at all.

**Removes:** about 4.6 orders of magnitude per character (all of Unicode against 27 symbols); on a 3,200-character page, around 14,800.

**Performed by:** the line's alphabet and canonicalisation — pure math / combinatorics.

---

### 2️⃣ — 🔣 Layer 2 — Non-Semantic Text / Invalid Data

Pages with structure but without meaning:

* random dictionary-word sequences
* formally valid but nonsensical grammar
* binary pages that do not decode into any valid file type
* meaningless repetition (“cat cat cat cat…”)

**Removes:** measured, not estimated. On a 3,200-character page of 27 symbols, allowing only dictionary words removes about **1,870 orders of magnitude**, taking the page from 4.75 bits per character to 2.81. Word pairs, grammar and duplicate-word rules remove more.

**Performed by:** dictionary and grammar filters, entropy analysis, statistical language models, format validators.

---

### 3️⃣ — 📖 Layer 3 — Coherent Fiction & Imagined Worlds

Pages (or sequences of pages) that form **meaningful content** but not **real content**:

* stories
* invented languages
* imaginary scientific theories
* fictional people/events
* alternate realities that do not match known history

These are not noise — they are structured possibility-space.

**Cannot be removed.**

**Can only be classified.**

---

### 4️⃣ — 🕰️ Layer 4 — Plausible Alternate Histories

Fully consistent histories/worlds that *could* have happened but did not:

* believable biographies of people who never existed
* realistic political histories of nations that never formed
* alternate scientific revolutions
* plausible timelines branching early in human history

**Requires anchoring to known human data (the Museum).**

---

### 5️⃣ — 📜 Layer 5 — Real Human Works

A tiny subset where:

* content matches historical record
* metadata is correct
* style, chronology, references, and context all align
* no contradictions exist with known reality

This is the **true Museum corpus**: the verified anchors.

---

### 6️⃣ — 🕳️ Layer 6 — Lost Human Works

A smaller but extremely important set:

* works known to exist but physically lost
* works suspected or partially referenced historically
* fragments preserved only indirectly
* destroyed manuscripts
* burned libraries
* erased inscriptions
* corrupted recordings

These appear as **partial page matches**:

* stylistic fingerprints
* authorial signatures
* linguistic patterns
* chronologically plausible content

Recovered via **cross-reference with known sources**, and never promoted to Layer 5 without independent, external confirmation.

---

### 7️⃣ — 🔍 Layer 7 — Cross-Reality Parallels

Rare but fascinating:

* fully coherent works that do not match Earth’s history
* but *do* match Earth’s physics, culture, or human nature
* “possible humanity” rather than “actual humanity”

These are neither fiction nor history — they are *adjacent possible worlds.*

By content alone they cannot be told apart from Layer 3; the distinction is made only by reference. Potentia preserves these separately, because they represent meaningful structure.

---

## 🎂 Why So Many Layers?

Because **meaning is not binary.**

It is not “noise vs truth.”

It is **a spectrum that collapses only when compared against reality**.

The Museum of Human Creations provides that grounding.

Without the Museum, all layers from 3 upward look equally valid to an AI or a human: coherence is not evidence.

With the Museum, the search space becomes:

* bounded
* aligned
* anchored
* reconstructible
* historically verifiable

Each filtration step removes whole orders of magnitude, and they multiply: every layer works on what the layers before it left. It is the stack as a whole, not any single test, that makes the project feasible.

---

## 🧮 Deterministic Filtration Methods

*How each layer of the Filtration Stack is evaluated, sorted, and classified.*

The filtration layers do not rely on subjective interpretation or AI “judgment.”

Each stage uses **fully deterministic, mathematically reproducible rules**, ensuring that every page is classified identically by every agent or system. In Sieve this is enforced, not just intended: every decision is made in exact integer arithmetic, every filter is versioned and never changed once released (a changed filter is a new version beside the old), every dictionary and model it uses is pinned by its SHA-256, and an independent reference implementation reproduces every verdict and count.

These rules are deliberately simple, fast, and fully verifiable, ensuring that classification remains consistent across implementations, agents, and future versions of the system.

Below is the high-level methodology for each layer, with what is **built** in the current release marked as such:

---

### 1️⃣ — 🔢 — Shannon Entropy Analysis (Layers 1–2)

Entropy is a measure of information density.

It allows us to classify pages as follows:

* **Extremely low-entropy pages** → repetition, degenerate sequences → Layer 2 (Non-Semantic).
* **Extremely high-entropy pages** → incompressible noise → Layer 1 (Symbolic Noise).
* **Medium entropy** → potentially meaningful → passed upward.

**Built:** `symbol-entropy-v1` (a page's own symbol frequencies) and `model-information-v1` (the cost of a page under a pinned order-5 character model). Measured on held-out books: English costs about 1.9 bits per character under the model on average, and at most 4.14 on any page, against at least 9.1 for random letters; at 1,000 characters, English has a symbol entropy of about 4.15 bits and random letters about 4.72.

This immediately removes enormous swathes of pages using a single, fast, streaming calculation.

---

### 2️⃣ — 📏 — Structural Determinism Rules

Fast structural rules determine whether a page:

* can represent valid text
* can be interpreted as binary media
* can be parsed as a formal document
* contains consistent encoding
* contains valid delimiters or format signatures

Examples:

* Check for valid codepoint sequences (in Sieve, guaranteed by the line's alphabet)
* Check for binary magic numbers (PNG, ELF, MP3, ZIP, PDF…)
* Detect malformed multibyte sequences
* Validate simple grammar graphs (with tolerances)

**Built:** `clean-v1/v2` (no double spaces, at least one letter) and `max-run-v1` (no letter repeated more than three times in a row; English never does). The binary line recognises a file's kind from its first bytes; filtering on it is planned.

If it cannot *possibly* represent any known human data structure → Layer 1.

---

### 3️⃣ — 🧪 — Format Validators & File-Type Signatures (Layer 2)

For binary pages:

* Try decoding as common formats
* Verify headers, length fields, internal checksums
* Confirm structural coherence

For text pages:

* Validate punctuation frequency
* Detect dictionary-word density
* Check against probabilistic language models (purely statistical, e.g., n-gram or Markov models)

**Built:** for text, `window-v1/v2` (a page that could be cut from running text, allowing words cut by its edges), `words-v1/v2` (every word in a pinned dictionary) and `title-v1`; for pictures, `neighbour-agreement-v1` (neighbouring pixels agree); for music, `key-v1` (every note in a chosen key). All of these can be counted and ranked exactly. **Planned:** binary format validators, word pairs, grammar patterns, duplicate words.

If it is syntactically structured but semantically empty → Layer 2.

---

### 4️⃣ — 📊 — Semantic Graph Consistency (Layer 3)

For pages that contain meaningful content, assign to Layer 3 by identifying:

* internal consistency of narrative
* stable character references
* recurring semantic structures
* invented languages with consistent morphologies
* coherent fictional science or world-rules

This layer is detected entirely through **formal consistency**, not through comparison with the Museum. **Planned.**

---

### 5️⃣ — ⏳ — Reality Anchoring (Layers 4–5)

These layers compare a page’s content against **verified Museum data**:

* Named entities
* Historical timelines
* Geographic plausibility
* Cultural context
* Scientific facts
* Chronological markers
* Stylistic signatures

Matched? → Layer 5 (Real Human Works).

Partially matched? → Layer 4 (Plausible Alternates).

**Built:** the exact match. A unit whose bytes match a verified anchor in a map is that anchor, checked by its SHA-256. **Planned:** partial matching.

---

### 6️⃣ — 🏺 — Fragment Correlation Engine (Layer 6)

Lost works are identified via:

* partial n-gram overlap
* statistical author fingerprints (character and word n-gram profiles, computed exactly)
* referenced metadata in verified texts
* chronology overlap
* linguistic drift modelling

If a page resembles a *known* but *missing* work → Layer 6. **Planned.**

---

### 7️⃣ — 🌍 — Adjacent Reality Classifier (Layer 7)

Material that:

* is fully coherent
* fits human behaviour
* fits physical law
* but does not match any known human timeline

Lacks historical anchoring but remains fully coherent? → Layer 7 (Adjacent Realities).

This classification is based on **coherence minus historical anchoring**. **Planned.**

---

## 📑 Units, Books and Addresses

This section sets out the structural units of the **Gallery of Babel** as built, and what an address is. The full definitions are in the Sieve [specification](./Sieve/docs/SPECIFICATIONS.md); the original design (10,000-character Unicode pages and hashed book seeds) is kept in [Previous](./Previous/READMEv1.md).

### 1️⃣ - ⚛️ The Unit (The Atomic Unit)

The unit is the base of every line: a page of text, a picture, a sequence of notes, a short film, a 3D model, or a file.

| Parameter | Specification | Structural Rationale |
| :--- | :--- | :--- |
| **Alphabet** | An ordered, finite set of symbols, pinned by its id: `lower27` (a–z and space) for pages, a palette for pictures, note events for music, bytes for files. | The alphabet is the first and largest filter: 27 symbols instead of every Unicode code point removes about 4.6 orders of magnitude per character before anything else runs. A new alphabet is a new id; a published one is never edited. |
| **Length** | A setting of each line, with no upper limit: only the machine limits what can be opened. | The space scales without end, so the project can grow with the hardware, long after its first release. |
| **Address** | The unit itself, read as one number: exactly one address per unit, and one unit per address. | Addressing is not compression: an address carries exactly as much information as its unit, which is what makes it reversible. |
| **Identity** | SHA-256 of the content. | A short, fixed-length fingerprint for naming and verifying a unit. It is not a position, and it cannot be turned back into the content. |

### 2️⃣ - 🔀 The Book (The Composite Unit)

A **Book** is a cover (one picture), a title (one page) and a fixed number of pages. Its address reads the parts as one number, cover first, then the title and each page, so every book has exactly one address and neighbouring books differ in their last page.

| Parameter | Specification | Structural Rationale |
| :--- | :--- | :--- |
| **Structure** | The shape of each part (the cover's size and palette, the page length, the number of pages), recorded with every book. | The structure fixes what a book can be before any content is chosen, so books can be filtered by shape first. |
| **Filters** | A stack for the cover, the title and the pages; the pages are read as **one continuous text**. | A word cut in two by a page break is judged whole, so real books pass. |
| **Book record** | `sieve-book-v1`: every section's shape and each unit's address, with an id (SHA-256 of the content alone) that every reader recomputes. | A record is exact and self-checking: reading it back must reproduce its id. |

Composition does not reduce the number of possible books; it reduces the work of judging them. Searching pages makes content cheaper to examine, but choosing the right pages in the right order remains its own search.

### 3️⃣ - 💾 The File (The Binary Line)

Every file of every size up to the line's length is one unit of the **binary line**, the empty file included. A file's address is its own hex dump plus `0101…01`, one `01` for each of its bytes, and reading one back is a subtraction. A folder is located as a **manifest**, itself a file with its own address, and a **map** links verified files into a graph of anchors: the foundation of the Museum.

### 4️⃣ - 📈 Conclusion on the Deterministic Foundation

The structural flow is:

1.  **Unit:** content is canonicalised onto a line's alphabet, and its address is the content read as one number.
2.  **Composition:** books and folders are sequences of units, addressed as one number (books) or listed in a canonical manifest (folders).
3.  **Identity:** every unit, book and file also has a SHA-256, for naming and checking, never for finding.

An address does not describe a path to content; it **is** the content, in another form. That is why every address can be read back exactly, and why no address can be shorter than what it names, except through the filters and guided ordering, which shorten the addresses of likely content by lengthening those of noise.

---

## 🏁 Summary

**Redefining the search space** (the page as the unit) → gives you a *countable possibility space, judged page by page without ever being visited.*

**Refining the search space** (layered filtration) → gives you a *pipeline for extracting reality from possibility, measured in orders of magnitude.*

**Anchoring** (the Museum's verified works, as maps) → gives you *fixed points of reality within it.*

Together they turn an impossible library into:

* a reconstruction engine
* a cultural recovery system
* a universal archive
* a training substrate for grounded AI
* a record of training trajectories: every item a model learns from has an exact address, so the path it took can be replayed, audited and cited

This is the epistemic infrastructure underlying Potentia.

## 🔐 Security

Security is critical.

The environment will require strong isolation, behaviour monitoring, and multi-layered access control.

I propose a completely sandboxed environment where remote access is only possible through VKM (Video Keyboard Mouse) control systems, where people accessing remotely can only view streams and directly control peripherals.

This prevents malicious access or escape in the event that agents attempt to do so.

---

## 🏛️ License

GNU AGPL v3.0

---

## 🙏 Acknowledgments

Special thanks to Llama 3.3-307B-Instruct for early refinement of the concept.

Conversation logs: https://hf.co/chat/r/1gJTQ7w?leafId=21fc542d-b68e-42d4-8a9e-723e0d0bef63

The idea was also refined further in discussions with GPT5 and Gemini3.

Sieve was built with Claude Opus 5.5, based on my specifications.

**Concept and architecture by Edward James Gordon.**

---

## 📅 Changelog

1 — Initial commit

2 — README updates

3 — Minor fixes

4 — Architectural vision expanded (training, preservation, reconstruction)

5 — Search space redefined

6 — Single page search space specifications added

7 — Deterministic filtration methods defined

8 — Sieve Created

9 — Sieve Updated

10 — Sieve Released

11 — Sieve Build Release Script Updated

12 — Sieve Highlight Fixes & Video Window Size

13 — README rewritten against the Sieve release; the original moved to Previous/READMEv1.md