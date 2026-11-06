# OLAP: Ideas, Decisions and Next Steps

*A running log for the OLAP Initiative and its simulation ([`Pages/space.html`](../space.html)).
Newest entries first. Each says who had the idea or made the decision, and when. How the page
works is in [README.md](README.md).*

**Concept: Edward James Gordon.** Entries marked *Claude* were worked out with Claude.

---

## 6 November 2026

### Decisions (Edward)

- **No mass is to be thrown off.** The OLAP system works with flywheels, as a satellite turns
  itself with reaction wheels: it rotates and aligns the planet for use in transit. Mass ejection
  keeps coming up as an assumption from the history of ideas for moving bodies. If it ever becomes
  a necessity, it does, but the system stays the steering wheel regardless.
- **Steering first, moving second.** The simulation looks first at steering, then at what could
  move the planet.
- **Flywheels were first placed at the stations,** but there may need to be many more round each
  ring, between the crossings. Hence a setting for wheels per ring, and the question of whether
  a mag-lev or ordinary train should act as the circle itself.
- **The idea of nudges at the Sun:** at the closest pass, nudges might take gravitational energy
  from the Sun.
- **Other uses:** if it cannot move a planet, the same system might be redesigned for smaller
  bodies, or scaled up as part of a Dyson sphere, or other things.
- **Changes are delivered as a zip,** extracted at the repository's root.

### Findings (Claude, from the page's physics)

- **The ring is the flywheel; the trains are its rim.** How the mass is split does not change the
  total: wheels at one radius and rim speed need the same mass whether there are 26 or 26,000.
  Only radius and speed reduce it, and nothing on a planet has a larger radius than a ring round
  the whole of it. So trains running all the way round each great circle are the best design.
  Wheels at the stations, or between them, cannot reach useful amounts.
- **Trains float as they speed up.** A train circling the planet weighs less on its track the
  faster it goes, and nothing at orbital speed at the surface (7.9 km/s for the Earth). Faster
  than that, the track must hold it down.
- **The scale on the Earth:**

  | Goal (Earth) | Trains at 7.9 km/s | As solid steel, each of 9 rings |
  |---|---|---|
  | The published 0.1% shift (0.057°) | 1.2×10²⁰ kg | 6.4 km square |
  | Tilt 1° | 2.0×10²¹ kg (1.5× the oceans) | 27 km square |
  | Lengthen the day by 1 minute | 8.1×10¹⁹ kg | 5.4 km square |

  At today's fastest train (603 km/h) each needs about 47 times more mass.
- **The scale elsewhere** (tilt 1°, trains at orbital speed at the surface, or the train record
  if that is faster):

  | Body | Mass | As solid steel, each ring |
  |---|---|---|
  | Mars | 2.8×10²⁰ kg | 13.5 km square |
  | The Moon | 1.4×10¹⁸ kg | 1.3 km square |
  | Ceres | 1.5×10¹⁸ kg | 2.7 km square |
  | A 10 km asteroid | 6.5×10¹⁰ kg (at 603 km/h) | 5.5 m square |

  On a small asteroid the system becomes buildable with today's engineering, although at that
  speed the track must hold the trains in, since orbital speed there is only 4 m/s. Steering
  small bodies (pointing them, spinning them up or down for gravity, or setting them up for
  mining) is a real use.
- **The nudges at the Sun cannot move the Earth.**
  - The Earth's orbit is nearly a circle (closest 0.983 AU, furthest 1.017 AU).
  - The Sun is the fixed centre, so it gives no energy; the machinery making the nudges does.
  - The pull differs across the Earth by only (Earth's radius / distance)² ≈ 2×10⁻⁹.
  - Shifting 0.1% of the Earth's mass every orbit would take about 10¹³ years to escape.
  - All of the Earth's spin energy is 1/12,000 of what escape needs.

  Energy can be taken from a body that moves, which is why the gravity assists take it from
  Jupiter.
- **The engine without mass ejection:** asteroid gravity assists drawing on Jupiter (after
  Korycansky, Laughlin & Adams 2001). They move nothing off the Earth. About 130,000 passes keep
  the Earth habitable to a billion years; escape needs millions more, with many asteroids in
  turn. The steering's job there is real: aligning and timing the planet for each pass.
- **The open problem is the drop to the Galactic Centre.** It means cancelling 241 km/s. There is
  no known way to do it without an engine: a star passes within 1 AU about once in 10¹² years.
  The slingshot itself works well once there (the Oberth effect). The alternatives are to stop
  at leaving the Sun, or to ride the Milky Way's meeting with Andromeda.

### The page (v0.3)

- The steering panel comes first, with the body to steer (Earth, Mars, the Moon, Ceres, an
  asteroid). It offers trains per ring (the default), wheels along the rings, or wheels at the
  stations. It shows each train's mass, its mass per metre, the solid-steel ring it equals, and
  its weight on the track.
- Gravity assists are the default engine. *Nudges against the Sun* is an option that shows its
  numbers. Mass ejection is kept only as "if it becomes necessary".
- The Galactic Centre drop is marked as the open problem unless mass ejection is chosen.
- The ledger now says each step's role: steering, engine or route.

### Ideas and next steps (open)

- **Energy budget:** where each step's energy comes from: fusion of ocean deuterium, uranium and
  thorium, the Earth's own heat, or Jupiter (via the assists).
- **The Moon:** does it come along? It steadies the Earth's tilt, could be steered itself, and
  stays bound in a slow escape.
- **Dyson-scale steering:** rings round a star, or round a ring habitat, as the same steering
  system scaled up; look at what holds a Dyson swarm's members in formation.
- **Steering in transit:** pointing a pole along the direction of travel as a shield (a 90°
  turn needs about two Moons of trains, unless the spin is slowed first, which costs the same).
- **Simulate the escape spiral properly,** step by step, with sunlight falling as it goes.
- **Audit the sources** against the original papers (some values were taken from memory when the
  reference sites were blocked).
- **More data:** Cosmicflows-4 distances; any 2MRS galaxy as a destination; Andromeda's own
  motion; close stellar passes from Gaia.
- **Presets and reports:** "published", "conservative" and "optimistic" plans, and a downloadable
  report with the data hashes.
