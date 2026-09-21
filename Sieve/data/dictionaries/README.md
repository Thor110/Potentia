# Dictionaries

`dictionaries.tsv` is the registry. Each line gives a dictionary's id (used with `--dict`), its file, language, whether it is the default, its SHA-256 and a description. A registered dictionary whose file no longer matches its hash is refused. `sieve dicts` lists and checks them; `sieve help dicts` explains how to add one.

English word lists for the `lower27` line, built from **SCOWL** (Spell Checker Oriented Word Lists), release **2020.12.07**, by Kevin Atkinson. SCOWL is the source of the Hunspell English dictionaries used by LibreOffice, Firefox and most Linux spell checkers. Its licence is permissive; see `SCOWL-Copyright.txt`, which must travel with these files.

| Id | File | SCOWL size | Words | SHA-256 |
| :--- | :--- | :--- | ---: | :--- |
| `scowl-en-35` | `scowl-2020.12.07-en-35.txt` | 35: common words | 40,201 | `fa4dfcfc6e792cf3b308d041b02955bd1fddb8559a220a6e52666abba59d02d1` |
| `scowl-en-60` | `scowl-2020.12.07-en-60.txt` | 60: SCOWL's recommended spell-check size (**default**) | 79,645 | `51ecd4eed87c71ca0de28627fb7f6646a7802d782e665a86848eccfba4cd8353` |
| `scowl-en-80` | `scowl-2020.12.07-en-80.txt` | 80: large, includes rare words | 251,174 | `47810b49e2abe0ce884aaea5e6c637f0d78bce9736f2e345087bcc3c00fafec4` |

**Included:** SCOWL's `english`, `american`, `british` and `british_z` word lists at every level up to the size, so both British and American spellings count ("colour" and "color").

**Excluded:**
- abbreviations
- proper names and capitalised words
- contractions and variant spellings
- anything that isn't entirely a–z (apostrophes, accents, hyphens)
- single letters other than "a" and "i"

Every sieve result records the SHA-256 of the dictionary it used. Changing a list changes its hash, so results remain traceable.

## Rebuilding

```sh
git clone --depth 1 --branch rel-2020.12.07 https://github.com/en-wl/wordlist.git scowl-src
make -C scowl-src        # needs make and perl; do not use -j
python3 tools/build_dictionary.py --scowl scowl-src/scowl/final --size 60 \
    --out data/dictionaries/scowl-2020.12.07-en-60.txt
```

The output is sorted, one word per line, with LF line endings, so the hashes above reproduce exactly.
