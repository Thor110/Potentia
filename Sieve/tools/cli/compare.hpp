// Sieve CLI — the comparison: a file's size against what the compressors make of it, and what
// its address costs. The COST page for files and folders (SPECIFICATIONS §12.2).
//
// Addressing is not compression (HANDOFF, standing rules): a file's place on the binary line is
// its hex dump plus 0101...01, so as a number it is as long as the file, to a byte, and written in
// hex it is twice as long. This puts that beside the two compressors people use, so the difference
// is measured rather than asserted:
//
//   deflate  what zip does: zlib at its strongest (level 9), each file on its own, as a zip
//            archive stores its files. Raw deflate, without the zip container's few dozen bytes
//            of header per file.
//   LZMA2    what 7z does: liblzma at preset 9 extreme, raw LZMA2, and for a folder all of its
//            files as one stream, as a 7z archive is "solid" by default. Without the container.
//
// Each figure is also given as a share of the original: under 100% is smaller. Each is an upper
// bound on how far the file can be compressed, never the limit itself: that limit (its Kolmogorov
// complexity) is not computable by any program (IDEAS §1).
#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace sieve::cli {

uint64_t deflate_size(std::span<const uint8_t> data);
uint64_t lzma2_size(std::span<const uint8_t> data);

struct Comparison
{
    uint64_t original = 0;       // bytes
    uint64_t deflate = 0;        // each file compressed on its own, added up
    uint64_t lzma2 = 0;          // everything as one stream
    uint64_t address_bytes = 0;  // the addresses as numbers, in bytes, added up
    uint64_t address_hex = 0;    // the addresses written in hex, in characters, added up
    uint64_t manifest = 0;       // a folder: the manifest's own size (0 for one file); with an installer, its v3 manifest's
    uint64_t manifest_deflate = 0, manifest_lzma2 = 0; // an installer's manifest, compressed
    uint64_t installer_hex = 0;  // the installer: the manifest's address, in hex
    uint64_t installer_raw = 0;  // and as raw bytes, as sieve-install reads it
    bool packed = false;         // the installer's manifest is v4, its files packed
};

// The table, one row a measure: its size, and as a share of the original.
std::string comparison_table(const Comparison& c);

} // namespace sieve::cli
