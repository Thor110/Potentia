#!/usr/bin/env python3
r"""The roof: how Sieve's time and memory grow with a file's size (Edward, 1 November 2026: "the roof
is around 100 MB to 1 GB by the looks of it"; docs/ODDITIES.md, the floor beside it).

  python tools/bench/bench_roof.py SIEVE BIGFILE [--sizes 1,4,16,64,256,1024] [--out FOLDER] [--ways size,decoding]
  tools\bench_roof.bat                    (Windows: asks for the file)

From BIGFILE (enwik9, or any large file of one kind) it takes its first 1, 4, 16, 64, 256 and 1,024
MiB (as far as the file goes), and each is made into Sieve instructions (`sieve locate`, no search
limit) for size and for decoding speed, then installed again (`sieve install`) and checked. For
each, measured: the `.sieve`'s bytes; the wall time and the processor time (user and system, every
thread) of making and of installing; and the most memory each took at once (Linux: the largest
child process's resident set, by the operating system's own count; Windows: the process's peak
working set, read while it runs). Where the curve bends, the roof is.

Results go to `<BIGFILE>-roof-<date and time>` beside it (outside the repository): results.md and
results.tsv, each written again after every run, so a run stopped part way keeps what it finished.
The prefixes and `.sieve` files are deleted once measured.
"""

import argparse
import datetime
import hashlib
import os
import platform
import shutil
import subprocess
import sys
import time
from pathlib import Path

try:
    import resource  # (Unix)
except ImportError:
    resource = None


def windows_peak_and_cpu(p):
    """A Windows process followed while it runs: its peak working set (bytes) and, once it has
    ended, its user and system time (seconds)."""
    import ctypes
    from ctypes import wintypes

    class PMC(ctypes.Structure):
        _fields_ = [("cb", wintypes.DWORD), ("PageFaultCount", wintypes.DWORD), ("PeakWorkingSetSize", ctypes.c_size_t),
                    ("WorkingSetSize", ctypes.c_size_t), ("QuotaPeakPagedPoolUsage", ctypes.c_size_t), ("QuotaPagedPoolUsage", ctypes.c_size_t),
                    ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t), ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
                    ("PagefileUsage", ctypes.c_size_t), ("PeakPagefileUsage", ctypes.c_size_t)]
    h = wintypes.HANDLE(int(p._handle))
    peak = 0
    while p.poll() is None:
        c = PMC()
        c.cb = ctypes.sizeof(PMC)
        if ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(c), c.cb):
            peak = max(peak, c.PeakWorkingSetSize)
        time.sleep(0.2)
    c = PMC()
    c.cb = ctypes.sizeof(PMC)
    if ctypes.windll.psapi.GetProcessMemoryInfo(h, ctypes.byref(c), c.cb):
        peak = max(peak, c.PeakWorkingSetSize)
    ft = [wintypes.FILETIME() for _ in range(4)]
    cpu = None
    if ctypes.windll.kernel32.GetProcessTimes(h, *[ctypes.byref(f) for f in ft]):
        s = lambda f: ((f.dwHighDateTime << 32) | f.dwLowDateTime) / 1e7
        cpu = s(ft[2]) + s(ft[3])
    return peak, cpu


def run(cmd, log):
    """A command's wall seconds, processor seconds and peak memory (bytes)."""
    t0 = time.perf_counter()
    with open(log, "w", encoding="utf-8") as out:
        out.write("$ " + " ".join(map(str, cmd)) + "\n")
        out.flush()
        if resource:
            # (Each command in a process of its own, so its peak is its own: ru_maxrss of the
            # children is the largest any has reached, here this one alone.)
            helper = [sys.executable, "-c",
                      "import resource,subprocess,sys;p=subprocess.run(sys.argv[1:]);r=resource.getrusage(resource.RUSAGE_CHILDREN);"
                      "print('ROOF %d %.3f %d' % (p.returncode, r.ru_utime+r.ru_stime, r.ru_maxrss*(1 if sys.platform=='darwin' else 1024)))"]
            p = subprocess.run(helper + list(map(str, cmd)), stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            text = p.stdout.decode("utf-8", "replace")
            out.write(text)
            line = [l for l in text.splitlines() if l.startswith("ROOF ")][-1].split()
            code, cpu, peak = int(line[1]), float(line[2]), int(line[3])
        else:
            p = subprocess.Popen(cmd, stdout=out, stderr=subprocess.STDOUT)
            peak, cpu = windows_peak_and_cpu(p)
            code = p.returncode
    return code, time.perf_counter() - t0, cpu, peak


def mib(n):
    return f"{n / (1 << 20):,.0f} MiB"


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve")
    ap.add_argument("bigfile")
    ap.add_argument("--sizes", default="1,4,16,64,256,1024", help="prefix sizes in MiB")
    ap.add_argument("--ways", default="size,decoding")
    ap.add_argument("--out")
    a = ap.parse_args()
    sieve = str(Path(a.sieve).resolve())
    big = Path(a.bigfile).resolve()
    total = big.stat().st_size
    sizes = sorted({min(int(s) << 20, total) for s in a.sizes.split(",")})
    started = datetime.datetime.now()
    out = Path(a.out) if a.out else big.parent / f"{big.name}-roof-{started:%Y%m%d-%H%M%S}"
    (out / "runs").mkdir(parents=True, exist_ok=True)
    rows = []

    def write():
        with open(out / "results.tsv", "w", encoding="utf-8") as f:
            f.write("bytes\tway\tsieve\tmade wall s\tmade cpu s\tmade peak bytes\tinstalled wall s\tinstalled cpu s\tinstalled peak bytes\tthe same\n")
            for r in rows:
                f.write("\t".join(str(r[k]) for k in ("n", "way", "sieve", "mw", "mc", "mp", "iw", "ic", "ip", "same")) + "\n")
        with open(out / "results.md", "w", encoding="utf-8") as f:
            f.write(f"# The roof: {big.name}\n\nMade {started:%Y-%m-%d %H:%M} on {platform.node()} ({platform.system()} {platform.release()}, "
                    f"{os.cpu_count()} logical processors), with {sieve}, no search limit. Each size is the file's first that many bytes. "
                    "Peak memory: the most the process held at once.\n\n")
            f.write("| Size | Way | `.sieve` | Made in (wall) | Made in (cpu) | Making's peak memory | Installed in (wall) | Installed in (cpu) | Installing's peak memory | The same |\n")
            f.write("| ---: | :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: | :--- |\n")
            for r in rows:
                f.write(f"| {mib(r['n'])} | {r['way']} | {r['sieve']:,} | {r['mw']:.1f} s | {r['mc']:.1f} s | {mib(r['mp'])} | "
                        f"{r['iw']:.1f} s | {r['ic']:.1f} s | {mib(r['ip'])} | {r['same']} |\n")

    with open(big, "rb") as src:
        for n in sizes:
            part = out / f"prefix-{n >> 20}MiB.bin"
            src.seek(0)
            with open(part, "wb") as dst:
                left = n
                while left:
                    chunk = src.read(min(left, 1 << 24))
                    dst.write(chunk)
                    left -= len(chunk)
            want = hashlib.sha256(part.read_bytes()).hexdigest()
            for way in a.ways.split(","):
                made = out / f"prefix-{n >> 20}MiB.{way}.sieve"
                extra = ["--prioritise-decoding"] if way == "decoding" else []
                print(f"{mib(n)}, for {way}: making ...", flush=True)
                code, mw, mc, mp = run([sieve, "locate", part, "--installer", made, "--search-per-file", "0"] + extra, out / "runs" / f"{n >> 20}MiB.{way}.make.txt")
                if code or not made.exists():
                    print(f"  making failed (exit {code}); its output is in runs/")
                    continue
                dest = out / "installed"
                shutil.rmtree(dest, ignore_errors=True)
                print(f"  {made.stat().st_size:,} bytes in {mw:.1f} s; installing ...", flush=True)
                code, iw, ic, ip = run([sieve, "install", made, "--to", dest], out / "runs" / f"{n >> 20}MiB.{way}.install.txt")
                got = dest / part.name
                same = "yes" if code == 0 and got.exists() and hashlib.sha256(got.read_bytes()).hexdigest() == want else "NO"
                rows.append({"n": n, "way": way, "sieve": made.stat().st_size, "mw": mw, "mc": mc or 0.0, "mp": mp, "iw": iw, "ic": ic or 0.0, "ip": ip, "same": same})
                print(f"  installed in {iw:.1f} s; peak memory making {mib(mp)}, installing {mib(ip)}; the same: {same}", flush=True)
                shutil.rmtree(dest, ignore_errors=True)
                made.unlink()
                write()
            part.unlink()
    print(f"Done: {out / 'results.md'}")


if __name__ == "__main__":
    main()
