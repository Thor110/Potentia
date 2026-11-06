#!/usr/bin/env python3
"""The lines' colours seen as colour-blind people see them (IDEAS 2.4): each line's two colours
(its walls and its edges, client/dimensions.hpp), and every pair of lines, compared as they are and
through a simulation of protanopia, deuteranopia and tritanopia, so that the pairs only colour told
apart, and which colour blindness loses them, can be found and changed.

  python tools/colour_check.py            (from the Sieve folder)

The simulation is Machado, Oliveira and Fernandes's (2009) at full severity, applied in linear RGB;
the difference between two colours is CIEDE2000 (about 2.3 is the least most people notice; under 10
two colours are easily taken for each other at a glance, as a wall at the end of a corridor is seen).
Lines that share a colour by design (a composition and the line it is made of: tracks and audio,
movies and video, worlds and models) are compared by their other colour, the edges they differ by.
It reads the source, so it needs no build. Prints a table; exit 0 always (it measures, it does not
judge).
"""

import math
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent

# Machado et al. 2009, severity 1.0 (linear RGB).
SIM = {
    "protanopia": [[0.152286, 1.052583, -0.204868], [0.114503, 0.786281, 0.099216], [-0.003882, -0.048116, 1.051998]],
    "deuteranopia": [[0.367322, 0.860646, -0.227968], [0.280085, 0.672501, 0.047413], [-0.011820, 0.042940, 0.968881]],
    "tritanopia": [[1.255528, -0.076749, -0.178779], [-0.078411, 0.930809, 0.147602], [0.004733, 0.691367, 0.303900]],
}


def to_linear(c):
    c /= 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def to_srgb(c):
    c = min(1.0, max(0.0, c))
    return 255.0 * (12.92 * c if c <= 0.0031308 else 1.055 * c ** (1 / 2.4) - 0.055)


def simulate(rgb, kind):
    if kind == "normal":
        return rgb
    lin = [to_linear(v) for v in rgb]
    m = SIM[kind]
    return tuple(to_srgb(sum(m[i][j] * lin[j] for j in range(3))) for i in range(3))


def lab(rgb):
    r, g, b = (to_linear(v) for v in rgb)
    x = (0.4124 * r + 0.3576 * g + 0.1805 * b) / 0.95047
    y = 0.2126 * r + 0.7152 * g + 0.0722 * b
    z = (0.0193 * r + 0.1192 * g + 0.9505 * b) / 1.08883
    f = lambda t: t ** (1 / 3) if t > 216 / 24389 else (24389 / 27 * t + 16) / 116
    fx, fy, fz = f(x), f(y), f(z)
    return 116 * fy - 16, 500 * (fx - fy), 200 * (fy - fz)


def ciede2000(c1, c2):
    L1, a1, b1 = lab(c1)
    L2, a2, b2 = lab(c2)
    C1, C2 = math.hypot(a1, b1), math.hypot(a2, b2)
    Cb = (C1 + C2) / 2
    G = 0.5 * (1 - math.sqrt(Cb ** 7 / (Cb ** 7 + 25 ** 7)))
    a1p, a2p = (1 + G) * a1, (1 + G) * a2
    C1p, C2p = math.hypot(a1p, b1), math.hypot(a2p, b2)
    h1p = math.degrees(math.atan2(b1, a1p)) % 360
    h2p = math.degrees(math.atan2(b2, a2p)) % 360
    dLp, dCp = L2 - L1, C2p - C1p
    dhp = 0 if C1p * C2p == 0 else (h2p - h1p if abs(h2p - h1p) <= 180 else h2p - h1p - 360 if h2p > h1p else h2p - h1p + 360)
    dHp = 2 * math.sqrt(C1p * C2p) * math.sin(math.radians(dhp / 2))
    Lbp, Cbp = (L1 + L2) / 2, (C1p + C2p) / 2
    hbp = h1p + h2p if C1p * C2p == 0 else ((h1p + h2p) / 2 if abs(h1p - h2p) <= 180 else (h1p + h2p + 360) / 2 if h1p + h2p < 360 else (h1p + h2p - 360) / 2)
    T = 1 - 0.17 * math.cos(math.radians(hbp - 30)) + 0.24 * math.cos(math.radians(2 * hbp)) + 0.32 * math.cos(math.radians(3 * hbp + 6)) \
        - 0.20 * math.cos(math.radians(4 * hbp - 63))
    dt = 30 * math.exp(-(((hbp - 275) / 25) ** 2))
    Rc = 2 * math.sqrt(Cbp ** 7 / (Cbp ** 7 + 25 ** 7))
    Sl = 1 + 0.015 * (Lbp - 50) ** 2 / math.sqrt(20 + (Lbp - 50) ** 2)
    Sc, Sh = 1 + 0.045 * Cbp, 1 + 0.015 * Cbp * T
    Rt = -math.sin(math.radians(2 * dt)) * Rc
    return math.sqrt((dLp / Sl) ** 2 + (dCp / Sc) ** 2 + (dHp / Sh) ** 2 + Rt * (dCp / Sc) * (dHp / Sh))


def lines():
    text = (ROOT / "client" / "dimensions.hpp").read_text(encoding="utf-8")
    pat = re.compile(r'\{Media::(\w+), "(\w+)".*?\{\{(\d+), (\d+), (\d+), 255\}, \{(\d+), (\d+), (\d+), 255\}')
    return [(m.group(2), tuple(int(m.group(i)) for i in (3, 4, 5)), tuple(int(m.group(i)) for i in (6, 7, 8))) for m in pat.finditer(text)]


def main():
    ls = lines()
    kinds = ["normal", "protanopia", "deuteranopia", "tritanopia"]
    print("Each pair of lines: the CIEDE2000 difference of their walls and of their edges, as seen")
    print("normally and through each simulation; the greater of the two (what tells them apart). Under 10 marked *.\n")
    print("%-16s %-16s" % ("line", "line") + "".join("%14s" % k for k in kinds))
    close = []
    for i in range(len(ls)):
        for j in range(i + 1, len(ls)):
            (n1, w1, e1), (n2, w2, e2) = ls[i], ls[j]
            row = []
            for k in kinds:
                dw = ciede2000(simulate(w1, k), simulate(w2, k))
                de = ciede2000(simulate(e1, k), simulate(e2, k))
                # Told apart by whichever differs more: walls, or (sharing walls by design) edges.
                d = max(dw, de)
                row.append(d)
            flag = [("*" if d < 10 else " ") for d in row]
            print("%-16s %-16s" % (n1, n2) + "".join("%13.1f%s" % (d, f) for d, f in zip(row, flag)))
            if row[0] >= 10 and min(row[1:]) < 10:
                close.append((n1, n2, row))
    print("\nTold apart normally, but not through a simulation (under 10):")
    for n1, n2, row in close:
        worst = min(range(1, 4), key=lambda k: row[k])
        print("  %s and %s: %.1f normally, %.1f with %s" % (n1, n2, row[0], row[worst], kinds[worst]))
    if not close:
        print("  none")
    return 0


if __name__ == "__main__":
    sys.exit(main())
