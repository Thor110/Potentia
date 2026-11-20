#!/usr/bin/env python3
"""Orders by sort key, measured (REVIEW-ROUND O9; IDEAS 19.256). Nothing here is built into Sieve.

An order of a line by a key puts the units with the smallest key first and, among units with the
same key, keeps them in positional order. A unit's address in that order is the number of units
before it, so it costs log2 of that many bits. Naming which of the K orders was used costs log2 K
more. This script measures, on pages of 32 symbols (lower27: a-z and the space), the orders:

  positional   the line as it is (the baseline)
  guided       the model's probability, highest first: Sieve's guided order (exact, from `sieve warp`)
  words-v2     units that pass words-v2 first (their compact number), the rest after (`sieve check`)
  entropy      the letters' entropy, lowest first (the page's letter counts alone)
  distinct     the number of different symbols, fewest first

The last two depend only on how many times each symbol occurs, so the units before a page are
counted exactly by going through every partition of 32 into at most 27 parts (8,349 of them): a
partition's units are 32! / prod(c!) arrangements times the ways to give its parts to symbols. Within
a key's tie the page's own place is not worked out; the script gives the bits at the tie's middle,
and at its end (the most it could cost).

Usage: python3 -I tools/sort_keys_study.py PATH/TO/sieve [--random N] [--seed S] TEXTFILE...
  Pages are the first 32 characters of prose lines (40 characters or more, mostly letters) of the
  text files, canonicalised by `sieve warp` itself; --random N adds N pages of uniformly random
  symbols as a control.
"""
import math
import os
import random
import re
import subprocess
import sys
import tempfile
from collections import Counter

L = 32
SYMBOLS = 27  # lower27: a-z and the space
ALPHABET = "abcdefghijklmnopqrstuvwxyz "


def run(sieve, *args):
    out = subprocess.run([sieve, *args], capture_output=True, text=True, check=True).stdout
    return out.replace("\r", "")


def partitions(n, most, top=None):
    """The partitions of n into at most `most` parts, each a descending tuple."""
    top = n if top is None else top
    if n == 0:
        yield ()
        return
    if most == 0:
        return
    for first in range(min(n, top), 0, -1):
        for rest in partitions(n - first, most - 1, first):
            yield (first,) + rest


def units_of(part):
    """How many pages have these letter counts, in some order of the symbols."""
    arrangements = math.factorial(L)
    for c in part:
        arrangements //= math.factorial(c)
    ways = math.factorial(SYMBOLS) // math.factorial(SYMBOLS - len(part))
    for m in Counter(part).values():
        ways //= math.factorial(m)
    return arrangements * ways


def entropy_key(part):
    # Lower entropy first; sum c log c decides it, compared exactly as the integer prod c^c (larger first).
    k = 1
    for c in part:
        k *= c ** c
    return -k


def distinct_key(part):
    return len(part)


def class_table(key):
    table = Counter()
    for part in partitions(L, SYMBOLS):
        table[key(part)] += units_of(part)
    order = sorted(table)
    below, before = {}, 0
    for k in order:
        below[k] = before
        before += table[k]
    assert before == SYMBOLS ** L
    return table, below


def bits(n):
    return math.log2(n) if n > 0 else 0.0


def key_bits(text, key, table, below):
    part = tuple(sorted(Counter(text).values(), reverse=True))
    k = key(part)
    return bits(below[k] + table[k] / 2 + 1), bits(below[k] + table[k])


def prose_pages(path, want):
    pages = []
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            line = line.strip()
            letters = sum(ch.isalpha() or ch == " " for ch in line)
            if len(line) >= 40 and letters >= 0.9 * len(line) and not line.startswith(("#", "|", "-", "*", ">")):
                pages.append(line[:L])
            if len(pages) >= want:
                break
    return pages


def measure(sieve, raw, words_ini, n_pass):
    pos = run(sieve, "warp", "--length", str(L), "--mode", "positional", raw)
    text = re.search(r'unit 1/1  "(.*)"', pos).group(1)
    place = int(re.search(r"^  positional  ([0-9a-f]+)", pos, re.M).group(1), 16)
    guided = run(sieve, "warp", "--length", str(L), "--mode", "guided", raw)
    guided_bits = int(re.search(r"^ +(\d+) bits:", guided, re.M).group(1))
    chk = run(sieve, "check", "--length", str(L), "--filters", words_ini, raw)
    m = re.search(r"survivor number (\d+) of", chk)
    # A survivor stands at its compact number; a page that fails, after every survivor: at most n_pass + its place.
    words_bits = bits(int(m.group(1)) + 1) if m else bits(n_pass + place)
    return text, bits(place + 1), guided_bits, words_bits, bool(m)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    sieve, rest = argv[1], argv[2:]
    n_random, seed, files = 0, 1, []
    i = 0
    while i < len(rest):
        if rest[i] == "--random":
            n_random, i = int(rest[i + 1]), i + 2
        elif rest[i] == "--seed":
            seed, i = int(rest[i + 1]), i + 2
        else:
            files.append(rest[i])
            i += 1
    raws = [("text", p) for f in files for p in prose_pages(f, 6)]
    rng = random.Random(seed)
    raws += [("random", "".join(rng.choice(ALPHABET) for _ in range(L))) for _ in range(n_random)]

    ent_table, ent_below = class_table(entropy_key)
    dis_table, dis_below = class_table(distinct_key)
    with tempfile.TemporaryDirectory() as tmp:
        words_ini = os.path.join(tmp, "words.ini")
        with open(words_ini, "w") as f:
            f.write("[text]\nmode = compact\nfilters = words-v2\n")
        stats = run(sieve, "filters", "--length", str(L), "--filters", words_ini)
        n_pass = int(re.search(r"^survivors +(\d+)", stats, re.M).group(1))
        rows = []
        for kind, raw in raws:
            text, pos, gd, wd, passed = measure(sieve, raw, words_ini, n_pass)
            ent_mid, ent_hi = key_bits(text, entropy_key, ent_table, ent_below)
            dis_mid, dis_hi = key_bits(text, distinct_key, dis_table, dis_below)
            rows.append((kind, text, pos, gd, wd, passed, ent_mid, ent_hi, dis_mid, dis_hi))

    orders = 5
    name = math.log2(orders)
    print(f"pages of {L} symbols (lower27); {len(rows)} pages; naming one of {orders} orders costs {name:.2f} bits")
    print(f"words-v2 keeps {n_pass} of 27^{L} pages ({bits(n_pass):.1f} of {bits(SYMBOLS ** L):.1f} bits)\n")
    print(f"  {'kind':<7} {'page':<34} {'positional':>10} {'guided':>7} {'words-v2':>9} {'entropy':>14} {'distinct':>14}")
    for kind, text, pos, gd, wd, passed, em, eh, dm, dh in rows:
        print(f"  {kind:<7} {chr(34) + text + chr(34):<34} {pos:>10.1f} {gd:>7d} {wd:>8.1f}{'' if passed else '*'} {em:>6.1f} ({eh:>5.1f}) {dm:>6.1f} ({dh:>5.1f})")
    print("  (* fails words-v2; entropy and distinct at the middle of their tie, and in brackets at its end)\n")
    for kind in ("text", "random"):
        sel = [r for r in rows if r[0] == kind]
        if not sel:
            continue
        n = len(sel)
        mean = lambda j: sum(r[j] for r in sel) / n
        print(f"  mean over {n} {kind} pages: positional {mean(2):.1f}, guided {mean(3):.1f}, words-v2 {mean(4):.1f}, "
              f"entropy {mean(6):.1f}, distinct {mean(8):.1f} bits")
        best = sum(min(r[2], r[3], r[4], r[6], r[8]) + name for r in sel) / n
        print(f"    the best order for each page, named ({name:.2f} bits): {best:.1f} bits; against positional {best - mean(2):+.1f}")
        # Sieve's own orders alone, guided or words-v2's compact, the choice named in 1 bit: what the new keys add to them.
        own = sum(min(r[3], r[4]) + 1 for r in sel) / n
        new_best = sum(1 for r in sel if min(r[6], r[8]) + name < min(r[3], r[4]) + 1)
        print(f"    guided or words-v2 alone, named (1 bit): {own:.1f} bits; pages where entropy or distinct beats both, named: {new_best} of {n}")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
