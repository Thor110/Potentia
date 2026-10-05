#!/usr/bin/env python3
"""The filter survey: how much each filter, and each stack of them, keeps of each line, at a grid of
settings, on a given machine.

It runs the `sieve` tool itself (`sieve filters`, the command the setup menu's counts agree with),
so the figures are the engine's own, exact where the engine counts exactly. A hardware profile
bounds every count the way the setup menu does on that machine: the filter memory one count's
tables may take, a time limit for each count, and how many counts run at once. A count past either
bound is recorded as such, with what it would have needed, so the tables show where the attainable
edge lies on that hardware rather than leaving gaps.

    python tools/survey/survey.py                      # the average profile, the default grid
    python tools/survey/survey.py --profile low
    python tools/survey/survey.py --ram-gb 12 --cores 6 --filter-memory 1024 --time-limit 90
    python tools/survey/survey.py --grid my-grid.tsv --only image,audio --dry-run

The grid (tools/survey/grid.tsv) and the profiles (tools/survey/profiles.tsv) are data; see their
headers. Results go to results/survey-<profile>-<date>.tsv (every row) and .md (tables by line).
"""

import argparse
import concurrent.futures
import datetime
import itertools
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from fractions import Fraction

# Survivor counts run to thousands of digits; Python 3.11 limits reading such numbers unless told not to.
if hasattr(sys, "set_int_max_str_digits"):
    sys.set_int_max_str_digits(0)

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))  # Sieve/
LINES = ("text", "image", "audio", "video", "models", "binary")


# ---------------------------------------------------------------- the grid and the profiles

def read_table(path):
    """Rows of a '|'-separated file, '#' comments and blank lines left out."""
    rows = []
    with open(path, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            line = raw.split("#", 1)[0].strip()
            if line:
                rows.append((n, [c.strip() for c in line.split("|")]))
    return rows


def expand_value(v):
    """'a,b,c' -> list; 'lo..hi' -> every integer; 'lo..hi:step' -> by adding step; 'lo..hi*k' ->
    by multiplying by k. Ranges include both ends."""
    out = []
    for part in v.split(","):
        m = re.fullmatch(r"(\d+)\.\.(\d+)(?:([:*])(\d+))?", part)
        if not m:
            out.append(part)
            continue
        lo, hi, op, k = int(m[1]), int(m[2]), m[3], int(m[4] or 1)
        if k < 1 or (op == "*" and k < 2):
            raise ValueError(f"a range's step must move it: {part}")
        x = lo
        while x <= hi:
            out.append(str(x))
            x = x * k if op == "*" else x + k
    return out


def expand_settings(text):
    """'width=5..7 height==width palette=mono,ega16' -> every combination, as lists of (key, value).
    'key==other' takes the other setting's value in each combination."""
    order, free, tied = [], [], []
    for tok in text.split():
        key, _, value = tok.partition("=")
        if not key or not _:
            raise ValueError(f"a setting is key=value: {tok}")
        order.append(key)
        if value.startswith("="):
            tied.append((key, value[1:]))
        else:
            free.append((key, expand_value(value)))
    for combo in itertools.product(*(vals for _, vals in free)):
        chosen = dict(zip((k for k, _ in free), combo))
        for key, other in tied:
            if other not in chosen:
                raise ValueError(f"{key}=={other}: there is no setting {other}")
            chosen[key] = chosen[other]
        yield [(k, chosen[k]) for k in order]  # in the order the grid gives them


def parse_stacks(text):
    """'each, all, a-v1+b-v1, c-v1[p=1,2]' -> stack specs: ('each',), ('all',), or
    ('list', [(name, {param: [values]})...])."""
    specs = []
    for item in re.split(r",(?![^\[]*\])", text):
        item = item.strip()
        if not item:
            continue
        if item in ("each", "all"):
            specs.append((item,))
            continue
        members = []
        for m in item.split("+"):
            mm = re.fullmatch(r"([A-Za-z0-9_.-]+)(?:\[(.*)\])?", m.strip())
            if not mm:
                raise ValueError(f"not a filter: {m}")
            params = {}
            for p in (mm[2] or "").split(";"):
                if p.strip():
                    k, _, v = p.partition("=")
                    params[k.strip()] = expand_value(v.strip())
            members.append((mm[1], params))
        specs.append(("list", members))
    return specs


def read_profiles(path):
    """name -> {ram_gb, cores, filter_memory_mb, time_limit_s}."""
    profiles = {}
    for n, cols in read_table(path):
        if len(cols) != 5:
            raise ValueError(f"{path}:{n}: a profile is name | ram_gb | cores | filter_memory_mb | time_limit_s")
        profiles[cols[0]] = {"ram_gb": float(cols[1]), "cores": int(cols[2]), "filter_memory_mb": int(cols[3]),
                             "time_limit_s": float(cols[4])}
    return profiles


# ---------------------------------------------------------------- running sieve

def sieve_exe(given):
    if given:
        return given
    for p in ("build/sieve", "build/Release/sieve.exe", "build/sieve.exe"):
        full = os.path.join(ROOT, p)
        if os.path.isfile(full):
            return full
    found = shutil.which("sieve")
    if found:
        return found
    sys.exit("survey: no sieve program found (build it, or give --sieve PATH)")


def run(cmd, time_limit):
    """(exit code or None if stopped at the time limit, stdout, stderr, seconds, peak MB or None)."""
    t0 = time.monotonic()
    p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    if hasattr(os, "wait4"):
        # Reading the pipes on threads, so a large output cannot fill them while we wait.
        out_box, err_box = [], []
        import threading
        ts = [threading.Thread(target=lambda: out_box.append(p.stdout.read())),
              threading.Thread(target=lambda: err_box.append(p.stderr.read()))]
        for t in ts:
            t.start()
        peak, code = None, None
        while True:
            pid, status, usage = os.wait4(p.pid, os.WNOHANG)
            if pid:
                code = os.waitstatus_to_exitcode(status)
                # ru_maxrss: kilobytes on Linux, bytes on macOS.
                peak = usage.ru_maxrss / (1024 * 1024 if sys.platform == "darwin" else 1024)
                break
            if time.monotonic() - t0 > time_limit:
                p.kill()
                os.wait4(p.pid, 0)
                break
            time.sleep(0.02)
        for t in ts:
            t.join()
        p.returncode = code if code is not None else -9
        return code, (out_box or [""])[0], (err_box or [""])[0], time.monotonic() - t0, peak
    try:
        out, err = p.communicate(timeout=time_limit)
        return p.returncode, out, err, time.monotonic() - t0, None
    except subprocess.TimeoutExpired:
        p.kill()
        out, err = p.communicate()
        return None, out, err, time.monotonic() - t0, None


def line_args(line, settings):
    args = ["--line", line]
    for k, v in settings:
        args += [f"--{k}", v]
    return args


def available_filters(exe, line, settings):
    """The filters the line offers, retired ones left out."""
    code, out, err, _, _ = run([exe, "filters"] + line_args(line, settings), 120)
    if code != 0:
        raise RuntimeError((err or out).strip().splitlines()[-1] if (err or out).strip() else "sieve filters failed")
    names = []
    for l in out.splitlines():
        m = re.match(r"\[[ x]\] (\S+)", l)
        if m and "(retired" not in l:
            names.append(m[1])
    return names


def stacks_for(spec, names):
    """A stack spec -> concrete stacks: lists of (name, {param: value})."""
    if spec[0] == "each":
        return [[(n, {})] for n in names]
    if spec[0] == "all":
        return [[(n, {}) for n in names]] if names else []
    out = []
    members = [m for m in spec[1] if m[0] in names or any(n.startswith(m[0] + "-v") for n in names)]
    if len(members) != len(spec[1]):
        return []  # a filter this line does not offer
    per_member = []
    for name, params in members:
        full = name if name in names else next(n for n in names if n.startswith(name + "-v"))
        keys = list(params)
        per_member.append([(full, dict(zip(keys, vals))) for vals in itertools.product(*(params[k] for k in keys))] or [(full, {})])
    for combo in itertools.product(*per_member):
        out.append(list(combo))
    return out


def settings_file(line, stack, folder):
    fd, path = tempfile.mkstemp(suffix=".ini", dir=folder)
    with os.fdopen(fd, "w", encoding="utf-8") as f:
        f.write(f"[{line}]\nmode = mark\nfilters = {', '.join(n for n, _ in stack)}\n")
        for n, params in stack:
            if params:
                f.write(f"\n[{line}.{n}]\n")
                for k, v in params.items():
                    f.write(f"{k} = {v}\n")
    return path


def percent(fr):
    """A share as a percentage, with as many decimals as it takes to get past the leading 9s or 0s."""
    if fr == 0:
        return "0%"
    if fr == 1:
        return "100%"
    p = fr * 100
    whole = int(p)
    rest = p - whole
    digits = ""
    lead = None
    for _ in range(4000):
        rest *= 10
        d = int(rest)
        rest -= d
        digits += str(d)
        if lead is None and d not in (0, 9):
            lead = len(digits)
        if lead is not None and len(digits) >= lead + 1:
            break
    return f"{whole}.{digits}%"


def log10_fraction(fr):
    if fr == 0:
        return "-inf"
    n, d = fr.numerator, fr.denominator
    shift = max(0, max(n.bit_length(), d.bit_length()) - 900)
    import math
    return f"{math.log10(n >> shift if n >> shift else 1) - math.log10(d >> shift if d >> shift else 1):.2f}"


def survey_one(exe, line, settings, stack, profile, folder):
    ini = settings_file(line, stack, folder)
    cmd = [exe, "filters"] + line_args(line, settings) + ["--filters", ini, "--filter-memory", str(profile["filter_memory_mb"])]
    code, out, err, secs, peak = run(cmd, profile["time_limit_s"])
    os.remove(ini)
    row = {"line": line, "settings": " ".join(f"{k}={v}" for k, v in settings), "stack": " + ".join(
        n + ("[" + ";".join(f"{k}={v}" for k, v in p.items()) + "]" if p else "") for n, p in stack),
        "status": "", "kept_log10": "", "kept": "", "removed": "", "seconds": f"{secs:.2f}",
        "peak_mb": "" if peak is None else f"{peak:.0f}", "note": ""}
    if code is None:
        row["status"] = "over time"
        row["note"] = f"stopped at {profile['time_limit_s']:g} s"
        return row
    if code != 0:
        row["status"] = "error"
        # The error itself, not the hint after it ("(see: sieve help ...)").
        lines = [l for l in (err or out).strip().splitlines() if l.strip() and not l.startswith("(see:")]
        row["note"] = (lines or ["?"])[-1]
        return row
    surv = re.search(r"^survivors\s+(\d+)", out, re.M)
    excl = re.search(r"^excluded\s+(\d+)", out, re.M)
    blocked = re.search(r"^compact\s+unavailable:\s*(.*)$", out, re.M)
    if surv and excl:
        s, e = int(surv[1]), int(excl[1])
        kept = Fraction(s, s + e)
        row.update(status="exact", kept_log10=log10_fraction(kept), kept=percent(kept), removed=percent(1 - kept))
    elif blocked:
        row["status"] = "judge only"
        row["note"] = blocked[1].strip()
    else:
        row["status"] = "no count"
    return row


def survey_safely(exe, line, settings, stack, profile, folder):
    """survey_one, with anything it could not do recorded as that row's error."""
    try:
        return survey_one(exe, line, settings, stack, profile, folder)
    except Exception as e:  # one count's trouble is that row's, not the survey's
        return {"line": line, "settings": " ".join(f"{k}={v}" for k, v in settings),
                "stack": " + ".join(n for n, _ in stack), "status": "error", "kept_log10": "", "kept": "", "removed": "",
                "seconds": "", "peak_mb": "", "note": f"{type(e).__name__}: {e}"}


# ---------------------------------------------------------------- output

COLUMNS = ("line", "settings", "stack", "status", "kept_log10", "kept", "removed", "seconds", "peak_mb", "note")


def write_tsv(path, rows, header):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        for h in header:
            f.write(f"# {h}\n")
        f.write("\t".join(COLUMNS) + "\n")
        for r in rows:
            f.write("\t".join(str(r[c]).replace("\t", " ") for c in COLUMNS) + "\n")


def write_md(path, rows, header):
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Filter survey\n\n")
        for h in header:
            f.write(f"- {h}\n")
        for line in LINES:
            mine = [r for r in rows if r["line"] == line]
            if not mine:
                continue
            f.write(f"\n## {line}\n\n| Settings | Stack | Kept | Removed | log10 kept | Status | Time |\n| :--- | :--- | ---: | ---: | ---: | :--- | ---: |\n")
            for r in mine:
                status = r["status"] + (f": {r['note']}" if r["note"] else "")
                f.write(f"| {r['settings']} | {r['stack']} | {r['kept']} | {r['removed']} | {r['kept_log10']} | {status} | {r['seconds']} s |\n")


# ---------------------------------------------------------------- main

def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--sieve", help="the sieve program (default: build/sieve, build/Release/sieve.exe, the PATH)")
    ap.add_argument("--grid", default=os.path.join(HERE, "grid.tsv"))
    ap.add_argument("--profiles", default=os.path.join(HERE, "profiles.tsv"))
    ap.add_argument("--profile", default="average", help="a profile's name in the profiles file (default average)")
    ap.add_argument("--ram-gb", type=float, help="this machine's memory: overrides the profile's")
    ap.add_argument("--cores", type=int, help="this machine's cores: overrides the profile's")
    ap.add_argument("--filter-memory", type=int, help="MB one count's tables may take: overrides the profile's")
    ap.add_argument("--time-limit", type=float, help="seconds one count may take: overrides the profile's")
    ap.add_argument("--only", help="lines to survey, comma separated (default every line in the grid)")
    ap.add_argument("--out", help="the results' path without extension (default results/survey-<profile>-<date>)")
    ap.add_argument("--dry-run", action="store_true", help="list what would be counted, and count nothing")
    a = ap.parse_args()

    profiles = read_profiles(a.profiles)
    if a.profile not in profiles:
        sys.exit(f"survey: no profile '{a.profile}' in {a.profiles} (there are: {', '.join(profiles)})")
    prof = dict(profiles[a.profile])
    for key, val in (("ram_gb", a.ram_gb), ("cores", a.cores), ("filter_memory_mb", a.filter_memory), ("time_limit_s", a.time_limit)):
        if val is not None:
            prof[key] = val
    custom = any(v is not None for v in (a.ram_gb, a.cores, a.filter_memory, a.time_limit))
    name = a.profile + ("-custom" if custom else "")
    # As many counts at once as the cores allow and the memory holds: each may take its filter
    # memory, and three quarters of the machine's memory is theirs.
    jobs = max(1, min(prof["cores"], int(prof["ram_gb"] * 1024 * 0.75 // max(1, prof["filter_memory_mb"]))))
    only = set(a.only.split(",")) if a.only else None
    exe = sieve_exe(a.sieve)

    work = []
    for n, cols in read_table(a.grid):
        if len(cols) != 3:
            sys.exit(f"survey: {a.grid}:{n}: a row is line | settings | stacks")
        line, settings_text, stacks_text = cols
        if line not in LINES:
            sys.exit(f"survey: {a.grid}:{n}: no line '{line}' (lines: {', '.join(LINES)})")
        if only and line not in only:
            continue
        specs = parse_stacks(stacks_text)
        for settings in expand_settings(settings_text):
            try:
                names = available_filters(exe, line, settings)
            except RuntimeError as e:
                work.append((line, settings, None, str(e)))
                continue
            seen = set()
            for spec in specs:
                for stack in stacks_for(spec, names):
                    key = tuple((s, tuple(sorted(p.items()))) for s, p in stack)
                    if key not in seen:
                        seen.add(key)
                        work.append((line, settings, stack, None))

    header = [f"profile {name}: {prof['ram_gb']:g} GB, {prof['cores']} cores, filter memory {prof['filter_memory_mb']} MB, "
              f"{prof['time_limit_s']:g} s a count, {jobs} at once",
              f"sieve {exe}", f"grid {os.path.relpath(a.grid, ROOT)}", f"made {datetime.datetime.now().isoformat(timespec='seconds')}",
              f"counts {len(work)}"]
    if a.dry_run:
        for h in header:
            print(h)
        for line, settings, stack, why in work:
            print(f"{line}\t{' '.join(f'{k}={v}' for k, v in settings)}\t"
                  f"{why or ' + '.join(n + (str(p) if p else '') for n, p in stack)}")
        return

    folder = tempfile.mkdtemp(prefix="sieve-survey-")
    rows = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=jobs) as pool:
        futures = []
        for line, settings, stack, why in work:
            if stack is None:
                rows.append({"line": line, "settings": " ".join(f"{k}={v}" for k, v in settings), "stack": "", "status": "error",
                             "kept_log10": "", "kept": "", "removed": "", "seconds": "", "peak_mb": "", "note": why})
                continue
            futures.append(pool.submit(survey_safely, exe, line, settings, stack, prof, folder))
        for i, fut in enumerate(futures, 1):
            r = fut.result()
            rows.append(r)
            print(f"[{i}/{len(futures)}] {r['line']} {r['settings']} {r['stack']}: {r['status']} {r['kept']} {r['note']}", flush=True)
    shutil.rmtree(folder, ignore_errors=True)
    order = {k: i for i, k in enumerate(LINES)}
    rows.sort(key=lambda r: (order[r["line"]], r["settings"], r["stack"]))
    base = a.out or os.path.join(ROOT, "results", f"survey-{name}-{datetime.date.today().isoformat()}")
    os.makedirs(os.path.dirname(base), exist_ok=True)
    write_tsv(base + ".tsv", rows, header)
    write_md(base + ".md", rows, header)
    print(f"wrote {base}.tsv and {base}.md")


if __name__ == "__main__":
    main()
