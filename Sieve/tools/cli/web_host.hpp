// Sieve CLI — the web plugin's host (IDEAS §19.92): `sieve web-host`, which a browser's extension
// starts through native messaging, decodes the `.sieve` files a page links to and hands back what
// they hold, for the browser to show as it would the file itself.
//
// What it accepts, since every `.sieve` it is given comes from a stranger:
//   - only instructions whose own SHA-256 is the one the link states (the page's `integrity`
//     attribute): checked before a byte of them is decoded, so a file that is not the one the page
//     meant never reaches the decoder (a file's own hashes prove nothing about who made it);
//   - one file at a time: a link names one thing, so a `.sieve` of many files is answered with its
//     listing (its folders and files, each with its size and SHA-256), and any one of them is
//     decoded when it is asked for by its path (24 October 2026; never written to the disk);
//   - nothing that asks for more than decoding: no items of Sieve's lines, no remade archives, no
//     map's graph part;
//   - at most `max_bytes` decoded (256 MiB unless given), by the sizes the instructions declare,
//     checked before anything that size is made (locate.hpp's DecodeLimit);
//   - nothing the vault withholds (vault.hpp), as installing and extracting refuse it.
//
// Native messaging (Chrome's, as Edge's and Firefox's): the browser starts the program with the
// extension's origin as its first argument and talks over standard input and output, each message
// a 4-byte length (this machine's byte order) and that many bytes of UTF-8 JSON. A message to the
// browser is at most 1 MB, so a file comes back in parts.
//
// Requests, one JSON object each:
//   {"id": N, "op": "hello"}
//       -> {"id": N, "ok": true, "sieve": "<version>"}
//   {"id": N, "op": "decode", "sha256": "<hex, or sha256-<base64> as an integrity attribute>",
//    "name": "<the .sieve's name, for messages>", "data": "<the .sieve, base64>"}
//       -> {"id": N, "ok": true, "name": "<the file's name>", "type": "<its media type>",
//           "size": <bytes>, "sha256": "<its SHA-256, hex>", "seconds": <checking and decoding
//           it here took>, "parts": K}
//          then K messages {"id": N, "part": i, "data": "<base64>"}, i from 0;
//       or, for a .sieve of many files and no "path": {"id": N, "ok": true, "listing": true,
//           "name": "<the .sieve's own name>", "files": [{"path", "size", "sha256"}, ...],
//           "folders": ["<path>/", ...], "size": <every file's bytes>, "seconds", "parts": 0};
//       or {"id": N, "ok": false, "error": "<why it was refused>"}.
//   A decode may name "path": the file of that path in the .sieve (as its listing gives it).
//   {"id": N, "op": "table", "sha256": "<as for decode>", "data": "<a site's index.sieve, base64>"}
//       -> {"id": N, "ok": true, "files": [{"path", "sha256"}, ...]}: a site's table (a map, as
//          `sieve site build` writes it), every file it names by its SHA-256 (25 October 2026).
#pragma once

#include <cstdint>
#include <iosfwd>
#include <string>
#include <vector>

namespace sieve::cli {

struct WebDecoded
{
    std::string name;   // the file's name, as the instructions give it
    std::string type;   // its media type, from its name (application/octet-stream when unknown)
    std::string sha256; // its own, hex
    std::vector<uint8_t> bytes;
};

inline constexpr uint64_t kWebMaxBytes = uint64_t(256) << 20;

struct WebFile
{
    std::string path, sha256; // as the listing gives it; its own SHA-256, hex
    uint64_t size = 0;
};

// What a `.sieve` is, checked as above: one file decoded (`path`, or its only file), or for a
// `.sieve` of many files and no `path`, its listing.
struct WebOpened
{
    bool listing = false;
    std::vector<WebFile> files;        // the listing (listing only)
    std::vector<std::string> folders;  // its folders, "a/b/" (listing only)
    uint64_t total = 0;                // every file's bytes
    WebDecoded file;                   // the file (not a listing)
};
// A site's table (index.sieve: a map), checked against `expected` as above: its files, each named
// by its SHA-256.
std::vector<WebFile> web_table(const std::vector<uint8_t>& sieve, const std::string& expected);

WebOpened web_open(const std::vector<uint8_t>& sieve, const std::string& expected, const std::string& path = "",
                   uint64_t max_bytes = kWebMaxBytes);

// The one file a `.sieve` holds, refused (std::runtime_error, saying why) unless the instructions'
// own SHA-256 is `expected` (hex, or "sha256-" and base64) and they are as above.
WebDecoded web_decode(const std::vector<uint8_t>& sieve, const std::string& expected, uint64_t max_bytes = kWebMaxBytes);

// A file's media type from its name's extension.
std::string media_type_of(const std::string& name);

// Base64 (the standard alphabet, padded), both ways; decoding throws on anything else.
std::string base64_of(const uint8_t* data, size_t n);
std::vector<uint8_t> bytes_of_base64(const std::string& text);

// The native messaging loop on these streams, until the browser closes them. 0 when it ends
// cleanly.
// `version` is what "hello" answers.
int run_web_host(std::istream& in, std::ostream& out, const std::string& version, uint64_t max_bytes = kWebMaxBytes);

} // namespace sieve::cli
