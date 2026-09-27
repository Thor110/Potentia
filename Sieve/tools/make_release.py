#!/usr/bin/env python3
"""Sieve's release, made in the one order that works (docs/SIEVE-INSTALL-USAGE.md, HANDOFF).

Every file of a release is named by its hash, and every hash depends on the step before it, so the
order matters and is easy to get wrong by hand:

  1. (--with-source) The source archive, from the files git tracks, and its Sieve instructions;
     data/maps/sieve.map is then made to name them, so the build carries the map of the source
     it came from. Without --with-source the shipped sieve.map is left as it is.
  2. The build (Release), with maps/sieve.map and potentia-license.txt beside the programs.
  3. The release folder: only what the program uses (the programs, their data folders, the maps,
     the licences), staged by name, never the build folder's own clutter.
  4. The folder compressed first (the proper procedure: an installer is never smaller than what it
     carries), as a solid 7z at 7-Zip's strongest.
  5. From the archive: Sieve instructions (sieve.sieve, for anyone with Sieve) and an installer
     program (sieve.exe on Windows, sieve-setup elsewhere, for anyone), made by the release's own
     sieve.
  6. The published map: sealed, naming both (found beside it), with its own SHA-256 printed, since
     a map cannot name itself.
  7. Checks: each published file installed into a scratch folder and compared with the archive,
     byte for byte; the map read back and every node verified beside it.
  8. SHA256SUMS.txt and a release-notes draft.

Published (the three files, Edward's naming): sieve.exe, sieve.sieve and sieve.map. The version is
in the release folder they install (Sieve-<version>) and in the notes, not in the names. GitHub adds
the source zip of the tagged commit by itself.

Usage (from the repository root, on the machine the release is for):

  python tools/make_release.py --version 0.13.0
  python tools/make_release.py --version 0.13.0 --build-dir out/build/x64-Release --out release
  python tools/make_release.py --version 0.13.0 --skip-build      # the build is already made
  python tools/make_release.py --version 0.13.0 --uncompressed    # install the folder itself

--uncompressed makes the instructions and the program from the release folder rather than the 7z:
larger to download, but it installs straight to a runnable folder, with no 7-Zip needed to unpack.

It needs Python 3.8+, CMake, and 7-Zip (7z on the PATH, or in Program Files). It changes nothing
in the repository except data/maps/sieve.map, and only with --with-source.
"""

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ".exe" if os.name == "nt" else ""

# What a release holds from the build folder (client/node_graph.cpp maps the same list as "This
# installation"): the programs, the folders they read, and the licences. Anything else in a build
# folder (CMakeFiles, objects, fetched sources, the trimmed SDL) is the build's, not the release's.
PROGRAMS = ["hallway", "sieve", "sieve-install"]
FOLDERS = ["dictionaries", "models", "lang", "fonts", "meshes", "third_party_licenses"]
FILES = ["potentia-license.txt"]


def fail(msg):
    print(f"make_release: {msg}", file=sys.stderr)
    sys.exit(1)


def step(msg):
    print(f"\n== {msg}")


def run(cmd, cwd=None):
    print("   $ " + " ".join(str(c) for c in cmd))
    r = subprocess.run([str(c) for c in cmd], cwd=cwd)
    if r.returncode != 0:
        fail(f"{cmd[0]} failed ({r.returncode})")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def same_tree(a, b):
    """Every file under a is under b with the same bytes, and b has nothing more."""
    fa = sorted(p.relative_to(a).as_posix() for p in Path(a).rglob("*") if p.is_file())
    fb = sorted(p.relative_to(b).as_posix() for p in Path(b).rglob("*") if p.is_file())
    return fa == fb and all(sha256(Path(a) / f) == sha256(Path(b) / f) for f in fa)


def find_7z():
    for name in ("7z", "7za", "7zz"):
        p = shutil.which(name)
        if p:
            return p
    for p in (r"C:\Program Files\7-Zip\7z.exe", r"C:\Program Files (x86)\7-Zip\7z.exe"):
        if Path(p).exists():
            return p
    fail("7-Zip not found: install it, or put 7z on the PATH")


def default_build_dir():
    preset = "x64-Release" if os.name == "nt" else "linux-release"
    return ROOT / "out" / "build" / preset


def seven_zip(sz, archive, inputs, cwd):
    # Solid LZMA2 at 7-Zip's strongest (-mx=9), as the comparison's "7z" measures.
    if archive.exists():
        archive.unlink()
    run([sz, "a", "-t7z", "-m0=lzma2", "-mx=9", "-ms=on", "-bd", archive] + inputs, cwd=cwd)


def main():
    ap = argparse.ArgumentParser(description="Make Sieve's release files, in order, and check them.")
    ap.add_argument("--version", required=True, help="the release's version, e.g. 0.13.0")
    ap.add_argument("--build-dir", type=Path, default=None, help="the CMake build folder (default: the Release preset's)")
    ap.add_argument("--out", type=Path, default=ROOT / "release", help="where the release files go (default: release/)")
    ap.add_argument("--name", default="sieve", help="the published files' base name (default: sieve)")
    ap.add_argument("--skip-build", action="store_true", help="use the build as it is")
    ap.add_argument("--with-source", action="store_true", help="also make the source's Sieve instructions and name them in sieve.map")
    ap.add_argument("--uncompressed", action="store_true", help="make the installers from the folder, not the 7z")
    a = ap.parse_args()

    build = (a.build_dir or default_build_dir()).resolve()
    name = a.name
    folder = f"Sieve-{a.version}"  # the release folder: what the 7z unpacks to, or what installs
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sz = find_7z()
    if not (ROOT.parent / "LICENSE").exists():
        fail(f"no LICENSE one folder up ({ROOT.parent / 'LICENSE'}): a release carries Potentia's licence")

    # 1. The source, and the map naming it, before the build that carries the map.
    if a.with_source:
        step("the source archive and its Sieve instructions")
        tracked = subprocess.run(["git", "ls-files", "-z"], cwd=ROOT, capture_output=True, check=True).stdout.split(b"\0")
        listfile = out / "source-files.txt"
        listfile.write_text("\n".join(t.decode("utf-8") for t in tracked if t), encoding="utf-8")
        src7z = out / f"Sieve-{a.version}-src.7z"
        seven_zip(sz, src7z, [f"@{listfile}"], cwd=ROOT)
        listfile.unlink()
        if a.skip_build:
            sieve = build / f"sieve{EXE}"
        else:
            run(["cmake", "--build", build, "--config", "Release", "--target", "sieve"])
            sieve = build / f"sieve{EXE}"
        src_sieve = out / f"Sieve-{a.version}-src.sieve"
        run([sieve, "locate", src7z, "--installer", src_sieve])
        run([sieve, "map", src_sieve, "--name", "sieve", "--seal", "--out", ROOT / "data" / "maps" / "sieve.map"])
        print(f"   data/maps/sieve.map now names {src_sieve.name} ({sha256(src_sieve)})")

    # 2. The build.
    if not a.skip_build:
        step("the build (Release)")
        run(["cmake", "--build", build, "--config", "Release"])
    for p in PROGRAMS:
        if not (build / f"{p}{EXE}").exists():
            fail(f"{p}{EXE} is not in {build}: build the client (SIEVE_BUILD_CLIENT) in Release")
    sieve = build / f"sieve{EXE}"

    # 3. The release folder, by name.
    step(f"the release folder: {folder}")
    stage = out / folder
    if stage.exists():
        shutil.rmtree(stage)
    stage.mkdir()
    for p in PROGRAMS:
        shutil.copy2(build / f"{p}{EXE}", stage)
    for d in FOLDERS:
        if (build / d).is_dir():
            shutil.copytree(build / d, stage / d)
    shutil.copytree(ROOT / "data" / "maps", stage / "maps")  # the maps shipped, never a person's own
    for f in FILES:
        if not (build / f).exists():
            fail(f"{f} is not in {build}")
        shutil.copy2(build / f, stage)

    # 4. Compressed first.
    step("compressed first: the 7z")
    archive = out / f"{name}.7z"
    seven_zip(sz, archive, [folder], cwd=out)
    carried = stage if a.uncompressed else archive

    # 5. The instructions and the program, made by the release's own sieve.
    step("Sieve instructions and the installer program")
    instructions = out / f"{name}.sieve"
    program = out / (f"{name}{EXE}" if EXE else f"{name}-setup")  # sieve.exe; elsewhere not the release folder's name
    run([sieve, "locate", carried, "--installer", instructions, "--program", program, "--compare"])

    # 6. The published map, naming both, beside them.
    step("the published map")
    mapfile = out / f"{name}.map"
    run([sieve, "map", program, instructions, "--name", name, "--seal", "--out", mapfile])

    # 7. Checks: every published file installs back to what it carries, and the map verifies.
    step("checks")
    with tempfile.TemporaryDirectory() as tmp:
        for i, f in enumerate((instructions, program)):
            dest = Path(tmp) / f"install{i}"
            run([sieve, "install", f, "--to", dest])
            if a.uncompressed:
                if not same_tree(stage, dest):
                    fail(f"{f.name} did not install the release folder as it is")
            else:
                got = dest / archive.name
                if not got.exists() or sha256(got) != sha256(archive):
                    fail(f"{f.name} did not install {archive.name} byte for byte")
            print(f"   {f.name}: installs back to what it carries, checked")
    text = subprocess.run([str(sieve), "map", str(mapfile)], capture_output=True, text=True, check=True).stdout
    for f in (program, instructions):
        if sha256(f) not in text:
            fail(f"{mapfile.name} does not name {f.name}")
    print(f"   {mapfile.name}: sealed, names both, by SHA-256")

    # 8. Sums and notes.
    step("SHA256SUMS.txt and the notes")
    published = [program, instructions, mapfile]
    sums = "".join(f"{sha256(f)}  {f.name}\n" for f in published)
    (out / "SHA256SUMS.txt").write_text(sums, encoding="utf-8")
    sizes = "\n".join(f"| `{f.name}` | {f.stat().st_size:,} bytes | `{sha256(f)}` |" for f in published)
    notes = f"""# Sieve {a.version}

The Gallery of Babel's engine and its hallway: every page, picture, sound, film, book, model and
file there can be, each with an address, walked through as a library.

## Downloads

| File | Size | SHA-256 |
|---|---|---|
{sizes}

- **`{program.name}`**: run it to install. Nothing else is needed.
- **`{instructions.name}`**: the same, as Sieve instructions, for anyone who already has Sieve
  (`sieve-install`, `sieve install {instructions.name} --to FOLDER`, or the hallway's File
  Locator).
- **`{mapfile.name}`**: a sealed map naming both. Put it beside them and open it in the node
  graph (O) to see them verified. Its own SHA-256 is above.

{f"The installers put the `{folder}` folder in place." if a.uncompressed else f"The installers put `{archive.name}` in place (compressed first: a fraction of the size); unpack it with 7-Zip to get the `{folder}` folder."}

Windows SmartScreen will warn that the programs are not signed; choose More info, then Run anyway.

Sieve is licensed under Potentia's licence (`potentia-license.txt`); the third-party licences are
in `third_party_licenses/`.
"""
    (out / "RELEASE-NOTES.md").write_text(notes, encoding="utf-8")
    print(sums, end="")
    print(f"\nDone. Publish these three from {out}:\n  {program.name}\n  {instructions.name}\n  {mapfile.name}")
    print("with RELEASE-NOTES.md as the release's description.")
    if a.with_source:
        print("data/maps/sieve.map changed: commit it (it is what this build carries).")


if __name__ == "__main__":
    main()
