#!/usr/bin/env python3
"""Potentia reference oracle (SPECIFICATIONS §12).

An independent, deliberately plain implementation of the raw address map,
canonicalisation and the M1 sieve filters. It uses Python's native big integers
and hashlib, sharing no code with the C++ core. The C++ core must agree with it
bit for bit; the conformance vectors in tests/vectors_v1.tsv are generated here.

    python3 potentia_ref.py vectors        > ../tests/vectors_v1.tsv
    python3 potentia_ref.py digit-vectors  > ../tests/vectors_digits_v1.tsv
    python3 potentia_ref.py canon-vectors  > ../tests/vectors_canon.tsv
    python3 potentia_ref.py image-vectors  > ../tests/vectors_image_v1.tsv
    python3 potentia_ref.py warp --length 32 "Some text"
    python3 potentia_ref.py read --length 32 --mode scrambled <hex>
    python3 potentia_ref.py sieve --dict words.txt --max 4
"""
import argparse
import hashlib
import struct
import sys
import unicodedata
from fractions import Fraction

ALPHABETS = {
    "lower27": " " + "abcdefghijklmnopqrstuvwxyz",
    "babel29": " " + "abcdefghijklmnopqrstuvwxyz" + ",.",
    "ascii95": "".join(chr(c) for c in range(0x20, 0x7F)),
}
ROUNDS = 8
WHITESPACE = {"\t", "\n", "\x0b", "\x0c", "\r", " ", "\xa0"}


class Space:
    def __init__(self, alphabet_id, length, key="potentia", base=None):
        # A text space (alphabet_id) or, with base=N, a space over N abstract symbols.
        self.symbols = ALPHABETS[alphabet_id] if alphabet_id else None
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
        prefix = b"POTENTIA/FEISTEL/1"
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
    symbols = ALPHABETS[alphabet_id]
    fold = not any("A" <= c <= "Z" for c in symbols)
    kept = []
    for orig in text:
        if orig in WHITESPACE:
            expanded = " "
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
    return [s[i:i + length].ljust(length) for i in range(0, len(s), length)]


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
    print("# potentia conformance vectors v1 (spec 2.0, feistel-sha256-v1)")
    print("# alphabet\tlength\tkey\tunit\tpositional\tscrambled")
    cases = []
    for alpha, lengths in (("lower27", [1, 2, 3, 7, 16, 32, 100]),
                           ("babel29", [1, 5, 40]),
                           ("ascii95", [1, 8, 64])):
        for L in lengths:
            for key in ("potentia", "alt-key"):
                sp = Space(alpha, L, key)
                g = stream(f"{alpha}/{L}/{key}")
                units = [" " * L, sp.symbols[-1] * L]
                for _ in range(4):
                    units.append("".join(sp.symbols[next(g) % sp.n] for _ in range(L)))
                cases += [(alpha, L, key, u) for u in units]
    # Readable examples.
    for text in ("it was the best of times", "the library of babel"):
        for L in (32, 1000):
            cases.append(("lower27", L, "potentia", canonicalise(text, "lower27", L)[0]))
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


def cmd_digit_vectors(_args):
    """Address vectors over abstract bases (image palettes, notes, video): base, length, key, digits, pos, scr."""
    print("# potentia digit conformance vectors v1 (feistel-sha256-v1)")
    print("# base\tlength\tkey\tdigits\tpositional\tscrambled")
    for base, lengths in ((2, [1, 25, 100, 200]), (16, [1, 100]), (104, [1, 16]), (256, [100]),
                          (16777216, [1, 4, 100])):
        for L in lengths:
            for key in ("potentia", "alt-key"):
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
    print("# potentia text canonicalisation vectors (canon-text-v1 and canon-text-v2)")
    print("# version\talphabet\tlength\tinput_hex\tunits_hex")
    for text in inputs:
        for version in ("v1", "v2"):
            for alpha, L in (("lower27", 16), ("babel29", 24), ("ascii95", 20)):
                units = canonicalise(text, alpha, L, version)
                uh = ",".join(u.encode("utf-8").hex() for u in units)
                print(f"{version}\t{alpha}\t{L}\t{text.encode('utf-8').hex()}\t{uh}")


def cmd_image_vectors(_args):
    """Image canonicalisation vectors: palette, srcW, srcH, dstW, dstH, rgba hex, digits."""
    print("# potentia image canonicalisation vectors (canon-image-v1)")
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


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    for name in ("warp", "read"):
        s = sub.add_parser(name)
        s.add_argument("--alphabet", default="lower27")
        s.add_argument("--length", type=int, required=True)
        s.add_argument("--key", default="potentia")
        s.add_argument("--mode", default="scrambled", choices=["positional", "scrambled"])
        s.add_argument("text", nargs="+")
    s = sub.add_parser("sieve")
    s.add_argument("--dict", required=True)
    s.add_argument("--max", type=int, default=4)
    sub.add_parser("vectors")
    sub.add_parser("digit-vectors")
    sub.add_parser("canon-vectors")
    sub.add_parser("image-vectors")
    args = p.parse_args()

    if args.cmd == "vectors":
        cmd_vectors(args)
    elif args.cmd == "digit-vectors":
        cmd_digit_vectors(args)
    elif args.cmd == "canon-vectors":
        cmd_canon_vectors(args)
    elif args.cmd == "image-vectors":
        cmd_image_vectors(args)
    elif args.cmd == "sieve":
        cmd_sieve(args)
    elif args.cmd == "warp":
        sp = Space(args.alphabet, args.length, args.key)
        for u in canonicalise(" ".join(args.text), args.alphabet, args.length):
            print(repr(u), sp.address_of(u, args.mode))
    elif args.cmd == "read":
        sp = Space(args.alphabet, args.length, args.key)
        print(sp.unit_at(args.text[0], args.mode))


if __name__ == "__main__":
    sys.exit(main())
