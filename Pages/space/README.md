# The OLAP Simulation

*The Orbital Simulation page ([`Pages/space.html`](../space.html)): a sandbox for planning the
OLAP Initiative's route and testing what it would take, step by step, with measured numbers.*

**Concept: Edward James Gordon** (the OLAP Initiative, 2025). Built with Claude from his documents:
*The OLAP Initiative*, *OLAP Global Positioning & Alignment Systems*, *OLA Integration*, *The Road
to OLAP*, and the moodboards.

## What the page does

The plan, as the documents set it out:

- **A planetary grid:** nine great-circle rings (the cube's nine planes of symmetry) crossing at
  26 stations: 6 quad, 12 dual and 8 tri. Each station has underground habitats, planetary
  flywheels and six Orbital Launch Accelerators.
- **The route:** leave the solar system in one to two billion years, before the brightening Sun
  boils the oceans; cross the Oort cloud; slingshot round the Galactic Centre towards another
  galaxy; leave colonies on the way.

The page plays the route out over time, at four scales (the Earth and its grid, the solar system,
the Milky Way, and the local universe from 2MRS). Every setting can be changed, and the address
keeps them, so a plan can be shared as a link. A feasibility ledger sets out each step, what it
needs, and whether known physics allows it.

### What the numbers say (with the published settings)

- **The published flywheel proof is right as arithmetic.** A 0.1% shift of the Earth's spin is
  5.86×10³⁰ kg m²/s; a millionth of it, at 1 rpm on a 1 km radius, takes 5.6×10¹⁹ kg. The
  million nudges add up, though: to hold the whole shift, the wheels together must hold all of
  it, 5.6×10²⁵ kg (9.4 Earth masses). Faster rims and wider wheels cut that. The nine rings,
  run as mag-lev mass streams near orbital speed, need about 1.2×10²⁰ kg: an eighth of Ceres.
- **Flywheels can steer, but cannot move the orbit.** Forces inside a body cannot move its centre
  of mass. The wheels can turn the axis and change the day; the accelerators (throwing mass
  away) or gravity assists must change the orbit. That is the platform's own name: the
  *Launch Accelerator* Platform. (Edward, 6 November 2026: no mass is meant to be thrown off; the
  flywheels are the steering wheel. The page now defaults to that, and keeps mass ejection only as
  an option "if it becomes necessary".)
- **Holding a tilt is a network task.** The Earth turns under the held vector once a day, so the
  rings must hand momentum between them all day. The globe shows each ring's share as it turns,
  which is why the stations must know their alignments exactly (the neutrino positioning
  document).
- **Staying habitable:** keeping today's sunlight means moving out to about 1.05 AU by a billion
  years from now. Gravity assists from a 10¹⁹ kg asteroid, taking energy from Jupiter (after
  Korycansky, Laughlin & Adams 2001), do it with one pass every few thousand years.
- **The published 7,961 years to 50,000 AU holds** if the Earth leaves at about 29.8 km/s, close
  to its orbital speed today.
- **The Oort cloud is not a wall.** With 10¹² comets spread from 2,000 to 100,000 AU, the
  expected number of collisions on the way through is about 10⁻⁷: far less than the ordinary
  risk of a 1 km impact over the same years. This answers the addendum's main worry.
- **The Galactic Centre slingshot works, through the Oberth effect.** Falling to Sgr A* takes
  about 40 million years and needs 241 km/s of the Sun's orbital speed cancelled first. A push of
  30 km/s at 300 AU from Sgr A* leaves the Galaxy at about 110 km/s. Nature already does it:
  S5-HVS1 was flung out at 1,755 km/s.
- **The expanding universe sets the final limit.** At 112 km/s the Earth can ever reach about
  0.9 Mpc: Andromeda (and it is coming to us anyway), but just 32 of 2MRS's 43,533 galaxies.

**[OLAP.md](OLAP.md)** keeps the evolution of the idea: from the first framing (April 2025) through
every change and its reason, where each idea stands, and what is open.

## Files

| File | What it is |
|---|---|
| `../space.html` | The page: layout and styles. |
| `OLAP.md` | The running log: ideas, decisions, findings and what to do next. |
| `constants.js` | **Every number, with its source.** Change a value here and everything follows. |
| `physics.js` | Pure functions: the Sun's brightening, flywheels, the grid, orbits, gravity assists, mass drivers, the Oort cloud, the Galaxy's potential, the route, the expanding universe. No page code. |
| `datasets.js` | Where each dataset comes from, how it is read, and the SHA-256 check against the record. |
| `views.js` | The four scales, drawn on a canvas. |
| `app.js` | The settings, the timeline, the panels, the ledger, the charts and the tables. |
| `data/manifest.js` | Written by the updater: each dataset's row count, SHA-256 and the date it last changed. |
| `data/exoplanets.js`, `data/land.js`, `data/skin-*.js` | Snapshots kept here (public data). |
| `tools/update_data.py` | Fetches every dataset again and rewrites the snapshots and the manifest. |
| `tools/check.mjs` | Checks the physics against values known independently. |

No build step and no libraries: the page runs as it is, from GitHub Pages or straight from the
folder (`file://`).

## Data

| Dataset | Source | Kept |
|---|---|---|
| 2MASS Redshift Survey | Huchra et al. 2012, ApJS 199, 26 (VizieR J/ApJS/199/26) | **Read live** from VizieR: the 2MRS team ask that their files not be redistributed. 44,599 galaxies, 43,533 with redshifts. |
| Local Group | McConnachie 2012, AJ 144, 4 (VizieR J/AJ/144/4) | Read live from VizieR. |
| Exoplanets | NASA Exoplanet Archive, `pscomppars` | Snapshot (the archive cannot be read from a browser on another site). |
| Land | Natural Earth v5.1.2, 1:110m (public domain) | Snapshot. |
| Surface maps | The Moon: NASA SVS CGI Moon Kit (LRO). Mars: USGS Viking MDIM 2.1 colour mosaic. Ceres: NASA Dawn PIA19606 (coloured by height). All public domain. | Snapshots (`data/skin-*.js`), loaded only when that body is shown. The asteroid's surface is generated from a fixed seed. |

Live tables are made canonical (the column-name line and the data lines, LF-terminated) and
hashed in the browser; the page says whether what it read is exactly what the manifest records.
It tries `vizier.cds.unistra.fr` first, then the mirror at `vizier.cfa.harvard.edu`.

## Keeping it up to date

- **Refresh the data:** `python3 Pages/space/tools/update_data.py` (Python 3.8+, standard library
  only). `--check` fetches and compares without writing; it exits non-zero if anything changed.
- **Change a constant:** edit `constants.js` (value and source), then run
  `node Pages/space/tools/check.mjs`.
- **Add a dataset:** add it to `DATASETS` in `tools/update_data.py` and to `D` in `datasets.js`
  (with its parser), run the updater, and for a snapshot add its `<script>` to `space.html`.
  Candidates: Cosmicflows-4 distances (better than redshift within ~10 Mpc), Gaia's nearby stars
  for close stellar passes, a fuller Milky Way potential.
- **Add a setting:** add an entry to `FIELDS` in `app.js` (its panel, default and control); it
  appears in the panel and the link at once. Use it in `compute()`.

Other pages quote the cosmic horizon counter's constants (`REACH_GLY`, `SEEN_GLY`,
`STARS_LOST_PER_SECOND`, now in `constants.js`): change them together with the Build or Buy page.

## Limits of the model

- The Sun's brightening follows Gough (1981), which holds while the Sun burns hydrogen in its core
  (about 6.3 billion more years).
- Orbits round the Sun are circular; the escape is either one quick push or a slow spiral
  (approximated as the circular speed plus the speed left over). The coast out is straight.
- Gravity assists use a simple flyby (the turn set by the closest approach) with a share of each
  kick along the Earth's path; the energy comes from Jupiter's orbit.
- The Galaxy is a Miyamoto–Nagai disc, a Hernquist bulge, an NFW halo scaled to 229 km/s at the
  Sun, and Sgr A*; the route lies in the plane. Near Sgr A* the figures are Newtonian.
- Distances in 2MRS come from redshift, which is poor within ~10 Mpc, where galaxies' own motions
  dominate. The Local Group is bound by gravity, so expansion does not apply inside it.
- The spiral arms are drawn for orientation only.
