// The four scales of the OLAP simulation, drawn on one canvas: the Earth and its grid, the solar
// system, the Milky Way, and the local universe from 2MRS. Each view draws the state handed to it
// (S: the plan, its results R, the time t in years from now, the loaded data) and keeps its own
// camera. app.js owns the state and the animation loop.
(function (root) {
  "use strict";
  const OLAP = root.OLAP, K = OLAP.K, P = OLAP.P;
  const COL = { blue: "#3d8fd6", amber: "#c07a10", rose: "#d0507a", ink: "#e0ddd8", ink2: "#b0aca5", muted: "#888", faint: "#666", dim: "#444", line: "#2a2a3a" };
  const D2R = Math.PI / 180;

  function text(ctx, s, x, y, color, align, size) {
    ctx.font = (size || 12) + "px 'Courier New', monospace";
    ctx.fillStyle = color || COL.muted;
    ctx.textAlign = align || "left";
    ctx.textBaseline = "middle";
    ctx.fillText(s, x, y);
  }
  function dot(ctx, x, y, r, fill, stroke) {
    ctx.beginPath(); ctx.arc(x, y, r, 0, 2 * Math.PI);
    if (fill) { ctx.fillStyle = fill; ctx.fill(); }
    if (stroke) { ctx.strokeStyle = stroke; ctx.lineWidth = 1.5; ctx.stroke(); }
  }
  function circle(ctx, x, y, r, color, dash, width) {
    ctx.beginPath(); ctx.setLineDash(dash || []); ctx.arc(x, y, Math.max(0, r), 0, 2 * Math.PI);
    ctx.strokeStyle = color; ctx.lineWidth = width || 1; ctx.stroke(); ctx.setLineDash([]);
  }

  // Galactic (l, b, distance) to Cartesian; x towards the Galactic Centre, z to the north pole.
  function galXYZ(l, b, d) {
    const cb = Math.cos(b * D2R);
    return [d * cb * Math.cos(l * D2R), d * cb * Math.sin(l * D2R), d * Math.sin(b * D2R)];
  }

  // ---------------------------------------------------------------------------------------------
  // 1. The Earth and the grid
  // ---------------------------------------------------------------------------------------------
  const planet = {
    cam: { lat: 25, lon: 10, zoom: 1 },
    mask: null, maskW: 720, maskH: 360,
    img: null,
    buildMask(geo) {
      const c = document.createElement("canvas"); c.width = this.maskW; c.height = this.maskH;
      const g = c.getContext("2d");
      g.fillStyle = "#000"; g.fillRect(0, 0, c.width, c.height);
      g.fillStyle = "#fff";
      const X = (lon) => (lon + 180) / 360 * c.width, Y = (lat) => (90 - lat) / 180 * c.height;
      for (const f of geo.features) {
        const polys = f.geometry.type === "Polygon" ? [f.geometry.coordinates] : f.geometry.coordinates;
        for (const poly of polys) {
          g.beginPath();
          for (const ring of poly) ring.forEach(([lon, lat], i) => i ? g.lineTo(X(lon), Y(lat)) : g.moveTo(X(lon), Y(lat)));
          g.fill("evenodd");
        }
      }
      const d = g.getImageData(0, 0, c.width, c.height).data;
      this.mask = new Uint8Array(c.width * c.height);
      for (let i = 0; i < this.mask.length; i++) this.mask[i] = d[i * 4] > 127 ? 1 : 0;
    },
    // Orthographic projection of an Earth-frame unit vector; lon0 includes the Earth's turning.
    project(v, lat0, lon0) {
      const lat = Math.asin(Math.max(-1, Math.min(1, v[2]))), lon = Math.atan2(v[1], v[0]);
      const dl = lon - lon0, cp = Math.cos(lat), sp = Math.sin(lat), c0 = Math.cos(lat0), s0 = Math.sin(lat0);
      return { x: cp * Math.sin(dl), y: sp * c0 - cp * s0 * Math.cos(dl), d: sp * s0 + cp * c0 * Math.cos(dl) };
    },
    draw(ctx, w, h, S) {
      const cam = this.cam;
      const Rg = 0.4 * Math.min(w, h) * cam.zoom, cx = w / 2, cy = h / 2 + 8;
      const lat0 = cam.lat * D2R, lon0 = (cam.lon + (S.spin ? S.dayPhase * 180 / Math.PI : 0)) * D2R;
      // The globe, drawn pixel by pixel from the land mask (cheap enough at a modest resolution)
      if (this.mask) {
        const N = Math.max(80, Math.min(360, Math.round(2 * Rg / 2)));
        if (!this.img || this.img.width !== N) { this.cv = document.createElement("canvas"); this.cv.width = this.cv.height = N; this.img = this.cv.getContext("2d").createImageData(N, N); }
        const data = this.img.data, c0 = Math.cos(lat0), s0 = Math.sin(lat0);
        for (let j = 0; j < N; j++) for (let i = 0; i < N; i++) {
          const x = (i + 0.5) / N * 2 - 1, y = 1 - (j + 0.5) / N * 2, rr = x * x + y * y, k = (j * N + i) * 4;
          if (rr > 1) { data[k + 3] = 0; continue; }
          const z = Math.sqrt(1 - rr);
          const lat = Math.asin(y * c0 + z * s0);
          const lon = lon0 + Math.atan2(x, z * c0 - y * s0);
          let mx = Math.floor(((lon / D2R + 540) % 360) / 360 * this.maskW), my = Math.floor((90 - lat / D2R) / 180 * this.maskH);
          if (my >= this.maskH) my = this.maskH - 1;
          const land = this.mask[my * this.maskW + mx];
          const shade = 0.55 + 0.45 * z;
          if (land) { data[k] = 70 * shade; data[k + 1] = 78 * shade; data[k + 2] = 66 * shade; }
          else { data[k] = 14 * shade; data[k + 1] = 26 * shade; data[k + 2] = 44 * shade; }
          data[k + 3] = 255;
        }
        this.cv.getContext("2d").putImageData(this.img, 0, 0);
        ctx.imageSmoothingEnabled = true;
        ctx.drawImage(this.cv, cx - Rg, cy - Rg, 2 * Rg, 2 * Rg);
      } else dot(ctx, cx, cy, Rg, "#0e1a2c");
      circle(ctx, cx, cy, Rg, COL.line);

      const Rm = S.R.gridR;
      // The rings: how much momentum each holds right now, as the Earth turns under a fixed vector
      const shares = S.R.ringSharesNow, maxS = Math.max(1e-30, ...shares.map(Math.abs));
      P.ringNormals.forEach((n0, idx) => {
        const n = P.mulMV(Rm, n0);
        const t = Math.abs(n[2]) < 0.9 ? [0, 0, 1] : [1, 0, 0];
        let u = [n[1] * t[2] - n[2] * t[1], n[2] * t[0] - n[0] * t[2], n[0] * t[1] - n[1] * t[0]];
        const ul = Math.hypot(...u); u = u.map(q => q / ul);
        const v2 = [n[1] * u[2] - n[2] * u[1], n[2] * u[0] - n[0] * u[2], n[0] * u[1] - n[1] * u[0]];
        const s = shares[idx] / maxS, col = s >= 0 ? COL.blue : COL.amber;
        let prev = null;
        for (let k = 0; k <= 180; k++) {
          const a = 2 * Math.PI * k / 180;
          const p = this.project([0, 1, 2].map(i => Math.cos(a) * u[i] + Math.sin(a) * v2[i]), lat0, lon0);
          const q = { x: cx + Rg * p.x, y: cy - Rg * p.y, front: p.d > 0 };
          if (prev) {
            ctx.beginPath(); ctx.moveTo(prev.x, prev.y); ctx.lineTo(q.x, q.y);
            if (q.front && prev.front) { ctx.strokeStyle = col; ctx.globalAlpha = 0.35 + 0.65 * Math.abs(s); ctx.lineWidth = 1 + 2.5 * Math.abs(s); }
            else { ctx.strokeStyle = COL.dim; ctx.globalAlpha = 0.35; ctx.lineWidth = 1; }
            ctx.stroke(); ctx.globalAlpha = 1;
          }
          prev = q;
        }
      });
      // The stations
      for (const nd of S.R.gridInfo.nodes) {
        const v = P.fromLatLon(nd.lat, nd.lon);
        const p = this.project(v, lat0, lon0);
        if (p.d < 0) continue;
        const r = nd.kind === "quad" ? 6 : nd.kind === "tri" ? 5 : 4;
        dot(ctx, cx + Rg * p.x, cy - Rg * p.y, r, nd.land ? COL.ink : "#050508", COL.ink);
      }
      // The spin axis
      const np = this.project([0, 0, 1], lat0, lon0), sp = this.project([0, 0, -1], lat0, lon0);
      ctx.setLineDash([3, 4]); ctx.strokeStyle = COL.faint; ctx.lineWidth = 1;
      ctx.beginPath(); ctx.moveTo(cx + 1.18 * Rg * sp.x, cy - 1.18 * Rg * sp.y); ctx.lineTo(cx + 1.18 * Rg * np.x, cy - 1.18 * Rg * np.y); ctx.stroke(); ctx.setLineDash([]);
      text(ctx, "N", cx + 1.24 * Rg * np.x, cy - 1.24 * Rg * np.y, COL.muted, "center");
      // Legend (left out on narrow screens, where the panel below explains the colours)
      if (w < 520) return;
      const lx = w - 230, ly = h - 70;
      ctx.fillStyle = "rgba(5,5,8,0.7)"; ctx.fillRect(lx - 8, ly - 14, 230, 66);
      ctx.strokeStyle = COL.blue; ctx.lineWidth = 3; ctx.beginPath(); ctx.moveTo(lx, ly); ctx.lineTo(lx + 22, ly); ctx.stroke();
      text(ctx, "ring spun one way", lx + 30, ly, COL.ink2);
      ctx.strokeStyle = COL.amber; ctx.beginPath(); ctx.moveTo(lx, ly + 18); ctx.lineTo(lx + 22, ly + 18); ctx.stroke();
      text(ctx, "ring spun the other way", lx + 30, ly + 18, COL.ink2);
      dot(ctx, lx + 6, ly + 36, 4, COL.ink, COL.ink); dot(ctx, lx + 18, ly + 36, 4, "#050508", COL.ink);
      text(ctx, "station on land / sea", lx + 30, ly + 36, COL.ink2);
    },
    drag(dx, dy) { this.cam.lon -= dx * 0.3 / this.cam.zoom; this.cam.lat = Math.max(-90, Math.min(90, this.cam.lat + dy * 0.3 / this.cam.zoom)); },
    wheel(dz) { this.cam.zoom = Math.max(0.5, Math.min(4, this.cam.zoom * Math.exp(-dz * 0.001))); },
    hint: "drag to turn the globe · scroll to zoom · line thickness: the momentum each ring holds now"
  };

  // ---------------------------------------------------------------------------------------------
  // 2. The solar system, on a logarithmic scale from 0.1 AU to 200,000 AU
  // ---------------------------------------------------------------------------------------------
  const sun = {
    cam: { zoom: 1 },
    draw(ctx, w, h, S) {
      const R = S.R, t = S.t;
      const cx = w / 2, cy = h / 2, Rpx = 0.47 * Math.min(w, h) * this.cam.zoom;
      const r0 = 0.1, r1 = 2e5, L = Math.log10(r1 / r0);
      const px = (au) => au <= r0 ? 0 : Rpx * Math.log10(au / r0) / L;
      // Habitable zone now, from the Sun's brightness at time t
      const lum = P.lum(Math.min(t, K.sunMSEndAge - K.sunAge));
      const hzIn = Math.sqrt(lum / S.plan.hzS), hzOut = Math.sqrt(lum / K.hzMaxGreen);
      ctx.beginPath(); ctx.arc(cx, cy, px(hzOut), 0, 2 * Math.PI); ctx.arc(cx, cy, px(hzIn), 0, 2 * Math.PI, true);
      ctx.fillStyle = "rgba(61,143,214,0.13)"; ctx.fill();
      text(ctx, "habitable zone", cx, cy - px(hzOut) - 9, COL.blue, "center", 11);
      // Oort cloud
      ctx.beginPath(); ctx.arc(cx, cy, px(K.oortOuter), 0, 2 * Math.PI); ctx.arc(cx, cy, px(K.oortInner), 0, 2 * Math.PI, true);
      ctx.fillStyle = "rgba(136,136,136,0.07)"; ctx.fill();
      text(ctx, "Oort cloud (2,000–100,000 AU)", cx, cy + px(K.oortOuter) - 10, COL.faint, "center", 11);
      circle(ctx, cx, cy, px(K.heliopause), COL.faint, [2, 4]);
      text(ctx, "heliopause", cx + px(K.heliopause) * 0.72 + 4, cy - px(K.heliopause) * 0.72, COL.faint, "left", 11);
      circle(ctx, cx, cy, px(K.docExitAU), COL.amber, [6, 5]);
      text(ctx, "50,000 AU (the published exit)", cx - px(K.docExitAU) * 0.71, cy - px(K.docExitAU) * 0.71 - 10, COL.amber, "center", 11);
      // Planets (orbits only: at these time scales their places are a blur)
      K.planets.forEach(([name, a], i) => {
        if (name === "Earth") return;
        circle(ctx, cx, cy, px(a), "#22222e");
        const ang = 1.9 + (i % 2 ? 0.55 : 0) + i * 0.12;   // stagger the names so they do not overlap
        text(ctx, name, cx + px(a) * Math.cos(ang), cy + px(a) * Math.sin(ang), COL.dim, "center", 10);
      });
      // Scale marks
      for (const au of [1, 100, 10000]) {
        text(ctx, au.toLocaleString("en-GB") + " AU", cx + px(au) + 3, cy + 10, COL.dim, "left", 10);
        ctx.fillStyle = COL.dim; ctx.fillRect(cx + px(au) - 0.5, cy - 3, 1, 6);
      }
      // Gliese 710's pass, near its time
      const g = K.gliese710, missAU = g.missPc * K.pc / K.AU;
      if (Math.abs(t - g.years) < 0.4e6) {
        const a = (t - g.years) / 0.4e6;
        dot(ctx, cx + px(missAU) * Math.cos(2.4 + a), cy + px(missAU) * Math.sin(2.4 + a), 4, COL.rose);
        text(ctx, "Gliese 710, 10,700 AU", cx + px(missAU) * Math.cos(2.4 + a) + 8, cy + px(missAU) * Math.sin(2.4 + a), COL.rose, "left", 11);
      }
      // The Sun
      const glow = 6 + 4 * Math.min(3, lum);
      const grad = ctx.createRadialGradient(cx, cy, 0, cx, cy, glow * 2.5);
      grad.addColorStop(0, "#fff3d0"); grad.addColorStop(0.3, "rgba(192,122,16,0.7)"); grad.addColorStop(1, "rgba(192,122,16,0)");
      ctx.fillStyle = grad; ctx.beginPath(); ctx.arc(cx, cy, glow * 2.5, 0, 2 * Math.PI); ctx.fill();
      // The Earth and its path
      const ang = -0.5;
      const rE = S.earthAU;
      if (rE != null) {
        ctx.strokeStyle = COL.blue; ctx.lineWidth = 2; ctx.beginPath();
        ctx.moveTo(cx + px(1) * Math.cos(ang), cy + px(1) * Math.sin(ang));
        ctx.lineTo(cx + px(rE) * Math.cos(ang), cy + px(rE) * Math.sin(ang)); ctx.stroke();
        if (rE <= 1.0001) circle(ctx, cx, cy, px(1), "rgba(61,143,214,0.5)");
        dot(ctx, cx + px(rE) * Math.cos(ang), cy + px(rE) * Math.sin(ang), 5, COL.blue, "#fff");
        text(ctx, "Earth", cx + px(rE) * Math.cos(ang) + 9, cy + px(rE) * Math.sin(ang) - 9, COL.ink, "left", 12);
      }
    },
    drag() {},
    wheel(dz) { this.cam.zoom = Math.max(0.5, Math.min(3, this.cam.zoom * Math.exp(-dz * 0.001))); },
    hint: "distances on a logarithmic scale · each ring is ten times the one inside it"
  };

  // ---------------------------------------------------------------------------------------------
  // 3. The Milky Way, seen from above its north pole (it turns clockwise)
  // ---------------------------------------------------------------------------------------------
  const galaxy = {
    cam: { kpc: 22, cx: 0, cy: 0 },
    draw(ctx, w, h, S) {
      this.w = w; this.h = h;
      const cam = this.cam, s = Math.min(w, h) / 2 / (cam.kpc * 1000); // px per pc
      const ox = w / 2 - cam.cx * s, oy = h / 2 - cam.cy * s;
      // Screen: Galactic Centre at the origin; the Sun starts at (-R0, 0); y drawn downward is +y_gal.
      const X = (x) => ox + x * s, Y = (y) => oy - y * s;
      // The disc
      const rd = 15000 * s;
      const grad = ctx.createRadialGradient(X(0), Y(0), 0, X(0), Y(0), rd);
      grad.addColorStop(0, "rgba(224,221,216,0.35)"); grad.addColorStop(0.15, "rgba(224,221,216,0.12)"); grad.addColorStop(1, "rgba(224,221,216,0)");
      ctx.fillStyle = grad; ctx.beginPath(); ctx.arc(X(0), Y(0), rd, 0, 2 * Math.PI); ctx.fill();
      // Spiral arms (illustrative: four logarithmic arms, pitch 12 degrees)
      ctx.strokeStyle = "rgba(224,221,216,0.08)"; ctx.lineWidth = Math.max(1, 900 * s);
      for (let k = 0; k < 4; k++) {
        ctx.beginPath();
        for (let i = 0; i <= 200; i++) {
          const th = i / 200 * 3.2 * Math.PI, r = 3000 * Math.exp(th * Math.tan(12 * D2R));
          if (r > 16000) break;
          const a = -(th + k * Math.PI / 2);
          i ? ctx.lineTo(X(r * Math.cos(a)), Y(r * Math.sin(a))) : ctx.moveTo(X(r * Math.cos(a)), Y(r * Math.sin(a)));
        }
        ctx.stroke();
      }
      // The Sun's orbit and place now
      circle(ctx, X(0), Y(0), K.R0 * s, "#2a2a3a", [4, 6]);
      dot(ctx, X(0), Y(0), 3, COL.ink);
      text(ctx, "Sgr A*", X(0) + 8, Y(0) + 10, COL.muted, "left", 11);
      const sp = S.sunGal;
      dot(ctx, X(sp[0]), Y(sp[1]), 4, COL.amber);
      text(ctx, "Sun", X(sp[0]) + 8, Y(sp[1]) - 10, COL.amber, "left", 11);
      // Nearby galaxies (Local Group catalogue), projected onto the plane
      const lg = S.data.localgroup;
      if (lg) for (const g of lg) {
        const p = galXYZ(g.l, g.b, g.D * 1000); const gx = p[0] - K.R0, gy = p[1];
        const sx = X(gx), sy = Y(gy);
        if (sx < -20 || sy < -20 || sx > w + 20 || sy > h + 20) continue;
        const big = g.MV != null && g.MV < -15;
        circle(ctx, sx, sy, big ? 5 : 2.5, big ? COL.ink2 : COL.dim);
        if (big || g.name === S.plan.target) text(ctx, g.name, sx + 8, sy, big ? COL.ink2 : COL.muted, "left", 11);
      }
      // The route
      const rt = S.R.route;
      if (rt) {
        ctx.strokeStyle = COL.blue; ctx.lineWidth = 2; ctx.beginPath();
        rt.path.forEach(([x, y], i) => i ? ctx.lineTo(X(x), Y(y)) : ctx.moveTo(X(x), Y(y)));
        ctx.stroke();
        if (rt.target) { dot(ctx, X(rt.target[0]), Y(rt.target[1]), 5, null, COL.blue); text(ctx, S.plan.target, X(rt.target[0]) + 9, Y(rt.target[1]) + 12, COL.blue, "left", 11); }
      }
      if (S.earthGal) {
        dot(ctx, X(S.earthGal[0]), Y(S.earthGal[1]), 4, COL.blue, "#fff");
        text(ctx, "Earth", X(S.earthGal[0]) + 8, Y(S.earthGal[1]) + 12, COL.ink, "left", 11);
      }
      // Scale bar
      const bar = [1, 2, 5, 10, 20, 50, 100, 200, 500, 1000].find(k => k * 1000 * s > 70) || 1000;
      ctx.fillStyle = COL.muted; ctx.fillRect(w - 30 - bar * 1000 * s, h - 24, bar * 1000 * s, 2);
      text(ctx, bar + " kpc", w - 30 - bar * 500 * s, h - 34, COL.muted, "center", 11);
      text(ctx, "spiral arms illustrative", w - 30, 22 + 40, COL.dim, "right", 10);
    },
    drag(dx, dy) { const s = 1 / (Math.min(this.w || 600, this.h || 600) / 2 / (this.cam.kpc * 1000)); this.cam.cx -= dx * s; this.cam.cy += dy * s; },
    wheel(dz) { this.cam.kpc = Math.max(0.5, Math.min(2000, this.cam.kpc * Math.exp(dz * 0.0012))); },
    // Frame the disc, or the Earth if it is further out
    fit(S, earth) {
      const d = earth ? Math.hypot(earth[0], earth[1]) : 0;
      this.cam.kpc = Math.max(20000, d * 1.25) / 1000; this.cam.cx = 0; this.cam.cy = 0;
    },
    // While playing, zoom out smoothly to keep the Earth in view
    follow(earth) {
      if (!earth) return;
      const want = Math.max(20000, Math.hypot(earth[0], earth[1]) * 1.25) / 1000;
      if (want > this.cam.kpc) this.cam.kpc += (want - this.cam.kpc) * 0.2;
    },
    hint: "drag to move · scroll to zoom · seen from the north: the Galaxy turns clockwise"
  };

  // ---------------------------------------------------------------------------------------------
  // 4. The local universe: every 2MRS galaxy, placed by its redshift
  // ---------------------------------------------------------------------------------------------
  // Well-known clusters, to find one's way (approximate positions of their central galaxies).
  const LANDMARKS = [
    ["Virgo cluster", 283.8, 74.5, 1284], ["Centaurus cluster", 302.4, 21.6, 2958], ["Hydra cluster", 269.6, 26.5, 3825],
    ["Norma cluster (Great Attractor)", 325.3, -7.3, 4871], ["Perseus cluster", 150.6, -13.3, 5264], ["Coma cluster", 58.1, 88.0, 6925]
  ];
  const universe = {
    cam: { yaw: 0.6, pitch: 0.5, mpc: 180 },
    xyz: null, src: null, cosmoKey: "",
    prepare(S) {
      const g = S.data["2mrs"]; if (!g) return;
      const key = S.plan.H0 + "|" + g.length;
      if (this.cosmoKey === key) return;
      const cos = S.R.cosmo;
      this.xyz = new Float32Array(g.length * 3); this.dist = new Float32Array(g.length);
      g.forEach((q, i) => {
        const d = cos.dcFast(Math.max(q.cz, 0) / (K.c / 1000));
        const p = galXYZ(q.l, q.b, d); this.xyz.set(p, i * 3); this.dist[i] = d;
      });
      this.lm = LANDMARKS.map(([n, l, b, cz]) => [n, galXYZ(l, b, cos.dcFast(cz / (K.c / 1000)))]);
      this.cosmoKey = key;
    },
    draw(ctx, w, h, S) {
      const cam = this.cam, s = Math.min(w, h) / 2 / cam.mpc, cx = w / 2, cy = h / 2;
      const cyw = Math.cos(cam.yaw), syw = Math.sin(cam.yaw), cp = Math.cos(cam.pitch), spt = Math.sin(cam.pitch);
      const proj = (x, y, z) => { const x1 = x * cyw - y * syw, y1 = x * syw + y * cyw; return [cx + x1 * s, cy - (z * cp - y1 * spt) * s, y1 * cp + z * spt]; };
      this.prepare(S);
      const reach = S.R.reachMpc;
      if (!this.xyz) {
        text(ctx, S.data2mrsMsg || "Reading 2MRS from VizieR…", cx, cy, COL.muted, "center", 14);
      } else {
        // Galaxies within reach in blue, the rest grey
        const n = this.dist.length;
        ctx.fillStyle = "rgba(136,136,136,0.55)";
        for (let i = 0; i < n; i++) {
          if (this.dist[i] <= reach) continue;
          const p = proj(this.xyz[i * 3], this.xyz[i * 3 + 1], this.xyz[i * 3 + 2]);
          ctx.fillRect(p[0], p[1], 1.2, 1.2);
        }
        ctx.fillStyle = COL.blue;
        for (let i = 0; i < n; i++) {
          if (this.dist[i] > reach) continue;
          const p = proj(this.xyz[i * 3], this.xyz[i * 3 + 1], this.xyz[i * 3 + 2]);
          ctx.fillRect(p[0] - 1, p[1] - 1, 2.5, 2.5);
        }
        for (const [name, v] of this.lm) {
          const p = proj(...v);
          circle(ctx, p[0], p[1], 6, COL.ink2);
          text(ctx, name, p[0] + 10, p[1], COL.ink2, "left", 11);
        }
      }
      // The reach of the Earth after the Galactic Centre, and the plane of the Milky Way
      circle(ctx, cx, cy, reach * s, COL.blue, [4, 4], 1.5);
      text(ctx, "the Earth's reach, ever: " + reach.toFixed(1) + " Mpc", cx, cy - reach * s - 12, COL.blue, "center", 11);
      ctx.strokeStyle = "rgba(208,80,122,0.35)"; ctx.lineWidth = 1; ctx.beginPath();
      for (let k = 0; k <= 72; k++) { const a = k / 72 * 2 * Math.PI, p = proj(Math.cos(a) * cam.mpc * 0.95, Math.sin(a) * cam.mpc * 0.95, 0); k ? ctx.lineTo(p[0], p[1]) : ctx.moveTo(p[0], p[1]); }
      ctx.stroke();
      const lab = proj(cam.mpc * 0.95, 0, 0);
      text(ctx, "Milky Way's plane: 2MRS cannot see through it", lab[0], lab[1] + 12, COL.rose, "center", 10);
      dot(ctx, cx, cy, 3, COL.amber);
      text(ctx, "Milky Way", cx + 8, cy + 10, COL.amber, "left", 11);
      // The target galaxy
      const rt = S.R.route;
      if (rt && rt.targetMpc) {
        const p = proj(...rt.targetMpc);
        ctx.strokeStyle = COL.blue; ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(cx, cy); ctx.lineTo(p[0], p[1]); ctx.stroke();
        dot(ctx, p[0], p[1], 4, null, COL.blue);
      }
      const bar = [1, 2, 5, 10, 20, 50, 100].find(k => k * s > 70) || 100;
      ctx.fillStyle = COL.muted; ctx.fillRect(w - 30 - bar * s, h - 24, bar * s, 2);
      text(ctx, bar + " Mpc", w - 30 - bar * s / 2, h - 34, COL.muted, "center", 11);
    },
    drag(dx, dy) { this.cam.yaw += dx * 0.006; this.cam.pitch = Math.max(-1.5, Math.min(1.5, this.cam.pitch + dy * 0.006)); },
    wheel(dz) { this.cam.mpc = Math.max(0.5, Math.min(600, this.cam.mpc * Math.exp(dz * 0.0012))); },
    hint: "drag to turn · scroll to zoom · distances from redshift (unreliable within ~10 Mpc)"
  };

  OLAP.views = { planet, sun, galaxy, universe, galXYZ, COL };
})(window);
