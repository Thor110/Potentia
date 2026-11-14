// Sieve hallway — how characters are drawn (character_look.hpp; IDEAS 19.214).
#include "character_look.hpp"

#include "app_settings.hpp"

#include "sieve/character_voxels.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <memory>
#include <utility>
#include <string>
#include <vector>

namespace hallway {

const char* character_drawing_name(int drawing)
{
    switch (drawing)
    {
    case kDrawPixels: return "pixels";
    case kDrawWireframe: return "wireframe";
    case kDrawVoxels: return "voxels";
    case kDrawSkin: return "skin";
    case kDrawQuads: return "quads";
    case kDrawSmooth: return "smooth";
    default: return "shaded";
    }
}

VoxelChoice voxel_choice_of(const AppSettings& app)
{
    VoxelChoice v;
    v.sizing.rule = app.character_voxel_size == 1 ? sieve::VoxelSizing::Lattice : sieve::VoxelSizing::Height;
    v.sizing.base_mm = uint32_t(std::clamp(app.character_voxel_base_mm, 1, 100));
    v.sizing.across = uint32_t(std::clamp(app.character_voxel_across, 1, 8));
    v.joined = app.character_voxel_joined;
    v.skin_blend = uint32_t(std::clamp(app.character_skin_blend, 0, 200));
    v.skin_detail = uint32_t(std::clamp(app.character_skin_detail, 0, 3));
    return v;
}

std::string character_drawing_report(int chosen, bool real_graphics, size_t triangles, const VoxelChoice& voxels, size_t unjoined)
{
    if (chosen == kDrawDefault) return std::string(); // (as the space drew before the choice came to it)
    std::string s = std::string("drawn as ") + character_drawing_name(chosen);
    if (!real_graphics) return std::string("drawn as wireframe (Real Graphics is off; ") + character_drawing_name(chosen) + " chosen)";
    if (chosen == kDrawVoxels)
    {
        s += ", " + std::to_string(triangles) + " triangles of cubes";
        // (said only where chosen, so a readout of today's cubes is as it was)
        if (voxels.joined) s += " (" + std::to_string(unjoined) + " before joining)";
        if (voxels.anchored()) s += ", cells on the lattice";
    }
    else if (chosen == kDrawSkin) s += ", " + std::to_string(triangles) + " triangles of skin";
    else if (chosen == kDrawQuads) s += ", " + std::to_string(triangles) + " quads of skin";
    else if (chosen == kDrawSmooth) s += ", " + std::to_string(triangles) + " triangles of smooth skin";
    return s;
}

std::string SkinReport::line(const VoxelChoice& voxels, int drawing) const
{
    char buf[160];
    std::snprintf(buf, sizeof buf, "cell %.1f units (the voxels' halved %u times%s), blend %.1f units (%u%%), grid %ux%ux%u, ", double(cell), unsigned(voxels.skin_detail),
                  coarser ? ", made coarser to fit" : "", double(blend), unsigned(voxels.skin_blend), unsigned(grid[0]), unsigned(grid[1]), unsigned(grid[2]));
    char ms_text[32];
    std::snprintf(ms_text, sizeof ms_text, "%.2f", ms);
    const char* what = drawing == kDrawQuads ? " quads made, " : " triangles made, ";
    std::string s = std::string(buf) + std::to_string(made) + what + std::to_string(drawn) + " drawn, made in " + ms_text + " ms" + (kept ? " (kept from a frame before)" : "");
    if (drawing == kDrawSmooth)
    {
        char smooth_text[64];
        std::snprintf(smooth_text, sizeof smooth_text, ", normals averaged in %.2f ms", smooth_ms);
        s += smooth_text;
    }
    return s;
}

// How many skins make_skin keeps: twice the most characters a space has said it draws in a frame, and
// never fewer than eight. Twice, because a frame's new skins (its characters that moved) come in before
// the last frame's are let go: with the room for both, a character standing still is always found from
// the frame before, however many are in sight.
static size_t skins_kept = 8;

void skins_kept_for(size_t characters_in_a_frame)
{
    skins_kept = std::max(skins_kept, std::max<size_t>(8, characters_in_a_frame * 2));
}

std::shared_ptr<const sieve::SkinMesh> make_skin(const std::vector<sieve::Sphere>& spheres, float voxel_cell, const VoxelChoice& voxels, SkinReport* report)
{
    const auto t0 = std::chrono::steady_clock::now();
    const sieve::SkinCell sc = sieve::skin_cell(spheres, voxel_cell, voxels.skin_detail);
    const float blend = sieve::skin_blend(spheres, voxels.skin_blend);
    // What it is made from, as a key: each sphere's centre and radius, the cell and the blend.
    std::vector<int32_t> key;
    key.reserve(spheres.size() * 4 + 2);
    for (const sieve::Sphere& p : spheres) key.insert(key.end(), {p.x, p.y, p.z, p.r});
    int32_t cb[2];
    std::memcpy(&cb[0], &sc.cell, sizeof(float));
    std::memcpy(&cb[1], &blend, sizeof(float));
    key.insert(key.end(), cb, cb + 2);
    // The last few made, the newest last (drawn on the main thread only). As many are kept as
    // skins_kept_for asks, so a space never draws more characters in a frame than are kept.
    struct Kept
    {
        std::vector<int32_t> key;
        std::shared_ptr<const sieve::SkinMesh> skin;
        double ms = 0;
    };
    static std::vector<Kept> kept;
    const size_t kKeep = skins_kept;
    for (size_t i = 0; i < kept.size(); ++i)
        if (kept[i].key == key)
        {
            Kept k = std::move(kept[i]);
            kept.erase(kept.begin() + std::ptrdiff_t(i));
            kept.push_back(std::move(k));
            if (report)
            {
                report->cell = sc.cell, report->blend = blend, report->coarser = sc.coarser, report->kept = true;
                for (int a = 0; a < 3; ++a) report->grid[a] = kept.back().skin->grid[a];
                report->made = kept.back().skin->triangles();
                report->ms = kept.back().ms;
            }
            return kept.back().skin;
        }
    auto skin = std::make_shared<const sieve::SkinMesh>(sieve::skin_mesh(spheres, sc.cell, blend));
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    while (kept.size() >= kKeep) kept.erase(kept.begin());
    kept.push_back(Kept{std::move(key), skin, ms});
    if (report)
    {
        report->cell = sc.cell, report->blend = blend, report->coarser = sc.coarser, report->kept = false;
        for (int a = 0; a < 3; ++a) report->grid[a] = skin->grid[a];
        report->made = skin->triangles();
        report->ms = ms;
    }
    return skin;
}

void skin_triangle_look(const sieve::SkinMesh& skin, size_t triangle, float normal[3], uint32_t& sphere)
{
    const sieve::SkinVertex& a = skin.vertices[skin.tris[triangle * 3]];
    const sieve::SkinVertex& b = skin.vertices[skin.tris[triangle * 3 + 1]];
    const sieve::SkinVertex& c = skin.vertices[skin.tris[triangle * 3 + 2]];
    float l = 0;
    for (int k = 0; k < 3; ++k) normal[k] = a.n[k] + b.n[k] + c.n[k], l += normal[k] * normal[k];
    l = std::sqrt(l);
    for (int k = 0; k < 3; ++k) normal[k] = l > 0 ? normal[k] / l : 0;
    sphere = b.sphere == c.sphere ? b.sphere : a.sphere;
}

void skin_quad_look(const sieve::SkinMesh& skin, size_t quad, float normal[3], uint32_t& sphere)
{
    const sieve::SkinVertex* v[4];
    for (int k = 0; k < 4; ++k) v[k] = &skin.vertices[skin.quads[quad * 4 + size_t(k)]];
    float l = 0;
    for (int k = 0; k < 3; ++k) normal[k] = v[0]->n[k] + v[1]->n[k] + v[2]->n[k] + v[3]->n[k], l += normal[k] * normal[k];
    l = std::sqrt(l);
    for (int k = 0; k < 3; ++k) normal[k] = l > 0 ? normal[k] / l : 0;
    // (the sphere most corners share; the first corner's wins a tie, so a face split evenly between two
    // spheres takes the same one every time)
    int best = 0;
    sphere = v[0]->sphere;
    for (int i = 0; i < 4; ++i)
    {
        int count = 0;
        for (int j = 0; j < 4; ++j) count += v[j]->sphere == v[i]->sphere;
        if (count > best) best = count, sphere = v[i]->sphere;
    }
}

std::string voxel_cell_report(const sieve::Character& at_rest, const VoxelChoice& voxels)
{
    const sieve::VoxelCell vc = sieve::voxel_cell_of(at_rest, voxels.sizing);
    char units[32];
    std::snprintf(units, sizeof units, "%.1f", double(vc.cell));
    if (!voxels.anchored()) return std::string(units) + (vc.coarser ? " units (its widest side in 256 cells: made coarser to fit)" : " units (its height in 16 cells)");
    const uint64_t mm = uint64_t(std::clamp<uint32_t>(voxels.sizing.base_mm, 1, 100)) << vc.level;
    return std::string(units) + " units (the lattice: " + std::to_string(mm) + " mm, level " + std::to_string(vc.level) + (vc.coarser ? ", made coarser to fit" : "") + ")";
}

// A colour's red, green and blue scaled by `light` (its alpha kept opaque).
static uint32_t scaled(uint32_t argb, float light)
{
    auto c = [&](uint32_t v) { return uint32_t(std::clamp(float(v & 0xFFu) * light, 0.0f, 255.0f) + 0.5f); };
    return 0xFF000000u | c(argb >> 16) << 16 | c(argb >> 8) << 8 | c(argb);
}

size_t add_skin_drawing(MeshBatch& batch, int drawing, const sieve::SkinMesh& skin, const SkinPlacing& at, double* smooth_ms)
{
    // Every vertex placed once, as the space takes it.
    std::vector<Vec3> w(skin.vertices.size());
    for (size_t i = 0; i < w.size(); ++i) w[i] = at.point(skin.vertices[i].p[0], skin.vertices[i].p[1], skin.vertices[i].p[2]);
    auto face = [&](const Vec3 q[3], uint32_t argb) {
        if (at.world) batch.add_face(q, argb);
        else batch.add_face_cam(q, argb);
    };
    size_t given = 0;
    if (drawing == kDrawQuads)
    {
        // The quads: each face of four corners as surface nets made it, turned towards the eye given as its
        // two triangles in one colour, and its four sides drawn over it darker (as the voxels' cubes are),
        // so the quads read as quads and not as the triangles they are filled with. A side two seen faces
        // share is given by both (the edges are drawn where nothing nearer is, so twice is the same).
        for (size_t f = 0; f < skin.quad_count(); ++f)
        {
            Vec3 q[4];
            for (int k = 0; k < 4; ++k) q[k] = w[skin.quads[f * 4 + size_t(k)]];
            float nn[3];
            uint32_t sphere = 0;
            skin_quad_look(skin, f, nn, sphere);
            const Vec3 n = at.direction(nn[0], nn[1], nn[2]);
            // (across the face: its diagonals' cross product, which a face not quite flat still has)
            Vec3 g = cross(q[2] - q[0], q[3] - q[1]);
            if (dot(g, n) < 0) g = g * -1.0f;
            const Vec3 centre = (q[0] + q[1] + q[2] + q[3]) * 0.25f;
            if (at.turned_away(g, centre)) continue;
            const uint32_t fc = at.colour(sphere, skin_lambert(at.cam_normal(centre, n)), q, 4), ec = scaled(fc, 0.55f);
            const Vec3 t1[3] = {q[0], q[1], q[2]}, t2[3] = {q[0], q[2], q[3]};
            face(t1, fc);
            face(t2, fc);
            for (int k = 0; k < 4; ++k)
            {
                if (at.world) batch.add_edge(q[k], q[(k + 1) % 4], ec);
                else batch.add_edge_cam(q[k], q[(k + 1) % 4], ec);
            }
            ++given;
        }
        return given;
    }
    // The smooth drawing's normals: averaged across the faces round each vertex, once a frame (they are
    // a small part of the frame's time; the readout says how small).
    std::vector<float> smooth;
    if (drawing == kDrawSmooth)
    {
        const auto t0 = std::chrono::steady_clock::now();
        smooth = sieve::skin_smooth_normals(skin);
        if (smooth_ms) *smooth_ms += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    }
    // The skin: each triangle turned towards the eye given (which way it faces taken from its corners, its
    // normal only saying which side is out, so a space whose placing mirrors the character still sees the
    // outside), lit in camera space as MeshBatch lights the shaded spheres: one colour a face, or for the
    // smooth drawing one a corner, each lit by its averaged normal and coloured as its own sphere, the
    // colours running between them across the face.
    for (size_t t = 0; t < skin.triangles(); ++t)
    {
        const Vec3 q[3] = {w[skin.tris[t * 3]], w[skin.tris[t * 3 + 1]], w[skin.tris[t * 3 + 2]]};
        float nn[3];
        uint32_t sphere = 0;
        skin_triangle_look(skin, t, nn, sphere);
        const Vec3 n = at.direction(nn[0], nn[1], nn[2]);
        Vec3 g = cross(q[1] - q[0], q[2] - q[0]);
        if (dot(g, n) < 0) g = g * -1.0f;
        const Vec3 centre = (q[0] + q[1] + q[2]) * (1.0f / 3.0f);
        if (at.turned_away(g, centre)) continue;
        if (drawing == kDrawSmooth)
        {
            uint32_t corner[3];
            for (int k = 0; k < 3; ++k)
            {
                const uint32_t v = skin.tris[t * 3 + size_t(k)];
                const float* m = &smooth[size_t(v) * 3];
                corner[k] = at.colour(skin.vertices[v].sphere, skin_lambert(at.cam_normal(q[k], at.direction(m[0], m[1], m[2]))), &q[k], 1);
            }
            if (at.world) batch.add_face_shaded(q, corner);
            else batch.add_face_shaded_cam(q, corner);
        }
        else face(q, at.colour(sphere, skin_lambert(at.cam_normal(centre, n)), q, 3));
        ++given;
    }
    return given;
}

size_t add_character_drawing(MeshBatch& batch, const Camera& cam, int drawing, const std::vector<sieve::Sphere>& spheres, float cell,
                             const CharacterPlacing& at, const std::function<MeshBatch::SphereLook(const sieve::Sphere&)>& look_of,
                             const VoxelChoice& voxels, size_t* unjoined)
{
    if (spheres.empty()) return 0;
    if (skin_drawing(drawing))
    {
        // The skin (IDEAS 19.225) and its quads and smooth drawing (IDEAS 19.214): placed and turned as the
        // character is, lit in camera space with the look's ambient light, coloured as each sphere's look.
        const std::shared_ptr<const sieve::SkinMesh> kept = make_skin(spheres, cell, voxels);
        SkinPlacing place;
        place.world = true;
        place.point = at.point;
        place.direction = [&](float x, float y, float z) { return normalize(at.direction(x, y, z)); };
        place.turned_away = [&](Vec3 g, Vec3 centre) { return dot(g, centre - cam.pos) >= 0; };
        place.cam_normal = [&](Vec3 p, Vec3 n) { return cam.to_camera(p + n) - cam.to_camera(p); };
        place.colour = [&](uint32_t sphere, float lambert, const Vec3*, int) {
            const MeshBatch::SphereLook look = look_of(spheres[sphere]);
            return scaled(look.argb, look.ambient + (1.0f - look.ambient) * lambert);
        };
        return add_skin_drawing(batch, drawing, *kept, place);
    }
    if (drawing == kDrawVoxels)
    {
        // The cubes, every space's the same (sieve::voxel_faces), placed and turned as the character is;
        // only the faces turned towards the eye given, each lit by the way it faces (from the upper left,
        // as the Creator lights them; the sides away from the light still read), its edges darker.
        const Vec3 light = normalize(Vec3{-0.45f, 0.55f, -0.70f});
        size_t tris = 0;
        // Joined (greedy meshing, IDEAS 19.215): the faces seen kept with their colours, then given as the
        // fewest rectangles of one colour that cover them; each cell's edges are still drawn as before, so
        // the cubes read as cubes and the picture is the same.
        std::vector<sieve::VoxelFace> seen;
        std::vector<uint32_t> seen_colour;
        for (const sieve::VoxelFace& f : sieve::voxel_faces(spheres, cell, voxels.anchored()))
        {
            const int* dv = sieve::kVoxelDirs[f.dir];
            const Vec3 n = at.direction(float(dv[0]), float(dv[1]), float(dv[2]));
            const Vec3 mid = at.point(f.mid[0], f.mid[1], f.mid[2]);
            if (dot(n, mid - cam.pos) >= 0) continue; // (turned away)
            Vec3 q[4];
            for (int k = 0; k < 4; ++k) q[k] = at.point(f.corner[k][0], f.corner[k][1], f.corner[k][2]);
            const uint32_t base = look_of(spheres[f.sphere]).argb;
            const float lit = 0.6f + 0.4f * std::max(0.0f, dot(n, light));
            const uint32_t fc = scaled(base, lit), ec = scaled(base, lit * 0.55f);
            if (unjoined) *unjoined += 2;
            if (voxels.joined)
            {
                seen.push_back(f);
                seen_colour.push_back(fc);
            }
            else
            {
                const Vec3 t1[3] = {q[0], q[1], q[2]}, t2[3] = {q[0], q[2], q[3]};
                batch.add_face(t1, fc);
                batch.add_face(t2, fc);
                tris += 2;
            }
            for (int k = 0; k < 4; ++k) batch.add_edge(q[k], q[(k + 1) % 4], ec);
        }
        for (const sieve::VoxelQuad& r : sieve::join_voxel_faces(seen, seen_colour))
        {
            Vec3 q[4];
            for (int k = 0; k < 4; ++k) q[k] = at.point(r.corner[k][0], r.corner[k][1], r.corner[k][2]);
            const Vec3 t1[3] = {q[0], q[1], q[2]}, t2[3] = {q[0], q[2], q[3]};
            batch.add_face(t1, seen_colour[r.face]);
            batch.add_face(t2, seen_colour[r.face]);
            tris += 2;
        }
        return tris;
    }
    // The pixels' squares: one size for the whole character (its cell as seen at its middle's depth), on
    // a grid fixed to the screen whose origin is that middle, as the Creator lays them.
    float cell_px = 2, gx = 0, gy = 0;
    if (drawing == kDrawPixels)
    {
        float lo[3] = {1e30f, 1e30f, 1e30f}, hi[3] = {-1e30f, -1e30f, -1e30f};
        for (const sieve::Sphere& p : spheres)
        {
            const float c[3] = {float(p.x), float(p.y), float(p.z)};
            for (int k = 0; k < 3; ++k) lo[k] = std::min(lo[k], c[k]), hi[k] = std::max(hi[k], c[k]);
        }
        const Vec3 c = cam.to_camera(at.point(0.5f * (lo[0] + hi[0]), 0.5f * (lo[1] + hi[1]), 0.5f * (lo[2] + hi[2])));
        if (c.z > cam.near_z)
        {
            cell_px = std::max(2.0f, cell * at.metres * cam.focal() / c.z);
            const Point2 g = cam.project_camera(c);
            gx = g.x, gy = g.y;
        }
    }
    for (const sieve::Sphere& p : spheres)
    {
        MeshBatch::SphereLook look = look_of(p);
        if (drawing == kDrawWireframe) look.style = MeshBatch::SphereLook::Style::Ring;
        else if (drawing == kDrawPixels)
        {
            look.style = MeshBatch::SphereLook::Style::Voxels;
            look.cell = cell_px, look.grid_x = gx, look.grid_y = gy;
        }
        batch.add_sphere(at.point(float(p.x), float(p.y), float(p.z)), float(p.r) * at.metres, look);
    }
    return 0;
}

} // namespace hallway
