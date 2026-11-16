#!/usr/bin/env python3
"""Each of Sieve's own formats written as a grammar (tools/grammars.py, data/filters/format-*.sfilter;
IDEAS 19.185, list F1) held to the format's reader: every document of a corpus judged twice, by the
reader itself (`sieve catalogue --check-format KIND`, or `hallway --check-format KIND` for the formats the
hallway reads) and by the grammar (`sieve filters --plugin ... --judge --escaped`, on an ascii96 line of
the document's own length), and the verdicts compared. They must agree on every document but the kinds a
format's notes name as beyond a walk (a save's languages out of the order of their ids), which the reader
alone refuses: those must be refused by the reader for that reason, and passed by the grammar.

The corpus, for each format: documents made at random (seeded, so every run has the same), the edges of
its grammar (the shortest, the longest fields, empty and doubled fields), and each document changed by one
character every way (deleted, doubled, replaced by each of a few others), so a grammar that is too loose
or too tight anywhere is found.

    python tools/check_formats.py SIEVE [HALLWAY] [WORK_FOLDER]
"""
import os
import random
import re
import struct
import subprocess
import sys

HEX = "0123456789abcdef"
ID = "abcdefghijklmnopqrstuvwxyz0123456789-._"
ACTIONS = ["idle", "walk", "run", "jump", "take", "read", "wave"]


def sha(r):
    return "".join(r.choice(HEX) for _ in range(64))


def ident(r, n=None):
    return "".join(r.choice(ID) for _ in range(n or r.randint(1, 10)))


# ---- the formats' documents


def actions_doc(r):
    return "sieve-actions-v1\n" + "".join(f"action {a} {sha(r) if r.random() < 0.5 else 'none'}\n" for a in ACTIONS)


def actions_edges(r):
    base = actions_doc(random.Random(1))
    return [
        "sieve-actions-v1\n" + "".join(f"action {a} none\n" for a in ACTIONS),
        "sieve-actions-v1\n" + "".join(f"action {a} {sha(r)}\n" for a in ACTIONS),
        base[:-1],  # no last line feed
        base + "\n",
        base.replace("sieve-actions-v1", "sieve-actions-v2"),
        base.replace("action idle", "action  idle"),
        "sieve-actions-v1\n" + "".join(f"action {a} none\n" for a in reversed(ACTIONS)),
        "sieve-actions-v1\n" + "".join(f"action {a} none\n" for a in ACTIONS[:-1]),
        "sieve-actions-v1\n" + "".join(f"action {a} {sha(r).upper()}\n" for a in ACTIONS),
        "sieve-actions-v1\n" + "".join(f"action {a} {sha(r)[:63]}\n" for a in ACTIONS),
        "sieve-actions-v1\r\n" + "".join(f"action {a} none\r\n" for a in ACTIONS),
    ]


def save_doc(r, langs=None):
    v2 = r.random() < 0.5
    t = ("sieve-save-v2\n" if v2 else "sieve-save-v1\n") + f"character {sha(r)}\n"
    if r.random() < 0.6:
        t += f"animation {sha(r)}\n"
    if v2:
        t += f"actions {sha(r)}\n"
    ls = sorted(set(langs if langs is not None else [ident(r) for _ in range(r.randint(0, 3))]))
    return t + "".join(f"speaks {l}\n" for l in ls)


def save_edges(r):
    c = f"character {sha(r)}\n"
    return [
        "sieve-save-v1\n" + c,
        "sieve-save-v2\n" + c + f"actions {sha(r)}\n",
        "sieve-save-v2\n" + c,  # a v2 without its map
        "sieve-save-v1\n" + c + f"actions {sha(r)}\n",  # a v1 with one
        "sieve-save-v1\n" + c + "speaks " + "a" * 128 + "\n",
        "sieve-save-v1\n" + c + "speaks " + "a" * 129 + "\n",
        "sieve-save-v1\n" + c + "speaks \n",
        "sieve-save-v1\n" + c + "speaks EN\n",
        "sieve-save-v1\n" + c + "speaks en\nspeaks fr\n",
        "sieve-save-v1\n" + c + "speaks fr\nspeaks en\n",  # out of order: the reader alone refuses
        "sieve-save-v1\n" + c + "speaks en\nspeaks en\n",  # twice: the reader alone refuses
        "sieve-save-v1\n" + c + f"animation {sha(r)}\nanimation {sha(r)}\n",
        "sieve-save-v1\n" + f"animation {sha(r)}\n" + c,
        "sieve-save-v1\n",
        "sieve-save-v1\n" + c[:-1],
    ]


def save_beyond_walk(doc, why):
    """A save the reader refuses only because its languages are out of order or twice."""
    return "not in the order of their ids" in why


def f32(x):
    """x as %.9g writes the float nearest it: the numbers of the hallway's own files."""
    return "%.9g" % struct.unpack("f", struct.pack("f", x))[0]


def is_f32_text(s):
    """Whether s is what %.9g writes for some float (the reader's canonical number)."""
    try:
        v = float(s)
    except ValueError:
        return False
    if abs(v) > 3.4028234663852886e38:
        return False
    return f32(v) == s


def num(r):
    k = r.randrange(6)
    if k == 0:
        return f32(0.0)
    if k == 1:
        return f32(r.uniform(-500, 500))
    if k == 2:
        return f32(r.uniform(-1, 1) * 10 ** r.randint(-12, 12))
    if k == 3:
        return str(r.randint(-1000, 1000))
    if k == 4:
        return f32(r.uniform(-3.2, 3.2))
    return f32(r.uniform(0, 2000))


def replay_doc(r):
    t = "sieve-replay-v1\n"
    line = r.choice(["text", "image", "binary", "audio", "models"])
    tile = r.choice(["0", "12", "-3", "123456789012345678901234567890"])
    t += f"start line={line} tile={tile} x={num(r)} y={num(r)} z={num(r)} yaw={num(r)} pitch={num(r)}\n"
    for _ in range(r.randint(0, 5)):
        held = sorted(r.sample(range(512), r.randint(0, 3)))
        toks = []
        for _ in range(r.randint(0, 3)):
            k = r.randrange(5)
            if k == 0:
                toks.append(f"{r.choice(['kd', 'ku'])}:{r.randrange(512)}:{r.randrange(2 ** 32)}:{r.randrange(65536)}:{r.randrange(2)}")
            elif k == 1:
                toks.append("tx:" + "".join(r.choice(HEX) for _ in range(2 * r.randint(1, 4))))
            elif k == 2:
                toks.append(f"mm:{num(r)}:{num(r)}:{num(r)}:{num(r)}")
            elif k == 3:
                toks.append(f"{r.choice(['bd', 'bu'])}:{r.randrange(256)}:{r.randrange(256)}:{num(r)}:{num(r)}")
            else:
                toks.append(f"mw:{num(r)}:{num(r)}")
        t += f"f {r.randint(1, 100000)} {','.join(map(str, held)) if held else '-'}" + "".join(" " + x for x in toks) + "\n"
    return t


def replay_edges(r):
    s = "sieve-replay-v1\nstart line=text tile=0 x=0 y=1.5 z=-2 yaw=0 pitch=0\n"
    return [
        s,
        s + "f 1 -\n",
        s + "f 100000 -\n",
        s + "f 100001 -\n",
        s + "f 0 -\n",
        s + "f 016 -\n",
        s + "f 16 0,511\n",
        s + "f 16 0,512\n",
        s + "f 16 5,3\n",  # out of order: the reader alone refuses
        s + "f 16 3,3\n",
        s + "f 16 - kd:4:97:0:1\n",
        s + "f 16 - kd:4:4294967295:65535:0\n",
        s + "f 16 - kd:4:4294967296:0:0\n",
        s + "f 16 - kd:4:97:65536:0\n",
        s + "f 16 - kd:4:97:0:2\n",
        s + "f 16 - tx:\n",
        s + "f 16 - tx:6\n",
        s + "f 16 - tx:4A\n",
        s + "f 16 - mm:1.5:2:0.25:-1\n",
        s + "f 16 - mm:1.50:2:0.25:-1\n",
        s + "f 16 - mm:1e+10:1e-10:1.5e+38:-1e-45\n",
        s + "f 16 - mm:1.00000001:0:0:0\n",  # %.9g's shape, no float's: the reader alone refuses
        s + "f 16 - mm:inf:0:0:0\n",
        s + "f 16 - mm:nan:0:0:0\n",
        s + "f 16 - bd:1:2:3:4 bu:1:2:3:4 mw:0:-1\n",
        s + "f 16 - bd:256:2:3:4\n",
        s + "f 16  -\n",
        s + "f 16\n",
        s.replace("x=0", "x=0.0"),
        s.replace("x=0", "x=-0"),
        s.replace("line=text", "line="),
        s.replace("tile=0", "tile=-12"),
        s.replace("x=0", "x=1e+05"),
        s.replace("x=0", "x=100000"),
        s.replace("x=0", "x=123456789"),
        s.replace("x=0", "x=1234567890"),
        s.replace("x=0", "x=1.23456789e+09"),
        s.replace("x=0", "x=0.0001"),
        s.replace("x=0", "x=1e-05"),
        s.replace("x=0", "x=0.00001"),
        s[:-1],
    ]


def replay_floats(doc):
    """Every field of a recording that holds a float: the start's five, a motion's four, a button's
    place, a wheel's two."""
    out = []
    lines = doc.split("\n")
    if len(lines) > 1:
        for f in lines[1].split(" ")[3:8]:
            out.append(f.split("=", 1)[-1])
    for line in lines[2:]:
        for tok in line.split(" ")[3:]:
            p = tok.split(":")
            if p[0] == "mm":
                out += p[1:]
            elif p[0] in ("bd", "bu"):
                out += p[3:]
            elif p[0] == "mw":
                out += p[1:]
    return out


def replay_beyond_walk(doc, why):
    """A recording the reader refuses only because its keys held are out of order, or a number is not a float's own."""
    for line in doc.split("\n"):
        f = line.split(" ")
        if len(f) >= 3 and f[0] == "f" and f[2] != "-":
            ks = [int(k) for k in f[2].split(",") if k.isdigit()]
            if any(b <= a for a, b in zip(ks, ks[1:])):
                return True
    return any(not is_f32_text(n) for n in replay_floats(doc))


WORLD_KINDS = ["pages", "image", "audio", "video", "books", "models", "binary", "tracks", "movies", "worlds", "ai", "bookcase"]


def world_doc(r):
    t = f"sieve-personal-world-v1\nseed {r.choice([0, 1, 12345, r.randrange(2 ** 64)])}\n"
    for _ in range(r.randint(0, 4)):
        t += f"place {r.choice(WORLD_KINDS)} {num(r)} {num(r)} {f32(r.uniform(-3.2, 3.2))}\n"
    return t


def world_edges(r):
    s = "sieve-personal-world-v1\nseed 0\n"
    return [
        s,
        s + "place pages 0 -52 0\n",
        "sieve-personal-world-v1\nseed 18446744073709551615\n",
        "sieve-personal-world-v1\nseed 18446744073709551616\n",
        "sieve-personal-world-v1\nseed 007\n",
        "sieve-personal-world-v1\n",
        s + "seed 1\n",
        s + "place pages 0 -52\n",
        s + "place pages 0 -52 0 0\n",
        s + "place nothing 0 -52 0\n",
        s + "place fav:" + "a" * 64 + " 0 0 0\n",
        s + "place pages 1e5 0 0\n",
        s + "place pages 100000 0 0\n",
        s + "place pages  0 0 0\n",
        s + "place bookcase 1.5 -2.25 3.14159274\n",
        s + "place pages 1.00000001 0 0\n",  # %.9g's shape, no float's: the round trip alone refuses
        s + "\n",
        s + "# a note\n",
        s[:-1],
    ]


def world_beyond_walk(doc, why):
    """A world the round trip refuses only because a number is not a float's own."""
    for line in doc.split("\n"):
        f = line.split(" ")
        if f[0] == "place" and len(f) == 5 and any(not is_f32_text(n) for n in f[2:]):
            return True
    return False


# sieve-personal-world-v2: everything its reader reads, in the order its writer writes it.

SHARP = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]
CODES = "seEqQhHw"
MODES = ["none", "lydian", "ionian", "mixolydian", "dorian", "aeolian", "phrygian", "locrian"]


def note_set(r):
    """A note set's id, its base and its voices (sieve/audio NoteSet)."""
    if r.random() < 0.3:
        return "notes104", 104, 1
    lo = r.randint(36, 85)
    hi = r.randint(lo + 11, 96)
    durs = "".join(c for c in CODES if r.random() < 0.5) or "q"
    v = r.randint(1, 4)
    name = lambda m: SHARP[m % 12] + str(m // 12 - 1)
    return f"notes2/{name(lo)}-{name(hi)}/{durs}/V{v}", (hi - lo + 2) * len(durs), v


def music_word(r):
    k = r.randrange(16)
    if k == 0:
        return "on=" + r.choice(["on", "off"])
    if k == 1:
        return "source=" + r.choice(["audio", "tracks"])
    if k == 2:
        key, lo, hi = r.choice([("volume", 0, 100), ("length", 1, 4096), ("units", 1, 64), ("tempo", 20, 300), ("echo", 0, 80), ("gap", 0, 600)])
        return f"{key}={r.choice([lo, hi, r.randint(lo, hi)])}"
    if k == 3:
        return "voice=" + r.choice(["soft", "sine", "triangle", "square"])
    if k == 4:
        if r.random() < 0.3:
            return "notes=notes104"
        lo = r.randint(36, 85)
        hi = r.randint(lo + 11, 96)
        spell = lambda m: r.choice([SHARP[m % 12] + str(m // 12 - 1), (SHARP[m % 12] + str(m // 12 - 1)).lower()])
        return f"notes=notes2/{spell(lo)}/{spell(hi)}/{''.join(c for c in CODES if r.random() < 0.5) or 'e'}/{r.randint(1, 4)}" + r.choice(["", "", "/"])
    if k == 5:
        return "character=" + r.choice(MODES)
    if k == 6:
        return "filters=" + ",".join(r.choice(["key-data-v2", "rest-v1", "x"]) for _ in range(r.randint(0, 3)))
    if k == 7:
        return f"param={r.choice(['key-data-v2', 'f'])}:{r.choice(['tonic', 'k'])}:{r.choice(['C', '1.5', 'a%20b', '%25', 'x:y', ''])}"
    if k == 8:
        sid, base, voices = note_set(r)
        units = r.choice([0, 1, 2, 3])
        n = max(1, units) * voices * r.randint(1, 3)
        return f"melody={sid}:{units}:" + ",".join(str(r.randrange(base)) for _ in range(n))
    if k == 9:
        return r.choice(["later=1", "x=", "tempo2=5", "music=on"])
    return music_word(r)


def music_line(r):
    words = [music_word(r) for _ in range(r.randint(0, 5))]
    return "music" + "".join(" " + w for w in words)


def world2_doc(r):
    t = f"sieve-personal-world-v2\nseed {r.choice([0, 1, 12345, r.randrange(2 ** 64)])}\n"
    if r.random() < 0.3:
        t += f"day {r.choice([1, 60, 1440, 2 ** 64 - 1, r.randrange(1, 2 ** 64)])}\n"
    corners = sorted({(r.randrange(-32, 33) * 10, r.randrange(-32, 33) * 10) for _ in range(r.choice([0, 0, 1, 3]))})
    for x, z in corners:
        t += f"ground {x} {z} {f32(r.choice([r.uniform(-500, 500), r.uniform(-1, 1), 500.0, -500.0, 0.5]))}\n"
    models = ["".join(r.choice(HEX) for _ in range(64)) for _ in range(r.choice([0, 0, 1, 2]))]
    for m in models:
        t += f"model {m}\n"
    if r.random() < 0.35:
        t += music_line(r) + "\n"
    for _ in range(r.choice([0, 0, 0, 1, 2])):
        t += r.choice(["author local", "layout pages 0 0 0 0", "face-cache " + "a" * 64, "characters", "characters later", "later  two  spaces "]) + "\n"
    chars_placed = r.random() < 0.2
    if chars_placed:
        t += "characters placed\n"
    for _ in range(r.randint(0, 3)):
        kinds = WORLD_KINDS + ["fav:" + "".join(r.choice(HEX) for _ in range(64))] + ["model:" + m for m in models]
        if chars_placed:
            kinds += ["char:you", "char:" + "".join(r.choice(HEX) for _ in range(64))]
        tilt = [f32(r.uniform(-3.2, 3.2)) if r.random() < 0.3 else "0" for _ in range(2)]
        t += f"place {r.choice(kinds)} {num(r)} {num(r)} {num(r)} {f32(r.uniform(-3.2, 3.2))} {tilt[0]} {tilt[1]} {r.choice(['fixed', 'physics'])}\n"
    return t


def world2_edges(r):
    s = "sieve-personal-world-v2\nseed 0\n"
    p = "place pages 0 0 -52 0 0 0 physics\n"
    sha = "0123456789abcdef" * 4
    return [
        s,  # nothing after the seed: written as v1
        s + p,
        s + "day 1440\n",
        s + "day 0\n",
        s + "day 18446744073709551615\n",
        s + "day 18446744073709551616\n",
        s + "day 060\n",
        s + "ground 0 0 1\n",
        s + "ground 0 0 0\n",
        s + "ground 0 0 -0\n",
        s + "ground 5 0 1\n",
        s + "ground 330 0 1\n",
        s + "ground -320 320 500\n",
        s + "ground 0 0 500.000031\n",
        s + "ground 0 0 1 2\n",
        s + "ground 10 0 1\nground 0 0 1\n",  # out of order: a walk does not weigh it
        s + "ground 0 0 1\nground 0 0 2\n",  # twice
        s + "model " + sha + "\n",
        s + "model " + sha + "\nmodel " + sha + "\n",  # twice
        s + "model " + sha.upper() + "\n",
        s + "place model:" + sha + " 0 0 0 0 0 0 fixed\n",  # placed, never named
        s + "model " + sha + "\nplace model:" + sha + " 0 0 0 0 0 0 fixed\n",
        s + "music\n",
        s + "music  on=off \n",
        s + " music on=on\n",
        s + "music on=maybe\n",
        s + "music volume=101\n",
        s + "music volume=07\n",
        s + "music notes=notes2/C3/C6/eq/1/\n",
        s + "music notes=notes2/C3/C6/eq/1//\n",
        s + "music notes=notes2/Db3/c#6/seEqQhHw/4\n",
        s + "music notes=notes2/C3/B3/eq/1\n",  # an octave exactly (12 notes)
        s + "music notes=notes2/C3/A#3/eq/1\n",  # one short
        s + "music notes=notes2/C2/C7/qe/1\n",  # durations out of order
        s + "music notes=notes2/B1/C7/e/1\n",  # below C2
        s + "music param=f:k:a%2\n",
        s + "music param=f::v\n",
        s + "music param=f:k:%41:b\n",
        s + "music melody=notes104:0:5,103,\n",
        s + "music melody=notes104:0:104\n",  # past the base: beyond a walk
        s + "music melody=notes2/C3-C6/eq/V2:1:1\n",  # not a whole number of voices
        s + "music melody=notes2/C3-C6/eq/V2:1:1,73\n",
        s + "music melody=notes2/c3-C6/eq/V2:1:1,2\n",  # a set's id has one spelling
        s + "music later=x\n",
        s + "music later\n",
        s + "music =x\n",
        s + "music on=on\nmusic on=off\n",
        s + "author local\n",
        s + "characters\n",
        s + "characters placed\n",
        s + " characters placed\n",
        s + "characters  placed\n",
        s + "characters placed now\n",
        s + "characters placedx\n",
        s + "characters placed\nplace char:you 0 0 -52 0 0 0 fixed\n",
        s + "place char:you 0 0 -52 0 0 0 fixed\n",  # a character placed without the line
        s + "place fav:" + "z" * 64 + " 0 0 0 0 0 0 fixed\n",
        s + "place fav:" + "z" * 63 + " 0 0 0 0 0 0 fixed\n",
        s + "place pages 0 0 0 0 0.5 0 fixed\n",
        s + "place pages 0 0 0 0 0 0 fixed\n",  # plain: v2 only if it stands off the ground
        s + "place pages 0 0 0 0 0 0 still\n",
        s + "place pages 0 0 0 0 0 0 fixed x\n",
        s + "seed 1\n",
        s + "\n",
        s + "   \n",
        s + "author local\nday 5\n",  # out of the writer's order
        s + "music\nday 5\n",
    ]


def world2_beyond_walk(doc, why):
    """A world the round trip refuses for what a walk does not weigh: a number not a float's own, the
    ground's corners out of order or given twice, a model named twice or placed unnamed, a melody's
    digits past its set's base or not a whole number of units and voices, or a world with nothing of
    v2's but where its places stand (which needs the seed's ground)."""
    lines = doc.split("\n")[2:-1]
    corners, models, placed_models, structural = [], [], [], False
    for line in lines:
        f = line.split(" ")
        if f[0] == "place" and len(f) == 9:
            if any(not is_f32_text(n) for n in f[2:8]):
                return True
            if f[1].startswith("model:"):
                placed_models.append(f[1][6:])
            if f[1] not in WORLD_KINDS or f[8] == "physics" or f[6] not in ("0", "-0") or f[7] not in ("0", "-0"):
                structural = True
            continue
        structural = True
        if f[0] == "ground" and len(f) == 4:
            if not is_f32_text(f[3]):
                return True
            corners.append((int(f[1]), int(f[2])))
        elif f[0] == "model":
            models.append(f[1])
        elif line.lstrip(" ").startswith("music"):
            for w in line.split():
                if w.startswith("melody="):
                    sid, units, digits = (w[7:].split(":", 2) + ["", ""])[:3]
                    if sid == "notes104":
                        base, voices = 104, 1
                    elif all(n[:-1] in SHARP for n in sid.split("/")[1].split("-")):
                        _, rng, durs, v = sid.split("/")
                        lo, hi = rng.split("-")
                        midi = lambda n: (int(n[-1]) + 1) * 12 + SHARP.index(n[:-1])
                        base, voices = (midi(hi) - midi(lo) + 2) * len(durs), int(v[1])
                    else:
                        continue  # (a set's id spelt otherwise: the grammar refuses it too)
                    ds = [int(d) for d in digits.rstrip(",").split(",")]
                    if any(d >= base for d in ds) or len(ds) % (max(1, int(units)) * voices) or len(ds) > 1 << 20:
                        return True
    if corners != sorted(set(corners)) or len(models) != len(set(models)) or any(m not in models for m in placed_models):
        return True
    return not structural


# sieve-fractal-v1: the fractal dimension's rulesets.

FRACTAL_CHARTED = ["sieve-fractal-v1\nsubstitution\naxiom 0\nrule 0 01\nrule 1 0\n", "sieve-fractal-v1\nlsystem 90\naxiom FX\nrule X X+YF+\nrule Y -FX-Y\n",
                   "sieve-fractal-v1\nlsystem 25\naxiom X\nrule F FF\nrule X F+[[X]-X]-F[-FX]+X\n"]


def fractal_doc(r):
    if r.random() < 0.5:
        sym = "0123456789abcdefghijklmnopqrstuvwxyz"
        pool = sym[:r.choice([2, 3, 10, 36])]
        used = sorted(r.sample(pool, r.randint(1, min(3, len(pool)))))
        word = lambda: "".join(r.choice(sym[:12]) for _ in range(r.randint(1, r.choice([3, 8, 64]))))
        t = "sieve-fractal-v1\nsubstitution\naxiom " + word() + "\n"
    else:
        letters = "FGXYABfz"
        used = sorted(r.sample(letters, r.randint(1, 3)))
        word = lambda: "".join(r.choice(letters + "+-[]|") for _ in range(r.randint(1, r.choice([4, 12, 64]))))
        t = f"sieve-fractal-v1\nlsystem {r.choice([1, 25, 60, 90, 120, 359, r.randint(1, 359)])}\naxiom " + word() + "\n"
    for s in used:
        t += f"rule {s} {word()}\n"
    return t


def fractal_edges(r):
    h = "sieve-fractal-v1\n"
    return FRACTAL_CHARTED + [
        h + "substitution\naxiom 0\n",  # no rule
        h + "substitution\naxiom 0\nrule 0 01\nrule 0 1\n",  # a symbol twice
        h + "substitution\naxiom 0\nrule 1 0\nrule 0 01\n",  # out of order: a walk does not weigh it
        h + "substitution\naxiom 0\nrule 0 " + "0" * 64 + "\n",
        h + "substitution\naxiom 0\nrule 0 " + "0" * 65 + "\n",
        h + "substitution\naxiom A\nrule A 0\n",  # a capital in a substitution
        h + "lsystem 0\naxiom F\nrule F FF\n",
        h + "lsystem 360\naxiom F\nrule F FF\n",
        h + "lsystem 090\naxiom F\nrule F FF\n",
        h + "lsystem 90\naxiom F\nrule + FF\n",  # only letters have rules
        h + "lsystem 90\naxiom F\nrule F F F\n",
        h + "lsystem 90\naxiom \nrule F F\n",
        h + "lsystem 90\naxiom F\nrule F FF",  # its last line not ended
        "sieve-fractal-v2\nsubstitution\naxiom 0\nrule 0 01\n",
    ]


def fractal2_doc(r):
    """v1's rulesets and v2's, half each: an angle with a tenth, a scale, or both."""
    if r.random() < 0.5:
        return fractal_doc(r)
    letters = "FGXYABfz"
    scale = r.random() < 0.6
    sym = letters + "+-[]|" + ("\"'" if scale else "")
    used = sorted(r.sample(letters, r.randint(1, 3)))
    word = lambda: "".join(r.choice(sym) for _ in range(r.randint(1, r.choice([4, 12, 64]))))
    tenth = f"{r.choice([0, 22, 25, 137, 359, r.randint(0, 359)])}.{r.randint(1, 9)}"
    angle = r.choice([tenth, str(r.randint(1, 359))]) if scale else tenth
    t = "sieve-fractal-v2\nlsystem " + angle
    if scale:
        t += " scale 0." + r.choice([str(r.randint(1, 9)), f"{r.randint(0, 9)}{r.randint(1, 9)}", f"{r.randint(0, 99):02d}{r.randint(1, 9)}"])
    t += "\naxiom " + word() + "\n"
    for s in used:
        t += f"rule {s} {word()}\n"
    return t


def fractal2_edges(r):
    h = "sieve-fractal-v2\nlsystem "
    return fractal_edges(r) + [
        "sieve-fractal-v2\nsubstitution\naxiom 0\nrule 0 01\n",  # substitutions are v1's
        h + "90\naxiom F\nrule F FF\n",  # nothing v1 cannot write
        h + "22.5\naxiom F\nrule F F[+F]F\n",
        h + "22.0\naxiom F\nrule F FF\n",  # a tenth of 0
        h + "0.5\naxiom F\nrule F FF\n",
        h + "00.5\naxiom F\nrule F FF\n",
        h + "360.5\naxiom F\nrule F FF\n",
        h + "359.9\naxiom F\nrule F FF\n",
        h + "22.55\naxiom F\nrule F FF\n",
        h + "90 scale 0.5\naxiom F\nrule F F\"F\n",
        h + "90 scale 0.707\naxiom F\nrule F F'F\n",
        h + "90 scale 0.70\naxiom F\nrule F FF\n",  # a 0 at its end
        h + "90 scale 0.0\naxiom F\nrule F FF\n",
        h + "90 scale 1.5\naxiom F\nrule F FF\n",
        h + "90 scale 0.7071\naxiom F\nrule F FF\n",
        h + "90 scale .5\naxiom F\nrule F FF\n",
        h + "22.5\naxiom F\nrule F F\"F\n",  # " without a scale
        h + "22.5 scale 0.9\naxiom F\nrule \" FF\n",  # only letters have rules
        "sieve-fractal-v1\nlsystem 90\naxiom F\nrule F F\"F\n",
    ]


def fractal3_doc(r):
    """v1's, v2's and v3's: a v3 with some of its optional lines, contexts and several rules a symbol."""
    if r.random() < 0.4:
        return fractal2_doc(r)
    syms = "FGXYABfz+-[]|!\"'&^\\/0123"
    word = lambda: "".join(r.choice(syms) for _ in range(r.randint(1, r.choice([4, 12, 255]))))
    frac = lambda: "." + "".join(r.choice("0123456789") for _ in range(r.randint(0, 5))) + r.choice("123456789")
    t = "sieve-fractal-v3\nlsystem " + r.choice([f"360/{r.randint(2, 9999)}", f"{r.randint(1, 359)}", f"{r.randint(0, 359)}{frac()}"]) + "\n"
    if r.random() < 0.3:
        t += "scale 0" + frac() + "\n"
    if r.random() < 0.3:
        t += f"seed {r.randint(1, 4294967295)}\n"
    if r.random() < 0.3:
        t += "ignore " + "".join(sorted(set(r.sample(syms, r.randint(1, 4))))) + "\n"
    for d in "0123456789":
        if r.random() < 0.15:
            t += f"define {d} turn {r.choice(['', '-'])}{r.randint(1, 359)}\n" if r.random() < 0.5 else f"define {d} scale {r.choice(['0' + frac(), str(r.randint(2, 999))])}\n"
    t += "axiom " + word() + "\n"
    for s in sorted(r.choices("FGXYAB", k=r.randint(1, 4))):
        head = (r.choice("ab") + "<" if r.random() < 0.2 else "") + s + (">" + r.choice("ab") if r.random() < 0.2 else "")
        t += f"rule {head} {word()}\n"
    return t


def fractal3_edges(r):
    h = "sieve-fractal-v3\nlsystem "
    return fractal2_edges(r) + [
        h + "90\naxiom F\nrule F FF\n",  # what v1 says, as v3: another text
        h + "360/34\naxiom F\nrule F F!+F\n",
        h + "360/1\naxiom F\nrule F F\n",
        h + "360/10000\naxiom F\nrule F F\n",
        h + "360\naxiom F\nrule F F\n",
        h + "0\naxiom F\nrule F F\n",
        h + "0.000001\naxiom F\nrule F F\n",
        h + "0.0000001\naxiom F\nrule F F\n",
        h + "22.50\naxiom F\nrule F F\n",
        h + "90\nscale 0.618034\naxiom F\nrule F F\"F\n",
        h + "90\nscale 1.5\naxiom F\nrule F F\n",
        h + "90\nseed 0\naxiom F\nrule F F\n",
        h + "90\nseed 4294967295\naxiom F\nrule F F\nrule F G\n",
        h + "90\nseed 4294967296\naxiom F\nrule F F\n",
        h + "90\nignore +-F\naxiom F\nrule a<F>b F\n",
        h + "90\nignore F-+\naxiom F\nrule F F\n",  # out of order: refused
        h + "90\nignore ++\naxiom F\nrule F F\n",
        h + "90\ndefine 1 turn 42\ndefine 2 turn -84\ndefine 3 scale 4.783\naxiom F\nrule F F12F3\n",
        h + "90\ndefine 2 turn 42\ndefine 1 turn 84\naxiom F\nrule F F\n",  # digits out of order
        h + "90\ndefine 1 turn 0\naxiom F\nrule F F\n",
        h + "90\ndefine 1 turn 360\naxiom F\nrule F F\n",
        h + "90\ndefine 1 scale 1\naxiom F\nrule F F\n",
        h + "90\ndefine 1 scale 1.5\naxiom F\nrule F F\n",
        h + "90\ndefine 1 scale 1000\naxiom F\nrule F F\n",
        h + "90\naxiom F\nrule F " + "F" * 255 + "\n",
        h + "90\naxiom F\nrule F " + "F" * 256 + "\n",
        h + "90\naxiom F\nrule F&F^F\\F/F\n",
        h + "90\naxiom F\nrule + F\n",
        h + "90\naxiom F\nrule a<+ F\n",
        h + "90\naxiom F\nrule G F\nrule F F\n",  # out of order: beyond a walk
        h + "90\nseed 3\nscale 0.5\naxiom F\nrule F F\n",  # lines out of order
    ]


def fractal4_doc(r):
    """v1's to v3's, and v4's: an IFS of a few maps, or an escape or converge formula grown at random."""
    if r.random() < 0.4:
        return fractal3_doc(r)
    frac = lambda: "." + "".join(r.choice("0123456789") for _ in range(r.randint(0, 5))) + r.choice("123456789")
    num = lambda zero: (r.choice(["0"]) if zero and r.random() < 0.3 else r.choice(["", "-"]) + r.choice([str(r.randint(1, 999)), "0" + frac(), str(r.randint(1, 999)) + frac()]))
    if r.random() < 0.4:
        return "sieve-fractal-v4\nifs\n" + "".join("map " + " ".join(num(True) for _ in range(6)) + "\n" for _ in range(r.randint(1, 4)))

    def term(depth):
        k = r.random()
        if depth > 3 or k < 0.4:
            return [r.choice(["z", "c", "i", num(False)])]
        if k < 0.6:
            return [r.choice(["abs", "conj", "cos", "exp", "log", "sin"])] + term(depth + 1)
        return [r.choice("+-*/^")] + term(depth + 1) + term(depth + 1)

    t = "sieve-fractal-v4\n" + r.choice(["escape ", "converge "]) + " ".join(term(0)) + "\n"
    if r.random() < 0.4:
        t += f"julia {num(True)} {num(True)}\n"
    if r.random() < 0.4:
        t += f"view {num(True)} {num(True)} {num(False).lstrip('-')}\n"
    return t


def fractal4_edges(r):
    h = "sieve-fractal-v4\n"
    return fractal3_edges(r) + [
        h + "escape z\n",
        h + "escape + ^ z 2 c\nview -0.5 0 3\n",
        h + "converge - z / - ^ z 3 1 * 3 ^ z 2\njulia 0 0\n",
        h + "escape + ^ abs z 2 c\n",
        h + "escape + + + + + + + z z z z z z z z\n",  # 8 awaited after the seventh +: the most
        h + "escape + + + + + + + + z z z z z z z z z\n",  # 9: refused
        h + "escape + z\n",  # awaits one more
        h + "escape z z\n",  # whole before its last
        h + "escape  z\n",
        h + "escape 0\n",  # a formula's number is not 0
        h + "escape -0.5\n",
        h + "escape -0\n",
        h + "escape 1000\n",
        h + "escape 0.1234567\n",
        h + "escape 1.50\n",
        h + "escape tan z\n",
        h + "escape z\nview 0 0 0\n",
        h + "escape z\nview 0 0 -1\n",
        h + "escape z\nview -0 0 1\n",
        h + "escape z\nview 0 0 1\njulia 0 0\n",  # out of order
        h + "escape z\njulia 0 0\njulia 0 0\n",
        h + "ifs\nmap 0 0 0 0 0 0\n",
        h + "ifs\nmap 0.5 0 0 0.5 0 0\nmap 0.5 0 0 0.5 0.5 0\n",
        h + "ifs\n",
        h + "ifs\nmap 0 0 0 0 0\n",
        h + "ifs\nmap 0 0 0 0 0 0 0\n",
        h + "ifs\nmap 00 0 0 0 0 0\n",
        h + "ifs\nmap 999.999999 -999.999999 0 0 0 0\n",
        h + "ifs\nmap 1000 0 0 0 0 0\n",
        h + "ifs\nescape z\n",
        h + "lsystem 90\naxiom F\nrule F FF\n",
    ]


def fractal_beyond_walk(doc, why):
    """A ruleset the reader refuses only because its rules are out of order or one is given twice."""
    return "in the order of their symbols" in why


FORMATS = [
    # name, reader program ("sieve" or "hallway"), kind, plugin, documents, edges, beyond a walk
    ("actions", "sieve", "actions", "format-actions-v1.sfilter", actions_doc, actions_edges, None),
    ("save", "sieve", "save", "format-save-v1.sfilter", save_doc, save_edges, save_beyond_walk),
    ("replay", "hallway", "replay", "format-replay-v1.sfilter", replay_doc, replay_edges, replay_beyond_walk),
    ("world", "hallway", "world", "format-world-v1.sfilter", world_doc, world_edges, world_beyond_walk),
    ("world-v2", "hallway", "world", "format-world-v2.sfilter", world2_doc, world2_edges, world2_beyond_walk),
    ("fractal", "sieve", "fractal", "format-fractal-v1.sfilter", fractal_doc, fractal_edges, fractal_beyond_walk),
    ("fractal-v2", "sieve", "fractal", "format-fractal-v2.sfilter", fractal2_doc, fractal2_edges, fractal_beyond_walk),
    ("fractal-v3", "sieve", "fractal", "format-fractal-v3.sfilter", fractal3_doc, fractal3_edges, fractal_beyond_walk),
    ("fractal-v4", "sieve", "fractal", "format-fractal-v4.sfilter", fractal4_doc, fractal4_edges, fractal_beyond_walk),
]

OTHERS = ["", "a", "0", " ", "\n", "-", "Z", ".", "\r"]


def mutations(doc, r, n):
    """n changes of one character each, every kind: deleted, doubled, replaced."""
    out = []
    for _ in range(n):
        i = r.randrange(len(doc))
        k = r.randrange(3)
        if k == 0:
            out.append(doc[:i] + doc[i + 1:])
        elif k == 1:
            out.append(doc[:i] + doc[i] + doc[i:])
        else:
            out.append(doc[:i] + r.choice(OTHERS) + doc[i + 1:])
    return out


def escape(doc):
    return doc.replace("\\", "\\\\").replace("\n", "\\n")


def corpus(make, edges, seed):
    r = random.Random(seed)
    docs = [make(r) for _ in range(150)]
    docs += edges(r)
    for d in list(docs[:60]):
        docs += mutations(d, r, 6)
    # (the grammar reads ascii96: a document with any other character is the reader's alone)
    return [d for d in docs if d and all(c == "\n" or " " <= c <= "~" for c in d)]


def run(cmd):
    p = subprocess.run(cmd, capture_output=True)
    return p.stdout.decode("utf-8", "replace").splitlines()


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    sieve = sys.argv[1]
    hallway = sys.argv[2] if len(sys.argv) > 2 and not os.path.isdir(sys.argv[2]) else None
    work = sys.argv[-1] if len(sys.argv) > 2 and os.path.isdir(sys.argv[-1]) else "."
    here = os.path.dirname(os.path.abspath(__file__))
    bad = 0
    for seed, (name, prog, kind, plugin, make, edges, beyond) in enumerate(FORMATS, 1):
        if prog == "hallway" and not hallway:
            print(f"{name}: skipped (no hallway given)")
            continue
        docs = corpus(make, edges, 185 + seed)
        path = os.path.join(work, f"format-{name}.txt")
        with open(path, "w", encoding="utf-8", newline="\n") as f:
            for d in docs:
                f.write(escape(d) + "\n")
        reader_prog = sieve if prog == "sieve" else hallway
        reader_cmd = [reader_prog, "catalogue", "--check-format", kind, "--in", path] if prog == "sieve" else [reader_prog, "--check-format", kind, "--in", path]
        reader = [l for l in run(reader_cmd) if l.startswith("read ")]
        judge = [l for l in run([sieve, "filters", "--plugin", os.path.join(here, "..", "data", "filters", plugin), "--line", "text", "--alphabet", "ascii96", "--length", "8",
                                 "--judge", path, "--escaped"]) if l.startswith("judge ")]
        if not len(reader) == len(docs) == len(judge):
            print(f"{name}: {len(docs)} documents, {len(reader)} read, {len(judge)} judged")
            bad += 1
            continue
        agree = beyond_n = passes = 0
        for d, rl, jl in zip(docs, reader, judge):
            r_ok, j_ok = rl.split()[1] == "pass", jl.split()[1] == "pass"
            why = rl.split("(", 1)[1].rstrip(")") if "(" in rl else ""
            passes += r_ok
            if r_ok == j_ok:
                agree += 1
            elif beyond and not r_ok and j_ok and beyond(d, why):
                beyond_n += 1
            else:
                bad += 1
                print(f"{name}: reader {'reads' if r_ok else 'refuses'} and grammar {'passes' if j_ok else 'fails'}: {escape(d)!r} {why}")
        print(f"{name}: {len(docs)} documents ({passes} read by the reader), {agree} verdicts the same" + (f", {beyond_n} beyond a walk (the reader alone refuses)" if beyond else ""))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
