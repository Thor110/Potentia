#!/usr/bin/env python3
"""What the packer's rope probe sees in a file: the rope's search against LZMA2 on 256 KiB at
several places (3 November 2026; IDEAS §19.118).

usage: python tools/rope_probe.py SIEVE FILE [--places 0,0.25,0.5,0.75]

For each place (a fraction of the file: 0 its start, 0.5 its middle), 256 KiB from there are given to
`sieve rope` (the rope's own search, no time limit) and to LZMA2 (raw, preset 9 extreme, as xz and
7-Zip make it), and the rope's bytes over LZMA2's printed. The packer leaves the rope's search out
where the start comes to 1.1 or more (and, from 1.1 to 1.2, the middle too); under 1.0 the rope wins
that piece. Nothing is written but a temporary file, removed after.
"""
import argparse
import lzma
import os
import subprocess
import sys
import tempfile

ap = argparse.ArgumentParser()
ap.add_argument("sieve")
ap.add_argument("file")
ap.add_argument("--places", default="0,0.25,0.5,0.75")
a = ap.parse_args()

PIECE = 256 << 10
size = os.path.getsize(a.file)
if size < PIECE:
    sys.exit(f"{a.file} is under 256 KiB: the probe is for files over 512 KiB")
print(f"{a.file}: {size:,} bytes")
with open(a.file, "rb") as f, tempfile.TemporaryDirectory() as tmp:
    piece_path = os.path.join(tmp, "piece")
    for place in (float(p) for p in a.places.split(",")):
        at = min(int(size * place), size - PIECE)
        f.seek(at)
        piece = f.read(PIECE)
        with open(piece_path, "wb") as out:
            out.write(piece)
        lz = len(lzma.compress(piece, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]))
        r = subprocess.run([a.sieve, "rope", piece_path], capture_output=True, text=True, errors="replace")
        rope = next((int(l.split()[1]) for l in r.stdout.splitlines() if l.startswith("code ")), None)
        way = next((l.split(None, 1)[1].strip() for l in r.stdout.splitlines() if l.startswith("wave ")), "")
        if rope is None:
            print(f"  at {at:>13,} ({place:.2f}): sieve rope gave no code: {r.stderr.strip()[:200]}")
            continue
        print(f"  at {at:>13,} ({place:.2f}): rope {rope:>7,}, LZMA2 {lz:>7,}, {rope / lz:.2f} times ({way})")
