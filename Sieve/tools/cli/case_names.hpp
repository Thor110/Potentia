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
// folded for A to Z only; letters beyond ASCII are compared as they are.
std::optional<std::pair<std::string, std::string>> names_one_ignoring_case(const std::vector<std::string>& files);

// Whether names written under `dest` ignore case. It is looked at, writing nothing: the nearest
// folder at or above `dest` that exists and has a letter in its name is asked for by that name with
// its letters' case turned, and the two are one where case is ignored. So it holds for a Windows
// drive seen from Linux (WSL's /mnt/c), and for a Mac folder made to heed case. Where no such
// folder is found, this system's default: Windows and macOS ignore case, others heed it.
bool folders_ignore_case(const std::filesystem::path& dest);

// Throws, naming the pair, where `files` has two names one where case is ignored and names under
// `dest` ignore case. (The pair is looked for first: `dest` is looked at only for such a pair.)
void refuse_names_one_ignoring_case(const std::vector<std::string>& files, const std::filesystem::path& dest);

} // namespace sieve::cli
