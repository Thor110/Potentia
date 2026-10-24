// Sieve's viewer (web/README.md): a .sieve a link named, shown as the file it holds once it has
// been checked against the link's hash and decoded. A page is shown in a sandboxed frame with no
// scripts, so nothing in it gets the extension's powers; its .sieve pictures are checked and put
// in first, and its links to .sieve files lead back here.

const params = new URLSearchParams(location.search);
const url = params.get("u") || "";
const integrity = params.get("i") || "";
const path = params.get("p") || ""; // one file of a .sieve of many, as its listing names it
const shown = document.getElementById("shown");
const what = document.getElementById("what");

const isSieve = (u) => /\.sieve(?:[?#].*)?$/i.test(u || "");

function decode(u, i, p) {
  return new Promise((resolve) => chrome.runtime.sendMessage({ op: "decode", url: u, integrity: i, path: p || "" }, (r) => resolve(r || { ok: false, error: "no answer" })));
}

// The viewer's own address for a .sieve (and one of its files).
function viewerFor(u, i, p) {
  const next = new URL(location.href);
  next.search = "";
  next.searchParams.set("u", u);
  next.searchParams.set("i", i || "");
  if (p) next.searchParams.set("p", p);
  return next.toString();
}

// A link in a file of a .sieve of many, to another of its files: that file's path, or null (a link
// with a scheme, from the server's root, an anchor alone, or out of the .sieve).
function pathIn(from, href) {
  if (!href || /^[a-z][a-z0-9+.-]*:/i.test(href) || href[0] === "/" || href[0] === "#" || href[0] === "?") return null;
  const parts = from.split("/").slice(0, -1);
  for (const part of decodeURIComponent(href.split(/[?#]/)[0]).split("/")) {
    if (part === "..") {
      if (!parts.length) return null;
      parts.pop();
    } else if (part && part !== ".") parts.push(part);
  }
  return parts.join("/") || null;
}

function sizeText(n) {
  return n < 1024 ? n + " bytes" : n < 1048576 ? (n / 1024).toFixed(1) + " KiB" : (n / 1048576).toFixed(1) + " MiB";
}

// A .sieve of many files: its folders and files, as a tree, each file opening here by its path.
function showListing(r) {
  const t = r.times || {};
  what.textContent = r.name + ": " + r.files.length + " files in " + r.folders.length + " folders, " + r.size + " bytes, from a .sieve of " + r.sieveBytes +
    ", checked against the hash its link states; decoded by Sieve in " + (t.decoded || 0).toFixed(3) + " s, " + (t.total || 0).toFixed(3) + " s in all";
  what.title = what.textContent;
  const list = document.createElement("div");
  list.className = "listing";
  const seen = new Set();
  for (const f of r.files) {
    const parts = f.path.split("/");
    for (let k = 1; k < parts.length; k++) {
      const folder = parts.slice(0, k).join("/") + "/";
      if (seen.has(folder)) continue;
      seen.add(folder);
      const row = document.createElement("div");
      row.className = "folder";
      row.style.paddingLeft = (k - 1) * 1.5 + "em";
      row.textContent = parts[k - 1] + "/";
      list.append(row);
    }
    const row = document.createElement("div");
    row.className = "file";
    row.style.paddingLeft = (parts.length - 1) * 1.5 + "em";
    const a = document.createElement("a");
    a.href = viewerFor(url, integrity, f.path);
    a.textContent = parts[parts.length - 1];
    const about = document.createElement("span");
    about.className = "about";
    about.textContent = sizeText(f.size) + ", SHA-256 " + f.sha256;
    row.append(a, about);
    list.append(row);
  }
  shown.replaceChildren(list);
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
    if (r.ok && r.listing) {
      r.ok = false;
      r.error = "it holds " + r.files.length + " files, and a picture names one";
    }
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
  // A page from a .sieve of many files: its pictures and links to the other files in it are taken
  // from the same .sieve (checked against the same hash), by their paths.
  if (path) {
    for (const img of doc.querySelectorAll("img[src]")) {
      const p = pathIn(path, img.getAttribute("src"));
      if (!p) continue;
      img.removeAttribute("srcset");
      img.removeAttribute("sizes");
      const before = performance.now();
      const r = await decode(url, integrity, p);
      pictures += 1;
      picturesSeconds += (performance.now() - before) / 1000;
      if (r.ok && !r.listing) {
        img.setAttribute("src", "data:" + r.type + ";base64," + r.data);
        img.setAttribute("title", "checked by Sieve: " + p + ", SHA-256 " + r.sha256);
      } else {
        const box = doc.createElement("span");
        box.className = "sieve-refused";
        const of = decodeURIComponent(new URL(url).pathname.split("/").pop());
        box.textContent = "Refused by Sieve: " + (r.ok ? "not a picture" : r.error.replace(/^it holds/, of + " holds"));
        img.replaceWith(box);
      }
    }
    for (const a of doc.querySelectorAll("a[href]")) {
      const p = pathIn(path, a.getAttribute("href"));
      if (p) a.setAttribute("href", viewerFor(url, integrity, p));
    }
  }
  for (const a of doc.querySelectorAll("a[href]")) {
    const href = a.getAttribute("href");
    if (!isSieve(href) || a.getAttribute("href").startsWith(location.origin)) continue;
    a.setAttribute("href", viewerFor(new URL(href, url).toString(), a.getAttribute("integrity") || ""));
  }
  return "<!doctype html>\n" + doc.documentElement.outerHTML;
}

async function show() {
  if (!url) return refuse("no .sieve was named");
  document.title = "Sieve - " + decodeURIComponent(new URL(url).pathname.split("/").pop());
  const r = await decode(url, integrity, path);
  if (!r.ok) return refuse(r.error);
  if (r.listing) return showListing(r);
  const t = r.times || {};
  const s = (v) => (v || 0).toFixed(3) + " s";
  const from = path ? " (" + path + ", one of the files of " + decodeURIComponent(new URL(url).pathname.split("/").pop()) + ")" : "";
  what.textContent = r.name + from + " (" + r.type + ", " + r.size + " bytes, from a .sieve of " + r.sieveBytes + "): checked against the hash its link states" +
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
