#!/usr/bin/env python3
"""Talks to `sieve web-host` as a browser does (native messaging: a 4-byte length, then JSON), to
check it without a browser (web/README.md; the CI runs it).

  python web/host/check_host.py SIEVE FILE.sieve EXPECTED_FILE   # decoded, the same bytes
  python web/host/check_host.py SIEVE FILE.sieve --refused        # refused (a wrong hash is given)
  python web/host/check_host.py SIEVE FILE.sieve --refused-as-is  # refused with its own hash
  python web/host/check_host.py SIEVE FILE.sieve --listing        # a .sieve of many files: its listing
  python web/host/check_host.py SIEVE FILE.sieve --path P EXPECTED_FILE   # its file P, the same bytes
  python web/host/check_host.py SIEVE index.sieve --table          # a site's table: its files and hashes

Prints what the host answered; exits non-zero if it is not what was asked for.
"""

import base64
import hashlib
import json
import struct
import subprocess
import sys


def talk(sieve, requests):
    # Started as the browser starts it: the extension's origin as its first argument.
    p = subprocess.Popen([sieve, "chrome-extension://checkhostcheckhostcheckhostcheck/"], stdin=subprocess.PIPE, stdout=subprocess.PIPE)
    for r in requests:
        body = json.dumps(r).encode("utf-8")
        p.stdin.write(struct.pack("=I", len(body)) + body)
    p.stdin.close()
    out = p.stdout.read()
    p.wait()
    replies, at = [], 0
    while at < len(out):
        (n,) = struct.unpack_from("=I", out, at)
        if n > 1024 * 1024:
            sys.exit(f"a message of {n} bytes, over native messaging's 1 MB")
        replies.append(json.loads(out[at + 4:at + 4 + n]))
        at += 4 + n
    return replies


def main():
    sieve, file = sys.argv[1], sys.argv[2]
    data = open(file, "rb").read()
    own = hashlib.sha256(data).hexdigest()
    mode = sys.argv[3]
    path = None
    if mode == "--path":
        path, mode = sys.argv[4], sys.argv[5]
    if mode == "--refused":
        sha = "0" * 64
    elif mode == "--refused-as-is":
        sha = own
    else:
        # As a page states it: an integrity attribute.
        sha = "sha256-" + base64.b64encode(hashlib.sha256(data).digest()).decode()
    request = {"id": 2, "op": "table" if mode == "--table" else "decode", "name": file, "sha256": sha, "data": base64.b64encode(data).decode()}
    if path is not None:
        request["path"] = path
    replies = talk(sieve, [{"id": 1, "op": "hello"}, request])
    hello, head = replies[0], replies[1]
    print("hello:", hello)
    print("reply:", {k: v for k, v in head.items() if k != "data"})
    if not hello.get("ok"):
        sys.exit("hello was not answered")
    if mode.startswith("--refused"):
        if head.get("ok"):
            sys.exit("it was not refused")
        return
    if not head.get("ok"):
        sys.exit("refused: " + head.get("error", ""))
    if mode == "--table":
        for f in head.get("files", []):
            print(f"table: {f['path']}  {f['sha256']}")
        print(f"table: {len(head.get('files', []))} files")
        return
    if mode == "--listing":
        if not head.get("listing") or not head.get("files"):
            sys.exit("no listing")
        for f in head["files"]:
            print(f"listed: {f['path']}  {f['size']} bytes  {f['sha256']}")
        print(f"listing: {len(head['files'])} files, {len(head['folders'])} folders, {head['size']} bytes")
        return
    if head.get("listing"):
        sys.exit("a listing, not a file")
    parts = replies[2:]
    if len(parts) != head["parts"]:
        sys.exit(f"{len(parts)} parts, not {head['parts']}")
    got = b"".join(base64.b64decode(p["data"]) for p in sorted(parts, key=lambda p: p["part"]))
    want = open(mode, "rb").read()
    if got != want or hashlib.sha256(got).hexdigest() != head["sha256"]:
        sys.exit("the bytes are not the file's")
    print(f"decoded: {head['name']}, {head['type']}, {len(got)} bytes in {len(parts)} part(s), the same as {mode}")


if __name__ == "__main__":
    main()
