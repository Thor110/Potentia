#!/usr/bin/env python3
r"""Builds the web test site (web/README.md) from src/, with the Sieve given, into site/:

  - every picture in src/pictures as images/<name>.sieve;
  - the four pages' sources and style.css as one pages.sieve (many files, which the extension
    refuses to show);
  - about.html and notes.html as about.sieve and notes.sieve, pages held in .sieve files;
  - index.html and gallery.html as plain pages, with style.css;
  - images/tampered.sieve: images/vitruvian.sieve with one byte changed after its hash was taken.

Every {{sri:PATH}} in a page becomes the integrity attribute of the file at PATH in site/
("sha256-" and the base64 of its SHA-256), so each page is made after the files it names; two pages
that named each other's could not be made at all, and are refused.

  web\test-site\serve_site.bat           (Windows: builds it, serves it, opens it in the browser)
  python web/test-site/make_site.py out\build\x64-Release\sieve.exe
  python -m http.server 8000 --directory web/test-site/site      (then http://localhost:8000/)

For the website (GitHub Pages), beside Pages/secure.html, with a link back to it:
  python Sieve/web/test-site/make_site.py SIEVE --out Pages/secure --home ../secure.html
"""

import argparse
import base64
import hashlib
import re
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SRC = HERE / "src"
PAGES_AS_SIEVE = ["about.html", "notes.html"]   # held in .sieve files
PAGES_PLAIN = ["index.html", "gallery.html"]   # served as they are
SLOT = re.compile(r"\{\{sri:([^}]+)\}\}")


def sri(path):
    return "sha256-" + base64.b64encode(hashlib.sha256(path.read_bytes()).digest()).decode()


def sieve_of(sieve, source, target):
    # The newest instructions; a second's search a file is plenty for a test.
    r = subprocess.run([str(sieve), "locate", str(source), "--installer", str(target), "--items", "--search-per-file", "1"],
                       capture_output=True, text=True, errors="replace")
    if r.returncode != 0 or not target.exists():
        sys.exit(f"sieve could not make {target.name}:\n{r.stdout}{r.stderr}")


def main():
    ap = argparse.ArgumentParser(description="Build the web test site.")
    ap.add_argument("sieve", type=Path, help="the sieve program (sieve.exe)")
    ap.add_argument("--out", type=Path, default=HERE / "site")
    ap.add_argument("--home", default="", help="a link back from the plain pages (the website's Pages/secure.html: ../secure.html)")
    a = ap.parse_args()
    sieve, out = a.sieve.resolve(), a.out.resolve()
    if not sieve.exists():
        sys.exit(f"{sieve} is not there")
    if out.exists():
        shutil.rmtree(out)
    (out / "images").mkdir(parents=True)

    # The pictures, one by one and all together, and one tampered with.
    for pic in sorted((SRC / "pictures").glob("*.jpg")):
        sieve_of(sieve, pic, out / "images" / (pic.stem + ".sieve"))
    with tempfile.TemporaryDirectory() as tmp:
        pages = Path(tmp) / "pages"
        pages.mkdir()
        for name in PAGES_AS_SIEVE + PAGES_PLAIN + ["style.css"]:
            shutil.copy2(SRC / name, pages / name)
        sieve_of(sieve, pages, out / "pages.sieve")
    good = (out / "images" / "vitruvian.sieve").read_bytes()
    bad = bytearray(good)
    bad[len(bad) // 2] ^= 0x01
    (out / "images" / "tampered.sieve").write_bytes(bytes(bad))
    shutil.copy2(SRC / "style.css", out / "style.css")

    # The pages, each once every file it names is made.
    left = {name: (SRC / name).read_text(encoding="utf-8") for name in PAGES_AS_SIEVE + PAGES_PLAIN}
    with tempfile.TemporaryDirectory() as tmp:
        while left:
            ready = [n for n, text in left.items() if all((out / p).exists() for p in SLOT.findall(text))]
            if not ready:
                sys.exit("these pages name each other's hashes, which cannot be (each would change the other's): " + ", ".join(sorted(left)))
            for name in ready:
                text = SLOT.sub(lambda m: sri(out / m.group(1)), left.pop(name))
                text = text.replace("{{home}}", f'<a href="{a.home}">Sieve Secure</a>' if a.home else "")
                if name in PAGES_AS_SIEVE:
                    page = Path(tmp) / name
                    page.write_text(text, encoding="utf-8", newline="\n")
                    sieve_of(sieve, page, out / (Path(name).stem + ".sieve"))
                else:
                    (out / name).write_text(text, encoding="utf-8", newline="\n")

    for f in sorted(out.rglob("*")):
        if f.is_file():
            print(f"{f.relative_to(out).as_posix():28} {f.stat().st_size:>9,} bytes  {sri(f)}")
    print(f"\nBuilt {out}. Serve it with:\n  python -m http.server 8000 --directory \"{out}\"\nand open http://localhost:8000/ in Chrome with the extension loaded.")


if __name__ == "__main__":
    main()
