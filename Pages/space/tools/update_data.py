#!/usr/bin/env python3
"""Fetches the OLAP simulation's datasets again and records exactly what was fetched.

    python3 Pages/space/tools/update_data.py            # fetch, check, then write snapshots and the manifest
    python3 Pages/space/tools/update_data.py --check    # fetch and compare with the manifest; change nothing
    python3 Pages/space/tools/update_data.py --verify   # offline: do the snapshot files match the manifest?

Exit codes: 0 everything matches (or was written); 1 something changed (--check) or a snapshot file
differs from the manifest (--verify); 2 an error: a source could not be reached, or what it sent
failed the checks below. Nothing is written unless every dataset was fetched and passed.

Three kinds of dataset (see DATASETS below, and datasets.js, which the page reads):

  live      read by the page straight from VizieR, because the authors ask that their files not
            be passed on (2MRS) or so the page always shows the archive's current version. Only
            the SHA-256 of the rows, their count and the query are kept here, so the page can say
            whether what it read is exactly what was recorded.
  snapshot  kept in this repository (data/<id>.js), because the source allows it and the page
            cannot fetch it from a browser. Text is stored with LF line ends. The page checks its
            SHA-256 against the manifest.
  image     a surface map, kept here as a data: URI (data/<id>.js), loaded when its body is shown.
            The SHA-256 is of the data: URI text; source_sha256 is of the image file itself.

The hash of a VizieR table is taken over its canonical text: the column-name line and the data
lines, each with trailing CRs removed, joined with LF and ending in LF. Comment lines (#), lines
holding only spaces and tabs, the units line and the dashes line are left out. A leading BOM is
dropped. datasets.js does exactly the same in the browser.

Checks on what is fetched: a VizieR table must have exactly the columns asked for (VizieR quietly
leaves out a column it does not know); every dataset must have at least min_rows rows; an image
must really be a JPEG or PNG of the expected size; a GeoJSON must have features.

Wikimedia limits how fast it is asked, so an image may take a few minutes when it says "wait".

Standard library only. Python 3.8 or later.
"""
import base64
import csv
import datetime
import hashlib
import io
import json
import os
import re
import struct
import sys
import time
import urllib.error
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
DATA = os.path.join(HERE, "..", "data")
MANIFEST = os.path.join(DATA, "manifest.js")

# datasets.js has the same list, in the same order: the page tries them in turn.
VIZIER_MIRRORS = ["https://vizier.cds.unistra.fr", "https://vizier.cfa.harvard.edu"]
USER_AGENT = "potentia-olap-data-updater (https://github.com/Thor110/Potentia)"


def vizier_path(source, columns):
    return "/viz-bin/asu-tsv?-source=" + source + "&-out.max=unlimited&-out=" + ",".join(columns)


DATASETS = [
    {
        "id": "2mrs", "kind": "live", "format": "vizier", "min_rows": 40000,
        "source": "J/ApJS/199/26/table3", "columns": ["ID", "GLON", "GLAT", "Kcmag", "cz"],
    },
    {
        "id": "localgroup", "kind": "live", "format": "vizier", "min_rows": 80,
        "source": "J/AJ/144/4/catalog", "columns": ["Name", "SubG", "MType", "GLON", "GLAT", "D", "HRV", "VMag"],
    },
    {
        "id": "exoplanets", "kind": "snapshot", "format": "csv", "min_rows": 5000,
        "columns": ["pl_name", "hostname", "sy_dist", "glon", "glat", "pl_rade", "pl_bmasse", "pl_insol", "pl_eqt", "disc_year"],
        "url": "https://exoplanetarchive.ipac.caltech.edu/TAP/sync?query="
               "select+pl_name,hostname,sy_dist,glon,glat,pl_rade,pl_bmasse,pl_insol,pl_eqt,disc_year"
               "+from+pscomppars+where+sy_dist+is+not+null+order+by+sy_dist,pl_name&format=csv",
    },
    {
        "id": "land", "kind": "snapshot", "format": "geojson", "min_rows": 100,
        "url": "https://raw.githubusercontent.com/nvkelso/natural-earth-vector/v5.1.2/geojson/ne_110m_land.geojson",
    },
    # Surface maps, equirectangular, north at the top. Where each map sits in its image, and the
    # longitude at its left edge, are in datasets.js ("frame"); re-measure the frame if a map's
    # size changes (the updater refuses a map whose size differs from "size").
    {
        "id": "skin-moon", "kind": "image", "format": "image", "size": [1024, 512],
        # NASA Scientific Visualization Studio, CGI Moon Kit: LRO LROC colour mosaic (public domain)
        "url": "https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/lroc_color_poles_1k.jpg",
    },
    {
        "id": "skin-mars", "kind": "image", "format": "image", "size": [960, 480],
        # USGS Viking MDIM 2.1 colourised global mosaic (public domain), via Wikimedia Commons
        "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/1/1f/Mars_Viking_MDIM21_ClrMosaic_1km.jpg/960px-Mars_Viking_MDIM21_ClrMosaic_1km.jpg",
    },
    {
        "id": "skin-ceres", "kind": "image", "format": "image", "size": [960, 489],
        # NASA/JPL-Caltech/UCLA/MPS/DLR/IDA, Dawn, PIA19606: global map coloured by height (public domain)
        "url": "https://upload.wikimedia.org/wikipedia/commons/thumb/6/67/PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg/960px-PIA19606-Ceres-Dawn-GlobalMap-Unannotated-20150728.jpg",
    },
]


class Bad(Exception):
    """What a source sent failed a check."""


def fetch_bytes(url, waits=(15, 45, 90)):
    """Fetch with retries on rate limits, server errors and network faults (waits 15, 45, 90 s,
    or as long as the server asks, up to 5 minutes)."""
    req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
    for attempt in range(len(waits) + 1):
        try:
            with urllib.request.urlopen(req, timeout=60) as r:
                return r.read()
        except urllib.error.HTTPError as e:
            if (e.code != 429 and e.code < 500) or attempt == len(waits):
                raise
            after = e.headers.get("Retry-After", "")
            wait = min(300, int(after)) if after.isdigit() else waits[attempt]
        except (urllib.error.URLError, TimeoutError, ConnectionError):
            if attempt == len(waits):
                raise
            wait = waits[attempt]
        print(f"  waiting {wait} s before trying {url[:60]}… again", file=sys.stderr)
        time.sleep(wait)


def decode(raw):
    return raw.decode("utf-8-sig")   # a leading BOM is dropped, as browsers do


def vizier_canonical(text):
    lines = [re.sub(r"\r+$", "", ln) for ln in text.split("\n")]
    lines = [ln for ln in lines if not re.fullmatch(r"[ \t]*", ln) and not ln.startswith("#")]
    if len(lines) < 3:
        raise Bad("VizieR sent no table (an error, or an unknown catalogue)")
    # lines[0] names, lines[1] units, lines[2] dashes, then the rows
    header, rows = lines[0], lines[3:]
    return header, header + "\n" + "".join(r + "\n" for r in rows), len(rows)


def sha256(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def image_size(raw):
    if raw[:8] == b"\x89PNG\r\n\x1a\n":
        w, h = struct.unpack(">II", raw[16:24])
        return "image/png", w, h
    if raw[:2] == b"\xff\xd8":
        i = 2
        while i < len(raw) - 9:
            if raw[i] != 0xFF:
                i += 1
                continue
            marker = raw[i + 1]
            if marker in (0xC0, 0xC1, 0xC2, 0xC3, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
                h, w = struct.unpack(">HH", raw[i + 5:i + 9])
                return "image/jpeg", w, h
            i += 2 + struct.unpack(">H", raw[i + 2:i + 4])[0]
    raise Bad("not a JPEG or PNG (perhaps an error page)")


def snapshot_js(id_, text):
    return ("// Written by tools/update_data.py; do not edit. Source and checks: data/manifest.js\n"
            "(window.OLAP = window.OLAP || {}).snap = window.OLAP.snap || {};\n"
            f"window.OLAP.snap[{json.dumps(id_)}] = {json.dumps(text)};\n")


def read_snapshot(id_):
    """The text a snapshot file holds (the inverse of snapshot_js)."""
    src = open(os.path.join(DATA, id_ + ".js"), encoding="utf-8").read()
    start = src.index("] = ", src.index("window.OLAP.snap[")) + 4
    return json.loads(src[start: src.rindex(";")])


def read_manifest():
    if not os.path.exists(MANIFEST):
        return {}
    src = open(MANIFEST, encoding="utf-8").read()
    start = src.index("{", src.index(".manifest ="))
    return json.loads(src[start: src.rindex("}") + 1])["datasets"]


def write_atomic(path, text):
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8", newline="\n") as f:
        f.write(text)
    os.replace(tmp, path)


def get(d):
    """Fetch and check one dataset. Returns (manifest entry, snapshot text or None)."""
    if d["kind"] == "live":
        path, last = vizier_path(d["source"], d["columns"]), None
        for m in VIZIER_MIRRORS:
            try:   # one short retry per mirror, then the next mirror
                header, canon, rows = vizier_canonical(decode(fetch_bytes(m + path, waits=(15,))))
                if header.split("\t") != d["columns"]:
                    raise Bad(f"columns are {header.split(chr(9))}, not {d['columns']}")
                break
            except (OSError, UnicodeDecodeError, Bad) as e:
                print(f"  {m}: {e}", file=sys.stderr)
                last = e
        else:
            raise Bad(f"no VizieR mirror gave the table ({last})")
        entry = {"rows": rows, "sha256": sha256(canon), "via": m, "query": path}
        snap = None
    elif d["kind"] == "image":
        raw = fetch_bytes(d["url"])
        kind, w, h = image_size(raw)
        if [w, h] != d["size"]:
            raise Bad(f"the map is {w}x{h}, not {d['size'][0]}x{d['size'][1]}: re-measure its frame in datasets.js, then update 'size'")
        snap = "data:" + kind + ";base64," + base64.b64encode(raw).decode("ascii")
        entry = {"bytes": len(raw), "size": [w, h], "sha256": sha256(snap), "source_sha256": hashlib.sha256(raw).hexdigest(), "url": d["url"]}
        rows = None
    else:
        snap = decode(fetch_bytes(d["url"])).replace("\r\n", "\n")
        if d["format"] == "csv":
            records = list(csv.reader(io.StringIO(snap)))
            if not records or records[0] != d["columns"]:
                raise Bad(f"columns are {records[0] if records else None}, not {d['columns']}")
            rows = len(records) - 1
        else:
            rows = len(json.loads(snap).get("features", []))
        entry = {"rows": rows, "sha256": sha256(snap), "bytes": len(snap.encode("utf-8")), "url": d["url"]}
    if rows is not None and rows < d.get("min_rows", 1):
        raise Bad(f"only {rows} rows (expected at least {d['min_rows']})")
    entry["format"] = d["format"]
    return entry, snap


def verify():
    """Offline: does every snapshot file hold exactly what the manifest records?"""
    man, bad = read_manifest(), 0
    for d in DATASETS:
        if d["kind"] == "live":
            continue
        try:
            ok = sha256(read_snapshot(d["id"])) == man.get(d["id"], {}).get("sha256")
        except (OSError, ValueError) as e:
            ok = False
            print(f"{d['id']:<11} cannot read: {e}")
        print(f"{d['id']:<11} {'matches the manifest' if ok else 'DIFFERS from the manifest'}")
        bad += not ok
    return 1 if bad else 0


def main():
    args = sys.argv[1:]
    unknown = [a for a in args if a not in ("--check", "--verify")]
    if unknown:
        raise SystemExit(f"unknown option {unknown[0]} (use --check, --verify, or nothing)")
    if "--verify" in args:
        sys.exit(verify())
    check = "--check" in args
    old, today = read_manifest(), datetime.date.today().isoformat()
    out, snaps, changed = {}, {}, False
    # Fetch and check everything first: nothing is written unless all of it passed.
    for d in DATASETS:
        try:
            entry, snap = get(d)
        except (Bad, OSError, ValueError) as e:
            print(f"{d['id']}: {e}", file=sys.stderr)
            print("Nothing was written.", file=sys.stderr)
            sys.exit(2)
        prev = old.get(d["id"], {})
        same = prev.get("sha256") == entry["sha256"]
        entry["retrieved"] = prev.get("retrieved", today) if same else today
        changed |= not same
        count = f"{entry['rows']:>6} rows " if "rows" in entry else f"{entry['bytes']:>6} bytes"
        print(f"{d['id']:<11} {count} sha256 {entry['sha256'][:16]}…  {'unchanged' if same else 'CHANGED'}")
        out[d["id"]] = entry
        if snap is not None:
            snaps[d["id"]] = snap
    if check:
        sys.exit(1 if changed else 0)
    for id_, snap in snaps.items():
        write_atomic(os.path.join(DATA, id_ + ".js"), snapshot_js(id_, snap))
    write_atomic(MANIFEST,
                 "// Written by tools/update_data.py; do not edit by hand.\n"
                 "// What each dataset was when last fetched: its size, SHA-256, where it came from, and the date it last changed.\n"
                 "(window.OLAP = window.OLAP || {}).manifest = "
                 + json.dumps({"datasets": out}, indent=2, ensure_ascii=False) + ";\n")


if __name__ == "__main__":
    main()
