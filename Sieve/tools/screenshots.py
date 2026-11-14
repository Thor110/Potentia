#!/usr/bin/env python3
"""Every screen and feature's screenshot made, tagged with the version, and made again only when what it
is made from has changed (IDEAS 19.202).

    python tools/screenshots.py --hallway build/hallway --out shots/
    python tools/screenshots.py --hallway build/hallway --out shots/ --only character
    python tools/screenshots.py --hallway build/hallway --out shots/ --all

The shots are listed in tools/shots.txt (name, the files each depends on, the hallway's arguments). In
--out the tool keeps shots.tsv, one line a shot: its name, the version it was made at, the SHA-256 of
its inputs (the files it depends on, the language file, its arguments and the program's version) and
of its picture. A shot whose inputs are unchanged is skipped; one made again whose picture comes out the
same keeps its old version tag (the picture did not change, so neither does its name). Each picture is
NAME-vVERSION.png beside shots.tsv, for the website to read.

Nothing personal is read: the hallway gets HOME set to a folder of the tool's own, its own settings
files, and seed 99's character (as tools/render_scales.py).
"""
import argparse
import glob
import hashlib
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))  # Sieve/
CHARACTER = "sieve-character-v1 seed=99 limbs=5 joints=3 branches=1 symmetry=bilateral head=yes\n"
WORLD = "sieve-personal-world-v2\nseed 0\nplace bookcase 0 22.9 -57 3.14159274 0 0 fixed\nplace pages -0.1 25.15 -56.82 0 0 0 fixed\n"
SIZE = "1280x720"


def version():
    with open(os.path.join(HERE, "CMakeLists.txt"), encoding="utf-8") as f:
        m = re.search(r"project\(Sieve VERSION ([0-9.]+)", f.read())
    return m.group(1) if m else "0"


def shots():
    out = []
    with open(os.path.join(HERE, "tools", "shots.txt"), encoding="utf-8") as f:
        for n, line in enumerate(f, 1):
            line = line.rstrip("\n")
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            parts = [p.strip() for p in line.split(" | ")]
            if len(parts) != 3:
                sys.exit(f"tools/shots.txt line {n}: three fields split by ' | ' are wanted")
            out.append({"name": parts[0], "depends": parts[1].split(), "args": parts[2].split()})
    return out


def inputs_sha(shot, ver):
    """The SHA-256 of what a shot is made from: its files (by name and contents, in order), the language
    file, its arguments and the version."""
    h = hashlib.sha256()
    files = set()
    for pat in shot["depends"] + ["data/lang/en.txt"]:
        for p in glob.glob(os.path.join(HERE, pat)):
            if os.path.isdir(p):  # (a folder: every file in it, at any depth)
                for root, _, names in os.walk(p):
                    files.update(os.path.join(root, n) for n in names)
            else:
                files.add(p)
    for p in sorted(files):
        h.update(os.path.relpath(p, HERE).replace(os.sep, "/").encode() + b"\0")
        with open(p, "rb") as f:
            h.update(hashlib.sha256(f.read()).digest())
    h.update(("\0".join(shot["args"]) + "\0" + ver).encode())
    return h.hexdigest()


def read_table(path):
    rows = {}
    if os.path.exists(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                p = line.rstrip("\n").split("\t")
                if len(p) == 4 and p[0] != "name":
                    rows[p[0]] = {"version": p[1], "inputs": p[2], "picture": p[3]}
    return rows


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--hallway", required=True, help="the hallway program")
    ap.add_argument("--out", required=True, help="where the shots and shots.tsv are kept")
    ap.add_argument("--only", help="only the shots whose name holds this word")
    ap.add_argument("--all", action="store_true", help="make every shot again, changed or not")
    a = ap.parse_args()
    hallway = os.path.abspath(a.hallway)
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    ver = version()
    table_path = os.path.join(out, "shots.tsv")
    table = read_table(table_path)
    work = tempfile.mkdtemp(prefix="sieve-shots-")
    files = {"c.character": CHARACTER, "w.world": WORLD}
    # (one for each character drawing: 0 pixels, 1 wireframe, 2 voxels, 3 shaded, 4 the skin, IDEAS 19.225;
    # 5 the skin's quads and 6 the smooth skin, IDEAS 19.214)
    for k in range(7):
        files[f"d{k}.ini"] = f"[character]\ndrawing = {k}\n[graphics]\nreal_graphics = on\n"
    # (the voxels on the world lattice with their faces joined, IDEAS 19.216 and 19.215)
    files["vl.ini"] = "[character]\ndrawing = 2\nvoxel_size = 1\nvoxel_joined = on\n[graphics]\nreal_graphics = on\n"
    for name, text in files.items():
        with open(os.path.join(work, name), "w", newline="\n") as f:
            f.write(text)
    stand = {"{character}": os.path.join(work, "c.character"), "{world}": os.path.join(work, "w.world")}
    for k in range(7):
        stand[f"{{d{k}}}"] = os.path.join(work, f"d{k}.ini")
    stand["{vl}"] = os.path.join(work, "vl.ini")
    env = dict(os.environ, SDL_VIDEO_DRIVER=os.environ.get("SDL_VIDEO_DRIVER", "offscreen"), HOME=work, USERPROFILE=work)
    made = kept = same = 0
    try:
        for shot in shots():
            if a.only and a.only not in shot["name"]:
                continue
            sha = inputs_sha(shot, ver)
            old = table.get(shot["name"])
            if old and old["inputs"] == sha and not a.all and os.path.exists(os.path.join(out, f"{shot['name']}-v{old['version']}.png")):
                kept += 1
                print(f"unchanged {shot['name']} (v{old['version']})")
                continue
            # A fresh world each time (the world file is written back when the dimension saves).
            with open(stand["{world}"], "w", newline="\n") as f:
                f.write(WORLD)
            args = [stand.get(x, x) for x in shot["args"]]
            # (the --press keys are one argument; its commas stay as they are)
            png = os.path.join(work, "shot.png")
            cmd = [hallway, "--size", SIZE] + args + ["--screenshot", png]
            r = subprocess.run(cmd, cwd=HERE, env=env, capture_output=True, text=True)
            if r.returncode != 0 or not os.path.exists(png):
                sys.exit(f"{shot['name']} failed: {' '.join(cmd)}\n{r.stdout[-1500:]}{r.stderr[-1500:]}")
            with open(png, "rb") as f:
                pic = hashlib.sha256(f.read()).hexdigest()
            if old and old["picture"] == pic and os.path.exists(os.path.join(out, f"{shot['name']}-v{old['version']}.png")):
                # Made again, the same picture: its old version tag stays.
                table[shot["name"]] = {"version": old["version"], "inputs": sha, "picture": pic}
                same += 1
                print(f"same      {shot['name']} (v{old['version']}: its inputs changed, its picture did not)")
                os.remove(png)
                continue
            if old:
                stale = os.path.join(out, f"{shot['name']}-v{old['version']}.png")
                if os.path.exists(stale) and old["version"] != ver:
                    os.remove(stale)  # (the newer one takes its place)
            shutil.move(png, os.path.join(out, f"{shot['name']}-v{ver}.png"))
            table[shot["name"]] = {"version": ver, "inputs": sha, "picture": pic}
            made += 1
            print(f"made      {shot['name']} (v{ver})")
    finally:
        shutil.rmtree(work, ignore_errors=True)
        with open(table_path, "w", encoding="utf-8", newline="\n") as f:
            f.write("name\tversion\tinputs\tpicture\n")
            for name in sorted(table):
                t = table[name]
                f.write(f"{name}\t{t['version']}\t{t['inputs']}\t{t['picture']}\n")
    print(f"{made} made, {same} made again the same, {kept} unchanged; listed in {table_path}")


if __name__ == "__main__":
    main()
