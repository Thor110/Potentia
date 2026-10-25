// Sieve's extension, its background (web/README.md, IDEAS §19.92). Asked by a page's content
// script, or by the viewer, for a .sieve a link names, it fetches the file, checks it against the
// hash the link states before anything else, and has Sieve on this computer (`sieve web-host`,
// through native messaging) decode it; the file comes back in parts, as base64.

const HOST = "com.potentia.sieve";

let port = null;
let nextId = 1;
const pending = new Map(); // id -> { resolve, reject, head, parts, got }

function connect() {
  port = chrome.runtime.connectNative(HOST);
  port.onMessage.addListener(onReply);
  port.onDisconnect.addListener(() => {
    const why = chrome.runtime.lastError ? chrome.runtime.lastError.message : "it stopped";
    for (const p of pending.values()) p.reject(new Error("Sieve could not be reached (" + why + "): is it registered? See web/README.md."));
    pending.clear();
    port = null;
  });
}

function onReply(msg) {
  const p = pending.get(msg.id);
  if (!p) return;
  if ("part" in msg) {
    p.parts[msg.part] = msg.data;
    p.got += 1;
  } else if (!msg.ok) {
    pending.delete(msg.id);
    p.reject(new Error(msg.error));
    return;
  } else {
    p.head = msg;
  }
  if (p.head && p.got === (p.head.parts || 0)) { // (a reply with no parts: hello, a table)
    pending.delete(msg.id);
    // Every part but the last is a whole number of 3-byte groups, so the parts join as one base64.
    p.resolve({ head: p.head, data: p.parts.join("") });
  }
}

function ask(request) {
  if (!port) connect();
  const id = nextId++;
  return new Promise((resolve, reject) => {
    pending.set(id, { resolve, reject, head: null, parts: [], got: 0 });
    port.postMessage(Object.assign({ id }, request));
  });
}

// The SHA-256 an integrity attribute states ("sha256-<base64>", among any others), as base64.
function statedSha256(integrity) {
  for (const token of (integrity || "").trim().split(/\s+/)) {
    if (token.startsWith("sha256-")) return token.slice(7).split("?")[0];
  }
  return null;
}

function base64Of(bytes) {
  let s = "";
  for (let i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
  return btoa(s);
}

// `path`: one file of a .sieve of many (as its listing names it); without it, such a .sieve is
// answered with its listing.
async function decode(url, integrity, path) {
  const stated = statedSha256(integrity);
  if (!stated) throw new Error("its link states no hash (an integrity=\"sha256-...\" attribute), so it is refused");
  const started = performance.now();
  const response = await fetch(url, { credentials: "omit", cache: "no-cache" });
  if (!response.ok) throw new Error("it could not be fetched (" + response.status + ")");
  const bytes = new Uint8Array(await response.arrayBuffer());
  // The link's hash first: a file that is not the one the page meant never reaches the decoder.
  const own = base64Of(new Uint8Array(await crypto.subtle.digest("SHA-256", bytes)));
  if (own !== stated) throw new Error("its SHA-256 is not the one its link states: refused");
  const checked = performance.now();
  const name = decodeURIComponent(new URL(url).pathname.split("/").pop());
  const request = { op: "decode", name, sha256: "sha256-" + stated, data: base64Of(bytes) };
  if (path) request.path = path;
  const { head, data } = await ask(request);
  const done = performance.now();
  // The times, in seconds: fetching it and checking its hash here; Sieve's own checking and
  // decoding; and the whole round trip to Sieve (the decoding, and handing the bytes both ways).
  const times = { fetched: (checked - started) / 1000, decoded: Number(head.seconds || 0), sieve: (done - checked) / 1000, total: (done - started) / 1000 };
  if (head.listing)
    return { listing: true, name: head.name, files: head.files, folders: head.folders, size: head.size, sieveSha256: stated, sieveBytes: bytes.length, times };
  return { name: head.name, type: head.type, size: head.size, sha256: head.sha256, sieveSha256: stated, sieveBytes: bytes.length, times, data };
}

// ---- A site's table (index.sieve, written by `sieve site build`): every file of the site by its
// SHA-256. Its own hash is remembered the first time a page of the site leads to it (or saved by
// hand on the settings page) and every later visit is held to it: a page is shown only once its
// bytes are the ones the table names (IDEAS §19.92, 25 October 2026).

function hexOf(bytes) {
  return Array.from(bytes, (b) => b.toString(16).padStart(2, "0")).join("");
}

async function sha256Hex(bytes) {
  return hexOf(new Uint8Array(await crypto.subtle.digest("SHA-256", bytes)));
}

async function rememberedSites() {
  return (await chrome.storage.local.get("sites")).sites || {};
}

// The guard (guard.js, at document_start) runs only on the sites remembered: a match pattern each.
async function registerGuard() {
  const sites = await rememberedSites();
  const matches = [...new Set(Object.keys(sites).map((prefix) => {
    const u = new URL(prefix);
    return u.protocol + "//" + u.hostname + u.pathname + "*";
  }))];
  try {
    await chrome.scripting.unregisterContentScripts({ ids: ["sieve-guard"] });
  } catch (e) {
    // none registered yet
  }
  if (matches.length)
    await chrome.scripting.registerContentScripts([{ id: "sieve-guard", matches, js: ["notice.js", "guard.js"], css: ["guard.css", "content.css"], runAt: "document_start" }]);
}

chrome.runtime.onInstalled.addListener(registerGuard);
chrome.runtime.onStartup.addListener(registerGuard);

const tables = new Map(); // a table's SHA-256 -> Map(path -> SHA-256)

async function tableOf(tableUrl) {
  const response = await fetch(tableUrl, { credentials: "omit", cache: "no-cache" });
  if (!response.ok) throw new Error("the site's table (" + tableUrl + ") could not be fetched (" + response.status + ")");
  const bytes = new Uint8Array(await response.arrayBuffer());
  const sha = await sha256Hex(bytes);
  if (!tables.has(sha)) {
    const { head } = await ask({ op: "table", sha256: sha, data: base64Of(bytes) });
    tables.set(sha, new Map(head.files.map((f) => [f.path, f.sha256])));
  }
  return { sha, files: tables.get(sha) };
}

// The remembered site a page is part of (the longest prefix), if any.
async function siteOf(url) {
  const sites = await rememberedSites();
  let best = null;
  for (const prefix of Object.keys(sites)) if (url.startsWith(prefix) && (!best || prefix.length > best.length)) best = prefix;
  return best ? { prefix: best, ...sites[best] } : null;
}

// A page checked against its site's table. `tableUrl`: where the page says the table is (a site not
// yet remembered); a remembered site's is its prefix + index.sieve. Answers {ok} with what was
// found, or {ok: false} and why: changed (the table's hash is not the one remembered), unlisted,
// mismatch, or an error.
async function guard(pageUrl, tableUrl) {
  const page = new URL(pageUrl);
  page.hash = "";
  page.search = "";
  let site = await siteOf(page.href);
  let first = false;
  if (!site) {
    if (!tableUrl) return { ok: true, none: true };
    const prefix = new URL(".", tableUrl).href;
    if (!page.href.startsWith(prefix)) return { ok: false, error: "the page names a table (" + tableUrl + ") above its own site's folder" };
    const t = await tableOf(prefix + "index.sieve");
    const sites = await rememberedSites();
    sites[prefix] = { sha256: t.sha, since: new Date().toISOString().slice(0, 10), how: "first visit" };
    await chrome.storage.local.set({ sites });
    await registerGuard();
    site = { prefix, ...sites[prefix] };
    first = true;
  }
  const t = await tableOf(site.prefix + "index.sieve");
  if (t.sha !== site.sha256) return { ok: false, changed: true, prefix: site.prefix, was: site.sha256, now: t.sha };
  let path = decodeURIComponent(page.href.slice(site.prefix.length));
  if (path === "" || path.endsWith("/")) path += "index.html";
  const want = t.files.get(path);
  if (!want) return { ok: false, unlisted: true, path, prefix: site.prefix };
  const response = await fetch(page.href, { credentials: "include" });
  const got = await sha256Hex(new Uint8Array(await response.arrayBuffer()));
  if (got !== want) return { ok: false, mismatch: true, path, prefix: site.prefix, got, want };
  return { ok: true, path, prefix: site.prefix, table: t.sha, first };
}

async function trust(prefix, sha256, how) {
  const sites = await rememberedSites();
  sites[prefix] = { sha256, since: new Date().toISOString().slice(0, 10), how };
  await chrome.storage.local.set({ sites });
  await registerGuard();
}

async function forget(prefix) {
  const sites = await rememberedSites();
  delete sites[prefix];
  await chrome.storage.local.set({ sites });
  await registerGuard();
}

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (!request) return false;
  const answer = (p) => {
    p.then((r) => sendResponse(r)).catch((e) => sendResponse({ ok: false, error: e.message }));
    return true;
  };
  if (request.op === "guard") return answer(guard(request.url, request.table));
  if (request.op === "trust") return answer(trust(request.prefix, request.sha256, request.how || "accepted").then(() => ({ ok: true })));
  if (request.op === "forget") return answer(forget(request.prefix).then(() => ({ ok: true })));
  if (request.op === "sites") return answer(rememberedSites().then((sites) => ({ ok: true, sites })));
  return false;
});

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (!request || request.op !== "decode") return false;
  decode(request.url, request.integrity, request.path)
    .then((r) => sendResponse(Object.assign({ ok: true }, r)))
    .catch((e) => sendResponse({ ok: false, error: e.message }));
  return true; // the answer comes later
});
