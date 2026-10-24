# Step 3: the long-open filter defects, IDEAS §2.1 to §2.3 (24 October 2026)

## §2.3: short gibberish through the -names lists
The default list, `scowl-en-60-names`, lets SCOWL's chemical symbols, units and acronyms through
when it's read in lower case: "zn", "tb", "hz", "ph", "ibm". SCOWL writes the symbols in mixed
case ("Zn"), so dropping all-caps tokens alone (the fix IDEAS proposed) would have missed them.

**New lists, `scowl-en-35-names-v2`, `-60-names-v2` and `-80-names-v2`**, are made from each
pinned `-names` file. Each leaves out:
- every entry of two or more letters written all in capitals: OK, IBM, NASCAR (45 in the 60 list);
- every entry of one or two letters whose lower case isn't in the plain list of the same size: Zn,
  Tb, Hz, pH, Ed, Jo (59 in the 60 list);
- but never "a" or "I".

Names of three letters or more stay (Amy, Dickens). The old lists are unchanged.
`tools/build_dictionary.py --names-v2 NAMES --plain PLAIN --out FILE` makes them, with no SCOWL
needed.

Measured with `sieve sift`:
| Pages of | `scowl-en-60-names` | `-names-v2` | `scowl-en-60` (no names) |
| ---: | ---: | ---: | ---: |
| 2 letters, all words | 134 | 73 | 73 |
| 64 letters, all words | 10^-37.03 of the space | 10^-38.49 (28 times fewer) | 10^-39.39 |

In Don Quixote (`pg996.txt`), no whole word was lost. The only tokens lost were pieces of accented
words split up by my quick count ("Señor" counted as "se"), not words Sieve would see.

**A decision for you:** they're registered, but not the default. Making `scowl-en-60-names-v2` the
default would change every result made with the default dictionary (stack ids record its hash,
and the docs' figures were measured with it). `function-words-v1` also names the old list's hash.
Shall I switch it?

## §2.1 and §2.2: `symbol-entropy-v2` and `model-information-v2`
- **The entropy against each alphabet's own most:** v2's bounds are thousandths of the most a unit
  of the line can have, log2 min(symbols, length). v1's millibits only suited 27 letters. Measured
  on Don Quixote in lower27 pages of 1,000 letters:
  - English: 826 to 877 (median 851);
  - random letters: 992 to 996;
  - on pages of 100: 700 to 885 and 920 to 960.

  The default top, 925, lies between English and random letters at both lengths.
- **Padding not counted (both filters):** a unit that ends in two or more SPACEs is judged on what
  comes before them, as `words-v2` does, and padding alone fails. Six letters of noise padded to 32
  with SPACEs passed `model-information-v1`. It fails v2.
- Both v1 and v2 are retired (judge only), as v1 was. So tailoring and every existing stack are
  unchanged. v1 now says "replaced by" v2.
- **Checked:** both are in the oracle, with 288 new verdict vectors and 24 black-and-white ranks.
  C++ agrees with the oracle on all of them. There are new unit tests, and the black-and-white
  ranker is checked exhaustively.

## Also
- Step 7 is added to the plan in HANDOFF: the CI in parts. Your words are in VOICES.
- Docs: SPECIFICATIONS, FILTERS-CONFLICTS, README, the dictionaries' README, `sieve help dicts` and
  `sift`, IDEAS §2.1 to §2.3.

## Tested here
- Unit tests: 77,033 checks, 0 failures.
- `check_help.py`: 0 missing.
- Not run: the full CI. The two vector files were made by the oracle, so step 15 should reproduce
  them.

## To apply
Copy `Sieve/` over and rebuild. The build copies the new dictionaries next to the programs.
