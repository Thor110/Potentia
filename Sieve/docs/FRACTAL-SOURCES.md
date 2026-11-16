# The fractal dimension's sources

Where each charted fractal (`data/fractals/chart.txt`, SPECIFICATIONS 12.0t) was transcribed from, and how its
source's notation was mapped onto `sieve-fractal-v1`, `-v2`, `-v3` or `-v4`: lists X2, X3 and X4, 16 November 2026. The
chart's `source` line names the source; the entries below give the rules as transcribed and every change made.
The three written for Sieve from a construction (the orb web, the Pythagoras tree and the H tree) are in
the chart alone. Eight X2 entries were written again in v3 by X3, exactly or to six decimals where X2 had
rounded them (SPECIFICATIONS 12.0t): Ken Philip's plant 07 (`360/14`), ABOP's branching pattern (0.686813),
Mandelbrot's Peano2 (0.707107), compound leaf 2 (0.735294), Penrose's kites and darts (0.618034) and the
spiral tiling (0.765367), with the Pythagoras tree and the H tree; their entries below are as X2 transcribed them.

Sources read: *The Algorithmic Beauty of Plants* (Prusinkiewicz and Lindenmayer, 1990; the 2004 electronic
edition), Wikipedia (as raw wikitext, November 2026), Fractint's `fractint.l` with its help text and `lsys.c`,
and Herb Savage's `tiling.l` from the Iterated Dynamics distribution. Paul Bourke's pages and the OEIS could
not be reached, so nothing is taken from them.

## Notation

What the Fractint documentation and code say about its symbols (checked, because v3 now maps them):
- \n and /n. The help text lists "\nn Increase angle nn degrees" and "/nn Decrease angle nn degrees", and "+ Increase angle", "- Decrease angle". So \ turns the same way as +, and / the same way as -. The code agrees: bslash and doplus both decrease the angle value, and slash and dominus both increase it. Fractint's y axis points down the screen, so a decreasing angle is a counterclockwise turn on screen. So + and \ are left turns and - and / are right turns, the same as our + (left) and - (right). So \n maps to a left turn of n degrees ("define D turn n") and /n to a right turn ("define D turn -n").
- Caution 1: in Fractint, \ and / change a separate heading (realangle) that is used only by D (draw) and M (move), while + - | change the heading used by F and G. The two never mix in Fractint. Every source below that uses \ or / uses only D and M (DoubleCesaro, VoderbergTile), so mapping both to one turtle heading is exact.
- Caution 2: Fractint's ! reverses \ and / as well as + and -. The Sieve turtle (core/src/fractal.cpp) swaps only + and -; a defined digit turn always turns the same way. So a \n under ! cannot be written as a digit turn. DoubleCesaro is therefore written with ANGLE 6 and repeated + and - (see its entry).
- Several rules for one symbol: Fractint does not choose between them. It appends them into one successor (help text: "all of the strings will be added together"; append_rule in lsys.c). X2's transcription took Snowflake2 and SnowflakeColor to be stochastic. That was wrong; they are deterministic.
- @Qx and @IQx: @QI3 means the same as @IQ3 (getnumber reads Q and I in either order).
- Characters with no meaning (in Fractint: anything other than F G D M + - | ! @ \ / [ ] C < >) draw nothing and stay in the string. Where they are dropped below, it is said.

General mapping conventions:
- Fractint "Angle n" means 360/n degrees. It is written as an exact decimal where there is one (8 -> 45), otherwise as 360/n.
- Fractint is case-insensitive. Its lowercase f is the drawing F, and its G (either case) moves without drawing, so it maps to our f.
- Empty rules ("F="): our WORD must be 1 to 255 symbols, so X -> (empty) is written X -> E, with E a letter that has no rule and does nothing. E is used for every empty rule here, because in each case the deleted symbol draws, and the old segments must vanish at the next step as they do in the source. Leaving X's rule out would keep them, which would draw a different picture.
- A Fractint scale pair @k ... @Ik is written ' ... " (or " ... ' for k < 1) with SCALE = 1/k (or k) rounded to 6 decimals. ' and " cancel exactly, so the step does not drift; only k itself is rounded, and each entry gives the rounding. The digit trick 1F2 (1 = scale p, 2 = scale 1/p) is exact only when both p and 1/p have at most 6 decimals (p = 2^a 5^b, e.g. 0.5, 0.8, 0.25). For p = 0.3, 0.7, 1/1.456 and similar, 1/p is a repeating decimal, so the pair leaves a factor slightly different from 1 after every use, and that error builds up along the path. Where ' and " can be used instead they are, and parametric ABOP figures that need two or more such ratios are listed at the end as not exact.
- ABOP's 3D symbols + - & ^ \ / | are ours unchanged; its wedge-shaped pitch-up sign is written ^.

## X2: the fractals sieve-fractal-v1 and v2 can write

id: quadratic-koch-island
title: Quadratic Koch island (Mandelbrot p.51)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), section 1.3, fig. 1.6
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: F-F+F+FF-F-F+F
mapping: none (also identical to Fractint "Koch3")
known dimension: 1.5 (log 8 / log 4; Wikipedia list, quadratic von Koch curve type 2 / Minkowski sausage)

id: quadratic-koch-island-18
title: Quadratic Koch island, 18-segment generator (Mandelbrot p.52)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.7a
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: F+FF-FF-F-F+F+FF-F-F+F+FF+FF-F
mapping: none. The source prints the successor over two lines; it is joined here. It matches Fractint "QuadKoch" (axiom F-F-F-F-), and Fractint "Koch6" is its mirror image.
known dimension: 1.6131 (ln 18 / ln 6; Wikipedia, Koch snowflake variants table, "Quadratic island")

id: quadratic-koch-curve-type1
title: Quadratic Koch curve, type 1 (quadratic modification of the snowflake curve)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.7b
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: -F
rule F: F+F-F-F+F
mapping: none (Wikipedia, L-system, Example 4 gives the same rule with axiom F)
known dimension: 1.4649 (log 5 / log 3; Wikipedia list, quadratic von Koch curve type 1)

id: islands-and-lakes
title: Combination of islands and lakes (Mandelbrot p.121)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.8
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F+F+F+F
rule F: F+f-FF+F+FF+Ff+FF-f+FF-F-FF-Ff-FFF
rule f: ffffff
mapping: none
known dimension:

id: abop-koch-1-9a
title: Koch curve variant (ABOP 1.9a)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9a
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: FF-F-F-F-F-F+F
mapping: none
known dimension:

id: abop-koch-1-9b
title: Koch curve variant (ABOP 1.9b)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9b
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: FF-F-F-F-FF
mapping: none
known dimension:

id: abop-koch-1-9c
title: Koch curve variant (ABOP 1.9c)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9c
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: FF-F+F-F-FF
mapping: none
known dimension:

id: abop-koch-1-9d
title: Koch curve variant (ABOP 1.9d)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9d
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: FF-F--F-F
mapping: none
known dimension:

id: abop-koch-1-9e
title: Koch curve variant (ABOP 1.9e)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9e
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: F-FF--F-F
mapping: none (Fractint "Curve3" is the same rule with axiom F-F-F-F-)
known dimension:

id: abop-koch-1-9f
title: Koch curve variant (ABOP 1.9f)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.9f
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: F-F+F-F-F
mapping: none
known dimension:

id: quadratic-gosper-e-curve
title: Quadratic Gosper curve (McKenna's E-curve)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.11b
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: -G
rule F: FF-G-G+F+F-G-GF+G+FFG-F+G+FF+G-FG-G-F+F+GG-
rule G: +FF-G-G+F+FG+F-GG-F-G+FGG-F-GF+F+G-G-F+F+GG
mapping: ABOP's edge symbols Fl -> F and Fr -> G (both draw). The source prints each successor over three lines; they are joined here. This matches Fractint "QuadGosper".
known dimension:

id: fass-curve-3x3
title: FASS curve, 3x3 macrotile
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.16a
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: -L
rule L: LF+RFR+FL-F-LFLFL-FRFR+
rule R: -LFLF+RFRFR+F+RF-LFL-FR
mapping: none (L and R are non-drawing node symbols; same as Fractint "Fass1")
known dimension:

id: fass-curve-4x4
title: FASS curve, 4x4 macrotile
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.16b
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: -L
rule L: LFLF+RFR+FLFL-FRF-LFL-FR+F+RF-LFL-FRFRFR+
rule R: -LFLFLF+RFR+FL-F-LF+RFR+FLF+RFRF-LFL-FRFR
mapping: none. The source wraps each successor over two lines; they are joined here. Same as Fractint "Fass2".
known dimension:

id: peano-curve
title: Peano curve (FASS, 3x3 diagonal tiles)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.17a
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 90
axiom: L
rule L: LFRFL-F-RFLFR+F+LFRFL
rule R: RFLFR+F+LFRFL-F-RFLFR
mapping: none. This is the same curve as Wikipedia's Peano curve L-system (X -> XFYFX+F+YFXFY-F-XFYFX, Y -> YFXFY-F-XFYFX+F+YFXFY), which is also Fractint "Peano3".
known dimension: 2 (Wikipedia list, Peano curve)

id: abop-plant-1-24a
title: Plant-like structure (ABOP 1.24a)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24a
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 25.7
axiom: F
rule F: F[+F]F[-F]F
mapping: none (ABOP gives δ=25.7°, n=5)
known dimension:

id: abop-plant-1-24b
title: Plant-like structure (ABOP 1.24b)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24b
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 20
axiom: F
rule F: F[+F]F[-F][F]
mapping: none (n=5)
known dimension:

id: abop-plant-1-24c
title: Plant-like structure / bush (ABOP 1.24c)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24c
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 22.5
axiom: F
rule F: FF-[-F+F+F]+[+F-F-F]
mapping: none (n=4; the successor wraps over two lines in the source)
known dimension:

id: abop-plant-1-24d
title: Plant-like structure (ABOP 1.24d)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24d
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 20
axiom: X
rule X: F[+X]F[-X]+X
rule F: FF
mapping: none (n=7)
known dimension:

id: abop-plant-1-24e
title: Plant-like structure (ABOP 1.24e)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24e
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 25.7
axiom: X
rule X: F[+X][-X]FX
rule F: FF
mapping: none (n=7)
known dimension:

id: abop-plant-1-24f
title: Plant-like structure (ABOP 1.24f)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.24f
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 22.5
axiom: X
rule X: F-[[X]+X]+F[+FX]-X
rule F: FF
mapping: none (n=5). This is the mirror image, at a different angle, of the charted Wikipedia "fractal plant".
known dimension:

id: abop-branching-pattern
title: Space-filling branching pattern (H-tree-like), ABOP eq. 1.9
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), section 1.10.3, eq. (1.9), fig. 1.39
url: http://algorithmicbotany.org/papers/abop/abop.pdf
family: lsystem
angle: 85
scale: 0.687
axiom: A
rule A: F[+"A][-"A]
mapping: The source is parametric: #define R 1.456, ω: A(1), A(s) -> F(s)[+A(s/R)][-A(s/R)], δ = 85°. The length parameter is replaced by the scale symbol. Each child A(s/R) becomes "A inside its branch, and SCALE = 1/1.456 = 0.6868, rounded to 0.687. Brackets restore the length, so siblings keep the parent's length, as in the source.
known dimension:

id: compound-leaf-alternating
title: Compound leaf with alternating branches (ABOP 5.12a)
source: Fractint fractint.l "Leaf2" (Adrian Mariano), from Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants, fig. 5.12a p.130
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 45
scale: 0.735
axiom: A
rule A: F[+X]FB
rule B: F[-Y]FA
rule X: A
rule Y: B
rule F: 'F"
mapping: Fractint "angle 8" gives 45°. Lowercase a, b, x, y and f are uppercased (Fractint is case-insensitive, and f is its drawing F). f=@1.36f@i1.36 is written as F -> 'F" with SCALE = 1/1.36 = 0.7353, rounded to 0.735. The ABOP original is parametric with R = 1.36 and apical delay D = 1.
known dimension:

id: sierpinski-curve
title: Sierpiński curve
source: Wikipedia, Sierpiński curve, "Representation as Lindenmayer system"
url: https://en.wikipedia.org/wiki/Sierpi%C5%84ski_curve
family: lsystem
angle: 45
axiom: F--XF--F--XF
rule X: XF+G+XF--F--XF+G+X
mapping: none (F and G both draw; the source notes the curve is usually drawn with different lengths for F and G, which is not supported here)
known dimension: 2 (Wikipedia list)

id: sierpinski-square-curve
title: Sierpiński square curve
source: Wikipedia, Sierpiński curve, "Representation as Lindenmayer system" (square curve)
url: https://en.wikipedia.org/wiki/Sierpi%C5%84ski_curve
family: lsystem
angle: 90
axiom: F+XF+F+XF
rule X: XF-F+F-XF+F+XF-F+F-X
mapping: none
known dimension: 2

id: sierpinski-arrowhead
title: Sierpiński arrowhead curve
source: Wikipedia, Sierpiński curve, "Arrowhead curve: Representation as Lindenmayer system"
url: https://en.wikipedia.org/wiki/Sierpi%C5%84ski_curve#Arrowhead_curve
family: lsystem
angle: 60
axiom: XF
rule X: YF+XF+Y
rule Y: XF-YF-X
mapping: none. The source writes the rules with spaces ("YF + XF + Y"); the spaces are removed. Equivalent forms: Wikipedia L-system "A -> B-A-B, B -> A+B+A" (A and B both draw); ABOP fig. 1.10b (Fr; Fl -> Fr+Fl+Fr, Fr -> Fl-Fr-Fl, δ=60°); Fractint "Curve4".
known dimension: 1.5850 (log2 3; Wikipedia list)

id: moore-curve
title: Moore curve
source: Wikipedia, Moore curve, "Representation as Lindenmayer system"
url: https://en.wikipedia.org/wiki/Moore_curve
family: lsystem
angle: 90
axiom: LFL+F+LFL
rule L: -RF+LFL+FR-
rule R: +LF-RFR-FL+
mapping: none. Wikipedia here defines - as left and + as right, the opposite of our grammar. Kept verbatim, so the drawing is the mirror image, which is the same shape since the Moore curve is mirror-symmetric.
known dimension: 2

id: peano-curve-switchback
title: Peano curve (generator form, "construction" variant)
source: Wikipedia, Peano curve, "L-system construction" (second system)
url: https://en.wikipedia.org/wiki/Peano_curve
family: lsystem
angle: 90
axiom: F
rule F: F+F-F-FF-F-F-FF
mapping: none. The source says + is clockwise; kept verbatim, so the drawing is the mirror image.
known dimension: 2

id: twindragon
title: Twindragon (Davis-Knuth dragon)
source: Wikipedia, Dragon curve, "Twindragon"
url: https://en.wikipedia.org/wiki/Dragon_curve#Twindragon
family: lsystem
angle: 90
axiom: FX+FX+
rule X: X+YF
rule Y: FX-Y
mapping: none
known dimension: boundary 1.5236 (Wikipedia list, boundary of the twindragon)

id: terdragon
title: Terdragon
source: Wikipedia, Dragon curve, "Terdragon"
url: https://en.wikipedia.org/wiki/Dragon_curve#Terdragon
family: lsystem
angle: 120
axiom: F
rule F: F+F-F
mapping: none
known dimension: 2 (curve); boundary 1.2619 (log3 4) (Wikipedia list)

id: fractal-binary-tree
title: Fractal (binary) tree
source: Wikipedia, L-system, "Example 2: fractal (binary) tree"
url: https://en.wikipedia.org/wiki/L-system
family: lsystem
angle: 45
axiom: G
rule F: FF
rule G: F[+G]-G
mapping: The source uses axiom 0 and rules 1 -> 11, 0 -> 1[0]0. In the source, 0 draws a segment ending in a leaf and 1 draws a segment; "[" means push and turn left 45°, and "]" means pop and turn right 45°. Mapped as 1 -> F, 0 -> G (both draw; the leaf is not drawn), "[" -> "[+" and "]" -> "]-".
known dimension:

id: cantor-set-lsystem
title: Cantor set (L-system form)
source: Wikipedia, L-system, "Example 3: Cantor set"
url: https://en.wikipedia.org/wiki/L-system
family: lsystem
angle: 90
axiom: F
rule F: FfF
rule f: fff
mapping: The source's A ("draw forward") -> F and B ("move forward") -> f. Source rules: A -> ABA, B -> BBB, axiom A. No angle is needed; 90 is a placeholder. Same as Fractint "CantorDust" (F=FGF, G=GGG). This is the L-system counterpart of the already-charted Cantor substitution.
known dimension: 0.6309 (log3 2)

id: koch-30-degree-island
title: Koch island at 30° (Fractint Koch2, after Mandelbrot)
source: Fractint fractint.l, "Koch2" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 30
axiom: F---F---F---F
rule F: -F+++F---F+
mapping: Fractint "Angle 12" gives 30°.
known dimension:

id: koch-30-degree-triangle
title: Koch curve on a 30° lattice (Fractint Koch4, after Mandelbrot)
source: Fractint fractint.l, "Koch4" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 30
axiom: F++++F++++F
rule F: +F--F++F-
mapping: Fractint "angle 12" gives 30°. Lowercase f is uppercased, since it is the drawing F in Fractint.
known dimension:

id: koch5-quadratic
title: Quadratic Koch variant (Fractint Koch5)
source: Fractint fractint.l, "Koch5"
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
axiom: F+F+F+F
rule F: F+F-F-FFF+F+F-F
mapping: Fractint "angle 4" gives 90°. The lowercase f in the axiom is uppercased (case-insensitive).
known dimension:

id: peano-mandelbrot-1
title: Peano curve (Mandelbrot form, Fractint Peano1)
source: Fractint fractint.l, "Peano1" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
axiom: F-F-F-F
rule F: F-F+F+F+F-F-F-F+F
mapping: Fractint "Angle 4" gives 90°.
known dimension:

id: peano-mandelbrot-2
title: Peano-type curve with √2 steps (Fractint Peano2, after Mandelbrot)
source: Fractint fractint.l, "Peano2" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 45
scale: 0.707
axiom: FXY++F++FXY++F
rule X: XY'-F"-FXY++F++FXY
rule Y: -'F-"FXY
mapping: Fractint "Angle 8" gives 45°. @Q2 (multiply by √2) -> ' and @IQ2 -> ", with SCALE = 1/√2, rounded to 0.707.
known dimension:

id: mandelbrot-island-1
title: Island with lakes (Fractint Island1, after Mandelbrot)
source: Fractint fractint.l, "Island1" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
scale: 0.125
axiom: F+F+F+F
rule F: FFFF-F+F+F-F[-fFF+F+FF+F]FF
rule f: 'f"
mapping: Fractint "angle 4" gives 90°. Fractint's G (move without drawing) -> f. G=@8G@I8 is written as f -> 'f" with SCALE = 1/8 = 0.125, which is exact.
known dimension:

id: mandelbrot-quartet
title: Quartet curve (Fractint Quartet, after Mandelbrot)
source: Fractint fractint.l, "Quartet" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
axiom: FB
rule A: FBFA+HFA+FB-FA
rule B: FB+FA-FB-JFBFA
rule F: E
rule H: -
rule J: +
mapping: Fractint "angle 4" gives 90°. Axiom "fb" is uppercased. Source "F=" (delete) uses empty-rule emulation, F -> E with E inert. H and J are letters that turn into - and + on the next step.
known dimension:

id: fractint-tree1
title: Binary tree with 0.6 scaling (Fractint Tree1, after Mandelbrot)
source: Fractint fractint.l, "Tree1" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 30
scale: 0.6
axiom: +++FX
rule X: "[-FX]+FX
mapping: Fractint "angle=12" gives 30°. @.6 -> " with SCALE 0.6, which is exact.
known dimension:

id: sierpinski-gasket-node
title: Sierpinski gasket, node-rewriting form (Fractint Sierpinski1)
source: Fractint fractint.l, "Sierpinski1" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 120
axiom: F
rule F: FXF
rule X: +FXF-FXF-FXF+
mapping: Fractint "angle 3" gives 120°.
known dimension: 1.5850 (log2 3; Sierpinski triangle)

id: sierpinski-gasket-sfi
title: Sierpinski gasket (The Science of Fractal Images form, Fractint Sierpinski2)
source: Fractint fractint.l, "Sierpinski2" (from The Science of Fractal Images)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 60
axiom: FXF--FF--FF
rule F: FF
rule X: --FXF++FXF++FXF--
mapping: Fractint "angle 6" gives 60°. Lowercase rule letters f and x are uppercased.
known dimension: 1.5850

id: sierpinski-gasket-branching
title: Sierpinski gasket by branching (Fractint Sierpinski3)
source: Fractint fractint.l, "Sierpinski3" (from Jim Hanan via Corbit)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 120
axiom: F-F-F
rule F: F[-F]F
mapping: Fractint "angle 3" gives 120°. The lowercase f rule is uppercased.
known dimension: 1.5850

id: sierpinski-square
title: Square Sierpinski (Fractint SierpinskiSquare / Square)
source: Fractint fractint.l, "SierpinskiSquare" (also "Square", Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
axiom: F+F+F+F
rule F: FF+F+F+F+FF
mapping: Fractint "angle 4" gives 90°.
known dimension:

id: square-gasket
title: Square gasket (Fractint SquareGasket)
source: Fractint fractint.l, "SquareGasket" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 90
axiom: X
rule X: +FXF+FXF+FXF+FXF
rule F: FF
mapping: Fractint "Angle 4" gives 90°.
known dimension:

id: triangular-gasket
title: Triangular gasket (Fractint ColorTriangGasket)
source: Fractint fractint.l, "ColorTriangGasket" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 60
axiom: --X
rule X: ++FXF++FXF++FXF
rule F: FF
mapping: Fractint "Angle 6" gives 60°. The trailing colour-increment symbol ">1" is dropped from X's rule; it affects colour only.
known dimension:

id: philip-plant-07
title: Plant (Ken Philip, The Science of Fractal Images p.285b)
source: Fractint fractint.l, "Plant07" (Ken Philip, from The Science of Fractal Images p.285)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 25.7
axiom: Z
rule Z: ZFX[+Z][-Z]
rule X: X[-FFF][+FFF]FX
mapping: Fractint "angle 14" gives 360/14 = 25.714°, rounded to 25.7. Lowercase rule letters z and x are uppercased.
known dimension:

id: philip-plant-08
title: Plant (Ken Philip, The Science of Fractal Images p.286)
source: Fractint fractint.l, "Plant08" (Ken Philip, from The Science of Fractal Images p.286)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 18
axiom: SLFFF
rule S: [+++Z][---Z]TS
rule Z: +H[-Z]L
rule H: -Z[+H]L
rule T: TL
rule L: [-FFF][+FFF]F
mapping: Fractint "angle 20" gives 18°. Lowercase rule letters are uppercased.
known dimension:

id: penrose-rhomb-tiling
title: Penrose tiling, P3 rhombs (Herb Savage)
source: Fractint fractint.l, "Penrose1" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers")
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 36
axiom: +WF--XF---YF--ZF
rule W: YF++ZF----XF[-YF----WF]++
rule X: +YF--ZF[---WF--XF]+
rule Y: -WF++XF[+++YF++ZF]-
rule Z: --YF++++WF[+ZF++++XF]--XF
rule F: E
mapping: Fractint "Angle 10" gives 36°. Source "F=" (delete) uses empty-rule emulation, F -> E with E inert.
known dimension: 2 (Wikipedia list, Penrose tiling)

id: penrose-rhomb-star
title: Penrose rhomb tiling, five-fold star start (Herb Savage)
source: Fractint fractint.l, "Penrose3" (Herb Savage)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 36
axiom: [X]++[X]++[X]++[X]++[X]
rule W: YF++ZF----XF[-YF----WF]++
rule X: +YF--ZF[---WF--XF]+
rule Y: -WF++XF[+++YF++ZF]-
rule Z: --YF++++WF[+ZF++++XF]--XF
rule F: E
mapping: As for penrose-rhomb-tiling. "Penrose4" is the same with [Y] in place of [X].
known dimension: 2

id: penrose-kites-and-darts
title: Penrose kites and darts, P2 (Herb Savage)
source: Iterated Dynamics tiling.l, "KitesAndDarts" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers")
url: https://raw.githubusercontent.com/LegalizeAdulthood/iterated-dynamics/master/home/lsystem/tiling.l
family: lsystem
angle: 36
scale: 0.618
axiom: Wf+Xf+Wf+Xf+Wf+Xf+Wf+Xf+Wf+X
rule W: [F][++'F][++f---"f|X-Y|f|W]
rule X: [F+++'F][++"fZ|X|-f|W]
rule Y: [+F]['F][+f"|Y+X]
rule Z: [-F]['F]["f--Wf|+Z]
rule F: E
mapping: Fractint "Angle 10" gives 36°. G (move) -> f. @1.618033989 -> ' and @.618033989 -> ", with SCALE = 0.618 (≈ 1/φ; the source uses 0.618033989). Source "F=" uses empty-rule emulation, F -> E. This assumes Fractint's [ ] saves the step size, as ours does.
known dimension: 2

id: spiral-tile
title: Spiral tiling (Herb Savage, after Gardner)
source: Iterated Dynamics tiling.l, "SpiralTile" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers")
url: https://raw.githubusercontent.com/LegalizeAdulthood/iterated-dynamics/master/home/lsystem/tiling.l
family: lsystem
angle: 22.5
scale: 0.765
axiom: X++X++X++X++|f|X++X++X++X
rule X: [FX+++++"F'[-----Y]+++++F]
rule Y: [F+++++"F'[-----Y]+++++F]
mapping: Fractint "Angle 16" gives 22.5°. G -> f. @.7653668647 -> " and @I.7653668647 -> ', with SCALE 0.765 (source 0.7653668647 = 2 sin 22.5°). The colour codes C12 and C10 are dropped.
known dimension:

id: sphinx-reptile
title: Sphinx rep-tile (Herb Savage)
source: Fractint fractint.l, "Sphinx" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers")
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 60
axiom: X
rule X: +FF-YFF+FF--FFF|X|F--YFFFYFFF|
rule Y: -FF+XFF-FF++FFF|Y|F++XFFFXFFF|
rule F: ff
rule f: ff
mapping: Fractint "Angle 6" gives 60°. Fractint's G (move without drawing) -> f, so F=GG becomes F -> ff and G=GG becomes f -> ff. As in the source, edges from earlier generations turn into invisible moves.
known dimension:

id: pentaplexity
title: Pentaplexity (Penrose)
source: Fractint fractint.l, "PentaPlexity" (Penrose's construction translated to an L-system by Joe Saverino)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 36
axiom: F++F++F++F++F
rule F: F++F++F|F-F++F
mapping: Fractint "Angle 10" gives 36°.
known dimension:

id: circular-tile
title: Circular tile (Fractint CircularTile)
source: Fractint fractint.l, "CircularTile" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 15
axiom: X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X+X
rule X: [F+F+F+F[---X-Y]+++++F++++++++F-F-F-F]
rule Y: [F+F+F+F[---Y]+++++F++++++++F-F-F-F]
mapping: Fractint "angle 24" gives 15°. Lowercase rule letters x and y are uppercased.
known dimension:

id: lars-cross-tree
title: Four-way binary branching (Fractint Lars1)
source: Fractint fractint.l, "Lars1" (Jonathan Osuch, after Lars Vangsgaard)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
angle: 45
axiom: [F]++[F]++[F]++F
rule F: F[+F][-F]
mapping: Fractint "Angle 8" gives 45°.
known dimension:

id: period-doubling
title: Period-doubling sequence
source: Wikipedia, Automatic sequence, "Period-doubling sequence" (OEIS A096268; Allouche & Shallit 2003 p.176)
url: https://en.wikipedia.org/wiki/Automatic_sequence
family: substitution
axiom: 0
rule 0: 01
rule 1: 00
mapping: none
known dimension:

id: rudin-shapiro
title: Rudin-Shapiro sequence
source: Wikipedia, Rudin–Shapiro sequence and Morphic word (Lothaire 2005 p.525); OEIS A020985
url: https://en.wikipedia.org/wiki/Rudin%E2%80%93Shapiro_sequence
family: substitution
axiom: a
rule a: ab
rule b: ac
rule c: db
rule d: dc
mapping: none. The source then applies a coding: a,b -> +1 and c,d -> -1 on the Rudin–Shapiro page, or a,b -> 0 and c,d -> 1 on Morphic word. The coding is not part of the substitution.
known dimension:

id: regular-paperfolding
title: Regular paperfolding (dragon curve) sequence
source: Wikipedia, Morphic word (Lothaire 2005 p.526); OEIS A014577 (cited on Wikipedia, Regular paperfolding sequence)
url: https://en.wikipedia.org/wiki/Morphic_word
family: substitution
axiom: a
rule a: ab
rule b: cb
rule c: ad
rule d: cd
mapping: none. The source then applies the coding a,b -> 0 and c,d -> 1. The Regular paperfolding sequence page instead gives a pair-rewriting form (11 -> 1101, 01 -> 1001, 10 -> 1100, 00 -> 1000), which is not a single-letter morphism.
known dimension:

id: baum-sweet
title: Baum–Sweet sequence
source: Wikipedia, Baum–Sweet sequence (Allouche & Shallit 2003 p.176); OEIS A086747
url: https://en.wikipedia.org/wiki/Baum%E2%80%93Sweet_sequence
family: substitution
axiom: a
rule a: ab
rule b: cb
rule c: bd
rule d: dd
mapping: The source's uppercase A, B, C, D are lowercased to a, b, c, d (only lowercase letters are allowed). The coding A,B -> 1 and C,D -> 0 is not part of the substitution.
known dimension:

id: tribonacci-word
title: Tribonacci word (Rauzy map)
source: Wikipedia, Rauzy fractal, "Tribonacci word" (Lothaire 2005 p.525; Pytheas Fogg 2002 p.232)
url: https://en.wikipedia.org/wiki/Rauzy_fractal#Tribonacci_word
family: substitution
axiom: 1
rule 1: 12
rule 2: 13
rule 3: 1
mapping: none (Wikipedia, Morphic word gives the same map over a, b, c: a -> ab, b -> ac, c -> a)
known dimension: boundary of the associated Rauzy fractal 1.0933 (Wikipedia list)

id: pell-silver-word
title: Pell (silver-ratio) word
source: Wikipedia, Ammann–Beenker tiling, "Pell and silver ratio features"
url: https://en.wikipedia.org/wiki/Ammann%E2%80%93Beenker_tiling
family: substitution
axiom: a
rule a: aba
rule b: a
mapping: The source's R -> RrR; r -> R uses an uppercase letter, so R -> a and r -> b. The source gives no axiom; "a" (R) is used. No OEIS number was given on the page, and OEIS itself was unreachable.
known dimension:

## X3: those sieve-fractal-v3 can write

id: cesaro-fractint
title: Cesaro curve (Fractint Cesaro, after Mandelbrot)
source: Fractint fractint.l, "Cesaro" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 360/34
axiom: FX
rule F: E
rule X: ----F!X!++++++++F!X!----
mapping: Fractint "Angle 34" is 360/34 degrees, written exactly as 360/34. Source "F=" (delete) uses empty-rule emulation, F -> E with E an inert letter that has no rule: F cannot simply be left without a rule, because the F segments of each generation must disappear at the next step, as in the source. ! is kept as is: Fractint's ! swaps + and - and is saved by [ ], as ours is.
known dimension: 

id: double-cesaro-fractint
title: Double Cesaro curve (Fractint DoubleCesaro, after Mandelbrot)
source: Fractint fractint.l, "DoubleCesaro" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 6
axiom: F+++++++++++++++F+++++++++++++++F+++++++++++++++F+++++++++++++++
rule F: +++++++!F!--------------!F!+++++++
mapping: The source draws only with D (draw, using Fractint's separate degree heading) and turns only with \n and /n, so its "Angle 4" is never used. D -> F. Fractint's \n turns the same way as + (left) and /n the same way as - (right) (see the note at the top). All its turns are multiples of 6 degrees, so ANGLE is set to 6 and \42 -> 7 "+", /84 -> 14 "-", \90 -> 15 "+". This is exact, and it keeps the source's ! working: Fractint's ! swaps \ and /, and our ! swaps + and -. Digit turns were NOT used, because the Sieve turtle (core/src/fractal.cpp) does not let ! reverse a digit turn, so "define 1 turn 42" inside !...! would turn the wrong way. Source: axiom D\90D\90D\90D\90, D=\42!D!/84!D!\42.
known dimension: 

id: mandelbrot-snowflake-sweep-1
title: Mandelbrot snowflake sweep 1 (Fractint SnowFlake1)
source: Fractint fractint.l, "SnowFlake1" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 30
scale: 0.57735
axiom: FR
rule F: E
rule R: ++!FRFU++FU++FU!---'FU|-"!FRFU!
rule U: !FRFU!|+'FR"+++!FR--FR--FRFU!--
mapping: Fractint "Angle 12" gives 30 degrees. @Q3 (multiply by sqrt 3) -> ' and @IQ3 -> ", with SCALE = 1/sqrt 3 = 0.5773502692 rounded to 6 decimals (0.577350, written 0.57735 as the grammar forbids a trailing 0). Every @Q3 is matched by an @IQ3 in the same successor, and ' and " cancel exactly, so the only error is sqrt 3 approximated as 1/0.57735 = 1.7320516 (relative 5e-7). "F=" uses empty-rule emulation, F -> E. ! kept.
known dimension: 

id: mandelbrot-snowflake-sweep-2
title: Mandelbrot snowflake sweep 2 (Fractint Snowflake2)
source: Fractint fractint.l, "Snowflake2" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 30
scale: 0.57735
axiom: F
rule F: ++!F!F--F--F"|+F!F--F--F!+++'F"|+F!F'|+F!F
mapping: Fractint "angle 12" gives 30 degrees. The source gives two "F=" lines. They are not alternatives: Fractint appends a second rule for the same symbol to the first (help text: "all of the strings will be added together"; append_rule in lsys.c), so the single rule here is the two strings joined: ++!F!F--F--F@IQ3|+F!F-- followed by F--F!+++@Q3F@QI3|+F!F@Q3|+F!F. @Q3 -> ', @IQ3 and @QI3 (the same: getnumber reads Q and I in either order) -> ", SCALE 0.57735 as in mandelbrot-snowflake-sweep-1. The scale symbols balance over the whole successor (/, x, /, x), so they cancel exactly. ! kept. X2's transcription's note that the source is stochastic was wrong.
known dimension: 

id: mandelbrot-snowflake-sweep-3
title: Mandelbrot snowflake sweep 3 (Fractint SnowFlake3)
source: Fractint fractint.l, "SnowFlake3" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 30
scale: 0.57735
axiom: FX
rule F: E
rule X: ++F!X!FY--FX--FY|+"FYF!X!++F!Y!++F!Y!FX'+++F!Y!FX
rule Y: FYF!X!+++"FYF!X!++F!X!++F!Y!FX'|+FX--FY--FXF!Y!++
mapping: Fractint "angle 12" gives 30 degrees. The source is lowercase (fx, x=, y=, f=); Fractint is case-insensitive, so x, y and the drawing f are uppercased. @iq3 -> " and @q3 -> ', SCALE 0.57735 (1/sqrt 3 rounded), balanced within each successor. "f=" uses empty-rule emulation, F -> E. ! kept.
known dimension: 

id: mandelbrot-snowflake-sweep-color
title: Mandelbrot snowflake sweep, colour version, geometry only (Fractint SnowflakeColor)
source: Fractint fractint.l, "SnowflakeColor" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 30
scale: 0.57735
axiom: F
rule F: --!F!F++F++F"|-F!F++F++F!---'F"|-F!F'|-F!F
mapping: Fractint "angle 12" gives 30 degrees. Every colour step "<1" is dropped (colour only). The source's three "F=" lines are appended into one rule as Fractint does (the third, "F=", is empty and adds nothing). @IQ3 and @QI3 -> ", @Q3 -> ', SCALE 0.57735. ! kept. The geometry is the mirror image of mandelbrot-snowflake-sweep-2 (+ and - exchanged).
known dimension: 

id: pentagram-fractint
title: Pentagram fractal (Fractint Pentagram)
source: Fractint fractint.l, "Pentagram" (created by Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 36
scale: 0.618034
axiom: FX++FX++FX++FX++FX
rule X: [++++"F"F!X!'F]
mapping: Fractint "angle 10" gives 36 degrees. The drawing f is uppercased. @i1.618033989 (x 0.6180339887) -> " and @.618033989 -> ", both with SCALE 0.618034 (each value rounded to 6 decimals); @i.618033989 (x 1.618033989) -> ', which divides by 0.618034 (1.6180330, relative error 6e-7). The source's commented-out rule "; f=f[++++@1.618033989f]" is not part of the system, so F has no rule. ! kept.
known dimension: 

id: compound-leaf-delayed
title: Compound leaf with alternating branches and delay (ABOP 5.12b, Fractint Leaf1)
source: Fractint fractint.l, "Leaf1" (Adrian Mariano), from Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants, fig. 5.12b p.130
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 45
scale: 0.847458
axiom: X
rule A: N
rule B: E
rule E: H
rule F: 'F"
rule H: J
rule J: Y
rule N: O
rule O: P
rule P: X
rule X: F[+A]FY
rule Y: F[-B]FX
mapping: Fractint "angle 8" gives 45 degrees. Lowercase rule letters (a, n, o, p, b, e, h, j, x, y) are uppercased (Fractint is case-insensitive). In "x=F[+A(4)]Fy" and "y=F[-B(4)]Fx" the text "(4)" is dropped: in Fractint "(", "4" and ")" are plain characters that draw nothing and have no rules (a number is only read after @, \, /, C, < or >), so they are inert. The delay of ABOP's A(4) is done in the source by the chains a->n->o->p->x and b->e->h->j->y, kept here. F=@1.18F@i1.18 -> F: 'F" with SCALE = 1/1.18 = 0.8474576 rounded to 0.847458; ' and " cancel exactly, and 1.18 becomes 1/0.847458 = 1.1799995.
known dimension: 

id: dragon-mandelbrot-form
title: Heighway dragon, Mandelbrot's form (Fractint Dragon)
source: Fractint fractint.l, "Dragon" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature); also the sample L-system in Fractint's help text
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 45
axiom: FX
rule F: E
rule X: -FX++FY-
rule Y: +FX--FY+
mapping: Fractint "Angle 8" gives 45 degrees. Lowercase rule letters x and y are uppercased. "F=" uses empty-rule emulation, F -> E. Same curve as the charted dragon-curve, in Mandelbrot's 45-degree node-rewriting form.
known dimension: 2 (curve); boundary 1.5236 (Wikipedia list, Heighway dragon)

id: flowsnake-mandelbrot-form
title: Flowsnake (Gosper curve), Mandelbrot's form (Fractint FlowSnake)
source: Fractint fractint.l, "FlowSnake" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 60
axiom: FL
rule F: E
rule L: FL-FR--FR+FL++FLFL+FR-
rule R: +FL-FRFR--FR-FL++FL+FR
mapping: Fractint "angle=6;" gives 60 degrees. Each source rule ends in a stray "\"," (L=FL-FR--FR+FL++FLFL+FR-", and R=...FL+FR"): in Fractint " is an ordinary inert character and the "," on the L line is also inert, so both are dropped; " must not be kept, since in our grammar it is the scale symbol. "F=" uses empty-rule emulation, F -> E. Same curve as the charted gosper-curve.
known dimension: 2 (curve; Wikipedia list, Gosper curve)

id: mandelbrot-island-2
title: Island with lakes, 6-scale moves (Fractint Island2, after Mandelbrot)
source: Fractint fractint.l, "Island2" (Adrian Mariano, from Mandelbrot, The Fractal Geometry of Nature)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 90
scale: 0.166667
axiom: F+F+F+F
rule F: F+fF-FF-F-FF+f+FF-fF+FF+F+FF-f-FFF
rule f: 'f"
mapping: Fractint "angle 4" gives 90 degrees. Fractint is case-insensitive, so the drawing f -> F and both g and G (move without drawing) -> f. g=@6G@I6 -> f: 'f" with SCALE = 1/6 rounded to 0.166667, an approximation: ' multiplies by 1/0.166667 = 5.999988 (relative error 2e-6); ' and " still cancel exactly, so there is no drift. ABOP fig. 1.8 (charted as islands-and-lakes) is the same island with f -> ffffff.
known dimension: 

id: penrose-rhomb-tiling-2
title: Penrose rhomb tiling, second start (Fractint Penrose2, Herb Savage)
source: Fractint fractint.l, "Penrose2" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers")
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 36
axiom: ++ZF----XF-YF----WF
rule F: E
rule W: YF++ZF----XF[-YF----WF]++
rule X: +YF--ZF[---WF--XF]+
rule Y: -WF++XF[+++YF++ZF]-
rule Z: --YF++++WF[+ZF++++XF]--XF
mapping: Fractint "Angle 10" gives 36 degrees. "F=" uses empty-rule emulation, F -> E. Rules as penrose-rhomb-tiling; only the axiom differs.
known dimension: 2 (Wikipedia list, Penrose tiling)

id: double-penrose
title: Double Penrose (Penrose3 and Penrose4 superimposed, Herb Savage)
source: Fractint fractint.l, "DoublePenrose" (Herb Savage)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 36
axiom: [X][Y]++[X][Y]++[X][Y]++[X][Y]++[X][Y]
rule F: E
rule W: YF++ZF----XF[-YF----WF]++
rule X: +YF--ZF[---WF--XF]+
rule Y: -WF++XF[+++YF++ZF]-
rule Z: --YF++++WF[+ZF++++XF]--XF
mapping: Fractint "Angle 10" gives 36 degrees. "F=" uses empty-rule emulation, F -> E.
known dimension: 

id: fractint-plant-10
title: Plant with paired branches (Fractint Plant10)
source: Fractint fractint.l, "Plant10" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 36
axiom: F
rule F: F[+FF][-FF]F[+FF][-FF]F
mapping: Fractint "angle 10" gives 36 degrees. The lowercase (drawing) f is uppercased.
known dimension: 

id: fractint-plant-11
title: Plant with nested branches (Fractint Plant11)
source: Fractint fractint.l, "Plant11" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 30
axiom: F
rule F: F[+F[+F][-F]F][-F[+F][-F]F]F[+F][-F]F
mapping: Fractint "angle 12" gives 30 degrees. The lowercase f in axiom and rule head is uppercased.
known dimension: 

id: fractint-mytree
title: Zigzag-branch tree (Fractint MyTree)
source: Fractint fractint.l, "MyTree" (Adrian Mariano)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 22.5
axiom: ++++F
rule F: FF-[XY]+[XY]
rule X: +FY
rule Y: -FX
mapping: Fractint "Angle 16" gives 22.5 degrees.
known dimension: 

id: daunis-man
title: Man (Fractint Man, Ivan Daunis)
source: Fractint fractint.l, "Man" (from Ivan Daunis; "looks like man with an odd number of iterations")
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 45
axiom: F++F++F++F
rule F: -F-FF+++F+FF-F
mapping: Fractint "Angle 8" gives 45 degrees.
known dimension: 

id: daunis-lace
title: Lace (Fractint Lace, Ivan Daunis)
source: Fractint fractint.l, "Lace" (from Ivan Daunis)
url: https://raw.githubusercontent.com/jhol/fractint/master/lsystem/fractint.l
family: lsystem
version: 3
angle: 45
axiom: F++F++F++F
rule F: F+++F---F+F---F++F--F++F
mapping: Fractint "Angle 8" gives 45 degrees.
known dimension: 

id: voderberg-spiral-tiling
title: Voderberg spiral tiling (Herb Savage)
source: Iterated Dynamics tiling.l, "VoderbergTile" (Herb Savage, based on Martin Gardner's "Penrose Tiles to Trapdoor Ciphers"; a spiral tiling by Heinz Voderberg)
url: https://raw.githubusercontent.com/LegalizeAdulthood/iterated-dynamics/master/home/lsystem/tiling.l
family: lsystem
version: 3
angle: 6
scale: 0.294687
define 1: turn 84.1
define 2: turn 46.37236
define 3: turn -46.37236
define 4: scale 4.783386
define 5: scale 0.209057
axiom: 1A++++++++++++++++4f5----------------A
rule A: X++X++X++X++X++X++X++X++X++X++X++X++X++X++X++Z
rule X: [F+++++++++++++F2'F"3F+++++++++++++++++++[++++++++++++++++++++++++++++X++++Y]F+++++++++++++F2'F"3F-------------F]
rule Y: [F+++++++++++++F2'F"3F-------------F++++++++++++++++++++++++++++[++++++++++++++++++++++++++++++++Y]F+++++++++++++F2'F"3F]
rule Z: [F+++++++++++++F2'F"3F+++++++++++++++++++F+++++++++++++F2'F"3F-------------F]
mapping: The source draws only with D and M (Fractint's degree-heading draw and move) and turns only with \n and /n, so its "Angle 30" is never used. D -> F, M -> f. \n turns like + (left), /n like - (right). Every turn that is a multiple of 6 degrees is written with ANGLE 6: \12 -> ++, \24 -> 4 "+", \78 -> 13 "+", /78 -> 13 "-", \96 -> 16 "+", /96 -> 16 "-", \114 -> 19 "+", \168 -> 28 "+", \192 -> 32 "+". The others use digits: \84.1 -> 1 (turn 84.1), \46.37236 -> 2 (turn 46.37236), /46.37236 -> 3 (turn -46.37236), all exact. @3.393427 -> ' and @I3.393427 -> ", with SCALE = 1/3.393427 = 0.29468735 rounded to 0.294687 (so ' multiplies by 3.3934306, relative error 1e-6; ' and " cancel exactly). @4.783386117 -> 4 (scale 4.783386, rounded from 4.783386117) and @I4.783386117 -> 5 (scale 0.209057, 1/4.783386117 = 0.20905693 rounded); these appear once, in the axiom, and leave a step factor of 4.783386 x 0.209057 = 1.0000005 on the second A. No ! is used, so digit turns are safe here.
known dimension: 

id: vertigo-spiral-46
title: Vertigo spiral, 46 steps a turn (Morgan Savage)
source: Iterated Dynamics tiling.l, "Vertigo1" (by Morgan Savage)
url: https://raw.githubusercontent.com/LegalizeAdulthood/iterated-dynamics/master/home/lsystem/tiling.l
family: lsystem
version: 3
angle: 360/46
scale: 0.9997
axiom: X
rule X: XF+"X
mapping: Fractint "Angle 46" is 360/46 degrees, written exactly. @.9997 -> " with SCALE 0.9997, exact (it is never undone, as in the source). The colour step ">1" is dropped. Source: X=X>1F+@.9997X. The source suggests order 13.
known dimension: 

id: vertigo-spiral-49
title: Vertigo spiral, 49 steps a turn (Morgan Savage)
source: Iterated Dynamics tiling.l, "Vertigo2" (by Morgan Savage)
url: https://raw.githubusercontent.com/LegalizeAdulthood/iterated-dynamics/master/home/lsystem/tiling.l
family: lsystem
version: 3
angle: 360/49
scale: 0.9997
axiom: X
rule X: XF+"X
mapping: Fractint "Angle 49" is 360/49 degrees, written exactly. @.9997 -> " with SCALE 0.9997, exact. The colour step ">1" is dropped.
known dimension: 

id: fass-curve-5x5
title: FASS curve, 5x5 diagonal macrotile
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.17b
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 45
axiom: L
rule L: L+F+R-F-L+F+R-F-L-F-R+F+L-F-R-F-L+F+R-F-L-F-R-F-L+F+R+F+L+F+R-F-L+F+R+F+L-R-F+F+L+F+R-F-L+F+R-F-L
rule R: R-F-L+F+R-F-L+F+R+F+L-F-R+F+L+F+R-F-L+F+R+F+L+F+R-F-L-F-R-F-L+F+R-F-L-F-R+F+L-F-R-F-L+F+R-F-L+F+R
mapping: none. The source prints each successor over two lines; they are joined here (99 symbols each). n=2 in the source. Checked against a 200 dpi render of the page.
known dimension: 2

id: hilbert-curve-3d
title: Three-dimensional Hilbert curve
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.19 (after Sharp et al. [139])
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 90
axiom: A
rule A: B-F+CFC+F-D&F^D-F+&&CFC+F+B//
rule B: A&F^CFB^F^D^^-F-D^|F^B|FC^F^A//
rule C: |D^|F^B-F+C^F^A&&FA&F^C+F+B^F^D//
rule D: |CFB-F+B|FA&F^A&&FB-F+B|FC//
mapping: ABOP's pitch-up symbol (printed as a wedge) -> ^. All other symbols (+ - & / |) are the same in both. n=2 in the source.
known dimension: 3 (space-filling in 3D)

id: abop-bush-3d
title: Three-dimensional bush-like structure
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.25
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 22.5
axiom: A
rule A: [&FLA]/////[&FLA]///////[&FLA]
rule F: S/////F
rule L: [^^-G+G+G-|-G+G+G]
rule S: FL
mapping: ABOP's ! (decrement segment width) and ' (next colour) are dropped: p1 A -> [&FL!A]/////'[&FL!A]///////'[&FL!A] loses its three ! and two ', and p4 L -> ['''^^{-f+f+f-|-f+f+f}] loses its three '. The wedge -> ^. The leaf is a filled polygon in the source ({ } with its edges as f); polygons are not available, so the braces are dropped and the polygon edges f are drawn as G, giving the leaf's outline. Spaces in the source ("S ///// F", "F L") are removed. n=7.
known dimension: 

id: abop-plant-3d-flowers
title: Three-dimensional plant with flowers
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.26
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 18
axiom: P
rule B: [&&&D/W////W////W////W////W]
rule D: FF
rule I: FS[//&&L][//^^L]FS
rule L: [+G-GG-G+|+G-GG-G]
rule P: I+[P+B]--//[--L]I[++L]-[PB]++PB
rule S: SFS
rule W: [^F][&&&&-G+G|-G+G]
mapping: Multi-letter modules become letters: plant -> P, internode -> I, seg -> S, leaf -> L, flower -> B, pedicel -> D, wedge -> W. Colour marks are dropped: ' in p4 (leaf), and the opening quote printed after pedicel in p5 and at the start of p7 (taken as ABOP's colour symbol; it does not affect geometry). Polygons: the braces are dropped and the polygon edges f are drawn as G (outline only). The wedge sign -> ^. The source's ff (typeset as a ligature) is two f. n=5.
known dimension: 

id: abop-stochastic-plant
title: Stochastic branching structure
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), section 1.7, fig. 1.27
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 25.7
seed: 1
axiom: F
rule F: F[+F]F[-F]F
rule F: F[+F]F
rule F: F[-F]F
mapping: The source's probabilities .33, .33 and .34 become equal probabilities 1/3 each (the grammar has no weights; the source itself says each production is chosen "with approximately the same probability of 1/3"). The source gives derivation length 5 but no angle for fig. 1.27; 25.7 degrees is taken from fig. 1.24a, whose deterministic rule F -> F[+F]F[-F]F is p1 here. This angle is an assumption, not printed with the figure. Seed 1 is our choice; each seed gives another specimen, as the six in the figure.
known dimension: 

id: abop-flower-field-plant
title: Stochastic flowering plant (one plant of the flower field)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), section 1.7, fig. 1.28 (stochastic modification of fig. 1.26)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 18
seed: 1
axiom: P
rule B: [&&&D/W////W////W////W////W]
rule D: FF
rule I: FS[//&&L][//^^L]FS
rule L: [+G-GG-G+|+G-GG-G]
rule P: I+[P+B]--//[--L]I[++L]-[PB]++PB
rule S: S[//&&L][//^^L]FS
rule S: SFS
rule S: S
mapping: As abop-plant-3d-flowers, with p3 (seg) replaced, as the source says, by three productions seg -> seg[//&&leaf][//^^leaf]Fseg (.33), seg -> segFseg (.33), seg -> seg (.34), here with equal probabilities 1/3. The field of 4 x 4 plants in the figure is not reproduced; this is the plant L-system. Seed 1 is our choice.
known dimension: 

id: hogeweg-hesper-a
title: Hogeweg and Hesper context-sensitive tree (a)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.31a (L-systems based on Hogeweg & Hesper 1974)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 22.5
ignore: !+-FQ
axiom: FbFbFb
rule Q: !Q
rule a<a>a: a
rule a<a>b: b[Q+QFbFb]
rule b<a>a: a
rule b<a>b: bFb
rule a<b>a: b
rule a<b>b: b
rule b<b>a: a
rule b<b>b: a
mapping: ABOP's letters 0 and 1 -> a and b (rule symbols must be letters; a and b draw nothing, as 0 and 1 do in the source). The source's #ignore: +-F becomes ignore !+-FQ (adding the two helper symbols below; ignore is written in ASCII order). The source also rewrites the turn symbols every step, * < + > * -> - and * < - > * -> +, which our grammar cannot do (only letters have rules). This is emulated exactly: each + or - written in a successor becomes Q+Q or Q-Q, and Q -> !Q, so after k further steps each Q has k ! in front of it. The first group of k ! flips the turn k times (k odd: + acts as -), and the second group, with the same number of !, restores the swap state, so the branch content and later turns are unaffected. Q and ! are ignored when contexts are matched, so the 2L context matches are the source's. The word contains extra Q and ! symbols but draws the same lines. The source's identity rules (e.g. 0<0>0 -> 0) are kept. n=30 in the source.
known dimension: 

id: hogeweg-hesper-b
title: Hogeweg and Hesper context-sensitive tree (b)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.31b (L-systems based on Hogeweg & Hesper 1974)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 22.5
ignore: !+-FQ
axiom: FbFbFb
rule Q: !Q
rule a<a>a: b
rule a<a>b: b[Q-QFbFb]
rule b<a>a: a
rule b<a>b: bFb
rule a<b>a: b
rule a<b>b: b
rule b<b>a: b
rule b<b>b: a
mapping: ABOP's letters 0 and 1 -> a and b (rule symbols must be letters; a and b draw nothing, as 0 and 1 do in the source). The source's #ignore: +-F becomes ignore !+-FQ (adding the two helper symbols below; ignore is written in ASCII order). The source also rewrites the turn symbols every step, * < + > * -> - and * < - > * -> +, which our grammar cannot do (only letters have rules). This is emulated exactly: each + or - written in a successor becomes Q+Q or Q-Q, and Q -> !Q, so after k further steps each Q has k ! in front of it. The first group of k ! flips the turn k times (k odd: + acts as -), and the second group, with the same number of !, restores the swap state, so the branch content and later turns are unaffected. Q and ! are ignored when contexts are matched, so the 2L context matches are the source's. The word contains extra Q and ! symbols but draws the same lines. The source's identity rules (e.g. 0<0>0 -> 0) are kept. n=30 in the source.
known dimension: 

id: hogeweg-hesper-c
title: Hogeweg and Hesper context-sensitive tree (c)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.31c (L-systems based on Hogeweg & Hesper 1974)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 25.75
ignore: !+-FQ
axiom: FbFbFb
rule Q: !Q
rule a<a>a: a
rule a<a>b: b
rule b<a>a: a
rule b<a>b: bFb
rule a<b>a: a
rule a<b>b: b[Q+QFbFb]
rule b<b>a: a
rule b<b>b: a
mapping: ABOP's letters 0 and 1 -> a and b (rule symbols must be letters; a and b draw nothing, as 0 and 1 do in the source). The source's #ignore: +-F becomes ignore !+-FQ (adding the two helper symbols below; ignore is written in ASCII order). The source also rewrites the turn symbols every step, * < + > * -> - and * < - > * -> +, which our grammar cannot do (only letters have rules). This is emulated exactly: each + or - written in a successor becomes Q+Q or Q-Q, and Q -> !Q, so after k further steps each Q has k ! in front of it. The first group of k ! flips the turn k times (k odd: + acts as -), and the second group, with the same number of !, restores the swap state, so the branch content and later turns are unaffected. Q and ! are ignored when contexts are matched, so the 2L context matches are the source's. The word contains extra Q and ! symbols but draws the same lines. The source's identity rules (e.g. 0<0>0 -> 0) are kept. n=26 in the source.
known dimension: 

id: hogeweg-hesper-d
title: Hogeweg and Hesper context-sensitive tree (d)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.31d (L-systems based on Hogeweg & Hesper 1974)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 25.75
ignore: !+-FQ
axiom: FaFbFb
rule Q: !Q
rule a<a>a: b
rule a<a>b: a
rule b<a>a: b
rule b<a>b: b[Q+QFbFb]
rule a<b>a: a
rule a<b>b: bFb
rule b<b>a: b
rule b<b>b: a
mapping: ABOP's letters 0 and 1 -> a and b (rule symbols must be letters; a and b draw nothing, as 0 and 1 do in the source). The source's #ignore: +-F becomes ignore !+-FQ (adding the two helper symbols below; ignore is written in ASCII order). The source also rewrites the turn symbols every step, * < + > * -> - and * < - > * -> +, which our grammar cannot do (only letters have rules). This is emulated exactly: each + or - written in a successor becomes Q+Q or Q-Q, and Q -> !Q, so after k further steps each Q has k ! in front of it. The first group of k ! flips the turn k times (k odd: + acts as -), and the second group, with the same number of !, restores the swap state, so the branch content and later turns are unaffected. Q and ! are ignored when contexts are matched, so the 2L context matches are the source's. The word contains extra Q and ! symbols but draws the same lines. The source's identity rules (e.g. 0<0>0 -> 0) are kept. n=24 in the source.
known dimension: 

id: hogeweg-hesper-e
title: Hogeweg and Hesper context-sensitive tree (e)
source: Prusinkiewicz & Lindenmayer, The Algorithmic Beauty of Plants (1990), fig. 1.31e (L-systems based on Hogeweg & Hesper 1974)
url: http://algorithmicbotany.org/papers/abop/abop.pdf (read from the archive.org mirror of the same PDF)
family: lsystem
version: 3
angle: 22.5
ignore: !+-FQ
axiom: FbFbFb
rule Q: !Q
rule a<a>a: a
rule a<a>b: b[Q-QFbFb]
rule b<a>a: a
rule b<a>b: bFb
rule a<b>a: b
rule a<b>b: b
rule b<b>a: b
rule b<b>b: a
mapping: ABOP's letters 0 and 1 -> a and b (rule symbols must be letters; a and b draw nothing, as 0 and 1 do in the source). The source's #ignore: +-F becomes ignore !+-FQ (adding the two helper symbols below; ignore is written in ASCII order). The source also rewrites the turn symbols every step, * < + > * -> - and * < - > * -> +, which our grammar cannot do (only letters have rules). This is emulated exactly: each + or - written in a successor becomes Q+Q or Q-Q, and Q -> !Q, so after k further steps each Q has k ! in front of it. The first group of k ! flips the turn k times (k odd: + acts as -), and the second group, with the same number of !, restores the swap state, so the branch content and later turns are unaffected. Q and ! are ignored when contexts are matched, so the 2L context matches are the source's. The word contains extra Q and ! symbols but draws the same lines. The source's identity rules (e.g. 0<0>0 -> 0) are kept. n=26 in the source.
known dimension: 

## X4: those sieve-fractal-v4 can write

Sources read for X4 (16 November 2026): Fractint's `ifs/fractint.ifs` (its IFS, each map "a b c d e f p" with
x' = a x + b y + e and y' = c x + d y + f, as v4's maps are; p, a probability for the chaos game, is not used,
since a v4 IFS draws every sequence of maps), its `common/fractalp.c` (each built-in type's default
parameters and corners) and `dos_help/help2.src` (each type's formula), from
https://raw.githubusercontent.com/jhol/fractint/master/; and Wikipedia, as raw wikitext. Conventions: a
number written as v4 writes it (a leading 0 added to ".5", trailing 0s dropped, "-0" and "0.00" as 0); a
complex map z -> a z + b as six numbers, x' = Re(a) x - Im(a) y + Re(b), y' = Im(a) x + Re(a) y + Im(b); a
Fractint view the centre of its default corners and their width (its pictures are 4:3, a v4 view square,
so a little more is seen above and below); a formula in prefix, its tokens in the order the source's
formula reads. Irrational numbers (1/3, sqrt 3 / 2, pi) are rounded to 6 decimals, each said in its
entry. Each entry was read by the reader and drawn, and the drawings looked over.

id: fractint-ifs-binary
title: Binary (Fractint IFS)
source: Fractint, fractint.ifs, "binary"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (127 characters):

    sieve-fractal-v4
    ifs
    map 0.5 0 0 0.5 -2.563477 -0.000003
    map 0.5 0 0 0.5 2.436544 -0.000003
    map 0 -0.5 0.5 0 4.873085 7.563492

mapping: 3 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-coral
title: Coral (Fractint IFS)
source: Fractint, fractint.ifs, "coral"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (195 characters):

    sieve-fractal-v4
    ifs
    map 0.307692 -0.531469 -0.461538 -0.293706 5.401953 8.655175
    map 0.307692 -0.076923 0.153846 -0.447552 -1.295248 4.15299
    map 0 0.545455 0.692308 -0.195804 -4.893637 7.269794

mapping: 3 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-crystal
title: Crystal (Fractint IFS)
source: Fractint, fractint.ifs, "crystal"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (142 characters):

    sieve-fractal-v4
    ifs
    map 0.69697 -0.481061 -0.393939 -0.662879 2.147003 10.310288
    map 0.090909 -0.443182 0.515152 -0.094697 4.286558 2.925762

mapping: 2 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-dragon
title: Dragon (Fractint IFS)
source: Fractint, fractint.ifs, "dragon"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (140 characters):

    sieve-fractal-v4
    ifs
    map 0.824074 0.281482 -0.212346 0.864198 -1.88229 -0.110607
    map 0.088272 0.520988 -0.463889 -0.377778 0.78536 8.095795

mapping: 2 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-floor
title: Floor (Fractint IFS)
source: Fractint, fractint.ifs, "floor"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (127 characters):

    sieve-fractal-v4
    ifs
    map 0 -0.5 0.5 0 -1.732366 3.366182
    map 0.5 0 0 0.5 -0.027891 5.014877
    map 0 0.5 -0.5 0 1.620804 3.310401

mapping: 3 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-koch3
title: Koch3 (Fractint IFS)
source: Fractint, fractint.ifs, "koch3"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (272 characters):

    sieve-fractal-v4
    ifs
    map 0.307692 0 0 0.294118 4.119164 1.604278
    map 0.192308 -0.205882 0.653846 0.088235 -0.68884 5.978916
    map 0.192308 0.205882 -0.653846 0.088235 0.66858 5.962514
    map 0.307692 0 0 0.294118 -4.13653 1.604278
    map 0.384615 0 0 -0.294118 -0.007718 2.941176

mapping: 5 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-spiral
title: Spiral (Fractint IFS)
source: Fractint, fractint.ifs, "spiral"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (198 characters):

    sieve-fractal-v4
    ifs
    map 0.787879 -0.424242 0.242424 0.859848 1.758647 1.408065
    map -0.121212 0.257576 0.151515 0.05303 -6.721654 1.377236
    map 0.181818 -0.136364 0.090909 0.181818 6.086107 1.568035

mapping: 3 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-swirl5
title: Swirl5 (Fractint IFS)
source: Fractint, fractint.ifs, "swirl5"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (142 characters):

    sieve-fractal-v4
    ifs
    map 0.745455 -0.459091 0.406061 0.887121 1.460279 0.691072
    map -0.424242 -0.065152 -0.175758 -0.218182 3.809567 6.741476

mapping: 2 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-tree
title: Tree (Fractint IFS)
source: Fractint, fractint.ifs, "tree"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (123 characters):

    sieve-fractal-v4
    ifs
    map 0 0 0 0.5 0 0
    map 0.42 -0.42 0.42 0.42 0 0.2
    map 0.42 0.42 -0.42 0.42 0 0.2
    map 0.1 0 0 0.1 0 0.2

mapping: 4 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-triangle
title: Triangle (Fractint IFS)
source: Fractint, fractint.ifs, "triangle"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (81 characters):

    sieve-fractal-v4
    ifs
    map 0.5 0 0 0.5 0 0
    map 0.5 0 0 0.5 0 1
    map 0.5 0 0 0.5 1 1

mapping: 3 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-zigzag2
title: Zigzag2 (Fractint IFS)
source: Fractint, fractint.ifs, "zigzag2"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (140 characters):

    sieve-fractal-v4
    ifs
    map -0.632407 -0.614815 -0.54537 0.659259 3.840822 1.282321
    map -0.036111 0.444444 0.210185 0.037037 2.071081 8.330552

mapping: 2 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: fractint-ifs-fractint
title: Fractint (the program's name, by Pieter Branderhorst) (Fractint IFS)
source: Fractint, fractint.ifs, "fractint"
url: https://raw.githubusercontent.com/jhol/fractint/master/ifs/fractint.ifs
ruleset (690 characters):

    sieve-fractal-v4
    ifs
    map 0 -0.11 0.22 0 -6.25 4.84
    map 0.11 0.02 0 0.11 -6.3 5.99
    map 0.06 0.02 0 0.1 -6.25 4.51
    map 0 -0.11 0.22 0 -4.34 4.84
    map 0.08 0 0 0.11 -4.5 5.99
    map 0 0.11 -0.08 0 -4.3 6.15
    map -0.09 0 -0.01 -0.13 -4.15 5.94
    map 0.06 0.11 -0.13 0 -4.69 4.15
    map 0.03 -0.11 0.23 0.11 -2.26 4.43
    map 0.03 0.11 -0.25 0 -2.57 4.99
    map 0.06 0 0 0.11 -2.4 4.46
    map 0 0.11 -0.19 0 -1.62 4.99
    map 0.09 -0.01 0 0.1 -0.58 2.96
    map -0.09 0 0 -0.11 -0.65 7.1
    map 0.12 0 0 0.11 1.24 6
    map 0 0.11 -0.22 0 0.68 4.8
    map -0.12 0 0 -0.13 6.17 7.18
    map 0 -0.11 0.22 0 6.78 4.84
    map 0 0.08 -0.25 0.02 2.21 4.95
    map 0 -0.11 0.22 0 4.1 4.84
    map 0 -0.11 0.22 0 5.25 5.23
    map 0.08 0.11 -0.25 0 3.57 4.99

mapping: 22 maps as written (a b c d e f; the seventh number, a probability for the chaos game, is not used: every sequence of maps is drawn)

id: barnsley-fern
title: Barnsley fern
source: Wikipedia, Barnsley fern (Barnsley, Fractals Everywhere); also Fractint's fractint.ifs "fern"
url: https://en.wikipedia.org/wiki/Barnsley_fern
ruleset (133 characters):

    sieve-fractal-v4
    ifs
    map 0 0 0 0.16 0 0
    map 0.85 0.04 -0.04 0.85 0 1.6
    map 0.2 -0.26 0.23 0.22 0 1.6
    map -0.15 0.28 0.26 0.24 0 0.44

mapping: the table's numbers as written (0.00 as 0); probabilities not used

id: cyclosorus-fern
title: Barnsley fern, mutated: Cyclosorus
source: Wikipedia, Barnsley fern, "Mutant varieties" (Cyclosorus or Thelypteridaceae)
url: https://en.wikipedia.org/wiki/Barnsley_fern
ruleset (152 characters):

    sieve-fractal-v4
    ifs
    map 0 0 0 0.25 0 -0.4
    map 0.95 0.005 -0.005 0.93 -0.002 0.5
    map 0.035 -0.2 0.16 0.04 -0.09 0.02
    map -0.04 0.2 0.16 0.04 0.083 0.12

mapping: the table's a b c d e g (g its f) as written

id: heighway-dragon-ifs
title: Heighway dragon, as an IFS
source: Wikipedia, Dragon curve, "[Heighway dragon] as an IFS": f1(z) = (1+i)z/2, f2(z) = 1 - (1-i)z/2
url: https://en.wikipedia.org/wiki/Dragon_curve
ruleset (73 characters):

    sieve-fractal-v4
    ifs
    map 0.5 -0.5 0.5 0.5 0 0
    map -0.5 -0.5 0.5 -0.5 1 0

mapping: each complex map z -> a z + b written as x' = Re(a) x - Im(a) y + Re(b), y' = Im(a) x + Re(a) y + Im(b)

id: twindragon-ifs
title: Twindragon (Davis-Knuth dragon), as an IFS
source: Wikipedia, Dragon curve, "Twindragon": f1(z) = (1+i)z/2, f2(z) = 1 - (1+i)z/2
url: https://en.wikipedia.org/wiki/Dragon_curve
ruleset (73 characters):

    sieve-fractal-v4
    ifs
    map 0.5 -0.5 0.5 0.5 0 0
    map -0.5 0.5 -0.5 -0.5 1 0

mapping: complex maps written as above

id: terdragon-ifs
title: Terdragon, as an IFS
source: Wikipedia, Dragon curve, "Terdragon": f1 = lz, f2 = (i/sqrt 3)z + l, f3 = lz + l*, l = 1/2 - i/(2 sqrt 3)
url: https://en.wikipedia.org/wiki/Dragon_curve
ruleset (139 characters):

    sieve-fractal-v4
    ifs
    map 0.5 0.288675 -0.288675 0.5 0 0
    map 0 -0.57735 0.57735 0 0.5 -0.288675
    map 0.5 0.288675 -0.288675 0.5 0.5 0.288675

mapping: complex maps written as above; 1/(2 sqrt 3) and 1/sqrt 3 rounded to 6 decimals (0.288675, 0.57735)

id: levy-c-curve-ifs
title: Levy C curve, as an IFS
source: Wikipedia, Levy C curve, "IFS": f1(z) = (1-i)z/2, f2(z) = 1 + (1+i)(z-1)/2
url: https://en.wikipedia.org/wiki/L%C3%A9vy_C_curve
ruleset (76 characters):

    sieve-fractal-v4
    ifs
    map 0.5 0.5 -0.5 0.5 0 0
    map 0.5 -0.5 0.5 0.5 0.5 -0.5

mapping: complex maps written as above (f2 = (1+i)/2 z + (1-i)/2)

id: cantor-set-ifs
title: Cantor set, as an IFS
source: Wikipedia, Cantor set, "Self-similarity": T_L(x) = x/3, T_R(x) = (2+x)/3
url: https://en.wikipedia.org/wiki/Cantor_set
ruleset (74 characters):

    sieve-fractal-v4
    ifs
    map 0.333333 0 0 0 0 0
    map 0.333333 0 0 0 0.666667 0

mapping: on the line y = 0 (y' = 0); 1/3 and 2/3 rounded to 6 decimals

id: sierpinski-triangle-ifs
title: Sierpinski triangle, as an IFS
source: Wikipedia, Sierpinski triangle: three copies of itself, each of ratio 1/2
url: https://en.wikipedia.org/wiki/Sierpi%C5%84ski_triangle
ruleset (93 characters):

    sieve-fractal-v4
    ifs
    map 0.5 0 0 0.5 0 0
    map 0.5 0 0 0.5 0.5 0
    map 0.5 0 0 0.5 0.25 0.433013

mapping: written from that construction on the equilateral triangle with corners (0, 0), (1, 0), (1/2, sqrt 3 / 2); sqrt 3 / 4 rounded to 6 decimals

id: sierpinski-carpet-ifs
title: Sierpinski carpet, as an IFS
source: Wikipedia, Sierpinski carpet: the square cut into nine, the centre taken away, and so on in each of the eight left
url: https://en.wikipedia.org/wiki/Sierpi%C5%84ski_carpet
ruleset (331 characters):

    sieve-fractal-v4
    ifs
    map 0.333333 0 0 0.333333 0 0
    map 0.333333 0 0 0.333333 0 0.333333
    map 0.333333 0 0 0.333333 0 0.666667
    map 0.333333 0 0 0.333333 0.333333 0
    map 0.333333 0 0 0.333333 0.333333 0.666667
    map 0.333333 0 0 0.333333 0.666667 0
    map 0.333333 0 0 0.333333 0.666667 0.333333
    map 0.333333 0 0 0.333333 0.666667 0.666667

mapping: written from that construction on the unit square; 1/3 and 2/3 rounded to 6 decimals

id: vicsek-fractal-ifs
title: Vicsek fractal (cross form), as an IFS
source: Wikipedia, Vicsek fractal: the square cut into nine, the centre and the four beside it kept (the cross form)
url: https://en.wikipedia.org/wiki/Vicsek_fractal
ruleset (227 characters):

    sieve-fractal-v4
    ifs
    map 0.333333 0 0 0.333333 0.333333 0.333333
    map 0.333333 0 0 0.333333 0 0.333333
    map 0.333333 0 0 0.333333 0.666667 0.333333
    map 0.333333 0 0 0.333333 0.333333 0
    map 0.333333 0 0 0.333333 0.333333 0.666667

mapping: written from that construction on the unit square; rounded as above

id: koch-curve-ifs
title: Koch curve, as an IFS
source: Wikipedia, Koch snowflake: von Koch's curve on a segment, its middle third replaced by two sides of a triangle
url: https://en.wikipedia.org/wiki/Koch_snowflake
ruleset (194 characters):

    sieve-fractal-v4
    ifs
    map 0.333333 0 0 0.333333 0 0
    map 0.166667 -0.288675 0.288675 0.166667 0.333333 0
    map 0.166667 0.288675 -0.288675 0.166667 0.5 0.288675
    map 0.333333 0 0 0.333333 0.666667 0

mapping: written from that construction on the segment (0, 0) to (1, 0): four maps of ratio 1/3, the middle two turned by 60 degrees each way; rounded to 6 decimals

id: mandelbrot-set
title: Mandelbrot set
source: Fractint, type mandel (z(n+1) = z(n)^2 + c), its default corners; Wikipedia, Mandelbrot set
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (48 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    view -0.5 0 4

mapping: z starts at 0 (Fractint starts at c, one step on: the same set)

id: fractint-julia
title: Julia set, Fractint's default (c = 0.3 + 0.6i)
source: Fractint, type julia, its default parameters 0.3 0.6 and corners
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (48 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia 0.3 0.6

id: multibrot-4
title: Multibrot set of degree 4
source: Fractint, type mandel4 (z(n+1) = z(n)^4 + c), its default corners; Wikipedia, Multibrot set
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (34 characters):

    sieve-fractal-v4
    escape + ^ z 4 c

id: fractint-julia4
title: Julia set of z^4 + c, Fractint's default (c = 0.6 + 0.55i)
source: Fractint, type julia4, its default parameters 0.6 0.55
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (49 characters):

    sieve-fractal-v4
    escape + ^ z 4 c
    julia 0.6 0.55

id: fractint-lambda
title: Lambda set, Fractint's default (lambda = 0.85 + 0.6i)
source: Fractint, type lambda (z(n+1) = lambda z(n) (1 - z(n))), its default parameters 0.85 0.6 and corners
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (66 characters):

    sieve-fractal-v4
    escape * * c z - 1 z
    julia 0.85 0.6
    view 0.5 0 4

mapping: lambda written as c (fixed by the julia line)

id: magnet-1-mandelbrot
title: Magnet fractal, type 1 (its Mandelbrot set)
source: Fractint, type magnet1m (z(n+1) = ((z^2 + c - 1) / (2z + c - 2))^2, z(0) = 0), its default corners
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (69 characters):

    sieve-fractal-v4
    escape ^ / + ^ z 2 - c 1 + * 2 z - c 2 2
    view 0 0 8

mapping: Fractint also colours the points that settle on 1; an escape picture colours only those that escape

id: magnet-1-julia
title: Magnet fractal, type 1 (Julia set, c = 0)
source: Fractint, type magnet1j, its default parameters 0 0 and corners
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (80 characters):

    sieve-fractal-v4
    escape ^ / + ^ z 2 - c 1 + * 2 z - c 2 2
    julia 0 0
    view 0 0 16

id: magnet-2-mandelbrot
title: Magnet fractal, type 2 (its Mandelbrot set)
source: Fractint, type magnet2m (z(n+1) = ((z^3 + 3(c-1)z + (c-1)(c-2)) / (3z^2 + 3(c-2)z + (c-1)(c-2) + 1))^2), its default corners
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (129 characters):

    sieve-fractal-v4
    escape ^ / + + ^ z 3 * * 3 - c 1 z * - c 1 - c 2 + + + * 3 ^ z 2 * * 3 - c 2 z * - c 1 - c 2 1 2
    view 1.1 0 5.2

mapping: Fractint also colours the points that settle on 1; an escape picture colours only those that escape, so the settled ones are the ground here

id: newton-z3-minus-1
title: Newton fractal of z^3 - 1
source: Fractint, type newton (degree 3, its default; z(0) = the point); Wikipedia, Newton fractal (z^3 - 1)
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (62 characters):

    sieve-fractal-v4
    converge - z / - ^ z 3 1 * 3 ^ z 2
    julia 0 0

mapping: Newton's step z - p(z)/p'(z) written out; julia 0 0 makes z start at the point (c is not used)

id: newton-z4-minus-1
title: Newton fractal of z^4 - 1
source: Fractint, type newton with degree 4 (its parameter: z^n - 1)
url: https://raw.githubusercontent.com/jhol/fractint/master/common/fractalp.c (defaults) and dos_help/help2.src (formulas)
ruleset (62 characters):

    sieve-fractal-v4
    converge - z / - ^ z 4 1 * 4 ^ z 3
    julia 0 0

mapping: as above

id: burning-ship
title: Burning Ship fractal
source: Wikipedia, Burning Ship fractal (z(n+1) = (|Re z| + i|Im z|)^2 + c, z(0) = 0)
url: https://en.wikipedia.org/wiki/Burning_Ship_fractal
ruleset (57 characters):

    sieve-fractal-v4
    escape + ^ abs z 2 c
    view -0.5 -0.5 3.5

mapping: drawn with y up, as every escape picture here is (it is often shown upside down)

id: tricorn
title: Tricorn (Mandelbar set)
source: Wikipedia, Tricorn (z -> conj(z)^2 + c)
url: https://en.wikipedia.org/wiki/Tricorn_(mathematics)
ruleset (39 characters):

    sieve-fractal-v4
    escape + ^ conj z 2 c

id: multibrot-3
title: Multibrot set of degree 3
source: Wikipedia, Multibrot set (z -> z^d + c, d = 3)
url: https://en.wikipedia.org/wiki/Multibrot_set
ruleset (34 characters):

    sieve-fractal-v4
    escape + ^ z 3 c

id: julia-0-4p0-6i
title: Julia set, c = -0.4 + 0.6i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.4 + 0.6i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (49 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.4 0.6

id: julia-0-285
title: Julia set, c = 0.285
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = 0.285)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (48 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia 0.285 0

id: julia-0-285p0-01i
title: Julia set, c = 0.285 + 0.01i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = 0.285 + 0.01i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (51 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia 0.285 0.01

id: julia-0-70176-0-3842i
title: Julia set, c = -0.70176 - 0.3842i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.70176 - 0.3842i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (57 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.70176 -0.3842

id: julia-0-835-0-2321i
title: Julia set, c = -0.835 - 0.2321i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.835 - 0.2321i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (55 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.835 -0.2321

id: julia-0-8p0-156i
title: Julia set, c = -0.8 + 0.156i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.8 + 0.156i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (51 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.8 0.156

id: julia-0-7269p0-1889i
title: Julia set, c = -0.7269 + 0.1889i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.7269 + 0.1889i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (55 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.7269 0.1889

id: julia-0-8i
title: Julia set, c = -0.8i
source: Wikipedia, Julia set, its gallery (f_c(z) = z^2 + c, c = -0.8i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (47 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia 0 -0.8

id: douady-corabbit
title: Douady corabbit (the rabbit's mirror image)
source: Wikipedia, Douady rabbit, "corabbit": c ~ -0.1226 - 0.7449i
url: https://en.wikipedia.org/wiki/Douady_rabbit
ruleset (56 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.1226 -0.7449

id: douady-rabbit
title: Douady rabbit
source: Wikipedia, Douady rabbit: the corabbit is its mirror image (c ~ -0.1226 - 0.7449i), so the rabbit's c is that one's conjugate
url: https://en.wikipedia.org/wiki/Douady_rabbit
ruleset (55 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.1226 0.7449

mapping: c = -0.1226 + 0.7449i, the conjugate of the corabbit's

id: julia-san-marco-dem
title: Julia set, c = -0.75 + 0.11i
source: Wikipedia, Julia set, its distance-estimation gallery (c = -0.75 + 0.11i)
url: https://en.wikipedia.org/wiki/Julia_set
ruleset (51 characters):

    sieve-fractal-v4
    escape + ^ z 2 c
    julia -0.75 0.11

id: collatz-fractal
title: Collatz fractal
source: Wikipedia, Collatz conjecture, "Iterating on real or complex numbers" (Letherman, Schleicher and Wood's f with h(z) = 0), centred at 0 with real parts -5 to 5
url: https://en.wikipedia.org/wiki/Collatz_conjecture
ruleset (168 characters):

    sieve-fractal-v4
    escape + + * / z 2 ^ cos * 1.570796 z 2 * / + * 3 z 1 2 ^ sin * 1.570796 z 2 * * 0.31831 - 0.5 cos * 3.141593 z sin * 3.141593 z
    julia 0 0
    view 0 0 10

mapping: pi/2, pi and 1/pi rounded to 6 decimals (1.570796, 3.141593, 0.31831); z starts at the point (julia 0 0; c is not used)

## Looked at and not charted

- Bourke's L-system pages (Crystal, Board, Rings, Tiles, Krishna anklets, Mango leaf, his trees): paulbourke.net is still blocked (DNS refused by the network policy). The archive.org API reports a Wayback snapshot of 2026-10-11, but web.archive.org itself resets the connection. Nothing was transcribed; third-party copies were not used.
- OEIS substitutions (Chacon sequence and others): oeis.org is blocked (proxy 403). Wikipedia has no "Chacon sequence" article (raw fetch returns 404). The OEIS-sourced substitutions were therefore not added, since no A-number could be checked.
- Fibonacci word fractal: the Wikipedia article gives the odd-even drawing rule and shows its L-system only as an image ("Fibonacci curve L-system.png", Commons; the image server answered 429). The cited papers are blocked (mathematica-journal.com, hal.science). The reachable arXiv papers (Ramirez, Rubiano & De Castro, arXiv:1212.1368; Hoffman & Steinhurst, arXiv:1601.04786) define the curve by the odd-even drawing rule, with no L-system text. So there was no published L-system text to transcribe.
- ABOP fig. 1.37a and 1.37b, row of trees: F(x) -> F(x*p)+F(x*h)--F(x*h)+F(x*q), with p = 0.3, q = 0.7 and h = sqrt(0.21). The scales are applied in sequence, not inside brackets, so each needs an exact inverse. Only one pair can use ' and " (SCALE 0.3, whose inverse ' is exact). The 0.7 and sqrt(0.21) pairs need digit pairs whose product is 1, and that is impossible in finite decimals because 1/0.7 repeats. The error would build up along the curve (about 1e-7 per pair, over 4^n segments). 1.37b also needs a delay parameter t. Not exact, so not charted.
- ABOP eq. (1.9), fig. 1.39: charted in X2 (abop-branching-pattern), written again in v3 with SCALE 0.686813 (1/1.456 = 0.6868131868...); it cannot be exact, as 1/1.456 repeats. Eq. (1.10) (F(s) -> F(s*R), not bracketed) would drift for the same reason.
- ABOP chapter 2 parametric trees (e.g. fig. 2.6 monopodial trees): their length ratios happen to be applied only where nothing follows, so they could be written exactly. But they turn by several different angles about different axes (& by a0, / by d = 137.5, + and - by a2). A digit can only turn in the plane (left or right), and & ^ \ / all use the one ANGLE. They also use $ and the width symbol !. Not expressible.
- ABOP fig. 1.30 (signal propagation, #ignore: +-): expressible (Fa -> F, Fb -> G, rule G<F: G), but it does not grow and is not a fractal, so it is skipped.
- Fractint "Bush" (fig. 1.24c rotated by ++++), "Plant09" (Plant07 with z renamed y), "Lars2" (Lars1 turned 45 degrees), "Curve2" (ABOP 1.9c), "Koch6" (mirror of ABOP 1.7a), "Koch1"/"KochCurve" (Koch), "DragonCurve", "Hilbert", "Peano3", "Square", the Penrose colour variants (ColorPenrose1, Penrose1Color, Penrose1Forced, KitesAndDartsColor, Lars1Color, Lars2Color): duplicates, rotations, mirror images or colour-only variants of entries already charted, so skipped. The old, commented-out PentaPlexity in fractint.l is not an active entry.
- Exact angles for earlier rounded entries: X2 wrote Fractint "angle 14" as 25.7 (philip-plant-07). v3 can write it exactly as 360/14. ABOP's own figures 1.24a/e print 25.7, so those stay. Written again so in X3.
- Every entry from fractint.l and tiling.l that is not in X2 or above is now charted. Nothing in those two files is still unwritable.
- X4, Fractint's types v4 cannot write yet: its 3D IFS (3dfern and Alex Matulich's polyhedra: 12 numbers a map, three dimensions; list X5), mandelfn and mandellambda (z starts at c, or at 0.5, where a v4 escape starts it at 0 or at the point), lambdafn (its default function is not in the files read), the phoenix types (z's value a step before), the barnsley types and the fn||fn types (a test inside the step), the orbit types (Henon, Lorenz, Martin, kamtorus: strange attractors, list X5) and diffusion-limited aggregation (random, list X5). The magnet fractal of type 2's Julia set was drawn and was noise (Fractint also colours the points that settle on 1, which an escape picture does not), so it is not charted; type 2's Mandelbrot set is, showing only the points that escape.
- X4, Wikipedia's: the Celtic and Buffalo variants of the Burning Ship (abs of the real part of z^2 alone, which `abs` cannot say), Siegel discs (no c given), Nova fractals (z starts at 1), and the Mandelbulb, Apollonian gasket and Rauzy fractal (three dimensions, circle inversion, a substitution's projection: list X5).
