"""A probe for v32's JPEG way (docs/NEXT-VERSION.md §4a, 3): not Sieve's code, but how its first
figures were measured: a baseline JPEG's coefficients read out of its Huffman coding (no
pixels made), written back with the file's own tables, and compared byte for byte; then the
coefficients laid out for a compressor. From ITU-T T.81 (the JPEG standard), sections F.1 and F.2.

  python tools/jpeg_probe.py FILE.jpg OUT_PREFIX
writes OUT_PREFIX.head (every byte but the scan's entropy-coded data), OUT_PREFIX.coef (the
coefficients, one layout), and says whether the scan comes back exactly.
"""
import struct
import sys

ZIGZAG_LEN = 64


def parse(b):
    """The markers up to the scan: Huffman tables, the frame's components, the scan's tables."""
    assert b[:2] == b"\xff\xd8", "not a JPEG"
    i, huff, frame, scan, restart = 2, {}, None, None, 0
    while True:
        assert b[i] == 0xFF
        while b[i + 1] == 0xFF:  # (fill bytes)
            i += 1
        m = b[i + 1]
        L = (b[i + 2] << 8) | b[i + 3]
        seg = b[i + 4:i + 2 + L]
        if m == 0xC4:  # DHT: one or more tables
            j = 0
            while j < len(seg):
                tc, th = seg[j] >> 4, seg[j] & 15
                counts = seg[j + 1:j + 17]
                n = sum(counts)
                values = seg[j + 17:j + 17 + n]
                # Canonical codes (C.2): code lengths 1..16 in order.
                codes, code, k = {}, 0, 0
                for length in range(1, 17):
                    for _ in range(counts[length - 1]):
                        codes[(length, code)] = values[k]
                        k += 1
                        code += 1
                    code <<= 1
                huff[(tc, th)] = codes
                j += 17 + n
        elif m in (0xC0, 0xC1):  # SOF0/SOF1: baseline / extended sequential, Huffman
            h, w, nc = (seg[1] << 8) | seg[2], (seg[3] << 8) | seg[4], seg[5]
            comps = []
            for c in range(nc):
                cid, hv, tq = seg[6 + 3 * c], seg[7 + 3 * c], seg[8 + 3 * c]
                comps.append({"id": cid, "h": hv >> 4, "v": hv & 15})
            frame = {"w": w, "h": h, "comps": comps}
        elif m in (0xC2, 0xC3, 0xC5, 0xC6, 0xC7, 0xC9, 0xCA, 0xCB, 0xCD, 0xCE, 0xCF):
            raise SystemExit("not a baseline JPEG (progressive, lossless or arithmetic)")
        elif m == 0xDD:
            restart = (seg[0] << 8) | seg[1]
        elif m == 0xDA:  # SOS
            ns = seg[0]
            sc = []
            for c in range(ns):
                cid, t = seg[1 + 2 * c], seg[2 + 2 * c]
                sc.append({"id": cid, "td": t >> 4, "ta": t & 15})
            scan = {"comps": sc, "start": i + 2 + L}
            return huff, frame, scan, restart
        i += 2 + L


class Bits:
    def __init__(self, b, at):
        self.b, self.at, self.acc, self.n = b, at, 0, 0

    def bit(self):
        if self.n == 0:
            c = self.b[self.at]
            if c == 0xFF:
                nxt = self.b[self.at + 1]
                assert nxt == 0x00, f"a marker in the scan at {self.at} (0xFF{nxt:02X}): restarts not handled here"
                self.at += 2
            else:
                self.at += 1
            self.acc, self.n = c, 8
        self.n -= 1
        return (self.acc >> self.n) & 1

    def bits(self, n):
        v = 0
        for _ in range(n):
            v = (v << 1) | self.bit()
        return v

    def huff(self, codes):
        code, length = 0, 0
        while length < 16:
            code = (code << 1) | self.bit()
            length += 1
            if (length, code) in codes:
                return codes[(length, code)]
        raise ValueError("no Huffman code")


def extend(v, s):  # F.2.2.1
    return v - (1 << s) + 1 if s and v < (1 << (s - 1)) else v


def decode(b):
    huff, frame, scan, restart = parse(b)
    assert not restart, "restart intervals not handled in this prototype"
    hmax = max(c["h"] for c in frame["comps"])
    vmax = max(c["v"] for c in frame["comps"])
    mcux = -(-frame["w"] // (8 * hmax))
    mcuy = -(-frame["h"] // (8 * vmax))
    order = []  # the scan's components, each with its tables and its sampling
    for s in scan["comps"]:
        f = next(c for c in frame["comps"] if c["id"] == s["id"])
        order.append((f["h"], f["v"], huff[(0, s["td"])], huff[(1, s["ta"])]))
    assert len(order) == len(frame["comps"]), "a scan of some components only: not handled"
    rd = Bits(b, scan["start"])
    pred = [0] * len(order)
    blocks = [[] for _ in order]  # per component, in scan order: 64 values each, zigzag order, DC as its difference
    for _ in range(mcux * mcuy):
        for ci, (h, v, dct, act) in enumerate(order):
            for _ in range(h * v):
                blk = [0] * 64
                s = rd.huff(dct)
                diff = extend(rd.bits(s), s)
                blk[0] = diff
                pred[ci] += diff
                k = 1
                while k < 64:
                    rs = rd.huff(act)
                    r, s = rs >> 4, rs & 15
                    if s == 0:
                        if r == 15:
                            k += 16
                            continue
                        break
                    k += r
                    blk[k] = extend(rd.bits(s), s)
                    k += 1
                blocks[ci].append(blk)
    # The rest of the last byte, as the file has it, and where the scan ends.
    end = rd.at
    pad_bits = rd.n
    pad = rd.acc & ((1 << pad_bits) - 1) if pad_bits else 0
    return huff, frame, scan, order, blocks, end, pad_bits, pad


def encode(order, blocks, pad_bits, pad):
    """The coefficients coded again with the same tables, as a baseline encoder does (F.1.2)."""
    out, acc, n = bytearray(), 0, 0

    def put(code, length):
        nonlocal acc, n
        acc, n = (acc << length) | code, n + length
        while n >= 8:
            n -= 8
            byte = (acc >> n) & 0xFF
            out.append(byte)
            if byte == 0xFF:
                out.append(0)
        acc &= (1 << n) - 1 if n else 0

    inv = []
    for (_, _, dct, act) in order:
        inv.append(({v: k for k, v in dct.items()}, {v: k for k, v in act.items()}))

    def magnitude(x):
        s = abs(x).bit_length()
        return s, (x if x >= 0 else x + (1 << s) - 1)

    count = [0] * len(order)
    idx = [0] * len(order)
    total = len(blocks[0]) // (order[0][0] * order[0][1])
    for _ in range(total):
        for ci, (h, v, _, _) in enumerate(order):
            dinv, ainv = inv[ci]
            for _ in range(h * v):
                blk = blocks[ci][idx[ci]]
                idx[ci] += 1
                s, bits = magnitude(blk[0])
                put(dinv[s][1], dinv[s][0])
                if s:
                    put(bits, s)
                run = 0
                last = max((k for k in range(1, 64) if blk[k]), default=0)
                for k in range(1, last + 1):
                    if blk[k] == 0:
                        run += 1
                        continue
                    while run >= 16:
                        put(ainv[0xF0][1], ainv[0xF0][0])
                        run -= 16
                    s, bits = magnitude(blk[k])
                    rs = (run << 4) | s
                    put(ainv[rs][1], ainv[rs][0])
                    put(bits, s)
                    run = 0
                if last < 63:
                    put(ainv[0x00][1], ainv[0x00][0])
    if n:  # the last byte's spare bits, as the file had them
        put(pad if pad_bits == 8 - n else (1 << (8 - n)) - 1, 8 - n)
    return bytes(out)


if __name__ == "__main__":
    src, prefix = sys.argv[1], sys.argv[2]
    b = open(src, "rb").read()
    huff, frame, scan, order, blocks, end, pad_bits, pad = decode(b)
    data = b[scan["start"]:end]
    again = encode(order, blocks, pad_bits, pad)
    exact = again == data
    nblocks = sum(len(x) for x in blocks)
    print(f"{src}: {len(b)} bytes, {frame['w']}x{frame['h']}, {nblocks} blocks; scan {len(data)} bytes; "
          f"coded again {'EXACTLY' if exact else 'NOT exactly'} ({len(again)} bytes); rest {len(b) - end} bytes after it")
    head = b[:scan["start"]] + b[end:]
    open(prefix + ".head", "wb").write(head)
    # Layout: each component's blocks, coefficient by coefficient (all DC differences, then all of
    # zigzag position 1, ...), each value as two bytes, signed, little-endian.
    out = bytearray()
    for comp in blocks:
        for k in range(64):
            for blk in comp:
                out += struct.pack("<h", blk[k])
    open(prefix + ".coef", "wb").write(bytes(out))
