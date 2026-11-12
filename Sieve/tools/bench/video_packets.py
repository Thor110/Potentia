#!/usr/bin/env python3
"""How much of a set of films is packets seen before (IDEAS 19.116's first measurement).

A film already compressed codes most frames as differences from others, so its frames are no help
to Sieve; but films encoded together (a film and its trailer, two cuts, a game's films sharing an
intro) can hold the very same packets, byte for byte. ffprobe lists every packet with a SHA-256 of
its data without decoding a frame, so counting the bytes of packets already seen (in the same film
or an earlier one) says whether carrying each packet once is worth a way of its own.

Usage:
  python tools/bench/video_packets.py FILM [FILM ...]     (or a folder: every file in it)

Prints, for each film and for the whole set: its bytes, its packets' bytes, and the bytes of
packets seen before, by stream kind. Needs ffprobe on the PATH. Reads only; changes nothing.
"""
import collections
import json
import pathlib
import subprocess
import sys


def packets(path):
    """Every packet of a file: (stream kind, size, sha256), in file order."""
    out = subprocess.run(["ffprobe", "-v", "error", "-show_packets", "-show_data_hash", "sha256",
                          "-show_entries", "packet=codec_type,size,data_hash", "-of", "json", str(path)],
                         capture_output=True, text=True, check=True).stdout
    return [(p.get("codec_type", "?"), int(p["size"]), p.get("data_hash", "")) for p in json.loads(out).get("packets", [])]


def main():
    files = []
    for a in sys.argv[1:]:
        p = pathlib.Path(a)
        files += sorted(q for q in p.rglob("*") if q.is_file()) if p.is_dir() else [p]
    if not files:
        sys.exit(__doc__)
    seen = set()
    total = collections.Counter()
    for f in files:
        try:
            ps = packets(f)
        except subprocess.CalledProcessError:
            print(f"{f.name}: not read by ffprobe, skipped")
            continue
        size = f.stat().st_size
        mine = collections.Counter()
        for kind, n, h in ps:
            mine[kind + " bytes"] += n
            if h and h in seen:
                mine[kind + " seen"] += n
            seen.add(h)
        pk = sum(v for k, v in mine.items() if k.endswith(" bytes"))
        again = sum(v for k, v in mine.items() if k.endswith(" seen"))
        print(f"{f.name}: {size:,} bytes, {len(ps):,} packets of {pk:,} bytes; seen before {again:,} ({100 * again / max(size, 1):.2f}% of the file)")
        total.update(mine)
        total["files bytes"] += size
    pk = sum(v for k, v in total.items() if k.endswith(" bytes") and not k.startswith("files"))
    again = sum(v for k, v in total.items() if k.endswith(" seen"))
    print(f"all: {total['files bytes']:,} bytes, packets {pk:,}; seen before {again:,} ({100 * again / max(total['files bytes'], 1):.2f}%)")
    for kind in sorted({k.split()[0] for k in total if k.endswith(" bytes") and not k.startswith("files")}):
        print(f"  {kind}: {total[kind + ' bytes']:,} bytes, seen before {total[kind + ' seen']:,}")


if __name__ == "__main__":
    main()
