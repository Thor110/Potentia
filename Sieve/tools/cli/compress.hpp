// Sieve CLI — sieve compress: a file, a folder or several files made into one of the formats people
// already use (gzip, xz, lzma, bzip2, zstd, lz4, brotli, zip, 7z, tar), at each one's strongest usual
// setting, and read back before it is kept. The right-click menu's "Compress to" (IDEAS §19.100).
//
// The stream formats (all but zip, 7z and tar) hold one stream: a single file goes in as it is, and a
// folder or several files go in as a tar first (NAME.tar.xz), as tar and the compressor would make
// them. zip compresses each file on its own (deflate 9); 7z all of them as one stream (LZMA2 9e,
// solid), as 7-Zip does by default. The settings are those the comparison table measures
// (compare.hpp), so what it says a format would make is what this makes.
#pragma once

#include "cli/args.hpp"

#include <cstdint>
#include <filesystem>
#include <map>
#include <span>
#include <string>
#include <vector>

namespace sieve::cli {

// One thing to go in: a file's bytes, or a folder (made empty if nothing else is under it).
struct CompressEntry
{
    std::string name; // relative, '/' between parts (UTF-8)
    bool dir = false;
    std::vector<uint8_t> bytes;
    int64_t mtime = 0; // seconds since 1970 (UTC)
};

// The formats, by the names `--format` takes: gz xz lzma bz2 zst lz4 br zip 7z tar.
const std::vector<std::string>& compress_formats();
bool is_stream_format(const std::string& format);

// The entries made into `format` (a stream format: one file's bytes given alone, or a tar of them),
// and read back: it throws if what was made does not give back what went in.
std::vector<uint8_t> compress_entries(const std::vector<CompressEntry>& entries, const std::string& format);

// Each stream format's own compressing and undoing, at the settings above.
std::vector<uint8_t> compress_stream(std::span<const uint8_t> data, const std::string& format);
std::vector<uint8_t> decompress_stream(std::span<const uint8_t> data, const std::string& format);

// What a tar or zip holds, read back by libarchive: name (folders ending '/') to bytes. Throws
// where it does not read.
std::map<std::string, std::vector<uint8_t>> libarchive_read(std::span<const uint8_t> data, bool zip);

// sieve compress PATH... --format F [--out FILE] [--force] [--gather] [--from-menu]
int cmd_compress(const Args& a);

} // namespace sieve::cli
