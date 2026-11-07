// Several files or folders chosen at once in Explorer, gathered into one run (6 November 2026):
// Explorer starts a program for each, so the first gathers the others' paths (gather.cpp).
#pragma once

#include <optional>
#include <string>
#include <vector>

namespace sieve::cli
{

// Every path chosen with `what` (its own first), where this program is the first of them to ask;
// nothing where another is, which was handed this one's path (and this program has nothing to do).
// Only paths in the same folder as `what` are taken.
// `kind`: the command and its format ("create", "compress zip" ...), so that two commands run on one
// folder at once are gathered apart.
std::optional<std::vector<std::string>> gather_chosen(const std::string& what, const std::string& kind);

} // namespace sieve::cli
