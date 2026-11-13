#!/usr/bin/env python3
"""The measured-figure record's grammar as a filter plugin (IDEAS 19.185's first step), held to the
reader: every line of a corpus judged twice, by `sieve catalogue --check-records` (read_format_stat,
the reader itself) and by `sieve filters --plugin data/filters/format-stat-record-v1.sfilter --judge`
(the plugin, on an ascii95 line of the line's own length), and the verdicts compared. They must agree
on every line but one kind: a record whose keys are not in increasing order, each once, which the
reader refuses and the plugin cannot see (a walk would have to remember a whole key). Those must be
refused by the reader for that reason alone, and passed by the plugin.

The corpus: records made at random (seeded, so every run has the same), every edge of the grammar
(the shortest, words of 64 and 65, numbers of 19 and 20 digits, empty fields, double spaces), and
each record changed by one character every way (deleted, doubled, replaced by each of a few others).

    python tools/check_stat_plugin.py SIEVE [WORK_FOLDER]
"""
import os
import random
import subprocess
import sys

WORD = "abcdefghijklmnopqrstuvwxyz0123456789._-"


def word(r, n=None):
    return "".join(r.choice(WORD) for _ in range(n or r.randint(1, 12)))


def record(r, keys=None):
    date = f"{r.randint(0, 9999):04d}-{r.randint(0, 99):02d}-{r.randint(0, 99):02d}"
    t = f"sieve-format-stat-v1 date={date} machine={word(r)} what={word(r)} result={word(r)}"
    ks = sorted(set(keys if keys is not None else [word(r) for _ in range(r.randint(0, 4))]))
    for k in ks:
        t += f" {k}={r.randint(0, 10 ** r.randint(1, 19) - 1)}"
    return t


def corpus():
    r = random.Random(185)
    lines = [record(r) for _ in range(300)]
    base = "sieve-format-stat-v1 date=2026-11-13 machine=desk what=pack result=lossless"
    lines += [
        "sieve-format-stat-v1 date=0000-00-00 machine=a what=a result=a",  # the shortest
        base,
        base + " bytes=1",
        base + " bytes=0000000000000000001",  # 19 digits, leading zeros
        base + " bytes=12345678901234567890",  # 20 digits
        base + " " + "k" * 64 + "=1",
        base + " " + "k" * 65 + "=1",
        base.replace("machine=desk", "machine=" + "m" * 64),
        base.replace("machine=desk", "machine=" + "m" * 65),
        base.replace("what=pack", "what=" + "w" * 64),
        base.replace("result=lossless", "result=" + "r" * 65),
        base.replace("machine=desk", "machine="),
        base + " ",
        base.replace(" what", "  what"),
        base + " bytes=",
        base + " =5",
        base + " bytes=5=5",
        base + " a=1 b=2",
        base + " b=1 a=2",  # out of order: the reader alone refuses
        base + " a=1 a=2",  # the same key twice: the reader alone refuses
        base + " a=1 b=2 b=3",
        base + " Bytes=1",
        base.replace("date=2026-11-13", "date=2026-1-13"),
        base.replace("date=2026-11-13", "date=2026/11/13"),
        base.replace("machine=desk", "machine=Desk"),
        base.replace("sieve-format-stat-v1", "sieve-format-stat-v2"),
        base.replace("date=", "Date="),
        "sieve-format-stat-v1 machine=desk date=2026-11-13 what=pack result=lossless",
        "sieve-format-stat-v1 date=2026-11-13 machine=desk what=pack",
    ]
    others = " =a5Z-./"
    for t in list(lines[:60]) + [base + " bytes=10 sieve=4"]:
        for i in range(len(t)):
            lines.append(t[:i] + t[i + 1:])
            lines.append(t[:i] + t[i] + t[i:])
            for c in others:
                if c != t[i]:
                    lines.append(t[:i] + c + t[i + 1:])
    # Shuffled keys, so the order rule is met often on purpose.
    for _ in range(100):
        ks = [word(r) for _ in range(r.randint(2, 4))]
        t = record(r, ks)
        head, figs = t.split(" result=")
        res, *pairs = figs.split(" ")
        r.shuffle(pairs)
        lines.append(head + " result=" + res + "".join(" " + p for p in pairs))
    seen, out = set(), []
    for l in lines:
        if l and not l.startswith("#") and l not in seen and all(" " <= c <= "~" for c in l):
            seen.add(l)
            out.append(l)
    return out


def main():
    sieve = sys.argv[1]
    work = sys.argv[2] if len(sys.argv) > 2 else "."
    here = os.path.dirname(os.path.abspath(__file__))
    plugin = os.path.join(here, "..", "data", "filters", "format-stat-record-v1.sfilter")
    lines = corpus()
    path = os.path.join(work, "stat-records.txt")
    with open(path, "w", encoding="ascii", newline="\n") as f:
        f.write("\n".join(lines) + "\n")
    # (bytes, decoded as UTF-8 here: a reason may hold any character, whatever the system's code page)
    rr = subprocess.run([sieve, "catalogue", "--check-records", path], capture_output=True)
    jr = subprocess.run([sieve, "filters", "--plugin", plugin, "--length", "100", "--judge", path], capture_output=True)
    reader = [l for l in rr.stdout.decode("utf-8", "replace").splitlines() if l.startswith("record ")]
    judge = [l for l in jr.stdout.decode("utf-8", "replace").splitlines() if l.startswith("judge ")]
    if not len(reader) == len(lines) == len(judge):
        # Say why, from each program's own words, rather than only the counts.
        raise SystemExit(f"verdicts: {len(reader)} from the reader (exit {rr.returncode}), {len(judge)} from the plugin (exit {jr.returncode}), "
                         f"for {len(lines)} lines\nthe reader said: {rr.stderr.decode('utf-8', 'replace')[-2000:]}\n"
                         f"the plugin said: {jr.stderr.decode('utf-8', 'replace')[-2000:]}")
    agree = order = 0
    for line, rv, jv in zip(lines, reader, judge):
        r_pass = rv.split()[1] == "pass"
        j_pass = jv.split()[1] == "pass"
        if r_pass == j_pass:
            agree += 1
            continue
        if not r_pass and j_pass and rv.endswith("(its keys are not in order, each once)"):
            order += 1
            continue
        raise SystemExit(f"the plugin and the reader differ:\n  {line}\n  reader: {rv}\n  plugin: {jv}")
    passed = sum(1 for l in reader if l.split()[1] == "pass")
    print(f"{len(lines)} lines: {agree} judged alike ({passed} records), {order} refused by the reader for their keys' order alone")


if __name__ == "__main__":
    main()
