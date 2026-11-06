#!/usr/bin/env python3
"""Writes tools/cli/normalise_table.hpp: Unicode's canonical decompositions (each character that
has one, fully decomposed, Hangul syllables apart: they are worked out by rule) and the combining
classes of the marks (6 November 2026, IDEAS 19.120: macOS takes a name written with é as one
character and with e and a combining accent as the same name). From Python's own Unicode
database, its version written in the table: python tools/gen_normalise.py > tools/cli/normalise_table.hpp"""
import sys
import unicodedata

dec = []
for cp in range(0x110000):
    if 0xAC00 <= cp <= 0xD7A3 or 0xD800 <= cp <= 0xDFFF:
        continue
    d = unicodedata.decomposition(chr(cp))
    if d and not d.startswith("<"):
        full = [ord(c) for c in unicodedata.normalize("NFD", chr(cp))]
        assert 1 <= len(full) <= 4
        dec.append((cp, full + [0] * (4 - len(full))))
ccc = [(cp, unicodedata.combining(chr(cp))) for cp in range(0x110000) if not 0xD800 <= cp <= 0xDFFF and unicodedata.combining(chr(cp))]

out = sys.stdout
out.write("// Made by tools/gen_normalise.py (Unicode %s, Python's own database): %d canonical decompositions,\n" % (unicodedata.unidata_version, len(dec)))
out.write("// each in full (up to four characters, 0 after; Hangul syllables apart, decomposed by rule),\n")
out.write("// and %d combining classes, both sorted by the character. Do not edit: run the script again.\n" % len(ccc))
out.write("#pragma once\n\n#include <cstdint>\n\nnamespace sieve::cli\n{\n\n")
out.write("struct Decomposition\n{\n    uint32_t from;\n    uint32_t to[4];\n};\n\nstruct CombiningClass\n{\n    uint32_t cp;\n    uint8_t ccc;\n};\n\n")


def table(name, kind, rows, fmt):
    out.write("inline constexpr %s %s[] = {\n" % (kind, name))
    line = "   "
    for r in rows:
        item = " " + fmt(r) + ","
        if len(line) + len(item) > 110:
            out.write(line + "\n")
            line = "   "
        line += item
    out.write(line + "\n};\n\n")


table("kDecompositions", "Decomposition", dec, lambda r: "{0x%X, {0x%X, 0x%X, 0x%X, 0x%X}}" % (r[0], *r[1]))
table("kCombiningClasses", "CombiningClass", ccc, lambda r: "{0x%X, %d}" % r)
out.write("} // namespace sieve::cli\n")
