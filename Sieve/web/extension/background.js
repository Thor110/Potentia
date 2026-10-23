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
  if (p.head && p.got === p.head.parts) {
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

async function decode(url, integrity) {
  const stated = statedSha256(integrity);
  if (!stated) throw new Error("its link states no hash (an integrity=\"sha256-...\" attribute), so it is refused");
  const response = await fetch(url, { credentials: "omit", cache: "no-cache" });
  if (!response.ok) throw new Error("it could not be fetched (" + response.status + ")");
  const bytes = new Uint8Array(await response.arrayBuffer());
  // The link's hash first: a file that is not the one the page meant never reaches the decoder.
  const own = base64Of(new Uint8Array(await crypto.subtle.digest("SHA-256", bytes)));
  if (own !== stated) throw new Error("its SHA-256 is not the one its link states: refused");
  const name = decodeURIComponent(new URL(url).pathname.split("/").pop());
  const { head, data } = await ask({ op: "decode", name, sha256: "sha256-" + stated, data: base64Of(bytes) });
  return { name: head.name, type: head.type, size: head.size, sha256: head.sha256, sieveSha256: stated, data };
}

chrome.runtime.onMessage.addListener((request, sender, sendResponse) => {
  if (!request || request.op !== "decode") return false;
  decode(request.url, request.integrity)
    .then((r) => sendResponse(Object.assign({ ok: true }, r)))
    .catch((e) => sendResponse({ ok: false, error: e.message }));
  return true; // the answer comes later
});
