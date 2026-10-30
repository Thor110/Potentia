"""Designed test files for the rope (IDEAS §19.108), and how far each can be pushed.

Edward (30 October 2026): a file made by propagating noise in the first place, "diagonal lines of the
same values for the perfect designed test case", then "test how far we can push that test file
before testing it in the real world". The first piece of the benchmark script and test files he
asked for (FORMATS.md, the long run).

Every file is made here from a fixed seed, so none is kept in the repository and every figure can be
made again. Each is measured four ways:
  - xz -9e (liblzma, by Python's lzma), the general compressor to beat;
  - the rope's search alone (`sieve rope FILE --seconds S`): its best wave, and its code's bytes;
  - the whole `.sieve` (`sieve locate FILE --installer`), and the way Sieve took for it;
  - its order-0 entropy, what a coder of each byte alone would need.

  python tools/rope_designs.py SIEVE [--seconds 5] [--out FOLDER] [--only NAME,...]

Writes the files and `results.md` (a table) into FOLDER (default: rope-designs, here).
"""

import argparse
import lzma
import math
import os
import random
import re
import subprocess
import sys

W, H = 256, 4096  # each file 1 MiB: 4,096 rows of 256 bytes


def rows(make):
    return b"".join(bytes(make(r)) for r in range(H))


def designs():
    """name -> (what it is, its bytes). Each from its own fixed seed."""
    d = {}
    d["checker"] = ("00 and FF along the diagonals (Edward's example)",
                    rows(lambda r: [0x00 if (c + r) % 2 == 0 else 0xFF for c in range(W)]))
    rng = random.Random(1)
    first = [rng.randrange(256) for _ in range(W)]
    d["diagonal"] = ("a random first row carried down the diagonals (each row the one above, moved one along)",
                     rows(lambda r: first[-(r % W):] + first[:-(r % W)] if r % W else first))
    d["vertical"] = ("a random first row carried straight down (each row the one above)", rows(lambda r: first))
    d["gradient"] = ("each byte its row plus its column (a smooth slope)", rows(lambda r: [(r + c) % 256 for c in range(W)]))
    rng = random.Random(2)
    diag = bytearray(d["diagonal"][1])
    for i in range(len(diag)):
        if rng.random() < 0.10:
            diag[i] = (diag[i] + rng.choice((-1, 1))) % 256
    d["diagonal+noise"] = ("the diagonal file with one byte in ten moved by one", bytes(diag))
    rng = random.Random(3)
    first250 = [rng.randrange(256) for _ in range(250)]
    d["diagonal-250"] = ("the diagonal carry in rows of 250 bytes (a width that does not divide 1 MiB)",
                         b"".join(bytes(first250[-(r % 250):] + first250[:-(r % 250)] if r % 250 else first250)
                                  for r in range(len(d["diagonal"][1]) // 250 + 1))[: W * H])
    half = H // 2
    rng = random.Random(4)
    a = [rng.randrange(256) for _ in range(W)]
    top = [a[-(r % W):] + a[:-(r % W)] if r % W else a for r in range(half)]
    bottom = [a[(r % W):] + a[:(r % W)] for r in range(H - half)]
    d["two-directions"] = ("the top half carried down-right, the bottom half down-left", b"".join(bytes(x) for x in top + bottom))
    rng = random.Random(5)
    d["noise"] = ("random bytes (the control: nothing can be taken off)", bytes(rng.randrange(256) for _ in range(W * H)))
    return d


def entropy0(b):
    counts = [0] * 256
    for x in b:
        counts[x] += 1
    n = len(b)
    return -sum(c / n * math.log2(c / n) for c in counts if c) * n / 8


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True).stdout


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve")
    ap.add_argument("--seconds", type=float, default=5)
    ap.add_argument("--out", default="rope-designs")
    ap.add_argument("--only", default="")
    a = ap.parse_args()
    os.makedirs(a.out, exist_ok=True)
    only = set(x for x in a.only.split(",") if x)
    rows_out = []
    for name, (what, data) in designs().items():
        if only and name not in only:
            continue
        path = os.path.join(a.out, name + ".bin")
        open(path, "wb").write(data)
        xz = len(lzma.compress(data, preset=9 | lzma.PRESET_EXTREME))
        rope = run([a.sieve, "rope", path, "--seconds", str(a.seconds)])
        wave = re.search(r"^wave\s+(.*?) \(", rope, re.M)
        code = re.search(r"^code\s+(\d+) bytes", rope, re.M)
        sv = os.path.join(a.out, name + ".sieve")
        rep = run([a.sieve, "locate", path, "--installer", sv])
        taken = re.search(r"taken: ([a-z0-9]+), (\d+) bytes", rep)
        rows_out.append((name, what, len(data), round(entropy0(data)), xz, int(code.group(1)) if code else None,
                         wave.group(1) if wave else "", os.path.getsize(sv) if os.path.exists(sv) else None,
                         taken.group(1) if taken else ""))
        print(f"{name}: xz {xz}, rope {rows_out[-1][5]} ({rows_out[-1][6]}), .sieve {rows_out[-1][7]}", flush=True)
    with open(os.path.join(a.out, "results.md"), "w") as f:
        f.write(f"# The rope's designed files\n\nMade by `tools/rope_designs.py`, the rope searching {a.seconds:g} s a file.\n\n")
        f.write("| File | What it is | Bytes | Order-0 entropy | xz -9e | The rope | Its best wave | `.sieve` |\n")
        f.write("| :--- | :--- | ---: | ---: | ---: | ---: | :--- | ---: |\n")
        for r in rows_out:
            f.write(f"| {r[0]} | {r[1]} | {r[2]:,} | {r[3]:,} | {r[4]:,} | {r[5]:,} | `{r[6]}` | {r[7]:,} |\n")
    print(f"wrote {os.path.join(a.out, 'results.md')}")


if __name__ == "__main__":
    sys.exit(main())
