#!/usr/bin/env python3
"""Damaged .sieve files given to a reader: each must be refused, or install the original files.

usage: damage_test.py READER SAMPLES_DIR OUT_DIR COUNT SEED [--timeout S] [--kind sieve|si|oracle]
                      [--only NAME,NAME]

For each sample SAMPLES_DIR/X.sieve, whose clean install is in SAMPLES_DIR/base-X, COUNT damaged
copies, each installed by the reader into a fresh folder: READER is sieve (--kind sieve),
sieve-install (--kind si) or reference/sieve_ref.py (--kind oracle). The damage: a bit flipped, a
byte changed, the file cut short, a few bytes overwritten, a byte inserted or deleted, or one
character of a mixing parameter changed (W1 to W7, B16 to B19, ...); half of it in the first 400
bytes, where the head and listing are.

Outcomes: refused (a non-zero exit, no signal, no sanitizer report), same (installed, the files
equal the clean install), DIFFERENT (installed, not equal), CRASH (a signal, or a report from a
build with -fsanitize=address,undefined), HANG (over --timeout). The bad ones are kept in OUT_DIR
as BAD-*.sieve, with what was done to each and what the reader said in OUT_DIR/bad.json.
Measured with it: results/damaged-v33-2026-11-03.md.
"""
import argparse, filecmp, json, os, random, re, shutil, subprocess, sys
from pathlib import Path

ap = argparse.ArgumentParser()
ap.add_argument("reader"); ap.add_argument("samples"); ap.add_argument("out")
ap.add_argument("count", type=int); ap.add_argument("seed", type=int)
ap.add_argument("--timeout", type=float, default=30)
ap.add_argument("--kind", default="sieve")
ap.add_argument("--only", default="")
a = ap.parse_args()
samples, out = Path(a.samples), Path(a.out)
out.mkdir(parents=True, exist_ok=True)
rng = random.Random(a.seed)

PARAM = re.compile(rb"(mix[4tn]|W1|R1|M4|C2|B\d+|F80|I\d+x\d)")


def damage(b):
    b = bytearray(b)
    n = len(b)
    head = min(n, 400)  # half the damage in the head and listing, where the parameters are
    pos = lambda: rng.randrange(head) if rng.random() < 0.5 else rng.randrange(n)
    k = rng.randrange(8)
    if k == 0:
        i = pos(); b[i] ^= 1 << rng.randrange(8); what = f"bit at {i}"
    elif k == 1:
        i = pos(); b[i] = rng.randrange(256); what = f"byte at {i}"
    elif k == 2:
        i = rng.randrange(1, n); del b[i:]; what = f"cut at {i}"
    elif k == 3:
        i = pos(); m = rng.randint(2, 8); b[i:i + m] = bytes(rng.randrange(256) for _ in range(m)); what = f"{m} bytes at {i}"
    elif k == 4:
        i = pos(); b.insert(i, rng.randrange(256)); what = f"inserted at {i}"
    elif k == 5:
        i = pos(); del b[i]; what = f"deleted at {i}"
    else:  # a parameter of the mixing written differently (W1 to W2, B16 to B30, ...)
        hits = [m for m in PARAM.finditer(bytes(b))]
        if not hits:
            i = pos(); b[i] ^= 0xFF; what = f"inverted at {i}"
        else:
            m = rng.choice(hits); s, e = m.span()
            i = rng.randrange(s, e); old = b[i]
            b[i] = rng.choice(b"0123456789WRMCBFIXx ")
            what = f"parameter {m.group().decode(errors='replace')} at {i}: {chr(old)} to {chr(b[i])}"
    return bytes(b), what


def same(x, y):
    c = filecmp.dircmp(x, y)
    if c.left_only or c.right_only or c.diff_files or c.funny_files:
        return False
    if any(not filecmp.cmp(Path(x) / f, Path(y) / f, shallow=False) for f in c.common_files):
        return False
    return all(same(Path(x) / d, Path(y) / d) for d in c.common_dirs)


tally, bad = {}, []
for sample in sorted(samples.glob("*.sieve")):
    name = sample.stem
    if a.only and name not in a.only.split(","):
        continue
    base = samples / f"base-{name}"
    clean = sample.read_bytes()
    for t in range(a.count):
        d, what = damage(clean)
        f = out / f"{name}-{t}.sieve"
        f.write_bytes(d)
        to = out / f"to-{name}-{t}"
        shutil.rmtree(to, ignore_errors=True)
        if a.kind == "sieve":
            cmd = [a.reader, "install", str(f), "--to", str(to)]
        elif a.kind == "si":
            cmd = [a.reader, str(f), "--to", str(to), "--yes"]
        else:
            cmd = [sys.executable, a.reader, "unpack", str(f), "--to", str(to)]
        env = dict(os.environ, SDL_VIDEODRIVER="dummy", ASAN_OPTIONS="detect_leaks=0:abort_on_error=0",
                   UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1")
        try:
            r = subprocess.run(cmd, capture_output=True, timeout=a.timeout, env=env,
                               cwd=str(Path(a.reader).parent) if a.kind == "oracle" else None)
            err = r.stderr.decode(errors="replace") + r.stdout.decode(errors="replace")
            if r.returncode < 0 or "Sanitizer" in err or "runtime error:" in err:
                outcome = "CRASH"
            elif r.returncode != 0:
                outcome = "refused"
            else:
                outcome = "same" if to.is_dir() and same(base, to) else "DIFFERENT"
        except subprocess.TimeoutExpired:
            outcome, err = "HANG", ""
        tally[(name, outcome)] = tally.get((name, outcome), 0) + 1
        if outcome in ("CRASH", "DIFFERENT", "HANG"):
            keep = out / f"BAD-{outcome}-{name}-{t}.sieve"
            shutil.copy(f, keep)
            bad.append({"sample": name, "t": t, "outcome": outcome, "damage": what, "file": str(keep), "said": err[-1500:]})
        else:
            f.unlink()
        shutil.rmtree(to, ignore_errors=True)
for k in sorted(tally):
    print(f"{k[0]:10} {k[1]:10} {tally[k]}")
print(f"{len(bad)} bad")
(out / "bad.json").write_text(json.dumps(bad, indent=1))
