# The weighing-order review: first pass done (2 November 2026)

Unzip over the repository root, on top of your commit 56fb703. One more local commit, 3bb7b59, not pushed. Only the review document changed.

The last two items were looked at, and neither needs a change:

- **Copies.** A file with the same bytes as another never reaches the delta matcher. The masks step before it carries the file as a mask of its twin's lines, for almost nothing, and even random bytes contain line feeds. I built a step to find copies by SHA-256 first, tried it on two identical 10 MiB files and a near copy, and took it out: it found nothing the masks had not.
- **The JPEG step.** It runs only with Compressed formats ticked, and its brunsli forms are left out of the search. Optimise Search's rule for JPEGs (mix them whole) applies only with that box unticked, so the two never meet.

The document ends with a summary of what the review built and what it measured and left.
