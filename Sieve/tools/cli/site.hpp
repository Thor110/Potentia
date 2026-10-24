// Sieve CLI — a website made of `.sieve` files, each named by the hash its links state
// (`sieve site`, IDEAS §19.92, web/README.md).
//
//   build SRC --out OUT   Copies the site in SRC to OUT, its pictures (and with `pages`, its pages)
//                         made into `.sieve` files, and every link and picture that names one given
//                         that file's hash, as the web writes one (`integrity="sha256-<base64>"`), so
//                         the Sieve extension shows it only if it is exactly that file. A page is
//                         made after every page it names; pages that link in a loop cannot each
//                         state the next one's hash, and are refused before anything is made, with
//                         the loop and a set of pages to keep plain that would break every loop. Last, a
//                         map of every file of the site, each named by its SHA-256 (`site.sieve`,
//                         sieve-manifest-v18): its own hash is the one to trust for the whole site.
//   check DIR             Every hash the site's pages state (its plain pages, and the pages held in
//                         its `.sieve` files) checked against the file it names, and every file
//                         against the site's map; what does not match is listed.
//
// What it reads in a page (HTML, or PHP, which is always kept as it is): the `src` of `img` and
// `source`, and the `href` of `a`, relative to the page, or from the server's root where the site is
// served from a path it starts with (`served_at`; a link with a scheme, to another server, or to an
// anchor alone is left as it is). Only a file one of those names is made into a
// `.sieve`, and never a page with a script (the extension's viewer runs none). A tag with `data-sieve-keep` is left exactly as written, and not checked (the test
// site's deliberately wrong links).
//
// What it cannot follow, nor can the extension: a file named in CSS, a script, PHP's code, or any
// other attribute (srcset, style, <link>, <video>...). Such a file is kept as it is as well, beside
// its `.sieve` (a page: kept plain), so nothing that names it is broken. It is found by its name, so
// a name a script puts together from parts is not found: `keep_originals` keeps every one.
#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace sieve::cli {

struct SiteOptions
{
    // The extensions made into `.sieve` files (lower case, no dot).
    std::vector<std::string> sieve_kinds = {"jpg", "jpeg", "png", "gif", "webp", "bmp", "svg"};
    bool pages = false;              // the pages (html, htm) too, but for those kept plain
    std::vector<std::string> plain;  // pages kept as they are (paths in the site); every index.html always is
    double search_per_file = 1;      // the rope's search for each file (seconds; 0: no limit)
    bool keep_originals = false;     // every picture made kept as it is too, beside its .sieve
    // Where the site is served from ("/" at the server's root, "/Potentia/" in a folder): a link
    // from the server's root under it ("/pictures/a.png") is read as the site's own. Empty: such
    // links are left alone.
    std::string served_at;
    // Told before each file is made: how many so far and of how many, and which.
    std::function<void(size_t, size_t, const std::string&)> progress;
};

struct SiteReport
{
    size_t files = 0, made = 0, pages_made = 0, links = 0;
    std::string map_sha256; // the site's map's own SHA-256: the site's root hash
    std::vector<std::string> notes; // what was left alone, and why
};

// Builds `out` from `src` (both folders; `out` must not be inside `src`, and is replaced, if it is
// empty or a site this built: one with a site.sieve).
SiteReport build_site(const std::filesystem::path& src, const std::filesystem::path& out, const SiteOptions& options);

struct SiteCheck
{
    size_t checked = 0;                  // stated hashes checked, and map entries
    size_t kept = 0;                     // links with data-sieve-keep: left as written, not checked
    size_t unread = 0;                   // links from the server's root stating a hash, not under --served-at
    std::vector<std::string> mismatches; // "page: what" for each that did not match
};

// Checks a built site (as above); `served_at` as SiteOptions'.
SiteCheck check_site(const std::filesystem::path& dir, const std::string& served_at = "");

// What a page's link states, as written ("sha256-" and the base64 of the SHA-256), for bytes.
std::string integrity_of(const std::vector<uint8_t>& bytes);

} // namespace sieve::cli
