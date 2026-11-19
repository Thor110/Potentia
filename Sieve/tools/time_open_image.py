"""How long a picture takes to open as itself and as Sieve instructions (.simage).

Usage (from the folder holding the Sieve programs, or give --bin; on Windows, time_open_image.bat beside it finds
the Release build and Python itself):
    python time_open_image.py PICTURE.tga [--bin FOLDER] [--runs 10]

It makes PICTURE.simage beside the picture if there is none (sieve-view --make-simage), then times, over
--runs runs each (the median and the fastest are printed):
  1. sieve-view opening the picture and writing its first screenshot, then exiting;
  2. the same for the .simage (the difference between 1 and 2 is what decoding the instructions costs);
  3. sieve install putting the picture's own file back from the .simage, byte for byte (checked).
Each run is a whole program start to exit, so it includes starting the program; the difference between the
two viewer times is the part that is Sieve's decoding.
"""
import argparse
import filecmp
import os
import shutil
import statistics
import subprocess
import sys
import tempfile
import time


def exe(folder, name):
    for n in (name + ".exe", name):
        p = os.path.join(folder, n)
        if os.path.isfile(p):
            return p
    sys.exit(f"no {name} in {folder}")


def timed(cmd, runs):
    out = []
    for _ in range(runs):
        t = time.perf_counter()
        r = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
        out.append(time.perf_counter() - t)
        if r.returncode != 0:
            sys.exit(f"{' '.join(cmd)} failed: {r.stderr.decode(errors='replace')[-400:]}")
    return out


def show(label, xs):
    print(f"{label:<44} median {statistics.median(xs) * 1000:8.1f} ms   fastest {min(xs) * 1000:8.1f} ms   ({len(xs)} runs)")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("picture")
    ap.add_argument("--bin", default=".")
    ap.add_argument("--runs", type=int, default=10)
    a = ap.parse_args()
    view, sieve = exe(a.bin, "sieve-view"), exe(a.bin, "sieve")
    pic = os.path.abspath(a.picture)
    simage = os.path.splitext(pic)[0] + ".simage"
    if not os.path.exists(simage):
        subprocess.run([view, pic, "--make-simage"], check=True, stdout=subprocess.DEVNULL)
    print(f"{os.path.basename(pic)}: {os.path.getsize(pic):,} bytes; {os.path.basename(simage)}: {os.path.getsize(simage):,} bytes")
    work = tempfile.mkdtemp()
    shot = os.path.join(work, "shot.bmp")
    try:
        subprocess.run([view, pic, "--screenshot", shot], stdout=subprocess.DEVNULL)  # (a first run warms the disk cache)
        v_pic = timed([view, pic, "--screenshot", shot], a.runs)
        v_sim = timed([view, simage, "--screenshot", shot], a.runs)
        inst = []
        for i in range(a.runs):
            to = os.path.join(work, f"out{i}")
            inst += timed([sieve, "install", simage, "--to", to], 1)
        back = os.path.join(work, "out0", os.path.basename(pic))
        same = os.path.exists(back) and filecmp.cmp(back, pic, shallow=False)
        show("viewer, the picture itself", v_pic)
        show("viewer, the .simage", v_sim)
        print(f"{'  decoding the .simage (the difference)':<44} median {(statistics.median(v_sim) - statistics.median(v_pic)) * 1000:8.1f} ms")
        show("sieve install, the .simage to its file", inst)
        print("the file put back is the original, byte for byte" if same else "THE FILE PUT BACK DIFFERS")
    finally:
        shutil.rmtree(work, ignore_errors=True)


if __name__ == "__main__":
    main()
