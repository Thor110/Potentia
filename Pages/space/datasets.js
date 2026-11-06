// The OLAP simulation's datasets: where each comes from, how to read it, and a check that what was
// read is exactly what was recorded in data/manifest.js (row count and SHA-256).
//
// To add a dataset: add an entry here and in tools/update_data.py's DATASETS, then run
//   python3 Pages/space/tools/update_data.py
// and, for a snapshot, add <script src="space/data/<id>.js"> to space.html.
(function (root) {
  "use strict";
  const OLAP = root.OLAP;

  // SHA-256 in plain JavaScript (works everywhere, file:// included; FIPS 180-4).
  function sha256(str) {
    const bytes = new TextEncoder().encode(str);
    const K = new Uint32Array([
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2]);
    const H = new Uint32Array([0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19]);
    const len = bytes.length, total = ((len + 9 + 63) >> 6) << 6;
    const buf = new Uint8Array(total);
    buf.set(bytes); buf[len] = 0x80;
    const dv = new DataView(buf.buffer);
    dv.setUint32(total - 8, Math.floor(len / 0x20000000)); dv.setUint32(total - 4, (len << 3) >>> 0);
    const W = new Uint32Array(64);
    const rot = (x, n) => (x >>> n) | (x << (32 - n));
    for (let o = 0; o < total; o += 64) {
      for (let i = 0; i < 16; i++) W[i] = dv.getUint32(o + i * 4);
      for (let i = 16; i < 64; i++) {
        const s0 = rot(W[i - 15], 7) ^ rot(W[i - 15], 18) ^ (W[i - 15] >>> 3);
        const s1 = rot(W[i - 2], 17) ^ rot(W[i - 2], 19) ^ (W[i - 2] >>> 10);
        W[i] = (W[i - 16] + s0 + W[i - 7] + s1) >>> 0;
      }
      let [a, b, c, d, e, f, g, h] = H;
      for (let i = 0; i < 64; i++) {
        const t1 = (h + (rot(e, 6) ^ rot(e, 11) ^ rot(e, 25)) + ((e & f) ^ (~e & g)) + K[i] + W[i]) >>> 0;
        const t2 = ((rot(a, 2) ^ rot(a, 13) ^ rot(a, 22)) + ((a & b) ^ (a & c) ^ (b & c))) >>> 0;
        h = g; g = f; f = e; e = (d + t1) >>> 0; d = c; c = b; b = a; a = (t1 + t2) >>> 0;
      }
      H[0] += a; H[1] += b; H[2] += c; H[3] += d; H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }
    return Array.from(H, x => x.toString(16).padStart(8, "0")).join("");
  }

  // A VizieR tab-separated table, made canonical as tools/update_data.py does.
  function vizierCanonical(text) {
    const lines = text.split("\n").map(l => l.replace(/\r$/, "")).filter(l => l.trim() && !l.startsWith("#"));
    const rows = lines.slice(3);
    return { canon: lines[0] + "\n" + rows.map(r => r + "\n").join(""), header: lines[0].split("\t"), rows };
  }
  const num = (s) => { const t = (s || "").trim(); return t === "" ? null : Number(t); };

  // A CSV line reader that respects quoted fields.
  function csvRows(text) {
    const out = [];
    for (const line of text.split("\n")) {
      if (!line) continue;
      const f = []; let cur = "", q = false;
      for (let i = 0; i < line.length; i++) {
        const ch = line[i];
        if (q) { if (ch === '"') { if (line[i + 1] === '"') { cur += '"'; i++; } else q = false; } else cur += ch; }
        else if (ch === '"') q = true; else if (ch === ",") { f.push(cur); cur = ""; } else cur += ch;
      }
      f.push(cur); out.push(f);
    }
    return out;
  }

  const VIZIER = ["https://vizier.cds.unistra.fr", "https://vizier.cfa.harvard.edu"];

  const D = {
    "2mrs": {
      title: "2MASS Redshift Survey (2MRS)",
      cite: "Huchra et al. 2012, ApJS 199, 26 — VizieR J/ApJS/199/26",
      terms: "Read live from VizieR, not copied here: the 2MRS team ask that their files not be redistributed.",
      kind: "live",
      path: "/viz-bin/asu-tsv?-source=J/ApJS/199/26/table3&-out.max=unlimited&-out=ID,GLON,GLAT,Kcmag,cz",
      parse(t) {
        const { header, rows } = t; const ix = (n) => header.indexOf(n);
        const iI = ix("ID"), iL = ix("GLON"), iB = ix("GLAT"), iK = ix("Kcmag"), iC = ix("cz");
        return rows.map(r => r.split("\t")).map(f => ({ id: f[iI].trim(), l: num(f[iL]), b: num(f[iB]), k: num(f[iK]), cz: num(f[iC]) }))
          .filter(g => g.cz != null);
      }
    },
    localgroup: {
      title: "Local Group and nearby galaxies",
      cite: "McConnachie 2012, AJ 144, 4 — VizieR J/AJ/144/4",
      terms: "Read live from VizieR.",
      kind: "live",
      path: "/viz-bin/asu-tsv?-source=J/AJ/144/4/catalog&-out.max=unlimited&-out=Name,SubG,MType,GLON,GLAT,D,HRV,VMag",
      parse(t) {
        const { header, rows } = t; const ix = (n) => header.indexOf(n);
        return rows.map(r => r.split("\t")).map(f => ({
          name: f[ix("Name")].trim(), sub: f[ix("SubG")].trim(), type: f[ix("MType")].trim(),
          l: num(f[ix("GLON")]), b: num(f[ix("GLAT")]), D: num(f[ix("D")]), hrv: num(f[ix("HRV")]), MV: num(f[ix("VMag")])
        })).filter(g => g.D != null);
      }
    },
    exoplanets: {
      title: "Confirmed exoplanets (composite parameters)",
      cite: "NASA Exoplanet Archive, Planetary Systems Composite Parameters (pscomppars)",
      terms: "Kept here as a snapshot: the archive's data are public and it cannot be read from a browser on another site.",
      kind: "snapshot",
      parse(text) {
        const rows = csvRows(text), h = rows[0];
        return rows.slice(1).map(f => {
          const o = {}; h.forEach((k, i) => { o[k] = i < 2 ? f[i] : num(f[i]); }); return o;
        });
      }
    },
    land: {
      title: "Land outlines (1:110m)",
      cite: "Natural Earth v5.1.2, ne_110m_land (public domain)",
      terms: "Kept here as a snapshot (public domain).",
      kind: "snapshot",
      parse: (text) => JSON.parse(text)
    }
  };

  const status = {};   // id -> { state, rows, verified, via, message }
  const loaded = {};   // id -> parsed data
  const pending = {};

  function verify(id, text, rows) {
    const m = (OLAP.manifest && OLAP.manifest.datasets[id]) || {};
    const h = sha256(text);
    return { sha256: h, expected: m.sha256, verified: h === m.sha256, rows, expectedRows: m.rows, retrieved: m.retrieved };
  }

  async function load(id, onChange) {
    if (loaded[id]) return loaded[id];
    if (pending[id]) return pending[id];
    const d = D[id];
    const set = (s) => { status[id] = Object.assign(status[id] || {}, s); if (onChange) onChange(id, status[id]); };
    pending[id] = (async () => {
      set({ state: "loading" });
      try {
        let data;
        if (d.kind === "snapshot") {
          const text = OLAP.snap && OLAP.snap[id];
          if (text == null) throw new Error("snapshot file missing");
          data = d.parse(text);
          const rows = id === "land" ? data.features.length : text.split("\n").length - 2;
          set(Object.assign({ state: "ready", via: "this repository" }, verify(id, text, rows)));
        } else {
          let text = null, via = null, err = null;
          for (const m of VIZIER) {
            try {
              const r = await fetch(m + d.path);
              if (!r.ok) throw new Error("HTTP " + r.status);
              text = await r.text(); via = m; break;
            } catch (e) { err = e; }
          }
          if (text == null) throw new Error("VizieR did not answer (" + (err && err.message) + ")");
          const t = vizierCanonical(text);
          data = d.parse(t);
          set(Object.assign({ state: "ready", via }, verify(id, t.canon, t.rows.length)));
        }
        loaded[id] = data;
        return data;
      } catch (e) {
        set({ state: "error", message: e.message });
        delete pending[id];
        throw e;
      }
    })();
    return pending[id];
  }

  OLAP.data = { D, load, status, loaded, sha256, vizierCanonical };
})(typeof window !== "undefined" ? window : globalThis);
