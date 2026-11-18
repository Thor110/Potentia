"""Recordings of the hallway written by hand (sieve-replay-v1, client/replay.hpp), for the trailer.

A Walker keeps where the camera is as the hallway would (client/hallway.cpp: 3 m/s walking along
(sin yaw, 0, cos yaw), 9 running, the mouse turning 0.0025 radians a pixel at 100% sensitivity) and
writes one frame line per step: the keys held and the mouse moved. Its sense of place is only for
planning the moves: the hallway itself decides where each step lands (walls, doors), and a door
changes the line, so a walk is written to cross where a door is (z 6.4 to 7.6 of a tile).
"""
import math
import struct

DT_US = 33333                 # 30 frames a second, as the film is made
DT = DT_US / 1e6
LOOK = 0.0025                 # radians a pixel of mouse (look_ at 100%)

# SDL3 scancodes and keycodes of the keys used.
SC = {"w": 26, "a": 4, "s": 22, "d": 7, "e": 8, "t": 23, "g": 10, "m": 16, "x": 27, "z": 29, "c": 6,
      "j": 13, "p": 19, "n": 17, "b": 5, "r": 21, "o": 18, "return": 40, "escape": 41, "backspace": 42,
      "tab": 43, "space": 44, "f1": 58, "home": 74, "pageup": 75, "pagedown": 78, "right": 79,
      "left": 80, "down": 81, "up": 82, "lshift": 225, "minus": 45, "equals": 46, "lbracket": 47,
      "rbracket": 48}
KEY = {"return": 13, "escape": 27, "backspace": 8, "tab": 9, "space": 32, "f1": 0x4000003A,
       "home": 0x4000004A, "pageup": 0x4000004B, "pagedown": 0x4000004E, "right": 0x4000004F,
       "left": 0x40000050, "down": 0x40000051, "up": 0x40000052, "lshift": 0x400000E1, "minus": 45,
       "equals": 61, "lbracket": 91, "rbracket": 93}


def f32(v):
    """A number as the recording keeps it: the float it is, written by %.9g."""
    return "%.9g" % struct.unpack("f", struct.pack("f", v))[0]


def keycode(name):
    return KEY.get(name, ord(name) if len(name) == 1 else 0)


class Walker:
    def __init__(self, line, tile, x=0.0, y=1.6, z=1.0, yaw=0.0, pitch=0.0):
        self.line, self.tile = line, tile
        self.x, self.y, self.z, self.yaw, self.pitch = x, y, z, yaw, pitch
        self.start = (x, y, z, yaw, pitch)
        self.frames = []
        # A click first: the hallway takes the mouse (relative mode), so its moves turn you.
        self.frame(events=["bd:1:1:960:540"])
        self.frame(events=["bu:1:1:960:540"])

    # One frame: keys held (names), and events (already written tokens).
    def frame(self, held=(), events=()):
        h = ",".join(str(c) for c in sorted(SC[k] for k in held)) or "-"
        self.frames.append("f %d %s%s" % (DT_US, h, "".join(" " + e for e in events)))

    def wait(self, seconds):
        for _ in range(round(seconds / DT)):
            self.frame()

    def press(self, name, mod=0, wait=0.0):
        sc, kc = SC[name], keycode(name)
        self.frame(events=["kd:%d:%d:%d:0" % (sc, kc, mod)])
        self.frame(events=["ku:%d:%d:%d:0" % (sc, kc, mod)])
        self.wait(wait)

    def type(self, text, per_char=0.06):
        for ch in text:
            self.frame(events=["tx:" + ch.encode("utf-8").hex()])
            self.wait(per_char)

    # Turn to a yaw (and pitch) over `seconds`, eased, by mouse moves, optionally walking meanwhile.
    def turn(self, yaw, seconds, pitch=None, walk=False, run=False):
        n = max(1, round(seconds / DT))
        y0, p0 = self.yaw, self.pitch
        p1 = self.pitch if pitch is None else pitch
        done_x = done_y = 0
        for i in range(1, n + 1):
            f = 0.5 - 0.5 * math.cos(math.pi * i / n)
            want_x = round((yaw - y0) * f / LOOK)
            want_y = round(-(p1 - p0) * f / LOOK)
            dx, dy = want_x - done_x, want_y - done_y
            done_x, done_y = want_x, want_y
            ev = ["mm:960:540:%d:%d" % (dx, dy)] if dx or dy else []
            held = (["w"] + (["lshift"] if run else [])) if walk else []
            self.frame(held, ev)
            self.yaw = y0 + done_x * LOOK
            self.pitch = p0 - done_y * LOOK
            if walk:
                self._step(9.0 if run else 3.0)

    def _step(self, speed):
        self.x += math.sin(self.yaw) * speed * DT
        self.z += math.cos(self.yaw) * speed * DT

    def walk(self, metres, run=False):
        speed = 9.0 if run else 3.0
        for _ in range(round(abs(metres) / (speed * DT))):
            self.frame(["w"] + (["lshift"] if run else []))
            self._step(speed)

    # Walk straight on until the corridor position is z (within a tile, 0 to 8 m; facing along it).
    def walk_to_z(self, z, run=False):
        ahead = (z - self.z) % 8.0 if math.cos(self.yaw) > 0 else (self.z - z) % 8.0
        self.walk(ahead, run)

    # Through the door beside you: turn to face the left wall (or right), walk through, and come out of
    # the next line's opposite door, a step into its corridor, turned to face along it (+z).
    def through_door(self, side="left", turn_time=0.7, step_in=1.4):
        self.turn(-math.pi / 2 if side == "left" else math.pi / 2, turn_time)
        self.walk(abs(self.x - (-2.2 if side == "left" else 2.2)))
        self.x = 1.9 if side == "left" else -1.9
        self.walk(step_in)
        self.turn(0.0, turn_time)

    def text(self):
        x, y, z, yaw, pitch = self.start
        head = "sieve-replay-v1\nstart line=%s tile=%s x=%s y=%s z=%s yaw=%s pitch=%s\n" % (
            self.line, self.tile, f32(x), f32(y), f32(z), f32(yaw), f32(pitch))
        return head + "".join(f + "\n" for f in self.frames)

    # The time into the recording now (for timing captions to what happens).
    def now(self):
        return len(self.frames) * DT

    def seconds(self):
        return len(self.frames) * DT
