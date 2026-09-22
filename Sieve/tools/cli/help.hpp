// Sieve CLI — help pages.
#pragma once

#include <string>
#include <vector>

namespace sieve::cli {

void print_usage();
// Prints the help page for a command or topic ("lines"). Returns false if there is none.
bool print_help(const std::string& name);
// The options a command's help page mentions (--name, without the dashes), plus the line
// options every command shares. Empty if there is no page for it.
std::vector<std::string> documented_options(const std::string& command);

} // namespace sieve::cli
