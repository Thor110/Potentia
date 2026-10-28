#!/usr/bin/env python3
r"""How long making Sieve instructions takes, by stage and by kind of file (IDEAS §19.97, §19.100).

Builds a fixed set of files from this repository's own (so every machine measures the same bytes),
makes each into Sieve instructions with `sieve locate FILE --installer` (the newest, as the File
Locator makes them), and reads `--timings` for the packer's stages: the rope's search and code,
cutting into pieces, LZMA2's quick estimates, settings trial and full passes, 7-Zip's pass, and the
archives' recipes. Writes a table per run to results/:

  tools\bench\bench_make.bat                                   (Windows: finds Python, uses the Release build)
  python tools/bench/bench_make.py SIEVE                       the quick set, 1 s of search a file
  python tools/bench/bench_make.py SIEVE --set full            and the large files (a 52 MB video)
  python tools/bench/bench_make.py SIEVE --search 0.5,2,10     several search times

Results: results/bench-make-<set>-<date>.tsv (every figure) and .md (a table). The time is the
command's own, from --timings (`sieve locate`), so starting the program is left out.
"""

import argparse
import datetime
import io
import math
import os
import platform
import re
import shutil
import struct
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
STAGES = ["pack.rope.search", "pack.pieces.rope", "pack.pieces.cut", "pack.pieces.blocks", "pack.pieces.boundaries", "pack.rope.code", "pack.lzma2.quick",
          "pack.lzma2.settings-trial", "pack.lzma2.first-pass", "pack.lzma2.tuned-pass", "pack.lzma2.7zip-pass",
          "pack.archives.recipe", "pack.mix"]
SHORT = {"pack.rope.search": "rope search", "pack.pieces.rope": "pieces' rope", "pack.pieces.cut": "cutting", "pack.pieces.blocks": "blocks", "pack.pieces.boundaries": "boundaries",
         "pack.rope.code": "rope code", "pack.lzma2.quick": "LZMA2 quick", "pack.lzma2.settings-trial": "LZMA2 trial",
         "pack.lzma2.first-pass": "LZMA2 pass 1", "pack.lzma2.tuned-pass": "LZMA2 tuned", "pack.lzma2.7zip-pass": "7-Zip pass",
         "pack.archives.recipe": "archives", "pack.mix": "mixing"}

# The set: (name in the set, kind, where in the repository, or how it is made).
QUICK = [
    ("prose.md", "prose", "docs/IDEAS.md"),
    ("code.cpp", "code", "tools/sieve_cli.cpp"),
    ("oracle.py", "code", "reference/sieve_ref.py"),
    ("table.csv", "table", "results/m1_sieve_scowl80.csv"),
    ("words.txt", "word list", "data/dictionaries/scowl-2020.12.07-en-80-names.txt"),
    ("rope.txt", "prose", "data/anchors/edward-james-gordon-thor110/Rope Around The Clock.txt"),
    ("screenshot.png", "picture (PNG)", "docs/images/hallway-edge.png"),
    ("vitruvian.jpg", "picture (JPEG)", "data/anchors/public-domain/Vitruvian.jpg"),
    ("door.jpg", "picture (JPEG)", "web/test-site/src/pictures/door.jpg"),
    ("spitfire.obj", "mesh", "data/anchors/edward-james-gordon-thor110/Spitfire.obj"),
    ("hymn.mid", "melody (MIDI)", "data/anchors/public-domain/hurrian-hymn-6.mid"),
    ("film.mp4", "video (MP4)", "data/anchors/public-domain/Roundhay_Garden_Scene_(1888)_7fps.mp4"),
    ("sound.wav", "sound (WAV)", "made: 2 s of a 440 Hz sine, 16-bit mono 22,050 Hz"),
    ("program.exe", "program (Windows)", "data/anchors/edward-james-gordon-thor110/sieve-v0.13.0.exe"),
    ("folder.tar", "tar", "made: a tar of the files above but the program (mtime 0)"),
]
LARGE = [
    ("recording.mp4", "video (MP4, large)", "data/anchors/edward-james-gordon-thor110/2026-09-21 23-25-35.mp4"),
]


def make_wav(path):
    rate, n = 22050, 2 * 22050
    data = b"".join(struct.pack("<h", int(12000 * math.sin(2 * math.pi * 440 * i / rate))) for i in range(n))
    with open(path, "wb") as f:
        f.write(b"RIFF" + struct.pack("<I", 36 + len(data)) + b"WAVEfmt " + struct.pack("<IHHIIHH", 16, 1, 1, rate, rate * 2, 2, 16)
                + b"data" + struct.pack("<I", len(data)) + data)


OWN_ANCHORS = "data/anchors/edward-james-gordon-thor110/"


def build_set(which, into, without_own_anchors=False):
    into.mkdir(parents=True, exist_ok=True)
    files = []
    for name, kind, src in QUICK + (LARGE if which == "full" else []):
        if without_own_anchors and src.startswith(OWN_ANCHORS):
            continue  # (and so the tar holds none of them either)
        out = into / name
        if src.startswith("made: 2 s"):
            make_wav(out)
        elif src.startswith("made: a tar"):
            with tarfile.open(out, "w", format=tarfile.USTAR_FORMAT) as t:
                for f in files:
                    if f[1] == "program (Windows)":
                        continue
                    info = t.gettarinfo(str(into / f[0]), arcname=f[0])
                    info.mtime, info.uid, info.gid, info.uname, info.gname = 0, 0, 0, "", ""
                    with open(into / f[0], "rb") as b:
                        t.addfile(info, b)
        else:
            shutil.copyfile(ROOT / src, out)
        files.append((name, kind, src))
    return files


def run(sieve, file, out, search):
    r = subprocess.run([str(sieve), "locate", str(file), "--installer", str(out), "--search-per-file", str(search), "--timings"],
                       capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        sys.exit(f"sieve locate {file.name} failed:\n{r.stdout}{r.stderr}")
    phases = {}
    for line in (r.stdout + r.stderr).splitlines():
        m = re.match(r"^(\S[^ ]*(?: [^ \d][^ ]*)*)\s+(\d+)\s+([\d.]+)\s+([\d.]+)\s+([\d.]+)$", line)
        if m:
            phases[m.group(1)] = (int(m.group(2)), float(m.group(3)))
    found = re.findall(r"sieve-manifest-v\d+", r.stdout + r.stderr)
    version = found[-1] if found else "?"
    return phases, version


def machine():
    cpu = platform.processor() or platform.machine()
    try:
        for line in open("/proc/cpuinfo"):
            if line.startswith("model name"):
                cpu = line.split(":", 1)[1].strip()
                break
    except OSError:
        pass
    return f"{cpu}, {os.cpu_count()} threads, {platform.system()} {platform.release()}"


def main():
    ap = argparse.ArgumentParser(description="Time making Sieve instructions, by stage and kind of file.")
    ap.add_argument("sieve", type=Path)
    ap.add_argument("--set", choices=["quick", "full"], default="quick")
    ap.add_argument("--search", default="1", help="seconds of the rope's search a file, comma-separated (default 1)")
    ap.add_argument("--out", type=Path, default=ROOT / "results")
    ap.add_argument("--only", default="", help="only these files of the set (names, comma-separated)")
    ap.add_argument("--without-own-anchors", action="store_true",
                    help="leave out the files from Edward's own anchors folder (" + OWN_ANCHORS + "), the tar's copies too")
    a = ap.parse_args()
    sieve = a.sieve.resolve()
    searches = [float(s) for s in a.search.split(",")]
    rows = []
    date = datetime.date.today().isoformat()
    a.out.mkdir(parents=True, exist_ok=True)
    stem = a.out / f"bench-make-{a.set}-{date}"
    cols = ["search", "name", "kind", "bytes", "sieve_bytes", "version", "seconds", "mb_per_s"] + STAGES
    tsv = open(stem.with_suffix(".tsv"), "w", encoding="utf-8", newline="\n")  # a row as each file is done
    tsv.write(f"# sieve bench-make, {date}, {machine()}, {sieve.name}\n" + "\t".join(cols) + "\n")
    only = set(filter(None, a.only.split(",")))
    with tempfile.TemporaryDirectory() as tmp:
        files = build_set(a.set, Path(tmp) / "set", a.without_own_anchors)
        for search in searches:
            for name, kind, src in files:
                if only and name not in only:
                    continue
                f = Path(tmp) / "set" / name
                out = Path(tmp) / (name + ".sieve")
                phases, version = run(sieve, f, out, search)
                size, made = f.stat().st_size, out.stat().st_size
                total = phases.get("sieve locate", (1, 0.0))[1] / 1000
                stages = {s: phases.get(s, (0, 0.0))[1] / 1000 for s in STAGES}
                rows.append(dict(search=search, name=name, kind=kind, bytes=size, sieve_bytes=made, version=version, seconds=total,
                                 mb_per_s=size / 1e6 / total if total else 0, **stages))
                r = rows[-1]
                tsv.write("\t".join(f"{r[c]:.3f}" if isinstance(r[c], float) else str(r[c]) for c in cols) + "\n")
                tsv.flush()
                print(f"{name:16} {size:>11,} -> {made:>11,} bytes  {total:8.2f} s  (search {search:g} s)", flush=True)
    tsv.close()
    with open(stem.with_suffix(".md"), "w", encoding="utf-8", newline="\n") as m:
        m.write(f"# Making Sieve instructions: time by stage ({date})\n\n{machine()}. `sieve locate FILE --installer` "
                "(the newest instructions), each file alone; seconds, from `--timings`. Stages nest in the total, and the "
                "pieces' rope includes its own search; cutting includes the blocks and the boundaries.\n")
        for search in searches:
            m.write(f"\n## Search {search:g} s a file\n\n| File | Kind | Bytes | Made | s | MB/s | " + " | ".join(SHORT[s] for s in STAGES) + " |\n")
            m.write("| :--- | :--- | ---: | ---: | ---: | ---: | " + " | ".join("---:" for _ in STAGES) + " |\n")
            for r in rows:
                if r["search"] != search:
                    continue
                m.write(f"| {r['name']} | {r['kind']} | {r['bytes']:,} | {r['sieve_bytes']:,} | {r['seconds']:.2f} | {r['mb_per_s']:.2f} | "
                        + " | ".join(f"{r[s]:.2f}" if r[s] else "" for s in STAGES) + " |\n")
    print(f"\nwritten: {stem.with_suffix('.tsv')} and {stem.with_suffix('.md')}")


if __name__ == "__main__":
    main()
