#!/usr/bin/env python3
"""Plot M1 sieve results: surviving fraction of the space against unit length.

    python3 tools/plot_sieve.py results/m1_sieve_scowl60.csv results/m1_sieve_scowl60.png "SCOWL 2020.12.07 size 60"
"""
import csv
import math
import sys

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

SURFACE, INK, INK2, GRID = "#fcfcfb", "#1a1a19", "#5f5e58", "#e6e5df"
SERIES = {  # fixed categorical order
    "clean": "#2a78d6",
    "window": "#eb6834",
    "words": "#1baf7a",
    "english": "#eda100",
}


def main(src, dst, dict_label="dictionary"):
    rows = [r for r in csv.DictReader(line for line in open(src) if not line.startswith("#"))]
    L = [int(r["length"]) for r in rows]
    y = {k: [float(r[f"log10_frac_{k}"]) for r in rows] for k in ("clean", "window", "words")}
    # Estimate only: English at ~1 bit/char, i.e. 2^L meaningful units out of 27^L.
    y["english"] = [n * (math.log10(2) - math.log10(27)) for n in L]

    fig, ax = plt.subplots(figsize=(10, 6), dpi=150)
    fig.patch.set_facecolor(SURFACE)
    ax.set_facecolor(SURFACE)
    labels = {
        "clean": "Clean: no double spaces",
        "window": "Window: could be cut from running text",
        "words": "Words: every token a dictionary word",
        "english": "English at ~1 bit/char (estimate, not measured)",
    }
    for k, color in SERIES.items():
        ax.plot(L, y[k], color=color, linewidth=2, linestyle="--" if k == "english" else "-",
                marker=None if k == "english" else "o", markersize=3.5, label=labels[k],
                solid_capstyle="round", zorder=3)
        # Direct label at the right end, in ink (identity comes from the swatch in the legend).
        nudge = {"window": 7, "words": -7}.get(k, 0)  # these two nearly coincide
        ax.annotate(f"{y[k][-1]:.0f}", xy=(L[-1], y[k][-1]), xytext=(6, nudge), textcoords="offset points",
                    va="center", fontsize=9, color=INK)

    ax.set_xlabel("Unit length (characters)", color=INK2)
    ax.set_ylabel("Surviving fraction of all 27^L units (log10)", color=INK2)
    ax.set_title("M1 sieve: how much of the lower27 space survives each filter", color=INK, loc="left",
                 fontsize=13, pad=12)
    ax.grid(True, color=GRID, linewidth=0.8, zorder=0)
    for s in ("top", "right"):
        ax.spines[s].set_visible(False)
    for s in ("left", "bottom"):
        ax.spines[s].set_color(GRID)
    ax.tick_params(colors=INK2)
    ax.set_xlim(0, L[-1] * 1.06)
    ax.legend(loc="lower left", frameon=False, labelcolor=INK)
    fig.text(0.01, 0.015,
            "Exact counts (dynamic programming), verified by brute force to L=6 and by pruned tree walk "
            f"to L=7. Dictionary: {dict_label}.",
            ha="left", va="bottom", fontsize=8.5, color=INK2)
    fig.tight_layout(rect=(0, 0.04, 1, 1))
    fig.savefig(dst, facecolor=SURFACE)


if __name__ == "__main__":
    main(*sys.argv[1:4])
