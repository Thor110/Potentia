"""Checks the Documentation's help library (data/help, IDEAS 19.201) against itself and the code.

Every page contents.txt lists has its file, and every page file is listed; every page begins with its
"index:" line; every link [[id]] or [[id|words]] names a page that is there; and every key a page names,
{key:SECTION/ID|shown} or {key1:SECTION/ID|shown}, is one the program declares (pk::declare("SECTION",
"ID", ...) or declare({"SECTION", "ID", ...}) in client/), so a page never names a key that does not
exist. Prints what is wrong, one a line, and exits 1 if anything is; prints a count and exits 0 if not.

Usage: python tools/check_library.py [SIEVE_FOLDER]   (default: the folder above this script)
"""

import pathlib
import re
import sys


def main() -> int:
    root = pathlib.Path(sys.argv[1]) if len(sys.argv) > 1 else pathlib.Path(__file__).resolve().parent.parent
    lib = root / "data" / "help"
    wrong = []

    listed = []
    for n, line in enumerate((lib / "contents.txt").read_text(encoding="utf-8").splitlines(), 1):
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        parts = line.split(" ", 2)
        if len(parts) < 3 or parts[0] not in ("category", "page"):
            wrong.append(f"contents.txt:{n}: not 'category ID Title' or 'page ID Title': {line}")
            continue
        if parts[0] == "page":
            listed.append(parts[1])
    for dup in sorted({p for p in listed if listed.count(p) > 1}):
        wrong.append(f"contents.txt: page {dup} listed more than once")

    files = {p.stem for p in lib.glob("*.txt") if p.name != "contents.txt"}
    for p in listed:
        if p not in files:
            wrong.append(f"contents.txt: page {p} has no file {p}.txt")
    for f in sorted(files - set(listed)):
        wrong.append(f"{f}.txt: not listed in contents.txt")

    declared = set()
    for src in (root / "client").glob("*.cpp"):
        text = src.read_text(encoding="utf-8", errors="replace")
        for m in re.finditer(r'declare\(\{?\s*"([a-z0-9_.]+)",\s*"([a-z0-9_]+)"', text):
            declared.add(f"{m.group(1)}/{m.group(2)}")

    links = keys = 0
    for p in listed:
        path = lib / f"{p}.txt"
        if not path.exists():
            continue
        text = path.read_text(encoding="utf-8")
        if not text.startswith("index:"):
            wrong.append(f"{p}.txt: does not begin with its 'index:' line")
        for m in re.finditer(r"\[\[([^\]|]+)(\|[^\]]*)?\]\]", text):
            links += 1
            if m.group(1).strip() not in listed:
                wrong.append(f"{p}.txt: link to {m.group(1).strip()}, which is not a page")
        for m in re.finditer(r"\{key1?:([^|}]+)\|[^}]*\}", text):
            keys += 1
            if m.group(1) not in declared:
                wrong.append(f"{p}.txt: key {m.group(1)}, which the program does not declare")

    for w in wrong:
        print(w)
    if wrong:
        return 1
    print(f"help library: {len(listed)} pages, {links} links, {keys} keys named, all found")
    return 0


if __name__ == "__main__":
    sys.exit(main())
