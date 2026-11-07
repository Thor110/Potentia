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
  // Returns NaN if f(lo) and f(hi) have the same sign (no root bracketed).
  P.bisect = function (f, lo, hi, iters) {
    let flo = f(lo);
    if ((flo < 0) === (f(hi) < 0)) return NaN;
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

  // The same land as a grid of cells (W x H, longitude -180..180, latitude 90..-90), filled by
  // scanlines with the same even-odd rule, so a look-up costs nothing. Used where many points are
  // tested (the rings, the search); the stations themselves use the exact polygons.
  P.rasterLand = function (rings, W, H) {
    W = W || 1440; H = H || 720;
    const cells = new Uint8Array(W * H);
    for (let j = 0; j < H; j++) {
      const lat = 90 - (j + 0.5) * 180 / H, xs = [];
      for (const r of rings) {
        if (lat < r.y0 || lat > r.y1) continue;
        const pts = r.ring;
        for (let i = 0, k = pts.length - 1; i < pts.length; k = i++) {
          const [xi, yi] = pts[i], [xk, yk] = pts[k];
          if ((yi > lat) !== (yk > lat)) xs.push((xk - xi) * (lat - yi) / (yk - yi) + xi);
        }
      }
      xs.sort((a, b) => a - b);
      for (let m = 0; m + 1 < xs.length; m += 2) {
        const a = Math.max(0, Math.ceil((xs[m] + 180) / 360 * W - 0.5)), b = Math.min(W - 1, Math.floor((xs[m + 1] + 180) / 360 * W - 0.5));
        for (let i = a; i <= b; i++) cells[j * W + i] = 1;
      }
    }
    return { W, H, cells, at(lat, lon) {
      const i = Math.min(W - 1, Math.floor(((lon + 540) % 360) / 360 * W)), j = Math.min(H - 1, Math.max(0, Math.floor((90 - lat) / 180 * H)));
      return cells[j * W + i] === 1;
    } };
  };

  // How a grid orientation sits on the land: nodes on land, and the share of ring length on land.
  // With a raster, the ring samples use it (fast); without, the polygons (exact, slow).
  P.gridLand = function (rings, R, samples, raster, nodesFast) {
    samples = samples || 180;
    const test = raster ? (lat, lon) => raster.at(lat, lon) : (lat, lon) => P.isLand(rings, lat, lon);
    const nodes = P.gridNodes.map(n => {
      const q = P.mulMV(R, n.p), [lat, lon] = P.toLatLon(q);
      return { kind: n.kind, lat, lon, land: nodesFast && raster ? raster.at(lat, lon) : P.isLand(rings, lat, lon) };
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
        if (test(lat, lon)) landPts++;
        all++;
      }
    }
    return { nodes, landNodes: nodes.filter(n => n.land).length, ringLandShare: landPts / all };
  };

  // Search orientations for the most nodes on land (then the most ring on land). Seeded, so the
  // same search gives the same answer. With poleQuad, a quad-station stays on the North Pole.
  // The search runs over the raster (fast); the winner's stations are then checked exactly.
  // onProgress(done, tries) is called between chunks when given; the result comes by promise then.
  P.optimiseGrid = function (rings, { tries, seed, poleQuad, raster, onProgress }) {
    const rnd = P.rng(seed || 1), N = tries || 3000;
    raster = raster || P.rasterLand(rings);
    let best = null, i = 0;
    const step = (count) => {
      for (const end = Math.min(N, i + count); i < end; i++) {
        const lat = poleQuad ? 90 : Math.asin(2 * rnd() - 1) * 180 / Math.PI;
        const lon = poleQuad ? 0 : rnd() * 360 - 180;
        const roll = rnd() * 90; // the grid repeats every 90 degrees about a quad-station
        const g = P.gridLand(rings, P.gridRotation(lat, lon, roll), 60, raster, true);
        const score = g.landNodes * 1000 + g.ringLandShare * 999;
        if (!best || score > best.score) best = { lat, lon, roll, score };
      }
    };
    const finish = () => {
      best.landNodes = P.gridLand(rings, P.gridRotation(best.lat, best.lon, best.roll), 180, raster).landNodes;
      return best;
    };
    if (!onProgress) { step(N); return finish(); }
    return new Promise((ok) => {
      const run = () => { step(200); onProgress(i, N); if (i < N) setTimeout(run, 0); else ok(finish()); };
      run();
    });
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
  // The passing star's gravity widens the target (a typical star is about half the Sun's mass).
  P.starPassRate = function (dAU, vRel, starMsun) {
    const GM = K.GMsun * (starMsun || K.typicalStarMass);
    const d = dAU * K.AU / K.pc, focus = 1 + (2 * GM / (dAU * K.AU)) / (vRel * vRel);
    return K.starDensity * Math.PI * d * d * focus * (vRel * K.year / K.pc);
  };

  // ---------------------------------------------------------------------------------------------
  // The energy budget: what each store holds (J), as upper bounds (all of it, used perfectly)
  // ---------------------------------------------------------------------------------------------
  P.energyStores = function (years) {
    const sunlight = K.solarConst * Math.PI * K.Rearth ** 2;     // W reaching the Earth
    const hMass = K.oceanMass * 2.016 / 18.015;                     // hydrogen in the oceans (kg)
    const dMass = hMass * K.oceanDH * 2.014 / 1.008;            // of which deuterium (kg)
    const crust = K.crustMass;
    return {
      sunlight: { J: sunlight * years * K.year, label: "All the sunlight reaching the Earth, until departure", renew: true },
      deuterium: { J: dMass * K.ddEnergy, label: "Fusing all the deuterium in the oceans (" + Math.round(dMass / 1e15) + " trillion tonnes)" },
      fission: { J: crust * (K.crustU + K.crustTh) * K.fissionEnergy, label: "Fissioning all the crust's uranium and thorium" },
      heat: { J: K.geoHeat * years * K.year, label: "The Earth's internal heat, until departure", renew: true },
      spin: { J: P.spinEnergy(), label: "The Earth's whole spin" },
      jupiter: { J: K.GMsun * K.Mjupiter / (2 * K.aJupiter * K.AU), label: "Jupiter's orbital energy (reached only through gravity assists)" }
    };
  };
  // Light as the only thing thrown away: all the sunlight reaching the Earth, beamed out behind it,
  // pushes with P/c. The speed it gives the Earth over the years (m/s).
  P.photonDv = (years) => K.solarConst * Math.PI * K.Rearth ** 2 / K.c / K.Mearth * years * K.year;
  // Taking everyone by ship instead: energy to send people (with their ship's mass) from the
  // Earth's surface out of the solar system with speed vinf left.
  P.shipsForAll = function ({ people, kgPerPerson, vinf, launchPerYear }) {
    const vOrbitEsc = P.escapeDvImpulse(1, vinf);                       // from Earth's orbit
    const vSurface = Math.sqrt(K.vEscEarth ** 2 + vOrbitEsc ** 2);      // needed at the surface
    const mass = people * kgPerPerson;
    return { mass, vSurface, energy: 0.5 * mass * vSurface ** 2, years: mass / launchPerYear };
  };

  // ---------------------------------------------------------------------------------------------
  // The escape, step by step
  // ---------------------------------------------------------------------------------------------
  // A steady push along the Earth's motion (acceleration f, m/s^2) from a circular orbit at aAU,
  // until the Earth is leaving with vinf. Far from escape, the orbit stays nearly circular and
  // widens as da/dt = 2 f a^1.5 / sqrt(mu) (averaged over each orbit); once the push is no longer
  // tiny next to the Sun's pull (beyond a_s, where f = 10^-3 of it), the motion is followed directly
  // (fourth-order Runge-Kutta). Returns { table: [[years, AU]...], years, dv (m/s), aSwitch }.
  // f is chosen from the time wanted: about (circular speed + vinf) / time.
  P.spiralEscape = function ({ aAU, vinf, years }) {
    const mu = K.GMsun, a0 = aAU * K.AU, v0 = Math.sqrt(mu / a0);
    const f = (v0 + vinf) / (years * K.year);
    const aS = Math.max(a0, Math.sqrt(mu / (1e-3 * f)));
    const table = [[0, aAU]];
    // Averaged phase, solved exactly: a^-1/2 = a0^-1/2 - f t / sqrt(mu)
    const tS = Math.sqrt(mu) * (1 / Math.sqrt(a0) - 1 / Math.sqrt(aS)) / f;
    for (let i = 1; i <= 200; i++) {
      const t = tS * i / 200, a = 1 / (1 / Math.sqrt(a0) - f * t / Math.sqrt(mu)) ** 2;
      table.push([t / K.year, a / K.AU]);
    }
    // Direct phase: position and velocity in the plane, starting circular at aS
    let x = aS, y = 0, vx = 0, vy = Math.sqrt(mu / aS), t = tS;
    const acc = (x, y, vx, vy) => {
      const r = Math.hypot(x, y), v = Math.hypot(vx, vy), g = -mu / (r * r * r);
      return [g * x + f * vx / v, g * y + f * vy / v];
    };
    for (let step = 0; step < 200000; step++) {
      const r = Math.hypot(x, y), E = 0.5 * (vx * vx + vy * vy) - mu / r;
      if (E >= 0.5 * vinf * vinf && E > 0) break;
      // A step short against the orbit, the distance and the change of speed (once unbound the
      // "orbit" is no measure at all)
      const v = Math.hypot(vx, vy);
      const dt = Math.min(2 * Math.PI * Math.sqrt(r * r * r / mu) / 400, 0.005 * r / v, 0.005 * (v + 1) / f);
      const k1 = acc(x, y, vx, vy);
      const k2 = acc(x + vx * dt / 2, y + vy * dt / 2, vx + k1[0] * dt / 2, vy + k1[1] * dt / 2);
      const k3 = acc(x + (vx + k1[0] * dt / 2) * dt / 2, y + (vy + k1[1] * dt / 2) * dt / 2, vx + k2[0] * dt / 2, vy + k2[1] * dt / 2);
      const k4 = acc(x + (vx + k2[0] * dt / 2) * dt, y + (vy + k2[1] * dt / 2) * dt, vx + k3[0] * dt, vy + k3[1] * dt);
      x += dt * (vx + dt / 6 * (k1[0] + k2[0] + k3[0]));   // classical RK4 for x'' = acc
      y += dt * (vy + dt / 6 * (k1[1] + k2[1] + k3[1]));
      vx += dt / 6 * (k1[0] + 2 * k2[0] + 2 * k3[0] + k4[0]);
      vy += dt / 6 * (k1[1] + 2 * k2[1] + 2 * k3[1] + k4[1]);
      t += dt;
      if (step % 20 === 0) table.push([t / K.year, Math.hypot(x, y) / K.AU]);
    }
    table.push([t / K.year, Math.hypot(x, y) / K.AU]);
    return { table, years: t / K.year, dv: f * t, f, aSwitchAU: aS / K.AU, endAU: Math.hypot(x, y) / K.AU };
  };

  // Does the Moon come along? It stays bound while it is well inside the Earth's Hill sphere (the
  // region where the Earth's pull beats the Sun's tide), and while no single kick to the Earth is a
  // sizeable part of the Moon's orbital speed. Both only get easier as the Earth leaves the Sun.
  P.moonCheck = function ({ aAU, kickMs }) {
    const aMoon = 3.844e8, vMoon = 1022;
    const hill = aAU * K.AU * Math.cbrt(K.Mearth / (3 * K.Msun));
    return { hillShare: aMoon / hill, kickShare: kickMs / vMoon, stays: aMoon / hill < 0.5 && kickMs / vMoon < 0.1 };
  };

  // Moving the whole solar system: a mirror sends part of the Sun's light one way, and the Sun
  // drifts the other, taking its planets with it (a Shkadov thruster). At most about L / (2c) of
  // thrust. The speed it gives the Sun in a time (m/s), and the time for a given change (years).
  P.shkadov = function ({ share, dv }) {
    const F = (share == null ? 1 : share) * K.Lsun / (2 * K.c), a = F / K.Msun;
    return { thrust: F, accel: a, dvPerGyr: a * 1e9 * K.year, yearsFor: dv / a / K.year };
  };

  // The Earth's mean surface temperature from its sunlight (in units of today's), keeping today's
  // greenhouse warming, with the Earth's own heat as the floor when sunlight fades.
  P.surfaceTemp = function (S) {
    const sun = K.surfaceTempNow * Math.pow(Math.max(S, 0), 0.25);
    const geo = Math.pow(K.geoHeat / (4 * Math.PI * K.Rearth ** 2) / K.sigmaSB, 0.25);
    return Math.pow(sun ** 4 + geo ** 4, 0.25);
  };
  // Sunlight at which the mean surface temperature reaches T (K), in units of today's
  P.sunlightForTemp = (T) => Math.pow(T / K.surfaceTempNow, 4);

  // Keeping everyone alive: power, space under the ground, and how long each fuel lasts.
  //   people; kW per person for living; kW per person for growing food under lights; m^2 of floor
  //   per person; m per level; overhead (corridors, plant, farms) as a multiple of living floor.
  P.lifeSupport = function ({ people, kWLive, kWFood, m2, levelM, overhead }) {
    const W = people * (kWLive + kWFood) * 1000;
    const stores = P.energyStores(0);
    const volume = people * m2 * levelM * (1 + overhead);      // m^3 dug out
    return {
      watts: W, geoShare: K.geoHeat / W,
      deuteriumYears: stores.deuterium.J / W / K.year, fissionYears: stores.fission.J / W / K.year,
      volume, rock: volume * K.rockDensity, floorKm2: people * m2 * (1 + overhead) / 1e6
    };
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
  // Tables of [time, distance], both increasing: interpolate one column from the other by binary
  // search. Below the table: its first row; above: the last distance, or Infinity for a time.
  P.interp = function (table, x, from, to, beyond) {
    const n = table.length;
    if (x <= table[0][from]) return table[0][to];
    if (x > table[n - 1][from]) return beyond === undefined ? table[n - 1][to] : beyond;
    let lo = 0, hi = n - 1;
    while (hi - lo > 1) { const mid = (lo + hi) >> 1; if (table[mid][from] < x) lo = mid; else hi = mid; }
    const a = table[lo], b = table[hi];
    return a[to] + (b[to] - a[to]) * (x - a[from]) / (b[from] - a[from]);
  };
  P.timeToRadius = (table, rAU) => P.interp(table, rAU, 1, 0, Infinity);
  P.radiusAtTime = (table, tYr) => P.interp(table, tYr, 0, 1);

  // Crossing the Oort cloud: the expected number of comets the Earth meets, taking the cloud as an
  // even spread of N comets between rIn and rOut (AU), crossed straight through at speed v (m/s).
  // The Earth's gravity widens its target: sigma = pi R^2 (1 + vesc^2 / v^2).
  // The Earth's own speed there is sqrt(vinf^2 + 2 mu / r): slow when vinf is small, which both
  // lengthens the crossing and widens the Earth's gravitational target, so both are integrated.
  P.oortHazard = function ({ N, rInAU, rOutAU, vinf }) {
    const rin = rInAU * K.AU, rout = rOutAU * K.AU;
    const n = N / (4 / 3 * Math.PI * (rout ** 3 - rin ** 3));
    const v = (r) => Math.sqrt(vinf * vinf + 2 * K.GMsun / r);
    const sigma = (r) => Math.PI * K.Rearth ** 2 * (1 + (K.vEscEarth / v(r)) ** 2);
    const lambda = n * P.simpson(sigma, rin, rout, 2000);
    const years = P.simpson((r) => 1 / v(r), rin, rout, 2000) / K.year;
    return { lambda, pAny: 1 - Math.exp(-lambda), years, background: years / K.impactKm, density: n };
  };

  // ---------------------------------------------------------------------------------------------
  // The Galaxy (in pc, Msun and km/s)
  // ---------------------------------------------------------------------------------------------

  const Gpc = 4.300917270e-3;  // G in pc (km/s)^2 / Msun
  P.Gpc = Gpc;
  // Circular speeds squared (km/s)^2 of each part, at radius r in the plane.
  // The halo can be given an edge (P.haloEdge, pc; 0 for none): beyond it no more halo mass, so
  // its pull falls off as from a point. Inside, nothing changes.
  P.haloEdge = 0;
  const nfwM = (r, Ms) => { const x = r / K.mwHalo.rs; return Ms * (Math.log(1 + x) - x / (1 + x)); };
  function vc2Parts(r, Ms) {
    const d = K.mwDisk, b = K.mwBulge;
    const rh = P.haloEdge > 0 ? Math.min(r, P.haloEdge) : r;
    return {
      bh: Gpc * K.MsgrA / r,
      bulge: Gpc * b.M * r / ((r + b.a) ** 2),
      disk: Gpc * d.M * r * r / Math.pow(r * r + (d.a + d.b) ** 2, 1.5),
      halo: Ms ? Gpc * nfwM(rh, Ms) / r : 0
    };
  }
  // The halo's mass scale, chosen so the circular speed at R0 is the measured one (worked out again
  // whenever a constant it depends on changes).
  P.haloMs = (function () {
    let key = null, cache = null;
    return function () {
      const k = [K.R0, K.vcSun, K.MsgrA, K.mwDisk.M, K.mwBulge.M, K.mwHalo.rs].join("|");
      if (k === key) return cache;
      const p = vc2Parts(K.R0, 0), x = K.R0 / K.mwHalo.rs;
      cache = (K.vcSun ** 2 - p.bh - p.bulge - p.disk) * K.R0 / (Gpc * (Math.log(1 + x) - x / (1 + x)));
      key = k;
      return cache;
    };
  })();
  P.galVc = (r) => { const p = vc2Parts(r, P.haloMs()); return Math.sqrt(p.bh + p.bulge + p.disk + p.halo); };
  // The potential in the plane (km/s)^2, zero far away.
  P.galPhi = function (r) {
    const d = K.mwDisk, b = K.mwBulge, rs = K.mwHalo.rs, Ms = P.haloMs(), e = P.haloEdge;
    let halo;
    if (e > 0 && r >= e) halo = -Gpc * nfwM(e, Ms) / r;
    else halo = -Gpc * Ms * Math.log(1 + r / rs) / r + (e > 0 ? Gpc * (Ms * Math.log(1 + e / rs) - nfwM(e, Ms)) / e : 0);
    return -Gpc * K.MsgrA / r - Gpc * b.M / (r + b.a) - Gpc * d.M / Math.sqrt(r * r + (d.a + d.b) ** 2) + halo;
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
    const Vt = K.vcSun + K.vSunPec[1], U = K.vSunPec[0], W = K.vSunPec[2];
    const phi0 = P.galPhi(R0);
    // Energy after the drop (per unit mass), with a small tangential speed vt kept
    const vpOf = (vt) => Math.sqrt(2 * (0.5 * vt * vt + phi0 - P.galPhi(rp)));
    // Angular momentum is kept: R0 vt = rp vp
    const vt = P.bisect((x) => R0 * x - rp * vpOf(x), 0, Vt, 200);
    const vp = vpOf(vt);
    const dvDrop = Math.hypot(Vt - vt, U, W);
    const E = 0.5 * vt * vt + phi0, h = R0 * vt;
    // The fall, from R0 (where the Earth starts, at rest but for vt: its farthest point) to rp
    const fallTable = P.orbitLeg(E, h, rp, R0);
    // The burn at closest approach, and what is left far away
    const Eafter = 0.5 * (vp + burnKms) ** 2 + P.galPhi(rp), hAfter = rp * (vp + burnKms);
    // Where it climbs back to, if it stays bound (Infinity if it escapes)
    const apoAfter = P.apocentre(Eafter, hAfter, rp);
    const vinf = isFinite(apoAfter) && Eafter <= 0 ? 0 : Math.sqrt(Math.max(0, 2 * Eafter));
    // Safety: where the black hole would tear the Earth apart, and its event horizon
    const MbhKg = K.MsgrA * K.Msun;
    const rTidalAU = K.Rearth * Math.cbrt(MbhKg / K.Mearth) / K.AU;
    const rsAU = 2 * K.G * MbhKg / (K.c * K.c) / K.AU;
    // Leaving straight from here instead, without the Galactic Centre
    const directDv = P.galVesc(R0) - Math.hypot(Vt, U, W);
    return { vt, vp, dvDrop, fallYears: fallTable[fallTable.length - 1][0], fallTable, Eafter, hAfter, vinf, apoAfter,
      rTidalAU, rsAU, vpOverC: vp * 1000 / K.c, directDv, vescR0: P.galVesc(R0), vcR0: P.galVc(R0) };
  };
  // The farthest point of an orbit with energy E and angular momentum h, beyond r1 (pc), where
  // 2 (E - phi) = h^2 / r^2. Infinity if it is not reached within 10^9 pc (it escapes).
  P.apocentre = function (E, h, r1) {
    const f = (r) => 2 * (E - P.galPhi(r)) - h * h / (r * r);
    let hi = Math.max(r1 * 2, 1);
    while (f(hi) > 0) { hi *= 2; if (hi > 1e9) return Infinity; }
    return P.bisect(f, r1 * 1.000001, hi, 200);
  };
  // Time against distance along one leg of an orbit between r1 (closest) and r2 (farthest, or the
  // end of the table), as [[years from r1, pc]...]. Both turning points make dt/dr infinite, so the
  // substitution r = r1 + (r2 - r1)(1 - cos u)/2 is used: it makes the integrand finite.
  P.orbitLeg = function (E, h, r1, r2, n) {
    n = n || 3000;
    const conv = K.pc / 1000 / K.year, half = (r2 - r1) / 2;
    const vr = (r) => Math.sqrt(Math.max(1e-300, 2 * (E - P.galPhi(r)) - h * h / (r * r)));
    const out = [[0, r1]];
    let t = 0;
    for (let i = 0; i < n; i++) {
      const u = Math.PI * (i + 0.5) / n, r = r1 + half * (1 - Math.cos(u));
      t += half * Math.sin(u) / vr(r) * (Math.PI / n) * conv;     // midpoint rule in u
      out.push([t, r1 + half * (1 - Math.cos(Math.PI * (i + 1) / n))]);
    }
    return out;
  };
  // Distance against time for the route: the fall from R0 to the closest approach, then the climb
  // out after the burn. Returns { fall: [[years before closest approach, pc]...],
  // exit: [[years after, pc]...], exitBound }.
  P.galTables = function (g) {
    const rp = g.fallTable[0][1];
    const bound = isFinite(g.apoAfter);
    let exit;
    if (bound) exit = P.orbitLeg(g.Eafter, g.hAfter, rp, g.apoAfter);
    else {
      // Unbound: substitute r = rp + w^2 (removes the turning point at rp), out to 5 Mpc
      const conv = K.pc / 1000 / K.year, n = 4000, wMax = Math.sqrt(5e6 - rp);
      const vr = (r) => Math.sqrt(Math.max(1e-300, 2 * (g.Eafter - P.galPhi(r)) - g.hAfter * g.hAfter / (r * r)));
      exit = [[0, rp]];
      let t = 0;
      for (let i = 0; i < n; i++) {
        const w = wMax * (i + 0.5) / n;
        t += 2 * w / vr(rp + w * w) * (wMax / n) * conv;
        const wb = wMax * (i + 1) / n;
        exit.push([t, rp + wb * wb]);
      }
    }
    return { fall: g.fallTable, exit, exitBound: bound };
  };
  // Look up a [time, distance] table at time t (linear between rows; the last row beyond).
  P.lookup = (table, t) => P.interp(table, t, 0, 1);
  // ... and the time at which a table reaches distance r.
  P.lookupTime = (table, r) => P.interp(table, r, 1, 0, Infinity);

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
