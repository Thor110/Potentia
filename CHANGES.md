# CI step 11: the site page's check (4 November 2026)

Commit 4ea60e7 (local, not pushed), on top of your a843c87.

**The failure.** Line 287 of 11-oracle.sh checks that a site's page is mixed with the word list. It expected the way to be exactly `mix4 B<n> F80`, followed by a tab.

**The cause.** The page is the first 9,000 bytes of Sieve/README.md. Since the README's text changed in the docs pass, the weighing also adds the match model's C2, so the way is now `mix4 B16 F80 C2`. Sieve is fine; the check was stricter than what it tests.

**The fix.** The check now allows those match-model settings after F80.

**Tested.** The whole of step 11 passes here (1,850 s with this machine's build).
