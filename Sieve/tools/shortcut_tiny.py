#!/usr/bin/env python3
r"""The shortcut seed, stage 1b (Edward, 1 November 2026; IDEAS §19.111): every arrangement of a
small file, and what real files favour.

Edward: "design a limited file with every possible arrangement and check every possible filter
against it ... think counting cards but bytes". Two parts.

1. Every file of a small set, each searched for its shortest recipe by the shortcut seed
   (tools/shortcut_seed.py, every transform, and carried as it is). Four sets of 65,536 files:
   every 2-byte file; 16 bytes each 00 or FF; 8 bytes each 00 to 03; 4 bytes each 00 to 0F. For
   each set: how the recipes' bits spread, how many files come under the set's own floor (the bits
   that name one file of 65,536: 16), which shortcuts and transforms the cheapest use, and the sum
   over every file of 2^-bits (a code that installs every file exactly cannot pass 1, the counting
   argument checked).

2. Counting the cards: the 2-, 3- and 4-byte arrangements of the repository's files, counted by
   kind of file on each file's first half and tried on its second (held out). Each arrangement is
   named by its rank in the counted order (the shuffle the counts give: the commonest first), as an
   Elias gamma code of the rank, an arrangement not counted by an escape and its bytes. Beside it,
   the bytes as they are (8 bits a byte) and the counts' own cost (-log2 of the arrangement's
   share, smoothed). Then the shuffle counted on one kind tried on every other.

  python tools/shortcut_tiny.py FOLDER [--out FOLDER] [--processes N] [--only 1|2]

FOLDER holds the real files (the stage 1 set); each is assigned a kind by its name.
"""

import argparse
import itertools
import math
import os
import sys
from multiprocessing import Pool
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
import shortcut_seed as seed  # noqa: E402

SETS = [
    ("every 2-byte file", 2, list(range(256))),
    ("16 bytes, each 00 or FF", 16, [0x00, 0xFF]),
    ("8 bytes, each 00 to 03", 8, [0, 1, 2, 3]),
    ("4 bytes, each 00 to 0F", 4, list(range(16))),
]


def one(args):
    data, w = args
    res = seed.search(bytes(data), w)
    b = min(res, key=lambda r: r["bits"])
    c = b["counts"]
    used = tuple(k for k in seed.KINDS if c.get(k))
    return b["bits"], b["transform"], used


def part1(out, processes):
    lines = ["# Every arrangement of a small file\n",
             "Each set holds 65,536 files; naming one of them takes 16 bits. The shortcut seed does not know the set: "
             "it codes bytes, as it would any file, so its bits include saying which bytes. Bits are the recipe's, before rounding to bytes.\n"]
    for name, n, values in SETS:
        files = [bytes(t) for t in itertools.product(values, repeat=n)]
        w = 4 if n >= 8 else 0
        with Pool(processes) as p:
            got = p.map(one, [(f, w) for f in files], chunksize=256)
        bits = np.array([g[0] for g in got])
        kraft = float(np.sum(np.power(2.0, -bits)))
        order = np.argsort(bits, kind="stable")
        tally_t, tally_k = {}, {}
        for g in got:
            tally_t[g[1]] = tally_t.get(g[1], 0) + 1
            tally_k[g[2] or ("literals only",)] = tally_k.get(g[2] or ("literals only",), 0) + 1
        lines.append(f"\n## {name}\n")
        lines.append(f"- Bits: least {bits.min():.1f}, median {np.median(bits):.1f}, mean {bits.mean():.1f}, most {bits.max():.1f}; "
                     f"under 16: {int((bits < 16).sum()):,} of {len(files):,}.")
        lines.append(f"- Sum of 2^-bits over every file: {kraft:.6f} (at most 1 for any code that installs every file exactly).")
        lines.append("- The cheapest ten: " + "; ".join(f"`{files[i].hex(' ')}` {bits[i]:.1f}" for i in order[:10]) + ".")
        lines.append("- The dearest three: " + "; ".join(f"`{files[i].hex(' ')}` {bits[i]:.1f}" for i in order[-3:]) + ".")
        lines.append("- Transforms taken: " + ", ".join(f"{k} {v:,}" for k, v in sorted(tally_t.items(), key=lambda kv: -kv[1])) + ".")
        lines.append("- Shortcuts used: " + ", ".join(f"{'+'.join(k)} {v:,}" for k, v in sorted(tally_k.items(), key=lambda kv: -kv[1])) + ".")
        print(name, "done", flush=True)
        (out / "every-arrangement.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


KIND_OF = [
    ("text", (".md", ".txt")), ("code", (".cpp", ".py")), ("data", (".obj", ".mid", ".csv")),
    ("pixels", (".rgb", ".gray")), ("compressed", (".jpg", ".png", ".mp4", ".sieve")), ("program", (".elf",)),
]


def kind(name):
    if name.startswith("designed-") or name == "random.bin":
        return None
    for k, ends in KIND_OF:
        if name.endswith(ends):
            return k
    return None


def windows(b, k):
    """The file's k-byte arrangements, side by side (not overlapping), as numbers."""
    a = np.frombuffer(b[: len(b) // k * k], dtype=np.uint8).reshape(-1, k).astype(np.uint64)
    v = np.zeros(len(a), dtype=np.uint64)
    for j in range(k):
        v = (v << np.uint64(8)) | a[:, j]
    return v


def gamma_bits(r):
    return 2 * np.floor(np.log2(r)) + 1


def costs(train, test, k):
    """Bits a byte of the held-out arrangements: as they are, by rank in the counted order, by the
    counts' own share."""
    vals, cnt = np.unique(train, return_counts=True)
    order = np.argsort(-cnt, kind="stable")
    ranked = vals[order]
    rank_of = {int(v): i + 1 for i, v in enumerate(ranked)}
    share = {int(v): c for v, c in zip(vals, cnt)}
    total = float(cnt.sum())
    seen = len(vals)
    by_rank, by_share = 0.0, 0.0
    escape = 2 * math.floor(math.log2(seen + 1)) + 1 + 8 * k  # (rank seen + 1 says "not counted", then its bytes)
    unseen_share = 0.5 * (256 ** k - seen)
    for v in test.tolist():
        r = rank_of.get(v)
        by_rank += (2 * math.floor(math.log2(r)) + 1) if r else escape
        c = share.get(v, 0)
        by_share += -math.log2((c + 0.5) / (total + 0.5 * seen + unseen_share)) if c else -math.log2(0.5 / (total + 0.5 * seen + unseen_share))
    n = len(test) * k
    return 8.0, by_rank / n, by_share / n


def part2(folder, out):
    files = {}
    for f in sorted(Path(folder).iterdir()):
        k = kind(f.name)
        if k:
            b = f.read_bytes()[: 4 << 20]
            files.setdefault(k, []).append(b)
    lines = ["# Counting the cards\n",
             "The repository's files (the stage 1 set, each up to its first 4 MiB) by kind. Each file's first half counted, its second half coded "
             "(held out). Bits a byte: the bytes as they are; each arrangement named by its rank in the counted order (the commonest first, an "
             "Elias gamma code; one not counted by an escape and its bytes); and by the counts' own share (smoothed).\n",
             "| Kind | Files | Arrangement | As they are | By rank (the shuffle) | By share |", "| :--- | ---: | ---: | ---: | ---: | ---: |"]
    halves = {k: ([b[: len(b) // 2] for b in v], [b[len(b) // 2:] for b in v]) for k, v in files.items()}
    for k, (tr, te) in halves.items():
        for size in (2, 3, 4):
            train = np.concatenate([windows(b, size) for b in tr])
            test = np.concatenate([windows(b, size) for b in te])
            a, r, s = costs(train, test, size)
            lines.append(f"| {k} | {len(tr)} | {size} bytes | {a:.2f} | {r:.2f} | {s:.2f} |")
        print("counted", k, flush=True)
    kinds = list(halves)
    lines += ["", "## The shuffle counted on one kind, tried on another (2-byte arrangements, bits a byte by rank)\n",
              "| Counted on \\ tried on | " + " | ".join(kinds) + " |", "| :--- |" + " ---: |" * len(kinds)]
    for k in kinds:
        train = np.concatenate([windows(b, 2) for b in halves[k][0]])
        row = []
        for j in kinds:
            test = np.concatenate([windows(b, 2) for b in halves[j][1]])
            row.append(f"{costs(train, test, 2)[1]:.2f}")
        lines.append(f"| {k} | " + " | ".join(row) + " |")
    (out / "counting-the-cards.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("folder")
    ap.add_argument("--out", default="shortcut-tiny")
    ap.add_argument("--processes", type=int, default=os.cpu_count() or 1)
    ap.add_argument("--only", choices=["1", "2"])
    a = ap.parse_args()
    out = Path(a.out)
    out.mkdir(parents=True, exist_ok=True)
    if a.only != "1":
        part2(a.folder, out)
    if a.only != "2":
        part1(out, a.processes)


if __name__ == "__main__":
    main()
