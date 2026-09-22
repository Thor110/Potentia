#include "mesh.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <utility>

namespace hallway {

namespace fs = std::filesystem;

namespace {

std::map<std::string, SDL_FColor> load_mtl(const fs::path& path)
{
    std::map<std::string, SDL_FColor> out;
    std::ifstream in(path);
    std::string line, current;
    while (std::getline(in, line))
    {
        std::istringstream ss(line);
        std::string key;
        ss >> key;
        if (key == "newmtl") ss >> current;
        else if (key == "Kd" && !current.empty())
        {
            float r = 0.5f, g = 0.5f, b = 0.5f;
            ss >> r >> g >> b;
            out[current] = {std::clamp(r, 0.0f, 1.0f), std::clamp(g, 0.0f, 1.0f), std::clamp(b, 0.0f, 1.0f), 1.0f};
        }
    }
    return out;
}

// An .obj index: 1-based, or negative counting back from the end.
int resolve(int i, size_t n) { return i > 0 ? i - 1 : int(n) + i; }

} // namespace

std::shared_ptr<const Mesh> load_obj(const fs::path& path)
{
    std::ifstream in(path);
    if (!in) return nullptr;
    auto mesh = std::make_shared<Mesh>();
    mesh->source = path.string();
    std::vector<Vec3> v, vn;
    std::map<std::string, SDL_FColor> materials;
    SDL_FColor kd{0.6f, 0.6f, 0.6f, 1.0f};
    std::string line;
    while (std::getline(in, line))
    {
        std::istringstream ss(line);
        std::string key;
        ss >> key;
        if (key == "v")
        {
            Vec3 p;
            ss >> p.x >> p.y >> p.z;
            v.push_back(p);
        }
        else if (key == "vn")
        {
            Vec3 n;
            ss >> n.x >> n.y >> n.z;
            vn.push_back(n);
        }
        else if (key == "mtllib")
        {
            std::string name;
            std::getline(ss >> std::ws, name);
            while (!name.empty() && (name.back() == '\r' || name.back() == ' ')) name.pop_back();
            for (auto& [k, c] : load_mtl(path.parent_path() / name)) materials[k] = c;
        }
        else if (key == "usemtl")
        {
            std::string name;
            ss >> name;
            const auto it = materials.find(name);
            kd = it == materials.end() ? SDL_FColor{0.6f, 0.6f, 0.6f, 1.0f} : it->second;
        }
        else if (key == "f")
        {
            std::vector<int> pi, ni;
            std::string tok;
            while (ss >> tok)
            {
                // v, v/vt, v//vn or v/vt/vn
                int a = 0, c = 0;
                const size_t s1 = tok.find('/');
                try
                {
                    a = std::stoi(tok.substr(0, s1));
                    if (s1 != std::string::npos)
                        if (const size_t s2 = tok.find('/', s1 + 1); s2 != std::string::npos && s2 + 1 < tok.size()) c = std::stoi(tok.substr(s2 + 1));
                }
                catch (...)
                {
                    pi.clear();
                    break;
                }
                pi.push_back(resolve(a, v.size()));
                ni.push_back(c ? resolve(c, vn.size()) : -1);
            }
            bool ok = pi.size() >= 3;
            for (int i : pi) ok = ok && i >= 0 && size_t(i) < v.size();
            if (!ok) continue;
            for (size_t k = 1; k + 1 < pi.size(); ++k)
            {
                MeshTri t;
                t.p[0] = v[size_t(pi[0])];
                t.p[1] = v[size_t(pi[k])];
                t.p[2] = v[size_t(pi[k + 1])];
                // The face's own normal, turned to agree with the file's vertex normal if it has one.
                Vec3 n = cross(t.p[1] - t.p[0], t.p[2] - t.p[0]);
                if (dot(n, n) < 1e-12f) continue;
                n = normalize(n);
                if (ni[0] >= 0 && size_t(ni[0]) < vn.size() && dot(n, vn[size_t(ni[0])]) < 0) n = n * -1.0f;
                t.n = n;
                t.kd = kd;
                for (const Vec3& p : t.p)
                {
                    mesh->lo = {std::min(mesh->lo.x, p.x), std::min(mesh->lo.y, p.y), std::min(mesh->lo.z, p.z)};
                    mesh->hi = {std::max(mesh->hi.x, p.x), std::max(mesh->hi.y, p.y), std::max(mesh->hi.z, p.z)};
                }
                mesh->tris.push_back(t);
            }
        }
    }
    if (mesh->tris.empty()) return nullptr;
    return mesh;
}

fs::path mesh_folder()
{
    std::vector<fs::path> folders = {fs::path("data") / "meshes"};
    if (const fs::path exe = sieve::cli::executable_dir(); !exe.empty())
    {
        folders.push_back(exe / "meshes");
        folders.push_back(exe / ".." / "data" / "meshes");
    }
    for (const auto& f : folders)
    {
        std::error_code ec;
        if (fs::is_directory(f, ec)) return f;
    }
    return folders.front();
}

std::shared_ptr<const Mesh> load_model(const std::string& model, const std::string& medium, float max_edge)
{
    const fs::path folder = mesh_folder();
    auto m = load_obj(folder / (model + "-" + medium + ".obj"));
    if (!m) m = load_obj(folder / "templates" / (model + ".obj"));
    if (!m || max_edge <= 0) return m;
    // Split every triangle at the midpoint of its longest edge until no edge is longer than max_edge.
    auto split = std::make_shared<Mesh>(*m);
    split->tris.clear();
    std::vector<MeshTri> todo = m->tris;
    while (!todo.empty())
    {
        MeshTri t = todo.back();
        todo.pop_back();
        int longest = 0;
        float best = 0;
        for (int i = 0; i < 3; ++i)
        {
            const Vec3 e = t.p[(i + 1) % 3] - t.p[i];
            if (dot(e, e) > best) { best = dot(e, e); longest = i; }
        }
        if (best <= max_edge * max_edge || split->tris.size() + todo.size() > 200000)
        {
            split->tris.push_back(t);
            continue;
        }
        const Vec3 a = t.p[longest], b = t.p[(longest + 1) % 3], c = t.p[(longest + 2) % 3];
        const Vec3 mid = (a + b) * 0.5f;
        MeshTri t1 = t, t2 = t;
        t1.p[0] = a, t1.p[1] = mid, t1.p[2] = c;
        t2.p[0] = mid, t2.p[1] = b, t2.p[2] = c;
        todo.push_back(t1);
        todo.push_back(t2);
    }
    return split;
}

void MeshBatch::begin(const Camera& cam, SDL_Color background, int width, int height)
{
    cam_ = &cam;
    bg_ = {background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f};
    w_ = float(width);
    h_ = float(height);
    tris_.clear();
}

void MeshBatch::emit(const std::vector<Point2>& poly, float depth, SDL_FColor c)
{
    for (size_t k = 1; k + 1 < poly.size(); ++k)
    {
        Tri t;
        t.depth = depth;
        const Point2 q[3] = {poly[0], poly[k], poly[k + 1]};
        for (int i = 0; i < 3; ++i) t.v[i] = {{q[i].x, q[i].y}, c, {0, 0}};
        tris_.push_back(t);
    }
}

void MeshBatch::add(const Mesh& mesh, const Placement& at)
{
    const Camera& cam = *cam_;
    auto place = [&](Vec3 p) {
        p.y *= at.scale_y;
        if (at.mirror_x) p.x = -p.x;
        return p + at.offset;
    };
    std::vector<Point2> poly;
    for (const MeshTri& t : mesh.tris)
    {
        const Vec3 a = place(t.p[0]), b = place(t.p[1]), c = place(t.p[2]);
        Vec3 n = t.n;
        if (at.mirror_x) n.x = -n.x;
        if (at.scale_y != 1.0f) n = normalize(Vec3{n.x * at.scale_y, n.y, n.z * at.scale_y}); // normals scale inversely
        // Faces turned away from the camera cannot be seen.
        const Vec3 centre = (a + b + c) * (1.0f / 3.0f);
        const Vec3 to_eye = cam.pos - centre;
        if (dot(n, to_eye) <= 0) continue;
        const Vec3 ca = cam.to_camera(a), cb = cam.to_camera(b), cc = cam.to_camera(c);
        if (ca.z < cam.near_z && cb.z < cam.near_z && cc.z < cam.near_z) continue;
        // Shading: a light at the eye, plus some ambient; then fog into the background.
        const float dist = std::sqrt(dot(to_eye, to_eye));
        const float lambert = std::clamp(dot(n, to_eye * (1.0f / std::max(dist, 1e-4f))), 0.0f, 1.0f);
        const float shade = 0.35f + 0.65f * lambert;
        const float fog = std::clamp(std::max((dist - 10.0f) / 45.0f, at.dim), 0.0f, 1.0f);
        const SDL_FColor col{t.kd.r * shade * (1 - fog) + bg_.r * fog, t.kd.g * shade * (1 - fog) + bg_.g * fog,
                             t.kd.b * shade * (1 - fog) + bg_.b * fog, 1.0f};
        const float depth = (ca.z + cb.z + cc.z) / 3.0f;
        if (ca.z >= cam.near_z && cb.z >= cam.near_z && cc.z >= cam.near_z)
        {
            const Point2 pa = cam.project_camera(ca), pb = cam.project_camera(cb), pc = cam.project_camera(cc);
            auto near_view = [&](Point2 p) { return p.x > -w_ && p.x < 2 * w_ && p.y > -h_ && p.y < 2 * h_; };
            if (near_view(pa) && near_view(pb) && near_view(pc))
            {
                Tri tr;
                tr.depth = depth;
                tr.v[0] = {{pa.x, pa.y}, col, {0, 0}};
                tr.v[1] = {{pb.x, pb.y}, col, {0, 0}};
                tr.v[2] = {{pc.x, pc.y}, col, {0, 0}};
                tris_.push_back(tr);
                continue;
            }
        }
        // Crossing the near plane or far off screen: clip it (rare, so the allocations are fine).
        poly = cam.project_polygon({a, b, c});
        if (poly.size() >= 3) emit(poly, depth, col);
    }
}

void MeshBatch::draw(SDL_Renderer* r)
{
    std::sort(tris_.begin(), tris_.end(), [](const Tri& a, const Tri& b) { return a.depth > b.depth; });
    verts_.clear();
    verts_.reserve(tris_.size() * 3);
    for (const Tri& t : tris_) verts_.insert(verts_.end(), t.v, t.v + 3);
    if (!verts_.empty()) SDL_RenderGeometry(r, nullptr, verts_.data(), int(verts_.size()), nullptr, 0);
}

} // namespace hallway
