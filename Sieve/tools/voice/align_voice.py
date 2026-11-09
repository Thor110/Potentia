#!/usr/bin/env python3
"""Align the voice's clips: where each speech sound (phone) starts and ends in each clip.

    python3 tools/fetch_voice.py                      # the clips as WAVs, into corpus/voice/lj-speech
    python3 tools/voice/align_voice.py                # writes data/voice/lj-speech/alignment.tsv

Sieve's own voice (tools/cli/voice.hpp) is joined from diphones cut from the clips, and needs to know
where each phone lies. Made once, and kept in the repository; the voice needs nothing below to run.

Each clip's transcript is read into phones by CMUdict (data/voice/lj-speech/cmudict.dict, ARPAbet);
espeak-ng (a build-time tool only: GPL, used through its library, never linked into Sieve or shipped)
speaks the same phones and says where each starts in its own speech; dynamic time warping over the two
recordings' MFCCs carries each start from espeak's speech to the clip. A word CMUdict does not have is
read by espeak itself, its phones named in ARPAbet again; a clip whose phones espeak cannot follow is
left out (44 of the 547 on 9 November 2026).

Needs: espeak-ng 1.51 (its library, libespeak-ng.so.1, with the en-us voice) and numpy.
Measured (results/voice-units-2026-11-09.md): fricatives' zero-crossing rate above vowels' in 95.2%
of pairs as aligned, against 62.9% and 57.4% with the alignment moved 100 ms either way.
"""
import argparse
import ctypes
import os
import re
import subprocess
import sys
import wave

import numpy as np

# ---- CMUdict's ARPAbet and espeak-ng's names
MAP = {'AA':'A:','AE':'a','AH':'V','AO':'O:','AW':'aU','AY':'aI','B':'b','CH':'tS','D':'d','DH':'D','EH':'E',
       'ER':'3:','EY':'eI','F':'f','G':'g','HH':'h','IH':'I','IY':'i:','JH':'dZ','K':'k','L':'l','M':'m','N':'n',
       'NG':'N','OW':'oU','OY':'OI','P':'p','R':'r','S':'s','SH':'S','T':'t','TH':'T','UH':'U','UW':'u:','V':'v',
       'W':'w','Y':'j','Z':'z','ZH':'Z'}
UNSTRESSED = {'AH':'@','ER':'3'}
def esp(phones):
    out = []
    for p in phones:
        base, stress = (p[:-1], p[-1]) if p[-1].isdigit() else (p, None)
        name = UNSTRESSED.get(base, MAP[base]) if stress == '0' else MAP[base]
        out.append(("'" if stress == '1' else ',' if stress == '2' else '') + name)
    return ''.join(out)

# ---- espeak-ng through its library: speech with each phone's position
lib = ctypes.CDLL('libespeak-ng.so.1')
class Event(ctypes.Structure):
    _fields_ = [('type', ctypes.c_int), ('unique_identifier', ctypes.c_uint), ('text_position', ctypes.c_int),
                ('length', ctypes.c_int), ('audio_position', ctypes.c_int), ('sample', ctypes.c_int),
                ('user_data', ctypes.c_void_p), ('id', ctypes.c_char * 8)]
CB = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_short), ctypes.c_int, ctypes.POINTER(Event))
samples, events = [], []
def cb(wav, n, ev):
    if wav and n > 0: samples.extend(wav[:n])
    i = 0
    while ev[i].type != 0:
        e = ev[i]
        if e.type == 7: events.append((e.audio_position, e.sample, bytes(e.id).split(b'\0')[0].decode('latin1')))
        i += 1
    return 0
cbf = CB(cb)
rate = lib.espeak_Initialize(2, 0, None, 1)  # synchronous, phoneme events
lib.espeak_SetSynthCallback(cbf)
lib.espeak_SetVoiceByName(b'en-us')
def synth(text):
    samples.clear(); events.clear()
    b = text.encode()
    lib.espeak_Synth(b, len(b) + 1, 0, 0, 0, 0x100, None, None)  # espeakPHONEMES: [[...]] read as phonemes
    lib.espeak_Synchronize()
    return list(samples), list(events)

# ---- the alignment

RATE, HOP, WIN = 22050, 220, 512
def mfcc(x):
    x = np.asarray(x, dtype=np.float64)
    x = np.append(x[0], x[1:] - 0.97 * x[:-1])
    n = 1 + max(0, (len(x) - WIN) // HOP)
    frames = np.stack([x[i*HOP:i*HOP+WIN] for i in range(n)]) * np.hamming(WIN)
    spec = np.abs(np.fft.rfft(frames, 512)) ** 2
    mel = lambda f: 2595 * np.log10(1 + f / 700); imel = lambda m: 700 * (10 ** (m / 2595) - 1)
    pts = imel(np.linspace(mel(80), mel(8000), 28)); bins = np.floor((512 + 1) * pts / RATE).astype(int)
    fb = np.zeros((26, 257))
    for j in range(26):
        a, b, c = bins[j], bins[j+1], bins[j+2]
        fb[j, a:b] = (np.arange(a, b) - a) / max(1, b - a); fb[j, b:c] = (c - np.arange(b, c)) / max(1, c - b)
    e = np.log(spec @ fb.T + 1e-8)
    k = np.arange(26); dct = np.cos(np.pi / 26 * (k[None, :] + 0.5) * np.arange(13)[:, None])
    c = e @ dct.T
    return (c - c.mean(0)) / (c.std(0) + 1e-8)

def dtw(a, b):
    n, m = len(a), len(b)
    cost = np.sqrt(((a[:, None, :] - b[None, :, :]) ** 2).sum(-1))
    D = np.full((n + 1, m + 1), np.inf); D[0, 0] = 0
    for i in range(1, n + 1):
        row = D[i]; prev = D[i - 1]
        best = np.minimum(prev[1:], prev[:-1])  # from above, diagonal
        r = cost[i - 1] + best
        # left moves (same i), cumulative along the row
        for j in range(1, m + 1):
            v = r[j - 1]
            if row[j - 1] + cost[i - 1, j - 1] < v: v = row[j - 1] + cost[i - 1, j - 1]
            row[j] = v
    # back-trace: for each a-frame, the b-frame it maps to (first)
    i, j = n, m; amap = np.zeros(n, dtype=int)
    while i > 0 and j > 0:
        amap[i - 1] = j - 1
        moves = [(D[i-1, j-1], i-1, j-1), (D[i-1, j], i-1, j), (D[i, j-1], i, j-1)]
        _, i, j = min(moves)
    return amap

REV = {v: k for k, v in MAP.items()}; REV.update({'@': 'AH', '3': 'ER', 't#': 'T', '@L': None, 'i': 'IY', 'I2': 'IH', 'a#': 'AH', 'aI@': None, 'e@': None, 'i@': None, 'U@': None, 'O@': None, 'A@': None, 'oU#': 'OW', 'I#': 'IH', 'n-': 'N', 'l/2': 'L', 'r-': 'R'})

def norm(name):
    return {'i': 'i:', 't#': 't', '3': '3:', '@': 'V', 'I2': 'I', 'I#': 'I', 'a#': 'V', 'oU#': 'oU', 'n-': 'n', 'l/2': 'l', 'r-': 'r', '@L': 'l'}.get(name, name)

def match(phones, ev):
    """Each phone's start in espeak's audio: the phones (their espeak names) and espeak's events lined
    up by edit distance; a phone with no event of its own takes a start between its neighbours'."""
    want = [norm(esp([p]).strip("',")) for p in phones]
    got = [norm(e[2]) for e in ev]
    n, m = len(want), len(got)
    D = np.zeros((n + 1, m + 1)); D[:, 0] = np.arange(n + 1); D[0, :] = np.arange(m + 1)
    for i in range(1, n + 1):
        for j in range(1, m + 1):
            D[i, j] = min(D[i-1, j-1] + (want[i-1] != got[j-1]), D[i-1, j] + 1, D[i, j-1] + 1)
    if D[n, m] > max(2, n // 5): return None  # too unlike: left out
    i, j, at = n, m, [None] * n
    while i > 0 and j > 0:
        if D[i, j] == D[i-1, j-1] + (want[i-1] != got[j-1]): at[i-1] = ev[j-1][1]; i -= 1; j -= 1
        elif D[i, j] == D[i-1, j] + 1: i -= 1
        else: j -= 1
    known = [k for k in range(n) if at[k] is not None]
    if not known: return None
    for k in range(n):
        if at[k] is None:
            lo = max([q for q in known if q < k], default=None); hi = min([q for q in known if q > k], default=None)
            if lo is None: at[k] = at[hi]
            elif hi is None: at[k] = at[lo]
            else: at[k] = at[lo] + (at[hi] - at[lo]) * (k - lo) // (hi - lo)
    return at

def read_wav(p):
    with wave.open(p, 'rb') as w:
        return np.frombuffer(w.readframes(w.getnframes()), dtype='<i2')

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    root = os.path.join(here, '..', '..')
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--dict', default=os.path.join(root, 'data', 'voice', 'lj-speech', 'cmudict.dict'))
    p.add_argument('--metadata', default=os.path.join(root, 'data', 'anchors', 'public-domain', 'voice', 'lj-speech', 'metadata.csv'))
    p.add_argument('--wavs', default=os.path.join(root, 'corpus', 'voice', 'lj-speech'))
    p.add_argument('--out', default=os.path.join(root, 'data', 'voice', 'lj-speech', 'alignment.tsv'))
    args = p.parse_args()
    dictp, meta, wavs, out = args.dict, args.metadata, args.wavs, args.out
    d = {}
    for l in open(dictp, encoding='utf-8'):
        w, *ph = l.split('#')[0].split()
        if '(' not in w: d.setdefault(w, ph)
    done = skipped = 0
    with open(out, 'w', encoding='utf-8') as o:
        o.write('# clip\tphone\tstart sample\tend sample (in the clip, 22,050 a second)\n')
        for line in open(meta, encoding='utf-8'):
            cid, _, norm = line.rstrip('\n').split('|')
            toks = re.findall(r"[a-z']+|[,.;:!?]", norm.lower())
            phones, parts, ok = [], [], True
            for t in toks:
                if t in ',.;:!?':
                    parts.append('_:'); continue
                w = t.strip("'")
                ph = d.get(w)
                if ph is None:
                    xs = subprocess.run(['espeak-ng', '-v', 'en-us', '-q', '-x', '--sep=|', w], capture_output=True, text=True).stdout.split()
                    ph = []
                    for chunk in xs:
                        for e in chunk.split('|'):
                            e = e.strip("',")
                            if not e: continue
                            a = REV.get(e)
                            if a is None: ok = False; break
                            ph.append(a + ('1' if a in ('AA','AE','AH','AO','AW','AY','EH','ER','EY','IH','IY','OW','OY','UH','UW') else ''))
                if not ok or not ph: ok = False; break
                phones += ph; parts.append(esp(ph))
            if not ok:
                skipped += 1; continue
            samples, events = synth('[[' + ' '.join(parts) + ']]')
            real = read_wav(os.path.join(wavs, cid + '.wav'))
            ev = [e for e in events if not e[2].startswith('_') and e[2] != ';']
            pos = match(phones, ev)
            if pos is None:
                skipped += 1; continue
            a, b = mfcc(samples), mfcc(real)
            amap = dtw(a, b)
            starts = []
            for sp in pos:
                f = min(len(amap) - 1, sp // HOP)
                starts.append(int(amap[f]) * HOP)
            ends = starts[1:] + [len(real)]
            # the last phone's end: where espeak's speech ends, carried across
            last_end = min(len(amap) - 1, (events[-1][1] if events else len(samples)) // HOP)
            ends[-1] = min(len(real), int(amap[last_end]) * HOP + HOP)
            for p, s, e in zip(phones, starts, ends):
                o.write(f'{cid}\t{p}\t{s}\t{max(s, e)}\n')
            done += 1
            if done % 50 == 0: print(done, 'aligned', flush=True)
    print('aligned', done, 'skipped', skipped)
if __name__ == "__main__": main()
