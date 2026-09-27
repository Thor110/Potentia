// The node graph: maps of verified anchors (tools/cli/map.hpp, SPECIFICATIONS §12.3), drawn in 3D.
//
// A map links real files: every node is a file or a folder, a file named by its size and SHA-256,
// and every edge a relation between two nodes ("contains", for a folder's contents). The viewer
// (O, or the pause menu) shows one map at a time, chosen from a dropdown: "This installation",
// the map of the program's own files, made when first chosen, so the executable you are running
// is the first anchor; every .map in the maps folder beside the program; and any map opened or
// made while the program runs. It lays the map out as a 3D force-directed graph (Fruchterman and
// Reingold's: every node pushes every other away, every edge pulls its two ends together, and the
// moves shrink step by step until it settles), which you turn with the mouse like a model. A
// selected file node that is here, and still has its bytes, is a verified anchor, and Go to it
// walks to it on the binary line.
//
// The item page's SORT tab shows the item in hand against the chosen map: a file on the binary
// line whose bytes some node names is that node, drawn in its place among the rest; anything else
// is a lone point. The whole state space is never drawn as a graph: nearly all of it is noise, and
// a graph as large as the space would show only a tangle. Maps are chosen, not the space.
//
// Nothing here is part of an address, and nothing is stored but what you ask to save: a map made
// from a folder goes into the maps folder so it is there next time, and a map can be exported as
// DOT or GraphML for other programs.

#include "hallway.hpp"

#include "cli/locate.hpp"
#include "cli/map.hpp"

namespace hallway::hall {

namespace {

namespace fs = std::filesystem;

fs::path from_u8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string u8(const fs::path& p)
{
    const std::u8string s = p.u8string();
    return std::string(s.begin(), s.end());
}

uint64_t mix(uint64_t x)
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

float unit_of(uint64_t h) { return float(double(h >> 11) / double(1ull << 53)) * 2.0f - 1.0f; } // [-1, 1)

fs::path program_dir()
{
    const char* base = SDL_GetBasePath();
    return base ? from_u8(base) : fs::current_path();
}

// What "This installation" maps: the programs and the folders the build puts beside them, as far
// as they are there, and nothing else (a build folder holds much more than the program uses).
const char* const kInstalled[] = {"hallway",        "hallway.exe", "sieve",  "sieve.exe", "sieve-install", "sieve-install.exe",
                                  "dictionaries",   "models",      "lang",   "fonts",     "meshes",        "maps",
                                  "third_party_licenses"};

cli::Map installation_map()
{
    const fs::path base = program_dir();
    cli::Manifest m;
    fs::path r = base;
    if (!r.has_filename()) r = r.parent_path();
    m.root = u8(r.filename());
    for (const char* name : kInstalled)
    {
        const fs::path p = base / name;
        std::error_code ec;
        if (fs::is_regular_file(p, ec))
        {
            cli::ManifestEntry e;
            e.path = name;
            e.size = uint64_t(fs::file_size(p));
            e.sha256 = cli::sha256_file_hex(p);
            m.entries.push_back(e);
        }
        else if (fs::is_directory(p, ec))
        {
            cli::ManifestEntry d;
            d.dir = true;
            d.path = std::string(name) + "/";
            m.entries.push_back(d);
            for (cli::ManifestEntry e : cli::walk_folder(p).entries)
            {
                e.path = std::string(name) + "/" + e.path;
                m.entries.push_back(e);
            }
        }
    }
    std::sort(m.entries.begin(), m.entries.end(), [](const cli::ManifestEntry& a, const cli::ManifestEntry& b) {
        return std::lexicographical_compare(a.path.begin(), a.path.end(), b.path.begin(), b.path.end(),
                                            [](char x, char y) { return uint8_t(x) < uint8_t(y); });
    });
    return cli::map_of_manifest(m, "This installation");
}

// Where a map file's root folder is: beside the map, under the root's name, if that is a folder;
// otherwise the map's own folder (a map kept inside what it maps).
fs::path root_of_map_file(const fs::path& file, const cli::Map& m)
{
    const fs::path dir = file.parent_path();
    std::error_code ec;
    if (fs::is_directory(dir / from_u8(m.root), ec)) return dir / from_u8(m.root);
    return dir;
}

std::optional<std::string> read_text(const fs::path& p)
{
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

constexpr int kLayoutSteps = 300;

void SDLCALL graph_dialog(void* user, const char* const* files, int)
{
    if (!files || !files[0]) return;
    auto* p = static_cast<std::pair<Hallway*, int>*>(user);
    p->first->graph_picked(p->second, files[0]);
}

} // namespace

// ---- a map made ready to draw

void Hallway::GraphMap::prepare()
{
    const size_t n = map.nodes.size();
    adj.assign(n, {});
    for (const cli::MapEdge& e : map.edges)
    {
        adj[e.from].push_back(e.to);
        adj[e.to].push_back(e.from);
    }
    present.assign(n, 0);
    verified.assign(n, -1);
    by_sha.clear();
    for (size_t i = 0; i < n; ++i)
    {
        const cli::MapNode& node = map.nodes[i];
        std::error_code ec;
        if (node.kind == cli::MapNode::Held)
        {
            // Its bytes are in the map, checked against its SHA-256 when the map was read.
            present[i] = 1;
            verified[i] = 1;
            by_sha.emplace(node.sha256, uint32_t(i));
            continue;
        }
        const fs::path p = cli::map_node_path(root, node);
        if (node.folder()) present[i] = fs::is_directory(p, ec) ? 1 : 0;
        else
        {
            present[i] = fs::is_regular_file(p, ec) && fs::file_size(p, ec) == node.size ? 1 : 0;
            by_sha.emplace(node.sha256, uint32_t(i));
        }
    }
    // Start from a burst: each node as far out as it is edges from the root, in a direction of its
    // own (from its number, so the same map always starts, and so settles, the same way).
    std::vector<int> depth(n, -1);
    std::deque<uint32_t> q;
    if (n) { depth[0] = 0; q.push_back(0); }
    while (!q.empty())
    {
        const uint32_t v = q.front();
        q.pop_front();
        for (uint32_t w : adj[v])
            if (depth[w] < 0) { depth[w] = depth[v] + 1; q.push_back(w); }
    }
    pos.assign(n, Vec3{});
    vel.assign(n, Vec3{});
    for (size_t i = 0; i < n; ++i)
    {
        Vec3 d{unit_of(mix(i * 3)), unit_of(mix(i * 3 + 1)), unit_of(mix(i * 3 + 2))};
        const float len = std::sqrt(dot(d, d));
        d = len > 1e-4f ? d * (1.0f / len) : Vec3{1, 0, 0};
        const float r = depth[i] < 0 ? 1.0f : float(depth[i]);
        pos[i] = d * (r + 0.2f * unit_of(mix(i + 77)));
    }
    steps = 0;
}

// One step: every node pushes every other away (k^2 / d), every edge pulls its ends together
// (d^2 / k), a little pull to the middle keeps pieces with no edge between them from drifting off,
// and no node moves further than the step's "temperature", which falls to nothing. With many nodes
// each is pushed by a sample of the others, scaled up, rather than by all of them.
bool Hallway::graph_step(GraphMap& g, int budget)
{
    const size_t n = g.map.nodes.size();
    if (!g.ready || n < 2 || g.steps >= kLayoutSteps) return false;
    constexpr float k = 1.0f;
    constexpr size_t kExact = 900, kSample = 128;
    const double cost = n <= kExact ? double(n) * double(n) / 2 : double(n) * kSample;
    const int iterations = std::clamp(int(double(budget) / std::max(1.0, cost)), 1, 20);
    std::vector<Vec3> disp(n);
    for (int it = 0; it < iterations && g.steps < kLayoutSteps; ++it, ++g.steps)
    {
        std::fill(disp.begin(), disp.end(), Vec3{});
        if (n <= kExact)
            for (size_t i = 0; i < n; ++i)
                for (size_t j = i + 1; j < n; ++j)
                {
                    const Vec3 d = g.pos[i] - g.pos[j];
                    const float d2 = std::max(dot(d, d), 1e-4f);
                    const Vec3 f = d * (k * k / d2);
                    disp[i] = disp[i] + f;
                    disp[j] = disp[j] - f;
                }
        else
        {
            const float scale = float(n - 1) / float(kSample);
            for (size_t i = 0; i < n; ++i)
                for (size_t s = 0; s < kSample; ++s)
                {
                    const size_t j = size_t(mix((uint64_t(i) << 32) ^ (uint64_t(g.steps) << 8) ^ s) % n);
                    if (j == i) continue;
                    const Vec3 d = g.pos[i] - g.pos[j];
                    const float d2 = std::max(dot(d, d), 1e-4f);
                    disp[i] = disp[i] + d * (scale * k * k / d2);
                }
        }
        for (const cli::MapEdge& e : g.map.edges)
        {
            const Vec3 d = g.pos[e.from] - g.pos[e.to];
            const float dist = std::sqrt(dot(d, d));
            const Vec3 f = d * (dist / k);
            disp[e.from] = disp[e.from] - f;
            disp[e.to] = disp[e.to] + f;
        }
        const float t = 0.35f * (1.0f - float(g.steps) / float(kLayoutSteps)) + 0.005f;
        for (size_t i = 0; i < n; ++i)
        {
            const Vec3 d = disp[i] - g.pos[i] * 0.05f;
            const float len = std::sqrt(dot(d, d));
            if (len > 1e-6f) g.pos[i] = g.pos[i] + d * (std::min(len, t) / len);
        }
    }
    return g.steps < kLayoutSteps;
}

// ---- the list of maps

void Hallway::graph_list()
{
    if (graph_listed_) return;
    graph_listed_ = true;
    GraphMap inst;
    inst.title = tr("graph.installation");
    inst.installation = true;
    inst.root = program_dir();
    graphs_.push_back(std::move(inst));
    std::vector<fs::path> files;
    std::error_code ec;
    for (const auto& e : fs::directory_iterator(program_dir() / "maps", ec))
        if (e.is_regular_file() && e.path().extension() == ".map") files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const fs::path& f : files)
    {
        GraphMap g;
        g.file = f;
        g.title = u8(f.stem());
        try
        {
            const auto text = read_text(f);
            if (!text) throw std::runtime_error("cannot read it");
            g.map = cli::Map::parse(*text);
            g.root = root_of_map_file(f, g.map);
            g.ready = true;
            g.prepare();
        }
        catch (const std::exception& ex)
        {
            g.error = ex.what();
        }
        graphs_.push_back(std::move(g));
    }
    // "This installation" is chosen to start with (graph_sel_ 0); maps to add anchors to are made
    // with the viewer's Create new map, and chosen with the dropdown or [ ].
}

bool Hallway::GraphMap::read_only() const { return installation || map.sealed || !ready; }

// Writes a changed map back to its file.
void Hallway::graph_save(GraphMap& g)
{
    if (g.file.empty()) g.file = program_dir() / "maps" / from_u8(g.title + ".map");
    const auto bytes = g.map.file();
    std::ofstream f(g.file, std::ios::binary);
    f.write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
    if (!f) throw std::runtime_error("cannot write " + u8(g.file));
}

// V on an item: the file in hand added to the chosen map as a held anchor (its bytes kept in the
// map, so the map can walk back to it), or, if the map has it already, removed from it. Only
// files: an item of another line belongs to that line's setup, which a map node does not record.
void Hallway::graph_add_anchor(const Book& bk)
{
    GraphMap* g = graph_current();
    if (!g || !g->ready)
    {
        message(tr("anchor.no_map"));
        return;
    }
    if (g->read_only())
    {
        message(trf("anchor.read_only", {g->title}));
        return;
    }
    if (!on_binary() || !bk.is_file)
    {
        message(tr("anchor.files_only"));
        return;
    }
    const auto bytes = file_of(bk);
    const std::string sha = cli::sha256_hex(bytes);
    if (const int at = g->map.find(sha); at > 0)
    {
        const std::string gone = g->map.label(size_t(at));
        try
        {
            g->map.remove(uint32_t(at));
            graph_save(*g);
            g->prepare();
            message(trf("anchor.removed", {gone, g->title}));
        }
        catch (const std::exception& ex)
        {
            message(trf("anchor.failed", {ex.what()}));
        }
        return;
    }
    // Its name: the file's own, if it was walked to from one; else its title, else the start of
    // its SHA-256, with the kind its first bytes say it is.
    std::string name;
    if (auto it = walked_names_.find(sha); it != walked_names_.end()) name = it->second;
    else
    {
        std::string t = title_text(bk);
        while (!t.empty() && t.back() == ' ') t.pop_back();
        while (!t.empty() && t.front() == ' ') t.erase(t.begin());
        std::string kind = file_type(bk.head, bk.file_size);
        for (char& c : kind) c = char(std::tolower(uint8_t(c)));
        name = (t.empty() ? "anchor-" + sha.substr(0, 12) : t) + (kind == "?" || kind == "empty" ? "" : "." + kind);
    }
    for (char& c : name)
        if (std::string("/\\:\t\r\n").find(c) != std::string::npos) c = '_';
    try
    {
        const uint32_t id = g->map.add_held(name, bytes);
        graph_save(*g);
        g->prepare();
        message(trf("anchor.added", {name, g->title, std::to_string(id)}));
    }
    catch (const std::exception& ex)
    {
        message(trf("anchor.failed", {ex.what()}));
    }
}

// The viewer's Remove from map: the chosen node, its links and its metadata.
void Hallway::graph_remove(int node)
{
    GraphMap* g = graph_current();
    if (!g || g->read_only() || node <= 0 || node >= int(g->map.nodes.size())) return;
    const std::string name = g->map.label(size_t(node));
    try
    {
        g->map.remove(uint32_t(node));
        graph_save(*g);
        g->prepare();
        graph_pick_ = -1;
        graph_status_ = trf("anchor.removed", {name, g->title});
    }
    catch (const std::exception& ex)
    {
        graph_status_ = trf("anchor.failed", {ex.what()});
    }
}

Hallway::GraphMap* Hallway::graph_current()
{
    graph_list();
    graph_poll(); // a map made or chosen since the last look (New map..., Open..., the worker)
    return graph_sel_ >= 0 && graph_sel_ < int(graphs_.size()) ? &graphs_[size_t(graph_sel_)] : nullptr;
}

void Hallway::graph_select(int i)
{
    graph_list();
    if (graphs_.empty()) return;
    graph_sel_ = (i % int(graphs_.size()) + int(graphs_.size())) % int(graphs_.size());
    graph_pick_ = -1;
    graph_go_armed_ = false;
    GraphMap& g = graphs_[size_t(graph_sel_)];
    // "This installation" is made the first time it is wanted, on the worker: it reads every file.
    if (g.installation && !g.ready && g.error.empty() && !graph_busy_) graph_make("");
}

// A map made on the worker: of a folder (saved into the maps folder, so it is there next time),
// or, with an empty path, of this installation.
void Hallway::graph_make(const std::string& folder, bool sync)
{
    if (graph_busy_) return;
    if (graph_worker_.joinable()) graph_worker_.join();
    graph_busy_ = true;
    graph_status_ = folder.empty() ? tr("graph.making_installation") : trf("graph.making", {folder});
    auto job = [this, folder] {
        GraphMap g;
        std::string status;
        try
        {
            if (folder.empty())
            {
                g.installation = true;
                g.title = tr("graph.installation");
                g.root = program_dir();
                g.map = installation_map();
            }
            else
            {
                const fs::path p = fs::absolute(from_u8(folder)).lexically_normal();
                fs::path r = p;
                if (!r.has_filename()) r = r.parent_path();
                g.title = u8(r.filename());
                g.root = r;
                g.map = cli::map_of_folder(r, g.title);
                // Kept in the maps folder beside the program, under a name not yet taken by a
                // different map; if that cannot be written, it stays for this session only.
                const fs::path dir = program_dir() / "maps";
                std::error_code ec;
                fs::create_directories(dir, ec);
                const auto bytes = g.map.file();
                const std::string text(bytes.begin(), bytes.end());
                fs::path out = dir / from_u8(g.title + ".map");
                for (int k = 2; fs::exists(out, ec) && read_text(out).value_or("") != text; ++k)
                    out = dir / from_u8(g.title + " (" + std::to_string(k) + ").map");
                std::ofstream f(out, std::ios::binary);
                f << text;
                if (f)
                {
                    g.file = out;
                    status = trf("graph.saved_to", {u8(out)});
                }
                else status = tr("graph.not_saved");
            }
            g.ready = true;
            g.prepare();
        }
        catch (const std::exception& ex)
        {
            g.error = ex.what();
            status = trf("graph.failed", {ex.what()});
        }
        std::lock_guard<std::mutex> lock(graph_lock_);
        graph_made_ = std::move(g);
        graph_status_ = status;
        graph_busy_ = false;
    };
    if (sync) job();
    else graph_worker_ = std::thread(job);
}

void Hallway::graph_picked(int what, const std::string& path)
{
    std::lock_guard<std::mutex> lock(graph_lock_);
    graph_pending_ = std::make_pair(what, path);
}

// What the worker and the dialogs have handed over since the last frame.
void Hallway::graph_poll()
{
    std::optional<GraphMap> made;
    std::optional<std::pair<int, std::string>> pending;
    {
        std::lock_guard<std::mutex> lock(graph_lock_);
        made.swap(graph_made_);
        pending.swap(graph_pending_);
    }
    if (made)
    {
        if (made->installation)
        {
            for (auto& g : graphs_)
                if (g.installation) g = std::move(*made);
        }
        else if (made->ready)
        {
            graphs_.push_back(std::move(*made));
            graph_sel_ = int(graphs_.size()) - 1;
            graph_pick_ = -1;
        }
    }
    if (!pending) return;
    const auto& [what, path] = *pending;
    if (what == 1) graph_make(path);
    else if (what == 2)
    {
        GraphMap g;
        g.file = from_u8(path);
        g.title = u8(g.file.stem());
        try
        {
            const auto text = read_text(g.file);
            if (!text) throw std::runtime_error("cannot read it");
            g.map = cli::Map::parse(*text);
            g.root = root_of_map_file(g.file, g.map);
            g.ready = true;
            g.prepare();
            graphs_.push_back(std::move(g));
            graph_sel_ = int(graphs_.size()) - 1;
            graph_pick_ = -1;
            graph_status_ = trf("graph.opened", {path});
        }
        catch (const std::exception& ex)
        {
            graph_status_ = trf("graph.failed", {ex.what()});
        }
    }
    else if (what == 3) graph_export(path);
    else if (what == 4)
    {
        // Create new map: an empty map (only its root) under the name chosen, in the maps folder
        // unless the dialog chose elsewhere.
        fs::path p = from_u8(path);
        if (p.extension() != ".map") p += ".map";
        std::error_code ec;
        if (fs::exists(p, ec))
        {
            graph_status_ = trf("graph.exists", {u8(p)});
            return;
        }
        try
        {
            fs::create_directories(p.parent_path(), ec);
            GraphMap g;
            g.file = p;
            g.title = u8(p.stem());
            g.map = cli::Map::empty(g.title);
            g.root = p.parent_path();
            g.ready = true;
            graph_save(g);
            g.prepare();
            graphs_.push_back(std::move(g));
            graph_sel_ = int(graphs_.size()) - 1;
            graph_pick_ = -1;
            graph_status_ = trf("graph.created", {u8(p)});
        }
        catch (const std::exception& ex)
        {
            graph_status_ = trf("graph.failed", {ex.what()});
        }
    }
}

void Hallway::graph_export(const std::string& to)
{
    GraphMap* g = graph_current();
    if (!g || !g->ready) return;
    const fs::path p = from_u8(to);
    const std::string ext = u8(p.extension());
    const auto file = g->map.file();
    const std::string text = ext == ".dot" || ext == ".gv" ? g->map.dot() : ext == ".graphml" ? g->map.graphml() : std::string(file.begin(), file.end());
    std::ofstream f(p, std::ios::binary);
    f << text;
    graph_status_ = f ? trf("graph.exported", {to}) : trf("graph.failed", {"cannot write " + to});
}

// ---- opening, closing, going

void Hallway::open_graph()
{
    graph_list();
    graph_open_ = true;
    graph_drop_ = false;
    graph_drag_ = false;
    graph_select(graph_sel_);
    // With a file in hand that the map names, start on it.
    if (in_hand_) graph_pick_ = sort_node(*in_hand_);
    SDL_SetWindowRelativeMouseMode(window_, false);
}

void Hallway::close_graph()
{
    graph_open_ = false;
    graph_drop_ = false;
    if (!pause_open_ && !in_hand_) SDL_SetWindowRelativeMouseMode(window_, true);
}

void Hallway::stop_graph()
{
    if (graph_worker_.joinable()) graph_worker_.join();
}

void Hallway::graph_map_now(const std::string& path, bool open)
{
    graph_list();
    if (path.empty() || path == "installation")
    {
        graph_select(0);
        stop_graph();
        if (graphs_[0].ready == false) graph_make("", true);
    }
    else if (fs::is_directory(from_u8(path))) graph_make(path, true);
    else graph_picked(2, path);
    graph_poll();
    if (GraphMap* g = graph_current())
        while (graph_step(*g, 50000000)) {}
    if (open) open_graph();
}

void Hallway::graph_go(int node)
{
    GraphMap* g = graph_current();
    if (!g || !g->ready || node < 0 || node >= int(g->map.nodes.size())) return;
    const cli::MapNode& n = g->map.nodes[size_t(node)];
    if (n.folder()) return;
    const fs::path p = cli::map_node_path(g->root, n);
    std::error_code ec;
    if (n.kind == cli::MapNode::File && !fs::is_regular_file(p, ec))
    {
        graph_status_ = trf("graph.not_here", {n.path});
        return;
    }
    // A held anchor's bytes are in the map; a pointed-at file's are read from where it is.
    const auto bytes = n.kind == cli::MapNode::Held ? n.bytes : cli::read_file_bytes(p);
    const bool same = bytes.size() == n.size && cli::sha256_hex(bytes) == n.sha256;
    g->verified[size_t(node)] = same ? 1 : 0;
    if (!same)
    {
        graph_status_ = trf("graph.changed", {n.path});
        return;
    }
    const bool past = bytes.size() > binary_space_->max_bytes();
    if (past && !graph_go_armed_)
    {
        graph_go_armed_ = true;
        graph_status_ = trf("loc.go_anyway", {std::to_string(bytes.size())});
        return;
    }
    graph_go_armed_ = false;
    const std::string name = g->map.label(size_t(node));
    close_graph();
    if (pause_open_) close_pause();
    walk_to_file(bytes, name, past);
}

// ---- the SORT tab: the item in hand as a node, or a lone point

int Hallway::sort_node(const Book& bk)
{
    if (!on_binary() || !bk.is_file) return -1;
    GraphMap* g = graph_current();
    if (!g || !g->ready) return -1;
    const std::string key = bk.index.to_hex();
    auto it = sort_sha_.find(key);
    if (it == sort_sha_.end())
    {
        if (sort_sha_.size() > 256) sort_sha_.clear();
        it = sort_sha_.emplace(key, cli::sha256_hex(file_of(bk))).first;
    }
    const auto node = g->by_sha.find(it->second);
    return node == g->by_sha.end() ? -1 : int(node->second);
}

void Hallway::draw_sort(const Book& bk, float x, float cy, float pw, float bottom)
{
    graph_poll();
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{235, 235, 235, 255};
    GraphMap* g = graph_current();
    if (g && g->installation && !g->ready && g->error.empty() && !graph_busy_) graph_make("");
    text(x + 14, cy, fit(trf("sort.map", {g ? g->title : "-"}), pw - 28, 2), 2, ink);
    cy += 24;
    const int node = sort_node(bk);
    std::string what;
    if (!g || (!g->ready && g->error.empty())) what = tr("graph.making_installation");
    else if (!g->error.empty()) what = trf("graph.failed", {g->error});
    else if (node >= 0)
    {
        const cli::MapNode& n = g->map.nodes[size_t(node)];
        std::string parent;
        for (const cli::MapEdge& e : g->map.edges)
            if (e.to == uint32_t(node) && e.relation == "contains") parent = g->map.label(e.from);
        what = trf("sort.anchor", {std::to_string(node), n.path, parent.empty() ? "-" : parent,
                                   std::to_string(g->adj[size_t(node)].size())});
    }
    else if (on_binary() && bk.is_file) what = trf("sort.lone_file", {g->title});
    else what = trf("sort.lone_item", {tr(theme().key)});
    for (const auto& l : wrap(what, size_t(std::max(20.0f, (pw - 28) / 8))))
    {
        text(x + 14, cy, l, 1, node >= 0 ? white : grey);
        cy += 13;
    }
    cy += 6;
    const SDL_FRect area{x + 14, cy, pw - 28, std::max(40.0f, bottom - cy - 20)};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    SDL_RenderFillRect(r_, &area);
    if (g && g->ready) graph_step(*g, 1500000);
    draw_graph(g, area, sort_yaw_, sort_pitch_, 1.0f, node, node < 0, -1, -1);
    text(x + 14, area.y + area.h + 4, fit(tr("sort.keys"), pw - 28, 1), 1, grey);
}

// The item page's tabs: ITEM, COST, SORT, and META when the item is an anchor of the chosen map
// with metadata there.
int Hallway::hand_tabs()
{
    if (!in_hand_) return 3;
    const int node = sort_node(*in_hand_);
    const GraphMap* g = graph_current();
    if (node <= 0 || !g) return 3;
    for (const cli::MapMeta& m : g->map.meta)
        if (m.node == uint32_t(node)) return 4;
    return 3;
}

void Hallway::draw_meta(const Book& bk, float x, float cy, float pw, float bottom)
{
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{235, 235, 235, 255};
    const GraphMap* g = graph_current();
    const int node = sort_node(bk);
    if (!g || node <= 0) return;
    text(x + 14, cy, fit(trf("meta.title", {g->title, std::to_string(node)}), pw - 28, 2), 2, ink);
    cy += 28;
    float kw = 0;
    for (const cli::MapMeta& m : g->map.meta)
        if (m.node == uint32_t(node)) kw = std::max(kw, text_width(m.key, 2));
    for (const cli::MapMeta& m : g->map.meta)
    {
        if (m.node != uint32_t(node)) continue;
        if (cy + 20 > bottom) break;
        text(x + 14, cy, m.key, 2, grey);
        for (const auto& l : wrap(m.value, size_t(std::max(10.0f, (pw - 42 - kw) / 16))))
        {
            if (cy + 20 > bottom) break;
            text(x + 28 + kw, cy, l, 2, white);
            cy += 20;
        }
        cy += 6;
    }
}

// ---- drawing a graph

int Hallway::draw_graph(GraphMap* g, SDL_FRect area, float yaw, float pitch, float zoom, int focus, bool lone, float mx, float my)
{
    const SDL_Rect clip{int(area.x), int(area.y), int(area.w), int(area.h)};
    SDL_SetRenderClipRect(r_, &clip);
    const float cx = area.x + area.w / 2, cy = area.y + area.h / 2;
    const SDL_Color file_ink = theme_of(kBinaryLine).edge;
    auto square = [&](float px, float py, float s, SDL_Color c) {
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, c.a);
        const SDL_FRect q{px - s / 2, py - s / 2, s, s};
        SDL_RenderFillRect(r_, &q);
    };
    if (lone || !g || !g->ready || g->map.nodes.empty())
    {
        // The item alone: nothing in the map is it, and nothing links to it.
        square(cx, cy, 8, SDL_Color{255, 220, 60, 255});
        SDL_SetRenderDrawColor(r_, 255, 220, 60, 90);
        const SDL_FRect ring{cx - 12, cy - 12, 24, 24};
        SDL_RenderRect(r_, &ring);
        SDL_SetRenderClipRect(r_, nullptr);
        return -1;
    }
    const size_t n = g->map.nodes.size();
    Vec3 c{};
    if (focus >= 0 && focus < int(n)) c = g->pos[size_t(focus)];
    else
    {
        for (const Vec3& p : g->pos) c = c + p;
        c = c * (1.0f / float(n));
    }
    float R = 1.0f;
    for (const Vec3& p : g->pos)
    {
        const Vec3 d = p - c;
        R = std::max(R, std::sqrt(dot(d, d)));
    }
    const float scale = std::min(area.w, area.h) * 0.45f / R * zoom;
    const float cyw = std::cos(yaw), syw = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
    const float cam = 3.0f * R;
    struct Proj { float x, y, z, f; };
    std::vector<Proj> pr(n);
    for (size_t i = 0; i < n; ++i)
    {
        const Vec3 d = g->pos[i] - c;
        const float x1 = cyw * d.x + syw * d.z, z1 = -syw * d.x + cyw * d.z;
        const float y2 = cp * d.y - sp * z1, z2 = sp * d.y + cp * z1;
        const float f = cam / std::max(0.2f * cam, cam + z2);
        pr[i] = {cx + x1 * scale * f, cy - y2 * scale * f, z2, f};
    }
    // Edges first, faint, brighter where they touch the focus.
    for (const cli::MapEdge& e : g->map.edges)
    {
        const bool lit = int(e.from) == focus || int(e.to) == focus || int(e.from) == graph_pick_ || int(e.to) == graph_pick_;
        const Uint8 a = lit ? 200 : Uint8(std::clamp(90.0f * (pr[e.from].f + pr[e.to].f) / 2, 25.0f, 110.0f));
        if (e.relation == "contains") SDL_SetRenderDrawColor(r_, file_ink.r, file_ink.g, file_ink.b, a);
        else
        {
            const uint64_t h = mix(std::hash<std::string>{}(e.relation));
            SDL_SetRenderDrawColor(r_, Uint8(128 + (h & 127)), Uint8(128 + ((h >> 8) & 127)), Uint8(128 + ((h >> 16) & 127)), a);
        }
        SDL_RenderLine(r_, pr[e.from].x, pr[e.from].y, pr[e.to].x, pr[e.to].y);
    }
    // Nodes far to near, so the near ones are drawn over.
    std::vector<uint32_t> order(n);
    for (size_t i = 0; i < n; ++i) order[i] = uint32_t(i);
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) { return pr[a].z > pr[b].z; });
    int hot = -1;
    float best = 10.0f * 10.0f;
    for (uint32_t i : order)
    {
        const cli::MapNode& node = g->map.nodes[i];
        SDL_Color col = node.folder() ? SDL_Color{210, 210, 210, 255} : file_ink;
        if (!g->present[i]) col = SDL_Color{95, 95, 95, 255};
        if (g->verified[i] == 1) col = SDL_Color{90, 255, 130, 255};
        if (g->verified[i] == 0) col = SDL_Color{255, 80, 80, 255};
        const float s = std::clamp((node.folder() ? 5.0f : 3.5f) * pr[i].f, 2.0f, 9.0f);
        square(pr[i].x, pr[i].y, s, col);
        const float dx = pr[i].x - mx, dy = pr[i].y - my;
        if (mx >= area.x && dx * dx + dy * dy < best)
        {
            best = dx * dx + dy * dy;
            hot = int(i);
        }
    }
    auto mark = [&](int i, SDL_Color c, float s) {
        if (i < 0 || i >= int(n)) return;
        SDL_SetRenderDrawColor(r_, c.r, c.g, c.b, 255);
        const SDL_FRect q{pr[size_t(i)].x - s / 2, pr[size_t(i)].y - s / 2, s, s};
        SDL_RenderRect(r_, &q);
    };
    auto label = [&](int i, SDL_Color c) {
        if (i < 0 || i >= int(n)) return;
        text(pr[size_t(i)].x + 8, pr[size_t(i)].y - 4, g->map.label(size_t(i)), 1, c);
    };
    // Small maps are labelled throughout; larger ones only where you point and what you chose.
    if (n <= 24)
        for (size_t i = 0; i < n; ++i) label(int(i), SDL_Color{170, 170, 170, 255});
    if (focus >= 0)
    {
        square(pr[size_t(focus)].x, pr[size_t(focus)].y, 8, SDL_Color{255, 220, 60, 255});
        mark(focus, SDL_Color{255, 220, 60, 255}, 16);
        label(focus, SDL_Color{255, 220, 60, 255});
    }
    mark(graph_pick_, SDL_Color{255, 255, 255, 255}, 14);
    label(graph_pick_, SDL_Color{255, 255, 255, 255});
    mark(hot, SDL_Color{255, 255, 255, 255}, 12);
    label(hot, SDL_Color{255, 255, 255, 255});
    SDL_SetRenderClipRect(r_, nullptr);
    return hot;
}

// ---- the viewer

void Hallway::graph_event(const SDL_Event& e)
{
    if (e.type == SDL_EVENT_DROP_FILE && e.drop.data)
    {
        const std::string p = e.drop.data;
        if (fs::is_directory(from_u8(p))) graph_make(p);
        else graph_picked(2, p);
    }
    if (e.type == SDL_EVENT_MOUSE_MOTION)
    {
        float mx = e.motion.x, my = e.motion.y;
        SDL_RenderCoordinatesFromWindow(r_, e.motion.x, e.motion.y, &mx, &my);
        if (graph_drag_)
        {
            graph_yaw_ += (mx - graph_mx_) * 0.01f;
            graph_pitch_ = std::clamp(graph_pitch_ + (my - graph_my_) * 0.01f, -1.5f, 1.5f);
        }
        graph_mx_ = mx;
        graph_my_ = my;
    }
    if (e.type == SDL_EVENT_MOUSE_WHEEL) graph_zoom_ = std::clamp(graph_zoom_ * std::pow(1.15f, e.wheel.y), 0.2f, 20.0f);
    std::string pressed;
    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN && e.button.button == SDL_BUTTON_LEFT)
    {
        for (const auto& [r, id] : graph_buttons_)
            if (graph_mx_ >= r.x && graph_mx_ < r.x + r.w && graph_my_ >= r.y && graph_my_ < r.y + r.h) pressed = id;
        if (pressed.empty())
        {
            if (graph_drop_) graph_drop_ = false;
            else
            {
                graph_drag_ = true;
                graph_down_x_ = graph_mx_;
                graph_down_y_ = graph_my_;
                if (e.button.clicks == 2 && graph_hot_ >= 0) pressed = "go";
            }
        }
    }
    if (e.type == SDL_EVENT_MOUSE_BUTTON_UP && e.button.button == SDL_BUTTON_LEFT && graph_drag_)
    {
        graph_drag_ = false;
        // A click that did not turn the graph chooses the node under it.
        if (std::abs(graph_mx_ - graph_down_x_) + std::abs(graph_my_ - graph_down_y_) < 4)
        {
            graph_pick_ = graph_hot_;
            graph_go_armed_ = false;
        }
    }
    if (e.type == SDL_EVENT_KEY_DOWN)
        switch (e.key.key)
        {
        case SDLK_ESCAPE:
            if (graph_drop_) graph_drop_ = false;
            else close_graph();
            return;
        case SDLK_O: close_graph(); return;
        case SDLK_LEFT: graph_yaw_ -= 0.1f; break;
        case SDLK_RIGHT: graph_yaw_ += 0.1f; break;
        case SDLK_UP: graph_pitch_ = std::clamp(graph_pitch_ - 0.1f, -1.5f, 1.5f); break;
        case SDLK_DOWN: graph_pitch_ = std::clamp(graph_pitch_ + 0.1f, -1.5f, 1.5f); break;
        case SDLK_EQUALS: case SDLK_KP_PLUS: graph_zoom_ = std::min(20.0f, graph_zoom_ * 1.15f); break;
        case SDLK_MINUS: case SDLK_KP_MINUS: graph_zoom_ = std::max(0.2f, graph_zoom_ / 1.15f); break;
        case SDLK_LEFTBRACKET: graph_select(graph_sel_ - 1); break;
        case SDLK_RIGHTBRACKET: graph_select(graph_sel_ + 1); break;
        case SDLK_HOME: graph_yaw_ = 0.6f; graph_pitch_ = 0.3f; graph_zoom_ = 1.0f; break;
        case SDLK_RETURN: case SDLK_KP_ENTER: pressed = "go"; break;
        case SDLK_DELETE: pressed = "remove"; break;
        case SDLK_TAB:
            // The nodes one by one, for the keyboard (Shift: backwards).
            if (GraphMap* g = graph_current(); g && g->ready && !g->map.nodes.empty())
            {
                const int n = int(g->map.nodes.size());
                graph_pick_ = (e.key.mod & SDL_KMOD_SHIFT) ? (graph_pick_ <= 0 ? n - 1 : graph_pick_ - 1) : (graph_pick_ + 1) % n;
                graph_go_armed_ = false;
            }
            break;
        default: break;
        }
    if (pressed.empty()) return;
    static std::pair<Hallway*, int> make_ctx, open_ctx, export_ctx, create_ctx;
    make_ctx = {this, 1};
    open_ctx = {this, 2};
    export_ctx = {this, 3};
    create_ctx = {this, 4};
    if (pressed == "drop") graph_drop_ = !graph_drop_;
    else if (pressed.rfind("pick:", 0) == 0)
    {
        graph_select(std::stoi(pressed.substr(5)));
        graph_drop_ = false;
    }
    else if (pressed == "make") SDL_ShowOpenFolderDialog(graph_dialog, &make_ctx, window_, nullptr, false);
    else if (pressed == "open")
    {
        static const SDL_DialogFileFilter kMaps[] = {{"Sieve map", "map"}};
        const std::string start = u8(program_dir() / "maps");
        SDL_ShowOpenFileDialog(graph_dialog, &open_ctx, window_, kMaps, 1, start.c_str(), false);
    }
    else if (pressed == "export")
    {
        GraphMap* g = graph_current();
        if (!g || !g->ready) return;
        static const SDL_DialogFileFilter kOut[] = {{"GraphML (Gephi, yEd, Cytoscape)", "graphml"}, {"Graphviz DOT", "dot"}, {"Sieve map", "map"}};
        const std::string start = u8(fs::current_path() / from_u8(g->title + ".graphml"));
        SDL_ShowSaveFileDialog(graph_dialog, &export_ctx, window_, kOut, 3, start.c_str());
    }
    else if (pressed == "go") graph_go(graph_pick_ >= 0 ? graph_pick_ : graph_hot_);
    else if (pressed == "remove") graph_remove(graph_pick_);
    else if (pressed == "create")
    {
        static const SDL_DialogFileFilter kMap[] = {{"Sieve map", "map"}};
        std::error_code ec;
        fs::create_directories(program_dir() / "maps", ec);
        const std::string start = u8(program_dir() / "maps" / "new map.map");
        SDL_ShowSaveFileDialog(graph_dialog, &create_ctx, window_, kMap, 1, start.c_str());
    }
}

void Hallway::draw_graph_view(float W, float H)
{
    graph_poll();
    GraphMap* g = graph_current();
    const SDL_Color ink = theme().edge, grey{150, 150, 150, 255}, white{255, 255, 255, 255}, red{255, 80, 80, 255};
    SDL_SetRenderDrawColor(r_, 0, 0, 0, 255);
    const SDL_FRect all{0, 0, W, H};
    SDL_RenderFillRect(r_, &all);
    text(20, 16, tr("graph.title"), 3, ink);
    text(20, 48, fit(tr("graph.subtitle"), W - 40, 1), 1, grey);
    graph_buttons_.clear();
    float bx = 20;
    auto button = [&](const std::string& id, const std::string& label, float y, bool enabled = true) {
        const float w = text_width(label, 2) + 24;
        const SDL_FRect r{bx, y, w, 30};
        const bool hot = graph_mx_ >= r.x && graph_mx_ < r.x + r.w && graph_my_ >= r.y && graph_my_ < r.y + r.h;
        SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, hot && enabled ? 60 : 20);
        SDL_RenderFillRect(r_, &r);
        SDL_SetRenderDrawColor(r_, enabled ? ink.r : 90, enabled ? ink.g : 90, enabled ? ink.b : 90, 255);
        SDL_RenderRect(r_, &r);
        text(r.x + 12, r.y + 7, label, 2, enabled ? white : grey);
        if (enabled) graph_buttons_.emplace_back(r, id);
        bx += w + 12;
        return r;
    };
    const bool busy = graph_busy_;
    const SDL_FRect drop = button("drop", fit((g ? g->title : std::string("-")), 360, 2) + "  v", 72);
    button("create", tr("graph.create"), 72, !busy);
    button("make", tr("graph.make"), 72, !busy);
    button("open", tr("graph.open"), 72, !busy);
    button("export", tr("graph.export"), 72, g && g->ready);
    // The graph, and what is chosen in it.
    const float info_h = 118;
    const SDL_FRect area{20, 114, W - 40, std::max(60.0f, H - 114 - info_h - 40)};
    SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 40);
    SDL_RenderRect(r_, &area);
    if (g && g->ready)
    {
        graph_step(*g, 3000000);
        graph_hot_ = draw_graph(g, area, graph_yaw_, graph_pitch_, graph_zoom_, -1, false, graph_drag_ ? -1 : graph_mx_, graph_my_);
        size_t files = 0;
        for (const auto& n : g->map.nodes) files += n.folder() ? 0 : 1;
        text(area.x + 8, area.y + 6, trf("graph.counts", {std::to_string(g->map.nodes.size()), std::to_string(files),
                                                          std::to_string(g->map.edges.size())}) +
                                         (g->read_only() ? "   " + tr(g->installation ? "graph.made_here" : "graph.sealed") : ""),
             1, grey);
        if (g->steps < kLayoutSteps) text(area.x + 8, area.y + 20, tr("graph.settling"), 1, grey);
    }
    else
    {
        graph_hot_ = -1;
        text(area.x + 12, area.y + 12, g && !g->error.empty() ? fit(trf("graph.failed", {g->error}), area.w - 24, 1) : tr("graph.making_installation"),
             1, g && !g->error.empty() ? red : grey);
    }
    float y = area.y + area.h + 10;
    if (g && g->ready && graph_pick_ >= 0 && graph_pick_ < int(g->map.nodes.size()))
    {
        const cli::MapNode& n = g->map.nodes[size_t(graph_pick_)];
        auto row = [&](const std::string& label, const std::string& value, SDL_Color c) {
            text(20, y, label, 1, grey);
            text(150, y, fit(value, W - 170, 1), 1, c);
            y += 15;
        };
        row(tr("graph.node"), trf("graph.node.value", {std::to_string(graph_pick_), g->map.label(size_t(graph_pick_)),
                                                        tr(n.folder() ? "graph.folder" : n.kind == cli::MapNode::Held ? "graph.held" : "graph.file")}), white);
        row(tr("graph.path"), n.path, white);
        if (!n.folder())
        {
            row(tr("graph.size"), std::to_string(n.size), white);
            row(tr("graph.sha256"), n.sha256, white);
        }
        const int v = g->verified[size_t(graph_pick_)];
        row(tr("graph.status"), tr(v == 1 ? "graph.verified" : v == 0 ? "graph.changed_short" : g->present[size_t(graph_pick_)] ? "graph.present" : "graph.absent"),
            v == 1 ? SDL_Color{90, 255, 130, 255} : v == 0 ? red : g->present[size_t(graph_pick_)] ? ink : grey);
        if (!n.folder() && g->present[size_t(graph_pick_)])
        {
            bx = W - 20 - (text_width(tr("loc.go"), 2) + 24);
            button("go", tr("loc.go"), area.y + area.h + 10);
        }
        // Removing: any node but the root, from a map that may be changed.
        if (graph_pick_ > 0 && !g->read_only())
        {
            bx = W - 20 - (text_width(tr("graph.remove"), 2) + 24);
            button("remove", tr("graph.remove"), area.y + area.h + 50);
        }
    }
    else text(20, y, fit(tr("graph.hint"), W - 40, 1), 1, grey);
    std::string status;
    {
        std::lock_guard<std::mutex> lock(graph_lock_);
        status = graph_status_;
    }
    if (!status.empty()) text(20, H - 44, fit(status, W - 40, 1), 1, busy ? white : ink);
    text(20, H - 24, fit(tr("graph.keys"), W - 40, 1), 1, grey);
    // The dropdown, over everything.
    if (graph_drop_)
    {
        float ly = drop.y + drop.h + 2;
        const float lw = std::max(drop.w, 360.0f);
        for (size_t i = 0; i < graphs_.size(); ++i)
        {
            const SDL_FRect r{drop.x, ly, lw, 24};
            const bool hot = graph_mx_ >= r.x && graph_mx_ < r.x + r.w && graph_my_ >= r.y && graph_my_ < r.y + r.h;
            SDL_SetRenderDrawColor(r_, hot ? 40 : 14, hot ? 40 : 14, hot ? 40 : 14, 255);
            SDL_RenderFillRect(r_, &r);
            SDL_SetRenderDrawColor(r_, ink.r, ink.g, ink.b, 120);
            SDL_RenderRect(r_, &r);
            const GraphMap& m = graphs_[i];
            const std::string note = !m.error.empty() ? "  (" + tr("graph.unreadable") + ")"
                                     : m.ready ? "  (" + std::to_string(m.map.nodes.size()) + (m.read_only() ? ", " + tr("graph.read_only") : "") + ")"
                                               : "";
            text(r.x + 8, r.y + 5, fit(m.title + note, lw - 16, 1.5f), 1.5f, int(i) == graph_sel_ ? ink : white);
            graph_buttons_.emplace_back(r, "pick:" + std::to_string(i));
            ly += 24;
        }
    }
}

} // namespace hallway::hall
