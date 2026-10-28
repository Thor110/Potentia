#!/usr/bin/env python3
r"""Sieve's release, made in the one order that works (docs/SIEVE-INSTALL-USAGE.md, HANDOFF).

Every file of a release is named by its hash, and every hash depends on the step before it, so the
order matters and is easy to get wrong by hand:

  1. (Nothing: the source's own steps are 6 and 7.)
  2. The build (Release), with potentia-license.txt beside the programs.
  3. The release folder: only what the program uses (the programs, their data folders, an empty
     maps folder, the licences), staged by name, never the build folder's own clutter, and from
     the source the right-click menu (context\) and the web extension with its host's scripts
     (web\), whose scripts find the programs in tools\. The hallway
     is the one program at the top; sieve and sieve-install go in tools\, with a note saying what
     they are, so nobody starts the wrong program first. (A build folder keeps them together; the
     programs look in both places.) No map is
     shipped inside it.
  4. (Nothing: the installer packs the folder itself now, at the newest version. --seven-zip
     compresses it to a solid 7z first and carries that instead, as releases did before, for
     comparing.)
  5. From the folder: Sieve instructions (sieve.sieve, for anyone with Sieve) and an installer
     program (sieve.exe on Windows, sieve-setup elsewhere, for anyone), made by the release's own
     sieve.
  6. The source: sieve-source.7z, the Sieve folder alone as the last Git commit has it (git
     archive), so it holds the committed source, this script included, and none of the build's
     clutter.
  7. source.sieve: the same source tree as the 7z, as Sieve instructions of the newest version
     (sieve locate --items), installing as the Sieve folder (27 October 2026, Edward: a plain
     .sieve of the source folder; it was a sealed map holding the 7z before, so no smaller than it).
  8. Checks: each installer installed into a scratch folder and compared with the release folder
     (or, with --seven-zip, the archive), byte for byte, and source.sieve installed back to the
     source tree.
  8b. metrics.txt: how much of each line the reference filters remove (data/metrics/
     reference-filters.ini), as the setup menu counts it, by the build's hallway (--metrics).
  8c. The build's record (IDEAS §19.50, §19.71): the build's unit suite run and timed, then
     tools/log_metrics.py writes a record (kind release) to metrics/builds and rewrites
     metrics/index.tsv, with the machine it ran on. Commit them with the release, so every release
     is measured on the record. --no-record leaves it out.
  9. SHA256SUMS.txt and a release-notes draft.

Published (Edward's naming): sieve.exe, sieve.sieve, source.sieve (the source as Sieve
instructions) and sieve-source.7z (the same source as a 7z, the baseline to compare it with), with
metrics.txt. (Until 27 October 2026 source.sieve was a sealed map naming the other two and holding
the 7z; until 22 October, sieve.map.) The version is in the release folder they install
(Sieve-<version>) and in the notes, not in the names. GitHub adds the source zip of the tagged commit
by itself.

Usage (from the repository root, on the machine the release is for):

  tools\make_release.bat                            # Windows: double-click it; everything by default
  python tools/make_release.py                      # the version from CMakeLists.txt, into release/
  python tools/make_release.py --version 0.13.75 --build-dir out/build/x64-Release --out release
  python tools/make_release.py --version 0.13.75 --skip-build      # the build is already made
  python tools/make_release.py --version 0.13.75 --seven-zip       # 7z first, as before v4, to compare

The instructions and the program carry the release folder packed at the newest version (sieve
locate --items; until 22 October 2026 sieve-manifest-v4), so they install straight to a runnable
folder and are smaller than the 7z was.
--seven-zip makes them from a solid 7z of the folder instead, as releases were made before v4, so
the two can be weighed against each other on the same build.

It needs Python 3.8+, CMake, and 7-Zip (7z on the PATH, or in Program Files) for the source archive. It changes
nothing in the repository but the build's record in metrics/ (step 8c). (--with-source, which rewrote a shipped data/maps/sieve.map to name a source
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
FOLDERS = ["dictionaries", "models", "lang", "prompts", "fonts", "meshes", "vault", "filters", "training", "third_party_licenses"]
FILES = ["potentia-license.txt"]
# From the source folder, beside the programs (24 October 2026): the right-click menu for .sieve
# files and the web extension with its host's scripts. Their scripts find tools\sieve-install and
# tools\sieve in the folder above them, so nothing needs a path given. The test site stays in the
# repository (it is built from the repository's pictures, with Python).
EXTRAS = ["context/README.md", "context/install_context.bat", "context/uninstall_context.bat",
          "web/extension", "web/host/install_host.bat", "web/host/uninstall_host.bat", "web/host/install_host.sh",
          "web/crawl_site.py", "web/crawl_site.bat"]
WEB_NOTE = """Sieve on the web: the extension

A Chrome extension that shows the .sieve files a page links to, each checked against
the hash its link states (integrity="sha256-..."), and decoded by Sieve on this computer.

Setting it up, once, for your user only:

  1. Register Sieve as the extension's host: run host\\install_host.bat (it finds
     tools\\sieve{exe} in the installation). host\\uninstall_host.bat undoes it.
     On Linux or a Mac: host/install_host.sh.
  2. Load the extension: open chrome://extensions, turn on Developer mode,
     choose Load unpacked, and pick the extension folder beside this file.

A site built by tools\\sieve{exe} site build has a table, index.sieve: the extension remembers
it on your first visit and shows a page of that site only once it is the one the table names (its
settings page lists the sites; right-click the extension's icon, Options). crawl_site.bat checks a
site on its server against its table, now and then or on a schedule.

Making a site's pictures and pages into .sieve files: tools\\sieve{exe} site build (sieve help
site). The full guide, and the test site, are in the repository: Sieve/web/README.md.
"""


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
    """The Sieve folder's source as sieve-source.7z, with Sieve/ as its one top folder; the tree it
    was made from is returned (work/source/Sieve), for source.sieve, and removed by the caller. From Git when
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
    print(f"   {dest.name}: {dest.stat().st_size:,} bytes")
    return tree_dir / "Sieve"


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
    ap.add_argument("--version", default=None, help="the release's version (default: CMakeLists.txt's, e.g. 0.13.75)")
    ap.add_argument("--build-dir", type=Path, default=None, help="the CMake build folder (default: the Release preset's)")
    ap.add_argument("--out", type=Path, default=ROOT / "release", help="where the release files go (default: release/)")
    ap.add_argument("--name", default="sieve", help="the published files' base name (default: sieve)")
    ap.add_argument("--skip-build", action="store_true", help="use the build as it is")
    ap.add_argument("--seven-zip", action="store_true", help="carry a solid 7z of the folder (as before v4), not the folder packed")
    ap.add_argument("--no-record", action="store_true", help="leave out step 8c, the build's record in metrics/")
    a = ap.parse_args()

    if not a.version:
        import re
        m = re.search(r"project\(Sieve VERSION ([0-9.]+)", (ROOT / "CMakeLists.txt").read_text(encoding="utf-8"))
        if not m:
            fail("no version given, and none found in CMakeLists.txt")
        a.version = m.group(1)
    print(f"Sieve {a.version}: the release goes to {a.out.resolve()}")
    build = (a.build_dir or default_build_dir()).resolve()
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
    folder = f"Sieve-{a.version}"  # the release folder: what installs (or what the 7z unpacks to)
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
    for rel in EXTRAS:
        src, dst = ROOT / rel, stage / rel
        if not src.exists():
            fail(f"{rel} is not in {ROOT}")
        dst.parent.mkdir(parents=True, exist_ok=True)
        if src.is_dir():
            shutil.copytree(src, dst, ignore=shutil.ignore_patterns("__pycache__"))
        else:
            shutil.copy2(src, dst)
    web_note = WEB_NOTE.format(exe=EXE)
    (stage / "web" / "README.txt").write_bytes((web_note.replace("\n", "\r\n") if os.name == "nt" else web_note).encode("utf-8"))
    for f in FILES:
        if not (build / f).exists():
            fail(f"{f} is not in {build}")
        shutil.copy2(build / f, stage)

    # 4. With --seven-zip only: compressed first, as before v4.
    archive = out / f"{name}.7z"
    if a.seven_zip:
        step("compressed first: the 7z")
        seven_zip(sz, archive, [folder], cwd=out)
    carried = archive if a.seven_zip else stage

    # 5. The instructions and the program, made by the release's own sieve.
    step("Sieve instructions and the installer program")
    instructions = out / f"{name}.sieve"
    program = out / (f"{name}{EXE}" if EXE else f"{name}-setup")  # sieve.exe; elsewhere not the release folder's name
    # --items: the newest version (until 22 October 2026 this was left out, so releases were v4).
    run([sieve, "locate", carried, "--installer", instructions, "--program", program, "--compare", "--items"])

    # 6. The source, as Git has it at the last commit: the Sieve folder alone, 7-zipped.
    step("the source: sieve-source.7z")
    source = out / f"{name}-source.7z"
    tree = source_7z(source, sz, out)

    # 7. source.sieve: the same tree as Sieve instructions of the newest version, installing as the
    # Sieve folder (the 7z beside it is the baseline it is compared with).
    step("the source as Sieve instructions: source.sieve")
    source_sieve = out / "source.sieve"
    run([sieve, "locate", tree, "--installer", source_sieve, "--items"])
    print(f"   {source_sieve.name}: {source_sieve.stat().st_size:,} bytes ({source.name}: {source.stat().st_size:,})")

    # 8. Checks: every published file installs back to what it carries.
    step("checks")
    with tempfile.TemporaryDirectory() as tmp:
        for i, f in enumerate((instructions, program)):
            dest = Path(tmp) / f"install{i}"
            run([sieve, "install", f, "--to", dest])
            if not a.seven_zip:
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
    with tempfile.TemporaryDirectory() as tmp:
        dest = Path(tmp) / "source"
        run([sieve, "install", source_sieve, "--to", dest])
        if not same_tree(tree, dest):
            fail(f"{source_sieve.name} did not install the source tree as it is")
    shutil.rmtree(tree.parent)
    print(f"   {source_sieve.name}: installs back to the source tree, every file byte for byte")

    # 8b. The metrics list (IDEAS §19.12): how much of each line the reference filters remove, as
    # the setup menu counts it, written beside the installer. The build's hallway counts it (the
    # release folder is packed and checked already, and is left as it is).
    step("metrics.txt")
    metrics = out / "metrics.txt"
    reference = ROOT / "data" / "metrics" / "reference-filters.ini"
    run([build / f"hallway{EXE}", "--metrics", metrics, "--filters", reference,
         "--metrics-text", ROOT / "data" / "metrics" / "reference-text.txt"], cwd=build)
    if not metrics.exists():
        fail("the hallway wrote no metrics.txt")
    metrics_text = metrics.read_text(encoding="utf-8")

    # 8c. The build's record: every release measured, on the machine that made it (IDEAS §19.71).
    # The unit suite is run here, timed, so its time is a figure of this machine; a failure stops
    # the release, since a release must pass its own tests.
    if not a.no_record:
        step("the build's record (metrics/)")
        tests = build / f"sieve_tests{EXE}"
        if not tests.exists():
            fail(f"sieve_tests{EXE} is not in {build}: build it, or pass --no-record")
        import time
        with tempfile.TemporaryDirectory() as tmp:
            unit_out = Path(tmp) / "unit.txt"
            began = time.monotonic()
            with open(unit_out, "w", encoding="utf-8") as f:
                tested = subprocess.run([str(tests), "tests"], cwd=ROOT, stdout=f, stderr=subprocess.STDOUT)
            seconds = time.monotonic() - began
            print(unit_out.read_text(encoding="utf-8", errors="replace").rstrip().splitlines()[-1])
            if tested.returncode != 0:
                fail("the unit tests failed: no release is made from a build that fails them")
            run([sys.executable, ROOT / "tools" / "log_metrics.py", "--unit", f"{build.name}={unit_out}",
                 "--hallway", build / f"hallway{EXE}", "--set", "kind=release", "--set", f"unit_seconds={seconds:.1f}"], cwd=ROOT)

    # 9. Sums and notes.
    step("SHA256SUMS.txt and the notes")
    published = [program, instructions, source_sieve, source, metrics]
    sums = "".join(f"{sha256(f)}  {f.name}\n" for f in published)
    (out / "SHA256SUMS.txt").write_text(sums, encoding="utf-8")
    sizes = "\n".join(f"| `{f.name}` | {f.stat().st_size:,} bytes | `{sha256(f)}` |" for f in published)
    if not a.seven_zip:
        where_note = (f"The installer (`{program.name}`, or `sieve-install` with `{instructions.name}`) puts the `{folder}` "
                      f"folder in place, every file checked against its SHA-256.")
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
- **`{source_sieve.name}`**: this release's source (the Sieve folder as committed) as Sieve
  instructions: `sieve install {source_sieve.name} --to FOLDER` (or `sieve-install`, or the File
  Locator) puts the Sieve folder there.
- **`{source.name}`**: the same source as a 7z, to compare with. The whole Potentia repository,
  Sieve included, is the release's "Source code" download.

{where_note}

## How much each line's filters remove

`{metrics.name}`: every line under Sieve's reference filters (`data/metrics/reference-filters.ini`,
the hard filters that count together), counted as the setup menu counts them.

```
{metrics_text}```

## Getting started

Start `hallway{EXE}` in the installed folder. `tools\\` holds the command-line tool (`sieve{EXE}`: open a command prompt there and run `sieve help`) and the installer the hallway uses to make installer programs. Esc opens the pause menu (the File Locator, the node graph, the navigator); the full guide is the README in the repository.

Two things to set up if you want them, each for your user only and undone by its uninstall script:
- **The right-click menu:** `context\\install_context.bat` adds a Sieve submenu to every `.sieve` file, with Extract here and Extract to a folder of its name (`context\\README.md`).
- **The web extension:** `web\\host\\install_host.bat`, then load `web\\extension` in `chrome://extensions` (Developer mode, Load unpacked): pages' `.sieve` pictures and links, checked against the hash each link states (`web\\README.txt`).

To check a download, compare its SHA-256 with the table above: in PowerShell, `Get-FileHash {program.name}`; elsewhere, `sha256sum`.

Windows SmartScreen will warn that the programs are not signed; choose More info, then Run anyway. This release is built for 64-bit Windows 10 and 11; on Linux and macOS, build from the source.

Sieve is part of Potentia and licensed under the GNU Affero General Public License v3 (`potentia-license.txt`); the third-party licences are in `third_party_licenses/`.
"""
    (out / "RELEASE-NOTES.md").write_text(notes, encoding="utf-8")
    print(sums, end="")
    print(f"\nDone. Publish these five from {out}:\n  {program.name}\n  {instructions.name}\n  {source_sieve.name}\n  {source.name}\n  {metrics.name}")
    print("with RELEASE-NOTES.md as the release's description.")


if __name__ == "__main__":
    main()
