// Sieve's extension, at the start of every page of a site whose table it remembers (registered for
// those sites only, by background.js). guard.css holds the page hidden; it is shown once the
// background has checked its bytes against the table, or stopped and told why (notice.js). A page
// shown anyway ("Show it anyway", not in the table) is remembered by the background for its tab,
// and checked each time all the same: only a page the table does not name is shown so.
globalThis.sieveGuarded = true;
chrome.runtime.sendMessage({ op: "guard", url: location.href }, (r) => {
  const tell = () => sieveNotice.tell(chrome.runtime.lastError ? { ok: false, error: chrome.runtime.lastError.message } : r || { ok: false, error: "no answer" });
  if (document.documentElement) tell();
  else document.addEventListener("DOMContentLoaded", tell);
});
