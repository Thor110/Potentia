#!/usr/bin/env python3
"""Build a pinned lower27 dictionary from SCOWL (Spell Checker Oriented Word Lists).

SCOWL is the word list behind the Hunspell English dictionaries used by LibreOffice,
Firefox and most Linux spell checkers. Words are graded by size level (10 = most
common ... 95 = very obscure); 60 is SCOWL's recommended size for spell checking.

What goes in:
    english-words, american-words, british-words, british_z-words  (all levels <= SIZE)
What stays out:
    abbreviations, proper names, capitalised words, contractions, variant spellings,
    any entry that is not entirely a-z (apostrophes, accents, hyphens), and single
    letters other than "a" and "i" (SCOWL lists every letter as a word, which would
    let strings like "b c d" pass as English).

Getting SCOWL (one time, needs git, make and perl):
    git clone --depth 1 --branch rel-2020.12.07 https://github.com/en-wl/wordlist.git scowl-src
    make -C scowl-src          # run without -j: the build is not parallel-safe

Then:
    python3 tools/build_dictionary.py --scowl scowl-src/scowl/final --size 60 \\
        --out data/dictionaries/scowl-2020.12.07-en-60.txt

With --names, the list also takes SCOWL's capitalised words and proper names (upper and
proper-names, e.g. "England", "Dickens") and keeps every word's case as SCOWL writes it:
    python3 tools/build_dictionary.py --scowl scowl-src/scowl/final --size 60 --names \\
        --out data/dictionaries/scowl-2020.12.07-en-60-names.txt
Sieve reads such a list folded to lower case for the lower-case alphabets ("England" matches
"england"), and the capitals stay in the file for filters that respect case.
"""
import argparse
import hashlib
import os
import re
import sys

CATEGORIES = ("english", "american", "british", "british_z")
LEVELS = (10, 20, 35, 40, 50, 55, 60, 70, 80, 95)


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument("--scowl", required=True, help="path to SCOWL's built 'final' directory")
    p.add_argument("--size", type=int, required=True, choices=LEVELS, help="largest SCOWL size level to include")
    p.add_argument("--out", required=True, help="output word list (one word per line)")
    p.add_argument("--names", action="store_true", help="also capitalised words and proper names, case kept")
    args = p.parse_args()

    words, files_used = set(), 0
    kinds = ("words", "upper", "proper-names") if args.names else ("words",)
    pattern = "[A-Za-z]+" if args.names else "[a-z]+"
    for cat in CATEGORIES:
        for kind in kinds:
            for level in LEVELS:
                if level > args.size:
                    continue
                path = os.path.join(args.scowl, f"{cat}-{kind}.{level}")
                if not os.path.exists(path):
                    continue
                files_used += 1
                with open(path, encoding="latin-1") as f:  # SCOWL final lists are ISO-8859-1
                    for line in f:
                        w = line.strip()
                        if re.fullmatch(pattern, w) and (len(w) > 1 or w in ("a", "i", "I")):
                            words.add(w)
    if not files_used:
        sys.exit(f"no SCOWL word files found in {args.scowl}")

    data = "".join(w + "\n" for w in sorted(words)).encode("ascii")
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as f:
        f.write(data)
    print(f"{args.out}: {len(words)} words from {files_used} files, sha256 {hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    main()
