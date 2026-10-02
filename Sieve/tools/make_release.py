#!/usr/bin/env python3
r"""Sieve's release, made in the one order that works (docs/SIEVE-INSTALL-USAGE.md, HANDOFF).

Every file of a release is named by its hash, and every hash depends on the step before it, so the
order matters and is easy to get wrong by hand:

  1. (Nothing: the source's own step is 6, now that the published map holds it.)
  2. The build (Release), with potentia-license.txt beside the programs.
  3. The release folder: only what the program uses (the programs, their data folders, an empty
     maps folder, the licences), staged by name, never the build folder's own clutter. The hallway
     is the one program at the top; sieve and sieve-install go in tools\, with a note saying what
     they are, so nobody starts the wrong program first. (A build folder keeps them together; the
     programs look in both places.) No map is
     shipped inside it: the published map names the release's files, so it cannot be one of them.
  4. The folder compressed first (the proper procedure: an installer is never smaller than what it
     carries), as a solid 7z at 7-Zip's strongest.
  5. From the archive: Sieve instructions (sieve.sieve, for anyone with Sieve) and an installer
     program (sieve.exe on Windows, sieve-setup elsewhere, for anyone), made by the release's own
     sieve.
  6. The source: sieve-source.7z, the Sieve folder alone as the last Git commit has it (git
     archive), so it holds the committed source, this script included, and none of the build's
     clutter.
  7. The published map: sealed, naming the installer and the instructions (found beside it) and
     holding the source (its bytes inside the map), with its own SHA-256 printed, since a map
     cannot name itself.
  8. Checks: each installer installed into a scratch folder and compared with the archive, byte
     for byte; the map read back and every node verified beside it.
  9. SHA256SUMS.txt and a release-notes draft.

Published (three files, Edward's naming): sieve.exe, sieve.sieve and sieve.map, the map naming the
other two and holding sieve-source.7z. The version is
in the release folder they install (Sieve-<version>) and in the notes, not in the names. GitHub adds
the source zip of the tagged commit by itself.

Usage (from the repository root, on the machine the release is for):

  tools\make_release.bat                            # Windows: double-click it; everything by default
  python tools/make_release.py                      # the version from CMakeLists.txt, into release/
  python tools/make_release.py --version 0.13.1 --build-dir out/build/x64-Release --out release
  python tools/make_release.py --version 0.13.1 --skip-build      # the build is already made
  python tools/make_release.py --version 0.13.1 --uncompressed    # install the folder itself

After publishing, make the map again to name GitHub's source zip as well (GitHub makes it only
once the release is out, so its hash cannot be known before):

  tools\make_release.bat --map-with "C:\Users\...\Downloads\Potentia-0.13.1.zip"

--uncompressed makes the instructions and the program from the release folder rather than the 7z:
larger to download, but it installs straight to a runnable folder, with no 7-Zip needed to unpack.

It needs Python 3.8+, CMake, and 7-Zip (7z on the PATH, or in Program Files). It changes nothing
in the repository. (--with-source, which rewrote a shipped data/maps/sieve.map to name a source
archive, was dropped on 27 September 2026: the published map holds the source instead.)
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
TOOLS = ["sieve", "sieve-install"]  # in the release, these go in tools/ below the hallway
TOOLS_NOTE = """Sieve's tools

These are Sieve's helper programs. To run Sieve, start hallway{exe} in the folder above.

  sieve{exe}          the command-line tool: locating files and folders, Sieve instructions,
                      maps, filters and models. Run it from a command prompt: sieve help
  sieve-install{exe}  the installer. The hallway's File Locator copies it to make installer
                      programs; run on its own, it installs a .sieve file beside it or dropped on it.

Both find the installation's data folders (dictionaries, models) in the folder above.
"""
FOLDERS = ["dictionaries", "models", "lang", "fonts", "meshes", "vault", "filters", "third_party_licenses"]
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


def source_7z(dest, sz, work):
    """The Sieve folder's source as sieve-source.7z, with Sieve/ as its one top folder. From Git when
    the folder is in a repository: the last commit's tree of the Sieve folder alone (git archive
    HEAD:<its path>, run from the repository's top folder, since run inside the Sieve folder it
    would narrow to it a second time and find nothing), so only what is committed goes in, none of
    the build's or the checks' scratch; with a warning if there are changes not yet committed.
    Without Git, the folder's files less the build folders and the release folder."""
    import tarfile
    import io
    if dest.exists():
        dest.unlink()
    tree_dir = work / "source"
    if tree_dir.exists():
        shutil.rmtree(tree_dir)
    tree_dir.mkdir(parents=True)
    inside = subprocess.run(["git", "rev-parse", "--show-prefix"], cwd=ROOT, capture_output=True, text=True)
    if inside.returncode == 0:
        prefix = inside.stdout.strip()  # the Sieve folder's path in the repository ("Sieve/", or "")
        dirty = subprocess.run(["git", "status", "--porcelain", "--", "."], cwd=ROOT, capture_output=True, text=True).stdout
        if dirty.strip():
            print("   warning: there are changes not committed; the source is the last commit, without them:")
            print("".join(f"     {l}\n" for l in dirty.strip().splitlines()[:10]), end="")
        top = subprocess.run(["git", "rev-parse", "--show-toplevel"], cwd=ROOT, capture_output=True, text=True, check=True).stdout.strip()
        tree = f"HEAD:{prefix.rstrip('/')}" if prefix else "HEAD"
        tar = subprocess.run(["git", "archive", "--format=tar", "--prefix=Sieve/", tree], cwd=top, capture_output=True, check=True).stdout
        with tarfile.open(fileobj=io.BytesIO(tar)) as t:
            t.extractall(tree_dir)
    else:
        skip = {"out", ".vs", "release", "lt", "__pycache__", ".git"}
        for p in sorted(ROOT.rglob("*")):
            rel = p.relative_to(ROOT)
            if not p.is_file() or rel.parts[0] in skip or rel.parts[0].startswith("build") or "__pycache__" in rel.parts:
                continue
            (tree_dir / "Sieve" / rel).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, tree_dir / "Sieve" / rel)
        print("   (not a Git repository: the folder's files, less the build folders)")
    seven_zip(sz, dest, ["Sieve"], cwd=tree_dir)
    shutil.rmtree(tree_dir)
    print(f"   {dest.name}: {dest.stat().st_size:,} bytes")


def map_with(a, build):
    """The map again, after the release is published: GitHub makes the release's source zip itself,
    so its hash is only known once the release is out. Names the published installer and
    instructions (already in the release folder) and FILE, copied beside them, found by name."""
    out = a.out.resolve()
    name = a.name
    program = out / (f"{name}{EXE}" if EXE else f"{name}-setup")
    instructions = out / f"{name}.sieve"
    for f in (program, instructions):
        if not f.exists():
            fail(f"{f.name} is not in {out}: make the release first")
    src = a.map_with.resolve()
    if not src.is_file():
        fail(f"{src} is not a file")
    beside = out / src.name
    if beside != src:
        shutil.copy2(src, beside)
    sieve = build / f"sieve{EXE}"
    if not sieve.exists():
        fail(f"sieve{EXE} is not in {build}")
    step(f"the map again, with {src.name}")
    mapfile = out / f"{name}.map"
    run([sieve, "map", program, instructions, beside, "--name", name, "--seal", "--out", mapfile])
    text = subprocess.run([str(sieve), "map", str(mapfile)], capture_output=True, text=True, check=True).stdout
    for f in (program, instructions, beside):
        if sha256(f) not in text:
            fail(f"{mapfile.name} does not name {f.name}")
    if f"\theld\t{source.stat().st_size}\t{sha256(source)}\t{source.name}" not in text:
        fail(f"{mapfile.name} does not hold {source.name}")
    print(f"   {mapfile.name}: sealed, names the installer and the instructions, holds the source, all by SHA-256")
    files = [program, instructions, beside, mapfile]
    sums = "".join(f"{sha256(f)}  {f.name}\n" for f in files)
    (out / "SHA256SUMS.txt").write_text(sums, encoding="utf-8")
    print(sums, end="")
    print(f"\nDone. Upload the new {mapfile.name} to the release (its SHA-256 is above).")
    print(f"Keep {src.name} as it is: the map names those exact bytes.")


def newer_sources(build):
    """Source files changed since the build's programs were made (at most three named), so a
    release made from an old build says so rather than shipping the old programs quietly."""
    programs = [build / f"{p}{EXE}" for p in PROGRAMS if (build / f"{p}{EXE}").exists()]
    if not programs:
        return []
    built = min(p.stat().st_mtime for p in programs)
    newer = []
    for folder in ("core", "tools", "client", "third_party"):
        for f in (ROOT / folder).rglob("*"):
            if f.suffix in (".cpp", ".hpp", ".h", ".c") and f.stat().st_mtime > built + 2:
                newer.append(f)
    if (ROOT / "CMakeLists.txt").stat().st_mtime > built + 2:
        newer.append(ROOT / "CMakeLists.txt")
    newer.sort(key=lambda f: f.stat().st_mtime, reverse=True)
    return [f.relative_to(ROOT).as_posix() for f in newer[:3]]


def main():
    ap = argparse.ArgumentParser(description="Make Sieve's release files, in order, and check them.")
    ap.add_argument("--version", default=None, help="the release's version (default: CMakeLists.txt's, e.g. 0.13.1)")
    ap.add_argument("--build-dir", type=Path, default=None, help="the CMake build folder (default: the Release preset's)")
    ap.add_argument("--out", type=Path, default=ROOT / "release", help="where the release files go (default: release/)")
    ap.add_argument("--name", default="sieve", help="the published files' base name (default: sieve)")
    ap.add_argument("--skip-build", action="store_true", help="use the build as it is")
    ap.add_argument("--uncompressed", action="store_true", help="make the installers from the folder, not the 7z")
    ap.add_argument("--map-with", type=Path, default=None, metavar="FILE",
                    help="after publishing: make sieve.map again, naming the release's files and FILE too (GitHub's source zip)")
    a = ap.parse_args()

    if not a.version:
        import re
        m = re.search(r"project\(Sieve VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
        if not m:
            fail("no version given, and none found in CMakeLists.txt")
        a.version = m.group(1)
    print(f"Sieve {a.version}: the release goes to {a.out.resolve()}")
    build = (a.build_dir or default_build_dir()).resolve()
    if a.map_with:
        map_with(a, build)
        return
    # Building needs CMake on the PATH (Visual Studio's own is only on the PATH in its Developer
    # PowerShell). Without it, the build already made is used, if there is one.
    if not a.skip_build and not shutil.which("cmake"):
        if all((build / f"{p}{EXE}").exists() for p in PROGRAMS):
            print(f"CMake is not on the PATH: using the build already in {build} (build it in Release first)")
            a.skip_build = True
        else:
            fail(f"CMake is not on the PATH, and there is no build in {build}: build Release in Visual Studio first")
    if a.skip_build:
        stale = newer_sources(build)
        if stale:
            print(f"WARNING: the build in {build} is older than the source ({', '.join(stale)} changed since).")
            print("         Build Release in Visual Studio first, or the release carries the old programs.")
    name = a.name
    folder = f"Sieve-{a.version}"  # the release folder: what the 7z unpacks to, or what installs
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sz = find_7z()
    if not (ROOT.parent / "LICENSE").exists():
        fail(f"no LICENSE one folder up ({ROOT.parent / 'LICENSE'}): a release carries Potentia's licence")

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
    (stage / "tools").mkdir()
    for p in PROGRAMS:
        shutil.copy2(build / f"{p}{EXE}", stage / "tools" if p in TOOLS else stage)
    note = TOOLS_NOTE.format(exe=EXE)
    (stage / "tools" / "README.txt").write_bytes((note.replace("\n", "\r\n") if os.name == "nt" else note).encode("utf-8"))
    for d in FOLDERS:
        if (build / d).is_dir():
            shutil.copytree(build / d, stage / d)
    (stage / "maps").mkdir()  # the node graph's maps folder, empty: never a person's own maps
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

    # 6. The source, as Git has it at the last commit: the Sieve folder alone, 7-zipped.
    step("the source: sieve-source.7z")
    source = out / f"{name}-source.7z"
    source_7z(source, sz, out)

    # 7. The published map: naming the installer and the instructions (found beside it), and
    # holding the source (its bytes inside the map, checked against its SHA-256 whenever the map
    # is read), then sealed.
    step("the published map")
    mapfile = out / f"{name}.map"
    run([sieve, "map", program, instructions, "--name", name, "--out", mapfile])
    run([sieve, "map", mapfile, "--add", source, "--seal", "--out", mapfile])

    # 8. Checks: every published file installs back to what it carries, and the map verifies.
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
    # The layout: sieve in tools/ finds the installation's data folders in the folder above. Run
    # from tools/ itself, which has no data/ folder (one where it is run is looked at first), so a
    # registry found at all is the one above; the path itself is not compared, since Windows may
    # spell the same folder differently (case, short names, a drive mapped through a link).
    for what, folder_name in (("dicts", "dictionaries"), ("models", "models")):
        shown = subprocess.run([str(stage / "tools" / f"sieve{EXE}"), what], capture_output=True, text=True,
                               errors="replace", cwd=stage / "tools")
        if shown.returncode != 0 or "registry  " not in shown.stdout or "hash ok" not in shown.stdout:
            fail(f"tools/sieve{EXE} does not find {folder_name} in the folder above it (is the build older than the "
                 f"source? build Release in Visual Studio, then run this again):\n{shown.stdout}{shown.stderr}")
    print(f"   {folder}: hallway{EXE} at the top; sieve and sieve-install in tools/, finding the data above")
    text = subprocess.run([str(sieve), "map", str(mapfile)], capture_output=True, text=True, check=True).stdout
    for f in (program, instructions, source):
        if sha256(f) not in text:
            fail(f"{mapfile.name} does not name {f.name}")
    if f"\theld\t{source.stat().st_size}\t{sha256(source)}\t{source.name}" not in text:
        fail(f"{mapfile.name} does not hold {source.name}")
    print(f"   {mapfile.name}: sealed, names the installer and the instructions, holds the source, all by SHA-256")

    # 9. Sums and notes.
    step("SHA256SUMS.txt and the notes")
    published = [program, instructions, mapfile]
    sums = "".join(f"{sha256(f)}  {f.name}\n" for f in published)
    (out / "SHA256SUMS.txt").write_text(sums, encoding="utf-8")
    sizes = "\n".join(f"| `{f.name}` | {f.stat().st_size:,} bytes | `{sha256(f)}` |" for f in published)
    if a.uncompressed:
        where_note = f"The installers put the `{folder}` folder in place."
    else:
        where_note = (f"The installer (`{program.name}`, or `sieve-install` with `{instructions.name}`) unpacks the release "
                      f"into a folder of its own: choose `C:\\Games` and you get `C:\\Games\\{name}\\`. (`sieve install` "
                      f"and the File Locator give back `{archive.name}` itself, byte for byte; unpack it with 7-Zip.)")
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
- **`{mapfile.name}`**: a sealed map naming both, and holding this release's source,
  `{source.name}` (the Sieve folder as committed). Put it beside them and open it in the node
  graph (O) to see them verified; select the source and Go to it, or take it out with
  `sieve map`. Its own SHA-256 is above. The whole Potentia repository, Sieve included, is the
  release's "Source code" download.

{where_note}

## Getting started

Start `hallway{EXE}` in the installed folder. `tools\\` holds the command-line tool (`sieve{EXE}`: open a command prompt there and run `sieve help`) and the installer the hallway uses to make installer programs. Esc opens the pause menu (the File Locator, the node graph, the navigator); the full guide is the README in the repository.

To check a download, compare its SHA-256 with the table above: in PowerShell, `Get-FileHash {program.name}`; elsewhere, `sha256sum`.

Windows SmartScreen will warn that the programs are not signed; choose More info, then Run anyway. This release is built for 64-bit Windows 10 and 11; on Linux and macOS, build from the source.

Sieve is part of Potentia and licensed under the GNU Affero General Public License v3 (`potentia-license.txt`); the third-party licences are in `third_party_licenses/`.
"""
    (out / "RELEASE-NOTES.md").write_text(notes, encoding="utf-8")
    print(sums, end="")
    print(f"\nDone. Publish these three from {out}:\n  {program.name}\n  {instructions.name}\n  {mapfile.name} (holding {source.name})")
    print("with RELEASE-NOTES.md as the release's description.")


if __name__ == "__main__":
    main()
