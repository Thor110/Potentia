"""The trailer for Potentia : Sieve, made from the programs themselves.

    python tools/trailer/trailer.py --hallway BUILD/hallway --sieve BUILD/sieve --out OUT_FOLDER

Every moving shot is a recording of the hallway written by hand (walker.py) and played into a film by
the hallway itself (--replay-video); every still is a picture the hallway takes of one of its screens
(stills.py); the cards and captions are drawn in Sieve's own font (cards.py); the music is melodies of
the audio line chosen as the hallway's music player chooses them (music.py). The figures quoted are
those written in the repository (Proof/STATISTICS.md, metrics/builds, docs/IDEAS.md), or measured by
the run itself (the help pages packed and installed, chapter 6). Needs ffmpeg, Python 3, Pillow, numpy.

--only NAME,... remakes those segments only; --skip-music leaves the film silent.
"""
import argparse
import math
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import cards  # noqa: E402
import music  # noqa: E402
from shoot import SIEVE, Hallway  # noqa: E402
from stills import STILLS  # noqa: E402
from walker import Walker  # noqa: E402

FPS = 30
ICON = os.path.join(HERE, "work-icon.png")  # (made by the run, not kept: .gitignore)
ENC = ["-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p", "-r", str(FPS)]


def ff(*args):
    subprocess.run(["ffmpeg", "-loglevel", "error", "-y"] + list(args), check=True)


def duration(path):
    out = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "csv=p=0", path],
                         capture_output=True, text=True).stdout
    return float(out.strip())


# ---------------------------------------------------------------- segments

class Card:
    def __init__(self, name, secs, **kw):
        self.name, self.secs, self.kw = name, secs, kw

    def make(self, ctx, out):
        png = os.path.join(ctx.work, self.name + ".png")
        cards.card(png, **self.kw)
        f = 0.6
        ff("-loop", "1", "-i", png, "-t", str(self.secs), "-vf",
           "fade=t=in:st=0:d=%g,fade=t=out:st=%g:d=%g" % (f, self.secs - f, f), *ENC, out)


def overlay_captions(ctx, src, out, caps, line, fade=0.4):
    """caps: (t0, t1, head, text). Each caption drawn and laid over its stretch of the film."""
    d = duration(src)
    inputs, chain, last = ["-i", src], [], "[0:v]"
    for i, (t0, t1, head, text) in enumerate(caps):
        png = os.path.join(ctx.work, "%s-cap%d.png" % (os.path.basename(out), i))
        cards.caption(png, text, line=line, head=head)
        inputs += ["-i", png]
        tag = "[v%d]" % i
        chain.append("%s[%d:v]overlay=enable='between(t,%g,%g)'%s" % (last, i + 1, t0, t1, tag))
        last = tag
    chain.append("%sfade=t=in:st=0:d=%g,fade=t=out:st=%g:d=%g[out]" % (last, fade, max(0, d - fade), fade))
    ff(*inputs, "-filter_complex", ";".join(chain), "-map", "[out]", *ENC, out)


class Film:
    """A recording played into a film; build(ctx, hallway) gives (walker, options, captions)."""

    def __init__(self, name, build, line="pages"):
        self.name, self.build, self.line = name, build, line

    def make(self, ctx, out):
        h = Hallway(ctx.hallway, os.path.join(ctx.work, "h-" + self.name))
        w, args, caps = self.build(ctx, h)
        raw = os.path.join(ctx.work, self.name + "-raw.mp4")
        h.film(w, raw, args)
        overlay_captions(ctx, raw, out, caps, self.line)


class Stills:
    """Pictures of screens, each held a few seconds with a slow push in and its caption."""

    def __init__(self, name, shots, line="pages", secs=5.5):
        self.name, self.shots, self.line, self.secs = name, shots, line, secs

    def make(self, ctx, out):
        parts = []
        for i, shot in enumerate(self.shots):
            key, head, text = shot[:3]
            secs = shot[3] if len(shot) > 3 else self.secs
            png = os.path.join(ctx.work, "still-%s.png" % key)
            if not os.path.exists(png):
                h = Hallway(ctx.hallway, os.path.join(ctx.work, "h-still-" + key))
                pre = STILLS.get(key + ":before")
                if pre:  # (a run whose settings this one picks up, e.g. OPTIMISE FILTERS first)
                    h.run(pre + ["--screenshot", png + ".before.png"], check=False)
                h.run(STILLS[key] + ["--screenshot", png], check=False)
            n = int(secs * FPS)
            clip = os.path.join(ctx.work, "%s-%d.mp4" % (self.name, i))
            zoomed = os.path.join(ctx.work, "%s-%d-z.mp4" % (self.name, i))
            ff("-loop", "1", "-i", png, "-vf",
               "scale=3840:2160,zoompan=z='1+0.05*on/%d':x='iw/2-(iw/zoom/2)':y='ih/2-(ih/zoom/2)':d=%d:s=1920x1080:fps=%d" % (n, n, FPS),
               "-frames:v", str(n), *ENC, zoomed)
            overlay_captions(ctx, zoomed, clip, [(0.5, secs - 0.3, head, text)] if text else [], self.line, fade=0.35)
            parts.append(clip)
        concat(parts, out)


def concat(parts, out):
    lst = out + ".txt"
    with open(lst, "w") as f:
        for p in parts:
            f.write("file '%s'\n" % os.path.abspath(p))
    ff("-f", "concat", "-safe", "0", "-i", lst, "-c", "copy", out)


# ---------------------------------------------------------------- the films

def start(h, args):
    line, tile = h.start_of(args)
    return line, tile


def cold_open(ctx, h):
    args = ["--no-menu", "--wireframe", "off"]
    w = Walker(*start(h, args), x=0.0, z=0.5, yaw=0.0, pitch=0.0)
    w.wait(1.0)
    w.walk(11.0)
    w.turn(-0.5, 2.0, walk=True)
    w.walk(5.0)
    w.turn(0.35, 3.0, walk=True)
    w.walk(6.0)
    caps = [(1.5, 6.0, None, "Every possible page."),
            (6.5, 11.0, None, "Every possible picture, melody, film and model."),
            (11.5, 16.5, None, "Every possible file."),
            (17.0, w.now() - 0.5, None, "Each one at exactly one address, on a shelf you can walk to.")]
    return w, args, caps


def address_film(ctx, h):
    args = ["--no-menu"]
    w = Walker(*start(h, args), x=0.0, z=1.0, yaw=0.0)
    caps = []
    w.wait(1.0)
    t = w.now()
    w.press("t", wait=0.6)
    w.type("it was the best of times", 0.07)
    w.wait(0.5)
    w.press("return", wait=4.0)
    caps.append((t, w.now(), "WARP (T)", "Type any text and go to it: the shelf where it has always stood, in hand."))
    t = w.now()
    for _ in range(3):
        w.press("c", wait=3.0)
    w.press("c", wait=1.0)
    caps.append((t, w.now(), "C: ITEM / COST / SORT / HELP",
                 "Its address, what it costs to name, where it sits among the anchors you have mapped, and every key."))
    w.press("e", wait=2.5)
    t = w.now()
    w.press("m", wait=3.5)
    caps.append((t, w.now(), "M: ORDERING", "Scrambled: the same line, every place shuffled by a key."))
    t = w.now()
    w.press("m", wait=4.0)
    caps.append((t, w.now(), "GUIDED", "Guided: a language model orders the line, so likely pages come first. "
                 "A 32-letter page: 152 bits positional, 58 guided."))
    w.press("m", wait=1.5)
    t = w.now()
    w.press("g", wait=0.5)
    w.type("50%")
    w.press("return", wait=3.5)
    caps.append((t, w.now(), "G: GO TO", "Any address, a percentage, or a tile. Halfway along the line."))
    t = w.now()
    w.press("x", wait=4.5)
    caps.append((t, w.now(), "X: THE NAVIGATOR", "The address itself, digit by digit; R for a random place, Shift+R a random line."))
    w.press("escape", wait=0.8)
    t = w.now()
    w.press("home", wait=3.5)
    caps.append((t, w.now(), "HOME", "Back to tile 0, where every line starts. The wheel, PgUp/PgDn and [ ] jump 1, 1,000 and 1,000,000 tiles."))
    return w, args, caps


TOUR = [
    ("BOOKS", "A cover, a title and 4 pages: about 10^259.1 books."),
    ("AUDIO", "Every melody of 16 notes, with its cover and title: about 10^108.2."),
    ("TRACKS", "4 melodies joined: about 10^205.0."),
    ("VIDEO", "Every film of 8 frames, 5 x 5 pixels: about 10^136.1."),
    ("MOVIES", "4 films joined: about 10^316.7."),
    ("MODELS", "Every mesh of 8 vertices and 12 triangles, about 10^107.2. A cube is one of them."),
    ("WORLDS", "4 models on a grid, each turned one of 24 ways: about 10^337.9."),
    ("AI", "Every language model of 1 layer, width 16, 4-bit weights: about 10^9864.2. Walking it downloads nothing."),
    ("FRACTALS", "Every ruleset up to 256 characters, about 10^405.1; the 197 charted fractals come first."),
    ("BINARY", "Every file up to 32 bytes, about 10^122.9: the line every other one is bounded by."),
]


def tour(ctx, h):
    args = ["--no-menu"]
    w = Walker(*start(h, args), x=0.4, z=1.0, yaw=0.0, pitch=-0.05)
    caps = [(0.3, 0.0, "PAGES", "Every page of 32 letters, with its title: about 10^91.6. The left door leads to the next line.")]
    w.walk(5.9)
    caps[0] = (0.3, w.now(), caps[0][2], caps[0][3])
    for name, text in TOUR:
        w.walk_to_z(7.0)
        w.through_door()
        t = w.now() - 0.8
        w.walk(2.4)
        # a look at the left-hand shelves, and on
        w.turn(-0.75, 1.2, pitch=-0.12)
        w.wait(1.4)
        w.turn(0.0, 1.0, pitch=-0.05)
        w.walk(2.1)
        caps.append((t, w.now(), name, text))
    w.wait(1.0)
    return w, args, caps


def tour_solid(ctx, h):
    args = ["--no-menu", "--line", "image", "--wireframe", "off"]
    w = Walker(*start(h, args), x=0.4, z=1.0, yaw=0.0, pitch=-0.05)
    caps = [(0.3, 5.0, "WIREFRAME OFF", "The same corridors drawn solid: wood, shelves, and doorways filled with data noise.")]
    w.walk(5.9)
    caps.append((5.2, w.now(), "IMAGE", "Every 10 x 10 black and white picture: about 10^75.9."))
    for name, text in [("PAGES", "Each line keeps its own colours."), ("BOOKS", "Grey, with black edges."),
                       ("AUDIO", "Green and amber.")]:
        w.walk_to_z(7.0)
        w.through_door()
        t = w.now() - 0.8
        w.walk(4.0)
        caps.append((t, w.now(), name, text))
    return w, args, caps


def jumps(ctx, h):
    args = ["--no-menu"]
    w = Walker(*start(h, args), x=0.0, z=1.0, yaw=0.0, pitch=-0.05)
    caps = []
    t = w.now()
    w.walk(4.0)
    for _ in range(int(3.0 * FPS)):
        w.frame(["w", "lshift"])
        w._step(9.0)
    caps.append((t, w.now(), "SHIFT: RUN", "Walking is 3 metres a second, running 9: a tile of 128 pages every second or so."))
    t = w.now()
    for k in ("pageup", "pageup", "rbracket", "rbracket", "rbracket"):
        w.press(k, wait=1.6)
    caps.append((t, w.now(), "PGUP AND ]", "1,000 tiles a press, then 1,000,000: the tile number at the top climbs, and the line goes on."))
    t = w.now()
    w.press("home", wait=2.5)
    caps.append((t, w.now(), "HOME", "However far you go, Home brings you back to tile 0."))
    return w, args, caps


def picture(ctx, h):
    args = ["--no-menu", "--line", "image", "--image-palette", "rgb332", "--image-width", "32", "--image-height", "32",
            "--warp", ICON]
    w = Walker(*start(h, args), x=0.0, z=1.0, yaw=0.0)
    caps = []
    w.wait(3.0)
    caps.append((0.3, w.now(), "A PICTURE, WARPED", "Sieve's own icon, warped onto the image line (32 x 32, 256 colours): the place it has always had."))
    t = w.now()
    w.press("z", wait=4.5)
    caps.append((t, w.now(), "Z: THE VIEWER", "Larger, and saved as PNG, GIF or MP4 with F."))
    w.press("escape", wait=0.8)
    w.press("e", wait=1.5)
    t = w.now()
    w.turn(-0.5, 1.5, pitch=-0.1)
    w.walk(3.0)
    w.turn(0.4, 2.0, walk=True)
    w.walk(2.0)
    caps.append((t, w.now(), "ITS NEIGHBOURS", "In positional order its neighbours differ from it only in their last pixels."))
    return w, args, caps


def fractal_walk(ctx, h):
    args = ["--no-menu", "--line", "fractals", "--wireframe", "off"]
    w = Walker(*start(h, args), x=0.2, z=0.5, yaw=-0.55, pitch=-0.1)
    w.wait(1.0)
    w.walk(3.0)
    w.turn(0.55, 2.5, walk=True)
    w.walk(3.0)
    w.turn(0.0, 1.5)
    w.walk(3.0)
    caps = [(0.5, w.now() * 0.5, "THE FRACTAL DIMENSION", "Six families of rulesets - L-systems, IFS, escape-time sets, cellular automata, attractors and flows - each drawn from its ruleset."),
            (w.now() * 0.5, w.now() - 0.3, None, "197 known fractals are charted, so they can be found, or set aside, on every other line too.")]
    return w, args, caps


def filtered_walk(ctx, h):
    # OPTIMISE FILTERS first, into this run's own filters file; then the pages line walked in compact
    # mode, where only the survivors stand on the shelves.
    h.run(["--menu", "--press", "Item:optimise,Return", "--screenshot", os.path.join(h.work, "optimise.png")])
    args = ["--no-menu", "--warp", "it was the best of times"]
    w = Walker(*start(h, args), x=0.3, z=1.0, yaw=-0.25, pitch=-0.1)
    w.press("e", wait=1.0)
    w.wait(1.0)
    w.walk(3.0)
    w.turn(0.3, 2.0)
    w.walk(6.0)
    w.turn(-0.3, 2.5, walk=True)
    w.walk(3.0)
    caps = [(0.5, w.now() * 0.5, "COMPACT", "After OPTIMISE FILTERS, the pages line in compact mode: only what survives every filter stands on the shelves."),
            (w.now() * 0.5, w.now() - 0.3, None, "Nothing was searched for. The survivors are counted, numbered and walked in order.")]
    return w, args, caps


def filtered_image(ctx, h):
    h.run(["--menu", "--press", "Item:optimise,Return", "--screenshot", os.path.join(h.work, "optimise.png")])
    args = ["--no-menu", "--line", "image"]
    w = Walker(*start(h, args), x=0.3, z=1.0, yaw=-0.6, pitch=-0.12)
    w.wait(1.0)
    w.walk(3.0)
    w.turn(0.0, 2.0)
    w.walk(5.0)
    caps = [(0.5, w.now() - 0.3, "THE IMAGE LINE, FILTERED", "The image line after OPTIMISE FILTERS: of its 2^100 pictures of 10 x 10, only those every filter keeps (10^-18.26 of them) stand on the shelves.")]
    return w, args, caps


def model_turn(ctx, h):
    args = ["--no-menu", "--line", "models", "--warp", os.path.join(HERE, "cube.obj")]
    w = Walker(*start(h, args), x=0.0, z=1.0, yaw=0.0)
    w.wait(1.5)
    for k in ("d", "d", "d", "a"):
        for _ in range(int(1.2 * FPS)):
            w.frame([k])
        w.wait(0.3)
    w.press("r", wait=1.5)
    caps = [(0.3, w.now() - 0.3, "MODELS IN HAND", "A cube warped to its mesh's address, and turned in the hands: A and D turn it, R sets it upright.")]
    return w, args, caps


# ---------------------------------------------------------------- the running order

def chapter(n, title, line, sub):
    return Card("ch%d" % n, 5.0, title=title, kicker="CHAPTER %d" % n, lines=[sub], line=line)


def storyboard(ctx):
    k = ctx.measured
    return [
        # 0. cold open
        ("world", Film("open", cold_open)),
        ("world", Card("title", 7.0, title="POTENTIA : SIEVE", kicker="GALLERY OF BABEL DIMENSIONS",
                       lines=["Every possible page, picture, melody, film, model and file,", "each at exactly one address."])),
        # 1. why
        ("menus", chapter(1, "WHY", "pages", "What it is for, and why it is built this way")),
        ("menus", Card("why1", 8.0, title="Exact records", line="pages", lines=[
            "Exact, verifiable records of human work, and of how AI models are made.",
            "", "A Museum of Human Creations, each work identified by its bytes,",
            "and the Gallery of Babel around it: every possible work at one address,",
            "searched outwards from the real ones with filters."])),
        ("menus", Card("why2", 10.0, title="The shortest description", line="ai", lines=[
            "The aim: everything named by the shortest description that makes it -",
            "its Kolmogorov complexity.", "",
            "That number cannot be computed in general, and Sieve does not pretend to.",
            "It works towards it: every way of describing a file is weighed,",
            "and the shortest one found is kept."])),
        ("menus", Card("why3", 10.0, title="Honest about addresses", line="binary", lines=[
            "In a library of everything, the address is the content in another form:",
            "an address is as long as what it names.", "",
            "What shortens it is knowing what is worth keeping:",
            "filters that set noise aside, and orderings that put the likely first."])),
        ("menus", Card("why4", 11.0, title="What it is for", line="audio", lines=[
            "HISTORICAL PRESERVATION - every work kept exactly, checked to the last byte.", "",
            "HUMAN UNIFICATION - one canonical name for every work, the same for everyone",
            "who has it, with no registry to consult.", "",
            "LONG-TERM ARCHIVING - versioned formats, two decoders, nothing that needs a server:",
            "what is written today still reads tomorrow."])),
        ("menus", Card("why5", 7.0, title="Exact, or it refuses", line="pages", lines=[
            "Everything it does is exact and checked:",
            "it either gives you back precisely what went in, or refuses and says why."])),
        # 2. the address
        ("world", chapter(2, "THE ADDRESS", "pages", "Warp, take, order, go to")),
        ("world", Film("address", address_film)),
        ("world", Film("jumps", jumps)),
        ("world", Stills("address-stills", [
            ("take-cost", "COST", "The shortest way found to name the item in hand, beside every other way: positional, scrambled, guided, as a file."),
            ("take-help", "HELP", "Every key for the item in hand, on its own tab."),
            ("navigator", "THE ADDRESS NAVIGATOR", "Every digit of where you stand, editable; a bearing (Tab) turns the address into an angle round the line."),
        ])),
        # 3. the dimensions
        ("world", chapter(3, "WALKING THE DIMENSIONS", "image", "Twelve lines, one corridor, a door to each")),
        ("world", Film("tour", tour)),
        ("world", Film("tour-solid", tour_solid, line="image")),
        ("world", Film("fractal-walk", fractal_walk, line="fractals")),
        ("world", Stills("look", [
            ("pages-glow", "GEOMETRY EDGE GLOW", "Every edge drawn glowing, from Potentia Settings > Graphics."),
            ("models-off", "MODELS, SOLID", "The models line with wireframe off: green wire on clay."),
            ("fractals", "FRACTALS", "The fractal dimension: L-systems, IFS, escape-time sets, cellular automata and attractors, drawn on the shelves."),
        ], line="fractals")),
        # 4. holding things
        ("world", chapter(4, "HOLDING THINGS", "audio", "Take anything off a shelf and look closer")),
        ("world", Film("picture", picture, line="image")),
        ("world", Stills("hold", [
            ("books-take", "A BOOK", "A book: its cover is a picture of the image line, its title and pages are pages of the pages line. N and B turn the pages."),
            ("audio-take", "A MELODY", "A melody in hand: P plays it, F saves it as MIDI."),
            ("worlds-take", "A WORLD", "A world: four models placed and turned, its .obj a key away."),
            ("binary-take", "J: THE FILE BEHIND IT", "Every item is also a file: J goes to it on the binary line, and back."),
            ("fractals-take", "THE INDEX OF FRACTALS", "The first book of the fractal line: the charted fractals, each at its own address."),
            ("ai-chat", "CHAT", "On the AI line, Enter talks to the model in hand - run by Sieve itself, no other runtime."),
            ("ai-chain", "CHAIN", "Chains: models handing text to each other, step by step."),
        ], line="audio", secs=6.5)),
        ("world", Film("model-turn", model_turn, line="models")),
        # 5. filters
        ("menus", chapter(5, "FILTERS: COUNT, DON'T VISIT", "video", "Setting noise aside without looking at it")),
        ("menus", Stills("filters", [
            ("setup-menu", "SET UP THE LINES", "F1: every line's shape, and how much of it every filter keeps, counted exactly before you walk in."),
            ("setup-filters", "A LINE'S FILTERS", "Each filter an automaton: they merge, and their survivors are counted and numbered, never searched for."),
            ("setup-details", "WHAT THE FIGURES MEAN", "I on a line: its size, what is kept, and how long an address is under the filters."),
            ("setup-optimised", "OPTIMISE FILTERS", "One row ticks every filter that counts, keeps each stack within the filter memory, and turns on compact."),
            ("setup-filters-opt", "THE IMAGE LINE, OPTIMISED", "Including today's picture filters: border, specks and gradient."),
        ], line="video", secs=6.0)),
        ("menus", Card("kept", 14.0, table=True, title="What OPTIMISE FILTERS keeps", line="video", body_scale=3, lines=[
            "at the default shapes: the share of each line that survives", "",
            "BINARY   10^-14.59        IMAGE    10^-18.26        PAGES   10^-20.38",
            "BOOKS    10^-116.02       AUDIO    10^-13.63        TRACKS  10^-93.08",
            "VIDEO    10^-1.12         MOVIES   10^-43.05        MODELS  10^-21.48",
            "WORLDS   10^-124.49       AI       10^-9854.27", "",
            "Each figure exact: counted, not sampled.", "Source: docs/IDEAS.md and CI step 04."])),
        ("world", Film("filtered", filtered_walk)),
        ("world", Film("filtered-image", filtered_image, line="image")),
        ("menus", Stills("plugins", [
            ("designer", "THE FILTER DESIGNER", "Write a filter as rules; it is saved as a .sfilter plugin, compiled to an automaton and counted like the built-in ones."),
            ("main-menu", "VERSIONED", "A filter that changes becomes a new version beside the old (clean-v1, clean-v2), so every address written stays readable."),
        ], line="video", secs=6.5)),
        # 6. files
        ("menus", chapter(6, "FILES, EXACTLY", "binary", "The same ideas, pointed at real files")),
        ("menus", Stills("locator", [
            ("locator", "THE FILE LOCATOR", "Every file weighed every way Sieve knows - LZMA2, the rope, mixing, copies, patterns, formats opened and made again - and the smallest kept."),
        ], line="binary", secs=7.0)),
        ("menus", Card("cli", 15.0, table=True, title="Packed here, just now", line="binary", body_scale=3, lines=[
            "sieve locate help --installer help.sieve --compare", "",
            "original               %s bytes" % k.get("orig", "?"),
            "zip (deflate, 9)       %s bytes" % k.get("zip", "?"),
            "7z (LZMA2, 9e)         %s bytes" % k.get("7z", "?"),
            "Sieve instructions     %s bytes" % k.get("sieve", "?"), "",
            "sieve install help.sieve  ->  %s" % k.get("install", "?"),
            "one byte changed          ->  %s" % k.get("bad", "?")])),
        ("menus", Card("bench", 15.0, table=True, title="Measured", line="binary", body_scale=3, lines=[
            "                        original        7z            .sieve      against 7z",
            "enwik8 (Wikipedia)    100,000,000   26,080,445    21,431,896     -17.82%",
            "enwik9 (Wikipedia)  1,000,000,000  230,120,517   185,105,231     -19.56%",
            "Silesia corpus        211,938,580   48,455,266    41,710,177     -13.92%",
            "Sieve's own release    45,967,736    8,235,077     6,470,236     -21.43%",
            "Sieve's source tree    35,396,955    6,844,677     5,196,966     -24.07%", "",
            "Source: Proof/STATISTICS.md"])),
        ("menus", Card("checked", 11.0, title="Checked, twice", line="binary", lines=[
            "Every file installed is checked against its SHA-256 before anything is written.",
            "A second decoder, written from the specification alone, unpacks what Sieve makes,",
            "and CI checks the two agree on every version.", "",
            "Where it loses, said plainly: making a .sieve is slow, and files already",
            "compressed (JPEG, video) gain little."])),
        # 7. everything else
        ("menus", chapter(7, "EVERYTHING ELSE", "ai", "A tour of the tools")),
        ("menus", Stills("tools", [
            ("pause-menu", "ESC: PAUSE", "Every tool is a key away from the corridor."),
            ("graph", "O: THE NODE GRAPH", "Folders and files as a map of verified anchors, each a place on the binary line."),
            ("manual-nav", "MANUAL NAVIGATION", "Walk the binary line byte by byte: a file's bytes left, its place round the circle right, every step logged and undoable."),
            ("cipher", "THE ENCRYPTION ENGINE", "Recipes of layers that encrypt and decrypt, try to break weak ciphers, and count the recipes."),
            ("media", "THE MEDIA PLAYER", "The music behind every screen: melodies of the audio line chosen by filters - what you are hearing now."),
            ("rings", "THE RINGS", "Notes as rings of colour, one ring an octave."),
            ("creator", "THE CHARACTER CREATOR", "Characters of spheres, their actions animated, imported from glTF skeletons."),
            ("pworld", "YOUR PERSONAL DIMENSION", "A world of your own: a hill under the sky, made from a seed, kept as one small file."),
            ("pworld-dusk", "DAY AND NIGHT", "Time of day held or running, the stars out at dusk."),
            ("pworld-shaper", "THE GROUND SHAPER", "Raise, lower and flatten the ground; place favourites and your own models."),
            ("harness", "THE AI TRAINING HARNESS", "Models trained in integer arithmetic: trained again on any machine, the same weights to the bit."),
            ("replays", "REPLAYS", "Everything you do in the hallway, recorded as inputs and played back exactly. Every moving shot in this film is one."),
            ("system-viewer", "THE SYSTEM VIEWER", "Every format Sieve knows, as one web of doors leading up to information."),
            ("docs", "DOCUMENTATION", "The whole manual inside the program, searchable, every key named."),
            ("settings-graphics", "POTENTIA SETTINGS", "Graphics, controls, language, programs, the infinite RAID, game systems."),
            ("settings-controls", "CONTROLS", "Every key can be changed; menus can be read aloud in Sieve's own voice."),
        ], line="ai", secs=6.0)),
        # 8. dynamic, modular, procedural
        ("menus", chapter(8, "DYNAMIC, MODULAR, PROCEDURAL", "fractals", "Anyone could turn it into anything")),
        ("menus", Card("modular", 12.0, title="Everything is data", line="fractals", lines=[
            "Lines, filters, formats, languages, fonts and keys are data, not code:",
            "a .sfilter plugin is counted with no code of its own,",
            "and every format carries its version (sieve-fractal-v1 to v6 side by side).", "",
            "Nothing is stored that can be generated: every page, picture, melody",
            "and model is made from its address, on the spot.", "",
            "It needs no server. Anyone could turn it into anything."])),
        ("menus", Card("close", 10.0, title="Everything, at its shortest", line="pages", lines=[
            "Every work kept exactly. Every work named the same way by everyone.",
            "Every name as short as what we know lets it be.", "",
            "For preservation, for one shared record, for the long term."])),
        ("world", Card("end", 10.0, title="POTENTIA : SIEVE", kicker="GALLERY OF BABEL DIMENSIONS", line="pages", lines=[
            "by Edward James Gordon (Thor110)", "",
            "github.com/thor110/Potentia", "free software, AGPL v3"],
            foot="Every shot recorded from the programs. Music: melodies of the audio line.")),
    ]


# ---------------------------------------------------------------- measuring chapter 6

def measure_cli(ctx):
    """The help pages packed, installed and a damaged copy refused, as chapter 6 quotes them."""
    import re
    d = os.path.join(ctx.work, "cli")
    shutil.rmtree(d, ignore_errors=True)
    os.makedirs(d)
    shutil.copytree(os.path.join(SIEVE, "data", "help"), os.path.join(d, "help"))
    r = subprocess.run([ctx.sieve, "locate", "help", "--installer", "help.sieve", "--compare"], cwd=d,
                       capture_output=True, text=True)
    out = r.stdout + r.stderr
    k = {}
    for key, pat in (("orig", r"original\s+(\d+) bytes"), ("zip", r"zip \(deflate, level 9\)\s+(\d+)"),
                     ("7z", r"7z \(LZMA2, preset 9e\)\s+(\d+)"), ("sieve", r"Sieve instructions\s+(\d+)")):
        m = re.search(pat, out)
        k[key] = "{:,}".format(int(m.group(1))) if m else "?"
    r = subprocess.run([ctx.sieve, "install", "help.sieve", "--to", "back"], cwd=d, capture_output=True, text=True)
    out = r.stdout + r.stderr
    m = re.search(r"installed (\d+) files", out)
    k["install"] = "%s files, each checked by SHA-256" % m.group(1) if m else "?"
    b = bytearray(open(os.path.join(d, "help.sieve"), "rb").read())
    b[len(b) // 2] ^= 1
    open(os.path.join(d, "bad.sieve"), "wb").write(b)
    r = subprocess.run([ctx.sieve, "install", "bad.sieve", "--to", "bad"], cwd=d, capture_output=True, text=True)
    k["bad"] = "refused, nothing written" if r.returncode != 0 and not os.path.exists(os.path.join(d, "bad")) else "?"
    return k


class Ctx:
    pass


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--hallway", required=True)
    ap.add_argument("--sieve", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--only", default="")
    ap.add_argument("--skip-music", action="store_true")
    a = ap.parse_args()
    ctx = Ctx()
    ctx.hallway, ctx.sieve = os.path.abspath(a.hallway), os.path.abspath(a.sieve)
    ctx.work = os.path.abspath(os.path.join(a.out, "work"))
    os.makedirs(ctx.work, exist_ok=True)
    ctx.measured = measure_cli(ctx)
    # Sieve's own icon, the picture warped onto the image line in chapter 4
    from PIL import Image
    Image.open(os.path.join(SIEVE, "data", "icons", "sieve.ico")).convert("RGB").save(ICON)
    board = storyboard(ctx)
    only = set(filter(None, a.only.split(",")))
    segs, sections = [], []
    for mode, seg in board:
        out = os.path.join(ctx.work, "seg-%s.mp4" % seg.name)
        if not os.path.exists(out) or seg.name in only or not only:
            print("making", seg.name, flush=True)
            seg.make(ctx, out)
        segs.append(out)
        sections.append((mode, duration(out)))
    silent = os.path.join(ctx.work, "trailer-silent.mp4")
    concat(segs, silent)
    final = os.path.join(a.out, "potentia-sieve-trailer.mp4")
    if a.skip_music:
        shutil.copy(silent, final)
    else:
        # sections of one mode in a row are one stretch of music
        merged = []
        for mode, secs in sections:
            if merged and merged[-1][0] == mode:
                merged[-1] = (mode, merged[-1][1] + secs)
            else:
                merged.append((mode, secs))
        wav = os.path.join(ctx.work, "music.wav")
        chosen, counts = music.soundtrack(wav, merged, ctx.sieve, SIEVE, ctx.work)
        with open(os.path.join(a.out, "music.txt"), "w") as f:
            for mode, c in counts.items():
                f.write("%s: %d survivors\n" % (mode, c))
            for mode, rank, notation in chosen:
                f.write("%s %d: %s\n" % (mode, rank, notation))
        ff("-i", silent, "-i", wav, "-c:v", "copy", "-c:a", "aac", "-b:a", "192k", "-shortest", final)
    total = duration(final)
    print("trailer: %s, %d:%02d" % (final, int(total) // 60, int(total) % 60))


if __name__ == "__main__":
    main()
