"""Every test of a RAR made again by your own rar (sieve-manifest-v22), in one go.

  py -3 tools\\rar_check.py                      (or double-click tools\\rar_check.bat)
  py -3 tools\\rar_check.py MY.rar OTHER.rar     your own RARs too
  options: --sieve PATH  --installer PATH  --rar PATH  --work FOLDER  --only NAME

It makes RARs from the repository's own files with the rar found, at the settings people use
(the defaults, each level, solid, BLAKE2, quick open, creation times, times to the second, a
dictionary named, files without their folder), adds any RARs given, and for each:

  1. packs it (sieve locate --items --optimise-archives, SIEVE_RAR_TRACE=1): made again or not, why,
     and for each try how many of its first bytes came out the same;
  2. installs it with rar there (sieve install): the RAR given back, byte for byte;
  3. installs it with no rar (SIEVE_RAR pointing nowhere): its files, and a warning saying so;
  4. installs it with sieve-install --yes, where there is one: the RAR given back again.

A table at the end, and everything in rar-check.log in the work folder (out\\rar-check), to send.
"""

import argparse
import filecmp
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SIEVE_DIR = os.path.dirname(HERE)
EXE = ".exe" if os.name == "nt" else ""


def first(paths):
    for p in paths:
        if p and os.path.isfile(p):
            return os.path.abspath(p)
    return None


def find_program(name, given):
    if given:
        return os.path.abspath(given)
    return first([os.path.join(SIEVE_DIR, "out", "build", c, name + EXE) for c in ("x64-Release", "x64-Debug", "linux-release")] +
                 [os.path.join(SIEVE_DIR, "build", name + EXE), os.path.join(SIEVE_DIR, "tools", name + EXE), os.path.join(HERE, name + EXE)])


def find_rar(given):
    if given:
        return os.path.abspath(given)
    if os.environ.get("SIEVE_RAR"):
        return os.environ["SIEVE_RAR"]
    on_path = shutil.which("rar")
    if on_path:
        return on_path
    return first([os.path.join(os.environ.get(v, ""), "WinRAR", "Rar.exe") for v in ("ProgramFiles", "ProgramW6432", "ProgramFiles(x86)")])


LOG = []


def log(text=""):
    print(text)
    LOG.append(text)


def run(args, cwd, env=None):
    e = dict(os.environ)
    e.update(env or {})
    r = subprocess.run(args, cwd=cwd, env=e, stdin=subprocess.DEVNULL, capture_output=True)
    out = (r.stdout + r.stderr).decode("utf-8", "replace").replace("\r", "")
    return r.returncode, out


def same_tree(a, b):
    c = filecmp.dircmp(a, b)
    if c.left_only or c.right_only or c.diff_files or c.funny_files:
        return False
    return all(same_tree(os.path.join(a, d), os.path.join(b, d)) for d in c.common_dirs)


# The RARs made here: (name, switches, what is added). "pack" is added with -r; "files" is two files
# named alone (no folder entries); "one" is a single file.
CASES = [
    ("default", [], "pack"),
    ("m0-stored", ["-m0"], "pack"),
    ("m1-fastest", ["-m1"], "pack"),
    ("m5-best", ["-m5"], "pack"),
    ("solid", ["-s"], "pack"),
    ("solid-m5", ["-s", "-m5"], "pack"),
    ("blake2", ["-htb"], "pack"),
    ("quick-open", ["-qo+"], "pack"),
    ("no-quick-open", ["-qo-"], "pack"),
    ("creation-times", ["-tsc"], "pack"),
    ("times-to-second", ["-tsm1"], "pack"),
    ("dictionary-1m", ["-md1m"], "pack"),
    ("files-alone", [], "files"),
    ("one-file", [], "one"),
]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("rars", nargs="*", help="your own RARs to try as well")
    ap.add_argument("--sieve")
    ap.add_argument("--installer")
    ap.add_argument("--rar")
    ap.add_argument("--work", default=os.path.join(SIEVE_DIR, "out", "rar-check"))
    ap.add_argument("--only", help="only the made case of this name (and the RARs given)")
    a = ap.parse_args()
    sieve = find_program("sieve", a.sieve)
    installer = find_program("sieve-install", a.installer)
    rar = find_rar(a.rar)
    work = os.path.abspath(a.work)
    if not sieve:
        sys.exit("sieve not found: build Release first, or give --sieve PATH")
    if not rar or not os.path.isfile(rar):
        sys.exit("rar not found (looked in --rar, SIEVE_RAR, the PATH and WinRAR's folder): give --rar PATH\\Rar.exe")
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)
    _, banner = run([rar], work)
    version = next((l.split("Copyright")[0].strip() for l in banner.splitlines() if l.startswith("RAR ") and "Copyright" in l), "(no version given)")
    log(f"sieve          {sieve}")
    log(f"sieve-install  {installer or '(none found: step 4 skipped)'}")
    log(f"rar            {rar}")
    log(f"rar's version  {version}")
    log(f"work folder    {work}")
    log()

    # What the RARs are made of: the repository's own files.
    src = os.path.join(work, "src")
    pack = os.path.join(src, "pack")
    shutil.copytree(os.path.join(SIEVE_DIR, "tests", "manifest_fixture"), pack)
    shutil.copy(os.path.join(SIEVE_DIR, "tests", "plugins", "moby-sentences.txt"), pack)
    os.makedirs(os.path.join(pack, "empty folder"), exist_ok=True)
    rars = []
    for name, switches, what in CASES:
        if a.only and name != a.only:
            continue
        out = os.path.join(work, name + ".rar")
        names = {"pack": ["-r", "--", out, "pack"], "files": ["--", out, os.path.join("pack", "a.txt"), os.path.join("pack", "Zebra.txt")],
                 "one": ["--", out, os.path.join("pack", "moby-sentences.txt")]}[what]
        code, said = run([rar, "a", "-cfg-", "-idq", "-y", *switches, *names], src)
        if code != 0 or not os.path.isfile(out):
            log(f"{name}: rar could not make it (exit {code}): {said.strip()[:200]}")
            continue
        rars.append((name, out, " ".join(switches) or "(defaults)"))
    for given in a.rars:
        name = os.path.splitext(os.path.basename(given))[0]
        out = os.path.join(work, "yours-" + os.path.basename(given))
        shutil.copy(given, out)
        rars.append(("yours-" + name, out, "(yours)"))

    rows = []
    for name, path, how in rars:
        log(f"==== {name}: {how}, {os.path.getsize(path)} bytes")
        case = os.path.join(work, name)
        os.makedirs(case)
        arc = os.path.join(case, os.path.basename(path))
        shutil.copy(path, arc)
        orig = open(arc, "rb").read()
        row = {"name": name, "made": "-", "rar": "-", "none": "-", "si": "-"}
        # 1. Packing.
        code, said = run([sieve, "locate", os.path.basename(arc), "--installer", "x.sieve", "--items", "--optimise-archives"], case,
                         {"SIEVE_RAR": rar, "SIEVE_RAR_TRACE": "1", "SIEVE_ARCHIVES_ALWAYS": "1"})
        for line in said.splitlines():
            if line.startswith(("rar:", "rar ", "unpacked", "manifest", "error")):
                log("  " + line)
        made = any(l.startswith("rar       ") and ": made again by " in l for l in said.splitlines())
        row["made"] = "yes" if made else "no" if code == 0 else f"failed ({code})"
        if made:
            # 2. Installed with rar: the RAR itself, byte for byte.
            code, said = run([sieve, "install", "x.sieve", "--to", "with-rar"], case, {"SIEVE_RAR": rar})
            back = os.path.join(case, "with-rar", os.path.basename(arc))
            ok = code == 0 and os.path.isfile(back) and open(back, "rb").read() == orig
            row["rar"] = "same" if ok else "DIFFERS"
            for line in said.splitlines():
                if line.startswith(("note", "warning", "error")):
                    log("  install, with rar:    " + line)
            # 3. Installed with no rar: its files, and a warning.
            code, said = run([sieve, "install", "x.sieve", "--to", "no-rar"], case, {"SIEVE_RAR": os.path.join(case, "no-such-rar" + EXE)})
            warned = any(l.startswith("warning   ") and "rar was not found" in l for l in said.splitlines())
            files = sum(len(f) for _, _, f in os.walk(os.path.join(case, "no-rar")))
            row["none"] = f"warned, {files} files" if code == 0 and warned and files else "NO WARNING" if code == 0 else f"failed ({code})"
            for line in said.splitlines():
                if line.startswith(("warning", "error")):
                    log("  install, no rar:      " + line)
            # 4. sieve-install, unattended.
            if installer:
                env = {"SIEVE_RAR": rar}
                if os.name != "nt" and not os.environ.get("DISPLAY") and not os.environ.get("SDL_VIDEO_DRIVER"):
                    env["SDL_VIDEO_DRIVER"] = "offscreen"  # (no screen: its window is drawn off it)
                code, said = run([installer, "x.sieve", "--to", "sieve-install", "--yes"], case, env)
                for line in said.splitlines():
                    if line.strip():
                        log("  sieve-install:        " + line)
                back = os.path.join(case, "sieve-install", os.path.basename(arc))
                row["si"] = "same" if code == 0 and os.path.isfile(back) and open(back, "rb").read() == orig else f"DIFFERS ({code})"
        rows.append(row)
        log()

    log("RAR                    packed: made again  installed with rar  installed with none   sieve-install")
    for r in rows:
        log(f"{r['name']:<22} {r['made']:<19} {r['rar']:<19} {r['none']:<20} {r['si']}")
    good = sum(1 for r in rows if r["made"] == "yes" and r["rar"] == "same" and r["none"].startswith("warned") and r["si"] in ("same", "-"))
    log()
    log(f"{good} of {len(rows)} made again and given back exactly. The log: {os.path.join(work, 'rar-check.log')}")
    with open(os.path.join(work, "rar-check.log"), "w", encoding="utf-8") as f:
        f.write("\n".join(LOG) + "\n")
    return 0 if good == len(rows) and rows else 1


sys.exit(main())
