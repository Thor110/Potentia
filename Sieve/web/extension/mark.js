// Sieve's extension, marking every page it runs in as the page starts (before its own scripts can
// look), so a page can tell its reader whether the extension is there (web/test-site does).
// At document_start Chrome may run this before the page's <html> element exists ("before any
// other DOM is constructed"), so it waits for it then; content.js marks the page again, later.
(() => {
  const mark = () => document.documentElement && document.documentElement.setAttribute("data-sieve-extension", "on");
  if (document.documentElement) return mark();
  const watch = new MutationObserver(() => {
    if (document.documentElement) {
      watch.disconnect();
      mark();
    }
  });
  watch.observe(document, { childList: true });
})();
