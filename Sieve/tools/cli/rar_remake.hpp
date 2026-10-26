// Sieve CLI -- a RAR made again by the machine's own `rar` program (sieve-manifest-v22).
//
// No free RAR compressor exists, and unRAR's licence forbids writing one, so the only way to give
// a RAR back exactly is the `rar` that made it, run as ffmpeg is (as its own process; Sieve links
// none of it). The same version of `rar`, with the same switches, given the same files with the
// same times (to the nanosecond) and attributes in the same order, writes the same bytes. So the
// packer (Optimise Archive Formats) reads a RAR 5 archive's headers for its switches and every
// member's times and attributes, writes its files out so, has the `rar` found make the archive
// again, and carries it as its contents with that recipe only where the result is byte for byte
// the archive. Installing writes the contents, then, where the same `rar` is found, makes the
// archive again, checks it against its SHA-256, and puts it in their place; where no `rar` is
// found, or another version, or it does not come out the same, the files are left in their folder
// and a note says so (Edward, 25 October 2026).
//
// Which rar: SIEVE_RAR, else a rar beside Sieve's own program, else the first on the PATH, else
// (Windows) WinRAR's own folder in Program Files.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace sieve::cli::rar {

// One entry of a RAR 5 archive, as its header holds it.
struct Member
{
    std::string path;       // '/' between parts, as the archive holds it
    bool dir = false;
    uint64_t attributes = 0; // the host's: Windows attributes, or a Unix mode
    std::optional<int64_t> mtime, ctime, atime; // nanoseconds since 1970 (UTC)
};

// What a RAR 5 archive's headers say of how it was made.
struct Archive
{
    std::vector<Member> members; // in the archive's order
    int host = -1;               // 0 Windows, 1 Unix (every member's)
    std::string switches;        // the switches they give: "-m3 -md4096k -s" ...
    std::vector<std::string> alternatives; // other switches to try, if those do not give it back
    std::string refused;         // why it cannot be made again (encrypted, a volume, links ...), or empty
};

// Nothing where it is not a RAR 5 archive (a RAR 4 one, say), or its headers are damaged.
std::optional<Archive> read_headers(const std::vector<uint8_t>& archive);

// The rar program, or nothing; and its version, as the first words of its banner ("RAR 7.01 x64").
std::optional<std::filesystem::path> find_program();
std::string program_version(const std::filesystem::path& program);
// Where rar was looked for, to say so when none is found.
std::string looked();

// Sets each member's attributes and times on the files (and folders) under `folder`: the last thing
// done before rar is run on them, so writing nothing after changes them. False, with why, where a
// time cannot be set here (a creation time on a Unix system).
bool set_times(const std::filesystem::path& folder, const std::vector<Member>& members, std::string& why);

// rar run in `folder` on the members, making `out`: how ("top", the top names with -r; "list", every
// member named in a list file) and with which switches. Its exit status; what it printed in `said`.
int make(const std::filesystem::path& program, const std::filesystem::path& folder, const std::vector<Member>& members,
         const std::string& how, const std::string& switches, const std::filesystem::path& out, std::string* said = nullptr);

} // namespace sieve::cli::rar
