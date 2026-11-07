// Runs every check on the OLAP simulation, locally. Nothing to install for the basic checks.
//
//   node Pages/space/tools/test.mjs              physics checks, and the snapshot files against the manifest
//   node Pages/space/tools/test.mjs --online     also fetch every dataset again and compare (a few minutes)
//   node Pages/space/tools/test.mjs --browser    also open the page in a browser and try it (needs Playwright:
//                                                 npm install playwright && npx playwright install chromium)
//
// Exits 0 if everything passed, 1 if anything failed. Works on Windows, macOS and Linux.
import { spawnSync } from "node:child_process";
import { createRequire } from "node:module";
import { createServer } from "node:http";
import { readFile } from "node:fs/promises";
import { fileURLToPath } from "node:url";
import { dirname, join, normalize, extname } from "node:path";

const here = dirname(fileURLToPath(import.meta.url));
const repo = normalize(join(here, "..", "..", ".."));
const args = process.argv.slice(2);
const unknown = args.filter(a => !["--online", "--browser"].includes(a));
if (unknown.length) { console.error("unknown option " + unknown[0] + " (use --online, --browser, or nothing)"); process.exit(2); }

const results = [];
function record(name, ok, detail) {
  results.push({ name, ok });
  console.log((ok ? "PASS " : "FAIL ") + name + (detail ? "  " + detail : ""));
}

// 1. The physics checks
{
  const r = spawnSync(process.execPath, [join(here, "check.mjs")], { encoding: "utf8" });
  const fails = (r.stdout.match(/^FAIL .*/gm) || []);
  record("physics checks (check.mjs)", r.status === 0, fails.join("; "));
}

// 2. The data: Python 3 under whichever name this system uses
function python(extra) {
  for (const cmd of process.platform === "win32" ? ["py", "python", "python3"] : ["python3", "python"]) {
    const pre = cmd === "py" ? ["-3"] : [];
    const r = spawnSync(cmd, [...pre, join(here, "update_data.py"), ...extra], { encoding: "utf8" });
    if (r.error && r.error.code === "ENOENT") continue;
    return r;
  }
  return null;
}
{
  const r = python(["--verify"]);
  if (!r) record("snapshot files match the manifest", false, "Python 3 not found");
  else record("snapshot files match the manifest (update_data.py --verify)", r.status === 0, r.status ? r.stdout.trim().split("\n").filter(l => /DIFFERS|cannot/.test(l)).join("; ") : "");
}
if (args.includes("--online")) {
  const r = python(["--check"]);
  const why = r && r.status === 1 ? "a source has changed: run update_data.py, then re-check the figures README lists"
    : r && r.status === 2 ? (r.stderr || "").trim().split("\n").pop() : "";
  record("every source still matches the manifest (update_data.py --check)", !!r && r.status === 0, why);
}

// 3. The page in a browser
if (args.includes("--browser")) {
  let chromium;
  try { ({ chromium } = await import("playwright")); }
  catch {
    // A global install is found by require, not by import
    try { ({ chromium } = createRequire(import.meta.url)("playwright")); }
    catch { record("browser tests", false, "Playwright is not installed: npm install playwright && npx playwright install chromium"); }
  }
  if (chromium) await browserTests(chromium);
}

async function browserTests(chromium) {
  // A small static server for the repository
  const types = { ".html": "text/html", ".js": "text/javascript", ".mjs": "text/javascript", ".css": "text/css", ".json": "application/json", ".png": "image/png", ".svg": "image/svg+xml", ".md": "text/plain" };
  const server = createServer(async (req, res) => {
    try {
      const path = normalize(join(repo, decodeURIComponent(new URL(req.url, "http://x").pathname)));
      if (!path.startsWith(repo)) { res.writeHead(403).end(); return; }
      const body = await readFile(path);
      res.writeHead(200, { "Content-Type": types[extname(path)] || "application/octet-stream" }).end(body);
    } catch { res.writeHead(404).end(); }
  });
  await new Promise(ok => server.listen(0, "127.0.0.1", ok));
  const base = "http://127.0.0.1:" + server.address().port + "/Pages/space.html";
  const browser = await chromium.launch();
  const page = async (hash) => {
    const p = await browser.newPage({ viewport: { width: 1280, height: 900 } });
    const errors = [], dialogs = [];
    p.on("pageerror", e => errors.push(e.message));
    p.on("dialog", d => { dialogs.push(d.message()); d.dismiss(); });
    await p.goto(base + (hash || ""));
    await p.waitForTimeout(1500);
    return { p, errors, dialogs };
  };
  try {
    // The page loads, its snapshots verify, and every view draws without errors
    {
      const { p, errors } = await page("");
      for (const v of ["planet", "sun", "galaxy", "universe"]) { await p.click(`.tab[data-view="${v}"]`); await p.waitForTimeout(400); }
      const verified = await p.evaluate(() => ["exoplanets", "land"].every(id => (OLAP.data.status[id] || {}).verified));
      record("page loads and every view draws", errors.length === 0, errors.join("; "));
      record("snapshots verify in the browser", verified);
      const ms = await p.evaluate(() => { const el = document.getElementById("f-vinf"); const t = performance.now(); el.value = 25; el.dispatchEvent(new Event("input")); return performance.now() - t; });
      record("a settings change is quick", ms < 50, ms.toFixed(1) + " ms");
      for (const preset of ["conservative", "optimistic", "published"]) await p.click(`[data-preset="${preset}"]`);
      await p.click("#robust-btn");
      await p.waitForFunction(() => document.querySelectorAll("#robust tr").length > 1, null, { timeout: 30000 }).catch(() => {});
      const rows = await p.$$eval("#robust tr", t => t.length);
      record("presets and the robustness test run", errors.length === 0 && rows > 1, errors.join("; "));
      await p.close();
    }
    // A link that tries to run script is refused
    {
      const { p, dialogs } = await page("#H0=%3Cimg%20src%3Dx%20onerror%3Dalert(1)%3E");
      await p.click('.tab[data-view="universe"]'); await p.waitForTimeout(800);
      record("a link cannot run script in the page", dialogs.length === 0);
      await p.close();
    }
    // A link with bad settings falls back to the defaults, and is tidied
    {
      const { p, errors } = await page("#method=rocket&depYr=abc&fwPerNode=0&bogus=1");
      const v = await p.evaluate(() => [document.getElementById("f-method").value, location.hash]);
      record("bad link settings fall back to the defaults", errors.length === 0 && v[0] === "flyby" && v[1] === "", JSON.stringify(v));
      await p.close();
    }
    // Every body draws its surface
    for (const body of ["moon", "mars", "ceres", "asteroid"]) {
      const { p, errors } = await page("#fwBody=" + body);
      await p.waitForTimeout(800);
      const ok = await p.evaluate((b) => !!OLAP.views.planet.skins[b], body);
      record("surface of " + body, ok && errors.length === 0, errors.join("; "));
      await p.close();
    }
    // A phone-sized screen: nothing pushes the page sideways
    {
      const p = await browser.newPage({ viewport: { width: 390, height: 844 } });
      await p.goto(base); await p.waitForTimeout(1200);
      const w = await p.evaluate(() => document.documentElement.scrollWidth);
      record("fits a phone's width", w <= 390, w + " px");
      await p.close();
    }
  } finally {
    await browser.close();
    server.close();
  }
}

const failed = results.filter(r => !r.ok).length;
console.log(failed ? `\n${failed} of ${results.length} failed` : `\nall ${results.length} passed`);
process.exit(failed ? 1 : 0);
