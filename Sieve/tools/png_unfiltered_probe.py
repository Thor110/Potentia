"""A probe for a PNG's rows carried unfiltered under temporary (ROADMAP D.1): not Sieve's code, but
how the idea was measured before it is built.

PNG's five row filters are sums and differences modulo 256, so each can be undone and done again
exactly given the row's filter byte. This writes, for a PNG, the same rows with every filter undone
(filter 0 on every row, deflated by zlib at level 9) as a PNG of their own, and the original's filter
bytes alone as a file, so that each can be carried by Sieve and measured beside the original:

  python tools/png_unfiltered_probe.py IN.png OUT.png FILTERS.bin

Only 8-bit, non-interlaced PNGs are read. It prints the size, the colour type and how many rows
took each filter.
"""

import struct
import sys
import zlib


def chunks(b):
    i = 8
    while i < len(b):
        (n,) = struct.unpack(">I", b[i : i + 4])
        yield b[i + 4 : i + 8], b[i + 8 : i + 8 + n]
        i += 12 + n


def chunk(t, d):
    return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)


def unfiltered(raw, h, stride, bpp):
    # Each row's filter undone against the row above, already undone (PNG's specification, §9).
    prev = bytearray(stride)
    rows, filters = [], bytearray()
    for y in range(h):
        f = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1 : (y + 1) * (stride + 1)])
        filters.append(f)
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            u = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if f == 1:
                line[x] = (line[x] + a) & 255
            elif f == 2:
                line[x] = (line[x] + u) & 255
            elif f == 3:
                line[x] = (line[x] + ((a + u) >> 1)) & 255
            elif f == 4:
                p = a + u - c
                pa, pb, pc = abs(p - a), abs(p - u), abs(p - c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else (u if pb <= pc else c))) & 255
        rows.append(bytes(line))
        prev = line
    return rows, bytes(filters)


def main():
    src, out_png, out_filters = sys.argv[1:4]
    b = open(src, "rb").read()
    cs = list(chunks(b))
    w, h, depth, ct, _, _, il = struct.unpack(">IIBBBBB", [d for t, d in cs if t == b"IHDR"][0])
    if il != 0 or depth != 8:
        sys.exit("only 8-bit, non-interlaced PNGs are read")
    bpp = {0: 1, 2: 3, 3: 1, 4: 2, 6: 4}[ct]
    rows, filters = unfiltered(zlib.decompress(b"".join(d for t, d in cs if t == b"IDAT")), h, w * bpp, bpp)
    out = b[:8]
    for t, d in cs:
        if t == b"IDAT":
            continue
        if t == b"IEND":
            out += chunk(b"IDAT", zlib.compress(b"".join(b"\0" + r for r in rows), 9))
        out += chunk(t, d)
    open(out_png, "wb").write(out)
    open(out_filters, "wb").write(filters)
    print(w, h, ct, "filters", {k: filters.count(k) for k in range(5)})


if __name__ == "__main__":
    main()
