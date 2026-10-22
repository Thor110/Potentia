#include "cli/map_sieve.hpp"

#include "cli/args.hpp"
#include "cli/items.hpp"
#include "cli/pack.hpp"

#include <fstream>
#include <iterator>
#include <random>
#include <stdexcept>

namespace sieve::cli {

namespace fs = std::filesystem;

Map map_of_instructions(const Manifest& m, const std::string& name)
{
    // Each file's bytes, cut from the contents as installing cuts them.
    std::vector<std::pair<const ManifestEntry*, std::vector<uint8_t>>> files;
    size_t at = 0;
    for (const ManifestEntry& e : m.entries)
    {
        if (e.dir) continue;
        if (!m.with_contents || m.contents.size() - at < e.size) throw std::runtime_error("the instructions end before " + e.path);
        files.emplace_back(&e, std::vector<uint8_t>(m.contents.begin() + std::ptrdiff_t(at), m.contents.begin() + std::ptrdiff_t(at + e.size)));
        at += size_t(e.size);
    }
    if (m.graph.empty())
    {
        // Instructions without a graph part: a map of the tree they carry, every file named by its
        // own SHA-256 (the listing may leave it out, the tree's covering it).
        Manifest named = m;
        size_t k = 0;
        for (ManifestEntry& e : named.entries)
            if (!e.dir) e.sha256 = sha256_hex(files[k++].second);
        return map_of_manifest(named, name);
    }
    Map map = Map::parse(m.graph, false);
    for (MapNode& node : map.nodes)
    {
        if (node.kind != MapNode::Held) continue;
        bool found = false;
        for (auto& [e, bytes] : files)
            if (e->path == node.path)
            {
                if (bytes.size() != node.size || sha256_hex(bytes) != node.sha256)
                    throw std::runtime_error("map: " + node.path + "'s bytes do not match its SHA-256");
                node.bytes = bytes;
                found = true;
                break;
            }
        if (!found) throw std::runtime_error("map: the instructions do not carry " + node.path);
    }
    return map;
}

bool is_old_map_file(const fs::path& file)
{
    std::ifstream in(file, std::ios::binary);
    char head[11] = {};
    in.read(head, sizeof head);
    return in.gcount() == 11 && std::string(head, 11) == "sieve-map-v";
}

Map read_map_file(const fs::path& file, bool* is_map)
{
    if (is_old_map_file(file))
    {
        if (is_map) *is_map = true;
        std::ifstream in(file, std::ios::binary);
        const std::string bytes((std::istreambuf_iterator<char>(in)), {});
        return Map::parse(bytes);
    }
    const Manifest m = installable_manifest(file, false);
    if (is_map) *is_map = !m.graph.empty();
    const std::u8string stem = file.stem().u8string();
    return map_of_instructions(m, std::string(stem.begin(), stem.end()));
}

void write_map_instructions(const Map& map, const fs::path& out, double search_per_file)
{
    // The held files, laid out as a folder named as the map's root, then packed as that folder's
    // instructions are; the map's text is their graph part.
    std::random_device rd;
    const fs::path work = fs::temp_directory_path() / ("sieve-map-" + std::to_string(rd()) + std::to_string(rd()));
    const fs::path dir = work / fs::path(std::u8string(map.root.begin(), map.root.end()));
    struct Cleanup
    {
        fs::path p;
        ~Cleanup()
        {
            std::error_code ec;
            fs::remove_all(p, ec);
        }
    } cleanup{work};
    fs::create_directories(dir);
    for (const MapNode& n : map.nodes)
    {
        if (n.kind != MapNode::Held) continue;
        std::ofstream f(dir / fs::path(std::u8string(n.path.begin(), n.path.end())), std::ios::binary);
        f.write(reinterpret_cast<const char*>(n.bytes.data()), std::streamsize(n.bytes.size()));
        if (!f) throw std::runtime_error("cannot stage " + n.path + " for the map");
    }
    Manifest m = walk_folder(dir);
    m.graph = map.text(); // before packing: the listing packed is the one with the graph part
    add_packed_items(m, dir, Args{}, nullptr, 8, false, search_per_file);
    write_address_file(out, binary_address(m.file()), false);
}

} // namespace sieve::cli
