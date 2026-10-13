# A once-over across Sieve, Potentia, the website and the docs (13 October 2026)

## Healthy
- The engine, the tests and the game build with no warnings.
- The unit suite passes: 76,549 checks, 0 failures here.
- Every Python tool compiles, and the CI file parses.
- Every on-screen string the game asks for is in `en.txt`.
- Every web page loads with its AI disclosure, no script errors and no broken local links.

## Fixed
- **Pages on a phone:** the front page and the audio, sieve, quest and space pages pushed sideways
  on a 390-pixel screen, because titles and footers couldn't wrap. The space page reached 804
  pixels; Build or Buy was 4 pixels over, from its long run of zeros. Each page now has the
  phone-only wrap rule the newer pages already had. Desktop layout is unchanged.
  - Files: `index.html`, `Pages/audio.html`, `sieve.html`, `quest.html`, `space.html`,
    `release.html`.
- **A duplicate idea number:** IDEAS had two entries numbered 5.12. The address navigator is now
  5.11a. The one reference to §5.12 means the junction, which keeps its number.
- **The corpus manifest:** `Sieve/.gitignore`'s `corpus/` also matched
  `data/models/corpus/gutenberg-nltk.tsv`. That is the list of the corpus's files and pinned
  SHA-256s that `fetch_corpus.py` checks the download against, so it has never been committed.
  The folder is now excepted. **If you have that file locally, GitHub Desktop will now offer it to
  commit.** If you don't, it has to be made again from the 15 NLTK books the pinned model was
  trained on; tell me and I'll do that.

## Found, not changed (your call)
- **The CI has never run on GitHub.** `build.yml` lives at `Sieve/.github/workflows/`, but GitHub
  only runs workflows from `.github/workflows/` at the repository's root. The only runs on
  record are the website's Pages builds. Every CI line has been checked by hand in these
  sessions. To switch it on:
  - move it to the root;
  - set every step to run inside `Sieve/`;
  - expect a first run that shows what has drifted.
- **"Claude Opus 5.5"** appears in your own acknowledgements (both READMEs, GAME.md, the anchors'
  copy, `Art/credit.txt`), as you asked. I left them as you wrote them.
- **No `favicon.ico`:** browsers log one harmless "not found" on the front page.

## Notes
- HANDOFF has an entry with all of the above.
