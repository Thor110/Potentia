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

// The largest .sieve sent to Sieve: its base64, with the request around it, within 64 MiB.
const MAX_SIEVE_BYTES = 48 * 1024 * 1024;

// A URL's part as text: decoded where it is well formed ("%" alone is not), else as it stands.
function decodedOr(text) {
  try {
    return decodeURIComponent(text);
  } catch (e) {
    return text;
  }
}

function base64Of(bytes) {
  let s = "";
  for (let i = 0; i < bytes.length; i += 0x8000) s += String.fromCharCode.apply(null, bytes.subarray(i, i + 0x8000));
  return btoa(s);
}

// A listing's or a table's body, sent in parts as base64 of UTF-8 JSON (a message from Sieve is at
// most 1 MB, and a listing of thousands of files is more).
function bodyOf(base64) {
  const s = atob(base64);
  const b = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) b[i] = s.charCodeAt(i);
  return JSON.parse(new TextDecoder("utf-8").decode(b));
}

// `path`: one file of a .sieve of many (as its listing names it); without it, such a .sieve is
// answered with its listing.
async function decode(url, integrity, path) {
  const stated = statedSha256(integrity);
  if (!stated) throw new Error("its link states no hash (an integrity=\"sha256-...\" attribute), so it is refused");
  const started = performance.now();
  // The .sieve last fetched and checked, kept a minute: a page in a .sieve of many files asks for
  // each of its pictures from the same .sieve, one after another, and its bytes are not fetched and
  // hashed again for each. (Kept only under its own hash, checked when it was fetched.)
  if (lastFetched && lastFetched.url === url && lastFetched.sha === stated && performance.now() - lastFetched.at < 60000)
    return finish(url, stated, lastFetched.bytes, path, started, performance.now());
  // One answer for every way a fetch fails: the extension may fetch from any server, and the page
  // reads what is said here, so naming the status would tell it what is on servers it cannot reach.
  let response;
  try {
    response = await fetch(url, { credentials: "omit", cache: "no-cache" });
  } catch (e) {
    response = null;
  }
  if (!response || !response.ok) throw new Error("it could not be fetched");
  const bytes = new Uint8Array(await response.arrayBuffer());
  // Sieve takes a message of at most 64 MiB, and base64 makes four bytes of three.
  if (bytes.length > MAX_SIEVE_BYTES) throw new Error("it is more than " + MAX_SIEVE_BYTES + " bytes, more than the plugin takes");
  // The link's hash first: a file that is not the one the page meant never reaches the decoder.
  const own = base64Of(new Uint8Array(await crypto.subtle.digest("SHA-256", bytes)));
  if (own !== stated) throw new Error("its SHA-256 is not the one its link states: refused");
  lastFetched = { url, sha: stated, bytes, at: performance.now() };
  return finish(url, stated, bytes, path, started, performance.now());
}

let lastFetched = null;

// The checked bytes handed to Sieve, and its answer given back with the times taken.
async function finish(url, stated, bytes, path, started, checked) {
  const name = decodedOr(new URL(url).pathname.split("/").pop());
  const request = { op: "decode", name, sha256: "sha256-" + stated, data: base64Of(bytes) };
  if (path) request.path = path;
  const { head, data } = await ask(request);
  const done = performance.now();
  // The times, in seconds: fetching it and checking its hash here; Sieve's own checking and
  // decoding; and the whole round trip to Sieve (the decoding, and handing the bytes both ways).
  const times = { fetched: (checked - started) / 1000, decoded: Number(head.seconds || 0), sieve: (done - checked) / 1000, total: (done - started) / 1000 };
  if (head.listing) {
    const body = bodyOf(data);
    return { listing: true, name: head.name, files: body.files, folders: body.folders, size: head.size, sieveSha256: stated, sieveBytes: bytes.length, times };
  }
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

// The remembered sites and the guard's registration are changed one change at a time: two at once
// (a first visit in two tabs, or one beside the browser starting) would each read the sites before
// the other wrote them, losing one, or register the guard twice, which Chrome refuses.
let siteChanges = Promise.resolve();
function oneAtATime(job) {
  const next = siteChanges.then(job, job);
  siteChanges = next.catch(() => {});
  return next;
}

function changeSites(change) {
  return oneAtATime(async () => {
    const sites = await rememberedSites();
    change(sites);
    await chrome.storage.local.set({ sites });
    await registerGuardNow();
    return sites;
  });
}

function registerGuard() {
  return oneAtATime(registerGuardNow);
}

// The guard (guard.js, at document_start) runs only on the sites remembered: a match pattern each.
async function registerGuardNow() {
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
    const { data } = await ask({ op: "table", sha256: sha, data: base64Of(bytes) });
    tables.set(sha, new Map(bodyOf(data).files.map((f) => [f.path, f.sha256])));
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

// A page as the guard knows it: its address without its query or fragment.
function pageOf(url) {
  const page = new URL(url);
  page.hash = "";
  page.search = "";
  return page;
}

// "Show it anyway" for a page not in its site's table, in one tab, until the tab is closed (or the
// browser is): kept here, where the page's own scripts cannot set it, as they could the page's
// sessionStorage (27 October 2026).
async function shownAnyway() {
  return (await chrome.storage.session.get("unchecked")).unchecked || {};
}

async function showAnyway(tabId, url) {
  const unchecked = await shownAnyway();
  unchecked[tabId + " " + pageOf(url).href] = true;
  await chrome.storage.session.set({ unchecked });
}

chrome.tabs.onRemoved.addListener(async (tabId) => {
  const unchecked = await shownAnyway();
  for (const key of Object.keys(unchecked)) if (key.startsWith(tabId + " ")) delete unchecked[key];
  await chrome.storage.session.set({ unchecked });
});

// A page checked against its site's table. `tableUrl`: where the page says the table is (a site not
// yet remembered); a remembered site's is its prefix + index.sieve. `tabId`: the tab it is in.
// Answers {ok} with what was found (unchecked: a page not in the table, shown anyway in this tab),
// or {ok: false} and why: changed (the table's hash is not the one remembered), unlisted, mismatch,
// or an error.
async function guard(pageUrl, tableUrl, tabId) {
  const page = pageOf(pageUrl);
  let site = await siteOf(page.href);
  let first = false;
  if (!site) {
    if (!tableUrl) return { ok: true, none: true };
    const prefix = new URL(".", tableUrl).href;
    if (!page.href.startsWith(prefix)) return { ok: false, error: "the page names a table (" + tableUrl + ") above its own site's folder" };
    const t = await tableOf(prefix + "index.sieve");
    const sites = await changeSites((all) => {
      all[prefix] = { sha256: t.sha, since: new Date().toISOString().slice(0, 10), how: "first visit" };
    });
    site = { prefix, ...sites[prefix] };
    first = true;
  }
  const t = await tableOf(site.prefix + "index.sieve");
  if (t.sha !== site.sha256) return { ok: false, changed: true, prefix: site.prefix, was: site.sha256, now: t.sha };
  let path = decodedOr(page.href.slice(site.prefix.length));
  if (path === "" || path.endsWith("/")) path += "index.html";
  const want = t.files.get(path);
  if (!want) {
    // (Only a page the table does not name can be shown anyway: one that does not match is refused
    // whatever was chosen before.)
    if (tabId !== undefined && (await shownAnyway())[tabId + " " + page.href]) return { ok: true, unchecked: true, path, prefix: site.prefix };
    return { ok: false, unlisted: true, path, prefix: site.prefix };
  }
  // (Revalidated, never taken from this extension's own cache: the browser keeps the page's cache
  // apart from it, so a copy cached here can be the page as it was while the tab shows it changed.
  // Seen with a server sending max-age=600, as GitHub Pages does: "checked", on a changed page.)
  const response = await fetch(page.href, { credentials: "include", cache: "no-cache" });
  const got = await sha256Hex(new Uint8Array(await response.arrayBuffer()));
  if (got !== want) return { ok: false, mismatch: true, path, prefix: site.prefix, got, want };
  return { ok: true, path, prefix: site.prefix, table: t.sha, first };
}

async function trust(prefix, sha256, how) {
  await changeSites((sites) => {
    sites[prefix] = { sha256, since: new Date().toISOString().slice(0, 10), how };
  });
}

async function forget(prefix) {
  await changeSites((sites) => {
    delete sites[prefix];
  });
}

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (!request) return false;
  const answer = (p) => {
    p.then((r) => sendResponse(r)).catch((e) => sendResponse({ ok: false, error: e.message }));
    return true;
  };
  const tabId = sender.tab ? sender.tab.id : undefined;
  if (request.op === "guard") return answer(guard(request.url, request.table, tabId));
  if (request.op === "unchecked") {
    if (tabId === undefined) return false;
    return answer(showAnyway(tabId, request.url).then(() => ({ ok: true })));
  }
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
