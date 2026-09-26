#!/usr/bin/env python3
"""Fetches GNU Unifont into data/fonts/unifont.hex, the way fetch_corpus.py fetches the corpus.

Sieve ships one font of its own, sieve8x8, which covers ASCII, Latin-1, Greek and a little more.
That is enough for the menus in the languages the repository ships, but not for a text line over
Cyrillic, kana, hieroglyphs or emoji, and not for the Binary Edge's rain, which draws code points
from every Unicode block it is pointed at.

Unifont covers essentially the whole of planes 0 and 1, and it is already in exactly the format
Sieve's font reader wants (see client/font.hpp). It is NOT stored in this repository, because it
is GPLv2+ (with the GNU Font Embedding Exception) while Sieve's own bundled font is public
domain, and a project should choose for itself what licences it carries. So it is fetched on
demand, each part checked against a pinned SHA-256, exactly as the training corpus is.

    python tools/fetch_unifont.py            # into data/fonts/unifont.hex
    python tools/fetch_unifont.py --plane0   # skip plane 1 (no emoji or hieroglyphs), 3.6 MB

Then name it in a language file's first line (font = unifont), or pick it in the hallway.
"""
import argparse
import hashlib
import pathlib
import sys
import urllib.request

# Pinned by content, not by branch: the hash is what makes this reproducible. Every part comes
# from Unifont's own plane00 and plane01 folders.
BASE = "https://raw.githubusercontent.com/iolo/unifont/master/font/"
PARTS = [
    ("plane00/unifont-base.hex", "abac97e6b32916cd5ff76d064b82b684c78422d9beb9da7cdaeeb0d20289714b",
     "plane 0: Latin, Greek, Cyrillic, Arabic, Hebrew, the Indic scripts, kana, symbols, Braille"),
    ("plane00/wqy.hex", "d7fa0af5483ab75c90642cfb05edb578a941ff1ad66e0cdf767f1c839b3968d9",
     "plane 0: the CJK ideographs, from WenQuanYi"),
    ("plane00/hangul-syllables.hex", "17ae511e6203a74f19390e4f0c3a4acff4c63cea79c158ab4e2fb48307449141",
     "plane 0: the Hangul syllables"),
    ("plane00/spaces.hex", "b8dde531fadf981caa760631daab4523ef7cc243847c74c88ad704a4d4778fb0",
     "plane 0: the spaces"),
    ("plane01/plane01.hex", "971322a6da655f2895dcf75f9583cd0828d22de7da0d2aaeef60aa3a35a012a0",
     "plane 1: emoji, hieroglyphs, cuneiform, Old Italic, Gothic and the other ancient scripts"),
]
LICENCE = (
    "GNU Unifont is copyright Roman Czyborra, Paul Hardy and others, and is licensed under the\n"
    "GNU GPL version 2 or later, with the GNU Font Embedding Exception. It is fetched here, not\n"
    "stored in this repository. See https://unifoundry.com/unifont/ for the font and its terms.\n"
)


def fetch(path: str, want: str) -> bytes:
    url = BASE + path
    print(f"downloading {url}")
    with urllib.request.urlopen(url, timeout=180) as r:
        data = r.read()
    got = hashlib.sha256(data).hexdigest()
    if got != want:
        raise SystemExit(f"{path}: sha256 is {got}, expected {want}\n"
                         "The upstream file has changed. Check it, then update the hash here.")
    return data


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--plane0", action="store_true", help="plane 0 only: no emoji, hieroglyphs or cuneiform")
    ap.add_argument("--out", default=None, help="where to write it (default: data/fonts/unifont.hex)")
    args = ap.parse_args()

    root = pathlib.Path(__file__).resolve().parent.parent
    out = pathlib.Path(args.out) if args.out else root / "data" / "fonts" / "unifont.hex"
    out.parent.mkdir(parents=True, exist_ok=True)

    glyphs: dict[int, str] = {}
    for path, want, what in PARTS:
        if args.plane0 and path.startswith("plane01/"):
            continue
        for line in fetch(path, want).decode("ascii", "replace").splitlines():
            if ":" not in line:
                continue
            cp, bits = line.split(":", 1)
            try:
                glyphs[int(cp, 16)] = bits.strip()
            except ValueError:
                pass
        print(f"  {what}")

    # One file, in code point order, with the height Sieve's reader looks for.
    with open(out, "w", encoding="ascii", newline="\n") as f:
        f.write("# GNU Unifont, fetched by tools/fetch_unifont.py. GPLv2+ with the font embedding\n")
        f.write("# exception; see https://unifoundry.com/unifont/ . Not part of Sieve.\n")
        f.write("# height: 16\n")
        for cp in sorted(glyphs):
            f.write(f"{cp:04X}:{glyphs[cp]}\n")

    size = out.stat().st_size
    print(f"\nwrote {out} ({len(glyphs)} glyphs, {size // 1024} KB)")
    print(f"planes: 0{'' if args.plane0 else ' and 1'}")
    print("\n" + LICENCE)
    print("To use it, put `font = unifont` on the first line of a language file in data/lang,")
    print("or turn on the Binary Edge in the hallway's graphics settings.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
