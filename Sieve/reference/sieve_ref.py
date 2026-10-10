#!/usr/bin/env python3
"""Sieve reference oracle (SPECIFICATIONS §12).

An independent, deliberately plain implementation of the raw address map,
canonicalisation and the M1 sieve filters. It uses Python's native big integers
and hashlib, sharing no code with the C++ core. The C++ core must agree with it
bit for bit; the conformance vectors in tests/vectors_v1.tsv are generated here.

    python3 sieve_ref.py biguint-vectors > ../tests/vectors_biguint_v1.tsv
    python3 sieve_ref.py biguint-large-vectors > ../tests/vectors_biguint_large_v1.tsv
    python3 sieve_ref.py vectors        > ../tests/vectors_v1.tsv
    python3 sieve_ref.py digit-vectors  > ../tests/vectors_digits_v1.tsv
    python3 sieve_ref.py canon-vectors  > ../tests/vectors_canon.tsv
    python3 sieve_ref.py bytes-vectors  > ../tests/vectors_bytes_v1.tsv
    python3 sieve_ref.py titled-vectors > ../tests/vectors_titled_v1.tsv
    python3 sieve_ref.py binary-vectors > ../tests/vectors_binary_v1.tsv
    python3 sieve_ref.py kind-vectors   > ../tests/vectors_kinds_v1.tsv
    python3 sieve_ref.py written-vectors > ../tests/vectors_written_v1.tsv
    python3 sieve_ref.py cross-vectors  > ../tests/vectors_cross_v1.tsv
    python3 sieve_ref.py picture-vectors > ../tests/vectors_picture_v1.tsv
    python3 sieve_ref.py utf8-vectors   > ../tests/vectors_utf8_v1.tsv
    python3 sieve_ref.py manifest ../tests/manifest_fixture [--addresses DIR] [--with-addresses | --with-contents]   # sieve-manifest-v1/v2/v3
    python3 sieve_ref.py map ../tests/manifest_fixture                # sieve-map-v1
    python3 sieve_ref.py image-vectors  > ../tests/vectors_image_v1.tsv
    python3 sieve_ref.py guided-vectors > ../tests/vectors_guided_v1.tsv
    python3 sieve_ref.py filter-vectors > ../tests/vectors_filters_v1.tsv
    python3 sieve_ref.py model-build --out /tmp/check.model   # rebuilds the pinned model
    python3 sieve_ref.py warp --length 32 "Some text"
    python3 sieve_ref.py read --length 32 --mode scrambled <hex>
    python3 sieve_ref.py sieve --dict words.txt --max 4
    python3 sieve_ref.py plugin ../data/filters/max-run-data-v1.sfilter --length 12 --params max=2
    python3 sieve_ref.py plugin ../data/filters/moby-grammar-v1.sfilter --length 6 --lazy \
        --judge ../tests/plugins/moby-sentences.txt   # too large to determinise: subsets as reached
"""
import argparse
import collections
import hashlib
import re
import math
import os
import struct
import sys
import unicodedata
from array import array
from fractions import Fraction

# Pinned symbol alphabets. The order of the symbols IS the digit order, and babel29's is
# libraryofbabel.info's, which is not code point order.
ALPHABETS = {
    "lower27": " " + "abcdefghijklmnopqrstuvwxyz",
    "babel29": " " + "abcdefghijklmnopqrstuvwxyz" + ",.",
    "ascii95": "".join(chr(c) for c in range(0x20, 0x7F)),
    "ascii96": "\n" + "".join(chr(c) for c in range(0x20, 0x7F)),
    # SPECIFICATIONS 12.1a: U+0000-U+00FF in code point order, so digit d is byte d.
    "bytes256": "".join(chr(c) for c in range(0x100)),
}

# A few named Unicode blocks, written out here from the standard rather than read from the C++
# table, so a vector over one of them checks that table rather than trusting it.
BLOCKS = {
    "greek": (0x0370, 0x03FF),
    "cyrillic": (0x0400, 0x04FF),
    "hiragana": (0x3040, 0x309F),
    "katakana": (0x30A0, 0x30FF),
    "ascii": (0x0020, 0x007E),
    "surrogates": (0xD800, 0xDFFF),
    "all-emojis": (0x1F300, 0x1FAFF),
}


def alphabet(spec):
    """A built-in id, or blocks and u+XXXX[-u+YYYY] ranges stacked with '+': the union of their
    code points, ascending. Mirrors sieve::alphabet_of."""
    if spec in ALPHABETS:
        return ALPHABETS[spec]
    parts, cur = [], ""
    for i, c in enumerate(spec):
        if c == "+" and not cur[-1:].lower() == "u":
            parts.append(cur)
            cur = ""
        else:
            cur += c
    parts.append(cur)
    points = set()
    for part in parts:
        if part in BLOCKS:
            lo, hi = BLOCKS[part]
        else:
            bits = [b for b in part.replace("u+", " ").replace("U+", " ").split("-") if b.strip()]
            lo = int(bits[0], 16)
            hi = int(bits[1], 16) if len(bits) > 1 else lo
        points.update(range(lo, hi + 1))
    return "".join(chr(c) for c in sorted(points))
ROUNDS = 8
WHITESPACE = {"\t", "\n", "\x0b", "\x0c", "\r", " ", "\xa0"}


class Space:
    def __init__(self, alphabet_id, length, key="sieve", base=None):
        # A text space (alphabet_id) or, with base=N, a space over N abstract symbols.
        self.symbols = alphabet(alphabet_id) if alphabet_id else None
        self.n = base if base else len(self.symbols)
        self.length = length
        self.key = key.encode("utf-8")
        self.size = self.n ** length
        self.hex_width = max(1, ((self.size - 1).bit_length() + 3) // 4)

    # -- unit text <-> digits (most significant first)
    def digits_of(self, text):
        assert len(text) == self.length, "wrong unit length"
        return [self.symbols.index(c) for c in text]

    def text_of(self, digits):
        return "".join(self.symbols[d] for d in digits)

    # -- scramble (feistel-sha256-v1)
    def _f(self, rnd, src, count):
        prefix = b"POTENTIA/FEISTEL/1"  # frozen constant of feistel-sha256-v1
        prefix += struct.pack("<I", len(self.key)) + self.key
        prefix += struct.pack("<IIII", rnd, self.n, self.length, len(src))
        prefix += b"".join(struct.pack("<I", d) for d in src)
        out, k = [], 0
        while len(out) < count:
            digest = hashlib.sha256(prefix + struct.pack("<I", k)).digest()
            for w in struct.unpack("<8I", digest):
                if len(out) < count:
                    out.append(w % self.n)
            k += 1
        return out

    def _halves(self, rnd):
        h = self.length // 2
        if rnd % 2 == 0:
            return (0, h), (h, self.length)  # src, dst
        return (h, self.length), (0, h)

    def scramble(self, d):
        d = list(d)
        for r in range(ROUNDS):
            (s0, s1), (t0, t1) = self._halves(r)
            f = self._f(r, d[s0:s1], t1 - t0)
            for j in range(t1 - t0):
                d[t0 + j] = (d[t0 + j] + f[j]) % self.n
        return d

    def unscramble(self, d):
        d = list(d)
        for r in reversed(range(ROUNDS)):
            (s0, s1), (t0, t1) = self._halves(r)
            f = self._f(r, d[s0:s1], t1 - t0)
            for j in range(t1 - t0):
                d[t0 + j] = (d[t0 + j] - f[j]) % self.n
        return d

    # -- addresses
    def _value(self, digits):
        v = 0
        for d in digits:
            v = v * self.n + d
        return v

    def _digits(self, v):
        out = []
        for _ in range(self.length):
            v, r = divmod(v, self.n)
            out.append(r)
        assert v == 0
        return out[::-1]

    def address_of(self, text, mode):
        d = self.digits_of(text) if isinstance(text, str) else list(text)
        if mode == "scrambled":
            d = self.scramble(d)
        return format(self._value(d), "x").zfill(self.hex_width)

    def unit_at(self, hexaddr, mode):
        v = int(hexaddr, 16)
        if v >= self.size:
            raise ValueError("address outside the space")
        d = self._digits(v)
        if mode == "scrambled":
            d = self.unscramble(d)
        return self.text_of(d)


TRANSLITERATE = {"\u00ab": '"', "\u00bb": '"', "\u02bc": "'", "\u2010": "-", "\u2011": "-", "\u2012": "-",
                 "\u2013": "-", "\u2014": "-", "\u2015": "-", "\u2018": "'", "\u2019": "'", "\u201c": '"',
                 "\u201d": '"', "\u201e": '"', "\u2026": "...", "\u2032": "'", "\u2033": '"', "\u2212": "-"}
FOLD_SPECIAL = {"Æ": "AE", "æ": "ae", "Ø": "O", "ø": "o", "Đ": "D", "đ": "d", "Ħ": "H", "ħ": "h", "ı": "i",
                "Ĳ": "IJ", "ĳ": "ij", "ĸ": "k", "Ŀ": "L", "ŀ": "l", "Ł": "L", "ł": "l", "ŉ": "n", "Ŋ": "N",
                "ŋ": "n", "Œ": "OE", "œ": "oe", "ß": "ss", "Þ": "TH", "þ": "th", "Ð": "D", "ð": "d", "ſ": "s",
                "Ŧ": "T", "ŧ": "t"}


def fold_letter(c):
    """canon-text-v2 step 3: Latin letters U+00C0-U+017F -> ASCII, else None."""
    if not (0xC0 <= ord(c) <= 0x17F) or not unicodedata.category(c).startswith("L"):
        return None
    return FOLD_SPECIAL.get(c) or unicodedata.normalize("NFD", c)[0]


def canonicalise(text, alphabet_id, length, version="v2"):
    """canon-text-v1 or canon-text-v2. Returns the list of units."""
    symbols = alphabet(alphabet_id)
    fold = not any("A" <= c <= "Z" for c in symbols)
    kept = []
    for orig in text:
        if orig in WHITESPACE:
            # Whitespace the alphabet holds itself stays as it is (ascii96's line feed); a
            # carriage return is dropped there, so either line ending canonicalises the same.
            if orig == "\r" and "\n" in symbols:
                continue
            expanded = orig if orig in symbols else " "
        elif version == "v2" and orig in TRANSLITERATE:
            expanded = TRANSLITERATE[orig]
        elif version == "v2" and fold_letter(orig):
            expanded = fold_letter(orig)
        else:
            expanded = orig
        for c in expanded:
            if fold and "A" <= c <= "Z":
                c = c.lower()
            if c in symbols:
                kept.append(c)
            elif version == "v1" or c == "'":
                pass
            else:
                kept.append(" ")
    canon = []
    for c in kept:
        if c == " " and (not canon or canon[-1] == " "):
            continue
        canon.append(c)
    while canon and canon[-1] == " ":
        canon.pop()
    s = "".join(canon)
    pad = symbols[0]  # digit 0: the space for every alphabet pinned before ascii96
    return [s[i:i + length].ljust(length, pad) for i in range(0, len(s), length)]


def canonicalise_bytes(data, alphabet_id, length):
    """canon-bytes-v1 (SPECIFICATIONS 12.1a): the identity. The bytes are cut into units of
    `length`, a last unit short of the length is padded with NUL (digit 0), and an empty file is
    one unit of NULs. Only an alphabet holding every byte value may use it. Returns the units as
    strings of the code points U+0000-U+00FF, one per byte, which is how the alphabet spells them."""
    symbols = alphabet(alphabet_id)
    assert all(chr(b) in symbols for b in range(0x100)), "canon-bytes-v1 needs every byte value"
    text = "".join(chr(b) for b in data)
    units = [text[i:i + length] for i in range(0, len(text), length)] or [""]
    return [u.ljust(length, "\x00") for u in units]


# -- image line (canon-image-v1), written independently with exact fractions
LEVELS3 = [0, 36, 73, 109, 146, 182, 219, 255]
LEVELS2 = [0, 85, 170, 255]
EGA16 = [(0x00, 0x00, 0x00), (0x00, 0x00, 0xAA), (0x00, 0xAA, 0x00), (0x00, 0xAA, 0xAA),
         (0xAA, 0x00, 0x00), (0xAA, 0x00, 0xAA), (0xAA, 0x55, 0x00), (0xAA, 0xAA, 0xAA),
         (0x55, 0x55, 0x55), (0x55, 0x55, 0xFF), (0x55, 0xFF, 0x55), (0x55, 0xFF, 0xFF),
         (0xFF, 0x55, 0x55), (0xFF, 0x55, 0xFF), (0xFF, 0xFF, 0x55), (0xFF, 0xFF, 0xFF)]


def palette_colours(pid):
    if pid == "mono":
        return [(0, 0, 0), (255, 255, 255)]
    if pid == "ega16":
        return EGA16
    if pid == "rgb332":
        return [(LEVELS3[i >> 5], LEVELS3[(i >> 2) & 7], LEVELS2[i & 3]) for i in range(256)]
    raise ValueError(pid)


def nearest(pid, c):
    if pid == "rgb24":
        return (c[0] << 16) | (c[1] << 8) | c[2]
    cols = palette_colours(pid)
    return min(range(len(cols)), key=lambda i: (sum((a - b) ** 2 for a, b in zip(cols[i], c)), i))


def canon_image(sw, sh, rgba, pid, tw, th):
    """Composite on black, exact area average (rounded half up), nearest palette colour."""
    rgb = []
    for i in range(sw * sh):
        a = rgba[i * 4 + 3]
        rgb.append(tuple((rgba[i * 4 + k] * a + 127) // 255 for k in range(3)))
    out = []
    for y in range(th):
        for x in range(tw):
            # Target pixel covers [x/tw, (x+1)/tw) x [y/th, (y+1)/th) of the unit square.
            acc = [Fraction(0)] * 3
            for sy in range(sh):
                oy = min(Fraction(y + 1, th), Fraction(sy + 1, sh)) - max(Fraction(y, th), Fraction(sy, sh))
                if oy <= 0:
                    continue
                for sx in range(sw):
                    ox = min(Fraction(x + 1, tw), Fraction(sx + 1, sw)) - max(Fraction(x, tw), Fraction(sx, sw))
                    if ox <= 0:
                        continue
                    for k in range(3):
                        acc[k] += rgb[sy * sw + sx][k] * ox * oy
            area = Fraction(1, tw * th)
            c = tuple(int(v / area + Fraction(1, 2)) for v in acc)  # floor(avg + 1/2)
            out.append(nearest(pid, c))
    return out


# -- M1 sieve filters (direct definition, for brute-force checks at small L)
def load_dict(path):
    words = set()
    with open(path, encoding="utf-8", errors="replace") as f:
        for line in f:
            w = line.strip().lower()
            if w and all("a" <= c <= "z" for c in w):
                words.add(w)
    prefixes, suffixes, substrings = set(), set(), set()
    for w in words:
        for i in range(len(w)):
            suffixes.add(w[i:])
            for j in range(i + 1, len(w) + 1):
                substrings.add(w[i:j])
        for j in range(1, len(w) + 1):
            prefixes.add(w[:j])
    return words, prefixes, suffixes, substrings


def passes(u, filt, d):
    words, prefixes, suffixes, substrings = d
    if "  " in u or u.strip() == "":
        return False
    if filt == "clean":
        return True
    n, i = len(u), 0
    while i < n:
        if u[i] == " ":
            i += 1
            continue
        j = i
        while j < n and u[j] != " ":
            j += 1
        t, ol, orr = u[i:j], i == 0, j == n
        if filt == "words":
            ok = t in words
        elif ol and orr:
            ok = t in substrings
        elif ol:
            ok = t in suffixes
        elif orr:
            ok = t in prefixes
        else:
            ok = t in words
        if not ok:
            return False
        i = j
    return True


def cmd_sieve(args):
    import itertools
    d = load_dict(args.dict)
    syms = ALPHABETS["lower27"]
    for L in range(1, args.max + 1):
        c = {"clean": 0, "window": 0, "words": 0}
        for tup in itertools.product(syms, repeat=L):
            u = "".join(tup)
            for f in c:
                if passes(u, f, d):
                    c[f] += 1
        print(f"{L},{c['clean']},{c['window']},{c['words']}")


def cmd_vectors(_args):
    """Deterministic conformance vectors: alphabet, length, key, unit, positional, scrambled."""
    # Deterministic pseudo-random digits from SHA-256 (stream(), below) so the file is reproducible.
    print("# sieve conformance vectors v1 (spec 2.0, feistel-sha256-v1)")
    print("# alphabet\tlength\tkey\tunit\tpositional\tscrambled")
    cases = []
    for alpha, lengths in (("lower27", [1, 2, 3, 7, 16, 32, 100]),
                           ("babel29", [1, 5, 40]),
                           ("ascii95", [1, 8, 64])):
        for L in lengths:
            for key in ("sieve", "alt-key"):
                sp = Space(alpha, L, key)
                g = stream(f"{alpha}/{L}/{key}")
                units = [" " * L, sp.symbols[-1] * L]
                for _ in range(4):
                    units.append("".join(sp.symbols[next(g) % sp.n] for _ in range(L)))
                cases += [(alpha, L, key, u) for u in units]
    # Readable examples.
    for text in ("it was the best of times", "the library of babel"):
        for L in (32, 1000):
            cases.append(("lower27", L, "sieve", canonicalise(text, "lower27", L)[0]))
    for alpha, L, key, u in cases:
        sp = Space(alpha, L, key)
        assert "\t" not in u and "\n" not in u
        pos, scr = sp.address_of(u, "positional"), sp.address_of(u, "scrambled")
        assert sp.unit_at(pos, "positional") == u and sp.unit_at(scr, "scrambled") == u
        print(f"{alpha}\t{L}\t{key}\t{u}\t{pos}\t{scr}")


def stream(seed):
    k = 0
    while True:
        for b in hashlib.sha256(f"{seed}/{k}".encode()).digest():
            yield b
        k += 1


def cmd_biguint_vectors(_args):
    """BigUint vectors: an operation, its operands and its exact result, from Python's own integers.

    The address path rests entirely on BigUint, so its arithmetic is pinned the same way every
    other rule is: Python works the answers out with its own arbitrary-precision integers, which
    share no code with the C++, and the C++ has to reproduce them exactly. Sizes run from a limb
    or two up past a page, and include the boundaries where a carry or a borrow crosses a limb,
    which is where a big-integer implementation goes wrong if it is going to.
    """
    print("# sieve biguint vectors (exact arithmetic, SPECIFICATIONS 4.3)")
    print("# op\ta\tb\tresult      (a and b hex, result hex unless the op says otherwise)")
    g = stream("biguint/v1")

    def rnd(bits):
        n = int.from_bytes(bytes(next(g) for _ in range((bits + 7) // 8)), "little")
        n |= 1 << (bits - 1)
        return n & ((1 << bits) - 1)

    # Sizes: a limb, either side of a limb boundary, an address, a paragraph, a page.
    values = [0, 1, 2, 255, 256, (1 << 32) - 1, 1 << 32, (1 << 32) + 1, (1 << 64) - 1, 1 << 64,
              (1 << 64) + 1, (1 << 96) - 1, (1 << 128) - 1]
    for bits in (17, 31, 32, 33, 63, 64, 65, 127, 129, 152, 1000, 4755):
        values.append(rnd(bits))
    # Every pair of the small and boundary values, and the big ones against a handful, so the
    # file stays a sensible size while still exercising long operands both ways round.
    small = values[:-2]
    big = values[-2:]
    pairs = [(a, b) for a in small for b in small]
    for x in big:
        for y in (0, 1, values[5], values[8], values[12], small[-1], x):
            pairs.append((x, y))
            pairs.append((y, x))

    def row(op, a, b, result):
        print(f"{op}\t{a:x}\t{b:x}\t{result}")

    for a, b in pairs:
        row("add", a, b, f"{a + b:x}")
        if a >= b: row("sub", a, b, f"{a - b:x}")
        row("mul", a, b, f"{a * b:x}")
        if b: row("divmod", a, b, f"{a // b:x},{a % b:x}")
        row("xor", a, b, f"{a ^ b:x}")
        row("cmp", a, b, "-1" if a < b else "1" if a > b else "0")
    for a in values:
        row("bits", a, 0, str(a.bit_length()))
        for s in (0, 1, 31, 32, 33, 64, 65, 200):
            row(f"shl{s}", a, 0, f"{a << s:x}")
            row(f"shr{s}", a, 0, f"{a >> s:x}")
        row("dec", a, 0, str(a))
        for base in (2, 16, 27, 104, 256, 1000):
            # to_digits over the smallest number of digits that holds it, then back.
            n = 1
            while base ** n <= a: n += 1
            digits = []
            v = a
            for _ in range(n):
                digits.append(v % base)
                v //= base
            row(f"digits{base}", a, 0, ",".join(str(d) for d in reversed(digits)))


def large_operand(spec):
    """An operand of the large BigUint vectors, from its written form.

    The operands run to hundreds of thousands of bits, so the vectors name them instead of writing
    them out, and both sides build them from the name:

        r:BITS:TAG      BITS bits read little-endian from the SHA-256 stream of
                        "biguint-large/v1/TAG" (the stream() above), top bit forced on
        m:BITS          2^BITS - 1, every bit set, which is where carries run longest
        s:BITS          2^(BITS-1) + 1, the two ends set and nothing between
        p:BASE:EXP:D    BASE^EXP + D, for D in -1, 0, 1: every digit BASE-1, or a one and zeros
    """
    f = spec.split(":")
    if f[0] == "r":
        bits = int(f[1])
        g = stream("biguint-large/v1/" + f[2])
        n = int.from_bytes(bytes(next(g) for _ in range((bits + 7) // 8)), "little")
        return (n & ((1 << bits) - 1)) | (1 << (bits - 1))
    if f[0] == "m":
        return (1 << int(f[1])) - 1
    if f[0] == "s":
        return (1 << (int(f[1]) - 1)) + 1
    if f[0] == "p":
        return int(f[1]) ** int(f[2]) + int(f[3])
    raise ValueError(spec)


def cmd_biguint_large_vectors(_args):
    """BigUint vectors at the lengths where the fast algorithms run.

    vectors_biguint_v1.tsv stops at 4,755 bits, which is below where multiplication turns to
    Karatsuba, division to a reciprocal and the base conversions to splitting in halves. These rows
    go to 400,000 bits, across each of those thresholds and the lopsided shapes between them. The
    results are far too long to write down, so each row gives the SHA-256 of the result's text in
    the same form vectors_biguint_v1.tsv would have used; the operands are named, as large_operand()
    describes, rather than written out.
    """
    sys.set_int_max_str_digits(0)
    print("# sieve large biguint vectors (exact arithmetic, SPECIFICATIONS 4.3)")
    print("# op\ta\tb\tsha256 of the result text      (operands as reference/sieve_ref.py large_operand())")
    rows = []
    # Multiplication: square and lopsided, either side of the Karatsuba threshold (32 limbs), at a
    # page, at 152,000 bits (a pages-line address) and past it, with all-ones and sparse operands.
    for a, b in (("r:2000:a", "r:2100:b"), ("r:4000:a", "r:4000:b"), ("r:4100:a", "r:2050:b"),
                 ("m:40000", "m:40000"), ("s:40000", "s:40000"), ("m:40000", "r:7000:c"),
                 ("r:152000:a", "r:152000:b"), ("r:152000:a", "r:5000:c"), ("r:152000:a", "r:151937:d"),
                 ("r:400000:a", "r:400000:b"), ("r:400000:a", "r:10300:c"), ("m:400000", "m:399999"),
                 ("p:27:30000:-1", "p:27:30000:-1")):
        rows.append(("mul", a, b))
    # Division: Knuth's algorithm and Barrett's either side of their thresholds (160 limbs with a
    # quotient twice as long, 1,000 limbs with one as long), divisors of every shape, exact
    # quotients, a quotient of one and of nothing.
    for a, b in (("r:30000:a", "r:10000:b"), ("r:20000:a", "r:10300:b"), ("r:31000:a", "r:10300:b"),
                 ("r:128000:a", "r:64000:b"), ("r:130000:a", "r:66000:b"), ("r:400000:a", "r:66000:b"),
                 ("r:400000:a", "r:150000:b"), ("r:304000:a", "r:152000:b"), ("m:304000", "m:152000"),
                 ("m:304000", "s:152000"), ("s:304000", "m:152000"), ("r:304000:a", "m:100000"),
                 ("p:27:60000:0", "p:27:30000:0"), ("p:27:60000:-1", "p:27:30000:0"),
                 ("p:27:60000:1", "p:27:30000:-1"), ("r:152000:a", "r:152000:a"), ("r:152000:b", "r:152000:a")):
        rows.append(("divmod", a, b))
    # Conversions: the operand itself (which checks how each side builds it), decimal, and digits
    # in bases with one digit per limb and many, over random values and over the extremes of a
    # base (every digit its largest, a one followed by zeros).
    for a in ("r:152000:a", "p:27:32000:-1", "p:27:32000:0", "p:27:32000:1", "p:10:40000:-1", "p:10:40000:0",
              "m:152000", "s:152000", "r:400000:a"):
        rows.append(("hex", a, "-"))
    for a in ("r:2600:a", "r:152000:a", "p:10:40000:-1", "p:10:40000:0", "m:152000", "r:400000:a"):
        rows.append(("dec", a, "-"))
    for base, vals in ((27, ("r:2600:a", "r:152000:a", "p:27:32000:-1", "p:27:32000:0", "p:27:32000:1", "r:400000:a")),
                       (3, ("r:152000:a", "p:3:90000:-1")), (10, ("r:152000:a",)), (95, ("r:152000:a", "p:95:20000:-1")),
                       (104, ("r:152000:a",)), (1000, ("r:152000:a",)), (65537, ("r:152000:a", "p:65537:9000:-1")),
                       (4294967295, ("r:152000:a", "p:4294967295:4000:-1"))):
        for a in vals:
            rows.append((f"digits{base}", a, "-"))

    sha = lambda t: hashlib.sha256(t.encode()).hexdigest()
    for op, sa, sb in rows:
        a = large_operand(sa)
        b = None if sb == "-" else large_operand(sb)
        if op == "mul":
            out = f"{a * b:x}"
        elif op == "divmod":
            out = f"{a // b:x},{a % b:x}"
        elif op == "hex":
            out = f"{a:x}"
        elif op == "dec":
            out = str(a)
        else:
            base = int(op[6:])
            digits = []
            v = a
            while True:
                v, d = divmod(v, base)
                digits.append(d)
                if v == 0:
                    break
            out = ",".join(str(d) for d in reversed(digits))
        print(f"{op}\t{sa}\t{sb}\t{sha(out)}")


def cmd_digit_vectors(_args):
    """Address vectors over abstract bases (image palettes, notes, video): base, length, key, digits, pos, scr."""
    print("# sieve digit conformance vectors v1 (feistel-sha256-v1)")
    print("# base\tlength\tkey\tdigits\tpositional\tscrambled")
    for base, lengths in ((2, [1, 25, 100, 200]), (16, [1, 100]), (104, [1, 16]), (256, [100]),
                          (16777216, [1, 4, 100])):
        for L in lengths:
            for key in ("sieve", "alt-key"):
                g = stream(f"digits/{base}/{L}/{key}")
                units = [[0] * L, [base - 1] * L]
                for _ in range(3):
                    units.append([int.from_bytes(bytes(next(g) for _ in range(4)), "little") % base for _ in range(L)])
                for d in units:
                    sp = Space(None, L, key, base=base)
                    pos, scr = sp.address_of(d, "positional"), sp.address_of(d, "scrambled")
                    assert sp._digits(int(scr, 16)) == sp.scramble(d)
                    print(f"{base}\t{L}\t{key}\t{','.join(map(str, d))}\t{pos}\t{scr}")


def cmd_bytes_vectors(_args):
    """The binary line: a file as units of a bytes256 line (canon-bytes-v1), and their addresses.

    Each row is an input file, the line it is cut for, the units that come out (as hex, since a
    file holds tabs, line ends and NULs that a vector file cannot) and each unit's positional and
    scrambled address. On bytes256 the positional address is the unit's own bytes read as one
    number, so that column is the hex dump of the unit: the demonstration that the addressing is
    honest.
    """
    print("# sieve binary-line vectors v1 (canon-bytes-v1 over feistel-sha256-v1)")
    print("# alphabet\tlength\tkey\tfile_hex\tunits_hex\tpositional\tscrambled   (units and addresses comma-separated)")
    g = stream("bytes/v1")
    noise = bytes(next(g) for _ in range(1000))
    files = [
        b"", b"\x00", b"\xff", b"\xde\xad\xbe\xef", bytes(range(256)),
        b"tab\there\r\nline\nnul\x00end",           # what text canonicalisation would change
        "caf\u00e9 \u6771\u4eac".encode("utf-8"),        # UTF-8 that must not be decoded
        b"\xc0\xaf\xed\xa0\x80\x80",                     # and bytes that are not UTF-8 at all
        noise,
    ]
    for data in files:
        for alpha, L, key in (("bytes256", 1, "sieve"), ("bytes256", 4, "sieve"), ("bytes256", 7, "other"),
                              ("bytes256", 256, "sieve")):
            if L == 1 and len(data) > 16:
                continue  # a file cut into single bytes adds nothing past a few rows
            sp = Space(alpha, L, key)
            units = canonicalise_bytes(data, alpha, L)
            uh = ",".join(bytes(ord(c) for c in u).hex() for u in units)
            pos = ",".join(sp.address_of(u, "positional") for u in units)
            scr = ",".join(sp.address_of(u, "scrambled") for u in units)
            print(f"{alpha}\t{L}\t{key}\t{data.hex() or '-'}\t{uh}\t{pos}\t{scr}")


def cmd_canon_vectors(_args):
    """Text canonicalisation vectors: version, alphabet, length, input (UTF-8 hex), units (hex, comma-separated)."""
    inputs = [
        "It was the best of times, it was the worst of times.",
        "A well-known author/editor didn't stop \u2014 na\u00efve caf\u00e9.",
        "\u201cQuoted\u201d text\u2026 and \u2018single\u2019 quotes \u00ab guillemets \u00bb",
        "\u00c6sop's \u0152uvre: \u00df, \u00f8, \u0142, \u00fe, \u0131, \u0133 \u2212 5 \u2013 10",
        "tabs\tand\nnewlines\r\n\u00a0nbsp   runs",
        "\u6771\u4eac and \U0001f600 emoji and 1984 and snake_case and e-mail",
        "   ", "!!!", "\u00c0\u00c9\u00ce\u00d5\u00dc \u00e0\u00e9\u00ee\u00f5\u00fc \u017f\u017e",
    ]
    # Every letter of the fold range, so the C++ table is checked entry by entry.
    inputs.append(" ".join(chr(c) for c in range(0xC0, 0x180)))
    print("# sieve text canonicalisation vectors (canon-text-v1 and canon-text-v2)")
    print("# version\talphabet\tlength\tinput_hex\tunits_hex")
    for text in inputs:
        for version in ("v1", "v2"):
            for alpha, L in (("lower27", 16), ("babel29", 24), ("ascii95", 20), ("ascii96", 20)):
                units = canonicalise(text, alpha, L, version)
                uh = ",".join(u.encode("utf-8").hex() for u in units)
                print(f"{version}\t{alpha}\t{L}\t{text.encode('utf-8').hex()}\t{uh}")


class ModelSpace:
    """The models line (modelspace-v1), written independently of the C++.

    V vertices of 3 coordinates on a grid of C cells across [-1, 1], then F faces of 3 vertex
    indices, read as one mixed-radix number, most significant first."""

    def __init__(self, vertices=8, faces=12, coords=16, key="sieve"):
        assert vertices >= 3 and faces >= 1
        assert 2 <= coords <= 4096 and coords & (coords - 1) == 0
        self.v, self.f, self.c, self.key = vertices, faces, coords, key
        self.decimals = coords.bit_length() - 1
        self.size = coords ** (3 * vertices) * vertices ** (3 * faces)
        self.hex_width = max(1, ((self.size - 1).bit_length() + 3) // 4)
        self.id = f"models/V{vertices}/F{faces}/C{coords}/key={key}/modelspace-v1"

    def index_of(self, verts, faces, mode="positional"):
        assert len(verts) == 3 * self.v and len(faces) == 3 * self.f
        n = 0
        for d in verts:
            assert 0 <= d < self.c
            n = n * self.c + d
        for d in faces:
            assert 0 <= d < self.v
            n = n * self.v + d
        return shuffle(self.key, self.id, self.size, n) if mode == "scrambled" else n

    def parts_at(self, index, mode="positional"):
        assert 0 <= index < self.size
        n = shuffle(self.key, self.id, self.size, index, inverse=True) if mode == "scrambled" else index
        faces = [0] * (3 * self.f)
        for i in range(3 * self.f - 1, -1, -1):
            n, faces[i] = divmod(n, self.v)
        verts = [0] * (3 * self.v)
        for i in range(3 * self.v - 1, -1, -1):
            n, verts[i] = divmod(n, self.c)
        return verts, faces

    def coord(self, d):
        """Exactly (2d + 1 - C) / C, as a Fraction."""
        return Fraction(2 * d + 1 - self.c, self.c)

    def coord_text(self, d):
        x = self.coord(d)
        units = x * 10 ** self.decimals
        assert units.denominator == 1, "the grid is not a terminating decimal"
        units = int(units)
        sign = "-" if units < 0 else "+"
        mag = abs(units)
        whole, frac = divmod(mag, 10 ** self.decimals)
        return f"{sign}{whole}.{frac:0{self.decimals}d}"

    def to_obj(self, verts, faces):
        """The canonical .obj: fixed width, line feeds only, and no two spaces in a row, so it
        survives canon-text-v2 on an ascii96 line unchanged."""
        width = len(str(self.v))
        out = []
        for i in range(self.v):
            out.append("v " + " ".join(self.coord_text(verts[3 * i + k]) for k in range(3)))
        for i in range(self.f):
            out.append("f " + " ".join(f"{faces[3 * i + k] + 1:0{width}d}" for k in range(3)))
        return "\n".join(out) + "\n"

    def obj_length(self):
        return self.v * (2 + 3 * (self.decimals + 3) + 2 + 1) + self.f * (2 + 3 * len(str(self.v)) + 2 + 1)


def cmd_model_vectors(_args):
    """Models line vectors: shape, key, mode, vertex digits, face digits, address, .obj sha256."""
    print("# sieve models line vectors (modelspace-v1)")
    print("# vertices\tfaces\tcoords\tkey\tmode\tverts\tfaces_digits\taddress\tobj_sha256")
    shapes = [(8, 12, 16, "sieve"), (8, 12, 16, "alt-key"), (4, 2, 4, "sieve"), (3, 1, 2, "sieve"),
              (12, 20, 64, "sieve")]
    for v, f, c, key in shapes:
        sp = ModelSpace(v, f, c, key)
        g = stream(f"{sp.id}")
        cases = []
        cases.append(([0] * (3 * v), [0] * (3 * f)))
        cases.append(([c - 1] * (3 * v), [v - 1] * (3 * f)))
        for _ in range(3):
            cases.append(([next(g) % c for _ in range(3 * v)], [next(g) % v for _ in range(3 * f)]))
        for verts, faces in cases:
            obj = sp.to_obj(verts, faces)
            assert len(obj) == sp.obj_length()
            assert "  " not in obj, "the canonical form must have no two spaces in a row"
            for mode in ("positional", "scrambled"):
                k = sp.index_of(verts, faces, mode)
                assert sp.parts_at(k, mode) == (verts, faces)
                addr = f"{k:0{sp.hex_width}x}"
                vs = ",".join(str(d) for d in verts)
                fs = ",".join(str(d) for d in faces)
                print(f"{v}\t{f}\t{c}\t{key}\t{mode}\t{vs}\t{fs}\t{addr}\t{hashlib.sha256(obj.encode()).hexdigest()}")


def cmd_alphabet_vectors(_args):
    """Alphabets built from Unicode blocks and ranges: spec, size, length, key, unit, addresses.

    The unit is written as its code points, not as UTF-8, because these lines can hold line
    feeds and surrogates - the very things a text form cannot carry."""
    print("# sieve alphabet vectors (stacked blocks and ranges)")
    print("# spec\tsize\tlength\tkey\tunit_cp\tpositional\tscrambled")
    specs = ["ascii96", "greek", "cyrillic+greek", "greek+cyrillic", "hiragana+katakana",
             "ascii+surrogates", "u+0370-u+03ff", "all-emojis", "u+00e9+u+00fc"]
    for spec in specs:
        syms = alphabet(spec)
        for L in (1, 4, 16):
            for key in ("sieve", "alt-key"):
                sp = Space(spec, L, key)
                g = stream(f"{spec}/{L}/{key}")
                units = [syms[0] * L, syms[-1] * L]
                for _ in range(3):
                    units.append("".join(syms[next(g) % sp.n] for _ in range(L)))
                for u in units:
                    cp = ",".join(f"{ord(c):04x}" for c in u)
                    pos, scr = sp.address_of(u, "positional"), sp.address_of(u, "scrambled")
                    assert sp.unit_at(pos, "positional") == u and sp.unit_at(scr, "scrambled") == u
                    print(f"{spec}\t{sp.n}\t{L}\t{key}\t{cp}\t{pos}\t{scr}")


def cmd_image_vectors(_args):
    """Image canonicalisation vectors: palette, srcW, srcH, dstW, dstH, rgba hex, digits."""
    print("# sieve image canonicalisation vectors (canon-image-v1)")
    print("# palette\tsrc_w\tsrc_h\tdst_w\tdst_h\trgba_hex\tdigits")
    cases = [(1, 1, 3, 2), (3, 2, 1, 1), (4, 4, 4, 4), (13, 7, 10, 10), (7, 13, 5, 5), (10, 10, 3, 3),
             (2, 3, 7, 5), (16, 16, 10, 10), (5, 5, 16, 16)]
    for pid in ("mono", "ega16", "rgb332", "rgb24"):
        for n, (sw, sh, tw, th) in enumerate(cases):
            g = stream(f"image/{pid}/{n}")
            rgba = []
            for _ in range(sw * sh):
                r, gg, b, a = next(g), next(g), next(g), next(g)
                a = 255 if a < 160 else (0 if a < 180 else a)  # mostly opaque, some transparent
                rgba += [r, gg, b, a]
            digits = canon_image(sw, sh, rgba, pid, tw, th)
            print(f"{pid}\t{sw}\t{sh}\t{tw}\t{th}\t{bytes(rgba).hex()}\t{','.join(map(str, digits))}")


# -- entropy-ordered addressing (sieve-charmodel-v1, witten-bell-v1, guided-ac-v1), written
#    independently of the C++ core: plain dicts, Python integers, no shared code.
TOTAL = 1 << 16
MODEL_HEADER = ["symbols", "base", "order", "min_count", "total", "smoothing", "padding",
                "trained_symbols", "corpus", "contexts"]


def corpus_manifest(path):
    rows = []
    for line in open(path, encoding="utf-8"):
        line = line.rstrip("\r\n")
        if line and not line.startswith("#"):
            rows.append(line.split("\t"))
    return rows


def training_text(manifest, texts, alphabet_id):
    """The train files in manifest order, each canonicalised (canon-text-v2), joined by one SPACE."""
    parts = []
    for name, enc, role, sha in corpus_manifest(manifest):
        if role != "train":
            continue
        raw = open(f"{texts}/{name}", "rb").read()
        assert hashlib.sha256(raw).hexdigest() == sha, name
        text = raw.decode("latin-1" if enc == "latin-1" else "utf-8")
        s = "".join(canonicalise(text, alphabet_id, 1))
        if s:
            parts.append(s)
    return " ".join(parts), sum(1 for _ in parts)


def build_model(stream, symbols, order, min_count, corpus):
    """Counts every context of up to `order` symbols; keeps those seen >= min_count times whose parent is kept."""
    n = len(symbols)
    idx = {c: i for i, c in enumerate(symbols)}
    digits = [idx[c] for c in stream]
    counts = {}
    for t in range(len(digits)):
        for k in range(0, min(order, t) + 1):
            ctx = tuple(digits[t - k:t])
            row = counts.get(ctx)
            if row is None:
                row = counts[ctx] = [0] * n
            row[digits[t]] += 1
    kept = []
    keep = set()
    for ctx in sorted(counts, key=lambda c: (len(c), c)):
        if ctx and (sum(counts[ctx]) < min_count or ctx[1:] not in keep):
            continue
        keep.add(ctx)
        kept.append((ctx, [(s, c) for s, c in enumerate(counts[ctx]) if c]))
    pad = idx.get(" ")
    head = {"symbols": None, "base": n, "order": order, "min_count": min_count, "total": TOTAL,
            "smoothing": "witten-bell-v1", "padding": "none" if pad is None else pad,
            "trained_symbols": len(digits), "corpus": corpus, "contexts": len(kept)}
    return head, kept


def write_model(alphabet_id, head, kept):
    head = dict(head, symbols=alphabet_id)
    out = ["sieve-charmodel-v1"] + [f"{k} {head[k]}" for k in MODEL_HEADER]
    for ctx, row in kept:
        c = "".join(f"{d:02x}" for d in ctx) if ctx else "-"
        out.append(c + "\t" + " ".join(f"{s}:{n}" for s, n in row))
    return ("\n".join(out) + "\n").encode("utf-8")


class Model:
    """Loads a model file and derives every frequency table exactly as SPECIFICATIONS 4.2 says."""

    def __init__(self, path):
        raw = open(path, "rb").read()
        self.sha256 = hashlib.sha256(raw).hexdigest()
        lines = raw.decode("utf-8").split("\n")
        assert lines[0] == "sieve-charmodel-v1" and lines[-1] == ""
        head = {}
        for key, line in zip(MODEL_HEADER, lines[1:11]):
            k, _, v = line.partition(" ")
            assert k == key, (k, key)
            head[key] = v
        self.n = int(head["base"])
        self.order = int(head["order"])
        self.symbols = head["symbols"]
        pad = None if head["padding"] == "none" else int(head["padding"])
        assert int(head["total"]) == TOTAL and head["smoothing"] == "witten-bell-v1"
        self.tables = {}
        for line in lines[11:-1]:
            c, row = line.split("\t")
            ctx = () if c == "-" else tuple(int(c[i:i + 2], 16) for i in range(0, len(c), 2))
            counts = [0] * self.n
            for item in row.split(" "):
                s, v = item.split(":")
                counts[int(s)] = int(v)
            total = sum(counts)
            if not ctx:
                a, d = [x + 1 for x in counts], total + self.n
            else:
                parent = self.tables[ctx[1:]]
                u = sum(1 for x in counts if x)
                a = [counts[s] * TOTAL + u * parent[s] for s in range(self.n)]
                d = (total + u) * TOTAL
            self.tables[ctx] = self.quantise(a, d)
        assert len(self.tables) == int(head["contexts"])
        if pad is not None and self.order >= 2:
            self.tables[(pad, pad)] = [TOTAL - (self.n - 1) if s == pad else 1 for s in range(self.n)]

    def quantise(self, a, d):
        free = TOTAL - self.n
        f = [1 + free * x // d for x in a]
        rem = [free * x % d for x in a]
        for s in sorted(range(self.n), key=lambda s: (-rem[s], s))[:TOTAL - sum(f)]:
            f[s] += 1
        assert sum(f) == TOTAL and min(f) >= 1
        return f

    def table(self, history):
        h = tuple(history[max(0, len(history) - self.order):])
        while h not in self.tables:
            h = h[1:]
        return self.tables[h]


def guided_interval(model, unit):
    low, width = 0, 1
    for i, s in enumerate(unit):
        f = model.table(unit[:i])
        low = low * TOTAL + sum(f[:s]) * width
        width *= f[s]
    return low, width


def guided_code(model, unit):
    """(point, bits): the lowest of the largest aligned blocks inside the unit's arc."""
    S = 16 * len(unit)
    low, width = guided_interval(model, unit)
    high = low + width
    for t in range(S, -1, -1):
        m = -(-low // (1 << t))
        if (m + 1) << t <= high:
            return m << t, S - t
    raise AssertionError("no block fits")


def guided_hex(point, bits, S):
    if bits == 0:
        return "0"
    nh = (bits + 3) // 4
    return format(point >> (S - 4 * nh), "0%dx" % nh)


def guided_unit_at(model, length, point):
    """The unit whose arc contains the point: walk down the tree, choosing the child arc that holds it."""
    unit = []
    low, width = 0, 1  # the arc so far, in units of TOTAL^-depth
    for i in range(length):
        f = model.table(unit)
        scale = TOTAL ** (length - 1 - i)  # from depth i+1 units to points
        cum = 0
        for s in range(model.n):
            if (low * TOTAL + (cum + f[s]) * width) * scale > point:
                break
            cum += f[s]
        unit.append(s)
        low = low * TOTAL + cum * width
        width *= f[s]
        assert low * scale <= point < (low + width) * scale
    return unit


def cmd_model_build(args):
    text, files = training_text(args.corpus, args.texts, args.alphabet)
    manifest_sha = hashlib.sha256(open(args.corpus, "rb").read()).hexdigest()
    name = args.corpus.replace("\\", "/").split("/")[-1]
    corpus = f"{name} sha256={manifest_sha} train_files={files} canon=canon-text-v2"
    head, kept = build_model(text, alphabet(args.alphabet), args.order, args.min_count, corpus)
    data = write_model(args.alphabet, head, kept)
    open(args.out, "wb").write(data)
    print(hashlib.sha256(data).hexdigest(), args.out)


def cmd_guided_vectors(args):
    """Guided address vectors: length, unit (digits), code bits, code (hex); and points -> units."""
    model = Model(args.model)
    sym = alphabet(model.symbols)
    print(f"# sieve guided conformance vectors v1 (guided-ac-v1, sieve-charmodel-v1, witten-bell-v1)")
    print(f"# model sha256 {model.sha256}")
    print("# kind\tlength\tunit\tbits\taddress")
    cases = []
    for text in ("it was the best of times it was the worst of times", "the library of babel",
                 "call me ishmael", "qzxj vvkw pqpq zzz", "a", ""):
        for L in (1, 8, 32, 100):
            units = canonicalise(text, model.symbols, L) or [" " * L]
            cases.append((L, units[0]))
    for L in (3, 16, 64):
        g = stream(f"guided/{L}")
        cases.append((L, "".join(sym[next(g) % model.n] for _ in range(L))))
        cases.append((L, " " * L))
        cases.append((L, sym[-1] * L))
    for L, u in cases:
        d = [sym.index(c) for c in u]
        point, bits = guided_code(model, d)
        h = guided_hex(point, bits, 16 * L)
        assert guided_unit_at(model, L, point) == d
        print(f"code\t{L}\t{u}\t{bits}\t{h}")
    # Points: arbitrary hex fractions decode to the unit whose arc holds them.
    for L in (4, 32, 100):
        g = stream(f"guided-points/{L}")
        for k in range(6):
            nh = [1, 3, 8, 16, 40, 4 * L + 2][k]
            h = "".join("0123456789abcdef"[next(g) % 16] for _ in range(nh))
            S = 16 * L
            v = int(h, 16)
            point = v << (S - 4 * nh) if 4 * nh <= S else v >> (4 * nh - S)
            u = "".join(sym[s] for s in guided_unit_at(model, L, point))
            print(f"point\t{L}\t{u}\t-\t{h}")


# -- the filtration stack (filters and rankers), written from their definitions: plain Python
#    integers, no shared code with the core.
def log2_q16(x):
    """floor(log2(x) * 2^16) by the pinned integer steps (sieve/intlog.hpp)."""
    n = x.bit_length() - 1
    m = x << (62 - n) if n <= 62 else x >> (n - 62)
    r = n << 16
    for i in range(16):
        m = (m * m) >> 62
        if m >= 2 << 62:
            m >>= 1
            r += 1 << (15 - i)
    return r


def f_max_run(u, max_run, space=0):
    run = 0
    for i, s in enumerate(u):
        run = run + 1 if i and s == u[i - 1] and s != space else 1
        if run > max_run:
            return False
    return True


def f_symbol_entropy(u, lo, hi):
    from collections import Counter
    L = len(u)
    hl = max(0, L * log2_q16(L) - sum(c * log2_q16(c) for c in Counter(u).values()))
    return lo * L * 65536 <= 1000 * hl <= hi * L * 65536


def f_model_information(u, model, max_mb):
    cost = 0
    for i, s in enumerate(u):
        f = model.table(u[:i])
        cost += 16 * 65536 - log2_q16(f[s])
    return cost * 1000 <= max_mb * len(u) * 65536


def f_neighbour_agreement(u, w, h, frames, min_pm):
    eq = pairs = 0
    fr = w * h
    for f in range(frames):
        for y in range(h):
            for x in range(w):
                i = f * fr + y * w + x
                for ok, j in ((x + 1 < w, i + 1), (y + 1 < h, i + w), (f + 1 < frames, i + fr)):
                    if ok:
                        pairs += 1
                        eq += u[i] == u[j]
    return pairs == 0 or eq * 1000 >= min_pm * pairs


class WordsRank:
    """Rank/unrank of words (or clean) survivors in address order, by memoised recursion over
    (automaton state, symbols left): an independent method from the core's word-length tables."""

    def __init__(self, words, L, clean=False, padding=False):
        self.L, self.clean, self.padding = L, clean, padding
        self.words = set(words)
        self.prefixes = {w[:i] for w in words for i in range(len(w) + 1)}
        self.memo = {}

    def step(self, state, c):
        # states: ("start",), ("sp0",) space before any letter, ("w", prefix), ("sp",) space after a
        # word, ("pad",) two or more trailing spaces (version 2 only; nothing may follow)
        ch = " abcdefghijklmnopqrstuvwxyz"[c]
        kind = state[0]
        if kind == "pad":
            return ("pad",) if ch == " " else None
        if ch == " ":
            if kind == "start":
                return ("sp0",)
            if kind == "sp" and self.padding:
                return ("pad",)
            if kind == "w" and (self.clean or state[1] in self.words):
                return ("sp",)
            return None
        if self.clean:
            return ("w", "")
        p = (state[1] if kind == "w" else "") + ch
        return ("w", p) if p in self.prefixes else None

    def accept(self, state):
        if state[0] == "pad":
            return True
        if self.clean:
            return state[0] in ("w", "sp")
        return (state[0] == "w" and state[1] in self.words) or state[0] == "sp"

    def count(self, state, r):
        key = (state, r)
        if key not in self.memo:
            if r == 0:
                v = 1 if self.accept(state) else 0
            else:
                v = 0
                for c in range(27):
                    t = self.step(state, c)
                    if t is not None:
                        v += self.count(t, r - 1)
            self.memo[key] = v
        return self.memo[key]

    def total(self):
        return self.count(("start",), self.L)

    def unrank(self, k):
        s, out = ("start",), []
        for i in range(self.L):
            r = self.L - i
            for c in range(27):
                t = self.step(s, c)
                if t is None:
                    continue
                n = self.count(t, r - 1)
                if k < n:
                    out.append(c)
                    s = t
                    break
                k -= n
        return out


class WindowRank:
    """Rank/unrank of window survivors (v1, or v2 with padding) by memoised recursion over an
    automaton built from plain sets of substrings, suffixes and prefixes of the words: an
    independent method from the core's trie and suffix-range histograms."""

    def __init__(self, d, L, padding=False):
        self.words, self.prefixes, self.suffixes, self.substrings = d
        self.L, self.padding = L, padding
        self.memo = {}

    def step(self, st, c):
        ch = " abcdefghijklmnopqrstuvwxyz"[c]
        k = st[0]
        if k in ("pp", "pad"):
            return ("pad",) if ch == " " else None
        if ch == " ":
            if k == "start":
                return ("sp0",)
            if k == "sp":
                return ("pad",) if self.padding else None
            if k == "sp0":
                return None
            whole = st[1] in (self.suffixes if k == "edge" else self.words)
            if whole:
                return ("sp",)
            return ("pp",) if self.padding else None
        if k == "start":
            return ("edge", ch) if ch in self.substrings else None
        if k == "edge":
            p = st[1] + ch
            return ("edge", p) if p in self.substrings else None
        p = (st[1] if k == "w" else "") + ch
        return ("w", p) if p in self.prefixes else None

    def accept(self, st):
        return st[0] in ("edge", "w", "sp", "pad")

    def count(self, st, r):
        key = (st, r)
        if key not in self.memo:
            if r == 0:
                v = 1 if self.accept(st) else 0
            else:
                v = 0
                for c in range(27):
                    t = self.step(st, c)
                    if t is not None:
                        v += self.count(t, r - 1)
            self.memo[key] = v
        return self.memo[key]

    def total(self):
        return self.count(("start",), self.L)

    def unrank(self, k):
        st, out = ("start",), []
        for i in range(self.L):
            for c in range(27):
                t = self.step(st, c)
                if t is None:
                    continue
                n = self.count(t, self.L - i - 1)
                if k < n:
                    out.append(c)
                    st = t
                    break
                k -= n
        return out


def passes_title(u, max_length, d):
    """title-v1: words (as words-v2) in the first N = min(max_length, L) symbols, then SPACEs."""
    n = min(max_length, len(u))
    return u[n:].strip(" ") == "" and passes_v2(u[:n], "words", d)


def passes_v2(u, filt, d):
    """clean-v2 / words-v2: as v1, but two or more trailing SPACEs are padding (the prefix is judged)."""
    t = u.rstrip(" ")
    return passes(t, filt, d) if len(u) - len(t) >= 2 else passes(u, filt, d)


def cmd_filter_vectors(_args):
    """Filter verdicts, fixed-point logarithms, and ranks of words/clean survivors."""
    here = "../data"
    model = Model(f"{here}/models/gutenberg-lower27-o5.model")
    dict_file = "scowl-2020.12.07-en-35.txt"
    d = load_dict(f"{here}/dictionaries/{dict_file}")
    sym = ALPHABETS["lower27"]
    print("# sieve filter conformance vectors v1 (clean-v1 window-v1 words-v1 clean-v2 words-v2 window-v2 title-v1 max-run-v1 symbol-entropy-v1")
    print("#   model-information-v1 neighbour-agreement-v1), dictionary scowl-en-35, model gutenberg-lower27-o5")
    print(f"# dictionary {dict_file}")
    print("# log2 <x> <floor(log2(x)*65536)>")
    for x in [1, 2, 3, 5, 7, 10, 27, 100, 1000, 65535, 65536, 65537, 10 ** 9, 2 ** 40 + 12345, 2 ** 63, 2 ** 64 - 1]:
        print(f"log2\t{x}\t{log2_q16(x)}")
    print("# verdict <filter> <params> <length> <unit> <1|0>")
    texts = ["it was the best of times", "the quick brown fox", "qzxj vvkw pqpq zzzz", "a  b", " an ant ", "aaaa bbb",
             "zzz", "call me ishmael", "xylophone", "tan tan tan", "hello world", "lorem ipsum dolor"]
    g = stream("filters")
    for L in (8, 32):
        units = [canonicalise(t, "lower27", L)[0] for t in texts]
        units += ["".join(sym[next(g) % 27] for _ in range(L)) for _ in range(6)]
        units.append(" " * L)
        units += [t.ljust(L)[:L] for t in ("ant", " an", "tan  x", "i ", "xqz")]  # padding edge cases
        for u in units:
            digits = [sym.index(c) for c in u]
            for f in ("clean", "window", "words"):
                print(f"verdict\t{f}-v1\tdictionary=scowl-en-35\t{L}\t{u}\t{int(passes(u, f, d))}")
            for f in ("clean", "words", "window"):
                print(f"verdict\t{f}-v2\tdictionary=scowl-en-35\t{L}\t{u}\t{int(passes_v2(u, f, d))}")
            for mx in (4, 12, 24):
                print(f"verdict\ttitle-v1\tdictionary=scowl-en-35,max_length={mx}\t{L}\t{u}\t{int(passes_title(u, mx, d))}")
            for mr in (2, 3):
                print(f"verdict\tmax-run-v1\tmax_run={mr}\t{L}\t{u}\t{int(f_max_run(digits, mr))}")
            for lo, hi in ((0, 4400), (2000, 3900), (3500, 32000)):
                print(f"verdict\tsymbol-entropy-v1\tmin_millibits={lo},max_millibits={hi}\t{L}\t{u}\t{int(f_symbol_entropy(digits, lo, hi))}")
            for mx in (2500, 5000, 9000):
                print(f"verdict\tmodel-information-v1\tmax_millibits={mx}\t{L}\t{u}\t{int(f_model_information(digits, model, mx))}")
    print("# image <w> <h> <frames> <min_permille> <pixels> <1|0>")
    for w, h, fr in ((10, 10, 1), (5, 5, 8), (3, 4, 2)):
        for k in range(5):
            px = [next(g) % 2 for _ in range(w * h * fr)] if k < 3 else [k % 2] * (w * h * fr)
            if k == 1:
                px = [(i % w + i // w) % 2 for i in range(w * h * fr)]
            for pm in (500, 600, 900):
                print(f"image\t{w}\t{h}\t{fr}\t{pm}\t{''.join(map(str, px))}\t{int(f_neighbour_agreement(px, w, h, fr, pm))}")
    print("# rank <filter> <length> <count> <rank> <unit>")
    words = sorted(d[0])
    for f, v, L in (("clean", 1, 12), ("words", 1, 5), ("words", 1, 12), ("clean", 2, 12), ("words", 2, 5), ("words", 2, 12)):
        rk = WordsRank(words, L, clean=(f == "clean"), padding=(v == 2))
        total = rk.total()
        g2 = stream(f"rank/{f}/{L}" if v == 1 else f"rank/{f}-v{v}/{L}")
        ks = [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(16)), "little") % total for _ in range(6)]
        for k in ks:
            u = "".join(sym[c] for c in rk.unrank(k))
            assert (passes if v == 1 else passes_v2)(u, f, d)
            print(f"rank\t{f}-v{v}\t{L}\t{total}\t{k}\t{u}")
    for v, L in ((1, 8), (2, 8), (1, 12), (2, 12)):
        rk = WindowRank(d, L, padding=(v == 2))
        total = rk.total()
        g2 = stream(f"rank/window-v{v}/{L}")
        for k in [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(16)), "little") % total for _ in range(6)]:
            u = "".join(sym[c] for c in rk.unrank(k))
            assert (passes if v == 1 else passes_v2)(u, "window", d)
            print(f"rank\twindow-v{v}\t{L}\t{total}\t{k}\t{u}")
    print("# trank <max_length> <length> <count> <rank> <unit>   (title-v1)")
    for mx, L in ((12, 32), (20, 32)):
        rk = WordsRank(words, mx, padding=True)
        total = rk.total()
        g2 = stream(f"rank/title/{mx}/{L}")
        for k in [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(16)), "little") % total for _ in range(6)]:
            u = "".join(sym[c] for c in rk.unrank(k)).ljust(L)
            assert passes_title(u, mx, d)
            print(f"trank\t{mx}\t{L}\t{total}\t{k}\t{u}")


# -- compact orderings (SPECIFICATIONS 8.5, 9): rankers on the other lines, the survivor shuffle,
#    and the sieved guided line, each written from its definition.
def entropy_allowed(L, lo, hi):
    """Two symbols: the numbers of 1s whose units pass symbol-entropy-v1."""
    return [k for k in range(L + 1) if f_symbol_entropy([1] * k + [0] * (L - k), lo, hi)]


class BinaryEntropyRank:
    def __init__(self, L, lo, hi):
        self.L, self.allowed = L, set(entropy_allowed(L, lo, hi))

    def after(self, ones, r):
        return sum(math.comb(r, k - ones) for k in self.allowed if ones <= k <= ones + r)

    def total(self):
        return self.after(0, self.L)

    def unrank(self, k):
        out, ones = [], 0
        for i in range(self.L):
            n0 = self.after(ones, self.L - i - 1)
            if k < n0:
                out.append(0)
            else:
                k -= n0
                out.append(1)
                ones += 1
        return out


NOTE_TONICS = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
NOTE_SCALES = {"major": [0, 2, 4, 5, 7, 9, 11], "minor": [0, 2, 3, 5, 7, 8, 10], "harmonic-minor": [0, 2, 3, 5, 7, 8, 11],
               "major-pentatonic": [0, 2, 4, 7, 9], "minor-pentatonic": [0, 3, 5, 7, 10], "blues": [0, 3, 5, 6, 7, 10]}


def key_allowed(tonic, scale):
    """notes104 symbols in the key: pitch index d // 4 (0 = rest, 1 = C4 ... 25 = C6)."""
    t = NOTE_TONICS.index(tonic)
    pcs = {(t + x) % 12 for x in NOTE_SCALES[scale]}
    return [d for d in range(104) if d // 4 == 0 or (d // 4 - 1) % 12 in pcs]


def key_unrank(allowed, L, k):
    a, out = len(allowed), []
    for i in range(L):
        q, k = divmod(k, a ** (L - 1 - i))
        out.append(allowed[q])
    return out


def shuffle_f(key, domain, n, rnd, src, bits):
    lp = lambda b: len(b).to_bytes(4, "little") + b
    msg = b"SIEVE/SHUFFLE/1" + lp(key.encode()) + lp(domain.encode()) + lp(format(n, "x").encode()) + rnd.to_bytes(4, "little") + lp(format(src, "x").encode())
    out, k = b"", 0
    while len(out) * 8 < bits:
        out += hashlib.sha256(msg + k.to_bytes(4, "little")).digest()
        k += 1
    return int.from_bytes(out, "big") >> (len(out) * 8 - bits)


def shuffle(key, domain, n, k, inverse=False):
    """shuffle-sha256-v1: an 8-round Feistel over max(2, bitlen(n-1)) bits, cycle-walked below n."""
    if n == 1:
        return k
    b = max(2, (n - 1).bit_length())
    lob = b // 2
    x = k
    while True:
        hi, lo = x >> lob, x & ((1 << lob) - 1)
        for r in (range(7, -1, -1) if inverse else range(8)):
            if r % 2 == 0:
                lo ^= shuffle_f(key, domain, n, r, hi, lob)
            else:
                hi ^= shuffle_f(key, domain, n, r, lo, b - lob)
        x = (hi << lob) | lo
        if x < n:
            return x


def restrict(f, live):
    """sieve-restrict-v1: the model's quantisation rule over the live symbols only."""
    if len(live) == len(f):
        return f
    d = sum(f[s] for s in live)
    spread = TOTAL - len(live)
    g = [0] * len(f)
    rem = [0] * len(f)
    for s in live:
        g[s] = 1 + spread * f[s] // d
        rem[s] = spread * f[s] % d
    for s in sorted(live, key=lambda s: (-rem[s], s))[:TOTAL - sum(g)]:
        g[s] += 1
    assert sum(g) == TOTAL
    return g


def sieved_table(model, rk, prefix, state):
    """The next-symbol table after `prefix`, keeping only symbols from which a survivor can still be reached."""
    f = model.table(prefix)
    live = [s for s in range(model.n) if (t := rk.step(state, s)) is not None and rk.count(t, rk.L - len(prefix) - 1) > 0]
    return restrict(f, live)


def sieved_interval(model, rk, unit):
    low, width, state = 0, 1, ("start",)
    for i, s in enumerate(unit):
        f = sieved_table(model, rk, unit[:i], state)
        assert f[s] > 0, "not a survivor"
        low = low * TOTAL + sum(f[:s]) * width
        width *= f[s]
        state = rk.step(state, s)
    return low, width


def sieved_code(model, rk, unit):
    S = 16 * len(unit)
    low, width = sieved_interval(model, rk, unit)
    for t in range(S, -1, -1):
        m = -(-low // (1 << t))
        if (m + 1) << t <= low + width:
            return m << t, S - t


def sieved_unit_at(model, rk, length, point):
    unit, low, width, state = [], 0, 1, ("start",)
    for i in range(length):
        f = sieved_table(model, rk, unit, state)
        scale = TOTAL ** (length - 1 - i)
        cum = 0
        for s in range(model.n):
            if (low * TOTAL + (cum + f[s]) * width) * scale > point:
                break
            cum += f[s]
        unit.append(s)
        low = low * TOTAL + cum * width
        width *= f[s]
        state = rk.step(state, s)
    return unit


class AgreementRank:
    """neighbour-agreement-v1 survivors by memoised recursion over (cell, disagreements, the last P
    cells as a tuple): an independent method from the core's limb tables."""

    def __init__(self, w, h, frames, base, min_pm):
        self.w, self.h, self.fr, self.B = w, h, frames, base
        self.n = w * h * frames
        self.P = w * h if frames > 1 else w
        pairs = h * (w - 1) * frames + w * (h - 1) * frames + w * h * (frames - 1)
        self.D = pairs - (min_pm * pairs + 999) // 1000
        self.memo = {}

    def cost(self, i, prev, c):
        """prev: every cell placed so far (only the last P matter)."""
        x, y, f = i % self.w, (i // self.w) % self.h, i // (self.w * self.h)
        k = 0
        if x > 0 and prev[i - 1] != c:
            k += 1
        if y > 0 and prev[i - self.w] != c:
            k += 1
        if f > 0 and prev[i - self.w * self.h] != c:
            k += 1
        return k

    def count(self, prev, d):
        i = len(prev)
        if d > self.D:
            return 0
        if i == self.n:
            return 1
        key = (i, d, tuple(prev[max(0, i - self.P):]))
        if key not in self.memo:
            self.memo[key] = sum(self.count(prev + [c], d + self.cost(i, prev, c)) for c in range(self.B))
        return self.memo[key]

    def total(self):
        return self.count([], 0)

    def unrank(self, k):
        prev, d = [], 0
        for i in range(self.n):
            for c in range(self.B):
                t = self.count(prev + [c], d + self.cost(i, prev, c))
                if k < t:
                    d += self.cost(i, prev, c)
                    prev.append(c)
                    break
                k -= t
        return prev


def cmd_compact_vectors(_args):
    """Rankers for the other lines, the survivor shuffle, and the sieved guided line."""
    here = "../data"
    print("# sieve compact conformance vectors v1 (shuffle-sha256-v1, sieve-restrict-v1 over guided-ac-v1,")
    print("#   symbol-entropy-v1 on two symbols, key-v1), dictionary scowl-en-35, model gutenberg-lower27-o5")
    print("# shuffle <n> <key> <domain> <k> <forward(k)>")
    g = stream("shuffle")
    for n in [1, 2, 3, 5, 17, 256, 257, 1000, 27 ** 5, 2 ** 64 + 13, 27 ** 40 - 7]:
        ks = sorted({0, n - 1, n // 2} | {int.from_bytes(bytes(next(g) for _ in range(40)), "little") % n for _ in range(3)})
        for key, dom in (("sieve", "test"), ("other", "0f52d47d")):
            for k in ks:
                j = shuffle(key, dom, n, k)
                assert shuffle(key, dom, n, j, inverse=True) == k
                print(f"shuffle\t{n}\t{key}\t{dom}\t{k}\t{j}")
    print("# rank <filter> <params> <length> <count> <rank> <unit digits, comma-separated>")
    for L, lo, hi in ((25, 0, 700), (100, 0, 500), (100, 600, 1000), (200, 0, 900)):
        rk = BinaryEntropyRank(L, lo, hi)
        total = rk.total()
        g2 = stream(f"entropy/{L}/{lo}/{hi}")
        for k in [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(32)), "little") % total for _ in range(4)]:
            u = rk.unrank(k)
            assert f_symbol_entropy(u, lo, hi)
            print(f"rank\tsymbol-entropy-v1\tmin_millibits={lo},max_millibits={hi}\t{L}\t{total}\t{k}\t{','.join(map(str, u))}")
    for L, tonic, scale in ((16, "C", "major"), (16, "D", "blues"), (40, "A#", "harmonic-minor")):
        allowed = key_allowed(tonic, scale)
        total = len(allowed) ** L
        g2 = stream(f"key/{L}/{tonic}/{scale}")
        for k in [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(32)), "little") % total for _ in range(4)]:
            u = key_unrank(allowed, L, k)
            print(f"rank\tkey-v1\ttonic={tonic},scale={scale}\t{L}\t{total}\t{k}\t{','.join(map(str, u))}")
    print("# nrank <width> <height> <frames> <colours> <min_permille> <count> <rank> <unit digits>   (neighbour-agreement-v1)")
    for w, h, fr, B, pm in ((5, 5, 1, 2, 600), (8, 8, 1, 2, 600), (8, 8, 1, 2, 750), (3, 3, 1, 3, 500), (4, 3, 1, 4, 400),
                            (2, 2, 3, 2, 600), (3, 2, 2, 2, 700)):
        rk = AgreementRank(w, h, fr, B, pm)
        sys.setrecursionlimit(10000)
        total = rk.total()
        g2 = stream(f"agreement/{w}x{h}x{fr}/{B}/{pm}")
        for k in [0, 1, total // 2, total - 1] + [int.from_bytes(bytes(next(g2) for _ in range(16)), "little") % total for _ in range(3)]:
            u = rk.unrank(k)
            assert f_neighbour_agreement(u, w, h, fr, pm)
            print(f"nrank\t{w}\t{h}\t{fr}\t{B}\t{pm}\t{total}\t{k}\t{','.join(map(str, u))}")
    model = Model(f"{here}/models/gutenberg-lower27-o5.model")
    d = load_dict(f"{here}/dictionaries/scowl-2020.12.07-en-35.txt")
    words = sorted(d[0])
    sym = ALPHABETS["lower27"]
    print("# sguided <filter> <length> <unit> <bits> <hex>   (the survivor's address on the sieved guided line)")
    print("# spoint <filter> <length> <hex point> <unit>      (the survivor whose sieved arc holds the point)")
    for f, L in (("words-v2", 16), ("clean-v2", 16), ("words-v1", 12)):
        rk = WordsRank(words, L, clean=f.startswith("clean"), padding=f.endswith("v2"))
        texts = ["it was the best", "call me ishmael", "the end", "a", "zoo", "an ant in a tin"]
        g3 = stream(f"sguided/{f}/{L}")
        units = [canonicalise(t, "lower27", L)[0] for t in texts]
        units += ["".join(sym[c] for c in rk.unrank(int.from_bytes(bytes(next(g3) for _ in range(16)), "little") % rk.total())) for _ in range(4)]
        for u in units:
            digits = [sym.index(c) for c in u]
            ok = (passes if f.endswith("v1") else passes_v2)(u, f.split("-")[0], d)
            if not ok:
                continue
            point, bits = sieved_code(model, rk, digits)
            print(f"sguided\t{f}\t{L}\t{u}\t{bits}\t{guided_hex(point, bits, 16 * L)}")
        for _ in range(5):
            S = 16 * L
            h = format(int.from_bytes(bytes(next(g3) for _ in range(S // 8)), "big"), "0%dx" % (S // 4))[:8]
            point = int(h, 16) << (S - 32)
            u = "".join(sym[c] for c in sieved_unit_at(model, rk, L, point))
            assert (passes if f.endswith("v1") else passes_v2)(u, f.split("-")[0], d)
            print(f"spoint\t{f}\t{L}\t{h}\t{u}")


# -- books (sieve-book-v1): an independent reader. Decodes every section and recomputes the
#    content id (sieve-book-id-v1).
BOOK_FIELDS = {"text": ["alphabet", "length", "canon"], "image": ["width", "height", "palette"],
               "video": ["width", "height", "frames", "palette"], "audio": ["length"]}
PALETTE_SIZES = {"mono": 2, "ega16": 16, "rgb332": 256, "rgb24": 1 << 24}


def book_read(text, model_dir="../data/models"):
    """Decodes a book record and checks its id. Raises ValueError on anything malformed (never
    assert: python -O would drop those checks)."""
    def need(cond, msg):
        if not cond:
            raise ValueError(msg)

    def field(i, key):
        need(i < len(lines), f"the book ends early (expected '{key} ...')")
        name, sep, value = lines[i].partition(" ")
        need(name == key and sep, f"book line {i + 1}: expected '{key} ...'")
        return value

    lines = text.split("\n")
    need(lines and lines[0] == "sieve-book-v1", "not a sieve-book-v1 record")
    models = {}
    for row in open(f"{model_dir}/models.tsv", encoding="utf-8"):
        cols = row.rstrip("\n").split("\t")
        if cols and not cols[0].startswith("#") and len(cols) >= 5:
            models[cols[0]] = (cols[1], cols[4])  # id -> (file, sha256)
    i, sections = 1, []
    while True:
        need(i < len(lines), "the book ends early")
        if lines[i] == "end":
            break
        role = field(i, "section")
        kind = field(i + 1, "line")
        need(kind in BOOK_FIELDS, f"book line {i + 2}: unknown line '{kind}'")
        i += 2
        f = {}
        for k in BOOK_FIELDS[kind] + ["key", "mode"]:
            f[k] = field(i, k)
            i += 1
        need(f["mode"] in ("positional", "scrambled", "guided"), "bad mode")
        model = None
        if f["mode"] == "guided":
            mid, _, msha = field(i, "model").partition(" ")
            need(mid in models, f"unknown model '{mid}'")
            model = Model(f"{model_dir}/{models[mid][0]}")
            need(model.sha256 == msha == models[mid][1], "model does not match")
            i += 1
        units = field(i, "units")
        need(units.isdigit(), "bad unit count")
        n = int(units)
        addrs = lines[i + 1:i + 1 + n]
        need(len(addrs) == n, "the book ends early")
        i += 1 + n
        if kind == "text":
            L, sym, base = int(f["length"]), f["alphabet"], None
        elif kind == "audio":
            L, sym, base = int(f["length"]), "notes104", 104
        else:
            w, h, fr = int(f["width"]), int(f["height"]), int(f.get("frames", 1))
            need(f["palette"] in PALETTE_SIZES, f"unknown palette '{f['palette']}'")
            L, base = w * h * fr, PALETTE_SIZES[f["palette"]]
            sym = f"{kind}/{f['palette']}/{w}x{h}" + (f"x{fr}" if kind == "video" else "")
        sp = Space(sym if kind == "text" else None, L, f["key"], base=base)
        units = []
        for a in addrs:
            need(a and all(c in "0123456789abcdef" for c in a), "not a lowercase hex address")
            if f["mode"] == "guided":
                S = 16 * L
                pt = int(a, 16) << (S - 4 * len(a))
                u = guided_unit_at(model, L, pt)
                point, bits = guided_code(model, u)
                need(guided_hex(point, bits, S) == a, "not the unit's own address")
            else:
                need(len(a) == sp.hex_width, "an address of the wrong width")
                d = sp._digits(int(a, 16))
                u = sp.unscramble(d) if f["mode"] == "scrambled" else d
            units.append(u)
        sections.append((role, kind, sym, L, sp, units))
    written = field(i + 1, "id")
    canon = ["sieve-book-id-v1"]
    for role, kind, sym, L, sp, units in sections:
        canon += [f"section {role}", f"shape {kind}/{sym}/L{L}", f"units {len(units)}"]
        canon += [format(sp._value(u), "x").zfill(sp.hex_width) for u in units]
    book_id = hashlib.sha256(("\n".join(canon) + "\n").encode()).hexdigest()
    need(book_id == written, "id does not match the content")
    return book_id, sections


def cmd_book_read(args):
    book_id, sections = book_read(open(args.book, encoding="utf-8").read())
    print(book_id)
    for role, kind, sym, L, sp, units in sections:
        if kind == "text":
            print("".join(sp.text_of(u) for u in units).rstrip(" "))


# -- the books line (bookspace-v1): cover, title, pages as one mixed-radix number; scrambled
#    through shuffle-sha256-v1 over the whole book count.
def book_space_id(cover_sym, cover_len, page_sym, page_len, pages, key):
    return f"books/{cover_sym}/L{cover_len}+{page_sym}/L{page_len}x{pages + 1}/key={key}/bookspace-v1"


def book_index(cover, cover_base, title, pages, page_base):
    v = 0
    for d in cover:
        v = v * cover_base + d
    for part in [title] + pages:
        for d in part:
            v = v * page_base + d
    return v


def book_parts(v, cover_base, cover_len, page_base, page_len, pages):
    def take(n, b):
        nonlocal v
        out = []
        for _ in range(n):
            v, r = divmod(v, b)
            out.append(r)
        return out[::-1]
    ps = [take(page_len, page_base) for _ in range(pages)][::-1]
    title = take(page_len, page_base)
    cover = take(cover_len, cover_base)
    assert v == 0
    return cover, title, ps


def cmd_book_vectors(_args):
    """Books-line addresses: positional (mixed radix) and scrambled (shuffle over the book count)."""
    print("# sieve books-line vectors v1 (bookspace-v1 over shuffle-sha256-v1)")
    print("# book <cover symbols> <cover colours> <cover length> <page alphabet> <page length> <pages> <key> <mode> <address> <cover digits> <title> <pages, | between>")
    sym = ALPHABETS["lower27"]
    g = stream("books")
    for cw, ch, L, P, key in ((2, 1, 1, 2, "sieve"), (4, 4, 8, 3, "sieve"), (10, 10, 32, 4, "sieve"), (10, 10, 32, 4, "other"), (5, 5, 64, 0, "sieve")):
        cover_sym, cover_len = f"image/mono/{cw}x{ch}", cw * ch
        n = 2 ** cover_len * 27 ** (L * (P + 1))
        dom = book_space_id(cover_sym, cover_len, "lower27", L, P, key)
        width = max(1, ((n - 1).bit_length() + 3) // 4)
        for mode in ("positional", "scrambled"):
            for k in [0, n - 1, n // 3] + [int.from_bytes(bytes(next(g) for _ in range(width)), "little") % n for _ in range(3)]:
                pos = shuffle(key, dom, n, k, inverse=True) if mode == "scrambled" else k
                cover, title, pages = book_parts(pos, 2, cover_len, 27, L, P)
                assert book_index(cover, 2, title, pages, 27) == pos
                txt = lambda d: "".join(sym[x] for x in d)
                print(f"book\t{cover_sym}\t2\t{cover_len}\tlower27\t{L}\t{P}\t{key}\t{mode}\t{format(k, 'x').zfill(width)}\t"
                      f"{''.join(map(str, cover))}\t{txt(title)}\t{'|'.join(txt(p) for p in pages)}")


# -- compositions (composition-v1): a cover, a title (or none) and N units of a base line as one
#    mixed-radix number, cover first; scrambled through shuffle-sha256-v1 over the whole count,
#    keyed with the unit space's key. Tracks are units of audio, movies units of video.
def composition_space_id(kind, cover, title, unit, n, key):
    part = lambda sp: f"{sp[0]}/L{sp[2]}"
    return f"{kind}/{part(cover)}+{part(title) if title else '-'}+{part(unit)}x{n}/key={key}/composition-v1"


def composition_index(cover_d, title_d, units_d, cover, title, unit):
    v = 0
    for d in cover_d:
        v = v * cover[1] + d
    if title:
        for d in title_d:
            v = v * title[1] + d
    for u in units_d:
        for d in u:
            v = v * unit[1] + d
    return v


def composition_parts(v, cover, title, unit, n):
    def take(length, base):
        nonlocal v
        out = []
        for _ in range(length):
            v, r = divmod(v, base)
            out.append(r)
        return out[::-1]
    units = [take(unit[2], unit[1]) for _ in range(n)][::-1]
    title_d = take(title[2], title[1]) if title else []
    cover_d = take(cover[2], cover[1])
    assert v == 0
    return cover_d, title_d, units


def cmd_composition_vectors(_args):
    """Composition addresses (tracks, movies): positional (mixed radix) and scrambled."""
    print("# sieve composition vectors v1 (composition-v1 over shuffle-sha256-v1)")
    print("# composition <kind> <cover symbols> <base> <length> <title symbols or -> <base> <length> <unit symbols> <base> <length> "
          "<units> <key> <mode> <address> <cover digits> <title digits> <units' digits, | between units> (digits . separated)")
    g = stream("compositions")
    cases = (
        ("tracks", ("image/mono/2x1", 2, 2), ("lower27", 27, 1), ("notes104", 104, 2), 2, "sieve"),
        ("movies", ("image/mono/2x2", 2, 4), None, ("video/mono/2x2x2", 2, 8), 3, "sieve"),
        ("tracks", ("image/ega16/4x4", 16, 16), ("lower27", 27, 4), ("notes104", 104, 16), 4, "other"),
        ("movies", ("image/mono/3x3", 2, 9), ("lower27", 27, 2), ("video/rgb332/2x2x4", 256, 16), 2, "sieve"),
    )
    for kind, cover, title, unit, n, key in cases:
        size = cover[1] ** cover[2] * (title[1] ** title[2] if title else 1) * unit[1] ** (unit[2] * n)
        dom = composition_space_id(kind, cover, title, unit, n, key)
        width = max(1, ((size - 1).bit_length() + 3) // 4)
        for mode in ("positional", "scrambled"):
            for k in [0, size - 1, size // 3] + [int.from_bytes(bytes(next(g) for _ in range(width)), "little") % size for _ in range(3)]:
                pos = shuffle(key, dom, size, k, inverse=True) if mode == "scrambled" else k
                cover_d, title_d, units = composition_parts(pos, cover, title, unit, n)
                assert composition_index(cover_d, title_d, units, cover, title, unit) == pos
                dots = lambda d: ".".join(map(str, d))
                t = title or ("-", 1, 0)
                print(f"composition\t{kind}\t{cover[0]}\t{cover[1]}\t{cover[2]}\t{t[0]}\t{t[1]}\t{t[2]}\t{unit[0]}\t{unit[1]}\t{unit[2]}\t{n}\t{key}\t"
                      f"{mode}\t{format(k, 'x').zfill(width)}\t{dots(cover_d)}\t{dots(title_d)}\t{'|'.join(dots(u) for u in units)}")


# -- worlds (worldspace-v1): a cover, a title (or none) and N slots, each a model of the models line
#    placed in a cell of a G x G x G grid with one of the 24 turns of a cube (turns-24-v1), as one
#    mixed-radix number, cover first; scrambled through shuffle-sha256-v1 over the whole count, keyed
#    with the models line's key. Written independently of the C++.
def world_turns():
    """turns-24-v1: row i of a turn takes old coordinate perm[i] times sign[i]; by permutation
    (lexicographic), then signs (rows 0, 1, 2 as bits 4, 2, 1 counting up, set = minus), keeping
    determinant +1."""
    import itertools
    out = []
    for perm in itertools.permutations(range(3)):
        for bits in range(8):
            sign = [-1 if bits & (4 >> i) else 1 for i in range(3)]
            m = [[sign[i] if j == perm[i] else 0 for j in range(3)] for i in range(3)]
            det = (m[0][0] * (m[1][1] * m[2][2] - m[1][2] * m[2][1]) - m[0][1] * (m[1][0] * m[2][2] - m[1][2] * m[2][0])
                   + m[0][2] * (m[1][0] * m[2][1] - m[1][1] * m[2][0]))
            if det == 1:
                out.append((perm, sign))
    assert len(out) == 24 and out[0] == ((0, 1, 2), [1, 1, 1])
    return out


class WorldSpace:
    def __init__(self, cover, title, models, slots, grid):
        """cover, title: (symbols id, base, length) or title None; models: a ModelSpace."""
        assert slots >= 1 and 1 <= grid <= 1024
        self.cover, self.title, self.m, self.n, self.g = cover, title, models, slots, grid
        self.places = grid ** 3 * 24
        self.slot_size = models.size * self.places
        self.size = cover[1] ** cover[2] * (title[1] ** title[2] if title else 1) * self.slot_size ** slots
        self.hex_width = max(1, ((self.size - 1).bit_length() + 3) // 4)
        part = lambda sp: f"{sp[0]}/L{sp[2]}"
        self.id = (f"worlds/{part(cover)}+{part(title) if title else '-'}+V{models.v}/F{models.f}/C{models.c}x{slots}"
                   f"/G{grid}/key={models.key}/worldspace-v1")

    def index_of(self, cover_d, title_d, slots, mode="positional"):
        v = 0
        for d in cover_d:
            v = v * self.cover[1] + d
        if self.title:
            for d in title_d:
                v = v * self.title[1] + d
        for model, x, y, z, t in slots:
            v = v * self.slot_size + model * self.places + ((x * self.g + y) * self.g + z) * 24 + t
        return shuffle(self.m.key, self.id, self.size, v) if mode == "scrambled" else v

    def parts_at(self, index, mode="positional"):
        v = shuffle(self.m.key, self.id, self.size, index, inverse=True) if mode == "scrambled" else index
        slots = []
        for _ in range(self.n):
            v, slot = divmod(v, self.slot_size)
            model, place = divmod(slot, self.places)
            place, t = divmod(place, 24)
            place, z = divmod(place, self.g)
            x, y = divmod(place, self.g)
            slots.append((model, x, y, z, t))
        slots.reverse()

        def take(length, base):
            nonlocal v
            out = []
            for _ in range(length):
                v, r = divmod(v, base)
                out.append(r)
            return out[::-1]
        title_d = take(self.title[2], self.title[1]) if self.title else []
        cover_d = take(self.cover[2], self.cover[1])
        assert v == 0
        return cover_d, title_d, slots

    def to_obj(self, slots):
        turns = world_turns()
        dec = self.m.decimals
        whole = len(str(self.g))
        width = len(str(self.m.v * self.n))

        def text(x):
            units = x * 10 ** dec
            assert units.denominator == 1
            units = int(units)
            w, f = divmod(abs(units), 10 ** dec)
            return f"{'-' if units < 0 else '+'}{w:0{whole}d}.{f:0{dec}d}"
        vlines, flines = [], []
        for n, (model, x, y, z, t) in enumerate(slots):
            verts, faces = self.m.parts_at(model)
            perm, sign = turns[t]
            centre = (2 * x + 1 - self.g, 2 * y + 1 - self.g, 2 * z + 1 - self.g)
            for i in range(self.m.v):
                p = [self.m.coord(verts[3 * i + k]) for k in range(3)]
                vlines.append("v " + " ".join(text(sign[k] * p[perm[k]] + centre[k]) for k in range(3)))
            for i in range(self.m.f):
                flines.append("f " + " ".join(f"{n * self.m.v + faces[3 * i + k] + 1:0{width}d}" for k in range(3)))
        return "\n".join(vlines + flines) + "\n"


def cmd_world_obj(args):
    """A world's .obj text, from its slots (MODEL:x.y.z.turn|..., each model's address on the models
    line in hex), as `sieve world --read` writes it. The cover and title do not enter the .obj."""
    ms = ModelSpace(args.vertices, args.faces, args.coords, args.key)
    ws = WorldSpace(("image/mono/1x1", 2, 1), None, ms, args.world_models, args.world_grid)
    slots = []
    for slot in args.slots.split("|"):
        model, place = slot.split(":")
        x, y, z, t = (int(v) for v in place.split("."))
        slots.append((int(model, 16), x, y, z, t))
    slots += [(0, 0, 0, 0, 0)] * (args.world_models - len(slots))
    sys.stdout.buffer.write(ws.to_obj(slots).encode())


def cmd_world_vectors(_args):
    """World addresses (worldspace-v1): positional and scrambled, each slot's model and place, and the
    world's .obj text by its SHA-256."""
    print("# sieve world vectors v1 (worldspace-v1, turns-24-v1, over shuffle-sha256-v1)")
    print("# turns <24 turns, each its permutation and signs, e.g. 012+++>")
    print("# world <cover symbols> <base> <length> <title symbols or -> <base> <length> <vertices> <faces> <coords> <slots> <grid> <key> "
          "<mode> <address> <cover digits> <title digits> <slots: model index hex:x.y.z.turn, | between> <obj sha256> (digits . separated)")
    print("turns\t" + " ".join("".join(map(str, p)) + "".join("+" if x > 0 else "-" for x in sg) for p, sg in world_turns()))
    g = stream("worlds")
    cases = (
        (("image/mono/2x1", 2, 2), ("lower27", 27, 1), (3, 1, 2, "sieve"), 2, 2),
        (("image/mono/2x2", 2, 4), None, (4, 2, 4, "sieve"), 3, 3),
        (("image/mono/10x10", 2, 100), ("lower27", 27, 32), (8, 12, 16, "sieve"), 4, 8),
        (("image/ega16/4x4", 16, 16), ("lower27", 27, 4), (8, 12, 16, "other"), 2, 16),
    )
    for cover, title, (v, f, c, key), n, grid in cases:
        ws = WorldSpace(cover, title, ModelSpace(v, f, c, key), n, grid)
        for mode in ("positional", "scrambled"):
            for k in [0, ws.size - 1, ws.size // 3] + [int.from_bytes(bytes(next(g) for _ in range(ws.hex_width)), "little") % ws.size
                                                      for _ in range(3)]:
                cover_d, title_d, slots = ws.parts_at(k, mode)
                assert ws.index_of(cover_d, title_d, slots, mode) == k
                obj = ws.to_obj(slots)
                dots = lambda d: ".".join(map(str, d))
                t = title or ("-", 1, 0)
                sl = "|".join(f"{m:0{ws.m.hex_width}x}:{x}.{y}.{z}.{tt}" for m, x, y, z, tt in slots)
                print(f"world\t{cover[0]}\t{cover[1]}\t{cover[2]}\t{t[0]}\t{t[1]}\t{t[2]}\t{v}\t{f}\t{c}\t{n}\t{grid}\t{key}\t"
                      f"{mode}\t{format(k, 'x').zfill(ws.hex_width)}\t{dots(cover_d)}\t{dots(title_d)}\t{sl}\t"
                      f"{hashlib.sha256(obj.encode()).hexdigest()}")


# -- titled lines (titled-v1): a cover (some lines), a title and a content index as one
#    mixed-radix number, cover first; scrambled through shuffle-sha256-v1 over the whole count.
def titled_space_id(content_shape, title_sym, title_len, cover, key):
    title_part = f"+title={title_sym}/L{title_len}" if title_len > 0 else ""  # a title length of 0 is no title
    cover_part = f"+cover={cover[0]}/L{cover[2]}" if cover else ""
    return f"titled/{content_shape}{title_part}{cover_part}/key={key}/titled-v1"


def titled_parts(v, content_size, title_base, title_len, cover):
    """Positional index -> (cover digits, title digits, content index), per SPECIFICATIONS §11."""
    def digits(n, base, length):
        out = []
        for _ in range(length):
            n, r = divmod(n, base)
            out.append(r)
        assert n == 0
        return out[::-1]
    rest, content = divmod(v, content_size)
    cover_n, title_n = divmod(rest, title_base ** title_len)
    return (digits(cover_n, cover[1], cover[2]) if cover else []), digits(title_n, title_base, title_len), content


def titled_index(cover_d, title_d, content, content_size, title_base, cover):
    num = lambda ds, b: sum(d * b ** i for i, d in enumerate(reversed(ds)))
    v = num(cover_d, cover[1]) if cover else 0
    v = v * title_base ** len(title_d) + num(title_d, title_base)
    return v * content_size + content


def cmd_titled_vectors(_args):
    """Titled-line addresses: positional (mixed radix, cover first) and scrambled (shuffle over the count)."""
    print("# sieve titled-line vectors v1 (titled-v1 over shuffle-sha256-v1)")
    print("# content_shape content_size title_alphabet T cover_symbols cover_base cover_length key mode address cover_digits title_digits content")
    print("# (sizes, addresses and contents in hex; digits comma-separated; '-' for no cover)")
    g = stream("titled")
    models = ModelSpace(4, 2, 4)
    shapes = (
        ("lower27/L16", 27 ** 16, "lower27", 8, None, "sieve"),
        ("notes/L6", 104 ** 6, "lower27", 4, ("image/mono/4x4", 2, 16), "sieve"),
        ("video/mono/2x2x3/L12", 2 ** 12, "lower27", 2, ("image/mono/2x2", 2, 4), "other"),
        ("models/V4/F2/C4", models.size, "lower27", 3, None, "sieve"),
        ("lower27/L4", 27 ** 4, "greek", 5, None, "sieve"),
        ("lower27/L6", 27 ** 6, "lower27", 0, None, "sieve"),
        ("notes/L3", 104 ** 3, "lower27", 0, ("image/mono/2x2", 2, 4), "other"),
    )
    for content_shape, cs, talpha, T, cover, key in shapes:
        tb = len(alphabet(talpha))
        n = (cover[1] ** cover[2] if cover else 1) * tb ** T * cs
        dom = titled_space_id(content_shape, talpha, T, cover, key)
        width = max(1, ((n - 1).bit_length() + 3) // 4)
        for mode in ("positional", "scrambled"):
            for k in [0, n - 1, n // 3] + [int.from_bytes(bytes(next(g) for _ in range(width)), "little") % n for _ in range(3)]:
                pos = shuffle(key, dom, n, k, inverse=True) if mode == "scrambled" else k
                cd, td, content = titled_parts(pos, cs, tb, T, cover)
                assert titled_index(cd, td, content, cs, tb, cover) == pos
                assert (shuffle(key, dom, n, pos) if mode == "scrambled" else pos) == k
                cov = f"{cover[0]}\t{cover[1]}\t{cover[2]}" if cover else "-\t0\t0"
                if T == 0 and not cover and mode == "positional":
                    assert pos == content  # no title and no cover: the content's own address
                print(f"{content_shape}\t{cs:x}\t{talpha}\t{T}\t{cov}\t{key}\t{mode}\t{format(k, 'x').zfill(width)}\t"
                      f"{','.join(map(str, cd)) or '-'}\t{','.join(map(str, td)) or '-'}\t{content:x}")


def binary_space_id(n, key):
    return f"binary/bytes256/L0-{n}/key={key}/binary-v1"


def binary_index(data):
    """binary-v1 positional: every shorter file comes first, then files of this length by their bytes
    read big-endian. Written from the definition (a sum over lengths), not from any shortcut."""
    shorter = sum(256 ** l for l in range(len(data)))
    return shorter + int.from_bytes(data, "big")


def binary_file(v):
    """The inverse: step over whole lengths until the rest is inside one."""
    length = 0
    while v >= 256 ** length:
        v -= 256 ** length
        length += 1
    return v.to_bytes(length, "big")


def cmd_binary_vectors(_args):
    """The binary line (binary-v1): every file of 0..N bytes, shortest first, then by value; scrambled
    through shuffle-sha256-v1 over the count."""
    print("# sieve binary-line vectors v1 (binary-v1 over shuffle-sha256-v1)")
    print("# max_bytes key mode address file_length file_hex (hex; '-' for the empty file)")
    g = stream("binary")
    named = [b"", b"\x00", b"\xff", b"\x00\x00", b"Sieve", bytes(range(8))]
    for n, key in ((1, "sieve"), (2, "sieve"), (3, "other"), (5, "sieve"), (40, "sieve"), (700, "sieve")):
        m = sum(256 ** l for l in range(n + 1))
        dom = binary_space_id(n, key)
        width = max(1, ((m - 1).bit_length() + 3) // 4)
        for mode in ("positional", "scrambled"):
            files = [f for f in named if len(f) <= n]
            files += [binary_file(k) for k in (0, m - 1, m // 3)]
            for _ in range(3):
                length = next(g) % (n + 1)
                files.append(bytes(next(g) for _ in range(length)))
            for f in files:
                pos = binary_index(f)
                assert binary_file(pos) == f and pos < m
                k = shuffle(key, dom, m, pos) if mode == "scrambled" else pos
                assert (shuffle(key, dom, m, k, inverse=True) if mode == "scrambled" else k) == pos
                print(f"{n}\t{key}\t{mode}\t{format(k, 'x').zfill(width)}\t{len(f)}\t{f.hex() or '-'}")


# ---------------------------------------------------------------- content-defined chunks (cdc-v1)
# Written from the rule in core/include/sieve/chunks.hpp, not from its code: h = 2h + gear[byte]
# mod 2^64; a chunk ends after a byte where h's top eight bits are zero (at 64 bytes or more), or
# at 1,024; h starts again at each chunk. gear[i]: SHA-256("cdc-v1 gear" + byte i), first 8 bytes
# big-endian. Each chunk is named by its SHA-256; a chunk of one repeated byte is "uniform".

CDC_MIN, CDC_MAX = 64, 1024
CDC_GEAR = [int.from_bytes(hashlib.sha256(b"cdc-v1 gear" + bytes([i])).digest()[:8], "big") for i in range(256)]


def cdc_chunks(data):
    out, start, h = [], 0, 0
    mask = (1 << 64) - 1
    for i, b in enumerate(data):
        h = ((h << 1) + CDC_GEAR[b]) & mask
        n = i + 1 - start
        if (n >= CDC_MIN and h >> 56 == 0) or n >= CDC_MAX:
            out.append((start, n))
            start, h = i + 1, 0
    if start < len(data):
        out.append((start, len(data) - start))
    return [(o, n, hashlib.sha256(data[o:o + n]).hexdigest(), len(set(data[o:o + n])) == 1) for o, n in out]


def cmd_chunks(args):
    """A file's cdc-v1 chunks: offset, length, SHA-256, and whether every byte is the same."""
    data = open(args.file, "rb").read()
    for o, n, h, u in cdc_chunks(data):
        print(f"{o}\t{n}\t{h}\t{'uniform' if u else '-'}")


def cmd_chunk_vectors(_args):
    """cdc-v1: the gear table's ends, then inputs and their chunks (offset:length:sha256:uniform)."""
    print("# sieve content-defined chunk vectors (cdc-v1)")
    print("# gear i value_hex | name input_hex chunks")
    for i in (0, 1, 127, 255):
        print(f"gear\t{i}\t{CDC_GEAR[i]:016x}")
    g = stream("chunks")
    noise = bytes(next(g) for _ in range(6000))
    text = (b"It was the best of times, it was the worst of times, it was the age of wisdom, "
            b"it was the age of foolishness, it was the epoch of belief. ") * 40
    changed = bytearray(noise[:3000]); changed[1500] ^= 0xFF
    cases = [("empty", b""), ("short", b"Sieve cdc"), ("zeros63", bytes(63)), ("zeros3000", bytes(3000)),
             ("noise", noise), ("fragment", noise[1234:1234 + 2500]), ("text", text[:4000]),
             ("changed", bytes(changed)), ("ramp", bytes(i % 256 for i in range(2600)))]
    for name, data in cases:
        cs = ",".join(f"{o}:{n}:{h}:{int(u)}" for o, n, h, u in cdc_chunks(data)) or "-"
        print(f"{name}\t{data.hex() or '-'}\t{cs}")


# ---------------------------------------------------------------- notes2 (canon-notes-v2)
# Written from the description in core/include/sieve/audio.hpp, not from audio.cpp.

NOTE2_CODES = "seEqQhHw"                          # s e e. q q. h h. w
NOTE2_LEN = dict(zip(NOTE2_CODES, [1, 2, 3, 4, 6, 8, 12, 16]))  # in sixteenths
NOTE2_WRITTEN = dict(zip(NOTE2_CODES, ["s", "e", "e.", "q", "q.", "h", "h.", "w"]))
SHARP_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
LETTER_SEMI = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def note2_midi(name):
    m = re.fullmatch(r"([A-Ga-g])([#b]?)([0-9])", name)
    if not m:
        raise ValueError(f"cannot read the note {name!r}")
    semi = LETTER_SEMI[m.group(1).upper()] + {"#": 1, "b": -1, "": 0}[m.group(2)]
    return (int(m.group(3)) + 1) * 12 + semi


def note2_name(midi):
    return SHARP_NAMES[midi % 12] + str(midi // 12 - 1)


class NoteSet2:
    def __init__(self, low, high, durations, voices):
        if not (36 <= low and high <= 96 and high - low >= 11):
            raise ValueError("a notes2 range lies in C2..C7 and spans an octave")
        if not durations or any(c not in NOTE2_CODES for c in durations) or \
                [NOTE2_CODES.index(c) for c in durations] != sorted({NOTE2_CODES.index(c) for c in durations}):
            raise ValueError("durations are distinct codes of seEqQhHw, in that order")
        if not 1 <= voices <= 4:
            raise ValueError("1 to 4 voices")
        self.low, self.high, self.durations, self.voices = low, high, durations, voices
        self.D = len(durations)
        self.P = high - low + 1
        self.base = (self.P + 1) * self.D

    def id(self):
        return f"notes2/{note2_name(self.low)}-{note2_name(self.high)}/{self.durations}/V{self.voices}"

    def token(self, d):
        pitch, dur = divmod(d, self.D)
        name = "R" if pitch == 0 else note2_name(self.low + pitch - 1)
        return name + NOTE2_WRITTEN[self.durations[dur]]


def canon_notes2(text, ns, L):
    """canon-notes-v2: units of voices x L digits, and what was changed on the way."""
    rep = dict(events=0, flats=0, octave=0, default=0, changed=0, padding=0)
    voices_text = text.split("//")
    if len(voices_text) > ns.voices:
        raise ValueError("more voices than the line has")
    voices = [[] for _ in range(ns.voices)]
    for v, part in enumerate(voices_text):
        for tok in re.split(r"[\s|,]+", part):
            if not tok:
                continue
            m = re.fullmatch(r"(?:([Rr])|([A-Ga-g])([#b]?)([0-9]))(?:([seqhwSEQHW])(\.?))?", tok)
            if not m:
                raise ValueError(f"cannot read note {tok!r}")
            if m.group(1):
                pitch = 0
            else:
                if m.group(3) == "b":
                    rep["flats"] += 1
                midi = (int(m.group(4)) + 1) * 12 + LETTER_SEMI[m.group(2).upper()] + {"#": 1, "b": -1, "": 0}[m.group(3)]
                moved = midi
                while moved < ns.low:
                    moved += 12
                while moved > ns.high:
                    moved -= 12
                rep["octave"] += moved != midi
                pitch = moved - ns.low + 1
            if m.group(5) is None:
                want = 4
                rep["default"] += 1
            else:
                written = m.group(5).lower() + m.group(6)
                if written not in NOTE2_WRITTEN.values():
                    raise ValueError(f"cannot read note {tok!r}")
                want = {w: NOTE2_LEN[c] for c, w in NOTE2_WRITTEN.items()}[written]
            # the nearest length the set has; a tie to the longer
            lengths = [NOTE2_LEN[c] for c in ns.durations]
            best = min(range(ns.D), key=lambda i: (abs(lengths[i] - want), -lengths[i]))
            rep["changed"] += lengths[best] != want
            voices[v].append(pitch * ns.D + best)
            rep["events"] += 1
    runs = max(1, max((len(x) + L - 1) // L for x in voices))
    units = []
    for k in range(runs):
        unit = []
        for x in voices:
            chunk = x[k * L:(k + 1) * L]
            rep["padding"] += L - len(chunk)
            unit += chunk + [0] * (L - len(chunk))
        units.append(unit)
    return units, rep


def notes2_notation(ns, digits):
    per = len(digits) // ns.voices
    return " // ".join(" ".join(ns.token(d) for d in digits[v * per:(v + 1) * per]) for v in range(ns.voices))


def notes2_midi(ns, digits):
    """Format 1: a tempo track, then a track per voice on channel v (480 ticks a quarter)."""
    def vlq(n):
        out = [n & 0x7F]
        n >>= 7
        while n:
            out.append(0x80 | (n & 0x7F))
            n >>= 7
        return bytes(reversed(out))

    def chunk(body):
        return b"MTrk" + struct.pack(">I", len(body)) + body

    tracks = [chunk(vlq(0) + bytes([0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20]) + vlq(0) + bytes([0xFF, 0x2F, 0x00]))]
    per = len(digits) // ns.voices
    for v in range(ns.voices):
        body = vlq(0) + bytes([0xC0 | v, 0])
        wait = 0
        for d in digits[v * per:(v + 1) * per]:
            pitch, dur = divmod(d, ns.D)
            ticks = NOTE2_LEN[ns.durations[dur]] * 120
            if pitch == 0:
                wait += ticks
                continue
            midi = ns.low + pitch - 1
            body += vlq(wait) + bytes([0x90 | v, midi, 96]) + vlq(ticks) + bytes([0x80 | v, midi, 0])
            wait = 0
        body += vlq(wait) + bytes([0xFF, 0x2F, 0x00])
        tracks.append(chunk(body))
    return b"MThd" + struct.pack(">IHHH", 6, 1, 1 + ns.voices, 480) + b"".join(tracks)


NOTES2_CASES = [
    # (low, high, durations, voices, L, notation)
    ("C3", "C6", "seEqQhHw", 1, 4, "C4q E4q. G4h Bb5s"),
    ("C3", "C6", "seEqQhHw", 2, 6, "C4q E4q. G4h Bb5s // C3w G2h"),
    ("C3", "C6", "eqhw", 1, 3, "C4s D4e. E4q. F4h. G4w"),
    ("C2", "C7", "seEqQhHw", 4, 2, "C1q // C8w // Rh. // A4"),
    ("C4", "B4", "sq", 1, 3, "C5q D3s E4"),
    ("C3", "C6", "seEqQhHw", 3, 2, "C4 D4 E4 // F4"),
    ("C3", "C6", "seEqQhHw", 2, 2, "// C4q"),
    ("C3", "C6", "seEqQhHw", 1, 2, "C6w C3s"),
    ("C3", "C6", "seEqQhHw", 1, 3, "c4q r e4h."),
    ("F#3", "G5", "EQH", 2, 3, "F#3e. G5q. Bb4h. | A4h, Gb4 // D4w Rs C4"),
    ("C3", "C6", "w", 1, 2, "C4s D4"),
    ("C3", "C6", "seEqQhHw", 4, 1, "C4 // D4 // E4 // F4"),
]


def cmd_notes2_vectors(args):
    """canon-notes-v2: set, events per voice, notation in; every unit's digits, the report, the
    first unit's notation and its MIDI file's SHA-256 out."""
    print("# set\tlength\tnotation\tunits\treport\tnotation_out\tmidi_sha256")
    for low, high, durs, voices, L, text in NOTES2_CASES:
        ns = NoteSet2(note2_midi(low), note2_midi(high), durs, voices)
        units, rep = canon_notes2(text, ns, L)
        report = ",".join(str(rep[k]) for k in ("events", "flats", "octave", "default", "changed", "padding"))
        print(f"{ns.id()}\t{L}\t{text}\t{';'.join(','.join(map(str, u)) for u in units)}\t{report}\t"
              f"{notes2_notation(ns, units[0])}\t{hashlib.sha256(notes2_midi(ns, units[0])).hexdigest()}")


# ---------------------------------------------------------------- canon-pcm-v1 (sound itself)
# Written from SPECIFICATIONS 3.3 and sieve/sound.hpp's rules, not from sound.cpp: its own WAV
# reader and writer, and the resampling done in exact fractions of a second rather than in the
# engine's integer spans.

from fractions import Fraction


class PcmFormat:
    def __init__(self, rate, bits, channels):
        if not (1 <= rate < 2**31) or not (1 <= bits <= 31) or not (1 <= channels <= 65535):
            raise ValueError("bad pcm set")
        self.rate, self.bits, self.channels = rate, bits, channels

    def id(self):
        return f"pcm/{self.rate}/{self.bits}/C{self.channels}"


def _wav_float_to_s32(x):
    if x != x:
        return 0
    v = math.floor(x * 2**31 + 0.5)
    return max(-2**31, min(2**31 - 1, v))


def wav_read(b):
    """(rate, channels, frames as lists of signed 32-bit samples)."""
    if b[:4] != b"RIFF" or b[8:12] != b"WAVE":
        raise ValueError("not a WAV file")
    at, fmt = 12, None
    while at + 8 <= len(b):
        cid, size = b[at:at + 4], struct.unpack_from("<I", b, at + 4)[0]
        body = at + 8
        if cid == b"fmt ":
            tag, ch, rate = struct.unpack_from("<HHI", b, body)
            align, bits = struct.unpack_from("<HH", b, body + 12)
            if tag == 0xFFFE:
                tag = struct.unpack_from("<H", b, body + 24)[0]
            fmt = (tag, ch, rate, align, bits)
        elif cid == b"data":
            tag, ch, rate, align, bits = fmt
            end = len(b) if size in (0, 0xFFFFFFFF) else min(len(b), body + size)
            n = (end - body) // align
            frames = []
            w = bits // 8
            for i in range(n):
                fr = []
                for c in range(ch):
                    p = body + (i * ch + c) * w
                    raw = b[p:p + w]
                    if tag == 3:
                        x = struct.unpack("<f" if bits == 32 else "<d", raw)[0]
                        fr.append(_wav_float_to_s32(x))
                    elif bits == 8:
                        fr.append((raw[0] - 128) * 2**24)
                    else:
                        v = int.from_bytes(raw, "little", signed=True)
                        fr.append(v * 2**(32 - bits))
                frames.append(fr)
            return rate, ch, frames
        at = body + size + (size & 1)
    raise ValueError("no data")


def _round_half_up(x):  # x a Fraction
    return math.floor(x + Fraction(1, 2))


def canon_pcm(wav, f, L):
    """canon-pcm-v1: units (lists of digits) and the report (source frames, samples, clipped, padding)."""
    srate, sch, frames = wav_read(wav)
    # 1. channels
    chans = []
    for fr in frames:
        if f.channels == 1 and sch > 1:
            chans.append([_round_half_up(Fraction(sum(fr), sch))])
        else:
            chans.append([fr[min(c, sch - 1)] for c in range(f.channels)])
    # 2. rate: target t spans [t/R, (t+1)/R) seconds; the source i spans [i/S, (i+1)/S).
    n = len(chans)
    total = Fraction(n, srate)
    count = math.ceil(total * f.rate)
    out = [[] for _ in range(f.channels)]
    clipped = 0
    hi, lo = 2**(f.bits - 1) - 1, -2**(f.bits - 1)
    for t in range(count):
        a, b = Fraction(t, f.rate), min(Fraction(t + 1, f.rate), total)
        first, last = math.floor(a * srate), math.ceil(b * srate)
        for c in range(f.channels):
            acc = Fraction(0)
            for i in range(first, min(last, n)):
                lo_i, hi_i = max(a, Fraction(i, srate)), min(b, Fraction(i + 1, srate))
                if hi_i > lo_i:
                    acc += chans[i][c] * (hi_i - lo_i)
            v = _round_half_up(acc / (b - a))
            # 3. depth
            s = _round_half_up(Fraction(v, 2**(32 - f.bits)))
            if s > hi or s < lo:
                s = max(lo, min(hi, s))
                clipped += 1
            out[c].append(s)
    # 4. units
    runs = max(1, -(-count // L))
    units = []
    for k in range(runs):
        u = []
        for c in range(f.channels):
            for i in range(k * L, (k + 1) * L):
                s = out[c][i] if i < count else 0
                u.append(s % 2**f.bits)
        units.append(u)
    return units, (n, count, clipped, (runs * L - count) * f.channels)


def pcm_wav(f, unit):
    """A unit as a WAV file, as SPECIFICATIONS 3.3 says it is written."""
    nb = (f.bits + 7) // 8
    shift = nb * 8 - f.bits
    L = len(unit) // f.channels
    data = bytearray()
    for i in range(L):
        for c in range(f.channels):
            d = unit[c * L + i]
            s = d - 2**f.bits if d >= 2**(f.bits - 1) else d
            s <<= shift
            data += bytes([s + 128]) if nb == 1 else (s % 2**(8 * nb)).to_bytes(nb, "little")
    plain = f.bits in (8, 16, 24) and f.channels <= 2
    if plain:
        fmt = struct.pack("<HHIIHH", 1, f.channels, f.rate, f.rate * f.channels * nb, f.channels * nb, nb * 8)
    else:
        mask = 4 if f.channels == 1 else 3 if f.channels == 2 else 0
        guid = bytes([1, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71])
        fmt = struct.pack("<HHIIHHHHI", 0xFFFE, f.channels, f.rate, f.rate * f.channels * nb, f.channels * nb, nb * 8, 22, f.bits, mask) + guid
    return b"RIFF" + struct.pack("<I", 4 + 8 + len(fmt) + 8 + len(data)) + b"WAVE" + b"fmt " + struct.pack("<I", len(fmt)) + fmt + \
        b"data" + struct.pack("<I", len(data)) + bytes(data)


def _wav_of(rate, ch, tag, bits, samples, extensible=False, stream=False):
    """A test WAV file: samples are ints for PCM (in the file's own range) or floats."""
    nb = bits // 8
    data = bytearray()
    for v in samples:
        if tag == 3:
            data += struct.pack("<f" if bits == 32 else "<d", v)
        elif bits == 8:
            data += bytes([v])
        else:
            data += (v % 2**bits).to_bytes(nb, "little")
    fmt = struct.pack("<HHIIHH", 0xFFFE if extensible else tag, ch, rate, rate * ch * nb, ch * nb, bits)
    if extensible:
        guid = bytes([tag, 0, 0, 0, 0, 0, 0x10, 0, 0x80, 0, 0, 0xAA, 0, 0x38, 0x9B, 0x71])
        fmt += struct.pack("<HHI", 22, bits, 0) + guid
    size = 0xFFFFFFFF if stream else len(data)
    return b"RIFF" + struct.pack("<I", 0xFFFFFFFF if stream else 4 + 8 + len(fmt) + 8 + len(data)) + b"WAVE" + b"fmt " + \
        struct.pack("<I", len(fmt)) + fmt + b"LIST" + struct.pack("<I", 3) + b"abc\0" + b"data" + struct.pack("<I", size) + bytes(data)


def _pcm_cases():
    seed = [12345]

    def rnd(lo, hi):
        seed[0] = (seed[0] * 1103515245 + 12345) % 2**31
        return lo + seed[0] % (hi - lo + 1)

    def ints(n, bits):
        return [rnd(0, 255) if bits == 8 else rnd(-2**(bits - 1), 2**(bits - 1) - 1) for _ in range(n)]

    cases = []
    # (source WAV, target rate, bits, channels, L)
    cases.append((_wav_of(8000, 1, 1, 8, ints(20, 8)), 8000, 8, 1, 8))           # identity, 3 units
    cases.append((_wav_of(8000, 1, 1, 16, ints(20, 16)), 8000, 8, 1, 32))        # depth down, padding
    cases.append((_wav_of(44100, 2, 1, 16, ints(2 * 50, 16)), 8000, 8, 1, 16))   # mix, rate down
    cases.append((_wav_of(11025, 1, 1, 24, ints(30, 24)), 22050, 12, 2, 40))     # rate up, channel repeated
    cases.append((_wav_of(48000, 3, 1, 32, ints(3 * 25, 32)), 32000, 31, 2, 9))  # first two channels, 31 bits
    cases.append((_wav_of(16000, 2, 3, 32, [((i * 37) % 200 - 100) / 64.0 for i in range(2 * 21)]), 7, 5, 2, 3))  # float, clipping, odd rate
    cases.append((_wav_of(22050, 1, 3, 64, [math.sin(i / 3.0) for i in range(40)]), 22050, 16, 1, 40))  # double
    cases.append((_wav_of(8000, 2, 1, 16, ints(2 * 12, 16), extensible=True), 3, 1, 1, 2))  # extensible, 1-bit, 3 Hz
    cases.append((_wav_of(44100, 6, 1, 16, ints(6 * 9, 16), extensible=True, stream=True), 44100, 24, 6, 9))  # stream sizes, 6 ch
    cases.append((_wav_of(9600, 1, 1, 8, [0, 255, 128, 127, 1] * 4), 4800, 3, 1, 4))  # 8-bit extremes, halves
    cases.append((_wav_of(1, 1, 1, 16, [32767, -32768, 1]), 2, 16, 1, 6))           # 1 Hz up to 2 Hz
    cases.append((_wav_of(8000, 4, 1, 16, ints(4 * 7, 16)), 8000, 16, 1, 7))       # four mixed to one
    cases.append((_wav_of(8000, 1, 1, 32, [2**31 - 1, -2**31, 2**31 - 2**23, 2**23]), 8000, 8, 1, 4))  # rounded past the top: clipped
    return cases


# ---------------------------------------------------------------- canon-notes-v3 (open-ended notes)
# Written from SPECIFICATIONS 3.4 and sieve/notes3.hpp's rules, not from notes3.cpp.

N3_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
N3_LETTERS = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}
N3_CODES = {"s": 1, "e": 2, "e.": 3, "q": 4, "q.": 6, "h": 8, "h.": 12, "w": 16}  # sixteenths


def n3_name(m):
    return N3_NAMES[m % 12] + str(m // 12 - 1)


class Notes3:
    def __init__(self, low, high, tpq, longest, levels, voices, tempo, instruments):
        assert 0 <= low and high <= 127 and high >= low + 11 and 1 <= tpq <= 960 and 1 <= longest <= 65535
        assert 1 <= levels <= 127 and 1 <= voices <= 15 and 1 <= tempo <= 1000
        if len(instruments) == 1:
            instruments = instruments * voices
        assert len(instruments) == voices
        self.low, self.high, self.tpq, self.longest, self.levels = low, high, tpq, longest, levels
        self.voices, self.tempo, self.instruments = voices, tempo, instruments

    def id(self):
        return (f"notes3/{n3_name(self.low)}..{n3_name(self.high)}/q{self.tpq}/d{self.longest}/v{self.levels}/V{self.voices}"
                f"/t{self.tempo}/i{','.join(map(str, self.instruments))}")

    def velocity(self, k):
        return _round_half_up(Fraction(127 * k, self.levels))

    def default_level(self):
        # The level whose velocity is nearest 96, a tie to the louder.
        return min(range(1, self.levels + 1), key=lambda k: (abs(self.velocity(k) - 96), -k))

    def digit(self, rest, midi, level, ticks):
        cls = 0 if rest else 1 + (midi - self.low) * self.levels + (level - 1)
        return (ticks - 1) + self.longest * cls

    def parts(self, d):
        cls, ticks = divmod(d, self.longest)
        if cls == 0:
            return None, None, ticks + 1
        p, k = divmod(cls - 1, self.levels)
        return self.low + p, k + 1, ticks + 1


def canon_notes3(text, ns, L):
    """units, and the report (events, flats, octave, default lengths, default levels, rounded,
    split, clamped, padding)."""
    rep = dict(events=0, flats=0, octave=0, dlen=0, dlev=0, rounded=0, split=0, clamped=0, padding=0)
    parts = text.split("//")
    if len(parts) > ns.voices:
        raise ValueError("too many voices")
    voices = [[] for _ in range(ns.voices)]
    for v, part in enumerate(parts):
        for tok in re.split(r"[\s|,]+", part):
            if not tok:
                continue
            m = re.fullmatch(r"(?:([A-Ga-g])([#b]?)(-?\d)|([Rr]))(?::(\d+)|([sehqwSEHQW]\.?))?(?:!(\d+))?", tok)
            if not m:
                raise ValueError(f"cannot read {tok}")
            rest = m[4] is not None
            midi = 0
            if not rest:
                semi = N3_LETTERS[m[1].upper()] + (1 if m[2] == "#" else -1 if m[2] == "b" else 0)
                if m[2] == "b":
                    rep["flats"] += 1
                midi = (int(m[3]) + 1) * 12 + semi
                shifted = False
                while midi < ns.low:
                    midi += 12
                    shifted = True
                while midi > ns.high:
                    midi -= 12
                    shifted = True
                rep["octave"] += shifted
            if m[5] is not None:
                ticks = int(m[5])
                if ticks == 0:
                    raise ValueError("a length of 0")
            elif m[6] is not None:
                six = N3_CODES[m[6].lower()]
                exact = Fraction(six * ns.tpq, 4)
                ticks = max(1, _round_half_up(exact))
                rep["rounded"] += exact.denominator != 1
            else:
                ticks = ns.tpq
                rep["dlen"] += 1
            level = ns.default_level()
            if m[7] is not None:
                if rest:
                    raise ValueError("a rest has no level")
                lv = int(m[7])
                if lv == 0:
                    raise ValueError("level 0")
                rep["clamped"] += lv > ns.levels
                level = min(lv, ns.levels)
            elif not rest:
                rep["dlev"] += 1
            rep["split"] += ticks > ns.longest
            first = True
            while ticks > 0:
                t = min(ticks, ns.longest)
                voices[v].append(ns.digit(rest or not first, midi, level, t))
                ticks -= t
                first = False
            rep["events"] += 1
    longest = max(len(x) for x in voices)
    runs = max(1, -(-longest // L))
    units = []
    for k in range(runs):
        u = []
        for x in voices:
            for e in range(L):
                at = k * L + e
                if at < len(x):
                    u.append(x[at])
                else:
                    u.append(0)
                    rep["padding"] += 1
        units.append(u)
    return units, rep


def notes3_notation(ns, unit):
    per = len(unit) // ns.voices
    out = []
    for v in range(ns.voices):
        toks = []
        for d in unit[v * per:(v + 1) * per]:
            midi, level, ticks = ns.parts(d)
            toks.append(f"R:{ticks}" if midi is None else f"{n3_name(midi)}:{ticks}!{level}")
        out.append(" ".join(toks))
    return " // ".join(out)


def notes3_midi(ns, unit):
    def vlq(v):
        b = [v & 0x7F]
        v >>= 7
        while v:
            b.append(0x80 | (v & 0x7F))
            v >>= 7
        return bytes(reversed(b))

    def chunk(t):
        return b"MTrk" + struct.pack(">I", len(t)) + t
    us = _round_half_up(Fraction(60000000, ns.tempo))
    out = b"MThd" + struct.pack(">IHHH", 6, 1, 1 + ns.voices, ns.tpq)
    out += chunk(vlq(0) + bytes([0xFF, 0x51, 0x03]) + us.to_bytes(3, "big") + vlq(0) + bytes([0xFF, 0x2F, 0]))
    per = len(unit) // ns.voices
    for v in range(ns.voices):
        ch = v if v < 9 else v + 1
        t = vlq(0) + bytes([0xC0 | ch, ns.instruments[v]])
        pending = 0
        for d in unit[v * per:(v + 1) * per]:
            midi, level, ticks = ns.parts(d)
            if midi is None:
                pending += ticks
                continue
            t += vlq(pending) + bytes([0x90 | ch, midi, ns.velocity(level)]) + vlq(ticks) + bytes([0x80 | ch, midi, 0])
            pending = 0
        t += vlq(pending) + bytes([0xFF, 0x2F, 0])
        out += chunk(t)
    return out


NOTES3_CASES = [
    # (low, high, tpq, longest, levels, voices, tempo, instruments, L, notation)
    ("C-1", "G9", 4, 16, 8, 1, 120, [0], 6, "C#4:3!5 R:1 Eb4q C4:40 G9:1!8 C-1e"),
    ("C-1", "G9", 4, 16, 8, 2, 90, [0, 40], 5, "C4 D4:2!1 // C2:16!3 R:2 A4h."),
    ("C3", "C6", 12, 48, 4, 1, 120, [19], 4, "C4e. D4q. E4s!9 F8w Bb1:100"),
    ("C2", "B2", 3, 6, 1, 3, 60, [0, 1, 2], 2, "C2s // D2e // E2q"),
    ("C4", "B4", 1, 1, 127, 1, 1000, [127], 3, "C4!127 D4!1 R E4:3!64"),
    ("C-1", "G9", 960, 3840, 16, 15, 120, [0], 1, " // ".join(f"C{v % 10}w!{v + 1}" for v in range(15))),
    ("C-1", "B0", 2, 4, 2, 2, 120, [0], 3, "// C9e B-1:5"),
    ("A0", "C8", 8, 64, 10, 1, 72, [5], 4, "a0 c8:64!10 Ab4q.!3 r:65"),
]


def cmd_notes3_vectors(args):
    """canon-notes-v3: the set, events per voice and notation in; every unit's digits, the report,
    the first unit's notation and its MIDI file's SHA-256 out."""
    print("# set\tlength\tnotation\tunits\treport\tnotation_out\tmidi_sha256")
    for low, high, tpq, longest, levels, voices, tempo, ins, L, text in NOTES3_CASES:
        ns = Notes3(_n3_midi(low), _n3_midi(high), tpq, longest, levels, voices, tempo, ins)
        units, rep = canon_notes3(text, ns, L)
        report = ",".join(str(rep[k]) for k in ("events", "flats", "octave", "dlen", "dlev", "rounded", "split", "clamped", "padding"))
        print(f"{ns.id()}\t{L}\t{text}\t{';'.join(','.join(map(str, u)) for u in units)}\t{report}\t"
              f"{notes3_notation(ns, units[0])}\t{hashlib.sha256(notes3_midi(ns, units[0])).hexdigest()}")


def _n3_midi(name):
    m = re.fullmatch(r"([A-G])([#b]?)(-?\d)", name)
    return (int(m[3]) + 1) * 12 + N3_LETTERS[m[1]] + (1 if m[2] == "#" else -1 if m[2] == "b" else 0)


# ---- the sound filters (core/src/filters/sound.cpp), counted their own way: one channel by a
# table over the last sample or the silent run, the channels as a number in base (one channel's count).

def _pcm_signed(bits, d):
    return d - 2**bits if d >= 2**(bits - 1) else d


def _sound_rule(name, bits, pct):
    """A channel's rule: (allowed first digits, allowed next digit given the state, next state)."""
    B = 2**bits
    if name == "peak":
        a = 2**(bits - 1) * pct // 100
        return (lambda st, d: -a <= _pcm_signed(bits, d) <= a), (lambda st, d: 0), 0
    if name == "step":
        lim = 2**bits * pct // 100
        return (lambda st, d: st is None or abs(_pcm_signed(bits, d) - _pcm_signed(bits, st)) <= lim), (lambda st, d: d), None
    if name == "silence":
        n = pct  # the most silent samples in a run
        return (lambda st, d: d != 0 or st + 1 <= n), (lambda st, d: st + 1 if d == 0 else 0), 0
    raise ValueError(name)


def _sound_table(name, bits, pct, L):
    """completions[r][state]: the ways to finish a channel with r samples left, from each state."""
    B = 2**bits
    ok, step, start = _sound_rule(name, bits, pct)
    from functools import lru_cache

    @lru_cache(maxsize=None)
    def comp(r, st):
        if r == 0:
            return 1
        return sum(comp(r - 1, step(st, d)) for d in range(B) if ok(st, d))
    return comp, ok, step, start


def _peak_digits(bits, pct):
    """sound-peak's survivors' digits, in digit order: 0 up to the limit, then the negative ones."""
    a = 2**(bits - 1) * pct // 100
    hi, lo = min(a, 2**(bits - 1) - 1), min(a, 2**(bits - 1))
    return hi, lo


def sound_count(name, bits, ch, L, pct):
    if name == "peak":
        hi, lo = _peak_digits(bits, pct)
        return (hi + 1 + lo) ** (L * ch)
    comp, _, _, start = _sound_table(name, bits, pct, L)
    return comp(L, start) ** ch


def sound_unrank(name, bits, ch, L, pct, k):
    if name == "peak":
        # Every sample on its own: the rank is a number in base |set|, each digit its sample's place.
        hi, lo = _peak_digits(bits, pct)
        n = hi + 1 + lo
        out = []
        for _ in range(L * ch):
            i = k % n
            k //= n
            out.append(i if i <= hi else 2**bits - lo + (i - hi - 1))
        return out[::-1]
    comp, ok, step, start = _sound_table(name, bits, pct, L)
    per = comp(L, start)
    parts = []
    for _ in range(ch):
        parts.append(k % per)
        k //= per
    out = []
    for part in reversed(parts):
        st = start
        for r in range(L, 0, -1):
            for d in range(2**bits):
                if not ok(st, d):
                    continue
                c = comp(r - 1, step(st, d))
                if part < c:
                    out.append(d)
                    st = step(st, d)
                    break
                part -= c
    return out


def _sound_ok_whole(name, bits, ch, L, pct, u):
    ok, step, start = _sound_rule(name, bits, pct)
    for c in range(ch):
        st = start
        for d in u[c * L:(c + 1) * L]:
            if not ok(st, d):
                return False
            st = step(st, d)
    return True


def cmd_sound_vectors(args):
    """The sound filters: survivors of a pcm line (bits, channels, samples a channel) under each, and
    a few survivors by rank."""
    from itertools import product
    print("# Sound filters (core/src/filters/sound.cpp): sound-peak-v1 (percent), sound-step-v1 (percent),")
    print("# silence-run-v1 (samples). The channels are judged one at a time.")
    print("# count  filter  bits  channels  samples  value  survivors")
    print("# unit   filter  bits  channels  samples  value  rank  digits(comma)")
    # Every unit of small lines against the counts and the unranking.
    for name, bits, ch, L, v in (("peak", 2, 1, 4, 50), ("peak", 3, 2, 2, 60), ("peak", 1, 1, 5, 100), ("step", 2, 1, 5, 25),
                                 ("step", 3, 2, 2, 30), ("step", 2, 1, 4, 100), ("silence", 2, 1, 6, 1), ("silence", 1, 2, 3, 0),
                                 ("silence", 3, 1, 4, 2)):
        kept = [list(u) for u in product(range(2**bits), repeat=ch * L) if _sound_ok_whole(name, bits, ch, L, v, u)]
        assert len(kept) == sound_count(name, bits, ch, L, v), (name, bits, ch, L, v)
        assert all(sound_unrank(name, bits, ch, L, v, i) == u for i, u in enumerate(kept))
        print(f"count\tsound-{name}\t{bits}\t{ch}\t{L}\t{v}\t{len(kept)}")
        for i in sorted({0, len(kept) // 3, len(kept) - 1}):
            print(f"unit\tsound-{name}\t{bits}\t{ch}\t{L}\t{v}\t{i}\t{','.join(map(str, kept[i]))}")
    # Larger: by the tables alone.
    g = stream("sound")
    for name, bits, ch, L, v in (("peak", 8, 1, 64, 90), ("peak", 16, 2, 32, 50), ("peak", 31, 1, 8, 75), ("step", 8, 1, 40, 10),
                                 ("step", 6, 2, 30, 25), ("silence", 8, 1, 60, 5), ("silence", 4, 3, 20, 0)):
        n = sound_count(name, bits, ch, L, v)
        print(f"count\tsound-{name}\t{bits}\t{ch}\t{L}\t{v}\t{n}")
        for i in sorted({0, n // 7, n - 1, int.from_bytes(bytes(next(g) for _ in range(40)), "big") % n}):
            u = sound_unrank(name, bits, ch, L, v, i)
            assert _sound_ok_whole(name, bits, ch, L, v, u)
            print(f"unit\tsound-{name}\t{bits}\t{ch}\t{L}\t{v}\t{i}\t{','.join(map(str, u))}")


def cmd_pcm_vectors(args):
    """canon-pcm-v1: the source WAV file (hex), the set and L in; every unit's digits, the report and
    the first unit's WAV file's SHA-256 out."""
    print("# set\tlength\twav_hex\tunits\treport\twav_sha256")
    for wav, rate, bits, ch, L in _pcm_cases():
        f = PcmFormat(rate, bits, ch)
        units, rep = canon_pcm(wav, f, L)
        print(f"{f.id()}\t{L}\t{wav.hex()}\t{';'.join(','.join(map(str, u)) for u in units)}\t{','.join(map(str, rep))}\t"
              f"{hashlib.sha256(pcm_wav(f, units[0])).hexdigest()}")


# ---------------------------------------------------------------- filter plugins (sieve-filter-v1)
# Written from the format in core/include/sieve/plugin.hpp and docs/FILTER-PLUGINS.md, not from
# plugin.cpp: its own tokenizer, expression reader and interpreter, and Hopcroft's minimisation
# where the engine uses Moore's (both end in the one canonical numbering: breadth first from the
# start, symbols in order), so the two agreeing on every reference plugin is a real check.

PLUGIN_FORMAT = "sieve-filter-v1"
PLUGIN_FORMAT2 = "sieve-filter-v2"  # v1, and comparisons, && || !, min max abs, if/else/fi, choice
                                    # parameters, the line's constants, and symbols families (notes*)
NOTES_BASE = 104
V2_RESERVED = {"min", "max", "abs", "BASE", "PITCHES", "DURATIONS", "LOW", "LENGTH", "SIXTEENTHS",
               "LEVELS", "LONGEST", "TPQ", "NOTE", "REST"}  # the last five: a notes3 line's (notes3*)
SIXTEENTHS = [1, 2, 3, 4, 6, 8, 12, 16]  # s e E q Q h H w, in sixteenths


class PluginError(Exception):
    pass


def _plugin_tokens(line, n):
    toks, i = [], 0
    while i < len(line):
        c = line[i]
        if c in " \t":
            i += 1
            continue
        if c == ";":
            break
        if c == '"':
            i += 1
            text, closed = "", False
            while i < len(line):
                ch = line[i]
                i += 1
                if ch == '"':
                    closed = True
                    break
                if ch == "\\":
                    if i >= len(line):
                        raise PluginError(f"line {n}: a quoted string ends in a backslash")
                    e = line[i]
                    i += 1
                    if e not in 'nt"\\':
                        raise PluginError(f"line {n}: unknown escape \\{e}")
                    text += {"n": "\n", "t": "\t"}.get(e, e)
                else:
                    text += ch
            if not closed:
                raise PluginError(f"line {n}: a quoted string is not closed")
            toks.append((text, True))
            continue
        depth, text = 0, ""
        while i < len(line) and (depth > 0 or line[i] not in ' \t;"'):
            if line[i] == "{":
                depth += 1
            if line[i] == "}":
                depth -= 1
                if depth < 0:
                    raise PluginError(f"line {n}: a }} without its {{")
            text += line[i]
            i += 1
        if depth:
            raise PluginError(f"line {n}: a {{ without its }}")
        toks.append((text, False))
    return toks


def _plugin_expr(s, env, n, v2=False):
    """+ - * / % and brackets over integers and names; / and % for non-negative numbers only.
    v2 adds, loosest first: ||, &&, the comparisons (1 or 0), then + - and * / %, with unary - and
    !, and min(a, b), max(a, b), abs(a). Every operand is worked out, whatever the other side."""
    if v2:
        toks = re.findall(r"\s*(\d+|[A-Za-z_]\w*|==|!=|<=|>=|&&|\|\||[-+*/%()<>!,]|\S)", s)
    else:
        toks = re.findall(r"\s*(\d+|[A-Za-z_]\w*|[-+*/%()]|\S)", s)
    pos = [0]
    LIMIT = 1 << 62

    def chk(v):
        if abs(v) > LIMIT:
            raise PluginError(f"line {n}: a number is too large")
        return v

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else None

    def take():
        pos[0] += 1
        return toks[pos[0] - 1]

    def atom():
        t = peek()
        if t is None:
            raise PluginError(f"line {n}: an expression ends too soon: '{s}'")
        if t == "-":
            take()
            return chk(-atom())
        if v2 and t == "!":
            take()
            return 0 if atom() else 1
        if t == "(":
            take()
            v = top()
            if peek() != ")":
                raise PluginError(f"line {n}: a ( without its ) in '{s}'")
            take()
            return v
        take()
        if t.isdigit():
            return chk(int(t))
        if v2 and t in ("LENGTH", "SIXTEENTHS"):
            # a duration's length in sixteenths: the k-th code's, or the line's own i-th duration's
            if peek() != "(":
                raise PluginError(f"line {n}: {t} takes its argument in brackets")
            take()
            a = top()
            if peek() != ")":
                raise PluginError(f"line {n}: a ( without its ) in '{s}'")
            take()
            if t == "SIXTEENTHS":
                if not 0 <= a <= 7:
                    raise PluginError(f"line {n}: SIXTEENTHS takes a code's place, 0 (s) to 7 (w)")
                return SIXTEENTHS[a]
            lengths = env.get("#lengths", [])
            if not 0 <= a < len(lengths):
                raise PluginError(f"line {n}: LENGTH(i) is the line's i-th duration")
            return lengths[a]
        if v2 and t in ("NOTE", "REST"):
            # a notes3 line's symbols: NOTE(p, k, n) pitch p (1 = LOW) at level k for n ticks, REST(n)
            if peek() != "(":
                raise PluginError(f"line {n}: {t} takes its arguments in brackets")
            take()
            args = [top()]
            while peek() == ",":
                take()
                args.append(top())
            if peek() != ")":
                raise PluginError(f"line {n}: a ( without its ) in '{s}'")
            take()
            if len(args) != (3 if t == "NOTE" else 1):
                raise PluginError(f"line {n}: {t} takes the wrong number of arguments")
            if "LONGEST" not in env:
                raise PluginError(f"line {n}: {t} is for a notes3 line (symbols notes3*)")
            ticks = args[-1]
            if not 1 <= ticks <= env["LONGEST"]:
                raise PluginError(f"line {n}: {t}'s ticks must be 1 to LONGEST")
            if t == "REST":
                return ticks - 1
            if not 1 <= args[0] <= env["PITCHES"] or not 1 <= args[1] <= env["LEVELS"]:
                raise PluginError(f"line {n}: NOTE's pitch or level is out of range")
            return (ticks - 1) + env["LONGEST"] * (1 + (args[0] - 1) * env["LEVELS"] + (args[1] - 1))
        if v2 and t in ("min", "max", "abs"):
            if peek() != "(":
                raise PluginError(f"line {n}: {t} takes its arguments in brackets")
            take()
            args = [top()]
            while peek() == ",":
                take()
                args.append(top())
            if peek() != ")":
                raise PluginError(f"line {n}: a ( without its ) in '{s}'")
            take()
            if len(args) != (1 if t == "abs" else 2):
                raise PluginError(f"line {n}: {t} takes the wrong number of arguments")
            return chk(abs(args[0])) if t == "abs" else (min(args) if t == "min" else max(args))
        if re.fullmatch(r"[A-Za-z_]\w*", t):
            if t not in env:
                raise PluginError(f"line {n}: '{t}' is not a parameter or a for variable")
            return env[t]
        raise PluginError(f"line {n}: cannot read the expression '{s}'")

    def mul():
        v = atom()
        while peek() in ("*", "/", "%"):
            op = take()
            r = atom()
            if op == "*":
                v = chk(v * r)
            else:
                if v < 0 or r <= 0:
                    raise PluginError(f"line {n}: {op} is for a number of 0 or more by one of 1 or more")
                v = v // r if op == "/" else v % r
        return v

    def add():
        v = mul()
        while peek() in ("+", "-"):
            op = take()
            r = mul()
            v = chk(v + r if op == "+" else v - r)
        return v

    def cmp():
        v = add()
        while peek() in ("==", "!=", "<", "<=", ">", ">="):
            op = take()
            r = add()
            v = int({"==": v == r, "!=": v != r, "<": v < r, "<=": v <= r, ">": v > r, ">=": v >= r}[op])
        return v

    def conj():
        v = cmp()
        while peek() == "&&":
            take()
            r = cmp()
            v = int(bool(v) and bool(r))
        return v

    def disj():
        v = conj()
        while peek() == "||":
            take()
            r = conj()
            v = int(bool(v) or bool(r))
        return v

    top = disj if v2 else add
    v = top()
    if pos[0] != len(toks):
        raise PluginError(f"line {n}: cannot read the expression '{s}'")
    return v


def _plugin_num(tok, env, n, v2=False):
    text, quoted = tok
    if quoted:
        raise PluginError(f"line {n}: expected a number, got a quoted string")
    if len(text) >= 2 and text[0] == "{" and text[-1] == "}":
        text = text[1:-1]
    elif "{" in text or "}" in text:
        raise PluginError(f"line {n}: an expression must be all inside {{braces}}: '{text}'")
    return _plugin_expr(text, env, n, v2)


def _split_range(s):
    depth = 0
    for i in range(len(s) - 1):
        depth += (s[i] == "{") - (s[i] == "}")
        if depth == 0 and s[i:i + 2] == "..":
            return s[:i], s[i + 2:]
    return None


def parse_plugin(text):
    head = {"params": [], "lines": [], "requires": []}
    body, n, first, ended, stack = [], 0, True, False, []
    for raw in text.split("\n"):
        n += 1
        if raw.endswith("\r"):
            raise PluginError(f"line {n}: line breaks must be line feeds only")
        st = raw.lstrip(" \t")
        if not st or st[0] in "#;":
            continue
        toks = _plugin_tokens(raw, n)
        if not toks:
            continue
        if ended:
            raise PluginError(f"line {n}: nothing may follow end")
        if first:
            if toks not in ([(PLUGIN_FORMAT, False)], [(PLUGIN_FORMAT2, False)]):
                raise PluginError(f"line {n}: a plugin starts with {PLUGIN_FORMAT} or {PLUGIN_FORMAT2}")
            head["v2"] = toks[0][0] == PLUGIN_FORMAT2
            first = False
            continue
        k = toks[0][0]
        rest = raw.split(";")[0].strip(" \t")[len(k):].strip(" \t")
        if k in ("id", "version", "origin", "symbols"):
            if len(toks) != 2:
                raise PluginError(f"line {n}: {k} takes 1 value")
            head[k] = toks[1][0]
        elif k in ("author", "describe"):
            head[k] = rest
        elif k == "lines":
            head["lines"] = [t for t, _ in toks[1:]]
        elif k == "requires":
            # a filter by its full name, then settings it pins, NAME=VALUE (kept in sorted order,
            # as the engine keeps them)
            name = toks[1][0]
            if not re.fullmatch(r".+-v[0-9]+", name):
                raise PluginError(f"line {n}: requires names a filter with its version, as <id>-v<n>")
            pins = dict(t.split("=", 1) for t, _ in toks[2:])
            head["requires"].append((name, pins))
        elif k == "param" and toks[2][0] == "dict":
            head["params"].append((toks[1][0], "" if toks[3][0] == "default" else toks[3][0], "dict", None))
        elif k == "tokens":
            head["separator"] = toks[2:]
        elif k == "edges":
            head["edges"] = toks[1][0]
        elif k == "padding":
            # padding trailing: a unit ending in two or more separators passes when what comes
            # before them passes on its own
            if toks[1][0] != "trailing":
                raise PluginError(f"line {n}: padding is trailing")
            head["padding"] = True
        elif k == "within":
            # within N: past the first N symbols, only separators
            head["within"] = (toks[1], n)
        elif k == "set":
            kind, _, src = toks[2][0].partition(":")
            head.setdefault("sets", []).append((toks[1][0], kind, src))
        elif k == "follow":
            head.setdefault("follow", []).append((toks[1][0], toks[2][0]))
        elif k in ("first", "last"):
            head[k] = [t for t, _ in toks[1:]]
        elif k == "param" and head["v2"] and toks[2][0] == "choice":
            # param NAME choice DEFAULT A,B,C: in expressions, its place in the list
            name, choices = toks[1][0], toks[4][0].split(",")
            if name in V2_RESERVED or toks[3][0] not in choices:
                raise PluginError(f"line {n}: a bad choice parameter")
            # A choice is ASCII letters, digits, # and - (FILTER-PLUGINS §14), and listed once.
            if any(not re.fullmatch(r"[A-Za-z0-9#-]+", c) for c in choices) or len(set(choices)) != len(choices):
                raise PluginError(f"line {n}: a choice is letters, digits, # and -, each listed once")
            head["params"].append((name, toks[3][0], "choice", choices))
        elif k == "param":
            name, kind = toks[1][0], toks[2][0]
            if kind != "int":
                raise PluginError(f"line {n}: a parameter's kind is int or dict")
            if head["v2"] and name in V2_RESERVED:
                raise PluginError(f"line {n}: {name} is kept for the format")
            d, lo, hi = (_plugin_num(t, {}, n, head["v2"]) for t in toks[3:6])
            if not lo <= d <= hi:
                raise PluginError(f"line {n}: a parameter's default must lie between its minimum and maximum")
            head["params"].append((name, d, lo, hi))
        elif k in ("class", "states", "start", "accept", "t", "for", "done") or (head["v2"] and k in ("if", "else", "fi")):
            body.append((n, toks))
            if k in ("for", "if"):
                stack.append(k)
            if k == "done":
                if not stack or stack[-1] != "for":
                    raise PluginError(f"line {n}: done without its for")
                stack.pop()
            if k == "else" and (not stack or stack[-1] != "if"):
                raise PluginError(f"line {n}: else without its if")
            if k == "fi":
                if not stack or stack[-1] != "if":
                    raise PluginError(f"line {n}: fi without its if")
                stack.pop()
        elif k == "end":
            if stack:
                raise PluginError(f"line {n}: a for is not closed by done")
            ended = True
        else:
            raise PluginError(f"line {n}: unknown keyword '{k}'")
    if not ended:
        raise PluginError("a plugin ends with end")
    for key in ("id", "version", "author", "origin", "symbols"):
        if key not in head:
            raise PluginError(f"the header has no {key}")
    head["name"] = f"{head['id']}-v{int(head['version'])}"
    head["form"] = "tokens" if "sets" in head else "table"
    return head, body


def plugin_base(symbols, base=None):
    if symbols in ALPHABETS:
        return len(ALPHABETS[symbols])
    if symbols in ("notes104", "notes*"):  # notes*: the note lines (only notes104 so far)
        return NOTES_BASE
    if symbols.startswith("palette:"):
        return PALETTE_SIZES[symbols[8:]]
    if symbols == "notes3*":  # the notes3 set's own (cmd_plugin)
        return base
    if base is None:
        raise PluginError(f"give --base for symbols {symbols}")
    return base


def compile_plugin(head, body, values, base, notes=None):
    env = {}
    v2 = head.get("v2", False)
    if v2:
        env["BASE"] = base
        if isinstance(notes, Notes3):  # a notes3 set (notes3*): NOTE() and REST() write its symbols
            env.update(PITCHES=notes.high - notes.low + 1, LOW=notes.low, LEVELS=notes.levels, LONGEST=notes.longest, TPQ=notes.tpq)
        elif notes is not None:  # a notes2 set: its own
            env.update(PITCHES=notes.P, DURATIONS=notes.D, LOW=notes.low)
            env["#lengths"] = [SIXTEENTHS[NOTE2_CODES.index(c)] for c in notes.durations]
        elif head["symbols"].startswith("notes"):
            env.update(PITCHES=25, DURATIONS=4, LOW=60)  # notes104: C4 (MIDI 60) .. C6, e q h w
            env["#lengths"] = [2, 4, 8, 16]
    for name, d, lo, hi in head["params"]:
        if lo == "dict":
            continue
        if lo == "choice":
            v = values.get(name, d)
            if v not in hi:
                raise PluginError(f"{head['name']}: {name} must be one of its choices")
            env[name] = hi.index(v)
            continue
        v = int(values.get(name, d))
        if not lo <= v <= hi:
            raise PluginError(f"{head['name']}: {name} must be {lo}..{hi}")
        env[name] = v
    alpha = ALPHABETS.get(head["symbols"])

    def sym_of(ch, n):
        if alpha is None:
            raise PluginError(f"line {n}: a quoted or lettered symbol needs a text line")
        if ch not in alpha:
            raise PluginError(f"line {n}: U+{ord(ch):04X} is not a symbol of {head['symbols']}")
        return alpha.index(ch)

    def digit(expr, n):
        v = _plugin_num((expr, False), env, n, v2)
        if not 0 <= v < base:
            raise PluginError(f"line {n}: @{v} is not a symbol of this line")
        return v

    def item(tok, n):
        text, quoted = tok
        if quoted:
            return {sym_of(c, n) for c in text}
        if text.startswith("@"):
            r = _split_range(text)
            if r:
                lo, hi = digit(r[0][1:], n), digit(r[1][1:], n)
                return set(range(lo, hi + 1))
            return {digit(text[1:], n)}
        if len(text) == 3 and text[1] == "-":
            return {sym_of(chr(c), n) for c in range(ord(text[0]), ord(text[2]) + 1)}
        raise PluginError(f"line {n}: cannot read the symbols '{text}'")

    classes, owner, star = {}, {}, None
    for n, toks in body:
        if toks[0][0] != "class":
            continue
        name = toks[1][0]
        if toks[2:] == [("*", False)]:
            star = name
            continue
        syms = set()
        for t in toks[2:]:
            syms |= item(t, n)
        for d in syms:
            if d in owner:
                raise PluginError(f"line {n}: symbol @{d} is in both {owner[d]} and {name}")
            owner[d] = name
        classes[name] = sorted(syms)
    if star:
        classes[star] = [d for d in range(base) if d not in owner]

    state = {"N": None, "start": None, "next": None, "accept": None}

    def st(tok, n):
        v = _plugin_num(tok, env, n, v2)
        if not 0 <= v < state["N"]:
            raise PluginError(f"line {n}: state {v} does not exist")
        return v

    def run(i, j):
        while i < j:
            n, toks = body[i]
            k = toks[0][0]
            if k == "states":
                state["N"] = _plugin_num(toks[1], env, n, v2)
                state["next"] = [[-1] * base for _ in range(state["N"])]
                state["accept"] = [False] * state["N"]
            elif k == "start":
                state["start"] = st(toks[1], n)
            elif k == "accept":
                for t in toks[1:]:
                    r = _split_range(t[0])
                    if r:
                        for s in range(st((r[0], False), n), st((r[1], False), n) + 1):
                            state["accept"][s] = True
                    else:
                        state["accept"][st(t, n)] = True
            elif k == "t":
                a, b = st(toks[1], n), st(toks[3], n)
                syms = classes[toks[2][0]] if not toks[2][1] and toks[2][0] in classes else sorted(item(toks[2], n))
                for c in syms:
                    cur = state["next"][a][c]
                    if cur != -1 and cur != b:
                        raise PluginError(f"line {n}: state {a} on symbol @{c} goes to both {cur} and {b}")
                    state["next"][a][c] = b
            elif k == "for":
                var = toks[1][0]
                lo, hi = _plugin_num(toks[2], env, n, v2), _plugin_num(toks[3], env, n, v2)
                depth, end = 0, i + 1
                while True:
                    kk = body[end][1][0][0]
                    if kk == "for":
                        depth += 1
                    if kk == "done":
                        if depth == 0:
                            break
                        depth -= 1
                    end += 1
                if var in env:
                    raise PluginError(f"line {n}: '{var}' is already a parameter or a for variable")
                for v in range(lo, hi + 1):
                    env[var] = v
                    run(i + 1, end)
                env.pop(var, None)
                i = end
            elif k == "if":
                # its else and fi, at its own depth of ifs
                depth, fi, els = 0, i + 1, None
                while True:
                    kk = body[fi][1][0][0]
                    if kk == "if":
                        depth += 1
                    elif kk == "fi":
                        if depth == 0:
                            break
                        depth -= 1
                    elif kk == "else" and depth == 0:
                        els = fi
                    fi += 1
                if _plugin_num(toks[1], env, n, v2):
                    run(i + 1, els if els is not None else fi)
                elif els is not None:
                    run(els + 1, fi)
                i = fi
            i += 1

    run(0, len(body))
    return state["N"], state["start"], state["next"], state["accept"]


def dictionary_registry(folder="../data/dictionaries"):
    """dictionaries.tsv: id -> (path, sha256), and the default id."""
    reg, default = {}, None
    for line in open(os.path.join(folder, "dictionaries.tsv"), encoding="utf-8"):
        if line.startswith("#") or not line.strip():
            continue
        f = line.rstrip("\n").split("\t")
        reg[f[0]] = (os.path.join(folder, f[1]), f[4])
        if f[3] == "yes":
            default = f[0]
    return reg, default


def token_nfa(head, values, base, folder):
    """The token form, built another way than the engine's: an NFA with one state per (trie node,
    reading of the token before), and for a cut first token one per trie node it could be at.
    Returns (distinct words, step(state, symbol) -> states, accepting(state), data); the start is
    state 0."""
    alpha = ALPHABETS[head["symbols"]]
    sep = set()
    for text, quoted in head["separator"]:
        chars = text if quoted else "".join(chr(c) for c in range(ord(text[0]), ord(text[2]) + 1))
        sep |= {alpha.index(c) for c in chars}
    names = [s[0] for s in head["sets"]]
    # the trie: children per node, and which sets each node ends a word of
    kids, ends, data = [{}], [set()], []
    for i, (name, kind, src) in enumerate(head["sets"]):
        if kind == "dict":
            if src.startswith("{"):
                pn = src[1:-1]
                src = values.get(pn, next(d for n_, d, lo, hi in head["params"] if n_ == pn))
            reg, default = dictionary_registry()
            ident = src or default
            path, sha = reg[ident]
            words = sorted(load_dict(path)[0])
            data.append(f"{name}=dict:{src or 'default'}={sha}")
        elif kind == "tags":
            # word<TAB>tags lines; the words carrying any of the tags asked for, spelled as the
            # line can: as they are, or lower-cased (A-Z and the Latin-1 capitals) on a line
            # without capitals; others are left out
            files, _, spec = src.rpartition(":")
            want, _, unwanted = spec.partition("-")
            parts = [open(os.path.join(folder, f), "rb").read() for f in files.split("+")]
            data.append(f"{name}=tags:{src}=" + "+".join(hashlib.sha256(p).hexdigest() for p in parts))
            raw = b"".join(p if p.endswith(b"\n") or not p else p + b"\n" for p in parts)
            fold = not any("A" <= c <= "Z" for c in alpha)  # "a line without capitals": no A-Z in it
            # A word is judged by the tags of all its lines together (every file, every spelling
            # that comes to the same word here), so "OF" (noun) and "of" (preposition) are one word
            # with both tags.
            tags_of = {}
            for line in raw.decode("utf-8").split("\n"):
                w, tab, tags = line.partition("\t")
                if not tab or not w:
                    continue
                if fold:
                    w = "".join(chr(ord(c) + 32) if ("A" <= c <= "Z" or (0xC0 <= ord(c) <= 0xDE and ord(c) != 0xD7)) and c not in alpha else c for c in w)
                if all(c in alpha and alpha.index(c) not in sep for c in w):
                    tags_of[w] = tags_of.get(w, "") + tags
            words = [w for w, tags in tags_of.items() if set(tags) & set(want) and not set(tags) & set(unwanted)]
            kind = "dict"  # spelled already: anything left that does not fit is skipped, as a dictionary's
        else:
            raw = open(os.path.join(folder, src), "rb").read()
            words = [w for w in raw.decode("utf-8").split("\n") if w]
            data.append(f"{name}=list:{src}={hashlib.sha256(raw).hexdigest()}")
        for w in words:
            if any(c not in alpha or alpha.index(c) in sep for c in w):
                if kind == "list":
                    raise PluginError(f"'{w}' is not a word of {head['symbols']}'s symbols")
                continue
            node = 0
            for c in w:
                d = alpha.index(c)
                if d not in kids[node]:
                    kids[node][d] = len(kids)
                    kids.append({})
                    ends.append(set())
                node = kids[node][d]
            ends[node].add(i)
    n = len(names)
    follow = {r: set(range(n)) for r in range(n)} if not head.get("follow") else {r: set() for r in range(n)}
    for a, b in head.get("follow", []):
        follow[names.index(a)].add(names.index(b))
    # The NFA's states are numbered, not tupled, so a large word list fits in memory:
    # S0 (before anything) 0, S1 (after a leading separator) 1, B r (between tokens, the last
    # read as set r) 2 + r, In (trie node, reading p of the token before; p = n before the first
    # token) 2 + n + node * (n + 1) + p, and Suf node (a cut first token) after those.
    START = n
    follow[START] = {names.index(s) for s in head["first"]} if "first" in head else set(range(n))
    last = {names.index(s) for s in head["last"]} if "last" in head else set(range(n))
    cut = head["edges"] == "cut"
    NT = len(kids)
    IN0, SUF0 = 2 + n, 2 + n + NT * (n + 1)
    incoming = {}
    for node, ch in enumerate(kids):
        for d, t in ch.items():
            incoming.setdefault(d, set()).add(t)

    def step(q, c):
        out = set()
        if q == 0:
            if c in sep:
                out.add(1)
            elif cut:
                out |= {SUF0 + t for t in incoming.get(c, ())}
            elif c in kids[0]:
                out.add(IN0 + kids[0][c] * (n + 1) + START)
        elif q == 1:
            if c not in sep and c in kids[0]:
                out.add(IN0 + kids[0][c] * (n + 1) + START)
        elif q < IN0:
            if c not in sep and c in kids[0]:
                out.add(IN0 + kids[0][c] * (n + 1) + (q - 2))
        elif q < SUF0:
            node, p = divmod(q - IN0, n + 1)
            if c in sep:
                out |= {2 + r for r in ends[node] if r in follow[p]}
            elif c in kids[node]:
                out.add(IN0 + kids[node][c] * (n + 1) + p)
        else:
            node = q - SUF0
            if c in sep:
                if ends[node]:
                    out |= {2 + r for r in range(n)}
            elif c in kids[node]:
                out.add(SUF0 + kids[node][c])
        return out

    def accepting(q):
        if q < 2:
            return False
        if q < IN0:
            return (q - 2) in last
        if q < SUF0:
            node, p = divmod(q - IN0, n + 1)
            return cut or any(r in follow[p] and r in last for r in ends[node])
        return True  # Suf: the whole unit, a substring of a word

    if head.get("padding"):
        # Trailing padding, as two more NFA states: from any state where the unit could end, a
        # separator may also lead to PEND (one separator after a part that passes); a second leads
        # to PAD, which takes only separators and accepts.
        PEND, PAD = SUF0 + NT, SUF0 + NT + 1
        plain_step, plain_accepting = step, accepting

        def step(q, c):
            if q == PEND or q == PAD:
                return {PAD} if c in sep else set()
            out = plain_step(q, c)
            if c in sep and plain_accepting(q):
                out = out | {PEND}
            return out

        def accepting(q):
            if q == PAD:
                return True
            if q == PEND:
                return False
            return plain_accepting(q)

    words_n = sum(1 for e in ends if e)  # distinct words: the token form's "declared" figure
    return words_n, step, accepting, " ".join(data)


def token_nfa_dfa(head, values, base, folder):
    """The token form's NFA (token_nfa), determinised by the ordinary subset construction.
    Returns (states declared, start, next, accept, data)."""
    words_n, step, accepting, data = token_nfa(head, values, base, folder)
    start = frozenset([0])
    index, order, nxt, acc = {start: 0}, [start], [], []
    i = 0
    while i < len(order):
        S = order[i]
        acc.append(any(accepting(q) for q in S))
        row = array("i", [-1]) * base
        for c in range(base):
            T = frozenset(t for q in S for t in step(q, c))
            if not T:
                continue
            if T not in index:
                index[T] = len(order)
                order.append(T)
            row[c] = index[T]
        nxt.append(row)
        order[i] = None  # done with: only the index needs it now
        i += 1
    del index, order
    return words_n, 0, nxt, acc, data


class LazyTokens:
    """The token form judged, counted and ranked without building the whole automaton: the subsets
    of NFA states are made only as a walk reaches them, and completions are counted for the
    subsets reachable within the length. For word lists too large to determinise here in full;
    `plugin --lazy` reports as `plugin` does, except the minimal size, which it does not know."""

    def __init__(self, step, accepting, base):
        self.step, self.accepting, self.base = step, accepting, base
        self.ids, self.sets, self.moves, self.memo = {}, [], {}, {}

    def id_of(self, S):
        if S not in self.ids:
            self.ids[S] = len(self.sets)
            self.sets.append(S)
        return self.ids[S]

    def move(self, i, c):
        key = (i, c)
        if key not in self.moves:
            T = frozenset(t for q in self.sets[i] for t in self.step(q, c))
            self.moves[key] = self.id_of(T) if T else -1
        return self.moves[key]

    def count(self, i, r):
        """Units of r more symbols that the walk from subset i accepts (iterative, by depth)."""
        if (i, r) in self.memo:
            return self.memo[(i, r)]
        # the subsets reachable at each depth, then counts from the deepest up
        layers = [{i}]
        for d in range(r):
            nxt = set()
            for s in layers[-1]:
                if (s, r - d) in self.memo:
                    continue
                for c in range(self.base):
                    t = self.move(s, c)
                    if t >= 0:
                        nxt.add(t)
            layers.append(nxt)
        for d in range(r, -1, -1):
            left = r - d
            for s in layers[d]:
                if (s, left) in self.memo:
                    continue
                if left == 0:
                    self.memo[(s, 0)] = 1 if any(self.accepting(q) for q in self.sets[s]) else 0
                else:
                    total = 0
                    for c in range(self.base):
                        t = self.move(s, c)
                        if t >= 0:
                            total += self.memo[(t, left - 1)]
                    self.memo[(s, left)] = total
        return self.memo[(i, r)]

    def unrank(self, L, k):
        s, out = self.id_of(frozenset([0])), []
        for i in range(L):
            for c in range(self.base):
                t = self.move(s, c)
                if t < 0:
                    continue
                m = self.count(t, L - i - 1)
                if k < m:
                    out.append(c)
                    s = t
                    break
                k -= m
        return out

    def accepts(self, unit):
        S = frozenset([0])
        for c in unit:
            S = frozenset(t for q in S for t in self.step(q, c))
            if not S:
                return False
        return any(self.accepting(q) for q in S)


def minimise_dfa(start, nxt, acc, base):
    """Trim to the live states, merge by Hopcroft's algorithm, number breadth first. The reverse
    transitions are kept as flat arrays (sources grouped by target, per symbol), so an automaton
    of millions of states fits in memory."""
    n = len(nxt)
    reach, todo = bytearray(n), [start]
    reach[start] = 1
    while todo:
        s = todo.pop()
        for t in nxt[s]:
            if t >= 0 and not reach[t]:
                reach[t] = 1
                todo.append(t)
    # predecessors over every symbol, grouped by target: sources back_src[back_at[t]:back_at[t + 1]]
    back_at = array("i", [0]) * (n + 1)
    for s in range(n):
        if reach[s]:
            for t in nxt[s]:
                if t >= 0:
                    back_at[t + 1] += 1
    for t in range(n):
        back_at[t + 1] += back_at[t]
    fill = array("i", back_at)
    back_src = array("i", [0]) * back_at[n]
    for s in range(n):
        if reach[s]:
            for t in nxt[s]:
                if t >= 0:
                    back_src[fill[t]] = s
                    fill[t] += 1
    del fill
    co, todo = bytearray(n), [s for s in range(n) if reach[s] and acc[s]]
    for s in todo:
        co[s] = 1
    while todo:
        s = todo.pop()
        for p in back_src[back_at[s]:back_at[s + 1]]:
            if not co[p]:
                co[p] = 1
                todo.append(p)
    del back_at, back_src
    live_list = [s for s in range(n) if reach[s] and co[s]]
    start_live = bool(co[start])
    del reach, co
    if not start_live:
        return None, [], []
    # The live states numbered 0..m-1 (idx), and the sink, DEAD = m: Hopcroft needs a complete
    # automaton, so every missing transition goes to the sink, which stays in a block of its own
    # (a live state can reach acceptance; the sink cannot).
    m = len(live_list)
    idx = array("i", [-1]) * n
    for k, s in enumerate(live_list):
        idx[s] = k
    DEAD = m

    def target(s, c):
        t = nxt[s][c]
        return idx[t] if t >= 0 and idx[t] >= 0 else DEAD

    # per symbol: sources grouped by target, rev_src[c][rev_at[c][t]:rev_at[c][t + 1]]
    rev_at, rev_src = [], []
    for c in range(base):
        at = array("i", [0]) * (m + 2)
        for s in live_list:
            at[target(s, c) + 1] += 1
        at[DEAD + 1] += 1  # the sink's own loop
        for t in range(m + 1):
            at[t + 1] += at[t]
        fill = array("i", at)
        src = array("i", [0]) * at[m + 1]
        for k, s in enumerate(live_list):
            t = target(s, c)
            src[fill[t]] = k
            fill[t] += 1
        src[fill[DEAD]] = DEAD
        rev_at.append(at)
        rev_src.append(src)
    # Hopcroft: blocks as sets, each state's block by index; a splitter's predecessors on each
    # symbol split every block they cut, and the smaller half (or both, if the block was waiting)
    # waits to split others.
    F = {k for k, s in enumerate(live_list) if acc[s]}
    blocks = [b for b in (F, set(range(m + 1)) - F) if b]
    block_of = array("i", [0]) * (m + 1)
    for i, b in enumerate(blocks):
        for s_ in b:
            block_of[s_] = i
    waiting = {min(range(len(blocks)), key=lambda i: len(blocks[i]))} if len(blocks) == 2 else set(range(len(blocks)))
    while waiting:
        A = list(blocks[waiting.pop()])
        for c in range(base):
            at, src = rev_at[c], rev_src[c]
            X = set()
            for t in A:
                X.update(src[at[t]:at[t + 1]])
            touched = {}
            for s_ in X:
                touched.setdefault(block_of[s_], set()).add(s_)
            for b, inter in touched.items():
                if len(inter) == len(blocks[b]):
                    continue
                nb = len(blocks)
                blocks[b] -= inter
                blocks.append(inter)
                for s_ in inter:
                    block_of[s_] = nb
                if b in waiting:
                    waiting.add(nb)
                else:
                    waiting.add(nb if len(inter) <= len(blocks[b]) else b)
    del rev_at, rev_src
    rep = {}
    for k in range(m):
        rep.setdefault(block_of[k], live_list[k])
    b0 = block_of[idx[start]]
    order, seq, q, qi = {b0: 0}, [], [b0], 0
    while qi < len(q):
        b = q[qi]
        qi += 1
        seq.append(b)
        s = rep[b]
        for c in range(base):
            t = target(s, c)
            if t != DEAD and block_of[t] not in order:
                order[block_of[t]] = len(order)
                q.append(block_of[t])
    m_next, m_acc = [], []
    for b in seq:
        s = rep[b]
        m_acc.append(acc[s])
        m_next.append([order[block_of[target(s, c)]] if target(s, c) != DEAD else -1 for c in range(base)])
    return 0, m_next, m_acc


def dfa_count_table(nxt, acc, L):
    n = len(nxt)
    table = [[1 if acc[s] else 0 for s in range(n)]]
    for _ in range(L):
        prev = table[-1]
        table.append([sum(prev[t] for t in nxt[s] if t >= 0) for s in range(n)])
    return table


def dfa_unrank(nxt, table, L, k):
    s, out = 0, []
    for i in range(L):
        for c, t in enumerate(nxt[s]):
            if t < 0:
                continue
            m = table[L - i - 1][t]
            if k < m:
                out.append(c)
                s = t
                break
            k -= m
    return out


def cmd_plugin(args):
    """One plugin file, reported line for line as `sieve filters --plugin` reports it."""
    data = open(args.file, "rb").read()
    head, body = parse_plugin(data.decode("utf-8"))
    values = dict(kv.split("=", 1) for kv in args.params.split(",")) if args.params else {}
    base = plugin_base(head["symbols"], args.base)
    notes, voices = None, 1
    if args.note_set == "notes2":
        # one voice's line; a unit is voices x --length, each voice judged on its own
        notes = NoteSet2(note2_midi(args.low or "C3"), note2_midi(args.high or "C6"), args.durations, args.voices)
        base, voices = notes.base, notes.voices
    elif args.note_set == "notes3" or head["symbols"] == "notes3*":
        # the same for notes3: base LONGEST * (1 + PITCHES * LEVELS)
        notes = Notes3(_n3_midi(args.low or "C-1"), _n3_midi(args.high or "G9"), args.tpq, args.longest, args.levels, args.voices, 120, [0])
        base, voices = notes.longest * (1 + (notes.high - notes.low + 1) * notes.levels), notes.voices
    word_data = ""
    lazy = None
    if head["form"] == "tokens" and args.lazy:
        N, step, accepting, word_data = token_nfa(head, values, base, os.path.dirname(os.path.abspath(args.file)))
        lazy = LazyTokens(step, accepting, base)
        m_next, m_acc = None, None
    elif head["form"] == "tokens":
        N, start, nxt, acc, word_data = token_nfa_dfa(head, values, base, os.path.dirname(os.path.abspath(args.file)))
    else:
        if args.lazy:
            raise SystemExit("--lazy is for the token form")
        N, start, nxt, acc = compile_plugin(head, body, values, base, notes)
    if lazy is None:
        s0, m_next, m_acc = minimise_dfa(start, nxt, acc, base)
    params = " ".join(f"{name}={values.get(name, d)}" for name, d, lo, hi in head["params"]) or "(none)"
    print(f"plugin     {head['name']}")
    print(f"sha256     {hashlib.sha256(data).hexdigest()}")
    print(f"origin     {head['origin']}")
    print(f"author     {head['author']}")
    print(f"symbols    {head['symbols']}")
    print(f"params     {params}")
    reqs = "; ".join(name + "".join(f" {k}={v}" for k, v in sorted(pins.items())) for name, pins in head["requires"])
    print(f"requires   {reqs or '(none)'}")
    print(f"form       {head['form']}" + (f"  {word_data}" if word_data else ""))
    kind_word = 'words' if head['form'] == 'tokens' else 'declared'
    print(f"states     {N} {kind_word}, " + ("(not determinised: --lazy)" if lazy else f"{len(m_next)} minimal"))
    L = args.length
    # within N: the automaton judges the first n = min(N, L) symbols, and each symbol after them is
    # any separator: so a survivor is the automaton's at n, then a string of separators, ranked
    # first by its first n symbols and then by the separators, in symbol order.
    within, seps = None, []
    if "within" in head:
        tok, wn = head["within"]
        env = {name: int(values.get(name, d)) for name, d, *rest in head["params"] if rest and rest[0] not in ("dict", "choice")}
        within = _plugin_num(tok, env, wn, head["v2"])
        print(f"within     {within}")
        alpha = ALPHABETS[head["symbols"]]
        for text, quoted in head["separator"]:
            chars = text if quoted else "".join(chr(c) for c in range(ord(text[0]), ord(text[2]) + 1))
            seps += [alpha.index(c) for c in chars]
        seps = sorted(set(seps))
    n_first = min(within, L) if within else L
    tail = len(seps) ** (L - n_first) if within else 1
    print(f"length     {args.length * voices}")
    if lazy:
        count = lazy.count(lazy.id_of(frozenset([0])), n_first)
    else:
        table = dfa_count_table(m_next, m_acc, n_first) if m_next else [[0]]
        count = table[n_first][0] if m_next else 0
    count *= tail
    one = count
    count = one ** voices  # every voice a survivor of its own
    print(f"survivors  {count}")
    print(f"excluded   {base ** (L * voices) - count}")
    if count:
        ks = [0]
        if count // 3 not in (0, count - 1):
            ks.append(count // 3)
        if count - 1 != 0:
            ks.append(count - 1)
        for k in ks:
            u, rest = [], k
            parts = []
            for _ in range(voices):  # the voices' ranks, as digits in base one voice's count
                rest, r = divmod(rest, one)
                parts.append(r)
            for part in reversed(parts):
                head_k, rest_k = divmod(part, tail)
                u += lazy.unrank(n_first, head_k) if lazy else dfa_unrank(m_next, table, n_first, head_k)
                for i in range(L - n_first - 1, -1, -1):  # the separators past n, most significant first
                    u.append(seps[rest_k // len(seps) ** i % len(seps)])
            print(f"rank       {k}  {','.join(map(str, u)) or '-'}")
    if args.judge:
        # each line of a text file judged as one unit of its own length, as `sieve filters --plugin
        # FILE --judge TEXT` does
        alpha = ALPHABETS.get(head["symbols"])
        if alpha is None:
            raise SystemExit("--judge reads text: the plugin is not for a text line")
        for line in open(args.judge, "rb").read().decode("utf-8").split("\n"):
            line = line[:-1] if line.endswith("\r") else line
            if not line or line.startswith("#"):  # a comment
                continue
            if any(ch not in alpha for ch in line):
                verdict = "unspellable"
            else:
                unit = [alpha.index(ch) for ch in line]
                # within N: the first N symbols judged, and only separators after them
                cut_at = min(within, len(unit)) if within is not None else len(unit)
                if any(c not in seps for c in unit[cut_at:]):
                    ok = False
                elif lazy:
                    ok = lazy.accepts(unit[:cut_at])
                else:
                    s = 0 if m_next else -1
                    for c in unit[:cut_at]:
                        if s < 0:
                            break
                        s = m_next[s][c]
                    ok = s >= 0 and m_acc[s]
                verdict = "pass" if ok else "FAIL"
            print(f"judge      {verdict}  {line}")


def cmd_manifest(args):
    """sieve-manifest-v1 (SPECIFICATIONS §12.2), from its definition: the folder walked to the bottom,
    links skipped, every folder ('/' at the end) and file (size, SHA-256) listed by its path relative to
    the root, UTF-8 with '/', sorted by the paths' bytes. With --addresses DIR, checks every file's
    address written there (DIR/<path>.hex) against binary-v1 and prints nothing else. --with-addresses
    gives v2 (each file's address in its line); --with-contents gives v3, an installer's manifest: the
    listing under 'sieve-manifest-v3', then every file's bytes one after another in the listing's order."""
    import os
    root = os.path.abspath(args.folder)
    entries, files, total = [], 0, 0
    if os.path.isfile(root):
        # A single file: as a folder holding just it, the root named after the file (§12.2).
        data = open(root, "rb").read()
        entries, files, total = [(os.path.basename(root), data)], 1, len(data)
    for here, dirs, names in ([] if os.path.isfile(root) else os.walk(root, followlinks=False)):
        for d in list(dirs):
            full = os.path.join(here, d)
            if os.path.islink(full):
                dirs.remove(d)
                continue
            entries.append((os.path.relpath(full, root).replace(os.sep, "/") + "/", None))
        for n in names:
            full = os.path.join(here, n)
            if os.path.islink(full) or not os.path.isfile(full):
                continue
            data = open(full, "rb").read()
            entries.append((os.path.relpath(full, root).replace(os.sep, "/"), data))
            files += 1
            total += len(data)
    entries.sort(key=lambda e: e[0].encode("utf-8"))
    if args.addresses:
        bad = 0
        for path, data in entries:
            if data is None:
                continue
            want = format(binary_index(data), "x")
            got = open(os.path.join(args.addresses, path + ".hex")).read().strip()
            if got != want:
                bad += 1
                print(f"address mismatch: {path}", file=sys.stderr)
        sys.exit(1 if bad else 0)
    v2 = args.with_addresses  # every file's binary-v1 address beside its hash
    v3 = args.with_contents   # an installer's manifest: the listing, then every file's bytes in order
    out = [f"sieve-manifest-v{3 if v3 else 2 if v2 else 1}", f"root {os.path.basename(root)}", f"files {files}", f"bytes {total}"]
    for path, data in entries:
        if data is None:
            out.append(f"d\t{path}")
        else:
            addr = f"{format(binary_index(data), 'x')}\t" if v2 else ""
            out.append(f"f\t{len(data)}\t{hashlib.sha256(data).hexdigest()}\t{addr}{path}")
    out.append("end")
    sys.stdout.buffer.write(("\n".join(out) + "\n").encode("utf-8"))
    if v3:
        for _, data in entries:
            if data is not None:
                sys.stdout.buffer.write(data)


def cmd_unpack(args):
    """A packed installer's manifest (sieve-manifest-v4, SPECIFICATIONS §12.2) unpacked from its
    definition, with nothing of Sieve's own: its two streams decoded by Python's lzma (raw LZMA2 at
    the dictionary each names; the x86 filter on the first), each file's way followed (x86 and raw:
    its bytes, next in its stream; lines K: the lines of file K its mask picks, the mask next in the
    lzma2 stream after every file carried as it is), and each file checked against its size and
    SHA-256. With --to DIR, the folders and files are written there. Prints the listing it unpacked."""
    import lzma
    data = open(args.manifest, "rb").read()
    end = data.index(b"\nend\n") + len(b"\nend\n")
    lines = data[:end].decode("utf-8").split("\n")[:-1]
    if lines[0] != "sieve-manifest-v4":
        sys.exit("not a v4 manifest: " + lines[0][:40])
    streams = []
    for i, name in ((4, "x86"), (5, "lzma2")):
        word, got, packed, size, dictionary = lines[i].split(" ")
        if word != "stream" or got != name:
            sys.exit(f"line {i + 1} is not the {name} stream")
        streams.append((int(packed), int(size), int(dictionary)))
    payload = data[end:]
    if len(payload) != streams[0][0] + streams[1][0]:
        sys.exit("the streams are not the size their lines say")

    def decode(blob, size, dictionary, x86):
        if size == 0:
            return b""
        filters = ([{"id": lzma.FILTER_X86}] if x86 else []) + [{"id": lzma.FILTER_LZMA2, "dict_size": dictionary}]
        out = lzma.decompress(blob, format=lzma.FORMAT_RAW, filters=filters)
        if len(out) != size:
            sys.exit("a stream unpacks to the wrong size")
        return out

    x86 = decode(payload[:streams[0][0]], *streams[0][1:], True)
    plain = decode(payload[streams[0][0]:], *streams[1][1:], False)
    entries = []  # (path, size, sha, way) for files; (path,) for folders
    for line in lines[6:-1]:
        f = line.split("\t")
        entries.append((f[1],) if f[0] == "d" else (f[4], int(f[1]), f[2], f[3]))
    files = [e for e in entries if len(e) == 4]
    got, xa, pa = [None] * len(files), 0, 0
    for i, (path, size, sha, way) in enumerate(files):
        if way == "x86":
            got[i], xa = x86[xa:xa + size], xa + size
        elif way == "raw":
            got[i], pa = plain[pa:pa + size], pa + size
    for i, (path, size, sha, way) in enumerate(files):
        if way.startswith("lines "):
            k = int(way[6:])
            base = got[k].split(b"\n")
            n = (len(base) + 7) // 8
            mask, pa = plain[pa:pa + n], pa + n
            got[i] = b"\n".join(l for j, l in enumerate(base) if mask[j // 8] >> (j % 8) & 1)
    if xa != len(x86) or pa != len(plain):
        sys.exit("the streams hold more than the files")
    for (path, size, sha, way), b in zip(files, got):
        if len(b) != size or hashlib.sha256(b).hexdigest() != sha:
            sys.exit(f"{path} does not unpack to its size and SHA-256")
    if args.to:
        for e in entries:
            target = os.path.join(args.to, e[0])
            if len(e) == 1:
                os.makedirs(target, exist_ok=True)
        for (path, size, sha, way), b in zip(files, got):
            target = os.path.join(args.to, path)
            os.makedirs(os.path.dirname(target) or ".", exist_ok=True)
            open(target, "wb").write(b)
    for path, size, sha, way in files:
        print(f"{way}\t{size}\t{path}")


def map_text(name, root, nodes, edges, sealed, held, meta=()):
    """A map's canonical text: v1 unless it holds a file, has metadata or is sealed. `meta` is
    (node, key, value) triples, sorted by node then key."""
    v2 = sealed or held or bool(meta)
    out = [f"sieve-map-v{2 if v2 else 1}", f"name {name}", f"root {root}"]
    if v2:
        out.append(f"sealed {'yes' if sealed else 'no'}")
    out += [f"nodes {len(nodes)}", f"edges {len(edges)}"] + ([f"meta {len(meta)}"] if v2 else []) + nodes
    out += [f"e\t{a}\t{b}\t{r}" for a, b, r in sorted(edges, key=lambda e: (e[0], e[1], e[2].encode()))]
    out += [f"m\t{n}\t{k}\t{v}" for n, k, v in sorted(meta, key=lambda m: (m[0], m[1].encode()))] + ["end"]
    return ("\n".join(out) + "\n").encode("utf-8")


def cmd_map(args):
    """sieve-map-v1/v2 (SPECIFICATIONS §12.3), from its definition. One folder: its root node './',
    then every folder ('/' at the end) and file (size, SHA-256) by its path's bytes, links skipped;
    each node but the root is contained by the folder its path is in. Several files: the root and
    each file by its name's bytes, contained by the root. --held: the root and each file held
    (its bytes after the text, in order), linked from the root as an anchor. --seal: sealed."""
    import os
    if args.held or len(args.folder) > 1 or os.path.isfile(args.folder[0]):
        name = args.name or "files"
        datas = [(os.path.basename(f), open(f, "rb").read()) for f in args.folder]
        if not args.held:
            datas.sort(key=lambda d: d[0].encode("utf-8"))
        kind = "held" if args.held else "file"
        nodes = ["n\t0\tfolder\t./"] + [f"n\t{i}\t{kind}\t{len(d)}\t{hashlib.sha256(d).hexdigest()}\t{n}"
                                          for i, (n, d) in enumerate(datas, start=1)]
        edges = [(0, i, "anchor" if args.held else "contains") for i in range(1, len(datas) + 1)]
        meta = []
        for spec in args.meta:  # NODE:key=value
            node, rest = spec.split(":", 1)
            key, value = rest.split("=", 1)
            meta.append((int(node), key, value))
        sys.stdout.buffer.write(map_text(name, name, nodes, edges, args.seal, args.held, meta))
        if args.held:
            for _, d in datas:
                sys.stdout.buffer.write(d)
        return
    root = os.path.abspath(args.folder[0])
    items = []
    for here, dirs, names in os.walk(root, followlinks=False):
        for d in list(dirs):
            full = os.path.join(here, d)
            if os.path.islink(full):
                dirs.remove(d)
                continue
            items.append((os.path.relpath(full, root).replace(os.sep, "/") + "/", None))
        for n in names:
            full = os.path.join(here, n)
            if os.path.islink(full) or not os.path.isfile(full):
                continue
            items.append((os.path.relpath(full, root).replace(os.sep, "/"), open(full, "rb").read()))
    items.sort(key=lambda e: e[0].encode("utf-8"))
    ids = {"": 0}
    nodes = ["n\t0\tfolder\t./"]
    edges = []
    for i, (path, data) in enumerate(items, start=1):
        if data is None:
            nodes.append(f"n\t{i}\tfolder\t{path}")
            ids[path[:-1]] = i
        else:
            nodes.append(f"n\t{i}\tfile\t{len(data)}\t{hashlib.sha256(data).hexdigest()}\t{path}")
        bare = path[:-1] if path.endswith("/") else path
        edges.append((ids[bare.rsplit("/", 1)[0] if "/" in bare else ""], i))
    name = os.path.basename(root)
    sys.stdout.buffer.write(map_text(args.name or name, name, nodes, [(a, b, "contains") for a, b in edges], args.seal, False))


def cmd_book_filter_vectors(_args):
    """Book filters (books-compact-v1): surviving books counted, unranked and shuffled, each part by
    its own independent ranker, the pages judged as one text."""
    here = "../data"
    dict_file = "scowl-2020.12.07-en-35.txt"
    raw = open(f"{here}/dictionaries/{dict_file}", "rb").read()
    dsha = hashlib.sha256(raw).hexdigest()
    dct = load_dict(f"{here}/dictionaries/{dict_file}")
    words = dct[0]
    sym = ALPHABETS["lower27"]
    txt = lambda d: "".join(sym[x] for x in d)
    sha = lambda t: hashlib.sha256(t.encode()).hexdigest()
    print("# sieve book-filter vectors v1 (books-compact-v1 over bookspace-v1 and shuffle-sha256-v1)")
    print(f"# dictionary {dict_file}; cover image/mono/WxH; pages lower27 of length L; key sieve")
    print("# bookdomain <case> <W> <H> <L> <P> <cover filter|-> <title max_length|-> <pages filter|-> <count> <domain>")
    print("# bookfilter <case> <mode> <address> <cover digits> <title> <pages, | between>")
    g = stream("book-filters")
    cases = [(3, 2, 8, 2, True, 4, True), (3, 2, 8, 2, False, 4, True), (2, 2, 6, 3, True, None, True), (3, 2, 8, 2, True, 6, False)]
    for ci, (W, H, L, P, cover_on, tmax, pages_on) in enumerate(cases):
        key = "sieve"
        clen = W * H
        parts = []  # (count, unrank, provenance or None)
        if cover_on:
            ar = AgreementRank(W, H, 1, 2, 600)
            parts.append((ar.total(), ar.unrank, f"image/image/mono/{W}x{H}/L{clen}; neighbour-agreement-v1{{min_permille=600}}"))
        else:
            parts.append((2 ** clen, lambda k, n=clen: [(k >> (n - 1 - i)) & 1 for i in range(n)], None))
        if tmax is not None:
            n = min(tmax, L)
            wr = WordsRank(words, n, padding=True)
            parts.append((wr.total(), lambda k, wr=wr, n=n: wr.unrank(k) + [0] * (L - n),
                          f"text/lower27/L{L}; title-v1{{dictionary=scowl-en-35 sha256={dsha} max_length={tmax}}}"))
        else:
            parts.append((27 ** L, lambda k: [(k // 27 ** (L - 1 - i)) % 27 for i in range(L)], None))
        if pages_on:
            br = WordsRank(words, P * L, padding=True)
            parts.append((br.total(), br.unrank, f"text/lower27/L{P * L}; words-v2{{dictionary=scowl-en-35 sha256={dsha}}}"))
        else:
            parts.append((27 ** (P * L), lambda k: [(k // 27 ** (P * L - 1 - i)) % 27 for i in range(P * L)], None))
        nc, nt, nb = (x[0] for x in parts)
        n = nc * nt * nb
        dom = "books-compact-v1/" + book_space_id(f"image/mono/{W}x{H}", clen, "lower27", L, P, key) + "".join(
            "/" + (sha(x[2]) if x[2] else "-") for x in parts)
        print(f"bookdomain\t{ci}\t{W}\t{H}\t{L}\t{P}\t{'neighbour-agreement-v1' if cover_on else '-'}\t"
              f"{tmax if tmax is not None else '-'}\t{'words-v2' if pages_on else '-'}\t{n}\t{dom}")
        width = max(1, ((n - 1).bit_length() + 3) // 4)
        for mode in ("positional", "scrambled"):
            for a in [0, n - 1, n // 2] + [int.from_bytes(bytes(next(g) for _ in range(width + 2)), "little") % n for _ in range(3)]:
                k = shuffle(key, dom, n, a, inverse=True) if mode == "scrambled" else a
                rest, kb = divmod(k, nb)
                kc, kt = divmod(rest, nt)
                cover, title, body = parts[0][1](kc), parts[1][1](kt), parts[2][1](kb)
                # Every row must be a surviving book, checked by the filters themselves (not asserts:
                # python -O would drop those).
                if cover_on and not f_neighbour_agreement(cover, W, H, 1, 600):
                    raise ValueError("cover does not pass")
                if tmax is not None and not passes_title(txt(title), tmax, dct):
                    raise ValueError("title does not pass")
                if pages_on and not passes_v2(txt(body), "words", dct):
                    raise ValueError("pages do not pass")
                pages = [body[i * L:(i + 1) * L] for i in range(P)]
                print(f"bookfilter\t{ci}\t{mode}\t{format(a, 'x').zfill(width)}\t{''.join(map(str, cover))}\t{txt(title)}\t"
                      f"{'|'.join(txt(p) for p in pages)}")


# ---------------------------------------------------------------- file kinds (file-kinds-v1)
# Written from the table and rules in core/include/sieve/filekind.hpp, not from its code: a file's
# kind is read from its first 16 bytes (fewer if it is shorter) and its size. EMPTY for no bytes;
# else the first signature in the table whose fixed bytes all lie within the file and match; else
# TXT when every one of those bytes is readable (20-7E, 80-FF, tab, line feed, carriage return);
# else "?".

FILE_SIGNATURES = [
    ("PNG", {0: b"\x89PNG\r\n\x1a\n"}),
    ("7Z", {0: b"7z\xbc\xaf\x27\x1c"}),
    ("XZ", {0: b"\xfd7zXZ\x00"}),
    ("MANIFEST", {0: b"sieve-manifest"}),
    ("BOOK", {0: b"sieve-book"}),
    ("GIF", {0: b"GIF87a"}),
    ("GIF", {0: b"GIF89a"}),
    ("WAV", {0: b"RIFF", 8: b"WAVE"}),
    ("AVI", {0: b"RIFF", 8: b"AVI "}),
    ("WEBP", {0: b"RIFF", 8: b"WEBP"}),
    ("MP4", {4: b"ftyp"}),
    ("ZIP", {0: b"PK\x03\x04"}),
    ("ZIP", {0: b"PK\x05\x06"}),
    ("ELF", {0: b"\x7fELF"}),
    ("PDF", {0: b"%PDF"}),
    ("RAR", {0: b"Rar!"}),
    ("OGG", {0: b"OggS"}),
    ("FLAC", {0: b"fLaC"}),
    ("MID", {0: b"MThd"}),
    ("JPG", {0: b"\xff\xd8\xff"}),
    ("MP3", {0: b"ID3"}),
    ("BZ2", {0: b"BZh"}),
    ("GZ", {0: b"\x1f\x8b"}),
    ("EXE", {0: b"MZ"}),
    ("BMP", {0: b"BM"}),
]
SIGS = [(k, {o + i: b for o, bs in parts.items() for i, b in enumerate(bs)}) for k, parts in FILE_SIGNATURES]
SIG_END = [max(f) + 1 for _, f in SIGS]
FILE_KINDS = ["EMPTY"] + list(dict.fromkeys(k for k, _ in SIGS)) + ["TXT", "?"]
KIND_HEAD = 16


def readable(b):
    return (b >= 0x20 or b in (9, 10, 13)) and b != 0x7F


def file_kind(head, size):
    if size == 0:
        return "EMPTY"
    h = bytes(head[: min(size, KIND_HEAD)])
    for (kind, fixed), end in zip(SIGS, SIG_END):
        if end <= len(h) and all(h[p] == b for p, b in fixed.items()):
            return kind
    return "TXT" if all(readable(b) for b in h) else "?"


def signed(kind):
    return kind not in ("EMPTY", "TXT", "?")


KIND_SET_NAMES = ["signed", "text", "signed-or-text", "unknown", "empty", "any"] + [k.lower() for k in FILE_KINDS if signed(k)]


def kind_set(name, keep="keep"):
    s = {
        "signed": {k for k in FILE_KINDS if signed(k)},
        "text": {"TXT"},
        "signed-or-text": {k for k in FILE_KINDS if signed(k)} | {"TXT"},
        "unknown": {"?"},
        "empty": {"EMPTY"},
        "any": set(FILE_KINDS),
    }.get(name)
    if s is None:
        s = {k for k in FILE_KINDS if signed(k) and k.lower() == name}
        assert s, name
    return set(FILE_KINDS) - s if keep == "exclude" else s


_KIND_WALK = None


def kind_walk():
    """The heads of 0..16 bytes, walked a byte at a time, once: a head's state is the set of
    signatures it still fits and whether it is readable so far. levels[p]: {state: heads of p bytes
    in it}; moves[p][state]: the state after each of the 256 bytes; groups[p][state]: those states
    with how many bytes lead to each."""
    global _KIND_WALK
    if _KIND_WALK is None:
        start = (frozenset(range(len(SIGS))), True)
        levels, moves, groups = [{start: 1}], [], []
        for p in range(KIND_HEAD):
            nxt, mv, gr = {}, {}, {}
            for st, n in levels[p].items():
                alive, txt = st
                row = [(frozenset(i for i in alive if SIGS[i][1].get(p, b) == b), txt and readable(b)) for b in range(256)]
                mv[st] = row
                g = {}
                for t in row:
                    g[t] = g.get(t, 0) + 1
                gr[st] = g
                for t, m in g.items():
                    nxt[t] = nxt.get(t, 0) + n * m
            levels.append(nxt)
            moves.append(mv)
            groups.append(gr)
        _KIND_WALK = (start, levels, moves, groups)
    return _KIND_WALK


class KindHeads:
    """A kind set's heads: heads[h] is how many heads of exactly h bytes have a kind in the set."""

    def __init__(self, kinds):
        self.kinds = kinds
        self.start, self.levels, self.moves, self.groups = kind_walk()
        self.heads = [sum(n for st, n in self.levels[h].items() if self.kind(h, st) in kinds) for h in range(KIND_HEAD + 1)]
        self.done = {}

    @staticmethod
    def kind(h, st):
        if h == 0:
            return "EMPTY"
        alive, txt = st
        for i in sorted(alive):
            if SIG_END[i] <= h:
                return SIGS[i][0]
        return "TXT" if txt else "?"

    def completions(self, h):
        """done[p][state]: heads of exactly h bytes through state after p bytes, of a kind chosen."""
        if h not in self.done:
            d = [None] * (h + 1)
            d[h] = {st: 1 if self.kind(h, st) in self.kinds else 0 for st in self.levels[h]}
            for p in range(h - 1, -1, -1):
                d[p] = {st: sum(m * d[p + 1][t] for t, m in self.groups[p][st].items()) for st in self.levels[p]}
            self.done[h] = d
        return self.done[h]

    def of_length(self, L):
        return self.heads[L] if L <= KIND_HEAD else self.heads[KIND_HEAD] * 256 ** (L - KIND_HEAD)

    def count(self, N):
        small = sum(self.of_length(L) for L in range(min(N, KIND_HEAD) + 1))
        # 256 + 256^2 + ... + 256^(N-16), times the heads of 16 bytes
        return small + (self.heads[KIND_HEAD] * sum(256 ** j for j in range(1, N - KIND_HEAD + 1)) if N > KIND_HEAD else 0)

    def unrank(self, N, k):
        L = 0
        while k >= self.of_length(L):
            k -= self.of_length(L)
            L += 1
            assert L <= N
        h = min(L, KIND_HEAD)
        tail = L - h
        head_k, rest = divmod(k, 256 ** tail)
        done = self.completions(h)
        st, out = self.start, []
        for p in range(h):
            for b in range(256):
                t = self.moves[p][st][b]
                m = done[p + 1][t]
                if head_k < m:
                    out.append(b)
                    st = t
                    break
                head_k -= m
        return bytes(out) + (rest.to_bytes(tail, "big") if tail else b"")


def binary_sieve_id(n, kinds, keep):
    prov = f"binary/L0-{n}; binary-kind-v1{{file-kinds-v1 kinds={kinds} keep={keep}}}"
    return prov, hashlib.sha256(prov.encode()).hexdigest()


def cmd_kind_vectors(_args):
    """file-kinds-v1 (a file's kind from its first bytes) and binary-kind-v1 (the binary line's files
    of chosen kinds, counted, numbered shortest first then by value, and their compact scrambled
    order through shuffle-sha256-v1 keyed with the line's key, the sieve's id as its domain)."""
    print("# sieve file-kind vectors v1 (file-kinds-v1, binary-kind-v1)")
    print("# kind     file_hex('-' empty) size kind   (size may exceed the bytes given: the first 16 are enough)")
    print("# count    max_bytes kinds keep survivors")
    print("# file     max_bytes kinds keep rank file_hex")
    print("# compact  max_bytes key kinds keep sieve_id compact_index survivor_number")
    files = [b"", b"a", b"\x00", b"\x7f", b"\t\n\r", b"MZ", b"M", b"MZ\x00", b"BM", b"\x1f\x8b", b"\x1f", b"ID3", b"BZh91",
             b"\xff\xd8\xff\xe0", b"\xff\xd8", b"MThd\x00\x00\x00\x06", b"MThd", b"MTh", b"fLaC", b"OggS", b"Rar!\x1a\x07",
             b"%PDF-1.7", b"\x7fELF\x02", b"PK\x03\x04", b"PK\x05\x06", b"PK\x01\x02", b"RIFF\x24\x00\x00\x00WAVEfmt ",
             b"RIFF\x00\x00\x00\x00AVI LIST", b"RIFF1234WEBPVP8 ", b"RIFF1234WAV", b"\x00\x00\x00\x18ftypmp42",
             b"abcdftyp", b"abcdfty", b"GIF89a", b"GIF87a", b"GIF88a", b"sieve-book-v1\n", b"sieve-manifest-v3\n",
             b"sieve-boo", b"\xfd7zXZ\x00\x00", b"\xfd7zXZ", b"7z\xbc\xaf\x27\x1c\x00", b"\x89PNG\r\n\x1a\n\x00",
             b"\x89PNG\r\n\x1a", "héllo".encode(), b"hello world", b"hello\x00", bytes(range(0x80, 0x90))]
    for f in files:
        print(f"kind\t{f.hex() or '-'}\t{len(f)}\t{file_kind(f, len(f))}")
    # A long file known by its first 16 bytes.
    for f, size in ((b"MZ" + bytes(14), 100000), (b"ab" * 8, 17), (bytes(16), 1 << 40), (b"abcdefghijklmnop", 3)):
        print(f"kind\t{f.hex()}\t{size}\t{file_kind(f, size)}")
    g = stream("kinds")
    for _ in range(40):
        n = next(g) % 20
        f = bytes(next(g) for _ in range(n))
        print(f"kind\t{f.hex() or '-'}\t{len(f)}\t{file_kind(f, len(f))}")
    heads = {}

    def heads_of(kinds, keep):
        key = (kinds, keep)
        if key not in heads:
            heads[key] = KindHeads(kind_set(kinds, keep))
        return heads[key]

    for n in (1, 2, 3, 5, 12, 16, 17, 20, 33, 300):
        for kinds in KIND_SET_NAMES:
            for keep in ("keep", "exclude"):
                if keep == "exclude" and kinds not in ("signed", "unknown", "text", "png", "empty"):
                    continue
                print(f"count\t{n}\t{kinds}\t{keep}\t{heads_of(kinds, keep).count(n)}")
    for n in (3, 16, 40):
        for kinds, keep in (("signed", "keep"), ("text", "keep"), ("png", "keep"), ("mid", "keep"), ("unknown", "keep"), ("signed", "exclude"), ("empty", "keep")):
            kh = heads_of(kinds, keep)
            c = kh.count(n)
            ranks = sorted({0, 1, c // 3, c // 2, c - 1} | {next(g) * next(g) % c for _ in range(3)}) if c > 1 else list(range(c))
            for k in ranks:
                f = kh.unrank(n, k)
                assert file_kind(f, len(f)) in kind_set(kinds, keep) and len(f) <= n
                print(f"file\t{n}\t{kinds}\t{keep}\t{k}\t{f.hex() or '-'}")
    for n, key, kinds, keep in ((16, "sieve", "signed", "keep"), (32, "sieve", "text", "keep"), (40, "other", "png", "keep"), (5, "sieve", "unknown", "exclude")):
        c = heads_of(kinds, keep).count(n)
        _, sid = binary_sieve_id(n, kinds, keep)
        for k in sorted({0, 1, c // 2, c - 1, next(g) % c}):
            j = shuffle(key, sid, c, k) if c > 1 else k
            assert shuffle(key, sid, c, j, inverse=True) == k
            print(f"compact\t{n}\t{key}\t{kinds}\t{keep}\t{sid}\t{j}\t{k}")


# ---------------------------------------------------------------- not-written-v1
# Written from the rules in core/include/sieve/written.hpp, not from its code. A unit is written
# out when some reading of its text is a file whose first bytes carry a signature (file-kinds-v1).
# Here each reading is decoded outright (for judging), and, separately, walked as a small machine
# whose states make an automaton (for counting). The binary reading is counted in closed form: a
# unit it reads is whitespace and n = 8m other symbols, all one of two (the first, a, and the
# other, b), and the bits (a = 0) spell m bytes X, which is signed or whose complement is.

W_SPACE = {9, 10, 11, 12, 13, 32}
W_READINGS = ["text", "hex", "base64", "base32", "decimal", "nibbles", "spelled", "binary"]
W_WORDS = ["zero", "one", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
           "eleven", "twelve", "thirteen", "fourteen", "fifteen"]


def w_signed(b):
    return len(b) > 0 and signed(file_kind(b, len(b)))


def w_decode(text, reading):
    """The byte strings a reading gives for the text (a str), or [] if it does not read it."""
    s = text.encode("utf-8")
    if reading == "text":
        raw = bytes(ord(c) for c in text) if all(ord(c) < 256 for c in text) else s
        trimmed = raw.rstrip(bytes(W_SPACE))
        return [raw, trimmed, trimmed + b"\n"]
    if reading == "hex":
        digits, i = [], 0
        while i < len(s):
            c = s[i]
            if c in b"0\\" and i + 1 < len(s) and s[i + 1] in b"xX":
                i += 2
                continue
            i += 1
            if c in W_SPACE or c in b",:;-_":
                continue
            ch = chr(c)
            if ch not in "0123456789abcdefABCDEF":
                return []
            digits.append(int(ch, 16))
        return [bytes(digits[i] * 16 + digits[i + 1] for i in range(0, len(digits), 2))] if digits and len(digits) % 2 == 0 else []
    t = bytes(c for c in s if c not in W_SPACE)
    if reading == "base64":
        if t.startswith(b"data:") and b"base64," in t:
            t = t[t.index(b"base64,") + 7:]
        t = t.rstrip(b"=")
        if not t or len(t) % 4 == 1:
            return []
        std = set(b"+/") & set(t)
        url = set(b"-_") & set(t)
        if std and url:
            return []
        alpha = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" + (b"-_" if url else b"+/")
        if any(c not in alpha for c in t):
            return []
        bits = "".join(format(alpha.index(c), "06b") for c in t)
        return [bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits) - 7, 8))]
    if reading == "base32":
        t = t.rstrip(b"=")
        alpha = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567"
        t = t.upper()
        if not t or any(c not in alpha for c in t):
            return []
        bits = "".join(format(alpha.index(c), "05b") for c in t)
        return [bytes(int(bits[i:i + 8], 2) for i in range(0, len(bits) - 7, 8))]
    if reading == "decimal":
        parts = re.split(rb"[\t\n\x0b\x0c\r ,;\[\]{}()]", s)
        if any(p and not p.isdigit() for p in parts):
            return []
        vals = [int(p) for p in parts if p]
        if not vals or any(len(p) > 3 for p in parts if p) or any(v > 255 for v in vals):
            return []
        return [bytes(vals)]
    if reading == "nibbles":
        t = t.lower()
        if not t or any(not (97 <= c <= 112) for c in t) or len(t) % 2:
            return []
        return [bytes((t[i] - 97) * 16 + t[i + 1] - 97 for i in range(0, len(t), 2))]
    if reading == "spelled":
        words = re.split(rb"[\t\n\x0b\x0c\r ,.\-]", s.lower())
        n = []
        for w in words:
            if not w:
                continue
            w = w.decode("latin-1")
            if len(w) == 1 and w in "abcdef":
                n.append(10 + "abcdef".index(w))
            elif w in W_WORDS:
                n.append(W_WORDS.index(w))
            else:
                return []
        return [bytes(n[i] * 16 + n[i + 1] for i in range(0, len(n), 2))] if n and len(n) % 2 == 0 else []
    if reading == "binary":
        if not t or len(t) % 8 or len(set(t)) != 2:
            return []
        a = t[0]
        x = bytes(int("".join("0" if c == a else "1" for c in t[i:i + 8]), 2) for i in range(0, len(t), 8))
        return [x, bytes(255 - v for v in x)]
    raise ValueError(reading)


def w_judge(text, readings, bytes_line=False):
    """The first reading (and the kind) under which the text is a signed file, or None."""
    if bytes_line:
        b = bytes(ord(c) for c in text)
        return ("bytes", file_kind(b, len(b))) if w_signed(b) else None
    for r in readings:
        for b in w_decode(text, r):
            if w_signed(b):
                return (r, file_kind(b, len(b)))
    return None


# The machines. A matcher state is None (no signature possible), "S" (signed) or (bytes seen,
# the signatures still fitting).
W_ALL_SIGS = frozenset(range(len(SIGS)))


def m_feed(h, b):
    if h is None or h == "S":
        return h
    pos, alive = h
    alive = frozenset(i for i in alive if SIGS[i][1].get(pos, b) == b)
    pos += 1
    if not alive:
        return None
    if any(SIG_END[i] <= pos for i in alive):
        return "S"
    return (pos, alive) if pos < KIND_HEAD else None


def m_decided(h):
    return h is None or h == "S"


def m_nib(par, hi, h, v):
    """A nibble into (parity, high half, matcher); None when the matcher has failed."""
    if par == 0:
        return (1, 0 if m_decided(h) else v, h)
    h = m_feed(h, hi * 16 + v)
    return None if h is None else (0, 0, h)


class WMachine:
    codepoints = False


class WText(WMachine):
    codepoints = True
    start = (0, (0, W_ALL_SIGS), (0, W_ALL_SIGS), (0, W_ALL_SIGS), (0, W_ALL_SIGS))

    def step(self, st, cp):
        high, al, tl, au, tu = st
        if cp < 256:
            al = m_feed(al, cp)
            if cp not in W_SPACE:
                tl = al
        else:
            high, al, tl = 1, None, None
        for b in chr(cp).encode("utf-8"):
            au = m_feed(au, b)
            if b not in W_SPACE:
                tu = au
        live = (au, tu) if high else (al, tl, au, tu)
        return (high, al, tl, au, tu) if any(x is not None for x in live) else None

    def accept(self, st):
        high, al, tl, au, tu = st
        a, t = (au, tu) if high else (al, tl)
        return a == "S" or t == "S" or m_feed(t, 10) == "S"


class WHex(WMachine):
    start = (0, 0, 0, (0, W_ALL_SIGS))  # held back ("" / "0" / "\\"), parity, high, matcher

    def plain(self, st, c):
        held, par, hi, h = st
        if c in b"0\\":
            return (c, par, hi, h)
        if c in W_SPACE or c in b",:;-_":
            return st
        if chr(c) not in "0123456789abcdefABCDEF":
            return None
        r = m_nib(par, hi, h, int(chr(c), 16))
        return None if r is None else (0,) + r

    def step(self, st, c):
        held = st[0]
        if held:
            if c in b"xX":
                return (0,) + st[1:]
            if held == ord("\\"):
                return None
            r = m_nib(st[1], st[2], st[3], 0)
            if r is None:
                return None
            st = (0,) + r
        return self.plain(st, c)

    def accept(self, st):
        held, par, hi, h = st
        if held == ord("\\"):
            return False
        if held:
            r = m_nib(par, hi, h, 0)
            if r is None:
                return False
            par, hi, h = r
        return par == 0 and h == "S"


B64 = {c: i for i, c in enumerate(b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")}


def b64_read(p, c):
    """One symbol into a base64 decoding (symbols mod 4, in the padding, alphabets used, bits
    waiting, their value, matcher, any symbol); None when it cannot go on."""
    cnt, pad, used, nb, acc, h, anyc = p
    if pad:
        return p if c == ord("=") else None
    if c == ord("="):
        return (cnt, 1, used, nb, acc, h, anyc)
    if c in B64:
        v = B64[c]
    elif c in b"+/":
        v, used = (62 if c == ord("+") else 63), used | 1
    elif c in b"-_":
        v, used = (62 if c == ord("-") else 63), used | 2
    else:
        return None
    if used == 3:
        return None
    cnt, anyc, nb = (cnt + 1) % 4, 1, nb + 6
    if m_decided(h):
        return (cnt, 0, used, nb % 8, 0, h, anyc)
    acc = acc * 64 + v
    if nb >= 8:
        nb -= 8
        h = m_feed(h, acc >> nb)
        acc &= (1 << nb) - 1
        if h is None:
            return None
        if h == "S":
            acc = 0
    return (cnt, 0, used, nb, acc, h, anyc)


def b64_done(p):
    return p is not None and p[6] and p[0] != 1 and p[5] == "S"


class WBase64(WMachine):
    start = ((0, 0, 0, 0, 0, (0, W_ALL_SIGS), 0), ("pre", 0))

    def step(self, st, c):
        if c in W_SPACE:
            return st
        plain, d = st
        plain = b64_read(plain, c) if plain is not None else None
        if d is not None:
            kind, v = d
            if kind == "pre":
                d = (("pre", v + 1) if v + 1 < 5 else ("look", 0)) if c == b"data:"[v] else None
            elif kind == "look":
                k = v + 1 if c == b"base64,"[v] else (1 if c == ord("b") else 0)
                d = ("in", (0, 0, 0, 0, 0, (0, W_ALL_SIGS), 0)) if k == 7 else ("look", k)
            else:
                v = b64_read(v, c)
                d = ("in", v) if v is not None else None
        return (plain, d) if plain is not None or d is not None else None

    def accept(self, st):
        plain, d = st
        return b64_done(plain) or (d is not None and d[0] == "in" and b64_done(d[1]))


B32 = {c: i for i, c in enumerate(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ234567")}


class WBase32(WMachine):
    start = (0, 0, 0, 0, (0, W_ALL_SIGS))  # padding, any, bits waiting, value, matcher

    def step(self, st, c):
        if c in W_SPACE:
            return st
        pad, anyc, nb, acc, h = st
        if pad:
            return st if c == ord("=") else None
        if c == ord("="):
            return (1, anyc, nb, acc, h)
        c = c - 32 if 97 <= c <= 122 else c
        if c not in B32:
            return None
        nb += 5
        if m_decided(h):
            return (0, 1, nb % 8, 0, h)
        acc = acc * 32 + B32[c]
        if nb >= 8:
            nb -= 8
            h = m_feed(h, acc >> nb)
            acc &= (1 << nb) - 1
            if h is None:
                return None
            if h == "S":
                acc = 0
        return (0, 1, nb, acc, h)

    def accept(self, st):
        return st[1] == 1 and st[4] == "S"


class WDecimal(WMachine):
    start = (0, 0, (0, W_ALL_SIGS))  # digits, value (256: too large), matcher

    @staticmethod
    def end(st):
        d, v, h = st
        if d == 0:
            return st
        if v > 255:
            return None
        h = m_feed(h, v)
        return None if h is None else (0, 0, h)

    def step(self, st, c):
        if 48 <= c <= 57:
            d, v, h = st
            if d == 3:
                return None
            return (d + 1, min(256, v * 10 + c - 48), h)
        if c in W_SPACE or c in b",;[]{}()":
            return self.end(st)
        return None

    def accept(self, st):
        e = self.end(st)
        return e is not None and e[2] == "S"


class WNibbles(WMachine):
    start = (0, 0, (0, W_ALL_SIGS))

    def step(self, st, c):
        if c in W_SPACE:
            return st
        c = c + 32 if 65 <= c <= 90 else c
        if not 97 <= c <= 112:
            return None
        return m_nib(st[0], st[1], st[2], c - 97)

    def accept(self, st):
        return st[0] == 0 and st[2] == "S"


class WSpelled(WMachine):
    start = ("", 0, 0, (0, W_ALL_SIGS))  # the word so far, parity, high, matcher

    @staticmethod
    def end(st):
        w, par, hi, h = st
        if not w:
            return st
        if len(w) == 1 and w in "abcdef":
            v = 10 + "abcdef".index(w)
        elif w in W_WORDS:
            v = W_WORDS.index(w)
        else:
            return None
        r = m_nib(par, hi, h, v)
        return None if r is None else ("",) + r

    def step(self, st, c):
        c = c + 32 if 65 <= c <= 90 else c
        if 97 <= c <= 122:
            w = st[0] + chr(c)
            if not any(x.startswith(w) for x in W_WORDS + list("abcdef")):
                return None
            return (w,) + st[1:]
        if c in W_SPACE or c in b",.-":
            return self.end(st)
        return None

    def accept(self, st):
        e = self.end(st)
        return e is not None and e[1] == 0 and e[3] == "S"


class WBytes(WMachine):
    start = (0, W_ALL_SIGS)

    def step(self, st, d):
        return m_feed(st, d)

    def accept(self, st):
        return st == "S"


W_MACHINES = {"text": WText, "hex": WHex, "base64": WBase64, "base32": WBase32, "decimal": WDecimal,
              "nibbles": WNibbles, "spelled": WSpelled}


def w_walk(m, inputs):
    """A machine's states over the symbols (inputs[c]: what symbol c feeds it), minimised."""
    ids, states, nxt, acc = {m.start: 0}, [m.start], [], []
    i = 0
    while i < len(states):
        st = states[i]
        i += 1
        acc.append(m.accept(st))
        row = []
        for feed in inputs:
            t = st
            for x in feed:
                t = m.step(t, x)
                if t is None:
                    break
            if t is None:
                row.append(-1)
            else:
                if t not in ids:
                    ids[t] = len(states)
                    states.append(t)
                row.append(ids[t])
        nxt.append(row)
    s0, mn, ma = minimise_dfa(0, nxt, acc, len(inputs))
    return (mn, ma) if s0 is not None else ([], [])


def w_union(a, b, base):
    (an, aa), (bn, ba) = a, b
    if not an:
        return b
    if not bn:
        return a
    ids, pairs, nxt, acc = {(0, 0): 0}, [(0, 0)], [], []
    i = 0
    while i < len(pairs):
        x, y = pairs[i]
        i += 1
        acc.append((x >= 0 and aa[x]) or (y >= 0 and ba[y]))
        row = []
        for c in range(base):
            t = (an[x][c] if x >= 0 else -1, bn[y][c] if y >= 0 else -1)
            if t == (-1, -1):
                row.append(-1)
                continue
            if t not in ids:
                ids[t] = len(pairs)
                pairs.append(t)
            row.append(ids[t])
        nxt.append(row)
    s0, mn, ma = minimise_dfa(0, nxt, acc, base)
    return (mn, ma) if s0 is not None else ([], [])


_W_OTHERS = {}


def w_others(alpha, readings):
    """O: the union of the readings but binary, over the alphabet (a line of every byte: its own bytes)."""
    key = (alpha, tuple(readings))
    if key not in _W_OTHERS:
        base = len(alpha)
        if alpha == ALPHABETS["bytes256"]:
            _W_OTHERS[key] = w_walk(WBytes(), [[c] for c in range(base)])
        else:
            o = ([], [])
            for r in readings:
                if r == "binary":
                    continue
                m = W_MACHINES[r]()
                o = w_union(o, w_walk(m, [[ord(ch)] if m.codepoints else list(ch.encode("utf-8")) for ch in alpha]), base)
            _W_OTHERS[key] = o
    return _W_OTHERS[key]


def w_patterns(m):
    """How many byte strings X of m bytes, the first below 80, are signed or have a signed complement."""
    states = {((0, W_ALL_SIGS), (0, W_ALL_SIGS)): 1}
    for i in range(m):
        nxt = {}
        for (x, y), n in states.items():
            for b in range(128 if i == 0 else 256):
                t = (m_feed(x, b), m_feed(y, 255 - b))
                if t == (None, None):
                    continue
                if m_decided(t[0]) and m_decided(t[1]):
                    t = ("S" if "S" in t else None, None)  # decided: only whether either is signed
                nxt[t] = nxt.get(t, 0) + n
        states = nxt
    return sum(n for (x, y), n in states.items() if x == "S" or y == "S")


def w_binary_count(alpha, L):
    """|Bn| at length L, in closed form (see above)."""
    w = sum(1 for ch in alpha if ord(ch) in W_SPACE)
    k = len(alpha) - w
    return sum(math.comb(L, 8 * m) * w ** (L - 8 * m) * k * (k - 1) * w_patterns(m) for m in range(2, L // 8 + 1))


def w_both(alpha, others, keep, L):
    """|D and Bn| at length L, D = others (O, with keep's plugins if any): every ordered pair of
    symbols (a first), walked over a, b and whitespace alone."""
    on, oa = others
    if not on:
        return 0
    ws = [c for c, ch in enumerate(alpha) if ord(ch) in W_SPACE]
    syms = [c for c, ch in enumerate(alpha) if ord(ch) not in W_SPACE]
    total = 0
    for a in syms:
        for b in syms:
            if a == b:
                continue
            # (automaton state, keep state, started, b seen, bits mod 8, byte so far, matcher X, matcher ~X)
            states = {(0, 0 if keep else -2, 0, 0, 0, 0, (0, W_ALL_SIGS), (0, W_ALL_SIGS)): 1}
            for _ in range(L):
                nxt = {}
                for st, n in states.items():
                    o, p, started, seen, cnt, acc, hx, hy = st
                    for c in ws + [a, b]:
                        o2 = on[o][c]
                        if o2 < 0:
                            continue
                        p2 = p
                        if keep:
                            p2 = keep[0][p][c]
                            if p2 < 0:
                                continue
                        if c in ws:
                            t = (o2, p2) + st[2:]
                        else:
                            if c == b and not started:
                                continue
                            bit = 1 if c == b else 0
                            started2, seen2, cnt2 = 1, seen or bit, (cnt + 1) % 8
                            acc2 = acc * 2 + bit
                            hx2, hy2 = hx, hy
                            if cnt2 == 0:
                                hx2, hy2, acc2 = m_feed(hx, acc2), m_feed(hy, 255 - acc2), 0
                                if hx2 is None and hy2 is None:
                                    continue
                            t = (o2, p2, started2, seen2, cnt2, acc2, hx2, hy2)
                        nxt[t] = nxt.get(t, 0) + n
                states = nxt
                if not states:
                    break
            total += sum(n for (o, p, started, seen, cnt, acc, hx, hy), n in states.items()
                         if oa[o] and (not keep or keep[1][p]) and seen and cnt == 0 and ("S" in (hx, hy)))
    return total


def w_survivors(alpha, readings, L, keep=None):
    """Units of length L that no reading finds written out (and that keep, a minimal automaton
    (nxt, acc), accepts): |P| - |P and O| - |P and Bn| + |P and O and Bn|, with P every unit when
    there is no keep."""
    base = len(alpha)
    o = w_others(alpha, readings)
    everything = ([[0] * base], [True])
    p = keep if keep else everything
    po = w_intersect(p, o, base) if keep else o
    count = lambda d: dfa_count_table(d[0], d[1], L)[L][0] if d[0] else 0
    kept = count(p) - count(po)
    if "binary" in readings and alpha != ALPHABETS["bytes256"]:
        kept -= w_binary_count(alpha, L) if not keep else w_both(alpha, p, None, L)
        kept += w_both(alpha, po, None, L)
    return kept


def w_intersect(a, b, base):
    (an, aa), (bn, ba) = a, b
    if not an or not bn:
        return ([], [])
    ids, pairs, nxt, acc = {(0, 0): 0}, [(0, 0)], [], []
    i = 0
    while i < len(pairs):
        x, y = pairs[i]
        i += 1
        acc.append(aa[x] and ba[y])
        row = []
        for c in range(base):
            t = (an[x][c], bn[y][c])
            if t[0] < 0 or t[1] < 0:
                row.append(-1)
                continue
            if t not in ids:
                ids[t] = len(pairs)
                pairs.append(t)
            row.append(ids[t])
        nxt.append(row)
    s0, mn, ma = minimise_dfa(0, nxt, acc, base)
    return (mn, ma) if s0 is not None else ([], [])


def cmd_written_vectors(_args):
    """not-written-v1: verdicts on chosen texts, and the survivors counted at several lengths."""
    print("# sieve not-written vectors v1 (not-written-v1 over file-kinds-v1)")
    print("# judge  alphabet readings text_utf8_hex verdict (reading:KIND, or -)")
    print("# count  alphabet readings length survivors [plugin params]")
    texts = [
        ("lower27", "abcdftyp"), ("lower27", "abcdftypxyz  "), ("lower27", "enfk"), ("lower27", "en fk"), ("lower27", "enf"),
        ("lower27", "jvnq"), ("lower27", "abababab" * 2), ("lower27", "abbaabab" + "abaababb"), ("ascii95", "MZ"),
        ("ascii95", "MZ   "), ("ascii95", "4d5a"), ("ascii95", "0x4d0x5a"), ("ascii95", "4d:5a"), ("ascii95", "\\x4d\\x5a"),
        ("ascii95", "4d5"), ("ascii95", "TVo="), ("ascii95", "TVo"), ("ascii95", "data:x;base64,TVo="), ("ascii95", "data:TVo="),
        ("ascii95", "JVBERi0="), ("ascii95", "JVBE+i0_"), ("ascii95", "JVGQ===="), ("ascii95", "jvgq"), ("ascii95", "77 90"),
        ("ascii95", "[77,90]"), ("ascii95", "077,090"), ("ascii95", "0077 90"), ("ascii95", "77 256"), ("ascii95", "four thirteen five ten"),
        ("ascii95", "four,d,five.a"), ("ascii95", "four thirteen five"), ("ascii95", "fourd five a"), ("ascii95", "ENFK"),
        ("ascii95", "xxyyxxyx" + "xyxyxyyx"), ("ascii95", "0100110101011010"), ("ascii95", "1011001010100101"),
        ("ascii95", "01001101 01011010"), ("ascii95", "0100110101011012"), ("ascii95", "%PDF-1.7"), ("ascii95", "  MZ"),
        ("ascii95", "BM"), ("ascii95", "ID3"), ("ascii95", "BZh"), ("ascii95", "Rar!"), ("ascii95", "hello"),
        ("ascii96", "MZ\n\n"), ("ascii96", "\nMZ"), ("ascii96", "4d5a\n"), ("bytes256", "MZ\x00\x00"), ("bytes256", "\x89PNG\r\n\x1a\n"),
        ("bytes256", "4d5a"), ("babel29", "four,thirteen.five,ten"), ("babel29", "enfk.."),
        ("u+0020-u+0020+u+004d-u+004d+u+005a-u+005a", "MZMZMZMZMZMZMZMZ"), ("u+0020-u+0020+u+004d-u+004d+u+005a-u+005a", "MMZMMMZZMZZMMZZM"),
    ]
    for alpha_id, text in texts:
        alpha = alphabet(alpha_id)
        assert all(ch in alpha for ch in text), (alpha_id, text)
        for readings in (["all"], ["text"], ["hex"], ["base64"], ["base32"], ["decimal"], ["nibbles"], ["spelled"], ["binary"]):
            rs = W_READINGS if readings == ["all"] else readings
            v = w_judge(text, rs, alpha == ALPHABETS["bytes256"])
            print(f"judge\t{alpha_id}\t{readings[0]}\t{text.encode('utf-8').hex()}\t{v[0] + ':' + v[1] if v else '-'}")
    g = stream("written")
    for alpha_id, L in (("lower27", 6), ("ascii95", 5)):
        alpha = alphabet(alpha_id)
        for _ in range(20):
            text = "".join(alpha[next(g) % len(alpha)] for _ in range(L))
            v = w_judge(text, W_READINGS)
            print(f"judge\t{alpha_id}\tall\t{text.encode('utf-8').hex()}\t{v[0] + ':' + v[1] if v else '-'}")
    rows = [("lower27", r, L) for r in ["all"] + W_READINGS for L in (4, 16)]
    rows += [("lower27", "all", L) for L in (1, 8, 24, 32, 40)]
    rows += [("babel29", r, 32) for r in ("all", "spelled", "binary")]
    rows += [("ascii95", r, L) for r in ("all", "text", "hex", "base64", "decimal", "binary") for L in (3, 16)]
    rows += [("bytes256", "all", 3), ("bytes256", "all", 32)]
    rows += [("u+0020-u+0020+u+004d-u+004d+u+005a-u+005a", r, L) for r in ("all", "text", "binary") for L in (16, 24)]
    rows += [("u+0030-u+0031+u+0078-u+0078", "all", 16), ("u+0061-u+0063", "all", 24)]
    for alpha_id, r, L in rows:
        alpha = alphabet(alpha_id)
        rs = W_READINGS if r == "all" else [r]
        print(f"count\t{alpha_id}\t{r}\t{L}\t{w_survivors(alpha, rs, L)}")
    # With a plugin's automaton kept too (the stack's count).
    for f, params, L in (("max-run-data-v1", "max=2", 16), ("clean-data-v1", "", 16)):
        head, body = parse_plugin(open(f"../data/filters/{f}.sfilter", encoding="utf-8").read())
        values = dict(kv.split("=", 1) for kv in params.split(",")) if params else {}
        base = plugin_base(head["symbols"])
        _, start, nxt, acc = compile_plugin(head, body, values, base)
        s0, mn, ma = minimise_dfa(start, nxt, acc, base)
        print(f"count\tlower27\tall\t{L}\t{w_survivors(ALPHABETS['lower27'], W_READINGS, L, (mn, ma))}\t{f}\t{params or '-'}")


# ---------------------------------------------------------------- one line filtered by every other
# Written from the rules in core/src/filters/crossline.cpp, core/include/sieve/written.hpp and
# core/include/sieve/filekind.hpp, not from their code.

SIGNED = {k for k in FILE_KINDS if signed(k)}


def kind_at_index(v):
    """The kind of the file at binary-v1 positional index v (the whole file, written out)."""
    f = binary_file(v)
    return file_kind(f, len(f))


def signed_before(x):
    """How many binary-v1 indexes below x hold a signed file: whole lengths, then this length's
    heads below its head (each with any tail), then its own head's tails below its own."""
    kh = KindHeads(SIGNED)
    if x == 0:
        return 0
    length, rest = 0, x
    while rest >= 256 ** length:
        rest -= 256 ** length
        length += 1
    total = sum(kh.of_length(l) for l in range(length))
    h = min(length, KIND_HEAD)
    head, tail = divmod(rest, 256 ** (length - h))
    hb = head.to_bytes(h, "big")
    done = kh.completions(h)
    st, below = kh.start, 0
    for p in range(h):
        for b in range(hb[p]):
            below += done[p + 1][kh.moves[p][st][b]]
        st = kh.moves[p][st][hb[p]]
    total += below * 256 ** (length - h)
    if kh.kind(h, st) in SIGNED:
        total += tail
    return total


def not_a_file_count(base, L, brute=False):
    n = base ** L
    if brute:
        return sum(1 for v in range(n) if kind_at_index(v) not in SIGNED)
    return n - signed_before(n)


def not_a_file_unrank(base, L, k):
    lo, hi = k, base ** L - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if (mid + 1) - signed_before(mid + 1) > k:
            hi = mid
        else:
            lo = mid + 1
    digits = []
    v = lo
    for _ in range(L):
        v, d = divmod(v, base)
        digits.append(d)
    return digits[::-1]


# not-other-line-v1: melody notation and .obj text, judged outright by patterns ...
NOTE_TOKEN = re.compile(r"(//|R|[A-G][#b]?[0-9])(s|e\.?|q\.?|h\.?|w)?$")
OBJ_V = re.compile(r"v( [+-]?[0-9]+(\.[0-9]+)?){3}$")
OBJ_F = re.compile(r"f( [1-9][0-9]*){3}$")


def other_line_judge(text, forms):
    if any(ord(c) >= 128 for c in text):
        return None
    if "notes" in forms:
        toks = [t for t in re.split(r"[ \n\r\t|,]", text) if t]
        if toks and all(NOTE_TOKEN.match(t) and not (t.startswith("//") and len(t) > 2) for t in toks) and any(t != "//" for t in toks):
            return "notes"
    if "obj" in forms:
        body = text.rstrip(" ")
        lines = body.split("\n")
        if body and all(l == "" or OBJ_V.match(l) or OBJ_F.match(l) for l in lines):
            if any(l.startswith("v") for l in lines) and any(l.startswith("f") for l in lines):
                return "obj"
    return None


# ... and, for counting, as machines of their own over the characters.
class ONotes:
    codepoints = True
    start = ("gap", False)

    def step(self, st, c):
        at, seen = st
        ch = chr(c)
        if ch in " \n\r\t|,":
            return None if at in ("letter", "accidental", "slash") else ("gap", seen)
        nxt = {
            "gap": {**{x: "letter" for x in "ABCDEFG"}, "R": "rest", "/": "slash"},
            "letter": {**{x: "accidental" for x in "#b"}, **{x: "octave" for x in "0123456789"}},
            "accidental": {x: "octave" for x in "0123456789"},
            "octave": {"e": "dotable", "q": "dotable", "h": "dotable", "s": "done", "w": "done"},
            "rest": {"e": "dotable", "q": "dotable", "h": "dotable", "s": "done", "w": "done"},
            "dotable": {".": "done"},
            "slash": {"/": "voices"},
        }.get(at, {})
        if ch not in nxt:
            return None
        to = nxt[ch]
        return (to, seen or to in ("octave", "rest"))

    def accept(self, st):
        return st[1] and st[0] not in ("letter", "accidental", "slash")


class OObj:
    """A .obj text as the judge reads it, with each line reduced to where it is in its grammar:
    (line kind "", "v" or "f"; phase; which number; kinds of line seen; in the trailing padding)."""
    codepoints = True
    start = ("", "", 0, frozenset(), False)
    DIGITS = "0123456789"

    @staticmethod
    def complete(kind, phase, idx):
        return idx == 3 and ((kind == "v" and phase in ("int", "frac")) or (kind == "f" and phase == "num"))

    def step(self, st, c):
        kind, phase, idx, seen, pad = st
        ch = chr(c)
        if pad:
            return st if ch == " " else None
        if ch == "\n":
            if not kind:
                return st
            return ("", "", 0, seen | {kind}, False) if self.complete(kind, phase, idx) else None
        if ch == " " and (not kind or self.complete(kind, phase, idx)):
            return ("", "", 0, seen | ({kind} if kind else set()), True)
        if not kind:
            return (ch, "k", 0, seen, False) if ch in "vf" else None
        if ch == " ":
            ok = phase == "k" or (idx < 3 and phase in (("int", "frac") if kind == "v" else ("num",)))
            return (kind, "sp", idx + 1, seen, False) if ok else None
        if kind == "v":
            to = {("sp", "+"): "sign", ("sp", "-"): "sign", ("int", "."): "dot"}.get((phase, ch))
            if ch in self.DIGITS:
                to = {"sp": "int", "sign": "int", "int": "int", "dot": "frac", "frac": "frac"}.get(phase)
        else:
            to = "num" if (phase == "num" and ch in self.DIGITS) or (phase == "sp" and ch in "123456789") else None
        return (kind, to, idx, seen, False) if to else None

    def accept(self, st):
        kind, phase, idx, seen, pad = st
        if kind:
            if not self.complete(kind, phase, idx):
                return False
            seen = seen | {kind}
        return {"v", "f"} <= seen


def other_line_count(alpha, forms, L):
    o = ([], [])
    if "notes" in forms:
        o = w_walk(ONotes(), [[ord(ch)] for ch in alpha])
    if "obj" in forms:
        o = w_union(o, w_walk(OObj(), [[ord(ch)] for ch in alpha]), len(alpha))
    written = dfa_count_table(o[0], o[1], L)[L][0] if o[0] else 0
    return len(alpha) ** L - written


def packed_count(base, L):
    """Units of L symbols of b bits whose bits, packed into floor(L b / 8) bytes, are not a signed
    file: a closed form, since the packing is a bijection onto bit strings."""
    b = base.bit_length() - 1
    m = L * b // 8
    signed_files = KindHeads(SIGNED).of_length(m)
    return base ** L - signed_files * 2 ** (L * b - 8 * m)


def pages_pattern(alpha, L):
    """A page's files on the binary line: exactly its text, one byte a symbol."""
    return [set(ord(c) for c in alpha)] * L


def pattern_in_kinds(kinds, allowed):
    """Files matching the pattern whose kind is in the set: walked byte by byte over the heads."""
    kh = KindHeads(kinds)
    h = min(len(allowed), KIND_HEAD)
    states = {kh.start: 1}
    for p in range(h):
        nxt = {}
        for st, n in states.items():
            for b in allowed[p]:
                t = kh.moves[p][st][b]
                nxt[t] = nxt.get(t, 0) + n
        states = nxt
    rest = 1
    for q in range(h, len(allowed)):
        rest *= len(allowed[q])
    return sum(n for st, n in states.items() if kh.kind(h, st) in kinds) * rest


def number_files_unrank(size, k):
    """The k-th number in [0, size) whose file on the binary line has no signature."""
    lo, hi = k, size - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if (mid + 1) - signed_before(mid + 1) > k:
            hi = mid
        else:
            lo = mid + 1
    return lo



# ---------------------------------------------------------------- the models line's own rules
# distinct-vertices-v1, distinct-indices-v1 and every-vertex-used-v1, written from their rules
# (core/src/filters/models.cpp), not from the engine's closed forms: the faces are walked as a
# machine whose state is the set of vertices used so far and what the current face has named.

def mesh_ok(V, F, C, rules, coords, faces):
    if "v" in rules:
        pts = [tuple(coords[3 * i:3 * i + 3]) for i in range(V)]
        if len(set(pts)) < V:
            return False
    if "i" in rules:
        for k in range(F):
            a, b, c = faces[3 * k:3 * k + 3]
            if a == b or b == c or a == c:
                return False
    if "u" in rules and len(set(faces)) < V:
        return False
    return True


class FaceWalk:
    """Completions of the face digits from every state, position by position (backwards)."""

    def __init__(self, V, F, rules):
        self.V, self.F, self.rules = V, F, rules
        self.full = (1 << V) - 1
        self.done = [dict() for _ in range(3 * F + 1)]
        self.done[3 * F] = None  # filled lazily by comp()

    def step(self, st, s):
        mask, named = st
        if "i" in self.rules and s in named:
            return None
        named = named + (s,)
        if len(named) == 3:
            named = ()
        return (mask | (1 << s) if "u" in self.rules else 0, named if "i" in self.rules else ())

    def comp(self, p, st):
        if p == 3 * self.F:
            return 1 if "u" not in self.rules or st[0] == self.full else 0
        memo = self.done[p]
        if st in memo:
            return memo[st]
        total = 0
        for s in range(self.V):
            t = self.step(st, s)
            if t is not None:
                total += self.comp(p + 1, t)
        memo[st] = total
        return total

    def count(self):
        return self.comp(0, (0, ()))

    def unrank(self, k):
        st, out = (0, ()), []
        for p in range(3 * self.F):
            for s in range(self.V):
                t = self.step(st, s)
                if t is None:
                    continue
                c = self.comp(p + 1, t)
                if k < c:
                    out.append(s)
                    st = t
                    break
                k -= c
        return out


# canonical-mesh-v1, from its rule: vertices in increasing order as grid points, each face starting
# at its smallest index (a < b, a < c, b != c), faces in increasing order as triples. Counted here
# by walking the faces as a machine (its state the last triple and, with every-vertex-used, the set
# of vertices named so far), not by the engine's binomials and inclusion and exclusion.

def canon_ok(V, F, C, rules, coords, faces):
    pts = [tuple(coords[3 * i:3 * i + 3]) for i in range(V)]
    if any(pts[i] >= pts[i + 1] for i in range(V - 1)):
        return False
    tri = [tuple(faces[3 * k:3 * k + 3]) for k in range(F)]
    if any(not (a < b and a < c and b != c) for a, b, c in tri):
        return False
    if any(tri[k] >= tri[k + 1] for k in range(F - 1)):
        return False
    return "u" not in rules or len(set(faces)) == V


class CanonWalk:
    """Increasing sequences of F rotated triples. comp(j, last, mask) is the number of ways to place
    faces j.. with every triple after `last`, mask the vertices named so far (with every-vertex-used;
    else 0). Tabled backwards as suffix sums over the next triple x:
        S[j][x][mask] = comp(j + 1, x, mask | m_x) + S[j][x + 1][mask],  comp(j, last, mask) = S[j][last + 1][mask]."""

    def __init__(self, V, F, rules):
        self.V, self.F, self.u = V, F, "u" in rules
        self.tri = [(a, b, c) for a in range(V) for b in range(V) for c in range(V) if a < b and a < c and b != c]
        self.m = [(1 << a) | (1 << b) | (1 << c) for a, b, c in self.tri]
        self.T = T = len(self.tri)
        full = (1 << V) - 1
        masks = range(1 << V) if self.u else [0]
        last = {mk: (1 if not self.u or mk == full else 0) for mk in masks}  # comp(F, ., mask)
        self.S = [None] * F
        for j in range(F - 1, -1, -1):
            row = {}
            for mk in masks:
                acc = 0
                row[T, mk] = 0
                for x in range(T - 1, -1, -1):
                    nm = (mk | self.m[x]) if self.u else 0
                    acc += last[nm] if j == F - 1 else self.S[j + 1][x + 1, nm]
                    row[x, mk] = acc
            self.S[j] = row

    def comp(self, j, last, mask):
        if j == self.F:
            return 1 if not self.u or mask == (1 << self.V) - 1 else 0
        return self.S[j][last + 1, mask]

    def count(self):
        return self.comp(0, -1, 0)

    def unrank(self, k):
        last, mask, out = -1, 0, []
        for j in range(self.F):
            for t in range(last + 1, self.T):
                m = (mask | self.m[t]) if self.u else 0
                n = self.comp(j + 1, t, m)
                if k < n:
                    out += list(self.tri[t])
                    last, mask = t, m
                    break
                k -= n
        return out


def canon_vertex_count(V, C):
    """Increasing sequences of V of the C^3 points, by a walk over the points (small), or C(P, V)."""
    P = C ** 3
    if P <= 4096:
        ways = [1] * (P + 1)  # ways[p]: sequences of the vertices still to place, from point p on
        for _ in range(V):
            nxt = [0] * (P + 1)
            for p in range(P - 1, -1, -1):
                nxt[p] = nxt[p + 1] + ways[p + 1]
            ways = nxt
        return ways[0]
    return math.comb(P, V)


def canon_vertex_unrank(V, C, k):
    P, out, start = C ** 3, [], 0
    for i in range(V):
        for p in range(start, P):
            n = math.comb(P - 1 - p, V - 1 - i)  # the increasing ways to place the rest above p
            if k < n:
                out += [p // (C * C), p // C % C, p % C]
                start = p + 1
                break
            k -= n
    return out


def vertex_strings(V, C, rules):
    P = C ** 3
    if "v" not in rules:
        return P ** V
    n = 1
    for i in range(V):
        n *= max(P - i, 0)
    return n


def vertex_unrank(V, C, rules, k):
    if "v" not in rules:
        return to_digits_n(k, C, 3 * V)
    P, taken, out = C ** 3, set(), []
    for i in range(V):
        block = 1  # the ways to place the vertices after this one
        for t in range(i + 1, V):
            block *= P - t
        q, k = divmod(k, block)
        for pt in range(P):  # the q-th point not yet taken
            if pt in taken:
                continue
            if q == 0:
                break
            q -= 1
        taken.add(pt)
        out += [pt // (C * C), pt // C % C, pt % C]
    return out


def to_digits_n(v, base, n):
    out = []
    for _ in range(n):
        v, d = divmod(v, base)
        out.append(d)
    return out[::-1]


def mesh_index(V, F, C, coords, faces):
    x = 0
    for d in coords:
        x = x * C + d
    for d in faces:
        x = x * V + d
    return x



# ---------------------------------------------------------------- not-a-pattern-v1
# Written from the rule (core/src/filters/pattern.cpp's header), not the engine's counting: the
# judge reads the unit directly; counts at large sizes come from inclusion and exclusion over the
# sets of block lengths (each set's units repeat with the gcd of its lengths), and the ramps that
# also repeat from Euler's phi (the steps of order s number phi(s) when s divides B^w).

def pat_limit(L, w, Q):
    return max([j for j in range(1, Q + 1) if 2 * j * w <= L] or [0])


def pat_value(ds, B, little):
    v = 0
    for d in (ds[::-1] if little else ds):
        v = v * B + d
    return v


def pat_digits(v, B, w, little):
    out = to_digits_n(v, B, w)
    return out[::-1] if little else out


def pat_judge(u, B, w, little, Q, ramps):
    """True when the unit is a pattern (fails the filter)."""
    L = len(u)
    for j in range(1, pat_limit(L, w, Q) + 1):
        if all(u[i] == u[i - j * w] for i in range(j * w, L)):
            return True
    if ramps and L // w >= 3:
        M = B ** w
        a = pat_value(u[:w], B, little)
        d = (pat_value(u[w:2 * w], B, little) - a) % M
        run, v = [], a
        while len(run) < L:
            run += pat_digits(v, B, w, little)
            v = (v + d) % M
        if run[:L] == list(u):
            return True
    return False


def phi(n):
    r, m, p = n, n, 2
    while p * p <= m:
        if m % p == 0:
            while m % p == 0:
                m //= p
            r -= r // p
        p += 1
    return r - r // m if m > 1 else r


def pat_count(B, L, w, Q, ramps):
    """Units kept, by inclusion and exclusion (no brute force)."""
    from math import gcd
    from itertools import combinations
    q = pat_limit(L, w, Q)
    rep = 0
    for size in range(1, q + 1):
        for S in combinations(range(1, q + 1), size):
            g = 0
            for x in S:
                g = gcd(g, x)
            rep += (-1) ** (size + 1) * B ** (g * w)
    out = rep
    if ramps and L // w >= 3:
        M = B ** w
        both = M * sum(phi(s) for s in range(1, q + 1) if M % s == 0)
        out += M * M - both
    return B ** L - out



def max_run_count(B, L, R, space):
    """Units of L symbols with no symbol but SPACE repeated more than R times in a row: walked by
    (last symbol, run length), counted with multiplicities (not the engine's automaton)."""
    ways = {None: 1}  # key: (symbol, run) or None at the start
    for _ in range(L):
        nxt = {}
        for key, n in ways.items():
            for x in range(B):
                if x == space:
                    k2 = ("sp", 1)
                elif key is not None and key[0] == x:
                    if key[1] + 1 > R:
                        continue
                    k2 = (x, key[1] + 1)
                else:
                    k2 = (x, 1)
                nxt[k2] = nxt.get(k2, 0) + n
        ways = nxt
    return sum(ways.values())


def cmd_cross_vectors(_args):
    """not-a-file-v1, not-other-line-v1, not-packed-v1 and not-an-item-v1 (pages), counted."""
    sys.set_int_max_str_digits(0)
    print("# sieve cross-line vectors v1")
    print("# not-a-file   base length survivors")
    print("# a-file-unit  base length rank digits(comma)")
    print("# other-judge  alphabet forms text_utf8_hex verdict")
    print("# other        alphabet forms length survivors")
    print("# packed       base length survivors")
    print("# item-pages   max_bytes kinds keep page_alphabet page_length survivors")
    print("# models       vertices faces coords survivors  (not-a-file-v1 on a model's positional index)")
    print("# model-unit   vertices faces coords rank positional_index")
    print("# mesh         vertices faces coords rules survivors  (rules: v distinct-vertices, i distinct-indices, u every-vertex-used, c canonical-mesh)")
    print("# mesh-unit    vertices faces coords rules rank positional_index")
    print("# max-run      alphabet length max_run survivors  (max-run-v1, counted since it became an automaton)")
    print("# pattern      base length width order period ramps survivors  (not-a-pattern-v1)")
    print("# pattern-unit base length width order period ramps rank digits(comma)")
    for base, L, brute in ((3, 8, True), (27, 3, True), (27, 4, True), (2, 12, True), (27, 32, False), (2, 100, False),
                           (104, 16, False), (304, 32, False), (16, 25, False), (256, 12, False), (29, 3200, False)):
        c = not_a_file_count(base, L, brute)
        if brute:
            assert c == base ** L - signed_before(base ** L)
        print(f"not-a-file\t{base}\t{L}\t{c}")
    g = stream("cross")
    for base, L in ((27, 32), (2, 100), (104, 16)):
        c = not_a_file_count(base, L)
        for k in sorted({0, c // 7, c // 2, c - 1, next(g) * next(g) % c}):
            print(f"a-file-unit\t{base}\t{L}\t{k}\t{','.join(map(str, not_a_file_unrank(base, L, k)))}")
    texts = ["C4q E4q G4h", "C4q E4q G4h   ", "Rq Re. C#5w", "C4 D4 // E4 F4", "C4q|D4q,E4h", "//", "C4x", "H4q", "C#", "Cb4h.",
             "C4q.. D4", "v 1 2 3\nf 1 2 3", "v -0.9375 +0.9375 0.5\nv 1 2 3\nf 1 2 3\n", "v 1 2 3\nf 1 2 3   ", "v 1 2\nf 1 2 3",
             "f 1 2 3", "v 1 2 3\nf 0 2 3", "v 1 2 3 \nf 1 2 3", "v 1. 2 3\nf 1 2 3", "\nv 1 2 3\n\nf 10 20 30", "hello world"]
    for alpha_id in ("ascii96",):
        alpha = alphabet(alpha_id)
        for t in texts:
            for forms in ("all", "notes", "obj"):
                fs = ["notes", "obj"] if forms == "all" else [forms]
                v = other_line_judge(t, fs)
                print(f"other-judge\t{alpha_id}\t{forms}\t{t.encode().hex()}\t{v or '-'}")
    for alpha_id, forms, L in (("ascii95", "all", 3), ("ascii95", "notes", 8), ("ascii95", "all", 12), ("ascii96", "obj", 16),
                               ("ascii96", "all", 20), ("lower27", "all", 32), ("babel29", "all", 16)):
        fs = ["notes", "obj"] if forms == "all" else [forms]
        print(f"other\t{alpha_id}\t{forms}\t{L}\t{other_line_count(alphabet(alpha_id), fs, L)}")
    for base, L in ((2, 100), (2, 25), (4, 40), (16, 25), (256, 12), (2, 12)):
        print(f"packed\t{base}\t{L}\t{packed_count(base, L)}")
    for n, kinds, keep, pa, pl in ((2, "any", "keep", "u+0061-u+0062", 1), (3, "text", "keep", "u+0061-u+0062", 2),
                                   (33, "any", "keep", "lower27", 32), (33, "text", "keep", "lower27", 32),
                                   (40, "signed", "exclude", "ascii95", 20), (8, "any", "keep", "bytes256", 3)):
        ks = kind_set(kinds, keep)
        alpha = alphabet(pa)
        allowed = [set(range(256))] * pl if alpha == ALPHABETS["bytes256"] else pages_pattern(alpha, pl)
        total = KindHeads(ks).count(n)
        pages = pattern_in_kinds(ks, allowed) if len(allowed) <= n else 0
        if pa == "lower27" and kinds == "text":
            assert pages == 27 ** 32 - 27 ** 28  # every page is TXT but those with "ftyp" at the fifth letter
        print(f"item-pages\t{n}\t{kinds}\t{keep}\t{pa}\t{pl}\t{total - pages}")
    # The models line: N = C^(3V) * V^(3F) models, numbered by their positional index.
    for V, F, C, brute in ((3, 1, 2, True), (3, 2, 2, True), (4, 1, 4, False), (8, 12, 16, False), (12, 20, 64, False)):
        size = C ** (3 * V) * V ** (3 * F)
        c = size - signed_before(size)
        if brute:
            assert c == sum(1 for v in range(size) if kind_at_index(v) not in SIGNED)
        print(f"models\t{V}\t{F}\t{C}\t{c}")
        if not brute:
            for k in sorted({0, c // 3, c - 1, next(g) * next(g) % c}):
                print(f"model-unit\t{V}\t{F}\t{C}\t{k}\t{number_files_unrank(size, k)}")
    # The models line's own rules: every model of a tiny shape by brute force, then the face walk.
    combos = ("v", "i", "u", "vi", "iu", "vu", "viu")
    V, F, C = 3, 2, 2
    for rules in combos:
        kept = []
        for x in range(C ** (3 * V) * V ** (3 * F)):
            digits = to_digits_n(x, V, 3 * F)
            coords = to_digits_n(x // V ** (3 * F), C, 3 * V)
            if mesh_ok(V, F, C, rules, coords, digits):
                kept.append(x)
        fc = FaceWalk(V, F, rules).count()
        assert len(kept) == vertex_strings(V, C, rules) * fc
        print(f"mesh\t{V}\t{F}\t{C}\t{rules}\t{len(kept)}")
        for k in sorted({0, len(kept) // 3, len(kept) - 1}):
            print(f"mesh-unit\t{V}\t{F}\t{C}\t{rules}\t{k}\t{kept[k]}")
    for V, F, C in ((4, 3, 4), (5, 4, 8), (8, 12, 16)):
        for rules in combos:
            fw = FaceWalk(V, F, rules)
            fc = fw.count()
            n = vertex_strings(V, C, rules) * fc
            print(f"mesh\t{V}\t{F}\t{C}\t{rules}\t{n}")
            if (V, F, C) == (4, 3, 4) or rules in ("iu", "viu"):
                for k in sorted({0, n // 7, n - 1, next(g) * next(g) % n}):
                    q, r = divmod(k, fc)
                    coords = vertex_unrank(V, C, rules, q)
                    faces = fw.unrank(r)
                    assert mesh_ok(V, F, C, rules, coords, faces)
                    print(f"mesh-unit\t{V}\t{F}\t{C}\t{rules}\t{k}\t{mesh_index(V, F, C, coords, faces)}")
    for alpha_id, L, R in (("lower27", 32, 3), ("lower27", 32, 1), ("lower27", 100, 2), ("ascii95", 32, 3), ("babel29", 20, 2)):
        al = alphabet(alpha_id)
        sp = al.index(" ") if " " in al else -1
        print(f"max-run\t{alpha_id}\t{L}\t{R}\t{max_run_count(len(al), L, R, sp)}")
    # not-a-pattern-v1: every unit of small lines by the judge, then the counts at full size.
    from itertools import product
    for B, L, w, order, Q, ramps in ((2, 16, 1, "big", 16, "on"), (3, 9, 1, "big", 16, "on"), (4, 8, 1, "big", 2, "on"),
                                     (2, 16, 2, "big", 16, "on"), (2, 16, 2, "little", 16, "on"), (2, 17, 4, "little", 2, "on"),
                                     (4, 9, 2, "big", 16, "on"), (3, 10, 1, "big", 3, "off"), (2, 12, 1, "big", 0, "on"),
                                     (27, 4, 1, "big", 16, "on")):
        kept = [u for u in product(range(B), repeat=L) if not pat_judge(u, B, w, order == "little", Q, ramps == "on")]
        assert len(kept) == pat_count(B, L, w, Q, ramps == "on")
        print(f"pattern\t{B}\t{L}\t{w}\t{order}\t{Q}\t{ramps}\t{len(kept)}")
        for k in sorted({0, len(kept) // 3, len(kept) // 2, len(kept) - 1}):
            print(f"pattern-unit\t{B}\t{L}\t{w}\t{order}\t{Q}\t{ramps}\t{k}\t{','.join(map(str, kept[k]))}")
    for B, L, w, order, Q, ramps in ((256, 32, 1, "big", 16, "on"), (256, 32, 2, "big", 16, "on"), (256, 32, 4, "little", 16, "on"),
                                     (256, 64, 8, "big", 16, "on"), (256, 64, 8, "little", 4, "off"), (27, 32, 1, "big", 16, "on"),
                                     (2, 100, 1, "big", 16, "on"), (2, 100, 8, "big", 16, "on"), (104, 16, 1, "big", 8, "on"),
                                     (29, 3200, 1, "big", 12, "on")):
        print(f"pattern\t{B}\t{L}\t{w}\t{order}\t{Q}\t{ramps}\t{pat_count(B, L, w, Q, ramps == 'on')}")
    # canonical-mesh-v1 (c), alone and with every-vertex-used (u); the other two rules it implies.
    # Every model of the tiny shape by brute force, then the face walk at larger shapes.
    V, F, C = 3, 2, 2
    for rules in ("c", "cu", "cviu"):
        kept = []
        for x in range(C ** (3 * V) * V ** (3 * F)):
            digits = to_digits_n(x, V, 3 * F)
            coords = to_digits_n(x // V ** (3 * F), C, 3 * V)
            if canon_ok(V, F, C, rules, coords, digits):
                kept.append(x)
        assert len(kept) == canon_vertex_count(V, C) * CanonWalk(V, F, rules).count()
        print(f"mesh\t{V}\t{F}\t{C}\t{rules}\t{len(kept)}")
        for k in sorted({0, len(kept) // 3, len(kept) - 1}):
            print(f"mesh-unit\t{V}\t{F}\t{C}\t{rules}\t{k}\t{kept[k]}")
    for V2, F2, C2, rs in ((4, 3, 4, ("c", "cu", "cviu")), (5, 4, 8, ("c", "cu", "cviu")), (8, 12, 16, ("c", "cu", "cviu")),
                           (10, 9, 16, ("cu",)), (12, 20, 64, ("c",))):
        for rules in rs:
            cw = CanonWalk(V2, F2, rules)
            fc = cw.count()
            n = canon_vertex_count(V2, C2) * fc
            print(f"mesh\t{V2}\t{F2}\t{C2}\t{rules}\t{n}")
            for k in sorted({0, n // 7, n - 1, next(g) * next(g) % n}):
                q, r = divmod(k, fc)
                coords, faces = canon_vertex_unrank(V2, C2, q), cw.unrank(r)
                assert canon_ok(V2, F2, C2, rules, coords, faces)
                print(f"mesh-unit\t{V2}\t{F2}\t{C2}\t{rules}\t{k}\t{mesh_index(V2, F2, C2, coords, faces)}")


# ---------------------------------------------------------------- palette-size-v1, row-runs-v1
# Written from the rules (core/src/filters/picture.cpp's header), counted by inclusion and
# exclusion and by binomials rather than by the engine's recurrences, and checked by brute force
# on small lines. A unit is pixels in address order; for a video with scope "frame" the colours
# are counted in each frame of W*H pixels on its own.

def palette_ok(u, k, scope):
    return all(len(set(u[i:i + scope])) <= k for i in range(0, len(u), scope))


def palette_finish(B, k, m, r):
    """Strings of r more pixels when m colours are already used, using at most k in all: choose the
    j new colours, then the strings over the m + j colours in which each new one appears."""
    total = 0
    for j in range(0, min(k - m, B - m, r) + 1):
        onto = sum((-1) ** i * math.comb(j, i) * (m + j - i) ** r for i in range(j + 1))
        total += math.comb(B - m, j) * onto
    return total if m <= k else 0


def palette_count(B, L, scope, k):
    return palette_finish(B, k, 0, scope) ** (L // scope)


def palette_ways(B, L, scope, k, pos, m):
    """Ways to finish from pixel pos (m colours used in its scope)."""
    left = (scope - pos % scope) % scope
    whole = palette_finish(B, k, 0, scope) ** ((L - pos - left) // scope)
    return whole if left == 0 else palette_finish(B, k, m, left) * whole


def palette_unrank(B, L, scope, k, n):
    out, used = [], set()
    for pos in range(L):
        m = len(used)
        boundary = (pos + 1) % scope == 0
        old = palette_ways(B, L, scope, k, pos + 1, 0 if boundary else m)
        new = palette_ways(B, L, scope, k, pos + 1, 0 if boundary else m + 1) if m < k else 0
        # The symbols in order, each worth `old` (a colour already used) or `new`.
        x = 0
        for c in sorted(used) + [B]:
            gap = c - x  # new colours x .. c-1
            if new and n < gap * new:
                x += n // new
                n %= new
                break
            n -= gap * new
            if c == B:
                raise AssertionError("palette unrank ran past the symbols")
            if n < old:
                x = c
                break
            n -= old
            x = c + 1
        out.append(x)
        used.add(x)
        if boundary:
            used = set()
    return out


def runs_ok(u, W, c):
    return all(sum(u[r + x] != u[r + x - 1] for x in range(1, W)) <= c for r in range(0, len(u), W))


def runs_row(B, W, c):
    return sum(B * math.comb(W - 1, t) * (B - 1) ** t for t in range(min(c, W - 1) + 1))


def runs_ways(B, W, L, c, pos, t):
    """Ways to finish from pixel pos, t changes so far in its row (pos inside a row)."""
    if t > c:
        return 0
    left = (W - pos % W) % W
    rows = runs_row(B, W, c) ** ((L - pos - left) // W)
    if left == 0:
        return rows
    return sum(math.comb(left, s) * (B - 1) ** s for s in range(c - t + 1)) * rows


def runs_unrank(B, W, L, c, n):
    out, t = [], 0
    for pos in range(L):
        if pos % W == 0:
            block = runs_ways(B, W, L, c, pos + 1, 0)
            out.append(n // block)
            n %= block
            t = 0
            continue
        last = out[-1]
        same, other = runs_ways(B, W, L, c, pos + 1, t), runs_ways(B, W, L, c, pos + 1, t + 1)
        if n < last * other:
            d = n // other
            n %= other
        elif n < last * other + same:
            n -= last * other
            d = last
        else:
            n -= last * other + same
            d = last + 1 + n // other
            n %= other
        t += d != last
        out.append(d)
    return out


def rank_by_unrank(unrank, count, u):
    """The rank of u, by halving over the survivors (the oracle has unrank; this gives rank too)."""
    lo, hi = 0, count - 1
    while lo < hi:
        mid = (lo + hi + 1) // 2
        if unrank(mid) <= u:
            lo = mid
        else:
            hi = mid - 1
    assert unrank(lo) == u
    return lo


def cmd_picture_vectors(_args):
    from itertools import product
    print("# Picture filters: palette-size-v1 and row-runs-v1 (core/src/filters/picture.cpp).")
    print("# palette       base width height frames scope colours survivors")
    print("# palette-unit  base width height frames scope colours rank pixels(comma)")
    print("# runs          base width height frames changes survivors")
    print("# runs-unit     base width height frames changes rank pixels(comma)")
    g = stream("picture")
    # Every unit of small lines, judged by the rule, against the counts and the unranking.
    for B, W, H, F, scope, k in ((3, 2, 2, 1, "film", 2), (4, 3, 2, 1, "film", 2), (2, 3, 3, 1, "film", 1), (3, 2, 1, 3, "frame", 1),
                                 (3, 2, 1, 3, "film", 2), (5, 2, 2, 1, "film", 3)):
        L, S = W * H * F, (W * H if scope == "frame" else W * H * F)
        kept = [list(u) for u in product(range(B), repeat=L) if palette_ok(u, k, S)]
        assert len(kept) == palette_count(B, L, S, k)
        assert all(palette_unrank(B, L, S, k, i) == u for i, u in enumerate(kept))
        print(f"palette\t{B}\t{W}\t{H}\t{F}\t{scope}\t{k}\t{len(kept)}")
        for i in sorted({0, len(kept) // 3, len(kept) - 1}):
            print(f"palette-unit\t{B}\t{W}\t{H}\t{F}\t{scope}\t{k}\t{i}\t{','.join(map(str, kept[i]))}")
    for B, W, H, c in ((2, 4, 3, 1), (3, 3, 2, 0), (3, 4, 2, 2), (4, 3, 2, 1)):
        L = W * H
        kept = [list(u) for u in product(range(B), repeat=L) if runs_ok(u, W, c)]
        assert len(kept) == runs_row(B, W, c) ** H
        assert all(runs_unrank(B, W, L, c, i) == u for i, u in enumerate(kept))
        print(f"runs\t{B}\t{W}\t{H}\t1\t{c}\t{len(kept)}")
        for i in sorted({0, len(kept) // 3, len(kept) - 1}):
            print(f"runs-unit\t{B}\t{W}\t{H}\t1\t{c}\t{i}\t{','.join(map(str, kept[i]))}")
    # Full size: the palettes of the image line (mono, ega16, rgb332, rgb24) and video.
    for B, W, H, F, scope, k in ((2, 10, 10, 1, "film", 1), (16, 10, 10, 1, "film", 4), (256, 8, 8, 1, "film", 8),
                                 (1 << 24, 10, 10, 1, "film", 16), (16, 5, 5, 8, "frame", 3), (16, 5, 5, 8, "film", 5),
                                 (1 << 24, 4, 4, 8, "frame", 4)):
        L, S = W * H * F, (W * H if scope == "frame" else W * H * F)
        n = palette_count(B, L, S, k)
        print(f"palette\t{B}\t{W}\t{H}\t{F}\t{scope}\t{k}\t{n}")
        for i in sorted({0, n // 7, n - 1, int.from_bytes(bytes(next(g) for _ in range(40)), "big") % n}):
            u = palette_unrank(B, L, S, k, i)
            assert palette_ok(u, k, S)
            print(f"palette-unit\t{B}\t{W}\t{H}\t{F}\t{scope}\t{k}\t{i}\t{','.join(map(str, u))}")
    for B, W, H, F, c in ((2, 10, 10, 1, 3), (16, 10, 10, 1, 2), (1 << 24, 10, 10, 1, 3), (16, 5, 5, 8, 1), (256, 64, 64, 1, 4)):
        L = W * H * F
        n = runs_row(B, W, c) ** (H * F)
        print(f"runs\t{B}\t{W}\t{H}\t{F}\t{c}\t{n}")
        for i in sorted({0, n // 7, n - 1, int.from_bytes(bytes(next(g) for _ in range(40)), "big") % n}):
            u = runs_unrank(B, W, L, c, i)
            assert runs_ok(u, W, c)
            print(f"runs-unit\t{B}\t{W}\t{H}\t{F}\t{c}\t{i}\t{','.join(map(str, u))}")


# ---------------------------------------------------------------- utf8-valid-v1
# Written from the rule, not the engine's byte automaton: a file is valid when Python's strict
# UTF-8 decoder takes it (RFC 3629) and, with controls = text, it holds no control character but
# tab, line feed and carriage return. Counted by code points: c_k code points take k bytes, so the
# valid files of n bytes number a(n) = sum over k of c_k a(n - k). Ranked through the fact that
# UTF-8 keeps code point order: files of one length compare byte by byte as their code points do.

def utf8_ranges(text_only):
    """Allowed code points as ascending (lo, hi, bytes) ranges."""
    one = [(9, 11, 1), (13, 14, 1), (0x20, 0x7F, 1)] if text_only else [(0, 0x80, 1)]
    two = [(0xA0, 0x800, 2)] if text_only else [(0x80, 0x800, 2)]
    return one + two + [(0x800, 0xD800, 3), (0xE000, 0x10000, 3), (0x10000, 0x110000, 4)]


def utf8_ok(b, text_only):
    try:
        t = b.decode("utf-8")
    except UnicodeDecodeError:
        return False
    if text_only and any((ord(c) < 0x20 and c not in "\t\n\r") or 0x7F <= ord(c) <= 0x9F for c in t):
        return False
    return True


def utf8_counts(n, text_only):
    c = collections.Counter()
    for lo, hi, k in utf8_ranges(text_only):
        c[k] += hi - lo
    a = [1] + [0] * n
    for m in range(1, n + 1):
        a[m] = sum(c[k] * a[m - k] for k in c if k <= m)
    return a


def utf8_unrank(n, text_only, k):
    a = utf8_counts(n, text_only)
    length = 0
    while k >= a[length]:
        k -= a[length]
        length += 1
    out, r = b"", length
    while r:
        for lo, hi, size in utf8_ranges(text_only):
            if size > r:
                continue
            block = a[r - size]
            if k < (hi - lo) * block:
                out += chr(lo + k // block).encode("utf-8")
                k %= block
                r -= size
                break
            k -= (hi - lo) * block
    return out


# utf8-valid-v1 counted with binary-kind-v1 and not-an-item-v1's pages. The kind is decided by the
# first 16 bytes, so the heads are walked byte by byte with two states: the kind walk's, and the
# bytes Python's own incremental UTF-8 decoder is holding back (a character begun). Held-back bytes
# are grouped by what the decoder will take next and how many bytes the character still needs, which
# is all that matters to what follows. Past the head, a file needs only to finish its character and
# then be any valid UTF-8, counted by code points (utf8_counts).

def utf8_control(c):
    return (ord(c) < 0x20 and c not in "\t\n\r") or 0x7F <= ord(c) <= 0x9F


def utf8_feed(pend, b, text_only):
    """The bytes the decoder holds back after `pend` then byte b, or None if it refuses them."""
    import codecs
    d = codecs.getincrementaldecoder("utf-8")("strict")
    try:
        out = d.decode(pend + bytes([b]), final=False)
    except UnicodeDecodeError:
        return None
    if text_only and any(utf8_control(c) for c in out):
        return None
    return d.getstate()[0]


_UTF8_HELD = {}


def utf8_held(text_only):
    """The decoder's held-back states: start key, moves[key][b] (a key or None), needs[key]."""
    if text_only not in _UTF8_HELD:
        def needs(pend):
            if not pend:
                return 0
            return (2 if pend[0] < 0xE0 else 3 if pend[0] < 0xF0 else 4) - len(pend)

        # Two held-back states are the same when they lead the same way after every byte, all the
        # way to the end of the character (at most 3 bytes): the decoder may hold back bytes it
        # refuses one byte later (ED A0, a surrogate's start), so what it takes next is not enough.
        keys = {}

        def key(pend):
            if not pend:
                return ()
            if pend not in keys:
                fut = []
                for b in range(256):
                    q = utf8_feed(pend, b, text_only)
                    if q is not None and (q == b"" or needs(q) > 0):
                        fut.append((b, key(q)))
                keys[pend] = (needs(pend), frozenset(fut))
            return keys[pend]

        start = key(b"")
        reps, moves, todo = {start: b""}, {}, [start]
        while todo:
            k = todo.pop()
            row = []
            for b in range(256):
                q = utf8_feed(reps[k], b, text_only)
                if q is None:
                    row.append(None)
                    continue
                k2 = key(q)
                if k2 not in reps:
                    reps[k2] = q
                    todo.append(k2)
                row.append(k2)
            moves[k] = row
        _UTF8_HELD[text_only] = (start, moves, {k: k[0] if k else 0 for k in reps})
    return _UTF8_HELD[text_only]


_UTF8_HEADS = {}


def utf8_heads(text_only):
    """levels[p]: {(kind state, held key): heads of p bytes}, p = 0..16."""
    if text_only not in _UTF8_HEADS:
        kstart, _levels, kmoves, _groups = kind_walk()
        ustart, umoves, _needs = utf8_held(text_only)
        levels = [{(kstart, ustart): 1}]
        for p in range(KIND_HEAD):
            nxt = {}
            for (st, k), n in levels[p].items():
                for b in range(256):
                    k2 = umoves[k][b]
                    if k2 is None:
                        continue
                    t = (kmoves[p][st][b], k2)
                    nxt[t] = nxt.get(t, 0) + n
            levels.append(nxt)
        _UTF8_HEADS[text_only] = levels
    return _UTF8_HEADS[text_only]


def utf8_joint(n, text_only, kinds, pages=None):
    levels = utf8_heads(text_only)
    start, umoves, needs = utf8_held(text_only)
    total = 0
    for h in range(min(n, KIND_HEAD) + 1):  # whole files within the head: nothing held back
        total += sum(c for (st, k), c in levels[h].items() if k == start and KindHeads.kind(h, st) in kinds)
    if n > KIND_HEAD:
        a = utf8_counts(n - KIND_HEAD, text_only)

        done = {}

        def finish(k, r):  # ways to end the character begun: r more bytes through the decoder
            if r == 0:
                return 1 if k == start else 0
            if (k, r) not in done:
                done[k, r] = sum(finish(k2, r - 1) for k2 in umoves[k] if k2 is not None)
            return done[k, r]

        for (st, k), c in levels[KIND_HEAD].items():
            if KindHeads.kind(KIND_HEAD, st) not in kinds:
                continue
            need = needs[k]
            ways = finish(k, need)
            total += c * ways * sum(a[r - need] for r in range(max(1, need), n - KIND_HEAD + 1))
    if pages is not None and len(pages) <= n:
        assert all(utf8_ok(bytes([b]), text_only) for place in pages for b in place)  # pages are ASCII text
        total -= pattern_in_kinds(kinds, pages)
    return total


def cmd_utf8_vectors(_args):
    print("# utf8-valid-v1 on the binary line (core/src/filekind.cpp, Utf8Counter).")
    print("# utf8       max_bytes controls survivors")
    print("# utf8-unit  max_bytes controls rank file_hex")
    print("# utf8-joint max_bytes controls kinds keep pages(- or alphabet:length) survivors (with binary-kind-v1, and not-an-item-v1's pages)")
    from itertools import product
    for text_only in (False, True):
        ctl = "text" if text_only else "any"
        # Every file of up to 2 bytes, judged by the decoder, against the counts and the order.
        files = [bytes(f) for n in range(3) for f in product(range(256), repeat=n)]
        kept = [f for f in files if utf8_ok(f, text_only)]
        assert len(kept) == sum(utf8_counts(2, text_only))
        assert all(utf8_unrank(2, text_only, i) == f for i, f in enumerate(kept))
        # Counted with the kinds: every file of up to 2 bytes, judged, against the walk.
        kinds_of = [(f, file_kind(f, len(f)), utf8_ok(f, text_only)) for f in files]
        for name in KIND_SET_NAMES:
            for keep in ("keep", "exclude"):
                ks = kind_set(name, keep)
                assert utf8_joint(2, text_only, ks) == sum(1 for _f, k, ok in kinds_of if ok and k in ks), (name, keep)
        g = stream("utf8/" + ctl)
        for n in (1, 2, 3, 4, 16, 32, 100, 1000):  # a binary line holds files of 0 .. n bytes, n >= 1
            total = sum(utf8_counts(n, text_only))
            print(f"utf8\t{n}\t{ctl}\t{total}")
            for i in sorted({0, total // 3, total - 1, int.from_bytes(bytes(next(g) for _ in range(n + 8)), "big") % total}):
                f = utf8_unrank(n, text_only, i)
                assert utf8_ok(f, text_only)
                print(f"utf8-unit\t{n}\t{ctl}\t{i}\t{f.hex() or '-'}")

    for text_only in (False, True):
        ctl = "text" if text_only else "any"
        for n in (1, 2, 16, 17, 20, 32, 100, 1000):
            for kinds, keep in (("any", "keep"), ("text", "keep"), ("text", "exclude"), ("signed", "keep"), ("unknown", "keep"), ("pdf", "keep"), ("exe", "exclude")):
                for pages in ("-", "lower27:20"):
                    pat = None if pages == "-" else pages_pattern(ALPHABETS[pages.split(":")[0]], int(pages.split(":")[1]))
                    print(f"utf8-joint\t{n}\t{ctl}\t{kinds}\t{keep}\t{pages}\t{utf8_joint(n, text_only, kind_set(kinds, keep), pat)}")


def main():
    # Vectors are compared byte for byte: always write "\n" line endings and UTF-8, on every platform.
    sys.stdout.reconfigure(newline="\n", encoding="utf-8")
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    for name in ("warp", "read"):
        s = sub.add_parser(name)
        s.add_argument("--alphabet", default="lower27")
        s.add_argument("--length", type=int, required=True)
        s.add_argument("--key", default="sieve")
        s.add_argument("--mode", default="scrambled", choices=["positional", "scrambled"])
        s.add_argument("text", nargs="+")
    s = sub.add_parser("sieve")
    s.add_argument("--dict", required=True)
    s.add_argument("--max", type=int, default=4)
    sub.add_parser("vectors")
    sub.add_parser("biguint-vectors")
    sub.add_parser("biguint-large-vectors")
    sub.add_parser("digit-vectors")
    sub.add_parser("bytes-vectors")
    sub.add_parser("canon-vectors")
    sub.add_parser("alphabet-vectors")
    sub.add_parser("model-vectors")
    sub.add_parser("image-vectors")
    s = sub.add_parser("model-build")
    s.add_argument("--corpus", default="../data/models/corpus/gutenberg-nltk.tsv")
    s.add_argument("--texts", default="../corpus/gutenberg")
    s.add_argument("--alphabet", default="lower27")
    s.add_argument("--order", type=int, default=5)
    s.add_argument("--min-count", type=int, default=8)
    s.add_argument("--out", required=True)
    sub.add_parser("filter-vectors")
    sub.add_parser("compact-vectors")
    sub.add_parser("book-vectors")
    sub.add_parser("titled-vectors")
    sub.add_parser("binary-vectors")
    sub.add_parser("chunk-vectors")
    sub.add_parser("notes2-vectors")
    sub.add_parser("pcm-vectors")
    sub.add_parser("sound-vectors")
    sub.add_parser("notes3-vectors")
    sub.add_parser("composition-vectors")
    sub.add_parser("world-vectors")
    s = sub.add_parser("world-obj")
    s.add_argument("--slots", required=True)
    s.add_argument("--world-models", type=int, default=4)
    s.add_argument("--world-grid", type=int, default=8)
    s.add_argument("--vertices", type=int, default=8)
    s.add_argument("--faces", type=int, default=12)
    s.add_argument("--coords", type=int, default=16)
    s.add_argument("--key", default="sieve")
    sub.add_parser("kind-vectors")
    sub.add_parser("written-vectors")
    sub.add_parser("cross-vectors")
    sub.add_parser("picture-vectors")
    sub.add_parser("utf8-vectors")
    s = sub.add_parser("plugin")
    s.add_argument("file")
    s.add_argument("--length", type=int, default=32)
    s.add_argument("--params", default="")
    s.add_argument("--base", type=int)
    s.add_argument("--lazy", action="store_true")  # the token form, without determinising in full
    s.add_argument("--judge")  # a text file: each line judged
    s.add_argument("--line")  # the engine's option, taken for the same command line (audio)
    s.add_argument("--note-set", default="notes104")
    s.add_argument("--low")  # default C3 (notes2), C-1 (notes3)
    s.add_argument("--high")  # default C6 (notes2), G9 (notes3)
    s.add_argument("--durations", default="seEqQhHw")
    s.add_argument("--voices", type=int, default=1)
    s.add_argument("--tpq", type=int, default=4)  # notes3
    s.add_argument("--longest", type=int, default=16)
    s.add_argument("--levels", type=int, default=8)
    s = sub.add_parser("chunks")
    s.add_argument("file")
    s = sub.add_parser("manifest")
    s.add_argument("folder")
    s.add_argument("--addresses")
    s.add_argument("--with-addresses", action="store_true")
    s.add_argument("--with-contents", action="store_true")
    s = sub.add_parser("unpack")
    s.add_argument("manifest")
    s.add_argument("--to")
    s = sub.add_parser("map")
    s.add_argument("folder", nargs="+")
    s.add_argument("--name")
    s.add_argument("--seal", action="store_true")
    s.add_argument("--held", action="store_true")
    s.add_argument("--meta", action="append", default=[])
    sub.add_parser("book-filter-vectors")
    s = sub.add_parser("book-read")
    s.add_argument("book")
    s = sub.add_parser("guided-vectors")
    s.add_argument("--model", default="../data/models/gutenberg-lower27-o5.model")
    args = p.parse_args()

    if args.cmd == "vectors":
        cmd_vectors(args)
    elif args.cmd == "biguint-vectors":
        cmd_biguint_vectors(args)
    elif args.cmd == "biguint-large-vectors":
        cmd_biguint_large_vectors(args)
    elif args.cmd == "bytes-vectors":
        cmd_bytes_vectors(args)
    elif args.cmd == "digit-vectors":
        cmd_digit_vectors(args)
    elif args.cmd == "canon-vectors":
        cmd_canon_vectors(args)
    elif args.cmd == "alphabet-vectors":
        cmd_alphabet_vectors(args)
    elif args.cmd == "model-vectors":
        cmd_model_vectors(args)
    elif args.cmd == "image-vectors":
        cmd_image_vectors(args)
    elif args.cmd == "sieve":
        cmd_sieve(args)
    elif args.cmd == "model-build":
        cmd_model_build(args)
    elif args.cmd == "guided-vectors":
        cmd_guided_vectors(args)
    elif args.cmd == "filter-vectors":
        cmd_filter_vectors(args)
    elif args.cmd == "compact-vectors":
        cmd_compact_vectors(args)
    elif args.cmd == "titled-vectors":
        cmd_titled_vectors(args)
    elif args.cmd == "binary-vectors":
        cmd_binary_vectors(args)
    elif args.cmd == "chunk-vectors":
        cmd_chunk_vectors(args)
    elif args.cmd == "notes2-vectors":
        cmd_notes2_vectors(args)
    elif args.cmd == "pcm-vectors":
        cmd_pcm_vectors(args)
    elif args.cmd == "sound-vectors":
        cmd_sound_vectors(args)
    elif args.cmd == "notes3-vectors":
        cmd_notes3_vectors(args)
    elif args.cmd == "composition-vectors":
        cmd_composition_vectors(args)
    elif args.cmd == "world-vectors":
        cmd_world_vectors(args)
    elif args.cmd == "world-obj":
        cmd_world_obj(args)
    elif args.cmd == "kind-vectors":
        cmd_kind_vectors(args)
    elif args.cmd == "written-vectors":
        cmd_written_vectors(args)
    elif args.cmd == "cross-vectors":
        cmd_cross_vectors(args)
    elif args.cmd == "picture-vectors":
        cmd_picture_vectors(args)
    elif args.cmd == "utf8-vectors":
        cmd_utf8_vectors(args)
    elif args.cmd == "chunks":
        cmd_chunks(args)
    elif args.cmd == "plugin":
        cmd_plugin(args)
    elif args.cmd == "manifest":
        cmd_manifest(args)
    elif args.cmd == "unpack":
        cmd_unpack(args)
    elif args.cmd == "map":
        cmd_map(args)
    elif args.cmd == "book-vectors":
        cmd_book_vectors(args)
    elif args.cmd == "book-filter-vectors":
        cmd_book_filter_vectors(args)
    elif args.cmd == "book-read":
        cmd_book_read(args)
    elif args.cmd == "warp":
        sp = Space(args.alphabet, args.length, args.key)
        for u in canonicalise(" ".join(args.text), args.alphabet, args.length):
            print(repr(u), sp.address_of(u, args.mode))
    elif args.cmd == "read":
        sp = Space(args.alphabet, args.length, args.key)
        print(sp.unit_at(args.text[0], args.mode))


if __name__ == "__main__":
    sys.exit(main())
