// The OLAP simulation's physics: pure functions, no page code, so every result can be checked
// on its own (`node Pages/space/tools/check.mjs`). Constants come from constants.js (OLAP.K).
//
// SI units throughout unless a name says otherwise (…AU, …Yr, …Kms, …Pc).
(function (root) {
  "use strict";
  const K = root.OLAP.K;
  const P = {};

  // ---------------------------------------------------------------------------------------------
  // Small numerical helpers
  // ---------------------------------------------------------------------------------------------

  // Composite Simpson's rule on [a, b] with n (even) intervals.
  P.simpson = function (f, a, b, n) {
    n = n || 2000; if (n % 2) n++;
    const h = (b - a) / n;
    let s = f(a) + f(b);
    for (let i = 1; i < n; i++) s += f(a + i * h) * (i % 2 ? 4 : 2);
    return s * h / 3;
  };

  // Bisection for f(x) = 0 on [lo, hi], f(lo) and f(hi) of opposite sign.
  P.bisect = function (f, lo, hi, iters) {
    let flo = f(lo);
    for (let i = 0; i < (iters || 200); i++) {
      const mid = 0.5 * (lo + hi), fm = f(mid);
      if ((fm < 0) === (flo < 0)) { lo = mid; flo = fm; } else hi = mid;
    }
    return 0.5 * (lo + hi);
  };

  // A small seeded generator (mulberry32), so every search is the same each time it is run.
  P.rng = function (seed) {
    let a = seed >>> 0;
    return function () {
      a = (a + 0x6D2B79F5) >>> 0;
      let t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  };

  // ---------------------------------------------------------------------------------------------
  // The Sun and the habitable zone
  // ---------------------------------------------------------------------------------------------

  // The Sun's luminosity, as a multiple of today's, tYr years from now (Gough 1981).
  P.lum = function (tYr) {
    const x = (K.sunAge + tYr) / K.sunAge;
    return 1 / (1 + K.goughK * (1 - x));
  };
  // Whether the formula still holds (the Sun still burning hydrogen in its core).
  P.lumValid = (tYr) => K.sunAge + tYr <= K.sunMSEndAge;

  // Years from now until sunlight at aAU reaches S (in units of today's sunlight at 1 AU).
  P.yearsUntilInsolation = function (S, aAU) {
    aAU = aAU || 1;
    const Lneeded = S * aAU * aAU;              // L/L_now that gives S at aAU
    const x = 1 - (1 / Lneeded - 1) / K.goughK;  // age / age_now
    return (x - 1) * K.sunAge;
  };

  // Sunlight at aAU, tYr from now, in units of today's at 1 AU.
  P.insolation = (tYr, aAU) => P.lum(tYr) / (aAU * aAU);
  // The distance that keeps sunlight at S, tYr from now.
  P.aForInsolation = (tYr, S) => Math.sqrt(P.lum(tYr) / S);

  // ---------------------------------------------------------------------------------------------
  // The Earth's spin and the flywheels
  // ---------------------------------------------------------------------------------------------

  P.earthI = () => K.IearthNorm * K.Mearth * K.ReqEarth * K.ReqEarth;   // kg m^2
  // Any body: moment of inertia, spin rate and spin angular momentum
  P.bodyI = (b) => b.k * b.M * (b.Rin || b.R) ** 2;
  P.bodyOmega = (b) => 2 * Math.PI / b.T;
  P.bodyL = (b) => P.bodyI(b) * P.bodyOmega(b);
  // The fastest a train can circle a body at its surface without lifting off its track
  P.surfaceOrbitSpeed = (b) => Math.sqrt(K.G * b.M / b.R);
  // How heavy a train is on its track at speed v, as a share of its weight at rest
  // (1 at rest, 0 at orbital speed; below 0 it pushes outward and must be held down).
  P.trainWeight = (b, v) => 1 - v * v / (K.G * b.M / b.R);
  P.earthL = () => P.earthI() * K.omegaEarth;                           // kg m^2 / s

  // The angular momentum the wheels must hold for a goal.
  //   tilt: turn the spin axis by tiltDeg (the wheels hold the difference: 2 L sin(θ/2))
  //   day:  lengthen (or shorten, if negative) the day by dayS seconds
  //   fraction: a share of the Earth's spin, as in the OLAP Initiative's proof
  P.momentumForGoal = function (goal, body) {
    const L = body ? P.bodyL(body) : P.earthL();
    const I = body ? P.bodyI(body) : P.earthI(), w = body ? P.bodyOmega(body) : K.omegaEarth;
    if (goal.kind === "tilt") return 2 * L * Math.sin((goal.tiltDeg * Math.PI / 180) / 2);
    if (goal.kind === "day") {
      const T = 2 * Math.PI / w;
      const w2 = 2 * Math.PI / (T + goal.dayS);
      return I * Math.abs(w - w2);
    }
    return goal.fraction * L;
  };

  // Rim speed for a material: v = sqrt(sigma / rho), or its own stated limit.
  P.materialSpeed = (m) => m.vmax || Math.sqrt(m.sigma / m.rho);

  // Flywheels (thin hoops: I = m r^2, so h = m r v and E = m v^2 / 2).
  //   dL: momentum to hold; r: radius (m); v: rim speed (m/s); n: wheels; power: W for spin-up.
  P.flywheels = function ({ dL, r, v, n, power }) {
    const M = dL / (r * v);
    const E = 0.5 * M * v * v;
    return {
      dL, totalMass: M, perWheel: M / n, energy: E,
      spinUpYears: power > 0 ? E / power / K.year : Infinity,
      massEarths: M / K.Mearth
    };
  };

  // The published proof, redone: a 0.1% shift over a million nudges, 1 km radius at 1 rpm.
  P.publishedProof = function () {
    const dL = K.docShiftFraction * P.earthL();
    const perNudge = dL / K.docNudges;
    const r = K.docWheelDiameter / 2, w = K.docWheelRPM * 2 * Math.PI / 60;
    const massPerNudge = perNudge / (r * r * w);
    return {
      L: P.earthL(), dL, perNudge, massPerNudge,
      // The nudges add up: the wheels must, between them, keep holding all of dL.
      massAll: massPerNudge * K.docNudges,
      tiltDeg: 2 * Math.asin(K.docShiftFraction / 2) * 180 / Math.PI
    };
  };

  // Tilt (degrees) reached after t seconds of spinning up wheels of total mass M and radius r at a
  // steady power, capped at the material's rim speed.
  P.tiltAfter = function ({ M, r, vmax, power, t }) {
    const v = Math.min(Math.sqrt(2 * power * t / M), vmax);
    const h = M * r * v;
    const s = Math.min(1, h / (2 * P.earthL()));
    return { v, h, tiltDeg: 2 * Math.asin(s) * 180 / Math.PI };
  };

  // ---------------------------------------------------------------------------------------------
  // The planetary grid: 9 great circles (the cube's planes of symmetry) and their 26 crossings
  // ---------------------------------------------------------------------------------------------

  const s2 = Math.SQRT1_2, s3 = 1 / Math.sqrt(3);
  // Each ring is the great circle at right angles to its normal; a flywheel in that ring holds
  // momentum along the normal.
  P.ringNormals = [
    [1, 0, 0], [0, 1, 0], [0, 0, 1],
    [s2, s2, 0], [s2, -s2, 0], [s2, 0, s2], [s2, 0, -s2], [0, s2, s2], [0, s2, -s2]
  ];
  P.gridNodes = (function () {
    const out = [];
    for (const a of [0, 1, 2]) for (const sg of [1, -1]) {
      const p = [0, 0, 0]; p[a] = sg; out.push({ p, kind: "quad" });
    }
    for (const [i, j] of [[0, 1], [0, 2], [1, 2]]) for (const si of [1, -1]) for (const sj of [1, -1]) {
      const p = [0, 0, 0]; p[i] = si * s2; p[j] = sj * s2; out.push({ p, kind: "dual" });
    }
    for (const x of [1, -1]) for (const y of [1, -1]) for (const z of [1, -1]) out.push({ p: [x * s3, y * s3, z * s3], kind: "tri" });
    return out;
  })();

  // 3-vectors and 3x3 matrices (row-major arrays of arrays)
  P.dot = (a, b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
  P.mulMV = (m, v) => [P.dot(m[0], v), P.dot(m[1], v), P.dot(m[2], v)];
  P.mulMM = (a, b) => a.map(r => [0, 1, 2].map(j => r[0] * b[0][j] + r[1] * b[1][j] + r[2] * b[2][j]));
  P.transpose = (m) => [0, 1, 2].map(i => [m[0][i], m[1][i], m[2][i]]);
  P.rotX = (t) => [[1, 0, 0], [0, Math.cos(t), -Math.sin(t)], [0, Math.sin(t), Math.cos(t)]];
  P.rotY = (t) => [[Math.cos(t), 0, Math.sin(t)], [0, 1, 0], [-Math.sin(t), 0, Math.cos(t)]];
  P.rotZ = (t) => [[Math.cos(t), -Math.sin(t), 0], [Math.sin(t), Math.cos(t), 0], [0, 0, 1]];
  P.inv3 = function (m) {
    const [a, b, c] = m[0], [d, e, f] = m[1], [g, h, i] = m[2];
    const A = e * i - f * h, B = -(d * i - f * g), Cc = d * h - e * g;
    const det = a * A + b * B + c * Cc;
    return [[A / det, -(b * i - c * h) / det, (b * f - c * e) / det],
            [B / det, (a * i - c * g) / det, -(a * f - c * d) / det],
            [Cc / det, -(a * h - b * g) / det, (a * e - b * d) / det]];
  };

  // Orientation of the grid on the globe, from three angles (degrees): the latitude and longitude
  // the grid's first quad-station is turned to, and a roll about that station.
  P.gridRotation = function (latDeg, lonDeg, rollDeg) {
    const d = Math.PI / 180;
    // Start with the +z quad-station at the North Pole, roll about z, tilt down to latitude, turn to longitude.
    return P.mulMM(P.rotZ(lonDeg * d), P.mulMM(P.rotY((90 - latDeg) * d), P.rotZ(rollDeg * d)));
  };

  P.toLatLon = (p) => [Math.asin(Math.max(-1, Math.min(1, p[2]))) * 180 / Math.PI, Math.atan2(p[1], p[0]) * 180 / Math.PI];
  P.fromLatLon = (lat, lon) => {
    const d = Math.PI / 180;
    return [Math.cos(lat * d) * Math.cos(lon * d), Math.cos(lat * d) * Math.sin(lon * d), Math.sin(lat * d)];
  };

  // Land polygons (GeoJSON) prepared for fast point tests: rings with bounding boxes.
  P.prepareLand = function (geojson) {
    const rings = [];
    for (const f of geojson.features) {
      const g = f.geometry;
      const polys = g.type === "Polygon" ? [g.coordinates] : g.coordinates;
      for (const poly of polys) for (const ring of poly) {
        let x0 = 1e9, x1 = -1e9, y0 = 1e9, y1 = -1e9;
        for (const [x, y] of ring) { x0 = Math.min(x0, x); x1 = Math.max(x1, x); y0 = Math.min(y0, y); y1 = Math.max(y1, y); }
        rings.push({ ring, x0, x1, y0, y1 });
      }
    }
    return rings;
  };
  // Even-odd test in longitude/latitude (holes count as rings too, which even-odd handles).
  P.isLand = function (rings, lat, lon) {
    let inside = false;
    for (const r of rings) {
      if (lon < r.x0 || lon > r.x1 || lat < r.y0 || lat > r.y1) continue;
      const pts = r.ring;
      for (let i = 0, j = pts.length - 1; i < pts.length; j = i++) {
        const [xi, yi] = pts[i], [xj, yj] = pts[j];
        if ((yi > lat) !== (yj > lat) && lon < (xj - xi) * (lat - yi) / (yj - yi) + xi) inside = !inside;
      }
    }
    return inside;
  };

  // How a grid orientation sits on the land: nodes on land, and the share of ring length on land.
  P.gridLand = function (rings, R, samples) {
    samples = samples || 180;
    const nodes = P.gridNodes.map(n => {
      const q = P.mulMV(R, n.p), [lat, lon] = P.toLatLon(q);
      return { kind: n.kind, lat, lon, land: P.isLand(rings, lat, lon) };
    });
    let landPts = 0, all = 0;
    for (const nrm of P.ringNormals) {
      const nn = P.mulMV(R, nrm);
      // Two unit vectors spanning the ring's plane
      const t = Math.abs(nn[2]) < 0.9 ? [0, 0, 1] : [1, 0, 0];
      let u = [nn[1] * t[2] - nn[2] * t[1], nn[2] * t[0] - nn[0] * t[2], nn[0] * t[1] - nn[1] * t[0]];
      const ul = Math.hypot(...u); u = u.map(x => x / ul);
      const w = [nn[1] * u[2] - nn[2] * u[1], nn[2] * u[0] - nn[0] * u[2], nn[0] * u[1] - nn[1] * u[0]];
      for (let k = 0; k < samples; k++) {
        const a = 2 * Math.PI * k / samples;
        const p = [0, 1, 2].map(i => Math.cos(a) * u[i] + Math.sin(a) * w[i]);
        const [lat, lon] = P.toLatLon(p);
        if (P.isLand(rings, lat, lon)) landPts++;
        all++;
      }
    }
    return { nodes, landNodes: nodes.filter(n => n.land).length, ringLandShare: landPts / all };
  };

  // Search orientations for the most nodes on land (then the most ring on land). Seeded, so the
  // same search gives the same answer. With poleQuad, a quad-station stays on the North Pole.
  P.optimiseGrid = function (rings, { tries, seed, poleQuad }) {
    const rnd = P.rng(seed || 1);
    let best = null;
    for (let i = 0; i < (tries || 3000); i++) {
      const lat = poleQuad ? 90 : Math.asin(2 * rnd() - 1) * 180 / Math.PI;
      const lon = poleQuad ? 0 : rnd() * 360 - 180;
      const roll = rnd() * 90; // the grid repeats every 90 degrees about a quad-station
      const R = P.gridRotation(lat, lon, roll);
      const g = P.gridLand(rings, R, 60);
      const score = g.landNodes * 1000 + g.ringLandShare * 999;
      if (!best || score > best.score) best = { lat, lon, roll, score, landNodes: g.landNodes };
    }
    return best;
  };

  // The momentum each ring holds, through the day, so that the network as a whole holds a fixed
  // vector H in space while the Earth turns beneath it (the least total, spread over all 9 rings).
  //   H: [x,y,z] in the Earth's frame at phase 0; phase: rotation angle of the Earth (radians).
  P.ringShares = function (R, H, phase) {
    const Hb = P.mulMV(P.rotZ(-phase), H);   // the fixed vector, seen from the turning Earth
    const N = P.ringNormals.map(n => P.mulMV(R, n));
    const S = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
    for (const n of N) for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++) S[i][j] += n[i] * n[j];
    const y = P.mulMV(P.inv3(S), Hb);
    return N.map(n => P.dot(n, y));
  };

  // ---------------------------------------------------------------------------------------------
  // Orbits around the Sun
  // ---------------------------------------------------------------------------------------------

  P.vCirc = (aAU) => Math.sqrt(K.GMsun / (aAU * K.AU));
  P.vEscSun = (aAU) => Math.sqrt(2 * K.GMsun / (aAU * K.AU));
  // Energy to widen a circular orbit from a0 to a1 (J).
  P.migrationEnergy = (a0, a1, M) => (M || K.Mearth) * K.GMsun / 2 * (1 / (a0 * K.AU) - 1 / (a1 * K.AU));
  // One quick push from a circular orbit at aAU to leave with speed vinf far away.
  P.escapeDvImpulse = (aAU, vinf) => Math.sqrt(2 * K.GMsun / (aAU * K.AU) + vinf * vinf) - P.vCirc(aAU);
  // A slow spiral out instead (low thrust): roughly the circular speed, then vinf.
  P.escapeDvSpiral = (aAU, vinf) => P.vCirc(aAU) + vinf;
  P.migrationDvSpiral = (a0, a1) => P.vCirc(a0) - P.vCirc(a1);
  P.escapeEnergy = (aAU, vinf, M) => (M || K.Mearth) * (vinf * vinf / 2 + K.GMsun / (2 * aAU * K.AU));

  // Throwing mass off the Earth (the Orbital Launch Accelerators as an engine).
  //   dv: velocity change wanted (m/s); vLaunch: speed at the launcher (m/s).
  // The mass leaves the Earth's pull with sqrt(vLaunch^2 - vesc^2). Relativistic rocket equation.
  P.massDriver = function ({ dv, vLaunch, M }) {
    M = M || K.Mearth;
    const vex = Math.sqrt(Math.max(0, vLaunch * vLaunch - K.vEscEarth * K.vEscEarth));
    if (vex <= 0) return { vex: 0, fraction: 1, massOut: Infinity, energy: Infinity };
    const ratio = Math.exp((K.c / vex) * Math.atanh(Math.min(dv / K.c, 0.999999)));
    const massOut = M * (1 - 1 / ratio);
    const g = 1 / Math.sqrt(1 - (vLaunch / K.c) ** 2);
    return { vex, fraction: 1 - 1 / ratio, massOut, energy: massOut * (g - 1) * K.c * K.c };
  };

  // Gravity assists from a large asteroid, drawing energy from Jupiter (after Korycansky, Laughlin
  // & Adams 2001, Astrophysics and Space Science 275, 349). Each pass bends the asteroid's path
  // round the Earth; the Earth takes the opposite kick.
  //   mObj: asteroid mass (kg); vRel: its speed relative to the Earth (m/s); rp: closest approach
  //   to the Earth's centre (m); eta: how much of the kick lies along the Earth's motion (0..1).
  P.flyby = function ({ mObj, vRel, rp, eta, aAU }) {
    const mu = K.G * K.Mearth;
    const sinHalf = 1 / (1 + rp * vRel * vRel / mu);
    const dvObj = 2 * vRel * sinHalf;               // the asteroid's change of velocity
    const dvEarth = mObj * dvObj / K.Mearth;        // the Earth's, opposite
    const dE = eta * K.Mearth * P.vCirc(aAU || 1) * dvEarth;  // energy given to the Earth's orbit
    return { deflectDeg: 2 * Math.asin(sinHalf) * 180 / Math.PI, dvObj, dvEarth, dE };
  };
  // How far Jupiter falls inward, giving up energy dE (J).
  P.jupiterShift = function (dE) {
    const E0 = -K.GMsun * K.Mjupiter / (2 * K.aJupiter * K.AU);
    const a1 = -K.GMsun * K.Mjupiter / (2 * (E0 - dE));
    return a1 / K.AU;
  };

  // Nudges against the Sun's pull: shifting a share f of the Earth's mass across its own radius in
  // step with the orbit, so the Sun's tide does work on it (the technique proposed for space
  // tethers that change length as they orbit). Energy gained per orbit, at most about
  //   (G M_sun M / 2a) * f * (R/a)^2 * e,
  // with e the orbit's eccentricity. An upper estimate, order of magnitude only; the energy itself
  // comes from the machinery doing the shifting, not from the Sun.
  P.sunNudges = function ({ f, aAU, e, need }) {
    const a = (aAU || 1) * K.AU;
    const perOrbit = K.GMsun * K.Mearth / (2 * a) * f * (K.Rearth / a) ** 2 * (e == null ? 0.0167 : e);
    const period = 2 * Math.PI * Math.sqrt(a ** 3 / K.GMsun) / K.year;
    return { perOrbit, perYear: perOrbit / period, years: need / (perOrbit / period) };
  };
  // Spin, used up entirely: how far the Earth's own spin energy goes towards a need (J)
  P.spinEnergy = () => 0.5 * P.earthI() * K.omegaEarth ** 2;
  // How often a star passes within d AU of the Earth, per year (n stars per pc^3, at v m/s)
  P.starPassRate = function (dAU, vRel) {
    const d = dAU * K.AU / K.pc, focus = 1 + (2 * K.GMsun / (dAU * K.AU)) / (vRel * vRel);
    return K.starDensity * Math.PI * d * d * focus * (vRel * K.year / K.pc);
  };

  // Leaving: distance against time on the way out, starting at aAU with speed sqrt(vinf^2 + 2mu/r)
  // (straight out; for distances far beyond the start the true hyperbola gives nearly the same).
  // Returns a table of [years, AU] from aAU out to rMaxAU.
  P.cruiseTable = function (aAU, vinf, rMaxAU) {
    const pts = [[0, aAU]];
    const n = 600, l0 = Math.log(aAU), l1 = Math.log(rMaxAU);
    let t = 0;
    const speed = (rAU) => Math.sqrt(vinf * vinf + 2 * K.GMsun / (rAU * K.AU));
    for (let i = 1; i <= n; i++) {
      const ra = Math.exp(l0 + (l1 - l0) * (i - 1) / n), rb = Math.exp(l0 + (l1 - l0) * i / n);
      // dt = dr / v, integrated with Simpson's rule over the step
      const f = (r) => K.AU / speed(r);
      t += P.simpson(f, ra, rb, 8) / K.year;
      pts.push([t, rb]);
    }
    return pts;
  };
  P.timeToRadius = function (table, rAU) {
    for (let i = 1; i < table.length; i++) if (table[i][1] >= rAU) {
      const [t0, r0] = table[i - 1], [t1, r1] = table[i];
      return t0 + (t1 - t0) * (rAU - r0) / (r1 - r0);
    }
    return Infinity;
  };
  P.radiusAtTime = function (table, tYr) {
    for (let i = 1; i < table.length; i++) if (table[i][0] >= tYr) {
      const [t0, r0] = table[i - 1], [t1, r1] = table[i];
      return r0 + (r1 - r0) * (tYr - t0) / (t1 - t0);
    }
    return table[table.length - 1][1];
  };

  // Crossing the Oort cloud: the expected number of comets the Earth meets, taking the cloud as an
  // even spread of N comets between rIn and rOut (AU), crossed straight through at speed v (m/s).
  // The Earth's gravity widens its target: sigma = pi R^2 (1 + vesc^2 / v^2).
  P.oortHazard = function ({ N, rInAU, rOutAU, v }) {
    const rin = rInAU * K.AU, rout = rOutAU * K.AU;
    const n = N / (4 / 3 * Math.PI * (rout ** 3 - rin ** 3));
    const sigma = Math.PI * K.Rearth ** 2 * (1 + (K.vEscEarth / v) ** 2);
    const lambda = n * sigma * (rout - rin);
    const years = (rout - rin) / v / K.year;
    return { lambda, pAny: 1 - Math.exp(-lambda), years, background: years / K.impactKm, density: n };
  };

  // ---------------------------------------------------------------------------------------------
  // The Galaxy (in pc, Msun and km/s)
  // ---------------------------------------------------------------------------------------------

  const Gpc = 4.300917270e-3;  // G in pc (km/s)^2 / Msun
  P.Gpc = Gpc;
  // Circular speeds squared (km/s)^2 of each part, at radius r in the plane.
  function vc2Parts(r, Ms) {
    const d = K.mwDisk, b = K.mwBulge, rs = K.mwHalo.rs, x = r / rs;
    return {
      bh: Gpc * K.MsgrA / r,
      bulge: Gpc * b.M * r / ((r + b.a) ** 2),
      disk: Gpc * d.M * r * r / Math.pow(r * r + (d.a + d.b) ** 2, 1.5),
      halo: Ms ? Gpc * Ms * (Math.log(1 + x) - x / (1 + x)) / r : 0
    };
  }
  // The halo's mass scale, chosen so the circular speed at R0 is the measured one.
  P.haloMs = (function () {
    let cache = null;
    return function () {
      if (cache) return cache;
      const p = vc2Parts(K.R0, 0), x = K.R0 / K.mwHalo.rs;
      cache = (K.vcSun ** 2 - p.bh - p.bulge - p.disk) * K.R0 / (Gpc * (Math.log(1 + x) - x / (1 + x)));
      return cache;
    };
  })();
  P.galVc = (r) => { const p = vc2Parts(r, P.haloMs()); return Math.sqrt(p.bh + p.bulge + p.disk + p.halo); };
  // The potential in the plane (km/s)^2, zero far away.
  P.galPhi = function (r) {
    const d = K.mwDisk, b = K.mwBulge, rs = K.mwHalo.rs;
    return -Gpc * K.MsgrA / r - Gpc * b.M / (r + b.a) - Gpc * d.M / Math.sqrt(r * r + (d.a + d.b) ** 2)
      - Gpc * P.haloMs() * Math.log(1 + r / rs) / r;
  };
  P.galVesc = (r) => Math.sqrt(-2 * P.galPhi(r));
  P.galMassWithin = (r) => P.galVc(r) ** 2 * r / Gpc;

  // The route through the Galactic Centre:
  //   1. at the Sun's place, cancel almost all of the Sun's orbital speed, keeping just enough to
  //      miss Sgr A* by rpAU;
  //   2. fall in; 3. at the closest point, fire burnKms (the Oberth effect: a push at high speed
  //      gives far more energy); 4. climb out of the Galaxy.
  P.galRoute = function ({ rpAU, burnKms }) {
    const R0 = K.R0, rp = rpAU * K.AU / K.pc;
    const Vt = K.vcSun + K.vSunPec[1], U = K.vSunPec[0];
    const phi0 = P.galPhi(R0);
    // Energy after the drop (per unit mass), with a small tangential speed vt kept
    const vpOf = (vt) => Math.sqrt(2 * (0.5 * vt * vt + phi0 - P.galPhi(rp)));
    // Angular momentum is kept: R0 vt = rp vp
    const vt = P.bisect((x) => R0 * x - rp * vpOf(x), 0, Vt, 200);
    const vp = vpOf(vt);
    const dvDrop = Math.hypot(Vt - vt, U);
    const E = 0.5 * vt * vt + phi0, h = R0 * vt;
    // Fall time: integrate dr / v_r from rp to R0 (in log r to resolve the deep well)
    const fall = P.simpson((lr) => {
      const r = Math.exp(lr);
      const v2 = 2 * (E - P.galPhi(r)) - (h * h) / (r * r);
      return v2 > 0 ? r / Math.sqrt(v2) : 0;
    }, Math.log(rp * 1.0001), Math.log(R0 * 0.9999), 4000) * K.pc / 1000 / K.year;
    // The burn at closest approach, and what is left far away
    const Eafter = 0.5 * (vp + burnKms) ** 2 + P.galPhi(rp);
    const vinf = Eafter > 0 ? Math.sqrt(2 * Eafter) : 0;
    // Without the burn: where it climbs back to (it stays bound)
    const apoNoBurn = R0;
    const apoAfter = Eafter > 0 ? Infinity : P.bisect((r) => P.galPhi(r) - Eafter, rp, 1e7, 200);
    // Safety: where the black hole would tear the Earth apart, and its event horizon
    const MbhKg = K.MsgrA * K.Msun;
    const rTidalAU = K.Rearth * Math.cbrt(MbhKg / K.Mearth) / K.AU;
    const rsAU = 2 * K.G * MbhKg / (K.c * K.c) / K.AU;
    // Leaving straight from here instead, without the Galactic Centre
    const directDv = P.galVesc(R0) - Math.hypot(Vt, U);
    return { vt, vp, dvDrop, fallYears: fall, Eafter, vinf, apoAfter, apoNoBurn, rTidalAU, rsAU,
      vpOverC: vp * 1000 / K.c, directDv, vescR0: P.galVesc(R0), vcR0: P.galVc(R0) };
  };
  // Distance against time for the route: the fall from R0 to the closest approach, then the climb
  // out after the burn. Returns { fall: [[years before closest approach, pc]...],
  // exit: [[years after, pc]...], exitBound }.
  P.galTables = function (g, rpAU, burnKms) {
    const rp = rpAU * K.AU / K.pc, R0 = K.R0, conv = K.pc / 1000 / K.year;
    const E = 0.5 * g.vt * g.vt + P.galPhi(R0), h = R0 * g.vt;
    const n = 300;
    const fallF = (lr) => { const r = Math.exp(lr), v2 = 2 * (E - P.galPhi(r)) - h * h / (r * r); return v2 > 0 ? r / Math.sqrt(v2) : 0; };
    const fall = [[0, rp]];
    let t = 0, lo = Math.log(rp * 1.0001), hi = Math.log(R0);
    for (let i = 1; i <= n; i++) {
      const a = lo + (hi - lo) * (i - 1) / n, b = lo + (hi - lo) * i / n;
      t += P.simpson(fallF, a, b, 6) * conv; fall.push([t, Math.exp(b)]);
    }
    const Ea = g.Eafter, ha = rp * (g.vp + burnKms);
    const exitF = (lr) => { const r = Math.exp(lr), v2 = 2 * (Ea - P.galPhi(r)) - ha * ha / (r * r); return v2 > 0 ? r / Math.sqrt(v2) : 0; };
    const rMax = Ea > 0 ? 5e6 : g.apoAfter * 0.9999;
    const exit = [[0, rp]];
    t = 0; hi = Math.log(rMax);
    for (let i = 1; i <= n; i++) {
      const a = lo + (hi - lo) * (i - 1) / n, b = lo + (hi - lo) * i / n;
      t += P.simpson(exitF, a, b, 6) * conv; exit.push([t, Math.exp(b)]);
    }
    return { fall, exit, exitBound: Ea <= 0 };
  };
  // Look up a [time, distance] table at time t (linear between rows; the last row beyond).
  P.lookup = function (table, t) {
    if (t <= table[0][0]) return table[0][1];
    for (let i = 1; i < table.length; i++) if (table[i][0] >= t) {
      const [t0, r0] = table[i - 1], [t1, r1] = table[i];
      return r0 + (r1 - r0) * (t - t0) / (t1 - t0);
    }
    return table[table.length - 1][1];
  };
  // ... and the time at which a table reaches distance r.
  P.lookupTime = function (table, r) {
    for (let i = 1; i < table.length; i++) if (table[i][1] >= r) {
      const [t0, r0] = table[i - 1], [t1, r1] = table[i];
      return t0 + (t1 - t0) * (r - r0) / (r1 - r0);
    }
    return Infinity;
  };

  // A burn of dv at speed v deep in a well: the speed left far away, compared with the same burn
  // made far from anything (the Oberth gain).
  P.oberth = (vp, vesc, dv) => Math.sqrt(Math.max(0, (vp + dv) ** 2 - vesc * vesc));

  // ---------------------------------------------------------------------------------------------
  // The expanding universe (flat ΛCDM, matter and dark energy; radiation is negligible here)
  // ---------------------------------------------------------------------------------------------

  P.cosmo = function (H0, Om) {
    H0 = H0 || K.H0; Om = Om == null ? K.OmegaM : Om;
    const OL = 1 - Om;
    const cKms = K.c / 1000, DH = cKms / H0;              // Hubble distance, Mpc
    const E = (a) => Math.sqrt(Om / (a * a * a) + OL);
    // Comoving distance to redshift z (Mpc)
    const dc = (z) => DH * P.simpson((zz) => 1 / Math.sqrt(Om * (1 + zz) ** 3 + OL), 0, z, 200);
    // A table for fast lookup of comoving distance from cz (km/s), for z up to 0.2
    const tabZ = [], tabD = [];
    for (let i = 0; i <= 400; i++) { const z = 0.2 * i / 400; tabZ.push(z); tabD.push(dc(z)); }
    const dcFast = (z) => {
      if (z <= 0) return 0;
      if (z >= 0.2) return dc(z);
      const f = z / 0.2 * 400, i = Math.floor(f);
      return tabD[i] + (tabD[i + 1] - tabD[i]) * (f - i);
    };
    // How far (comoving, Mpc) a body sent today at speed v (m/s) can ever get. Its momentum fades
    // as the universe grows (p ∝ 1/a), so even light only reaches the event horizon.
    //   chi = ∫ v(a) / (a^2 H) da from a=1 to ∞; with s = 1/a this is ∫_0^1 v(1/s) / E(1/s) ds / H0.
    const reach = function (v) {
      const u0 = v >= K.c ? Infinity : v / Math.sqrt(1 - (v / K.c) ** 2);
      const vOf = (s) => {
        if (!isFinite(u0)) return K.c;
        const u = u0 * s;
        return u / Math.sqrt(1 + (u / K.c) ** 2);
      };
      return DH * P.simpson((s) => (vOf(s) / K.c) / Math.sqrt(Om * s * s * s + OL), 0, 1, 2000);
    };
    return { H0, Om, OL, DH, E, dc, dcFast, reach };
  };

  P.MpcToGly = (d) => d * 1e6 * K.pc / K.ly / 1e9;

  root.OLAP.P = P;
})(typeof window !== "undefined" ? window : globalThis);
