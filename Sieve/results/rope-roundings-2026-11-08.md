# Do the rope's leftovers repeat across files? (8 November 2026)

Edward's idea (7 November 2026): round a file's place to something short and keep what the rounding
loses in a table, or tables of tables, of common roundings shipped with Sieve as a setting. Whether
it can pay comes down to one question: do the leftovers (what the rope's wave misses, which is what a
rounding would have to restore) repeat across different files? If they do, a table of them is worth
having; if not, naming an entry costs as much as it saves.

**How.** `tools/bench/residual_repeats.py`: each file's leftovers under a `P3` wave (before + above −
diagonal, as rope-wave-v1's `P3`), as 16-bit values (`L16`), cut into their two byte planes. A table
of every k-byte chunk of four training files' planes (k = 4, 8, 16); each test file's chunks looked up
in it. A hit saves what the chunk costs under the test file's own table (order 0, as the rope codes a
plane), less what naming it costs (log2 of the table's size), and every chunk pays a hit-or-miss
flag. Training: WoWRevived's Music-Human/02 to 05 and Silesia's `mr`; tested on Music-Martian/03 to
05, Silesia's `x-ray` (rows of 1,901 16-bit pixels) and 50 MB of random bytes, the control.

| k | Martian/03 | Martian/04 | Martian/05 | x-ray | Noise |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 4 bytes | −17.06% | −6.66% | −9.51% | −51.03% | −0.01% |
| 8 bytes | −2.78% | +0.52% | +0.38% | −21.74% | 0% |
| 16 bytes | +1.93% | +1.31% | +1.60% | −4.58% | 0% |

(The net saving as a share of what the file's leftovers cost now; + is smaller. The table itself, 12.3
million chunks of 16 bytes for k = 16, about 200 MB, is not charged here.)

**What the hits are** (`hits_detail.py`, k = 16, the WAVs): in the low byte plane the whole gain is
one chunk, met 115,288 times in training: a run of zeros, a wave that predicts exactly (silence). In
the high plane it grows with the table, to 3.5% of the plane with its 65,536 most common chunks (1
MiB): leftovers near zero have high bytes of 00 or FF, so the chunks are runs of signs.

**The same without a table** (`orders.py`): each leftover byte coded given the one before it (order
1), against order 0, as the rope codes a plane now:

| | Order 0 (now) | Order 1 | Saved |
| :--- | ---: | ---: | ---: |
| Music-Martian/03 | 39,507,787 | 38,544,682 | 2.44% |
| Music-Martian/04 | 42,166,709 | 41,227,375 | 2.23% |
| Music-Martian/05 | 42,305,166 | 41,142,787 | 2.75% |
| Noise | 49,999,956 | 49,988,222 | 0.02% |

Order 2 looks larger (8 to 24% on a plane), but its static estimate learns 65,536 tables from the data
without charging for them: on the noise it shows 7%, which is not there. Only an adaptive coder can
measure it honestly.

**Verdict.** The leftovers do not repeat as roundings: what repeats is silence and runs of small
values, structure of the file's own neighbourhood, which a context does better than a table. Short
chunks cost more to name than they save; noise never hits. The finding to take forward is the order-1
saving, 2.2 to 2.7% of these WAVs with no table, which the rope does not take now (each plane is one
table): a new rope version could code each leftover byte under a table chosen by the byte before, as
fast to decode as now (a table lookup a byte). That needs a new format version.
