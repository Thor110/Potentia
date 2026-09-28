# Filter plugins

*Built (28 September 2026): the format with parameters, the table form and the token form, the
engine (judge, count, rank, compact, stacks of plugins), `requires`, the loader and `filters/`
folder, `sieve filters --plugin`, `--plugins` and `--relations`, the oracle's own parser and
engine, and six reference plugins, all checked against the built-in filters they port (section
11), the filter designer (section 12), tagged lists with the first grammar filters, over the
Moby part-of-speech list (section 13), and `sieve-filter-v2` with the melody filters (section 14).*

Filters written as data files and dropped into a folder, so that anyone
(a person, or an AI) can add a filter to the stack without touching the C++ or rebuilding Sieve.
A plugin describes a walk through states, one symbol at a time, and one engine runs every such
file: it judges units, and, because every counting filter in Sieve is already a walk of that
shape (SPECIFICATIONS §8.5, rankers), it also counts, ranks and compacts their survivors exactly,
with no code of the plugin's own. The file's SHA-256 is its identity, so "versioned, never edited"
holds by construction, and the Python oracle runs the same files with its own engine, so every
plugin's counts are checked to the last digit like everything else. Nothing here changes an
existing filter, address or format: the built-in filters stay compiled in, exactly as they are.

## 1. Why data, not code

| | Compiled plugins (DLLs) | Data plugins (this proposal) |
| :--- | :--- | :--- |
| Safety | Arbitrary code on the user's machine | Data only: the engine is the only code |
| Platforms | One build per system | One file, everywhere |
| Exactness | Whatever the plugin does | Integer arithmetic in one engine, as now |
| The oracle | Cannot check what it cannot run | Runs every file with its own engine |
| Versioning | Nothing stops a swapped file | The SHA-256 is the version; a changed file is a new filter |
| Counting and compact | Each plugin would need its own ranker | Every plugin gets one from the engine |

What data cannot express is left compiled in: the entropy and model-information filters are
arithmetic over whole units, not walks, and stay as they are. A judge-only scripting tier (for
format validators and whole-mesh checks, which judge but cannot count) is left for later (§9).

## 2. Where plugins live

- A `filters/` folder beside the program (in an installation, the folder above `tools\`), made by
  the build like `maps/`. The repository's `data/filters/` holds the reference plugins (§7),
  copied there by the build.
- One plugin per file: `<id>-v<version>.sfilter`, plain UTF-8 text with line feeds.
- Loaded when the setup menu opens and when the hallway starts. A file that fails to load is
  listed with the reason, never silently skipped.

## 3. Identity and versions

- A plugin declares an `id` (lower-case letters, digits, hyphens) and a `version` (a positive
  integer). Its name in stacks is `<id>-v<version>`, as the built-in filters' are.
- Its **SHA-256** (of the file's bytes, exactly) pins it. The stack's provenance records it,
  as it records a dictionary's: `plugin:no-double-space-v1{sha256=...}`, so the stack id changes
  if the file does.
- Two files with the same id and version but different bytes are refused, both of them: a
  version is never edited. A changed filter is the next version, beside the old.
- An id may not be a built-in filter's id.
- A plugin names its **origin**: `human`, `ai-directed` (made by an AI under a person's direction)
  or `ai` (made by an AI on its own), and its author. The setup menu shows both.

## 4. The file format, `sieve-filter-v1`

Canonical, line by line: a keyword and its value; `;` starts a comment anywhere outside quotes, and
a line whose first character is `#` is a comment. Two ways of
writing the walk: a **table** (states and transitions, for small rules) and **tokens** (word lists
and which may follow which, for words, word pairs and grammar). Both compile to the same thing.

### 4.1 The header

```
sieve-filter-v1
id        no-double-space
version   1
author    Edward James Gordon
origin    human                  ; human | ai-directed | ai
lines     text                   ; the lines it applies to (§8)
symbols   lower27                ; the alphabet it is written for, by id
describe  No two SPACEs in a row, and at least one letter.
```

`requires` (any number of lines, in the header) names filters that are switched on with this one,
each by its full name, built-in or custom, with any settings it pins:

```
requires  clean-v1
requires  max-run-data-v1  max=3
```

Ticking the plugin ticks what it requires (and what those require), each at its pinned settings;
a prerequisite already ticked keeps its own, and the filters window and `sieve filters` say where
they differ ("tidy-data-v1 requires max-run-data-v1 at max=3; it is set to max=5"). A stack built
from a hand-edited file gets its prerequisites too. `requires` is not `implies`: it says what must be
switched on alongside, not what the plugin's survivors are known to pass. A plugin whose
prerequisite is missing, refused, itself, or pinned at a setting it does not have, is refused.

`symbols` binds the plugin to one alphabet (or palette, or note set): symbols are named in the
alphabet's own terms, and a plugin written for `lower27` is not offered on an `ascii96` line.

### 4.2 Table form

```
class     space   " "
class     letter  a-z

states    4                      ; 0 start, 1 space before any letter, 2 after a letter, 3 space after a letter
start     0
accept    2 3
; from  class   to               ; a missing (state, class) is dead: the unit fails
t   0   space   1
t   0   letter  2
t   1   letter  2
t   2   letter  2
t   2   space   3
t   3   letter  2
end
```

`class` names a set of symbols: `"quoted"` characters, a range `a-z`, a symbol by its digit `@7`, a
range of digits `@0..@9`, or `*` (every symbol in no other class). A symbol belongs to at most one
class; a symbol in no class is dead everywhere, unless a transition names it directly (the symbol
field of `t` takes a class, or symbols written as in a class). Symbols by digit are how plugins for
pictures, melodies and `any` line are written.

**Parameters and loops** (Edward: any value a filter uses should be adjustable, so filters are
quick to make and test). A plugin declares its parameters, and its numbers can be expressions over
them; `for` repeats lines, so a table's size and shape can follow a parameter:

```
param     max  int  3  1  64  longest run of one letter allowed    ; name, kind, default, min, max, text
states    {1 + 26 * max}                                          ; integers, names, or {expressions}
accept    0..{26 * max}
for d 1 26                                                        ; d = 1 .. 26 (none when from > to)
  t   0   @{d}   {1 + (d - 1) * max}
done
```

Expressions are integers with `+ - * / %` and brackets over parameters and `for` variables (`/` and
`%` for non-negative numbers only). Parameters appear in the setup menu and the filter settings like
a built-in filter's, and every value in use is part of the stack's provenance, beside the file's
SHA-256: `max-run-data-v1{plugin sha256=... max=2}`. The file is still the version: a setting is not
an edit. This
example is the built-in `clean-v1`, written as data (checked while drafting: this table gives
exactly `clean-v1`'s survivor counts at lengths 1, 2, 5 and 32, 6106292877396388000207847302280459363471654912 at 32).

### 4.3 Token form

```
tokens    separator " "          ; tokens are runs between separators
edges     whole                  ; whole | cut: cut lets a token touching the unit's start be a
                                 ; suffix of a word, one touching its end a prefix, and one
                                 ; touching both a substring (a page cut from running text)
param     dictionary  dict  default   ; a dictionary setting (a registered id, or default)
set       word    dict:{dictionary}   ; a registered dictionary (dict:ID, or dict:{a dict param})
set       det     list:determiners.txt        ; a word list beside the plugin, pinned by its SHA-256
set       noun    tags:pos.tsv+more.tsv:Np-DP ; a tagged list (word<TAB>tags): the words carrying
                                              ; any of N p and none of D P
follow    det     noun                        ; which set may follow which (none: any order)
follow    noun    verb
first     det noun                            ; the sets the first token may be (default: all)
last      noun verb                           ; the sets the last token may be (default: all)
end
```

As built: one separator between tokens, at most one at each end, at least one token (so `clean`
is part of every token filter, as it is of `words` and `window`). A token cut by an edge may be any
set.

**Tagged lists** (`tags:FILES:TAGS`) are files of `word<TAB>tags`, each tag one character, beside
the plugin; several are read as one when joined with `+`, and each file's SHA-256 is in the
provenance. `TAGS` is the tags a word must carry at least one of, and `TAGS-EXCLUDED` adds the tags
it must carry none of. A word carries the tags of all its lines together: in every file, and in
every spelling that comes to the same word on the line (a line without capitals reads A-Z and the
Latin-1 capitals in lower case, so Moby's "OF", a noun, and "of", a preposition, are one word tagged
`PN`, and `Np-P` leaves it out). Words the line cannot spell, or that hold a separator (phrases), are
left out, as a dictionary's are.

A token must be a word of some set; a unit passes when its tokens chain by `follow` (with no
`follow` lines at all, any order passes, which makes this `words`). A word in several sets can be
any of them: the engine keeps every reading at once and determinises, so nothing is counted twice.
Word pairs are `follow` over sets of words (or single words, for small lists); grammar is `follow`
over part-of-speech sets from a tagged list.

Word lists and tagged lists live beside the plugin or in the registries, each pinned by SHA-256
in the provenance, exactly as dictionaries are now. The first grammar plugins read the Moby
part-of-speech list, which is in the public domain (section 13).

## 5. The engine

- **Compiling.** Both forms become one deterministic automaton over the line's symbols: token
  form by building each set's trie and combining them with the `follow` relation, then the subset
  construction where a word has several readings. The result is **minimised** (Hopcroft), so two
  plugins describing the same rule compile to the same automaton.
- **Judging.** Run the automaton over the unit; it passes if it ends in an accepting state. Linear
  in the unit's length, always available, in every mode.
- **Counting and ranking.** `completions(state, r)`, the number of ways to finish from a state
  with `r` symbols left, is a table built backwards from the accepting states, in exact integers.
  With it and `next(state, symbol)`, the engine is a ranker like the built-in ones, so counting,
  ranking, unranking, compact mode in every ordering and the sieved guided line
  (`sieve-restrict-v1`) all work unchanged.
- **Stacks of plugins.** Two plugins on one line combine into their product automaton (minimised
  again), so a stack of plugins ranks without needing `implies`. A stack mixing plugins with a
  built-in filter ranks when the built-in implies the rest or the plugin's automaton is folded
  into the built-in's walk; otherwise it judges only, and says why, as now.
- **Limits.** The table is `states × length` numbers. Past a budget (a setting, as the unit cache
  is), the plugin judges only (mark, hide, excluded) and the menu says so. Hardware limits, not
  design ones.

## 6. Checking

- `sieve filters --plugin FILE` loads a plugin and prints its header, its compiled size (states
  before and after minimising), its SHA-256 and, where it can count, its survivors and excluded
  units at a length.
- `sieve_ref.py plugin FILE --length L` does the same from its own loader and engine. CI compares
  them for every file in `data/filters/`, at several lengths.
- `sieve check` judges content against plugins as it does against built-in filters, and `sieve
  filters --plugin FILE --judge TEXT` judges each line of a text file as a unit of its own length
  (the oracle's `plugin --judge` prints the same).
- A token-form plugin too large for the oracle to determinise (the Moby grammars) is checked with
  `sieve_ref.py plugin FILE --lazy`: the subsets are made only as a walk reaches them, so counts,
  ranks and verdicts come out exactly, at the lengths the reachable subsets fit in memory.

## 7. Reference plugins

The built-in filters that are walks are ported to data, as templates for anyone writing a plugin
and as the engine's strongest test: each must give **exactly** the built-in's survivor counts at
every length checked, and the same verdict on every unit.

| Plugin | Ports | Form |
| :--- | :--- | :--- |
| `clean-data-v1` | `clean-v1` | table (above) |
| `words-data-v1` | `words-v1` | tokens, one set, whole edges |
| `window-data-v1` | `window-v1` | tokens, one set, cut edges |
| `key-data-v1` | `key-v1` (C major) | table over the note alphabet |
| `moby-grammar-v1`, `moby-grammar-strict-v1` | (new: English word order by part of speech) | tokens, nine sets from tagged lists, whole edges |
| `key-data-v2` | `key-v1`, every scale | table, v2 (a choice parameter) |
| `melody-leap-v1`, `melody-lengths-v1`, `melody-rests-v1`, `melody-ending-v1`, `melody-range-v1` | (new: melody shape) | table, v2 |

They are new filters with new ids (the built-ins keep theirs); a built-in and its port agreeing is
a conformance check, not a replacement.

## 8. Which lines

| Line | v1 |
| :--- | :--- |
| Pages, and a book's title and pages | Judge, count, compact |
| Image, video (by pixel, in address order) | Judge, count, compact (where the states stay small) |
| Audio | Judge, count, compact |
| Binary | Judge; count and compact where the budget allows (files are counted length by length, shortest first, as the line orders them) |
| Models | Later: a model's digits have different ranges (coordinates, then face indices), which the table form would need per-position classes for |

## 9. Not in v1

- **Parameter kinds beyond integers, dictionaries and choices** (a word list, a set of symbols):
  v1 has `int` and `dict`; v2 adds `choice` (section 14).
- **Judge-only scripts.** Filters that must see a whole unit at once (file-format validators,
  watertight meshes) need code; a sandboxed, deterministic tier (WebAssembly, say) could carry
  them later, judging only.
- **Soft models as plugins.** A pinned n-gram model as a data file for the guided ordering is the
  same idea for the soft side, and a natural v2.

## 10. Order of work

1. **Built.** The format, the loader and the table form, with parameters; the engine's judge, count,
   rank and compact; the `filters/` folder; `sieve filters --plugin`. The oracle's loader and
   engine, and CI comparing them. (Ran the full test harness.)
2. **Built.** `clean-data-v1` and `key-data-v1` checked against the built-ins, and `max-run-data-v1`.
3. **Built.** The token form; `words-data-v1` and `window-data-v1`, checked against the built-ins.
4. The setup menu: plugins listed with their origin and author, reasons for any that fail to load.
5. **Built.** Grammar from a tagged list: `moby-grammar-v1` and `moby-grammar-strict-v1` (section 13).
   Word pairs by sets are written the same way, with lists instead of tags.
6. The binary line (judging), then counting where the budget allows.

## 11. As built

- **Core.** `sieve/dfa.hpp`: the automaton, minimised to a canonical form (trimmed to live states,
  merged by Hopcroft's refinement with a sink, numbered breadth first from the start, symbols in
  order), the
  product of two, and `DfaRanker`, a completion table built backwards with each state's transitions
  grouped by target. `sieve/plugin.hpp`: the parser (errors name their line), the compiler (the
  body run with the parameters' values), the plugin filter, and the registry that `find_filter`
  and `filters_for` read after the built-in filters. A stack of plugins ranks through the product
  of their automata. Counting needs a table of states x (length + 1) numbers; past 512 MB a plugin
  judges only, and says so.
- **Loading.** `tools/cli/plugins.*`: every `*.sfilter` in the installation's `filters/` folder
  (the build copies `data/filters/` there; the release ships it) and in `data/filters/` when run
  from the repository, read once at start-up by the command line and the hallway. `sieve filters
  --plugins` lists each file with its name and SHA-256, or why it was refused: malformed, a
  built-in's id, or another file with the same name and version but different bytes (then neither
  is used). The same file found twice is used once.
- **Checking.** `sieve filters --plugin FILE [--length L] [--params k=v,...]` reports the header,
  the states as declared and minimal, the survivors, what is excluded and three survivors by rank.
  The oracle (`sieve_ref.py plugin`) has its own tokenizer, expressions and interpreter, and
  minimises by its own Hopcroft, written separately; the canonical numbering makes the two
  reports identical, and CI diffs them for every reference plugin at five lengths and several
  settings. (Writing it caught a real difference first: Hopcroft on an incomplete automaton merges
  states that differ only in a missing transition, so the oracle completes the automaton with a
  sink before refining.) The unit tests check the ports against the built-ins: `clean-data-v1`'s
  counts are `clean-v1`'s at lengths 1 to 200 and it judges the same; `max-run-data-v1` judges as
  `max-run-v1` at `max` 1 to 4, exhaustively at length 3 and on run-heavy units at 24, and counts
  where the built-in cannot; `key-data-v1` counts as `key-v1` in the major key on all twelve
  tonics; a stack of two plugins ranks exactly; renumbered states compile to the same automaton;
  and ten kinds of mistake are refused with their line.
- **Reference plugins** (`data/filters/`): `clean-data-v1` (table), `max-run-data-v1` (parameter
  `max`, loops; unlike `max-run-v1` it counts, so compact mode works with it), `key-data-v1`
  (parameter `tonic`, symbols by digit on the notes line), `tidy-data-v1` (`requires`),
  `words-data-v1` and `window-data-v1` (the token form, with a `dict` parameter).
- **The token form.** The engine builds the automaton directly: words from every set in one trie
  (each node knowing which sets it ends a word of); between tokens the state is the set of
  readings the last token can have, inside a token the trie node and the readings of the token
  before; with `edges cut`, a first token touching the start runs through a suffix automaton of
  all the words. The oracle builds it another way (an NFA with a state per trie node and reading,
  and for a cut token one per trie node it could be at, then the subset construction and
  Hopcroft), and the two agree state for state: `words-data-v1` (179,305 states made, 14,334
  minimal with scowl-en-35) and `window-data-v1`, and a toy grammar (`tests/plugins/`: four word
  lists, follow, first and last, "run" both noun and verb, whole and cut edges) at four lengths.
  The unit tests find the ports judging exactly as `words-v1` and `window-v1`, exhaustively at
  lengths 1 to 4 and counting the same to 9, and the grammar accepting and refusing the sentences
  it should. Compiled automata are kept for the run (the default dictionary takes about 3 s).
  Counting needs the table to fit (512 MB): a dictionary's automaton counts at page lengths of a
  few hundred characters; past that the plugin judges only, where the built-in `words` still
  counts with its own method. `--relations` finds `words-data-v1` stricter than `window-data-v1`,
  as it should be.
- **Prerequisites and the custom tab** (the same night). `requires` as above (`tidy-data-v1` is the
  example: its own rule is a word-length limit, and it brings `clean-data-v1` and
  `max-run-data-v1` at `max=3` with it). The setup menu's filters window has two tabs, BUILT-IN
  FILTERS and CUSTOM FILTERS, so what is compiled into Sieve and what is a plugin never mix; the
  custom tab shows each plugin's author, origin, file and prerequisites, notes where prerequisites
  are missing or set differently, and lists every refused file with the reason.
- **Duplicates and relations.** `subset(a, b)` (`sieve/dfa.hpp`) decides exactly, over every
  length, whether everything one rule keeps the other keeps too; two minimal automata keep the
  same units exactly when they are equal. `sieve filters --plugin FILE --relations` reports how a
  plugin compares with every other custom filter for its line: the same rule (a duplicate),
  stricter, or looser. The designer runs this on save.
- **Later** (noted): the menu showing plugins' origin and author on their own line, and load errors
  (for now: `sieve filters --plugins`); chunk sizes as a parameter (VAULT.md section 8).

## 12. The filter designer (built; Edward's design)

A node editor for making filters inside Sieve, opened from the **main menu** (not the setup menu,
which is for choosing and tuning filters). It edits `.sfilter` files: the file stays the source of
truth, so a filter made in the designer and one written by hand are the same kind of thing, hashed,
versioned and checked by the oracle alike.

- **The entry node: prerequisites.** The `requires` list, with a **+** to add an entry and, beside
  each listed filter, a **+** that opens a node for the settings it pins.
- **The reference list.** The same filter list as the setup menu's (built-in and custom tabs), with
  the designer's own controls: to see what exists and what each does, and to drag one in as a
  prerequisite.
- **Blocks, not states.** The nodes are the file's building blocks (symbols and classes, parameters,
  rule blocks: a table, or word sets joined by `follow` edges); a drawn state diagram is a viewer
  for automata small enough to read (max-run at 64 has 1,665 states).
- **Live testing.** Survivors at a chosen length, survivors pulled out by rank, and a box to type
  text into and see pass or fail, with the node that rejected it.
- **On save:** the relation check against every filter for the line, so duplicates are caught
  before they are made ("the same rule as clean-data-v1"), and stricter or looser rules are named.
- **Relations between filters** beyond `requires` (Edward): a filter could require another's
  opposite, for example; each such relation is an exact operation on the automata.
- **First use:** the Moby grammar filters were written by hand, as planned (Edward: the designer is
  for others' filters; these were chosen from data by a search). They open in the designer, which
  tests them from where their tagged lists are.

### As built

- **Main menu > Filter Designer** (`client/designer.*`, the screen; `client/designer_model.*`, the
  filter as parts, its file, testing and saving). Three panels: the filter list (built-in and
  custom tabs, the chosen one described; R requires it, Enter opens a custom one here); the canvas
  of nodes; the test panel.
- **Nodes.** ENTRY: REQUIRES, each prerequisite with a **+** that opens a SETTINGS node for what it
  pins (Left/Right steps a number or cycles a choice; Delete unpins, back to "its own") and **+ add a
  filter it requires** (then choose in the list). HEADER (id, version, author, origin, lines,
  symbols, describe, form). PARAMETERS (name, kind int or dict, default, min, max, about; **+ add**).
  RULE: WORD SETS (separator, edges, **+ add a word set**) and a SET node per set (name, its words:
  a dictionary, a dict parameter or a word list, may start, may end, **edit its words** for a list,
  **link** to draw a `follow` arrow to another set; arrows are drawn between the nodes, and Delete
  on an arrow's row removes it). RULE: TABLE, whose lines open in a multi-line editor. Nodes are
  placed in columns and move when dragged by their titles; the canvas pans with a right-drag or
  Ctrl+arrows. Where each node sits is kept in the file as comments the parser skips.
- **The test panel**, on a worker (a dictionary takes seconds; the window never waits): states made
  and minimal, survivors and what is excluded at a length (Left/Right, Shift x10), five survivors by
  rank, text typed in and judged, with why it fails ("the token 'on' (characters 13 to 14) is not
  the start of a word that can come there"; a table's errors by the editor's own line numbers), the
  relation check, and Save.
- **Tested when asked** (Edward): opening the designer or changing the filter compiles nothing.
  F5 or TEST NOW compiles and counts; Save tests first if the filter changed since its last test,
  and saves only a filter that compiles; the relation check and judging typed text test first when
  needed. Results from before a change stay on show, marked as from before it.
- **The progress window** (Edward): while the worker compiles, tests or checks relations for more
  than a fifth of a second, a window in the middle of the screen says what it is doing and for how
  long ("building the automaton from 88986 words (209178 trie nodes)", "minimising 418357
  states", "counting the survivors at length 32", "comparing with window-data-v1 (3 of 5)"), then
  for two seconds how it went. It never blocks: Esc hides it and the work carries on, and a Save
  asked for meanwhile happens when the worker is free. The steps come from the compiler itself
  (`compile_plugin`'s `step`). Measured (release build, the default dictionary, cold): reading
  0.27 s, building 0.23 s, minimising 0.43 s (Hopcroft's algorithm on flat arrays, which replaced
  a first Moore refinement that took 0.95 s and is far slower in a debug build), counting 0.1 s: about
  1 s in all, then instant from the cache. `--timings` records `designer.test` and
  `designer.relations`.
- **Saving** (Ctrl+S) writes `<id>-v<version>.sfilter` and its word lists to the filters folder and
  registers it at once (it is in the CUSTOM FILTERS tab straight away), then runs the relation
  check, so a duplicate is named as soon as it is made. A version that exists with other contents
  is never overwritten: Save offers the next free version. A word list that exists with other words
  is never changed underneath the filters using it: the new one needs a new name.
- **A round trip** (CI): a plugin opened in the designer and written back is the same rule. The
  designer writes its own layout, so a hand-written file's comments are not kept (its rule is).
- **Scripted** (for CI and documentation): `hallway --designer [--design FILE] [--script
  "keys,=typed text,..."] [--design-out FILE] --screenshot PNG` prints the filter's name and the
  status line. CI makes and saves a word filter by script, finds it registered and named the same
  rule as `words-data-v1`, and finds a second save of other contents offered version 2.
- **Tagged lists in the designer.** A set's words may be `tags:FILES:TAGS`, written in the file (a
  set's words choice cycles through the tagged sources the filter already reads, as well as
  dictionaries and lists; typing a new one in the designer is for later). The lists are large and not edited here, so they are read where the opened file is (copied
  into the scratch folder once, kept while unchanged), and Save copies them beside the saved filter
  when they are not there yet; one of the same name with other contents stops the save. The Moby
  grammar tests in the designer in about 8 s (release build; the progress window names each step).

## 13. The Moby grammar filters (built)

Two plugins judge English word order by part of speech: `moby-grammar-v1` (lenient) and
`moby-grammar-strict-v1`. Their data and how it was made are in `data/filters/moby-pos-v1.md`;
in short:

- **The lists.** `moby-pos-v1.tsv` is Moby Part-of-Speech II (Grady Ward, public domain by his
  grant of January 2001), every entry, converted to UTF-8 `word<TAB>codes` by `tools/moby_pos.py`,
  which prints the source's and the output's SHA-256. Moby lacks regular inflections, so
  `moby-inflections-v1.tsv` adds the plurals, verb forms, comparatives and -ly adverbs that the rules
  of English make from its words and that SCOWL's largest list confirms (73,070). Moby lacks most
  names, so `moby-names-v1.tsv` adds SCOWL's names that Moby lacks, as nouns (16,052).
- **The sets.** det `DI`, noun `Np-DIPCro`, pron `ro`, verb `Vti-DIPCro`, adj `A-DIPCro`, adv
  `v-DI`, prep `P`, conj `C`, interj `!`. The exclusions stop function words doubling as content
  words (Moby gives "the" an adverb reading and "a" a noun one, which would let almost any order
  through).
- **The rule** (`follow`, `first`, `last`) was chosen by a search, not by hand. It started from every
  pairing allowed and removed pairings one at a time while real sentences lost less than shuffled
  ones did (strict), or less than half as much (lenient). It was tuned on three NLTK Gutenberg books
  and measured on four others it never saw. On 2,790 held-out sentences of 4 to 14 words, all in the
  lists:

  | Plugin | Real pass | Shuffled pass | Random words pass | Minimal states |
  | :--- | ---: | ---: | ---: | ---: |
  | `moby-grammar-v1` | 83.5% | 42.8% | 41.4% | 287,267 |
  | `moby-grammar-strict-v1` | 58.8% | 18.4% | 17.1% | 215,179 |

  Both are sieves for word salad, not parsers: a bigram grammar over parts of speech has a ceiling.
  Neither lets a sentence start with a verb, so imperatives fail. A word in no list fails its
  sentence.
- **Cost.** 258,024 distinct words. Compiling takes about 6 s cold in a release build (reading 0.4 s,
  building 0.3 s, minimising 1.6 million states 2.4 s, counting at length 32 about 3 s), then instant
  from the cache. Both count at page lengths within the 512 MB table budget.
- **Checked.** The unit tests judge sentences with both (real order passes, the same words shuffled
  fail, "quickly the of" fails because "of" is no noun, a strict sentence may not start with a verb).
  `tests/plugins/toy-tags-v1` and `toy-tags-cut-v1` test tagged lists against the oracle in CI: two
  files as one, an excluded tag, capitals folded, a word judged by the tags of all its lines, and
  lines skipped.
- **Against the oracle, at full size.** The oracle's NFA keeps each token's reading apart, so its
  subset construction for the Moby lists runs past ten million states and does not fit in 8 GB (the
  engine's direct construction makes 1.6 million). Rather than build the oracle the engine's way,
  its lazy mode makes subsets only as they are reached. With every word of the lists, the two agree
  exactly at length 8 (282 billion units: 399,429,320 survivors lenient, 15,477,561 strict), on the
  three ranked survivors, and on 200 held-out sentences (`tests/plugins/moby-sentences.txt`, each
  real sentence followed by the same words shuffled: lenient 82 real and 37 shuffled pass, strict
  67 and 17). At length 9 the reachable subsets outgrow 8 GB. CI runs the same comparison at
  length 6 (about 20 s each).

## 14. `sieve-filter-v2` and the melody filters (built)

A melody filter needs to ask "is this note within four semitones of the last one?", which v1's
arithmetic cannot say. v1 is a version and is never edited, so the table form gained what it
needed as `sieve-filter-v2`: a file says which it is on its first line, and a v1 file reads exactly
as before (what v2 adds is refused in it, by name). The designer reads and writes both, and
upgrades a filter to v2 when it uses something only v2 has.

- **Expressions.** Comparisons (`== != < <= > >=`, giving 1 or 0), `&&`, `||` and `!` (0 is false,
  anything else true), and `min(a, b)`, `max(a, b)`, `abs(a)`. Loosest first: `||`, `&&`, the
  comparisons, `+ -`, `* / %`, then unary `-` and `!`. Every operand is worked out, whatever the
  other side says, so a mistake is never hidden by a short cut.
- **`if {EXPR}` … `else` … `fi`**, around any lines of the table, nesting with `for` and with each
  other.
- **`param NAME choice DEFAULT A,B,C [text]`**: one of a list, by name in the settings and the
  provenance, by its place in the list (from 0) in expressions. The menus cycle through it.
- **The line's constants:** `BASE` (its number of symbols), and on a note line `PITCHES` (25 on
  `notes104`), `DURATIONS` (4) and `LOW` (60, the MIDI number of pitch 1): a note's symbol is
  `pitch * DURATIONS + duration`, pitch 0 the rest. A parameter or `for` variable may not take
  these names, nor `min`, `max` or `abs`.
- **Families of symbols:** `symbols notes*` applies to every line whose symbols' id begins `notes`:
  `notes104` and every `notes2` set (SPECIFICATIONS §3.2), with the constants taken from the set
  (a `notes2` set's own `PITCHES`, `DURATIONS` and `LOW`). On a line of several voices the stack
  hands a plugin one voice at a time, so a plugin is always written for one line of events;
  `sieve filters --plugin` reports it the same way (one voice's automaton; the count to the power
  of the voices), and so does the oracle. `key-data-v2`, `melody-leap-v1`, `melody-rests-v1` and
  `melody-range-v1` are checked against the oracle on `notes2` sets of other ranges and durations
  with one to three voices (CI). `melody-lengths-v1` and `melody-ending-v1` name `notes104`'s four
  durations in their choices, so they are for `notes104` only; `notes2` versions would name its
  eight.

The engine and the oracle each have their own reading of it. The oracle's comparison with the
engine caught a real error in the oracle's first `if` (a variable reused for the block's end cut
the loop short) before anything shipped, and the unit tests caught the engine refusing a choice
parameter written without a description. A toy of the grammar (`tests/plugins/toy-v2-v1`) is
checked against brute force at lengths 1 to 3 and against the oracle in CI.

**The melody filters** (`data/filters`, all counting, so any stack of them ranks through the
product of their automata):

| Plugin | Rule | Parameters |
| :--- | :--- | :--- |
| `key-data-v2` | every note in the scale on the tonic (as `key-v1`); rests pass | tonic, scale (choices) |
| `melody-leap-v1` | no two neighbouring notes further apart than `leap` semitones; rests passed over, or a fresh start | leap (1–24), rest_resets |
| `melody-lengths-v1` | notes and rests from `shortest` to `longest`, at most `eighths` eighths in a row (`notes104`) | shortest, longest, eighths |
| `melody-rests-v1` | at most `run` rests in a row and `total` in all, none first unless `leading` | run, total, leading |
| `melody-ending-v1` | the last event is the tonic, held at least `hold` (`notes104`) | tonic, hold |
| `melody-range-v1` | every note between `low` and `high` (MIDI numbers) | low, high |

Checked: each against the oracle at lengths 1, 2, 5 and 17 and several settings (CI); counts by
hand where they have a closed form (C major leaves 15 of the 25 pitches, 64 symbols, so 64^17 at
length 17; no rests at all, 100^17); melodies in notation judged in the unit tests; and a stack of
three (key, leap 2, ending) counted exactly against all 104^3 units at length 3, and ranked and
unranked through every survivor.

**The music player's defaults** (new installs; a saved `sieve-music.ini` keeps its own) are stacks
of these, so a track is drawn by rank: WORLD in C major, leaps of 4 at most, quarter to whole notes
with no eighths, a rest at a time and four in all, ending on C held at least a half note, between
C4 and G5 (about 1.5 × 10^38 tracks at 32 notes); MENUS in A minor, leaps of 5, eighths to halves
with two eighths in a row at most, ending on A, C4 to E5 (about 4 × 10^40). Without the plugins (a
filters folder not found) the player falls back to `key-v1`.


## 15. One line filtered by another (built)

Edward's idea: every line could set aside what really belongs on another. A page that is a PNG
written out in hex is a file, not a page, and the binary line already holds that file. Two
built-in filters do this, and both count exactly.

**Exclusion and requirement.** A line can use another in two ways, and they are very different
in size:

- **Exclusion** ("not something that belongs elsewhere"): what another line counts as its own is
  a tiny share of this line, so taking all of it away removes only that share. If the stack keeps
  `P` units and the units that are some other line's content are `W`, the stack with exclusion
  keeps `|P| − |P ∩ W|`. `W` is far smaller than `P`, so the count hardly moves. Taking away
  "100% of six other lines" takes away six slivers, not six hundred percent. Exclusion is for
  **cleanliness**: each line keeps only what is its own.
- **Requirement** ("must be valid there as well"): every requirement multiplies the share kept,
  `|P ∩ Q| ≈ |P| · |Q| / N` for rules that have nothing to do with each other, which is how a
  stack of a line's own filters removes so much. Between two lines' rules the product is usually
  empty rather than distilled.

The powers of ten come off through each line's own filters; exclusion keeps the lines clean.
`sieve filters` now prints both the kept and the set-aside shares as powers of ten of the line,
so any stack can be measured this way.

**`not-written-v1`** (text lines, and the books' title and pages; `core/src/written.cpp`) fails a
unit that some reading of it is a file whose first bytes carry a signature (`file-kinds-v1`,
below). The readings are the vault's decoders less ascii85 (SPECIFICATIONS §8.5), each over the
unit's whole text: its own bytes (as it is, trimmed, and trimmed with a line feed), hex, base64,
base32, decimal, the letters a–p, spelled-out digits, and any two symbols as bits, both ways
round. A parameter, `readings`, checks all of them or just one. On a line of every byte
(`bytes256`) a unit is read only as its own bytes.

What it sets aside on the pages line at 32 symbols (exact; the share is display only):

| Reading | `lower27` | `ascii95` | What it is on `lower27` |
| :--- | :--- | :--- | :--- |
| all | 10^-4.23 | 10^-3.65 | |
| text | 10^-5.73 | 10^-3.65 | "ftyp" at the fifth letter makes an MP4 (on `ascii95`: "MZ" or "BM" first) |
| hex | none | 10^-21.58 | no digits, and a–f alone never spell a signature |
| base64 | none | 10^-9.41 | |
| base32 | 10^-4.24 | 10^-10.91 | any run of letters is base32: one page in about 17,000 is an EXE, BMP or GZ |
| decimal | none | 10^-28.01 | |
| nibbles | 10^-11.06 | 10^-19.28 | "enfk" is 4d 5a, "MZ" |
| spelled | 10^-35.93 | 10^-39.15 | |
| binary | 10^-32.99 | 10^-49.35 | two letters as bits |

So on the default pages line, about one page in 17,000 is a file written out. That is the whole
effect of taking another line's content away. The largest part is base32, which reads any letters
at all as data, so 2-byte signatures turn up by chance.

**How it counts.** Each reading but binary is a small machine over the text: the decoder's own
state, and the signature matcher's (after `p` decoded bytes, which signatures still fit). The
matcher is decided after at most 14 bytes, and a reading whose matcher has failed is dead, so the
machines stay small. Walked over the alphabet's symbols, each is an automaton; their union `O` is
minimised (2,590 states on `lower27`, 9,350 on `ascii95`). The binary reading cannot be one
automaton of that kind, because it has to remember which two symbols it has seen as well as the
bits so far (on `lower27` that would be millions of states). But which two symbols they are
changes nothing except which symbols it accepts next. So it is walked over what a symbol is to it
(the first symbol, the other, or whitespace), with each class's number of symbols, as an automaton
`Bn` of 872 states. The units a stack keeps, with `P` its plugins' automaton (every unit when
there are none), are
```
kept = |P| − |P ∩ O| − |P ∩ Bn| + |P ∩ O ∩ Bn|
```
The last two terms are walked with the two symbols named, and only while both automata are alive.
Once both symbols are known, only they and whitespace can follow, so the automaton from there is
cut down to those few symbols and minimised; pairs that give the same small automaton share their
counts. `|Bn|` from a state comes from its own table. Ranking and unranking follow from the
counts, as for every ranker. The whole thing is exact, and it combines with plugins (a stack of
plugins and `not-written-v1` counts and compacts). With other built-in filters it judges only,
like any two rankers that do not imply each other. On an alphabet whose symbols take more than one
byte of UTF-8, with the binary reading chosen, it judges only; the verdicts are the same.

**Checked:**
- Every unit of `lower27` at length 4, `ascii95` at 3 and `bytes256` at 2 (unit tests), and in
  development every unit of 3-symbol alphabets at length 16 (43 million units each, including
  {space, M, Z}, where readings overlap). Three things agree: the automata's verdict, the readings
  decoded outright, and the count.
- Survivors round-trip through rank and unrank at length 32 on three alphabets.
- The oracle counts independently. `O` is its own Python machines, determinised and minimised.
  `|Bn|` is in closed form: `Σ C(L, 8m) · w^(L−8m) · K(K−1) · S(m)`, where `S(m)` is the number of
  byte strings of `m` bytes, the first below 80, that are signed or have a signed complement. The
  overlap is walked pair by pair. 50 counts, including stacks with `max-run-data-v1` and
  `clean-data-v1`, and 535 verdicts are in `tests/vectors_written_v1.tsv`, and CI regenerates them.

**`binary-kind-v1`** (the binary line; `core/src/filekind.cpp`) keeps the files of chosen kinds
(`kinds`: signed, text, signed-or-text, unknown, empty, any, or one kind; `keep`: keep or exclude).
A file's kind is decided by its first 16 bytes and its size (`file-kinds-v1`: the hallway's label
table, with MID added), so the survivors are counted and ranked exactly on a binary line of any
length. A head of up to 16 bytes is walked byte by byte through an automaton of which signatures
still fit and whether every byte so far is readable; a file of `L > 16` bytes is a head of 16 and
any `L − 16` bytes after it. Every kind's share of the line (`sieve filters --line binary`),
at 32 bytes:

| Kind | Share | Kind | Share |
| :--- | :--- | :--- | :--- |
| ? | 10^-0.06 | ZIP | 10^-9.33 |
| TXT | 10^-0.87 (about 1 in 7) | MP4, ELF, PDF, RAR, OGG, FLAC, MID | 10^-9.63 each |
| EXE, BMP, GZ | 10^-4.82 each | GIF | 10^-14.15 |
| JPG, MP3, BZ2 | 10^-7.22 each | PNG | 10^-19.27 |

This is why a cluster of TXT files turns up easily on the binary line. A file reads as TXT when
all of its first 16 bytes are readable, 226 of the 256 byte values, which is 0.883^16 ≈ 1 in 7.
Neighbouring files share their first bytes, which are the top digits of the address, so they share
their label too. Checked against every file of up to 2 bytes (unit tests), and against the
oracle's own head automaton: counts at ten lengths up to 300 bytes, files by rank, and the compact
scrambled order (`tests/vectors_kinds_v1.tsv`, 583 rows).

**Between the two lines, J.** In the hallway, J on an item in hand jumps between it and its file:
from any line, to the item's file (as F saves it, a picture at one pixel a pixel) on the binary
line; from the binary line, a file of a kind a line holds (TXT, PNG, JPG, GIF, BMP, MID, BOOK) to
that line, fitted to it as T fits what is warped in. A melody's MIDI file comes back as the same
music (`midi_to_notation`, checked on 300 melodies across five note sets).

## 16. Every line filtered by every other (built)

§15 took the binary line's content away from the pages. Edward's table takes each line's content
away from every other line, in the same spirit: exclusion, for cleanliness. Four filters do it.

| Line | Filter | Sets aside |
| :--- | :--- | :--- |
| pages, books' text | `not-written-v1` (§15) | files written out as text |
| pages | `not-other-line-v1` | melody notation, and a model's `.obj` text |
| image, video | `not-packed-v1` | pixels which, packed as bits into bytes, are a file with a signature |
| every line but binary | `not-a-file-v1` | a unit whose own number, as a place on the binary line, holds a file with a signature |
| binary | `not-an-item-v1` | a file that is exactly an item of another line, as F saves it |

A line made of other lines' parts is not asked to exclude its parts: a book holds pages and a
picture, and a video is frames, by design.

**`not-a-file-v1`** reads the unit as the number it is (its symbols as base-`B` digits, as the
positional address does) and asks what file stands at that place on the binary line. The file's
kind comes from its first bytes and its length alone (`file-kinds-v1`), and both come from the top
of the number, so the file is never written out. With `E(x)` the signed files at binary places
below `x` (`KindCounter::count_before`: whole lengths, then heads below this head, then this
head's tails), the survivors below `x` are `x − E(x)`. Every prefix of a unit is a range of
numbers, so its completions are `S(hi) − S(lo)`, and ranking walks those; unranking searches
for the number. Exact at any length.

**`not-other-line-v1`** (text lines whose symbols are not `bytes256`) has two forms, `notes` and
`obj`, in a `forms` parameter. Melody notation is what `canon-notes-v1` and `-v2` read: notes A–G
with `#` or `b` and an octave digit, rests `R`, the durations `s e e. q q. h h. w` or none, `//`
between voices, apart by whitespace, `|` or `,`, with at least one note or rest. A model's text is
`v` lines of three numbers and `f` lines of three indices from 1, apart by line feeds, at least one
of each, with padding spaces only at the end. Each is a small machine walked into an automaton
over the alphabet; the filter keeps its complement, so it counts, ranks, and combines with plugins
and `not-written-v1` like any automaton.

**`not-packed-v1`** (image and video lines of 2, 4, 16 or 256 colours: `b` bits a pixel) packs the
unit's pixels, first bit highest, into `⌊L·b/8⌋` bytes, and fails the unit if those bytes are a file
with a signature. Packing is one-to-one, so this is `binary-kind-v1`'s head automaton walked bit by
bit: an automaton, counted and ranked exactly.

**`not-an-item-v1`** (binary) fails a file that is exactly another line's item as F saves it
(`items`: pages by default, or melodies, pictures, models, or all):
- **pages**: exactly the page's text, as F now saves it on every alphabet. When each symbol
  is one byte, the pages are a pattern of allowed bytes per position, and the survivors are counted
  exactly: the kind survivors less the pages among them, `|K| − |K ∩ P|`, and ranked by the same
  subtraction, walking the pattern through the head automaton.
- **melodies**: a MIDI file that reads back (`midi_to_notation`) as a melody of the audio line
  whose MIDI is that file, byte for byte;
- **pictures**: a PNG that decodes to a picture (or a video's sheet of frames) of the line which
  saves as that PNG, byte for byte;
- **models**: an `.obj` text that the models line saves as itself.
Those three are judged file by file, so with them the binary line cannot be counted or compacted
(it falls back to hide and says why).

**The models line's stack** (`core/src/modelsieve.cpp`, `[models]` in the settings). A model is
not a run of symbols of one base: it is a mixed-radix number (coordinates in base `C`, then face
indices in base `V`), and its positional index is that number. So, like the binary line, it has a
stack of its own (`ModelSieve`) that judges a model by its index. It holds `not-a-file-v1`, the
same rule as on every other line, worked on the index itself: survivors below `x` are
`x − E(x)`, a survivor's number is `S(index)`, and the k-th survivor is found by halving. It counts
exactly, and the models line compacts (positional, or scrambled by `shuffle-sha256-v1` over the
survivors, keyed with the line's key and the stack's id), its titles blank as a compact line's are.
The other lines' content a model could be is already handled from their side: `.obj` text on the
pages (`not-other-line-v1`) and a model's `.obj` file on the binary line (`not-an-item-v1`). The
models line's own filters join the same stack (below).

**The models line's own filters** (`core/src/filters/models.cpp`; SPECIFICATIONS §12's first two
tiers). These are requirement filters, not exclusion: they remove most of the line.

| Filter | Keeps | Counted as | Kept, 8 vertices, 12 faces, grid 16 |
| :--- | :--- | :--- | :--- |
| `distinct-vertices-v1` | no two vertices at the same point | `P(P−1)…(P−V+1)`, `P = C³` | about all (sets aside 10^-2.17) |
| `distinct-indices-v1` | no face names a vertex twice | each face `V(V−1)(V−2)` ways, not `V³` | 10^-2.20 |
| `every-vertex-used-v1` | every vertex named by a face | `Σⱼ (−1)ʲ C(V,j) · (face ways with V−j)^F` | 10^-0.03 (sets aside 10^-1.19) |
| all three | | the product of the parts | 10^-2.21 |

The rules on the vertices and on the faces are separate, so the survivors are the vertex strings
kept times the face strings kept, and a survivor's number is its vertices' rank times the face
strings kept, plus its faces' rank. That is exactly the positional order of the survivors, since a
model's vertices are the top digits of its number. Each part is ranked digit by digit: below each
digit, the completions of every smaller one. For the vertices those depend only on how many points
are taken. For the faces they depend on how many vertices are still unused, which vertices the
current face has named, and how many faces are left, and inclusion and exclusion gives them in
closed form, however many faces there are. Ranking walks `3F` digits and tries up to `V` at each,
so past a budget (`3F · V · (V+1)` at 4 million with every-vertex-used) a stack still counts but
judges only.

Ticked with `not-a-file-v1`, the stack judges but cannot count: not-a-file is arithmetic on the
whole number, the rules walk its digits, and counting the two together is the open problem of the
text lines too (see the note under "Together" above).

**What they set aside** (exact; the shares are display only; hallway defaults):

| Line | Filter | Set aside |
| :--- | :--- | :--- |
| pages, `lower27` 32 | `not-a-file-v1` | 10^-4.38 |
| pages, `lower27` or `babel29` | `not-other-line-v1` | none (no digits, so no notes and no `.obj`) |
| pages, `ascii95` 32 | `not-other-line-v1` | 10^-32.93 |
| image, black and white 10×10 | `not-packed-v1` | 10^-4.34 |
| image, black and white 10×10 | `not-a-file-v1` | 10^-5.54 |
| audio, 16 notes | `not-a-file-v1` | 10^-5.30 |
| video, 5×5×8 | `not-packed-v1` | 10^-4.34 |
| models, 8 vertices, 12 faces, grid 16 | `not-a-file-v1` | 10^-5.54 |
| binary, 32 bytes, pages `lower27` 32 | `not-an-item-v1` (pages) | 10^-31.26 |
| binary, 33 bytes, pages `lower27` 32 | `not-an-item-v1` (pages) | 10^-33.67 |

F once added a line feed to a page saved from an alphabet without one; it no longer does (a page
and its file are one to one, and addresses are unchanged: a page's comes from its symbols, a file's
from its bytes). `not-an-item-v1` was corrected with it before any release. The 2-byte signatures
(`MZ`, `BM`, `1F 8B`) are most of every share, as in §15.

**Together.** Each filter counts on its own. `not-other-line-v1` and `not-packed-v1` are automata,
so they combine with plugins, with each other and with `not-written-v1`. `not-a-file-v1` is counted
by arithmetic on the unit's number, not by an automaton over its symbols, so ticked with another
ranking filter the stack judges only (hide, not compact). Counting both together would need the
other filter's survivors inside each of the binary line's signed ranges, which are ranges of a
base-256 number against a base-`B` unit: not built.

**Checked:**
- The oracle (`reference/sieve_ref.py cross-vectors`, `tests/vectors_cross_v1.tsv`, 115 rows, about
  10 seconds) has its own implementations: `E(x)` from its own head automaton, the notation and
  `.obj` machines (checked against its own judges on 30,000 random `.obj`-like texts and every text
  of a small alphabet to length 6), the packed count in closed form (packing is a bijection), and
  the pages among the kind survivors walked through its head automaton. Every row matches.
- Every unit of short lines (unit tests): `not-a-file-v1` against `file_kind_at` on base 2 at 16,
  27 at 3 and 256 at 2, with rank and unrank; `not-packed-v1` against `packed_as` at three sizes;
  and the binary line's survivors with pages excluded, in order, none of them a page; every model
  of a 3-vertex, 1-face, 2-step shape (13,824) against `file_kind_at`, through both compact orders.
- The models line in the oracle: its counts at five shapes (two by brute force) and survivors by
  rank at three, from its own `signed_before`. The line's own rules: every model of the 3-vertex,
  2-face, 2-step shape (373,248) by brute force for all seven combinations, then counts at three
  larger shapes and survivors by rank, from the oracle's own walk over the faces (its state the set
  of vertices used and the current face's named vertices), a different method from the engine's
  closed forms. 218 rows in all. The unit tests also walk every model of the 3/1/2 shape through
  both compact orders with all three rules.
