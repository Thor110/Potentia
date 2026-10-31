// Sieve CLI -- a JPEG made again exactly from its brunsli form (v32 in the making, `jbr`).
//
// brunsli (Google, MIT; third_party_licenses/brunsli), pinned in CMakeLists.txt: a JPEG's own coded
// values recoded by a model made for them, given back byte for byte. Measured on ten original JPEGs
// (NEXT-VERSION §4a, 3): 15.25% to 21.91% smaller, every one exact. Its headers are seen only here.

#pragma once

#include <cstdint>
#include <optional>
#include <vector>

namespace sieve::cli {

// Whether the bytes begin as a JPEG does (FF D8 FF).
bool is_jpeg(const std::vector<uint8_t>& b);

// The JPEG's brunsli form, where brunsli reads it and gives it back byte for byte; else nothing.
std::optional<std::vector<uint8_t>> jpeg_to_brunsli(const std::vector<uint8_t>& jpeg);

// The JPEG again from its brunsli form; refused where the form does not read.
std::vector<uint8_t> brunsli_to_jpeg(const std::vector<uint8_t>& form);

// The same JPEG with Huffman tables made for its own values (ITU T.81 Annex K.2, as `jpegtran
// -optimize` makes them): the same coefficients, so the same pixels, in fewer bytes. Only for a
// sequential JPEG whose tables are each defined once, before its first scan; given only where it
// reads back to the same coefficients and quantisation tables and is smaller; else nothing. (v32 in
// the making: what Compressed Formats' permanent box installs for a JPEG.)
std::optional<std::vector<uint8_t>> jpeg_optimised(const std::vector<uint8_t>& jpeg);

} // namespace sieve::cli
