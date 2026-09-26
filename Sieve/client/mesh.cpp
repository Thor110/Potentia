#include "mesh.hpp"

#include "cli/dictionaries.hpp"

#include <algorithm>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <functional>
#include <map>
#include <mutex>
#include <sstream>
#include <thread>
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

std::shared_ptr<const Mesh> load_model(const std::string& model, const std::string& medium)
{
    const fs::path folder = mesh_folder();
    auto m = load_obj(folder / (model + "-" + medium + ".obj"));
    if (!m) m = load_obj(folder / "templates" / (model + ".obj"));
    return m;
}

std::shared_ptr<const Mesh> facing_x(const Mesh& mesh)
{
    auto out = std::make_shared<Mesh>(mesh);
    out->tris.clear();
    for (const MeshTri& t : mesh.tris)
        if (t.n.x > 0.3f || t.n.y > 0.7f) out->tris.push_back(t);
    if (out->tris.empty()) out->tris = mesh.tris;
    return out;
}

// One side of a corridor mesh removed: every triangle that lies in the wall at `sign * x` goes,
// leaving the floor, the ceiling and the other wall. Used for the binary line, which is an
// ordinary tile of corridor with one side missing (world.hpp). The test is the whole triangle
// standing out at that side, so a floor or ceiling panel that merely reaches the wall is kept.
std::shared_ptr<const Mesh> half_x(const Mesh& mesh, float sign, float at)
{
    auto out = std::make_shared<Mesh>(mesh);
    out->tris.clear();
    for (const MeshTri& t : mesh.tris)
    {
        const bool in_wall = t.p[0].x * sign >= at && t.p[1].x * sign >= at && t.p[2].x * sign >= at;
        if (!in_wall) out->tris.push_back(t);
    }
    if (out->tris.empty()) out->tris = mesh.tris;
    out->lo = {1e9f, 1e9f, 1e9f};
    out->hi = {-1e9f, -1e9f, -1e9f};
    for (const MeshTri& t : out->tris)
        for (const Vec3& v : t.p)
        {
            out->lo = {std::min(out->lo.x, v.x), std::min(out->lo.y, v.y), std::min(out->lo.z, v.z)};
            out->hi = {std::max(out->hi.x, v.x), std::max(out->hi.y, v.y), std::max(out->hi.z, v.z)};
        }
    return out;
}

// A box, as two triangles a face: the short wall on the binary line's open edge.
std::shared_ptr<const Mesh> box_mesh(Vec3 lo, Vec3 hi, SDL_FColor kd)
{
    auto out = std::make_shared<Mesh>();
    out->source = "(generated)";
    out->lo = lo;
    out->hi = hi;
    auto quad = [&](Vec3 a, Vec3 b, Vec3 c, Vec3 d, Vec3 n) {
        out->tris.push_back({{a, b, c}, n, kd});
        out->tris.push_back({{a, c, d}, n, kd});
    };
    const Vec3 l = lo, h = hi;
    quad({l.x, l.y, l.z}, {l.x, h.y, l.z}, {l.x, h.y, h.z}, {l.x, l.y, h.z}, {-1, 0, 0});
    quad({h.x, l.y, h.z}, {h.x, h.y, h.z}, {h.x, h.y, l.z}, {h.x, l.y, l.z}, {1, 0, 0});
    quad({l.x, h.y, l.z}, {h.x, h.y, l.z}, {h.x, h.y, h.z}, {l.x, h.y, h.z}, {0, 1, 0});
    quad({l.x, l.y, l.z}, {l.x, l.y, h.z}, {h.x, l.y, h.z}, {h.x, l.y, l.z}, {0, -1, 0});
    quad({l.x, l.y, l.z}, {h.x, l.y, l.z}, {h.x, h.y, l.z}, {l.x, h.y, l.z}, {0, 0, -1});
    quad({l.x, l.y, h.z}, {l.x, h.y, h.z}, {h.x, h.y, h.z}, {h.x, l.y, h.z}, {0, 0, 1});
    return out;
}

// A few worker threads, each drawing one band of the screen; the calling thread takes a band too.
struct MeshBatch::Pool
{
    explicit Pool(unsigned workers)
    {
        for (unsigned i = 0; i < workers; ++i) threads.emplace_back([this, i] { run(i + 1); });
    }
    ~Pool()
    {
        {
            std::lock_guard<std::mutex> lock(mu);
            stop = true;
        }
        wake.notify_all();
        for (auto& t : threads) t.join();
    }
    // Runs job(band) for bands 0 .. bands-1, band 0 on this thread; returns when all are done.
    void run_all(const std::function<void(unsigned)>& f)
    {
        {
            std::lock_guard<std::mutex> lock(mu);
            job = &f;
            ++generation;
            pending = unsigned(threads.size());
        }
        wake.notify_all();
        f(0);
        std::unique_lock<std::mutex> lock(mu);
        done.wait(lock, [this] { return pending == 0; });
        job = nullptr;
    }
    unsigned bands() const { return unsigned(threads.size()) + 1; }

private:
    void run(unsigned band)
    {
        uint64_t seen = 0;
        for (;;)
        {
            const std::function<void(unsigned)>* f = nullptr;
            {
                std::unique_lock<std::mutex> lock(mu);
                wake.wait(lock, [&] { return stop || generation != seen; });
                if (stop) return;
                seen = generation;
                f = job;
            }
            (*f)(band);
            std::lock_guard<std::mutex> lock(mu);
            if (--pending == 0) done.notify_one();
        }
    }
    std::vector<std::thread> threads;
    std::mutex mu;
    std::condition_variable wake, done;
    const std::function<void(unsigned)>* job = nullptr;
    uint64_t generation = 0;
    unsigned pending = 0;
    bool stop = false;
};

MeshBatch::MeshBatch()
{
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    pool_ = std::make_unique<Pool>(std::min(hw, 8u) - 1);
}

MeshBatch::~MeshBatch() { release(); }

void MeshBatch::release()
{
    if (texture_) SDL_DestroyTexture(texture_);
    texture_ = nullptr;
    texture_owner_ = nullptr;
    tex_w_ = tex_h_ = 0;
}

void MeshBatch::begin(const Camera& cam, SDL_Color background, int width, int height)
{
    cam_ = &cam;
    bg_ = {background.r / 255.0f, background.g / 255.0f, background.b / 255.0f, 1.0f};
    bg_argb_ = 0xFF000000u | uint32_t(background.r) << 16 | uint32_t(background.g) << 8 | uint32_t(background.b);
    w_ = std::max(1, width);
    h_ = std::max(1, height);
    tris_.clear();
}

void MeshBatch::push(const Vec3* c, uint32_t colour, float bias)
{
    Tri t;
    t.colour = colour;
    float iz[3];
    for (int i = 0; i < 3; ++i)
    {
        const Point2 p = cam_->project_camera(c[i]);
        t.x[i] = p.x;
        t.y[i] = p.y;
        iz[i] = (1.0f + bias) / c[i].z;
    }
    t.ymin = std::min({t.y[0], t.y[1], t.y[2]});
    t.ymax = std::max({t.y[0], t.y[1], t.y[2]});
    if (t.ymax < 0 || t.ymin > float(h_)) return;
    if (std::max({t.x[0], t.x[1], t.x[2]}) < 0 || std::min({t.x[0], t.x[1], t.x[2]}) > float(w_)) return;
    // The plane of 1/z, once per triangle (not once per band that draws it).
    const float det = (t.x[1] - t.x[0]) * (t.y[2] - t.y[0]) - (t.x[2] - t.x[0]) * (t.y[1] - t.y[0]);
    if (std::fabs(det) < 1e-6f) return; // edge-on: no area
    const float d1 = iz[1] - iz[0], d2 = iz[2] - iz[0];
    t.A = (d1 * (t.y[2] - t.y[0]) - d2 * (t.y[1] - t.y[0])) / det;
    t.B = (d2 * (t.x[1] - t.x[0]) - d1 * (t.x[2] - t.x[0])) / det;
    t.C = iz[0] - t.A * t.x[0] - t.B * t.y[0];
    tris_.push_back(t);
}

void MeshBatch::add(const Mesh& mesh, const Placement& at)
{
    const Camera& cam = *cam_;
    auto place = [&](Vec3 p) {
        p = p * at.scale;
        p.y *= at.scale_y;
        if (at.mirror_x) p.x = -p.x;
        return p + at.offset;
    };
    Vec3 in[4], out[4];
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
        in[0] = cam.to_camera(a);
        in[1] = cam.to_camera(b);
        in[2] = cam.to_camera(c);
        const float nz = cam.near_z;
        if (in[0].z < nz && in[1].z < nz && in[2].z < nz) continue;
        // Shading: a light from the eye, plus some ambient; then fog into the background.
        const float dist = std::sqrt(dot(to_eye, to_eye));
        // Lit along the view direction rather than towards each face's centre, so the two halves of
        // a flat quad come out exactly the same shade.
        const float lambert = std::clamp(-dot(n, cam.forward()), 0.0f, 1.0f);
        const float shade = 0.35f + 0.65f * lambert;
        const float fog = std::clamp(std::max((dist - 10.0f) / 45.0f, at.dim), 0.0f, 1.0f);
        auto channel = [&](float k, float bg) { return uint32_t(std::clamp(k * shade * (1 - fog) + bg * fog, 0.0f, 1.0f) * 255.0f + 0.5f); };
        const uint32_t colour = 0xFF000000u | channel(t.kd.r, bg_.r) << 16 | channel(t.kd.g, bg_.g) << 8 | channel(t.kd.b, bg_.b);
        if (in[0].z >= nz && in[1].z >= nz && in[2].z >= nz)
        {
            push(in, colour, at.depth_bias);
            continue;
        }
        // Crossing the near plane: clip to z >= near (a triangle becomes one or two).
        int m = 0;
        for (int i = 0; i < 3; ++i)
        {
            const Vec3 p = in[i], q = in[(i + 1) % 3];
            if (p.z >= nz) out[m++] = p;
            if ((p.z >= nz) != (q.z >= nz)) out[m++] = p + (q - p) * ((nz - p.z) / (q.z - p.z));
        }
        for (int i = 1; i + 1 < m; ++i)
        {
            const Vec3 tri[3] = {out[0], out[i], out[i + 1]};
            push(tri, colour, at.depth_bias);
        }
    }
}

// Rows y0 .. y1-1: clear them, then fill every triangle that reaches them. A pixel is covered
// when its centre is inside the triangle (so neighbouring triangles never both draw an edge
// pixel, and leave no gaps); 1/z is planar in screen space, so it is interpolated exactly.
void MeshBatch::raster_band(int y0, int y1)
{
    const int W = w_, P = pitch_;
    std::fill(pixels_ + std::ptrdiff_t(y0) * P, pixels_ + std::ptrdiff_t(y1) * P, bg_argb_);
    std::fill(depth_.begin() + std::ptrdiff_t(y0) * W, depth_.begin() + std::ptrdiff_t(y1) * W, 0.0f);
    for (const Tri& t : tris_)
    {
        if (t.ymax < float(y0) || t.ymin > float(y1)) continue;
        const float A = t.A, B = t.B, C = t.C;
        const int ya = std::max(y0, int(std::ceil(t.ymin - 0.5f))), yb = std::min(y1 - 1, int(std::ceil(t.ymax - 0.5f)) - 1);
        for (int y = ya; y <= yb; ++y)
        {
            const float yc = float(y) + 0.5f;
            // Where the row crosses the triangle's edges.
            float xl = 1e30f, xr = -1e30f;
            for (int e = 0; e < 3; ++e)
            {
                float ax = t.x[e], ay = t.y[e], bx = t.x[(e + 1) % 3], by = t.y[(e + 1) % 3];
                if ((ay <= yc) == (by <= yc)) continue; // this edge does not cross the row
                // The same edge is shared by the neighbouring triangle, walked the other way:
                // order its ends so both compute the very same crossing (no cracks).
                if (ay > by || (ay == by && ax > bx))
                {
                    std::swap(ax, bx);
                    std::swap(ay, by);
                }
                const float x = ax + (bx - ax) * (yc - ay) / (by - ay);
                xl = std::min(xl, x);
                xr = std::max(xr, x);
            }
            if (xl > xr) continue;
            const int xa = std::max(0, int(std::ceil(xl - 0.5f))), xb = std::min(W - 1, int(std::ceil(xr - 0.5f)) - 1);
            if (xa > xb) continue;
            const float row = B * yc + C + A * 0.5f;
            uint32_t* cp = pixels_ + size_t(y) * size_t(P);
            float* dp = depth_.data() + size_t(y) * size_t(W);
            for (int x = xa; x <= xb; ++x)
            {
                const float iz = A * float(x) + row; // evaluated, not accumulated: no drift on long spans
                if (iz > dp[x])
                {
                    dp[x] = iz;
                    cp[x] = t.colour;
                }
            }
        }
    }
}

void MeshBatch::draw(SDL_Renderer* r)
{
    const size_t n = size_t(w_) * size_t(h_);
    if (depth_.size() != n) depth_.assign(n, 0.0f);
    if (texture_ && (texture_owner_ != r || tex_w_ != w_ || tex_h_ != h_)) release();
    if (!texture_)
    {
        texture_ = SDL_CreateTexture(r, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, w_, h_);
        if (!texture_) return;
        SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
        SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
        texture_owner_ = r;
        tex_w_ = w_;
        tex_h_ = h_;
    }
    // Into our own image, then one upload: writing into the texture's memory (SDL_LockTexture)
    // measured several times slower with the software renderer, which is the slowest case anyway.
    if (colour_.size() != n) colour_.assign(n, 0);
    pixels_ = colour_.data();
    pitch_ = w_;
    // Bands of rows, one per thread.
    const unsigned bands = pool_->bands();
    pool_->run_all([&](unsigned b) {
        const int y0 = int(int64_t(h_) * b / bands), y1 = int(int64_t(h_) * (b + 1) / bands);
        if (y1 > y0) raster_band(y0, y1);
    });
    drawn_ = tris_.size();
    SDL_UpdateTexture(texture_, nullptr, colour_.data(), w_ * 4);
    pixels_ = nullptr;
    SDL_RenderTexture(r, texture_, nullptr, nullptr);
}

} // namespace hallway
