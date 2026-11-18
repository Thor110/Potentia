"""Running the hallway for the trailer: a recording played into a film, or a still taken."""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SIEVE = os.path.abspath(os.path.join(HERE, "..", ".."))
ENV = dict(os.environ, SDL_VIDEO_DRIVER="offscreen")
SIZE = "1920x1080"


class Hallway:
    def __init__(self, exe, work):
        self.exe, self.work = exe, work
        os.makedirs(work, exist_ok=True)
        # A settings file of its own, so nothing of the person running it is drawn (their character,
        # their dimension) and the film is the same for everyone.
        self.settings = os.path.join(work, "sieve-hallway.ini")
        self.filters = os.path.join(work, "sieve-filters.ini")

    def run(self, args, check=True):
        own = [] if "--filters" in args else ["--filters", self.filters]
        cmd = [self.exe, "--size", SIZE, "--settings", self.settings, "--no-music"] + own + args
        p = subprocess.run(cmd, cwd=SIEVE, env=ENV, capture_output=True, text=True)
        if check and p.returncode != 0:
            sys.exit("hallway failed: %s\n%s%s" % (" ".join(cmd), p.stdout, p.stderr))
        return p.stdout

    # Where a hallway started with these options begins: its line name and tile.
    def start_of(self, args):
        path = os.path.join(self.work, "start.replay")
        self.run(args + ["--record", path, "--screenshot", os.path.join(self.work, "start.png")])
        m = re.search(r"start line=(\S+) tile=(\S+)", open(path).read())
        return m.group(1), m.group(2)

    def film(self, walker, out, args=()):
        rp = os.path.splitext(out)[0] + ".replay"
        with open(rp, "w") as f:
            f.write(walker.text())
        return self.run(list(args) + ["--replay", rp, "--replay-video", out, "--screenshot", os.path.splitext(out)[0] + "-end.png"])

    def still(self, out, args=()):
        return self.run(list(args) + ["--screenshot", out])
