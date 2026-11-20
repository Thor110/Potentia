# Changes

One commit on top of 32a5d05 ("sieve O8").

## Orders by sort key: O9, measured
- **`tools/sort_keys_study.py`:** the study. A line ordered by a key puts the units with the smallest key first (ties
  kept in positional order), so a page's address is how many pages stand before it; naming which order was used costs
  log2 of the number of orders. It tries five orders on pages of 32 symbols: positional; guided (the model's
  probability, highest first: Sieve's guided order); words-v2 passes first; the letters' entropy, lowest first; and
  the number of different symbols, fewest first. The last two are counted exactly, over every way of splitting 32
  letters into counts. Run it as `python3 -I tools/sort_keys_study.py PATH/TO/sieve --random 8 TEXTFILE...`
  (about a minute here).
- **The result (IDEAS 19.256):** on 24 prose pages, positional takes 150.4 bits on average, guided 97.0, words-v2 first
  137.5, entropy 137.0, distinct 142.7. The entropy and distinct orders beat guided and compact (named) on none of the
  24 pages: whatever a page's letter counts give away, the model sees too, and more. On random pages no order beats
  positional. words-v2 first is compact mode with the failing pages put after the survivors.
- **So:** nothing to build. The keys that mean something and can be counted are the ones Sieve already orders by
  (guided and compact), and with O8 that leaves Seed Search (O10) nothing to find; it is still open in REVIEW-ROUND
  for your call.

## Files to delete by hand
None.
