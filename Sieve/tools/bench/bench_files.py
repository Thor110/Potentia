#!/usr/bin/env python3
r"""Every file of a folder made into Sieve instructions four ways, each installed again, for the
figures in Proof/STATISTICS.md (Edward, 1 November 2026: one run overnight, each time they need
updating).

  tools\bench_files.bat                         (Windows: asks for the folder, finds Python and sieve.exe)
  tools\bench_files_optimise_temporary.bat      the same with Compressed formats, temporary
  tools\bench_files_optimise_permanent.bat      the same with Compressed formats, permanent
  tools\bench_light.bat                         the light bench: decode/no and size/try alone
  python tools/bench/bench_files.py SIEVE FOLDER [--keep] [--compressed temporary|permanent] [--ways W,W]

The files of FOLDER (not its subfolders), largest first. Each is made four ways, with no search
limit (as the File Locator's figures are made):

  size/try     for size, Try every way            (--search-every-wave)
  size/no      for size, without it
  decode/try   Prioritise Decoding Speed, Try every way   (--prioritise-decoding --search-every-wave)
  decode/no    Prioritise Decoding Speed, without it

and each `.sieve` is installed again (`sieve install`), its file checked against the original.
--ways names the ones to make (the light bench: "decode/no,size/try", the two that differ; on
Edward's 153 JPEGs and on enwik9, decode/try came to decode/no's bytes and size/no to size/try's,
each taking longer, 2 November 2026).
With --compressed temporary, every run has the box ticked (`--compressed-temporary`: an archive,
compressed file, PNG or JPEG made again exactly on installing), so the check is the same. With
--compressed permanent (`--compressed-permanent`: installed as what it holds, or a PNG or JPEG
re-saved smaller), what is installed is not the file's bytes by design: the check is then that
installing succeeded, Sieve itself having checked every file it wrote against its SHA-256. For
each it keeps the `.sieve`'s size, how long making it took (the command's own time, start to end),
and how long installing took (decoding, and checking and writing, as Sieve reports them). Beside
them, once a file: the zip and 7z figures the File Locator gives (deflate at level 9; LZMA2 at
preset 9e, one stream, no container), made here by Python's zlib and lzma.

Beside each run, what carried it: the packer's report lines that name a way (LZMA2 and the x86
filter, mixing alone or in groups, the rope, pieces, deltas, brunsli, the recipes that make archives
and pictures again), gathered into a "Carried by" column, and each line whole in results.tsv
(IDEAS §19.111: a record of which existing methods were used, how, and what finished the job).

Results go to a folder beside FOLDER, `<FOLDER>-bench-<date and time>` (with a box,
`<FOLDER>-bench-temporary-...` or `-permanent-...`, so its tables stay apart), outside the repository
(the files may be one's own): results.md (a table a file), results.tsv (every figure), and each
run's own output. Each table is written again after every run, so a run stopped part way keeps what
it finished. The `.sieve` files are deleted once measured, unless --keep.
"""

import argparse
import datetime
import hashlib
import lzma
import os
import platform
import re
import shutil
import subprocess
import sys
import time
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
try:
    import sieve_read  # (tools/sieve_read.py: each file's way read back from the .sieve)
except Exception:  # (the oracle not to be had: the column left empty)
    sieve_read = None

MODES = [
    ("size/try", ["--search-every-wave"]),
    ("size/no", []),
    ("decode/try", ["--prioritise-decoding", "--search-every-wave"]),
    ("decode/no", ["--prioritise-decoding"]),
]


def sha256_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def others(path):
    """The File Locator's comparison: zip's deflate at level 9, and LZMA2 at preset 9e as one
    stream, each without its container."""
    data = Path(path).read_bytes()
    z = zlib.compressobj(9, zlib.DEFLATED, -15)
    zipped = len(z.compress(data) + z.flush())
    seven = len(lzma.compress(data, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]))
    return zipped, seven


def run(cmd, log):
    t0 = time.perf_counter()
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    seconds = time.perf_counter() - t0
    out = p.stdout.decode("utf-8", "replace").replace("\r", "")
    with open(log, "w", encoding="utf-8") as f:
        f.write("$ " + " ".join(str(c) for c in cmd) + "\n" + out + f"\n(exit {p.returncode}, {seconds:.3f} s)\n")
    return p.returncode, out, seconds


def pct(a, b):
    return f"{100.0 * a / b:.2f}%" if b else ""


def against(a, b):
    return f"{100.0 * (a - b) / b:+.2f}%" if b else ""


# What carried each result (Edward, 1 November 2026: "recording how much of what existing compression
# methods were used, how they were used and whether or not the rope or mixed methods are what
# finished them off ... a compression engine that mixes and matches existing techniques"). Each of
# the packer's report lines, by the way it names, and the method with its maker.
CARRIERS = [
    ("packed", "LZMA2 (xz/7-Zip's), with the x86 filter for programs"),
    ("mix", "mixing (Sieve's mix-v4), alone"),
    ("groups", "mixing (Sieve's mix-v4), files together in groups"),
    ("stream", "mixing (Sieve's mix-v4) over the LZMA2 stream"),
    ("weighed", "files mixed weighed back into the LZMA2 stream (Sieve's)"),
    ("rope", "the rope (Sieve's)"),
    ("pieces", "pieces (Sieve's: LZMA2 and the rope by turns)"),
    ("deltas", "a file as its differences from another (Sieve's delta-v1)"),
    ("jpegs", "a JPEG as its brunsli form (Google's brunsli)"),
    ("optimised", "a JPEG's tables made for its values (Sieve's, after jpegtran)"),
    ("pngs", "a PNG re-saved at its smallest (zlib)"),
    ("remade", "archives and compressed files made again (their own encoders; preflate for deflate, stb_image's PNG)"),
    ("members", "archives made again from a skeleton and their files (Sieve's)"),
    ("unpacked", "archives carried as their contents (libarchive and Sieve's readers)"),
    ("items", "items of Sieve's lines (Sieve's)"),
]


def carried_by(said):
    """The report's lines that say what carried the result: {way: its line}."""
    out = {}
    for line in said.split("\n"):
        key = line.split(" ", 1)[0]
        if any(key == k for k, _ in CARRIERS) and line[len(key):len(key) + 2] == "  ":
            if key == "items" and "none;" in line:
                continue
            out[key] = line[10:].strip() if len(line) > 10 else ""
    return out


def carried_short(c):
    """One cell: each way that carried something, with its count of files and what it came to."""
    parts = []
    for key, _ in CARRIERS:
        if key not in c:
            continue
        line = c[key]
        if key == "packed":
            m = re.match(r"(\d+) bytes .*?(\d+) program\(s\) in the x86 stream \((\d+) bytes\), (\d+) file\(s\) as they are \((\d+) bytes\)", line)
            if m and (int(m.group(2)) or int(m.group(4))):
                parts.append(f"LZMA2 {m.group(4)} file(s){', x86 ' + m.group(2) if int(m.group(2)) else ''} in {int(m.group(1)):,}")
            continue
        n = re.match(r"(\d+)", line)
        into = re.search(r"carried in (\d+)", line) or re.search(r"as (\d+)[;)]", line)
        parts.append(f"{key} {n.group(1) if n else ''}" + (f" in {int(into.group(1)):,}" if into else ""))
    return "; ".join(parts)


def ways_in_full(made):
    """Each file's way as the .sieve's listing writes it (its method and every setting: "mix4 B22 I3x1
    R1 M4 C2", "rope L8 W949 S1 P3 K0", "raw"), and what made it again on installing, read back from
    the .sieve (Edward, 2 November 2026: "list the exact formulae used behind each end result")."""
    if sieve_read is None:
        return ""
    try:
        info = sieve_read.read(made)
        out = [f["way"] for f in info["files"]]
        out += [r.split(" ", 3)[2] if r.count(" ") >= 2 else r for r in info["remade"]]
        return "; ".join(dict.fromkeys(out))
    except (Exception, SystemExit):  # (the oracle refuses by sys.exit: the column left empty, the bench goes on)
        return ""


def write_tables(out, rows, files_info, started, sieve, box):
    with open(out / "results.tsv", "w", encoding="utf-8") as f:
        f.write("file\tbytes\tzip\t7z\tmode\tsieve\tmade (s)\tdecoded (s)\tchecked and written (s)\tinstalled the same\tcarried by\tways in full\t"
                + "\t".join(f"report: {k}" for k, _ in CARRIERS) + "\n")
        for r in rows:
            c = r.get("carried", {})
            f.write("\t".join(str(r[k]) for k in ("file", "bytes", "zip", "7z", "mode", "sieve", "made", "decoded", "written", "same"))
                    + "\t" + carried_short(c) + "\t" + r.get("ways", "") + "\t" + "\t".join(c.get(k, "") for k, _ in CARRIERS) + "\n")
    with open(out / "results.md", "w", encoding="utf-8") as f:
        title = {"": "", "temporary": ", Compressed formats temporary", "permanent": ", Compressed formats permanent"}[box]
        f.write(f"# Sieve on {len(files_info)} file(s), four ways each{title}\n\n")
        f.write(f"Made {started:%Y-%m-%d %H:%M} on {platform.node()} ({platform.system()} {platform.release()}, {os.cpu_count()} logical processors), "
                f"with {sieve}, no search limit"
                + {"": "", "temporary": ", the Compressed formats box temporary ticked (made again exactly on installing)",
                   "permanent": ", the Compressed formats box permanent ticked (installed as what it holds, or re-saved smaller: \"The same\" is then whether installing succeeded, Sieve checking what it wrote)"}[box]
                + ". zip is deflate at level 9 and 7z LZMA2 at preset 9e as one stream, each without its container "
                f"(as the File Locator gives them). \"Against 7z\" is the `.sieve` against that 7z.\n")
        for name, info in files_info.items():
            f.write(f"\n## {name}\n\n{info['bytes']:,} bytes; zip {info['zip']:,} ({pct(info['zip'], info['bytes'])}); 7z {info['7z']:,} ({pct(info['7z'], info['bytes'])})\n\n")
            f.write("| Way | `.sieve` | Of the file | Against 7z | Made in | Decoded in | Checked and written in | The same | Carried by | Ways in full |\n")
            f.write("| :--- | ---: | ---: | ---: | ---: | ---: | ---: | :--- | :--- | :--- |\n")
            for r in rows:
                if r["file"] != name:
                    continue
                if r["sieve"] == "":
                    f.write(f"| {r['mode']} | failed | | | {r['made']} s | | | | | |\n")
                    continue
                f.write(f"| {r['mode']} | {r['sieve']:,} | {pct(r['sieve'], r['bytes'])} | {against(r['sieve'], r['7z'])} | {r['made']} s | "
                        f"{r['decoded']} s | {r['written']} s | {r['same']} | {carried_short(r.get('carried', {}))} | `{r.get('ways', '')}` |\n")
        f.write("\n## The ways named in Carried by\n\n" + "\n".join(f"- `{k}`: {what}" for k, what in CARRIERS) + "\n")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve", help="sieve (sieve.exe on Windows)")
    ap.add_argument("folder", help="the folder whose files to measure")
    ap.add_argument("--keep", action="store_true", help="keep each .sieve made")
    ap.add_argument("--compressed", choices=["temporary", "permanent"], help="tick a Compressed formats box for every run")
    ap.add_argument("--ways", default=",".join(m[0] for m in MODES), type=lambda v: v.split(","),
                    help="the ways to make, of size/try, size/no, decode/try, decode/no (the light bench: decode/no,size/try)")
    a = ap.parse_args()
    if not a.ways or any(w not in [m[0] for m in MODES] for w in a.ways):
        sys.exit("--ways: of " + ", ".join(m[0] for m in MODES) + ", by commas")
    box = a.compressed or ""
    box_flags = {"": [], "temporary": ["--compressed-temporary"], "permanent": ["--compressed-permanent"]}[box]
    sieve = str(Path(a.sieve).resolve())
    folder = Path(a.folder).resolve()
    if not folder.is_dir():
        sys.exit(f"{folder} is not a folder")
    files = sorted((p for p in folder.iterdir() if p.is_file()), key=lambda p: (-p.stat().st_size, p.name))
    if not files:
        sys.exit(f"{folder} holds no files")
    started = datetime.datetime.now()
    out = folder.parent / f"{folder.name}-bench-{box + '-' if box else ''}{started:%Y%m%d-%H%M%S}"
    (out / "runs").mkdir(parents=True)
    print(f"{len(files)} file(s), largest first; results in {out}")
    rows, files_info = [], {}
    for n, path in enumerate(files, 1):
        size = path.stat().st_size
        print(f"\n[{n}/{len(files)}] {path.name}: {size:,} bytes")
        print("  zip and 7z (by zlib and lzma) ...", flush=True)
        zipped, seven = others(path)
        files_info[path.name] = {"bytes": size, "zip": zipped, "7z": seven}
        print(f"  zip {zipped:,}, 7z {seven:,}")
        original = sha256_of(path)
        for mode, flags in [m for m in MODES if m[0] in a.ways]:
            tag = mode.replace("/", "-")
            made = out / f"{path.name}.{tag}.sieve"
            print(f"  {mode}: making ...", flush=True)
            code, said, seconds = run([sieve, "locate", str(path), "--installer", str(made), "--search-per-file", "0", "--timings"] + flags + box_flags,
                                      out / "runs" / f"{path.name}.{tag}.make.txt")
            row = {"file": path.name, "bytes": size, "zip": zipped, "7z": seven, "mode": mode, "sieve": "", "made": f"{seconds:.1f}",
                   "decoded": "", "written": "", "same": ""}
            if code != 0 or not made.exists():
                print(f"  {mode}: making failed (exit {code}); its output is in runs/")
                rows.append(row)
                write_tables(out, rows, files_info, started, sieve, box)
                continue
            row["sieve"] = made.stat().st_size
            row["carried"] = carried_by(said)
            row["ways"] = ways_in_full(made)
            dest = out / "installed"
            shutil.rmtree(dest, ignore_errors=True)
            print(f"  {mode}: {row['sieve']:,} bytes in {seconds:.1f} s; installing ...", flush=True)
            code, said, _ = run([sieve, "install", str(made), "--to", str(dest)], out / "runs" / f"{path.name}.{tag}.install.txt")
            m = re.search(r"decoded in ([0-9.]+) s, checked and written in ([0-9.]+) s", said)
            if m:
                row["decoded"], row["written"] = m.group(1), m.group(2)
            got = dest / path.name
            if box == "permanent":
                # (installed as what it holds, or re-saved: not the file's bytes by design)
                row["same"] = ("yes" if got.is_file() and sha256_of(got) == original else "installed") if code == 0 else "NO"
            else:
                row["same"] = "yes" if code == 0 and got.exists() and sha256_of(got) == original else "NO"
            print(f"  {mode}: decoded in {row['decoded']} s, checked and written in {row['written']} s; the same: {row['same']}")
            shutil.rmtree(dest, ignore_errors=True)
            if not a.keep:
                made.unlink()
            rows.append(row)
            write_tables(out, rows, files_info, started, sieve, box)
    print(f"\nDone: {out / 'results.md'}")


if __name__ == "__main__":
    main()
