#!/usr/bin/env python3
"""Web dictionary measurement (3 November 2026).

Learns dictionaries (zstd's trainer) from the learning sets, then weighs each test page:
brotli (quality 11, the default window), LZMA2 alone and after the dictionary, Sieve's mixing
(mix-v4, B20 F80) alone and primed on the dictionary. "After the dictionary" is measured as
the cost of dictionary + page less the cost of the dictionary alone, the same model carrying on:
for mixing that is exactly what a primed model would code; for LZMA2 it stands in for a preset
dictionary (within a few bytes).

usage: measure_web_dictionary.py WEB_DIR SIEVE_BIN OUT_DIR

WEB_DIR holds the MDN repositories as mdn-* and the npm packages under npm/ (results/
web-dictionary-2026-11-03.md lists them); the pages tested from the repository are its own.
"""
import concurrent.futures as cf
import hashlib
import json
import lzma
import os
import pathlib
import subprocess
import sys
import tempfile

import brotli
import zstandard

web, sieve, out = map(pathlib.Path, sys.argv[1:4])
out.mkdir(parents=True, exist_ok=True)
CHUNK = 64 * 1024
SITE = pathlib.Path(__file__).resolve().parents[2]  # the repository's root: the site's pages


def files_under(root, exts, skip=()):
    for p in sorted(root.rglob("*")):
        if p.is_file() and p.suffix.lower() in exts and ".git" not in p.parts and not any(s in str(p) for s in skip):
            yield p


def samples(paths):
    seen, got = set(), []
    for p in paths:
        b = p.read_bytes()
        h = hashlib.sha256(b).digest()
        if h in seen or len(b) < 64:
            continue
        seen.add(h)
        got += [b[i:i + CHUNK] for i in range(0, len(b), CHUNK)]
    return got


WEB_EXT = {".html", ".htm", ".css", ".js", ".mjs"}
mdn_learn = [p for d in sorted(web.glob("mdn-*")) if d.name != "mdn-learning-area" for p in files_under(d, WEB_EXT)]
libs = [p for d in sorted((web / "npm").iterdir()) if d.name != "htmlparser-benchmark" for p in files_under(d, WEB_EXT, skip=(".map",))]
sets = {"mdn": samples(mdn_learn), "libs": samples(libs)}
sets["both"] = sets["mdn"] + sets["libs"]
for k, v in sets.items():
    print(f"learning set {k}: {len(v)} samples, {sum(map(len, v)):,} bytes", flush=True)

dicts = {}
for k, v in sets.items():
    for size in (65536, 131072):
        d = zstandard.train_dictionary(size, v, threads=-1).as_bytes()
        name = f"{k}-{size // 1024}k"
        (out / f"{name}.dict").write_bytes(d)
        dicts[name] = d
        print(f"dictionary {name}: {len(d):,} bytes", flush=True)

tests = {
    "2014": sorted((web / "npm/htmlparser-benchmark/package/files").glob("*.html")),
    "edward": sorted(SITE.glob("index.html")) + sorted((SITE / "Pages").rglob("*.html")),
    "mdn-la": list(files_under(web / "mdn-learning-area", {".html"})),
}


def lz(b):
    return len(lzma.compress(b, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]))


def mix(b):
    with tempfile.TemporaryDirectory() as t:
        f, o = pathlib.Path(t, "f"), pathlib.Path(t, "o")
        f.write_bytes(b)
        subprocess.run([str(sieve), "mix", str(f), "--words", "80", "--by-use", "--params", "B20 F80", "--out", str(o)],
                       check=True, stdout=subprocess.DEVNULL, cwd=sieve.parent)
        return o.stat().st_size


base = {name: (lz(d), mix(d)) for name, d in dicts.items()}
print("dictionaries alone (LZMA2, mixing):", base, flush=True)


def weigh(job):
    group, path = job
    b = path.read_bytes()
    r = {"group": group, "page": str(path), "size": len(b), "brotli": len(brotli.compress(b, quality=11)), "lzma2": lz(b), "mix": mix(b)}
    for name, d in dicts.items():
        r[f"lzma2+{name}"] = lz(d + b) - base[name][0]
        r[f"mix+{name}"] = mix(d + b) - base[name][1]
    return r


jobs = [(g, p) for g, ps in tests.items() for p in ps if p.stat().st_size > 0]
rows = []
with cf.ThreadPoolExecutor(os.cpu_count()) as ex:
    for i, r in enumerate(ex.map(weigh, jobs)):
        rows.append(r)
        if i % 25 == 0:
            print(f"{i + 1}/{len(jobs)}", flush=True)
(out / "rows.json").write_text(json.dumps(rows, indent=0))

cols = ["brotli", "lzma2", "mix"] + [f"{w}+{n}" for w in ("lzma2", "mix") for n in dicts]
for g in tests:
    rs = [r for r in rows if r["group"] == g]
    print(f"\n{g}: {len(rs)} pages, {sum(r['size'] for r in rs):,} bytes")
    for c in cols:
        tot = sum(r[c] for r in rs)
        wins = sum(r[c] < r["brotli"] for r in rs)
        print(f"  {c:18} {tot:>12,}  {100 * tot / sum(r['brotli'] for r in rs) - 100:+6.1f}% vs brotli, smaller on {wins}")
