"""Where a CI step's time goes: its .trace (ci/run.sh writes one beside each step's log, each command
as it starts, after the time it started) added up by block, a block being a step's lines from one
comment that follows a command to the next.

  python ci/trace_times.py LOGS/11-oracle.trace ci/steps/11-oracle.sh [--top N]

Each command's time is until the next command started, so a block's is what its commands took,
waiting on what they ran included. Commands in functions and other files count to the step's line
that called them, as far as the trace shows it (the last line of the step's own file before them).
"""

import argparse
import re
import sys


def blocks(script):
    # Each line's block: the line of the comment that begins it, and that comment's first line.
    first, title, out = 1, "(the step's start)", {}
    lines = open(script, encoding="utf-8").read().split("\n")
    after_command = True
    for n, line in enumerate(lines, 1):
        comment = line.lstrip().startswith("#")
        if comment and after_command and not line.startswith("#!"):
            first, title = n, line.strip("# ").strip()
        after_command = not comment and bool(line.strip())
        out[n] = (first, title)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("trace")
    ap.add_argument("script")
    ap.add_argument("--top", type=int, default=15)
    a = ap.parse_args()
    name = a.script.replace("\\", "/").rsplit("/", 1)[-1]
    of = blocks(a.script)
    pat = re.compile(r"^\++ (\d+\.\d+) (\S+?):(\d+): ")
    marks = []
    for line in open(a.trace, encoding="utf-8", errors="replace"):
        m = pat.match(line)
        if m:
            marks.append((float(m.group(1)), m.group(2), int(m.group(3))))
    if len(marks) < 2:
        sys.exit("no times in the trace (it needs bash 5, and ci/run.sh's trace)")
    spent, last_own = {}, 1
    for (t, f, n), (t2, _, _) in zip(marks, marks[1:]):
        if f == name:
            last_own = n
        b = of.get(last_own, (last_own, "?"))
        spent[b] = spent.get(b, 0.0) + (t2 - t)
    total = marks[-1][0] - marks[0][0]
    print(f"{total:9.1f} s  in all, from the first command to the last one's start")
    for (first, title), s in sorted(spent.items(), key=lambda kv: -kv[1])[:a.top]:
        print(f"{s:9.1f} s  {100 * s / total:5.1f}%  line {first}: {title[:90]}")


if __name__ == "__main__":
    main()
