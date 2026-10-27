#!/usr/bin/env python3
r"""Sieve against the usual compressors, on one folder: each one's size, the ratio to the folder's
bytes, how long it took to make and to unpack, and a check that it gives the folder back.

  python tools/bench/compare.py FOLDER --sieve PATH\TO\sieve.exe [--out results\compare-NAME.md]

What it runs, each only where it can be had (anything missing is skipped and said so):
  - Sieve: `sieve locate FOLDER --installer X.sieve --items` (as make_release.py makes source.sieve),
    then `sieve install`, and the tree compared file for file;
  - 7-Zip's own program (7z, 7zz, or 7-Zip in Program Files) at -mx9, else py7zr (LZMA2 preset 9
    extreme, solid);
  - xz -9e, zstd 19 and zstd 22 with a long window, brotli 11 (their programs, else the Python
    packages zstandard and brotli), bzip2 -9, gzip -9, all over one tar of the folder;
  - zip -9 (Python's zipfile, deflate), file by file as most zips are made.
Python's own lzma, bz2, gzip and zipfile are always there. `pip install zstandard brotli py7zr` adds
the rest where their programs are not installed.

Times are wall-clock on this machine, as each tool uses it (Sieve and 7-Zip use several cores).
A tar's time is the compressor's alone; the tar is made once, beforehand. Sizes are what you would
hand someone. Nothing here is Sieve's own measure of itself: every method gets the same folder.
"""

import argparse
import bz2
import gzip
import hashlib
import io
import lzma
import os
import platform
import shutil
import subprocess
import sys
import tarfile
import tempfile
import time
import zipfile
from pathlib import Path


def tar_of(folder):
    # One tar, its order and metadata fixed, so every compressor reads the same bytes.
    buf = io.BytesIO()
    with tarfile.open(fileobj=buf, mode="w", format=tarfile.PAX_FORMAT) as t:
        for root, dirs, files in os.walk(folder):
            dirs.sort()
            for f in sorted(files):
                p = Path(root) / f
                info = t.gettarinfo(str(p), arcname=str(p.relative_to(folder.parent)).replace(os.sep, "/"))
                info.mtime, info.uid, info.gid, info.uname, info.gname = 0, 0, 0, "", ""
                with open(p, "rb") as fh:
                    t.addfile(info, fh)
    return buf.getvalue()


def tree_hashes(folder):
    out = {}
    for root, _, files in os.walk(folder):
        for f in files:
            p = Path(root) / f
            out[str(p.relative_to(folder)).replace(os.sep, "/")] = hashlib.sha256(p.read_bytes()).hexdigest()
    return out


def program(*names):
    for n in names:
        found = shutil.which(n)
        if found:
            return found
    if os.name == "nt":
        for base in (os.environ.get("ProgramFiles", ""), os.environ.get("ProgramFiles(x86)", "")):
            p = Path(base) / "7-Zip" / "7z.exe"
            if base and p.exists() and "7z" in names:
                return str(p)
    return None


def piped(args):
    return lambda b: subprocess.run(args, input=b, capture_output=True, check=True).stdout


def timed(f, *a, **k):
    t0 = time.perf_counter()
    r = f(*a, **k)
    return r, time.perf_counter() - t0


def main():
    ap = argparse.ArgumentParser(description="Sieve against the usual compressors, on one folder.")
    ap.add_argument("folder", type=Path)
    ap.add_argument("--sieve", type=Path, help="the sieve program (sieve.exe); without it, Sieve is skipped")
    ap.add_argument("--out", type=Path, help="also write the table here (Markdown)")
    a = ap.parse_args()
    folder = a.folder.resolve()
    files = [Path(r) / f for r, _, fs in os.walk(folder) for f in fs]
    total = sum(p.stat().st_size for p in files)
    rows, skipped = [], []

    def row(name, size, pack, unpack, how):
        rows.append((name, size, pack, unpack, how))
        print(f"{name:32} {size:>13,}  {total / size:6.2f}:1  {pack:8.1f} s  {unpack:7.2f} s", flush=True)

    print(f"{folder}: {len(files):,} files, {total:,} bytes", flush=True)
    work = Path(tempfile.mkdtemp(prefix="sieve-compare-"))
    try:
        # Sieve.
        if a.sieve:
            sieve = a.sieve.resolve()
            made = work / "folder.sieve"
            r, pack = timed(subprocess.run, [str(sieve), "locate", str(folder), "--installer", str(made), "--items"], capture_output=True, text=True)
            if r.returncode != 0:
                sys.exit("sieve locate failed:\n" + r.stdout + r.stderr)
            r, unpack = timed(subprocess.run, [str(sieve), "install", str(made), "--to", str(work / "sieve-out")], capture_output=True, text=True)
            if r.returncode != 0:
                sys.exit("sieve install failed:\n" + r.stdout + r.stderr)
            if tree_hashes(work / "sieve-out") != tree_hashes(folder):  # (install puts the folder's contents there)
                sys.exit("sieve install did not give the folder back")
            row("Sieve (.sieve)", made.stat().st_size, pack, unpack, f"sieve locate --installer --items ({sieve.name})")
        else:
            skipped.append("Sieve (no --sieve given)")

        # 7-Zip.
        seven = program("7z", "7zz", "7za")
        if seven:
            arc = work / "a.7z"
            _, pack = timed(subprocess.run, [seven, "a", "-mx9", "-bd", "-bso0", str(arc), str(folder)], check=True)
            _, unpack = timed(subprocess.run, [seven, "x", "-bd", "-bso0", f"-o{work / '7z-out'}", str(arc)], check=True)
            row("7z -mx9", arc.stat().st_size, pack, unpack, seven)
        else:
            try:
                import py7zr

                arc = work / "a.7z"

                def make():
                    with py7zr.SevenZipFile(arc, "w", filters=[{"id": py7zr.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]) as z:
                        z.writeall(folder, folder.name)

                def take():
                    with py7zr.SevenZipFile(arc, "r") as z:
                        z.extractall(work / "7z-out")

                _, pack = timed(make)
                _, unpack = timed(take)
                row("7z (LZMA2 9 extreme, solid)", arc.stat().st_size, pack, unpack, "py7zr")
            except ImportError:
                skipped.append("7z (no 7-Zip program, and no py7zr)")

        # The stream compressors, over one tar.
        raw = tar_of(folder)
        methods = []
        xz = program("xz")
        methods.append(("tar.xz (xz -9e)", piped([xz, "-9e", "-T1", "-c"]), piped([xz, "-dc"]), "xz") if xz else
                       ("tar.xz (xz -9e)", lambda b: lzma.compress(b, preset=9 | lzma.PRESET_EXTREME), lzma.decompress, "Python lzma"))
        zstd = program("zstd")
        if zstd:
            methods.append(("tar.zst (zstd -19)", piped([zstd, "-19", "-q", "-c"]), piped([zstd, "-dq", "-c"]), "zstd"))
            methods.append(("tar.zst (zstd --ultra -22 --long)", piped([zstd, "--ultra", "-22", "--long=27", "-q", "-c"]), piped([zstd, "-dq", "--long=27", "-c"]), "zstd"))
        else:
            try:
                import zstandard

                big = zstandard.ZstdCompressionParameters.from_level(22, window_log=27, enable_ldm=True)
                methods.append(("tar.zst (zstd 19)", lambda b: zstandard.ZstdCompressor(level=19).compress(b),
                                lambda c: zstandard.ZstdDecompressor().decompress(c, max_output_size=len(raw)), "zstandard"))
                methods.append(("tar.zst (zstd 22, long window)", lambda b: zstandard.ZstdCompressor(compression_params=big).compress(b),
                                lambda c: zstandard.ZstdDecompressor(max_window_size=1 << 27).decompress(c, max_output_size=len(raw)), "zstandard"))
            except ImportError:
                skipped.append("zstd (no zstd program, and no zstandard)")
        br = program("brotli")
        if br:
            methods.append(("tar.br (brotli -q 11 -w 24)", piped([br, "-q", "11", "-w", "24", "-c"]), piped([br, "-dc"]), "brotli"))
        else:
            try:
                import brotli

                methods.append(("tar.br (brotli 11, window 24)", lambda b: brotli.compress(b, quality=11, lgwin=24), brotli.decompress, "brotli (Python)"))
            except ImportError:
                skipped.append("brotli (no brotli program, and no brotli package)")
        methods.append(("tar.bz2 (bzip2 -9)", lambda b: bz2.compress(b, 9), bz2.decompress, "Python bz2"))
        methods.append(("tar.gz (gzip -9)", lambda b: gzip.compress(b, 9, mtime=0), gzip.decompress, "Python gzip"))
        for name, comp, decomp, how in methods:
            c, pack = timed(comp, raw)
            d, unpack = timed(decomp, c)
            if d != raw:
                sys.exit(f"{name} did not give the tar back")
            row(name, len(c), pack, unpack, how)

        # zip, file by file.
        arc = work / "a.zip"

        def make_zip():
            with zipfile.ZipFile(arc, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as z:
                for p in sorted(files):
                    z.write(p, str(p.relative_to(folder.parent)).replace(os.sep, "/"))

        def take_zip():
            with zipfile.ZipFile(arc) as z:
                z.extractall(work / "zip-out")

        _, pack = timed(make_zip)
        _, unpack = timed(take_zip)
        row("zip (deflate 9)", arc.stat().st_size, pack, unpack, "Python zipfile")
    finally:
        shutil.rmtree(work, ignore_errors=True)

    rows.sort(key=lambda r: r[1])
    table = [f"**{folder.name}**: {len(files):,} files, {total:,} bytes. "
             f"{platform.system()} {platform.machine()}, {os.cpu_count()} logical processors; `tools/bench/compare.py`, "
             f"{time.strftime('%d %B %Y')}.", "",
             "| Method | Bytes | Ratio | Of the folder | Make (s) | Unpack (s) |", "| :--- | ---: | ---: | ---: | ---: | ---: |"]
    for name, size, pack, unpack, how in rows:
        table.append(f"| {name} | {size:,} | {total / size:.2f}:1 | {100 * size / total:.2f}% | {pack:.1f} | {unpack:.2f} |")
    if skipped:
        table += ["", "Not run here: " + "; ".join(skipped) + "."]
    text = "\n".join(table) + "\n"
    print("\n" + text)
    if a.out:
        a.out.parent.mkdir(parents=True, exist_ok=True)
        a.out.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
