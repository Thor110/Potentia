// Sieve's extension, marking every page it runs in as the page starts (before its own scripts can
// look), so a page can tell its reader whether the extension is there (web/test-site does).
document.documentElement.setAttribute("data-sieve-extension", "on");
