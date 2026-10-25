"""sieve-install extracting archives of other kinds (the right-click menu's Extract on them).

  python ci/extract_check.py SIEVE_INSTALL WORK_FOLDER

Every archive is made here from the repository's own files (tests/manifest_fixture, and libarchive's
test archives in tests/archives), extracted by sieve-install unattended, and what comes out compared
with what went in. One layer at a time, as 7-Zip's: a .tar.gz gives its .tar.
"""

import bz2
import filecmp
import gzip
import hashlib
import io
import lzma
import os
import shutil
import subprocess
import sys
import tarfile
import zipfile

INSTALLER, WORK = sys.argv[1], sys.argv[2]
HERE = os.path.dirname(os.path.abspath(__file__))
SIEVE = os.path.dirname(HERE)
FIXTURE = os.path.join(SIEVE, "tests", "manifest_fixture")
ARCHIVES = os.path.join(SIEVE, "tests", "archives")


def run(*args, ok=True):
    r = subprocess.run([INSTALLER, *args, "--yes"], stdin=subprocess.DEVNULL, capture_output=True, text=True)
    if (r.returncode == 0) != ok:
        sys.exit(f"sieve-install {' '.join(args)}: exit {r.returncode}, expected {'0' if ok else 'a failure'}\n{r.stderr}")
    return r


def same_tree(a, b):
    c = filecmp.dircmp(a, b)
    if c.left_only or c.right_only or c.diff_files or c.funny_files:
        sys.exit(f"{a} and {b} differ: {c.left_only} {c.right_only} {c.diff_files}")
    for d in c.common_dirs:
        same_tree(os.path.join(a, d), os.path.join(b, d))


def fresh(name):
    p = os.path.join(WORK, name)
    shutil.rmtree(p, ignore_errors=True)
    return p


shutil.rmtree(WORK, ignore_errors=True)
os.makedirs(WORK)
text = open(os.path.join(FIXTURE, "a.txt"), "rb").read() * 50

# The names it is set up for: one a line, the kinds in pack.cpp's archive_kinds.
exts = subprocess.run([INSTALLER, "--archive-extensions"], capture_output=True, text=True, check=True).stdout.split()
for e in (".7z", ".zip", ".rar", ".tar", ".gz", ".tgz", ".xz", ".bz2", ".zst", ".cab", ".iso", ".lzh"):
    assert e in exts, e
print(f"  {len(exts)} names for the right-click menu")

# A zip with one top folder: into a folder of its own, the top folder left out (as a 7z's);
# extracted here, kept.
z = os.path.join(WORK, "fixture.zip")
with zipfile.ZipFile(z, "w", zipfile.ZIP_DEFLATED) as f:
    for root, dirs, files in os.walk(FIXTURE):
        for n in sorted(files):
            full = os.path.join(root, n)
            f.write(full, "manifest_fixture/" + os.path.relpath(full, FIXTURE).replace(os.sep, "/"))
run(z, "--to", fresh("zip_folder"))
same_tree(FIXTURE, os.path.join(WORK, "zip_folder"))
run(z, "--to", fresh("zip_here"), "--extract-here")
same_tree(FIXTURE, os.path.join(WORK, "zip_here", "manifest_fixture"))
# A second time over it is refused, and nothing is changed.
run(z, "--to", os.path.join(WORK, "zip_folder"), ok=False)

# The repository's 7z of the same folder.
run(os.path.join(SIEVE, "tests", "manifest_fixture.7z"), "--to", fresh("7z"))
same_tree(FIXTURE, os.path.join(WORK, "7z"))

# A tar, as it is; a .tar.gz, .tgz, .tar.bz2 and .tar.xz each give the .tar (one layer only).
tar = os.path.join(WORK, "fixture.tar")
with tarfile.open(tar, "w", format=tarfile.PAX_FORMAT) as t:
    t.add(FIXTURE, "manifest_fixture")
    link = tarfile.TarInfo("manifest_fixture/link")
    link.type, link.linkname = tarfile.SYMTYPE, "a.txt"
    t.addfile(link)
tar_bytes = open(tar, "rb").read()
r = run(tar, "--to", fresh("tar"))
same_tree(FIXTURE, os.path.join(WORK, "tar"))  # the link left out
for name, data in (("fixture.tar.gz", gzip.compress(tar_bytes)), ("fixture.tgz", gzip.compress(tar_bytes)),
                   ("fixture.tar.bz2", bz2.compress(tar_bytes)), ("fixture.tar.xz", lzma.compress(tar_bytes))):
    p = os.path.join(WORK, name)
    open(p, "wb").write(data)
    out = fresh(name + "_out")
    run(p, "--to", out)
    assert os.listdir(out) == ["fixture.tar"], (name, os.listdir(out))
    assert open(os.path.join(out, "fixture.tar"), "rb").read() == tar_bytes, name

# One file compressed: its name without the extension.
singles = [("a.txt.gz", gzip.compress(text)), ("a.txt.bz2", bz2.compress(text)), ("a.txt.xz", lzma.compress(text)),
           ("a.txt.lzma", lzma.compress(text, format=lzma.FORMAT_ALONE))]
try:
    import lz4.frame
    singles.append(("a.txt.lz4", lz4.frame.compress(text)))
except ImportError:
    pass
try:
    import brotli
    singles.append(("a.txt.br", brotli.compress(text)))
except ImportError:
    pass
try:
    from compression import zstd  # Python 3.14
    singles.append(("a.txt.zst", zstd.compress(text)))
except ImportError:
    if shutil.which("zstd"):
        singles.append(("a.txt.zst", subprocess.run(["zstd", "-c"], input=text, capture_output=True, check=True).stdout))
for name, data in singles:
    p = os.path.join(WORK, name)
    open(p, "wb").write(data)
    out = fresh(name + "_out")
    run(p, "--to", out)
    assert open(os.path.join(out, "a.txt"), "rb").read() == text, name
print(f"  {len(singles)} single streams: {', '.join(n.split('.', 2)[2] for n, _ in singles)}")

# libarchive's own test archives: RAR 4 and 5, cab (MSZIP and LZX), LHA.
for name, sums in (("test_read_format_cab_1.cab", {"dir1/file1": "d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b",
                                                   "dir2/file2": "60f47caf717b06cf21b3bbb7775e49269a1b5cd6b94bba62da29a2ecb048ccf2",
                                                   "empty": "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"}),
                   ("test_read_format_cab_2.cab", {"zero": "8ec8d677b0280efeddd04d0b25d8b80fa50a9825382fa93a17d61c4ca002b1ad",
                                                   "dir1/file1": "d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b"}),
                   ("test_read_format_rar_noeof.rar", {"test.txt": "5a5f16e01faf8adf92eb4499a2d3e93010c4b41dbb7f698f4a8466d9f58e6dd2"}),
                   ("test_read_format_rar5_compressed.rar", {"test.bin": "588870a2dade35c2650fbb7898c9a9c7f21fce7c281198604e8d0c9737f2c375"}),
                   ("test_read_format_lha_header3.lzh", {"file1": "d0c504f06bbd64d183524eb35e5482ee5d966d456b905a24147165b2904d301b",
                                                         "file2": "60f47caf717b06cf21b3bbb7775e49269a1b5cd6b94bba62da29a2ecb048ccf2"})):
    out = fresh(name + "_out")
    run(os.path.join(ARCHIVES, name), "--to", out, "--extract-here")
    for p, h in sums.items():
        assert hashlib.sha256(open(os.path.join(out, p), "rb").read()).hexdigest() == h, (name, p)

# ar and cpio (newc), written here by hand.
files = [("a.txt", open(os.path.join(FIXTURE, "a.txt"), "rb").read()), ("Zebra.txt", open(os.path.join(FIXTURE, "Zebra.txt"), "rb").read())]
ar = b"!<arch>\n"
for n, d in files:
    ar += f"{n + '/':<16}{0:<12}{0:<6}{0:<6}{100644:<8}{len(d):<10}`\n".encode() + d + (b"\n" if len(d) % 2 else b"")
cpio = b""
for i, (n, d) in enumerate(files + [("TRAILER!!!", b"")]):
    nm = n.encode() + b"\0"
    mode = 0o100644 if n != "TRAILER!!!" else 0
    cpio += b"070701" + b"".join(b"%08X" % v for v in (i + 1, mode, 0, 0, 1, 0, len(d), 0, 0, 0, 0, len(nm), 0)) + nm
    cpio += b"\0" * (-len(cpio) % 4) + d + b"\0" * (-len(d) % 4)
for name, data in (("two.ar", ar), ("two.cpio", cpio)):
    p = os.path.join(WORK, name)
    open(p, "wb").write(data)
    out = fresh(name + "_out")
    run(p, "--to", out)
    for n, d in files:
        assert open(os.path.join(out, n), "rb").read() == d, (name, n)

# Damaged: refused, nothing written.
bad = os.path.join(WORK, "damaged.zip")
open(bad, "wb").write(open(z, "rb").read()[:300])
run(bad, "--to", os.path.join(WORK, "damaged_out"), ok=False)
assert not os.path.exists(os.path.join(WORK, "damaged_out"))
print("ok")
