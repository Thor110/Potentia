// Checks the simulation's physics against values known independently.
//   node Pages/space/tools/check.mjs
// Prints each check and exits non-zero if any fails. Run it after changing constants.js or
// physics.js.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import vm from "node:vm";

const here = dirname(fileURLToPath(import.meta.url));
const ctx = { console };
ctx.globalThis = ctx;
vm.createContext(ctx);
for (const f of ["constants.js", "physics.js"]) vm.runInContext(readFileSync(join(here, "..", f), "utf8"), ctx, { filename: f });
const { K, P } = ctx.OLAP;

let failed = 0;
function check(name, got, want, relTol, absTol) {
  const ok = Math.abs(got - want) <= Math.abs(want) * relTol + (absTol || 0);
  if (!ok) failed++;
  console.log(`${ok ? "ok  " : "FAIL"} ${name}: ${Number(got).toPrecision(5)} (want ${Number(want).toPrecision(5)} ±${relTol * 100}%)`);
}

// The Earth's spin angular momentum, as used in the OLAP Initiative's proof: 5.86e33 kg m^2/s.
check("Earth's spin angular momentum", P.earthL(), 5.86e33, 0.01);
// The proof's own figure: 5.6e19 kg per nudge.
const pr = P.publishedProof();
check("published mass per nudge", pr.massPerNudge, K.docMassPerNudge, 0.01);
// Earth's mean orbital speed, 29.78 km/s; escape from 1 AU, 42.1 km/s.
check("Earth's orbital speed (m/s)", P.vCirc(1), 29780, 0.002);
check("solar escape speed at 1 AU (m/s)", P.vEscSun(1), 42120, 0.002);
// Sunlight 10% brighter (runaway greenhouse near S=1.1) in about a billion years.
check("years until sunlight at 1 AU is 1.1x today's", P.yearsUntilInsolation(1.1, 1), 1.04e9, 0.03);
// The Galaxy model reproduces the circular speed it was scaled to, and an escape speed near the
// measured 528 km/s (Deason et al. 2019, +24/-25).
check("circular speed at R0 (km/s)", P.galVc(K.R0), K.vcSun, 1e-6);
check("escape speed at R0 (km/s)", P.galVesc(K.R0), K.vEscGal, 0.10);
// The cosmic event horizon today: what light sent now can ever reach, the page's 16.58 Gly.
const cos = P.cosmo();
check("event horizon (Gly)", P.MpcToGly(cos.reach(K.c)), K.REACH_GLY, 0.01);
// Comoving distance to z = 0.1 with Planck 2018 (H0 67.4, Om 0.315): about 434 Mpc.
check("comoving distance to z=0.1 (Mpc)", cos.dc(0.1), 434, 0.01);
// Tidal radius of Sgr A* for the Earth, R (M/m)^(1/3): about 0.48 AU.
check("Earth's tidal radius at Sgr A* (AU)", P.galRoute({ rpAU: 100, burnKms: 0 }).rTidalAU, 0.48, 0.05);
// The grid: 26 nodes, 6 + 12 + 8, and the 9 rings span space evenly (their sum n n^T is 3 I).
check("grid nodes", P.gridNodes.length, 26, 0);
const S = [[0, 0, 0], [0, 0, 0], [0, 0, 0]];
for (const n of P.ringNormals) for (let i = 0; i < 3; i++) for (let j = 0; j < 3; j++) S[i][j] += n[i] * n[j];
check("ring set is even in every direction", S[0][0] + S[1][1] + S[2][2], 9, 1e-12);
check("…and has no preferred axis", Math.abs(S[0][1]) + Math.abs(S[0][2]) + Math.abs(S[1][2]), 0, 0, 1e-12);
// A ring's momentum shares reproduce the vector asked for
const R = P.gridRotation(52, -1, 17), H = [1, 2, 3];
const sh = P.ringShares(R, H, 0.7);
const back = [0, 0, 0];
P.ringNormals.forEach((n, i) => { const m = P.mulMV(R, n); for (let k = 0; k < 3; k++) back[k] += sh[i] * m[k]; });
const Hb = P.mulMV(P.rotZ(-0.7), H);
check("ring shares sum back to the vector", Math.hypot(back[0] - Hb[0], back[1] - Hb[1], back[2] - Hb[2]), 0, 0, 1e-12);
// Escape energy from 1 AU with nothing to spare: G M m / 2a, 2.65e33 J.
check("energy to leave the Sun from 1 AU (J)", P.escapeEnergy(1, 0), 2.65e33, 0.01);

// The fall to the Galactic Centre: the table (3,000 steps) against a fine reference (200,000)
const g = P.galRoute({ rpAU: 300, burnKms: 30 });
const fine = P.orbitLeg(0.5 * g.vt * g.vt + P.galPhi(K.R0), K.R0 * g.vt, 300 * K.AU / K.pc, K.R0, 200000);
check("fall time to Sgr A* (years), table against fine reference", g.fallYears, fine[fine.length - 1][0], 0.002);
// A bound climb-out finds its true farthest point (it once returned the search's edge, 10^7 pc)
const gb = P.galRoute({ rpAU: 300, burnKms: 25 });
const fApo = 2 * (gb.Eafter - P.galPhi(gb.apoAfter)) - (gb.hAfter / gb.apoAfter) ** 2;
check("bound after a small push: the farthest point is a true turning point", isFinite(gb.apoAfter) && Math.abs(fApo) < 1e-6 * Math.abs(gb.Eafter) ? 1 : 0, 1, 0);
// The Oort cloud at the slowest exit (vinf = 0): the Earth still moves at sqrt(2 mu / r), so the
// crossing takes about 2.4 million years and the expected collisions stay small
const oz = P.oortHazard({ N: 1e12, rInAU: 2000, rOutAU: 1e5, vinf: 0 });
check("Oort crossing time at vinf = 0 (years)", oz.years, 2.37e6, 0.03);
check("Oort collisions at vinf = 0 stay below 10^-3", oz.lambda < 1e-3 ? 1 : 0, 1, 0);
// Binary-search look-ups agree with the tables they read
const tb = P.cruiseTable(1, 29600, 1e5);
check("look-up: distance at the time a distance is reached", P.radiusAtTime(tb, P.timeToRadius(tb, 5e4)), 5e4, 1e-9);

console.log(failed ? `\n${failed} check(s) failed` : "\nall checks passed");
process.exit(failed ? 1 : 0);
