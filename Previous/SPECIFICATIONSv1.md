# Potentia — Gallery of Babel Specifications

*Concrete design for the search-space engine. Supersedes the search-space sections of the README.*

**Concept and architecture by Edward James Gordon.**

---

## 1. Purpose

The Gallery of Babel is **not** an attempt to enumerate every possible work. It is a system that **orders** the space of possible works so that meaningful content is reachable, classifiable and verifiable, and noise is pushed to the margins.

| The system does | The system does not |
| :--- | :--- |
| Address every unit uniquely and reversibly | Enumerate the meaningful set at realistic scales |
| Classify any given unit deterministically | Recover lost works from content alone |
| Guide navigation towards meaningful regions | Treat fluency or apparent meaning as evidence of reality |
| Verify units against external anchors (the Museum) | Use hashes as navigable addresses |

---

## 2. Core Quantities

Estimates assume ~1 bit/char of real information in meaningful English text (Shannon's range: 0.6–1.3).

| Space | All possibilities | Meaningful (est.) |
| :--- | :--- | :--- |
| Babel page (25 symbols × 3,200 chars) | ~10^4,473 | ~10^963 |
| Babel book (25 symbols × 1,312,000 chars) | ~10^1,834,097 | ~10^394,951 |
| Paragraph (27 symbols × 1,000 chars) | ~10^1,431 | ~10^301 |

- Meaningful fraction of Babel books: ~**10^-1,439,146** (about 1.44 million orders of magnitude of noise).
- Physical ceiling on computation (Lloyd): ~**10^120** operations for the observable universe.
- Conclusion: filtering removes almost all of the space, but the remainder is still unenumerable beyond small unit lengths. The design therefore prioritises **navigation and verification** over enumeration.

---

## 3. State Space Definition (Parameterised)

The state space is defined entirely by a small set of parameters, so it can be dialled from trivially small to full scale.

| Parameter | Description | Example values |
| :--- | :--- | :--- |
| `ALPHABET` | Ordered, finite symbol set | `a–z + space` (27), Babel (25), ASCII printable (95), bytes (256), pinned Unicode set |
| `UNIT_LENGTH` | Symbols per unit | 4–12 (testing), 1,000 (paragraph), 3,200 (Babel page) |
| `ENCODING` | How symbols are stored | Text symbols, raw bytes |
| `SPEC_VERSION` | Version of this spec and all pinned data | `1.0` |

Rules:

- **Unicode alphabets are pinned to a specific Unicode version** and contain only assigned, printable codepoints. Surrogates (U+D800–U+DFFF), noncharacters and unassigned codepoints are excluded, so the alphabet size is exact.
- Every quantity (space size, meaningful estimate, filter survival rate) is computed and reported **per parameter set**.

---

## 4. Address Map (1D)

Every unit has exactly one address, and every address decodes to exactly one unit (a bijection).

### 4.1 Raw Mode

- **Positional:** address = unit read as a base-`|ALPHABET|` number. Neighbours share a prefix.
- **Scrambled:** positional address passed through a fixed invertible permutation. Meaning is spread evenly (Borges-style browsing).

Address length equals unit length × log₂|ALPHABET| bits. No compression is possible in raw mode.

### 4.2 Entropy-Ordered Mode

- Addresses are produced by **arithmetic coding driven by a language model**.
- The unit interval is divided so that probable (meaningful) units occupy wide arcs and noise occupies vanishingly thin ones.
- A meaningful unit's address length is approximately its information content (~1 bit/char for English), instead of ~4.75 bits/char in raw mode.
- A random address decodes, with high probability, to meaningful-looking output.

**Warning:** entropy-ordered decoding is equivalent to sampling from the language model. Output from a random address is fluent by construction and **carries no evidence of being real**.

### 4.3 Determinism Requirements

Encoding and decoding must be **bit-for-bit identical** on every machine, or addresses resolve to different units.

- The arithmetic coder uses **integer or fixed-point arithmetic only**, with fully specified rounding.
- Language model outputs are **quantised to integer frequency tables** before entering the coder.
- The language model (weights, tokenizer, quantisation) is pinned by **cryptographic hash** as part of the parameter set.
- Changing the model changes every entropy-ordered address, so models are versioned, never silently replaced.

---

## 5. Similarity Map (High-Dimensional)

A 1D ordering can only make shared-prefix units neighbours ("This is" / "Thin ice", not "This is" / "That is"). Similarity requires many dimensions.

- Each unit may be assigned a **vector embedding** capturing semantic similarity.
- Nearest-neighbour search answers "what is near this unit?"
- The **hexagonal gallery** is the navigable, spatial view onto this map. Rooms hold clusters of similar units; the address line remains the canonical identity.

| Geometry | Answers | Canonical? |
| :--- | :--- | :--- |
| Address line (§4) | "Which unit is this, exactly?" | Yes |
| Similarity map (§5) | "What is like this unit?" | No (derived, rebuildable) |

---

## 6. Classification Stack

Classification is deterministic: given the same parameter set and pinned data, every implementation must produce the same layer for the same unit.

| Layer | Name | Decidable from content alone? | Method |
| :--- | :--- | :--- | :--- |
| 1 | Symbolic noise | Yes | Encoding validity, Shannon entropy (high) |
| 2 | Non-semantic structure | Yes | Entropy (low), repetition, dictionary density, n-gram/Markov scores, format validators |
| 3 | Coherent fiction | Yes | Language model likelihood, internal consistency checks |
| 4 | Plausible alternate history | **No** | Requires comparison against Museum anchors |
| 5 | Real human works | **No** | Exact or near-exact match to Museum anchors |
| 6 | Lost human works | **No** | Requires independent external evidence (fragments, references) |
| 7 | Adjacent realities | **No** | Coherent, unanchored; indistinguishable from Layer 4 by content |

Rules:

- Layers 4, 6 and 7 **cannot be separated by content alone**. A "lost work" candidate is only ever a candidate until external evidence confirms it.
- Every classification records the parameter set, filter versions and scores that produced it (provenance).

### 6.1 Worked Example: Fabrication From Noise

Four characters drawn uniformly from full Unicode: `蹪ꡎ孺徨` (stumble, a Phags-pa letter, child, wander). A translator produced "Tui'er Huang", and an LLM then fluently linked it to an opera melody system and a Tang dynasty prophecy book. The input is verified noise (Layer 1); the interpretation is fabricated meaning. The classification stack must place the input in Layer 1 regardless of how meaningful any interpretation of it feels.

---

## 7. Content Addressing (The Museum)

Hashes are for **identity**, not navigation.

- Every verified work in the Museum is identified by **SHA-256** of its canonical form.
- Hashes prove identity and provenance but **cannot be reversed** into content, and cannot uniquely address the full possibility space (pigeonhole: 2^256 hashes vs. far more possible units).
- Each Museum entry stores: content hash, entropy-ordered address (under a pinned model), source, provenance chain, verification status.

---

## 8. Composition

- A **book** (or any larger work) is an ordered sequence of unit addresses plus a structure record (unit count, formatting, type).
- Composition does not reduce the number of possible books. Searching units reduces the work of *examining* content; choosing the correct ordering remains a separate search.

---

## 9. Implementation Plan

1. **Reference implementation** in a high-level language. Clarity over speed. This is the source of truth.
2. **Conformance test suite**: fixed inputs with expected addresses, layers and scores, checked bit for bit.
3. **Fast core** in C (SIMD where profiling justifies it; assembly only for proven hot spots) covering addressing, hashing and classification.
4. **Language model as a pluggable scorer**, run through existing GPU libraries, with quantised integer outputs passed to the core.
5. The fast core must pass the full conformance suite against the reference implementation before it is trusted.

---

## 10. Milestones

| # | Milestone | Deliverable |
| :--- | :--- | :--- |
| M1 | Exhaustive sieve | Parameterised enumerator + Layer 1–2 filters, run over **every** unit at lengths 4–8; plot of surviving fraction vs. length |
| M2 | Raw addressing | Positional and scrambled bijections with round-trip tests |
| M3 | Entropy-ordered addressing | Integer arithmetic coder with a small pinned model; measured bits/char on real text |
| M4 | Paragraph-scale sampling | Classification and sampling at `UNIT_LENGTH` ≈ 1,000; extrapolation checked against M1 results |
| M5 | Similarity map | Embeddings, nearest-neighbour search, hex gallery view |
| M6 | Museum | Content-addressed archive with provenance and verification workflow |

---

## 11. Known Limits

- Exhaustive enumeration is only possible at small unit lengths (roughly ≤ 8–12 symbols, depending on alphabet).
- Faster code gives constant-factor gains, not orders of magnitude.
- Meaning-guided navigation cannot distinguish real, lost and invented works without external evidence.
- Humans and models both perceive patterns in noise; the system must not.
