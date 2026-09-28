// Sieve — finding and loading filter plugins (sieve/plugin.hpp, docs/FILTER-PLUGINS.md).
//
// Every *.sfilter file in the `filters` folder of the installation (beside the programs, or the
// folder above an installation's tools\), and in data/filters when run from the repository, is
// read once, at start-up, and registered. A file that does not load is listed with the reason,
// never skipped silently: `sieve filters --plugins` and the setup menu show the list. Two files
// with the same name and version but different bytes are both refused (a version is never
// edited), as is a plugin that takes a built-in filter's id.
#pragma once

#include <string>
#include <vector>

namespace sieve::cli {

struct PluginFile
{
    std::string path, name, sha256, error; // error: empty when it loaded
};

// Loads and registers the plugins (the first call does the work; later calls return the list).
const std::vector<PluginFile>& load_plugins();

} // namespace sieve::cli
