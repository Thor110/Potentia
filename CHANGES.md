# IDEAS: floating point near the same; §19.122, a larger model (5 November 2026)

This commit sits on top of bf4a79d (the seeds zip). It and the three IDEAS commits before it are local and not pushed.

## §19.121, a new bullet: without integer arithmetic

- **Near the same is enough where a person only looks:** the hallway's view, previews, thumbnails, the shelves.
- **It is not enough to name a file or to make one again.** Near is another file, with another SHA-256.
- **A stored correction can't rescue it.** The installer would apply the correction to its own, slightly different, output.
- **So:** floating point where a person looks, integers where Sieve names or makes a file again.

## §19.122: a much larger, capable model

What it needs first:

- **§19.119's two changes** (QK-norm and the probability floor). Both problems grow with the model.
- **A decision on exactness.** Names need exact use, not exact training. A model trained in floating point, then fixed once in integers, is exact wherever it runs from then on. The cost is that the training run can no longer be replayed, so the record would hold the weights' SHA-256 instead.
- **Data** with its licences recorded, and a size chosen from a measured curve rather than a guess.
- **Installers would need it** for any file mixed with it, as they now need the dictionaries.
