#!/usr/bin/env python3
"""Writes tools/cli/case_fold_table.hpp: each Unicode character that a folder ignoring case takes as
another, and the one it takes it as (6 November 2026, IDEAS 19.120: names that are one name where
case is ignored, beyond A to Z).

The fold is one character to one: a character's lowercase of its uppercase, where both are a
single character (Python's own Unicode database, its version written in the table). That is how
Windows compares names (its upcase table maps each character to one other), and close to macOS's;
so `ß` stays `ß` (not "ss"), `É` folds to `é`, `Σ`, `σ` and `ς` to one. Run it with the Python that
is to set the Unicode version: python tools/gen_case_fold.py > tools/cli/case_fold_table.hpp"""
import sys
import unicodedata


def fold(c):
    u = c.upper()
    if len(u) != 1:
        u = c
    lo = u.lower()
    return lo if len(lo) == 1 else u


pairs = []
for cp in range(0x110000):
    if 0xD800 <= cp <= 0xDFFF:
        continue
    c = chr(cp)
    f = fold(c)
    if f != c:
        pairs.append((cp, ord(f)))

out = sys.stdout
out.write("// Made by tools/gen_case_fold.py (Unicode %s, Python's own database): each character a folder\n" % unicodedata.unidata_version)
out.write("// ignoring case takes as another, and that other: %d pairs, sorted by the first. Do not edit: run\n" % len(pairs))
out.write("// the script again.\n#pragma once\n\n#include <cstdint>\n\nnamespace sieve::cli\n{\n\n")
out.write("struct CaseFoldPair\n{\n    uint32_t from, to;\n};\n\n")
out.write("inline constexpr CaseFoldPair kCaseFold[] = {\n")
line = "   "
for a, b in pairs:
    item = " {0x%X, 0x%X}," % (a, b)
    if len(line) + len(item) > 110:
        out.write(line + "\n")
        line = "   "
    line += item
out.write(line + "\n};\n\n} // namespace sieve::cli\n")
