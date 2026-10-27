// Sieve's extension: what a page is told when it is checked against its site's table (guard.js,
// content.js; web/README.md). A small mark when it matched; otherwise the page is stopped and a
// notice put in its place, saying why, with what can be done.
globalThis.sieveNotice = (() => {
  const shown = () => document.documentElement.setAttribute("data-sieve-checked", "1");

  function badge(text) {
    shown();
    const b = document.createElement("div");
    b.className = "sieve-badge";
    b.textContent = text;
    b.title = text;
    (document.body || document.documentElement).append(b);
    setTimeout(() => b.classList.add("sieve-badge-quiet"), 4000);
  }

  // The page stopped and replaced: a title, what happened, and buttons [label, action].
  function stop(title, lines, buttons) {
    window.stop();
    const box = document.createElement("div");
    box.className = "sieve-notice";
    const h = document.createElement("strong");
    h.textContent = title;
    box.append(h);
    for (const line of lines) {
      const p = document.createElement("p");
      p.textContent = line;
      box.append(p);
    }
    const row = document.createElement("div");
    for (const [label, action] of buttons) {
      const button = document.createElement("button");
      button.textContent = label;
      // Only the reader's own click: the page's scripts share this document and can call click()
      // on a button, and "Trust the new table" from a changed site's own script would undo the check.
      button.addEventListener("click", (event) => {
        if (event.isTrusted) action();
      });
      row.append(button);
    }
    box.append(row);
    const body = document.createElement("body");
    body.className = "sieve-notice-page";
    body.append(box);
    document.documentElement.replaceChildren(document.createElement("head"), body);
    shown();
  }

  // What the background found (its guard answer), told to the page.
  function tell(r) {
    if (r.ok) {
      if (r.none) return shown();
      return badge("Sieve: checked against this site's table" + (r.first ? " (remembered from now on: first visit)" : "") + " - " + r.path);
    }
    if (r.changed)
      return stop("Sieve: this site's table has changed", [
        "The table this site gave when you first came (index.sieve, SHA-256 " + r.was + ") is not the one it gives now (" + r.now + ").",
        "If the site's owner has updated it, trust the new table. If not, someone may have changed the site: leave it.",
      ], [["Trust the new table", () => chrome.runtime.sendMessage({ op: "trust", prefix: r.prefix, sha256: r.now, how: "accepted a changed table" }, () => location.reload())],
          ["Leave", () => history.back()]]);
    if (r.mismatch)
      return stop("Sieve: refused", [
        "This page (" + r.path + ") is not the one its site's table names: its SHA-256 is " + r.got + ", the table's " + r.want + ".",
        "Someone may have changed it on the server, or the site was updated without its table.",
      ], [["Leave", () => history.back()]]);
    if (r.unlisted)
      return stop("Sieve: not in this site's table", ["This page (" + r.path + ") is not one the site's table names, so it cannot be checked."],
        [["Show it anyway (not checked)", () => {
          sessionStorage.setItem("sieve-unchecked:" + location.href, "1"); // for this tab's session only
          location.reload();
        }], ["Leave", () => history.back()]]);
    return stop("Sieve: the page could not be checked", [r.error || "no answer"], [["Leave", () => history.back()]]);
  }

  return { tell, badge, shown };
})();
