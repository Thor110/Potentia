#!/usr/bin/env python3
"""Makes filter plugins that search the space around known content (Potentia's layers 4 to 6).

Two kinds, each written as a table-form .sfilter (docs/FILTER-PLUGINS.md), so the engine judges,
counts, ranks and compacts its survivors exactly, the oracle checks it with its own engine, and it
combines with every other automaton on the line (the word and grammar plugins, not-written-v1):

    near      the units within `distance` substitutions of an anchor: a page that differs from a
              known page in a few characters (a damaged copy, a variant, a misprint). The anchors
              fix the unit length; a unit of any other length passes none of them.
    contains  the units that hold at least one of the fragments somewhere: pages that quote a
              known line of a lost work, wherever on the page it falls (an Aho-Corasick machine).

The anchors and fragments are text, one per line of a UTF-8 file, read onto the line's alphabet as
canon-text-v2 reads a lower-case line (lower27, babel29: case folded, any other character a
SPACE, runs of SPACEs one) or exactly (ascii95, ascii96: every character must be on the line).
The rule is the file, so the plugin is versioned by its bytes like any other; the anchors are
written into it, and their source file's SHA-256 is in its comments.

    python tools/build_anchor_plugins.py near --id dickens-near --anchors pages.txt --distance 3 --out data/filters
    python tools/build_anchor_plugins.py contains --id lost-lines --fragments lines.txt --out data/filters

With one anchor the distance is a parameter of the plugin (up to --distance); with several it is
fixed at --distance, because the machine is the union of their neighbourhoods made deterministic
here, and its states depend on it.
"""

import argparse
import collections
import hashlib
import os
import re
import sys

LETTERS = "abcdefghijklmnopqrstuvwxyz"
ALPHABETS = {
    "lower27": " " + LETTERS,
    "babel29": " " + LETTERS + ",.",
    "ascii95": "".join(chr(c) for c in range(0x20, 0x7F)),
    "ascii96": "\n" + "".join(chr(c) for c in range(0x20, 0x7F)),
}


def fold(text, symbols):
    """Text onto the line's alphabet, as described above."""
    alpha = ALPHABETS[symbols]
    if symbols in ("lower27", "babel29"):
        text = "".join(c if c in alpha else " " for c in text.lower())
        return re.sub(" +", " ", text)
    bad = sorted({c for c in text if c not in alpha})
    if bad:
        sys.exit(f"not on {symbols}: {' '.join(repr(c) for c in bad)}")
    return text


def quoted(chars):
    out = []
    for c in chars:
        out.append({"\n": "\\n", "\t": "\\t", '"': '\\"', "\\": "\\\\"}.get(c, c))
    return '"' + "".join(out) + '"'


def header(a, describe, comment, source_sha):
    lines = ["sieve-filter-v2"]
    lines += ["; " + c if c else ";" for c in comment]
    lines += [f"; Made by tools/build_anchor_plugins.py from {os.path.basename(a.source)} (sha256 {source_sha}).",
              f"id        {a.id}", f"version   {a.version}", f"author    {a.author}", f"origin    {a.origin}",
              "lines     text", f"symbols   {a.symbols}", f"describe  {describe}"]
    return lines


def read_lines(path, symbols):
    data = open(path, "rb").read()
    items = [fold(l, symbols) for l in data.decode("utf-8").split("\n") if l.strip() and not l.startswith("#")]
    return items, hashlib.sha256(data).hexdigest()


def near(a):
    anchors, sha = read_lines(a.source, a.symbols)
    alpha = ALPHABETS[a.symbols]
    if not anchors:
        sys.exit("no anchors")
    L = len(anchors[0])
    if any(len(x) != L for x in anchors):
        sys.exit("the anchors must all have the same length (it is the unit length the plugin judges)")
    show = [x.replace("\n", "\\n") for x in anchors]
    comment = [f"Near an anchor: a unit of {L} symbols within so many substitutions of one of these",
               "anchors (Hamming distance), for searching the space around known content:"]
    comment += [f"  \"{x}\"" for x in show[:20]] + ([f"  ... and {len(show) - 20} more"] if len(show) > 20 else [])
    if len(anchors) == 1:
        x = anchors[0]
        lines = header(a, f"Within the distance (substitutions) of the anchor \"{show[0][:40]}\".", comment +
                       ["The state is the place in the unit and the substitutions so far: 1 + p * (distance + 1) + m;",
                        "state 0 is the end of the unit, reached only by a unit of the anchor's length."], sha)
        lines += [f"param     distance  int  {a.distance}  0  {max(a.distance, L)}  the most substitutions allowed", "",
                  f"states    {{1 + {L} * (distance + 1)}}", "start     1", "accept    0", "for m 0 {distance}"]
        for p, ch in enumerate(x):
            here = f"{{1 + {p} * (distance + 1) + m}}"
            nxt = "0" if p == L - 1 else f"{{1 + {p + 1} * (distance + 1) + m}}"
            nxt_miss = "0" if p == L - 1 else f"{{1 + {p + 1} * (distance + 1) + m + 1}}"
            others = [c for c in alpha if c != ch]
            lines.append(f"  t {here} {quoted(ch)} {nxt}")
            lines += ["  if {m < distance}", f"    t {here} {quoted(others)} {nxt_miss}", "  fi"]
        return lines + ["done", "end"]
    # Several anchors: states are (place, the substitutions so far against each anchor, capped at
    # distance + 1 = out of reach), made deterministic breadth first.
    k = a.distance
    start = (0, tuple(0 for _ in anchors))
    index, order, trans = {start: 0}, [start], []
    i = 0
    while i < len(order):
        p, ms = order[i]
        if p < L:
            by = collections.defaultdict(list)
            for c in alpha:
                nm = tuple(min(k + 1, m + (x[p] != c)) for m, x in zip(ms, anchors))
                if all(m > k for m in nm):
                    continue
                st = (p + 1, nm)
                if st not in index:
                    index[st] = len(order)
                    order.append(st)
                by[index[st]].append(c)
            for t, cs in sorted(by.items()):
                trans.append(f"t {i} {quoted(cs)} {t}")
        i += 1
    accept = [str(j) for j, (p, ms) in enumerate(order) if p == L]
    lines = header(a, f"Within {k} substitutions of one of {len(anchors)} anchors.", comment +
                   [f"The distance is fixed at {k}: the states are the place in the unit and the substitutions so",
                    "far against each anchor, made deterministic."], sha)
    lines += ["", f"states    {len(order)}", "start     0", "accept    " + " ".join(accept)]
    return lines + trans + ["end"]


def contains(a):
    frags, sha = read_lines(a.source, a.symbols)
    alpha = ALPHABETS[a.symbols]
    frags = [f for f in frags if f]
    if not frags:
        sys.exit("no fragments")
    # Aho-Corasick: a trie of the fragments with failure links, and one state, FOUND, that every
    # completed fragment leads to and that keeps the unit for good.
    goto, fail, out = [{}], [0], [False]
    for f in frags:
        s = 0
        for c in f:
            if c not in goto[s]:
                goto[s][c] = len(goto)
                goto.append({})
                fail.append(0)
                out.append(False)
            s = goto[s][c]
        out[s] = True
    queue = collections.deque(goto[0].values())
    while queue:
        s = queue.popleft()
        for c, t in goto[s].items():
            queue.append(t)
            f = fail[s]
            while f and c not in goto[f]:
                f = fail[f]
            fail[t] = goto[f][c] if c in goto[f] and goto[f][c] != t else 0
            out[t] = out[t] or out[fail[t]]

    def step(s, c):
        while s and c not in goto[s]:
            s = fail[s]
        return goto[s].get(c, 0)

    n = len(goto)
    found = n
    lines = header(a, f"Holds at least one of {len(frags)} fragments.", [
        "Contains a fragment: the unit holds at least one of these fragments somewhere in it, for",
        "searching for pages that quote known lines of a lost work:"] +
        [f"  \"{x.replace(chr(10), chr(92) + 'n')}\"" for x in frags[:20]] +
        ([f"  ... and {len(frags) - 20} more"] if len(frags) > 20 else []) +
        [f"The states are a machine over the fragments (Aho-Corasick); state {found} is a fragment found."], sha)
    lines += ["", f"states    {n + 1}", "start     0", f"accept    {found}", f"t {found} {quoted(alpha)} {found}"]
    for s in range(n):
        by = collections.defaultdict(list)
        for c in alpha:
            t = step(s, c)
            by[found if out[t] else t].append(c)
        for t, cs in sorted(by.items()):
            lines.append(f"t {s} {quoted(cs)} {t}")
    return lines + ["end"]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    sub = ap.add_subparsers(dest="kind", required=True)
    for kind in ("near", "contains"):
        s = sub.add_parser(kind)
        s.add_argument("--id", required=True, help="the plugin's id (lower-case letters, digits, hyphens)")
        s.add_argument("--version", type=int, default=1)
        s.add_argument("--symbols", default="lower27", choices=sorted(ALPHABETS))
        s.add_argument("--author", default="Edward James Gordon")
        s.add_argument("--origin", default="human", choices=["human", "ai-directed", "ai"])
        s.add_argument("--out", default=".", help="the folder to write <id>-v<version>.sfilter into")
        if kind == "near":
            s.add_argument("--anchors", dest="source", required=True, help="UTF-8 text, one anchor per line")
            s.add_argument("--distance", type=int, default=2)
        else:
            s.add_argument("--fragments", dest="source", required=True, help="UTF-8 text, one fragment per line")
    a = ap.parse_args()
    if not re.fullmatch(r"[a-z0-9]+(-[a-z0-9]+)*", a.id):
        sys.exit("an id is lower-case letters, digits and hyphens")
    lines = near(a) if a.kind == "near" else contains(a)
    data = ("\n".join(lines) + "\n").encode("utf-8")
    path = os.path.join(a.out, f"{a.id}-v{a.version}.sfilter")
    with open(path, "wb") as f:
        f.write(data)
    print(f"{path}  {len(data)} bytes  sha256 {hashlib.sha256(data).hexdigest()}")


if __name__ == "__main__":
    main()
