# function-words-v2, and scowl-en-60-names-v2 as the default (24 October 2026)

As you suggested: v1 is kept as it is, and v2 is made beside it.

## What changed
- **`function-words-v2`** (`data/filters/function-words-v2.sfilter` and `.tsv`): v1's rules, with
  every token a word of `scowl-en-60-names-v2`, the list without SCOWL's symbols and acronyms. So it
  implies `words-v2` with the default list again. `tools/build_text_plugins.py` writes it, and
  writes v1's files unchanged beside it (`--check` passes).
- **The default dictionary is now `scowl-en-60-names-v2`** (`dictionaries.tsv`). `scowl-en-60-names`
  stays registered, so a stack made with it still reproduces by naming it. `function-words-v1` keeps
  its own copy of that list in its `.tsv`, so it judges exactly as before.
- **CI:** a new line compares `function-words-v2` with the oracle, as v1 is compared.
- **Docs:** SPECIFICATIONS (dictionaries), FILTER-PLUGINS (the generated plugins' table),
  `sieve help sift` and `dicts`, the dictionaries' README (which still called `scowl-en-60` the
  default), IDEAS §2.3, HANDOFF and VOICES.

## Measured here
| | `function-words-v1` | `function-words-v2` |
| :--- | ---: | ---: |
| Pages of 32 letters kept | 2.95 × 10^27 (10^-18.33, as documented) | 5.25 × 10^26 (10^-19.08) |
| The 100 held-out sentences and their 100 shuffles | 159 of 200 pass | the same 159, verdict for verdict |

## Tested here
- Unit tests: 0 failures.
- `check_help.py`: 0 missing.
- `sieve dicts`: the new default's hash checks.
- The new CI line: the engine and the oracle agree.

## To apply
Copy `Sieve/` and `.github/` over, and rebuild.
