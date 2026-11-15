#!/usr/bin/env python3
"""Sieve's own formats written as filter plugins (IDEAS 19.185, list F1): each format's grammar given as a
regular expression over the ascii96 alphabet (a line feed and printable ASCII), compiled to the smallest
automaton that walks it, and written out as a `sieve-filter-v1` table, so the valid documents of the
format are exactly the plugin's survivors on an ascii96 line of their length.

The grammars are written here, once, from the readers (the code that reads each format is the
reference: tools/check_formats.py holds every plugin to its reader on a corpus). The .sfilter files are
what this writes, kept in data/filters so they are read and reviewed as the other plugins are; CI writes
them again and checks they are the same, so a grammar changed here and not written out is caught.

    python tools/grammars.py            write every plugin into data/filters
    python tools/grammars.py --check    write nothing; exit 1 if any written file differs

The compiler: a regular expression (built from the helpers below, not parsed from a string) becomes a
Thompson automaton, then the subset construction gives a deterministic one, then Hopcroft's algorithm
the smallest, with the state no accepting state can be reached from taken away (a missing transition is
dead in a plugin). Symbols that every state treats alike are one class, so a table names classes, not
96 symbols. States are numbered in the order a walk first meets them, from the start, so the table reads
top to bottom as the document does.
"""
import os
import sys

# ---- the alphabet: ascii96, a line feed (digit 0) and U+0020-U+007E (digits 1 to 95)

ALPHABET = ["\n"] + [chr(c) for c in range(32, 127)]
DIGIT = {c: i for i, c in enumerate(ALPHABET)}

# ---- regular expressions, as trees


class Re:
    def __add__(self, other):
        return seq(self, other)

    def __or__(self, other):
        return alt(self, other)


class Sym(Re):
    def __init__(self, chars):
        self.chars = frozenset(chars)


class Seq(Re):
    def __init__(self, parts):
        self.parts = parts


class Alt(Re):
    def __init__(self, parts):
        self.parts = parts


class Star(Re):
    def __init__(self, inner):
        self.inner = inner


class Empty(Re):
    pass


def lit(s):
    return Seq([Sym([c]) for c in s]) if len(s) != 1 else Sym([s])


def chars(s):
    return Sym(s)


def seq(*parts):
    return Seq([p if isinstance(p, Re) else lit(p) for p in parts])


def alt(*parts):
    return Alt([p if isinstance(p, Re) else lit(p) for p in parts])


def star(r):
    return Star(r if isinstance(r, Re) else lit(r))


def opt(r):
    return alt(Empty(), r)


def rep(r, lo, hi):
    """r between lo and hi times (hi None: no end)."""
    r = r if isinstance(r, Re) else lit(r)
    parts = [r] * lo
    if hi is None:
        parts.append(star(r))
    else:
        tail = Empty()
        for _ in range(hi - lo):
            tail = opt(seq(r, tail))
        parts.append(tail)
    return Seq(parts)


LOWER_HEX = "0123456789abcdef"
DIGITS = "0123456789"
NONZERO = "123456789"


def sha256():
    """64 hexadecimal digits in small letters (sieve/hex.hpp is_sha256_hex)."""
    return rep(chars(LOWER_HEX), 64, 64)


def plain_id(most):
    """1 to `most` of a-z, 0-9, '-', '.', '_' (sieve/text.hpp is_plain_id)."""
    return rep(chars("abcdefghijklmnopqrstuvwxyz0123456789-._"), 1, most)


def decimal_at_most(limit):
    """A whole number from 0 to `limit` in decimal, with no sign and no leading zero ("0" itself)."""
    s = str(limit)
    options = [lit("0")] if limit >= 0 else []
    # shorter than the limit: any number of that many digits
    for n in range(1, len(s)):
        options.append(seq(chars(NONZERO), rep(chars(DIGITS), n - 1, n - 1)))
    # as long as the limit: at most it, digit by digit
    for i in range(len(s)):
        d = int(s[i])
        lo = 1 if i == 0 else 0
        if d > lo:
            options.append(seq(lit(s[:i]), chars("".join(str(k) for k in range(lo, d))), rep(chars(DIGITS), len(s) - i - 1, len(s) - i - 1)))
    if len(s) >= 1 and s != "0":
        options.append(lit(s))
    return alt(*options)


def decimal_between(lo, hi):
    """A whole number from lo to hi (lo >= 1) in decimal, no leading zero."""
    assert lo >= 1
    if lo == 1:
        return alt(*[o for o in decimal_at_most(hi).parts if not (isinstance(o, Sym) and o.chars == frozenset("0"))])
    raise ValueError("decimal_between: lo above 1 is not needed yet")


def g9(most=38, least=45):
    """A number as printf's %.9g writes a finite number (a float by default: the hallway's files hold
    floats; a double's range is most=308, least=324).

    Fixed notation where the decimal exponent X is from -4 to 8: the digits with no trailing zero after
    a point and no point without a digit after it, at most 9 significant digits; otherwise d.ddddddddde
    and a sign and at least two exponent digits (X from 9 up to `most`, or from -5 down to -`least`), the
    mantissa likewise stripped. Zero is "0" ("-0" for negative zero). This is the shape of what %.9g
    writes; which of these a float or double gives back exactly (a float keeps about 7 digits, so
    1.00000001 is no float's) is beyond a walk, and left to the reader."""
    sign = opt(lit("-"))
    nz, d = chars(NONZERO), chars(DIGITS)

    def frac(most):  # a point and 1..most digits, the last not 0
        if most < 1:
            return Empty()
        return opt(seq(".", rep(d, 0, most - 1), nz))

    fixed = []
    # integer part of k digits (1..9), X = k - 1, fraction of up to 9 - k digits
    for k in range(1, 10):
        fixed.append(seq(nz, rep(d, k - 1, k - 1), frac(9 - k)))
    # 0.000ddd: X = -1 .. -4 (z leading zeros after the point, z = 0..3), then 1..9 significant digits
    for z in range(0, 4):
        fixed.append(seq("0.", rep(lit("0"), z, z), alt(nz, seq(nz, rep(d, 0, 7), nz))))
    # exponential: X <= -5 or X >= 9, at least two digits
    big = alt(*[lit("%02d" % x) for x in range(9, most + 1)])
    small = alt(*[lit("%02d" % x) for x in range(5, least + 1)])
    mant = seq(nz, frac(9))
    expo = alt(seq("e+", big), seq("e-", small))
    return alt(lit("0"), lit("-0"), seq(sign, alt(*fixed)), seq(sign, mant, expo))


# ---- the compiler


def nfa(r):
    """Thompson's construction: (states, start, accept), states a list of (epsilon targets, [(chars, target)])."""
    eps, edges = [], []

    def new():
        eps.append([])
        edges.append([])
        return len(eps) - 1

    def build(r):
        if isinstance(r, Empty):
            s = new()
            return s, s
        if isinstance(r, Sym):
            a, b = new(), new()
            edges[a].append((r.chars, b))
            return a, b
        if isinstance(r, Seq):
            if not r.parts:
                s = new()
                return s, s
            first, last = build(r.parts[0])
            for p in r.parts[1:]:
                a, b = build(p)
                eps[last].append(a)
                last = b
            return first, last
        if isinstance(r, Alt):
            a, b = new(), new()
            for p in r.parts:
                x, y = build(p)
                eps[a].append(x)
                eps[y].append(b)
            return a, b
        if isinstance(r, Star):
            a, b = new(), new()
            x, y = build(r.inner)
            eps[a] += [x, b]
            eps[y] += [x, b]
            return a, b
        raise TypeError(r)

    s, f = build(r)
    return eps, edges, s, f


def dfa(r):
    eps, edges, start, final = nfa(r)

    def closure(states):
        stack, seen = list(states), set(states)
        while stack:
            for t in eps[stack.pop()]:
                if t not in seen:
                    seen.add(t)
                    stack.append(t)
        return frozenset(seen)

    first = closure([start])
    index, sets, trans = {first: 0}, [first], []
    i = 0
    while i < len(sets):
        cur, row = sets[i], {}
        for c in ALPHABET:
            nxt = set()
            for s in cur:
                for cs, t in edges[s]:
                    if c in cs:
                        nxt.add(t)
            if nxt:
                key = closure(nxt)
                if key not in index:
                    index[key] = len(sets)
                    sets.append(key)
                row[c] = index[key]
        trans.append(row)
        i += 1
    accept = {k for k, s in enumerate(sets) if final in s}
    return trans, accept


def minimise(trans, accept):
    """Moore's refinement (simple, and quick enough at these sizes), then renumbered in walk order from
    the start, the states that reach no accepting state left out."""
    n = len(trans)
    part = [1 if s in accept else 0 for s in range(n)]
    while True:
        sig = {}
        new = []
        for s in range(n):
            key = (part[s], tuple(part[trans[s][c]] if c in trans[s] else -1 for c in ALPHABET))
            new.append(sig.setdefault(key, len(sig)))
        if len(sig) == len(set(part)):
            part = new
            break
        part = new
    m = max(part) + 1
    mtrans = [dict() for _ in range(m)]
    macc = set()
    for s in range(n):
        for c, t in trans[s].items():
            mtrans[part[s]][c] = part[t]
        if s in accept:
            macc.add(part[s])
    # the states that can reach an accepting state
    live, changed = set(macc), True
    while changed:
        changed = False
        for s in range(m):
            if s not in live and any(t in live for t in mtrans[s].values()):
                live.add(s)
                changed = True
    start = part[0]
    if start not in live:
        raise ValueError("the grammar admits nothing")
    order, seen = [start], {start}
    i = 0
    while i < len(order):
        for c in ALPHABET:
            t = mtrans[order[i]].get(c)
            if t is not None and t in live and t not in seen:
                seen.add(t)
                order.append(t)
        i += 1
    num = {s: k for k, s in enumerate(order)}
    out = [{c: num[t] for c, t in mtrans[s].items() if t in live} for s in order]
    return out, sorted(num[s] for s in macc if s in num)


def symbol_spec(cs):
    """A class's symbols as a plugin names them: runs of digits as @a..@b, single ones as @a."""
    ds = sorted(DIGIT[c] for c in cs)
    parts, i = [], 0
    while i < len(ds):
        j = i
        while j + 1 < len(ds) and ds[j + 1] == ds[j] + 1:
            j += 1
        parts.append(f"@{ds[i]}" if i == j else f"@{ds[i]}..@{ds[j]}")
        i = j + 1
    return " ".join(parts)


def ranges(nums):
    out, i = [], 0
    while i < len(nums):
        j = i
        while j + 1 < len(nums) and nums[j + 1] == nums[j] + 1:
            j += 1
        out.append(str(nums[i]) if i == j else f"{nums[i]}..{nums[j]}")
        i = j + 1
    return " ".join(out)


def sfilter(r, head):
    trans, accept = minimise(*dfa(r))
    # classes: the symbols whose column is the same in every state
    cols = {}
    for c in ALPHABET:
        key = tuple(row.get(c, -1) for row in trans)
        if any(k >= 0 for k in key):
            cols.setdefault(key, []).append(c)
    classes = sorted(cols.items(), key=lambda kv: min(DIGIT[c] for c in kv[1]))
    names = {}
    lines = list(head)
    for i, (key, cs) in enumerate(classes):
        name = f"c{i}"
        for c in cs:
            names[c] = name
        shown = "".join("\\n" if c == "\n" else c for c in sorted(cs, key=DIGIT.get))
        if len(shown) > 40:
            shown = shown[:37] + "..."
        lines.append(f"class     {name:<5} {symbol_spec(cs):<24} ; {shown}")
    lines += ["", f"states    {len(trans)}", "start     0", f"accept    {ranges(accept)}"]
    for s, row in enumerate(trans):
        done = set()
        for c in sorted(row, key=DIGIT.get):
            n = names[c]
            if n in done:
                continue
            done.add(n)
            lines.append(f"t   {s:<5} {n:<5} {row[c]}")
    lines.append("end")
    return "\n".join(lines) + "\n", len(trans), len(classes)


# ---- the formats

ACTIONS = ["idle", "walk", "run", "jump", "take", "read", "wave"]  # sieve/actions.hpp kActionNames


def actions_v1():
    body = [lit("sieve-actions-v1\n")]
    for a in ACTIONS:
        body.append(seq(f"action {a} ", alt(sha256(), lit("none")), "\n"))
    return Seq(body)


def save():
    sha = lambda key: seq(f"{key} ", sha256(), "\n")
    speaks = star(seq("speaks ", plain_id(128), "\n"))
    v1 = seq("sieve-save-v1\n", sha("character"), opt(sha("animation")), speaks)
    v2 = seq("sieve-save-v2\n", sha("character"), opt(sha("animation")), sha("actions"), speaks)
    return alt(v1, v2)


def replay_v1():
    """sieve-replay-v1 (client/replay.cpp read_replay): the start, then a line a frame."""
    ns = chars("".join(c for c in ALPHABET if c not in "\n "))  # a field's characters: anything but a space
    f = g9()
    num = lambda most: decimal_at_most(most)
    start = seq("start line=", star(ns), " tile=", star(ns), " x=", f, " y=", f, " z=", f, " yaw=", f, " pitch=", f, "\n")
    held = alt(lit("-"), seq(num(511), star(seq(",", num(511)))))
    key = seq(alt(lit("kd:"), lit("ku:")), num(511), ":", num(0xFFFFFFFF), ":", num(0xFFFF), ":", chars("01"))
    # (each byte two digits, never 00: the text is kept as SDL keeps it, ended by a NUL, so a 00 byte
    # would end it there and the rest never be written back)
    byte = alt(seq(chars(LOWER_HEX[1:]), chars(LOWER_HEX)), seq("0", chars(LOWER_HEX[1:])))
    text = seq("tx:", rep(byte, 1, None))
    motion = seq("mm:", f, ":", f, ":", f, ":", f)
    button = seq(alt(lit("bd:"), lit("bu:")), num(255), ":", num(255), ":", f, ":", f)
    wheel = seq("mw:", f, ":", f)
    token = alt(key, text, motion, button, wheel)
    frame = seq("f ", decimal_between(1, 100000), " ", held, star(seq(" ", token)), "\n")
    return seq("sieve-replay-v1\n", start, star(frame))


WORLD_KINDS = ["pages", "image", "audio", "video", "books", "models", "binary", "tracks", "movies", "worlds", "ai", "bookcase"]  # PersonalWorld::catalogue


def world_v1():
    """sieve-personal-world-v1 as it is written (client/personal_world.cpp save): its seed, then what is placed."""
    f = g9()
    place = seq("place ", alt(*[lit(k) for k in WORLD_KINDS]), " ", f, " ", f, " ", f, "\n")
    return seq("sieve-personal-world-v1\nseed ", decimal_at_most(2 ** 64 - 1), "\n", star(place))


# ---- sieve-personal-world-v2 (SPECIFICATIONS 12.0r, client/personal_world.cpp load and save)

NOT_SPACE = "".join(c for c in ALPHABET if c not in "\n ")  # a word's characters
ANY = "".join(c for c in ALPHABET if c != "\n")  # a line's characters


def word_except(words, alphabet=NOT_SPACE):
    """Any word of one or more of `alphabet` but those in `words`: the words walked as a tree, each step
    off it leading to any word at all."""
    words = set(words)

    def at(p):
        out = [] if (p in words or p == "") else [Empty()]
        nxt = sorted({w[len(p)] for w in words if w.startswith(p) and len(w) > len(p)})
        others = "".join(c for c in alphabet if c not in nxt)
        if others:
            out.append(seq(chars(others), star(chars(alphabet))))
        for c in nxt:
            out.append(seq(lit(c), at(p + c)))
        return alt(*out)

    return at("")


def decimal_range(lo, hi):
    """A whole number from lo to hi, with no sign or leading zero (a small range, each written out)."""
    return alt(*[lit(str(n)) for n in range(lo, hi + 1)])


def g9_nonzero_within_500():
    """What %.9g writes for a float other than 0 and at most 500 either way (a ground corner's change):
    g9's shapes with no exponent above 2, a 3-digit whole part from 100 to 499, or 500 itself."""
    nz, d = chars(NONZERO), chars(DIGITS)

    def frac(most):
        return opt(seq(".", rep(d, 0, most - 1), nz))

    fixed = [seq(nz, frac(8)), seq(nz, d, frac(7)), seq(chars("1234"), d, d, frac(6)), lit("500")]
    for z in range(0, 4):
        fixed.append(seq("0.", rep(lit("0"), z, z), alt(nz, seq(nz, rep(d, 0, 7), nz))))
    small = alt(*[lit("%02d" % x) for x in range(5, 46)])
    return seq(opt(lit("-")), alt(*fixed, seq(nz, frac(9), "e-", small)))


NOTE_LETTERS = {"C": 0, "D": 2, "E": 4, "F": 5, "G": 7, "A": 9, "B": 11}  # sieve/audio letter_semitone
SHARP_NAMES = ["C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"]  # sieve/audio midi_name
DURATION_CODES = "seEqQhHw"  # sieve/audio kNoteDurationCodes
MUSIC_MODES = ["lydian", "ionian", "mixolydian", "dorian", "aeolian", "phrygian", "locrian"]  # client/music.cpp kModes


def note_spellings():
    """Every name note_midi_of reads, by its MIDI number: a letter (either case), # or b or nothing,
    then one digit."""
    out = {}
    for big, semi in NOTE_LETTERS.items():
        for letter in (big, big.lower()):
            for acc, da in (("", 0), ("#", 1), ("b", -1)):
                for o in range(10):
                    m = (o + 1) * 12 + semi + da
                    if m >= 0:
                        out.setdefault(m, []).append(letter + acc + str(o))
    return out


def durations():
    """Duration codes listed once each, shortest first, at least one."""
    return alt(*[lit("".join(c for i, c in enumerate(DURATION_CODES) if mask >> i & 1)) for mask in range(1, 256)])


def note_range(spell, joiner):
    """LOW joiner HIGH, within MIDI 36 to 96 and at least an octave (sieve/audio make_note_set)."""
    out = []
    for lo in range(36, 86):
        highs = [n for m in range(lo + 11, 97) for n in spell(m)]
        out.append(seq(alt(*[lit(n) for n in spell(lo)]), joiner, alt(*[lit(n) for n in highs])))
    return alt(*out)


MUSIC_KEYS = ["on", "source", "volume", "length", "units", "tempo", "echo", "gap", "voice", "notes", "character", "filters", "param", "melody"]


def music_line():
    """A `music` line as client/music.cpp personal_music_of reads it (a world writes it back as read)."""
    spellings = note_spellings()
    notes2 = seq("notes2/", note_range(lambda m: spellings.get(m, []), "/"), "/", durations(), "/", chars("1234"), opt(lit("/")))
    set_id = alt(lit("notes104"), seq("notes2/", note_range(lambda m: [SHARP_NAMES[m % 12] + str(m // 12 - 1)], "-"), "/", durations(), "/V", chars("1234")))
    no_colon = chars("".join(c for c in NOT_SPACE if c != ":"))
    hexd = chars("0123456789abcdefABCDEF")
    pct_value = star(alt(chars("".join(c for c in NOT_SPACE if c != "%")), seq("%", hexd, hexd)))
    digit = alt(lit("0"), seq(chars(NONZERO), rep(chars(DIGITS), 0, 2)))  # (below its set's base, at most 496)
    values = {
        "on": alt(lit("on"), lit("off")),
        "source": alt(lit("audio"), lit("tracks")),
        "volume": decimal_range(0, 100),
        "length": decimal_range(1, 4096),
        "units": decimal_range(1, 64),
        "tempo": decimal_range(20, 300),
        "echo": decimal_range(0, 80),
        "gap": decimal_range(0, 600),
        "voice": alt(*[lit(v) for v in ("soft", "sine", "triangle", "square")]),
        "notes": alt(lit("notes104"), notes2),
        "character": alt(*[lit(m) for m in ["none"] + MUSIC_MODES]),
        "filters": star(chars(NOT_SPACE)),
        "param": seq(rep(no_colon, 1, None), ":", rep(no_colon, 1, None), ":", pct_value),
        "melody": seq(set_id, ":", decimal_range(0, 64), ":", digit, star(seq(",", digit)), opt(lit(","))),
    }
    assert list(values) == MUSIC_KEYS
    no_eq = "".join(c for c in NOT_SPACE if c != "=")
    unknown = seq(word_except(MUSIC_KEYS, no_eq), "=", star(chars(NOT_SPACE)))  # (a later build's: kept)
    word = alt(*[seq(f"{k}=", v) for k, v in values.items()], unknown)
    return seq(star(lit(" ")), "music", star(seq(rep(lit(" "), 1, None), word)), star(lit(" ")))


def world_v2():
    """sieve-personal-world-v2 as a world is read with nothing left out and written back byte for byte:
    the lines in the order the writer writes them, and something after the seed (a world needing none
    of v2 is written as v1)."""
    f = g9()
    corner = alt(*[lit(str(x)) for x in range(-320, 321, 10)])
    day = seq("day ", decimal_between(1, 2 ** 64 - 1), "\n")
    ground = seq("ground ", corner, " ", corner, " ", g9_nonzero_within_500(), "\n")
    model = seq("model ", sha256(), "\n")
    music = seq(music_line(), "\n")
    # A line kept without being read: its first word none the reader reads ("characters" only where
    # the next is not "placed"), kept as it is.
    rest = opt(seq(" ", star(chars(ANY))))
    kept = seq(star(lit(" ")), alt(seq(word_except(["seed", "day", "ground", "model", "music", "place", "characters"]), rest),
                                   seq("characters", alt(star(lit(" ")), seq(rep(lit(" "), 1, None), word_except(["placed"]), rest)))), "\n")
    word64 = rep(chars(NOT_SPACE), 64, 64)
    things = [lit(k) for k in WORLD_KINDS] + [seq("fav:", word64), seq("model:", sha256())]
    people = things + [lit("char:you"), seq("char:", word64)]
    place = lambda kinds: seq("place ", alt(*kinds), " ", f, " ", f, " ", f, " ", f, " ", f, " ", f, " ", alt(lit("fixed"), lit("physics")), "\n")
    placed = alt(star(place(things)), seq("characters placed\n", star(place(people))))
    placed1 = alt(rep(place(things), 1, None), seq("characters placed\n", star(place(people))))
    # (the first part that is there, then the rest as they may be)
    after = [day, star(ground), star(model), opt(music), star(kept)]
    firsts = [seq(day, *after[1:], placed), seq(rep(ground, 1, None), *after[2:], placed), seq(rep(model, 1, None), *after[3:], placed),
              seq(music, after[4], placed), seq(rep(kept, 1, None), placed), placed1]
    return seq("sieve-personal-world-v2\nseed ", decimal_at_most(2 ** 64 - 1), "\n", alt(*firsts))


PLUGINS = [
    ("format-actions", 1, actions_v1,
     "One sieve-actions-v1 map exactly: an action's animation for each of the seven actions, in order (IDEAS 19.185, 19.186).",
     ["The map of actions to animations (sieve-actions-v1, SPECIFICATIONS 12.0p's neighbour, core/include/sieve/actions.hpp)",
      "written as a filter: \"sieve-actions-v1\", then \"action NAME SHA-256\" or \"action NAME none\" for idle, walk,",
      "run, jump, take, read and wave, once each in that order, every line ending in a line feed. Its",
      "survivors are exactly the maps read_actions reads (tools/check_formats.py holds it to the reader)."]),
    ("format-save", 1, save,
     "One sieve-save-v1 or sieve-save-v2 exactly (the languages' order aside): everything chosen in character creation (IDEAS 19.185).",
     ["A character's save (sieve-save-v1 and sieve-save-v2, core/include/sieve/save.hpp) written as a filter:",
      "the version, \"character SHA-256\", an \"animation SHA-256\" where there is one, in v2 \"actions SHA-256\",",
      "then any number of \"speaks ID\" (1 to 128 of a-z, 0-9, '-', '.', '_'), every line ending in a line feed.",
      "One thing the reader asks and a walk cannot: that the languages stand in the order of their ids, each",
      "once (it would have to remember a whole id to weigh the next against it). So its survivors are the",
      "reader's saves with their languages in any order; tools/check_formats.py checks every other verdict."]),
    ("format-replay", 1, replay_v1,
     "One sieve-replay-v1 recording exactly (the keys held in order and the numbers a float's own aside): where it began and every input (IDEAS 19.185, 19.184).",
     ["A recording of the hallway (sieve-replay-v1, SPECIFICATIONS 12.0q, client/replay.cpp) written as a filter:",
      "\"sieve-replay-v1\", then \"start line=NAME tile=T x= y= z= yaw= pitch=\" (NAME and T any characters but a",
      "space; the numbers as %.9g writes them), then a line a frame: \"f DT HELD\" (DT 1 to 100000; HELD - or",
      "scancodes 0 to 511 apart by commas) and its events (kd:/ku:SC:KEY:MOD:0|1, tx:HEX with no 00 byte,",
      "mm:X:Y:DX:DY, bd:/bu:B:CLICKS:X:Y, mw:DX:DY), every line ending in a line feed. Two things the reader asks and a",
      "walk does not: that the keys held stand in increasing order (a walk would need a state for each last",
      "key), and that each number is the one %.9g writes for a float (1.00000001 has %.9g's shape but no",
      "float is it). So its survivors are the reader's recordings with those two let be."]),
    ("format-world", 2, world_v2,
     "One sieve-personal-world-v2 exactly as it is written (what a walk cannot weigh aside): a personal dimension's seed, day, ground, models, music, kept lines and what is placed (IDEAS 19.185, 19.187).",
     ["A personal dimension's world (sieve-personal-world-v2, SPECIFICATIONS 12.0r, client/personal_world.cpp)",
      "written as a filter, in the order its writer writes it: \"sieve-personal-world-v2\", \"seed N\", then \"day N\"",
      "(1 to 2^64 - 1), \"ground X Z DH\" lines (X and Z on the 10 m grid from -320 to 320, DH not 0 and at most",
      "500 either way), \"model SHA\" lines, a \"music\" line as the Media Player reads it, the lines kept without",
      "being read, \"characters placed\" (only then a char: placed), then \"place WHAT X Y Z YAW PITCH ROLL fixed|physics\"",
      "lines, at least one of them all (a world with nothing after its seed is written as v1). Things a walk",
      "does not weigh, each checked by tools/check_formats.py: the ground's corners in order and once each, a",
      "model named once and every model placed named, a melody's digits below its note set's base and a",
      "whole number of units and voices, each number the one %.9g writes for a float, and, in a world with",
      "nothing else of v2's, whether a place stands off the ground (that needs the seed's ground)."]),
    ("format-world", 1, world_v1,
     "One sieve-personal-world-v1 exactly as it is written (the numbers a float's own aside): a personal dimension's seed and what is placed (IDEAS 19.185, 19.187).",
     ["A personal dimension's world (sieve-personal-world-v1, SPECIFICATIONS 12.0r, client/personal_world.cpp)",
      "written as a filter, as its writer writes it: \"sieve-personal-world-v1\", \"seed N\" (0 to 2^64 - 1, no",
      "leading zero), then \"place KIND X Z YAW\" a line (KIND one of the catalogue's twelve, the numbers as",
      "%.9g writes them), every line ending in a line feed. The reader is lenient (it reads \"1e5\" and two",
      "spaces, and leaves out what it cannot read); the grammar is the form a world is read in with nothing",
      "left out and written back byte for byte, which is what tools/check_formats.py holds it to. One thing",
      "the round trip asks and a walk does not: that each number is the one %.9g writes for a float."]),
]


def write_all(check=False):
    here = os.path.dirname(os.path.abspath(__file__))
    folder = os.path.join(here, "..", "data", "filters")
    bad = 0
    for pid, ver, make, describe, notes in PLUGINS:
        head = ["sieve-filter-v1"]
        head += ["; " + n for n in notes]
        head += ["; Written by tools/grammars.py (the grammar is there; this table is what it compiles to).", ""]
        head += [f"id        {pid}", f"version   {ver}", "author    Edward James Gordon (written with Claude)", "origin    ai-directed",
                 "lines     text", "symbols   ascii96", f"describe  {describe}", ""]
        text, n_states, n_classes = sfilter(make(), head)
        path = os.path.join(folder, f"{pid}-v{ver}.sfilter")
        old = open(path, encoding="utf-8").read() if os.path.exists(path) else None
        if check:
            if old != text:
                print(f"{pid}-v{ver}.sfilter differs from what tools/grammars.py writes")
                bad += 1
        else:
            with open(path, "w", encoding="utf-8", newline="\n") as f:
                f.write(text)
            print(f"{pid}-v{ver}.sfilter: {n_states} states, {n_classes} classes")
    return bad


if __name__ == "__main__":
    sys.exit(1 if write_all("--check" in sys.argv) else 0)
