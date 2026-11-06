#!/usr/bin/env python3
"""Fetches the OLAP simulation's datasets again and records exactly what was fetched.

    python3 Pages/space/tools/update_data.py           # fetch, write snapshots and the manifest
    python3 Pages/space/tools/update_data.py --check   # fetch and compare; change nothing

Two kinds of dataset (see DATASETS below, and datasets.js, which the page reads):

  snapshot  kept in this repository (data/<id>.js), because the source allows it and the page
            cannot fetch it from a browser. The page checks its SHA-256 against the manifest.
  image     a surface map, kept here as a data: URI (data/<id>.js), loaded when its body is shown.
            The SHA-256 is of the data: URI text.
  live      read by the page straight from VizieR, because the authors ask that their files not
            be passed on (2MRS) or so the page always shows the archive's current version. Only
            the SHA-256 of the rows and their count are kept here, so the page can say whether
            what it read is exactly what was recorded.

The hash of a VizieR table is taken over its canonical text: the column-name line and the data
lines, each without trailing CR, joined with LF and ending in LF (comments, the units line and the
dashes line are left out). datasets.js does the same in the browser.

Standard library only. Python 3.8 or later.
"""
import base64
import datetime
import hashlib
import json
import os
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "..", "data")

VIZIER_MIRRORS = ["https://vizier.cds.unistra.fr", "https://vizier.cfa.harvard.edu"]

DATASETS = [
    {
        "id": "2mrs",
        "kind": "live",
        "vizier": "/viz-bin/asu-tsv?-source=J/ApJS/199/26/table3&-out.max=unlimited&-out=ID,GLON,GLAT,Kcmag,cz",
    },
    {
        "id": "localgroup",
        "kind": "live",
        "vizier": "/viz-bin/asu-tsv?-source=J/AJ/144/4/catalog&-out.max=unlimited&-out=Name,SubG,MType,GLON,GLAT,D,HRV,VMag",
    },
    {
        "id": "exoplanets",
        "kind": "snapshot",
        "url": "https://exoplanetarchive.ipac.caltech.edu/TAP/sync?query="
               "select+pl_name,hostname,sy_dist,glon,glat,pl_rade,pl_bmasse,pl_insol,pl_eqt,disc_year"
               "+from+pscomppars+where+sy_dist+is+not+null+order+by+sy_dist,pl_name&format=csv",
    },
    {
        "id": "land",
        "kind": "snapshot",
        "url": "https://raw.githubusercontent.com/nvkelso/natural-earth-vector/v5.1.2/geojson/ne_110m_land.geojson",
    },
    # Surface maps, equirectangular, longitude -180 to 180 from left to right, north at the top.
    {
        "id": "skin-moon",
        "kind": "image",
        # NASA Scientific Visualization Studio, CGI Moon Kit: LRO LROC colour mosaic (public domain)
        "url": "https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/lroc_color_poles_1k.jpg",
    },
    {
        "id": "skin-mars",
        "kind": "image",
        # USGS Viking MDIM 2.1 colourised global mosaic (public domain), via Wikimedia Commons
        "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/1/1f/Mars_Viking_MDIM21_ClrMosaic_1km.jpg/960px-Mars_Viking_MDIM21_ClrMosaic_1km.jpg",
    },
    {
        "id": "skin-ceres",
        "kind": "image",
        # NASA/JPL-Caltech/UCLA/MPS/DLR/IDA, Dawn, PIA19606: global map coloured by height (public domain)
        "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/6/67/PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg/960px-PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg",
    },
]


def fetch_bytes(url):
    # Wikimedia asks for a descriptive User-Agent and limits how fast it is asked: wait and retry.
    req = urllib.request.Request(url, headers={"User-Agent": "potentia-olap-data-updater (https://github.com/Thor110/Potentia)"})
    for attempt in range(4):
        try:
            with urllib.request.urlopen(req, timeout=300) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            if e.code != 429 or attempt == 3:
                raise
            time.sleep(90)


def fetch(url):
    return fetch_bytes(url).decode("utf-8")


def vizier_canonical(text):
    lines = [ln.rstrip("\r") for ln in text.split("\n")]
    lines = [ln for ln in lines if ln.strip() and not ln.startswith("#")]
    # lines[0] names, lines[1] units, lines[2] dashes, then the rows
    header, rows = lines[0], lines[3:]
    return header + "\n" + "".join(r + "\n" for r in rows), len(rows)


def sha256(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def main():
    check = "--check" in sys.argv
    manifest_path = os.path.join(DATA, "manifest.js")
    old = {}
    if os.path.exists(manifest_path):
        src = open(manifest_path, encoding="utf-8").read()
        start = src.index("{", src.index(".manifest ="))
        old = json.loads(src[start: src.rindex("}") + 1])["datasets"]
    today = datetime.date.today().isoformat()
    out = {}
    changed = False
    for d in DATASETS:
        if d["kind"] == "live":
            last = None
            for m in VIZIER_MIRRORS:
                try:
                    text = fetch(m + d["vizier"])
                    break
                except Exception as e:  # try the next mirror
                    last = e
            else:
                raise SystemExit(f"{d['id']}: no VizieR mirror answered ({last})")
            canon, rows = vizier_canonical(text)
            entry = {"rows": rows, "sha256": sha256(canon), "via": m}
        elif d["kind"] == "image":
            raw = fetch_bytes(d["url"])
            kind = "image/png" if raw[:4] == b"\x89PNG" else "image/jpeg"
            text = "data:" + kind + ";base64," + base64.b64encode(raw).decode("ascii")
            entry = {"rows": len(raw), "sha256": sha256(text), "bytes": len(raw), "source_sha256": hashlib.sha256(raw).hexdigest()}
            if not check:
                with open(os.path.join(DATA, d["id"] + ".js"), "w", encoding="utf-8", newline="\n") as f:
                    f.write("// Written by tools/update_data.py; do not edit. Source and checks: data/manifest.js\n")
                    f.write("(window.OLAP = window.OLAP || {}).snap = window.OLAP.snap || {};\n")
                    f.write(f"window.OLAP.snap[{json.dumps(d['id'])}] = {json.dumps(text)};\n")
        else:
            text = fetch(d["url"])
            rows = text.count("\n") - 1 if d["id"] != "land" else len(json.loads(text)["features"])
            entry = {"rows": rows, "sha256": sha256(text), "bytes": len(text.encode("utf-8"))}
            if not check:
                with open(os.path.join(DATA, d["id"] + ".js"), "w", encoding="utf-8", newline="\n") as f:
                    f.write("// Written by tools/update_data.py; do not edit. Source and checks: data/manifest.js\n")
                    f.write("(window.OLAP = window.OLAP || {}).snap = window.OLAP.snap || {};\n")
                    f.write(f"window.OLAP.snap[{json.dumps(d['id'])}] = {json.dumps(text)};\n")
        prev = old.get(d["id"], {})
        same = prev.get("sha256") == entry["sha256"]
        entry["retrieved"] = prev.get("retrieved", today) if same else today
        changed |= not same
        print(f"{d['id']:<11} {entry['rows']:>6} rows  sha256 {entry['sha256'][:16]}…  {'unchanged' if same else 'CHANGED'}")
        out[d["id"]] = entry
    if check:
        sys.exit(1 if changed else 0)
    with open(manifest_path, "w", encoding="utf-8", newline="\n") as f:
        f.write("// Written by tools/update_data.py; do not edit by hand.\n")
        f.write("// What each dataset was when last fetched: row count, SHA-256, and the date it last changed.\n")
        f.write("(window.OLAP = window.OLAP || {}).manifest = ")
        f.write(json.dumps({"datasets": out}, indent=2, ensure_ascii=False))
        f.write(";\n")


if __name__ == "__main__":
    main()
