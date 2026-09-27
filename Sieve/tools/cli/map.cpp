// Maps (map.hpp): built from a folder's manifest or from some files, written and read in one
// canonical text (v1, or v2 with held anchors, a seal and metadata), changed an anchor at a time,
// and written for other programs as DOT and GraphML.

#include "cli/map.hpp"

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

namespace {

bool less_bytes(const std::string& a, const std::string& b)
{
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](char x, char y) { return uint8_t(x) < uint8_t(y); });
}

std::vector<std::string> split_tabs(std::string_view line)
{
    std::vector<std::string> f;
    for (size_t at = 0;;)
    {
        const size_t tab = line.find('\t', at);
        f.emplace_back(line.substr(at, tab == std::string_view::npos ? std::string_view::npos : tab - at));
        if (tab == std::string_view::npos) break;
        at = tab + 1;
    }
    return f;
}

bool is_word(const std::string& r)
{
    if (r.empty()) return false;
    for (char c : r)
        if (!((c >= 'a' && c <= 'z') || c == '-')) return false;
    return true;
}

bool is_hex64(const std::string& s)
{
    if (s.size() != 64) return false;
    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

// Paths are read by the viewer, never written, but a map from somewhere else is still kept inside
// its root: no absolute path, drive, backslash, "." or ".." part.
bool safe_path(const std::string& p)
{
    if (p == "./") return true;
    if (p.empty() || p[0] == '/' || p.find(':') != std::string::npos || p.find('\\') != std::string::npos) return false;
    const std::string body = p.back() == '/' ? p.substr(0, p.size() - 1) : p;
    for (size_t at = 0; at <= body.size();)
    {
        size_t slash = body.find('/', at);
        if (slash == std::string::npos) slash = body.size();
        const std::string part = body.substr(at, slash - at);
        if (part.empty() || part == "." || part == "..") return false;
        at = slash + 1;
    }
    return true;
}

std::string xml_escape(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '&') o += "&amp;";
        else if (c == '<') o += "&lt;";
        else if (c == '>') o += "&gt;";
        else if (c == '"') o += "&quot;";
        else o += c;
    }
    return o;
}

std::string dot_escape(const std::string& s)
{
    std::string o;
    for (char c : s)
    {
        if (c == '"' || c == '\\') o += '\\';
        o += c;
    }
    return o;
}

void sort_edges(std::vector<MapEdge>& edges)
{
    std::sort(edges.begin(), edges.end(), [](const MapEdge& a, const MapEdge& b) {
        if (a.from != b.from) return a.from < b.from;
        if (a.to != b.to) return a.to < b.to;
        return less_bytes(a.relation, b.relation);
    });
}

const char* kind_word(MapNode::Kind k) { return k == MapNode::Folder ? "folder" : k == MapNode::File ? "file" : "held"; }

} // namespace

bool is_held_name(const std::string& name)
{
    return !name.empty() && name != "." && name != ".." && name.find_first_of("/\\:\t\r\n") == std::string::npos;
}

std::string MapNode::label() const
{
    std::string p = folder() && !path.empty() ? path.substr(0, path.size() - 1) : path;
    const size_t slash = p.rfind('/');
    return slash == std::string::npos ? p : p.substr(slash + 1);
}

std::string Map::label(size_t node) const { return nodes[node].path == "./" ? root : nodes[node].label(); }

bool Map::needs_v2() const
{
    if (sealed || !meta.empty()) return true;
    for (const MapNode& n : nodes)
        if (n.kind == MapNode::Held) return true;
    return false;
}

std::string Map::text() const
{
    const bool v2 = needs_v2();
    std::string t = std::string(v2 ? kMapVersion2 : kMapVersion) + "\n";
    t += "name " + name + "\n";
    t += "root " + root + "\n";
    if (v2) t += std::string("sealed ") + (sealed ? "yes" : "no") + "\n";
    t += "nodes " + std::to_string(nodes.size()) + "\n";
    t += "edges " + std::to_string(edges.size()) + "\n";
    if (v2) t += "meta " + std::to_string(meta.size()) + "\n";
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const MapNode& n = nodes[i];
        if (n.folder()) t += "n\t" + std::to_string(i) + "\tfolder\t" + n.path + "\n";
        else t += "n\t" + std::to_string(i) + "\t" + kind_word(n.kind) + "\t" + std::to_string(n.size) + "\t" + n.sha256 + "\t" + n.path + "\n";
    }
    for (const MapEdge& e : edges) t += "e\t" + std::to_string(e.from) + "\t" + std::to_string(e.to) + "\t" + e.relation + "\n";
    for (const MapMeta& m : meta) t += "m\t" + std::to_string(m.node) + "\t" + m.key + "\t" + m.value + "\n";
    t += "end\n";
    return t;
}

std::vector<uint8_t> Map::file() const
{
    const std::string t = text();
    std::vector<uint8_t> out(t.begin(), t.end());
    for (const MapNode& n : nodes)
        if (n.kind == MapNode::Held) out.insert(out.end(), n.bytes.begin(), n.bytes.end());
    return out;
}

Map Map::parse(std::string_view whole)
{
    // The text, line by line up to and including "end"; after it, a v2 map's held bytes.
    std::vector<std::string_view> lines;
    size_t after = std::string_view::npos;
    for (size_t at = 0; at < whole.size();)
    {
        const size_t nl = whole.find('\n', at);
        if (nl == std::string_view::npos) throw std::runtime_error("map: the last line has no line feed");
        lines.push_back(whole.substr(at, nl - at));
        at = nl + 1;
        if (lines.back() == "end")
        {
            after = at;
            break;
        }
    }
    if (after == std::string_view::npos) throw std::runtime_error("map: no 'end'");
    if (lines.empty() || (lines[0] != kMapVersion && lines[0] != kMapVersion2)) throw std::runtime_error("not a sieve map");
    const bool v2 = lines[0] == kMapVersion2;
    size_t li = 1;
    auto field = [&](const char* key) {
        if (li >= lines.size()) throw std::runtime_error("map: too short");
        const std::string_view line = lines[li++];
        const std::string k = std::string(key) + " ";
        if (line.substr(0, k.size()) != k) throw std::runtime_error(std::string("map: expected '") + key + "'");
        return std::string(line.substr(k.size()));
    };
    Map m;
    m.name = field("name");
    m.root = field("root");
    if (v2)
    {
        const std::string s = field("sealed");
        if (s != "yes" && s != "no") throw std::runtime_error("map: 'sealed' is yes or no");
        m.sealed = s == "yes";
    }
    const uint64_t n = std::stoull(field("nodes")), e = std::stoull(field("edges")), mc = v2 ? std::stoull(field("meta")) : 0;
    if (lines.size() != li + n + e + mc + 1) throw std::runtime_error("map: its counts do not match its lines");
    for (size_t i = 0; i < n; ++i)
    {
        const auto f = split_tabs(lines[li + i]);
        const std::string where = "map: line " + std::to_string(li + i + 1);
        MapNode node;
        if (f.size() == 4 && f[0] == "n" && f[2] == "folder")
        {
            node.kind = MapNode::Folder;
            node.path = f[3];
            if (!safe_path(node.path) || node.path.back() != '/') throw std::runtime_error("map: refused path " + node.path);
        }
        else if (f.size() == 6 && f[0] == "n" && (f[2] == "file" || (v2 && f[2] == "held")))
        {
            node.kind = f[2] == "file" ? MapNode::File : MapNode::Held;
            node.size = std::stoull(f[3]);
            node.sha256 = f[4];
            node.path = f[5];
            if (!is_hex64(node.sha256)) throw std::runtime_error(where + " has no SHA-256");
            if (node.kind == MapNode::File ? (!safe_path(node.path) || node.path.back() == '/') : !is_held_name(node.path))
                throw std::runtime_error("map: refused path " + node.path);
        }
        else throw std::runtime_error(where + " is not a node");
        if (f[1] != std::to_string(i)) throw std::runtime_error("map: node ids must count up from 0");
        m.nodes.push_back(std::move(node));
    }
    li += n;
    for (size_t i = 0; i < e; ++i)
    {
        const auto f = split_tabs(lines[li + i]);
        if (f.size() != 4 || f[0] != "e" || !is_word(f[3])) throw std::runtime_error("map: line " + std::to_string(li + i + 1) + " is not an edge");
        MapEdge edge{uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), f[3]};
        if (edge.from >= n || edge.to >= n) throw std::runtime_error("map: an edge names a node that is not there");
        m.edges.push_back(std::move(edge));
    }
    li += e;
    for (size_t i = 0; i < mc; ++i)
    {
        const auto f = split_tabs(lines[li + i]);
        if (f.size() != 4 || f[0] != "m" || !is_word(f[2])) throw std::runtime_error("map: line " + std::to_string(li + i + 1) + " is not metadata");
        MapMeta md{uint32_t(std::stoul(f[1])), f[2], f[3]};
        if (md.node >= n) throw std::runtime_error("map: metadata names a node that is not there");
        m.meta.push_back(std::move(md));
    }
    // The held bytes: every held node's, in node order, exactly filling what is left, and each
    // checked against its SHA-256, so a held anchor is verified as it is read.
    size_t at = after;
    for (MapNode& node : m.nodes)
    {
        if (node.kind != MapNode::Held) continue;
        if (whole.size() - at < node.size) throw std::runtime_error("map: it ends before " + node.path + "'s bytes");
        node.bytes.assign(whole.begin() + std::ptrdiff_t(at), whole.begin() + std::ptrdiff_t(at + node.size));
        at += size_t(node.size);
        if (sha256_hex(node.bytes) != node.sha256) throw std::runtime_error("map: " + node.path + "'s bytes do not match its SHA-256");
    }
    if (at != whole.size()) throw std::runtime_error(v2 ? "map: bytes after the held anchors" : "map: something after 'end'");
    return m;
}

Map Map::empty(const std::string& name)
{
    Map m;
    m.name = name;
    m.root = name;
    MapNode root;
    root.kind = MapNode::Folder;
    root.path = "./";
    m.nodes.push_back(root);
    return m;
}

int Map::find(const std::string& sha256) const
{
    for (size_t i = 0; i < nodes.size(); ++i)
        if (!nodes[i].folder() && nodes[i].sha256 == sha256) return int(i);
    return -1;
}

uint32_t Map::add_held(const std::string& label, const std::vector<uint8_t>& bytes)
{
    if (sealed) throw std::runtime_error("the map " + name + " is sealed: it is not changed");
    if (!is_held_name(label)) throw std::runtime_error("an anchor's name cannot hold / \\ : or line breaks: " + label);
    const std::string sha = sha256_hex(bytes);
    if (const int at = find(sha); at >= 0) throw std::runtime_error("the map already has these bytes, as node " + std::to_string(at));
    MapNode node;
    node.kind = MapNode::Held;
    node.path = label;
    node.size = bytes.size();
    node.sha256 = sha;
    node.bytes = bytes;
    const uint32_t id = uint32_t(nodes.size());
    nodes.push_back(std::move(node));
    edges.push_back(MapEdge{0, id, "anchor"});
    sort_edges(edges);
    return id;
}

void Map::remove(uint32_t node)
{
    if (sealed) throw std::runtime_error("the map " + name + " is sealed: it is not changed");
    if (node == 0 || node >= nodes.size()) throw std::runtime_error("that node cannot be removed");
    nodes.erase(nodes.begin() + std::ptrdiff_t(node));
    auto shift = [node](uint32_t v) { return v > node ? v - 1 : v; };
    std::vector<MapEdge> kept;
    for (const MapEdge& e : edges)
        if (e.from != node && e.to != node) kept.push_back(MapEdge{shift(e.from), shift(e.to), e.relation});
    sort_edges(kept);
    edges = std::move(kept);
    std::vector<MapMeta> kept_meta;
    for (const MapMeta& md : meta)
        if (md.node != node) kept_meta.push_back(MapMeta{shift(md.node), md.key, md.value});
    meta = std::move(kept_meta);
}

void Map::set_meta(uint32_t node, const std::string& key, const std::string& value)
{
    if (sealed) throw std::runtime_error("the map " + name + " is sealed: it is not changed");
    if (node >= nodes.size()) throw std::runtime_error("no node " + std::to_string(node));
    if (!is_word(key)) throw std::runtime_error("a metadata key is a lower-case word: " + key);
    if (value.find_first_of("\t\r\n") != std::string::npos) throw std::runtime_error("a metadata value cannot hold tabs or line breaks");
    meta.erase(std::remove_if(meta.begin(), meta.end(), [&](const MapMeta& m) { return m.node == node && m.key == key; }), meta.end());
    if (!value.empty()) meta.push_back(MapMeta{node, key, value});
    std::sort(meta.begin(), meta.end(), [](const MapMeta& a, const MapMeta& b) {
        if (a.node != b.node) return a.node < b.node;
        return less_bytes(a.key, b.key);
    });
}

Map map_of_manifest(const Manifest& m, const std::string& name)
{
    Map map;
    map.name = name;
    map.root = m.root;
    MapNode root;
    root.kind = MapNode::Folder;
    root.path = "./";
    map.nodes.push_back(root);
    std::map<std::string, uint32_t> folder_id; // a folder's path, without its '/', to its node
    folder_id[""] = 0;
    for (const ManifestEntry& e : m.entries)
    {
        MapNode n;
        n.kind = e.dir ? MapNode::Folder : MapNode::File;
        n.path = e.path;
        n.size = e.size;
        n.sha256 = e.sha256;
        const uint32_t id = uint32_t(map.nodes.size());
        map.nodes.push_back(n);
        if (e.dir) folder_id[e.path.substr(0, e.path.size() - 1)] = id;
    }
    // Every node but the root is contained by the folder its path is in. The manifest lists a
    // folder before everything in it (its path is a prefix, so it sorts first), so the parent is
    // always already known.
    for (uint32_t id = 1; id < map.nodes.size(); ++id)
    {
        std::string p = map.nodes[id].path;
        if (map.nodes[id].folder()) p.pop_back();
        const size_t slash = p.rfind('/');
        const std::string parent = slash == std::string::npos ? "" : p.substr(0, slash);
        const auto it = folder_id.find(parent);
        if (it == folder_id.end()) throw std::runtime_error("map: " + map.nodes[id].path + " has no folder in the manifest");
        map.edges.push_back(MapEdge{it->second, id, "contains"});
    }
    sort_edges(map.edges);
    return map;
}

Map map_of_folder(const fs::path& folder, const std::string& name) { return map_of_manifest(walk_folder(folder), name); }

Map map_of_files(const std::vector<fs::path>& files, const std::string& name)
{
    Manifest m;
    m.root = name;
    std::set<std::string> seen;
    for (const fs::path& f : files)
    {
        ManifestEntry e = manifest_of_file(f).entries.at(0);
        if (!seen.insert(e.path).second) throw std::runtime_error("two files named " + e.path + ": a map of files finds them by name");
        m.entries.push_back(e);
    }
    std::sort(m.entries.begin(), m.entries.end(), [](const ManifestEntry& a, const ManifestEntry& b) { return less_bytes(a.path, b.path); });
    return map_of_manifest(m, name);
}

fs::path map_node_path(const fs::path& root, const MapNode& n)
{
    if (n.path == "./") return root;
    const std::string p = n.folder() ? n.path.substr(0, n.path.size() - 1) : n.path;
    return root / fs::path(std::u8string(p.begin(), p.end()));
}

std::string Map::dot() const
{
    std::string t = "digraph \"" + dot_escape(name) + "\" {\n  // " + (needs_v2() ? kMapVersion2 : kMapVersion) + ", root " + dot_escape(root) + "\n";
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const MapNode& n = nodes[i];
        t += "  n" + std::to_string(i) + " [label=\"" + dot_escape(label(i)) + "\", shape=" + (n.folder() ? "folder" : "box") + ", kind=\"" + kind_word(n.kind) + "\"";
        if (!n.folder()) t += ", size_bytes=\"" + std::to_string(n.size) + "\", sha256=\"" + n.sha256 + "\"";
        t += ", path=\"" + dot_escape(n.path) + "\"];\n";
    }
    for (const MapEdge& e : edges)
        t += "  n" + std::to_string(e.from) + " -> n" + std::to_string(e.to) + " [label=\"" + e.relation + "\"];\n";
    t += "}\n";
    return t;
}

std::string Map::graphml() const
{
    std::string t = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<graphml xmlns=\"http://graphml.graphdrawing.org/xmlns\">\n"
                    "  <key id=\"label\" for=\"node\" attr.name=\"label\" attr.type=\"string\"/>\n"
                    "  <key id=\"kind\" for=\"node\" attr.name=\"kind\" attr.type=\"string\"/>\n"
                    "  <key id=\"path\" for=\"node\" attr.name=\"path\" attr.type=\"string\"/>\n"
                    "  <key id=\"size\" for=\"node\" attr.name=\"size\" attr.type=\"long\"/>\n"
                    "  <key id=\"sha256\" for=\"node\" attr.name=\"sha256\" attr.type=\"string\"/>\n"
                    "  <key id=\"relation\" for=\"edge\" attr.name=\"relation\" attr.type=\"string\"/>\n";
    t += "  <graph id=\"" + xml_escape(name) + "\" edgedefault=\"directed\">\n";
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const MapNode& n = nodes[i];
        t += "    <node id=\"n" + std::to_string(i) + "\"><data key=\"label\">" + xml_escape(label(i)) + "</data><data key=\"kind\">" +
             kind_word(n.kind) + "</data><data key=\"path\">" + xml_escape(n.path) + "</data>";
        if (!n.folder()) t += "<data key=\"size\">" + std::to_string(n.size) + "</data><data key=\"sha256\">" + n.sha256 + "</data>";
        t += "</node>\n";
    }
    for (size_t i = 0; i < edges.size(); ++i)
        t += "    <edge id=\"e" + std::to_string(i) + "\" source=\"n" + std::to_string(edges[i].from) + "\" target=\"n" +
             std::to_string(edges[i].to) + "\"><data key=\"relation\">" + edges[i].relation + "</data></edge>\n";
    t += "  </graph>\n</graphml>\n";
    return t;
}

} // namespace sieve::cli
