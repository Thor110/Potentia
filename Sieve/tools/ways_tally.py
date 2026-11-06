#!/usr/bin/env python3
r"""Which ways carried the files: a tally over any number of `.sieve` files (Edward, 4 November
2026: "take note of which methods win out more often ... I don't want to have someone cherry pick an
example and just claim LZMA2 did all the work").

  python tools/ways_tally.py PATH [PATH ...]               the listings only
  python tools/ways_tally.py PATH [PATH ...] --sieve SIEVE  installed too: each file with a code of
                                                            its own set against LZMA2 alone
  ... --md OUT.md --tsv OUT.tsv                             the tables written out as well

A PATH is a `.sieve` file or a folder searched for them. Each file's way is read from the listing (as
tools/sieve_read.py reads it, through the oracle), whatever carried it: a way of Sieve's own, LZMA2,
or a file made again on installing around an inner way (a JPEG's brunsli form, an archive or PNG
remade, a text's base64 decoded). For each way, what came from where it was first written:

  - the files it carried and their bytes;
  - the bytes it took in the `.sieve`, where they can be told apart: a code of the file's own (the
    rope, pieces' rope part, mixing, runs: each code is read from the rope stream by the length before
    it, and the codes checked to add up to the stream); the streams shared by many files (LZMA2's,
    the x86 filter's, the items') are given as streams, since one file's share of a solid stream is
    not recorded;
  - with --sieve: every file with a code of its own packed alone by LZMA2 (xz -9e, raw, as 7-Zip
    makes it) from the installed folder, and what its own code saved against that.

Members (the files an archive or a text is taken apart into, `<file>/<n>`) are counted apart: they
are not files of the folder.
"""

import argparse
import lzma
import subprocess
import sys
import tempfile
from collections import OrderedDict
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))
sys.path.insert(0, str(HERE.parent / "reference"))
import sieve_read  # noqa: E402

# Each way, what it is and whose: the methods borrowed are named as such.
WAYS = OrderedDict([
    ("raw", ("LZMA2, in the shared lzma2 stream", "7-Zip's (LZMA2)")),
    ("x86", ("LZMA2 after the x86 filter, in the x86 stream", "7-Zip's (LZMA2, BCJ)")),
    ("rope", ("the rope", "Sieve's")),
    ("pieces", ("pieces: the rope and LZMA2 by turns", "Sieve's, with LZMA2")),
    ("mix4", ("mixing, alone", "Sieve's (context mixing)")),
    ("mixt", ("mixing, files mixed together as a group", "Sieve's (context mixing)")),
    ("mix3", ("mixing, mix-v3", "Sieve's (context mixing)")),
    ("mix2", ("mixing, mix-v2", "Sieve's (context mixing)")),
    ("mix", ("mixing, mix-v1", "Sieve's (context mixing)")),
    ("runs", ("runs, patterns and ramps", "Sieve's")),
    ("copy", ("a copy of another file", "Sieve's")),
    ("lines", ("the lines of another file, by a mask", "Sieve's")),
    ("delta", ("its differences from another file", "Sieve's")),
    ("item", ("an item of one of Sieve's lines, by its place", "Sieve's")),
])
OWN_CODE = {"rope", "pieces", "mix4", "mixt", "mix3", "mix2", "mix", "runs"}  # (mixt: the group's code, with its first file)
# What a file can be made again from on installing, around its inner way.
RECIPES = OrderedDict([
    ("jbr", "a JPEG from its brunsli form (Google's brunsli)"),
    ("pngr", "a PNG re-saved from its rows (zlib)"), ("skr", "a PNG from its rows and skeleton"),
    ("sk", "a PNG from its skeleton"), ("stb", "a PNG stb_image_write made"),
    ("skm", "an archive (gzip, zip, 7z) from its skeleton and members"),
    ("spn", "a text with binary written in it (base64, hex), decoded"),
    ("gif", "a GIF from its frames' colour indices (v34 draft)"),
    ("xz", "an xz file remade"), ("gz", "a gzip file remade"), ("bz2", "a bzip2 file remade"), ("zst", "a zstd file remade"),
    ("lz4", "an lz4 file remade"), ("br", "a brotli file remade"), ("lzma", "an lzma file remade"), ("lz", "an lzip file remade"),
])


def split_way(way):
    """(recipe or None, its members (skm, spn), the inner way's words)."""
    t = way.split()
    if not t:
        return None, 0, ["?"]
    if t[0] in RECIPES:
        for j in range(1, len(t)):
            if (t[j] in WAYS or t[j] == "mixn") and t[j] != "item":
                members = int(t[1]) if t[0] in ("skm", "spn") and t[1].isdigit() else 0
                return t[0], members, t[j:]
        return t[0], 0, ["?"]
    return None, 0, t


def entries(info):
    """The listing's files and items in order: path, size, recipe, inner way, member or not."""
    out, members_left, member_of = [], 0, ""
    for ln in info["lines"][1:]:
        if ln.startswith("f\t"):
            f = ln.split("\t")
            size, way, path = int(f[1]), f[-2], f[-1]
            recipe, members, inner = split_way(way)
            is_member = members_left > 0 and path.startswith(member_of + "/")
            if is_member:
                members_left -= 1
            out.append({"path": path, "size": size, "recipe": recipe, "inner": inner, "member": is_member})
            if members:
                members_left, member_of = members, path
        elif ln.startswith("i\t"):
            f = ln.split("\t")
            out.append({"path": f[-1], "size": None, "recipe": None, "inner": ["item"], "member": False})
    return out


def own_codes(info, es):
    """Each own-code entry's code length, read from the rope stream in listing order; and whether the
    lengths add up to the stream exactly (the check that the reading is right)."""
    at = 0
    for s in info["streams"]:
        if s["name"] == "rope":
            blob = info["payload"][at:at + s["packed"]]
            pos, got = 0, {}
            for i, e in enumerate(es):
                if e["inner"][0] in OWN_CODE:
                    n, pos = sieve_read.varint(blob, pos)
                    got[i] = n
                    pos += n
            return got, pos == len(blob)
        at += s["packed"]
    return {}, not any(e["inner"][0] in OWN_CODE for e in es)


def lzma2_alone(b):
    return len(lzma.compress(b, format=lzma.FORMAT_RAW, filters=[{"id": lzma.FILTER_LZMA2, "preset": 9 | lzma.PRESET_EXTREME}]))


def installed_files(sieve, path, tmp):
    subprocess.run([sieve, "install", str(path), "--to", tmp], check=True, stdout=subprocess.DEVNULL)
    return {str(p.relative_to(tmp)).replace("\\", "/"): p for p in Path(tmp).rglob("*") if p.is_file()}


def find(files, root, path):
    for key in ((root + "/" + path) if root else path, path):
        if key in files:
            return files[key]
    hits = [p for k, p in files.items() if k.endswith("/" + path) or k == path]
    return hits[0] if len(hits) == 1 else None


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("paths", nargs="+")
    ap.add_argument("--sieve", help="install each with this sieve and set each own code against LZMA2 alone")
    ap.add_argument("--md")
    ap.add_argument("--tsv")
    a = ap.parse_args()
    sieves = []
    for p in map(Path, a.paths):
        sieves += sorted(p.rglob("*.sieve")) if p.is_dir() else [p]

    ways = OrderedDict()     # way -> files, bytes, own code bytes, (with --sieve) LZMA2 alone, files measured
    recipes = OrderedDict()  # recipe -> files, bytes
    streams = OrderedDict()  # stream -> packed, holding
    members = [0, 0]
    total_in, total_out, unread = 0, 0, []
    rows = []
    for sp in sieves:
        try:
            info = sieve_read.read(sp)
        except SystemExit as ex:  # (the oracle refuses what it cannot read: said, and the rest goes on)
            unread.append(f"{sp}: {ex}")
            continue
        es = entries(info)
        codes, adds_up = own_codes(info, es)
        if not adds_up:
            unread.append(f"{sp}: its codes do not add up to its rope stream (a way this script does not know?)")
        files = {}
        tmp = None
        if a.sieve:
            tmp = tempfile.TemporaryDirectory()
            files = installed_files(a.sieve, sp, tmp.name)
        root = next((ln[5:] for ln in info["lines"] if ln.startswith("root ")), "")
        size_in = 0
        mine = {"codes": 0, "lz": 0, "ways": OrderedDict()}
        group = None  # (a mixed group: its files joined, set against LZMA2 as one, as its code covers them all)

        def close_group():
            if group is not None and group["parts"]:
                w = ways["mixt"]
                z = lzma2_alone(b"".join(group["parts"]))
                w[3] += z
                w[4] += 1
                mine["lz"] += z

        for i, e in enumerate(es):
            if e["inner"][0] == "mixn":
                e["inner"] = ["mixt"] + e["inner"][1:]
                e["in_group"] = True
            elif e["inner"][0] == "mixt":
                close_group()
                group = {"parts": []}
            if e["member"]:
                members[0] += 1
                members[1] += e["size"] or 0
            k = e["inner"][0]
            if e["size"] is None and files:
                p = find(files, root, e["path"])
                e["size"] = p.stat().st_size if p else 0
            size = e["size"] or 0
            w = ways.setdefault(k, [0, 0, 0, 0, 0])
            if not e["member"]:
                w[0] += 1
                w[1] += size
                size_in += size
                mine["ways"][k] = mine["ways"].get(k, 0) + size
            if k == "mixt":
                # (a group's code with its first file; every file of it joined for LZMA2)
                w[2] += codes.get(i, 0)
                mine["codes"] += codes.get(i, 0)
                if files and group is not None:
                    p = find(files, root, e["path"])
                    if p:
                        group["parts"].append(p.read_bytes())
            elif i in codes:
                w[2] += codes[i]
                mine["codes"] += codes[i]
                if files and not e["member"]:
                    p = find(files, root, e["path"])
                    if p:
                        z = lzma2_alone(p.read_bytes())
                        w[3] += z
                        w[4] += 1
                        mine["lz"] += z
            if e["recipe"] and not e["member"]:
                r = recipes.setdefault(e["recipe"], [0, 0])
                r[0] += 1
                r[1] += size
        close_group()
        for s in info["streams"]:
            t = streams.setdefault(s["name"], [0, 0])
            t[0] += s["packed"]
            t[1] += s["bytes"]
        if tmp:
            tmp.cleanup()
        total_in += size_in
        total_out += info["size"]
        main_way = max(mine["ways"], key=mine["ways"].get) if mine["ways"] else ""
        rows.append((sp.name, info["version"], len(es), size_in, info["size"], main_way, mine["codes"], mine["lz"] if files else 0))

    out = []
    out.append(f"# Which ways carried the files\n")
    out.append(f"{len(rows)} `.sieve` file(s), {total_in:,} bytes of files in {total_out:,}.\n")
    out.append("| Way | Whose | Files | Their bytes | Own codes | LZMA2 alone (same files) | Saved against LZMA2 |")
    out.append("| :--- | :--- | ---: | ---: | ---: | ---: | ---: |")
    for k, (n, b, c, lz, m) in ways.items():
        what, whose = WAYS.get(k, (k, "?"))
        saved = f"{lz - c:,} ({100 * (lz - c) / lz:.1f}%)" if m and lz else ""
        out.append(f"| {what} (`{k}`) | {whose} | {n:,} | {b:,} | {f'{c:,}' if c else ''} | {f'{lz:,}' if m else ''} | {saved} |")
    if recipes:
        out.append("\nMade again on installing, around the inner way above:\n")
        out.append("| Recipe | Files | Their bytes |")
        out.append("| :--- | ---: | ---: |")
        for k, (n, b) in recipes.items():
            out.append(f"| {RECIPES.get(k, k)} (`{k}`) | {n:,} | {b:,} |")
    if members[0]:
        out.append(f"\nMembers (taken out of an archive or a text, carried their own ways, counted above by their way only): {members[0]:,}, {members[1]:,} bytes.")
    out.append("\nStreams (shared by their files: one file's share of a solid stream is not recorded):\n")
    out.append("| Stream | In the `.sieve` | Holding |")
    out.append("| :--- | ---: | ---: |")
    for k, (p, b) in streams.items():
        out.append(f"| {k} | {p:,} | {b:,} |")
    out.append("\nEach `.sieve` (its main way: the one that carried most of its bytes):\n")
    out.append("| `.sieve` | Version | Entries | Files' bytes | Its bytes | Main way | Own codes | LZMA2 alone (same files) |")
    out.append("| :--- | :--- | ---: | ---: | ---: | :--- | ---: | ---: |")
    for r in rows:
        out.append(f"| {r[0]} | {r[1]} | {r[2]:,} | {r[3]:,} | {r[4]:,} | `{r[5]}` | {f'{r[6]:,}' if r[6] else ''} | {f'{r[7]:,}' if r[7] else ''} |")
    if unread:
        out.append("\nNot read:\n")
        out += [f"- {u}" for u in unread]
    text = "\n".join(out) + "\n"
    print(text)
    if a.md:
        Path(a.md).write_text(text, encoding="utf-8")
    if a.tsv:
        with open(a.tsv, "w", encoding="utf-8") as t:
            t.write("way\twhose\tfiles\tbytes\town_codes\tlzma2_alone\tmeasured\n")
            for k, (n, b, c, lz, m) in ways.items():
                t.write(f"{k}\t{WAYS.get(k, (k, '?'))[1]}\t{n}\t{b}\t{c}\t{lz}\t{m}\n")


if __name__ == "__main__":
    main()
