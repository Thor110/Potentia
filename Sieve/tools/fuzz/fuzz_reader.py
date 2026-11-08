"""Fuzzes Sieve's reader (6 November 2026): valid .sieve files mutated, their raw bytes and their
listing as the text the reader parses (edge numbers, ways, paths, lines deleted, doubled, swapped or
spliced from another seed), each installed by a sanitizer build (ASan and UBSan), and whatever
crashes, hangs or reports is kept (results/fuzz-2026-11-06.md).

  python -I tools/fuzz/fuzz_reader.py SIEVE SEEDS OUT SECONDS     (FUZZ_WORKERS=3, say)

SIEVE: a sieve built with -fsanitize=address,undefined (its own build folder and its own _deps),
with the dictionaries beside it. SEEDS: a folder of .sieve files (the CI's, under 1.5 MB, do well).
OUT: where findings go (find-*.txt with the case beside it; stats-*.txt as it runs; limits.txt).
Each seed is first installed as it is: three times its own time (at least 30 s) is its limit, and
seeds over 20 s are left out, so a slow seed is not taken for a hang. The LZMA SDK's deliberate
unaligned loads and stores are suppressed (ubsan.supp beside this). Note that ASan's own cap on one
allocation (2 GiB here) is below the installer's bound (half the memory available), so a listing
declaring a file of 2 to 4 GiB is reported though the installer allows it; those are read and set
aside, not fixed.
"""
import os, random, re, shutil, subprocess, sys, time, multiprocessing as mp
from pathlib import Path

HERE = Path(__file__).resolve().parent
SIEVE = sys.argv[1]        # the sanitizer build's sieve
SEEDS = sys.argv[2]        # folder of seed .sieve files
OUT = sys.argv[3]          # findings
SECONDS = float(sys.argv[4])
sys.path.insert(0, str(HERE.parent.parent / "reference"))
import sieve_ref as ref  # noqa: E402

EDGE = [0, 1, 2, 3, 7, 8, 13, 16, 31, 32, 63, 64, 127, 128, 255, 256, 4095, 4096, 65535, 65536,
        2**20, 2**24, 2**31 - 1, 2**31, 2**32 - 1, 2**32, 2**32 + 1, 2**40, 2**53, 2**62, 2**63 - 1, 2**63, 2**64 - 1]
WAYS = ["raw", "x86", "rope L8 W1 S1 P1 K0", "rope L8 W4096 S4 P3 K9", "pieces z1 r1 rope L8 W1 S1 P1 K0", "mix4 B16", "mix4 B30",
        "mix4 B22 I0x3", "mix4 B22 I65535x4 P256", "mix4 B16 D60", "mix4 B16 F80 W1", "mixt B16 T99999999", "mixn", "runs", "copy 0", "copy 99999",
        "delta 0 1", "delta 1 99999999", "gif 254 13 13 raw", "gif 1 271 192271 raw", "flac 12 13 raw", "flac 80 95211 raw", "tiff 10 20 raw", "tiff 0 4096 raw", "tiff 453 43463 raw", "pcx 129 7042 raw", "pcx 128 128 raw", "pcx 999999 18561 raw", "pmp 10334 raw", "pmp 1 raw", "pmp 99999999 raw", "spn 1 10 raw", "skm 2 10 raw", "skr 10 2 1 1 13 raw",
        "pngr 5 0 33 33 40 raw", "stb 9 0 1 1 1 1 raw", "jbr 10 raw", "sk 10 20 raw", "xz 6 1 10 raw", "gz 9 8 0 10 raw", "zst 1.5.6 3 0 0 10 raw",
        "lz4 1 4 0 0 0 0 10 raw", "br 11 24 10 raw", "lzma 9e 10 raw", "bz2 9 10 raw", "item 0 0", "lines 0"]
PATHS = ["../x", "a/../../b", "/abs", "C:\\x", "con.txt", "a//b", "", ".", "a\\..\\..\\b", "x" * 300, "a/b/" * 40 + "c", "aux", "dir/"]


def seeds():
    out = []
    for name in sorted(os.listdir(SEEDS)):
        raw = open(os.path.join(SEEDS, name), "rb").read()
        try:
            text = ref.v16_text(ref.instructions_bytes(raw))
        except BaseException:
            text = None
        out.append((name, raw, text))
    return out


def mutate_number(tok, rnd):
    n = int(tok)
    c = rnd.random()
    if c < 0.5:
        v = rnd.choice(EDGE)
    elif c < 0.8:
        v = max(0, n + rnd.choice([-2, -1, 1, 2]))
    elif c < 0.9:
        v = n * rnd.choice([2, 10, 1000])
    else:
        return "9" * rnd.randint(19, 40)
    return str(v)


def mutate_text(text, others, rnd):
    end = text.find(b"\nend\n")
    if end < 0:
        return None
    head, tail = text[:end + 1].decode("utf-8", "replace"), text[end + 1:]
    lines = head.split("\n")[:-1]
    for _ in range(rnd.randint(1, 3)):
        k = rnd.randrange(1, len(lines))
        c = rnd.random()
        parts = re.split(r"([\t ])", lines[k])
        nums = [i for i, p in enumerate(parts) if p.isdigit()]
        if c < 0.45 and nums:
            i = rnd.choice(nums)
            parts[i] = mutate_number(parts[i], rnd)
            lines[k] = "".join(parts)
        elif c < 0.6 and lines[k].startswith("f\t"):
            f = lines[k].split("\t")
            f[2] = rnd.choice(WAYS) if len(f) > 2 else rnd.choice(WAYS)
            lines[k] = "\t".join(f)
        elif c < 0.7 and lines[k].startswith("f\t"):
            f = lines[k].split("\t")
            f[-1] = rnd.choice(PATHS)
            lines[k] = "\t".join(f)
        elif c < 0.78:
            del lines[k]
            if len(lines) < 2:
                return None
        elif c < 0.86:
            lines.insert(k, lines[rnd.randrange(1, len(lines))])
        elif c < 0.93 and len(lines) > 2:
            j = rnd.randrange(1, len(lines))
            lines[k], lines[j] = lines[j], lines[k]
        else:
            o = rnd.choice(others)
            if o:
                ol = o[:o.find(b"\nend\n")].decode("utf-8", "replace").split("\n")[1:]
                if ol:
                    lines.insert(k, rnd.choice(ol))
    return ("\n".join(lines) + "\n").encode("utf-8") + tail


def mutate_raw(raw, rnd):
    b = bytearray(raw)
    if not b:
        return None
    c = rnd.random()
    if c < 0.4:
        for _ in range(rnd.randint(1, 8)):
            i = rnd.randrange(len(b))
            b[i] ^= 1 << rnd.randrange(8)
    elif c < 0.6:
        for _ in range(rnd.randint(1, 4)):
            b[rnd.randrange(len(b))] = rnd.choice([0, 1, 0x7F, 0x80, 0xFE, 0xFF, rnd.randrange(256)])
    elif c < 0.75:
        b = b[:rnd.randrange(len(b))]
    elif c < 0.88:
        i = rnd.randrange(len(b))
        b[i:i] = bytes(rnd.randrange(256) for _ in range(rnd.randint(1, 64)))
    else:
        i = rnd.randrange(len(b))
        del b[i:i + rnd.randint(1, 64)]
    return bytes(b)


def run_one(path, workdir, limit=60):
    dest = os.path.join(workdir, "out")
    shutil.rmtree(dest, ignore_errors=True)
    env = dict(os.environ, ASAN_OPTIONS="detect_leaks=0:allocator_may_return_null=1:max_allocation_size_mb=2048:abort_on_error=0:handle_abort=1",
               UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1:suppressions=" + str(HERE / "ubsan.supp"))
    t0 = time.time()
    p = subprocess.Popen([SIEVE, "install", path, "--to", dest], stdout=subprocess.DEVNULL, stderr=subprocess.PIPE, env=env)
    try:
        _, err = p.communicate(timeout=limit)
        timed_out = False
    except subprocess.TimeoutExpired:
        p.kill()
        _, err = p.communicate()
        timed_out = True
    return p.returncode, err.decode("utf-8", "replace"), timed_out, time.time() - t0


def worker(arg):
    wid, limits = arg  # (the seeds' limits passed in: under spawn a worker does not see the parent's)
    LIMITS.update(limits)
    rnd = random.Random(wid * 7919 + int(time.time()))
    all_seeds = seeds()
    texts = [t for _, _, t in all_seeds]
    workdir = os.path.join(OUT, "work%d" % wid)
    os.makedirs(workdir, exist_ok=True)
    stats = {"runs": 0, "ok": 0, "refused": 0, "found": 0}
    all_seeds = [x for x in all_seeds if x[0] in LIMITS]
    end_at = time.time() + SECONDS
    while time.time() < end_at:
        name, raw, text = rnd.choice(all_seeds)
        use_text = text is not None and rnd.random() < 0.65
        data = mutate_text(text, texts, rnd) if use_text else mutate_raw(raw, rnd)
        if data is None:
            continue
        path = os.path.join(workdir, "case.manifest" if use_text else "case.sieve")
        open(path, "wb").write(data)
        code, err, timed_out, took = run_one(path, workdir, LIMITS[name])
        stats["runs"] += 1
        bad = timed_out or code < 0 or "AddressSanitizer" in err or "runtime error:" in err or "LeakSanitizer" in err or "terminate called" in err \
            or "std::bad_alloc" in err or code >= 128
        if bad:
            stats["found"] += 1
            tag = "%d-%d" % (wid, stats["found"])
            shutil.copy(path, os.path.join(OUT, "find-%s-%s%s" % (tag, name, ".manifest" if use_text else ".sieve")))
            open(os.path.join(OUT, "find-%s.txt" % tag), "w").write("seed %s\ncode %s timed_out %s took %.1f\n%s" % (name, code, timed_out, took, err[-6000:]))
        elif code == 0:
            stats["ok"] += 1
        else:
            stats["refused"] += 1
        if stats["runs"] % 20 == 0:
            open(os.path.join(OUT, "stats-%d.txt" % wid), "w").write("%s seeds %d\n" % (stats, len(all_seeds)))
    open(os.path.join(OUT, "stats-%d.txt" % wid), "w").write("%s seeds %d done\n" % (stats, len(all_seeds)))
    return stats


LIMITS = {}


def time_seed(item):
    """A seed's own time, clean: three times it (at least 30 s) is its limit, so a slow seed made
    slow by a mutation is not taken for a hang. Seeds over 20 s clean are left out."""
    name, raw, _ = item
    workdir = os.path.join(OUT, "seedtime-%s" % name)
    os.makedirs(workdir, exist_ok=True)
    p = os.path.join(workdir, "seed.sieve")
    open(p, "wb").write(raw)
    code, err, to, took = run_one(p, workdir, 120)
    shutil.rmtree(workdir, ignore_errors=True)
    return name, (None if to or took > 20 else max(30.0, 3 * took)), code, err[-400:]


if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    n = int(os.environ.get("FUZZ_WORKERS", "4"))
    with mp.Pool(n) as pool:
        for name, lim, code, err in pool.map(time_seed, seeds()):
            if lim is not None:
                LIMITS[name] = lim
            if code != 0:
                open(os.path.join(OUT, "seed-fails.txt"), "a").write("%s code %s %s\n" % (name, code, err.replace("\n", " | ")))
    open(os.path.join(OUT, "limits.txt"), "w").write("".join("%s %.0f\n" % kv for kv in sorted(LIMITS.items())))
    with mp.Pool(n) as pool:
        results = pool.map(worker, [(i, dict(LIMITS)) for i in range(n)])
    total = {k: sum(r[k] for r in results) for k in results[0]}
    print(total)
