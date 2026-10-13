#!/usr/bin/env python3
"""Logs a build's metrics, on the record (IDEAS §19.50, §19.69).

run_tests.bat runs this once everything it ran has passed. It writes one record a build,
metrics/builds/YYYY-MM-DD-HHMMSS.txt, then writes metrics/index.tsv again from every record there,
so the index can never disagree with the records. The website and the game read the index.

  python tools/log_metrics.py [--unit NAME=FILE ...] [--hallway EXE] [--set KEY=VALUE ...] [--dry-run]

  --unit NAME=FILE   a unit test run's output (NAME a build, e.g. x64-Release): its last
                     "N checks, F failures" line is logged
  --hallway EXE      a built hallway: its metrics list (--metrics) under the reference filters
                     (data/metrics/reference-filters.ini), as make_release writes it, is logged
                     and kept whole in the record
  --set KEY=VALUE    a measure the caller made itself (make_release: kind = release, and how long
                     the unit suite took, unit_seconds); KEY is lower case, digits and _
  --dry-run          print the record without writing anything

The machine is recorded by what it is, read from the system: the processor's name, its logical
processors, the memory, and on Windows the graphics adapters. When metrics/machines/ holds a
machine file (sieve-machine-v1) whose cpu matches, the record names it (machine_profile), and the
website's Specs vs Stats page shows the machine's full specification beside its figures. The
machine's name (its host name) is never recorded.

A record (sieve-metrics-v1) is plain text: its first line "sieve-metrics-v1", then "key = value"
lines, then, after a line "hallway-metrics", the hallway's list as it wrote it. A measure that
changes meaning gets a new key, never a new meaning for an old one, so every column of the index
is one series (IDEAS §19.50: the same measure, the same version).
"""
import datetime
import os
import platform
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, ".."))
METRICS = os.path.join(ROOT, "metrics")
BUILDS = os.path.join(METRICS, "builds")
INDEX = os.path.join(METRICS, "index.tsv")
FORMAT = "sieve-metrics-v1"
REFERENCE_FILTERS = os.path.join(ROOT, "data", "metrics", "reference-filters.ini")

# The source counted as lines of code: the engine, the game, the tools and the oracle.
SOURCE_DIRS = ["core", "client", "tools", "reference"]
SOURCE_EXTS = (".cpp", ".hpp", ".h", ".py")

# The index's columns, in order. A key a record lacks is left empty in its row.
COLUMNS = ["date", "kind", "commit", "version", "machine", "machine_profile", "cpu", "logical_processors",
           "memory_gb", "gpu", "unit_checks", "unit_builds", "unit_seconds", "vector_files", "voices", "ideas",
           "spec_sections", "commands", "source_files", "source_lines"]
MACHINES = os.path.join(METRICS, "machines")
MACHINE_FORMAT = "sieve-machine-v1"


def git(*args):
    try:
        out = subprocess.run(["git"] + list(args), cwd=ROOT, capture_output=True, text=True)
        return out.stdout.strip() if out.returncode == 0 else ""
    except OSError:
        return ""


def read(path):
    with open(path, encoding="utf-8") as f:
        return f.read()


def version():
    m = re.search(r"project\(Sieve VERSION ([0-9.]+)", read(os.path.join(ROOT, "CMakeLists.txt")))
    return m.group(1) if m else ""


def plain(text):
    return " ".join(text.split()).lower()


def cpu_name():
    if sys.platform == "win32":
        try:
            import winreg
            with winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, r"HARDWARE\DESCRIPTION\System\CentralProcessor\0") as k:
                return " ".join(str(winreg.QueryValueEx(k, "ProcessorNameString")[0]).split())
        except OSError:
            return ""
    try:
        for line in open("/proc/cpuinfo", encoding="utf-8", errors="replace"):
            if line.startswith("model name"):
                return " ".join(line.split(":", 1)[1].split())
    except OSError:
        pass
    return platform.processor()


def memory_gb():
    """The installed memory in GiB, rounded to the nearest whole one."""
    if sys.platform == "win32":
        import ctypes

        class Status(ctypes.Structure):
            _fields_ = [("length", ctypes.c_ulong), ("load", ctypes.c_ulong), ("total", ctypes.c_ulonglong),
                        ("free", ctypes.c_ulonglong), ("page_total", ctypes.c_ulonglong), ("page_free", ctypes.c_ulonglong),
                        ("virtual_total", ctypes.c_ulonglong), ("virtual_free", ctypes.c_ulonglong), ("extended", ctypes.c_ulonglong)]
        st = Status()
        st.length = ctypes.sizeof(Status)
        if ctypes.windll.kernel32.GlobalMemoryStatusEx(ctypes.byref(st)):
            return str(round(st.total / 2**30))
        return ""
    try:
        for line in open("/proc/meminfo", encoding="utf-8"):
            if line.startswith("MemTotal:"):
                return str(round(int(line.split()[1]) * 1024 / 2**30))
    except OSError:
        pass
    return ""


def gpu_names():
    """The graphics adapters, on Windows (through PowerShell); elsewhere, not recorded."""
    if sys.platform != "win32":
        return ""
    try:
        out = subprocess.run(["powershell", "-NoProfile", "-Command",
                              "(Get-CimInstance Win32_VideoController).Name"], capture_output=True, text=True, timeout=30)
        return "; ".join(n.strip() for n in out.stdout.splitlines() if n.strip())
    except (OSError, subprocess.SubprocessError):
        return ""


def machine_profile(cpu):
    """The machine file (metrics/machines/NAME.txt) whose cpu is this one's, if any."""
    if not cpu or not os.path.isdir(MACHINES):
        return ""
    for name in sorted(os.listdir(MACHINES)):
        if not name.endswith(".txt"):
            continue
        lines = read(os.path.join(MACHINES, name)).splitlines()
        if not lines or lines[0] != MACHINE_FORMAT:
            continue
        for line in lines[1:]:
            k, sep, v = line.partition(" = ")
            if sep and k == "cpu" and plain(v) == plain(cpu):
                return name[:-4]
    return ""


def repository_counts():
    """What the repository itself holds: each a count anyone can repeat on the same commit."""
    counts = {}
    sys.path.insert(0, HERE)
    import check_vectors  # the list CI checks, so the two can never differ
    counts["vector_files"] = len(check_vectors.CHECKS)
    counts["voices"] = len(re.findall(r"^- \*\*[^*]+\*\* \([^,)]+, [^)]+\): ", read(os.path.join(ROOT, "docs", "VOICES.md")), re.M))
    counts["ideas"] = len(set(re.findall(r"^\*\*(\d+\.\d+)[a-z]? ", read(os.path.join(ROOT, "docs", "IDEAS.md")), re.M)))
    counts["spec_sections"] = len(re.findall(r"^#{2,4} ", read(os.path.join(ROOT, "docs", "SPECIFICATIONS.md")), re.M))
    counts["commands"] = len(re.findall(r"^### `", read(os.path.join(ROOT, "README.md")), re.M))
    files = lines = 0
    for d in SOURCE_DIRS:
        for top, dirs, names in os.walk(os.path.join(ROOT, d)):
            dirs[:] = [x for x in dirs if x != "__pycache__"]
            for n in names:
                if n.endswith(SOURCE_EXTS):
                    files += 1
                    with open(os.path.join(top, n), "rb") as f:
                        lines += f.read().count(b"\n")
    counts["source_files"] = files
    counts["source_lines"] = lines
    return counts


def unit_checks(pairs):
    """The unit suite's "N checks, F failures" from each run named: (total of the last run, builds)."""
    checks, builds = None, []
    for pair in pairs:
        name, _, path = pair.partition("=")
        if not os.path.exists(path):
            continue
        found = re.findall(r"^(\d+) checks, (\d+) failures\s*$", open(path, encoding="utf-8", errors="replace").read(), re.M)
        if not found:
            continue
        n, failures = map(int, found[-1])
        if failures:
            sys.exit(f"log_metrics: {name} has {failures} failures; nothing logged")
        builds.append(name)
        checks = n if checks is None else max(checks, n)
    return checks, builds


def hallway_metrics(exe):
    """The hallway's metrics list under the reference filters, and each line's kept share (log10)."""
    exe = os.path.abspath(exe)
    with tempfile.TemporaryDirectory() as tmp:
        out = os.path.join(tmp, "metrics.txt")
        subprocess.run([exe, "--metrics", out, "--filters", REFERENCE_FILTERS], cwd=os.path.dirname(exe), capture_output=True)
        if not os.path.exists(out):
            return "", {}
        text = read(out)
    kept = {}
    for name, value in re.findall(r"^(\w+)\s*: .*\( Kept : 10\^(-?[0-9.]+)", text, re.M):
        kept["kept_log10_" + name.lower()] = value
    return text, kept


def parse(path):
    keys, lines = {}, read(path).splitlines()
    if not lines or lines[0] != FORMAT:
        return None
    for line in lines[1:]:
        if line == "hallway-metrics":
            break
        k, sep, v = line.partition(" = ")
        if sep:
            keys[k] = v
    return keys


def write_index():
    rows = []
    for name in sorted(os.listdir(BUILDS)):
        if name.endswith(".txt"):
            keys = parse(os.path.join(BUILDS, name))
            if keys:
                rows.append(keys)
    extra = sorted({k for r in rows for k in r if k not in COLUMNS})
    columns = COLUMNS + extra
    with open(INDEX, "w", encoding="utf-8", newline="\n") as f:
        f.write("\t".join(columns) + "\n")
        for r in rows:
            f.write("\t".join(r.get(c, "") for c in columns) + "\n")
    return len(rows)


def main():
    args = sys.argv[1:]
    units, hallway, dry, sets = [], None, False, {}
    i = 0
    while i < len(args):
        if args[i] == "--unit" and i + 1 < len(args):
            units.append(args[i + 1]); i += 2
        elif args[i] == "--hallway" and i + 1 < len(args):
            hallway = args[i + 1]; i += 2
        elif args[i] == "--set" and i + 1 < len(args) and re.fullmatch(r"[a-z0-9_]+=[^\t\n]*", args[i + 1]):
            k, _, v = args[i + 1].partition("=")
            sets[k] = v; i += 2
        elif args[i] == "--dry-run":
            dry = True; i += 1
        else:
            sys.exit(__doc__)
    now = datetime.datetime.now(datetime.timezone.utc).replace(microsecond=0)
    keys = {
        "date": now.strftime("%Y-%m-%dT%H:%M:%SZ"),
        "commit": git("rev-parse", "--short=12", "HEAD") + ("+changes" if git("status", "--porcelain", "--untracked-files=no") else ""),
        "version": version(),
        # The kind of machine, not which one: the system, its release and the processor's family.
        "machine": " ".join(x for x in (platform.system(), platform.release(), platform.machine()) if x),
    }
    cpu = cpu_name()
    keys.update({"machine_profile": machine_profile(cpu), "cpu": cpu, "logical_processors": str(os.cpu_count() or ""),
                 "memory_gb": memory_gb(), "gpu": gpu_names()})
    keys["kind"] = "tests"
    keys.update(sets)
    checks, builds = unit_checks(units)
    if checks is not None:
        keys["unit_checks"] = str(checks)
        keys["unit_builds"] = " ".join(builds)
    keys.update({k: str(v) for k, v in repository_counts().items()})
    listing = ""
    if hallway and os.path.exists(hallway):
        listing, kept = hallway_metrics(hallway)
        keys.update(kept)
    record = FORMAT + "\n" + "".join(f"{k} = {v}\n" for k, v in keys.items() if v != "")
    if listing:
        record += "hallway-metrics\n" + listing
    if dry:
        print(record, end="")
        return 0
    os.makedirs(BUILDS, exist_ok=True)
    path = os.path.join(BUILDS, now.strftime("%Y-%m-%d-%H%M%S") + ".txt")
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(record)
    n = write_index()
    print(f"metrics logged: {os.path.relpath(path, ROOT)} ({n} build{'' if n == 1 else 's'} on the record in metrics/index.tsv)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
