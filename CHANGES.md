# The release notes come out as you fixed them (30 October 2026)

Unzip over the repository root. This goes on top of the last zip ("0.13.77, and a plan").
Committed locally (not pushed). Only `make_release.py` changes; nothing to rebuild.

`make_release.py` wrote the notes with paragraphs and list items wrapped at about 100 characters.
GitHub shows a line break inside a paragraph as a break, which is what you were fixing by hand.
Each paragraph and list item is now written on one line:

- the opening line;
- the three download items;
- the metrics paragraph.

**Checked:** I rendered the notes from the new template with 0.13.76's values (your table, your
metrics text) and compared them with your `formatting.txt`. They match exactly; the only
difference is that your file has Windows line endings, which Python writes on Windows anyway.

## Files
- `Sieve/tools/make_release.py`
- `Sieve/docs/HANDOFF.md`
