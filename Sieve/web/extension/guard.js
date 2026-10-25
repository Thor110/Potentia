// Sieve's extension, at the start of every page of a site whose table it remembers (registered for
// those sites only, by background.js). guard.css holds the page hidden; it is shown once the
// background has checked its bytes against the table, or stopped and told why (notice.js).
globalThis.sieveGuarded = true;
const unchecked = (() => {
  try {
    return sessionStorage.getItem("sieve-unchecked:" + location.href) === "1"; // "Show it anyway", in this tab
  } catch (e) {
    return false;
  }
})();
if (unchecked) {
  const mark = () => sieveNotice.badge("Sieve: shown anyway - this page is not in its site's table, and was not checked");
  if (document.body) mark();
  else document.addEventListener("DOMContentLoaded", mark);
} else chrome.runtime.sendMessage({ op: "guard", url: location.href }, (r) => {
  const tell = () => sieveNotice.tell(chrome.runtime.lastError ? { ok: false, error: chrome.runtime.lastError.message } : r || { ok: false, error: "no answer" });
  if (document.documentElement) tell();
  else document.addEventListener("DOMContentLoaded", tell);
});
