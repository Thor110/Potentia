"""The trailer's music: melodies of the audio line, chosen as the hallway's music player chooses them
(client/music.cpp): a rank drawn at random below the count of its stack's survivors, and the melody at
that rank read (`sieve read --survivor K`). Each mode's stack and sound are the player's defaults:
WORLD (walking) in C major, long notes, the soft struck voice at 72 a minute with 35% echo; MENUS in
A minor, the plain sine at 66 with 20%. The voices are client/music.cpp's formulas, made here as
samples (the player makes them as it plays)."""
import math
import os
import random
import re
import subprocess
import wave

import numpy as np

RATE = 44100
STACK = ["key-data-v2", "melody-leap-v1", "melody-lengths-v1", "melody-rests-v1", "melody-ending-v1", "melody-range-v1"]
MODES = {
    "world": dict(tonic="C", scale="major", leap=4, shortest="q", longest="w", eighths=0, run=1, total=4,
                  hold="h", high=79, tempo=72, voice="soft", echo=0.35),
    "menus": dict(tonic="A", scale="minor", leap=5, shortest="e", longest="h", eighths=2, run=2, total=6,
                  hold="q", high=76, tempo=66, voice="sine", echo=0.20),
}
BEATS = {"s": 0.25, "e": 0.5, "E": 0.75, "q": 1, "Q": 1.5, "h": 2, "H": 3, "w": 4}
STEPS = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}


def filters_ini(path, m):
    with open(path, "w") as f:
        f.write("[audio]\nmode = compact\nfilters = %s\n" % ", ".join(STACK))
        f.write("[audio.key-data-v2]\ntonic = %s\nscale = %s\n" % (m["tonic"], m["scale"]))
        f.write("[audio.melody-leap-v1]\nleap = %d\nrest_resets = no\n" % m["leap"])
        f.write("[audio.melody-lengths-v1]\nshortest = %s\nlongest = %s\neighths = %d\n" % (m["shortest"], m["longest"], m["eighths"]))
        f.write("[audio.melody-rests-v1]\nrun = %d\ntotal = %d\nleading = no\n" % (m["run"], m["total"]))
        f.write("[audio.melody-ending-v1]\ntonic = %s\nhold = %s\n" % (m["tonic"], m["hold"]))
        f.write("[audio.melody-range-v1]\nlow = 60\nhigh = %d\n" % m["high"])


def draw(sieve, sieve_dir, work, mode, n, seed):
    """n melodies of the mode, as (notation, rank) pairs."""
    ini = os.path.join(work, "music-%s.ini" % mode)
    filters_ini(ini, MODES[mode])
    base = [sieve, "--line", "audio", "--length", "32", "--filters", ini]
    out = subprocess.run([sieve, "filters"] + base[1:], cwd=sieve_dir, capture_output=True, text=True).stdout
    count = int(re.search(r"^survivors\s+(\d+)", out, re.M).group(1))
    rng = random.Random(seed)
    got = []
    for _ in range(n):
        k = rng.randrange(count)
        r = subprocess.run([sieve, "read"] + base[1:] + ["--survivor", str(k), "--mode", "positional"], cwd=sieve_dir,
                           capture_output=True, text=True)
        got.append((r.stdout.strip().splitlines()[-1], k))
    return got, count


def notes_of(notation):
    out = []
    for tok in notation.split():
        m = re.fullmatch(r"R([seEqQhHw])", tok)
        if m:
            out.append((None, BEATS[m.group(1)]))
            continue
        m = re.fullmatch(r"([A-G])([#b]?)(-?\d)([seEqQhHw])", tok)
        if not m:
            raise ValueError("a note not understood: " + tok)
        midi = 12 * (int(m.group(3)) + 1) + STEPS[m.group(1)] + {"#": 1, "b": -1, "": 0}[m.group(2)]
        out.append((midi, BEATS[m.group(4)]))
    return out


def render(notation, mode):
    """A melody as samples, its echo left to ring after it."""
    m = MODES[mode]
    beat = 60.0 / m["tempo"]
    parts = []
    for midi, beats in notes_of(notation):
        n = int(beats * beat * RATE)
        if midi is None:
            parts.append(np.zeros(n))
            continue
        t = np.arange(n) / RATE
        left = (n - np.arange(n)).astype(float)
        f = 440.0 * 2 ** ((midi - 69) / 12)
        ph = 2 * math.pi * f * t
        if m["voice"] == "soft":   # struck, like a chime
            env = np.minimum(1, t / 0.01) * (0.15 + 0.85 * np.exp(-3 * t)) * np.minimum(1, left / (0.06 * RATE))
            x = 0.3 * env * (np.sin(ph) + 0.12 * np.sin(2 * ph))
        else:                      # sine
            env = np.minimum(1, t / 0.03) * np.minimum(1, left / (0.08 * RATE))
            x = 0.3 * env * np.sin(ph)
        parts.append(x)
    dry = np.concatenate(parts + [np.zeros(int(2.5 * RATE))])
    # the echo: a delay line of 0.33 s fed back (y = x + echo * y[-delay])
    d = int(0.33 * RATE)
    y = dry.copy()
    for i in range(d, len(y), d):
        j = min(len(y), i + d)
        y[i:j] += m["echo"] * y[i - d:j - d]
    return y


def soundtrack(path, sections, sieve, sieve_dir, work, gap=2.5, seed=110):
    """sections: (mode, seconds) in order. Each filled with melodies of its mode one after another, a
    gap between them, faded in and out over two seconds at its ends; written as a 16-bit WAV."""
    total = sum(s for _, s in sections)
    mix = np.zeros(int(total * RATE) + RATE)
    pools, counts = {}, {}
    for mode in {m for m, _ in sections}:
        pools[mode], counts[mode] = draw(sieve, sieve_dir, work, mode, 24, seed + len(mode))
    used = {k: 0 for k in pools}
    pos = 0.0
    chosen = []
    for mode, secs in sections:
        start, end = int(pos * RATE), int((pos + secs) * RATE)
        cur = start + int(0.8 * RATE)
        while cur < end - 4 * RATE:
            notation, rank = pools[mode][used[mode] % len(pools[mode])]
            used[mode] += 1
            s = render(notation, mode)
            s = s[: max(0, end - cur)]
            mix[cur:cur + len(s)] += s
            chosen.append((mode, rank, notation))
            cur += len(s) + int(gap * RATE)
        fade = int(2 * RATE)
        seg = mix[start:end]
        if len(seg) > 2 * fade:
            seg[:fade] *= np.linspace(0, 1, fade)
            seg[-fade:] *= np.linspace(1, 0, fade)
        pos += secs
    mix = mix[: int(total * RATE)]
    peak = np.max(np.abs(mix)) or 1
    mix = mix / peak * 0.5
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes((mix * 32767).astype("<i2").tobytes())
    return chosen, counts
