// Sieve's extension: the sites whose tables it remembers (background.js), and saving one by hand.
const rows = document.querySelector("#sites tbody");
const said = document.getElementById("said");

function show() {
  chrome.runtime.sendMessage({ op: "sites" }, (r) => {
    rows.replaceChildren();
    const sites = (r && r.sites) || {};
    for (const [prefix, s] of Object.entries(sites)) {
      const tr = document.createElement("tr");
      for (const text of [prefix, s.sha256, s.since, s.how]) {
        const td = document.createElement("td");
        td.textContent = text;
        tr.append(td);
      }
      const td = document.createElement("td");
      const forget = document.createElement("button");
      forget.textContent = "Forget";
      forget.addEventListener("click", () => chrome.runtime.sendMessage({ op: "forget", prefix }, show));
      td.append(forget);
      tr.append(td);
      rows.append(tr);
    }
    if (!Object.keys(sites).length) {
      const tr = document.createElement("tr");
      const td = document.createElement("td");
      td.colSpan = 5;
      td.textContent = "None yet.";
      tr.append(td);
      rows.append(tr);
    }
  });
}

document.getElementById("save").addEventListener("click", () => {
  let prefix = document.getElementById("prefix").value.trim();
  const sha = document.getElementById("sha").value.trim().toLowerCase();
  try {
    const u = new URL(prefix);
    if (u.protocol !== "http:" && u.protocol !== "https:") throw new Error();
    u.search = "";
    u.hash = "";
    prefix = u.href.endsWith("/") ? u.href : u.href.replace(/[^/]*$/, ""); // the site's folder
  } catch (e) {
    said.textContent = "That is not a site's address (http:// or https://).";
    return;
  }
  if (!/^[0-9a-f]{64}$/.test(sha)) {
    said.textContent = "A table's SHA-256 is 64 hex digits.";
    return;
  }
  chrome.runtime.sendMessage({ op: "trust", prefix, sha256: sha, how: "saved by hand" }, () => {
    said.textContent = "Saved: " + prefix;
    show();
  });
});

show();
