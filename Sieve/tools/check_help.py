#!/usr/bin/env python3
"""Checks that the programs' own help names every option they read (the CI runs it).

  - sieve: every option a command reads (a.has("x"), a.get("x"), ...) is named on that command's
    page of `sieve help` (tools/cli/help.cpp), or among the options every command takes;
  - hallway: every option it reads is in its usage text (client/app_main.cpp, kUsage);
  - every command `sieve` runs has a help page.

  python tools/check_help.py        (from the Sieve folder)

It reads the sources, not the programs, so it needs no build. Exit 1, listing what is missing.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
READS = re.compile(r'\b(?:a|args)\s*\.\s*(?:has|get|get_u32|get_positive|require_positive)\s*\(\s*"([a-z0-9-]+)"')
NAMED = re.compile(r"--([a-z0-9][a-z0-9-]*)")
# Read by every command (sieve_cli.cpp's main), and said once in `sieve help`'s overview.
EVERYWHERE = {"help", "timings", "filter-memory", "merge-cache", "unit-time", "ffmpeg"}
# A command's own name for the help page it is on, where the function's name differs.
PAGE_OF = {"sieve": "sift"}
# Functions that read options for a command other than the one their name says.
HELPERS = {"cmd_check_binary": "check", "cmd_check_book": "check", "cmd_filters_binary": "filters",
           "cmd_filters_books": "filters", "cmd_filters_plugin": "filters", "cmd_filters_models": "filters",
           "cmd_filters_ai": "filters"}
# Files whose options belong to one command's page.
FILES = {"diffuse.cpp": "diffuse", "ai.cpp": ("ai", "filters"),  # filters --line ai is in ai.cpp
         "train_cmd.cpp": "ai-train", "rope.cpp": "rope",
         "tensors.cpp": "tensors", "cipher_cmd.cpp": "cipher", "chat.cpp": "chat", "site.cpp": "site"}


def text(path):
    return (ROOT / path).read_text(encoding="utf-8", errors="replace")


def help_pages():
    h = text("tools/cli/help.cpp")
    starts = [(m.start(), m.group(1)) for m in re.finditer(r'\n        \{"([a-z-]+)", "', h)]
    # The options pages share, defined once (const Option kShort = {...};), by name.
    consts = {m.group(1): set(NAMED.findall(m.group(2)))
              for m in re.finditer(r"const (?:Option|std::string) (k[A-Za-z]+)\s*=\s*(.*?);\n", h, re.S)}
    pages = {}
    for i, (pos, name) in enumerate(starts):
        end = starts[i + 1][0] if i + 1 < len(starts) else len(h)
        block = h[pos:end]
        pages[name] = set(NAMED.findall(block)).union(*(consts[k] for k in set(re.findall(r"\b(k[A-Z][A-Za-z]+)\b", block)) if k in consts))
    # The line options and --key, which every line's page shows (kLine, kLineOptions, kKey).
    shared = set(NAMED.findall(h[h.index("const Option kLine"):h.index("const Option kShort")]))
    return pages, shared | EVERYWHERE | {"line", "key", "model", "filters"}


def main():
    pages, shared = help_pages()
    missing = []
    cli = text("tools/sieve_cli.cpp")
    # Every function defined at the top level: a cmd_ function's options belong to its command's
    # page; a helper's (used by several commands) to some page.
    funcs = [(m.start(), m.group(1)) for m in re.finditer(r"\n[A-Za-z_][^\n;(]*?\b([a-z_][a-z_0-9]*)\s*\([^;{]*\)\s*(?:const\s*)?\n\{", cli)]
    anywhere = set().union(*pages.values())
    for i, (pos, fn) in enumerate(funcs):
        end = funcs[i + 1][0] if i + 1 < len(funcs) else len(cli)
        keys = set(READS.findall(cli[pos:end]))
        if not keys:
            continue
        if not fn.startswith("cmd_"):
            for k in sorted(keys - anywhere - shared):
                missing.append(f"--{k} is read (tools/sieve_cli.cpp, {fn}) but on no help page")
            continue
        page = HELPERS.get(fn) or fn[4:].replace("_composed", "").replace("_", "-")
        page = PAGE_OF.get(page, page)
        if page not in pages:
            missing.append(f"sieve {page} ({fn}) has no help page")
            continue
        for k in sorted(keys - pages[page] - shared):
            missing.append(f"sieve {page}: --{k} is read (tools/sieve_cli.cpp, {fn}) but not on its help page")
    for name, page in FILES.items():
        keys = set(READS.findall(text("tools/cli/" + name)))
        names = page if isinstance(page, tuple) else (page,)
        page = names[0]
        for k in sorted(keys - set().union(*(pages.get(n, set()) for n in names)) - shared):
            missing.append(f"sieve {page}: --{k} is read (tools/cli/{name}) but not on its help page")
    for cmd in sorted(set(re.findall(r'a\.command == "([a-z-]+)"', cli))):
        if PAGE_OF.get(cmd, cmd) not in pages:
            missing.append(f"sieve {cmd} has no help page")

    app = text("client/app_main.cpp")
    u0 = app.index("const char* kUsage =")
    usage = set(NAMED.findall(app[u0:app.index('";\n', u0)]))
    read = set()
    for f in sorted((ROOT / "client").glob("*.cpp")):
        read |= set(READS.findall(f.read_text(encoding="utf-8", errors="replace")))
    for k in sorted(read - usage - EVERYWHERE - {"help"}):
        missing.append(f"hallway: --{k} is read (client/) but not in its usage text (client/app_main.cpp)")

    for m in missing:
        print(m)
    print(f"help: {sum(len(v) for v in pages.values())} options named on {len(pages)} pages; hallway: {len(usage)}; "
          f"{len(missing)} missing")
    return 1 if missing else 0


if __name__ == "__main__":
    sys.exit(main())
