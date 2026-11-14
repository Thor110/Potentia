# Idea: the hash search in parallel

- IDEAS 19.219 gains: the range divided among threads (about N times as fast with N), each stepping forwards and backwards from its division's point until it meets its neighbours; with a best guess of the place, the threads start about it and spread outwards so the likeliest are tried first. The same division could spread across machines that opt in.

Docs only.

## Files to delete by hand
- None.
