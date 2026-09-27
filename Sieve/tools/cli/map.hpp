// Sieve CLI — maps: node graphs of verified anchors (SPECIFICATIONS §12.3).
//
// A map links real things in the library to one another. Its nodes are files and folders: a file
// is named by its size and SHA-256, so a map says exactly which bytes it means, and those bytes are
// one unit of the binary line (§12.1). Its edges say how two nodes are related: `contains`, from a
// folder to what is directly in it, and `anchor`, from a map's root to an anchor added to it. A
// relation is any lower-case word, so later relations (a newer version, a patch, anything a tool
// or a person records) are written the same way without changing the format.
//
// A file node either points at a file (`file`: found at its path, relative to the map's root) or
// holds it (`held`: its bytes are in the map, after the text, as Sieve instructions hold theirs).
// A map of a folder points; anchors added in the hallway are held, so a map carries what it needs
// to walk back to them, and can be sent as one file. Where a pointed-at file is on this computer
// and matches, or a held one's bytes match (they are checked when the map is read), the node is a
// *verified anchor*.
//
// Two versions, both canonical (the same content always gives the same bytes):
//
//     sieve-map-v1                            sieve-map-v2
//     name <the map's name>                   name <name>
//     root <its root folder's name>           root <root>
//     nodes <N>                               sealed yes|no
//     edges <E>                               nodes <N>
//     n	<id>	folder	<path>/                  edges <E>
//     n	<id>	file	<size>	<sha256>	<path>  meta <M>
//     e	<from>	<to>	<relation>              n ... (as v1, and:)
//     end                                     n	<id>	held	<size>	<sha256>	<name>
//                                             e ... (as v1)
//                                             m	<id>	<key>	<value>
//                                             end
//                                             <every held node's bytes, in node order>
//
// Ids count up from 0; node 0 is the root folder, "./". Edges are sorted by from, to, relation;
// metadata by node, then key. `sealed yes` asks every tool not to change the map (the release map
// is sealed; its published SHA-256 is what shows a copy has been altered). Metadata is a node's
// extra facts as key and value, a key being a lower-case word; none are written yet, and the
// format has room for them. A map is written as v1 when it uses nothing of v2's (no held node, no
// metadata, not sealed), so a map of a folder is v1, and the oracle writes the same one.
//
// A map can be written for other programs too: Graphviz's DOT and GraphML.
#pragma once

#include "cli/locate.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace sieve::cli {

inline constexpr const char* kMapVersion = "sieve-map-v1";
inline constexpr const char* kMapVersion2 = "sieve-map-v2";

struct MapNode
{
    enum Kind { Folder, File, Held } kind = File;
    std::string path;    // a folder's ends in '/', the root is "./"; a held node's is its name
    uint64_t size = 0;   // files
    std::string sha256;  // files
    std::vector<uint8_t> bytes; // held: the file itself
    bool folder() const { return kind == Folder; }
    std::string label() const; // the last part of the path
};

struct MapEdge
{
    uint32_t from = 0, to = 0;
    std::string relation; // a lower-case word
};

struct MapMeta
{
    uint32_t node = 0;
    std::string key, value;
};

struct Map
{
    std::string name, root;
    bool sealed = false;
    std::vector<MapNode> nodes;
    std::vector<MapEdge> edges;
    std::vector<MapMeta> meta;
    bool needs_v2() const;
    std::string text() const;                    // the canonical text (for v2, without the held bytes)
    std::vector<uint8_t> file() const;           // the whole map as a file: the text, then the held bytes
    static Map parse(std::string_view whole);    // v1 or v2; held bytes checked against their SHA-256
    std::string dot() const;                     // Graphviz
    std::string graphml() const;                 // GraphML
    std::string label(size_t node) const;        // the root's is the root's name
    // An empty map: only its root.
    static Map empty(const std::string& name);
    // The node whose file has this SHA-256, or -1.
    int find(const std::string& sha256) const;
    // Adds a file as a held anchor, linked from the root ("anchor"); returns its node. Refused if
    // the map is sealed, the name cannot be a file name, or the map already has these bytes.
    uint32_t add_held(const std::string& name, const std::vector<uint8_t>& bytes);
    // Removes a node (never the root), its edges and its metadata; later nodes move down one.
    void remove(uint32_t node);
    // Sets a node's metadata: key (a lower-case word) to value (no tabs or line breaks); an empty
    // value removes the key.
    void set_meta(uint32_t node, const std::string& key, const std::string& value);
};

// A folder's map: its manifest (walk_folder) with every folder linked to what is directly in it.
Map map_of_manifest(const Manifest& m, const std::string& name);
Map map_of_folder(const std::filesystem::path& folder, const std::string& name);
// A map of some files, found beside the map (a release's archives, say): the root, and each file.
Map map_of_files(const std::vector<std::filesystem::path>& files, const std::string& name);

// Where a map's pointed-at node is on this computer, given the folder the map's root is.
std::filesystem::path map_node_path(const std::filesystem::path& root, const MapNode& n);

// A name as a held node may have it: one file name, no separators, tabs or line breaks.
bool is_held_name(const std::string& name);

} // namespace sieve::cli
