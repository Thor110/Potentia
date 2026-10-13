# Metrics Over Time (13 October 2026)

## The new page: `Pages/metrics.html`
- **What it shows:** your metrics, read from `Sieve/metrics/index.tsv` in the repository, as
  line charts: a dot a build, joined in the order they were made.
- **One chart a measure:**
  - unit test checks;
  - seconds for the unit suite;
  - checks a second (the two divided);
  - vector files, specification sections, commands, source files and lines, ideas, and voices;
  - what the reference filters keep on each line.

  Each chart has its own scale, because the measures are in different units. Putting two scales
  on one axis would mislead.
- **Choosing what to show:** the machine (`edward-1` by default, or every machine), and releases
  or every build.
- **Reading a chart:**
  - hovering shows the nearest build's value, date and time, version and commit;
  - every chart opens as a table too;
  - a measure that hasn't changed says "the same in every build shown", with no made-up range
    drawn around it;
  - the line under each title says what the top and base lines stand for (the highest and lowest
    on the record).
- **On a phone:** charts are drawn at the screen's width, so their text stays readable. There's no
  sideways scroll.
- **The AI disclosure** is at the top, as on every page.

## The button
On Build or Buy: "Metrics Over Time", below every metric and above Build It Or Buy It.

## Notes
- IDEAS §19.50 records the page as built.
- HANDOFF has an entry for this round.

## Checked here
- In Chromium, at phone width and desktop width, against your two release records: the charts
  draw, the hover works, and there are no errors.
- **What you'll see now:**
  - the suite went from 478.0 to 407.8 seconds, −70.2;
  - checks a second went from 320.28 to 375.42;
  - lines of source went up by 440;
  - unit checks stayed at 153,096.
