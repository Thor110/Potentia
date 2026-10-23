// Sieve's extension, in every page (web/README.md). A picture whose source is a .sieve is shown as
// the file it holds, once its background has checked it against the hash the picture's integrity
// attribute states and Sieve has decoded it; otherwise a refusal is shown in its place. A link to a
// .sieve opens the extension's viewer, which does the same for a page.

(() => {
  // The page marked as one the extension runs in (mark.js marks it as it starts; again here, in
  // case the page had no <html> element yet then).
  document.documentElement.setAttribute("data-sieve-extension", "on");
  const isSieve = (url) => /\.sieve(?:[?#].*)?$/i.test(url || "");
  const viewer = chrome.runtime.getURL("viewer.html");

  function bytesOf(base64) {
    const s = atob(base64);
    const b = new Uint8Array(s.length);
    for (let i = 0; i < s.length; i++) b[i] = s.charCodeAt(i);
    return b;
  }

  function refuse(img, why) {
    const box = document.createElement("div");
    box.className = "sieve-refused";
    box.setAttribute("role", "img");
    box.setAttribute("aria-label", "Refused by Sieve: " + why);
    const width = img.getAttribute("width"), height = img.getAttribute("height");
    if (width) box.style.width = /^\d+$/.test(width) ? width + "px" : width;
    if (height) box.style.height = /^\d+$/.test(height) ? height + "px" : height;
    const title = document.createElement("strong");
    title.textContent = "Refused by Sieve";
    const text = document.createElement("span");
    text.textContent = why;
    box.append(title, text);
    img.replaceWith(box);
  }

  // A picture's other sources (srcset, and a <picture>'s <source>s) win over its src, so they would
  // show a file nothing checked: taken away, leaving only the .sieve.
  function onlyTheSieve(img) {
    img.removeAttribute("srcset");
    img.removeAttribute("sizes");
    if (img.parentElement && img.parentElement.tagName === "PICTURE")
      for (const s of img.parentElement.querySelectorAll("source")) s.remove();
  }

  function showPicture(img) {
    if (img.dataset.sieve) return;
    img.dataset.sieve = "checking";
    onlyTheSieve(img);
    chrome.runtime.sendMessage({ op: "decode", url: img.src, integrity: img.getAttribute("integrity") }, (r) => {
      if (chrome.runtime.lastError) return refuse(img, chrome.runtime.lastError.message);
      if (!r || !r.ok) return refuse(img, r ? r.error : "no answer");
      const blob = new Blob([bytesOf(r.data)], { type: r.type });
      img.dataset.sieve = "shown";
      const t = r.times || {};
      img.title = (img.title ? img.title + " - " : "") + "checked by Sieve: " + r.name + ", decoded in " + (t.decoded || 0).toFixed(3) + " s (" +
        (t.total || 0).toFixed(3) + " s in all), SHA-256 " + r.sha256;
      img.src = URL.createObjectURL(blob);
    });
  }

  function viaViewer(a) {
    if (a.dataset.sieve) return;
    a.dataset.sieve = "viewer";
    const target = new URL(viewer);
    target.searchParams.set("u", a.href);
    target.searchParams.set("i", a.getAttribute("integrity") || "");
    a.href = target.toString();
  }

  function scan(root) {
    for (const img of root.querySelectorAll("img[src]")) if (isSieve(img.getAttribute("src"))) showPicture(img);
    for (const a of root.querySelectorAll("a[href]")) if (isSieve(a.getAttribute("href"))) viaViewer(a);
  }

  scan(document);
  new MutationObserver((changes) => {
    for (const c of changes) for (const n of c.addedNodes) if (n.nodeType === 1) scan(n.parentNode || n);
  }).observe(document.documentElement, { childList: true, subtree: true });
})();
