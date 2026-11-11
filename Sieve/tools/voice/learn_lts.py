#!/usr/bin/env python3
"""Letter-to-sound rules for Sieve's voice, learnt from CMUdict: how to say a word it does not have.

    python3 tools/voice/learn_lts.py        # writes data/voice/lj-speech/lts.tsv

Each word's letters are aligned to its phones (a letter to none, one or two phones; four rounds of
choosing the likeliest alignment, the counts of one round weighing the next), then for each letter the
phones it most often gives in each context of the letters around it: three before and after, then
two before and three after, two and two, one and two, one and one, none and one, and the letter alone,
each context inside the one before, so a rule the next shorter context already gives is dropped
without changing any answer. A word is said by the longest context written for each of its letters.

Measured (results/voice-units-2026-11-09.md, 11 November 2026): learnt without 2,000 words held out,
the rules said 55.0% of them exactly as CMUdict does (10.3% of their phones wrong); a known word
with its ending first (printers: printer, and -s), then the rules, 58.6%.

The rules are derived from CMUdict (its BSD-style licence: data/voice/lj-speech/cmudict-LICENSE.txt).
Run again, the script writes the same file, byte for byte.
"""
import collections
import math
import os
import re
import sys
def load(p):
    d = {}
    for l in open(p, encoding='utf-8'):
        w, *ph = l.split('#')[0].split()
        if '(' not in w and re.fullmatch(r"[a-z]+", w): d.setdefault(w, tuple(re.sub(r'\d','',x) for x in ph))
    return d
def align_all(d, iters=4):
    # P(chunk | letter), chunk = () or (p,) or (p,q)
    prob = collections.defaultdict(lambda: 1e-3)
    pairs = {}
    for it in range(iters):
        counts = collections.Counter()
        for w, ph in d.items():
            n, m = len(w), len(ph)
            if m > 2 * n: continue
            # DP over letters i, phones j
            best = [[-1e18] * (m + 1) for _ in range(n + 1)]; back = [[None] * (m + 1) for _ in range(n + 1)]
            best[0][0] = 0
            for i in range(n):
                for j in range(m + 1):
                    if best[i][j] < -1e17: continue
                    for k in (0, 1, 2):
                        if j + k > m: continue
                        ch = ph[j:j+k]
                        s = best[i][j] + (math.log(prob[(w[i], ch)]) if it else (0 if k == 1 else -1))
                        if s > best[i+1][j+k]: best[i+1][j+k] = s; back[i+1][j+k] = (j, ch)
            if best[n][m] < -1e17: continue
            seq = []; j = m
            for i in range(n, 0, -1):
                pj, ch = back[i][j]; seq.append(ch); j = pj
            seq.reverse(); pairs[w] = seq
            for c, ch in zip(w, seq): counts[(c, ch)] += 1
        tot = collections.Counter()
        for (c, ch), v in counts.items(): tot[c] += v
        prob = collections.defaultdict(lambda: 1e-6, {k: v / tot[k[0]] for k, v in counts.items()})
    return pairs
CTX = [(3,3),(2,3),(2,2),(1,2),(1,1),(0,1),(0,0)]
def train(pairs):
    tables = {c: collections.defaultdict(collections.Counter) for c in CTX}
    for w, seq in pairs.items():
        pw = '#' * 3 + w + '#' * 3
        for i, ch in enumerate(seq):
            for (l, r) in CTX:
                key = pw[3+i-l:3+i] + '[' + w[i] + ']' + pw[4+i:4+i+r]
                tables[(l, r)][key][ch] += 1
    rules = {}
    for (l, r) in CTX:
        for key, cnt in tables[(l, r)].items():
            ch, n = cnt.most_common(1)[0]
            rules[key] = (ch, n, sum(cnt.values()))
    return rules
def apply(w, rules, mincount=2):
    pw = '#' * 3 + w + '#' * 3; out = []
    for i in range(len(w)):
        for (l, r) in CTX:
            key = pw[3+i-l:3+i] + '[' + w[i] + ']' + pw[4+i:4+i+r]
            if key in rules and (rules[key][2] >= mincount or (l, r) == (0, 0)):
                out += list(rules[key][0]); break
    return tuple(out)
def prune(rules):
    """Rules the next shorter context already gives are dropped: the same answers, fewer rules."""
    kept = {}
    for (l, r) in CTX:  # longest first
        for key, v in rules.items():
            i = key.index('[')
            if i != l or len(key) - key.index(']') - 1 != r: continue
            if (l, r) == (0, 0): kept[key] = v; continue
            w = key  # the letters around, as a word to look at
            # what the shorter contexts (in CTX order after this one) would give for this key's letter
            centre = key[i:key.index(']') + 1]
            left, right = key[:i], key[key.index(']') + 1:]
            back = None
            for (l2, r2) in CTX[CTX.index((l, r)) + 1:]:
                if l2 > l or r2 > r: continue
                k2 = left[len(left) - l2:] + centre + right[:r2]
                if k2 in rules: back = rules[k2][0]; break
            if back != v[0]: kept[key] = v
    return kept
def apply_pruned(w, rules):
    pw = '#' * 3 + w + '#' * 3; out = []
    for i in range(len(w)):
        for (l, r) in CTX:
            key = pw[3+i-l:3+i] + '[' + w[i] + ']' + pw[4+i:4+i+r]
            if key in rules: out += list(rules[key][0]); break
    return tuple(out)

def main():
    here = os.path.dirname(os.path.abspath(__file__))
    folder = os.path.join(here, '..', '..', 'data', 'voice', 'lj-speech')
    d = load(os.path.join(folder, 'cmudict.dict'))
    rules = prune(train(align_all(d)))
    with open(os.path.join(folder, 'lts.tsv'), 'w', encoding='utf-8', newline='\n') as o:
        o.write('# letter-to-sound rules learnt from CMUdict by tools/voice/learn_lts.py: the letters before, the\n')
        o.write('# letter in brackets, the letters after (# past the word\'s ends), then the phones it gives (- none)\n')
        for key in sorted(rules):
            o.write(key + '\t' + (' '.join(rules[key][0]) or '-') + '\n')
    print(len(rules), 'rules from', len(d), 'words')


if __name__ == '__main__':
    main()
