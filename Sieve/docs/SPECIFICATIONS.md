# Sieve — Gallery of Babel Specifications

*Version 2.0. Concrete design for the search-space engine and its navigable view. Supersedes the search-space sections of the Potentia README and Specifications v1.0. Sieve is the Gallery of Babel part of Potentia; the alignment thesis stays with Potentia.*

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
- Measured (M3): the pinned order-5 character model codes held-out English at **1.93–1.95 bits/char** (2.48 for Shakespeare). By that model, a 1,000-character paragraph is one of about 2^1,930 ≈ 10^581 plausible ones, against 10^1,431 raw. A better model moves this towards Shannon's figure.

---

## 3. Lines (State Spaces)

The Gallery is built on **four unit lines**, one per modality. Each line is a one-dimensional address space defined by a parameter set, so every line can be dialled from trivially small to full scale. The hallway has eleven lines in all: these four, the books, tracks and movies composed of them (§11), the models line (§12), the worlds composed of models (§12.0), the AI line (§12.0d) and the binary line (§12.1).

| Parameter | Description |
| :--- | :--- |
| `MODALITY` | `text`, `image`, `audio`, `video` |
| `ALPHABET` | Ordered, finite symbol set (characters, pixel values, note events) |
| `UNIT_LENGTH` | Symbols per unit (characters, pixels, events, frames × pixels) |
| `MODEL` | Pinned scorer for entropy ordering and classification (by hash) |
| `SPEC_VERSION` | Version of this spec and all pinned data |

Rules:

- An alphabet is an **ordered list of Unicode code point ranges**, and those ranges are its digit order: digit 0 is the first code point of the first range, and the digits run on through each range in turn. The order is **pinned** — it is what every address on a line means — so an alphabet's ranges may never be edited once published. A correction is a new alphabet, or a new block, under a new id, exactly as a changed filter is a new version.
- Alphabets built from named Unicode blocks order their code points **ascending**, so digit 0 is the lowest they hold. The alphabets pinned before that rule keep the order they were pinned with: `lower27` and `ascii95` are ascending anyway; `babel29` is not, because it follows libraryofbabel.info's order (space, a–z, comma, period).
- Blocks may be **stacked** (`greek+cyrillic`), as may raw ranges (`u+0370-u+03ff`). Stacked code points are unioned, sorted and deduplicated, so blocks that overlap never give a symbol twice and the order the parts are written in does not change the result. `sieve alphabets` lists the blocks; `sieve alphabets --spec SPEC` works one out.
- **Surrogates** (U+D800–U+DFFF), noncharacters and unassigned code points may be included. They are code points like any other for addressing, counting, filtering and drawing — a surrogate draws as the replacement glyph — but surrogates have **no UTF-8 encoding**, so a unit holding one has no text form: it cannot be warped in from text or written into a `.book` file. Sieving them out is a filter's job, not the alphabet's.
- `ascii96` is the printable ASCII with **U+000A** added, so a unit can hold a file whose line breaks are part of what it says. `canon-text-v2` keeps whitespace the alphabet holds itself and drops a carriage return where the alphabet holds a line feed, so either line ending canonicalises the same way; padding is digit 0, which is the space for every alphabet pinned before `ascii96` and the line feed for that one.
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
| Audio | `notes2`, C3–C6 × 8 durations (304 events) × 32 events, one voice | ~10^79 | Compact with ranking filters |
| Audio | `notes2`, C2–C7 × 8 durations (496 events) × 4 voices × 32 events | ~10^345 | Compact with ranking filters |
| Audio | `notes3`, every MIDI pitch × 8 levels × lengths of 1-16 ticks (16,400 events) × 16 events, one voice | ~10^67 | Open-ended notes (§3.4) |
| Audio | `pcm`, 8000 a second, 8-bit, one channel × 8000 samples (one second) | ~10^19,266 | Sound itself (§3.3): positional and scrambled; no model yet |
| Video | 5×5, 1-bit × 8 frames | ~10^60 | Guided only (deferred) |

Audio begins as a **symbolic** line (note sequences), not raw samples. Raw audio at even one second of 8 kHz 8-bit is 256^8,000 possibilities and carries almost no structure per sample; symbolic audio reuses all of the text machinery unchanged.

### 3.2 The `notes2` Family (as built)

`notes104` is fixed and stays exactly as it was: its addresses, notation and MIDI files do not change. `notes2` is a family of larger note sets beside it, one chosen per line, each named by its symbols' id, `notes2/<LOW>-<HIGH>/<DURATIONS>/V<VOICES>` (e.g. `notes2/C3-C6/seEqQhHw/V2`):

- **Pitches:** every semitone from `LOW` to `HIGH`, within C2–C7 (MIDI 36–96) and at least an octave apart; default C3–C6 (37 pitches). Names are written with sharps; a set has one spelling.
- **Durations:** any of `s` (sixteenth), `e` (eighth), `E` (dotted eighth), `q` (quarter), `Q` (dotted quarter), `h` (half), `H` (dotted half) and `w` (whole), listed in that order; default all eight.
- **Voices:** 1 to 4, each its own line of events with its own time.
- **Digits:** `pitch_index × DURATIONS + duration_index`, pitch 0 the rest and pitch 1 `LOW`; digit 0, a rest of the shortest duration, is the padding symbol. A set has `(PITCHES + 1) × DURATIONS` symbols (304 by default).
- **Units:** `VOICES × L` events, voice by voice: `L` is the line's length in events per voice. Addresses read the whole unit as one number, as every line does.
- **Filters** on a line of several voices judge each voice on its own (a unit passes when every voice does). The survivors are one voice's to the power of the voices, and a ranker for one voice ranks the whole: a unit's rank is its voices' ranks read as one number in base (one voice's count), voice 1 first.
- **Playback:** the voices sound together. Saved, a unit is a format-1 MIDI file, a tempo track and a track per voice on its own channel (120 bpm, 480 ticks a quarter).
- **MIDI read back** (`midi_to_notation`, for J in the hallway, §12.1): each track with notes, or with a length and none (a voice of rests), is a voice, as many as the set has; one note at a time in each (a note starting inside another is left out); times rounded to the set's shortest duration; each note and gap written in the set's durations, longest first. A file the hallway saved comes back as the same music, though a run of rests may come back as fewer events.
- **Books:** `sieve-book-v1` gives an audio section only its length, so books hold `notes104` melodies alone; making a book section of a `notes2` line is refused rather than misread.

### 3.4 Open-Ended Notes: the `notes3` Family (as built)

A third note family, beside `notes104` and `notes2` and before sound itself, with nothing capped but MIDI. Each set is named by its id, `notes3/<LOW>..<HIGH>/q<TPQ>/d<LONGEST>/v<LEVELS>/V<VOICES>/t<TEMPO>/i<INSTRUMENTS>` (e.g. `notes3/C-1..G9/q4/d16/v8/V1/t120/i0`):

- **Pitches:** every semitone from `LOW` to `HIGH` within MIDI 0-127 (C-1 to G9), at least an octave apart.
- **Lengths:** whole ticks, `TPQ` to a quarter note (1-960), from 1 to `LONGEST` ticks (1-65,535).
- **Loudness:** `LEVELS` levels (1-127); level k plays at velocity round(127 k / LEVELS).
- **Voices:** 1 to 15, each on its own MIDI channel (the drums' channel 10 left out).
- **Tempo and instruments:** quarter notes a minute (1-1000) and a General MIDI program a voice. They are how the set plays and saves, not which units it has: they do not change an address.
- **Digits:** `(length − 1) + LONGEST × class`, class 0 a rest and `1 + p × LEVELS + (k − 1)` pitch p at level k. A set has `LONGEST × (1 + PITCHES × LEVELS)` symbols; digit 0, a rest of one tick, is the padding symbol. A unit is `VOICES × L` events, voice by voice.
- **Notation:** `C#4:3!5` is C#4 for 3 ticks at level 5; `R:2` a rest of 2 ticks; voices between ` // `. The length may be one of notes2's codes instead (`E4q`, `Bb3e.`), and either part may be left out.
- **Files:** a unit saves as a format-1 MIDI file with `TPQ` ticks a quarter, a tempo track, and a track per voice with its instrument and each note at its level's velocity. A MIDI file reads back with times rounded to the set's ticks and velocities to the nearest level (a tie to the louder); its tempo and instruments are not read, being the line's. It comes back as the same music, though a run of rests may come back as fewer events (MIDI has no rests).
- **Filters:** a stack judges each voice on its own (`line_voices`). The general filters apply. The melody plugins for `notes104` and `notes2` (`symbols notes*`) do not, being written for their digits; `notes3` has its own family, `symbols notes3*`, whose plugins name its symbols with `NOTE(p, k, n)` (pitch p from LOW, level k, n ticks) and `REST(n)`, and know its `PITCHES`, `LOW`, `LEVELS`, `LONGEST` and `TPQ` (docs/FILTER-PLUGINS.md §14): `key-notes3-v1` and `melody-{leap,range,ambitus,rests,gapfill,metre,ending,lengths,loudness}-notes3-v1`.
- **Books:** refused, as `notes2` and `pcm` are.

### 3.3 Sound Itself: the `pcm` Set (as built)

Beside the note sets, the audio line can hold sound itself: the `pcm` set, chosen on the line like any note set (`--note-set pcm`; the hallway's AUDIO SET). `notes104` stays the default, and nothing about the note sets changes. A `pcm` set is named by its id, `pcm/<RATE>/<BITS>/C<CHANNELS>` (e.g. `pcm/8000/8/C1`), and nothing is capped but what a unit can hold:

- **Rate:** samples a second, from 1 to 2^31 − 1; default 8000.
- **Bits:** a sample, from 1 to 31 (a digit is a 32-bit number, and 2^32 is not one); default 8.
- **Channels:** from 1 to 65,535 (what a WAV file carries); default 1.
- **Digits:** each sample is one digit, its own bits read as an unsigned number (two's complement), so digit 0 is silence and the padding symbol. A set has 2^BITS symbols.
- **Units:** `CHANNELS × L` samples, channel by channel, as a `notes2` unit is voice by voice: `L` is the line's length in samples per channel (one second at the rate unless given). Addresses read the whole unit as one number, as every line does.
- **Files:** a unit saves as a WAV file: interleaved, in the smallest whole-byte container that holds the bits (8-bit unsigned, else signed little-endian), each sample shifted up to fill it; plain PCM for 8, 16 and 24 bits on one or two channels, `WAVE_FORMAT_EXTENSIBLE` otherwise, with the set's own bits as its valid bits. The WAV a unit saves as reads back to the same unit.
- **Playing:** the hallway plays the sound in hand at its own rate (the music player mixes its channels to one; the fallback synth plays up to eight as they are).
- **Filters:** a stack judges each channel on its own, as it does each voice of a note set, so its survivors are one channel's to the power of the channels (`line_voices`, `VoicesRanker`, which packs a channel, a place and the one channel's state into 64 bits sized to the line). Its own filters are `sound-peak-v1`, `sound-step-v1` and `silence-run-v1` (§8 table; the oracle's `sound-vectors` pins their counts and ranks); the filters for any line apply too (`not-a-file-v1`, `not-a-pattern-v1`, entropy), and the note filters do not. Filters between sound and the note sets are still to come (IDEAS.md §12).
- **Books:** `sieve-book-v1` holds `notes104` melodies only, so a `pcm` section is refused, as a `notes2` one is.

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
- A meaningful unit's address length ≈ its information content (measured: ~1.9 bits/char for English with the default model, vs 4.75 in raw mode).

**Warning:** entropy-ordered decoding is equivalent to sampling from the model. A random address decodes to fluent output that **carries no evidence of being real**.

#### 4.2.1 Model (`sieve-charmodel-v1`, smoothing `witten-bell-v1`)

A model gives, for every history, an integer frequency table over the `N` symbols that sums to `T = 2^16`, with every entry at least 1, so that every unit keeps a nonzero share of the line.

- **File.** A text file: a fixed header (`symbols`, `base`, `order`, `min_count`, `total`, `smoothing`, `padding`, `trained_symbols`, `corpus`, `contexts`, in that order), then one line per stored context: the context as two hex digits per symbol, oldest first (`-` for the empty context), a TAB, and `symbol:count` pairs in ascending symbol order. Contexts are sorted by length, then by symbols. A file has exactly one spelling (a reader re-writes it and refuses any difference), so its SHA-256 identifies the model.
- **Training.** Count how often each symbol follows each context of up to `ORDER` symbols in the training stream. Store the empty context, and every other context seen at least `MIN_COUNT` times whose parent (the context minus its oldest symbol) is stored.
- **Tables**, parents first. With `n_s` the stored counts, `n` their sum, `u` the number of symbols with `n_s > 0` and `parent_s` the parent's finished table:
  - empty context: `a_s = n_s + 1`, `D = n + N`
  - other contexts: `a_s = n_s·T + u·parent_s`, `D = (n + u)·T`
  - then `f_s = 1 + ⌊(T − N)·a_s / D⌋`, and the `T − Σf` units left over go one each to the symbols with the largest remainders `(T − N)·a_s mod D`, ties to the lowest symbol.
- **Padding.** If the header names a padding symbol `p` (SPACE for text), the context `p p` has the fixed table `f_p = T − (N − 1)`, 1 elsewhere. Canonical text never contains two SPACEs in a row, so they only occur as the padding of a last unit, which then costs almost nothing.
- **Lookup.** The table for a history is that of its longest suffix, at most `ORDER` symbols, that is stored. A unit's history starts empty.
- **Default model** `gutenberg-lower27-o5`: order 5, min count 8, 101,294 contexts, trained on 10,510,198 symbols from 15 public-domain Project Gutenberg books (the NLTK selection, `data/models/corpus/gutenberg-nltk.tsv`). Three further books are held out for measurement.

#### 4.2.2 Coder (`guided-ac-v1`)

With the table `F_i` for the history `u_0…u_{i−1}` and its cumulative `C_i`:

`low_0 = 0, w_0 = 1;  low_{i+1} = low_i·T + C_i(u_i)·w_i;  w_{i+1} = w_i·F_i(u_i)`

The unit's arc is `[low_L, low_L + w_L)` in `[0, 2^S)`, `S = 16·L`. The arcs of all `N^L` units tile the line exactly, in positional (dictionary) order, each as wide as its probability.

- **Point.** Any binary fraction in `[0, 1)`. Every point lies in exactly one arc, so every point decodes to exactly one unit.
- **Address.** The shortest aligned block `[m/2^b, (m+1)/2^b)` that fits inside the unit's arc (the lowest, if two fit), written as `m/2^b` in `⌈b/4⌉` hexadecimal fraction digits. Then `−log2 P(u) ≤ b < −log2 P(u) + 2`: the address length is the unit's information content to within two bits, and the addresses form a prefix code. Units and addresses are in one-to-one correspondence; any other hex string is still a valid point, which reads the unit whose arc contains it.
- **Example.** `"it was the best of times"` at `L = 32`: raw 152 bits, guided `90922c700685e28` (58 bits, 1.81 bits/char).

### 4.3 Determinism

Encoding and decoding must be **bit-for-bit identical** on every machine.

- All address arithmetic uses **exact integers** (arbitrary precision). Floating point is prohibited in the address path: a double holds 53 bits of precision, while a single paragraph address needs ~4,755.
- The arithmetic coder uses integer or fixed-point arithmetic with fully specified rounding.
- Model outputs are **quantised to integer frequency tables** before entering the coder.
- Models (weights, tokenizer, quantisation) are pinned by **cryptographic hash**. Changing a model changes every entropy-ordered address on that line, so models are versioned, never silently replaced.
- A model's training is itself reproducible: the corpus manifest pins every file by SHA-256, and training the same corpus with the same options gives the same file byte for byte. The reference oracle rebuilds the default model independently and must match its hash.

**The big integer itself.** `BigUint` holds a value as little-endian limbs of **2^64**, and every operation on the address path goes through it, so its arithmetic is pinned like every other rule: the reference oracle works the answers out with Python's own arbitrary-precision integers, sharing no code with the core, and the core must reproduce them exactly (`tests/vectors_biguint_v1.tsv`). The vectors run from a limb or two up past a page and include the boundaries where a carry or a borrow crosses a limb, which is where an implementation of this kind goes wrong if it is going to. A second set, `tests/vectors_biguint_large_v1.tsv`, runs to 400,000 bits, where multiplication, division and the base conversions change algorithm; its operands are too long to write down, so each row names its operands by a rule both sides follow (`large_operand()` in the oracle) and gives the SHA-256 of the exact result.

Which algorithm computes an answer is chosen by the operands' lengths — schoolbook or Karatsuba multiplication, Knuth's long division or division by a Newton reciprocal, peeling digits a limb at a time or splitting the number in halves by a power of the base — and is never visible in the answer. Every estimate on the fast paths is followed by an exact correction, so the choice changes how long an answer takes and nothing else.

A limb of 2^64 needs 128-bit products and a 128-bit numerator for division. Where the compiler offers a 128-bit integer that is used; where it does not, the same answers are built from 32-bit halves with 64-bit intermediates. Both are exact and both are tested — `SIEVE_BIGUINT_PORTABLE` forces the second so it is exercised even on compilers that have the first. Nothing about the limb width is visible in an address: it is how the number is held, not what the number is.

---

## 5. The Hallway

Each line is presented as a **single hallway**: an endless corridor with a shelf on each side. Position along the hallway *is* the address.

### 5.1 Layout

- **Shelf, door, repeat.** Shelving runs continuously; doors (§7) appear at fixed intervals.
- **Sides:** left and right shelves hold consecutive halves of the local range.
- **Tile size is a setting, and it is not part of an address.** How many units stand on one tile — 128 by default, 256 offered in the menu, any power of two from 2 to 4096 on the command line — decides only how a unit's index is cut into a corridor coordinate (`tile = index >> bits`, `slot = index & mask`). The index itself, and therefore every address, is untouched. A *tile number* is therefore only meaningful alongside the setting that produced it, and both the hallway's readout and `sieve info` state it.
- **One corridor for all lines.** The four lines, and the books line composed from them (§11), share a single endless corridor, numbered by one signed position in **tiles of 128 book slots**. 128 is a power of two, and the implementation enforces this at compile time. Each line **repeats** along the corridor: a line of `N` units spans `⌈N / 128⌉` tiles per copy, and copy `c` begins at tile `c·⌈N / 128⌉`. Within a copy, slot `i` holds the unit whose raw address is `i`, or, in the guided view at zoom `d`, the point `i / 2^d`, so a guided loop has `2^d` books.
- **Padding.** When `N` is not a multiple of 128, the last tile of each copy ends in empty shelf space, so every copy starts on a fresh tile. A line whose size is a power of two, at least 128, has no padding, and when every line's size is a power of two the loops **nest**: each line's start line falls on a start line of every smaller line. `sieve info` reports how each line fits.
- **Start line.** The beginning of each copy is marked by a checkered strip across the floor, in the line's two colours. Where every line begins a copy at once (always at tile 0, and wherever else their loops coincide) the start line is doubled (a second strip a metre further on).
- **Setup menu.** Before entering, the reader sets each line's shape, with no upper limit: the state spaces are meant to scale without end. A map draws the lines side by side, one copy each, as bars whose length is the line's size in bits, with the binary line at each end (§12.1) since that is where it runs. The longest line fills the height, so no bar leaves the screen, and there is a minimum bar length. Only the machine limits what can be opened. The menu warns when an address would be slow to work with, and refuses when one would not fit in memory. Those thresholds describe the hardware, not the design.
- **Filters.** A magnifying glass beside each line's title in the setup menu opens that line's filter list over the map: the display mode (§9), every filter the line offers with a tickbox and its description, and the parameters of each ticked filter. A footer gives the stack's state. Where the stack can count its survivors exactly, the map shows them as a filled bar inside the line's bar, labelled with the survivors' size in bits (log2 of their count).
- **Views:**
  - **Guided view** (default, entropy-ordered): shelf length ∝ probability. Meaningful units fill long runs; noise is too thin to occupy floor space. Only shelved units (§9) are shown.
  - **Raw view** (diagnostic): fixed-width units, every unit shown, noise everywhere. This is Borges' library, and the contrast with the guided view is the thesis made visible.

### 5.2 Zoom (Depth)

The hallway is navigable at any scale through **zoom depth** `d`:

- At depth `d`, each shelf segment represents all units sharing a `d`-symbol prefix, labelled with that prefix.
- Walking into a segment expands it into its next symbol. At `d = UNIT_LENGTH`, segments are individual units (books).
- In the guided view, segment length ∝ the prefix's total probability, so at every depth the routes towards meaning are wide and routes into noise are hairlines. Structurally this is a trie; physically it remains one hallway.

**As built** (guided view): depth is counted in bits of the guided line rather than in symbols. At depth `d`, consecutive books are points `2^−d` apart, and each shows the unit whose arc contains its point. A probable unit owns a long arc, so it fills many books when zoomed in. Zoomed out, each book is the likeliest unit in its `2^−d` slice, so the shelves read as the most probable continuations. Warping sets `d` to the length of the unit's address, which puts the unit exactly on a book.

### 5.3 Readout

The client always displays: line, view, zoom depth, current address (truncated, expandable) and fractional position. It also shows the corridor tile, and the loop's length in tiles with its padding. A tile and slot (`TILE:SLOT`) identify a place on the corridor for every line at once (`sieve read --at`).

### 5.3.1 Application Settings (as built)

Besides the state spaces (§5.1) and the filters (§8.5), the client keeps its own settings in `sieve-hallway.ini` next to the executable, edited in the main menu (Start Sieve / Settings / Exit Sieve) and applied at once:

- **Graphics:** resolution (chosen from the display on first start), fullscreen, VSync (on by default), **Geometry Edge Glow** and **Real Graphics** (each turns the other off; both off by default), **Door Portals** (off by default; turning Real Graphics on turns it on, and nothing turns it off), and an FPS counter. Door Portals fills each doorway with procedural data noise in the colour of the line it leads to — an integer hash of the noise cell (two screen pixels square) and the frame number, fading to black at the frame. Real Graphics draws the four models of `data/meshes` (`hallway`, `bookshelf`, `book`, `marker`, one set per line) with a depth buffer; any missing model leaves that part wireframe.
- **Controls:** mouse sensitivity and invert mouse Y. While a unit is in hand the player stands still.
- **Language:** every on-screen string comes from a language file in `data/lang` (`key = value`, UTF-8), whose first line names its bitmap font in `data/fonts` (Unifont `.hex` format). A key a language leaves out falls back to English.

None of this changes an address, a filter or a count: it is presentation only.

### 5.4 Appearance

The hallway is a wireframe. Each line has exactly two colours, a solid background and the colour of every edge, and doors are solid black:

| Line (in door order) | Background | Edges |
| :--- | :--- | :--- |
| Image | blue | cyan |
| Text (pages) | black | white |
| Books | grey | black |
| Audio | green | amber |
| Tracks | green | black |
| Video | red | yellow |
| Movies | red | black |
| Models | clay | green |
| Binary | black | green |

Black edges mark a line composed of another's units: books of pages, tracks of audio, movies of video. Each line's colours, names and place in the door order are defined once, in `client/dimensions.hpp`.

The text line's units are pages, so the hallway and the setup menu call it PAGES; `--line pages` is accepted as another name for `--line text`.

One tile of geometry (a bookcase and a door on each wall) is built once and repeated along the corridor. Edges fade towards the background with distance. Each tile holds 128 consecutive slots (4 shelves of 16 books on each wall): the left wall, then the right, shelf by shelf from the top, in walking order along each shelf. Padding slots have no book.

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
  - **Reading the file.** stb_image reads PNG, JPEG, BMP, GIF and TGA, and always reads them first, so the addresses of those files never change. Every other picture or video format (MP4, WebM, MKV, MOV, AVI, WebP, TIFF, ...) is read by an ffmpeg program run as its own process; Sieve links none of it and works without it. ffmpeg (5.1 or later) gives each frame the file stores once, in order (`-fps_mode passthrough`, as a GIF's frames are read), as 8-bit RGBA, converted with `bitexact`, `accurate_rnd` and `full_chroma_int`, and stops one frame past `FRAMES`. The frames are fitted one at a time, so a long video is never held whole. The report names the ffmpeg that read the file: lossless sources give the same frames from any ffmpeg (checked against the same frame as a PNG), while a lossy codec's decoding is the codec's own standard, and its colour conversion is pinned by those flags. Which ffmpeg: `--ffmpeg PATH` (the hallway's `ffmpeg` setting), else `SIEVE_FFMPEG`, else one beside Sieve's program, else the first on the `PATH`.
  - **Saving as other formats.** A unit saves as its own file (PNG, MIDI, WAV, text) without ffmpeg. With one, it can also be saved as JPEG, WebP, BMP or TIFF (image), animated GIF, MP4 or WebM (video, `--fps` frames a second, 8 unless given), and FLAC, MP3, Ogg Vorbis, Opus or AAC (sound), each offered only when the ffmpeg found has its encoder (`ffmpeg -encoders`). Sieve writes its own file, then ffmpeg converts it: pictures scaled up by nearest neighbour, a GIF's palette made from the frames without dithering. Lossless formats come back to the same address; lossy ones may not. `sieve read --out` chooses by the extension (an extension none of these has gives the line's own file, as before); the hallway's F lists them in the save dialog.
- **Audio (`canon-notes-v1`):** note notation (`C4q F#5e Bb4h Rq`) converts to the 104-symbol event alphabet (a rest or C4–C6, each with four durations). Flats are written as sharps, out-of-range notes move by whole octaves into C4–C6, and the last unit is padded with eighth rests.
- **Audio, `notes2` (`canon-notes-v2`):** as v1, with the durations `s e e. q q. h h. w` (a missing one is `q`), voices separated by `//` (fewer voices than the line's are filled with rests; more are an error), pitches moved by whole octaves into the set's range, and a duration the set lacks replaced by the nearest it has by length, a tie going to the longer. Each voice is cut into runs of `L`, every voice is padded to the same number of runs with digit 0, and unit `k` holds run `k` of every voice. Output uses sharps and ` // ` between voices. The oracle has its own implementation, and `tests/vectors_notes2_v1.tsv` pins the units, the report, the notation and each MIDI file's SHA-256.

- **Audio, `notes3` (`canon-notes-v3`):** a missing length is a quarter (`TPQ` ticks); a notes2 code is round(sixteenths × TPQ / 4) ticks, at least 1 (reported when not whole); a missing level is the one whose velocity is nearest 96 (what notes104 and notes2 play at), a tie to the louder, and a level above `LEVELS` is `LEVELS` (reported); flats become sharps; a pitch outside the range moves by whole octaves into it; a length beyond `LONGEST` is the event for `LONGEST` ticks, then rests of at most `LONGEST` for the remainder (reported); fewer voices than the set's are filled with rests, more are an error; each voice is cut into runs of L and padded with digit 0. A MIDI file given to warp is read back first. The oracle has its own implementation, and `tests/vectors_notes3_v1.tsv` pins the units, the report, the notation and each MIDI file's SHA-256.
- **Audio, `pcm` (`canon-pcm-v1`):** a sound file (WAV, read by Sieve itself: PCM of 8, 16, 24 or 32 bits, or 32- or 64-bit floating point, plain or extensible; any other format through ffmpeg, its first audio stream decoded as stored, `-flags +bitexact`, never resampled or mixed there). Every sample is first a signed 32-bit number: 8-bit WAV's unsigned bytes centred, shorter samples shifted up, floating point scaled by 2^31, rounded half up and clamped. Then, in this order: (1) channels: to one channel, the mean of all of them, rounded half up; to C of more, the first C; to more than there are, the last repeated; (2) rate: exact area-averaging, as pictures are stretched: target sample t spans [t/R, (t+1)/R) seconds and is the mean of the source samples over that span, each weighted by how much of it it covers, rounded half up (the same rule up and down; upward it holds each sample), with ceil(n·R/S) samples made and the last, if the sound ends inside it, the mean of what it covers; (3) depth: rounded half up to the set's bits and clamped (reported as clipped); (4) units: each channel cut into runs of L, every channel padded with silence to the same number of runs; unit k holds run k of every channel. The engine does it a block at a time in integer spans; the oracle has its own implementation in exact fractions, and `tests/vectors_pcm_v1.tsv` pins the units, the report and each WAV file's SHA-256 (every WAV encoding, mixing, splitting, rates up and down, 1 to 31 bits, clipping).

Palettes are pinned by the implementation: `mono` (2), `ega16` (16), `rgb332` (256) and `rgb24` (16,777,216). Index 0 is black in every palette.

The warp reports every transformation applied, so the reader knows exactly what was located.

### 6.3 Multi-Unit Input

Input longer than one unit becomes a **sequence of addresses**. The warp lands on the first and displays the rest as a trail.

### 6.4 Arrival

- **Where:** every unit appears once in each copy of its line along the corridor. A warp (or go-to) always goes to the **first copy**, where the corridor position equals the unit's address. A warp is therefore fully determined by what is warped to, including what lies through every door afterwards.
- **What:** the reader lands facing the unit, and it opens in hand.

- **Shelved unit:** the reader arrives at its shelf position.
- **Unshelved unit:** the unit is presented **in hand**, as a book the reader is holding, with its address and classification, but with no shelf position and nowhere to be returned to. Units never physically leave the shelves; the in-hand view is a separate presentation.

---

## 7. Doors

Doors connect the lines in order, and the binary line (§12.1) is the end of the run at both sides: **binary → Text → Image → Audio → Video → Books → Models → binary**. Left goes to the next line and right to the previous one; binary has one wall, so it has one door, and it leads back to the line you came from.

### 7.1 Mapping

A door keeps the reader's **angle**: the same share of the way round the line's loop. From tile `t` of line A's loop of `T_A` tiles, the reader arrives at tile `⌊t · T_B / T_A⌋` of line B's loop of `T_B` tiles, computed exactly, so it is the same place round the circle to the full precision of the shorter loop. The reader faces the same slot, and the starts coincide (tile 0 is tile 0 on every line).

- Every line keeps its own place. Walking and jumping move only the line the reader is on, and every other line keeps the place the reader last stood on it: 0 until visited.
- A shorter line cannot hold every place of a longer one. So **stepping straight back** through a door, without having moved, returns the reader to exactly the tile they left, not the nearest. Doors taken in a row unwind in turn: the hallway keeps a stack of the doors taken since the reader last moved.
- *Superseded (September 2026):* doors used to keep the corridor tile itself (`t mod ⌈M / 128⌉` on the target line). On a longer line that is nearly always about 0°, and on a shorter one it wraps to an unrelated angle, which is not "the same place on the other line".
- Doors sit in the walls: the left wall's doors lead to the next line in the cycle, the right wall's to the previous one. The reader comes in through the opposite wall's door.
- Doors do not preserve the fraction along a line. An earlier design mapped `b = ⌊a × M / N⌋`, but moving one tile on a small line then jumped astronomically on a large one, so walking between lines was not navigable.

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

### 8.5 The Filtration Stack (as built)

The structure tests of §8.1 are built as **filters**: small, self-contained modules, each deciding whether one unit passes. Today's filters are S0/S1 tests; S2 tests and the anchoring axis (§8.2) will join the same stack.

**Modules.** Filters are compiled in. Every filter is registered in one list (`core/src/filters/builtin.cpp`); adding one takes a new source file and one line there. A filter has an id and a version, and is named `id-vN` (`words-v2`). **A registered version never changes.** A changed filter is registered as the next version beside the old one, so every recorded result stays reproducible. A bare id (`words`) means its newest version. Each filter declares:

- the lines it applies to;
- its parameters (integers with a range and step, or registry ids such as a dictionary or model), each with a default;
- which other filters every one of its survivors also passes (`implies`, used by compact mode below);
- optionally a **ranker**, which counts its survivors exactly at any unit length and converts between a survivor and its position among the survivors in address order, without visiting the others. A ranker is a walk through a finite set of states, one symbol at a time: `next(state, symbol)` is the state after the symbol (or dead), and `completions(state, r)` counts the ways to finish with `r` more symbols so that the unit passes. Counting, ranking and unranking follow from these two for every ranker alike, and so does the sieved guided line (§9).

**Exactness.** Every decision is made in integer arithmetic, so all machines and the reference oracle agree bit for bit. Logarithms use `lg(x) = ⌊log2(x) · 2^16⌋` for 64-bit `x`, computed exactly by repeated squaring. The oracle reproduces each filter independently: verdicts, logarithms, survivor counts and ranks (`tests/vectors_filters_v1.tsv`).

**Filters (as built).** `L` is the unit length, `c` a symbol's count within the unit, `f` a symbol's frequency in the pinned model out of `2^16`.

| Filter | Lines | Passes when | Ranker |
| :--- | :--- | :--- | :--- |
| `clean-v1` | text (`lower27`) | no two SPACEs in a row, and at least one letter | yes |
| `window-v1` | text (`lower27`) | could be cut from running English: whole dictionary words inside, a word's suffix at the left edge, a word's prefix at the right edge (a lone token: any substring of a word). Implies `clean-v1`. | yes |
| `words-v1` | text (`lower27`) | clean, and every token is a dictionary word. Implies `clean-v1`, `window-v1`. | yes |
| `clean-v2`, `words-v2`, `window-v2` | text (`lower27`) | as v1, but a unit may end in two or more SPACEs of padding, in which case the part before them is judged by v1. This admits the last unit of warped text. `window-v2` implies `clean-v2`; `words-v2` implies `clean-v2` and `window-v2`. `window-v2` is the filter for pages cut from real books: the words cut by the page edges are allowed. | yes |
| `title-v1` (retired for `title-data-v1`, the same rule in the token form) | text (`lower27`) | a title page: within the first `N = min(max_length, L)` characters, the unit passes `words-v2` at length `N`, and every character after them is SPACE (default `max_length` 64). Implies `clean-v2`, `window-v2`, `words-v2`. | yes |
| `max-run-v1` | text | no symbol other than SPACE repeated more than `max_run` times in a row (default 3) | yes: an automaton (its state is the last symbol and its run), so it also merges with the custom filters and `not-written-v1`; it judges only when that table would pass 16 million entries |
| `symbol-entropy-v1` | every line | `min · L · 2^16 ≤ 1000 · HL ≤ max · L · 2^16`, where `HL = max(0, L·lg(L) − Σ c·lg(c))` (millibits per symbol; defaults 0 and 4400) | two-symbol lines (black-and-white images and video), up to the length whose rank fits the time budget (about 1,000 at 50 ms; measured on the machine) |
| `model-information-v1` | text, with a model | `1000 · Σ (16·2^16 − lg(f)) ≤ max · L · 2^16`, each symbol's `f` taken in its context as in §4.2 (default max 5000 millibits per symbol) | no |
| `neighbour-agreement-v1` | image, video | among all horizontally, vertically and (video) frame-to-frame adjacent pixel pairs, at least `min` per thousand share a colour (default 600) | while the table fits (below) |
| `key-v1` | audio | every note's pitch class is in the scale on the tonic (parameters `tonic` C…B and `scale`: major, minor, harmonic-minor, major-pentatonic, minor-pentatonic, blues); rests always pass | yes |
| `not-written-v1` | text | no reading of the unit is a file whose first bytes carry a signature (`file-kinds-v1`, §12.1): its own bytes, hex, base64, base32, decimal, the letters a–p, spelled-out digits, or any two symbols as bits (the vault's decoders less ascii85; parameter `readings`: all, or one). On `bytes256` a unit is read only as its own bytes. | yes; with plugins too (docs/FILTER-PLUGINS.md §15) |
| `binary-kind-v1` | binary | the file's kind (`file-kinds-v1`, from its first 16 bytes and its size) is one of those chosen (`kinds`: signed, text, signed-or-text, unknown, empty, any, or one kind), or with `keep = exclude` is not | yes, at any length |
| `not-a-file-v1` | text, image, audio, video, models | the unit's own number, read as a place on the binary line (binary-v1), does not hold a file whose first bytes carry a signature; counted from the file's head alone (FILTER-PLUGINS §16) | yes |
| `not-other-line-v1` | text (not bytes256) | the page is not melody notation (`notes`) or a model's `.obj` text (`obj`); `forms`: all, notes or obj; an automaton | yes |
| `not-packed-v1` | image, video (2, 4, 16 or 256 colours) | the pixels, packed as bits into bytes, are not a file with a signature; an automaton | yes |
| `not-a-pattern-v1` | text, image, audio, video | not a repeat of a block of up to `period` values (default 16) all the way along, nor a ramp of values counting up by a fixed step (`ramps`); values of `width` digits (1, 2, 4, 8), `order` big or little; FILTER-PLUGINS §18 | yes |
| `distinct-vertices-v1` | models | no two vertices stand at the same point of the grid; `P(P−1)…(P−V+1)` of the coordinate strings, `P = C³` | yes |
| `distinct-indices-v1` | models | no face names the same vertex twice | yes |
| `every-vertex-used-v1` | models | every vertex is named by at least one face; inclusion and exclusion over the vertices left out | yes, within a budget |
| `not-an-item-v1` | binary | the file is not exactly another line's item as F saves it (`items`: pages, melodies, pictures, models, all); pages count exactly, the rest are judged file by file | pages only |
| `palette-size-v1` | image, video | at most `colours` distinct colours (default 16); on video, `scope` film or each frame (FILTER-PLUGINS §19) | yes, on every palette: completions depend only on the colours used so far |
| `row-runs-v1` | image, video | along each row of pixels, at most `changes` places where a pixel differs from its left neighbour (default 3) | yes, at any width and palette |
| `sound-peak-v1` | audio (`pcm`) | every sample s has −A ≤ s ≤ A, A = ⌊2^(BITS−1) · `percent` / 100⌋ (default 90) | yes, at every depth: an automaton of one state to 24 bits, past that a mixed-radix count of its own |
| `sound-step-v1` | audio (`pcm`) | neighbouring samples differ by at most ⌊2^BITS · `percent` / 100⌋ (default 50) | an automaton (its state the sample before) to 11 bits; judged past that |
| `silence-run-v1` | audio (`pcm`) | no run of more than `samples` silent samples, digit 0 (default 4000) | an automaton (its state the run so far) while it fits; judged past that |
| `canonical-mesh-v1` | models | the vertices increase as grid points, each face starts at its smallest index (rotation only, so winding is kept), the faces increase as triples: one encoding of each mesh | yes: C(C³, V) · C(V(V−1)(V−2)/3, F); with every-vertex-used, while its table fits in the filter memory (16 vertices in 512 MB) |
| `utf8-valid-v1` | binary | well-formed UTF-8 throughout (RFC 3629); `controls = text`: no control character but tab, LF, CR | yes, on its own, while its table fits |

The rankers for `clean` and `words` count with exact tables over word-length histograms of the dictionary's trie. The `window` rankers add the two edge tokens: the left edge is counted from histograms of the dictionary's suffixes (each distinct suffix once, so a token that ends several words is counted once), and the right edge from the trie, where any live prefix may end the unit. `title-v1` walks `words-v2` over the first `N` positions and then only SPACE. They agree with M1's counts at every length checked, and exhaustively with the filters themselves at small lengths. They are built for units up to 20,000 symbols. On two symbols, symbol entropy depends only on the number `k` of 1s, so its survivors are the units whose `k` lies in an allowed set `K`, and `completions(j ones, r) = Σ_{k∈K} C(r, k − j)`. `key-v1` judges each position alone, so with `a` allowed symbols `completions = a^r`.

`neighbour-agreement-v1` is counted by a transfer matrix. Cells are placed in address order, and each is compared with the neighbours already placed: left, up, and the same pixel one frame back. A unit passes when it has at most `D = pairs − ⌈min · pairs / 1000⌉` disagreeing pairs. The walk state is (next cell, disagreements so far, the last `P` cells), where `P` is the width for pictures and the whole frame for video. `T[i][p][b]`, the number of ways to fill the cells from `i` on with at most `b` more disagreements, is built backwards from the last cell. The table has `n · B^P · (D + 1)` entries (budgets beyond what the remaining cells could spend are clamped), so its size is exponential in the width. That is inherent to counting pictures exactly by their neighbours, not a shortcoming of the implementation. As built, it is offered up to `2^24` profiles and `2^28` limbs (1 GiB). A 10×10 black-and-white picture takes about 0.1 s and 60 MB; 3×3 video fits; 5×5 video does not.

These length and size limits are hardware limits, not design ones.

Measured separations (lower27, pinned model, held-out books): English costs at most 4.14 bits per symbol under the model and random letters at least 9.1; at length 1,000 English has symbol entropy about 4.15 bits and random letters about 4.72; English never runs a letter more than 3 times. `model-information` and `symbol-entropy` count padding SPACEs like any other symbol, so a heavily padded unit reads as lower-information than its text alone.

**Hard and soft.** Every built-in filter is labelled `hard` (sets aside only noise or another line's content, with no collateral: the exclusion filters, max-run, not-a-pattern, the models line's distinct rules and canonical-mesh, binary-kind) or `soft` (may set aside what a person would keep: the word, model, key and picture rules, every-vertex-used, utf8-valid). The label is not part of the rule and changes no version (`FilterSpec::category`; FILTER-PLUGINS §17, §19).

**Dictionaries.** The word filters take a registered dictionary (`data/dictionaries/dictionaries.tsv`, each file pinned by its SHA-256). The lists are SCOWL 2020.12.07 at sizes 35, 60 and 80, each with and without proper names; the `-names` lists keep the words' case as SCOWL spells them (`England`, `Dickens`), and the lower-case alphabets read capitals as lower case when a list is loaded, so one cased list serves every alphabet. The default is `scowl-en-60-names`, because real books name people and places. A dictionary is part of a stack's provenance: changing it changes the stack id, the survivor counts and every compact address.

**Stack.** The filters ticked for a line form its stack; a unit passes the stack when it passes every filter. The stack's **provenance** lists every filter's name, parameters and the SHA-256 of the data it uses (`words-v2{dictionary=default sha256=…}; max-run-v1{max_run=3}`, preceded by the line's shape), and the **stack id** is the SHA-256 of that string. Both are recorded with any result. A stack can **rank** when one ticked filter has a ranker and implies every other ticked filter, with the same value for every parameter they share (`words` with one dictionary does not imply `window` with another), so its survivors are exactly the stack's.

**Settings.** A plain-text file, `sieve-filters.ini`, next to the executable (or wherever `--filters` points) holds each line's mode, its ticked filters and their parameters:

```
[text]
mode = compact
filters = words-v2
[text.words-v2]
dictionary = scowl-en-35
```

The hallway's setup menu edits and saves it (§5.1); hand edits are welcome. A missing file means nothing is ticked. Every line has a section, `[models]` and `[binary]` included (one `[binary]`: it is one line, met at both ends of the corridor), and every column of the setup menu's map has a magnifying glass, both binary columns opening the same one. The models line has no filters registered yet, so its section keeps only a mode; the binary line has `binary-kind-v1`, and its files are judged, counted and compacted like any line's units (§12.1). `sieve filters` lists a line's filters and its stack, with the share of the line it keeps and sets aside as powers of ten (`--line binary` adds every kind's share); `sieve check` runs content through every filter (`--line binary --file F` judges a file).

**One line filtered by another.** `not-written-v1` sets aside a unit that belongs on the binary line: a file written out as text. It is exclusion, which removes only the share of the line that another line's content takes up (on `lower27` pages of 32 symbols, 10^-4.23), not requirement, which multiplies shares; see docs/FILTER-PLUGINS.md §15 for the difference, the counting, and the measured shares. Every line sets aside the others' content in the same way (§16 there): `not-a-file-v1` (a unit whose number is a place on the binary line holding a signed file), `not-other-line-v1` (pages that are melody notation or `.obj` text), `not-packed-v1` (pictures whose packed bits are a signed file) and, on the binary line, `not-an-item-v1` (a file that is exactly another line's item).

---

## 9. Shelving

The address space is complete; the shelves are curated.

- A unit is **shelved** if it is **S2**, or if it is **A2/A3** at any structure level.
- All other units (S0, S1 unanchored) are **unshelved**. They remain fully addressable and can be reached by warp or address, where they are presented in hand (§6.4).
- In the guided view, unshelved units already occupy negligible length; omitting them removes the remaining gaps. In the raw view, every unit is shown regardless.
- Shelving thresholds are part of the pinned parameter set.

**As built** (through the filtration stack of §8.5). Each line has a display mode; mark and hide work in every ordering:

| Mode | Shelves |
| :--- | :--- |
| off | every unit, no judging |
| mark | every unit; units that fail the stack are drawn faint, as a visible record of what the stack rejects |
| hide | units that fail are left out; the rest keep their places, so the gaps show what was sieved |
| compact | only survivors, packed together in address order: slot `k` of a loop holds the `k`-th survivor, and a loop is as long as the survivor count |
| full | compact with the titles and covers kept (§11): on the pages, image, audio and video lines, every survivor with every title and cover, a titled line over the survivors (its content the survivor's number in positional order). On the books, tracks and movies lines, which keep their titles in compact, it is compact; on the models and binary lines it is compact for now. *(Added 7 October 2026: compact names a unit by its content alone, full by everything it carries; which a use wants depends on whether its files need titles or file names.)* |
| excluded | hide turned round: units that pass are left out and units that fail keep their places, each with the filter that rejected it, so what a stack sets aside can be walked and checked. Where the stack can rank, the count of excluded units (the line's size less the survivors) is exact, in the top bar and in `sieve filters`. *(Added 28 September 2026. Excluded units packed together, as compact packs survivors, would need their own compact addressing and is not built.)* |

Compact mode needs a stack that can rank; otherwise the hallway falls back to hide and says why. It works in every ordering, each with its own compact form over the `N` survivors (numbered `0 … N−1` in positional order):

| Ordering | Compact address of survivor `k` |
| :--- | :--- |
| positional | `k`, its survivor number |
| scrambled | `shuffle(k)`, a keyed permutation of `[0, N)` (`shuffle-sha256-v1`, below) |
| guided | its address on the guided line restricted to survivors (`sieve-restrict-v1`, below) |

Compact positional and scrambled addresses are hex numbers below `N`, zero-padded to the width `N − 1` needs. Content reached by warp or address that fails the stack is shown in hand, marked as not on the shelves, together with the filter that rejected it. The command line takes `--compact` on `warp`, `read` and `browse`, and `read --survivor K` reads the `K`-th survivor.

**`shuffle-sha256-v1`.** A permutation of `[0, N)`, keyed by the line's key and the stack id. Let `b = max(2, bitlength(N − 1))`, and split a value `x < 2^b` into its high `⌈b/2⌉` bits `H` and low `⌊b/2⌋` bits `Lo`. Eight Feistel rounds `r = 0 … 7`: even rounds `Lo ^= F(r, H)`, odd rounds `H ^= F(r, Lo)`. `F(r, v)` is the leading bits, read big-endian, of `SHA-256(m ‖ 0) ‖ SHA-256(m ‖ 1) ‖ …`, where the block counters are u32 little-endian. `m` is `"SIEVE/SHUFFLE/1"` followed by, each prefixed with its u32 little-endian length, the key, the stack id and `N` in lowercase hex; then `r` as u32 little-endian and `v` in lowercase hex, length-prefixed. A result of `N` or more is permuted again (cycle walking) until it falls below `N`. Because `2^b ≤ 2N`, that takes at most two passes on average. `N = 1` is the identity.

**`sieve-restrict-v1`.** The guided line (§4.2.2) restricted to survivors. At each step the ranker's walk decides which next symbols can still lead to a survivor. If all can, the model's table is used unchanged. Otherwise the table keeps only those live symbols, with `a_s` their model frequencies, `D = Σ a_s` and `N'` their number, and requantises them with the model's own rule (§4.2.1): `f_s = 1 + ⌊(T − N') a_s / D⌋`, with the leftover units going one each to the largest remainders `(T − N') a_s mod D`, ties to the lowest symbol. Dead symbols get 0. The arcs then tile `[0, 1)` with survivors only, and no point decodes to anything else. Each survivor's arc is as wide as the model finds it among the survivors sharing its beginning, and its address still obeys `−log2 P' ≤ b < −log2 P' + 2` under the restricted probabilities `P'`.

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
- **Books (as built).** A book is an ordered list of labelled sections (`title`, `cover`, `pages`, ...); each is a run of units on one line. The record `sieve-book-v1` is plain text: `section LABEL`, `line KIND`, that line's shape fields in a fixed order (text: `alphabet`, `length`, `canon`; image: `width`, `height`, `palette`; video: `width`, `height`, `frames`, `palette`; audio: `length`), `key`, `mode` (positional, scrambled or guided), `model ID SHA256` for guided sections, `units N`, then one address per unit. The record closes with `end` and `id`. The **id** (`sieve-book-id-v1`) is the SHA-256 of the content alone, written as a canonical text: `sieve-book-id-v1`, then for each section `section LABEL`, `shape KIND/SYMBOLS/L<length>`, `units N` and each unit's positional address, one per line. The key, the ordering and the model only choose how addresses are written, so they do not change the id. A reader recomputes every unit and must reproduce the id; a guided address must be the unit's own address, not merely a point inside its arc. The title is a page like any other; the cover is a picture. Bibliographic facts (author, date) belong in the Registry (§10), not in the book. **`sieve-book-v2`** is the same record with one field more, written only where it is needed (a record that does not need it is written, and read, as v1): an audio section on any note set but `notes104` names its set after its line, `notes <symbols id>` (a `notes2`, `notes3` or `pcm` id, which says the whole set), and its `length` is then per voice or channel. The id is computed as for v1. **Tracks and movies keep records of the same kind:** a `cover` section of the image line, a `title` section of the titled lines' title, and a `units` section of the audio or video line with at most the line's `N` units; a missing cover, title or unit is blank, and a blank cover or title is left out, so one item has one record and one id however it was made. The sections say which line a record belongs on: `pages` books, `units` of audio tracks, `units` of video movies.
- **The books line (as built, `bookspace-v1`).** The books line holds every book of one shape. A book is a cover (one unit of the image line), a title (one unit of the pages line) and `P` pages (`P` more units of the pages line), so there are `|cover| · |page|^(P+1)` books. Positional order reads the parts as one mixed-radix number, most significant first: the cover's digits in base `|cover symbols|`, then the title's and every page's digits in base `|page symbols|`. Neighbouring books differ in the last page. Scrambled order passes that index through `shuffle-sha256-v1` over the whole book count, keyed with the key, with the books line's id as the domain (`books/<cover symbols>/L<n>+<page symbols>/L<n>x<P+1>/key=<key>/bookspace-v1`), so neighbouring books are unrelated. There is no guided ordering yet. A book record whose cover and pages have the lines' shape opens on its shelf; a missing cover or title is blank, and missing pages are blank pages.
- **Book filters (as built).** The books line has one display mode (§9) and a stack for each part: the **cover** stack sees the cover as a picture of the image line, the **title** stack sees the title as one page, and the **pages** stack sees all `P` pages read as **one continuous text** of `P · L` symbols. A fixed page length cuts words in two at every page break; reading the pages as one text judges such a word whole, so real books pass. A book passes when each part passes its stack; a part with no filters passes everything. The settings file holds them as `[books]` (`mode`), `[books.cover]`, `[books.title]` and `[books.pages]` (`filters`), with parameters in `[books.<part>.<filter>]`.
- **Compact books (`books-compact-v1`).** When every part with filters can rank, the surviving books are counted and ranked exactly: with `Nc`, `Nt`, `Nb` the survivors of the cover, title and body (a part with no filters keeps all `base^length` of its units), there are `N = Nc · Nt · Nb` surviving books, and survivor `k` is the combination with ranks `(kc, kt, kb)`, `k = (kc · Nt + kt) · Nb + kb`. Since each part ranks in its own address order and the positional book address puts the cover first, then the title, then the pages, survivors keep the books line's positional order. Compact addresses follow §9: positional is `k`; scrambled is `shuffle-sha256-v1` of `k` over `[0, N)`, keyed with the key, with domain `books-compact-v1/<books line id>/<cover>/<title>/<pages>`, each part being its stack id or `-` for a part with no filters. `sieve check --book FILE` judges a record by the books filters and prints its survivor number and compact addresses; `sieve filters --line books` lists the parts' filters and counts the survivors.
- **Tracks and movies (as built, `composition-v1`).** Two lines hold compositions of another line's units: a **track** is a cover (one unit of the image line), a title (one unit of the titled lines' title space, below, or none when the title length is 0) and `N` units of the audio line; a **movie** is the same with `N` units of the video line. `N` is a setting of each (units per track, units per movie; 4 by default). There are `|cover| · |title| · |unit|^N` of them, so each holds as many as its base line's units `N` at a time, with a cover and title of its own as well as its units keeping theirs on their own line. Positional order reads the parts as one mixed-radix number, most significant first: the cover's digits in base `|cover symbols|`, the title's in base `|title symbols|`, then every unit's digits in turn in base `|unit symbols|`; neighbours differ in the last unit. Scrambled order passes that index through `shuffle-sha256-v1` over the whole count, keyed with the unit line's key, with the composition's id as the domain: `<tracks|movies>/<cover symbols>/L<n>+<title symbols>/L<T>+<unit symbols>/L<n>x<N>/key=<key>/composition-v1`, the title written `-` when there is none. Addresses are that index in hex, zero-padded to the width `N - 1` needs. Books predate this and keep `bookspace-v1` (their title is a whole page), so no book's address changes. An item is shown, played and saved as one unit of its line `N` units long: its units joined **strand by strand** (a unit of audio is its voices, or its channels, one after another, so the joined unit is voice 1 of every unit in turn, then voice 2, and so on; a video's frames simply follow on). A unit warped in is split the same way, with a blank cover and title. **Filters (`composition-compact-v1`):** one display mode and four stacks: the **cover** as a picture, the **title** as a title, the **units**, each judged on its own as a unit of its line, and the **joined** stack, which judges the units joined (as above) as one unit of their line `N` units long, so that what runs across the seams between units is judged, as a book's pages are read as one text. With `Nc`, `Nt` and `Nu` the survivors of each, there are `Nc · Nt · Nu^N` surviving compositions, counted and ranked exactly when every part with filters can rank; survivor `k` has ranks `(kc, kt, ku1 ... kuN)` as one mixed-radix number, cover first, so survivors keep the positional order. With the joined stack and no per-unit filters, the units are one part, the joined unit, counted by its own stack: `Nc · Nt · Nj` survivors, survivor `k` with ranks `(kc, kt, kj)`, so survivors follow the joined unit's order, which is the positional order when a unit is one strand (video, one voice) and strand by strand when it is more. With both the per-unit and the joined stack, compositions are judged but not counted (compact falls back to hide). Compact addresses follow §9, as compact books do, with domain `composition-compact-v1/<id>/<cover>/<title>/<units>`, each part its stack id or `-`, and `/<joined>` after it when the joined stack has filters (so no address without one changes). The settings file holds them as `[tracks]` and `[movies]` (`mode`), with `[<line>.cover]`, `[<line>.title]`, `[<line>.units]` and `[<line>.joined]`.
- **Titled lines (as built, `titled-v1`).** Every line in the hallway but the books, tracks and movies lines (which have titles of their own) carries a title, as a book does (the binary line's is §12.1), and the audio and video lines a cover as well. A titled unit is a **cover** (audio and video only: one unit of the image line), a **title** (one unit of the title space: the pages line's alphabet at the **title length** `T`, a setting of its own, 32 by default) and its **content** (one unit of the line as it was: a page, a picture, notes, a film or a model). With `Nc` covers (1 on a line without them), `Nt = |title symbols|^T` titles and `Nx` contents there are `Nc · Nt · Nx` titled units. The content's index is its positional address on its own line: its digits read as one number for a unit line, and the models line's positional index (§12) for a model. Positional order reads the three as one mixed-radix number, cover first, as the books line does: `(cover · Nt + title) · Nx + content`, with the cover and the title each read as one number of their own digits. Neighbouring units differ in their content. Scrambled order passes that index through `shuffle-sha256-v1` over the whole count, keyed with the line's key, with the titled line's id as the domain: `titled/<content>[+title=<title symbols>/L<T>][+cover=<cover symbols>/L<n>]/key=<key>/titled-v1`, where `<content>` is the content's shape, `<symbols>/L<n>` for a unit line (`lower27/L3200`, `image/mono/10x10/L100`) and `models/V<v>/F<f>/C<c>` for the models line. Addresses are that index in hex, zero-padded to the width `N - 1` needs. Filters over a titled line are applied bottom-up, content first, then title, then cover (not yet built; IDEAS §5.13). A title length of 0 means no title: `Nt = 1`, the title part of the id is left out, and on a line without a cover the positional address is the content's own. A unit warped in on its own has a blank title (every digit 0) and a blank cover. Guided ordering and compact mode (§9) order and count the content alone. **Full** mode (§9) keeps the title and cover: its space is this titled space with the survivors as its content, `Nc · Nt · S` units for `S` survivors, the content's index being the survivor's number in positional order, and its id `titled/<content>/survivors=<stack id>[+title=...][+cover=...]/key=<key>/titled-v1`, so its scrambled order is a shuffle over the whole of it, domain-separated by the stack. A unit warped in on its own has a blank title and cover there too. Titles and covers are all kept until filters can judge those parts, as the books line's already do.
- Composition does not reduce the number of possible books. Searching units reduces the work of *examining* content; choosing the correct ordering remains a separate search.

---

## 12. The Models Line

A fifth unit line: every possible mesh of one shape. A model is **V vertices and F triangles**, with each coordinate one of **C** steps across [-1, 1] and each face naming three vertices, so the line holds

    N = C^(3V) * V^(3F)

models. `modelspace-v1` numbers them as one mixed-radix number, most significant first: every coordinate (base C), then every face index (base V); `scrambled` is that index through `shuffle-sha256-v1`, keyed and domain-separated by the space id, exactly as the books line is. The default shape (V = 8, F = 12, C = 16) is 2^204 models, and a cube is one of them.

**The grid is cell-centred.** C is a power of two and coordinate *d* is exactly `(2d + 1 - C) / C`, so the grid is symmetric about the origin, runs from `-1 + 1/C` to `1 - 1/C`, and every coordinate is a terminating decimal with `log2(C)` places. The origin itself lies between two cells, as a pixel grid of even width has no centre pixel. A cube fitted to the line comes back symmetric.

**Canonical `.obj`.** Each model has one exact text form: `v` lines of three signed fixed-width decimals, then `f` lines of three zero-padded 1-based indices, separated by single spaces, line feeds only, nothing optional. It has **no two spaces in a row anywhere** — signs are written as `+` or `-`, and indices are padded with zeros rather than spaces — because `canon-text-v2` collapses runs of spaces. That is deliberate: the form is designed to survive the text line.

**One object, two lines.** Because the form is exact and fixed-width, a model of `obj_length()` characters is also one unit of an **`ascii96`** text line of that length (§3). Warping the `.obj` onto that line and reading it back gives the same file byte for byte, and warping that file back onto the models line gives the same model address. A single object therefore has an address on the models line and an address on the pages line, and they agree — which is what joins the symbolic lines to the spatial ones. The default shape's `.obj` is 304 characters, so the cube fits on a single page.

**Fitting a mesh.** Any `.obj` lands somewhere on the line: it is centred, scaled so its longest side fills the grid, and each coordinate binned to its cell. Vertices past V and faces past F are dropped and counted, a mesh with too few faces is filled out, and indices naming a vertex the line does not have are clamped. `sieve mesh` does all of this and reports every change, as `warp` does for the other lines.

**Filters (as built).** The models line has a stack of its own, `[models]` (`core/include/sieve/modelsieve.hpp`), which judges a model by its positional index, since a model is a mixed-radix number rather than symbols of one base. It holds `not-a-file-v1` (§8.5) and the line's own filters, each counted exactly, and the line compacts in both orderings, titles blank. Built: `distinct-vertices-v1` (no two vertices at one point), `distinct-indices-v1` (no degenerate faces) and `every-vertex-used-v1`, in closed form (FILTER-PLUGINS §16); with `not-a-file-v1` as well the stack judges only. The line's filters fall in three tiers, the first two now built: local per-face constraints such as distinct indices, which rank exactly; small-V constraints such as "every vertex is used", which rank exactly as a state machine whose state is the set of vertices used so far (256 states at V = 8); and whole-mesh properties — watertight, manifold, non-self-intersecting, convex — which judge but do not rank, and so run in `mark` or `hide` mode like any other unrankable filter.

### 12.0 Worlds (`worldspace-v1`, as built)

A line of compositions of models: a **world** is a cover (one unit of the image line), a title (one unit of the titled lines' title space, §11, or none when the title length is 0) and `N` **slots**, each a model of the models line **placed** in the world: its **cell**, one of a grid of `G` cells along each axis, and its **turn**, one of the 24 rotations that take a cube onto itself. `N` (models per world, 4 by default) and `G` (cells along each axis, 8 by default, at most 1024) are settings; the models are of the models line's shape. So a slot is one of `|models| · G^3 · 24` and there are

    |cover| · |title| · (|models| · G^3 · 24)^N

worlds. **Positional** order reads the parts as one mixed-radix number, most significant first: the cover's digits in base `|cover symbols|`, the title's in base `|title symbols|`, then each slot in turn as its model's positional index on the models line (§12, base `|models|`), its cell's `x`, `y` and `z` (base `G` each) and its turn (base 24); neighbours differ in the last slot's turn. **Scrambled** order passes that index through `shuffle-sha256-v1` over the whole count, keyed with the models line's key, with the world space's id as the domain: `worlds/<cover symbols>/L<n>+<title symbols>/L<T>+V<v>/F<f>/C<c>x<N>/G<G>/key=<key>/worldspace-v1`, the title written `-` when there is none. Addresses are that index in hex, zero-padded to the width `N - 1` needs.

**The turns (`turns-24-v1`).** Every 3×3 matrix with one entry of +1 or -1 in each row and column and determinant +1: row `i` takes the old coordinate `perm[i]` times `sign[i]`. They are ordered by the permutation (`perm` read as a word, lexicographically: 012, 021, 102, 120, 201, 210), then by the signs (rows 0, 1, 2 as the bits 4, 2, 1 of a number counting up from 0, a set bit meaning minus), keeping those of determinant +1, four for each permutation. Turn 0 is the identity.

**Placing.** A model's coordinates lie in (-1, 1), each a multiple of `1/C` (§12); turned, they still do. Cell `(x, y, z)` has its centre at `(2x + 1 - G, 2y + 1 - G, 2z + 1 - G)`, so a world is `G` model-widths along each axis and a model fills its cell. Every coordinate of a world is a multiple of `1/C`, so its **`.obj`** text is exact: every slot's vertices, turned and placed, in slot order, each line `v` and three coordinates written with their sign, their whole part zero-padded to the digits of `G` and `log2(C)` places; then every slot's faces, their 1-based indices moved past the slots before it and zero-padded to the digits of `N · V`; single spaces, line feeds only. The cover and title are not part of the `.obj`. Textures, and UVs to put them on (IDEAS §14), are not part of this version.

**Filters (`world-compact-v1`).** One display mode and three stacks: the **cover** as a picture, the **title** as a title, and each slot's **model** by the models line's own stack (§12, `ModelSieve`); a slot's place has none. With `Nc`, `Nt` and `Nm` their survivors (a part with no filters keeping all its units), there are `Nc · Nt · (Nm · G^3 · 24)^N` surviving worlds, counted and ranked exactly when every part with filters can rank; survivor `k` has ranks `(kc, kt, then each slot's model rank and place)` as one mixed-radix number, cover first, so survivors keep the positional order. Compact addresses follow §9, with domain `world-compact-v1/<id>/<cover>/<title>/<models>`, each part its stack id or `-`. The settings file holds them as `[worlds]` (`mode`), with `[worlds.cover]`, `[worlds.title]` and `[worlds.models]`. The oracle's `world-vectors` are the conformance vectors, and `world-obj` writes a world's `.obj` from its slots.

### 12.0a Model files (`safetensors-layout-v1`, as built)

Not a line yet: the first step towards an AI dimension (IDEAS §15), a model file read as its parts. A **safetensors** file is its **start**, an 8-byte little-endian length `N` and `N` bytes of JSON, an object whose members are the tensors, each `{"dtype", "shape", "data_offsets": [begin, end)}` (offsets from the end of the start), and an optional `"__metadata__"` object of strings; and its **weights**, the tensors' bytes end to end, little-endian, row-major. A reader refuses a start whose tensors' offsets do not hold their shapes, or overlap, or leave gaps.

**`safetensors-layout-v1`** is the start as the safetensors library (0.4) writes it, from the metadata and each tensor's name, type and shape alone: the tensors ordered by type, latest first in the library's list (`BOOL U8 I8 F8_E5M2 F8_E4M3 I16 U16 F16 BF16 I32 U32 F32 F64 I64 U64`), then by name in byte order; each tensor's offsets following the last; the JSON written compactly, `"__metadata__"` first, then each tensor as `{"dtype":..,"shape":[..],"data_offsets":[b,e]}`, strings escaped as serde_json escapes them; padded with spaces to a multiple of 8. A file whose start is this one needs no start of its own. (Checked on files of one type; the order between types is pinned by the list above.)

**From a configuration.** For a Llama model (`config.json`, `model_type` `llama`, no biases), the tensors are, as the transformers library names them: `model.embed_tokens.weight` `[vocab, hidden]`; for each layer `l`, under `model.layers.l.`, `input_layernorm.weight` and `post_attention_layernorm.weight` `[hidden]`, `self_attn.q_proj.weight` `[heads · head_dim, hidden]`, `self_attn.k_proj.weight` and `self_attn.v_proj.weight` `[kv_heads · head_dim, hidden]`, `self_attn.o_proj.weight` `[hidden, heads · head_dim]`, `mlp.gate_proj.weight` and `mlp.up_proj.weight` `[intermediate, hidden]`, `mlp.down_proj.weight` `[hidden, intermediate]`; `model.norm.weight` `[hidden]`; and `lm_head.weight` `[vocab, hidden]` unless `tie_word_embeddings`. Their type is `torch_dtype`; `head_dim` is `hidden / heads` unless given; `kv_heads` is `heads` unless given. With the layout above and the metadata `{"format":"pt"}` (as PyTorch saves), that is the whole start, so for such a model the start is its configuration.

**Statistics** (16-bit float types, BF16 and F16): each tensor's values taken as 16-bit symbols: their count, how many are distinct, the order-0 entropy of the symbols, of their high bytes and of their low bytes, in bits a value (`-Σ p log2 p` over the symbols in increasing order), and the mean and standard deviation of the numbers (summed in file order in double precision; `sd = sqrt(max(0, Σx²/n - mean²))`). "Coded by frequency" is the sum over tensors of entropy times count, in whole bytes, without the tables of counts. `sieve tensors` (core `sieve/safetensors.hpp`) and the oracle's `tensors` compute these apart and print the same table (`--tsv`, entropies to 6 places, mean and deviation as `%.9e`); the oracle's `tensors-fixture` makes a small stand-in model of a configuration's shape, its values seeded, for tests.

### 12.0b Model files coded under a prior (`sieve-weights-v1`, as built)

A safetensors file (§12.0a) coded losslessly under a prior over its weights: the first version of the noise gradient as a prior (IDEAS §16.8b). The weights of one **kind** (the tensors whose names differ only in their layer number, `model.layers.<n>.` written `model.layers.*.`, and of one type) share one table of how often each 16-bit value occurs, and each tensor's values are coded under its kind's table, so that likely values cost few bits. The file comes back byte for byte, checked by its SHA-256.

**The stream**, integers little-endian, a *varint* an unsigned LEB128 number: `sieve-weights-v1` and a line feed; a varint `S` and the file's start (`S` bytes, as it was); a varint `G` and `G` **tables**, one for each kind of BF16 or F16 tensor with values, in the order its first tensor comes in the file, each a varint `k` and `k` pairs (varint gap, varint frequency), the values increasing, the first gap the value itself, the frequencies summing to 2^24; then **each tensor** in the order of its bytes in the file: a coded tensor as a varint `W` and `W` 32-bit words (its stream), any other as its bytes; and 32 bytes, the SHA-256 of the whole file. A reader refuses a stream whose tables do not sum to 2^24, whose words do not end where its values do, with bytes after the hash, or whose rebuilt file has another hash.

**A table** from a kind's counts `c_i` of its `k` distinct values over `N` values: `f_i = 1 + floor(c_i · (2^24 - k) / N)`, and what is left of 2^24 added to the most frequent value (the lowest, on a tie).

**The coder** (rANS: a 64-bit state, 32-bit words, 24-bit frequencies). With `L = 2^31`, a value's frequency `f` and `b` the sum of the frequencies of the values below it, the tensor's values are coded from the last to the first, from `x = L`: if `x ≥ ((L >> 24) << 32) · f`, put out the low 32 bits of `x` and shift `x` right by 32; then `x = ((x div f) << 24) + (x mod f) + b`. The stream is `x`'s low word, its high word, then the words put out, the last first. Reading: `x` is the first two words; for each value, the value whose range `[b, b + f)` holds `x mod 2^24`, then `x = f · (x >> 24) + (x mod 2^24) - b`, and while `x < L`, `x = (x << 32) | the next word`. At the end `x = L` and every word has been read.

`sieve tensors FILE --pack OUT` writes it (and reads it back to check), `--unpack FILE --out OUT` rebuilds the model file; the oracle's `weights-pack` and `weights-unpack` do the same apart, and their streams are the tool's, byte for byte. On SmolLM2-360M-Instruct: 476,729,540 bytes for 723,674,912 (65.9%; xz -6: 512,876,944), of which the start 32,675, the 11 tables 117,878 and the coded tensors 476,578,938.

### 12.0c A language model run by Sieve (`sieve chat`, as built)

Not a line yet: the engine the AI dimension's viewer will run (IDEAS §15). A model is the folder it is published as: `config.json`, `model.safetensors` (§12.0a) and `tokenizer.json`. No other runtime is used.

**The tokenizer** (core `sieve/llm_tokenizer.hpp`) reads byte-level BPE from `tokenizer.json`, as GPT-2 made it: no normalizer; a pre-tokenizer of `ByteLevel` (no prefix space, its pattern on) alone or after `Digits` (individual digits); a `BPE` model without byte fallback. Anything else is refused. To encode: the added tokens written in the text (when asked for) are found first, the earliest and then the longest; the rest is split, every number character (Unicode N) alone under `Digits`, then by the pattern `'s|'t|'re|'ve|'m|'ll|'d| ?\p{L}+| ?\p{N}+| ?[^\s\p{L}\p{N}]+|\s+(?!\S)|\s+` tried in that order (letters and numbers the Unicode general categories L and N, `\s` the White_Space property; core `sieve/unicode_classes.hpp`, generated for Unicode 14.0 by `tools/gen_unicode_classes.py`); each piece's UTF-8 bytes are written as GPT-2's byte characters and merged, the pair of lowest merge rank first (the leftmost on a tie), until no listed pair is left; each symbol is a token. A byte whose character is not a token is left out (as the library the file comes from does, there being no unknown token). Decoding writes each token's characters back as bytes; an added token is its text.

**The model** (core `sieve/llm.hpp`): a Llama model (`model_type` `llama`, no biases, no rope scaling, silu), its tensors named as §12.0a gives them. For each token at position `p`: `x` is its row of the embedding; each layer takes `h = rmsnorm(x) · input_layernorm`, `q, k, v = Wq h, Wk h, Wv h`, turns each head of `q` and `k` by the rotary embedding (element `i` of the first half with element `i` of the second, by the angle `p / theta^(2i / head_dim)`), keeps `k` and `v`, lets each query head attend over its group's keys at `0..p` (`softmax(q·k / sqrt(head_dim))` over the values), adds `Wo` of the joined heads to `x`, then adds `Wdown (silu(Wgate h) · Wup h)` with `h = rmsnorm(x) · post_attention_layernorm`; the logits are `Wout (rmsnorm(x) · norm)`, `Wout` the embedding when tied. `rmsnorm(x) = x / sqrt(mean(x²) + eps)`. Weights stay as stored (BF16 widened as it is used); the arithmetic is 32-bit, each product's sum in one fixed order (eight lanes, then the lanes), so the logits do not depend on the number of threads. It is floating point: the last bits may differ between processors and compilers, which is why it is a viewer's engine and not yet an address's.

**Choosing** (`Sampler`): a repeat penalty on the reply's tokens if asked; at temperature 0 the likeliest token; otherwise the tokens by falling logit, the top `k` if set, their probabilities at that temperature, the fewest whose probabilities reach `top_p`, and one of those drawn by a seeded generator (splitmix64), the same for a seed on every machine given the same logits. SmolLM2's makers suggest a temperature of 0.2 and a `top_p` of 0.9, the defaults.

**The chat** is ChatML, as SmolLM2's template writes it: `<|im_start|>system\n{system}<|im_end|>\n` once, then for each turn `<|im_start|>user\n{message}<|im_end|>\n<|im_start|>assistant\n`, the reply up to `<|im_end|>`, and `\n`. The system line defaults to the template's.

**Checked:** on SmolLM2-360M-Instruct, the tokens are those of Hugging Face's `tokenizers` on 5,736 texts (prose, code, numbers, spaces of every kind, contractions, many scripts, emoji, control characters, random strings), and so are the oracle's (`llm-encode-lines`); the logits after a 42-token chat prompt are within 5.4·10⁻⁵ of the official ONNX export's (KL divergence 2·10⁻¹¹). In CI, on a stand-in model (`tensors-fixture`) with the oracle's stand-in tokenizer (`llm-tokenizer-fixture`, `tests/llama_tiny_tokenizer.json`), the tokens are the oracle's and the logits within 10⁻³ of the oracle's 64-bit ones (`llm-logits`).

### 12.0d The AI line (`aispace-v1`, as built)

A line of language models: every model of one **shape**, numbered by its weights (IDEAS §15). The shape is a Llama model (§12.0c) of `L` layers, width `H` and `A` attention heads (the head size `H / A` even; as many key and value heads), a feed-forward of `4H`, and a vocabulary of the 256 bytes (token `b` is byte `b`), its output tied to its embedding and its norms all 1. Its weights are the embedding `[256, H]` and, in each layer, `q`, `k`, `v` and `o` `[H, H]`, `gate` and `up` `[4H, H]` and `down` `[H, 4H]`: `W = 256H + 16·L·H²` of them, each a **digit** of `B` bits (1 to 8). `L` (1 to 64), `H` (2 to 4096), `A` and `B` are settings; the defaults, **Micro**, are 1 layer, width 16, 2 heads and 4 bits: 8,192 weights, 2^32768 models.

**Values.** Digit `d` of a tensor of `c` columns stands for `(2d - m) / m · scale`, `m = 2^B - 1`: `2^B` values evenly spaced over `[-scale, scale]`, `scale` 1 for the embedding and `sqrt(3 / c)` for the rest (so that a layer keeps its activations' size), computed in 64-bit floating point and rounded once to a 32-bit float.

**Addresses.** The line holds `2^(B·W)` models. **Positional** order reads the digits as one number in base `2^B`, the first most significant: the tensors by name in byte order (the norms, which have no digits, left out), each row-major. So an address is the weights' digits written out, and positional neighbours differ in the last weight of the last tensor by name (at one layer, `model.layers.0.self_attn.v_proj.weight`). **Scrambled** order passes that index through `shuffle-sha256-v1` over the whole line, keyed with the line's key, with the space's id as the domain: `ai/L<L>/H<H>/A<A>/F<4H>/B<B>/V256/key=<key>/aispace-v1`. Addresses are hex, zero-padded to the width `2^(B·W) - 1` needs (8,192 digits at the defaults). The line has no filters yet: every model is on the shelves.

**A model's files**: `config.json` (`LlamaForCausalLM`, the shape above, 512 positions, `rms_norm_eps` 1e-5, `rope_theta` 10000, silu, tied, no biases, `torch_dtype` `float32`); `model.safetensors`, every tensor of the shape (the norms' ones included) as F32, laid out by `safetensors-layout-v1` (§12.0a) with the metadata `{"format":"pt"}`; and `tokenizer.json`, byte-level BPE whose vocabulary is the 256 byte characters, token `b` for byte `b`, with no merges and no added tokens. `sieve chat --raw` talks to them. **Warping** a file back: it must be a model of this shape (F32, BF16 or F16), its norms all 1, and every weight exactly one of its tensor's `2^B` values; its digits are then its address, and anything else has none on this line.

**Speaking.** A model continues a text byte by byte (`continue_text`, §12.0c's engine with no chat format): the text's bytes are the tokens, and each byte it writes is drawn by the `Sampler` (temperature 0.8 unless given). It is floating point, as §12.0c's is: what a model says may differ in its last bits between machines; its address does not.

**Checked.** The oracle's `ai-vectors` (`tests/vectors_ai_v1.tsv`: 30 rows over three shapes, both orders, the bearings 0, 13, 137, 246 and 359; the SHA-256 of each address, of its digits and of its `model.safetensors`) are written apart from the C++ and reproduced by the tool and the unit tests; `ai-read` writes a model's files, which are the tool's byte for byte, and `llm-logits` runs them, within 1.5·10⁻⁶ of the tool.

### 12.1a The bytes256 alphabet

`bytes256` is the 256 code points U+0000-U+00FF in code point order, so **digit d is byte d**. A file of N bytes is exactly one unit of a `bytes256` line of length N.

**`canon-bytes-v1`** is the identity, and is used for any alphabet holding every byte value. Nothing is transliterated, folded, dropped or collapsed, because on such a line every byte already means itself and a file has to come back byte for byte. A last unit short of the length is padded with NUL, which is digit 0, and an empty file is one unit of NULs, so every file has an address.

Content for such a line is read and written as **raw bytes**: warping does not decode UTF-8 (most files are not valid UTF-8), and `--out` writes bytes without encoding or a trailing newline.

It is the only alphabet that packs exactly: 256 symbols is 8 bits each, so a unit's address in positional order is its own bytes read as one number, and its hex form is the file's hex dump. That makes it the clearest demonstration of what §6 means by an address being a bijection: the address of a 100 KB file is 200,000 hex digits, because it *is* the file. Addresses of that length do not fit on a command line, so `read` accepts `--address-file PATH`.

### 12.1 The binary line

The last line, and the one the others are bounded by:

    binary | image  pages  books  audio  tracks  video  movies  models | binary

It is listed twice because it is met from either end, but it is **one line**, not two. It wraps around the outside of the others — a single closed loop, in binary, around everything they address. Its room is always the same way round, shelves on the line's left and the edge on its right; met from the models end, you are turned round to face the other way along it, so the two ends run in opposite directions, as the two ends of one line do.

**Shape.** It is the **same space** as any other tile of corridor — the same width, the same height, the same bookcase — and an ordinary line in every respect the engine cares about: its own two colours (black, with green edges and green text), its own place in the door order, its own column on the map. The one difference is that it has **one side**. One wall carries the bookcases; in place of the other the floor ends, at a short wall no higher than your waist, and past that there is nothing. Green rain falls off that edge for ever, filling the opening from the ceiling down and passing behind the wall — characters from every Unicode block, and now and then a code point written out as the surrogate pair it is stored as.

**Doors.** Its one wall carries its one door, so the other lines **no longer loop into one another**: they start and finish at binary. Walking left out of `image` runs `pages`, `books`, `audio`, `tracks`, `video`, `movies`, `models`, and then binary. Walking right out of `image` reaches binary directly, from the other end. Either way the door you came in by is the door you leave by, because there is only one wall to put a door in: it leads back to the line you came from, and its sign names that line (image, the first, when you start on binary).

**Units (`binary-v1`).** The line holds **every file of 0 to N bytes**, the empty file included, where N is the line's length setting. There are

    M = 1 + 256 + 256^2 + ... + 256^N = (256^(N+1) - 1) / 255

of them. In positional order they are numbered **shortest first**, and within one length by their bytes read as one big-endian number (the first byte most significant):

    index(file of L bytes) = (256^L - 1) / 255 + bytes as a number

The first term is how many files are shorter. In hex it is `01` written L times, so a file's address is its own hex dump plus `0101...01`, and reading one back is a subtraction and a hex conversion: linear in the file's length. Nothing is padded, unlike a fixed-length `bytes256` line (§12.1a): a file and the same file with a NUL after it are two files. A file longer than N is refused. **Scrambled** is the positional index through `shuffle-sha256-v1` over [0, M), keyed with the line's key and domain-separated by the space id `binary/bytes256/L0-<N>/key=<key>/binary-v1`. Like the other lines, a binary unit carries a **title** (§11), but no cover: its content shape is `binary/L0-<N>`, so its titled id is `titled/binary/L0-<N>[+title=...]/key=<key>/titled-v1` and its index `title · M + file`. What a file is, is read from its own first bytes instead: the hallway shows a **kind label** on its front (ZIP, PNG, MID, EXE, PDF, TXT, EMPTY, and so on; `?` for any it does not know), taken from the file's signature by **`file-kinds-v1`** (`core/include/sieve/filekind.hpp`): EMPTY for no bytes; else the first signature in its table whose fixed bytes all lie within the file and match; else TXT when every one of the first 16 bytes (or all, if fewer) is readable (20–7E, 80–FF, tab, line feed, carriage return); else `?`. The label is right whatever its title says. It is never part of the address. The binary line's filter, `binary-kind-v1` (§8.5), reads the same table, and because a kind depends only on the first 16 bytes and the size, its survivors are counted and ranked exactly: those of `L <= 16` bytes by walking the heads, those of `L > 16` as the heads of 16 bytes times `256^(L-16)`, numbered as the line numbers files. In **compact** mode only the surviving files stand on the shelves, closed up, positional by survivor number or scrambled by `shuffle-sha256-v1` over the survivors (keyed with the line's key, the sieve's id as its domain); their titles are blank there, as a compact line's are. Mark, hide and excluded judge each file from its head. The first bytes come from the top of the number (the top of the index less `01` written as many times, with one borrow from below), so a file is not worked out whole to be labelled. A file warped in from a path (the File Locator, or **T** with a path) takes its **file name** as its title, canonicalised as a page's text is and cut to the title length; typed bytes have a blank title. A picture of one's own choosing for a file is a separate layer of metadata to come (anchors, IDEAS §4.8), not part of the address. The oracle's `binary-vectors` are the conformance vectors.

**Size.** With N the size of a file, the line is as large as the whole of that file's space and every smaller one: N = 2 MB is an address of about 4.2 million hex digits. The size is a setting like every other line's, and FIND MY LIMITS can be told to spend the budget on it.

### 12.2 Locating files, and manifests (`sieve-manifest-v1`)

**Between an item and its file (J).** With an item in hand, **J** on any other line goes to the item's file on the binary line: the file F saves (a page as its text, a picture as a PNG at one pixel a pixel, a melody as MIDI, a model as its `.obj`, a book as its title and pages as text), with the item's title. The binary line is made longer if the file needs it, only the room you stand in keeping its pictures past 64 KB, and its door leads back to the line the item came from. On the binary line, **J** opens a file of a kind a line holds on that line, fitted to it as T fits what is warped in: TXT on the pages line (its units a trail), PNG, JPG, GIF and BMP on the image line (an animated GIF on the video line), MP4, AVI, WebP and files of no known kind through ffmpeg when there is one (§6.2: a video on the video line, a single picture on the image line), MID on the audio line when it holds notes (read back by `midi_to_notation`, §3.2), WAV, MP3, OGG and FLAC on it when it holds sound itself (§3.3, `canon-pcm-v1`; through ffmpeg but WAV), and a book record on the books line. The report says what fitting changed. A file of another kind stays where it is. The vault is asked at each step.

**Locating** a file is naming its place on the binary line: its `binary-v1` positional address (§12.1), which is the file's hex dump plus `0101...01`, one `01` for each of its bytes. It depends only on the file, not on the line's length setting, so it is the file's name in the space; its SHA-256 is its short identity.

**Weighing** (as built, `tools/cli/weigh.*`: `sieve locate --weigh`, the File Locator) is a measurement, not a format: no address changes. A file's **own address** is its place on the binary line, written in hex (4 bits a digit). Its other ways: its number among the binary line's survivors, where the `[binary]` filters rank and it passes them; and, where it is exactly an item of the pages, image, audio or video line (`item_of`: the file is byte for byte the file F saves for some unit, the test not-an-item-v1 makes), its positional place on that line and its number among that line's survivors. Each number is weighed as its shortest route among the places it is one of (§3.6's variable length addressing, bearings to 20 places; past 2^16 bits, its hex digits). A way on another line needs its **shape** (the line's symbols and length, `lower27/L32`, priced at 8 bits a character) and, for a survivor number, its **stack** (one bit for each filter the line offers that can count, and each setting at log2 of the values it allows), each paid once for every file that uses it; the ways allowed are then narrowed while that lowers the total (a line or stack that costs more than it saves is dropped). With tailoring, each line's filters are tailored to the files that are its items (the search of §9's COST tab, its description paid for) and the files weighed again under what it finds.

A **manifest** is a folder located: walked to the bottom and listed in one canonical text, so that the same tree always gives the same bytes, and the manifest — itself a file, with an identity and a place on the binary line — names the whole tree:

    sieve-manifest-v1
    root <the folder's own name>
    files <how many files>
    bytes <their sizes added up>
    d<TAB><path>/                        a folder
    f<TAB><size><TAB><sha256><TAB><path> a file
    end

Paths are relative to the root, UTF-8, with `/` between their parts; a folder's ends in `/`. Entries are sorted by their paths' bytes, compared as unsigned. Fields are separated by one tab, lines end in a line feed, the path comes last. A path containing a tab or a line break cannot be listed and is refused. Symbolic links are neither followed nor listed. The files' addresses are not in the manifest: each address is its file, and would make the manifest as large as the tree. `sieve locate` writes them beside it on request. The oracle's `manifest` command writes the same manifest from this definition, and checks the addresses.

**Address listings (`sieve-manifest-v2`).** v1 with every file's address in it, in hex, between its SHA-256 and its path: `f<TAB><size><TAB><sha256><TAB><address><TAB><path>`, the empty file's address written `0` (`--with-addresses`). Two hex digits a byte, so it is more than twice the tree's size. Installers were first made from v2, which made each one twice as large as it need be; they are made from v3 now, and a v2 installer still installs.

**Installers (`sieve-manifest-v3`).** An installer's manifest is the v1 text with `sieve-manifest-v3` as its first line, and then, straight after `end` and its line feed, the **raw bytes of every file**, one after another in the listing's order, with nothing between them and nothing after: the sizes in the listing say where each file ends, and the bytes must add up to the `bytes` line exactly. It carries the whole tree at its own size plus the listing, which is the least an address of the tree can be without compressing first. Its own address — one number — is the installer: `sieve install` reads that number back into the manifest, cuts every file from the bytes after the listing (for a v2 manifest, reads every address back into its file), checks each against its size and SHA-256 before writing anything, and refuses paths that are absolute, name a drive, use a backslash, or have a part that is `.` or `..`. The oracle writes the same v2 and v3 manifests from this definition (`--with-addresses`, `--with-contents`). **A single file** has Sieve instructions exactly as a folder holding just it would: a v3 manifest whose root is the file's name and whose one entry is the file, then its bytes (`sieve locate FILE --installer`, `--program`). It installs as that file, under its name, checked against its SHA-256. The listing costs a little over a hundred bytes; a file's bare address (`--out`, in hex) saves those but carries neither its name nor any check, since every number is some file. In the hallway, an installer (the `.sieve`) is called **Sieve instructions**, and the manifest is not offered on its own: it is part of the instructions, for the tools.

**Packed installers (`sieve-manifest-v4`, the installers made now).** The v3 listing with `sieve-manifest-v4` as its first line, two lines after `bytes` naming its streams, and each file's **way** between its SHA-256 and its path:

```
stream x86 <packed bytes> <bytes> <dictionary>
stream lzma2 <packed bytes> <bytes> <dictionary>
f	<size>	<sha256>	<way>	<path>
```

After `end` and its line feed come the two streams, the x86 one first, each raw LZMA2 (no container) whose dictionary size gives its property byte as the LZMA2 format defines it (the smallest `p` with `dictionary <= (2 | (p & 1)) << (p / 2 + 11)`); the x86 stream's bytes, unpacked, are then passed back through the x86 branch filter (7-Zip's BCJ, xz's x86 filter) from position 0. A file's way is `x86` (its bytes, next in the x86 stream), `raw` (its bytes, next in the lzma2 stream) or `lines K` (derived from the file on the `K`-th `f` line, counting from 0, which is carried `x86` or `raw`: its lines, split at each line feed, so `n` line feeds make `n + 1` lines, are some of file K's in its order, and a mask of them, one bit for each of file K's lines, the first in the low bit of the first byte, `ceil(lines / 8)` bytes, is in the lzma2 stream). The lzma2 stream holds, in the listing's order, every `raw` file's bytes, and then every derived file's mask. Every file is checked against its size and SHA-256 as in v3; a stream that does not unpack to its size, or that holds more than the files, is refused. **Packing** (`tools/cli/pack.*`): programs (EXE, ELF by `file-kinds-v1`) go `x86`; a text is derived from a larger file carried as it is (never one itself derived from) when its lines are some of that file's in order and its mask packs smaller than it does; the rest go `raw`; both streams are LZMA2 preset 9 extreme with the smallest dictionary that holds them (4 KiB to 64 MiB). What is packed is unpacked and compared before it is used, and where the v4 manifest would be no smaller than the v3 the files are carried as they are, in v3. The oracle unpacks v4 from this definition with Python's lzma (`sieve_ref.py unpack`). `--v3` makes v3 regardless.

**Installer files.** An installer is its manifest's address stored as **raw bytes**: the number in base 256, most significant byte first, with no header, conventionally named `<name>.sieve`. It is exactly as long as the manifest, or one byte longer when adding `0101...01` carries past the first byte, and it reads as the manifest with every byte one higher (`sieve-manifest-v3` begins `tjfwf.nbojgftu.w4`, `sieve-manifest-v4` begins `tjfwf.nbojgftu.w5`). `--hex` writes and reads the same number in hex instead, at twice the size. `sieve-install` is the installer program: it opens a `.sieve` file, asks where, checks every file, writes them, and on Cancel removes what it wrote.

**Installer programs.** An installer program is a copy of `sieve-install` with an installer attached to its end: the program's own bytes, then the installer's raw bytes, then their count as 8 bytes, least significant first, then the 16 ASCII bytes `sieve-attached-1`. Attaching to a program that already ends in such a trailer replaces what was attached. `sieve-install` run without a file named looks at its own end first; `sieve install` accepts a program as its installer.

**Comparison.** `sieve locate --compare` sets beside the original size what zip's compressor (deflate, zlib level 9, each file on its own) and 7z's (LZMA2, preset 9 extreme, a folder as one solid stream) make of it, without their archives' headers, and the address as a number and in hex, each as a share of the original. The address is never smaller than the file: addressing is not compression. A compressor's figure is an upper bound on how far the data can be reduced, never the limit itself, which (its Kolmogorov complexity) no program can compute.

### 12.3 Maps: node graphs of verified anchors (`sieve-map-v1`)

A **map** links real things in the library. Its nodes are files and folders; a file is named by its size and SHA-256, so a map says exactly which bytes it means, and those bytes are one unit of the binary line (§12.1). Its edges are relations between two nodes. A map holds no file's bytes: a node is a claim that a file with this size and hash exists, and where it was found relative to the map's root. Where the file is on this computer and its SHA-256 matches, the node is a **verified anchor**. Maps are metadata about the space, never part of any address.

The text is canonical, so the same folder always gives the same map:

```
sieve-map-v1
name <the map's name>
root <the root folder's name>
nodes <N>
edges <E>
n<TAB><id><TAB>folder<TAB><path>/
n<TAB><id><TAB>file<TAB><size><TAB><sha256><TAB><path>
e<TAB><from><TAB><to><TAB><relation>
end
```

Ids count up from 0 in order. The root is node 0, with path `./`. Paths are as in a manifest: UTF-8, `/` between parts, relative to the root, and never absolute, with a drive, a backslash, or a `.` or `..` part. A **map of a folder** lists the root and then the folder's v1 manifest entries in the manifest's order, and links every node but the root to the folder its path is in, with the relation `contains`. Edges are sorted by from, then to, then relation. A relation is any word of lower-case letters and hyphens, so later relations (a newer version, a patch, anything recorded by a tool or a person) are written the same way within v1. `sieve map FOLDER` writes one; the oracle writes the same map from this definition (`sieve_ref.py map`). `--dot` and `--graphml` write it for other programs (Graphviz; Gephi, yEd, Cytoscape).

**Version 2 (`sieve-map-v2`): held anchors, a seal, metadata.** v2 adds three things to v1's text:
- a line `sealed yes|no` after `root`;
- a line `meta <M>` after `edges`;
- **held** nodes, `n<TAB><id><TAB>held<TAB><size><TAB><sha256><TAB><name>`, whose bytes follow the text straight after `end` and its line feed, every held node's in node order, filling the rest of the file exactly. Each is checked against its SHA-256 when the map is read. A held name is one file name, with no `/`, `\`, `:`, tab or line break.

**Metadata** lines, `m<TAB><node><TAB><key><TAB><value>`, come after the edges, sorted by node then key. A key is a lower-case word. None are written yet: this is room for an item's extra facts later.

A map is written as v1 whenever it uses none of these (no held node, no metadata, not sealed), so a map of a folder is always v1. Adding a held anchor links it from the root with the relation `anchor`, and refuses bytes the map already names. Removing a node (never the root) drops its edges and metadata, and every later node moves down one. A **sealed** map is not changed by the tools. The release map is sealed, and its published SHA-256 is what shows a copy has been altered, since a map is a file anyone can edit.

`sieve map` makes and changes maps:
- `FOLDER`, as v1;
- `FILE...`, the root and those files, found beside the map;
- `MAP --add FILE [--as NAME]`, `--remove ID`, `--seal`.

The oracle writes the folder map, the files map (sealed or not) and the held map (`--held`) from this definition.

**The maps shipped.** None. The build makes an empty `maps` folder beside the programs, for the node graph, and leaves any map already in it alone. *(Until 27 September 2026 a sealed `maps/sieve.map` was shipped, only its root, meant to name the source archive at a release; the published map now holds the source, so it was dropped.)* An archive cannot name itself: adding the map changes the archive, and so its hash. So the map naming both archives is published beside the downloads, not inside them. "This installation" is made fresh, is read-only, and is the map chosen when the hallway starts. Maps to add anchors to are made with the node graph's **New map...** (an empty map under the name chosen, in `maps`) or `sieve map --new NAME`. *(A `default.map` shipped empty was tried and dropped the same day, 27 September 2026: New map... makes one when it is wanted.)*

**In the hallway.** The node graph (O, or the pause menu's third row) shows one map at a time, chosen from a list:
- "This installation" is made when first chosen, from the programs beside the hallway and the folders the build puts there. The executable being run is the first anchor.
- Every `.map` in the `maps` folder beside the program is listed.
- Any map opened or made during the session is added. A map made from a folder is saved into `maps`.

The map is laid out in 3D by force direction (Fruchterman–Reingold: every node repels every other, every edge pulls its ends together, the steps shrink to nothing over 300 steps). It starts from each node's distance from the root, in a direction taken from its id, so the same map always settles the same way. Above 900 nodes, each node is repelled by a sample of 128 others, scaled up. A file node whose file is here can be walked to on the binary line, after its SHA-256 is checked.

**V** on any tab of the item page adds the file in hand to the chosen map as a held anchor, or, if the map already names its bytes, removes it; the page says which V will do. Only files: the binary line is where anchors belong, and an item of another line belongs to that line's setup, which a node does not record. Its name is the file's own name if you walked to it from a file, else its title, with the kind its first bytes say it is. [ and ] choose the map on any tab, and the page shows which map is chosen. The viewer's **Remove from map** (Delete) removes the chosen node. Sealed maps and "This installation" refuse both. When the item is an anchor of the chosen map and its node has metadata, the item page has a fourth tab, **META**, listing it (key and value; `sieve map MAP --meta NODE:key=value` sets it, an empty value removes it). The item page's **SORT** tab shows the item in hand against the chosen map. A file on the binary line whose SHA-256 a node names *is* that node, a verified anchor by construction, and is drawn in its place in the map. Anything else is a lone point. The state space as a whole is never drawn as a graph: nearly all of it is noise, and a graph the size of the space would show only a tangle.

## 13. Architecture and Implementation

- **Core** (headless): addressing, canonicalisation, classification, door mapping and Registry access, exposed through a narrow interface and a command-line tool.
- **Client** (the hallway): rendering and input only. It asks the core what is at a position and **never decides content itself**, so it can be replaced without touching the core.

Implementation (as built):

1. **Core** in C++20 with no dependencies: exact integer arithmetic throughout the address path, SHA-256 (with a hardware path where the CPU has SHA instructions; both paths must give identical digests), the scramble, canonicalisation for every line, and the sieve. The same code serves the command-line tool and, later, the hallway client.
2. **Independent reference oracle** in Python. It shares no code with the core and generates the conformance vectors; the core must reproduce them bit for bit.
3. **Filtration stack** (§8.5): versioned filter modules in the core, one registry, exact integer decisions and survivor rankers. The client only displays their verdicts.
4. **Conformance suite:** fixed inputs with expected addresses, canonical forms and image quantisations. It runs on Windows, Linux and macOS on every push, which also confirms that every platform produces identical addresses.
5. **Pinned data:** dictionaries are registered by id and SHA-256 (`data/dictionaries/dictionaries.tsv`) and are refused if their hash no longer matches.
6. **Models:** the core contains the character model and the exact coder (§4.2). Larger frequency models (longer contexts, more text) can replace it, provided they emit the same kind of pinned integer tables. Models are character-frequency statistics only.
7. **Hand-tuned code** (SIMD, assembly) only where profiling proves it worthwhile, and only once it passes the full conformance suite.

---

## 14. Milestones

| # | Milestone | Deliverable | Status |
| :--- | :--- | :--- | :--- |
| M1 | Exhaustive sieve | Enumerator + S0/S1 filters over **every** unit: text at lengths 4–8, images at 5×5 and 6×6 1-bit; plot of surviving fraction vs. size | **Text done.** Exact counts to length 1,000, cross-checked by brute force and pruned walk. The M1 filters are now modules of the filtration stack (§8.5), with exact survivor ranking. Images: exact survivor counts for `neighbour-agreement-v1` and black-and-white `symbol-entropy-v1` at every size whose table fits (for example, 6,053,792,982,126,620,565,936,264,884 of the 2^100 10×10 pictures agree on at least 60% of neighbouring pairs); the surviving-fraction plot is not made yet. |
| M2 | Raw addressing and warp | Positional and scrambled bijections, canonicalisation rules, CLI warp, round-trip tests | **Done**, for all four lines. Includes neighbour stepping. |
| M3 | Entropy-ordered addressing | Integer arithmetic coder with a small pinned text model; measured bits/char on real text | **Done for text (lower27).** Exact BigUint coder, order-5 pinned model; 1.93 bits/char on held-out Alice, 1.95 on Chesterton. Model rebuilt byte for byte by the oracle; guided vectors checked on every push. Models for the other lines need corpora. |
| M4 | Hallway prototype | 2D side view: shelves, zoom depth, readout, warp box, in-hand view, guided/raw toggle | **Built directly in 3D** (see M8). The raw and guided views are done: shelves, readout, warp and go-to, in-hand view, ordering toggle (positional, scrambled, guided) and zoom depth. Filtered shelving (§9) is done in every ordering: mark, hide, and compact shelves of survivors only (positional, shuffled, and on the sieved guided line). Compact works on every line with a ranking filter: text, black-and-white images and video, and audio. |
| M5 | Doors | Image and symbolic audio lines; fractional door mapping with return paths | **Done.** Exact door mapping in the core, with a return-path stack in the hallway. |
| M6 | Anchor Registry | Local, signed, append-only Registry with review workflow; anchored units shelved and marked | Not started |
| M7 | Paragraph-scale sampling | Classification and sampling at `UNIT_LENGTH` ≈ 1,000; extrapolation checked against M1 | Not started |
| M8 | 3D hallway | 3D client over the same core interface | **Raw and guided views done.** Wireframe SDL3 client over the core: books, doors, the four line colours, warp, zoom, playback of audio books. |

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
