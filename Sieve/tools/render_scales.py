#!/usr/bin/env python3
"""The render scales sheet: one character and the shelf items drawn at every scale the one renderer has
(IDEAS 19.207, 19.214), each panel a real frame made by the hallway itself, laid out with captions.

A recipe to keep and grow (Edward, 14 November 2026: "this is something we should expand and build upon
more in the future, so it is worth keeping a recipe of sorts for this render"): to add a scale, add a
step to STEPS (how the hallway is run, and where in the frame to look) and it takes its place on the sheet.

    python tools/render_scales.py --hallway build/hallway --out render-scales.png

Every frame is made in a folder of its own (--work, else a temporary one) from what this file writes there:
the character's settings line (seed 99) and the small worlds. Nothing personal is read: the hallway is
given its own settings files, and HOME is pointed into the work folder.
"""
import argparse
import os
import subprocess
import sys
import tempfile

from PIL import Image, ImageDraw, ImageFont

CHARACTER = "sieve-character-v1 seed=99 limbs=5 joints=3 branches=1 symmetry=bilateral head=yes\n"
# (the character is 8 m ahead of where you begin; a bookcase 2.7 m ahead whose end crosses it)
WORLD_BOOKCASE = "sieve-personal-world-v2\nseed 0\nplace bookcase 0 22.9 -57 3.14159274 0 0 fixed\nplace pages -0.1 25.15 -56.82 0 0 0 fixed\n"
WORLD_CROSSING = "sieve-personal-world-v2\nseed 0\nplace bookcase 2.95 22.9 -53.4 3.14159274 0 0 fixed\n"
SIZE = "1600x900"
CREATOR = (640, 140, 1560, 700)  # where the Character Creator draws the character, at SIZE
HALL = (300, 150, 1500, 850)
WORLD_NEAR = (420, 250, 1180, 690)

# Each step: (caption, line under it, settings file name or None, hallway arguments, crop box).
# Settings files: "wire" (Real Graphics off), "real" (on), "d0".."d3" (the Character Creator's drawings).
STEPS = [
    ("PURE STATE", "a settings line: everything else is made from it", None, None, None),
    ("SKELETON + SPHERES OF INFLUENCE", "bones joining each body piece's reach (rings)", "d1", ["--menu", "--real-graphics", "--character", "c.character", "--creator"], CREATOR),
    ("PIXELS", "flat squares of one size, nearest sphere's colour", "d0", ["--menu", "--real-graphics", "--character", "c.character", "--creator"], CREATOR),
    ("SPHERES, TRUE 3D", "each sphere's depth at every pixel", "d3", ["--menu", "--real-graphics", "--character", "c.character", "--creator"], CREATOR),
    ("VOXELS", "cubes on a grid fixed to the character", "d2", ["--menu", "--real-graphics", "--character", "c.character", "--creator"], CREATOR),
    ("VOXELS, TURNED", "tops and sides: they turn with it", "d2", ["--menu", "--real-graphics", "--character", "c.character", "--creator", "--press", "],],],]"], (600, 120, 1600, 720)),
    ("TRIANGLES", "a mesh's edges, hidden where covered", "wire", ["--personal-world", "--world-file", "bookcase.world", "--pworld-look", "0", "--no-map", "--no-compass"], (110, 110, 1490, 905)),
    ("TRIANGLES FILLED", "the same mesh, solid", "real", ["--personal-world", "--world-file", "bookcase.world", "--pworld-look", "0", "--no-map", "--no-compass"], (100, 90, 1500, 900)),
    ("ONE DEPTH BUFFER, WIREFRAME", "rings cut by a bookcase's edge", "wire", ["--character", "c.character", "--personal-world", "--world-file", "crossing.world", "--no-map", "--no-compass", "--pworld-walk", "0.95", "--pworld-look", "-12"], WORLD_NEAR),
    ("ONE DEPTH BUFFER, SOLID", "spheres and triangles cut each other", "real", ["--character", "c.character", "--personal-world", "--world-file", "crossing.world", "--no-map", "--no-compass", "--pworld-walk", "0.95", "--pworld-look", "-12"], WORLD_NEAR),
    ("THE HALLWAY, WIREFRAME", "Real Graphics off: rings and bones", "wire", ["--no-menu", "--character", "c.character", "--pose", "0,1.6,0,-10"], HALL),
    ("THE HALLWAY, REAL GRAPHICS", "shaded spheres among the shelves", "real", ["--no-menu", "--character", "c.character", "--pose", "0,1.6,0,-10"], HALL),
]
FOOT = [
    "Every panel is a frame drawn by Sieve through one renderer (MeshBatch): one character (seed 99) and the shelf items.",
    "The spheres are each body piece's sphere of influence: a basic skeleton for now. Quads and smooth surfaces (NURBS) wait on a shell grown",
    "over them (IDEAS 19.214); greedy meshing of the voxels is IDEAS 19.215. Made by tools/render_scales.py.",
]


def font(name, size):
    for folder in ("/usr/share/fonts/truetype/dejavu/", "C:/Windows/Fonts/"):
        try:
            return ImageFont.truetype(folder + name, size)
        except OSError:
            pass
    return ImageFont.load_default()


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--hallway", required=True, help="the hallway program")
    ap.add_argument("--out", required=True, help="the sheet, a PNG")
    ap.add_argument("--work", help="where the frames are made (default: a temporary folder)")
    a = ap.parse_args()
    hallway = os.path.abspath(a.hallway)
    work = a.work or tempfile.mkdtemp(prefix="render-scales-")
    os.makedirs(work, exist_ok=True)
    files = {"c.character": CHARACTER, "bookcase.world": WORLD_BOOKCASE, "crossing.world": WORLD_CROSSING,
             "wire.ini": "[graphics]\nreal_graphics = off\n", "real.ini": "[graphics]\nreal_graphics = on\nedge_glow = off\n"}
    for k in range(4):
        files[f"d{k}.ini"] = f"[character]\ndrawing = {k}\n"
    for name, text in files.items():
        with open(os.path.join(work, name), "w", newline="\n") as f:
            f.write(text)
    env = dict(os.environ, SDL_VIDEO_DRIVER=os.environ.get("SDL_VIDEO_DRIVER", "offscreen"), HOME=work, USERPROFILE=work)

    PW, PH, G, COLS = 440, 260, 16, 4
    panels = []
    for i, (cap, sub, settings, args, box) in enumerate(STEPS):
        if args is None:
            state = Image.new("RGB", (PW, PH), (8, 10, 14))
            d = ImageDraw.Draw(state)
            m = font("DejaVuSansMono.ttf", 14)
            words = CHARACTER.split()
            lines = [" ".join(words[:2]), "  " + " ".join(words[2:5]), "  " + " ".join(words[5:]), "",
                     "its spheres of influence, unfolded", "from the seed: the same line makes", "the same character on any machine"]
            for k, l in enumerate(lines):
                d.text((20, 18 + 22 * k), l, font=m, fill=(150, 220, 160))
            panels.append((state, cap, sub))
            continue
        shot = os.path.join(work, f"step{i:02d}.png")
        cmd = [hallway, "--size", SIZE, "--settings", settings + ".ini"] + args + ["--screenshot", shot]
        r = subprocess.run(cmd, cwd=work, env=env, capture_output=True, text=True)
        if r.returncode != 0 or not os.path.exists(shot):
            sys.exit(f"step {i + 1} ({cap}) failed: {' '.join(cmd)}\n{r.stdout[-2000:]}{r.stderr[-2000:]}")
        img = Image.open(shot).convert("RGB").crop(box)
        s = max(PW / img.width, PH / img.height)
        img = img.resize((int(img.width * s) + 1, int(img.height * s) + 1), Image.LANCZOS)
        x, y = (img.width - PW) // 2, (img.height - PH) // 2
        panels.append((img.crop((x, y, x + PW, y + PH)), cap, sub))

    rows = (len(panels) + COLS - 1) // COLS
    TH, CH, FH = 70, 46, 24 + 20 * len(FOOT)
    W, H = COLS * PW + (COLS + 1) * G, TH + rows * (PH + CH + G) + FH
    out = Image.new("RGB", (W, H), (4, 6, 9))
    d = ImageDraw.Draw(out)
    d.text((G, 18), "POTENTIA : SIEVE : ONE RENDERER, MANY SCALES", font=font("DejaVuSansMono-Bold.ttf", 30), fill=(235, 235, 240))
    cap_f, sub_f = font("DejaVuSansMono-Bold.ttf", 16), font("DejaVuSansMono.ttf", 13)
    for i, (img, cap, sub) in enumerate(panels):
        r, c = divmod(i, COLS)
        x, y = G + c * (PW + G), TH + r * (PH + CH + G)
        out.paste(img, (x, y))
        d.rectangle((x - 1, y - 1, x + PW, y + PH), outline=(70, 74, 84))
        d.text((x, y + PH + 6), f"{i + 1}. {cap}", font=cap_f, fill=(235, 235, 240))
        d.text((x, y + PH + 26), sub, font=sub_f, fill=(150, 155, 165))
    for k, l in enumerate(FOOT):
        d.text((G, H - FH + 4 + 20 * k), l, font=sub_f, fill=(150, 155, 165))
    out.save(a.out)
    print(f"saved {a.out} ({len(panels)} panels; frames in {work})")


if __name__ == "__main__":
    main()
