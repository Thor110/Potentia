// Sieve CLI — maps: node graphs of verified anchors (SPECIFICATIONS §12.3).
//
// A map links real things in the library to one another. Its nodes are files and folders: a file
// is named by its size and SHA-256, so a map says exactly which bytes it means, and those bytes are
// one unit of the binary line (§12.1). Its edges say how two nodes are related. A map built from a
// folder has one relation, `contains`, from each folder to what is directly in it; the relation is
// a lower-case word, so other relations (a newer version, a patch, anything a later tool or person
// records) are written the same way without changing the format.
//
// A map never holds the bytes it points at: a node is a claim that a file with this size and hash
// exists, and where it was found, relative to the map's root. Walking to it needs the file itself,
// because a file's address is the file (addressing is not compression); where the file is on this
// computer and matches, the node is a *verified anchor*. So a map is small, can be handed round and
// bundled with a release, and says, of whatever it names, exactly which bytes are meant.
//
// The text is canonical, like a manifest's (locate.hpp): the same folder always gives the same
// map, so a map has an identity and a place on the binary line of its own.
//
//     sieve-map-v1
//     name <the map's name>
//     root <the folder it was made from: its name>
//     nodes <how many>
//     edges <how many>
//     n	<id>	folder	<path>/                 the root is "./"; ids count up from 0 in order
//     n	<id>	file	<size>	<sha256>	<path>
//     e	<from>	<to>	<relation>              sorted by from, then to, then relation
//     end
//
// Paths are UTF-8 with '/' between their parts, relative to the root, as in a manifest; nodes come
// in the manifest's order (the root first, then by the paths' bytes), so a map from a folder is its
// v1 manifest's entries with the structure made explicit. A map can be written for other programs
// too: Graphviz's DOT and GraphML (Gephi, yEd, Cytoscape and most graph tools read one or other).
#pragma once

#include "cli/locate.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::cli {

inline constexpr const char* kMapVersion = "sieve-map-v1";

struct MapNode
{
    bool folder = false;
    std::string path;    // relative to the root, '/' between parts; a folder's ends in '/'; the root is "./"
    uint64_t size = 0;   // files
    std::string sha256;  // files
    std::string label() const; // the last part of the path (the root: the map's root name)
};

struct MapEdge
{
    uint32_t from = 0, to = 0;
    std::string relation; // a lower-case word: "contains" for a folder's contents
};

struct Map
{
    std::string name, root;
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    std::string text() const;                    // the canonical form above
    static Map parse(std::string_view text);     // throws if it is not one
    std::string dot() const;                     // Graphviz
    std::string graphml() const;                 // GraphML
    std::string label(size_t node) const;        // the root's is the root's name
};

// A folder's map: its manifest (walk_folder) with every folder linked to what is directly in it.
Map map_of_manifest(const Manifest& m, const std::string& name);
Map map_of_folder(const std::filesystem::path& folder, const std::string& name);

// Where a map's node is on this computer, given the folder the map's root is: `root / path`.
std::filesystem::path map_node_path(const std::filesystem::path& root, const MapNode& n);

} // namespace sieve::cli
