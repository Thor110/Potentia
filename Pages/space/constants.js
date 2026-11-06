// The OLAP simulation's constants, each with where it comes from.
//
// Every number the simulation uses is here, once, so it can be checked and updated in one place.
// To update one: change `v`, change `src` to say where the new value comes from, and run
// `node Pages/space/tools/check.mjs` to see that the checks still pass.
//
// Units are SI unless `u` says otherwise.
(function (root) {
  "use strict";
  const C = {
    // Physics
    G:        { v: 6.67430e-11,     u: "m^3 kg^-1 s^-2", src: "CODATA 2018" },
    c:        { v: 299792458,       u: "m/s",            src: "exact (SI definition)" },
    sigmaSB:  { v: 5.670374419e-8,  u: "W m^-2 K^-4",    src: "CODATA 2018" },
    year:     { v: 31557600,        u: "s",              src: "Julian year (IAU)" },
    AU:       { v: 1.495978707e11,  u: "m",              src: "IAU 2012 Resolution B2 (exact)" },
    pc:       { v: 3.0856775814913673e16, u: "m",        src: "IAU 2015 Resolution B2" },
    ly:       { v: 9.4607304725808e15, u: "m",           src: "Julian light year" },
    Msun:     { v: 1.98847e30,      u: "kg",             src: "IAU 2015 nominal GM / CODATA G" },

    // The Sun
    GMsun:    { v: 1.32712440018e20, u: "m^3 s^-2",      src: "IAU 2015 Resolution B3 (nominal)" },
    Lsun:     { v: 3.828e26,        u: "W",              src: "IAU 2015 Resolution B3 (nominal)" },
    sunAge:   { v: 4.57e9,          u: "yr",             src: "Bonanno, Schlattl & Paternò 2002, A&A 390, 1115" },
    // Gough (1981): L(t)/L_now = 1 / (1 + 0.4 (1 - t/t_now)), t the Sun's age. Main sequence only.
    goughK:   { v: 0.4,             u: "",               src: "Gough 1981, Solar Physics 74, 21" },
    sunMSEndAge: { v: 10.9e9,       u: "yr",             src: "Sackmann, Boothroyd & Kraemer 1993, ApJ 418, 457 (end of core hydrogen burning)" },
    sunRGBTipFromNow: { v: 7.59e9,  u: "yr",             src: "Schröder & Smith 2008, MNRAS 386, 155 (tip of the red giant branch; Earth engulfed)" },
    // Habitable zone for a Sun-like star, in units of today's sunlight at 1 AU (S_eff).
    hzRunaway:   { v: 1.107,        u: "S_earth",        src: "Kopparapu et al. 2014, ApJL 787, L29 (runaway greenhouse, 1 Earth mass)" },
    hzMaxGreen:  { v: 0.356,        u: "S_earth",        src: "Kopparapu et al. 2013, ApJ 765, 131 (maximum greenhouse)" },

    // The Earth
    Mearth:   { v: 5.9722e24,       u: "kg",             src: "IAU 2015 / NASA Earth fact sheet" },
    Rearth:   { v: 6.3710e6,        u: "m",              src: "mean radius, NASA Earth fact sheet" },
    ReqEarth: { v: 6.3781e6,        u: "m",              src: "equatorial radius, IAU 2015 nominal" },
    IearthNorm: { v: 0.3307,        u: "M R_eq^2",       src: "polar moment C/(M R_eq^2), Williams 1994 / IERS" },
    omegaEarth: { v: 7.2921150e-5,  u: "rad/s",          src: "IERS Conventions 2010" },
    obliquity:  { v: 23.4393,       u: "deg",            src: "IAU 2006 (J2000)" },
    vEscEarth:  { v: 11186,         u: "m/s",            src: "NASA Earth fact sheet" },
    aEarth:     { v: 1.00000261,    u: "AU",             src: "Standish 1992 / JPL mean elements" },
    geoHeat:    { v: 47e12,         u: "W",              src: "Davies & Davies 2010, Solid Earth 1, 5" },
    solarConst: { v: 1361,          u: "W/m^2",          src: "Kopp & Lean 2011, GRL 38, L01706" },
    worldPower: { v: 19.7e12,       u: "W",              src: "620 EJ/yr primary energy, Energy Institute Statistical Review 2024" },
    anthroMass: { v: 1.1e15,        u: "kg",             src: "all human-made mass c. 2020, Elhacham et al. 2020, Nature 588, 442" },
    oceanMass:  { v: 1.4e21,        u: "kg",             src: "NOAA / standard value" },
    crustMass:  { v: 2.77e22,       u: "kg",             src: "about 0.47% of Earth's mass (standard value)" },
    threeGorges: { v: 3.9e13,       u: "kg",             src: "the OLAP Initiative, p.13 (39 trillion kg of water)" },

    // Other bodies
    Mmoon:    { v: 7.342e22,        u: "kg",             src: "NASA Moon fact sheet" },
    Mceres:   { v: 9.384e20,        u: "kg",             src: "Park et al. 2016, Nature 537, 515" },
    Mjupiter: { v: 1.89813e27,      u: "kg",             src: "NASA Jupiter fact sheet" },
    aJupiter: { v: 5.2038,          u: "AU",             src: "NASA Jupiter fact sheet" },
    planets:  { v: [
      ["Mercury", 0.3871], ["Venus", 0.7233], ["Earth", 1.0000], ["Mars", 1.5237],
      ["Jupiter", 5.2038], ["Saturn", 9.5826], ["Uranus", 19.1913], ["Neptune", 30.07]
    ], u: "AU", src: "NASA planetary fact sheets (semi-major axes)" },
    heliopause: { v: 121,           u: "AU",             src: "Voyager 1 crossing, Stone et al. 2013, Science 341, 150" },
    oortInner:  { v: 2000,          u: "AU",             src: "inner (Hills) cloud edge, Dones et al. 2004 (order of magnitude)" },
    oortOuter:  { v: 100000,        u: "AU",             src: "outer edge, Oort 1950 / Dones et al. 2004 (order of magnitude)" },
    oortCount:  { v: 1e12,          u: "comets >~1 km",  src: "Dones et al. 2004 (range 1e11 to 1e12)" },
    gliese710:  { v: { years: 1.28e6, missPc: 0.052 }, u: "", src: "Bailer-Jones et al. 2018, A&A 616, A37 (Gaia DR2)" },
    impactKm:   { v: 6e5,           u: "yr between >=1 km impacts", src: "order of magnitude; NASA NEO estimates (0.5 to 1 Myr)" },

    // The Galaxy
    R0:       { v: 8277,            u: "pc",             src: "GRAVITY Collaboration 2022, A&A 657, L12" },
    vcSun:    { v: 229.0,           u: "km/s",           src: "Eilers et al. 2019, ApJ 871, 120" },
    vSunPec:  { v: [11.1, 12.24, 7.25], u: "km/s (U,V,W)", src: "Schönrich, Binney & Dehnen 2010, MNRAS 403, 1829" },
    vEscGal:  { v: 528,             u: "km/s",           src: "Deason et al. 2019, MNRAS 485, 3514 (escape speed at the Sun)" },
    MsgrA:    { v: 4.297e6,         u: "Msun",           src: "GRAVITY Collaboration 2022, A&A 657, L12" },
    s2Peri:   { v: 120,             u: "AU",             src: "S2's closest approach to Sgr A*, GRAVITY Collaboration 2018" },
    hvsPrecedent: { v: 1755,        u: "km/s",           src: "S5-HVS1, flung from Sgr A* 4.8 Myr ago; Koposov et al. 2020, MNRAS 491, 2465" },
    // A simple Milky Way potential (after Bovy 2015, ApJS 216, 29, "MWPotential2014"),
    // with the halo scaled so the circular speed at R0 is vcSun. See physics.js.
    mwDisk:   { v: { M: 6.8e10, a: 3000, b: 280 }, u: "Msun, pc", src: "Miyamoto-Nagai disc, Bovy 2015" },
    mwBulge:  { v: { M: 0.9e10, a: 500 },          u: "Msun, pc", src: "Hernquist bulge (Bovy 2015 mass, Hernquist profile)" },
    mwHalo:   { v: { rs: 16000 },                  u: "pc",       src: "NFW scale radius, Bovy 2015" },
    m31Merge: { v: 0.5,             u: "probability within 10 Gyr", src: "Sawala et al. 2025, Nature Astronomy" },

    // Cosmology (flat ΛCDM)
    H0:       { v: 67.4,            u: "km/s/Mpc",       src: "Planck 2018 VI, A&A 641, A6" },
    H0local:  { v: 73.04,           u: "km/s/Mpc",       src: "Riess et al. 2022 (SH0ES), ApJL 934, L7" },
    OmegaM:   { v: 0.315,           u: "",               src: "Planck 2018 VI" },
    // The cosmic horizon counter (kept exactly as before: other pages quote these).
    REACH_GLY:  { v: 16.58,         u: "Gly",            src: "event horizon today, Planck 2018 cosmology" },
    SEEN_GLY:   { v: 46.18,         u: "Gly",            src: "particle horizon today, Planck 2018 cosmology" },
    STARS_LOST_PER_SECOND: { v: 24500, u: "stars/s",     src: "estimate kept from earlier versions of this page" },

    // The OLAP documents (Edward James Gordon, 2025)
    docNodes:   { v: { quad: 6, dual: 12, tri: 8 }, u: "", src: "the OLAP Initiative, p.3" },
    docOLAperNode: { v: 6,          u: "",               src: "OLA Integration, p.1" },
    docWheelDiameter: { v: 2000,    u: "m",              src: "the OLAP Initiative Moodboard, p.3 (a wheel over 2 km across)" },
    docWheelRPM: { v: 1,            u: "rpm",            src: "the OLAP Initiative, p.11" },
    docShiftFraction: { v: 0.001,   u: "of L",           src: "the OLAP Initiative, p.11 (a 0.1% shift)" },
    docNudges:  { v: 1e6,           u: "",               src: "the OLAP Initiative, p.11" },
    docMassPerNudge: { v: 5.6e19,   u: "kg",             src: "the OLAP Initiative, p.11 (GPT-4o's working)" },
    docDepartureYears: { v: [1e9, 2e9], u: "yr",         src: "the OLAP Initiative, p.3" },
    docExitYears: { v: 7961,        u: "yr",             src: "the OLAP Initiative, p.11 (50,000 AU / 2π AU per year)" },
    docExitAU:  { v: 50000,         u: "AU",             src: "the OLAP Initiative, p.11" }
  };

  // Flywheel materials: the fastest a thin hoop can turn is v = sqrt(sigma / rho) (hoop stress).
  // Working stresses are typical published values with margin, not limits.
  C.materials = { v: [
    { id: "doc",     name: "As published: 2 km wheel at 1 rpm", vmax: 104.72, note: "the OLAP Initiative, p.11" },
    { id: "steel",   name: "High-strength steel", sigma: 1.0e9, rho: 7850, note: "working stress ~1 GPa" },
    { id: "cfrp",    name: "Carbon-fibre composite", sigma: 1.5e9, rho: 1600, note: "working stress ~1.5 GPa (flywheel practice)" },
    { id: "cnt",     name: "Carbon nanotube / graphene (theoretical)", sigma: 50e9, rho: 1300, note: "far beyond any built structure" },
    { id: "stream",  name: "Maglev mass stream in a ring tunnel", vmax: 7900, note: "a stream below orbital speed rests on its tunnel: no hoop stress; the limit is orbital speed" }
  ], src: "hoop stress v = sqrt(sigma/rho); values typical" };

  // Bodies the steering system could be built into (it is not only for the Earth).
  //   M kg, R m (mean), k = moment of inertia / (M R^2), T s (sidereal rotation).
  C.bodies = { v: [
    { id: "earth", name: "Earth", M: 5.9722e24, R: 6.3710e6, Rin: 6.3781e6, k: 0.3307, T: 86164.0905 },
    { id: "mars", name: "Mars", M: 6.4171e23, R: 3.3895e6, k: 0.3644, T: 88642.66 },
    { id: "moon", name: "The Moon", M: 7.342e22, R: 1.7374e6, k: 0.3929, T: 2360591.5 },
    { id: "ceres", name: "Ceres", M: 9.384e20, R: 4.697e5, k: 0.37, T: 32667 },
    { id: "asteroid", name: "A 10 km rocky asteroid", M: 1.047e15, R: 5000, k: 0.4, T: 21600 }
  ], src: "NASA fact sheets (Earth, Mars, Moon); Park et al. 2016 (Ceres); the asteroid is illustrative (density 2,000 kg/m^3, 6 h day)" };
  C.starDensity = { v: 0.1, u: "stars per pc^3", src: "local stellar number density, order of magnitude" };
  C.maglevRecord = { v: 167.5, u: "m/s", src: "603 km/h, JR Central L0 series, 2015" };
  // Train speeds to choose from: realistic first. Safety rises with every step up.
  C.trainSpeeds = { v: [
    { id: "record", name: "Today's record: 603 km/h (proven)", v: 167.5 },
    { id: "tube", name: "1,000 km/h in a low-pressure tube (being developed)", v: 277.8 },
    { id: "mach3", name: "3,600 km/h in a vacuum tube (speculative)", v: 1000 },
    { id: "orbit", name: "Orbital speed for this body (the theoretical limit)", v: null },
    { id: "custom", name: "Set my own", v: null }
  ], src: "JR Central L0 (2015); low-pressure tube maglev targets (2020s); beyond that, speculative" };

  // Energy: what there is to spend
  C.oceanDH = { v: 155.76e-6, u: "D atoms per H atom", src: "VSMOW, IAEA reference water" };
  C.ddEnergy = { v: 3.45e14, u: "J per kg of deuterium", src: "full D-D burn: 6 D -> 2 He-4 + 2 p + 2 n + 43.2 MeV" };
  C.fissionEnergy = { v: 8.2e13, u: "J per kg fissioned", src: "about 200 MeV per fission of a heavy nucleus" };
  C.crustU = { v: 1.3e-6, u: "mass fraction", src: "bulk continental crust, Rudnick & Gao 2014, Treatise on Geochemistry 4.1" };
  C.crustTh = { v: 5.6e-6, u: "mass fraction", src: "bulk continental crust, Rudnick & Gao 2014" };
  C.population = { v: 8.2e9, u: "people", src: "UN World Population Prospects 2024" };
  C.launchPerYear = { v: 3e6, u: "kg to orbit per year", src: "order of magnitude, worldwide c. 2024" };
  C.tntMegaton = { v: 4.184e15, u: "J", src: "definition" };

  root.OLAP = root.OLAP || {};
  root.OLAP.C = C;
  // Shorthand: K.x is C.x.v
  root.OLAP.K = new Proxy(C, { get: (t, k) => (t[k] ? t[k].v : undefined) });
})(typeof window !== "undefined" ? window : globalThis);
