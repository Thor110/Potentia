# The rope's kept bytes as wheels: the landscape, and steering (3 November 2026)

IDEAS §19.117, Edward's idea: the rope's kept bytes (the third, the two thirds, and further
divisions) as free values, "wheels" turned to steer the file towards a smaller code, learnt on 4
bytes, then 8, then 16. The question that decides it: is the size, as a wheel turns, a landscape
with valleys to roll down (steering works), or noise (no better than guessing)? And where the
valleys lead.

## How it was measured

`tools/measure_rope_wheels.py`, outside Sieve (nothing in Sieve changed). The rope at P0, where the
kept points matter most: every value is predicted by the straight line through them, and what the
line leaves costs its order-0 entropy, with the stream's table as rope-wave-v1 counts it, plus a
byte for each free wheel. Each wheel can take any value; the file's own byte where it lies is then
coded with the rest. 256 KiB of each file (the Vitruvian picture whole, 64 KiB):

- where the rope wins: `vitruvian-centre-256.gray` (tests/rope), a made-up 8-bit sound (two tones and
  a little noise), Silesia's `x-ray` (from byte 4,000,000) and `mr` (from its middle);
- where it loses: a program (`ooffice` from byte 2,000,000) and text (`dickens` from byte 1,000,000).

Then:

- **One wheel:** the third, through all 256 values (two thirds as the file's own).
- **Two wheels:** the third and two thirds, a 64 by 64 grid (every fourth value).
- **Steering 4, 8 and 16 wheels** (evenly spaced, with the start and end): each wheel in turn moved
  by 32, then 8, then 1, kept where it helps, until nothing helps. This was done twice, from the
  file's own bytes and from 128 everywhere. It was set against as many random settings as steering
  tried.

## Is it smooth?

One wheel, size against value:

| File | Local minima (of 254) | Each step like the last (correlation) | Best value, file's own byte |
| :--- | ---: | ---: | :--- |
| Vitruvian picture | 1 | 0.99 | 212, 202 |
| sound | 7 | 0.98 | 39, 101 |
| x-ray | 14 | 0.85 | 10, 9 |
| mr | 5 | 0.77 | 0, 0 |
| text | 1 | 0.31 | 102, 108 |
| program | 25 | 0.08 | 139, 68 |

Two wheels (the 64 by 64 grid): 1 valley for the picture, 5 for the sound, 3 for mr, 4 for text,
27 for x-ray and 31 for the program.

**The landscape is smooth where the file is** (pictures and sound): one wheel's curve on the picture
falls steadily from 65,673 bytes at 0 to its floor near 212, and rises after. On a program it is
noise: 25 dips in 256 values, and a step tells nothing of the next.

## Does steering find good settings?

Bytes after steering, against the best of as many random settings:

| File | Wheels | Steered (from the file's own bytes) | Random, the same number of tries |
| :--- | ---: | ---: | ---: |
| Vitruvian picture | 4 | 51,886 | 56,710 |
| | 16 | 51,655 | 60,878 |
| sound | 16 | 257,018 | 261,248 |
| x-ray | 16 | 219,391 | 255,162 |
| mr | 16 | 138,068 | 247,706 |
| text | 16 | 196,720 (168,999 from 128) | 244,082 |
| program | 16 | 241,816 (224,020 from 128) | 261,407 |

**Steering beats guessing everywhere**, from 1.6% (the sound) to 44% (mr), in a few hundred tries.
With 4 or more wheels there are several valleys: steering from the file's own bytes and from 128
mostly ended in different places, and either can be the better.

## Where the valleys lead

| File | Plain bytes (order 0) | Best steered (P0 line) | The rope as it is (`sieve rope`) |
| :--- | ---: | ---: | ---: |
| Vitruvian picture | 51,544 | 51,531 (64 wheels) | 44,729 (`L8 W256 S1 P4 K0`) |
| sound | 255,815 | 255,518 (256 wheels) | 130,094 (`L8 W3487 S1 P2 K1`) |
| x-ray | 212,686 | 219,391 (16) | |
| mr | 129,010 | 138,068 (16) | |
| text | 147,960 | 168,999 (16) | |
| program | 214,172 | 224,020 (16) | |

**Nowhere worth going.** At best, steering the line's wheels comes to what the bytes cost coded
plainly, as if there were no line: 13 bytes under on the picture with 64 wheels, 297 on the sound
with 256. The rope as it is, predicting each value from its neighbours, is far smaller (the picture
13% under plain, the sound 49%). A straight line through a few points follows only a file's slowest
trend, and these files have little in it. More wheels give the line more to follow, but each costs
its byte, and at 64 to 256 wheels the line is about as good as no line.

## What it says

- **Steering works as a way to search:** smooth landscapes on pictures and sound, and steering
  finds in hundreds of tries what guessing does not find in as many. The Manual Navigation screen
  (IDEAS §19.117) can use it as planned.
- **The kept bytes on the rope's line are the wrong car.** Their best setting is no better than no
  line at all. For steering to pay in the packer, the wheels must touch every value's prediction:
  the weights of the neighbours in the rope's propagation, say, or a bias for each region. That is
  the next measurement, with the same script and a different car.
- **This is the rope without propagation (P0).** With propagation (P1 to P4), the line is used only
  where a value has no neighbour before it, so the kept bytes matter less still.

## The second car: wheels on every value's prediction (4 November 2026)

Edward: "The ropes bytes are indeed the wrong thing to steer [...] Measure the wheels first, then
build Manual Navigation." `tools/measure_prediction_wheels.py`: each value predicted as a weighted
sum of three neighbours plus a bias (for a picture: left, above, above-left; for a file of one
dimension: the three values before it), each weight a byte (in 32nds, −4 to +4), the bias a byte;
then a bias for each 4,096 values (16 to 64 more wheels). Counted as above, against the rope's own
fixed predictions counted the same way (no wheels paid for). The same six files; the pictures at
their shape (Vitruvian 256 wide; x-ray 2 bytes a pixel, 3,802 a row; mr 2 and 1,024).

| File | Plain | Left | Gradient | Triangle | Best steered (weights, bias) | With region biases | Random, as many tries |
| :--- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Vitruvian picture | 51,544 | 48,944 | 46,409 | **44,887** | 45,422 (25, 16, −16; 83) | 45,426 | 49,714 |
| sound | 255,815 | 144,200 | 152,309 | 144,601 | **144,127** (32, 0, 0; −20) | 144,191 | 205,918 |
| x-ray | 212,686 | **160,734** | 169,495 | 162,704 | 160,736 (32, 0, 0; 1) | 160,800 | 183,404 |
| mr | 129,010 | 97,970 | **87,601** | 87,660 | 87,605 (32, 32, −32; 0) | 87,662 | 112,813 |
| program | **214,172** | 243,913 | 246,562 | 250,225 | 214,176 (no prediction) | 214,240 | 214,176 |
| text | **147,960** | 207,235 | 246,587 | 222,779 | 147,964 (no prediction) | 148,028 | 147,964 |

- **Steering finds the right fixed prediction, and no better one.** On the sound, x-ray and mr it
  turned the wheels to "the value to the left" or "the gradient", which the rope already tries (its
  P1 to P4); the best it found past them is 72 bytes on the sound (0.05%). On the picture it stopped
  in a valley above the triangle (45,422 against 44,887), which no straight weighting can make. On
  a program and text it found that no prediction is best.
- **The region biases found nothing:** steering left almost all of them at 0, and each costs its
  byte.
- **Steering still beats guessing**, by 9% on the picture and 30% on the sound. Where it starts still
  matters: from the gradient, the sound stayed at 151,956.

## What the two cars say

Turning a few fixed numbers for a whole file finds what the rope's search already finds, because
the rope's search is itself a walk over a few such numbers (its waves, phases and kept points), and
it measures the real cost of each where this counts order-0 entropy. Each steered prediction above
also came out larger than the rope's own code of the same file (the picture 44,729, the sound
130,094, by `sieve rope`). What can do better is a prediction that changes as the file goes: mixing
weighs its inputs afresh at every bit, and makes the picture 42,488 bytes at its shape (`sieve mix
--picture 256x1`, B16), 5% under the rope; on the made-up sound it makes 146,396, and the rope stays
smaller. Steering stays useful as a way to search by hand: the Manual Navigation screen, where every
byte of the file is a wheel.
