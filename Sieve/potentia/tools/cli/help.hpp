// Potentia CLI — help pages.
#pragma once

#include <string>

namespace potentia::cli {

void print_usage();
// Prints the help page for a command or topic ("lines"). Returns false if there is none.
bool print_help(const std::string& name);

} // namespace potentia::cli
