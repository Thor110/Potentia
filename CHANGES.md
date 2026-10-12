# A JUDGE-ONLY FILTERS tab

Apply after ai-language.zip and nav-random.zip. The shared files here carry their changes too.

- **The filters window** has a fourth tab, JUDGE-ONLY FILTERS, to the right of the retired
  filters. It holds the filters that decide which units pass but cannot count them, so a line
  that ticks one cannot compact. Today these are the AI line's canonical-model-v1 and language-v1.
  The counting filters stand at one end of the tabs and these at the other, as you put it, and
  the filters can be sorted by it later.
- **`FilterSpec::judge_only`** marks a filter explicitly. The existing "counts as" label couldn't
  be trusted for this: binary-kind counts, but has no label. A retired filter stays on the
  retired tab.
- **The tick-all keys** (C, and X for every line) leave judge-only filters alone, as they do
  retired ones. Z still ticks them on their own tab, and they can be ticked by hand.
- **`sieve filters`** marks them: "(hard, judge-only)", "(soft, judge-only)".
- **Docs:** README (the four tabs); IDEAS §15 (your reasoning, marked built).
- **Checked here:**
  - a screenshot of the tab on the AI line;
  - two CI lines on `sieve filters --line ai`, and a screenshot line for the tab.
