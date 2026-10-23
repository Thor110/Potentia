// Sieve CLI — the web plugin's host (IDEAS §19.92): `sieve web-host`, which a browser's extension
// starts through native messaging, decodes the `.sieve` files a page links to and hands back what
// they hold, for the browser to show as it would the file itself.
//
// What it accepts, since every `.sieve` it is given comes from a stranger:
//   - only instructions whose own SHA-256 is the one the link states (the page's `integrity`
//     attribute): checked before a byte of them is decoded, so a file that is not the one the page
//     meant never reaches the decoder (a file's own hashes prove nothing about who made it);
//   - only one file: a link names one thing (a `.sieve` of many files is saved and installed with
//     Sieve instead);
//   - nothing that asks for more than decoding: no items of Sieve's lines, no remade archives, no
//     map's graph part;
//   - at most `max_bytes` decoded (256 MiB unless given).
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
//           "size": <bytes>, "sha256": "<its SHA-256, hex>", "parts": K}
//          then K messages {"id": N, "part": i, "data": "<base64>"}, i from 0;
//       or {"id": N, "ok": false, "error": "<why it was refused>"}.
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
