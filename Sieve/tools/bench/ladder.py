#!/usr/bin/env python3
r"""Where Sieve starts to win: each file given, cut to its first 1 KB, 4 KB, 16 KB and so on, and
each cut packed on its own by Sieve and by 7z, sizes side by side (HANDOFF's plan, step 2).

  python tools/bench/ladder.py FILE [FILE ...] --sieve PATH\TO\sieve.exe [--sizes 1K,4K,16K,64K,256K,1M]
                                [--decoding] [--out results\ladder-NAME.md]

A cut is the file's first N bytes, written as a file of its own under the original's name, so a
`.sieve` carries the same name at every size. Sieve: `sieve locate CUT --installer X.sieve --items`
(with --decoding, Prioritise Decoding Speed as well, `--prioritise-decoding`), installed again and
checked byte for byte. 7z: 7-Zip's own program at -mx9 where it is installed, else py7zr (LZMA2
preset 9 extreme). A size larger than the file is left out. Pick a text, a picture and a program,
so the table shows where each kind turns. Nothing here is Sieve's own measure of itself.
"""

import argparse
import lzma
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from compare import program  # noqa: E402  (7-Zip's program, where it is installed)


def size_of(text):
    text = text.strip().upper()
    for suffix, mult in (("K", 1 << 10), ("M", 1 << 20), ("G", 1 << 30)):
        if text.endswith(suffix):
            return int(float(text[:-1]) * mult)
    return int(text)


def label(n):
    for suffix, mult in (("MB", 1 << 20), ("KB", 1 << 10)):
        if n >= mult and n % mult == 0:
            return f"{n // mult} {suffix}"
    return f"{n:,} bytes"


def seven_zip_size(path, work):
    arc = work / "cut.7z"
    arc.unlink(missing_ok=True)
    seven = program("7z", "7zz", "7za")
    if seven:
        subprocess.run([seven, "a", "-mx9", "-bd", "-bso0", str(arc), str(path)], check=True)
        return arc.stat().st_size, "7-Zip -mx9"
    import py7zr  # (pip install py7zr where 7-Zip is not installed)

    with py7zr.SevenZipFile(arc, "w", filters=[{"id": py7zr.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]) as z:
        z.write(path, path.name)
    return arc.stat().st_size, "py7zr, LZMA2 9 extreme"


def sieve_size(sieve, path, work, decoding):
    made = work / "cut.sieve"
    made.unlink(missing_ok=True)
    args = [str(sieve), "locate", str(path), "--installer", str(made), "--items"]
    if decoding:
        args.append("--prioritise-decoding")
    t0 = time.perf_counter()
    r = subprocess.run(args, capture_output=True, text=True)
    took = time.perf_counter() - t0
    if r.returncode != 0:
        sys.exit("sieve locate failed:\n" + r.stdout + r.stderr)
    back = work / "back"
    r = subprocess.run([str(sieve), "install", str(made), "--to", str(back), "--force"], capture_output=True, text=True)
    if r.returncode != 0 or (back / path.name).read_bytes() != path.read_bytes():
        sys.exit("sieve install did not give the cut back:\n" + r.stdout + r.stderr)
    return made.stat().st_size, took


def main():
    ap = argparse.ArgumentParser(description="Where Sieve starts to win: files cut to growing sizes, Sieve against 7z.")
    ap.add_argument("files", type=Path, nargs="+")
    ap.add_argument("--sieve", type=Path, required=True, help="the sieve program (sieve.exe)")
    ap.add_argument("--sizes", default="1K,4K,16K,64K,256K,1M", help="the cuts, comma separated (K, M and G allowed)")
    ap.add_argument("--decoding", action="store_true", help="Sieve with Prioritise Decoding Speed (no mixing)")
    ap.add_argument("--out", type=Path, help="also write the table here (Markdown)")
    a = ap.parse_args()
    sizes = [size_of(s) for s in a.sizes.split(",") if s.strip()]
    sieve = a.sieve.resolve()
    lines = ["| File | Cut | 7z | Sieve | Against 7z | Sieve's time |", "| :--- | ---: | ---: | ---: | ---: | ---: |"]
    how7 = ""
    with tempfile.TemporaryDirectory(prefix="sieve-ladder-") as tmp:
        work = Path(tmp)
        for f in a.files:
            data = f.read_bytes()
            for n in [s for s in sizes if s <= len(data)] + ([len(data)] if len(data) not in sizes else []):
                cut_dir = work / "cut"
                cut_dir.mkdir(exist_ok=True)
                cut = cut_dir / f.name
                cut.write_bytes(data[:n])
                z, how7 = seven_zip_size(cut, work)
                s, took = sieve_size(sieve, cut, work, a.decoding)
                against = (s - z) / z * 100
                cut_label = label(n) + (" (whole)" if n == len(data) else "")
                print(f"{f.name:28} {cut_label:>18} 7z {z:>11,}  sieve {s:>11,}  {against:+7.2f}%  {took:7.1f} s", flush=True)
                lines.append(f"| {f.name} | {cut_label} | {z:,} | {s:,} | {against:+.2f}% | {took:.1f} s |")
    note = (f"\n7z: {how7}. Sieve: `sieve locate --installer --items`"
            + (" `--prioritise-decoding`" if a.decoding else "") + f" ({sieve.name}), each installed back and checked.\n")
    if a.out:
        a.out.write_text("\n".join(lines) + "\n" + note, encoding="utf-8")
        print(f"written {a.out}")


if __name__ == "__main__":
    main()
