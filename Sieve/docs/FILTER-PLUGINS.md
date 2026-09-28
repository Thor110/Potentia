# Filter plugins

*Built (28 September 2026): the format with parameters, the table form and the token form, the
engine (judge, count, rank, compact, stacks of plugins), `requires`, the loader and `filters/`
folder, `sieve filters --plugin`, `--plugins` and `--relations`, the oracle's own parser and
engine, and six reference plugins, all checked against the built-in filters they port (section
11), and the filter designer (section 12). Next: the Moby grammar filter.*

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
follow    det     noun                        ; which set may follow which (none: any order)
follow    noun    verb
first     det noun                            ; the sets the first token may be (default: all)
last      noun verb                           ; the sets the last token may be (default: all)
end
```

As built: one separator between tokens, at most one at each end, at least one token (so `clean`
is part of every token filter, as it is of `words` and `window`). A token cut by an edge may be any
set. The Moby tagged list (`tags:`) comes with the grammar filter.

A token must be a word of some set; a unit passes when its tokens chain by `follow` (with no
`follow` lines at all, any order passes, which makes this `words`). A word in several sets can be
any of them: the engine keeps every reading at once and determinises, so nothing is counted twice.
Word pairs are `follow` over sets of words (or single words, for small lists); grammar is `follow`
over part-of-speech sets from a tagged list.

Word lists and tagged lists live beside the plugin or in the registries, each pinned by SHA-256
in the provenance, exactly as dictionaries are now. A tagged English list in the public domain
(the Moby part-of-speech list is one) would be the first grammar plugin's data.

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
- `sieve check` judges content against plugins as it does against built-in filters.

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

- **Parameter kinds beyond integers and dictionaries** (a word list, a set of symbols, a choice):
  v1 has `int` and `dict`.
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
5. The first new filters: word pairs (by sets), then grammar (a tagged list first).
6. The binary line (judging), then counting where the budget allows.

## 11. As built

- **Core.** `sieve/dfa.hpp`: the automaton, minimised to a canonical form (trimmed to live states,
  merged by Moore's refinement, numbered breadth first from the start, symbols in order), the
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
  minimises by Hopcroft's algorithm rather than Moore's; the canonical numbering makes the two
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
- **Later** (noted): the Moby part-of-speech list for the first grammar plugin, with or after the
  token form; the menu showing plugins' origin and author on their own line, and load errors
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
- **First use:** the Moby part-of-speech list and the first grammar filter, built in the designer
  on the token form.

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
