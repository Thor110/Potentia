"""The rope's wave on a WAV, written out whole: an encoder and a decoder in plain Python (its
standard library alone), so that anyone can read how it works and check what it does.

    python rope_wav.py demo SOUND.wav            encode, decode, compare, and weigh against FLAC
    python rope_wav.py encode SOUND.wav SOUND.rope
    python rope_wav.py decode SOUND.rope SOUND.wav

It takes 16-bit PCM WAVs, mono or stereo. It is Sieve's recipe for the WoWRevived music (ROPE-POST
section 3, `L16 W2 S1 P3 T1`), in a container of its own: the same model, so close to Sieve's sizes,
but not Sieve's bytes.

How it works
------------
1. The WAV is split into its samples and everything else (the header and any other chunks: the
   "skeleton"), which is kept as it is, so the file comes back byte for byte.
2. The samples are laid out in lines of one frame (left, right). Each sample is predicted from its
   neighbours, the "wave":
     - a left sample (no value before it in its line) from the line above: the left sample of the
       frame before;
     - a right sample from before + above - diagonal: this frame's left, plus the right of the frame
       before, less the left of the frame before. The right channel's last value, moved by as much as
       the left channel just moved: the two channels predict each other.
3. What the wave misses, (sample - prediction) mod 65536, is the leftover. Each is put in one of nine
   bands by how busy its neighbourhood is (|a - c| + |b - c|, cut at 2, 4, 8 ... 128; a ninth band
   where there are no such neighbours), so quiet passages and loud ones are counted apart.
4. Each leftover's two bytes are coded by rANS, with a table of how often each byte occurs for each
   band and byte (18 tables, stored at the start, compressed by zlib).
5. Decoding runs the same wave in the same order: predict, add the leftover, next.
"""
import array
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import zlib

BANDS = (2, 4, 8, 16, 32, 64, 128)
SCALE_BITS = 12
SCALE = 1 << SCALE_BITS
RANS_L = 1 << 23


# ---- the WAV: its samples and its skeleton ------------------------------------------------------
def split_wav(b):
    """(channels, where the samples start, their size): a 16-bit PCM WAV's data chunk."""
    if b[:4] != b"RIFF" or b[8:12] != b"WAVE":
        sys.exit("not a WAV")
    at, fmt = 12, None
    while at + 8 <= len(b):
        cid, size = b[at:at + 4], struct.unpack("<I", b[at + 4:at + 8])[0]
        if cid == b"fmt ":
            tag, ch, _, _, align, bits = struct.unpack("<HHIIHH", b[at + 8:at + 24])
            if tag not in (1, 0xFFFE) or bits != 16 or ch not in (1, 2) or align != 2 * ch:
                sys.exit("this demonstration takes 16-bit PCM WAVs, mono or stereo")
            fmt = ch
        elif cid == b"data":
            if fmt is None or at + 8 + size > len(b) or size % (2 * fmt):
                sys.exit("a WAV this demonstration does not read")
            return fmt, at + 8, size
        at += 8 + size + (size & 1)
    sys.exit("a WAV with no data chunk")


# ---- the wave -------------------------------------------------------------------------------------
def band(a, b, c):
    g = abs(a - c) + abs(b - c)
    for k, edge in enumerate(BANDS):
        if g < edge:
            return k
    return 7


def predict(x, i, w):
    """The wave's prediction for sample i of x (lines of w samples), and its band (as the rope's P3
    and T1). Values are the samples as unsigned 16-bit numbers."""
    col = i % w
    has_a, has_b = col >= 1, i >= w
    a = x[i - 1] if has_a else 0
    bb = x[i - w] if has_b else 0
    if has_a and has_b:
        c = x[i - w - 1]
        p = a + bb - c
        p = 0 if p < 0 else 65535 if p > 65535 else p
        return p, band(a, bb, c)
    if has_a:
        return a, 8
    if has_b:
        return bb, 8
    return 0, 8


# ---- rANS ---------------------------------------------------------------------------------------
def normalise(counts):
    """Counts scaled to sum to SCALE, every byte seen keeping at least 1."""
    total = sum(counts)
    if total == 0:
        return [SCALE] + [0] * 255
    f = [max(1, c * SCALE // total) if c else 0 for c in counts]
    while sum(f) > SCALE:
        k = max(range(256), key=lambda j: f[j])
        f[k] -= 1
    while sum(f) < SCALE:
        k = max(range(256), key=lambda j: counts[j] - f[j] * total / SCALE)
        f[k] += 1
    return f


def cumulative(f):
    c, run = [0] * 257, 0
    for k in range(256):
        c[k] = run
        run += f[k]
    c[256] = run
    return c


def encode(wav):
    ch, at, size = split_wav(wav)
    skeleton = wav[:at] + wav[at + size:]
    x = array.array("H")
    x.frombytes(wav[at:at + size])
    if sys.byteorder == "big":
        x.byteswap()
    n = len(x)
    # The wave run through the file: each leftover's two bytes, and their band.
    lo, hi, bd = array.array("B", bytes(n)), array.array("B", bytes(n)), array.array("B", bytes(n))
    counts = [[0] * 256 for _ in range(18)]
    for i in range(n):
        p, k = predict(x, i, ch)
        e = (x[i] - p) & 0xFFFF
        lo[i], hi[i], bd[i] = e & 255, e >> 8, k
        counts[2 * k][e & 255] += 1
        counts[2 * k + 1][e >> 8] += 1
    freqs = [normalise(c) for c in counts]
    cums = [cumulative(f) for f in freqs]
    # rANS codes backwards, so the decoder reads forwards: the last symbol first.
    out = bytearray()
    state = RANS_L
    for i in range(n - 1, -1, -1):
        for t, s in ((2 * bd[i] + 1, hi[i]), (2 * bd[i], lo[i])):
            f, c = freqs[t][s], cums[t][s]
            limit = ((RANS_L >> SCALE_BITS) << 8) * f
            while state >= limit:
                out.append(state & 255)
                state >>= 8
            state = ((state // f) << SCALE_BITS) + (state % f) + c
    out += struct.pack("<I", state)
    out.reverse()
    tables = zlib.compress(b"".join(struct.pack("<256H", *f) for f in freqs), 9)
    head = b"ROPE" + struct.pack("<IIIIH", len(skeleton), at, n, len(tables), ch)
    return head + skeleton + tables + bytes(out)


def decode(rope):
    if rope[:4] != b"ROPE":
        sys.exit("not this demonstration's file")
    sk_len, at, n, t_len, ch = struct.unpack("<IIIIH", rope[4:22])
    skeleton = rope[22:22 + sk_len]
    raw = zlib.decompress(rope[22 + sk_len:22 + sk_len + t_len])
    freqs = [list(struct.unpack("<256H", raw[k * 512:(k + 1) * 512])) for k in range(18)]
    cums = [cumulative(f) for f in freqs]
    slots = []
    for f in freqs:  # which byte each of the SCALE slots belongs to
        s = bytearray(SCALE)
        p = 0
        for k in range(256):
            s[p:p + f[k]] = bytes([k]) * f[k]
            p += f[k]
        slots.append(s)
    data = rope[22 + sk_len + t_len:]
    state = struct.unpack(">I", data[:4])[0]
    pos = 4
    x = array.array("H", bytes(2 * n))
    mask = SCALE - 1
    for i in range(n):
        p, k = predict(x, i, ch)
        e = 0
        for t, shift in ((2 * k, 0), (2 * k + 1, 8)):
            slot = state & mask
            s = slots[t][slot]
            state = freqs[t][s] * (state >> SCALE_BITS) + slot - cums[t][s]
            while state < RANS_L:
                state = (state << 8) | data[pos]
                pos += 1
            e |= s << shift
        x[i] = (p + e) & 0xFFFF
    if sys.byteorder == "big":
        x.byteswap()
    return skeleton[:at] + x.tobytes() + skeleton[at:]


# ---- FLAC, for comparison -------------------------------------------------------------------------
def flac_size(path):
    """FLAC's size for the same file, where a flac or ffmpeg program is found; else None."""
    with tempfile.TemporaryDirectory() as d:
        out = os.path.join(d, "x.flac")
        if shutil.which("flac"):
            cmd = ["flac", "-8", "-s", "-f", "-o", out, path]
        elif shutil.which("ffmpeg"):
            cmd = ["ffmpeg", "-v", "error", "-y", "-i", path, "-c:a", "flac", "-compression_level", "8", out]
        else:
            return None, None
        subprocess.run(cmd, check=True)
        return os.path.getsize(out), cmd[0]


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("demo", "encode", "decode"):
        sys.exit(__doc__)
    if sys.argv[1] == "encode":
        open(sys.argv[3], "wb").write(encode(open(sys.argv[2], "rb").read()))
        return
    if sys.argv[1] == "decode":
        open(sys.argv[3], "wb").write(decode(open(sys.argv[2], "rb").read()))
        return
    path = sys.argv[2]
    wav = open(path, "rb").read()
    t0 = time.time()
    rope = encode(wav)
    t1 = time.time()
    back = decode(rope)
    t2 = time.time()
    print(f"the WAV        {len(wav):>14,} bytes")
    print(f"the rope       {len(rope):>14,} bytes  ({100 * len(rope) / len(wav):.1f}%; encoded in {t1 - t0:.1f} s, decoded in {t2 - t1:.1f} s, in Python)")
    print(f"given back     {'byte for byte' if back == wav else 'DIFFERENT'}")
    size, by = flac_size(path)
    if size:
        print(f"FLAC           {size:>14,} bytes  ({100 * size / len(wav):.1f}%; {by}, level 8)")
    else:
        print("FLAC           (no flac or ffmpeg program found to compare with)")


if __name__ == "__main__":
    main()

# The rope is a general idea: a wave run through any file's values, its misses coded. On sound, a
# waveform, FLAC does the same thing better: it fits its prediction to each block of samples, from
# up to 32 samples back, where the rope takes one or two neighbours as they come. Measured on
# WoWRevived's seven WAVs (results/flac-way-2026-11-08.md): the rope 276,730,651 bytes, FLAC
# 222,740,143; and Sieve now carries a WAV by FLAC.
#
# Why aren't we using FLAC, planet Earth?
