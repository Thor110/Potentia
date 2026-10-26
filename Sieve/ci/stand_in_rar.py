#!/usr/bin/env python3
"""A stand-in for the rar program, for CI only: no rar can be had there (it is not free).

It answers as rar does to what Sieve asks of it (tools/cli/rar_remake.cpp): with no arguments, a
banner ("RAR 0.01 ... Copyright"); and `a` (add), with the switches Sieve gives, making a RAR 5
archive as RARLAB's description of the format has one (rarlab.com/technote.htm), every file
stored (-m0 only), its time to the nanosecond, Unix attributes. The same files with the same times
give the same bytes, as the real rar's do, so Sieve's way of making a RAR again is tested end to
end; whether the real rar gives a RAR back is for a machine that has it.

  stand_in_rar.py a [-cfg-] [-idq] [-y] [-m0] [-s] [-qo+|-qo-] [-tsm1] [-r] [-scfl] -- OUT NAME... | @LIST
"""

import os
import struct
import sys
import zlib

BANNER = "\nRAR 0.01 sieve-stand-in   Copyright (c) 2026 Sieve's CI (not RARLAB's)   25 Oct 2026\n"


def vint(n):
    out = bytearray()
    while True:
        b = n & 0x7F
        n >>= 7
        out.append(b | (0x80 if n else 0))
        if not n:
            return bytes(out)


def header(kind, flags, fields, extra=b"", data_size=None):
    body = vint(kind)
    hflags = flags | (1 if extra else 0) | (2 if data_size is not None else 0)
    body += vint(hflags)
    if extra:
        body += vint(len(extra))
    if data_size is not None:
        body += vint(data_size)
    body += fields + extra
    h = vint(len(body)) + body
    return struct.pack("<I", zlib.crc32(h) & 0xFFFFFFFF) + h


def main(argv):
    if not argv or argv[0] != "a":
        sys.stdout.write(BANNER)
        return 0 if not argv else 1
    args = argv[1:]
    solid = recurse = low_time = False
    while args and args[0] != "--":
        sw = args.pop(0)
        if sw in ("-cfg-", "-idq", "-y", "-m0", "-qo-", "-qo+", "-scfl"):
            pass
        elif sw == "-s":
            solid = True
        elif sw == "-r":
            recurse = True
        elif sw == "-tsm1":
            low_time = True
        else:
            sys.stderr.write("stand-in rar: switch not known: " + sw + "\n")
            return 2
    args.pop(0)  # --
    out, names = args[0], args[1:]
    if len(names) == 1 and names[0].startswith("@"):
        names = [line.rstrip("\n") for line in open(names[0][1:], encoding="utf-8") if line.strip()]
    members = []

    def add(path):
        if os.path.isdir(path):
            members.append(path)
            if recurse:
                for n in sorted(os.listdir(path)):
                    add(path + "/" + n)
        else:
            members.append(path)

    for n in names:
        add(n.replace("\\", "/"))
    o = bytearray(b"Rar!\x1a\x07\x01\x00")
    o += header(1, 0, vint(4 if solid else 0))
    for p in members:
        st = os.stat(p)
        is_dir = os.path.isdir(p)
        data = b"" if is_dir else open(p, "rb").read()
        name = p.encode("utf-8")
        ff = 1 if is_dir else 4
        if low_time:
            ff |= 2
        fields = vint(ff) + vint(len(data)) + vint(st.st_mode)
        if low_time:
            fields += struct.pack("<I", st.st_mtime_ns // 1000000000)
        if not is_dir:
            fields += struct.pack("<I", zlib.crc32(data) & 0xFFFFFFFF)
        fields += vint(0) + vint(1) + vint(len(name)) + name  # stored; Unix
        extra = b""
        if not low_time:
            rec = vint(3) + vint(1 | 2 | 16) + struct.pack("<II", st.st_mtime_ns // 1000000000, st.st_mtime_ns % 1000000000)
            extra = vint(len(rec)) + rec
        o += header(2, 0, fields, extra, None if is_dir else len(data))
        o += data
    o += header(5, 0, vint(0))
    with open(out, "wb") as f:
        f.write(o)
    return 0


sys.exit(main(sys.argv[1:]))
