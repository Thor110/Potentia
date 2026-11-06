// The OLAP simulation page: the plan's settings, the results worked out from them, the timeline,
// and the panels, ledger and tables that show it all. Physics is in physics.js, constants in
// constants.js, data in datasets.js, drawing in views.js.
(function () {
  "use strict";
  const OLAP = window.OLAP, C = OLAP.C, K = OLAP.K, P = OLAP.P, V = OLAP.views;
  const $ = (id) => document.getElementById(id);

  // ---------------------------------------------------------------------------------------------
  // Formatting
  // ---------------------------------------------------------------------------------------------
  function sci(x, d) {
    if (x == null || !isFinite(x)) return x === Infinity ? "∞" : "—";
    if (x === 0) return "0";
    const e = Math.floor(Math.log10(Math.abs(x)));
    if (e >= -2 && e < 5) return Number(x.toPrecision(d || 3)).toLocaleString("en-GB");
    const m = x / Math.pow(10, e);
    return (Math.round(m * 10 ** ((d || 3) - 1)) / 10 ** ((d || 3) - 1)) + "×10<sup>" + e + "</sup>";
  }
  const grp = (x) => Math.round(x).toLocaleString("en-GB");
  function years(y) {
    if (!isFinite(y)) return "never";
    const a = Math.abs(y);
    if (a < 1 / 365.25 / 24 / 60) return sci(y * K.year, 2) + " seconds";
    if (a < 1 / 365.25 / 24) return Number((y * K.year / 60).toPrecision(2)).toLocaleString("en-GB") + " minutes";
    if (a < 1 / 365.25) return Number((y * K.year / 3600).toPrecision(2)).toLocaleString("en-GB") + " hours";
    if (a < 1) return Number((y * 365.25).toPrecision(2)).toLocaleString("en-GB") + " days";
    if (a < 1e4) return grp(y) + " years";
    if (a < 1e6) return Number((y / 1e3).toPrecision(3)) + " thousand years";
    if (a < 1e9) return Number((y / 1e6).toPrecision(3)) + " million years";
    if (a < 1e12) return Number((y / 1e9).toPrecision(3)) + " billion years";
    return sci(y) + " years";
  }
  const kms = (v) => Number((v / 1000).toPrecision(3)).toLocaleString("en-GB") + " km/s";
  function massCmp(kg) {
    if (kg >= 0.5 * K.Mmoon) return "= " + sci(kg / K.Mearth, 2) + " Earth masses";
    if (kg >= 0.05 * K.Mceres) return "= " + sci(kg / K.Mceres, 2) + " × the dwarf planet Ceres";
    if (kg >= K.oceanMass * 0.01) return "= " + sci(kg / K.oceanMass, 2) + " × all the Earth's oceans";
    return "= " + sci(kg / K.anthroMass, 2) + " × everything humanity has ever built";
  }
  function energyCmp(J) {
    const sunlight = K.solarConst * Math.PI * K.Rearth ** 2;
    return "= " + years(J / K.worldPower / K.year) + " of today's world energy use, or " +
      years(J / sunlight / K.year) + " of all the sunlight reaching the Earth, or " +
      years(J / K.Lsun / K.year) + " of the Sun's entire output";
  }

  // ---------------------------------------------------------------------------------------------
  // The plan: every setting, its default (the published plan where it gives one), and its control
  // ---------------------------------------------------------------------------------------------
  const MAT = C.materials.v, BODIES = C.bodies.v;
  const FIELDS = [
    { panel: "fw", key: "fwBody", label: "Body being steered", type: "select", def: "earth", options: BODIES.map(b => [b.id, b.name]) },
    { panel: "fw", key: "fwGoal", label: "What the steering must do", type: "select", def: "fraction",
      options: [["fraction", "Shift a share of the spin (as published)"], ["tilt", "Tilt the axis by an angle"], ["day", "Change the length of the day"]] },
    { panel: "fw", key: "fwFraction", label: "Share of the spin", type: "log", min: 1e-9, max: 0.1, def: 1e-3, fmt: (v) => sci(v * 100) + "%", show: (p) => p.fwGoal === "fraction" },
    { panel: "fw", key: "fwTilt", label: "Tilt", type: "log", min: 1e-6, max: 90, def: 1, fmt: (v) => sci(v) + "°", show: (p) => p.fwGoal === "tilt" },
    { panel: "fw", key: "fwDay", label: "Change in the day", type: "log", min: 1e-6, max: 3600, def: 1, fmt: (v) => sci(v) + " s", show: (p) => p.fwGoal === "day" },
    { panel: "fw", key: "fwGeom", label: "What spins", type: "select", def: "ring",
      options: [["ring", "Trains circling each ring: the circle itself"], ["along", "Wheels all along the rings"], ["node", "Wheels at the 26 stations"]] },
    { panel: "fw", key: "trainOrbit", label: "Trains at orbital speed for this body (the fastest without being held down)", type: "check", def: true, show: (p) => p.fwGeom === "ring" },
    { panel: "fw", key: "trainV", label: "Train speed", type: "log", min: 1, max: 12000, def: 7900, fmt: (v) => grp(v) + " m/s (" + grp(v * 3.6) + " km/h)", show: (p) => p.fwGeom === "ring" && !p.trainOrbit },
    { panel: "fw", key: "trainsPerRing", label: "Trains per ring", type: "log", min: 1, max: 1e5, def: 1000, int: true, fmt: (v) => grp(v), show: (p) => p.fwGeom === "ring" },
    { panel: "fw", key: "fwPerRing", label: "Wheels per ring", type: "log", min: 1, max: 1e5, def: 100, int: true, fmt: (v) => grp(v), show: (p) => p.fwGeom === "along" },
    { panel: "fw", key: "fwPerNode", label: "Wheels per station", type: "log", min: 1, max: 1000, def: 1, int: true, fmt: (v) => grp(v), show: (p) => p.fwGeom === "node" },
    { panel: "fw", key: "fwRadius", label: "Wheel radius", type: "log", min: 10, max: 1e5, def: 1000, fmt: (v) => sci(v) + " m", show: (p) => p.fwGeom !== "ring" },
    { panel: "fw", key: "fwMaterial", label: "How fast the rim may go", type: "select", def: "cfrp", options: MAT.filter(m => m.id !== "stream").map(m => [m.id, m.name + " (" + Math.round(P.materialSpeed(m)) + " m/s)"]), show: (p) => p.fwGeom !== "ring" },
    { panel: "fw", key: "fwPowerTW", label: "Power to spin them up", type: "log", min: 0.01, max: 1e5, def: K.worldPower / 1e12, fmt: (v) => sci(v) + " TW" },

    { panel: "grid", key: "gLat", label: "First quad-station: latitude", type: "range", min: -90, max: 90, step: 0.5, def: 90, fmt: (v) => v + "°" },
    { panel: "grid", key: "gLon", label: "…longitude", type: "range", min: -180, max: 180, step: 0.5, def: 0, fmt: (v) => v + "°" },
    { panel: "grid", key: "gRoll", label: "…turned about it by", type: "range", min: 0, max: 90, step: 0.5, def: 0, fmt: (v) => v + "°" },
    { panel: "grid", key: "poleQuad", label: "Keep a quad-station on the North Pole when searching", type: "check", def: false },

    { panel: "sun", key: "hzS", label: "Too much sunlight (runaway greenhouse)", type: "range", min: 1.0, max: 1.4, step: 0.005, def: K.hzRunaway, fmt: (v) => "× " + Number(v).toFixed(3) + " today's" },
    { panel: "sun", key: "migrate", label: "Move the Earth outward as the Sun brightens", type: "check", def: true },
    { panel: "sun", key: "keepS", label: "…keeping sunlight at", type: "range", min: 0.8, max: 1.1, step: 0.005, def: 1, fmt: (v) => "× " + Number(v).toFixed(3) + " today's", show: (p) => p.migrate },
    { panel: "sun", key: "depYr", label: "Leave the Sun in", type: "log", min: 1e3, max: 6e9, def: 1e9, fmt: (v) => years(v) },
    { panel: "sun", key: "vinf", label: "Speed left once clear of the Sun", type: "range", min: 0, max: 100, step: 0.1, def: 29.6, fmt: (v) => v + " km/s" },
    { panel: "sun", key: "escDur", label: "Time taken by the escape push", type: "log", min: 0.01, max: 1e9, def: 1e7, fmt: (v) => years(v) },
    { panel: "sun", key: "method", label: "What pushes the Earth", type: "select", def: "flyby",
      options: [["flyby", "Gravity assists from asteroids, taking energy from Jupiter"], ["sun", "Nudges against the Sun's pull, by the steering system"], ["flywheels", "The flywheels alone"], ["driver", "Throwing mass off the Earth (only if it becomes necessary)"]] },
    { panel: "sun", key: "sunF", label: "Share of the Earth's mass shifted each orbit", type: "log", min: 1e-7, max: 0.1, def: 1e-3, fmt: (v) => sci(v * 100) + "%", show: (p) => p.method === "sun" },
    { panel: "sun", key: "flyMass", label: "Asteroid mass", type: "log", min: 1e16, max: 1e22, def: 1e19, fmt: (v) => sci(v) + " kg", show: (p) => p.method === "flyby" },
    { panel: "sun", key: "flyVrel", label: "Its speed past the Earth", type: "range", min: 1, max: 30, step: 0.5, def: 5, fmt: (v) => v + " km/s", show: (p) => p.method === "flyby" },
    { panel: "sun", key: "flyRp", label: "Closest approach (from the Earth's centre)", type: "log", min: 7000, max: 1e6, def: 1e4, fmt: (v) => grp(v) + " km", show: (p) => p.method === "flyby" },
    { panel: "sun", key: "flyEta", label: "Share of each kick along the Earth's path", type: "range", min: 0.05, max: 1, step: 0.05, def: 0.5, fmt: (v) => Math.round(v * 100) + "%", show: (p) => p.method === "flyby" },
    { panel: "sun", key: "drvV", label: "Launch speed", type: "log", min: 12, max: 150000, def: 30, fmt: (v) => kms(v * 1000), show: (p) => p.method === "driver" },
    { panel: "sun", key: "olaPayload", label: "Mass per launch", type: "log", min: 1e3, max: 1e12, def: 1e6, fmt: (v) => sci(v) + " kg", show: (p) => p.method === "driver" },
    { panel: "sun", key: "oortN", label: "Comets in the Oort cloud", type: "log", min: 1e10, max: 1e13, def: K.oortCount, fmt: (v) => sci(v) },

    { panel: "gal", key: "galOn", label: "Go on through the Galactic Centre", type: "check", def: true },
    { panel: "gal", key: "rpAU", label: "Closest approach to Sgr A*", type: "log", min: 0.2, max: 1e5, def: 300, fmt: (v) => sci(v) + " AU", show: (p) => p.galOn },
    { panel: "gal", key: "burnKms", label: "Push at closest approach", type: "log", min: 0.01, max: 3000, def: 30, fmt: (v) => sci(v) + " km/s", show: (p) => p.galOn },
    { panel: "gal", key: "galDrop", label: "How the drop and the push are made", type: "select", def: "open", options: [["open", "Not yet known: the open problem"], ["driver", "Throwing mass off the Earth (only if it becomes necessary)"]], show: (p) => p.galOn },
    { panel: "gal", key: "galVex", label: "Launch speed for the galactic pushes", type: "log", min: 30, max: 150000, def: 30000, fmt: (v) => kms(v * 1000), show: (p) => p.galOn && p.galDrop === "driver" },
    { panel: "gal", key: "target", label: "Destination", type: "select", def: "Andromeda", options: [], show: (p) => p.galOn },
    { panel: "gal", key: "H0", label: "Hubble constant", type: "select", def: String(K.H0),
      options: [[String(K.H0), K.H0 + " km/s/Mpc (Planck 2018)"], [String(K.H0local), K.H0local + " km/s/Mpc (SH0ES 2022)"]] },

    { panel: "col", key: "colV", label: "Colony ship speed", type: "log", min: 1e-4, max: 0.5, def: 0.01, fmt: (v) => sci(v * 100) + "% of light" }
  ];
  const DEF = {}; FIELDS.forEach(f => DEF[f.key] = f.def);
  let plan = Object.assign({}, DEF);

  // Targets before the Local Group catalogue has loaded (McConnachie 2012's values).
  const FALLBACK_TARGETS = [
    { name: "Andromeda", l: 121.2, b: -21.6, D: 783 }, { name: "Triangulum", l: 133.6, b: -31.3, D: 809 },
    { name: "LMC", l: 280.5, b: -32.9, D: 51 }, { name: "SMC", l: 302.8, b: -44.3, D: 64 }
  ];
  const targets = () => (OLAP.data.loaded.localgroup || FALLBACK_TARGETS).filter(g => g.D > 0);

  // ---------------------------------------------------------------------------------------------
  // Working out the plan
  // ---------------------------------------------------------------------------------------------
  let landRings = null;
  // The speed from 1 AU that gives the published 7,961 years to 50,000 AU (it depends on nothing in the plan)
  const VINF_FOR_DOC = P.bisect((x) => P.timeToRadius(P.cruiseTable(1, x, 6e4), K.docExitAU) - K.docExitYears, 1000, 200000, 40);
  let R = {};
  function compute() {
    const p = plan, r = {};
    // Flywheels
    const goal = p.fwGoal === "tilt" ? { kind: "tilt", tiltDeg: p.fwTilt } : p.fwGoal === "day" ? { kind: "day", dayS: p.fwDay } : { kind: "fraction", fraction: p.fwFraction };
    const body = BODIES.find(b => b.id === p.fwBody) || BODIES[0], isEarth = body.id === "earth";
    const dL = isEarth ? P.momentumForGoal(goal) : P.momentumForGoal(goal, body);
    const mat = MAT.find(m => m.id === p.fwMaterial) || MAT[0];
    const geom = p.fwGeom, ring = geom === "ring";
    const v = ring ? (p.trainOrbit ? P.surfaceOrbitSpeed(body) : p.trainV) : P.materialSpeed(mat);
    const rad = ring ? body.R - Math.min(2000, 0.02 * body.R) : p.fwRadius;   // trains run 2 km down (less in small bodies)
    const n = ring ? 9 : geom === "along" ? 9 * Math.round(p.fwPerRing) : 26 * Math.round(p.fwPerNode);
    r.fw = Object.assign(P.flywheels({ dL, r: rad, v, n, power: p.fwPowerTW * 1e12 }), { v, r: rad, n, ring, geom, mat, goal, body, isEarth, L: isEarth ? P.earthL() : P.bodyL(body) });
    if (ring) {
      const fw = r.fw;
      fw.trains = 9 * Math.round(p.trainsPerRing);
      fw.perTrain = fw.totalMass / fw.trains;
      fw.ringLen = 2 * Math.PI * rad;
      fw.perMetre = fw.totalMass / (9 * fw.ringLen);
      fw.weight = P.trainWeight(body, v);
      fw.vOrb = P.surfaceOrbitSpeed(body);
    }
    r.proof = P.publishedProof();
    // Grid
    r.gridR = P.gridRotation(p.gLat, p.gLon, p.gRoll);
    r.gridInfo = landRings ? P.gridLand(landRings, r.gridR, 180) : { nodes: P.gridNodes.map(nd => { const [lat, lon] = P.toLatLon(P.mulMV(r.gridR, nd.p)); return { kind: nd.kind, lat, lon, land: false }; }), landNodes: 0, ringLandShare: 0 };
    // The fixed vector the rings hold: across the axis for a tilt, along it for the day
    r.H = goal.kind === "day" ? [0, 0, dL] : [dL, 0, 0];
    // The Sun
    r.tHZ = P.yearsUntilInsolation(p.hzS, 1);
    r.tMS = K.sunMSEndAge - K.sunAge;
    r.tMigStart = p.migrate ? Math.max(0, P.yearsUntilInsolation(p.keepS, 1)) : Infinity;
    r.aAt = (t) => (p.migrate && t > r.tMigStart) ? Math.max(1, P.aForInsolation(Math.min(t, r.tMS), p.keepS)) : 1;
    const tDep = p.depYr;
    r.aDep = r.aAt(tDep);
    r.migE = P.migrationEnergy(1, r.aDep);
    const vinf = p.vinf * 1000;
    const period = 2 * Math.PI * Math.sqrt((r.aDep * K.AU) ** 3 / K.GMsun) / K.year;
    r.impulsive = p.escDur < 0.1 * period;
    r.escDv = r.impulsive ? P.escapeDvImpulse(r.aDep, vinf) : P.escapeDvSpiral(r.aDep, vinf);
    r.escDvImp = P.escapeDvImpulse(r.aDep, vinf);
    r.escE = P.escapeEnergy(r.aDep, vinf);
    r.migDv = P.migrationDvSpiral(1, r.aDep);
    r.lateDeparture = !p.migrate && tDep > r.tHZ;
    r.pastMS = tDep > r.tMS;
    // Methods
    r.fb = P.flyby({ mObj: p.flyMass, vRel: p.flyVrel * 1000, rp: p.flyRp * 1000, eta: p.flyEta, aAU: 1 });
    r.fbMigN = r.migE / r.fb.dE;
    r.fbMigEvery = r.fbMigN > 0 ? (tDep - Math.min(tDep, r.tMigStart)) / r.fbMigN : Infinity;
    r.fbEscN = r.escE / r.fb.dE;
    r.fbEscEvery = p.escDur / r.fbEscN;
    // An asteroid shuttling between the Earth (1 AU) and Jupiter takes about one orbit of a 3.1 AU ellipse
    r.shuttleYr = Math.pow((1 + K.aJupiter) / 2, 1.5);
    r.fbAsteroids = Math.max(1, Math.ceil(r.shuttleYr / Math.min(r.fbEscEvery, r.fbMigEvery)));
    r.jupA = P.jupiterShift(r.migE + r.escE);
    r.mdMig = P.massDriver({ dv: r.migDv, vLaunch: p.drvV * 1000 });
    r.mdEsc = P.massDriver({ dv: r.escDv, vLaunch: p.drvV * 1000, M: K.Mearth - (isFinite(r.mdMig.massOut) ? r.mdMig.massOut : 0) });
    r.mdLaunches = (r.mdMig.massOut + r.mdEsc.massOut) / p.olaPayload;
    r.sunN = P.sunNudges({ f: p.sunF, aAU: 1, need: r.migE + r.escE });
    r.spinE = P.spinEnergy();
    r.mdRate = r.mdEsc.massOut / p.olaPayload / (p.escDur * K.year) / (26 * K.docOLAperNode);
    r.escPower = r.escE / (p.escDur * K.year);
    // Leaving
    r.cruise = P.cruiseTable(r.aDep, Math.max(vinf, 1), 3e5);
    r.tHelio = P.timeToRadius(r.cruise, K.heliopause);
    r.t50k = P.timeToRadius(r.cruise, K.docExitAU);
    r.tOort = P.timeToRadius(r.cruise, K.oortOuter);
    r.vinfForDoc = VINF_FOR_DOC;
    r.oort = P.oortHazard({ N: p.oortN, rInAU: K.oortInner, rOutAU: K.oortOuter, v: Math.max(vinf, 1) });
    r.tSurf = Math.pow(K.geoHeat / (4 * Math.PI * K.Rearth ** 2) / K.sigmaSB, 0.25);
    // The Galaxy
    r.galOn = p.galOn;
    r.gal = P.galRoute({ rpAU: p.rpAU, burnKms: p.burnKms });
    r.galTab = P.galTables(r.gal, p.rpAU, p.burnKms);
    r.tLeave = tDep + p.escDur;   // the escape push runs from tDep to tLeave; the coast starts then
    r.tDrop = r.tLeave + (isFinite(r.tOort) ? r.tOort : 0);
    r.tGC = r.tDrop + r.galTab.fall[r.galTab.fall.length - 1][0];
    const gv = p.galVex * 1000;
    r.mdDrop = P.massDriver({ dv: r.gal.dvDrop * 1000, vLaunch: gv });
    r.mdBurn = P.massDriver({ dv: p.burnKms * 1000, vLaunch: gv, M: K.Mearth - r.mdDrop.massOut });
    r.dropE = 0.5 * K.Mearth * (r.gal.dvDrop * 1000) ** 2;
    r.directMd = P.massDriver({ dv: r.gal.directDv * 1000, vLaunch: gv });
    r.mdDropRef = P.massDriver({ dv: r.gal.dvDrop * 1000, vLaunch: 0.1 * K.c });
    r.starRate = P.starPassRate(1, 30000);   // a star within 1 AU, at a typical 30 km/s
    r.tooClose = p.rpAU < 2 * r.gal.rTidalAU;
    r.amongS = p.rpAU < 2000;
    // Destination
    const tg = targets().find(g => g.name === p.target) || targets().find(g => g.name === "Andromeda") || targets()[0];
    r.targetObj = tg;
    const h = V.galXYZ(tg.l, tg.b, tg.D * 1000);
    const tgc = [h[0] - K.R0, h[1], h[2]];
    r.targetDistPc = Math.hypot(...tgc);
    r.targetDir = tgc.map(x => x / r.targetDistPc);
    r.tTravel = r.gal.vinf > 0 ? P.lookupTime(r.galTab.exit, r.targetDistPc) : Infinity;
    r.tArrive = r.tGC + r.tTravel;
    r.cosmo = P.cosmo(Number(p.H0), K.OmegaM);
    r.reachMpc = r.cosmo.reach(r.gal.vinf * 1000);
    r.lightReachGly = P.MpcToGly(r.cosmo.reach(K.c));
    // Route drawn in the Galaxy's plane (pc)
    const sunAt = (t) => { const th = Math.PI - (K.vcSun * 1000 / (K.R0 * K.pc)) * t * K.year; return [K.R0 * Math.cos(th), K.R0 * Math.sin(th)]; };
    r.sunAt = sunAt;
    const sd = sunAt(r.tDrop), sdl = Math.hypot(...sd);
    const dir2 = [r.targetDir[0], r.targetDir[1]], d2l = Math.hypot(...dir2) || 1;
    r.route = r.galOn ? {
      drop: sd, dropUnit: [sd[0] / sdl, sd[1] / sdl], outUnit: [dir2[0] / d2l, dir2[1] / d2l], proj: d2l,
      target: [tgc[0], tgc[1]], targetMpc: h.map(x => x / 1e6),
      path: [sd, [0, 0], r.gal.vinf > 0 ? [tgc[0], tgc[1]] : [dir2[0] / d2l * r.gal.apoAfter * d2l, dir2[1] / d2l * r.gal.apoAfter * d2l]]
    } : null;
    // 2MRS reach
    const g2 = OLAP.data.loaded["2mrs"];
    if (g2) {
      let inside = 0;
      const cz = r.reachMpc;
      for (const q of g2) if (r.cosmo.dcFast(Math.max(q.cz, 0) / (K.c / 1000)) <= cz) inside++;
      r.reach2mrs = { inside, all: g2.length };
    }
    // Events on the timeline
    const ev = [
      { t: r.tMigStart, label: "the Earth starts moving outward", show: p.migrate && r.tMigStart < tDep },
      { t: r.tHZ, label: "sunlight at 1 AU passes the runaway-greenhouse limit", show: true },
      { t: K.gliese710.years, label: "Gliese 710 passes through the Oort cloud", show: true },
      { t: tDep, label: "departure: the escape push begins", show: true },
      { t: r.tLeave, label: "the escape push ends: the Earth coasts away", show: true },
      { t: r.tLeave + r.tHelio, label: "past the heliopause", show: true },
      { t: r.tLeave + r.t50k, label: "50,000 AU (the published exit)", show: true },
      { t: r.tLeave + r.tOort, label: "clear of the Oort cloud", show: true },
      { t: r.tGC, label: "closest approach to Sgr A*", show: r.galOn },
      { t: r.tArrive, label: "arrival: " + tg.name, show: r.galOn && isFinite(r.tArrive) },
      { t: r.tMS, label: "the Sun leaves the main sequence", show: true },
      { t: K.sunRGBTipFromNow, label: "the Sun at its largest: Earth's old orbit engulfed", show: true }
    ];
    r.events = ev.filter(e => e.show && isFinite(e.t)).sort((a, b) => a.t - b.t);
    R = r;
  }

  // Where the Earth is at time t (years from now)
  function earthState(t) {
    const r = R, out = { phase: "", au: null, gal: null };
    const tDep = plan.depYr;
    if (t < tDep) { out.au = r.aAt(t); out.phase = t < 1e4 ? "Building the grid, the flywheels and the accelerators" : t < r.tMigStart || !plan.migrate ? "In orbit round the Sun" : "Migrating outward as the Sun brightens"; }
    else if (t < r.tLeave) { out.au = r.aDep; out.phase = "The escape push is under way"; }
    else if (t < r.tDrop || !r.galOn) {
      out.au = P.radiusAtTime(r.cruise, t - r.tLeave);
      out.phase = t - r.tLeave < r.tHelio ? "Leaving the Sun" : t - r.tLeave < r.tOort ? "Crossing the Oort cloud" : "Between the stars, still moving with the Sun round the Galaxy";
    }
    out.gal = r.sunAt(Math.min(t, r.galOn ? r.tDrop : t));
    if (r.galOn && t >= r.tDrop) {
      const fallT = r.tGC - r.tDrop;
      if (t < r.tGC) {
        const d = P.lookup(r.galTab.fall, fallT - (t - r.tDrop));
        out.gal = [r.route.dropUnit[0] * d, r.route.dropUnit[1] * d];
        out.phase = "Falling towards the Galactic Centre";
      } else {
        const d = P.lookup(r.galTab.exit, t - r.tGC);
        const dd = Math.min(d, r.gal.vinf > 0 ? r.targetDistPc : Infinity);
        out.gal = [r.route.outUnit[0] * dd * r.route.proj, r.route.outUnit[1] * dd * r.route.proj];
        out.phase = t < r.tArrive ? (r.gal.vinf > 0 ? "Bound for " + r.targetObj.name : "Flung out, but still bound to the Milky Way") : "Arrived at " + r.targetObj.name;
        out.distPc = dd;
      }
    }
    return out;
  }

  // ---------------------------------------------------------------------------------------------
  // Panels
  // ---------------------------------------------------------------------------------------------
  const PANELS = [
    { id: "fw", title: "1 · Steering: the flywheels", why: "Mass set spinning inside a planet takes angular momentum from it, as reaction wheels turn a satellite: this is the platform's steering wheel. Whatever turn it gives lasts only while the mass keeps spinning. The same system could steer the Moon, Mars, Ceres or an asteroid." },
    { id: "grid", title: "The planetary grid", why: "Nine great circles, the cube's planes of symmetry, cross at 6 quad-stations, 12 dual-stations and 8 tri-stations. The plan wants as many stations as possible on land." },
    { id: "sun", title: "2 · Moving: leaving the Sun", why: "The Sun brightens by about 1% every 110 million years. The plan's 1–2 billion years is about when the Earth's oceans would begin to boil away where it is now." },
    { id: "gal", title: "3 · The route through the Galaxy", why: "The published route falls to the Galactic Centre and slingshots round it towards another galaxy." }
  ];
  const ctrl = {};
  const LOGN = 1000;
  function buildPanels() {
    const host = $("panels");
    for (const pn of PANELS) {
      const el = document.createElement("div"); el.className = "panel";
      el.innerHTML = `<h2>${pn.title}</h2><div class="why">${pn.why}</div><div class="fields"></div><div class="extra"></div><div class="out" id="out-${pn.id}"></div>`;
      host.appendChild(el);
      pn.el = el;
    }
    for (const f of FIELDS) {
      const pn = PANELS.find(q => q.id === f.panel);
      const hostEl = pn ? pn.el.querySelector(".fields") : document.createElement("div"); // colony speed: moved into place later
      const wrap = document.createElement("div"); wrap.className = "field" + (f.type === "check" ? " check" : "");
      const id = "f-" + f.key;
      if (f.type === "check") {
        wrap.innerHTML = `<label><input type="checkbox" id="${id}"> &nbsp;<span>${f.label}</span></label>`;
      } else if (f.type === "select") {
        wrap.innerHTML = `<label for="${id}"><span>${f.label}</span><span></span></label><select id="${id}"></select>`;
      } else {
        wrap.innerHTML = `<label for="${id}"><span>${f.label}</span><span id="${id}-v"></span></label><input type="range" id="${id}">`;
      }
      hostEl.appendChild(wrap);
      const input = wrap.querySelector("input,select");
      if (f.type === "select") fillOptions(f, input);
      if (f.type === "log") { input.min = 0; input.max = LOGN; input.step = 1; }
      if (f.type === "range") { input.min = f.min; input.max = f.max; input.step = f.step; }
      input.addEventListener("input", () => {
        let v;
        if (f.type === "check") v = input.checked;
        else if (f.type === "select") v = input.value;
        else if (f.type === "log") { v = f.min * Math.pow(f.max / f.min, input.value / LOGN); if (f.int) v = Math.round(v); else v = Number(v.toPrecision(4)); }
        else v = Number(input.value);
        plan[f.key] = v;
        changed();
      });
      ctrl[f.key] = { f, input, wrap };
    }
    // Extras
    const g = PANELS[1].el.querySelector(".extra");
    g.innerHTML = `<button class="btn-small" id="opt-grid">Search for the most stations on land</button><span class="note" id="opt-status"></span>`;
    $("opt-grid").addEventListener("click", optimiseGrid);
    const f = PANELS[0].el.querySelector(".extra");
    f.innerHTML = `<canvas class="chart" id="chart-fw" aria-label="Momentum held against time"></canvas>`;
  }
  function fillOptions(f, sel) {
    const opts = f.key === "target" ? targets().slice().sort((a, b) => a.D - b.D).map(g => [g.name, g.name + " (" + (g.D >= 1000 ? (g.D / 1000).toFixed(2) + " Mpc" : g.D + " kpc") + ")"]) : f.options;
    sel.innerHTML = opts.map(([v, t]) => `<option value="${v}">${t}</option>`).join("");
  }
  function syncControls() {
    for (const k in ctrl) {
      const { f, input, wrap } = ctrl[k], v = plan[k];
      if (f.type === "check") input.checked = !!v;
      else if (f.type === "select") input.value = String(v);
      else if (f.type === "log") input.value = Math.round(LOGN * Math.log(v / f.min) / Math.log(f.max / f.min));
      else input.value = v;
      const lab = document.getElementById("f-" + k + "-v");
      if (lab && f.fmt) lab.innerHTML = f.fmt(v);
      wrap.style.display = f.show && !f.show(plan) ? "none" : "";
    }
  }

  function renderOutputs() {
    const r = R, p = plan, fw = r.fw;
    const bname = fw.body.name.replace(/^The /, "the ").replace(/^A /, "a ");
    const goalText = fw.goal.kind === "tilt" ? "turn " + bname + "'s axis " + sci(p.fwTilt) + "°" : fw.goal.kind === "day" ? "change " + bname + "'s day by " + sci(p.fwDay) + " s" : "shift " + sci(p.fwFraction * 100) + "% of " + bname + "'s spin";
    const what = fw.geom === "ring" ? "trains" : "wheels";
    const trainLines = fw.ring ? [
      `<p><b>Trains:</b> ${grp(fw.trains)} in all, ${grp(p.trainsPerRing)} on each ring, ${sci(fw.perTrain)} kg each, at ${grp(fw.v)} m/s; or ${sci(fw.perMetre)} kg on every metre of the ${grp(fw.ringLen / 1000)} km of each ring: as solid steel, a ring ${sci(Math.sqrt(fw.perMetre / 7850))} m square in cross-section</p>`,
      fw.weight >= 0 ? `<p><b>On the track:</b> at this speed a train weighs ${Math.round(fw.weight * 100)}% of its weight at rest: the faster it goes, the more it floats, and at ${grp(fw.vOrb)} m/s (orbital speed at the surface) it weighs nothing.</p>`
        : `<p class="v-big">▲ Faster than orbital speed at the surface (${grp(fw.vOrb)} m/s): the trains push outward with ${-fw.weight < 2 ? Math.round(-fw.weight * 100) + "% of" : sci(-fw.weight) + " times"} their weight, so the track must hold them in, as a hoop or under the rock above.</p>`,
      `<p class="note">The fastest train yet ran at ${grp(K.maglevRecord * 3.6)} km/h (${grp(K.maglevRecord)} m/s). At that speed the same goal needs ${sci(fw.dL / (fw.r * K.maglevRecord))} kg.</p>`
    ] : [
      `<p class="note">How the mass is split does not change the total: ${what} held at one radius and rim speed need the same mass whether there are 26 or 26,000 of them. Only the radius and the speed reduce it, and nothing on a body has a larger radius than a ring round the whole of it. That is why the trains are the flywheel.</p>`
    ];
    $("out-fw").innerHTML = [
      `<p><b>Momentum the steering must hold:</b> ${sci(fw.dL)} kg m²/s, to ${goalText} (its whole spin is ${sci(fw.L)}).</p>`,
      `<p><b>Mass, all together:</b> ${sci(fw.totalMass)} kg <span class="cmp">${massCmp(fw.totalMass)}${fw.isEarth ? "" : "; " + sci(fw.totalMass / fw.body.M, 2) + " of " + bname.replace(/^a /, "the ") + "'s own mass"}</span></p>`,
      fw.ring ? "" : `<p><b>Each of the ${grp(fw.n)} wheels:</b> ${sci(fw.perWheel)} kg, ${sci(fw.r)} m in radius, rim at ${grp(fw.v)} m/s</p>`,
      ...trainLines,
      `<p><b>Energy stored:</b> ${sci(fw.energy)} J <span class="cmp">${energyCmp(fw.energy)}</span></p>`,
      `<p><b>Time to spin up</b> at ${sci(p.fwPowerTW)} TW: ${years(fw.spinUpYears)}</p>`,
      `<p class="note"><b>The published proof, redone:</b> a 0.1% shift of the Earth's spin is ${sci(r.proof.dL)} kg m²/s; a millionth of it, at 1 rpm on a 1 km radius, needs ${sci(r.proof.massPerNudge)} kg, as published. The million nudges add up, though: to hold the whole shift the wheels together need ${sci(r.proof.massAll)} kg, ${sci(r.proof.massAll / K.Mearth, 2)} Earth masses. That shift tilts the axis by ${r.proof.tiltDeg.toFixed(3)}°.</p>`,
      `<p class="note"><b>Steering, not the engine:</b> forces inside a body cannot move its centre of mass, so the trains turn the planet and set its day but do not move it along its orbit. That needs something outside: see <i>Moving</i>.</p>`,
      fw.goal.kind !== "day" ? `<p class="note"><b>Holding a tilt:</b> the planet turns under the held direction once a day, so the rings hand momentum to one another all day long (the globe shows each ring's share now). That is why every station must know its exact alignment (the neutrino positioning of <i>Global Positioning &amp; Alignment Systems</i>).</p>` : ""
    ].join("");

    const gi = r.gridInfo, nk = (k) => gi.nodes.filter(n => n.kind === k && n.land).length;
    const ringLen = 9 * 2 * Math.PI * K.Rearth / 1000;
    $("out-grid").innerHTML = [
      landRings ? `<p><b>Stations on land:</b> ${gi.landNodes} of 26 (quad ${nk("quad")}/6, dual ${nk("dual")}/12, tri ${nk("tri")}/8)</p>` : "<p>Loading the land outlines…</p>",
      `<p><b>Rings:</b> 9 great circles, ${grp(ringLen)} km of tunnel and mag-lev line in all; ${landRings ? Math.round(gi.ringLandShare * 100) + "% of it under land" : "…"}</p>`,
      `<p><b>Launch accelerators:</b> ${26 * K.docOLAperNode}, six round each station</p>`,
      `<p class="note">The plan leaves the stations at sea until the oceans freeze on the way out. The nine rings can hold momentum in any direction, as a satellite's set of reaction wheels can (the checks confirm the set is even in every direction).</p>`
    ].join("");

    const methodOut = p.method === "flyby" ? [
      `<p><b>Each pass:</b> bends the asteroid ${r.fb.deflectDeg.toFixed(0)}° and gives the Earth ${sci(r.fb.dvEarth * 1000)} mm/s, ${sci(r.fb.dE)} J</p>`,
      p.migrate ? `<p><b>Migration:</b> ${sci(r.fbMigN)} passes, one every ${years(r.fbMigEvery)}</p>` : "",
      `<p><b>Escape:</b> ${sci(r.fbEscN)} more passes over ${years(p.escDur)}, one every ${years(r.fbEscEvery)}</p>`,
      r.fbAsteroids > 1 ? `<p class="v-big">▲ One asteroid takes about ${r.shuttleYr.toFixed(1)} years to swing out to Jupiter and back, so this needs about ${sci(r.fbAsteroids)} of them working in turn (or a longer push, or bigger asteroids).</p>` : "",
      `<p><b>Jupiter pays:</b> its orbit shrinks from ${K.aJupiter} AU to ${r.jupA.toFixed(3)} AU</p>`,
      `<p class="note">After Korycansky, Laughlin &amp; Adams (2001): the asteroid swings by Jupiter between passes, taking energy from it and handing it to the Earth.</p>`
    ] : p.method === "sun" ? [
      `<p><b>Each orbit, shifting ${sci(p.sunF * 100)}% of the Earth's mass across its radius gains at most</b> ${sci(r.sunN.perOrbit)} J</p>`,
      `<p class="v-no">✗ Staying habitable and escaping need ${sci(r.migE + r.escE)} J: ${years(r.sunN.years)} of nudging, against ${years(r.tMS)} left of the Sun's steady life.</p>`,
      `<p class="note">Why so little: the Sun's pull differs across the Earth by only (Earth's radius / its distance)² ≈ 2×10<sup>-9</sup> of itself, and the orbit is nearly a circle (closest 0.983 AU, furthest 1.017 AU), so there is little rhythm to pump. And the Sun does not pay: it is the fixed centre, so the energy comes from the machinery doing the shifting. Even spending all the Earth's spin (${sci(r.spinE)} J) gives 1/${grp((r.migE + r.escE) / r.spinE)} of what is needed. Energy can be taken from a body that moves, which is why the gravity assists take it from Jupiter.</p>`
    ] : p.method === "driver" ? [
      `<p><b>Mass leaves the Earth at</b> ${kms(r.mdEsc.vex)} once clear of its gravity</p>`,
      p.migrate ? `<p><b>Migration throws away</b> ${sci(r.mdMig.massOut)} kg (${sci(r.mdMig.fraction * 100, 2)}% of the Earth)</p>` : "",
      `<p><b>Escape throws away</b> ${sci(r.mdEsc.massOut)} kg (${sci(r.mdEsc.fraction * 100, 2)}% of the Earth) <span class="cmp">${massCmp(r.mdEsc.massOut)}</span></p>`,
      `<p><b>Launch energy:</b> ${sci(r.mdMig.energy + r.mdEsc.energy)} J <span class="cmp">${energyCmp(r.mdMig.energy + r.mdEsc.energy)}</span></p>`,
      `<p><b>Launches:</b> ${sci(r.mdLaunches)} of ${sci(p.olaPayload)} kg; during the escape, ${sci(r.mdRate)} a second from each of the 156 accelerators</p>`
    ] : [
      `<p class="v-no">✗ <b>Change of orbit from the flywheels:</b> none. Wheels inside the Earth cannot change its momentum round the Sun, however many there are.</p>`
    ];
    $("out-sun").innerHTML = [
      `<p><b>Without moving:</b> sunlight at 1 AU passes ${p.hzS.toFixed(3)}× today's in ${years(r.tHZ)}.</p>`,
      p.migrate ? `<p><b>Moving out:</b> to ${r.aDep.toFixed(3)} AU by departure, ${sci(r.migE)} J <span class="cmp">${energyCmp(r.migE)}</span></p>` : "",
      r.lateDeparture ? `<p class="v-no">✗ Departure comes after the Earth has left the habitable zone: move it outward, or leave sooner.</p>` : "",
      r.pastMS ? `<p class="v-no">✗ Departure is after the Sun leaves the main sequence (in ${years(r.tMS)}); the brightness model no longer holds.</p>` : "",
      `<p><b>Escape:</b> ${kms(r.escDv)} ${r.impulsive ? "in one quick push" : "as a slow spiral"} (one quick push would need ${kms(r.escDvImp)}), ${sci(r.escE)} J <span class="cmp">${energyCmp(r.escE)}; as power over the push, ${sci(r.escPower)} W</span></p>`,
      ...methodOut,
      `<p><b>On the way out:</b> heliopause in ${years(r.tHelio)}, 50,000 AU in ${years(r.t50k)}, clear of the Oort cloud in ${years(r.tOort)}</p>`,
      `<p class="note">The published 7,961 years to 50,000 AU holds if the Earth leaves at ${kms(r.vinfForDoc)}, close to its orbital speed today.</p>`,
      `<p><b>Oort cloud:</b> expected collisions crossing it ${sci(r.oort.lambda, 2)}, against ${sci(r.oort.background, 2)} ordinary 1 km impacts over the same ${years(r.oort.years)}. The cloud is so sparse (one comet per ${sci(1 / r.oort.density / K.AU ** 3, 2)} cubic AU) that crossing it adds almost nothing to the risk.</p>`,
      `<p class="note">Far from the Sun the surface, warmed only by the Earth's own ${sci(K.geoHeat / 1e12, 2)} TW of internal heat, settles near ${Math.round(r.tSurf)} K (${Math.round(r.tSurf - 273.15)} °C). Life must move underground, as the plan intends.</p>`
    ].join("");

    const g = r.gal;
    const tr = r.targetObj;
    $("out-gal").innerHTML = p.galOn ? [
      `<p><b>Drop towards the centre:</b> cancel ${kms(g.dvDrop * 1000)} of the Sun's orbital speed, ${sci(r.dropE)} J <span class="cmp">${energyCmp(r.dropE)}</span></p>`,
      p.galDrop === "driver" ? `<p><b>…by throwing away</b> ${sci(r.mdDrop.fraction * 100, 3)}% of the Earth at ${kms(p.galVex * 1000)}</p>`
        : `<p class="v-big">? <b>The open problem:</b> no known way to make this drop without an engine. Gravity assists need something moving to take energy from, and a star passes within 1 AU of the Earth about once in ${years(1 / r.starRate)}. Throwing mass off instead would take ${sci(r.mdDropRef.fraction * 100, 2)}% of the Earth even at a tenth of light speed. Leaving the Sun is possible without it; this leg is where the plan needs a new idea, or the Milky Way's meeting with Andromeda.</p>`,
      `<p><b>Fall:</b> ${years(r.tGC - r.tDrop)}, reaching ${kms(g.vp * 1000)} (${(g.vpOverC * 100).toFixed(1)}% of light) at ${sci(p.rpAU)} AU from Sgr A*</p>`,
      r.tooClose ? `<p class="v-no">✗ Too close: Sgr A*'s tides tear the Earth apart inside ${g.rTidalAU.toFixed(2)} AU.</p>` : "",
      r.amongS ? `<p class="v-big">▲ Among the S-stars: S2 itself comes within ${K.s2Peri} AU of Sgr A*.</p>` : "",
      g.vpOverC > 0.1 ? `<p class="note">At over a tenth of light speed relativity begins to matter; these figures are Newtonian.</p>` : "",
      `<p><b>Push of ${sci(p.burnKms)} km/s at closest approach:</b> leaves the Galaxy at ${g.vinf > 0 ? kms(g.vinf * 1000) : "nothing: it stays bound, climbing back out to " + sci(g.apoAfter / 1000) + " kpc"} <span class="cmp">(the Oberth effect: the same push far from anything gives just ${sci(p.burnKms)} km/s)</span></p>`,
      p.galDrop === "driver" ? `<p><b>…costing</b> ${sci(r.mdBurn.fraction * 100, 3)}% of the Earth at ${kms(p.galVex * 1000)}</p>` : `<p class="note">The push at closest approach needs an engine too: the same open problem, though far smaller than the drop.</p>`,
      `<p class="note">Without the slingshot the Earth would fall back out to where it began. Leaving the Galaxy directly from the Sun's orbit would take ${kms(g.directDv * 1000)}, ${sci(r.directMd.fraction * 100, 3)}% of the Earth at the same launch speed. Nature does it already: S5-HVS1 was flung from Sgr A* at ${grp(K.hvsPrecedent)} km/s.</p>`,
      `<p><b>${tr.name}:</b> ${sci(r.targetDistPc / 1000)} kpc from the Galactic Centre; arrival ${isFinite(r.tTravel) ? years(r.tTravel) + " after the slingshot" : ": never, on this route"}</p>`,
      tr.name === "Andromeda" ? `<p class="note">Andromeda is already falling towards us, at about 110 km/s. The two may merge within 10 billion years (an even chance, Sawala et al. 2025).</p>` : "",
      `<p><b>Reach, ever, at ${g.vinf > 0 ? kms(g.vinf * 1000) : "0 km/s"}:</b> ${r.reachMpc.toFixed(2)} Mpc, as the universe's expansion slows anything moving through it${r.reach2mrs ? `; ${grp(r.reach2mrs.inside)} of 2MRS's ${grp(r.reach2mrs.all)} galaxies (${(100 * r.reach2mrs.inside / r.reach2mrs.all).toFixed(2)}%)` : ""}. Light sent today reaches ${r.lightReachGly.toFixed(2)} Gly.</p>`
    ].join("") : `<p>The Earth stays with the Sun's orbit round the Galaxy, a wandering planet among the stars.</p>`;
    renderLedger(); renderColonies();
  }

  // ---------------------------------------------------------------------------------------------
  // The ledger
  // ---------------------------------------------------------------------------------------------
  const OK = '<span class="verdict v-ok">✓ known physics</span>', BIG = '<span class="verdict v-big">▲ far beyond anything built</span>', NO = '<span class="verdict v-no">✗ not possible this way</span>';
  const OPEN = '<span class="verdict v-big">? open problem</span>';
  function renderLedger() {
    const r = R, p = plan, rows = [], fw = r.fw;
    const fwV = fw.totalMass > fw.body.M * 0.01 ? NO : fw.totalMass > K.anthroMass * 1000 ? BIG : OK;
    rows.push(["Steering", "Hold the steering (" + fw.body.name + ")", sci(fw.totalMass) + " kg of " + (fw.ring ? "trains" : "wheels") + "; " + sci(fw.energy) + " J", massCmp(fw.totalMass) + "; spin-up " + years(fw.spinUpYears), fwV]);
    rows.push(["Steering", "Move the orbit with the steering", "a change in the Earth's momentum round the Sun", "forces inside a body cannot move its centre of mass", NO]);
    const how = (n, every, frac) => p.method === "flyby" ? sci(n) + " asteroid passes, one every " + years(every) + (r.fbAsteroids > 1 ? " (" + sci(r.fbAsteroids) + " asteroids)" : "")
      : p.method === "driver" ? sci(frac * 100, 2) + "% of the Earth thrown away"
      : p.method === "sun" ? years(r.sunN.years) + " of nudges against the Sun" : "nothing";
    const engV = p.method === "flywheels" || p.method === "sun" ? NO : BIG;
    if (p.migrate) rows.push(["Engine", "Stay habitable until departure", "out to " + r.aDep.toFixed(3) + " AU; " + sci(r.migE) + " J", how(r.fbMigN, r.fbMigEvery, r.mdMig.fraction), engV]);
    rows.push(["Engine", "Escape the Sun", kms(r.escDv) + "; " + sci(r.escE) + " J", how(r.fbEscN, r.fbEscEvery, r.mdEsc.fraction), r.lateDeparture ? NO : engV]);
    rows.push(["Route", "Reach 50,000 AU", years(r.t50k), "published: 7,961 years (needs " + kms(r.vinfForDoc) + ")", OK]);
    rows.push(["Route", "Cross the Oort cloud", sci(r.oort.lambda, 2) + " expected collisions", sci(r.oort.background, 2) + " ordinary 1 km impacts in the same time", OK]);
    rows.push(["Route", "Live without the Sun", "the surface near " + Math.round(r.tSurf) + " K", "heat from inside the Earth: " + sci(K.geoHeat / 1e12, 2) + " TW, " + (K.geoHeat / K.worldPower).toFixed(1) + "× today's world energy use", OK]);
    if (p.galOn) {
      rows.push(["Engine", "Drop to the Galactic Centre", kms(r.gal.dvDrop * 1000) + "; " + sci(r.dropE) + " J", p.galDrop === "driver" ? sci(r.mdDrop.fraction * 100, 3) + "% of the Earth at " + kms(p.galVex * 1000) : "no known way without an engine", p.galDrop === "driver" ? (r.mdDrop.fraction > 0.5 ? NO : BIG) : OPEN]);
      rows.push(["Route", "Slingshot round Sgr A*", sci(p.rpAU) + " AU at " + (r.gal.vpOverC * 100).toFixed(1) + "% of light", r.tooClose ? "inside the tidal radius (" + r.gal.rTidalAU.toFixed(2) + " AU)" : r.gal.vinf > 0 ? "leaves at " + kms(r.gal.vinf * 1000) : "stays bound", r.tooClose ? NO : r.gal.vinf > 0 ? BIG : NO]);
      rows.push(["Route", "Reach " + r.targetObj.name, isFinite(r.tTravel) ? years(r.tTravel) : "never", "reach ever: " + r.reachMpc.toFixed(2) + " Mpc", isFinite(r.tTravel) ? (p.galDrop === "driver" ? BIG : OPEN) : NO]);
    }
    $("ledger").innerHTML = "<tr><th>Role</th><th>Step</th><th>Needs</th><th>Compared with</th><th>Verdict</th></tr>" +
      rows.map(rw => "<tr>" + rw.map(c => "<td>" + c + "</td>").join("") + "</tr>").join("");
  }

  // ---------------------------------------------------------------------------------------------
  // Colonies: confirmed planets that might be habitable, nearest first
  // ---------------------------------------------------------------------------------------------
  function renderColonies() {
    const ex = OLAP.data.loaded.exoplanets;
    if (!ex) return;
    const ok = ex.filter(q => q.pl_insol != null && q.pl_insol >= 0.25 && q.pl_insol <= 1.7 &&
      ((q.pl_rade != null && q.pl_rade <= 1.8) || (q.pl_rade == null && q.pl_bmasse != null && q.pl_bmasse <= 5)));
    const ly = K.pc / K.ly, v = plan.colV;
    $("colony-intro").innerHTML = `The plan sends colonies ahead from the launch accelerators. Of the ${grp(ex.length)} confirmed planets with known distances (NASA Exoplanet Archive), ${ok.length} receive between a quarter and 1.7 times the Earth's sunlight and are probably rocky (under 1.8 Earth radii, or under 5 Earth masses where no radius is known). The nearest are below, with the travel time at the colony ships' speed. <span id="colony-fields"></span>`;
    // Move the speed control into place
    const holder = $("colony-fields"); if (holder && ctrl.colV) holder.appendChild(ctrl.colV.wrap);
    const rows = ok.slice(0, 15).map(q => {
      const dly = q.sy_dist * ly;
      return `<tr><td>${q.pl_name}</td><td>${dly.toFixed(1)}</td><td>${q.pl_insol.toFixed(2)}</td><td>${q.pl_rade != null ? q.pl_rade.toFixed(2) : "—"}</td><td>${q.pl_eqt != null ? Math.round(q.pl_eqt) : "—"}</td><td>${years(dly / v)}</td><td>${q.disc_year}</td></tr>`;
    });
    $("colonies").innerHTML = `<tr><th>Planet</th><th>Light years</th><th>Sunlight (Earth = 1)</th><th>Radius (Earths)</th><th>Temperature (K)</th><th>Travel at ${sci(v * 100)}% of light</th><th>Found</th></tr>` + rows.join("");
  }

  // ---------------------------------------------------------------------------------------------
  // Data and constants tables
  // ---------------------------------------------------------------------------------------------
  function renderDatasets() {
    const st = OLAP.data.status, D = OLAP.data.D;
    $("datasets").innerHTML = "<tr><th>Dataset</th><th>Source</th><th>State</th><th>Checked</th></tr>" + Object.keys(D).map(id => {
      const d = D[id], s = st[id] || {}, m = (OLAP.manifest.datasets[id]) || {};
      const state = s.state === "ready" ? grp(s.rows) + " rows, via " + s.via : s.state === "loading" ? "loading…" : s.state === "error" ? "could not load: " + s.message : (d.kind === "live" ? `<button class="btn-small" data-load="${id}">Load from VizieR</button>` : "not loaded");
      const chk = s.state === "ready" ? (s.verified ? "✓ SHA-256 matches the record of " + m.retrieved : "✗ differs from the record of " + m.retrieved + " (" + grp(s.rows) + " rows against " + grp(m.rows) + "): the archive has changed; run update_data.py") : "recorded " + (m.retrieved || "—") + ", " + grp(m.rows || 0) + " rows";
      return `<tr><td>${d.title}</td><td>${d.cite}<br><span class="cmp">${d.terms}</span></td><td>${state}</td><td>${chk}</td></tr>`;
    }).join("");
    document.querySelectorAll("[data-load]").forEach(b => b.onclick = () => loadData(b.dataset.load));
  }
  function renderConstants() {
    const rows = Object.keys(C).map(k => {
      const v = C[k].v; let s;
      if (Array.isArray(v)) s = v.map(x => Array.isArray(x) ? x.join(" ") : (x && x.name) || x).join(", ");
      else if (typeof v === "object") s = Object.entries(v).map(([a, b]) => a + " " + b).join(", ");
      else s = typeof v === "number" ? sci(v, 6) : v;
      return `<tr><td>${k}</td><td>${s} ${C[k].u || ""}</td><td>${C[k].src}</td></tr>`;
    });
    $("constants").innerHTML = "<tr><th>Name</th><th>Value</th><th>Source</th></tr>" + rows.join("");
  }

  // ---------------------------------------------------------------------------------------------
  // Charts (canvas; the x axis is the same logarithmic time as the slider)
  // ---------------------------------------------------------------------------------------------
  const XMAX = 10.5;
  const tOfX = (x) => Math.pow(10, x) - 1, xOfT = (t) => Math.log10(t + 1);
  function setupCanvas(cv) {
    const dpr = window.devicePixelRatio || 1, w = cv.clientWidth, h = cv.clientHeight;
    if (cv.width !== Math.round(w * dpr) || cv.height !== Math.round(h * dpr)) { cv.width = Math.round(w * dpr); cv.height = Math.round(h * dpr); }
    const ctx = cv.getContext("2d"); ctx.setTransform(dpr, 0, 0, dpr, 0, 0); ctx.clearRect(0, 0, w, h);
    return { ctx, w, h };
  }
  function axisX(ctx, w, h, m) {
    ctx.font = "11px 'Courier New', monospace"; ctx.fillStyle = "#666"; ctx.textAlign = "center"; ctx.textBaseline = "top";
    const span = w - m.l - m.r;
    for (const [x, lab] of [[0, "now"], [3, "1,000 yr"], [6, "1 Myr"], [9, "1 Gyr"], [10, "10 Gyr"]]) {
      const px = m.l + span * x / XMAX;
      const show = span > 600 || (span > 330 ? x !== 10 : x === 0 || x === 6 || x === 9);
      if (show) ctx.fillText(lab, px, h - m.b + 6);
      ctx.fillStyle = "#1a1a24"; ctx.fillRect(px, m.t, 1, h - m.t - m.b); ctx.fillStyle = "#666";
    }
  }
  let sunHover = null, fwHover = null;
  function drawSunChart() {
    const { ctx, w, h } = setupCanvas($("chart-sun"));
    const m = { l: 44, r: w < 600 ? 120 : 150, t: 12, b: 22 }, W = w - m.l - m.r, H = h - m.t - m.b, ymax = 2.4;
    const X = (x) => m.l + W * x / XMAX, Y = (s) => m.t + H * (1 - Math.min(s, ymax) / ymax);
    axisX(ctx, w, h, m);
    ctx.fillStyle = "#666"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
    for (const s of [0, 1, 2]) { ctx.fillText(s + "×", m.l - 6, Y(s)); ctx.fillStyle = "#1a1a24"; ctx.fillRect(m.l, Y(s), W, 1); ctx.fillStyle = "#666"; }
    // Limit line
    ctx.setLineDash([5, 4]); ctx.strokeStyle = "#d0507a"; ctx.lineWidth = 1.5; ctx.beginPath(); ctx.moveTo(m.l, Y(plan.hzS)); ctx.lineTo(m.l + W, Y(plan.hzS)); ctx.stroke(); ctx.setLineDash([]);
    ctx.fillStyle = "#b0aca5"; ctx.textAlign = "left"; ctx.fillText(w < 600 ? "too hot" : "runaway greenhouse", m.l + W + 8, Y(plan.hzS));
    const series = [
      { col: "#c07a10", label: "staying at 1 AU", f: (t) => t <= R.tMS ? P.lum(t) : null },
      { col: "#3d8fd6", label: "with the plan", f: (t) => { const e = earthState(t); return e.au != null ? P.lum(Math.min(t, R.tMS)) / (e.au * e.au) : 0; } }
    ];
    for (const s of series) {
      ctx.strokeStyle = s.col; ctx.lineWidth = 2; ctx.beginPath(); let started = false, last = null;
      for (let i = 0; i <= 600; i++) {
        const x = XMAX * i / 600, v = s.f(tOfX(x));
        if (v == null) break;
        if (!started) { ctx.moveTo(X(x), Y(v)); started = true; } else ctx.lineTo(X(x), Y(v));
        last = [X(x), Y(v)];
      }
      ctx.stroke();
      s.end = last;
    }
    ctx.textAlign = "left";
    series.forEach((s, i) => { ctx.fillStyle = "#b0aca5"; ctx.fillRect(m.l + W + 8, m.t + 6 + i * 16, 10, 2); ctx.fillStyle = s.col; ctx.fillRect(m.l + W + 8, m.t + 5 + i * 16, 10, 4); ctx.fillStyle = "#b0aca5"; ctx.fillText(s.label, m.l + W + 24, m.t + 7 + i * 16); });
    // Events
    for (const e of R.events) { const px = X(xOfT(e.t)); ctx.fillStyle = "#888"; ctx.fillRect(px - 0.5, m.t + H - 6, 1, 6); }
    // Now
    const nx = X(xOfT(simT));
    ctx.fillStyle = "#e0ddd8"; ctx.fillRect(nx - 0.5, m.t, 1, H);
    // Hover
    if (sunHover != null) {
      const x = Math.max(0, Math.min(XMAX, (sunHover - m.l) / W * XMAX)), t = tOfX(x);
      ctx.fillStyle = "rgba(224,221,216,0.4)"; ctx.fillRect(X(x) - 0.5, m.t, 1, H);
      const near = R.events.filter(e => Math.abs(xOfT(e.t) - x) < 0.08).map(e => e.label);
      const lines = [years(t) + " from now", "at 1 AU: " + (t <= R.tMS ? P.lum(t).toFixed(3) + "×" : "—"), "with the plan: " + series[1].f(t).toFixed(3) + "×", ...near];
      const bw = Math.max(...lines.map(s => ctx.measureText(s).width)) + 16, bx = Math.min(X(x) + 10, w - bw - 4);
      ctx.fillStyle = "rgba(13,13,20,0.95)"; ctx.fillRect(bx, m.t, bw, 16 * lines.length + 8); ctx.strokeStyle = "#2a2a3a"; ctx.strokeRect(bx, m.t, bw, 16 * lines.length + 8);
      ctx.fillStyle = "#e0ddd8"; lines.forEach((s, i) => ctx.fillText(s, bx + 8, m.t + 12 + 16 * i));
    }
    $("chart-sun-note").textContent = "Sunlight on the Earth, as a multiple of today's, against time on the same scale as the slider. Ticks along the bottom mark the plan's events; hover for them.";
  }
  function drawFwChart() {
    const cv = $("chart-fw"); if (!cv) return;
    const { ctx, w, h } = setupCanvas(cv);
    const m = { l: 44, r: 12, t: 22, b: 22 }, W = w - m.l - m.r, H = h - m.t - m.b;
    const X = (x) => m.l + W * x / XMAX, Y = (s) => m.t + H * (1 - s);
    axisX(ctx, w, h, m);
    ctx.fillStyle = "#666"; ctx.textAlign = "right"; ctx.textBaseline = "middle";
    for (const s of [0, 0.5, 1]) { ctx.fillText(Math.round(s * 100) + "%", m.l - 6, Y(s)); ctx.fillStyle = "#1a1a24"; ctx.fillRect(m.l, Y(s), W, 1); ctx.fillStyle = "#666"; }
    ctx.textAlign = "left"; ctx.fillStyle = "#b0aca5"; ctx.fillText("share of the goal held, while spinning up", m.l, 10);
    const T = R.fw.spinUpYears, f = (t) => Math.min(1, Math.sqrt(t / T));
    ctx.strokeStyle = "#3d8fd6"; ctx.lineWidth = 2; ctx.beginPath();
    for (let i = 0; i <= 400; i++) { const x = XMAX * i / 400, v = f(tOfX(x)); i ? ctx.lineTo(X(x), Y(v)) : ctx.moveTo(X(x), Y(v)); }
    ctx.stroke();
    const nx = X(xOfT(simT)); ctx.fillStyle = "#e0ddd8"; ctx.fillRect(nx - 0.5, m.t, 1, H);
    if (fwHover != null) {
      const x = Math.max(0, Math.min(XMAX, (fwHover - m.l) / W * XMAX)), t = tOfX(x), s = years(t) + ": " + (f(t) * 100).toFixed(1) + "%";
      const bw = ctx.measureText(s).width + 16, bx = Math.min(X(x) + 10, w - bw - 4);
      ctx.fillStyle = "rgba(224,221,216,0.4)"; ctx.fillRect(X(x) - 0.5, m.t, 1, H);
      ctx.fillStyle = "rgba(13,13,20,0.95)"; ctx.fillRect(bx, m.t, bw, 22); ctx.fillStyle = "#e0ddd8"; ctx.fillText(s, bx + 8, m.t + 11);
    }
  }

  // ---------------------------------------------------------------------------------------------
  // The simulation: time, views, playing
  // ---------------------------------------------------------------------------------------------
  let simT = 0, playing = false, view = "planet", lastFrame = 0, resetState = 0;
  const slider = $("time-slider"), canvas = $("view");
  function setTime(t, fromSlider) {
    simT = Math.max(0, t);
    if (!fromSlider) slider.value = Math.round(xOfT(simT) * 100);
    const e = earthState(simT);
    $("time-readout").textContent = (simT < 1 ? "Today" : "Year +" + years(simT).replace(" years", "")) + " · " + e.phase;
  }
  function viewFor(t) {
    const e = earthState(t);
    if (t < 1e3) return "planet";
    if (e.au != null && (t < R.tDrop || !R.galOn)) return "sun";
    if (R.galOn && t > R.tGC && e.distPc > 3e5) return "universe";
    return "galaxy";
  }
  function setView(v) {
    view = v;
    document.querySelectorAll(".tab").forEach(b => b.setAttribute("aria-selected", b.dataset.view === v ? "true" : "false"));
    $("hint").textContent = V[v].hint;
    if (v === "universe") loadData("2mrs");
    if (v === "galaxy") V.galaxy.fit({ R }, earthState(simT).gal);
  }
  function hudText(e) {
    const r = R;
    if (view === "planet") {
      const held = Math.min(1, Math.sqrt(simT / r.fw.spinUpYears));
      const big = Math.max(...r.ringSharesNow.map(Math.abs));
      return `<b>${r.fw.isEarth ? "Earth &amp; Grid" : "Steering " + r.fw.body.name}</b><br>${r.fw.isEarth ? r.gridInfo.landNodes + " of 26 stations on land · " + Math.round(r.gridInfo.ringLandShare * 100) + "% of the rings under land" : "the same grid of 9 rings and 26 stations, scaled to " + r.fw.body.name.replace(/^The /, "the ").replace(/^A /, "a ")}<br>Steering: ${sci(r.fw.totalMass)} kg in ${r.fw.ring ? grp(r.fw.trains) + " trains on 9 rings" : grp(r.fw.n) + " wheels"} · ${(held * 100).toFixed(1)}% of the goal held by now<br>Busiest ring now: ${sci(big)} kg m²/s`;
    }
    if (view === "sun") {
      const au = e.au != null ? e.au : null;
      const S = au != null ? P.lum(Math.min(simT, r.tMS)) / (au * au) : 0;
      return `<b>Solar System</b><br>${e.phase}<br>${au != null ? "Distance from the Sun: " + (au < 100 ? au.toFixed(3) : grp(au)) + " AU · sunlight " + (S < 0.01 ? sci(S, 2) : S.toFixed(3)) + "× today's" : "The Earth has left the Sun behind"}<br>The Sun: ${P.lumValid(simT) ? P.lum(simT).toFixed(3) + "× today's brightness" : "past the main sequence"}`;
    }
    if (view === "galaxy") {
      const gp = e.gal ? Math.hypot(e.gal[0], e.gal[1]) : K.R0;
      return `<b>Milky Way</b><br>${e.phase}<br>From the Galactic Centre: ${sci(gp / 1000)} kpc<br>${r.galOn ? "Route: drop " + kms(r.gal.dvDrop * 1000) + " · fall " + years(r.tGC - r.tDrop) + " · leave at " + (r.gal.vinf > 0 ? kms(r.gal.vinf * 1000) : "— (bound)") : "No galactic route"}`;
    }
    const st = OLAP.data.status["2mrs"] || {};
    return `<b>Local Universe</b><br>${st.state === "ready" ? grp(st.rows) + " 2MRS galaxies (" + (st.verified ? "✓ verified" : "✗ differs from the record") + "), " + grp(OLAP.data.loaded["2mrs"].length) + " with redshifts" : st.state === "error" ? "2MRS could not be read: " + st.message : "Reading 2MRS from VizieR…"}<br>Within the Earth's reach, ever: ${r.reach2mrs ? grp(r.reach2mrs.inside) + " (" + (100 * r.reach2mrs.inside / r.reach2mrs.all).toFixed(2) + "%)" : "…"} · H0 ${plan.H0}<br>${e.phase}`;
  }
  function frame(now) {
    const dt = lastFrame ? Math.min(0.1, (now - lastFrame) / 1000) : 0; lastFrame = now;
    if (playing) {
      const x = Math.min(XMAX, xOfT(simT) + dt * 0.35);
      setTime(tOfX(x));
      if ($("follow").checked) {
        const v = viewFor(simT); if (v !== view) setView(v);
        if (view === "galaxy") V.galaxy.follow(earthState(simT).gal);
      }
      if (x >= XMAX) togglePlay(false);
      drawSunChart(); drawFwChart();
    }
    const dayPhase = (now / 1000) * 2 * Math.PI / 24;   // one day every 24 seconds
    R.ringSharesNow = P.ringShares(R.gridR, R.H, dayPhase);
    const e = earthState(simT);
    const S = { plan, R, t: simT, data: OLAP.data.loaded, dayPhase, spin: true, earthAU: e.au, earthGal: e.gal, sunGal: R.sunAt(simT), data2mrsMsg: (OLAP.data.status["2mrs"] || {}).message };
    const dpr = window.devicePixelRatio || 1, w = canvas.clientWidth, h = canvas.clientHeight;
    if (canvas.width !== Math.round(w * dpr) || canvas.height !== Math.round(h * dpr)) { canvas.width = Math.round(w * dpr); canvas.height = Math.round(h * dpr); }
    const ctx = canvas.getContext("2d"); ctx.setTransform(dpr, 0, 0, dpr, 0, 0); ctx.clearRect(0, 0, w, h);
    V[view].draw(ctx, w, h, S);
    $("hud").innerHTML = hudText(e);
    requestAnimationFrame(frame);
  }
  function togglePlay(on) {
    playing = on == null ? !playing : on;
    $("play-btn").textContent = playing ? "Pause" : (simT > 0 ? "Resume" : "Engage");
  }

  // Pointer: drag and scroll on the view
  let drag = null;
  canvas.addEventListener("pointerdown", (e) => { drag = { x: e.clientX, y: e.clientY }; canvas.setPointerCapture(e.pointerId); });
  canvas.addEventListener("pointermove", (e) => { if (!drag) return; V[view].drag(e.clientX - drag.x, e.clientY - drag.y); drag = { x: e.clientX, y: e.clientY }; });
  canvas.addEventListener("pointerup", () => { drag = null; });
  canvas.addEventListener("wheel", (e) => { e.preventDefault(); V[view].wheel(e.deltaY); }, { passive: false });

  // ---------------------------------------------------------------------------------------------
  // Data loading
  // ---------------------------------------------------------------------------------------------
  function loadData(id) {
    return OLAP.data.load(id, () => renderDatasets()).then(() => {
      if (id === "localgroup") { fillOptions(ctrl.target.f, ctrl.target.input); syncControls(); }
      if (id === "land") { landRings = P.prepareLand(OLAP.data.loaded.land); V.planet.buildMask(OLAP.data.loaded.land); }
      compute(); renderOutputs(); renderDatasets();
    }).catch(() => renderDatasets());
  }

  function optimiseGrid() {
    if (!landRings) return;
    $("opt-status").textContent = " searching…";
    setTimeout(() => {
      const b = P.optimiseGrid(landRings, { tries: plan.poleQuad ? 900 : 4000, seed: 2025, poleQuad: plan.poleQuad });
      plan.gLat = Math.round(b.lat * 2) / 2; plan.gLon = Math.round(b.lon * 2) / 2; plan.gRoll = Math.round(b.roll * 2) / 2;
      $("opt-status").textContent = " best found: " + b.landNodes + " stations on land (a seeded search, so the same every time)";
      V.planet.cam.lat = plan.gLat; V.planet.cam.lon = plan.gLon;
      changed();
    }, 30);
  }

  // ---------------------------------------------------------------------------------------------
  // The plan in the address, so a link keeps its settings
  // ---------------------------------------------------------------------------------------------
  function writeHash() {
    const q = new URLSearchParams();
    for (const k in plan) if (plan[k] !== DEF[k]) q.set(k, String(plan[k]));
    const s = q.toString();
    history.replaceState(null, "", s ? "#" + s : location.pathname + location.search);
  }
  function readHash() {
    const q = new URLSearchParams(location.hash.slice(1));
    for (const [k, v] of q) {
      if (!(k in DEF)) continue;
      const d = DEF[k];
      plan[k] = typeof d === "boolean" ? v === "true" : typeof d === "number" ? Number(v) : v;
    }
  }
  let hashTimer = null;
  function changed() {
    compute(); syncControls(); renderOutputs(); drawSunChart(); drawFwChart();
    clearTimeout(hashTimer); hashTimer = setTimeout(writeHash, 300);
  }

  // ---------------------------------------------------------------------------------------------
  // The cosmic horizon counter (unchanged: the Build or Buy page quotes its constants)
  // ---------------------------------------------------------------------------------------------
  const currentYear = new Date().getFullYear();
  const EPOCH = Date.UTC(currentYear, 0, 1);
  const REACH_GLY = K.REACH_GLY, SEEN_GLY = K.SEEN_GLY, SECONDS_PER_YEAR = 31557600, STARS_LOST_PER_SECOND = K.STARS_LOST_PER_SECOND;
  function updateCosmicMetrics() {
    const now = Date.now();
    const yearsElapsed = (now - EPOCH) / (SECONDS_PER_YEAR * 1000);
    const currentReach = REACH_GLY - (yearsElapsed * 1e-9);
    const currentSeen = SEEN_GLY + (yearsElapsed * 1e-9);
    const unreachablePct = 100 * (1 - (currentReach / currentSeen) ** 3);
    const starsLostTotal = Math.floor((now - EPOCH) / 1000 * STARS_LOST_PER_SECOND).toLocaleString();
    $("limitation-counter").innerHTML = `${unreachablePct.toFixed(1)}% Unreachable<br>${(100 - unreachablePct).toFixed(1)}% Still Reachable`;
    $("star-loss-ticker").innerText = `~${starsLostTotal} stars crossed permanently since the start of ${currentYear}.`;
  }

  // ---------------------------------------------------------------------------------------------
  // Start
  // ---------------------------------------------------------------------------------------------
  readHash();
  buildPanels();
  renderConstants();
  compute();
  syncControls();
  renderOutputs();
  renderDatasets();
  setInterval(updateCosmicMetrics, 1000); updateCosmicMetrics();
  $("horizon-check").textContent = "Worked out here from the Planck 2018 cosmology, light sent today can reach " + P.MpcToGly(P.cosmo().reach(K.c)).toFixed(2) + " billion light years, ever (the counter uses " + K.REACH_GLY + ").";
  loadData("land"); loadData("exoplanets"); loadData("localgroup");
  setView("planet"); setTime(0);

  slider.addEventListener("input", () => { setTime(tOfX(slider.value / 100), true); drawSunChart(); drawFwChart(); });
  $("play-btn").addEventListener("click", () => { if (simT >= tOfX(XMAX) * 0.999) setTime(0); togglePlay(); });
  $("reset-btn").addEventListener("click", () => {
    if (resetState === 0) { resetState = 1; $("reset-btn").textContent = "Sure?"; $("reset-btn").classList.add("warning"); setTimeout(() => { resetState = 0; $("reset-btn").textContent = "Reset"; $("reset-btn").classList.remove("warning"); }, 3000); return; }
    resetState = 0; $("reset-btn").textContent = "Reset"; $("reset-btn").classList.remove("warning");
    togglePlay(false); setTime(0); setView("planet"); drawSunChart(); drawFwChart(); $("play-btn").textContent = "Engage";
  });
  document.querySelectorAll(".tab").forEach(b => b.addEventListener("click", () => { setView(b.dataset.view); if (playing) $("follow").checked = false; }));
  for (const [id, set] of [["chart-sun", (x) => sunHover = x], ["chart-fw", (x) => fwHover = x]]) {
    const cv = $(id);
    cv.addEventListener("pointermove", (e) => { set(e.clientX - cv.getBoundingClientRect().left); drawSunChart(); drawFwChart(); });
    cv.addEventListener("pointerleave", () => { set(null); drawSunChart(); drawFwChart(); });
  }
  $("copy-link").addEventListener("click", () => {
    writeHash();
    (navigator.clipboard ? navigator.clipboard.writeText(location.href) : Promise.reject()).then(() => $("copy-status").textContent = " copied", () => $("copy-status").textContent = " copy this page's address");
  });
  $("defaults").addEventListener("click", () => { plan = Object.assign({}, DEF); changed(); });
  window.addEventListener("resize", () => { drawSunChart(); drawFwChart(); });
  const fsBtn = $("fs-btn"), simContainer = $("sim-container");
  fsBtn.addEventListener("click", () => {
    if (!document.fullscreenElement) simContainer.requestFullscreen().catch(err => console.error(`Fullscreen failed: ${err.message}`));
    else document.exitFullscreen();
  });
  document.addEventListener("fullscreenchange", () => { fsBtn.innerText = document.fullscreenElement ? "[X] Exit Fullscreen" : "[ ] Fullscreen"; });
  drawSunChart(); drawFwChart();
  requestAnimationFrame(frame);
})();
