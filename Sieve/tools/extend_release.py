#!/usr/bin/env python3
r"""Sieve's release extensions: what the release leaves out, each as Sieve instructions of its own.

The release carries as little as it can (Edward, 9 November 2026: "the release should carry as little
as possible realistically ... We could perhaps package stuff like this separately as another sieve
file to be extracted to the Sieve directory"). What it leaves out is here, each a .sieve that installs
into a Sieve folder (the release's, Sieve-<version>, or this repository's Sieve folder) beside what is
there, and nothing else: install it with sieve-install, choosing the Sieve folder, or with
`tools\sieve install voice.sieve --to <the Sieve folder>`. Anyone with the whole repository has it all
already.

  voice.sieve   Sieve's own voice (Text to Voice, `sieve speak`): voice\lj-speech, with its hour of
                LJ Speech's recordings (public domain) as FLAC, its pronouncing dictionary (CMUdict,
                its licence beside it) and its alignment. Without it, Text to Voice speaks in
                Windows' own voice.

Usage (from the repository root, after a Release build; make_release is separate and unchanged):

  tools\extend_release.bat                          # Windows: double-click it; every extension
  python tools/extend_release.py                    # into release/
  python tools/extend_release.py --only voice --build-dir out/build/x64-Release --out release

Each is made by the build's own sieve at the newest version (sieve locate --items), then checked:
installed into a scratch folder and compared with what it was made from, byte for byte. The voice
is also made to say a sentence from where it was installed.
"""
import argparse
import contextlib
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
EXE = ".exe" if os.name == "nt" else ""

# Each extension: its name (the .sieve's), and what goes in it, as (where it lies in a Sieve folder,
# where it is in the repository). A folder is taken whole.
EXTENSIONS = {
    "voice": [
        ("voice/lj-speech/README.md", "data/voice/lj-speech/README.md"),
        ("voice/lj-speech/alignment.tsv", "data/voice/lj-speech/alignment.tsv"),
        ("voice/lj-speech/cmudict.dict", "data/voice/lj-speech/cmudict.dict"),
        ("voice/lj-speech/cmudict-LICENSE.txt", "data/voice/lj-speech/cmudict-LICENSE.txt"),
        ("voice/lj-speech/clips", "data/anchors/public-domain/voice/lj-speech/clips"),
        ("voice/lj-speech/RECORDINGS.md", "data/anchors/public-domain/voice/lj-speech/README.md"),
        ("voice/lj-speech/metadata.csv", "data/anchors/public-domain/voice/lj-speech/metadata.csv"),
    ],
}


def fail(msg):
    print(f"extend_release: {msg}", file=sys.stderr)
    sys.exit(1)


def run(cmd, cwd=None):
    print("   $ " + " ".join(str(c) for c in cmd))
    r = subprocess.run([str(c) for c in cmd], cwd=cwd)
    if r.returncode != 0:
        fail(f"{Path(str(cmd[0])).name} failed ({r.returncode})")


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


@contextlib.contextmanager
def scratch_dir():
    """A temporary folder, removed afterwards as far as it can be (on Windows a file may be held a moment)."""
    tmp = tempfile.mkdtemp()
    try:
        yield Path(tmp)
    finally:
        shutil.rmtree(tmp, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    preset = "x64-Release" if os.name == "nt" else "linux-release"
    ap.add_argument("--build-dir", type=Path, default=ROOT / "out" / "build" / preset, help="the Release build (its sieve makes them)")
    ap.add_argument("--out", type=Path, default=ROOT / "release", help="where the .sieve files go (default: release/)")
    ap.add_argument("--only", action="append", choices=sorted(EXTENSIONS), help="just this extension (may be given again)")
    a = ap.parse_args()

    sieve = a.build_dir / f"sieve{EXE}"
    if not sieve.exists():
        sieve = a.build_dir / "Release" / f"sieve{EXE}"
    if not sieve.exists():
        fail(f"no sieve{EXE} in {a.build_dir}: build Release first, or give --build-dir")
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=True)

    for name in a.only or sorted(EXTENSIONS):
        print(f"\n== {name}.sieve")
        with scratch_dir() as tmp:
            # Staged as it lies in a Sieve folder.
            stage = tmp / name
            for dst, src in EXTENSIONS[name]:
                s, d = ROOT / src, stage / dst
                if not s.exists():
                    fail(f"{src} is not in {ROOT}")
                d.parent.mkdir(parents=True, exist_ok=True)
                if s.is_dir():
                    shutil.copytree(s, d)
                else:
                    shutil.copy2(s, d)
            files = [p for p in stage.rglob("*") if p.is_file()]
            print(f"   {len(files)} files, {sum(p.stat().st_size for p in files):,} bytes")
            target = out / f"{name}.sieve"
            run([sieve, "locate", stage, "--installer", target, "--items"])
            print(f"   {target.name}: {target.stat().st_size:,} bytes")
            # Checked: installed into an empty folder, it gives back what it was made from.
            back = tmp / "installed"
            run([sieve, "install", target, "--to", back])
            if not same_tree(stage, back):
                fail(f"{target.name} did not install what it was made from, byte for byte")
            print(f"   {target.name}: installs back to what it was made from, every file byte for byte")
            if name == "voice":
                said = tmp / "said.wav"
                run([sieve, "speak", "The voice is installed.", "--voice", back / "voice" / "lj-speech", "--out", said])
                if said.stat().st_size <= 44:
                    fail("the installed voice said nothing")
                print("   the installed voice speaks")
            (out / f"{name}.sieve.sha256").write_text(f"{sha256(target)}  {target.name}\n", encoding="utf-8")
    print(f"\nDone: in {out}. Install one into a Sieve folder with sieve-install (choose the Sieve folder),")
    print(f"or: tools{os.sep}sieve{EXE} install <name>.sieve --to <the Sieve folder>")


if __name__ == "__main__":
    main()
