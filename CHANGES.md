# Changes

One commit on top of a241a2c ("sieve metrics").

## CI step 03: the shared filters one bit more since list M2

Your full CI run failed step 03 on the weighing's total: it printed
`best ways 272 + shared 260 = 532 bits` where the step expected `shared 259 = 531`.
The audio line has two more filters to choose from since list M2 (melody-cadence-v1
and melody-contrary-v1), so describing the tailored audio filters takes 28 bits
rather than 27. Nothing about the weighing itself changed.

- `Sieve/ci/steps/03-filters.sh`: both checks of the total now expect 532 bits, and
  the comment above them names the new filters.

Checked: step 03 passes here (336 s).

## Files to delete by hand

None.
