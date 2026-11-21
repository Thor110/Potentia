#!/usr/bin/env python3
r"""What a `.sieve` was made of, read back from the `.sieve` alone (Edward, 1 November 2026: "a
script that can log that from an existing .sieve ... That way we can reverse engineer the results if
they ever get lost"; IDEAS §19.111).

  python tools/sieve_read.py FILE.sieve             light: its head and listing only
  python tools/sieve_read.py FILE.sieve --full SIEVE  full: installed too, each file measured

Light reads the head and the listing (the listing is LZMA2 but small), through the oracle
(reference/sieve_ref.py, which reads every version), and decodes no stream: each file's way and
size, the ways tallied (files and bytes), each stream's size and how it is carried, and each code a
file has of its own (the rope's stream holds each mixed file's, a group's and the rope's codes, each
after its length), read by its length.

Full installs it as well (`sieve install`, into a folder of its own that is then removed) and packs
each file carried in a shared stream alone by LZMA2 (xz -9e, raw): what each would have cost on its
own. That is an estimate of its share, not the record: the stream was made as one.
"""

import argparse
import lzma
import subprocess
import sys
import tempfile
from collections import OrderedDict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent / "reference"))
import sieve_ref as ref  # noqa: E402

# The inner ways whose code is the file's own, next in the rope stream after its length (mixt: the
# group's code, with its first file).
OWN_CODE = {"rope", "pieces", "mix4", "mixt", "mix3", "mix2", "mix", "runs"}
# The words a recipe starts with (what a file is made again from on installing, around its inner
# way): the oracle's list, and lz, which ways_tally.py has named too.
RECIPES = set(ref.RECIPE_WORDS) | {"lz"}
WAY_NAMES = OrderedDict([
    ("raw", "LZMA2 (xz/7-Zip's), in the lzma2 stream"), ("x86", "LZMA2 with the x86 filter (xz/7-Zip's)"),
    ("mixt", "mixing (Sieve's mix-v4), a group's first file"), ("mixn", "mixing, in that group"),
    ("mix4", "mixing (Sieve's mix-v4), alone"), ("mix3", "mixing (mix-v3)"), ("mix2", "mixing (mix-v2)"), ("mix", "mixing (mix-v1)"),
    ("rope", "the rope (Sieve's)"), ("pieces", "pieces (LZMA2 and the rope by turns)"),
    ("lines", "the lines of another file (Sieve's)"), ("delta", "its differences from another (Sieve's delta-v1)"),
    ("item", "an item of Sieve's lines"), ("runs", "runs, patterns and ramps (Sieve's)"),
    ("copy", "a copy of another file (Sieve's)"),
])


def varint(b, at):
    v = shift = 0
    while True:
        c = b[at]
        at += 1
        v |= (c & 0x7F) << shift
        shift += 7
        if c < 0x80:
            return v, at


def read(path):
    raw = ref.instructions_bytes(Path(path).read_bytes())
    data = ref.v16_text(raw)
    end = data.index(b"\nend\n") + len(b"\nend\n")
    lines = data[:end].decode("utf-8", "replace").split("\n")[:-1]
    payload = data[end:]
    streams, files, folders, remade = [], [], 0, []
    for ln in lines[1:]:
        if ln.startswith("stream "):
            parts = ln.split(" ")
            streams.append({"name": parts[1], "packed": int(parts[2]), "bytes": int(parts[3]), "how": " ".join(parts[4:])})
        elif ln.startswith("f\t"):
            f = ln.split("\t")
            files.append({"size": int(f[1]), "way": f[-2], "path": f[-1], "fields": f})
        elif ln.startswith("d\t"):
            folders += 1
        elif ln.startswith(("r ", "u ", "a ")):
            remade.append(ln)
    return {"version": lines[0], "lines": lines, "streams": streams, "files": files, "folders": folders, "remade": remade, "payload": payload, "size": len(raw)}


def split_way(way):
    """A way taken apart: its recipe (or None), the members it counts (skm, spn), and the inner way's
    words. The inner way is the first word after the recipe that names a way."""
    t = way.split()
    if not t:
        return None, 0, ["?"]
    if t[0] in RECIPES:
        for j in range(1, len(t)):
            if t[j] in WAY_NAMES and t[j] != "item":
                members = int(t[1]) if t[0] in ("skm", "spn") and t[1].isdigit() else 0
                return t[0], members, t[j:]
        return t[0], 0, ["?"]
    return None, 0, t


def own_codes(info):
    """Each file's own code's length, read from the rope stream by the lengths before each code; and
    whether those lengths and codes make the stream exactly (the check that they were read right)."""
    at, out = 0, {}
    for s in info["streams"]:
        if s["name"] == "rope":
            blob = info["payload"][at:at + s["packed"]]
            if s["packed"] != s["bytes"]:
                return out, True  # (a rope stream carried packed: not read here)
            pos = 0
            for f in info["files"]:
                if split_way(f["way"])[2][0] in OWN_CODE:
                    if pos >= len(blob):
                        return out, False
                    try:
                        n, pos = varint(blob, pos)
                    except IndexError:
                        return out, False
                    out[f["path"]] = n
                    pos += n
            return out, pos == len(blob)
        at += s["packed"]
    return out, not any(split_way(f["way"])[2][0] in OWN_CODE for f in info["files"])


def way_kind(way):
    return way.split(" ", 1)[0]


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve_file")
    ap.add_argument("--full", metavar="SIEVE", help="install it with this sieve and measure each file")
    a = ap.parse_args()
    info = read(a.sieve_file)
    codes, whole = own_codes(info)
    print(f"{a.sieve_file}: {info['size']:,} bytes, {info['version']}; {len(info['files'])} file(s), {info['folders']} folder(s)")
    if not whole:
        print("warning: the own codes read do not make the rope stream exactly, so their lengths below are not to be trusted")
    print("\nStreams (packed, holding, how):")
    for s in info["streams"]:
        print(f"  {s['name']:6} {s['packed']:>12,} {s['bytes']:>12,}  {s['how']}")
    tally = OrderedDict()
    for f in info["files"]:
        k = way_kind(f["way"])
        t = tally.setdefault(k, [0, 0, 0])
        t[0] += 1
        t[1] += f["size"]
        t[2] += codes.get(f["path"], 0)
    print("\nWays (files, their bytes, their own codes where they have them):")
    for k, (n, b, c) in tally.items():
        print(f"  {k:7} {n:>6} {b:>14,}  {(f'{c:,}' if c else ''):>12}  {WAY_NAMES.get(k, '')}")
    if info["remade"]:
        print(f"\nMade again on installing: {len(info['remade'])} record(s) (archives, compressed files, pictures)")
    print("\nFiles (size, way, own code):")
    for f in info["files"]:
        c = codes.get(f["path"])
        print(f"  {f['size']:>12,}  {f['way'][:48]:48}  {(f'{c:,}' if c is not None else ''):>10}  {f['path']}")
    if a.full:
        with tempfile.TemporaryDirectory() as tmp:
            subprocess.run([a.full, "install", a.sieve_file, "--to", tmp], check=True, stdout=subprocess.DEVNULL)
            print("\nFull: each file in a shared stream packed alone by LZMA2 (xz -9e, raw), an estimate of its share:")
            for f in info["files"]:
                if way_kind(f["way"]) in ("raw", "x86", "mixn", "mixt"):
                    p = Path(tmp) / info["lines"][1][5:] / f["path"] if info["lines"][1].startswith("root ") else Path(tmp) / f["path"]
                    if not p.exists():
                        p = next(Path(tmp).rglob(Path(f["path"]).name), None)
                    if p and p.is_file():
                        x = p.read_bytes()
                        z = len(lzma.compress(x, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]))
                        print(f"  {len(x):>12,} -> {z:>10,}  {f['path']}")


if __name__ == "__main__":
    main()
