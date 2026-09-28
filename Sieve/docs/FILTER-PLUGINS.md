# Filter plugins (draft for review)

A proposal, not yet built: filters written as data files and dropped into a folder, so that anyone
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

Canonical, line by line: a keyword and its value, `;` or `#` starting a comment. Two ways of
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

`class` names a set of symbols (ranges and single symbols in the alphabet's terms); every symbol
belongs to at most one class used in the table, and a symbol in no class is dead everywhere. This
example is the built-in `clean-v1`, written as data (checked while drafting: this table gives
exactly `clean-v1`'s survivor counts at lengths 1, 2, 5 and 32, 6106292877396388000207847302280459363471654912 at 32).

### 4.3 Token form

```
tokens    separator " "          ; tokens are runs between separators
edges     whole                  ; whole | cut: cut lets the first and last token be a suffix
                                 ; and a prefix of a word (a page cut from running text)
set       word    dict:scowl-en-60-names      ; a registered dictionary, pinned by its SHA-256
set       det     list:determiners.txt        ; a word list beside the plugin, pinned the same way
set       noun    tags:moby-pos.tsv:N         ; words carrying a tag in a pinned tagged list
follow    det     noun                        ; which set may follow which (any set may start
follow    noun    verb                        ; and end, unless start/end lines say otherwise)
start     det noun
end
```

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

- **Parameters.** A variant is a new file; a plugin has no settings of its own. (Simple and exact;
  parameters can come in a later version of the format.)
- **Judge-only scripts.** Filters that must see a whole unit at once (file-format validators,
  watertight meshes) need code; a sandboxed, deterministic tier (WebAssembly, say) could carry
  them later, judging only.
- **Soft models as plugins.** A pinned n-gram model as a data file for the guided ordering is the
  same idea for the soft side, and a natural v2.

## 10. Order of work

1. The format, the loader and the table form; the engine's judge, count, rank and compact; the
   `filters/` folder; `sieve filters --plugin`. The oracle's loader and engine, and CI comparing
   them. **This needs the full test harness** (a new ranker in the core, and new oracle vectors).
2. `clean-data-v1` and `key-data-v1`, checked against the built-ins.
3. The token form; `words-data-v1` and `window-data-v1`, checked against the built-ins.
4. The setup menu: plugins listed with their origin and author, reasons for any that fail to load.
5. The first new filters: word pairs (by sets), then grammar (a tagged list first).
6. The binary line (judging), then counting where the budget allows.
