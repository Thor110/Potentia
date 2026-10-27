#!/usr/bin/env python3
r"""Builds the web test site (web/README.md) from src/, with the Sieve given, into site/.

The work is Sieve's own, `sieve site build`, which builds any site the same way (sieve help site):
every picture made into a .sieve, the pages too but for index.html and gallery.html, every link to
one given that file's hash, and index.sieve, the table of every file. What this adds is the test's
own: before the build, in a copy of src/,

  - {{home}} becomes a link back to the website's Sieve Secure page (--home), or nothing;
  - pages.sieve: the pages' sources and style.css, many files in one .sieve, which the extension's
    viewer shows as their listing;
  - directory.sieve: four of the pictures in folders within folders, and a page among them showing
    them by their paths there, for the viewer's directory listing (Directory Listing Testing);
  - pictures/tampered.sieve: Vitruvian Man's .sieve (made as the build makes it) with one byte
    changed after its hash was taken, and that hash written into the gallery's {{tampered}}.

The links meant to be wrong carry data-sieve-keep, so the build leaves them as they are. Last,
`sieve site check` reads the built site as the extension would.

  web\test-site\serve_site.bat           (Windows: builds it, serves it, opens it in the browser)
  python web/test-site/make_site.py out\build\x64-Release\sieve.exe
  python -m http.server 8000 --directory web/test-site/site      (then http://localhost:8000/)

For the website (GitHub Pages), beside Pages/secure.html, with a link back to it:
  python Sieve/web/test-site/make_site.py SIEVE --out Pages/secure --home ../secure.html
"""

import argparse
import base64
import hashlib
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "src"
PLAIN = ["gallery.html"]   # kept a plain page (its slideshow is a script); index.html always is


# directory.sieve's pictures: (its folder, a picture of src/pictures).
DIRECTORY = [("drawings", "vitruvian.jpg"), ("hallway", "door.jpg"), ("hallway", "book.jpg"), ("hallway/menus", "filters.jpg")]

DIRECTORY_PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>Directory Listing Testing</title>
<style>
body { background: #0b0b0b; color: #d8d8d8; font: 15px/1.5 sans-serif; margin: 2em; }
img { max-width: 360px; border: 1px solid #333; display: block; margin: 0.5em 0 1.5em; }
a { color: #7fd0ff; }
</style>
</head>
<body>
<h1>Directory Listing Testing</h1>
<p>This page is one of the files of <code>directory.sieve</code>. Its pictures are others, named by
their paths in it, so each is taken from the same <code>.sieve</code> and checked against the same hash.</p>
<p>drawings/vitruvian.jpg</p><img src="drawings/vitruvian.jpg" alt="Vitruvian Man">
<p>hallway/door.jpg</p><img src="hallway/door.jpg" alt="A door in the hallway">
<p>hallway/book.jpg</p><img src="hallway/book.jpg" alt="A book in the hallway">
<p>hallway/menus/filters.jpg</p><img src="hallway/menus/filters.jpg" alt="The setup menu's filters">
<p><a href="hallway/menus/filters.jpg">The setup menu's picture alone</a>, opened from the same <code>.sieve</code>.</p>
</body>
</html>
"""


def sri(path):
    return "sha256-" + base64.b64encode(hashlib.sha256(path.read_bytes()).digest()).decode()


def run(sieve, *args):
    r = subprocess.run([str(sieve), *map(str, args)], capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        sys.exit(f"sieve {' '.join(map(str, args))} failed:\n{r.stdout}{r.stderr}")
    return r


def main():
    ap = argparse.ArgumentParser(description="Build the web test site.")
    ap.add_argument("sieve", type=Path, help="the sieve program (sieve.exe)")
    ap.add_argument("--out", type=Path, default=HERE / "site")
    ap.add_argument("--home", default="", help="a link back from the pages (the website's Pages/secure.html: ../secure.html)")
    a = ap.parse_args()
    sieve, out = a.sieve.resolve(), a.out.resolve()
    if not sieve.exists() and sieve.with_name(sieve.name + ".exe").exists():
        sieve = sieve.with_name(sieve.name + ".exe")   # named as bash names it, without .exe (Windows)
    if not sieve.exists():
        sys.exit(f"{sieve} is not there")
    if out.exists():
        shutil.rmtree(out)   # this script's own output (sieve site build replaces only a site it built)

    with tempfile.TemporaryDirectory() as tmp:
        stage = Path(tmp) / "secure"
        shutil.copytree(SRC, stage)
        home = f'<a href="{a.home}">Sieve Secure</a>' if a.home else ""
        for page in stage.glob("*.html"):
            page.write_text(page.read_text(encoding="utf-8").replace("{{home}}", home), encoding="utf-8", newline="\n")

        # Every page in one .sieve (many files, so refused).
        pages = Path(tmp) / "pages"
        pages.mkdir()
        for f in sorted(stage.glob("*.html")) + [stage / "style.css"]:
            shutil.copy2(f, pages / f.name)
        run(sieve, "locate", pages, "--installer", stage / "pages.sieve", "--search-per-file", "1")

        # A small tree of pictures in one .sieve, for the viewer's directory listing: folders within
        # folders, and a page among them whose pictures and links are other files of the same .sieve.
        tree = Path(tmp) / "directory"
        for folder, name in DIRECTORY:
            (tree / folder).mkdir(parents=True, exist_ok=True)
            shutil.copy2(SRC / "pictures" / name, tree / folder / name)
        (tree / "index.html").write_text(DIRECTORY_PAGE, encoding="utf-8", newline="\n")
        run(sieve, "locate", tree, "--installer", stage / "directory.sieve", "--search-per-file", "1")

        # Vitruvian Man made as the build makes it, its hash taken, then one byte changed.
        alone = Path(tmp) / "alone"
        alone.mkdir()
        shutil.copy2(SRC / "pictures" / "vitruvian.jpg", alone / "vitruvian.jpg")
        (alone / "index.html").write_text('<img src="vitruvian.jpg">\n', encoding="utf-8")   # only what a page names is made
        run(sieve, "site", "build", alone, "--out", Path(tmp) / "alone-built")
        good = Path(tmp) / "alone-built" / "vitruvian.sieve"
        bad = bytearray(good.read_bytes())
        bad[len(bad) // 2] ^= 0x01
        (stage / "pictures" / "tampered.sieve").write_bytes(bytes(bad))
        gallery = stage / "gallery.html"
        gallery.write_text(gallery.read_text(encoding="utf-8").replace("{{tampered}}", sri(good)), encoding="utf-8", newline="\n")

        r = run(sieve, "site", "build", stage, "--out", out, "--sieve-pages", "--plain", ",".join(PLAIN))
        sys.stderr.write(r.stderr)
        root = r.stdout.strip()
        if (out / "pictures" / "vitruvian.sieve").read_bytes() != good.read_bytes():
            print("note: the site's vitruvian.sieve came out unlike the one tampered with (a search cut short by time?);"
                  " the tampered picture is still refused, but is no longer that file with one byte changed")

    check = subprocess.run([str(sieve), "site", "check", str(out)], capture_output=True, text=True, errors="replace")
    sys.stderr.write(check.stdout + check.stderr)
    if check.returncode != 0:
        sys.exit("the built site does not match the hashes it states")

    for f in sorted(out.rglob("*")):
        if f.is_file():
            print(f"{f.relative_to(out).as_posix():28} {f.stat().st_size:>9,} bytes  {sri(f)}")
    print(f"\n{root}\nBuilt {out}. Serve it with:\n  python -m http.server 8000 --directory \"{out}\"\nand open http://localhost:8000/ in Chrome with the extension loaded.")


if __name__ == "__main__":
    main()
