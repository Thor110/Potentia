#!/usr/bin/env python3
"""docs/SETTINGS.md written from the code (IDEAS 19.134, stage 8): every setting the hallway has,
its default and what it is, as `hallway --write-settings` writes them (app_settings.cpp's own table,
so the document cannot say other than the code). A section met twice in the file is one section here,
its settings in the order they come.

    python tools/settings_doc.py HALLWAY            write docs/SETTINGS.md
    python tools/settings_doc.py HALLWAY --check    fail, saying where, if docs/SETTINGS.md differs
"""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
DOC = os.path.join(HERE, "..", "docs", "SETTINGS.md")


def settings(hallway):
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "defaults.ini")
        env = dict(os.environ, SDL_VIDEO_DRIVER="offscreen")
        subprocess.run([hallway, "--write-settings", out], check=True, capture_output=True, env=env)
        with open(out, encoding="utf-8") as f:
            text = f.read()
    sections, order, current = {}, [], None
    for line in text.splitlines():
        s = line.strip()
        if not s or s.startswith(";"):
            continue
        if s.startswith("[") and s.endswith("]"):
            current = s[1:-1]
            if current not in sections:
                sections[current] = []
                order.append(current)
            continue
        body, _, comment = line.partition(" ;")
        key, _, value = body.partition("=")
        sections[current].append((key.strip(), value.strip(), comment.strip()))
    return order, sections


def document(order, sections):
    t = ["# Settings", "",
         "Every setting in `sieve-hallway.ini`, its default and what it is. **Written from the code** by",
         "`tools/settings_doc.py` (from `hallway --write-settings`, so from the settings' own table in",
         "`client/app_settings.cpp`); CI step 10 fails if this file differs from what the code writes. Do not",
         "edit it by hand: change the code, then run `python tools/settings_doc.py build/hallway`.", "",
         "Most are also on the main menu's Settings and the setup menu; the hallway's options win over the",
         "file for the run they are given in. An empty default is written empty.", ""]
    for name in order:
        t += [f"## [{name}]", "", "| Setting | Default | What it is |", "| :--- | :--- | :--- |"]
        for key, value, comment in sections[name]:
            esc = lambda x: x.replace("|", "\\|")
            t.append(f"| `{key}` | {('`' + esc(value) + '`') if value else '(empty)'} | {esc(comment)} |")
        t.append("")
    return "\n".join(t)


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    text = document(*settings(sys.argv[1]))
    if "--check" in sys.argv[2:]:
        with open(DOC, encoding="utf-8") as f:
            have = f.read()
        if have != text:
            a, b = have.splitlines(), text.splitlines()
            for i in range(max(len(a), len(b))):
                x = a[i] if i < len(a) else "(nothing)"
                y = b[i] if i < len(b) else "(nothing)"
                if x != y:
                    sys.exit(f"docs/SETTINGS.md differs from the code at line {i + 1}:\n  the file: {x}\n  the code: {y}\n"
                             "run: python tools/settings_doc.py HALLWAY")
        print(f"settings: docs/SETTINGS.md matches the code ({sum(len(v) for v in sections_count(text))} settings)")
        return
    with open(DOC, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    print("wrote docs/SETTINGS.md")


def sections_count(text):
    return [[l for l in text.splitlines() if l.startswith("| `")]]


if __name__ == "__main__":
    main()
