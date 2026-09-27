// Maps (map.hpp): built from a folder's manifest, written and read in one canonical text, and
// written for other programs as DOT and GraphML.

#include "cli/map.hpp"

#include <algorithm>
#include <map>
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

bool is_relation(const std::string& r)
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

} // namespace

std::string MapNode::label() const
{
    std::string p = folder && !path.empty() ? path.substr(0, path.size() - 1) : path;
    const size_t slash = p.rfind('/');
    return slash == std::string::npos ? p : p.substr(slash + 1);
}

std::string Map::label(size_t node) const { return nodes[node].path == "./" ? root : nodes[node].label(); }

std::string Map::text() const
{
    std::string t = std::string(kMapVersion) + "\n";
    t += "name " + name + "\n";
    t += "root " + root + "\n";
    t += "nodes " + std::to_string(nodes.size()) + "\n";
    t += "edges " + std::to_string(edges.size()) + "\n";
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const MapNode& n = nodes[i];
        if (n.folder) t += "n\t" + std::to_string(i) + "\tfolder\t" + n.path + "\n";
        else t += "n\t" + std::to_string(i) + "\tfile\t" + std::to_string(n.size) + "\t" + n.sha256 + "\t" + n.path + "\n";
    }
    for (const MapEdge& e : edges) t += "e\t" + std::to_string(e.from) + "\t" + std::to_string(e.to) + "\t" + e.relation + "\n";
    t += "end\n";
    return t;
}

Map Map::parse(std::string_view text)
{
    std::vector<std::string_view> lines;
    for (size_t at = 0; at < text.size();)
    {
        const size_t nl = text.find('\n', at);
        if (nl == std::string_view::npos) throw std::runtime_error("map: the last line has no line feed");
        lines.push_back(text.substr(at, nl - at));
        at = nl + 1;
    }
    auto field = [](std::string_view line, const char* key) {
        const std::string k = std::string(key) + " ";
        if (line.substr(0, k.size()) != k) throw std::runtime_error(std::string("map: expected '") + key + "'");
        return std::string(line.substr(k.size()));
    };
    if (lines.size() < 6 || lines[0] != kMapVersion) throw std::runtime_error("not a sieve map");
    if (lines.back() != "end") throw std::runtime_error("map: no 'end'");
    Map m;
    m.name = field(lines[1], "name");
    m.root = field(lines[2], "root");
    const uint64_t n = std::stoull(field(lines[3], "nodes")), e = std::stoull(field(lines[4], "edges"));
    if (lines.size() != 6 + n + e) throw std::runtime_error("map: its counts do not match its lines");
    for (size_t i = 0; i < n; ++i)
    {
        const auto f = split_tabs(lines[5 + i]);
        MapNode node;
        if (f.size() == 4 && f[0] == "n" && f[2] == "folder") { node.folder = true; node.path = f[3]; }
        else if (f.size() == 6 && f[0] == "n" && f[2] == "file")
        {
            node.size = std::stoull(f[3]);
            node.sha256 = f[4];
            node.path = f[5];
            if (!is_hex64(node.sha256)) throw std::runtime_error("map: line " + std::to_string(6 + i) + " has no SHA-256");
        }
        else throw std::runtime_error("map: line " + std::to_string(6 + i) + " is not a node");
        if (f[1] != std::to_string(i)) throw std::runtime_error("map: node ids must count up from 0");
        if (!safe_path(node.path) || node.folder != (node.path.back() == '/'))
            throw std::runtime_error("map: refused path " + node.path);
        m.nodes.push_back(std::move(node));
    }
    for (size_t i = 0; i < e; ++i)
    {
        const auto f = split_tabs(lines[5 + n + i]);
        if (f.size() != 4 || f[0] != "e" || !is_relation(f[3])) throw std::runtime_error("map: line " + std::to_string(6 + n + i) + " is not an edge");
        MapEdge edge{uint32_t(std::stoul(f[1])), uint32_t(std::stoul(f[2])), f[3]};
        if (edge.from >= n || edge.to >= n) throw std::runtime_error("map: an edge names a node that is not there");
        m.edges.push_back(std::move(edge));
    }
    return m;
}

Map map_of_manifest(const Manifest& m, const std::string& name)
{
    Map map;
    map.name = name;
    map.root = m.root;
    map.nodes.push_back(MapNode{true, "./", 0, {}});
    std::map<std::string, uint32_t> folder_id; // a folder's path, without its '/', to its node
    folder_id[""] = 0;
    for (const ManifestEntry& e : m.entries)
    {
        MapNode n;
        n.folder = e.dir;
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
        if (map.nodes[id].folder) p.pop_back();
        const size_t slash = p.rfind('/');
        const std::string parent = slash == std::string::npos ? "" : p.substr(0, slash);
        const auto it = folder_id.find(parent);
        if (it == folder_id.end()) throw std::runtime_error("map: " + map.nodes[id].path + " has no folder in the manifest");
        map.edges.push_back(MapEdge{it->second, id, "contains"});
    }
    std::sort(map.edges.begin(), map.edges.end(), [](const MapEdge& a, const MapEdge& b) {
        if (a.from != b.from) return a.from < b.from;
        if (a.to != b.to) return a.to < b.to;
        return less_bytes(a.relation, b.relation);
    });
    return map;
}

Map map_of_folder(const fs::path& folder, const std::string& name) { return map_of_manifest(walk_folder(folder), name); }

fs::path map_node_path(const fs::path& root, const MapNode& n)
{
    if (n.path == "./") return root;
    const std::string p = n.folder ? n.path.substr(0, n.path.size() - 1) : n.path;
    return root / fs::path(std::u8string(p.begin(), p.end()));
}

std::string Map::dot() const
{
    std::string t = "digraph \"" + dot_escape(name) + "\" {\n  // " + kMapVersion + ", root " + dot_escape(root) + "\n";
    for (size_t i = 0; i < nodes.size(); ++i)
    {
        const MapNode& n = nodes[i];
        t += "  n" + std::to_string(i) + " [label=\"" + dot_escape(label(i)) + "\", shape=" + (n.folder ? "folder" : "box");
        if (!n.folder) t += ", size_bytes=\"" + std::to_string(n.size) + "\", sha256=\"" + n.sha256 + "\"";
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
             (n.folder ? "folder" : "file") + "</data><data key=\"path\">" + xml_escape(n.path) + "</data>";
        if (!n.folder) t += "<data key=\"size\">" + std::to_string(n.size) + "</data><data key=\"sha256\">" + n.sha256 + "</data>";
        t += "</node>\n";
    }
    for (size_t i = 0; i < edges.size(); ++i)
        t += "    <edge id=\"e" + std::to_string(i) + "\" source=\"n" + std::to_string(edges[i].from) + "\" target=\"n" +
             std::to_string(edges[i].to) + "\"><data key=\"relation\">" + edges[i].relation + "</data></edge>\n";
    t += "  </graph>\n</graphml>\n";
    return t;
}

} // namespace sieve::cli
