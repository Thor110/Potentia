# Claude's ideas

Suggestions of my own (Claude), gathered in review 5 (21 November 2026): a whole-codebase review by ten reviewers, one an area each, and one more agent reading the project as a whole. These are ideas, not decisions: nothing here is built or planned until Edward says so. Each was checked against docs/IDEAS.md so as not to repeat one already recorded there, but some may overlap; where one does, IDEAS.md wins.

The fixes the same review found are in [REVIEW-5.md](REVIEW-5.md).

## The project as a whole

### C1. Addresses that catch their own typos and wrong settings: an optional check suffix and a self-describing locator

WHAT: an optional suffix that warp prints and read checks, e.g. `007b30...e27~k3`. The check is the first 2 to 3 base-32 characters of SHA-256(the line's canonical settings string + mode + address digits). Add a full 'locator' form that carries the settings too, e.g. `sieve:text/lower27/L32/key=sieve/scrambled/007b30...e27~k3`, which read, --goto and the File Locator accept directly. WHY: the README says an address is a place you can hand to someone else, but nothing protects it in transit. Every address holds a unit, so any slip still decodes to something. Measured here: `sieve read --length 32 --mode scrambled 007b...e28` (last digit changed) prints 'dpjy pgazqlaejvqlxydehmsb rxzqvg' with no warning; the same address under `--key other` prints other noise; the positional address with its last digit changed prints 'it was the best of times       a'; and the guided address read at `--length 31` returns the text with no warning. For a project whose promise is exactness, a silent wrong answer when someone types or pastes an address is the main way a user loses trust. HOW: the suffix sits after the address, so every address (positional, scrambled, guided, compact) stays exactly as it is. A bare address is still accepted as now; a wrong suffix is refused, and read says which part disagrees (digits or settings). The settings string already exists in canonical form (warp prints `space lower27/L32/key=sieve/feistel-sha256-v1`). Write it as `address-check-v1` in SPECIFICATIONS, with oracle vectors. COST: about a day in core and CLI, half a day in the oracle and vectors, and a CI step-02 check that a one-digit change is refused. No .sieve or address bytes change.

*Where:* core/src/sieve.cpp (warp/read), tools/cli/lines.cpp, client (address entry / --goto, favourites), SPECIFICATIONS §4, oracle reference/sieve_ref.py

### C2. An output ledger: every .sieve and address the build makes from a fixed corpus, pinned by SHA-256 and checked in CI

WHAT: a checked-in table of SHA-256 values for every output the hard constraints protect. It covers a fixed small corpus (about 3 to 5 MB: a text, source files, a PNG, a JPEG, a WAV, a GIF, a program, a zip, a pair of near-duplicates) packed at every frozen version (`--v33`, the default v34), every `--priority`, and Compressed/Sound temporary and permanent. It also covers warp outputs for a set of units in every ordering (positional, scrambled, guided, compact under a few filter stacks) on every line. `tools/ledger.py check` remakes the outputs and diffs them; `tools/ledger.py update` rewrites the table, so an intended change appears as a one-line diff in the same commit. WHY: the project's hardest rule ('the same .sieve bytes, the same addresses') is checked today by hand, before and after each change: OPTIMISATIONS.md and every review entry describe comparisons made by hand. CI pins survivor counts and a few hashes, but a packer change that moves a byte in a way CI does not exercise goes unseen. A ledger makes accidental drift impossible to merge and makes every intended format change visible. HOW: reuse the CI runner's fresh-copy setup; with --changed, run only the ledger rows whose code a change reaches (ci/reach.txt). The corpus must be public or generated, never anchors. COST: about a day to write. CI time of a few minutes, mostly the size-priority mixing; this could be capped by keeping each mixed file under 200 KB. Nothing it checks changes any output.

*Where:* new tests/ledger/ (corpus list + ledger.tsv), new tools/ledger.py, ci/steps/02 and 11, docs/OPTIMISATIONS.md's rule

### C3. One table of recipe kinds in place of the parallel lists in locate.cpp and pack.cpp

WHAT: one `constexpr` table of recipe kinds. Each row holds: name ('jbr', 'gif', 'flac', 'tiff', 'pcx', 'pmp' ...), listing format number (21..29), the manifest version that introduced it, the number of words before the inner way, whether its words must be plain numbers, a pointer to its remake function, and its help line. WHY: today a recipe's facts are written in several places that must agree. kArchiveFormats gives the number. A 19-way chain of ternaries in recipe_words gives the word count. A separate kNumbers list in remake_archive says which words are numbers. An if-chain dispatches the remake. Measured here: the literal "pcx appears 13 times across pack.cpp, pcx_rle and locate.cpp, "tiff 15 times, and "pmp 17 times. A v35 recipe means editing all of them in step, and missing one is a reader/writer disagreement that only the oracle catches. This follows the project's own rule (ROADMAP: 'one way of doing a thing is shared rather than copied', IDEAS §19.196). HOW: build the table and derive archive_format_number/name, recipe_words, the numbers check and the dispatch from it. Keep the order exactly so every listing byte is the same; the existing CI step 11 and the output ledger above would show it. The oracle keeps its own independent table on purpose. COST: one to two days of mechanical refactoring in tools/cli. No output changes. It shortens pack.cpp (8,028 lines) a little, and makes the next format version cheaper and safer to add.

*Where:* tools/cli/locate.cpp (kArchiveFormats, recipe_words), tools/cli/pack.cpp (kNumbers in remake_archive, the if-chain of recipe.rfind(...)), tools/cli/lines.cpp, tools/cli/web_host.cpp, help.cpp

### C4. Primers for more kinds than web text: a source-code primer and a prose primer for small files (v35 candidates)

WHAT: v33's W1 primes mix-v4 on 64 KiB learnt from MDN before a web text (8.8% under brotli on unseen pages). Do the same for two more kinds whose small files are common in folders: program source (C/C++/Python/JS) and English prose (Markdown/plain text). Each would be a pinned 64 KiB primer, named by SHA-256 and carried by installer programs as the dictionaries are. WHY: measured here with `sieve mix --params B20` (primer + file, less primer alone). With 64 KiB of the client's own C++ as primer, five unseen source files of 1 to 12 KB came out 13% to 26% smaller: intlog.cpp 464 to 362 bytes, timings.cpp 1,184 to 875, json.cpp 2,153 to 1,850, alphabet.cpp 2,647 to 2,194, compare.cpp 3,118 to 2,641. With 64 KiB of VISION.md as primer, four 3 to 5 KB docs came out 14% to 23% smaller (RAID-NOTICE.md 1,202 to 929). These are upper bounds, because primer and files share an author and codebase; the real primers must come from other public sources under compatible licences, never from the benchmark folders. The gain is largest on single small files and on the first files of a stream, which is where the 'ladder of cuts' says Sieve loses to 7z. HOW: build the primers as W1's was (a learnt dictionary), and measure first on held-out public corpora (a few open-source projects, Gutenberg books not in the guided model's training set). Weigh them per file as W1 is (a primer is used only where smaller), behind --v35-draft. COST: one to two days to measure. A primer adds 64 KiB to the dictionaries folder and adds weighing time for text-like files only. The new mixing parameter (W2/W3) needs the spec and the oracle.

*Where:* core/src/mix.cpp (W1's priming path), data/dictionaries/, tools/cli/pack.cpp weighing, docs/NEXT-VERSION.md v35 list, tools/measure_web_dictionary.py as the template

### C5. Mixing in lanes for large files, so install can use every core (v35 candidate, large files only)

WHAT: a mixing parameter, e.g. `N4`, that cuts one large file (or group) into N equal lanes. Each lane is coded by its own mix-v4 model from a cold start, and the lane codes sit one after another, each with its length. Install decodes the lanes on N threads. WHY: mixing decodes at about 0.6 MB/s on one core, and IDEAS line 4495 records that 'one file's mixing is one stream, which threads cannot split without a new version'. enwik8 installs in 149 s, and the rising bar exists only because of that single-core floor. With 4 lanes, a 100 MB mixed file would install in about a quarter of the time. That might let Balanced keep mixing on files where it now falls back to LZMA2. Cost of lanes, measured here: 1,702,100 bytes of the docs mixed whole at B22 gave 407,926 bytes; cut into 4 lanes of 425 KB, 436,588 (+7.0%). So lanes are wrong for small files. The question to measure is the loss at 4 x 25 MB (enwik8) and 4 x 4 MB. The loss should shrink as the lanes grow, since each model warms up once. HOW: the packer weighs the lane form only above a size threshold and only under Balanced/Speed, using the same rule as the rising bar (saving against install seconds). The reader runs lanes on pack_threads(). Memory is N models (240 MB each at B22), so N must be bounded by the installer's half-of-memory rule. COST: a measurement first (an hour with `sieve mix` on split files). If it holds up, about 2 days for spec, packer, reader and the oracle (lanes are just concatenated mix-v4 codes, so the oracle changes little). Existing .sieve bytes do not change.

*Where:* core/src/mix.cpp (Mix4Run), tools/cli/pack.cpp (mixing's rising bar over 16 MiB, Balanced priority), SPECIFICATIONS mixt/mix4 parameters, oracle

### C6. Where the bits go: per-symbol cost maps for addresses and for mixing, shown in the existing viewer

WHAT: three views of one quantity, the bits each symbol costs. (1) `sieve warp --why`: under the guided ordering, each character with its -log2 P from the pinned model and a running total that ends at the address length ("it was the best of times" costs 58 bits: which characters cost the most?). (2) `sieve mix FILE --cost-map OUT.tsv`: the bits mixing spent on each byte (or each 256-byte block), next to LZMA2's literal/match cost where that can be had. (3) In the hallway's existing book/page viewer, a key that tints each character by its guided cost: no new window and no menu row. WHY: the project's central claim, that an address carries exactly the information of what it names and only modelling shortens it (README, IDEAS §0), is now argued in prose. A cost map lets anyone see it: meaningful text glows cheap, noise glows expensive, and a typo shows as a spike. For compression research, a cost map of a benchmark file shows where Sieve loses to cmix or xz (tables, numbers, headers), which tells you which model to build next better than whole-file totals do. 'Context mixing', 'surprisal' and cost maps appear nowhere in the docs. HOW: the guided coder already has each symbol's frequency F_i/T. The mixer already has each bit's 12-bit p, so the sum of -log2 over a byte's 8 bits is a side output (diagnostic only, in floating point, never feeding any code). COST: about a day for the CLI parts and a day for the viewer tint. No address or .sieve byte changes, since the maps are read-only.

*Where:* core/src/guided.cpp, core/src/mix.cpp, tools/cli (warp, mix), client/viewer.cpp (book/page view), docs/MANUAL.md

### C7. Differential fuzzing: generated folders packed by the C++ and installed by the oracle, and random line settings warped by both

WHAT: a seeded generator of small folders that hit the packer's decisions. It would mix: text fragments; near-duplicate pairs (delta, copy K); a file that is another's lines (masks); tiny PNGs from stb and from zlib; a GIF; a WAV; a short x86 program; base64 inside text (spn); a zip holding a JPEG; names equal but for case or NFC/NFD; empty files and empty folders. Each folder is packed by the C++ under each priority and Compressed/Sound setting, then installed by the C++ and by the oracle (`sieve_ref.py unpack`), and all three trees are compared. A second mode draws random line settings (alphabet, length, min-length, palette, key, filter stacks, compact) and units, and compares C++ warp/read with the oracle's. Any disagreement is saved with its seed, and the folder is shrunk to a minimal fixture. WHY: fuzz_reader.py mutates valid .sieve files to find crashes in the reader, and the conformance vectors are fixed points. Nothing yet searches for the case where the C++ writer and reader agree with each other but not with the specification. That is the failure the oracle exists to catch, and it is likeliest where several ways combine, which the v33 review found by hand ('the six pieces together'). HOW: Python, one process per worker as fuzz_reader.py does. Keep files small (under 64 KB) because the oracle's mixing is slow; time-box each run. COST: two to three days for the generator and the shrinker, then machine time only. No output changes, though it may turn up bugs.

*Where:* tools/fuzz/ (next to fuzz_reader.py), reference/sieve_ref.py, ci (a nightly or opt-in step)

### C8. A .sieve made against an installed folder: release updates and backups that carry only what changed (v35 candidate)

WHAT: `sieve locate NEW --installer update.sieve --base OLD`. OLD is a folder or a .sieve, named in the head by its tree SHA-256. In the new .sieve, a file may be 'as base file K' (unchanged) or 'delta against base file K' (v32's delta way, with its source outside the folder). Install needs `--base OLD_FOLDER`. It checks every base file named against the SHA-256 in OLD's listing before using it, then checks every output as now. WHY: delta and copy K already find what a folder shares with itself, and nothing reuses what the person already has. Sieve's own release (45.9 MB, a 6.47 MB .sieve) changes a few programs between versions. An update made this way would be the changed programs' deltas, likely well under a megabyte, still exact and checked. It also gives the infinite RAID and the vision's preservation framework the classic incremental backup: each snapshot carried as differences from the one before, each fully verified. HOW: version the head with a base hash, and add two way names that read from the base tree. The reader refuses to start unless the base is present and every named file matches, so a wrong base can never produce wrong output. The oracle needs a base folder argument. COST: 3 to 4 days with spec, oracle and CI. It changes no existing .sieve, since it is a new version used only with --base.

*Where:* tools/cli/locate.cpp and pack.cpp (delta and copy-K ways), core installer, SPECIFICATIONS §12.2, tools/make_release.py, sieve-install

### C9. mix-v5 research: bit histories (indirect contexts), hash check bytes and a confidence input, measured against mix-v4

WHAT: a measured prototype of three changes from the lpaq/paq lineage, each in integers. (a) Indirect contexts: each hashed context slot holds an 8-bit bit-history state (lpaq's nex table), mapped to a probability by an adaptive StateMap, in place of or beside today's direct counter (Counter.p moved by 1/(n+2)). (b) A check byte per slot, so two contexts that hash together stop sharing a counter (today `t_[base + c0*0x9E3779B1 & mask]` has no check, and at B22 on large files collisions are certain). (c) A second mixer input per context, giving the counter's confidence (its n) or 'is this a deterministic context' as lpaq's run model does. WHY: IDEAS §19.126 records the gap to cmix (15% to 26% on text and source) and lists 'more context models, more mixer and SSE layers', but not these. These change how every existing context predicts, not which contexts exist, and in the paq family they are among the cheapest per gain. SSE was retired for costing 9% to 29% of the time; indirect states cost a table lookup per input and may give a better gain per second. HOW: prototype as `sieve mix --v5-trial` on the standard five (Don Quixote, the tools/cli tar, SCOWL 80, Vitruvian pixels, a program) and enwik8's first 16 MB. Report bytes and decode seconds next to mix-v4; adopt only parts that pay for their time under Balanced. COST: 2 to 3 days to prototype and measure. If adopted, a new mixing version with an oracle implementation (the oracle's slowness bounds its tests to small files). No existing bytes change.

*Where:* core/src/mix.cpp (Model: Counter tables indexed by hashed context with no check), results/, docs/NEXT-VERSION.md

### C10. A glossary that maps Sieve's words to the field's standard terms

WHAT: a two-column table, Sieve's word against the term a compression or computer-science reader knows, with a one-line note. Examples: mixing = context mixing (lpaq-style, logistic mixing, binary arithmetic coding); way/recipe = coding method/transform chain; weighing = trial compression and selection; temporary/permanent = lossless recompression (as precomp/preflate) versus normalisation that changes the bytes installed; jbr = brunsli JPEG recompression; the x86 filter = BCJ; the rope = Sieve's own predictive transform; the binary line's address = bijective base-256 numeration; positional/compact = ranking/unranking (enumerative coding over a DFA); guided = arithmetic coding under a static order-5 model; the roof = memory/size limit; groups = solid blocks. WHY: the project's plain-language style suits a general reader. But the audiences SHOWCASE-PLAN names (encode.su via ROPE-POST.md, people comparing it with 7z/zstd) look for the standard names. Checked here: 'context mixing' appears 0 times in README, MANUAL, METHODS, FEATURES, SIEVE-FORMAT and ROPE-POST. A reader who cannot match 'mixing' to context mixing or 'the counting argument' to the pigeonhole principle may misjudge the claims, or miss that the address-is-not-compression argument is stated correctly. HOW: one section in MANUAL linked from the README's documentation table, and a few standard terms placed once in parentheses in ROPE-POST and SIEVE-FORMAT. The docs-checked-from-code stage can confirm each mapping. COST: half a day. No code.

*Where:* docs/MANUAL.md (Key terms), docs/METHODS.md, docs/ROPE-POST.md, docs/SIEVE-FORMAT.md, README.md

### C11. Reproducible release builds, checked by CI: the same source gives the same sieve.exe bytes

WHAT: make the release binaries bit-for-bit reproducible from a tagged commit with a pinned toolchain. On MSVC: /Brepro, /PDBALTPATH, no __DATE__/__TIME__, a fixed VERSIONINFO date. On GCC/Clang: -ffile-prefix-map, SOURCE_DATE_EPOCH, a fixed link order. Add a CI job that builds twice in different directories, compares SHA-256, and publishes the expected hash beside the release's file list. WHY: IDEAS 4.3, 4.8 and §16 each note that the release as first anchor and 'compiles itself' need reproducible builds, and that 'an executable differs ... between two builds of the same source'. No step to make it so is recorded. For a tool whose whole promise is 'exactly what went in, checked', a release anyone can rebuild and hash is the strongest proof that the published sieve.exe is the published source. It also lets the release anchor (sieve-build-release-v1) be a claim others can check rather than one they must trust. HOW: first measure what differs between two builds today (diffoscope on Linux; on Windows, compare after /Brepro) and fix those sources one at a time. Bundled third-party code built through FetchContent may need its own flags. COST: one to three days depending on what the diff shows; CI doubles one build job. No .sieve or address changes.

*Where:* CMakeLists.txt / CMakePresets.json (compiler flags), tools/make_release.py, .github workflow, docs/SIEVE-INSTALL-USAGE.md

### C12. Test and audit without installing: check a .sieve fully while writing nothing, and check an installed folder against its .sieve without decoding

WHAT: two modes on the existing install command, with no new windows. (1) `sieve install X.sieve --test`: decode everything and check every file's size and SHA-256 and the tree hash, then write nothing and print pass/fail with timings (7-Zip's `t`). (2) `sieve install X.sieve --audit FOLDER`: read only the listing (which `sieve map X.sieve` already prints with every file's size and SHA-256, and which needs no stream decoded), hash the files in FOLDER, and report missing, changed and extra files. WHY: the vision is preservation ('a digital civilisational backup'; the RAID 'kept whole by its owner's checks'). Preservation needs periodic integrity checks: has the archive rotted, and has the installed copy drifted from it? Today the only check is a full install into a scratch folder. --audit costs only a hash of the folder, so it can run over a large install in seconds where install takes minutes. HOW: --test reuses the install path with the writer replaced by a counter. --audit reuses map's listing reader and the SHA-NI hash path, and needs no format change, though it needs per-file hashes (--items, or the tree's leaves where the listing has them). Where a .sieve lacks per-file hashes, say so and fall back to --test. A right-click 'Test' entry could come later (not on the main or pause menus). COST: about a day. No outputs change.

*Where:* tools/cli (install command, map's listing reader), context menu (context/), sieve-install, docs/SIEVE-INSTALL-USAGE.md

### C13. A tour you can play: shipped replays with spoken captions in the existing Replays viewer

WHAT: three or four short replays shipped with the release and listed in the existing Replays viewer: 'an address is a place' (warp a sentence, walk to it); 'filters set the noise aside' (raw view against guided view); 'a .sieve comes back exactly' (the File Locator packing and installing, with the ratio shown); 'the shelves of other lines'. Each carries captions that appear on screen and are spoken in Sieve's own voice at chosen frames. WHY: Sieve is hard to explain in words (VISION's own primer warns of 'jumping into page 100'), and the hallway is its most direct explanation. Recording and replay already play the hallway back exactly from a few bytes a frame, and the voice exists. A tour is then a few kilobytes of text that cannot drift from the program, because CI step 10 can replay it and screenshot it as it does now. The trailer (a video) goes stale with every UI change; a replay fails loudly in CI instead. HOW: a sidecar `NAME.captions` (frame number, then text) leaves sieve-replay-v1 unchanged. Alternatively, a `sieve-replay-v2` token `cap:HEX`, specified with a grammar as the other save formats are getting. The viewer draws the caption and speaks it if Read aloud is on. Nothing is added to the main or pause menus: the tours are entries in the Replays list. COST: about a day for captions in the replay player, plus recording time for the tours. Replays depend on settings and start position, so each tour must name its settings, which the start line already checks.

*Where:* client/replay.hpp/.cpp (sieve-replay-v1), the Replays viewer, client/speech.cpp (Sieve's voice), data/ (shipped replays), docs/SHOWCASE-PLAN.md

## Core arithmetic, automata and addressing

### C14. Product-tree guided coding: exact intervals in quasi-linear time

A guided interval is a composition of affine maps: a run A followed by a run B gives low = low_A * T^|B| + w_A * low_B and width = w_A * w_B. Splitting the unit in halves and combining with the existing Karatsuba mul gives O(M(n) log n) instead of O(n^2), with exactly the same integers, so every guided address stays the same. Measured here: interval at 20,000 characters takes 124-236 ms and unit_at 250-420 ms, and both grow with the square. The 200,000-character pages the review notes mention would take tens of seconds. Decoding is sequential by nature. It could use the same bisection on blocks of symbols: decode a block from a truncated top window, then verify and correct it exactly with the block's composed map. That needs care and a differential test against today's unit_at. Cost: a few days, plus oracle vectors at long lengths.

*Where:* core/src/guided.cpp (interval, unit_at), core/src/biguint.cpp (mul), tests/

### C15. A differential BigUint fuzzer in CI against Python's integers

The fixed vectors pin known cases, but the risky edges are the algorithm switch points (kKaratsuba 32, kLadderBarrett 64, kBarrett 160, kBarrettOnce 1000, kConvertLeaf 40) and all-ones or top-bit operands. This review's throwaway fuzzer (scratchpad review5/fuzz.py + bfuzz.cpp) checks mul, divmod (including 3n-by-n Barrett), mod, gcd, to/from_decimal and to/from_digits in bases 2..2^32-1 against Python. It covers 0..6,300 limbs at those thresholds in a few seconds and found nothing today. Seeded and run in step 11 under both SIEVE_BIGUINT_PORTABLE and native, it would catch a regression from any future threshold or algorithm change (Toom-3, an FFT multiply) at once. Cost: an hour to tidy and wire in.

*Where:* core/src/biguint.cpp, ci/steps/11-oracle.sh, tools/

### C16. Persist merged automata and counting tables across sessions, keyed by provenance hash

merge_plugins and shared_table already key their results by the plugins' provenance. Both are pure functions of that key, the symbols and the length. Merging every text plugin takes about 5.4 s and its table about 1 s, every time the program starts and settles a stack. Writing the minimal Dfa (next/accept arrays) and PackedRows to a cache folder under the key's SHA-256 would make a second start instant. Each entry would carry its own hash, as the other provenance caches do (REVIEW-ROUND N5-18 audited them), be checked on load, and be evicted by size. Nothing in an address depends on it. A wrong entry would show up as a count mismatch against the minimal automaton's own count, which can be re-verified cheaply from row 0 upward.

*Where:* core/src/filter.cpp (merge_plugins, shared_table), core/include/sieve/packed.hpp, core/src/dfa.cpp

### C17. Build the product of automata minimised on the fly

intersect() builds the whole reachable product and only then minimises it. That is why a merge can cost |A|x|B| memory even when the result is small (finding R5-ARITH-2). There are two well-known ways to bound it by the result. (1) Merge in a balanced tree rather than a left fold, minimising each partial product so that the largest intermediate is closer to the final size. (2) An on-the-fly product with hash-consed right languages, where states are identified by their outgoing signature as they are completed, as in Daciuk-style incremental minimisation for acyclic parts. Both give the same canonical automaton (minimise is canonical), so no count or rank changes. Worth measuring on the real plugin set (sizes and merge times per pair) before choosing.

*Where:* core/src/dfa.cpp (intersect, minimise), core/src/filter.cpp (merge_plugins)

## Filters and plugins

### C18. One identity for a plugin and its data

Today a plugin's identity (FilterSpec.plugin_sha256, the cache key, the registry's de-duplication) is the .sfilter's bytes, while its word lists are hashed separately into provenance. A bundle hash (the .sfilter plus each list/tags file it names, in order) would be the one key for the registry, the compile cache, the packed .sieve form and provenance. Finding F1 would then be impossible by construction, and two plugins whose lists differ could never share one name. Cost: one hash per list at load (Moby is a few MB) and a documented change to what the identity means, as a new version of the identity, not an edit.

*Where:* core/src/plugin.cpp, tools/cli/plugins.cpp, docs/FILTER-PLUGINS.md, reference/sieve_ref.py

### C19. A byte-budgeted cache for every process-kept automaton

kept_dfa, kept_answer, g_compiled (cleared wholesale past 64 entries of any size) and History each grow on their own and none is counted against the filter memory or the merge-cache share. One LRU keyed by name, measured in bytes and limited by merge_cache_share() x filter_memory(), would make memory predictable when settings are swept (the rows-alike slider, plugin parameters). Results stay the same because everything evicted is rebuilt deterministically.

*Where:* core/src/filters/dfa_bound.hpp, core/src/plugin.cpp (g_compiled), core/src/filters/changes_ranker.hpp (History), written.cpp

### C20. Differential fuzzing of the .sfilter reader against the oracle

The parser and compiler take untrusted text, and the oracle has its own parser. A libFuzzer/ASan target over parse_plugin + compile_plugin (with a corpus of the shipped plugins and mutated expressions, for/if nesting, ranges and quoting), plus a small Python harness that compares the C++ and oracle verdicts on the files that compile (accept, refuse, error line), would catch disagreements and crashes that the hand-written cases do not reach. It fits the existing hostile-.sieve fuzzing set-up.

*Where:* core/src/plugin.cpp, reference/sieve_ref.py plugin, CI

### C21. Property test that every ranking filter's judge and ranker agree

Exhaustive judge-against-ranker checks exist for many filters one by one. A registry-wide loop would remove the need to remember a new filter: for each FilterSpec, on every line it applies to and that is small enough to enumerate (text L<=3, images 3x2, notes 2-3 events, at default and edge settings), compare passes() with the ranker's count, rank(unrank(k)) and unrank order. Filters that judge only are skipped automatically. It guards the property that compact addresses rely on, at a CI cost of seconds.

*Where:* tests/test_core.cpp, core/src/filters/*

## The lines' spaces and sieves

### C22. One numbering kernel for every composite line

Books, compositions, worlds, titled lines, models and their sieves each write their own mixed-radix join and split (cover x title x units), some linearly (WorldSpace: one multiply per slot, so quadratic) and one balanced (CompositionSieve::join_ranks). A single `MixedRadix` type would hold a list of parts, each a (count, rank, unrank) triple, and do a balanced product-tree join and split with cached powers per level. Every space and sieve would declare its parts. One property test (random parts -> index -> parts, and index ordering equal to the old code over random inputs) would then cover them all, and the speed-up for many slots or units would come to every line at once. Cost: a few hundred lines and a careful equality test against today's code; no address changes.

*Where:* core/src/bookspace.cpp, booksieve.cpp, composition.cpp, worldspace.cpp, titledspace.cpp, modelspace.cpp

### C23. An expansion budget shared by every canonicaliser

S1-S3 come from one cause: a canonicaliser whose output size is set by numbers inside the input (ticks, rates, deltas), not by the input's length. A small `ExpansionBudget` (most output digits, for example the unit length x a most-units setting, or N x input bytes) passed to canonicalise_notes, notes2, notes3, the MIDI readers, PcmCanoniser and ImageCanoniser would make every refusal uniform and name the limit in the warp report ("would make 96,000,000 samples; the most is ..."). Cost: small, and no address changes for accepted inputs.

*Where:* core/src/audio.cpp, notes3.cpp, sound.cpp, image.cpp; tools/cli/lines.cpp; client/item_save.cpp

### C24. Fuzz the lines' readers under a memory cap in CI

The repository already fuzzes the .sieve reader (task 82). The same libFuzzer/ASan harness with -rss_limit_mb=512 over midi_to_notation, midi_to_notes3, canonicalise_notes*, read_wav + PcmCanoniser, read_any_obj, read_fractal and notes3_set_of / note_set_of / pcm_format_of would have found S1-S3 and S6 in minutes. A short run of each (30 s) fits in CI step 01, with the crash corpus kept under tests/.

*Where:* tests/, ci/steps/01-*.sh, core/src/audio.cpp, notes3.cpp, sound.cpp, modelspace.cpp, fractal.cpp

### C25. The AI line without a BigUint of its size

The AI line's size is exactly 2^bits, and an address is just the weights' bits. Holding size_, its hex, and the Shuffle's copy of both costs about 8 bytes per weight (S4). A specialised power-of-two Shuffle (Feistel over bit strings, with its round inputs hashed from a streamed N) could give the same scrambled addresses while working on byte vectors, so the menu could offer real model widths. Worth measuring first: the time for one scrambled address at 16x1024 today, against a byte-vector version that is checked equal on small shapes.

*Where:* core/src/aispace.cpp, core/src/compact.cpp (Shuffle), client/menu.cpp

## Characters, saves, RAID, AI and the rest of core

### C26. Fuzz harnesses for core's untrusted readers, run in CI under ASan/UBSan

Earlier rounds fuzzed the .sieve reader, but the readers in this area have none: read_gltf/read_model, jpeg_eighth, safetensors::parse_header, weights::unpack, Tokenizer construction and encode, read_raid_piece/manifest/tally, read_animation/read_character/read_save, and RaidHeld::serve over a byte stream. Two of the findings here (the sparse overflow and the scan bomb) are the kind a fuzzer finds in seconds. Build: one small libFuzzer (or AFL-compatible) entry per reader under tests/fuzz/, seeded from the test vectors, plus a fixed-time CI step that runs each for N seconds with an RSS and time limit, so allocation and time bombs fail the run as crashes do. Cost: about a day to write, a few CI minutes per run.

*Where:* tests/ (new fuzz targets), ci/, core/src/importers.cpp, jpeg_dc.cpp, safetensors.cpp, weights*.cpp, llm_tokenizer.cpp, raid*.cpp, animation.cpp, character.cpp, save.cpp

### C27. One 'output per input byte' budget for decoders and importers

Each reader has its own roof (kMaxTriangles, 1 GiB in jpeg_eighth, 100 MB headers, kAnimationMaxFrames), and each one leaves a gap: sizes multiply across fields. A small shared Budget object passed down (bytes allowed = a constant plus a ratio times input size, settable by the caller, e.g. the viewer from free memory as it already does for pictures) and charged at each allocation or loop would close these gaps in one place and give one consistent refusal message. The same input reads the same while within budget.

*Where:* core/include (new budget.hpp), importers.cpp, jpeg_dc.cpp, weights.cpp, client/picture_source.cpp, client/viewer.cpp

### C28. LLM step: attention across heads on the pool, and fewer pool hand-offs a layer

Model::step runs the attention of every head on one thread. Pool::each also refuses to split anything under 64 items, so the heads (14 for Qwen-0.5B) cannot use it as is. Each layer also makes 7 separate pool dispatches (q, k, v, o, gate, up, down). Worth measuring: tokens/s against context length (128, 512, 2048) as it is, then (a) heads split over threads with per-thread score buffers (the same numbers, since each head is independent) and (b) q/k/v and gate/up in one dispatch each. Attention grows linearly with position, so the gain should rise with longer chats. The sampler's full stable_sort of the vocabulary each token is worth timing alongside.

*Where:* core/src/llm.cpp (Model::step, Pool, Sampler::pick)

### C29. Faster GF(2^8) for RAID pieces with split-nibble tables

add_scaled builds a 256-entry table per coefficient and looks up one byte at a time. The usual erasure-coding kernel (ISA-L, Jerasure) splits each byte into two 4-bit halves and uses two 16-entry tables with PSHUFB (SSSE3/AVX2, NEON vtbl), handling 32 bytes per instruction pair, about 10x faster with identical bytes. Making parity for K+M up to 256 and joining large wholes would gain most. It could sit behind the same runtime CPU check training.cpp already uses (cpu_has_avx2), keeping the scalar path as the reference and comparing them in a test.

*Where:* core/src/raid_piece.cpp (add_scaled, raid_pieces, raid_join)

## The packer and format codecs

### C30. One compare-as-you-encode helper for every recipe search

Each recipe search (gzip levels x memory levels, xz/lzma presets, bz2, stb PNG levels, zstd/lz4 prefixes, brotli's `against`) answers the same question: does encoder E with settings S make exactly these bytes? Today each answers it its own way, and some (gzip, xz, lzma, stb) only at the end. A small template that runs an encoder with a bounded output buffer, compares each piece with the target and stops at the first difference would make every search cost about one encode on a miss and no more on a hit, without changing which settings win. It would take about a day: wrappers for zlib, liblzma, libbzip2 and stb's zlib callback, plus a test that the chosen recipes stay the same on CI step 11's archives.

*Where:* tools/cli/pack.cpp (gzip_recipe, xz_recipe, lzma_recipe, bz2_recipe, png_by_stb), tools/cli/png_resave.cpp

### C31. Algorithmic-complexity checks in CI for every format detector

R5-P2 (quadratic span finding) shows a gap: fuzzing checks that the reader is safe, but nothing checks how time grows with size on the packing side. A CI step could build adversarial inputs at two sizes (N and 4N) for each detector: lists with varying separators and long hex/base64 runs for spans, long single-byte runs and near-duplicates for delta, many tiny GIF frames on a big canvas, TIFFs with thousands of strips, gzip and zip bombs. It would fail when time or peak memory grows faster than about 5x. It is cheap to run (seconds) and would catch the next O(n^2) before a user's file does.

*Where:* ci/, tests/, tools/cli/text_spans.cpp, delta.cpp, gif_lzw.cpp, tiff_codec.cpp, vault.cpp

### C32. Vault picture checks streamed frame by frame, with a stated fail-closed policy

Build the PDQ check for animations on gif_lzw's own LZW decoder and a single canvas: decode, compose, hash and drop each frame, keeping one canvas and the previous frame. Memory becomes O(canvas), not O(frames x canvas). The same streaming idea fits APNG and multi-page TIFF later. Pair it with a written policy in docs/VAULT.md: a picture that cannot be checked within the budget is refused with a reason, never passed. It would also let the viewer play long GIFs without holding every frame.

*Where:* tools/cli/vault.cpp, tools/cli/image_io.cpp, tools/cli/gif_lzw.cpp, docs/VAULT.md

### C33. Measure the archive step's time per MB by format and encoder of origin

The archives step can dominate a pack (37 s of 154 s here for one 7-Zip gzip), but how much depends on which encoder made the file: zlib, 7-Zip, libdeflate, pigz, zopfli, Info-ZIP. A one-off table of seconds per MB for each recipe search and for preflate, by encoder of origin and size, would show where an early stop or a header sniff (for example 7-Zip's gzip never matches zlib, so skip straight to preflate) pays most. It would also give the File Locator's Optimise Search routes real numbers.

*Where:* tools/cli/pack.cpp archives step, results/, docs/OPTIMISATIONS.md

## The command line

### C34. One declared option table per command

Today, flags live in a 150-key `is_flag` string chain (args.hpp), values are parsed ad hoc (parse_whole in some places, bare stoul/stod in others), and 'unknown option' is guessed by scanning help text for '--'. A table per command, {name, kind: flag|whole(max)|seconds|path|text, help}, could drive parse_args (flags known per command, so `--out --force` cannot swallow a flag), the validation (whole and seconds parsers in one place, paths always made with from_u8), the warning, and the help page's option list. It is a mechanical move of about 40 commands; existing behaviour can be checked by running each help example before and after.

*Where:* tools/cli/args.hpp, tools/cli/help.cpp, tools/sieve_cli.cpp, tools/cli/*.cpp cmd_* functions

### C35. Incremental site build

`sieve site build` remakes every picture and page on each run, with up to 1 s of rope search per file. Each made .sieve could be kept under a cache key of (source SHA-256, Sieve version, priority/draft and search settings), for example in the plugin cache directory. A rebuild would then copy unchanged .sieve files and remake only what changed, plus the pages whose stated hashes depend on them, which follows from the existing `needs` order. The output stays byte for byte the same; worth measuring on the project's own website.

*Where:* tools/cli/site.cpp (make_sieve, build_site), tools/cli/sieve_cache.cpp

### C36. Protocol fuzzing and per-peer fairness for raid serve and the web host

Both run over untrusted bytes. A libFuzzer or socketpair harness could feed RaidHeld::serve and run_web_host random and mutated streams (truncated lengths, 64 MiB-1 messages, huge put sizes, ids of every JSON type) under ASan/UBSan, alongside a small load test: N honest clients against M slow or abusive ones, measuring answer latency. That would turn the fairness fix above into a measured property (for example, 'an honest put is answered within T s while 32 slow peers are connected') checked in CI.

*Where:* tools/cli/raid_socket.cpp, core/src/raid_net.cpp, tools/cli/web_host.cpp, tests/

### C37. Measure and cut the web host's peak memory per decode

A decode now holds the base64 request (up to 64 MiB), its decoded bytes, the cached Manifest's contents (up to 128 MiB kept), a copy of each file to hash it (web_open's bytes_of, which copies even in listing mode), and the whole base64 reply text per part. Measure the peak RSS for a 256 MiB file and for a 9,000-file listing. Then hash spans in place instead of copying (Sha256::update on a span), stream the parts' base64 straight from the manifest's contents, and drop the per-request copy, so several tabs at once do not double memory use. The replies stay the same.

*Where:* tools/cli/web_host.cpp (web_open, write_in_parts, run_web_host)

## The hallway, menus and HUD

### C38. A judge-ahead service for every filtered line

Generalise find_room_files and vault_ahead into one background verdict service. A small pool, fed by the walking direction, judges the tiles ahead and the visible field for any line in mark, hide or excluded mode, and publishes verdicts by (room_gen, key). book() then reads a verdict that is ready, or marks the item pending, and render draws a pending item faint or not at all. Expensive judge-only filters (fractal paths, PDQ, AI language checks) would then never block a frame, and it would replace three ad-hoc thread blocks. It costs a lock-free result queue and a 'pending' state in Book. Verdicts are unchanged.

*Where:* client/hallway.cpp (book, render, find_room_files, vault_ahead), client/hallway.hpp

### C39. Settings-derived figures computed once per change in the setup menu

Gather everything the setup menu derives from settings (line_sizes, display_px per line, display_mb, closeup count and size, the budget and tallest_face) into one struct, rebuilt when a generation counter bumps in adjust, reset, find_limits, load and the overlay closing. render, draw_budget, stack_info and details_lines then read the struct. This removes the class of per-frame recomputation found here and makes it hard to reintroduce.

*Where:* client/menu.cpp, client/menu.hpp

### C40. A per-frame cost probe in CI

Add a --frame-probe N mode to hallway that renders N frames of a given screen (main menu, setup menu, hallway on each line) and reports per frame: file opens (counted through find_data_folder and a wrapped ifstream helper), gpu::bytes by use before and after, and allocations if cheap. CI step 04 or 10 would assert zero file opens per idle menu frame and a flat gpu::bytes after N item changes. That would have caught both the faces.ini reads and the listed_tex_ accounting drift.

*Where:* client/app_main.cpp, client/gpu_memory.cpp, ci/steps/04, ci/steps/10

### C41. Measure and cache v4 to v6 verdicts by ruleset text

On the fractals line, verdicts of judge-only filters depend only on the ruleset text and the filter version. A disk-backed cache keyed by the stack id plus the ruleset's SHA-256, next to the filters' cache, would make returning to the line's start (where v6's last rulesets stand) instant after the first visit. It would also let the setup menu sample how many of v4 to v6 pass. Measure first how many distinct rulesets a session judges, to see whether the cache pays.

*Where:* client/listed_shelf.cpp, core/src/filters/fractals.cpp, the filter cache folder

## The other client programs

### C42. Fuzz the client's own parsers as the reader was fuzzed

The .sieve reader was fuzzed under ASan/UBSan (task 82), but the client parsers that take files from disk or from .sieve members never were: read_obj and read_mtl, read_replay, PersonalWorld::load, the Unifont .hex loader, gif_frame_count/png_size/make_pixels, help_library contents and the music ini. A libFuzzer target per parser, with a memory and time limit, would have found C1 at once. Cost: one small CMake target with sanitizers, a seed corpus from data/ and tests, and a short CI run of a minute per target.

*Where:* client/mesh.cpp, client/replay.cpp, client/personal_world.cpp, client/font.cpp, client/picture_source.cpp, CMakeLists.txt, ci/

### C43. Installs that are atomic, replaced files included

Today a cancel or failure removes only what the install created; a file written over with Replace ticked stays overwritten (unpack_7z.cpp:374, install_tree). Write each file to a sibling temporary name (or a staging folder on the same volume), move any file it replaces into a backup name, and rename everything into place only once all are written. On cancel, rename the backups back. The installer could then truly say 'Nothing was left behind' in every case, and a power cut mid-install would leave no half-written file. Cost: a rename layer shared by install_tree, unpack_7z and unpack_archive, plus handling a destination on another volume.

*Where:* client/installer_main.cpp, client/unpack_7z.cpp, tools/cli/locate.cpp (install_tree)

### C44. A cover entry for .simage, so thumbnails read one picture

Rather than decoding a gallery to show its first picture (C6), the listing could mark one entry as the cover. That entry is placed first and read alone by a decoder that stops after it. The packer would have to keep it independently decodable, for example in its own stream. This is a format question (a new manifest version or recipe word), so it is a suggestion for when the .simage format next changes. Measure first how long Explorer thumbnails of large galleries take today.

*Where:* client/thumbs_main.cpp, client/picture_source.cpp, docs/SIEVE-FORMAT.md

### C45. Measure the stand-alone programs' worst cases in CI

sieve-install, sieve-view and the thumbnail path all have limits (memory budget, kMaxFiles, kOwnModelTriangles, check_7z_sizes), but nothing records their peak memory and time on adversarial inputs. A CI step could run each headless (SDL_VIDEODRIVER=dummy, --yes / --check) on a small set of crafted files (7z with one huge solid block, a 64 MB .obj, a .simage of 4096 tiny pictures, a replay with large tx tokens). It would log max RSS and seconds against a recorded ceiling, so any regression that undoes a limit shows up as a number.

*Where:* ci/, client/installer_main.cpp, client/view_main.cpp, client/personal_world.cpp

## The reference oracle and Python tools

### C46. A differential fuzzer of the oracle against the C++ reader, in CI

tools/damage_test.py runs one reader at a time and counts a non-zero exit as 'refused', so it cannot see the two readers disagreeing. A variant could damage each sample once and run both 'sieve install' and 'sieve_ref.py unpack' on it. It would flag any case where one installs and the other refuses, or where both install but the trees differ. It should also apply structured mutations: padded or over-long varints, swapped listing lines, sizes plus or minus 1, way words changed to another version's. I ran a rough version: 500 random damages of png24, t29fast and a runs/copy/raw sample found no split. The padded-varint case (F1) did split, so the structured mutations are what find things. A seeded 2-minute run in step 11 would cost little, using fast samples without mixing, since mix decoding in the oracle takes 18 to 35 s a file.

*Where:* tools/damage_test.py, ci/steps/11-oracle.sh, reference/*.sieve samples

### C47. One list of the oracle's vector commands, read by CI, check_vectors.py and the C++ tests

The vector commands are listed by hand in at least three places: ci/steps/11-oracle.sh, tools/check_vectors.py and tests/test_core.cpp's file names. They have already drifted (F3). A small tests/vectors.tsv (command and arguments, file, expected seconds) could drive all three. check_vectors.py would iterate it, step 11 would call check_vectors.py, and a ctest could fail when a tests/vectors_*.tsv file is missing from it. That also gives one place to record each command's run time for trimming CI.

*Where:* tests/, tools/check_vectors.py, ci/steps/11-oracle.sh, tools/run_tests.bat

### C48. Split sieve_ref.py into modules by line or format, keeping one entry point

The oracle is 11,986 lines in one file, with 14 copies of the varint loop and several copies of 'n, shift = 0, 0 / while True'. A package under reference/sieve_ref/ (bigint and spaces, filters, formats/manifest, codecs/rope and mix, models), with sieve_ref.py as a thin CLI importing it, would make shared helpers natural and let tools (sieve_read, ways_tally) import a listing reader without loading all of it. It needs no third-party code and stays independent of the C++. The cost is a careful move with step 11 as the safety net, since every vector must reproduce byte for byte.

*Where:* reference/sieve_ref.py, tools/sieve_read.py, tools/ways_tally.py

### C49. Measure and speed up the oracle's mixing decoders

Unpacking a 22 KB mix4 .sieve (m33-33) takes 18.6 s in the oracle, and the 26 KB ws32 takes 35.7 s. This is why mixing samples stay tiny and why the oracle is unusable on real folders. Profiling mix4_file and mix2_file (cProfile on m33-33) would show whether the cost is in Python big-int maths, per-bit list appends or dictionary lookups. Moving inner state into array('i')/memoryview, hoisting attribute lookups and precomputing the stretch/squash tables would likely cut it severalfold without changing the arithmetic. CI step 11 could then carry larger and more varied mixed samples, such as groups, v34 x86 and match ways, at the same time budget.

*Where:* reference/sieve_ref.py (mix_file, mix2_file, mix4_file), ci/steps/11-oracle.sh

## Tests and CI

### C50. One long-lived hallway process for the screenshot checks

Steps 04 and 10 start the hallway about 600 times, and each start pays fixed costs: about 1 s here (the fractal space, the 5-gram model at 0.15 s, plugins and the SDL window). A `--batch FILE` mode could read one scenario per line (the same options as now), reset state between scenarios and print each scenario's output between markers. One process per block of checks would remove most of step 10's 26 minutes without changing any check. It costs a reset function in the client and a small rewrite of the step to group runs.

*Where:* client/hallway.cpp, ci/steps/04-models.sh, ci/steps/10-hallway.sh

### C51. A per-check time table from the .trace files, kept as a CI artefact

ci/trace_times.py already adds up the traced commands by block. Running it at the end of every ci/run.sh and GitHub job, and saving the top 30 slowest lines as an artefact (or appending them to the job summary), would show where the 45 minutes of step 11 go after each change. A threshold could warn when a single check passes 60 s. This is cheap to build: a few lines in run.sh and the workflow.

*Where:* ci/run.sh, ci/trace_times.py, .github/workflows/build.yml

### C52. An oracle server for the plugin comparisons

Step 11 starts about 575 oracle processes and as many engine processes just to diff `filters --plugin` outputs. A `sieve_ref.py plugin-batch` reading (plugin, length, params) cases from stdin, and a matching `sieve filters --plugin-batch`, would make each loop two processes. The current specs would stay in one table file the C++ tests could read too. This removes the per-process start cost (0.2 s for the oracle, 0.16 s for the engine) and keeps every comparison.

*Where:* reference/sieve_ref.py, tools/sieve_cli.cpp, ci/steps/11-oracle.sh

### C53. Mutation testing of the CI checks

Several checks have passed with their feature broken (the `! grep -q` checks and `book` undefined in earlier reviews, and T9 here). A small tool could apply a set of deliberate breakages to the build: flip a menu key, return early in a filter, or swap the scrambled and positional orders. For each breakage it would run only the steps reach.txt names and record which checks failed. A breakage that every step survives marks a blind spot. It could run occasionally, not on every push, and would cost a mutation list plus a driver around ci/run.sh.

*Where:* ci/run.sh, ci/reach.txt, a new tools/ci_mutants.py
