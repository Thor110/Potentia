// Sieve — whether Windows finds and runs sieve-thumbs.dll for a file (sieve-view --thumbnail-check).
#pragma once

#include <filesystem>

namespace sieve::view {

// Each step Explorer takes to give a file its thumbnail, said as it is taken: the handler named
// for the file's extension, the DLL that class names, that DLL loaded and asked for the thumbnail
// directly, and then the thumbnail asked of Windows' shell as Explorer asks it (which may come from
// its cache). Returns 0 when the DLL gave a thumbnail, 1 otherwise; elsewhere than Windows, says
// there is nothing to check and returns 1.
int thumbnail_check(const std::filesystem::path& file);

} // namespace sieve::view
