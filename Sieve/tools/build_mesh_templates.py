#!/usr/bin/env python3
"""Writes the Real Graphics template models to data/meshes/templates/ (see README.md there).

    python3 tools/build_mesh_templates.py

Four templates, each a Wavefront .obj with its .mtl, built to the hallway's own measurements
(client/world.hpp), in metres, Y up, the corridor running along +Z:

    hallway.obj    one 8 m tile of corridor: floor, ceiling, both walls, and a doorway with its
                   frame and door in each wall (the door is recessed; the engine keeps it black)
    bookshelf.obj  the bookcase on the LEFT wall (the right one is its mirror image in X)
    book.obj       one book at the uniform size (0.40 m tall), spine facing +X (the corridor,
                   for the left wall)
    marker.obj     the checkered start/finish strip on the floor

Copy each to <model>-<media>.obj (hallway-pages.obj, book-audio.obj, ...) and model from there.
"""
import os

# client/world.hpp
HALF_WIDTH, HEIGHT, TILE = 2.0, 3.0, 8.0
CASE_FRONT, SHELF_END = 1.65, 6.0
DOOR_START, DOOR_END, DOOR_TOP = 6.4, 7.6, 2.2
ROWS, ROW_TOP, ROW_HEIGHT = 4, 2.65, 0.55
COLS = 16
BOOK_PITCH, BOOK_WIDTH, UNIFORM_BOOK_HEIGHT = SHELF_END / COLS, 0.28, 0.40
CASE_TOP = ROW_TOP + 0.2

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "data", "meshes", "templates")


class Mesh:
    def __init__(self, name):
        self.name = name
        self.v, self.vt, self.vn = [], [], []
        self.groups = []  # (group, material, [faces]); a face is [(v, vt, vn), ...]

    def group(self, name, material):
        self.groups.append((name, material, []))

    def quad(self, p0, p1, p2, p3, n):
        """A quad, counter-clockwise seen from the side its normal `n` points to. UVs: p0 = (0, 0),
        p1 = (u, 0), p3 = (0, v), u and v the edge lengths in metres (so textures tile per metre)."""
        def length(a, b):
            return sum((x - y) ** 2 for x, y in zip(a, b)) ** 0.5
        # Wind counter-clockwise about the normal, whatever order the corners came in.
        e1 = [b - a for a, b in zip(p0, p1)]
        e2 = [b - a for a, b in zip(p0, p3)]
        cross = (e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0])
        if sum(c * d for c, d in zip(cross, n)) < 0:
            p1, p3 = p3, p1
        u, w = length(p0, p1), length(p0, p3)
        base_v, base_vt = len(self.v), len(self.vt)
        self.v += [p0, p1, p2, p3]
        self.vt += [(0, 0), (u, 0), (u, w), (0, w)]
        if n not in self.vn:
            self.vn.append(n)
        ni = self.vn.index(n) + 1
        self.groups[-1][2].append([(base_v + k + 1, base_vt + k + 1, ni) for k in range(4)])

    def box(self, x0, x1, y0, y1, z0, z1, skip=()):
        """An axis-aligned box, faces outward. `skip`: faces to leave out ('-x', '+x', '-y', ...)."""
        faces = {
            "-x": ((x0, y0, z1), (x0, y0, z0), (x0, y1, z0), (x0, y1, z1), (-1, 0, 0)),
            "+x": ((x1, y0, z0), (x1, y0, z1), (x1, y1, z1), (x1, y1, z0), (1, 0, 0)),
            "-y": ((x0, y0, z1), (x1, y0, z1), (x1, y0, z0), (x0, y0, z0), (0, -1, 0)),
            "+y": ((x0, y1, z0), (x1, y1, z0), (x1, y1, z1), (x0, y1, z1), (0, 1, 0)),
            "-z": ((x1, y0, z0), (x0, y0, z0), (x0, y1, z0), (x1, y1, z0), (0, 0, -1)),
            "+z": ((x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1), (0, 0, 1)),
        }
        for k, (a, b, c, d, n) in faces.items():
            if k not in skip:
                self.quad(a, b, c, d, n)

    def write(self, header, materials):
        os.makedirs(OUT, exist_ok=True)
        fmt = lambda x: f"{x:.4f}".rstrip("0").rstrip(".") if abs(x) > 1e-9 else "0"
        with open(os.path.join(OUT, self.name + ".obj"), "w", newline="\n") as f:
            f.write("".join(f"# {line}\n" for line in header))
            f.write(f"mtllib {self.name}.mtl\n")
            for p in self.v:
                f.write("v " + " ".join(fmt(c) for c in p) + "\n")
            for t in self.vt:
                f.write("vt " + " ".join(fmt(c) for c in t) + "\n")
            for n in self.vn:
                f.write("vn " + " ".join(fmt(c) for c in n) + "\n")
            for name, material, faces in self.groups:
                f.write(f"g {name}\nusemtl {material}\n")
                for face in faces:
                    f.write("f " + " ".join(f"{a}/{b}/{c}" for a, b, c in face) + "\n")
        with open(os.path.join(OUT, self.name + ".mtl"), "w", newline="\n") as f:
            f.write(f"# Materials for {self.name}.obj: placeholder colours, one material per part.\n")
            for name, rgb, note in materials:
                f.write(f"\n# {note}\nnewmtl {name}\nKd {rgb[0]:.3f} {rgb[1]:.3f} {rgb[2]:.3f}\nKa 0 0 0\nKs 0 0 0\nd 1\nillum 1\n")


def hallway():
    m = Mesh("hallway")
    w, h, t = HALF_WIDTH, HEIGHT, TILE
    m.group("floor", "floor")
    m.quad((-w, 0, 0), (w, 0, 0), (w, 0, t), (-w, 0, t), (0, 1, 0))
    m.group("ceiling", "ceiling")
    m.quad((-w, h, t), (w, h, t), (w, h, 0), (-w, h, 0), (0, -1, 0))
    frame, trim, recess = 0.08, 0.04, 0.06  # door frame width, how far it stands out, door depth
    for side, sx in (("left", -1), ("right", 1)):
        x = sx * w
        n = (-sx, 0, 0)  # the wall faces into the corridor

        def wall_quad(z0, z1, y0, y1, xx=x, nn=n):
            if sx < 0:
                m.quad((xx, y0, z0), (xx, y0, z1), (xx, y1, z1), (xx, y1, z0), nn)
            else:
                m.quad((xx, y0, z1), (xx, y0, z0), (xx, y1, z0), (xx, y1, z1), nn)

        m.group(f"wall_{side}", "wall")
        wall_quad(0, DOOR_START, 0, h)
        wall_quad(DOOR_END, t, 0, h)
        wall_quad(DOOR_START, DOOR_END, DOOR_TOP, h)
        # The doorway's reveal (the sides and top of the opening, back to the door).
        m.group(f"door_{side}", "door")
        xd = x + sx * recess
        wall_quad(DOOR_START, DOOR_END, 0, DOOR_TOP, xd)
        m.group(f"door_reveal_{side}", "wall")
        lo, hi = sorted((x, xd))
        m.box(lo, hi, 0, DOOR_TOP, DOOR_START - 0.001, DOOR_START, skip=("-x", "+x", "-y", "+y", "-z"))
        m.box(lo, hi, 0, DOOR_TOP, DOOR_END, DOOR_END + 0.001, skip=("-x", "+x", "-y", "+y", "+z"))
        m.box(lo, hi, DOOR_TOP, DOOR_TOP + 0.001, DOOR_START, DOOR_END, skip=("-x", "+x", "-z", "+z", "+y"))
        m.group(f"door_sill_{side}", "floor")
        m.quad((lo, 0, DOOR_START), (hi, 0, DOOR_START), (hi, 0, DOOR_END), (lo, 0, DOOR_END), (0, 1, 0))
        # The frame: two posts and a lintel standing out of the wall.
        m.group(f"door_frame_{side}", "door_frame")
        fx0, fx1 = sorted((x, x - sx * trim))
        face_skip = ("+x",) if sx > 0 else ("-x",)  # the side against the wall is never seen
        m.box(fx0, fx1, 0, DOOR_TOP + frame, DOOR_START - frame, DOOR_START, skip=face_skip + ("-y",))
        m.box(fx0, fx1, 0, DOOR_TOP + frame, DOOR_END, DOOR_END + frame, skip=face_skip + ("-y",))
        m.box(fx0, fx1, DOOR_TOP, DOOR_TOP + frame, DOOR_START, DOOR_END, skip=face_skip)
    header = [
        "Sieve Real Graphics template: hallway (one tile of corridor). Built by tools/build_mesh_templates.py.",
        "Units metres, Y up, the corridor runs along +Z. The tile spans z 0..8, x -2..2, y 0..3; it is",
        "drawn once per tile, repeated every 8 m. Doorways: z 6.4..7.6, 2.2 m high, in both walls.",
        "Groups: floor, ceiling, wall_left/right, door_left/right, door_reveal_left/right, door_sill_left/right,",
        "door_frame_left/right.",
        "Keep the door groups where they are: walking through z 6.4..7.6 at a wall is how you change line.",
    ]
    m.write(header, [("floor", (0.10, 0.10, 0.10), "floor"), ("ceiling", (0.12, 0.12, 0.12), "ceiling"),
                     ("wall", (0.20, 0.20, 0.20), "walls and the doorway's reveal"),
                     ("door_frame", (0.45, 0.45, 0.45), "door frames"), ("door", (0.0, 0.0, 0.0), "doors (black in the wireframe)")])


def bookshelf():
    m = Mesh("bookshelf")
    front, back, board, side = -CASE_FRONT, -HALF_WIDTH, 0.03, 0.04
    # The case: two side panels, a top, a plinth, the back, and a board under each row of books.
    m.group("case_sides", "shelf_wood")
    m.box(back, front, 0, CASE_TOP, 0, side, skip=("-x",))
    m.box(back, front, 0, CASE_TOP, SHELF_END - side, SHELF_END, skip=("-x",))
    m.group("case_top", "shelf_wood")
    m.box(back, front, CASE_TOP - board, CASE_TOP, 0, SHELF_END, skip=("-x",))
    m.group("case_back", "shelf_back")
    m.quad((back + 0.001, 0, SHELF_END), (back + 0.001, 0, 0), (back + 0.001, CASE_TOP, 0), (back + 0.001, CASE_TOP, SHELF_END), (1, 0, 0))
    m.group("boards", "shelf_wood")
    for r in range(ROWS + 1):
        y = ROW_TOP - r * ROW_HEIGHT  # the wireframe's shelf lines; books stand 0.02 above each
        m.box(back, front, y - board, y + 0.02 - 0.001, side, SHELF_END - side, skip=("-x",))
    m.group("plinth", "shelf_wood")
    m.box(back, front, 0, ROW_TOP - ROWS * ROW_HEIGHT - board, side, SHELF_END - side, skip=("-x", "-y"))
    header = [
        "Sieve Real Graphics template: bookshelf (the LEFT wall's). Built by tools/build_mesh_templates.py.",
        "Units metres, Y up, the corridor runs along +Z. The case spans z 0..6 of each tile, from the wall",
        "(x = -2) to its front (x = -1.65), 2.85 m tall. The right wall's case is this one mirrored in X.",
        "Rows of books: 4, their tops at y = 2.65, 2.10, 1.55, 1.00 (0.55 apart); each book stands on the",
        "board 0.02 above y = 2.65 - (row + 1) * 0.55. 16 books per row, 0.375 m apart, from z = 0.",
    ]
    m.write(header, [("shelf_wood", (0.35, 0.25, 0.15), "the case and its boards"), ("shelf_back", (0.18, 0.13, 0.08), "the back panel")])


def book():
    m = Mesh("book")
    hw, hgt, depth, cover = BOOK_WIDTH / 2, UNIFORM_BOOK_HEIGHT, 0.30, 0.012
    # Local frame: origin at the bottom of the spine, centred across it; the spine faces +X.
    m.group("spine", "book_spine")
    m.box(-0.004, 0, 0, hgt, -hw, hw, skip=("-x", "-y", "+y", "-z", "+z"))
    m.group("covers", "book_cover")
    m.box(-depth, 0, 0, hgt, -hw, -hw + cover, skip=("+x",))
    m.box(-depth, 0, 0, hgt, hw - cover, hw, skip=("+x",))
    m.group("pages", "book_pages")
    m.box(-depth + 0.004, -0.004, 0.006, hgt - 0.006, -hw + cover, hw - cover, skip=("-z", "+z", "+x"))
    header = [
        "Sieve Real Graphics template: book (one slot). Built by tools/build_mesh_templates.py.",
        "Units metres, Y up. Local frame: origin at the bottom of the spine, centred across it; the",
        "spine faces +X (the corridor, for the left wall; the right wall's books are mirrored in X).",
        "Size: 0.28 wide (Z), 0.40 tall (Y, the uniform size), 0.30 deep (-X). Placed at x = -1.65, on",
        "its row's board, centred in its 0.375 m slot. Pages, image and books lines scale it in Y to",
        "the slot's height (0.34..0.46 m); the audio (record) and video (tape) lines never scale it.",
    ]
    m.write(header, [("book_spine", (0.60, 0.10, 0.10), "the spine (faces the corridor)"),
                     ("book_cover", (0.50, 0.08, 0.08), "front and back covers"), ("book_pages", (0.90, 0.88, 0.80), "the page block")])


def marker():
    m = Mesh("marker")
    squares, sq = 16, 2 * HALF_WIDTH / 16
    for row in range(2):
        for i in range(squares):
            light = (i + row) % 2 == 0
            m.group(f"square_{row}_{i}", "marker_light" if light else "marker_dark")
            x0, x1 = -HALF_WIDTH + i * sq, -HALF_WIDTH + (i + 1) * sq
            z0, z1 = -sq + row * sq, row * sq
            y = 0.001
            m.quad((x0, y, z0), (x0, y, z1), (x1, y, z1), (x1, y, z0), (0, 1, 0))
    header = [
        "Sieve Real Graphics template: the start/finish marker. Built by tools/build_mesh_templates.py.",
        "Units metres, Y up. A checkered strip on the floor across the corridor, two rows of 16",
        "squares of 0.25 m, from z = -0.25 to 0.25 about the line where a loop of the line starts",
        "(z = 0 of a tile). Where every line starts together, the engine draws it twice, 1 m apart.",
        "It sits 1 mm above the floor so it never flickers against it.",
    ]
    m.write(header, [("marker_light", (1.0, 1.0, 1.0), "light squares (the line's edge colour in the wireframe)"),
                     ("marker_dark", (0.0, 0.0, 0.0), "dark squares (the line's background colour)")])


if __name__ == "__main__":
    hallway()
    bookshelf()
    book()
    marker()
    print("wrote", os.path.normpath(OUT))
