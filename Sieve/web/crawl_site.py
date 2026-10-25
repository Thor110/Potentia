#!/usr/bin/env python3
r"""Checks a site on its server against its table (web/README.md, IDEAS §19.92): run it now and then,
or on a schedule, so that a page, a script or the table changed on the server is known at once.

  python web/crawl_site.py https://example.com/ --table SHA256 [--sieve PATH] [--served-at PATH] [--every MINUTES]

1. Fetches the site's table (index.sieve, at the address given) and compares its SHA-256 with the
   one you kept (`sieve site build` prints it): a different table is reported, and nothing else is
   trusted then.
2. Fetches every file the table names, into a folder of its own, each as the server sends it.
3. Runs `sieve site check` on that copy: every file against the table, and every hash a page states
   (its links to .sieve files, its scripts' and stylesheets' integrity) against the file it names.

Exit 0 when all of it matches, 1 when anything does not (each mismatch printed), 2 when the site
could not be fetched. With --every, it checks again every MINUTES until stopped, printing a line
each time. Pages a server writes as it serves them (PHP and the like) are files the table cannot
name by a fixed hash: they show as not matching, and are a question for later (HANDOFF, step 4).
"""

import argparse
import datetime
import hashlib
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.parse
import urllib.request


def fetch(url):
    request = urllib.request.Request(url, headers={"User-Agent": "sieve-crawl-site", "Cache-Control": "no-cache"})
    with urllib.request.urlopen(request, timeout=60) as response:
        return response.read()


def table_files(sieve, table_path):
    """The files the table names: (path, sha256), from `sieve map`."""
    out = subprocess.run([sieve, "map", table_path], capture_output=True, text=True, encoding="utf-8", errors="replace")
    if out.returncode != 0:
        raise RuntimeError("sieve could not read the table: " + (out.stderr or out.stdout).strip())
    files = []
    for line in out.stdout.splitlines():
        f = line.split("\t")
        if len(f) >= 6 and f[0] == "n" and f[2] == "file":
            files.append((f[5], f[4]))
    return files


def check_once(a):
    site = a.url if a.url.endswith("/") else a.url + "/"
    stamp = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    work = tempfile.mkdtemp(prefix="sieve-crawl-")
    try:
        try:
            table = fetch(site + "index.sieve")
        except Exception as e:
            print(f"{stamp}  could not fetch {site}index.sieve: {e}")
            return 2
        got = hashlib.sha256(table).hexdigest()
        if got != a.table.lower():
            print(f"{stamp}  MISMATCH  the site's table: its SHA-256 is {got}, not the {a.table.lower()} kept for it")
            return 1
        with open(os.path.join(work, "index.sieve"), "wb") as f:
            f.write(table)
        files = table_files(a.sieve, os.path.join(work, "index.sieve"))
        for path, _ in files:
            url = site + urllib.parse.quote(path)
            try:
                data = fetch(url)
            except Exception as e:
                print(f"{stamp}  MISMATCH  {path}: could not be fetched ({e})")
                return 1
            dest = os.path.join(work, *path.split("/"))
            os.makedirs(os.path.dirname(dest), exist_ok=True)
            with open(dest, "wb") as f:
                f.write(data)
        cmd = [a.sieve, "site", "check", work] + (["--served-at", a.served_at] if a.served_at else [])
        out = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8", errors="replace")
        for line in out.stdout.splitlines():
            print(f"{stamp}  MISMATCH  {line.replace('mismatch  ', '', 1)}")
        summary = (out.stderr.strip().splitlines() or ["?"])[-1].replace("site      ", "")
        print(f"{stamp}  {site}: table as kept, {len(files)} files fetched; {summary}")
        return 0 if out.returncode == 0 else 1
    finally:
        shutil.rmtree(work, ignore_errors=True)


def main():
    ap = argparse.ArgumentParser(description="Check a site on its server against its table (index.sieve).")
    ap.add_argument("url", help="the site's address (the folder its index.sieve is in)")
    ap.add_argument("--table", required=True, help="the table's SHA-256, as sieve site build printed it")
    ap.add_argument("--sieve", default="sieve", help="the sieve program (default: sieve on the PATH)")
    ap.add_argument("--served-at", default="", help="as given to sieve site build, for links from the server's root")
    ap.add_argument("--every", type=float, default=0, help="check again every MINUTES, until stopped")
    a = ap.parse_args()
    if not a.every:
        return check_once(a)
    while True:
        check_once(a)
        time.sleep(a.every * 60)


if __name__ == "__main__":
    sys.exit(main())
