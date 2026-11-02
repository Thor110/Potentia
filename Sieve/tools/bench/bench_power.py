#!/usr/bin/env python3
r"""What a file made smaller would save the world's networks, and what it would move onto the people
who install it (Edward, 2 November 2026; IDEAS §19.111, "What it costs the world and what it saves").

  python tools/bench/bench_power.py SIEVE FILE... [--out FOLDER] [settings]

Each file is made each way: Sieve for size and for decoding speed (Archive and compressed formats
temporary, so pictures and archives are made again exactly), and as a reference zip (deflate 9), 7z
(LZMA2 9e) and, where Python has it, brotli (11). For each way it measures, and these are the only
figures it measures:
  - its bytes;
  - the processor time to make it, once (the user and system time of the process, which counts
    every thread, not the wall clock);
  - the processor time to install it on the user's machine (the same);
  - where the processor's own energy counter can be read (Linux's RAPL under /sys/class/powercap),
    the joules of making and installing, measured.
Everything else is a setting, an assumption, written beside the results:
  --net-kwh-per-gb LOW HIGH   energy to move a gigabyte across the networks (published estimates
                              differ about tenfold between studies and years; the defaults, 0.006
                              and 0.06, span that: Aslan et al. 2018 put it at 0.06 for 2015, and
                              falling by about half every two years)
  --user-watts W              the user's processor while installing, where it cannot be measured (35)
  --server-watts W            the maker's processor while making, where it cannot be measured (65)
  --fetches N ...             fetches a day to show (1,000, 1,000,000, 1,000,000,000)
From them, against 7z: the network energy a fetch saves, the energy a fetch moves onto its user
(installing, against 7z's own), what a day comes to at each count of fetches, and how many fetches
it takes for the saving to pay for making it, once. A way whose extra installing costs more than its
saving on the network never pays, however many fetch it.
"""

import argparse
import datetime
import lzma
import os
import platform
try:
    import resource  # (Unix: the children's processor time)
except ImportError:
    resource = None
import shutil
import subprocess
import sys
import tempfile
import time
import zlib
from pathlib import Path

try:
    import brotli  # (where Python has it)
except ImportError:
    brotli = None

# (the package zones alone, intel-rapl:0, intel-rapl:1 ...: their sub-zones, core and uncore, are
# counted inside them, so adding those would count the same joules twice)
RAPL = [p for p in Path("/sys/class/powercap").glob("intel-rapl:*/energy_uj") if p.parent.name.count(":") == 1] if Path("/sys/class/powercap").exists() else []


def rapl_uj():
    try:
        return sum(int(p.read_text()) for p in RAPL) if RAPL else None
    except (OSError, ValueError):
        return None


def windows_cpu(p):
    """A finished process's user and system time, from Windows (GetProcessTimes, 100 ns units)."""
    import ctypes
    from ctypes import wintypes
    ft = [wintypes.FILETIME() for _ in range(4)]
    ok = ctypes.windll.kernel32.GetProcessTimes(wintypes.HANDLE(int(p._handle)), *[ctypes.byref(f) for f in ft])
    if not ok:
        return None
    as_s = lambda f: ((f.dwHighDateTime << 32) | f.dwLowDateTime) / 1e7
    return as_s(ft[2]) + as_s(ft[3])


def run(cmd):
    """A command's processor seconds (its user and system time, every thread) and, where the
    processor's counter can be read, joules."""
    e0 = rapl_uj()
    if resource:
        r0 = resource.getrusage(resource.RUSAGE_CHILDREN)
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    out, _ = p.communicate()
    e1 = rapl_uj()
    if resource:
        r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
        cpu = (r1.ru_utime + r1.ru_stime) - (r0.ru_utime + r0.ru_stime)
    else:
        cpu = windows_cpu(p) or 0.0  # (0 where Windows would not say)
    if p.returncode:
        raise RuntimeError(" ".join(map(str, cmd)) + ": " + out.decode("utf-8", "replace")[-400:])
    return cpu, (e1 - e0) / 1e6 if e0 is not None and e1 is not None and e1 >= e0 else None


def in_process(fn):
    t0 = time.process_time()
    out = fn()
    return out, time.process_time() - t0


def overhead(sieve, tmp):
    """What starting Sieve costs, apart from the work: making and installing a 1-byte file, the
    least of three (taken off Sieve's figures, as the other ways run inside this process)."""
    one = Path(tmp) / "one.bin"
    one.write_bytes(b"x")
    made, dest = Path(tmp) / "one.sieve", Path(tmp) / "one-inst"
    m, i = [], []
    for _ in range(3):
        m.append(run([sieve, "locate", str(one), "--installer", str(made)])[0])
        shutil.rmtree(dest, ignore_errors=True)
        i.append(run([sieve, "install", str(made), "--to", str(dest)])[0])
    return min(m), min(i)


def measure(sieve, f, tmp, base):
    x = Path(f).read_bytes()
    ways = {}
    for name, extra in (("Sieve, size", []), ("Sieve, decoding speed", ["--prioritise-decoding"])):
        made = Path(tmp) / "f.sieve"
        cpu_make, j_make = run([sieve, "locate", str(f), "--installer", str(made), "--compressed-temporary"] + extra)
        dest = Path(tmp) / "inst"
        shutil.rmtree(dest, ignore_errors=True)
        cpu_inst, j_inst = run([sieve, "install", str(made), "--to", str(dest)])
        ways[name] = {"bytes": made.stat().st_size, "make": max(0.0, cpu_make - base[0]), "install": max(0.0, cpu_inst - base[1]), "j_make": j_make, "j_install": j_inst}
        made.unlink()
        shutil.rmtree(dest, ignore_errors=True)
    back = Path(tmp) / "back.bin"  # (each reference writes what it installs, as Sieve does)
    z, tz = in_process(lambda: zlib.compress(x, 9))
    _, tzd = in_process(lambda: back.write_bytes(zlib.decompress(z)))
    ways["zip (deflate 9)"] = {"bytes": len(z), "make": tz, "install": tzd}
    filt = [{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]
    s7, t7 = in_process(lambda: lzma.compress(x, format=lzma.FORMAT_RAW, filters=filt))
    _, t7d = in_process(lambda: back.write_bytes(lzma.decompress(s7, format=lzma.FORMAT_RAW, filters=filt)))
    ways["7z (LZMA2 9e)"] = {"bytes": len(s7), "make": t7, "install": t7d}
    if brotli:
        b, tb = in_process(lambda: brotli.compress(x, quality=11))
        _, tbd = in_process(lambda: back.write_bytes(brotli.decompress(b)))
        ways["brotli (11)"] = {"bytes": len(b), "make": tb, "install": tbd}
    return len(x), ways


def kwh(joules):
    return joules / 3.6e6


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("sieve")
    ap.add_argument("files", nargs="+")
    ap.add_argument("--out", default=None)
    ap.add_argument("--net-kwh-per-gb", nargs=2, type=float, default=[0.006, 0.06], metavar=("LOW", "HIGH"))
    ap.add_argument("--user-watts", type=float, default=35.0)
    ap.add_argument("--server-watts", type=float, default=65.0)
    ap.add_argument("--fetches", nargs="+", type=float, default=[1e3, 1e6, 1e9])
    a = ap.parse_args()
    sieve = str(Path(a.sieve).resolve())
    started = datetime.datetime.now()
    out = Path(a.out) if a.out else Path(a.files[0]).resolve().parent / f"power-bench-{started:%Y%m%d-%H%M%S}"
    out.mkdir(parents=True, exist_ok=True)
    lo, hi = a.net_kwh_per_gb
    md = [f"# What it saves the networks and what it moves onto users\n",
          f"Made {started:%Y-%m-%d %H:%M} on {platform.node()} ({platform.system()} {platform.release()}, {os.cpu_count()} logical processors). "
          f"Measured: bytes, and processor seconds to make and to install (user and system time, every thread){', and joules by the processor counter' if RAPL else ''}. "
          f"Assumed (settings): moving a gigabyte {lo:g} to {hi:g} kWh; the user's processor {a.user_watts:g} W and the maker's {a.server_watts:g} W "
          f"where no counter is read. Against 7z: a fetch's saving on the networks, and the energy a fetch moves onto its user (installing, against 7z's own).\n"]
    tsv = ["file\tbytes\tway\tsize\tmake cpu s\tinstall cpu s\tmake J (counter)\tinstall J (counter)"]
    with tempfile.TemporaryDirectory() as tmp:
        base = overhead(sieve, tmp)
        md.append(f"\nSieve's own start-up, measured on a 1-byte file and taken off its figures: making {base[0]:.3f} cpu s, installing {base[1]:.3f} cpu s. "
                  "What is left of installing includes checking each file's SHA-256 and writing it; the other ways write what they install too.\n")
        for f in a.files:
            n, ways = measure(sieve, f, tmp, base)
            ref = ways["7z (LZMA2 9e)"]
            md.append(f"\n## {Path(f).name}: {n:,} bytes\n")
            md.append("| Way | Bytes | Made in (cpu s) | Installed in (cpu s) | A fetch saves the networks (Wh) | A fetch moves onto the user (Wh) | Pays for making after (fetches) |")
            md.append("| :--- | ---: | ---: | ---: | ---: | ---: | ---: |")
            for name, w in ways.items():
                tsv.append("\t".join(str(v) for v in (Path(f).name, n, name, w["bytes"], f"{w['make']:.3f}", f"{w['install']:.3f}",
                                                      "" if w.get("j_make") is None else f"{w['j_make']:.1f}", "" if w.get("j_install") is None else f"{w['j_install']:.1f}")))
                saved_gb = (ref["bytes"] - w["bytes"]) / 1e9
                save_lo, save_hi = saved_gb * lo * 1000, saved_gb * hi * 1000  # Wh
                user_j = (w["j_install"] if w.get("j_install") is not None else w["install"] * a.user_watts) - ref["install"] * a.user_watts
                user_wh = user_j / 3600
                make_j = w["j_make"] if w.get("j_make") is not None else w["make"] * a.server_watts
                net_lo, net_hi = save_lo - user_wh, save_hi - user_wh
                if name == "7z (LZMA2 9e)":
                    pays = "(the reference)"
                elif max(net_lo, net_hi) <= 0:
                    pays = "never"
                else:
                    ends = sorted((make_j / 3600 / v if v > 0 else float("inf")) for v in (net_lo, net_hi))
                    pays = " to ".join("never" if e == float("inf") else f"{e:,.0f}" for e in ends)
                md.append(f"| {name} | {w['bytes']:,} | {w['make']:.2f} | {w['install']:.3f} | {save_lo:.2e} to {save_hi:.2e} | {user_wh:+.2e} | {pays} |")
            md.append("\nA day, at each count of fetches (kWh: the networks' saving, low to high, less what moves onto users):\n")
            md.append("| Way | " + " | ".join(f"{c:,.0f} a day" for c in a.fetches) + " |")
            md.append("| :--- |" + " ---: |" * len(a.fetches))
            for name, w in ways.items():
                saved_gb = (ref["bytes"] - w["bytes"]) / 1e9
                user_wh = ((w["j_install"] if w.get("j_install") is not None else w["install"] * a.user_watts) - ref["install"] * a.user_watts) / 3600
                cells = []
                for c in a.fetches:
                    lo_k = (saved_gb * lo * 1000 - user_wh) * c / 1000
                    hi_k = (saved_gb * hi * 1000 - user_wh) * c / 1000
                    cells.append(f"{lo_k:,.3g} to {hi_k:,.3g}")
                md.append(f"| {name} | " + " | ".join(cells) + " |")
            print(f"{Path(f).name}: done", flush=True)
            (out / "results.md").write_text("\n".join(md) + "\n", encoding="utf-8")
            (out / "results.tsv").write_text("\n".join(tsv) + "\n", encoding="utf-8")
    print(f"Done: {out / 'results.md'}")


if __name__ == "__main__":
    main()
