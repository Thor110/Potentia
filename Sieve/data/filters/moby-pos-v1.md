# The Moby part-of-speech lists

These three tagged lists (`word<TAB>codes`, one a line, UTF-8, line feeds) are read by the grammar plugins `moby-grammar-v1` and `moby-grammar-strict-v1` through `set NAME tags:FILES:TAGS` (docs/FILTER-PLUGINS.md). Each plugin records every list's SHA-256 in its provenance, so a changed list is visibly a different filter.

| File | Entries | SHA-256 | What it is |
| :--- | ---: | :--- | :--- |
| `moby-pos-v1.tsv` | 233,356 | `d66669a7b04c411f7da4d60fd21a12a20f7d87f5b8f73141849c97c96e0a1f58` | Moby Part-of-Speech II, every entry, converted |
| `moby-inflections-v1.tsv` | 73,070 | `c02b639f8c9cdf2161e32aa282acfbb4032c50a18158b9ea556821f016285922` | Regular inflections Moby lacks, confirmed by SCOWL |
| `moby-names-v1.tsv` | 16,052 | `044906b37a88f07bffb6fe8d8671fb0bdb4642c2f62c3965e59967ef5c4c228f` | Proper names Moby lacks, from SCOWL, tagged N |

## The source

**Moby Part-of-Speech II** by Grady Ward: 233,356 words and phrases, each with its parts of speech in priority order. It is public domain ("Public Domain material by grant from the author, January, 2001", Project Gutenberg eBook #3203). The notice is in `third_party_licenses/Moby/PUBLIC-DOMAIN.txt`.

The copy converted here is `moby/mpos/mobyposi.i` from the mirror at github.com/Hyneman/moby-project, commit `672f6bdca054c42d375f065ffee87e8ceba0c242`. Its SHA-256 is `daa369396e90e16ed8eb89b9e70e6b83939d021a7bd58077c82d3be7fe1a2d14`. The file uses the Macintosh character set, carriage returns, and the byte 0xD7 between word and codes. The mirror's `mpos/readme` still carries the 1993 commercial licence the list was first sold under; the 2001 grant replaced it.

The codes, one character each:

| Code | Meaning | Code | Meaning | Code | Meaning |
| :--- | :--- | :--- | :--- | :--- | :--- |
| N | noun | p | plural | h | noun phrase |
| V | verb (usually participle) | t | verb (transitive) | i | verb (intransitive) |
| A | adjective | v | adverb | C | conjunction |
| P | preposition | ! | interjection | r | pronoun |
| D | definite article | I | indefinite article | o | nominative |

## Rebuilding

`tools/moby_pos.py` makes all three lists and prints the input's and output's hashes, which should match the ones above:

```sh
git clone https://github.com/Hyneman/moby-project.git
git -C moby-project checkout 672f6bdca054c42d375f065ffee87e8ceba0c242
python3 tools/moby_pos.py moby-project/moby/mpos/mobyposi.i data/filters/moby-pos-v1.tsv
python3 tools/moby_pos.py --inflect data/filters/moby-pos-v1.tsv \
    data/dictionaries/scowl-2020.12.07-en-80.txt data/filters/moby-inflections-v1.tsv
python3 tools/moby_pos.py --names data/filters/moby-pos-v1.tsv \
    data/dictionaries/scowl-2020.12.07-en-80-names.txt data/dictionaries/scowl-2020.12.07-en-80.txt \
    data/filters/moby-names-v1.tsv
```

**The conversion** keeps every entry in the file's own order, including phrases and accented words. Each line becomes `word<TAB>codes`. A plugin keeps only what its line can spell: on `lower27`, capitals are read in lower case, and phrases (which hold a SPACE, the separator) and accented words are left out.

**The inflections.** Moby lists base forms and irregular ones ("grew", "gave"), but not regular inflections ("walked", "years", "younger"). The rules of English make them from each single lower-case word of Moby:
- plurals of nouns, tagged p;
- the third person, past and -ing forms of verbs, tagged with the verb's own codes; past forms are also tagged A, and -ing forms also N and A;
- comparatives and superlatives of adjectives, tagged A;
- -ly adverbs from adjectives, tagged v.

A form is kept only if it is a word of SCOWL's largest English list and Moby does not already list it. Nothing is invented.

**The names.** These are the words of SCOWL's size-80 names list that are not in its plain list (so they are names) and not in Moby, tagged N. Novels are full of names that Moby lacks.

Because the inflections and names are SCOWL's words, SCOWL's notice (`third_party_licenses/SCOWL/Copyright.txt`) travels with them.

## How a plugin reads them

A word carries the tags of all its lines, in every file joined with `+` and in every spelling that comes to the same word on the line. `set noun tags:...:Np-DIPCro` is then the words that carry N or p and none of D, I, P, C, r, o.

This matters because Moby lists capitalised entries apart from lower-case ones. For example, "OF" (an abbreviation) is a noun, while "of" is a preposition. Read in lower case, the two are one word tagged `PN`, so the noun set's exclusion of P keeps "of" out of it.

## The grammar plugins

Both plugins use the same nine sets:

| Set | Tags | Meaning |
| :--- | :--- | :--- |
| det | `DI` | articles |
| noun | `Np-DIPCro` | nouns and plurals, not function words |
| pron | `ro` | pronouns |
| verb | `Vti-DIPCro` | verbs, not function words |
| adj | `A-DIPCro` | adjectives, not function words |
| adv | `v-DI` | adverbs, not articles ("the" is listed as an adverb too) |
| prep | `P` | prepositions |
| conj | `C` | conjunctions |
| interj | `!` | interjections |

A word in several sets may be any of them. Which set may follow which, and how a sentence may start and end, was chosen from real sentences rather than by hand:
1. Start with every pairing allowed.
2. Take pairings away one at a time, keeping each removal that cost real sentences less than it cost the same sentences shuffled (strict), or less than half as much (lenient).
3. Tune on three books of the NLTK Gutenberg corpus (Persuasion, The Man Who Was Thursday, Buster Bear).
4. Measure on four other books the tuning never saw (Moby-Dick, Emma, Alice in Wonderland, The Parent's Assistant).

The test set is 2,790 sentences of 4 to 14 words whose words are all in the lists. The measurements were made with the compiled plugins:

| Plugin | Real sentences pass | Shuffled pass | Random words pass |
| :--- | ---: | ---: | ---: |
| `moby-grammar-v1` (lenient) | 83.5% | 42.8% | 41.4% |
| `moby-grammar-strict-v1` | 58.8% | 18.4% | 17.1% |

"Shuffled" is each sentence's own words in random order, three shuffles each. "Random words" is sentences of the same length built from the test set's vocabulary.

**What they are.** They are a sieve for word salad, not parsers: a bigram grammar over parts of speech has a ceiling, and these sit near it.

**What they miss:**
- A word that is in none of the lists (a rare name, dialect, a coinage) fails every sentence it is in.
- Imperatives ("call me ishmael") fail, because neither plugin lets a sentence start with a verb.
- Nothing here knows punctuation. Both plugins judge a unit of words and single SPACEs, with at most one SPACE at each end.

The corpus used for tuning and measuring is not shipped. `tools/fetch_corpus.py` fetches it into `corpus/`, which git ignores.
