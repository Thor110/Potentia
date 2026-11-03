# The missing word list said plainly (2 November 2026)

Unzip this over the repository root, on top of your commit ad9b984 ("sieve docs post link"). One
local commit, not pushed: 1224dc6. The unit suite passes (77,147 checks, 0 failures). The full CI
hasn't been run on it.

A Sieve that can't find its dictionaries folder still packs, but mixes texts without the word
list, and they come out larger: a 20 KB text, 7,217 bytes against 6,205. Until now only each
file's own line said so ("the word list not to be had here").

Now it's said in two places:
- **`sieve locate`'s closing report:** a `words` line with the reason, for example
  `words     mixing's word list could not be had (cannot find the dictionary registry
  (dictionaries.tsv)): the texts that would have been weighed with it were mixed without it, larger
  (16% on a 20 KB text); Sieve finds its dictionaries folder beside it`. Checked here: a copy of
  sieve away from its data prints it; the one beside its data doesn't.
- **The File Locator's log:** a `warning:` line under `settings:`, before any file's line. This
  one compiles but I couldn't run the hallway here, so your next File Locator run with a moved
  Sieve is its test.
