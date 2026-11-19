# Changes

Two commits on top of f150a70 ("sieve N5 finish"): the MSVC fix from the last zip, and this one.

- **OPTIMISE FILTERS no longer ticks near-model or language** on the AI line: they pick a run's neighbourhood and
  language, not noise, so they are left for ticking by hand. The AI line now gets weight-signs-v1.
- **The lag spike on OPTIMISE FILTERS:** its last step (keeping each line's stack within the filter memory) built
  every line's counting tables on the window's own thread, seconds on the books' pages. It now runs on a worker and
  the window keeps answering; the ticks come in when it is done (the same ticks as before).
- **REVIEW-ROUND list O:** pages of a range of lengths (MIN LENGTH and MAX LENGTH, default 1 to 32), planned in
  stages; work starts next.

## Files to delete by hand
None.
