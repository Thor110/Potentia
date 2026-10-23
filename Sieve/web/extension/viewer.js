// Sieve's viewer (web/README.md): a .sieve a link named, shown as the file it holds once it has
// been checked against the link's hash and decoded. A page is shown in a sandboxed frame with no
// scripts, so nothing in it gets the extension's powers; its .sieve pictures are checked and put
// in first, and its links to .sieve files lead back here.

const params = new URLSearchParams(location.search);
const url = params.get("u") || "";
const integrity = params.get("i") || "";
const shown = document.getElementById("shown");
const what = document.getElementById("what");

const isSieve = (u) => /\.sieve(?:[?#].*)?$/i.test(u || "");

function decode(u, i) {
  return new Promise((resolve) => chrome.runtime.sendMessage({ op: "decode", url: u, integrity: i }, (r) => resolve(r || { ok: false, error: "no answer" })));
}

function bytesOf(base64) {
  const s = atob(base64);
  const b = new Uint8Array(s.length);
  for (let i = 0; i < s.length; i++) b[i] = s.charCodeAt(i);
  return b;
}

function textOf(base64) {
  return new TextDecoder("utf-8").decode(bytesOf(base64));
}

function refuse(why) {
  document.getElementById("bar").classList.add("refused");
  what.textContent = "refused: " + url;
  const box = document.createElement("div");
  box.className = "refusal";
  const title = document.createElement("strong");
  title.textContent = "Refused by Sieve";
  const text = document.createElement("div");
  text.textContent = why;
  box.append(title, text);
  shown.replaceChildren(box);
}

// A decoded page, made ready for its frame: its address for its relative links, every link
// leaving the frame, its .sieve pictures checked and put in as data, its .sieve links through
// the viewer.
let pictures = 0, picturesSeconds = 0; // the page's own .sieve pictures, and the time they took

async function preparePage(html) {
  const doc = new DOMParser().parseFromString(html, "text/html");
  for (const s of doc.querySelectorAll("script")) s.remove(); // the frame runs none anyway
  const base = doc.createElement("base");
  base.href = url;
  base.target = "_top";
  doc.head.prepend(base);
  const style = doc.createElement("style");
  style.textContent = ".sieve-refused{display:inline-block;padding:1em;border:2px solid #c03030;background:#2a0d0d;color:#ffb0b0;font:14px sans-serif}";
  doc.head.append(style);
  for (const img of doc.querySelectorAll("img[src]")) {
    const src = img.getAttribute("src");
    if (!isSieve(src)) continue;
    // Its other sources (srcset, a <picture>'s <source>s) would win over the checked file: taken away.
    img.removeAttribute("srcset");
    img.removeAttribute("sizes");
    if (img.parentElement && img.parentElement.tagName === "PICTURE") for (const s of img.parentElement.querySelectorAll("source")) s.remove();
    const before = performance.now();
    const r = await decode(new URL(src, url).toString(), img.getAttribute("integrity"));
    pictures += 1;
    picturesSeconds += (performance.now() - before) / 1000;
    if (r.ok) {
      img.setAttribute("src", "data:" + r.type + ";base64," + r.data);
      img.setAttribute("title", "checked by Sieve: " + r.name + ", SHA-256 " + r.sha256);
    } else {
      const box = doc.createElement("span");
      box.className = "sieve-refused";
      box.textContent = "Refused by Sieve: " + r.error;
      img.replaceWith(box);
    }
  }
  for (const a of doc.querySelectorAll("a[href]")) {
    const href = a.getAttribute("href");
    if (!isSieve(href)) continue;
    const next = new URL(location.href);
    next.search = "";
    next.searchParams.set("u", new URL(href, url).toString());
    next.searchParams.set("i", a.getAttribute("integrity") || "");
    a.setAttribute("href", next.toString());
  }
  return "<!doctype html>\n" + doc.documentElement.outerHTML;
}

async function show() {
  if (!url) return refuse("no .sieve was named");
  document.title = "Sieve - " + decodeURIComponent(new URL(url).pathname.split("/").pop());
  const r = await decode(url, integrity);
  if (!r.ok) return refuse(r.error);
  const t = r.times || {};
  const s = (v) => (v || 0).toFixed(3) + " s";
  what.textContent = r.name + " (" + r.type + ", " + r.size + " bytes, from a .sieve of " + r.sieveBytes + "): checked against the hash its link states" +
    "; fetched and checked in " + s(t.fetched) + ", decoded by Sieve in " + s(t.decoded) + " (" + s(t.sieve) + " with the hand-over), " + s(t.total) + " in all" +
    "; SHA-256 " + r.sha256;
  what.title = what.textContent;
  if (r.type === "text/html") {
    const frame = document.createElement("iframe");
    // No scripts, no forms, no access to this page: only its links, which leave the viewer.
    frame.setAttribute("sandbox", "allow-top-navigation-by-user-activation allow-popups");
    frame.srcdoc = await preparePage(textOf(r.data));
    shown.replaceChildren(frame);
    if (pictures) {
      what.textContent += "; and its " + pictures + " .sieve picture" + (pictures === 1 ? "" : "s") + " in " + picturesSeconds.toFixed(3) + " s";
      what.title = what.textContent;
    }
    return;
  }
  const blobUrl = URL.createObjectURL(new Blob([bytesOf(r.data)], { type: r.type }));
  let el;
  if (r.type.startsWith("image/")) {
    el = document.createElement("img");
    el.src = blobUrl;
    el.alt = r.name;
  } else if (r.type.startsWith("video/") || r.type.startsWith("audio/")) {
    el = document.createElement(r.type.startsWith("video/") ? "video" : "audio");
    el.controls = true;
    el.src = blobUrl;
  } else if (r.type.startsWith("text/") || r.type === "application/json") {
    el = document.createElement("pre");
    el.textContent = textOf(r.data);
  } else {
    el = document.createElement("a");
    el.className = "download";
    el.href = blobUrl;
    el.download = r.name;
    el.textContent = "Save " + r.name;
  }
  shown.replaceChildren(el);
}

show();
