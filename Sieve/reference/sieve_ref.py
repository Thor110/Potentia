#!/usr/bin/env python3
"""Sieve reference oracle (SPECIFICATIONS §12).

An independent, deliberately plain implementation of the raw address map,
canonicalisation and the M1 sieve filters. It uses Python's native big integers
and hashlib, sharing no code with the C++ core. The C++ core must agree with it
bit for bit; the conformance vectors in tests/vectors_v1.tsv are generated here.

    python3 sieve_ref.py biguint-vectors > ../tests/vectors_biguint_v1.tsv
    python3 sieve_ref.py vectors        > ../tests/vectors_v1.tsv
    python3 sieve_ref.py digit-vectors  > ../tests/vectors_digits_v1.tsv
    python3 sieve_ref.py canon-vectors  > ../tests/vectors_canon.tsv
    python3 sieve_ref.py image-vectors  > ../tests/vectors_image_v1.tsv
    python3 sieve_ref.py guided-vectors > ../tests/vectors_guided_v1.tsv
    python3 sieve_ref.py filter-vectors > ../tests/vectors_filters_v1.tsv
    python3 sieve_ref.py model-build --out /tmp/check.model   # rebuilds the pinned model
    python3 sieve_ref.py warp --length 32 "Some text"
    python3 sieve_ref.py read --length 32 --mode scrambled <hex>
    python3 sieve_ref.py sieve --dict words.txt --max 4
"""
import argparse
import hashlib
import math
import struct
import sys
import unicodedata
from fractions import Fraction

# Pinned symbol alphabets. The order of the symbols IS the digit order, and babel29's is
# libraryofbabel.info's, which is not code point order.
ALPHABETS = {
    "lower27": " " + "abcdefghijklmnopqrstuvwxyz",
    "babel29": " " + "abcdefghijklmnopqrstuvwxyz" + ",.",
    "ascii95": "".join(chr(c) for c in range(0x20, 0x7F)),
    "ascii96": "\n" + "".join(chr(c) for c in range(0x20, 0x7F)),
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
    sub.add_parser("digit-vectors")
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
