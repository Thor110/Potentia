#!/usr/bin/env python3
"""Converts the Moby Part-of-Speech II list into the tagged-list form filter plugins read.

The Moby Project's lexicons were placed in the public domain by their author, Grady Ward
("Public Domain material by grant from the author, January, 2001": Project Gutenberg eBook #3203,
"Moby Part of Speech List"). The part-of-speech list is one entry a line, a word or phrase, then
a separator, then its part-of-speech codes, one character each:

    N noun   p plural   h noun phrase   V verb (usually participle)   t verb (transitive)
    i verb (intransitive)   A adjective   v adverb   C conjunction   P preposition
    ! interjection   r pronoun   D definite article   I indefinite article   o nominative

The file as distributed (mobyposi.i, 233,356 entries) is in the Macintosh character set with
carriage returns, the separator the byte 0xD7; the Gutenberg text describes a backslash, which is
read as well. The output is UTF-8 with line feeds, `word<TAB>codes` a line, in the file's own
order, every entry kept (phrases and accented words too; a plugin keeps what its line can spell).

    python3 tools/moby_pos.py mobyposi.i data/filters/moby-pos-v1.tsv

prints the input's and the output's SHA-256, so the shipped file can be checked against its source
(data/filters/moby-pos-v1.md records both).

Moby lists base forms and irregular ones ("grew", "gave") but not regular inflections ("walked",
"years", "younger"). The second use makes those, as a list of their own:

    python3 tools/moby_pos.py --inflect data/filters/moby-pos-v1.tsv \
        data/dictionaries/scowl-2020.12.07-en-80.txt data/filters/moby-inflections-v1.tsv

From each single lower-case word of the list and its codes, by the regular rules of English:
nouns (N) their plurals (-s, -es after s x z ch sh, consonant-y to -ies), tagged p; verbs (V t i)
their third person (the same endings), past (-ed, -d after e, consonant-y to -ied, the final
consonant doubled after a single short vowel) and -ing forms (dropping a final e, ie to -ying,
doubling as for -ed), tagged with the verb's own codes, the past forms also A and the -ing forms
also N and A (participles and gerunds); adjectives (A) their comparatives and superlatives (-er,
-est, -r and -st after e, consonant-y to -ier and -iest, doubling), tagged A. A form is kept only
where it is a word of the dictionary given (SCOWL's largest English list), so nothing is invented,
and only where Moby does not list it already with those codes. Adjectives also make their -ly
adverbs (happily, gently, remarkably, basically), tagged v. Sorted, word<TAB>codes a line.

A third use lists proper names Moby lacks (novels are full of them), tagged N:

    python3 tools/moby_pos.py --names data/filters/moby-pos-v1.tsv \
        data/dictionaries/scowl-2020.12.07-en-80-names.txt data/dictionaries/scowl-2020.12.07-en-80.txt \
        data/filters/moby-names-v1.tsv

the words of the names list that are not in the plain list (so they are names) and not in Moby.
"""
import hashlib
import sys


def convert(raw: bytes) -> bytes:
    text = raw.decode("mac_roman")
    out = []
    for line in text.replace("\r\n", "\n").replace("\r", "\n").split("\n"):
        if not line:
            continue
        for sep in ("◊", "\\"):  # 0xD7 in the Macintosh set is U+25CA
            if sep in line:
                word, _, codes = line.rpartition(sep)
                break
        else:
            raise SystemExit(f"no separator in: {line!r}")
        if not word or not codes:
            raise SystemExit(f"malformed entry: {line!r}")
        out.append(f"{word}\t{codes}\n")
    return "".join(out).encode("utf-8")


VOWELS = set("aeiou")


def plural(w):
    """The -s forms: plural nouns, third-person verbs."""
    if w.endswith(("s", "x", "z", "ch", "sh")):
        return [w + "es"]
    if len(w) > 1 and w[-1] == "y" and w[-2] not in VOWELS:
        return [w[:-1] + "ies"]
    return [w + "s"]


def doubles(w):
    """Whether the final consonant may double before -ed, -ing, -er (stop, run, big)."""
    return (len(w) >= 3 and w[-1] not in VOWELS and w[-1] not in "wxy" and w[-2] in VOWELS and w[-3] not in VOWELS)


def past(w):
    if w.endswith("e"):
        return [w + "d"]
    if len(w) > 1 and w[-1] == "y" and w[-2] not in VOWELS:
        return [w[:-1] + "ied"]
    out = [w + "ed"]
    if doubles(w):
        out.append(w + w[-1] + "ed")
    return out


def ing(w):
    if w.endswith("ie"):
        return [w[:-2] + "ying"]
    if w.endswith("e") and not w.endswith(("ee", "ye", "oe")):
        return [w[:-1] + "ing", w + "ing"]
    out = [w + "ing"]
    if doubles(w):
        out.append(w + w[-1] + "ing")
    return out


def compare(w):
    if w.endswith("e"):
        return [w + "r", w + "st"]
    if len(w) > 1 and w[-1] == "y" and w[-2] not in VOWELS:
        return [w[:-1] + "ier", w[:-1] + "iest"]
    out = [w + "er", w + "est"]
    if doubles(w):
        out += [w + w[-1] + "er", w + w[-1] + "est"]
    return out


def adverb(w):
    """-ly adverbs from adjectives: happy/happily, gentle/gently, remarkable/remarkably, basic/basically."""
    if len(w) > 1 and w[-1] == "y" and w[-2] not in VOWELS:
        return [w[:-1] + "ily"]
    if w.endswith("le") and len(w) > 2 and w[-3] not in VOWELS:
        return [w[:-1] + "y"]
    if w.endswith("ic"):
        return [w + "ally", w + "ly"]
    return [w + "ly"]


def inflect(pos_text: str, dictionary: set) -> bytes:
    known = {}
    for line in pos_text.split("\n"):
        word, _, codes = line.partition("\t")
        if word:
            known.setdefault(word, set()).update(codes)
    made = {}

    def add(form, codes):
        if form in dictionary and not set(codes) <= known.get(form, set()):
            made.setdefault(form, set()).update(codes)

    for word, codes in known.items():
        if not word.isascii() or not word.isalpha() or not word.islower():
            continue
        verb = "".join(c for c in "Vti" if c in codes)
        if "N" in codes:
            for f in plural(word):
                add(f, "p")
        if verb:
            for f in plural(word):
                add(f, verb)
            for f in past(word):
                add(f, verb + "A")
            for f in ing(word):
                add(f, verb + "NA")
        if "A" in codes:
            for f in compare(word):
                add(f, "A")
            for f in adverb(word):
                add(f, "v")
    order = "NphVtiAvCP!rDIo"
    return "".join(f"{w}\t{''.join(c for c in order if c in made[w])}\n" for w in sorted(made)).encode("utf-8")


def names(pos_text: str, with_names: set, plain: set) -> bytes:
    known = {line.partition("\t")[0].lower() for line in pos_text.split("\n") if line}
    made = sorted(w for w in with_names - plain if w.isascii() and w.isalpha() and w not in known)
    return "".join(f"{w}\tN\n" for w in made).encode("utf-8")


def read_words(path):
    raw = open(path, "rb").read()
    return raw, {w.strip().lower() for w in raw.decode("utf-8", "replace").split("\n") if w.strip()}


def main():
    if len(sys.argv) == 6 and sys.argv[1] == "--names":
        pos = open(sys.argv[2], "rb").read()
        raw_n, with_names = read_words(sys.argv[3])
        raw_p, plain = read_words(sys.argv[4])
        data = names(pos.decode("utf-8"), with_names, plain)
        open(sys.argv[5], "wb").write(data)
        for label, raw, path in (("list", pos, sys.argv[2]), ("names", raw_n, sys.argv[3]), ("plain", raw_p, sys.argv[4])):
            print(f"{label:10}  {hashlib.sha256(raw).hexdigest()}  {path}")
        entries = data.count(b"\n")
        print(f"output      {hashlib.sha256(data).hexdigest()}  {sys.argv[5]}  ({entries} entries)")
        return
    if len(sys.argv) == 5 and sys.argv[1] == "--inflect":
        pos = open(sys.argv[2], "rb").read()
        dic_raw = open(sys.argv[3], "rb").read()
        dictionary = {w.strip().lower() for w in dic_raw.decode("utf-8", "replace").split("\n") if w.strip()}
        data = inflect(pos.decode("utf-8"), dictionary)
        open(sys.argv[4], "wb").write(data)
        print(f"list        {hashlib.sha256(pos).hexdigest()}  {sys.argv[2]}")
        print(f"dictionary  {hashlib.sha256(dic_raw).hexdigest()}  {sys.argv[3]}")
        entries = data.count(b"\n")
        print(f"output      {hashlib.sha256(data).hexdigest()}  {sys.argv[4]}  ({entries} entries)")
        return
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    raw = open(sys.argv[1], "rb").read()
    data = convert(raw)
    open(sys.argv[2], "wb").write(data)
    print(f"source  {hashlib.sha256(raw).hexdigest()}  {sys.argv[1]}")
    entries = data.count(b"\n")
    print(f"output  {hashlib.sha256(data).hexdigest()}  {sys.argv[2]}  ({entries} entries)")


if __name__ == "__main__":
    main()
