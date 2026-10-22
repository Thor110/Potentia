// Sieve CLI — maps as Sieve instructions (sieve-manifest-v18, SPECIFICATIONS §12.3).
//
// A map is written as Sieve instructions: its held files are the instructions' files, packed as
// any are and put back by installing, and its text (nodes, edges, metadata, the seal) is their
// graph part. Any instructions read as a map: by their graph part where they have one, else as a
// map of the tree they carry. The older map files (sieve-map-v1, v2) are still read; nothing
// writes them any more (IDEAS §19.93).
#pragma once

#include "cli/locate.hpp"
#include "cli/map.hpp"

#include <filesystem>

namespace sieve::cli {

// The map these instructions are: their graph part, its held nodes' bytes taken from their files
// (each checked against its SHA-256), or, without one, a map of the tree they carry named `name`.
Map map_of_instructions(const Manifest& m, const std::string& name);

// A map from a file: Sieve instructions (any), or an older map file (sieve-map-v1, v2). `is_map`, if
// given, is told whether the file is a map (an older map file, or instructions with a graph part)
// rather than instructions read as the map of their tree.
Map read_map_file(const std::filesystem::path& file, bool* is_map = nullptr);

// Whether a file is an older map file (it begins "sieve-map-v").
bool is_old_map_file(const std::filesystem::path& file);

// Writes a map as Sieve instructions (v18) to `out`: its held files packed (the newest version, as
// `sieve locate --items` packs, the rope's search unlimited), its text the graph part.
void write_map_instructions(const Map& map, const std::filesystem::path& out);

} // namespace sieve::cli
