// The installer's 7z unpacking: when what an installer carries is one 7z archive, the installer
// unpacks it rather than writing the archive itself (docs/SIEVE-INSTALL-USAGE.md).
//
// A release is compressed first, as one solid 7z (the proper procedure: an installer is never
// smaller than what it carries), so its installer carries one file, sieve.7z. Written as it is,
// that leaves the person a 7z to unpack by hand; so sieve-install, and only sieve-install,
// recognises a 7z by its signature and unpacks it into a folder named after the file
// (sieve.7z -> sieve\). When everything in the archive is inside one top folder (a release's
// Sieve-<version>\), that folder's contents go straight into it, so the result is C:\TEST\sieve\...
// and not C:\TEST\sieve\Sieve-0.13.1\.... The locator (sieve install, the File Locator's installs)
// is left byte for byte: it gives back exactly the file that was located.
//
// The archive's own SHA-256 is checked before this is reached (the manifest names it), and 7z
// checks every file's CRC as it decodes. The decoder is the LZMA SDK's (third_party/lzma, public
// domain): LZMA, LZMA2, PPMd and the BCJ, BCJ2, ARM and Delta filters, which is everything 7-Zip
// writes by default; an encrypted archive, or one in another method, is refused before anything is
// written.
#pragma once

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace sieve::install {

// A 7z archive's first six bytes: '7', 'z', BC AF 27 1C.
bool is_7z(const std::vector<uint8_t>& bytes);

struct Unpacked
{
    std::string path; // relative, '/' between parts, the common top folder (if any) removed
    bool dir = false;
    uint64_t size = 0;
};

struct SevenZip
{
    std::vector<Unpacked> entries; // in the archive's order
    uint64_t files = 0, bytes = 0;
    std::string top;               // the one top folder every entry was in, removed from the paths, or ""
};

// Reads an archive's listing (not its contents). Throws if it is not a 7z this can unpack, or if
// any name in it could reach outside the folder it unpacks into (absolute, a drive, or a ".." part).
SevenZip list_7z(const std::vector<uint8_t>& archive);

// Unpacks into `dest` (made if need be). Files already there are refused unless `force`.
// `progress(done, total, path)` is told of each file as it is written. If `cancel` becomes true, or
// anything fails, what was written so far is removed again, the folders made included; a cancel
// returns false, a failure throws.
bool unpack_7z(const std::vector<uint8_t>& archive, const std::filesystem::path& dest, bool force,
               const std::function<void(size_t done, size_t total, const std::string& path)>& progress,
               const std::atomic<bool>* cancel = nullptr);

} // namespace sieve::install
