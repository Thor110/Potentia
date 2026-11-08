# training-v3: the probabilities' floor and ceiling (9 November 2026)

As in the post draft's "quantised waffle" and IDEAS §19.119: every byte's probability is now held
between 1/65,536 and 65,281/65,536 by the output softmax (each byte 1 + e_j (2^16 - 256) / sum e), in
training and in use. No byte is ever ruled out (0) or certain. training-v3 is the default for new runs
(sieve ai-train --record, the harness's New run); --training-v2 and --training-v1 record the older;
runs already recorded (the shipped ones) train exactly as before. Picture and melody models stay v2.

Measured (results/training-floor-2026-11-09.md):
- The shipped recipe: the right byte given 0 in 316 of 632,490 predictions (v1) -> 0 (v3); bytes at 0
  44.19% -> 0%; mean right-byte probability 28.03% both; 14.5 s / 13.7 s.
- 4 layers, width 64 (MANUAL, loss scale 2^6): 1,606 of 119,998 (1.34%) -> 0; 37.13% -> 0%; 15.18% ->
  15.22%; 29.8 s / 30.0 s.

Files: core/include/sieve/training.hpp, core/src/training.cpp; tools/cli/train.cpp, train_cmd.cpp,
args.hpp, help.cpp; reference/sieve_ref.py (tr_softmax_floored, train-vectors --v3); tests/
vectors_training_v3.tsv (new), test_core.cpp; ci/steps/04-models.sh (a v3 run replayed by the oracle,
--limits finding no zeros; the v2 recording now asks --training-v2); docs SPECIFICATIONS §12.0e,
MANUAL, IDEAS §19.119; results/training-floor-2026-11-09.md.

Checked here: unit suite 4/4 (v3 vectors with 1 and 3 threads; v1/v2 vectors unchanged); the new
step 04 lines; the shipped run still reproduces.

Please run: build, CI step 04 and the full CI. Goes on top of sieve-reset-defaults.zip.
