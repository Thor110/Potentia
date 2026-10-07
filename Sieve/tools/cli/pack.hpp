// Sieve CLI -- an installer's files packed (sieve-manifest-v4, locate.hpp).
//
// Each file's way is chosen, then the two streams are compressed:
//
//   - a program (its kind, from its first bytes, EXE or ELF) goes in the x86 stream: the x86
//     branch filter turns its calls' relative addresses into absolute ones, which repeat, and LZMA2
//     then finds them;
//   - a text whose lines are some of another file's lines, in its order, is derived from it: a mask
//     of that file's lines, one bit each, where the mask packs smaller than the text (each SCOWL
//     list is a mask over the largest). The other file must be carried as it is, and one file
//     derived from is never itself derived;
//   - everything else goes in the lzma2 stream as it is.
//
// Both streams are LZMA2 at its strongest (preset 9, extreme), with a dictionary no larger than the
// stream needs, so unpacking takes no more memory than it must. What is made is read back (unpacked
// and every file compared) before it is used, so a packer fault can never make an installer that
// gives back something else.
#pragma once

#include "cli/locate.hpp"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace sieve::cli {

struct PackReport
{
    bool kept_v3 = false;                            // packing would not have made it smaller: v3 instead
    size_t x86 = 0, raw = 0, derived = 0;            // files carried each way
    uint64_t x86_bytes = 0, raw_bytes = 0, derived_bytes = 0, mask_bytes = 0;
    uint64_t packed = 0;                             // the two streams as carried
    std::vector<std::pair<std::string, std::string>> derivations; // a derived file, and the file it is made from
};

// Every file's bytes packed after the manifest, making it v4, an installer's. Reads every file again
// (or takes them from a v3 manifest's contents, if it has them) and checks it against the size and
// SHA-256 the walk found, so a file changed in between is refused rather than installed wrong.
// Where packing would not make the installer smaller (a few bytes of text, files already
// compressed), the files are carried as they are instead (v3), so an installer is never larger
// than it need be.
void add_packed(Manifest& m, const std::filesystem::path& root, PackReport* report = nullptr);

// The report as lines, for the tool and the File Locator.
std::string pack_report_text(const PackReport& r, uint64_t original);

} // namespace sieve::cli
