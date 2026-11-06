// Sieve — names that are one name where case is ignored.
//
// A folder on Windows, and on macOS by default, takes `A.txt` and `a.txt` as one name. A `.sieve`
// or an archive made on Linux can hold both, and installing it there would write the second over
// the first: every installer checks that no file is already there before writing, and neither is.
// So each writer asks this first, and refuses, naming the pair, where names ignore case.
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace sieve::cli
{

// The first two of `files` (paths with '/' between their parts) that are one name where case is
// ignored, or a file that is one name with a folder another file is in (`a` and `A/b`). Case is
// folded as Windows folds it, each letter Unicode gives a case to one other (case_fold_table.hpp:
// É and é one, Σ σ ς one; ß stays ß). Written two ways (é as one character and as e and an accent),
// they are two here, as on Windows: names_one_on_macos below takes them as one.
std::optional<std::pair<std::string, std::string>> names_one_ignoring_case(const std::vector<std::string>& files);
// The same as macOS compares names: also each letter as one character and as a letter and its
// combining accents taken as one (Unicode's canonical decomposition, normalise_table.hpp), which
// Windows keeps as two (é written either way). The packer warns of such a pair as macOS's alone.
std::optional<std::pair<std::string, std::string>> names_one_on_macos(const std::vector<std::string>& files);

// Whether names written under `dest` ignore case. It is looked at, writing nothing: the nearest
// folder at or above `dest` that exists and has a letter in its name is asked for by that name with
// its letters' case turned, and the two are one where case is ignored. So it holds for a Windows
// drive seen from Linux (WSL's /mnt/c), and for a Mac folder made to heed case. Where no such
// folder is found, this system's default: Windows and macOS ignore case, others heed it.
bool folders_ignore_case(const std::filesystem::path& dest);

// Throws, naming the pair, where `files` has two names one where case is ignored and names under
// `dest` ignore case. (The pair is looked for first: `dest` is looked at only for such a pair.)
void refuse_names_one_ignoring_case(const std::vector<std::string>& files, const std::filesystem::path& dest);

// Names Windows cannot hold (0.13.78, IDEAS §19.120): a part of a path that is one of the devices'
// names (CON, PRN, AUX, NUL, COM0 to COM9, LPT0 to LPT9, in any case, with or without an
// extension: aux.txt is AUX), that ends in a dot or a space (Windows takes them off, so `a.` is
// `a`), or that holds a character Windows keeps out of names (a control character, or one of
// < > : " / \ | ? *; a colon writes a stream of the file before it, unseen). Writing such a name
// on Windows writes another file, or a device, while the install says it succeeded. The first of
// `files` (paths with '/' between their parts) with such a part, and why; nullopt where none has.
std::optional<std::pair<std::string, std::string>> name_windows_cannot_hold(const std::vector<std::string>& files);

// Throws, naming the file and why, where `files` has a name Windows cannot hold, on Windows; on
// other systems such names are names, and nothing is refused (the packer warns of them instead).
void refuse_names_windows_cannot_hold(const std::vector<std::string>& files);

} // namespace sieve::cli
